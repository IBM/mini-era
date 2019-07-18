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
typedef enum {
  no_message,
  car_on_right,
  car_on_left,
  car_behind,
  car_in_front,
  speed_violation,
  constuction_ahead
} message_t;

/* These thresholds (in meters) are used by the plan_and_control()
 * function to make plan and control decisions.
 */
#define THRESHOLD_1 10
#define THRESHOLD_2 30
#define THRESHOLD_3 50

/* Kernels initialization */
status_t init_cv_kernel(char*);
status_t init_rad_kernel(char*);
status_t init_vit_kernel(char*);

bool_t eof_cv_kernel();
bool_t eof_rad_kernel();
bool_t eof_vit_kernel();

label_t iterate_cv_kernel();
distance_t iterate_rad_kernel();
message_t iterate_vit_kernel();

vehicle_state_t plan_and_control(label_t, distance_t, message_t, vehicle_state_t);

#endif
