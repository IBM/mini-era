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

/* Types definitions */
typedef float distance_t;
typedef enum {false, true} bool_t;
typedef enum {error, success} status_t;

/* These are GLOBAL and affect the underlying world, etc. */
#define NUM_LANES     5
#define NUM_OBJECTS   5
#define NUM_MESSAGES  4

#define INF_DISTANCE           550 // radar tops out at ~500m 
#define RADAR_BUCKET_DISTANCE  50  // The radar is in steps of 50

/* These thresholds (in meters) are used by the plan_and_control()
 * function to make plan and control decisions.
 */
#define THRESHOLD_1 155.0
#define THRESHOLD_2 205.0
#define THRESHOLD_3 305.0

#define VIT_CLEAR_THRESHOLD  THRESHOLD_1


/* These are some global type defines, etc. */
typedef struct
{
  enum {lhazard, left, center, right, rhazard} lane;
  float speed;
} vehicle_state_t;


/* Pre-defined labels used by the computer vision kernel */
typedef enum {
  no_label,
  bus,
  car,
  pedestrian,
  truck
} label_t;


/* Pre-defined messages used by the Viterbi decoding kernel */
/*  These now conform to version 0.4 of the specification   */
typedef enum {
  safe_to_move_right_or_left   = 0,
  safe_to_move_right_only      = 1,
  safe_to_move_left_only       = 2,
  unsafe_to_move_left_or_right = 3 
} message_t;

extern char* lane_names[NUM_LANES];
extern char* message_names[NUM_MESSAGES];
extern char* object_names[NUM_OBJECTS];

extern unsigned vit_msgs_behavior;
  


/* Input Trace Functions */
status_t init_trace_reader(char * tr_fn);
bool_t eof_trace_reader();
bool_t read_next_trace_record(vehicle_state_t vs);
void closeout_trace_reader();

/* Kernels initialization */
status_t init_cv_kernel(char* py_file, char* dict_fn);
status_t init_rad_kernel(char* dict_fn);
status_t init_vit_kernel(char* dict_fn);


label_t run_object_classification(unsigned tr_val);
label_t iterate_cv_kernel(vehicle_state_t vs);
distance_t iterate_rad_kernel(vehicle_state_t vs);
message_t iterate_vit_kernel(vehicle_state_t vs);

vehicle_state_t plan_and_control(label_t, distance_t, message_t, vehicle_state_t);

// These routines are used for any finalk, end-of-run operations/output
void closeout_cv_kernel();
void closeout_rad_kernel();
void closeout_vit_kernel();

#endif
