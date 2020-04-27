/*
 * Copyright 2020 IBM
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

#ifndef H_SCHEDULER_INTERFACE_H
#define H_SCHEDULER_INTERFACE_H

#include <stdint.h>

//#include "verbose.h"

#include "base_types.h"

extern unsigned fft_logn_samples;

extern status_t initialize_scheduler();

extern void schedule_fft(float * data);

extern void schedule_viterbi(int n_cbps, int n_traceback, int n_data_bits, uint8_t* inMem, uint8_t* inData, uint8_t* outMem);

extern void shutdown_scheduler();

#endif
