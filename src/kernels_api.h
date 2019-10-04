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

// If you uncomment the #define VERBOSE then run generates debugging output
//#define VERBOSE
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

#define MAX_OBJ_IN_LANE  16

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
  unsafe_to_move_left_or_right = 3,
  num_messages
} message_t;

#include "calc_fmcw_dist.h"

typedef struct {
  unsigned int return_id;
  float distance;
  float return_data[2 * RADAR_N];
} radar_dict_entry_t;

#include "utils.h"

typedef struct {
  unsigned int msg_num;
  unsigned int msg_id;
  ofdm_param   ofdm_p;
  frame_param  frame_p;
  uint8_t      in_bits[MAX_ENCODED_BITS];
} vit_dict_entry_t;


extern char* lane_names[NUM_LANES+1];
extern char* message_names[NUM_MESSAGES+1];
extern char* object_names[NUM_OBJECTS+1];

extern unsigned vit_msgs_behavior;
  
extern unsigned total_obj; // Total non-'N' obstacle objects across all lanes this time step

/* Input Trace Functions */
status_t init_trace_reader(char * tr_fn);
bool_t eof_trace_reader(void);
bool_t read_next_trace_record(vehicle_state_t vs);
void closeout_trace_reader(void);

/* Kernels initialization */
status_t init_cv_kernel(char* dict_fn);
status_t init_rad_kernel(char* dict_fn);
status_t init_vit_kernel(char* dict_fn);


label_t run_object_classification(unsigned tr_val);

label_t iterate_cv_kernel(vehicle_state_t vs);

void execute_cv_kernel(label_t* in_tr_val, size_t in_tr_val_size,
		       label_t* out_label, size_t out_label_size);

void post_execute_cv_kernel(label_t tr_val, label_t cnn_val);

radar_dict_entry_t* iterate_rad_kernel(vehicle_state_t vs);
void execute_rad_kernel(float* inputs, size_t input_size_bytes,
			unsigned int N, unsigned int logn, int sign,
			distance_t * distance, size_t dist_size);
void post_execute_rad_kernel(distance_t tr_val, distance_t rad_val);

vit_dict_entry_t* iterate_vit_kernel(vehicle_state_t vs);
void execute_vit_kernel(ofdm_param* ofdm_ptr,    size_t ofdm_parms_size,
			frame_param* frame_ptr,  size_t frame_parm_size,
			uint8_t* input_bits,     size_t input_bits_size,
			char* out_msg_txt,       size_t out_msg_txt_size,
			message_t* out_message,  size_t out_message_size,
			int num_msgs_to_decode);

void post_execute_vit_kernel(message_t tr_msg, message_t dec_msg);

void plan_and_control(label_t* label,                 size_t size_label,
		      distance_t* distance,           size_t size_distance,
		      message_t* message,             size_t size_message,
		      vehicle_state_t* vehicle_state, size_t size_vehicle_state);

// These routines are used for any final, end-of-run operations/output
void closeout_cv_kernel(void);
void closeout_rad_kernel(void);
void closeout_vit_kernel(void);

#endif
