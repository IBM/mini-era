
void
fft (float * data, size_t bytes_data, unsigned int N, unsigned int logn, int sign)
{

  __visc__hint(CPU_TARGET);
  __visc__attributes(1, data, 1, data);

  unsigned int transform_length;
  unsigned int a, b, i, j, bit;
  float theta, t_real, t_imag, w_real, w_imag, s, t, s2, z_real, z_imag;

  transform_length = 1;

  /* bit reversal */
  bit_reverse (data, N, logn);

  /* calculation */
  for (bit = 0; bit < logn; bit++) {
    w_real = 1.0;
    w_imag = 0.0;

    theta = 1.0 * sign * M_PI / (float) transform_length;

    s = sin (theta);
    t = sin (0.5 * theta);
    s2 = 2.0 * t * t;

    for (a = 0; a < transform_length; a++) {
      for (b = 0; b < N; b += 2 * transform_length) {
	i = b + a;
	j = b + a + transform_length;

	z_real = data[2*j  ];
	z_imag = data[2*j+1];

	t_real = w_real * z_real - w_imag * z_imag;
	t_imag = w_real * z_imag + w_imag * z_real;

	/* write the result */
	data[2*j  ]  = data[2*i  ] - t_real;
	data[2*j+1]  = data[2*i+1] - t_imag;
	data[2*i  ] += t_real;
	data[2*i+1] += t_imag;
      }

      /* adjust w */
      t_real = w_real - (s * w_imag + s2 * w_real);
      t_imag = w_imag + (s * w_real - s2 * w_imag);
      w_real = t_real;
      w_imag = t_imag;

    }

    transform_length *= 2;
  }

  // Argmax computation inlined
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
  if (!(max_psd > 1e-10*pow(8192,2)))
    distance = INFINITY;

  __visc__return(1, distance);
}

void miniERARoot(/* 0 */ float * data, size_t bytes_data,/* 1 */
		 /* 2 */ unsigned int N,
		 /* 3 */ unsigned int logn,
		 /* 4 */ int sign) {

  __visc__hint(visc::CPU_TARGET);
//  __visc__attributes(x, ..., x, ...);

  // FFT Node
  void* FFT_node = __visc__createNodeND(0, fft);

  // Viterbi Node
  void* VB_node = __visc__createNodeND(0, viterbi_node_function);

  // CV Nodes
  // nodes generated from DNN compiled from Keras here

  // Plan and Control Node
  void* PC_node = __visc__createNodeND(0, planAndControl_node_function);

  __visc__edge(FFT_node, PC_node, 1, , , 0);
  __visc__edge(VB_node, PC_node, 1, , , 0);
  __visc__edge(/* last of Keras nodes */, PC_node, 1, , , 0); // tensor result
  __visc__edge(/* last of Keras nodes */, PC_node, 1, , , 0); // size of tensor

}

typedef struct __attribute__((__packed__)) {
  float * data, size_t bytes_data,
  unsigned int N,
  unsigned int logn,
  int sign
} RootIn;

int main() {

}
