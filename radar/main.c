/* -*-Mode: C;-*- */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "calc_fmcw_dist.h"

int read_input_file(float* data, unsigned int num, char* filename)
{
  FILE *fp = fopen(filename, "r");
  int i;
  for (i=0; i<num; i++) {
    fscanf(fp, "%f", (data+i));
  }
  fclose(fp);
  return 0;
}


int main (int argc, char * argv[])
{
  if (argc == 2) {
    printf("FMCW file used: %s\n", argv[1]);
  }
  else {
    printf("Usage: %s file.dat\n", argv[0]);
    return 1;
  }
  
  float * a;
  size_t data_size = 2 * RADAR_N * sizeof(float);
  a = malloc (data_size);
  read_input_file (a, RADAR_N, argv[1]);
  float dist;
  calculate_peak_dist_from_fmcw(a, data_size, &dist, sizeof(float));
  printf("Distance of object from FMCW data = %.2f m\n", dist);
  free (a);
  return 0;
}
