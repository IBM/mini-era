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

#ifndef _kernels_api_h
#define _kernels_api_h

#ifdef VERBOSE
#define DEBUG(x) x
#else
#define DEBUG(x)
#endif

#define TIME

#include <stdio.h>
#include <stdlib.h>

/* Types definitions */
typedef float distance_t;
typedef enum {false, true} bool_t;
typedef enum {error, success} status_t;

/* These are some top-level defines for the dictionaries */

#include "radar/calc_fmcw_dist.h"

typedef struct {
  unsigned int return_id;
  float distance;
  float return_data[2 * RADAR_N];
} radar_dict_entry_t;

#include "viterbi/utils.h"
typedef struct {
  unsigned int msg_num;
  unsigned int msg_id;
  ofdm_param   ofdm_p;
  frame_param  frame_p;
  uint8_t      in_bits[MAX_ENCODED_BITS];
} vit_dict_entry_t;


/* These are GLOBAL and affect the underlying world, etc. */
#define NUM_LANES     5
#define NUM_OBJECTS   5
#define NUM_MESSAGES  4

#define MAX_OBJ_IN_LANE  16

#define MAX_DISTANCE     500.0  // Max resolution distance of radar is < 500.0m
#define DIST_STEP_SIZE    50.0
#define INF_DISTANCE     (MAX_DISTANCE + DIST_STEP_SIZE)
#define RADAR_BUCKET_DISTANCE  DIST_STEP_SIZE  // The radar is in steps of 50

/* These thresholds (in meters) are used by the plan_and_control()
 * function to make plan and control decisions.
 */
#define THRESHOLD_1 155.0
#define THRESHOLD_2 205.0
#define THRESHOLD_3 305.0

#define VIT_CLEAR_THRESHOLD  THRESHOLD_1


/* Pre-defined labels used by the computer vision kernel */
typedef enum {
  myself = -1,
  no_label = 0,
  car,
  truck,
  pedestrian,
  bicycle
} label_t;


/* The potential (horizontal) positions of any object (i.e. lane indications) */
typedef enum {
  lhazard = 0, 
  left, 
  center, 
  right,
  rhazard,
} lane_t;

/* These are some global type defines, etc. */
typedef struct
{
  bool_t active;
  lane_t lane;
  float speed;
} vehicle_state_t;



/* Pre-defined messages used by the Viterbi decoding kernel */
/*  These now conform to version 0.4 of the specification   */
typedef enum {
  safe_to_move_right_or_left   = 0,
  safe_to_move_right_only      = 1,
  safe_to_move_left_only       = 2,
  unsafe_to_move_left_or_right = 3,
  num_message_t
} message_t;


extern bool_t output_viz_trace;

extern char* lane_names[NUM_LANES];
extern char* message_names[NUM_MESSAGES];
extern char* object_names[NUM_OBJECTS];

extern unsigned vit_msgs_behavior;
  
extern unsigned total_obj; // Total non-'N' obstacle objects across all lanes this time step
extern unsigned obj_in_lane[NUM_LANES]; // Number of obstacle objects in each lane this time step (at least one, 'n')
extern unsigned lane_dist[NUM_LANES][MAX_OBJ_IN_LANE]; // The distance to each obstacle object in each lane
extern char     lane_obj[NUM_LANES][MAX_OBJ_IN_LANE]; // The type of each obstacle object in each lane

extern char  nearest_obj[NUM_LANES];
extern float nearest_dist[NUM_LANES];

extern unsigned hist_total_objs[NUM_LANES * MAX_OBJ_IN_LANE];

extern unsigned rand_seed;


typedef struct occupancy_map_struct {
  lane_t     my_lane;     // The lane and distance are a stand-in for map-observer's position
  unsigned   my_distance; // distance_t is float
  uint8_t    map[(int)MAX_DISTANCE][NUM_LANES];
} occupancy_map_t;
  
typedef struct mymap_input_struct {
  label_t obj;
  distance_t dist;
} mymap_input_t;

extern mymap_input_t     global_mymap_inputs[NUM_LANES];
extern occupancy_map_t   global_occupancy_map;
extern occupancy_map_t   global_other_maps[MAX_OBJ_IN_LANE * NUM_LANES];
extern unsigned          num_other_maps;

/* Input Trace Functions */
status_t init_trace_reader(char * tr_fn);
bool_t eof_trace_reader(void);
bool_t read_next_trace_record(vehicle_state_t vs);
void closeout_trace_reader(void);

/* Kernels initialization */
status_t init_cv_kernel(char* py_file, char* dict_fn);
status_t init_rad_kernel(char* dict_fn);
status_t init_vit_kernel(char* dict_fn);
status_t init_mymap_kernel(char* dict_fn);
status_t init_cbmap_kernel(char* dict_fn);


label_t run_object_classification(unsigned tr_val);
label_t iterate_cv_kernel(vehicle_state_t vs);
label_t execute_cv_kernel(label_t in_tr_val);
void    post_execute_cv_kernel(label_t tr_val, label_t d_object);

radar_dict_entry_t* iterate_rad_kernel(vehicle_state_t vs);
distance_t execute_rad_kernel(float * inputs);
void       post_execute_rad_kernel(distance_t tr_dist, distance_t dist);

vit_dict_entry_t* iterate_vit_kernel(vehicle_state_t vs);
message_t execute_vit_kernel(vit_dict_entry_t* trace_msg, int num_msgs);
void      post_execute_vit_kernel(message_t tr_msg, message_t dec_msg);

void  iterate_mymap_kernel(vehicle_state_t vs, mymap_input_t* map_near_obj);
void execute_mymap_kernel(vehicle_state_t vs, mymap_input_t* map_near_obj, occupancy_map_t* mymap);
void  post_execute_mymap_kernel();

void  iterate_cbmap_kernel(vehicle_state_t vs);
void execute_cbmap_kernel(vehicle_state_t vs, occupancy_map_t* the_occ_map, occupancy_map_t* cbmap_inputs, int num_inputs);
void  post_execute_cbmap_kernel();


vehicle_state_t plan_and_control(label_t, distance_t, message_t, vehicle_state_t);

// These routines are used for any finalk, end-of-run operations/output
void closeout_cv_kernel(void);
void closeout_rad_kernel(void);
void closeout_vit_kernel(void);
void closeout_mymap_kernel(void);
void closeout_cbmap_kernel(void);

#endif
