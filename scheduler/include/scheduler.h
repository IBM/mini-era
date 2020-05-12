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
#include <pthread.h>

//#include "verbose.h"

#include "base_types.h"

#define MAX_ACCEL_OF_EACH_TYPE   10


typedef enum { NO_TASK_JOB = 0,
	       FFT_TASK,
	       VITERBI_TASK,
	       NUM_JOB_TYPES } scheduler_jobs_t;

typedef enum { NO_TASK   = 0,
	       BASE_TASK = 1,
	       ELEVATED_TASK = 2,
	       CRITICAL_TASK = 3,
	       NUM_TASK_CRIT_LEVELS} task_criticality_t;
	       
typedef enum { TASK_FREE = 0,
	       TASK_ALLOCATED,
	       TASK_QUEUED,
	       TASK_RUNNING,
	       TASK_DONE,
	       NUM_TASK_STATUS} task_status_t;

typedef enum { cpu_accel_t = 0,
	       fft_hwr_accel_t,
	       vit_hwr_accel_t,
	       no_accelerator_t,
	       NUM_ACCEL_TYPES} accelerator_type_t;

// This is a metatdata structure; it is used to hold all information for any job
//  to be invoked through the scheduler.  This includes a description of the
//  job type, and all input/output data space for the task
// The job types are defined above in the scheduler_jobs_t enumeration
// The data (i.e. inputs, outputs, etc. ) are transferred here as a "bulk data"
//  memory (of abstract uint8_t or bytes) and a size.  The interpretation of this
//  block of data is task-dependent, and can have an over-laid structure, etc.

typedef union task_metadata_entry_union {
  struct task_metadata_struct {
    int32_t  block_id;             // +4 Bytes : master-pool-index; a unique ID per metadata task
    task_status_t  status;         // +4 Bytes : -1 = free, 0 = allocated, 1 = queued, 2 = running, 3 = done ?
    pthread_t       thread_id;     // +8?Bytes : set when we invoke pthread_create (at least for CPU)
    accelerator_type_t  accelerator_type;     // +4 bytes : indicates which accelerator this task is executing on
    int32_t  accelerator_id;       // +4 bytes : indicates which accelerator this task is executing on
    scheduler_jobs_t job_type;     // +4 Bytes : see above enumeration
    task_criticality_t crit_level; // +4 Bytes : [0 .. ?] ?
    int32_t  data_size;            // +4 Bytes : Number of bytes occupied in data (NOT USED/NOT NEEDED?)
    uint8_t  data[128*1024];       // 128 KB (FFT 16k complex float) : All the data (in/out, etc.)
  } metadata;
  uint8_t rawBytes[128*1024 + 64]; // Max size of an entry -- 16k*2*32 entries (FFT) + MDB data fields + pad
} task_metadata_block_t;


typedef struct {
  int32_t n_data_bits;
  int32_t n_cbps;
  int32_t n_traceback;
  int32_t psdu_size;
  int32_t inMem_size;    // The first inMem_size bytes of theData are the inMem (input memories)
  int32_t inData_size;   // The next inData_size bytes of theData are the inData (input data)
  int32_t outData_size;  // The next outData_size bytes of theData are the outData (output data)
  uint8_t theData[64*1024]; // This is larger than needed (~24780 + 18585) but less than FFT requires (so okay)
}  viterbi_data_struct_t;


extern unsigned fft_logn_samples;

extern status_t initialize_scheduler();

extern task_metadata_block_t* get_task_metadata_block(scheduler_jobs_t task_type, task_criticality_t crit_level);
extern void free_task_metadata_block(task_metadata_block_t* mb);

extern void request_execution(task_metadata_block_t* task_metadata_block);
extern void wait_all_critical();

extern int get_task_status(int task_id);

extern void print_base_metadata_block_contents(task_metadata_block_t* mb);
extern void print_fft_metadata_block_contents(task_metadata_block_t* mb);
extern void print_viterbi_metadata_block_contents(task_metadata_block_t* mb);

//extern void schedule_fft(task_metadata_block_t* task_metadata_block);

extern void schedule_viterbi(int n_cbps, int n_traceback, int n_data_bits, uint8_t* inMem, uint8_t* inData, uint8_t* outMem);

extern void shutdown_scheduler();

#endif
