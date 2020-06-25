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

// NC
#include "base_types.h" 
#include "calc_fmcw_dist.h"
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

float MAX_OBJECT_SIZE; // Max size of an object
float MIN_OBJECT_DIST; // Minimum distance between obstacle objects
extern float IMPACT_DISTANCE; // Minimum distance at which an obstacle "impacts" MyCar (collision case)

unsigned NEW_OBJ_THRESHOLD;     // RAND > this to create new obstacle object
// These are a succession of checks:
unsigned NEW_OBJ_CAR_THRESHOLD;     // RAND < this -- it is a car
unsigned NEW_OBJ_TRUCK_THRESHOLD;   // RAND >= car and < this -- it is a truck
unsigned NEW_OBJ_BIKE_THRESHOLD;    // RAND >= truck and < this, it is a bike (else a person)

unsigned NUM_CAR_SPEEDS;
unsigned NUM_TRUCK_SPEEDS;
unsigned NUM_BIKE_SPEEDS;
unsigned NUM_PERSON_SPEEDS;

float*    car_speeds;
float*    truck_speeds;
float*    bike_speeds;
float*    person_speeds;

unsigned* car_sp_thds;
unsigned* truck_sp_thds;
unsigned* bike_sp_thds;
unsigned* person_sp_thds;

// These are defined in kernels_api but the world_description_file can alter their settings for a sim run.
float car_goal_speed = 50.0; 
float car_accel_rate = 15.0;
float car_decel_rate = 15.0;

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
  printf("at distance %.1f speed %.1f\n", st->distance, st->speed);
}  

int    min_obst_lane;
int    max_obst_lane;


