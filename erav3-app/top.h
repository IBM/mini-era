#ifndef INC_TOP_H
#define INC_TOP_H

#include <stdio.h>

#include "type.h"
#include "base.h"

void initialize_for_run();

void top(uint8_t* out_msg, unsigned in_msg_len, unsigned num_in_data, fx_pt* in_data);
void do_from_top(unsigned in_msg_len, unsigned num_inputs, float *in_real, float* in_imag, unsigned * num_msg_chars, uint8_t* out_msg );

#endif
