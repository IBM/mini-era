/* -*-Mode: C;-*- */
#ifndef INCLUDED_CALC_FMCW_DIST_H
#define INCLUDED_CALC_FMCW_DIST_H

/* Some constant definitions */
#define RADAR_LOGN (14)
#define RADAR_N (1 << RADAR_LOGN)
#define RADAR_c 300000000
#define RADAR_fs 32768000
#define RADAR_alpha 4800000000000
#define RADAR_threshold -100

/* Some function declarations */
extern float calculate_peak_dist_from_fmcw(float* data, size_t data_size_bytes);

#endif
