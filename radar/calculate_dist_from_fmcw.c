/* -*-Mode: C;-*- */

#include <stdio.h>
#include <stdlib.h>

#include "fft-1d.h"

#include "calc_fmcw_dist.h"

float calculate_peak_dist_from_fmcw(float* data)
{
  fft (data, RADAR_N, RADAR_LOGN, -1);
  float max_psd = 0;
  unsigned int max_index;
  unsigned int i;
  float temp;
  for (i=0; i < RADAR_N; i++) {
    temp = (pow(data[2*i],2) + pow(data[2*i+1],2))/100.0;
    if (temp > max_psd) {
      max_psd = temp;
      max_index = i;
    }
  }
  float distance = ((float)(max_index*((float)RADAR_fs)/((float)(RADAR_N))))*0.5*RADAR_c/((float)(RADAR_alpha));
  //printf("Max distance is %.3f\nMax PSD is %4E\nMax index is %d\n", distance, max_psd, max_index);
  if (max_psd > 1e-10*pow(8192,2)) {
    return distance;
  } else {
    return INFINITY;
  }
}

