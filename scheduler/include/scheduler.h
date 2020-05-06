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

typedef enum { fft_task = 0,
	       viterbi_task } scheduler_jobs_t;

// This is a metatdata structure; it is used to hold all information for any job
//  to be invoked through the scheduler.  This includes a description of the
//  job type, and all input/output data space for the task
// The job types are defined above in the scheduler_jobs_t enumeration
// The data (i.e. inputs, outputs, etc. ) are transferred here as a "bulk data"
//  memory (of abstract uint8_t or bytes) and a size.  The interpretation of this
//  block of data is task-dependent, and can have an over-laid structure, etc.

typedef union task_metadata_entry_union {
  struct task_metadata_struct {
    int32_t  metadata_block_id;   // +4 Bytes : master-pool-index; a unique ID per metadata task
    int32_t  status;              // +4 Bytes : -1 = free, 0 = allocated, 1 = queued, 2 = running, 3 = done ?
    scheduler_jobs_t job_type;    // +4 Bytes : see above enumeration
    int32_t  criticality_level;   // +4 Bytes : [0 .. ?] ?
    int32_t  data_size;           // +4 Bytes : Number of bytes occupied in data (NOT USED/NOT NEEDED?)
    uint8_t  data[128*1024];      // 128 KB (FFT 16k complex float) : All the data (in/out, etc.)
  } metadata;
  uint8_t rawBytes[128*1024 + 32];  // Max size of an entry -- 16k*2*32 entries (FFT) + MDB data fields + pad
} task_metadata_block_t;


typedef struct {
  int32_t n_data_bits;
  int32_t n_cbps;
  int32_t n_traceback;
  int32_t inMem_size;   // The first inMem_size bytes of theData are the inMem
  int32_t inData_size;  // The next inData_size bytes of theData are the inData
  int32_t outMem_size;  // The next outMem_size bytes of theData are the outMem
  uint8_t* theData;
}  viterbi_data_struct_t;


extern unsigned fft_logn_samples;

extern status_t initialize_scheduler();

extern task_metadata_block_t* get_task_metadata_block();

extern void schedule_task(task_metadata_block_t* task_metadata_block);
extern int get_task_status(int task_id);

//extern void schedule_fft(task_metadata_block_t* task_metadata_block);

extern void schedule_viterbi(int n_cbps, int n_traceback, int n_data_bits, uint8_t* inMem, uint8_t* inData, uint8_t* outMem);

extern void shutdown_scheduler();

#endif
