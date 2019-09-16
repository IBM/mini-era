/*
 * Copyright 1995 Phil Karn, KA9Q
 * Copyright 2008 Free Software Foundation, Inc.
 * 2014 Added SSE2 implementation Bogdan Diaconescu
 *
 * This file is part of GNU Radio
 *
 * GNU Radio is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * GNU Radio is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Radio; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

/*
 * Viterbi decoder for K=7 rate=1/2 convolutional code
 * Some modifications from original Karn code by Matt Ettus
 * Major modifications by adding SSE2 code by Bogdan Diaconescu
 */
#include <stdio.h>
#include <stdint.h>
#include "base.h"
#include "viterbi_decoder_generic.h"
#include "viterbi_standalone.h"

// GLOBAL VARIABLES

// Position in circular buffer where the current decoded byte is stored
static int d_store_pos = 0;
// Metrics for each state
//static unsigned char d_mmresult[64] __attribute__((aligned(16)));
// Paths for each state
//static unsigned char d_ppresult[TRACEBACK_MAX][64] __attribute__((aligned(16)));



uint8_t* depuncture(ofdm_param *ofdm,   size_t ofdm_size,
		    frame_param *frame, size_t frame_size,
		    int ntraceback,
		    int k_val,
		    unsigned char *d_depuncture_pattern,  size_t depunc_ptn_size,
		    uint8_t *in,        size_t in_size)
{
  int count;
  int n_cbps = ofdm->n_cbps;
  uint8_t *depunctured;
  //printf("Depunture call...\n");
  if (ntraceback == 5) {
    count = frame->n_sym * n_cbps;
    depunctured = in;
  } else {
    depunctured = d_depunctured;
    count = 0;
    for(int i = 0; i < frame->n_sym; i++) {
      for(int k = 0; k < n_cbps; k++) {
	while (d_depuncture_pattern[count % (2 * k_val)] == 0) {
	  depunctured[count] = 2;
	  count++;
	}

	// Insert received bits
	depunctured[count] = in[i * n_cbps + k];
	count++;

	while (d_depuncture_pattern[count % (2 * k_val)] == 0) {
	  depunctured[count] = 2;
	  count++;
	}
      }
    }
  }
  //printf("  depuncture count = %u\n", count);
  return depunctured;
}

/* The basic Viterbi decoder operation, called a "butterfly"
 * operation because of the way it looks on a trellis diagram. Each
 * butterfly involves an Add-Compare-Select (ACS) operation on the two nodes
 * where the 0 and 1 paths from the current node merge at the next step of
 * the trellis.
 *
 * The code polynomials are assumed to have 1's on both ends. Given a
 * function encode_state() that returns the two symbols for a given
 * encoder state in the low two bits, such a code will have the following
 * identities for even 'n' < 64:
 *
 * 	encode_state(n) = encode_state(n+65)
 *	encode_state(n+1) = encode_state(n+64) = (3 ^ encode_state(n))
 *
 * Any convolutional code you would actually want to use will have
 * these properties, so these assumptions aren't too limiting.
 *
 * Doing this as a macro lets the compiler evaluate at compile time the
 * many expressions that depend on the loop index and encoder state and
 * emit them as immediate arguments.
 * This makes an enormous difference on register-starved machines such
 * as the Intel x86 family where evaluating these expressions at runtime
 * would spill over into memory.
 */

// INPUTS/OUTPUTS:  All are 64-entry (bytes) arrays randomly accessed.
//    symbols : INPUT        : Array [  4 bytes ] 
//    mm0     : INPUT/OUTPUT : Array [ 64 bytes ]
//    mm1     : INPUT/OUTPUT : Array [ 64 bytes ]
//    pp0     : INPUT/OUTPUT : Array [ 64 bytes ] 
//    pp1     : INPUT/OUTPUT : Array [ 64 bytes ]
// d_branchtab27_generic[1].c[] : INPUT : Array [2].c[32] {GLOBAL}
//

