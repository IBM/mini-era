/* -*-Mode: C;-*- */

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "fft-1d.h"

#include "calc_fmcw_dist.h"
#include "scheduler.h"

#ifdef INT_TIME
struct timeval calc_start, calc_stop;
uint64_t calc_sec  = 0LL;
uint64_t calc_usec = 0LL;

struct timeval fft_stop, fft_start;
uint64_t fft_sec  = 0LL;
uint64_t fft_usec = 0LL;

struct timeval fft_br_stop, fft_br_start;
uint64_t fft_br_sec  = 0LL;
uint64_t fft_br_usec = 0LL;

struct timeval fft_cvtin_stop, fft_cvtin_start;
uint64_t fft_cvtin_sec  = 0LL;
uint64_t fft_cvtin_usec = 0LL;

struct timeval fft_cvtout_stop, fft_cvtout_start;
uint64_t fft_cvtout_sec  = 0LL;
uint64_t fft_cvtout_usec = 0LL;

struct timeval cdfmcw_stop, cdfmcw_start;
uint64_t cdfmcw_sec  = 0LL;
uint64_t cdfmcw_usec = 0LL;
#endif

unsigned RADAR_LOGN    = 0;   // Log2 of the number of samples
unsigned RADAR_N       = 0;   // The number of samples (2^LOGN)
float    RADAR_fs      = 0.0; // Sampling Frequency
float    RADAR_alpha   = 0.0; // Chirp rate (saw-tooth)
// CONSTANTS
#define RADAR_c          300000000.0  // Speed of Light in Meters/Sec
#define RADAR_threshold -100;

//float   RADAR_psd_threshold = 1e-10*pow(8192,2);  // ~= 0.006711 and 450 ~= 0.163635 in 16K
float   RADAR_psd_threshold = 0.0067108864;

void init_calculate_peak_dist(unsigned fft_logn_samples)
{
  switch (fft_logn_samples) {
  case 10:
    RADAR_LOGN  = 10;
    RADAR_fs    = 204800.0;
    RADAR_alpha = 30000000000.0;
    RADAR_psd_threshold = 0.000316; // 1e-10*pow(8192,2);  // 450m ~= 0.000638 so psd_thres ~= 0.000316 ?
    break;
  case 14:
    RADAR_LOGN  = 14;
    RADAR_fs    = 32768000.0;
    RADAR_alpha = 4800000000000.0;
    RADAR_psd_threshold = 1e-10*pow(8192,2);
    break;
  default:
    printf("ERROR : Unsupported Log-N FFT Samples Value: %u\n", fft_logn_samples);
    exit(-1);
  }
  RADAR_N = (1 << RADAR_LOGN);
}



// This now illustrates the use of the "task metadata" to transfer information for an FFT operation.
//  NOTE: We request a metadata block form the scheduler -- if one is not currently available, then what?
//     OPTIONS: 1. Wait for one to become available
//              2. Purge a lower-criticality task (if there is one), else wait
//              3. Drop task if this is lowest-criticality (?), else wait
//   To make the task independent of the calling program, we need to copy over the data into the metadata block
//      This treats the scheduler like an off-load accelerator, in many ways.
//   Then we should try to make thes CPU accelerators run in independent threads (pthreads, I think)?
//      This will let us refine the non-blocking behavior, and start the more detailed behavior of the
//        scheduler implementation (i.e. ranking, queue management, etc.)

void start_calculate_peak_dist_from_fmcw(task_metadata_block_t* fft_metadata_block, float* data)
{
  fft_metadata_block->data_size = 2 * RADAR_N * sizeof(float);
  // Copy over our task data to the MetaData Block
  //fft_metadata_block->data = (uint8_t*)data;
  float* mdataptr = (float*)fft_metadata_block->data_view.fft_data;
  for (int i = 0; i < 2*RADAR_N; i++) {
    mdataptr[i] = data[i];
  }

 #ifdef INT_TIME
  gettimeofday(&calc_start, NULL);
 #endif

 #ifdef INT_TIME
  gettimeofday(&fft_start, NULL);
 #endif // INT_TIME
  //  schedule_fft(data);
  request_execution(fft_metadata_block);
  // This now ends this block -- we've kicked off execution
};

// NOTE: This routine DOES NOT copy out the data results -- a call to
//   calculate_peak_distance_from_fmcw now results in alteration ONLY
//   of the metadata task data; we could send in the data pointer and
//   over-write the original input data with the FFT results (As we used to)
//   but this seems un-necessary since we only want the final "distance" really.
float
finish_calculate_peak_dist_from_fmcw(task_metadata_block_t* fft_metadata_block)
{
  float* mdataptr = (float*)fft_metadata_block->data_view.fft_data;
 #ifdef INT_TIME
  gettimeofday(&fft_stop, NULL);
  fft_sec  += fft_stop.tv_sec  - fft_start.tv_sec;
  fft_usec += fft_stop.tv_usec - fft_start.tv_usec;
 #endif // INT_TIME

 #ifdef INT_TIME
  gettimeofday(&calc_stop, NULL);
  calc_sec  += calc_stop.tv_sec  - calc_start.tv_sec;
  calc_usec += calc_stop.tv_usec - calc_start.tv_usec;

  gettimeofday(&cdfmcw_start, NULL);
 #endif // INT_TIME

  float* data = mdataptr;
  /* // Now we need to copy out the results (from the executed, accelerator task) */
  /* for (int i = 0; i < 2*RADAR_N; i++) { */
  /*   data[i] = mdataptr[i]; */
  /* } */

  float max_psd = 0;
  unsigned int max_index = 0;
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
 #ifdef INT_TIME
  gettimeofday(&cdfmcw_stop, NULL);
  cdfmcw_sec  += cdfmcw_stop.tv_sec  - cdfmcw_start.tv_sec;
  cdfmcw_usec += cdfmcw_stop.tv_usec - cdfmcw_start.tv_usec;
 #endif // INT_TIME
  //printf("max_psd = %f  vs %f\n", max_psd, 1e-10*pow(8192,2));
  if (max_psd > RADAR_psd_threshold) {
    return distance;
  } else {
    return INFINITY;
  }
}

