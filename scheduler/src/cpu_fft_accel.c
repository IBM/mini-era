/* -*-Mode: C;-*- */

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "verbose.h"
#include "scheduler.h"

#include "fft-1d.h"

#include "calc_fmcw_dist.h"

#ifdef INT_TIME
extern struct timeval calc_start, calc_stop;
extern uint64_t calc_sec;
extern uint64_t calc_usec;

extern struct timeval fft_stop, fft_start;
extern uint64_t fft_sec;
extern uint64_t fft_usec;

#endif

// Putting this into a pthreads invocation mode...
void execute_cpu_fft_accelerator(task_metadata_block_t* task_metadata_block)
{
  DEBUG(printf("In execute_cpu_fft_accelerator: MB %d  CL %d\n", task_metadata_block->metadata_block_id, task_metadata_block->criticality_level ));
  float * data = (float*)(task_metadata_block->data_view.fft_data);

#ifdef INT_TIME
  gettimeofday(&(task_metadata_block->fft_timings.calc_start), NULL);
 #endif

 #ifdef INT_TIME
  gettimeofday(&(task_metadata_block->fft_timings.fft_start), NULL);
 #endif // INT_TIME
  fft(task_metadata_block, data, 1<<fft_logn_samples, fft_logn_samples, -1);
  /* for (int j = 0; j < 2 * RADAR_N; j++) { */
  /*   printf("%u,%f\n", j, data[j]); */
  /* } */
 #ifdef INT_TIME
  struct timeval stop_time;
  gettimeofday(&stop_time, NULL);
  task_metadata_block->fft_timings.fft_sec  += stop_time.tv_sec  - task_metadata_block->fft_timings.fft_start.tv_sec;
  task_metadata_block->fft_timings.fft_usec += stop_time.tv_usec - task_metadata_block->fft_timings.fft_start.tv_usec;
  task_metadata_block->fft_timings.calc_sec  += stop_time.tv_sec  - task_metadata_block->fft_timings.calc_start.tv_sec;
  task_metadata_block->fft_timings.calc_usec += stop_time.tv_usec - task_metadata_block->fft_timings.calc_start.tv_usec;
 #endif

  TDEBUG(printf("MB_THREAD %u calling mark_task_done...\n", task_metadata_block->block_id));
  mark_task_done(task_metadata_block);
}

