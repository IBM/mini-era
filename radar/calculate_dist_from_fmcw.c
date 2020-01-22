/* -*-Mode: C;-*- */

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "fft-1d.h"

#include "calc_fmcw_dist.h"

#ifdef INT_TIME
struct timeval fft_stop, fft_start;
uint64_t fft_sec  = 0LL;
uint64_t fft_usec = 0LL;

struct timeval cdfmcw_stop, cdfmcw_start;
uint64_t cdfmcw_sec  = 0LL;
uint64_t cdfmcw_usec = 0LL;
#endif

float calculate_peak_dist_from_fmcw(float* data)
{
#ifdef INT_TIME
  gettimeofday(&fft_start, NULL);
#endif
  fft (data, RADAR_N, RADAR_LOGN, -1);
#ifdef INT_TIME
  gettimeofday(&fft_stop, NULL);
  fft_sec  += fft_stop.tv_sec  - fft_start.tv_sec;
  fft_usec += fft_stop.tv_usec - fft_start.tv_usec;

  gettimeofday(&cdfmcw_start, NULL);
#endif
  float max_psd = 0;
  unsigned int max_index;
  unsigned int i;
  float temp;
  //printf("\nSCAN OF FFT OUTPUT DATA\n");
  for (i=0; i < RADAR_N; i++) {
    //printf("CALC DATA %6u = %f %f\n", 2*i, data[2*i], data[2*i+1]);
    temp = (pow(data[2*i],2) + pow(data[2*i+1],2))/100.0;
    if (temp > max_psd) {
      max_psd = temp;
      max_index = i;
    }
  }
  float distance = ((float)(max_index*((float)RADAR_fs)/((float)(RADAR_N))))*0.5*RADAR_c/((float)(RADAR_alpha));
  //printf("Max distance is %.3f\nMax PSD is %4E\nMax index is %d\n", distance, max_psd, max_index);
#ifdef INT_TIME
  gettimeofday(&cdfmcw_stop, NULL);
  cdfmcw_sec  += cdfmcw_stop.tv_sec  - cdfmcw_start.tv_sec;
  cdfmcw_usec += cdfmcw_stop.tv_usec - cdfmcw_start.tv_usec;
#endif
  if (max_psd > 1e-10*pow(8192,2)) {
    return distance;
  } else {
    return INFINITY;
  }
}

