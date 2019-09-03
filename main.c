/*
 * Copyright 2019 IBM
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <string.h>
#include "kernels_api.h"
#include <stdlib.h>
#include <sys/time.h>
#define TIME
char * cv_dict  = "traces/objects_dictionary.dfn";
char * rad_dict = "traces/radar_dictionary.dfn";
char * vit_dict = "traces/vit_dictionary.dfn";

int main(int argc, char *argv[])
{
  vehicle_state_t vehicle_state;
  label_t label;
  distance_t distance;
  message_t message;

  /* We plan to use three separate trace files to drive the three different kernels
   * that are part of mini-ERA (CV, radar, Viterbi). All these three trace files
   * are required to have the same base name, using the file extension to indicate
   * which kernel the trace corresponds to (cv, rad, vit).
   */
  if (argc != 2)
  {
    printf("Usage: %s <trace_basename>\n\n", argv[0]);
    printf("Where <trace_basename> is the basename of the trace files to load:\n");
    printf("  <trace_basename>.cv  : trace to feed the computer vision kernel\n");
    printf("  <trace_basename>.rad : trace to feed the radar (FFT-1D) kernel\n");
    printf("  <trace_basename>.vit : trace to feed the Viterbi decoding kernel\n");

    return 1;
  }


  /* Trace filename construction */
  char * in_trace = argv[1];
  printf("Input trace file: %s\n", in_trace);

  char cv_py_file[] = "../cv/keras_cnn/lenet.py";

  /* Trace Reader initialization */
  if (!init_trace_reader(in_trace))
  {
    printf("Error: the trace reader couldn't be initialized properly.\n");
    return 1;
  }

  /* Kernels initialization */
  if (!init_cv_kernel(cv_py_file, cv_dict))
  {
    printf("Error: the computer vision kernel couldn't be initialized properly.\n");
    return 1;
  }
  if (!init_rad_kernel(rad_dict))
  {
    printf("Error: the radar kernel couldn't be initialized properly.\n");
    return 1;
  }
  if (!init_vit_kernel(vit_dict))
  {
    printf("Error: the Viterbi decoding kernel couldn't be initialized properly.\n");
    return 1;
  }

  /* We assume the vehicle starts in the following state:
   *  - Lane: center
   *  - Speed: 50 mph
   */
  vehicle_state.lane  = center;
  vehicle_state.speed = 50;
  DEBUG(printf("Vehicle starts with the following state: lane %u speed %.1f\n", vehicle_state.lane, vehicle_state.speed));
  /*** MAIN LOOP -- iterates until all the traces are fully consumed ***/
  #ifdef TIME
  	  int loop=0;
	  struct timeval stop, start;
  #endif
  while (!eof_trace_reader())
  {
    DEBUG(printf("Vehicle_State: Lane %u %s Speed %.1f\n", vehicle_state.lane, lane_names[vehicle_state.lane], vehicle_state.speed));
    /* The input trace contains the per-epoch (time-step) input data */
    read_next_trace_record(vehicle_state);

    /* The computer vision kernel performs object recognition on the
     * next image, and returns the corresponding label. 
     * This process takes place locally (i.e. within this car).
     */
    label = iterate_cv_kernel(vehicle_state);


    /* The radar kernel performs distance estimation on the next radar
     * data, and returns the estimated distance to the object.
     */
    distance = iterate_rad_kernel(vehicle_state);


    /* The Viterbi decoding kernel performs Viterbi decoding on the next
     * OFDM symbol (message), and returns the extracted message.
     * This message can come from another car (including, for example,
     * its 'pose') or from the infrastructure (like speed violation or
     * road construction warnings). For simplicity, we define a fix set
     * of message classes (e.g. car on the right, car on the left, etc.)
     */
    message = iterate_vit_kernel(vehicle_state);


    /* The plan_and_control() function makes planning and control decisions
     * based on the currently perceived information. It returns the new
     * vehicle state.
     */
    vehicle_state = plan_and_control(label, distance, message, vehicle_state);
    DEBUG(printf("New vehicle state: lane %u speed %.1f\n\n", vehicle_state.lane, vehicle_state.speed));
    
    #ifdef TIME  
          loop++;
          if (loop == 1) { 
  	  gettimeofday(&start, NULL);
	  }
    #endif	  
  }

  #ifdef TIME
  	gettimeofday(&stop, NULL);
  #endif 

  /* All the traces have been fully consumed. Quitting... */
  closeout_cv_kernel();
  closeout_rad_kernel();
  closeout_vit_kernel();

  #ifdef TIME
  	printf("Program run time in milliseconds %f\n", (double) (stop.tv_sec - start.tv_sec) * 1000 + (double) (stop.tv_usec - start.tv_usec) / 1000);
  #endif 
  printf("\nDone.\n");
  return 0;
}
