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

/* Types definitions */
typedef float distance_t;
typedef enum {false, true} bool_t;
typedef enum {error, success} status_t;

typedef struct
{
  enum {left, center, right} lane;
  float speed;
} vehicle_state_t;

/* Pre-defined labels used by the computer vision kernel */
typedef enum {
  no_label,
  car,
  truck,
  pedestrian,
  bicycle
} label_t;

/* Pre-defined messages used by the Viterbi decoding kernel */
/*  These now conform to version 0.4 of the specification   */
typedef enum {
  safe_to_move_right_or_left   = 0,
  safe_to_move_right_only      = 1,
  safe_to_move_left_only       = 2,
  unsafe_to_move_left_or_right = 3 
} message_t;

/* These thresholds (in meters) are used by the plan_and_control()
 * function to make plan and control decisions.
 */
#define THRESHOLD_1 250
#define THRESHOLD_2 400
#define THRESHOLD_3 500

/* Kernels initialization */
status_t init_cv_kernel(char*, char*);
status_t init_rad_kernel(char*);
status_t init_vit_kernel(char*);

bool_t eof_cv_kernel();
bool_t eof_rad_kernel();
bool_t eof_vit_kernel();

label_t iterate_cv_kernel(vehicle_state_t vs);
distance_t iterate_rad_kernel(vehicle_state_t vs);
message_t iterate_vit_kernel(vehicle_state_t vs);

vehicle_state_t plan_and_control(label_t, distance_t, message_t, vehicle_state_t);

#endif
