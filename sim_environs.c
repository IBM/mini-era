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

object_state_t* the_world[5][13]; // This is [lane][y-posn]
object_state_t* the_objects;      // This is the master list of all currently live objects in the world.
object_state_t my_car;

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
init_sim_environs()
{
  for (int x = 0; x < 5; x++) {
    for (int y = 0; y < 13; y++) {
      the_world[x][y] = NULL;
    }
  }

  the_objects = NULL;

  // Set up the default initial state of my car: Middle lane at medium speed.
  my_car.lane = center;
  my_car.object = myself;
  my_car.speed = 2;
  my_car.previous = NULL;	// not used for my_car
  my_car.next = NULL;		// not used for my_car
  my_car.ahead_in_lane = NULL;	// not used for my_car
  my_car.behind_in_lane = NULL; // not used for my_car

  init_vit_kernel("vit_trace_v04.tr"); // Only gets dictionary!
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
    // Move from y=12 downto y=0 and advance all objects in the world
    for (int y = 12; y >= 0; y--) {
      object_state_t* elem_p = the_world[x][y];
      if (elem_p != NULL) { // We have an object at [x][y]
	int delta_speed = my_car.speed - elem_p->speed;
	int ny = y + delta_speed;
	if (ny > 12) { // Object is "off the world" (we've passed it completely)
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
	  the_world[x][ny] = elem_p;
	  the_world[x][y] = NULL;
	}
      }
    }	
  }

  // Now determine for each major lane (i.e. Left, Middle, Right) 
  //   whether to add a new object or not...
  for (int x = 1; x < 4; x++) {
    if (the_world[x][0] == NULL) { // There is a space for a new object
      int num = (rand() % (100)); // Return a value from [0,99]
      if (num > 90) {
        // Create a new object (car) and add it to the lane at position [x][0]
        object_state_t* no_p = (object_state_t*)calloc(1, sizeof(object_state_t));
	no_p->lane = x;
	no_p->object = car;
	no_p->speed = 1;
	no_p->previous = NULL;
	no_p->next = the_objects;
	the_objects = no_p;
	no_p->ahead_in_lane = NULL;
	no_p->behind_in_lane = NULL; // FIXME
	the_world[x][0] = no_p;
	printf("Adding "); print_object(no_p, x, 0);
      }
    }
  }

  visualize_world();

  // Now we have the state for this (new) time step
  //  Use this to determine my_car's input data, e.g. 
  //  safe_to_move_L/R, etc.
  // NOTE: Currently I am ignoring moving INTO the hazard lanes...
  message_t viterbi_in_state = -1;
  switch (my_car.lane) {
  case lhazard : 
    if (the_world[1][10] == NULL) { 
      viterbi_in_state = safe_to_move_right_only; 
    } else { 
      viterbi_in_state = unsafe_to_move_left_or_right;
    }
    break;
  case left : 
    if (the_world[2][10] == NULL) { 
      viterbi_in_state = safe_to_move_right_only; 
    } else { 
      viterbi_in_state = unsafe_to_move_left_or_right;
    }
    break;
  case center : 
    if (the_world[3][10] == NULL) { 
      if (the_world[1][10] == NULL) { 
	viterbi_in_state = safe_to_move_right_or_left; 
      } else {
	viterbi_in_state = safe_to_move_right_only;
      }
    } else if (the_world[1][10] == NULL) { 
      viterbi_in_state = safe_to_move_left_only;      
    } else { 
      viterbi_in_state = unsafe_to_move_left_or_right;
    }
    break;
  case right : 
    if (the_world[2][10] == NULL) { 
      viterbi_in_state = safe_to_move_left_only; 
    } else { 
      viterbi_in_state = unsafe_to_move_left_or_right;
    }
    break;
  case rhazard : 
    if (the_world[3][10] == NULL) { 
      viterbi_in_state = safe_to_move_left_only; 
    } else { 
      viterbi_in_state = unsafe_to_move_left_or_right;
    }
    break;
  default: printf("ERROR "); 
  }
  printf(" viterbi_in_state = %d\n", viterbi_in_state);
  do_viterbi_work(viterbi_in_state, false); // set "true" for debug output
  
}


void
visualize_world()
{
  // For each position (distance)
  for (int y = 0; y < 10; y++) {
    // For each lane in the world
    printf("%2d : ", y);
    for (int x = 0; x < 5; x++) {
      printf("| ");
      if (the_world[x][y] == NULL) {
	printf("  ");
      } else {
	printf("X ");
      }
    }
    printf("|\n");
  }
  
  printf("%2d : ", 10);
  for (int x = 0; x < 5; x++) {
    printf("| ");
    if (x == my_car.lane) {
      printf("* ");
    } else if (the_world[x][10] == NULL) {
      printf("  ");
    } else {
      printf("X ");
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
	printf("X ");
      }
    }
    printf("|\n");
  }
  printf("\n\n");
}



/* These are types, functions, etc. required for VITERBI */
#include "viterbi/utils.h"
#include "viterbi/viterbi_decoder_generic.h"
#include "viterbi/base.h"

/* These are some top-level defines needed for VITERBI */
typedef struct {
  unsigned int msg_id;
  ofdm_param   ofdm_p;
  frame_param  frame_p;
  uint8_t      in_bits[MAX_ENCODED_BITS];
} vit_dict_entry_t;