status_t
init_sim_environs(char* wdesc_fn, vehicle_state_t* vehicle_state)
{
  DEBUG(printf("In init_sim_environs with world description file %s\n", wdesc_fn));
  // Read in the object images dictionary file
  FILE *wdescF = fopen(wdesc_fn, "r");
  if (!wdescF)
  {
    printf("Error: unable to open the world description file %s\n", wdesc_fn);
    return error;
  }

  if (fscanf(wdescF, "MAX_OBJECT_SIZE %f\n", &MAX_OBJECT_SIZE)) {
    printf("MAX_OBJECT_SIZE %.1f\n", MAX_OBJECT_SIZE);
  } else {
    printf("Error: unable to read MAX_OBJECT_SIZE from %s\n", wdesc_fn);
    return error;
  }

  if (fscanf(wdescF, "MIN_OBJECT_DIST %f\n", &MIN_OBJECT_DIST)) {
    printf("MIN_OBJECT_DIST %.1f\n", MIN_OBJECT_DIST);
  } else {
    printf("Error: unable to read MIN_OBJECT_DIST from %s\n", wdesc_fn);
    return error;
  }

  if (fscanf(wdescF, "IMPACT_DISTANCE %f\n", &IMPACT_DISTANCE)) {
    printf("IMPACT_DISTANCE %.1f\n", IMPACT_DISTANCE);
  } else {
    printf("Error: unable to read IMPACT_DISTANCE from %s\n", wdesc_fn);
    return error;
  }

  /* #define NEW_OBJ_THRESHOLD 97     // RAND > this to create new obstacle object */
  if (fscanf(wdescF, "NEW_OBJ_THRESHOLD %u\n", &NEW_OBJ_THRESHOLD)) {
    printf("NEW_OBJ_THRESHOLD %u\n", NEW_OBJ_THRESHOLD);
  } else {
    printf("Error: unable to read NEW_OBJ_THRESHOLD from %s\n", wdesc_fn);
    return error;
  }

  /* // These are a succession of checks: */
  /* #define NEW_OBJ_CAR_THRESHOLD    45   // RAND < this -- it is a car */
  /* #define NEW_OBJ_TRUCK_THRESHOLD  70   // RAND >= car and < this -- it is a truck */
  /* #define NEW_OBJ_BIKE_THRESHOLD   95   // RAND >= truck and < this, it is a bike (else a person) */
  if (fscanf(wdescF, "NEW_OBJ: CAR %u TRUCK %u BIKE %u\n", &NEW_OBJ_CAR_THRESHOLD, &NEW_OBJ_TRUCK_THRESHOLD, &NEW_OBJ_BIKE_THRESHOLD)) {
    printf("NEW_OBJ: CAR %u TRUCK %u BIKE %u\n", NEW_OBJ_CAR_THRESHOLD, NEW_OBJ_TRUCK_THRESHOLD, NEW_OBJ_BIKE_THRESHOLD);
  } else {
    printf("Error: unable to read NEW_OBJ CAR TRUCK and BIKE THRESHOLDS from %s\n", wdesc_fn);
    return error;
  }

  /* #define NUM_CAR_SPEEDS      5 */
  if (fscanf(wdescF, "NUM_CAR_SPEEDS %u\n", &NUM_CAR_SPEEDS)) {
    printf("NUM_CAR_SPEEDS %u\n", NUM_CAR_SPEEDS);
  } else {
    printf("Error: unable to read NUM_CAR_SPEEDS from %s\n", wdesc_fn);
    return error;
  }
  /* unsigned car_speeds[NUM_CAR_SPEEDS]        = { 45, 40, 35, 30, 25 };  // The possible speeds */
  /* unsigned car_sp_thds[NUM_CAR_SPEEDS]       = { 15, 75, 90, 95, 100 }; // The thresholds for each speed (RAND: 0-99) */
  car_speeds = (float *)calloc(NUM_CAR_SPEEDS, sizeof(float));
  if (car_speeds == NULL) {
    printf("Error: unable to allocate %u car_speeds array\n", NUM_CAR_SPEEDS);
    return error;
  }
  car_sp_thds = (unsigned *)calloc(NUM_CAR_SPEEDS, sizeof(unsigned));
  if (car_sp_thds == NULL) {
    printf("Error: unable to allocate %u car_sp_thds array\n", NUM_CAR_SPEEDS);
    return error;
  }
  for (int i = 0; i < NUM_CAR_SPEEDS; i++) {
    if (fscanf(wdescF, " CAR SPEED %f PROB %u\n", &(car_speeds[i]), &(car_sp_thds[i]))) {
      printf(" CAR_SPEED %.1f PROB %u\n", car_speeds[i], car_sp_thds[i]);
    } else {
      printf("Error: unable to read CAR_SPEEDS %u from %s\n", i, wdesc_fn);
      return error;
    }
  }
    
  /* #define NUM_TRUCK_SPEEDS    4 */
  if (fscanf(wdescF, "NUM_TRUCK_SPEEDS %u\n", &NUM_TRUCK_SPEEDS)) {
    printf("NUM_TRUCK_SPEEDS %u\n", NUM_TRUCK_SPEEDS);
  } else {
    printf("Error: unable to read NUM_TRUCK_SPEEDS from %s\n", wdesc_fn);
    return error;
  }
  /* unsigned truck_speeds[NUM_TRUCK_SPEEDS]    = { 40, 35, 30, 25 }; */
  /* unsigned truck_sp_thds[NUM_TRUCK_SPEEDS]   = { 50, 85, 95, 100 }; // The thresholds for each speed (RAND: 0-99) */
  truck_speeds = (float *)calloc(NUM_TRUCK_SPEEDS, sizeof(float));
  if (truck_speeds == NULL) {
    printf("Error: unable to allocate %u truck_speeds array\n", NUM_TRUCK_SPEEDS);
    return error;
  }
  truck_sp_thds = (unsigned *)calloc(NUM_TRUCK_SPEEDS, sizeof(unsigned));
  if (truck_sp_thds == NULL) {
    printf("Error: unable to allocate %u truck_sp_thds array\n", NUM_TRUCK_SPEEDS);
    return error;
  }
  for (int i = 0; i < NUM_TRUCK_SPEEDS; i++) {
    if (fscanf(wdescF, " TRUCK SPEED %f PROB %u\n", &(truck_speeds[i]), &(truck_sp_thds[i]))) {
      printf(" TRUCK_SPEED %.1f PROB %u\n", truck_speeds[i], truck_sp_thds[i]);
    } else {
      printf("Error: unable to read TRUCK_SPEEDS %u from %s\n", i, wdesc_fn);
      return error;
    }
  }

  /* #define NUM_BIKE_SPEEDS     3 */
  if (fscanf(wdescF, "NUM_BIKE_SPEEDS %u\n", &NUM_BIKE_SPEEDS)) {
    printf("NUM_BIKE_SPEEDS %u\n", NUM_BIKE_SPEEDS);
  } else {
    printf("Error: unable to read NUM_BIKE_SPEEDS from %s\n", wdesc_fn);
    return error;
  }
  /* unsigned bike_speeds[NUM_BIKE_SPEEDS]      = { 35, 30, 20 }; */
  /* unsigned bike_sp_thds[NUM_BIKE_SPEEDS]     = { 70, 95, 100 }; // The thresholds for each speed (RAND: 0-99) */
  bike_speeds = (float *)calloc(NUM_BIKE_SPEEDS, sizeof(float));
  if (bike_speeds == NULL) {
    printf("Error: unable to allocate %u bike_speeds array\n", NUM_BIKE_SPEEDS);
    return error;
  }
  bike_sp_thds = (unsigned *)calloc(NUM_BIKE_SPEEDS, sizeof(unsigned));
  if (bike_sp_thds == NULL) {
    printf("Error: unable to allocate %u bike_sp_thds array\n", NUM_BIKE_SPEEDS);
    return error;
  }
  for (int i = 0; i < NUM_BIKE_SPEEDS; i++) {
    if (fscanf(wdescF, " BIKE SPEED %f PROB %u\n", &(bike_speeds[i]), &(bike_sp_thds[i]))) {
      printf(" BIKE_SPEED %.1f PROB %u\n", bike_speeds[i], bike_sp_thds[i]);
    } else {
      printf("Error: unable to read BIKE_SPEEDS %u from %s\n", i, wdesc_fn);
      return error;
    }
  }

  /* #define NUM_PERSON_SPEEDS   2 */
  if (fscanf(wdescF, "NUM_PERSON_SPEEDS %u\n", &NUM_PERSON_SPEEDS)) {
    printf("NUM_PERSON_SPEEDS %u\n", NUM_PERSON_SPEEDS);
  } else {
    printf("Error: unable to read NUM_PERSON_SPEEDS from %s\n", wdesc_fn);
    return error;
  }
  /* unsigned person_speeds[NUM_PERSON_SPEEDS]  = { 15, 10 }; */
  /* unsigned person_sp_thds[NUM_PERSON_SPEEDS] = { 50, 100 }; // The thresholds for each speed (RAND: 0-99) */
  person_speeds = (float *)calloc(NUM_PERSON_SPEEDS, sizeof(float));
  if (person_speeds == NULL) {
    printf("Error: unable to allocate %u person_speeds array\n", NUM_PERSON_SPEEDS);
    return error;
  }
  person_sp_thds = (unsigned *)calloc(NUM_PERSON_SPEEDS, sizeof(unsigned));
  if (person_sp_thds == NULL) {
    printf("Error: unable to allocate %u person_sp_thds array\n", NUM_PERSON_SPEEDS);
    return error;
  }
  for (int i = 0; i < NUM_PERSON_SPEEDS; i++) {
    if (fscanf(wdescF, " PERSON SPEED %f PROB %u\n", &(person_speeds[i]), &(person_sp_thds[i]))) {
      printf(" PERSON_SPEED %.1f PROB %u\n", person_speeds[i], person_sp_thds[i]);
    } else {
      printf("Error: unable to read PERSON_SPEEDS %u from %s\n", i, wdesc_fn);
      return error;
    }
  }

  // Set up the default initial state of my car: Middle lane at medium speed.
  my_car.lane = vehicle_state->lane;
  my_car.object = myself;
  my_car.speed = vehicle_state->speed;
  my_car.previous = NULL;	// not used for my_car
  my_car.next = NULL;		// not used for my_car

  // My Car parameters:
  if (fscanf(wdescF, "MY_CAR GOAL SPEED %f\n", &(car_goal_speed))) {
    printf("MY_CAR GOAL SPEED %.1f\n", car_goal_speed);
  } else {
    printf("Error: unable to read MY_CAR GOAL SPEED from %s\n", wdesc_fn);
    return error;
  }
  if (fscanf(wdescF, "MY_CAR ACCEL RATE %f\n", &(car_accel_rate))) {
    printf("MY_CAR ACCEL RATE %.1f\n", car_accel_rate);
  } else {
    printf("Error: unable to read MY_CAR ACCEL RATE from %s\n", wdesc_fn);
    return error;
  }
  if (fscanf(wdescF, "MY_CAR DECEL RATE %f\n", &(car_decel_rate))) {
    printf("MY_CAR DECEL RATE %.1f\n", car_decel_rate);
  } else {
    printf("Error: unable to read MY_CAR DECEL RATE from %s\n", wdesc_fn);
    return error;
  }

  // Starting conditions for My-Car
  if (fscanf(wdescF, "MY_CAR LANE %u SPEED %f\n", &(my_car.lane), &(my_car.speed))) {
    printf("MY_CAR LANE %u SPEED %.1f\n", my_car.lane, my_car.speed);
    vehicle_state->lane  = my_car.lane;
    vehicle_state->speed = my_car.speed;    
  } else {
    printf("Error: unable to read MY_CAR LANE and SPEED from %s\n", wdesc_fn);
    return error;
  }
  printf("\n");
  
  // Initialize everything else...
  for (int i = 0; i < 5; i++) {
    the_objects[i] = NULL;
  }

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
  return success;
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
      if ((new_dist < 0) || (new_dist > MAX_DISTANCE)) { // Object is "off the world" (we've passed it completely)
	DEBUG(printf("  new_dist < 0 or > MAX_DIST --> drop object from environment\n");
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
	  DEBUG(printf("  new_dist %.1f > %.1f behind value : speed drops to %.1f\n", new_dist, behind, obj->speed));
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
	//no_p->lane = in_lane;
	no_p->lane = static_cast<lane_t>(in_lane); // NC
	//no_p->object = car; break;
	int objn = (rand() % 100); // Return a value from [0,99]
	int spdn = (rand() % 100); // Return a value from [0,99]
	if (objn < NEW_OBJ_CAR_THRESHOLD) {
	  no_p->object = car;
	  no_p->size =  MAX_OBJECT_SIZE; // 5.0; // UNUSED?
	  for (int si = 0; si < NUM_CAR_SPEEDS; si++) {
	    if (spdn < car_sp_thds[si]) {
	      no_p->speed = car_speeds[si];
	      si = NUM_CAR_SPEEDS;
	    }
	  }
	} else if (objn < NEW_OBJ_TRUCK_THRESHOLD) { 
	  no_p->object = truck;
	  no_p->size =  MAX_OBJECT_SIZE; // 5.0; // UNUSED?
	  for (int si = 0; si < NUM_TRUCK_SPEEDS; si++) {
	    if (spdn < truck_sp_thds[si]) {
	      no_p->speed = truck_speeds[si];
	      si = NUM_TRUCK_SPEEDS;
	    }
	  }
	} else if (objn < NEW_OBJ_BIKE_THRESHOLD) {
	  no_p->object = bicycle;
	  no_p->size =  MAX_OBJECT_SIZE; // 5.0; // UNUSED?
	  for (int si = 0; si < NUM_BIKE_SPEEDS; si++) {
	    if (spdn < bike_sp_thds[si]) {
	      no_p->speed = bike_speeds[si];
	      si = NUM_BIKE_SPEEDS;
	    }
	  }
	}
	else {
	  no_p->object = pedestrian; 
	  no_p->size =  MAX_OBJECT_SIZE; // 5.0; // UNUSED?
	  for (int si = 0; si < NUM_PERSON_SPEEDS; si++) {
	    if (spdn < person_sp_thds[si]) {
	      no_p->speed = person_speeds[si];
	      si = NUM_PERSON_SPEEDS;
	    }
	  }
	}
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
    if (!vehicle_state.active) {
      printf("  VizTrace: %d,", -vehicle_state.lane);
    } else {
      printf("  VizTrace: %d,", vehicle_state.lane);
    }      
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
	lane_obj[in_lane][obj_in_lane[in_lane]] = vis_obj_ids[obj->object];
	lane_dist[in_lane][obj_in_lane[in_lane]] = obj->distance;
	nearest_obj[in_lane]  = vis_obj_ids[obj->object];
	nearest_dist[in_lane] = obj->distance;
	obj_in_lane[in_lane]++;
	if (output_viz_trace) {
	  if (outputs_in_lane > 0) { printf(" "); }
	  printf("%c:%u", vis_obj_ids[obj->object], (int)obj->distance);
	  outputs_in_lane++;       
	}
	total_obj++;
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


