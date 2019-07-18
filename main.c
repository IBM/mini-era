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
  char cv_trace[strlen(argv[1]+5)];
  char rad_trace[strlen(argv[1]+5)];
  char vit_trace[strlen(argv[1]+5)];
  strcpy(cv_trace,  argv[1]);
  strcpy(rad_trace, argv[1]);
  strcpy(vit_trace, argv[1]);
  strcat(cv_trace,  ".cv");
  strcat(rad_trace, ".rad");
  strcat(vit_trace, ".vit");

  printf("Compuer vision trace file: %s\n", cv_trace);
  printf("Radar trace file: %s\n", rad_trace);
  printf("Viterbi decoding trace file: %s\n", vit_trace);


  /* Kernels initialization */
  if (!init_cv_kernel(cv_trace))
  {
    printf("Error: the computer vision kernel couldn't be initialized properly.\n");
    return 1;
  }
  if (!init_rad_kernel(rad_trace))
  {
    printf("Error: the radar kernel couldn't be initialized properly.\n");
    return 1;
  }
  if (!init_vit_kernel(vit_trace))
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

  /*** MAIN LOOP -- iterates until all the traces are fully consumed ***/
  while (!eof_cv_kernel() || !eof_rad_kernel() || !eof_vit_kernel())
  {

    /* The computer vision kernel performs object recognition on the
     * next image in the input trace, and returns the corresponding
     * label. This process takes place locally (i.e. within this car).
     */
    label = iterate_cv_kernel();


    /* The radar kernel performs distance estimation on the next radar
     * data in the input trace, and returns the estimated distance to
     * the object.
     */
    distance = iterate_rad_kernel();


    /* The Viterbi decoding kernel performs Viterbi decoding on the next
     * OFDM symbol in the input trace, and returns the extracted message.
     * This message can come from another car (including, for example,
     * its 'pose') or from the infrastructure (like speed violation or
     * road construction warnings). For simplicity, we define a fix set
     * of message classes (e.g. car on the right, car on the left, etc.)
     */
    message = iterate_vit_kernel();


    /* The plan_and_control() function makes planning and control decisions
     * based on the currently perceived information. It returns the new
     * vehicle state.
     */
    vehicle_state = plan_and_control(label, distance, message, vehicle_state);

  }

  /* All the traces have been fully consumed. Quitting... */
  return 0;
}
