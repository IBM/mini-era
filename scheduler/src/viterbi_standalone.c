#include <stdio.h>
//#include <stdbool.h>

#include "base.h"
#include "viterbi_flat.h"

#define DO_OUTPUT_VALUE_CHECKING

ofdm_param ofdm = {   0,   //  encoding   : 0 = BPSK_1_2
		     13,   //  rate_field : rate field ofSIGNAL header
		      1,   //  n_bpsc     : coded bits per subcarrier
		     48,   //  n_cbps     : coded bits per OFDM symbol
		     24 }; //  n_dbps     : data bits per OFDM symbol

frame_param frame = {  1528,    // psdu_size      : PSDU size in bytes 
		        511,    // n_sym          : number of OFDM symbols
		         18,    // n_pad          : number of padding bits in DATA field
		      24528,    // n_encoded_bits : number of encoded bits
		      12264 };  // n_data_bits    : number of data bits, including service and padding

