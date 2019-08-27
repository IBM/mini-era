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

#ifdef VERBOSE
 #define DEBUG(x) x
#else
 #define DEBUG(x)
#endif

/* Types definitions */
#define MAX_DISTANCE     500.0  // Max resolution distance of radar is < 500.0m
#define MAX_OBJECT_SIZE   10.0  // Max size of an object
#define MIN_OBJECT_DIST   MAX_OBJECT_SIZE

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
  unsigned    obj_id;
  label_t     object;
  lane_t      lane;
  unsigned    speed;
  distance_t  distance;
  distance_t  size; // Size of object in distance_t units
  
  struct obj_struct* previous;  // The previous object in the list
  struct obj_struct* next;      // The next object in the list
} object_state_t;

/* Pre-defined messages used by the Viterbi decoding kernel */
/*  These now conform to version 0.4 of the specification   */
typedef enum {
  safe_to_move_right_or_left   = 0,
  safe_to_move_right_only      = 1,
  safe_to_move_left_only       = 2,
  unsafe_to_move_left_or_right = 3 
} message_t;


/* The "world" consists of the (5) lanes (horizontal) and N vertical distances *
 *       "x"     0         1         2         3       4
 *   "y"   | LHazard |   Left  |  Middle |  Right  | RHazard |
 *         |---------|---------|---------|---------|---------|
 *    N    |         |         |         | Truck   |         |
 *  N-1    |         |         |         |         |         |
 *  N-2    |         |  Car    |         |         |         |
 *  N-3    |         |         |         |         |         |
 *  ...
 *    4    |         |         |         |         |         |
 *    3    |         |         |  Myself |         |         |
 *    2    |         |         |         |         |         |
 *    1    |         |  Pedest |         |         |         |
 *    0    |         |         |         |         |         |
 */

// Function/interface declarations
void print_object(object_state_t* st);
void init_sim_environs();
void iterate_sim_environs();
void visualize_world();

#endif
