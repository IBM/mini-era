/* -*-Mode: C;-*- */
#ifndef INCLUDED_CALC_FMCW_DIST_H
#define INCLUDED_CALC_FMCW_DIST_H

#include <stdint.h>

/* Some global FFT Radar definitions */
#define MAX_RADAR_N  (1<<14) // Max we allow is 16k samples
extern unsigned RADAR_LOGN;  // Log2 of the number of samples
extern unsigned RADAR_N;     // The number of samples (2^LOGN)
extern float    RADAR_fs;    // Sampling Frequency
extern float    RADAR_alpha; // Chirp rate (saw-tooth)

/* Some function declarations */
//extern void  init_calculate_peak_dist();
// NC add argument
extern void  init_calculate_peak_dist(unsigned fft_logn_samples);

extern float calculate_peak_dist_from_fmcw(float* data);

#ifdef INT_TIME
extern uint64_t calc_sec;
extern uint64_t calc_usec;

extern uint64_t fft_sec;
extern uint64_t fft_usec;

extern uint64_t fft_br_sec;
extern uint64_t fft_br_usec;

extern uint64_t fft_cvtin_sec;
extern uint64_t fft_cvtin_usec;

extern uint64_t fft_cvtout_sec;
extern uint64_t fft_cvtout_usec;

extern uint64_t cdfmcw_sec;
extern uint64_t cdfmcw_usec;
#endif

#endif