void viterbi_butterfly2_generic(unsigned char *symbols,
				unsigned char *d_brtab27[2],
				unsigned char *mm0, unsigned char *mm1, unsigned char *pp0,
				unsigned char *pp1)
{
  //unsigned char *d_brtab27[2] = {&(d_branchtab27_generic[0].c[0]), &(d_branchtab27_generic[1].c[0])};

  // These are used to "virtually" rename the uses below (for symmetry; reduces code size)
  //  Really these are functionally "offset pointers" into the above arrays....
  unsigned char *metric0, *metric1;
  unsigned char *path0, *path1;

  // Operate on 4 symbols (2 bits) at a time

  unsigned char m0[16], m1[16], m2[16], m3[16], decision0[16], decision1[16], survivor0[16], survivor1[16];
  unsigned char metsv[16], metsvm[16];
  unsigned char shift0[16], shift1[16];
  unsigned char tmp0[16], tmp1[16];
  unsigned char sym0v[16], sym1v[16];
  unsigned short simd_epi16;
  unsigned int   first_symbol;
  unsigned int   second_symbol;

  // Set up for the first two symbols (0 and 1)
  metric0 = mm0;
  path0 = pp0;
  metric1 = mm1;
  path1 = pp1;
  first_symbol = 0;
  second_symbol = first_symbol+1;
  for (int j = 0; j < 16; j++) {
    sym0v[j] = symbols[first_symbol];
    sym1v[j] = symbols[second_symbol];
  }

  for (int s = 0; s < 2; s++) { // iterate across the 2 symbol groups
    // This is the basic viterbi butterfly for 2 symbols (we need therefore 2 passes for 4 total symbols)
    for (int i = 0; i < 2; i++) {
      if (symbols[first_symbol] == 2) {
	for (int j = 0; j < 16; j++) {
          //metsvm[j] = d_branchtab27_generic[1].c[(i*16) + j] ^ sym1v[j];
	  metsvm[j] = d_brtab27[1][(i*16) + j] ^ sym1v[j];
	  metsv[j] = 1 - metsvm[j];
	}
      }
      else if (symbols[second_symbol] == 2) {
	for (int j = 0; j < 16; j++) {
          //metsvm[j] = d_branchtab27_generic[0].c[(i*16) + j] ^ sym0v[j];
	  metsvm[j] = d_brtab27[0][(i*16) + j] ^ sym0v[j];
	  metsv[j] = 1 - metsvm[j];
	}
      }
      else {
	for (int j = 0; j < 16; j++) {
          //metsvm[j] = (d_branchtab27_generic[0].c[(i*16) + j] ^ sym0v[j]) + (d_branchtab27_generic[1].c[(i*16) + j] ^ sym1v[j]);
	  metsvm[j] = (d_brtab27[0][(i*16) + j] ^ sym0v[j]) + (d_brtab27[1][(i*16) + j] ^ sym1v[j]);
	  metsv[j] = 2 - metsvm[j];
	}
      }

      for (int j = 0; j < 16; j++) {
	m0[j] = metric0[(i*16) + j] + metsv[j];
	m1[j] = metric0[((i+2)*16) + j] + metsvm[j];
	m2[j] = metric0[(i*16) + j] + metsvm[j];
	m3[j] = metric0[((i+2)*16) + j] + metsv[j];
      }

      for (int j = 0; j < 16; j++) {
	decision0[j] = ((m0[j] - m1[j]) > 0) ? 0xff : 0x0;
	decision1[j] = ((m2[j] - m3[j]) > 0) ? 0xff : 0x0;
	survivor0[j] = (decision0[j] & m0[j]) | ((~decision0[j]) & m1[j]);
	survivor1[j] = (decision1[j] & m2[j]) | ((~decision1[j]) & m3[j]);
      }

      for (int j = 0; j < 16; j += 2) {
	simd_epi16 = path0[(i*16) + j];
	simd_epi16 |= path0[(i*16) + (j+1)] << 8;
	simd_epi16 <<= 1;
	shift0[j] = simd_epi16;
	shift0[j+1] = simd_epi16 >> 8;

	simd_epi16 = path0[((i+2)*16) + j];
	simd_epi16 |= path0[((i+2)*16) + (j+1)] << 8;
	simd_epi16 <<= 1;
	shift1[j] = simd_epi16;
	shift1[j+1] = simd_epi16 >> 8;
      }
      for (int j = 0; j < 16; j++) {
	shift1[j] = shift1[j] + 1;
      }

      for (int j = 0, k = 0; j < 16; j += 2, k++) {
	metric1[(2*i*16) + j] = survivor0[k];
	metric1[(2*i*16) + (j+1)] = survivor1[k];
      }
      for (int j = 0; j < 16; j++) {
	tmp0[j] = (decision0[j] & shift0[j]) | ((~decision0[j]) & shift1[j]);
      }

      for (int j = 0, k = 8; j < 16; j += 2, k++) {
	metric1[((2*i+1)*16) + j] = survivor0[k];
	metric1[((2*i+1)*16) + (j+1)] = survivor1[k];
      }
      for (int j = 0; j < 16; j++) {
	tmp1[j] = (decision1[j] & shift0[j]) | ((~decision1[j]) & shift1[j]);
      }

      for (int j = 0, k = 0; j < 16; j += 2, k++) {
	path1[(2*i*16) + j] = tmp0[k];
	path1[(2*i*16) + (j+1)] = tmp1[k];
      }
      for (int j = 0, k = 8; j < 16; j += 2, k++) {
	path1[((2*i+1)*16) + j] = tmp0[k];
	path1[((2*i+1)*16) + (j+1)] = tmp1[k];
      }
    }

    // Set up for the second two symbols (2 and 3)
    metric0 = mm1;
    path0 = pp1;
    metric1 = mm0;
    path1 = pp0;
    first_symbol = 2;
    second_symbol = first_symbol+1;
    for (int j = 0; j < 16; j++) {
      sym0v[j] = symbols[first_symbol];
      sym1v[j] = symbols[second_symbol];
    }
  }
}

