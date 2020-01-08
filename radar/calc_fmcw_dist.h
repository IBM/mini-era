/* -*-Mode: C;-*- */
#ifndef INCLUDED_CALC_FMCW_DIST_H
#define INCLUDED_CALC_FMCW_DIST_H

#include <stdint.h>

/* Some constant definitions */
#define RADAR_LOGN (14)
#define RADAR_N (1 << RADAR_LOGN)
#define RADAR_c 300000000
#define RADAR_fs 32768000
#define RADAR_alpha 4800000000000
#define RADAR_threshold -100

/* Some function declarations */
extern float calculate_peak_dist_from_fmcw(float* data);

#ifdef INT_TIME
extern uint64_t fft_sec;
extern uint64_t fft_usec;

extern uint64_t cdfmcw_sec;
extern uint64_t cdfmcw_usec;
#endif

#endif
