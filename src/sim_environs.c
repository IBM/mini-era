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
#include <stdlib.h>
#include "kernels_api.h"
#include "sim_environs.h"

/* These are structures, etc. used in the environment */

// This is the master list of all currently live objects in the world.
//  This is a sorted list (by distance) of objects per lane
unsigned global_object_id = 0;
object_state_t* the_objects[5];

// This represents my car.
object_state_t my_car;		

unsigned time_steps;            // The number of elapsed time steps
unsigned max_time_steps = 5000; // The max time steps to simulate (default to 5000)

// This controls whether we can have multiple obstacles in a lane at a time
bool_t   one_obstacle_per_lane = false; // false = unlimited

// These are to output the Visualizer trace
char vis_obj_ids[NUM_OBJECTS] = {'N', 'C', 'T', 'P', 'B'};

#define NEW_OBJ_THRESHOLD 97     // RAND > this to create new obstacle object

// These are a succession of checks:
#define NEW_OBJ_CAR_THRESHOLD    45   // RAND < this -- it is a car
#define NEW_OBJ_TRUCK_THRESHOLD  70   // RAND >= car and < this -- it is a truck
#define NEW_OBJ_BIKE_THRESHOLD   95   // RAND >= truck and < this, it is a bike (else a person)

#define NUM_CAR_SPEEDS      5
#define NUM_TRUCK_SPEEDS    4
#define NUM_BIKE_SPEEDS     3
#define NUM_PERSON_SPEEDS   2

unsigned car_speeds[NUM_CAR_SPEEDS]        = { 45, 40, 35, 30, 25 };  // The possible speeds
unsigned car_sp_thds[NUM_CAR_SPEEDS]       = { 15, 75, 90, 95, 100 }; // The thresholds for each speed (RAND: 0-99)

unsigned truck_speeds[NUM_TRUCK_SPEEDS]    = { 40, 35, 30, 25 };
unsigned truck_sp_thds[NUM_TRUCK_SPEEDS]   = { 50, 85, 95, 100 }; // The thresholds for each speed (RAND: 0-99)

unsigned bike_speeds[NUM_BIKE_SPEEDS]      = { 35, 30, 20 };
unsigned bike_sp_thds[NUM_BIKE_SPEEDS]     = { 70, 95, 100 }; // The thresholds for each speed (RAND: 0-99)

unsigned person_speeds[NUM_PERSON_SPEEDS]  = { 15, 10 };
unsigned person_sp_thds[NUM_PERSON_SPEEDS] = { 50, 100 }; // The thresholds for each speed (RAND: 0-99)

void
print_object(object_state_t* st) {
  printf(" Object # %u ", st->obj_id);
  switch(st->object) {
  case myself: printf("My_Car "); break;
  case no_label : printf("No_Label "); break;
  case car : printf("Car "); break;
  case truck : printf("Truck "); break;
  case pedestrian : printf("Person "); break;
  case bicycle : printf("Bike "); break;
  default: printf("ERROR "); 
  }
  printf("size %.1f in ", st->size);
  switch(st->lane) {
  case lhazard : printf(" L-Hazard "); break;
  case left : printf(" Left "); break;
  case center : printf(" Middle "); break;
  case right : printf(" Right "); break;
  case rhazard : printf(" R-Hazard "); break;
  default: printf("ERROR "); 
  }  
  printf("at distance %.1f speed %u\n", st->distance, st->speed);
}  

int    min_obst_lane;
int    max_obst_lane;


void
init_sim_environs()
{
  for (int i = 0; i < 5; i++) {
    the_objects[i] = NULL;
  }

  // Set up the default initial state of my car: Middle lane at medium speed.
  my_car.lane = center;
  my_car.object = myself;
  my_car.speed = 50;
  my_car.previous = NULL;	// not used for my_car
  my_car.next = NULL;		// not used for my_car

  time_steps = 0;

  srand(rand_seed);
  printf("Using rand seed: %u\n", rand_seed);

  if (all_obstacle_lanes_mode == true) {
    min_obst_lane  = 0;
    max_obst_lane = NUM_LANES;
  } else {
    // Obstacles are NOT in the far-left or far-right (Hazard) lanes
    min_obst_lane  = 1;
    max_obst_lane = (NUM_LANES - 1);
  }    
}


