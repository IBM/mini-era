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

#include "gen_trace.h"

/* These are structures, etc. used in the environment */

#define GLOBAL_MAX_LANES        5
#define GLOBAL_MAX_DISTANCE   128

unsigned num_lanes = 3; // Left, Center, Right; 4 = add RHazard, 5 = add LHazard
int min_lane;
int max_lane;
  
unsigned max_distance = 6; // Max distance in the "world"
bool_t   one_obstacle_per_lane = true; // false = unlimited
bool_t   end_with_all_blocked = true;
bool_t   did_last_insert = false;

object_state_t* obj_in_lane[GLOBAL_MAX_LANES]; /* Indicates nearest object in lane */

object_state_t* the_world[GLOBAL_MAX_LANES][GLOBAL_MAX_DISTANCE]; // This is [lane][distance] 
object_state_t* the_objects;      // This is the master list of all currently live objects in the world.
object_state_t my_car;

char get_vis_char_of_object(object_state_t* world_objp) 
{
  char c;
  switch(world_objp->object) {
  case myself     : c = '*'; break;
  case no_label   : c = '?'; break;
  case car        : c = 'C'; break;
  case truck      : c = 'T'; break;
  case pedestrian : c = 'P'; break;
  case bicycle    : c = 'B'; break;
  default: c = '#'; break;
  }
  return c;
}

void
print_object(object_state_t* st, int x, int y) {
  printf(" Object ");
  switch(st->object) {
  case myself: printf("My_Car "); break;
  case no_label : printf("No_Label "); break;
  case car : printf("Car "); break;
  case truck : printf("Truck "); break;
  case pedestrian : printf("Person "); break;
  case bicycle : printf("Bike "); break;
  default: printf("ERROR "); 
  }
  printf("in ");
  switch(st->lane) {
  case lhazard : printf(" L-Hazard "); break;
  case left : printf(" Left "); break;
  case center : printf(" Middle "); break;
  case right : printf(" Right "); break;
  case rhazard : printf(" R-Hazard "); break;
  default: printf("ERROR "); 
  }  
  printf("at x %d y %d speed %u\n", x, y, st->speed);
}  


void
init_environment(int argc, char* argv[])
{
  min_lane = (num_lanes < 5) ? 1 : 0;
  max_lane = (num_lanes < 4) ? 4 : 5;
  
  // Clear out all possible world state positions
  for (int x = 0; x < GLOBAL_MAX_LANES; x++) {
    for (int y = 0; y < GLOBAL_MAX_DISTANCE; y++) {
      the_world[x][y] = NULL;
    }
  }

  the_objects = NULL;

  // Set up the default initial state of my car: Middle lane at medium speed.
  my_car.lane = center;
  my_car.object = myself;
  my_car.speed = 5;
  my_car.previous = NULL;	// not used for my_car
  my_car.next = NULL;		// not used for my_car
  my_car.ahead_in_lane = NULL;	// not used for my_car
  my_car.behind_in_lane = NULL; // not used for my_car

}



void
insert_all_lanes()
{
  for (int i = 1; i <=3; i++) {
    object_state_t* no_p = (object_state_t*)calloc(1, sizeof(object_state_t));
    no_p->lane   = i;
    no_p->distance = max_distance-1;
    no_p->object = car;
    no_p->speed  = 4;
    no_p->previous = NULL;
    no_p->next = the_objects;
    the_objects = no_p;
    no_p->ahead_in_lane = NULL;
    no_p->behind_in_lane = NULL; // FIXME
    the_world[i][max_distance-1] = no_p;
    printf("Adding Car : ");
    print_object(no_p, i, max_distance-1);
    if (obj_in_lane[i] == NULL) {
      obj_in_lane[i] = no_p;
    }
  }
}

void
insert_into_lane(int x)
{
  int num = (rand() % (100)); // Return a value from [0,99]
  if (num > 90) {
    // Create a new object (car) and add it to the lane at position [x][0]
    object_state_t* no_p = (object_state_t*)calloc(1, sizeof(object_state_t));
    no_p->lane = x;
    no_p->distance = max_distance-1;
    //no_p->object = car; break;
    int objn = (rand() % 100); // Return a value from [0,99]
    int spdn = (rand() % 100); // Return a value from [0,99]
    if (objn < 45) {
      no_p->object = car;
      no_p->speed = (spdn < 70) ? 4 : 3;
    } else if (objn < 70) { 
      no_p->object = truck;
      no_p->speed = (spdn < 50) ? 4 : 3;
    } else if (objn < 95) {
      no_p->object = bicycle;
      no_p->speed = (spdn < 30) ? 4 : 3;
    }
    else {
      no_p->object = pedestrian; 
      no_p->speed = 3;
    }
    no_p->previous = NULL;
    no_p->next = the_objects;
    the_objects = no_p;
    no_p->ahead_in_lane = NULL;
    no_p->behind_in_lane = NULL; // FIXME
    the_world[x][max_distance-1] = no_p;
    printf("Adding %u : ", objn); print_object(no_p, x, max_distance-1);
    if (obj_in_lane[x] == NULL) {
      obj_in_lane[x] = no_p;
    }
  }
}