//  Find current best path
// 
// INPUTS/OUTPUTS:  
//    RET_VAL     : (ignored)
//    mm0         : INPUT/OUTPUT  : Array [ 64 ]
//    pp0         : INPUT/OUTPUT  : Array [ 64 ] 
//    pp1         : INPUT/OUTPUT  : Array [ 64 ]
//    ntraceback  : INPUT         : int (I think effectively const for given run type; here 5 I think)
//    outbuf      : OUTPUT        : 1 byte
//    d_store_pos : GLOBAL IN/OUT : int (position in circular traceback buffer?)
//    d_mmresult  : GLOBAL OUTPUT : Array [ 64 bytes ] 
//    d_ppresult  : GLOBAL OUTPUT : Array [ntraceback][ 64 bytes ]

unsigned char viterbi_get_output_generic(unsigned char *mm0,                                    size_t mm0_size,
					 unsigned char *pp0,                                    size_t pp0_size,
					 int ntraceback,
					 unsigned char*  mmresult __attribute__((aligned(16))), size_t mmres_size,
					 unsigned char ppresult[TRACEBACK_MAX][64] __attribute__((aligned(16))), size_t ppres_size,
					 unsigned char *outbuf,                                 size_t outbuf_size) {
  int i;
  int bestmetric, minmetric;
  int beststate = 0;
  int pos = 0;
  int j;

  // circular buffer with the last ntraceback paths
  d_store_pos = (d_store_pos + 1) % ntraceback;

  for (i = 0; i < 4; i++) {
    for (j = 0; j < 16; j++) {
      mmresult[(i*16) + j] = mm0[(i*16) + j];
      ppresult[d_store_pos][(i*16) + j] = pp0[(i*16) + j];
    }
  }

  // Find out the best final state
  bestmetric = mmresult[beststate];
  minmetric = mmresult[beststate];

  for (i = 1; i < 64; i++) {
    if (mmresult[i] > bestmetric) {
      bestmetric = mmresult[i];
      beststate = i;
    }
    if (mmresult[i] < minmetric) {
      minmetric = mmresult[i];
    }
  }

  // Trace back
  for (i = 0, pos = d_store_pos; i < (ntraceback - 1); i++) {
    // Obtain the state from the output bits
    // by clocking in the output bits in reverse order.
    // The state has only 6 bits
    beststate = ppresult[pos][beststate] >> 2;
    pos = (pos - 1 + ntraceback) % ntraceback;
  }

  // Store output byte
  *outbuf = ppresult[pos][beststate];

  for (i = 0; i < 4; i++) {
    for (j = 0; j < 16; j++) {
      pp0[(i*16) + j] = 0;
      mm0[(i*16) + j] = mm0[(i*16) + j] - minmetric;
    }
  }

  return bestmetric;
}