/* NOTES:
 * In this implementation we do a back-to-front pass for each lane...
 * This ensures that there is "open space" behind a car before it "drops back" into that space
 * BUT this only works (well) if the cars/objects all move at the same rate (as one another).  
 * IF we allow objects to have different speeds, then there are different relative motions, and 
 *  the objects must ALSO avoid one another (presumably by altering their speed)
 */

bool_t
iterate_sim_environs(vehicle_state_t vehicle_state) 
{
  DEBUG(printf("In iterate_sim_environments with %u time_steps vs %u max\n", time_steps, max_time_steps));
  if (time_steps == max_time_steps) {
    return false;
  }
  // Clear the global object-in-lane information state
  total_obj = 0;
  for (int i = 0; i < NUM_LANES; i++) {
    obj_in_lane[i] = 0;
    nearest_obj[i]  = 'N';
    nearest_dist[i] = INF_DISTANCE;
  }

  my_car.lane = vehicle_state.lane;
  my_car.speed = vehicle_state.speed;
  
  // For each lane in the world, advance the objects relative to My-Car
  for (int in_lane = min_obst_lane; in_lane < max_obst_lane; in_lane++) {
    // Iterate through the objects in the lane from farthest to closest
    // If this obstacle would move "past" (or "through") another obstacle,
    //   adjust that obstacle's speed (downward) and check it will not collide
    object_state_t* obj = the_objects[in_lane];
    float behind = MAX_DISTANCE;
    while (obj != NULL) {
      int delta_dist = my_car.speed - obj->speed;
      float new_dist  = (obj->distance - (1.0 * delta_dist));
      DEBUG(printf(" Lane %u ddist %d to new_dist %.1f for", in_lane, delta_dist, new_dist); print_object(obj)); 
      if (new_dist < 0) { // Object is "off the world" (we've passed it completely)
	DEBUG(printf("  new_dist < 0 --> drop object from environment\n");
	      printf("   OBJ :"); print_object(obj);
	      printf("   LIST:"); print_object(the_objects[in_lane]);
	      if (obj->next != NULL) { printf("   NEXT:"); print_object(obj->next); }
	      if (obj->previous != NULL) { printf("   PREVIOUS:"); print_object(obj->previous); }
	      );
	// Delete the object from the universe!
	if (obj->previous != NULL) {
	    obj->previous->next = obj->next;
	}
	if (obj->next != NULL) {
	  obj->next->previous = obj->previous;
	}
	if (the_objects[in_lane] == obj) {
	  the_objects[in_lane] = obj->next;
	}
      } else {
	if (new_dist > behind) { //obj->previous->distance >= obj->distance) {
	  // We would collide with the car behind us -- slow down
	  unsigned slower = (int)(new_dist - behind + 0.999);
	  obj->distance = behind;
	  obj->speed -= slower;
	  DEBUG(printf("  new_dist %.1f > %.1f behind value : speed drops to %u\n", new_dist, behind, obj->speed));
	} else {
	  obj->distance = new_dist;
	}
	behind = obj->distance - MIN_OBJECT_DIST;
	DEBUG(printf("  RESULT: Lane %u OBJ", in_lane); print_object(obj));
      }
      obj = obj->next; // move to the next object
    }
  }
  
  // Now determine for each major lane (i.e. Left, Middle, Right) 
  //   whether to add a new object or not...
  for (int in_lane = min_obst_lane; in_lane < max_obst_lane; in_lane++) {
    object_state_t * pobj = the_objects[in_lane];
    if ((pobj == NULL) ||
	(!one_obstacle_per_lane && (pobj->distance < (MAX_DISTANCE - MAX_OBJECT_SIZE - MIN_OBJECT_DIST))) ) {
      // There is space for a new object to enter
      int num = (rand() % (100)); // Return a value from [0,99]
      if (num > NEW_OBJ_THRESHOLD) {
        // Create a new object (car) and add it to the lane at position [in_lane][0]
        object_state_t* no_p = (object_state_t*)calloc(1, sizeof(object_state_t));
	no_p->obj_id = global_object_id++;
	no_p->lane = in_lane;
	//no_p->object = car; break;
	int objn = (rand() % 100); // Return a value from [0,99]
	int spdn = (rand() % 100); // Return a value from [0,99]
	if (objn < NEW_OBJ_CAR_THRESHOLD) {
	  no_p->object = car;
	  no_p->size =  5.0; // UNUSED?
	  for (int si = 0; si < NUM_CAR_SPEEDS; si++) {
	    if (spdn < car_sp_thds[si]) {
	      no_p->speed = car_speeds[si];
	      si = NUM_CAR_SPEEDS;
	    }
	  }
	} else if (objn < NEW_OBJ_TRUCK_THRESHOLD) { 
	  no_p->object = truck;
	  no_p->size =  5.0; // UNUSED?
	  for (int si = 0; si < NUM_TRUCK_SPEEDS; si++) {
	    if (spdn < truck_sp_thds[si]) {
	      no_p->speed = truck_speeds[si];
	      si = NUM_TRUCK_SPEEDS;
	    }
	  }
	} else if (objn < NEW_OBJ_BIKE_THRESHOLD) {
	  no_p->object = bicycle;
	  no_p->size =  5.0; // UNUSED?
	  for (int si = 0; si < NUM_BIKE_SPEEDS; si++) {
	    if (spdn < bike_sp_thds[si]) {
	      no_p->speed = bike_speeds[si];
	      si = NUM_BIKE_SPEEDS;
	    }
	  }
	}
	else {
	  no_p->object = pedestrian; 
	  no_p->size =  5.0; // UNUSED?
	  for (int si = 0; si < NUM_PERSON_SPEEDS; si++) {
	    if (spdn < person_sp_thds[si]) {
	      no_p->speed = person_speeds[si];
	      si = NUM_PERSON_SPEEDS;
	    }
	  }
	}
	/* switch(objn) {  */
	/* case 0: no_p->object = car;        no_p->speed = 40;  no_p->size =  5.0; break; */
	/* case 1: no_p->object = truck;      no_p->speed = 30;  no_p->size = 10.0; break; */
	/* case 2: no_p->object = pedestrian; no_p->speed = 10;  no_p->size =  2.0; break; */
	/* case 3: no_p->object = bicycle;    no_p->speed = 20;  no_p->size =  5.0; break; */
	/* } */
	no_p->distance = MAX_DISTANCE;
	no_p->previous = NULL;
	no_p->next = the_objects[in_lane];
	if (the_objects[in_lane] != NULL) {
	  the_objects[in_lane]->previous = no_p;
	}
	the_objects[in_lane] = no_p;
	DEBUG(printf("Adding"); print_object(no_p));
	//printf("Adding"); print_object(no_p);
      }
    }
  }

  // Now set up the global objects-in-lane, etc. indications.
  if (output_viz_trace) {
    printf("  VizTrace: %u,", vehicle_state.lane);
  }
  for (int in_lane = min_obst_lane; in_lane < max_obst_lane; in_lane++) {
    object_state_t* obj = the_objects[in_lane];
    int outputs_in_lane = 0;
    if (output_viz_trace && (in_lane > min_obst_lane)) {
      printf(",");
    }
    if (obj != NULL) {
      while (obj != NULL) {
	// Now add this to the lane_obj, etc.
	lane_obj[in_lane][obj_in_lane[in_lane]] = obj->object;
	nearest_obj[in_lane]  = vis_obj_ids[obj->object];
	nearest_dist[in_lane] = obj->distance;
	obj_in_lane[in_lane]++;
	if (output_viz_trace) {
	  if (outputs_in_lane > 0) { printf(" "); }
	  printf("%c:%u", vis_obj_ids[obj->object], (int)obj->distance);
	  outputs_in_lane++;       
	}
	obj = obj->next; // move to the next object
      }
    } else {
      if (output_viz_trace) {
	printf("N:%u", (int)INF_DISTANCE);
      }
    }
  }
  if (output_viz_trace) { printf("\n"); }
  DEBUG(visualize_world());
  time_steps++; // Iterate the count of time_steps so far
  return true;
}


void
visualize_world()
{
  // For each lane 
  for (int x = 0; x < 5; x++) {
    // List the objects in the lane
    object_state_t* pobj = the_objects[x];
    if (pobj != NULL) {
      printf("Lane %u has :", x);
      while (pobj != NULL) {
	print_object(pobj);
	pobj = pobj->next;
      }
    } else {
      printf("Lane %u is empty\n", x);
    }
  }
  printf("\n\n");
}


