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
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

#include "kernels_api.h"
#include "sim_environs.h"

#define TIME
char * cv_dict   = "traces/objects_dictionary.dfn";
char * rad_dict  = "traces/radar_dictionary.dfn";
char * vit_dict  = "traces/vit_dictionary.dfn";
char * mymap_dict = "traces/mymap_dictionary.dfn";
char * cbmap_dict = "traces/cbmap_dictionary.dfn";

bool_t all_obstacle_lanes_mode = false;
unsigned time_step;

void print_usage(char * pname) {
  printf("Usage: %s <OPTIONS>\n", pname);
  printf(" OPTIONS:\n");
  printf("    -h         : print this helpfule usage info\n");
  printf("    -o         : print the Visualizer output traace information during the run\n");
#ifdef USE_SIM_ENVIRON
  printf("    -s <N>     : Sets the max number of time steps to simulate\n");
  printf("    -r <N>     : Sets the rand random number seed to N\n");
  printf("    -A         : Allow obstacle vehciles in All lanes (otherwise not in left or right hazard lanes)\n");
  printf("    -W <wfile> : defines the world environment parameters description file <wfile> to use\n");
#else
  printf("    -t <trace> : defines the input trace file <trace> to use\n");
#endif
  printf("    -v <N>     : defines Viterbi messaging behavior:\n");
  printf("               :      0 = One short message per time step\n");
  printf("               :      1 = One long  message per time step\n");
  printf("               :      2 = One short message per obstacle per time step\n");
  printf("               :      3 = One long  message per obstacle per time step\n");
  printf("               :      4 = One short msg per obstacle + 1 per time step\n");
  printf("               :      5 = One long  msg per obstacle + 1 per time step\n");
}