void viterbi_reset(ofdm_param *ofdm,   size_t ofdm_size,
		   unsigned char*  d_metric0_generic __attribute__ ((aligned(16))), size_t d_m0_size,
		   unsigned char*  d_metric1_generic __attribute__ ((aligned(16))), size_t d_m1_size,
		   unsigned char*  d_path0_generic __attribute__ ((aligned(16))),   size_t d_p0_size,
		   unsigned char*  d_path1_generic __attribute__ ((aligned(16))),   size_t d_p1_size,
		   unsigned char*  d_mmresult __attribute__((aligned(16))),         size_t mmres_size,
		   unsigned char d_ppresult[TRACEBACK_MAX][64] __attribute__((aligned(16))),         size_t ppres_size,
		   d_branchtab27_t* d_branchtab27_generic,                          size_t d_brt_size,
		   int* d_ntraceback,                    size_t ntrbk_size,
		   int* d_k,                             size_t d_k_size,
		   unsigned char *d_depuncture_pattern,  size_t depunc_ptn_size
		   )
{
  int i, j;

  for (i = 0; i < 4; i++) {
    d_metric0_generic[i] = 0;
    d_path0_generic[i] = 0;
  }

  int polys[2] = { 0x6d, 0x4f };
  for(i=0; i < 32; i++) {
    d_branchtab27_generic[0].c[i] = (polys[0] < 0) ^ PARTAB[(2*i) & abs(polys[0])] ? 1 : 0;
    d_branchtab27_generic[1].c[i] = (polys[1] < 0) ^ PARTAB[(2*i) & abs(polys[1])] ? 1 : 0;
  }

  for (i = 0; i < 64; i++) {
    d_mmresult[i] = 0;
    for (j = 0; j < TRACEBACK_MAX; j++) {
      d_ppresult[j][i] = 0;
    }
  }

  switch(ofdm->encoding) {
  case BPSK_1_2:
  case QPSK_1_2:
  case QAM16_1_2:
    *d_ntraceback = 5;
    d_depuncture_pattern = PUNCTURE_1_2;
    *d_k = 1;
    break;
  case QAM64_2_3:
    *d_ntraceback = 9;
    d_depuncture_pattern = PUNCTURE_2_3;
    *d_k = 2;
    break;
  case BPSK_3_4:
  case QPSK_3_4:
  case QAM16_3_4:
  case QAM64_3_4:
    *d_ntraceback = 10;
    d_depuncture_pattern = PUNCTURE_3_4;
    *d_k = 3;
    break;
  }
}


/* This is the main "decode" function; it prepares data and repeatedly
 * calls the viterbi butterfly2 routine to do steps of decoding.
 */
// INPUTS/OUTPUTS:  
//    ofdm   : INPUT        : Struct (see utils.h) [enum, char, int, int, int]
//    frame  : INPUT/OUTPUT : Struct (see utils.h) [int, int, int, int]
//    in     : INPUT/OUTPUT : Array [ MAX_ENCODED_BITS == 24780 ]

