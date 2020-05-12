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
  DEBUG(printf("In execute_cpu_fft_accelerator: MB %d  CL %d\n", task_metadata_block->metadata.metadata_block_id, task_metadata_block->metadata.criticality_level ));
  float * data = (float*)(task_metadata_block->metadata.data);

#ifdef INT_TIME
  gettimeofday(&calc_start, NULL);
 #endif

 #ifdef INT_TIME
  gettimeofday(&fft_start, NULL);
 #endif // INT_TIME
  fft(data, 1<<fft_logn_samples, fft_logn_samples, -1);
  /* for (int j = 0; j < 2 * RADAR_N; j++) { */
  /*   printf("%u,%f\n", j, data[j]); */
  /* } */
 #ifdef INT_TIME
  gettimeofday(&fft_stop, NULL);
  fft_sec  += fft_stop.tv_sec  - fft_start.tv_sec;
  fft_usec += fft_stop.tv_usec - fft_start.tv_usec;
 #endif // INT_TIME

 #ifdef INT_TIME
  gettimeofday(&calc_stop, NULL);
  calc_sec  += calc_stop.tv_sec  - calc_start.tv_sec;
  calc_usec += calc_stop.tv_usec - calc_start.tv_usec;
 #endif

  task_metadata_block->metadata.status = TASK_DONE; // done
  release_accelerator_for_task(task_metadata_block);
  return NULL;
}

