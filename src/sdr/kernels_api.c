/*
 * Copyright 2019 IBM
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "debug.h"
#include "kernels_api.h"

extern unsigned time_step;

#ifdef USE_XMIT_PIPE
status_t init_xmit_kernel()
{
  DEBUG(printf("In the init_xmit_kernel routine\n"));
  xmit_pipe_init();
  return success;
}

void iterate_xmit_kernel()
{
  DEBUG(printf("In iterate_xmit_kernel\n"));

  return;
}


void execute_xmit_kernel(int in_msg_len, char * in_msg, int* n_out, float* out_real, float* out_imag)
{
  DEBUG(printf("In execute_xmit_kernel\n"));
  do_xmit_pipeline(in_msg_len, in_msg, n_out, out_real, out_imag);
  return;
}

void post_execute_xmit_kernel()
{
  DEBUG(printf("In post_execute_xmit_kernel\n"));

  return;
}



void closeout_xmit_kernel()
{
  // Nothing to do?
  return;
}

#endif



#ifdef USE_RECV_PIPE
#ifndef USE_XMIT_PIPE // We are in file-driven receive pipe only mode
#define MAX_XMIT_OUTPUTS   41800   // Really 41782 I think

extern char     recv_in_fname[256];
extern uint32_t xmit_msg_len;
extern int      xmit_num_out;
extern float    xmit_out_real[MAX_XMIT_OUTPUTS];
extern float    xmit_out_imag[MAX_XMIT_OUTPUTS];
#endif

status_t init_recv_kernel()
{
  DEBUG(printf("In init_recv_kernel...\n"));
 #ifndef USE_XMIT_PIPE
  // Read in the encoded message data
  FILE *inF = fopen(recv_in_fname, "r");
  if (!inF) {
    printf("Error: unable to open receiver-pipeline input encoded message file %s\n", recv_in_fname);
    return error;
  }

  // Read in the encoded message data
  // Read the length and number of encoded complex values
  if (fscanf(inF, "%u %d\n", &xmit_msg_len, &xmit_num_out) != 2) {
    printf("ERROR reading the encoded msg length and number of complex values\n");
    fclose(inF);
    exit(-2);
  }    
  DEBUG(printf("  The message is %u bytes and %u complex encoded values\n", xmit_msg_len, xmit_num_out));
  for (int i = 0; i < xmit_num_out; i++) {
    if (fscanf(inF, "%f %f", &xmit_out_real[i], &xmit_out_imag[i]) != 2) {
      printf("ERROR reading the complex input %d values\n", i);
      fclose(inF);
      exit(-2);
    }
  }
  fclose(inF);
  #endif

  return success;
}

void iterate_recv_kernel()
{
  DEBUG(printf("In iterate_recv_kernel\n"));

  return;
}

void execute_recv_kernel(int in_msg_len, int n_in, float* in_real, float* in_imag, int* out_msg_len, char* out_msg)
{
  DEBUG(printf("In execute_recv_kernel\n"));
  do_recv_pipeline(n_in, in_real, in_imag, out_msg_len, out_msg);
  return;
}

void post_execute_recv_kernel()
{
  DEBUG(printf("In post_execute_recv_kernel\n"));

  return;
}


void closeout_recv_kernel()
{
  /* printf("\nHistogram of Recvar Distances:\n"); */
  /* for (int di = 0; di < num_recvar_dictionary_items; di++) { */
  /*   printf("    %3u | %8.3f | %9u \n", di, the_recvar_return_dict[di].distance, hist_distances[di]); */
  /* } */

  return;
}



#endif

