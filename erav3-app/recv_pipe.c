
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>
#include <math.h>
#include <unistd.h>
#include <assert.h>

#include "debug.h"
#include "base.h"
#include "recv_pipe.h"
#include "top.h"

/********************************************************************************
 * This routine manages the initializations for all recv pipeline components
 ********************************************************************************/
void
recv_pipe_init() {
  initialize_for_run();
}



/********************************************************************************
 * This routine manages the transmit pipeline functions and components
 ********************************************************************************/
void
do_recv_pipeline(int msg_len, int num_recvd_vals, float* recvd_in_real, float* recvd_in_imag, int* recvd_msg_len, char * recvd_msg)
{
  DEBUG(printf("In do_recv_pipeline: num_received_vals = %u\n", num_recvd_vals); fflush(stdout));
  do_from_top(msg_len, num_recvd_vals, recvd_in_real, recvd_in_imag, (unsigned*)recvd_msg_len, (uint8_t*)recvd_msg);
}


