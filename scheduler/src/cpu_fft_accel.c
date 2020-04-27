/* -*-Mode: C;-*- */

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

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

void execute_cpu_fft_accelerator(float* data)
{
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
}

