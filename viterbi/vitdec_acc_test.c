/*
 * vit-accel_test.c
 * Author Name: J-D Wellman
 * Created on: DEc 9, 2019
 * Function: generates a random input bit stream and then decodes it
 *
 *  Source: https://github.com/bastibl/gr-ieee802-11/blob/maint-3.8/lib/viterbi_decoder/base.cc
 *
 * C implementation_source:https://github.com/IBM/era/tree/master/mini-era/viterbi
 *
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "utils.h"
#include "base.h"
#include "viterbi_flat.h"

#define SHOW_INPUTS_AND_OUTPUTS   0

/* extern void descrambler(uint8_t* in, int psdusize, char* out_msg, uint8_t* ref, uint8_t *msg); */
/* extern uint8_t* decode(ofdm_param *ofdm, frame_param *frame, uint8_t *in, int* n_dec_char); */

#ifdef USE_ESP_INTERFACE
 extern void do_decoding(int in_n_data_bits, int in_cbps, int in_ntraceback, unsigned char *inMemory);
 uint8_t inMemory[43449]; // This is "minimally sized for max entries"
 /* #else */
 //extern uint8_t* do_decoding(int in_cbps, int in_ntraceback, const unsigned char* in_depuncture_pattern, int in_n_data_bits, uint8_t* depd_data); */
#endif

int main(int argc, char* argv[])
{
  int n_cbps = 48;
  int n_data_bits = 12264;
  int ntraceback = 5;
  unsigned char depunct_ptn[6] = {1, 1, 0, 0, 0, 0}; // PATTERN_1_2 Extended with zeros

  /* uint8_t input[MAX_ENCODED_BITS]; // MAX_Encoded_bits]; //={0}; */
  /* uint8_t output[MAX_ENCODED_BITS * 3 / 4]; // MAX_Decoded_bits]; // ={0}; 1000 is maximum used here */

  // We randomly set the input (encoded bits)
#ifdef USE_ESP_INTERFACE
  {
    // Copy inputs into the inMemory for esp-interface version

    int imi = 0;
    int polys[2] = { 0x6d, 0x4f };
    for(int i=0; i < 32; i++) {
      inMemory[imi] = (polys[0] < 0) ^ PARTAB[(2*i) & abs(polys[0])] ? 1 : 0;
      inMemory[imi+32] = (polys[1] < 0) ^ PARTAB[(2*i) & abs(polys[1])] ? 1 : 0;
      imi++;
    }
    if (imi != 32) { printf("ERROR : imi = %u and should be 32\n", imi); }
    imi += 32;
    
    if (imi != 64) { printf("ERROR : imi = %u and should be 64\n", imi); }
    // imi = 64;
    for (int ti = 0; ti < 6; ti ++) {
      inMemory[imi++] = depunct_ptn[ti];
    }
    if (imi != 70) { printf("ERROR : imi = %u and should be 70\n", imi); }
    // imi = 70
    imi += 2; // Padding
    for (int ti = 0; ti < MAX_ENCODED_BITS; ti ++) {
      inMemory[imi++] = rand() & 0x01;
    }
    if (imi != 24852) { printf("ERROR : imi = %u and should be 24852\n", imi); }
    // imi = 24862 : OUTPUT ONLY -- DON'T NEED TO SEND INPUTS
    // Reset the output space (for cleaner testing results)
    for (int ti = 0; ti < (MAX_ENCODED_BITS * 3 / 4); ti ++) {
      inMemory[imi++] = 0;
    }

  VERBOSE({
      printf("INPUT: iM %p input %p size %lu\n", inMemory, &inMemory[72], sizeof(inMemory[0]));
      int per_row = 0;
      int timi = 72;
      printf("%p : ", &inMemory[timi]);
      for (int ti = timi; ti < MAX_ENCODED_BITS+timi; ti++) {
	per_row++;
	if ((per_row % 8) == 0) {
	  printf(" ");
	}
	printf("%u", inMemory[ti]);
	if (per_row == 39) {
	  printf("\n");
	  printf("%p : ", &inMemory[ti]);
	  per_row = 0;
	}
      }
      printf("\n");
      printf("AT_DO_DECODING: iM %p output %p size %lu\n", inMemory, &inMemory[24852], sizeof(inMemory[0]));
      printf("\n");
    });
  
    do_decoding(n_data_bits, n_cbps, ntraceback, inMemory);

    // Display the outputs
  VERBOSE({
      imi = 24852; // start of the outputs
      printf("OUTPUT: iM %p out %p size %lu\n", inMemory, &inMemory[24852], sizeof(inMemory[0]));
      printf("%p : ", &inMemory[imi]);
      int per_row = 0;
      for (int ti = imi; ti < imi+(MAX_ENCODED_BITS * 3 / 4); ti ++) {
	per_row++;
	if ((per_row % 8) == 0) {
	  printf(" ");
	}
	printf("%u", inMemory[ti]);
	if (per_row == 39) {
	  printf("\n");
	  printf("%p : ", &inMemory[ti]);
	  per_row = 0;
	}
      }
    });
  }
#else
  //do_decoding(in_cbps, int in_ntraceback, const unsigned char* in_depuncture_pattern, int in_n_data_bits, uint8_t* depd_data);
  printf("UNSUPPORTED BUILD MODE...\n");
#endif

  return 0;
}