uint8_t descramble[1600]; // I think this covers our max use cases
uint8_t actual_msg[1600];

unsigned int      num_dictionary_items = 0;
vit_dict_entry_t* the_viterbi_trace_dict;

extern void descrambler(uint8_t* in, int psdusize, char* out_msg, uint8_t* ref, uint8_t *msg);


status_t 
init_vit_kernel(char* trace_filename)
{
  FILE * vit_trace = fopen(trace_filename,"r");
  if (!vit_trace) {
    printf("Error: unable to open trace file %s\n", trace_filename);
    return error;
  }

  // Read in the trace message dictionary from the trace file
  // Read the number of messages
  fscanf(vit_trace, "%u\n", &num_dictionary_items);
  printf("Reading in %u Vitberi Message Dictionary entries...\n", num_dictionary_items);
  the_viterbi_trace_dict = (vit_dict_entry_t*)calloc(num_dictionary_items, sizeof(vit_dict_entry_t));
  if (the_viterbi_trace_dict == NULL) {
    printf("ERROR : Cannot allocate Viterbi Trace Dictionary memory space");
    return error;
  }

  // Read in each dictionary item
  for (int i = 0; i < num_dictionary_items; i++)  {
    //fscanf(vit_trace, "%u\n", &the_viterbi_trace_dict[i].msg_id);
    the_viterbi_trace_dict[i].msg_id = i;
    printf("Reading in Message %u\n", the_viterbi_trace_dict[i].msg_id);

    int in_bpsc, in_cbps, in_dbps, in_encoding, in_rate; // OFDM PARMS
    fscanf(vit_trace, "%d %d %d %d %d\n", &in_bpsc, &in_cbps, &in_dbps, &in_encoding, &in_rate);
    the_viterbi_trace_dict[i].ofdm_p.encoding   = in_encoding;
    the_viterbi_trace_dict[i].ofdm_p.n_bpsc     = in_bpsc;
    the_viterbi_trace_dict[i].ofdm_p.n_cbps     = in_cbps;
    the_viterbi_trace_dict[i].ofdm_p.n_dbps     = in_dbps;
    the_viterbi_trace_dict[i].ofdm_p.rate_field = in_rate;

    int in_pdsu_size, in_sym, in_pad, in_encoded_bits, in_data_bits;
    fscanf(vit_trace, "%d %d %d %d %d\n", &in_pdsu_size, &in_sym, &in_pad, &in_encoded_bits, &in_data_bits);
    the_viterbi_trace_dict[i].frame_p.psdu_size      = in_pdsu_size;
    the_viterbi_trace_dict[i].frame_p.n_sym          = in_sym;
    the_viterbi_trace_dict[i].frame_p.n_pad          = in_pad;
    the_viterbi_trace_dict[i].frame_p.n_encoded_bits = in_encoded_bits;
    the_viterbi_trace_dict[i].frame_p.n_data_bits    = in_data_bits;

    int num_in_bits = in_encoded_bits + 10; // strlen(str3)+10; //additional 10 values
    printf("  and which has %u input bits\n", num_in_bits);
    for (int ci = 0; ci < num_in_bits; ci++) { 
      unsigned c;
      fscanf(vit_trace, "%u ", &c); 
      the_viterbi_trace_dict[i].in_bits[ci] = (uint8_t)c;
    }
  }

  return success;
}



message_t 
do_viterbi_work(message_t in_msg, bool_t debug)
{
  unsigned tr_val = in_msg;
  
  char the_msg[1600]; // Return from decode; large enough for any of our messages
  message_t msg;      // The output from this routine

  vit_dict_entry_t* trace_msg; // Will hold msg input data for decode, based on trace input
  if (debug) { printf("In do_viterbi_work for message %d\n", in_msg); }

  switch(tr_val) {
  case 0: // safe_to_move_right_or_left
    trace_msg = &the_viterbi_trace_dict[0];
    msg = safe_to_move_right_or_left;  // Cheating - already know output result.
    break;
  case 1: // safe_to_move_right
    trace_msg = &the_viterbi_trace_dict[1];
    msg = safe_to_move_right_only;  // Cheating - already know output result.
    break;
  case 2: // safe_to_move_left
    trace_msg = &the_viterbi_trace_dict[2];
    msg = safe_to_move_left_only;  // Cheating - already know output result.
    break;
  case 3: // unsafe_to_move_left_or_right
    trace_msg = &the_viterbi_trace_dict[3];
    msg = unsafe_to_move_left_or_right;  // Cheating - already know output result.
    break;
  }

  // Send through the viterbi decoder
  uint8_t *result;
  if (debug) { printf("In do_viterbi_work calling decode\n"); }
  result = decode(&(trace_msg->ofdm_p), &(trace_msg->frame_p), &(trace_msg->in_bits[0]));
  if (debug) { printf("In do_viterbi_work back from decode\n"); }

  // descramble the output - put it in result
  int psdusize = trace_msg->frame_p.psdu_size;
  if (debug) { printf("In do_viterbi_work calling descrambler\n"); }
  descrambler(result, psdusize, the_msg, NULL /*descram_ref*/, NULL /*msg*/);
  if (debug) { printf("In do_viterbi_work back from descrambler\n"); }

  // Can check contents of "the_msg" to determine which message;
  //   here we "cheat" and return the message indicated by the trace.
  if (debug) { printf("In do_viterbi_work creturning message %d\n", msg); }
  return msg;
}