void viterbi_decode(ofdm_param *ofdm,   size_t ofdm_size,
		    frame_param *frame, size_t frame_size,
		    uint8_t *in,        size_t in_size,
		    uint8_t* l_decoded, size_t decd_size)
{
  // Local memories
  unsigned char d_metric0_generic[64] __attribute__ ((aligned(16)));
  unsigned char d_metric1_generic[64] __attribute__ ((aligned(16)));
  unsigned char d_path0_generic[64] __attribute__ ((aligned(16)));
  unsigned char d_path1_generic[64] __attribute__ ((aligned(16)));

  // Metrics for each state
  unsigned char d_mmresult[64] __attribute__((aligned(16)));
  // Paths for each state
  unsigned char d_ppresult[TRACEBACK_MAX][64] __attribute__((aligned(16)));

  d_branchtab27_t d_branchtab27_generic[2];
  
  int d_ntraceback;
  int d_k;
  unsigned char *d_depuncture_pattern;

  viterbi_reset( ofdm,                  ofdm_size,
		 d_metric0_generic,     64,
		 d_metric1_generic,     64,
		 d_path0_generic,       64,
		 d_path1_generic,       64,
		 d_mmresult,            64,
		 d_ppresult,            TRACEBACK_MAX * 64,
		 d_branchtab27_generic, 2*sizeof(d_branchtab27_t),
		 &d_ntraceback,         sizeof(int),
		 &d_k,                  sizeof(int),
		 d_depuncture_pattern,  6);

  uint8_t *depunctured = depuncture(ofdm,      ofdm_size,
				    frame,     frame_size,
				    d_ntraceback, d_k,
				    d_depuncture_pattern,  6,
				    in,        in_size);
	
  int in_count = 0;
  int out_count = 0;
  int n_decoded = 0;

  //printf("uint8_t DECODER_VERIF_DATA[7000][4][64] = {\n");
  //printf("void set_viterbi_decode_verif_data() {\n");

  int viterbi_butterfly_calls = 0;
  while(n_decoded < frame->n_data_bits) {
    //printf("n_decoded = %d vs %d = frame->n_data_bits\n", n_decoded, frame->n_data_bits);
    if ((in_count % 4) == 0) { //0 or 3
      //printf(" Viterbi_Butterfly Call,%d,n_decoded,%d,n_data_bits,%d,in_count,%d,%d\n", viterbi_butterfly_calls, n_decoded, frame->n_data_bits, in_count, (in_count & 0xfffffffc));
      unsigned char *d_brtab27[2] = {&(d_branchtab27_generic[0].c[0]), &(d_branchtab27_generic[1].c[0])};
      viterbi_butterfly2_generic(&depunctured[in_count & 0xfffffffc],
				 d_brtab27,
				 d_metric0_generic, d_metric1_generic,
				 d_path0_generic,   d_path1_generic);
      viterbi_butterfly_calls++; // Do not increment until after the comparison code.

      if ((in_count > 0) && (in_count % 16) == 8) { // 8 or 11
	unsigned char c;
	viterbi_get_output_generic(d_metric0_generic,     64,
				   d_path0_generic,       64,
				   d_ntraceback,
				   d_mmresult,            64,
				   d_ppresult,            TRACEBACK_MAX * 64,
				   &c,                    1);
	//std::cout << "OUTPUT: " << (unsigned int)c << std::endl; 
	if (out_count >= d_ntraceback) {
	  for (int i= 0; i < 8; i++) {
	    l_decoded[(out_count - d_ntraceback) * 8 + i] = (c >> (7 - i)) & 0x1;
	    //printf("l_decoded[ %u ] written\n", (out_count - d_ntraceback) * 8 + i);
	    n_decoded++;
	  }
	}
	out_count++;
      }
    }
    in_count++;
  }
  //printf("};\n");
  //return l_decoded;
}

