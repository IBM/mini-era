#ifndef SIMPLE_DFT_H
#define SIMPLE_DFT_H

void simple_dft(const float *inreal, const float *inimag,
		float *outreal, float *outimag,
		int inverse, int shift, unsigned n);

#endif

  
