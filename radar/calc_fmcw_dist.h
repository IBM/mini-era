/* -*-Mode: C;-*- */
#ifndef INCLUDED_CALC_FMCW_DIST_H
#define INCLUDED_CALC_FMCW_DIST_H

/* Some constant definitions */
#define LOGN (14)
#define N (1 << LOGN)
#define c 300000000
#define fs 32768000
#define alpha 4800000000000
#define threshold -100

/* Some function declarations */
extern float calculate_peak_dist_from_fmcw(float* data);

#endif