/* NOTES:
 * In this implementation we do a back-to-front pass for each lane...
 * This ensures that there is "open space" behind a car before it "drops back" into that space
 * BUT this only works (well) if the cars/objects all move at the same rate (as one another).  
 * IF we allow objects to have different speeds, then there are different relative motions, and 
 *  the objects must ALSO avoid one another (presumably by altering their speed)
 */

bool_t // indicates we've inserted and run to last items gone
iterate_environment(bool_t doing_end_all_blocked) 
{
  // Track whether there are any obstacles in a lane
  for (int i = 0; i < GLOBAL_MAX_LANES; i++) {
    obj_in_lane[i] = NULL;
  }
  // For each lane in the world
  for (int x = min_lane; x < max_lane; x++) {
    // Move from y=12 downto y=0 and advance all objects in the world
    for (int y = 0; y < max_distance; y++) {
      object_state_t* elem_p = the_world[x][y];
      if (elem_p != NULL) { // We have an object at [x][y]
	int delta_speed = my_car.speed - elem_p->speed;
	int ny = y - delta_speed;
	if (ny < 0) { // Object is "off the world" (we've passed it completely)
	  // Delete the object from the universe!
	  if (elem_p->next != NULL) {
	    if (elem_p->previous != NULL) {
	      elem_p->previous->next = elem_p->next;
	      //} else {
	      //elem_p->previous->next = NULL;
	    }
	  }
	  //delete elem_p;
	  the_world[x][y] = NULL;
	} else if (delta_speed != 0) { // This object will move and still be in the world
	  if (the_world[x][ny] != NULL) {
	    printf("ERROR : cannot move object from [%d][%d] to [%d][%d]\n", x, y, x, ny);
	    print_object(elem_p, x, y);
	    //exit(-1);
	  }
	  printf("Moving obj from %u %u to %u %u\n", x, y, x, ny);
	  elem_p->distance = ny;
	  the_world[x][ny] = elem_p;
	  the_world[x][y] = NULL;
	  if (obj_in_lane[x] == NULL) { // Only keep the closest
	    obj_in_lane[x] = elem_p;
	  }
	}
      }
    }	
  }

  // Now determine for each major lane (i.e. Left, Middle, Right) 
  //   whether to add a new object or not...
  if (doing_end_all_blocked) {
    bool_t no_insert = false;
    for (int i = 1; i < 4; i++) { // We only add to non-hazard lanes (ever)
      no_insert |= (obj_in_lane[i] != NULL);
      //printf("no_insert = %u from lane %u : %p", no_insert, i, obj_in_lane[i]);
    }
    if ((!did_last_insert) && (!no_insert)) {
      insert_all_lanes();
      did_last_insert = true;
    }
  } else {
    for (int x = 1; x < 4; x++) { // We only add to non-hazard lanes (ever)
      if ((the_world[x][max_distance-1] == NULL) && // There is a space for a new object
	  (!one_obstacle_per_lane || (obj_in_lane[x] == NULL))) { // And there is no obstacle in lane already when that is restricted
	insert_into_lane(x);
      }
    }
  }
  visualize_world();

    bool_t lanes_occupied = false;
  //printf(" Lanes: ");
  for (int i = 0; i < GLOBAL_MAX_LANES; i++) {
    lanes_occupied |= (obj_in_lane[i] != NULL);
    if (obj_in_lane[i] != NULL) {
      printf("  Lane %i : ", i); print_object(obj_in_lane[i], obj_in_lane[i]->lane, obj_in_lane[i]->distance);
    }
    //printf("%p ", (void*)obj_in_lane[i]);
  }
  //printf("\n");
  

  // Now we have the state for this (new) time step
  //  Use this to determine my_car's input data, e.g. 
  //  safe_to_move_L/R, etc.
  // NOTE: Currently I am ignoring moving INTO the hazard lanes...
  label_t obj_in_state[5] = { 0, 0, 0};
  char    cv_in_state[5] = {'N', 'N', 'N', 'N', 'N'};
  for (int l = 0; l < GLOBAL_MAX_LANES; l++) {
    if (obj_in_lane[l] != NULL) {
      obj_in_state[l] = obj_in_lane[l]->object;
      cv_in_state[l]  = mapLabelToChar[obj_in_state[l]];
    } else {
      obj_in_state[l] = no_label;
    }
  }
    
  label_t rad_in_state[5] = { 0, 0, 0, 0, 0};
  for (int l = 0; l < GLOBAL_MAX_LANES; l++) {
    if (obj_in_lane[l] != NULL) {
      rad_in_state[l] = obj_in_lane[l]->distance;
    } else {
      rad_in_state[l] = max_distance;
    }
  }
  printf("TRACE_LINE: %c%u %c%u %c%u\n", cv_in_state[1], rad_in_state[1], cv_in_state[2], rad_in_state[2], cv_in_state[3], rad_in_state[3]);
  
  message_t viterbi_in_state[5] = {0, 0, 0, 0, 0};
  // Left Lane
  if ((the_world[2][0] == NULL) && (the_world[2][1] == NULL)) {
    viterbi_in_state[1] = safe_to_move_right_only; 
  } else { 
    viterbi_in_state[1] = unsafe_to_move_left_or_right;
  }
  // Middle Lane
  if ((the_world[3][0] == NULL) && (the_world[3][1] == NULL)) {
    if ((the_world[1][0] == NULL) && (the_world[1][1] == NULL)) {
      viterbi_in_state[2] = safe_to_move_right_or_left; 
    } else {
      viterbi_in_state[2] = safe_to_move_right_only;
    }
  } else if ((the_world[1][0] == NULL) && (the_world[1][1] == NULL)) {
    viterbi_in_state[2] = safe_to_move_left_only;      
  } else { 
    viterbi_in_state[2] = unsafe_to_move_left_or_right;
  }
  // Right Lane
  if ((the_world[2][0] == NULL) && (the_world[2][1] == NULL)) {
    viterbi_in_state[3] = safe_to_move_left_only; 
  } else { 
    viterbi_in_state[3] = unsafe_to_move_left_or_right;
  }
  /* case rhazard :  */
  /*   if (the_world[3][10] == NULL) {  */
  /*     viterbi_in_state = safe_to_move_left_only;  */
  /*   } else {  */
  /*     viterbi_in_state = unsafe_to_move_left_or_right; */
  /*   } */
  printf(" viterbi_in_state = %u %u %u\n", viterbi_in_state[1], viterbi_in_state[2], viterbi_in_state[3]);

  bool_t res = did_last_insert && (!lanes_occupied);
  //printf("iterate results = %u && %u = %u\n", did_last_insert, !lanes_occupied, res);
  return res;
}


