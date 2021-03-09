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

#include "kernels_api.h"

/* Types definitions */
#define MAX_OBJECT_SIZE   50.0  // Max size of an object
#define MIN_OBJECT_DIST   MAX_OBJECT_SIZE

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


extern unsigned rand_seed;

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
void   print_object(object_state_t* st);
void   init_sim_environs();
bool_t iterate_sim_environs(vehicle_state_t vehicle_state);
void   visualize_world();

extern unsigned max_time_steps; // The max stime steps to simulate

extern bool_t all_obstacle_lanes_mode;

#endif

