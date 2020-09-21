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

#ifndef _kernels_api_h
#define _kernels_api_h

#define TIME

#include "base.h"
#ifdef USE_XMIT_PIPE
#include "xmit_pipe.h"
status_t init_xmit_kernel(void);
void     iterate_xmit_kernel(void);
void     execute_xmit_kernel(int in_msg_len, char* in_msg, int* n_out, float* out_real, float* out_imag);
void     post_execute_xmit_kernel(void);
void     closeout_xmit_kernel(void);
#endif

#ifdef USE_RECV_PIPE
#include "recv_pipe.h"
status_t init_recv_kernel(void);
void     iterate_recv_kernel(void);
void     execute_recv_kernel(int in_msg_len, int n_in, float* in_real, float* in_imag, int* out_msg_len, char* out_msg);
void     post_execute_recv_kernel(void);
void     closeout_recv_kernel(void);
#endif


#endif