void
visualize_world()
{
  // For each position (distance)
  for (int y = max_distance-1; y >=0; y--) {
    // For each lane in the world
    printf("%2d : ", y);
    for (int x = 0; x < 5; x++) {
      printf("| ");
      if (the_world[x][y] == NULL) {
	printf("  ");
      } else {
	//printf("X ");
	printf("%c ", get_vis_char_of_object(the_world[x][y]));
      }
    }
    printf("|\n");
  }
  /**  
       printf("%2d : ", 10);
       for (int x = 0; x < 5; x++) {
       printf("| ");
       if (x == my_car.lane) {
       printf("* ");
       } else if (the_world[x][10] == NULL) {
       printf("  ");
       } else {
       //printf("X ");
       printf("%c ", get_vis_char_of_object(the_world[x][10]));
       }
       }
       printf("|\n");

       for (int y = 11; y < 13; y++) {
       // For each lane in the world
       printf("%2d : ", y);
       for (int x = 0; x < 5; x++) {
       printf("| ");
       if (the_world[x][y] == NULL) {
       printf("  ");
       } else {
       //printf("X ");
       printf("%c ", get_vis_char_of_object(the_world[x][y]));
       }
       }
       printf("|\n");
       }
  **/
  printf("\n\n");
}






int main(int argc, char *argv[])
{
  init_environment(argc, argv);
  int i;
  for (i = 0; i < 100; i++) {
    printf("\n\nTime Step %d\n", i);
    iterate_environment(false);
  }
  if (end_with_all_blocked) {
    do {
      printf("\n\nTime Step %d\n", i++);
    } while (iterate_environment(true) == 0) ;
  }
  return 0;
}
