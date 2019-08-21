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

#ifndef _sim_environs_h
#define _sim_environs_h

/* Types definitions */
typedef float distance_t;
typedef enum {false, true} bool_t;
typedef enum {error, success} status_t;

/* Pre-defined labels used by the computer vision kernel */
typedef enum {
  myself   = -1,
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

/* Object state; includes lane, type of object, and speed (0, 1, 2, ...) */
typedef struct obj_struct {
  lane_t      lane;
  label_t     object;
  unsigned    speed;
  struct obj_struct* previous;  // The previous object in the list
  struct obj_struct* next;      // The next object in the list
  struct obj_struct* ahead_in_lane;
  struct obj_struct* behind_in_lane;  
} object_state_t;

/* Pre-defined messages used by the Viterbi decoding kernel */
/*  These now conform to version 0.4 of the specification   */
typedef enum {
  safe_to_move_right_or_left   = 0,
  safe_to_move_right_only      = 1,
  safe_to_move_left_only       = 2,
  unsafe_to_move_left_or_right = 3 
} message_t;


/* This is the "world" which consists of the (5) lanes (horizontal) and N vertical slots (distances) *
 *       "x"     0         1         2         3       4
 *   "y"   | LHazard |   Left  |  Middle |  Right  | RHazard |
 *         |---------|---------|---------|---------|---------|
 *    0    |         |         |         | Truck   |         |
 *    1    |         |         |         |         |         |
 *    2    |         |  Car    |         |         |         |
 *    3    |         |         |         |         |         |
 *    4    |         |         |         |         |         |
 *    5    |         |         |  Bike   |         |         |
 *    6    |         |         |         |         |         |
 *    7    |         |         |         |         |         |
 *    8    |         |         |         |         |         |
 *    9    |         |         |  Myself |         |         |
 *   10    |         |         |         |         |         |
 *   11    |         |  Pedest |         |         |         |
 *   12    |         |         |         |         |         |
 */

// Function/interface declarations
void print_object(object_state_t* st, int x, int y);
void init_sim_environs();
void iterate_sim_environs();
void visualize_world();

status_t init_vit_kernel(char* trace_filename);
message_t do_viterbi_work(message_t in_msg, bool_t debug);



#endif
