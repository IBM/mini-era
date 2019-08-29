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

#include "sim_environs.h"

/* These are structures, etc. used in the environment */

// This is the master list of all currently live objects in the world.
//  This is a sorted list (by distance) of objects per lane
unsigned global_object_id = 0;
object_state_t* the_objects[5];

// This represents my car.
object_state_t my_car;		

// This controls whether we can have multiple obstacles in a lane at a time
bool_t   one_obstacle_per_lane = true; // false = unlimited

#define NEW_OBJ_THRESHOLD 90     // RAND > this to create new obstacle object

// These are a succession of checks:
#define NEW_OBJ_CAR_THRESHOLD    45   // RAND < this -- it is a car
#define NEW_OBJ_TRUCK_THRESHOLD  70   // RAND >= car and < this -- it is a truck
#define NEW_OBJ_BIKE_THRESHOLD   95   // RAND >= truck and < this, it is a bike (else a person)

#define NUM_CAR_SPEEDS      5
#define NUM_TRUCK_SPEEDS    4
#define NUM_BIKE_SPEEDS     3
#define NUM_PERSON_SPEEDS   2

unsigned car_speeds[NUM_CAR_SPEEDS]        = { 45, 40, 35, 30, 25 };  // The possible speeds
unsigned car_sp_thds[NUM_CAR_SPEEDS]       = { 15, 65, 85, 95, 100 }; // The thresholds for each speed (RAND: 0-99)

unsigned truck_speeds[NUM_TRUCK_SPEEDS]    = { 40, 35, 30, 25 };
unsigned truck_sp_thds[NUM_TRUCK_SPEEDS]   = { 20, 65, 95, 100 }; // The thresholds for each speed (RAND: 0-99)

unsigned bike_speeds[NUM_BIKE_SPEEDS]      = { 30, 25, 20 };
unsigned bike_sp_thds[NUM_BIKE_SPEEDS]     = { 30, 75, 100 }; // The thresholds for each speed (RAND: 0-99)

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
}


/* NOTES:
 * In this implementation we do a back-to-front pass for each lane...
 * This ensures that there is "open space" behind a car before it "drops back" into that space
 * BUT this only works (well) if the cars/objects all move at the same rate (as one another).  
 * IF we allow objects to have different speeds, then there are different relative motions, and 
 *  the objects must ALSO avoid one another (presumably by altering their speed)
 */

void
iterate_sim_environs() 
{
  // For each lane in the world
  for (int x = 0; x < 5; x++) {
    // Iterate through the objects in the lane from farthest to closest
    // If this obstacle would move "past" (or "through") another obstacle,
    //   adjust that obstacle's speed (downward) and check it will not collide
    object_state_t* obj = the_objects[x];
    float behind = MAX_DISTANCE;
    while (obj != NULL) {
      int delta_dist = my_car.speed - obj->speed;
      float new_dist  = (obj->distance - (1.0 * delta_dist));
      DEBUG(printf(" Lane %u ddist %d to new_dist %.1f for", x, delta_dist, new_dist); print_object(obj)); 
      if (new_dist < 0) { // Object is "off the world" (we've passed it completely)
	DEBUG(printf("  new_dist < 0 --> drop object from environment\n");
	      printf("   OBJ :"); print_object(obj);
	      printf("   LIST:"); print_object(the_objects[x]);
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
	if (the_objects[x] == obj) {
	  the_objects[x] = obj->next;
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
	DEBUG(printf("  RESULT: Lane %u OBJ", x); print_object(obj));
      }
      obj = obj->next; // move to the next object
    }
  }
  
  // Now determine for each major lane (i.e. Left, Middle, Right) 
  //   whether to add a new object or not...
  for (int x = 1; x < 4; x++) {
    object_state_t * pobj = the_objects[x];
    if ((pobj == NULL) ||
	(!one_obstacle_per_lane && (pobj->distance < (MAX_DISTANCE - MAX_OBJECT_SIZE - MIN_OBJECT_DIST))) ) {
      // There is space for a new object to enter
      int num = (rand() % (100)); // Return a value from [0,99]
      if (num > NEW_OBJ_THRESHOLD) {
        // Create a new object (car) and add it to the lane at position [x][0]
        object_state_t* no_p = (object_state_t*)calloc(1, sizeof(object_state_t));
	no_p->obj_id = global_object_id++;
	no_p->lane = x;
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
	no_p->next = the_objects[x];
	if (the_objects[x] != NULL) {
	  the_objects[x]->previous = no_p;
	}
	the_objects[x] = no_p;
	DEBUG(printf("Adding"); print_object(no_p));
      }
    }
  }

  DEBUG(visualize_world());
  dump_trace_record();

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



void
dump_trace_record()
{
  // For each lane
  DEBUG(printf("TRLN:"));
#ifdef FOR_VIS
  printf("%u,", 2); // This is MY Car lane position
#endif
  for (int x = 1; x < 4; x++) {
    // List the objects in the lane
    object_state_t* pobj = the_objects[x];
    if (x > 1) { printf(","); }
    if (pobj != NULL) {
      int num = 0;
      while (pobj != NULL) {
	if (num > 0) { printf(" "); }
	switch(pobj->object) {
	case myself     : printf("*:"); break;
	case no_label   : printf("N:"); break;
	case car        : printf("C:"); break;
	case truck      : printf("T:"); break;
	case pedestrian : printf("B:"); break;
	case bicycle    : printf("B:"); break;
	default: printf("ERROR "); 
	}
	if (pobj->object == no_label) {
	  printf("11");
	} else {
	  printf("%u", (int)(pobj->distance));
	}
	pobj = pobj->next;
	num++;
      }
    } else {
      printf("N:%u", (int)INF_DISTANCE);
    }
  }
  printf("\n");
}