int main(int argc, char *argv[])
{
  vehicle_state_t vehicle_state;
  label_t label;
  distance_t distance;
  message_t message;
#ifdef USE_SIM_ENVIRON
  char* world_desc_file_name = "default_world.desc";
#else
  char* trace_file; 
#endif
  int opt; 
      
  // put ':' in the starting of the 
  // string so that program can  
  // distinguish between '?' and ':'
  while((opt = getopt(argc, argv, ":hAot:v:s:r:W:")) != -1) {  
    switch(opt) {  
    case 'h':
      print_usage(argv[0]);
      exit(0);
    case 'A':
      all_obstacle_lanes_mode = true;
      break;
    case 'o':
      output_viz_trace = true;
      break;
    case 's':
#ifdef USE_SIM_ENVIRON
      max_time_steps = atoi(optarg);
      printf("Using %u maximum time steps (simulation)\n", max_time_steps);
#endif
      break;
    case 'r':
#ifdef USE_SIM_ENVIRON
      rand_seed = atoi(optarg);
#endif
      break;
    case 't':
#ifndef USE_SIM_ENVIRON
      trace_file = optarg;
      printf("Using trace file: %s\n", trace_file);
#endif
      break;
    case 'v':
      vit_msgs_behavior = atoi(optarg);
      printf("Using viterbi behavior %u\n", vit_msgs_behavior);
      break;
    case 'W':
#ifdef USE_SIM_ENVIRON
      world_desc_file_name = optarg;
      printf("Using world description file: %s\n", world_desc_file_name);
#endif
      break;
    case ':':
      printf("option needs a value\n");
      break;  
    case '?':
      printf("unknown option: %c\n", optopt); 
    break;
    }  
  }  
      
  // optind is for the extra arguments 
  // which are not parsed 
  for(; optind < argc; optind++){      
    printf("extra arguments: %s\n", argv[optind]);  
  } 
  
  
  /* We plan to use three separate trace files to drive the three different kernels
   * that are part of mini-ERA (CV, radar, Viterbi). All these three trace files
   * are required to have the same base name, using the file extension to indicate
   * which kernel the trace corresponds to (cv, rad, vit).
   */
  /* if (argc != 2) */
  /* { */
  /*   printf("Usage: %s <trace_basename>\n\n", argv[0]); */
  /*   printf("Where <trace_basename> is the basename of the trace files to load:\n"); */
  /*   printf("  <trace_basename>.cv  : trace to feed the computer vision kernel\n"); */
  /*   printf("  <trace_basename>.rad : trace to feed the radar (FFT-1D) kernel\n"); */
  /*   printf("  <trace_basename>.vit : trace to feed the Viterbi decoding kernel\n"); */

  /*   return 1; */
  /* } */


  char cv_py_file[] = "../cv/keras_cnn/lenet.py";

#ifndef USE_SIM_ENVIRON
  /* Trace filename construction */
  /* char * trace_file = argv[1]; */
  printf("Input trace file: %s\n", trace_file);

  /* Trace Reader initialization */
  if (!init_trace_reader(trace_file))
  {
    printf("Error: the trace reader couldn't be initialized properly.\n");
    return 1;
  }
#endif
  
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
  if (!init_mymap_kernel(mymap_dict))
  {
    printf("Error: the Viterbi decoding kernel couldn't be initialized properly.\n");
    return 1;
  }
  if (!init_cbmap_kernel(cbmap_dict))
  {
    printf("Error: the Viterbi decoding kernel couldn't be initialized properly.\n");
    return 1;
  }

  /* We assume the vehicle starts in the following state:
   *  - Lane: center
   *  - Speed: 50 mph
   */
  vehicle_state.active  = true;
  vehicle_state.lane    = center;
  vehicle_state.speed   = 50;
  DEBUG(printf("Vehicle starts with the following state: active: %u lane %u speed %.1f\n", vehicle_state.active, vehicle_state.lane, vehicle_state.speed));

  #ifdef USE_SIM_ENVIRON
  // In simulation mode, we could start the main car is a different state (lane, speed)
  init_sim_environs(world_desc_file_name, &vehicle_state);
  #endif
  
/*** MAIN LOOP -- iterates until all the traces are fully consumed ***/
  time_step = 0;
  #ifdef TIME
  struct timeval stop, start;
  #endif

  /* The input trace contains the per-epoch (time-step) input data */
#ifdef USE_SIM_ENVIRON
  DEBUG(printf("\n\nTime Step %d\n", time_step));  
  while (iterate_sim_environs(vehicle_state))
#else //TRACE DRIVEN MODE
  read_next_trace_record(vehicle_state);
  while (!eof_trace_reader())
#endif
  {
    DEBUG(printf("Vehicle_State: Lane %u %s Speed %.1f\n", vehicle_state.lane, lane_names[vehicle_state.lane], vehicle_state.speed));

    /* The computer vision kernel performs object recognition on the
     * next image, and returns the corresponding label. 
     * This process takes place locally (i.e. within this car).
     */
    label_t cv_tr_label = iterate_cv_kernel(vehicle_state);

    /* The radar kernel performs distance estimation on the next radar
     * data, and returns the estimated distance to the object.
     */
    radar_dict_entry_t* rdentry_p = iterate_rad_kernel(vehicle_state);
    distance_t rdict_dist = rdentry_p->distance;
    float * ref_in = rdentry_p->return_data;
    float radar_inputs[2*RADAR_N];
    for (int ii = 0; ii < 2*RADAR_N; ii++) {
      radar_inputs[ii] = ref_in[ii];
    }

    /* The Viterbi decoding kernel performs Viterbi decoding on the next
     * OFDM symbol (message), and returns the extracted message.
     * This message can come from another car (including, for example,
     * its 'pose') or from the infrastructure (like speed violation or
     * road construction warnings). For simplicity, we define a fix set
     * of message classes (e.g. car on the right, car on the left, etc.)
     */
    vit_dict_entry_t* vdentry_p = iterate_vit_kernel(vehicle_state);

    // Here we will simulate multiple cases, based on global vit_msgs_behavior
    int num_vit_msgs = 1;   // the number of messages to send this time step (1 is default) 
    switch(vit_msgs_behavior) {
    case 2: num_vit_msgs = total_obj; break;
    case 3: num_vit_msgs = total_obj; break;
    case 4: num_vit_msgs = total_obj + 1; break;
    case 5: num_vit_msgs = total_obj + 1; break;
    }

    iterate_mymap_kernel(vehicle_state, global_mymap_inputs); // This sets up my occupancy map inputs
    iterate_cbmap_kernel(vehicle_state);                      // This sets up other-vehicle's occupancy maps

    // EXECUTE the kernels using the now known inputs 
    label = execute_cv_kernel(cv_tr_label);
    distance = execute_rad_kernel(radar_inputs);
    message = execute_vit_kernel(vdentry_p, num_vit_msgs);

    execute_mymap_kernel(vehicle_state, global_mymap_inputs, &global_occupancy_map); // This builds the_occupany_map from my inputs and mmap inputs
    execute_cbmap_kernel(vehicle_state, &global_occupancy_map, global_other_maps, num_other_maps); // This builds on the_occupany_map using other maps

    // POST-EXECUTE each kernels to gather stats, etc.
    post_execute_cv_kernel(cv_tr_label, label);
    post_execute_rad_kernel(rdict_dist, distance);
    for (int mi = 0; mi < num_vit_msgs; mi++) {
      post_execute_vit_kernel(vdentry_p->msg_id, message);
    }
    post_execute_mymap_kernel();
    post_execute_cbmap_kernel();
    
    /* The plan_and_control() function makes planning and control decisions
     * based on the currently perceived information. It returns the new
     * vehicle state.
     */
    vehicle_state = plan_and_control(label, distance, message, vehicle_state);
    DEBUG(printf("New vehicle state: lane %u speed %.1f\n\n", vehicle_state.lane, vehicle_state.speed));

    #ifdef TIME  
    time_step++;
    if (time_step == 1) { 
      gettimeofday(&start, NULL);
    }
    #endif	  

    #ifndef USE_SIM_ENVIRON
    read_next_trace_record(vehicle_state);
    #endif
  }

  #ifdef TIME
  	gettimeofday(&stop, NULL);
  #endif 

  /* All the traces have been fully consumed. Quitting... */
  closeout_cv_kernel();
  closeout_rad_kernel();
  closeout_vit_kernel();
  closeout_mymap_kernel();
  closeout_cbmap_kernel();

  #ifdef TIME
  printf("Program run time in milliseconds %f\n", (double) (stop.tv_sec - start.tv_sec) * 1000 + (double) (stop.tv_usec - start.tv_usec) / 1000);
  #endif 
  printf("\nDone.\n");
  return 0;
}
