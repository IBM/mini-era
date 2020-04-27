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

#include "base.h"
#include "viterbi_flat.h"
#include "viterbi_standalone.h"

#include "scheduler.h"


#ifdef INT_TIME
struct timeval dodec_stop, dodec_start;
uint64_t dodec_sec  = 0LL;
uint64_t dodec_usec = 0LL;

struct timeval depunc_stop, depunc_start;
uint64_t depunc_sec  = 0LL;
uint64_t depunc_usec = 0LL;
#endif

#undef  GENERATE_CHECK_VALUES
//#define  GENERATE_CHECK_VALUES

// GLOBAL VARIABLES
t_branchtab27 d_branchtab27_generic[2];
//unsigned char d_metric0_generic[64] __attribute__ ((aligned(16)));
//unsigned char d_metric1_generic[64] __attribute__ ((aligned(16)));
//unsigned char d_path0_generic[64] __attribute__ ((aligned(16)));
//unsigned char d_path1_generic[64] __attribute__ ((aligned(16)));

// Position in circular buffer where the current decoded byte is stored
// int d_store_pos = 0;
// Metrics for each state
unsigned char d_mmresult[64] __attribute__((aligned(16)));
// Paths for each state
unsigned char d_ppresult[TRACEBACK_MAX][64] __attribute__((aligned(16)));


// This routine "depunctures" the input data stream according to the 
//  relevant encoding parameters, etc. and returns the depunctured data.

uint8_t* depuncture(uint8_t *in) {
  int count;
  int n_cbps = d_ofdm->n_cbps;
  uint8_t *depunctured;
  //printf("Depunture call...\n");
  if (d_ntraceback == 5) {
    count = d_frame->n_sym * n_cbps;
    depunctured = in;
  } else {
    depunctured = d_depunctured;
    count = 0;
    for(int i = 0; i < d_frame->n_sym; i++) {
      for(int k = 0; k < n_cbps; k++) {
	while (d_depuncture_pattern[count % (2 * d_k)] == 0) {
	  depunctured[count] = 2;
	  count++;
	}

	// Insert received bits
	depunctured[count] = in[i * n_cbps + k];
	count++;

	while (d_depuncture_pattern[count % (2 * d_k)] == 0) {
	  depunctured[count] = 2;
	  count++;
	}
      }
    }
  }
  //printf("  depuncture count = %u\n", count);
  return depunctured;
}




// Initialize starting metrics to prefer 0 state
void viterbi_chunks_init_generic() {
  int i, j;

  int polys[2] = { 0x6d, 0x4f };
  for(i=0; i < 32; i++) {
    d_branchtab27_generic[0].c[i] = (polys[0] < 0) ^ PARTAB[(2*i) & abs(polys[0])] ? 1 : 0;
    d_branchtab27_generic[1].c[i] = (polys[1] < 0) ^ PARTAB[(2*i) & abs(polys[1])] ? 1 : 0;
  }

}


void reset() {

  viterbi_chunks_init_generic();

  switch(d_ofdm->encoding) {
  case BPSK_1_2:
  case QPSK_1_2:
  case QAM16_1_2:
    d_ntraceback = 5;
    d_depuncture_pattern = PUNCTURE_1_2;
    d_k = 1;
    break;
  case QAM64_2_3:
    d_ntraceback = 9;
    d_depuncture_pattern = PUNCTURE_2_3;
    d_k = 2;
    break;
  case BPSK_3_4:
  case QPSK_3_4:
  case QAM16_3_4:
  case QAM64_3_4:
    d_ntraceback = 10;
    d_depuncture_pattern = PUNCTURE_3_4;
    d_k = 3;
    break;
  }
}



/* This is the main "decode" function; it prepares data and repeatedly
 * calls the viterbi butterfly2 routine to do steps of decoding.
 */
// INPUTS/OUTPUTS:  
//    ofdm   : INPUT  : Struct (see utils.h) [enum, char, int, int, int]
//    frame  : INPUT  : Struct (see utils.h) [int, int, int, int]
//    in     : INPUT  : uint8_t Array [ MAX_ENCODED_BITS == 24780 ]
//  <return> : OUTPUT : uint8_t Array [ MAX_ENCODED_BITS * 3 / 4 == 18585 ] : The decoded data stream

uint8_t* decode(ofdm_param *ofdm, frame_param *frame, uint8_t *in, int* n_dec_char) {

  d_ofdm = ofdm;
  d_frame = frame;

  *n_dec_char = 0; // We don't return this from do_decoding -- but we could?

  reset();

#ifdef INT_TIME
  gettimeofday(&depunc_start, NULL);
#endif
  uint8_t *depunctured = depuncture(in);
#ifdef INT_TIME
  gettimeofday(&depunc_stop, NULL);
  depunc_sec  += depunc_stop.tv_sec  - depunc_start.tv_sec;
  depunc_usec += depunc_stop.tv_usec - depunc_start.tv_usec;
#endif

  DO_VERBOSE({
      printf("VBS: depunctured = [\n");
      for (int ti = 0; ti < MAX_ENCODED_BITS; ti ++) {
	if (ti > 0) { printf(", "); }
	if ((ti > 0) && ((ti % 8) == 0)) { printf("  "); }
	if ((ti > 0) && ((ti % 40) == 0)) { printf("\n"); }
	printf("%02x", depunctured[ti]);
      }
      printf("\n");
    });

  {
    // Copy inputs into the inMemory for esp-interface version
    uint8_t inMemory[70];  // This is "minimally sized for max entries"
    uint8_t inData[MAX_ENCODED_BITS];
    // Copy some multi-block stuff into a single memory (cleaner transport)
    int imi = 0;
    for (int ti = 0; ti < 2; ti ++) {
      for (int tj = 0; tj < 32; tj++) {
	inMemory[imi++] = d_branchtab27_generic[ti].c[tj];
      }
    }
    if (imi != 64) { printf("ERROR : imi = %u and should be 64\n", imi); }
    // imi = 64;
    for (int ti = 0; ti < 6; ti ++) {
      inMemory[imi++] = d_depuncture_pattern[ti];
    }
    if (imi != 70) { printf("ERROR : imi = %u and should be 70\n", imi); }

    // Call the do_decoding routine
    //  NOTE: We are sending addresses in our space -- the accelerator should xfer the data in this model.
    DEBUG(printf("Calling schedule_viterbit with nDb %u nCb %u nTrb %u\n", frame->n_data_bits, ofdm->n_cbps, d_ntraceback));
    schedule_viterbi(frame->n_data_bits, ofdm->n_cbps, d_ntraceback, inMemory, depunctured, d_decoded);
    DEBUG(for (int i = 0; i < 32; i++) {
		    printf("VIT_OUT %3u : %3u \n", i, d_decoded[i]);
	    });
  }
 #ifdef GENERATE_CHECK_VALUES
  printf("LAST-OUTPUT\n\n");
 #endif
  return d_decoded;
}


