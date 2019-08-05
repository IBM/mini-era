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

#include <python2.7/Python.h>
#include<stdio.h>
#include "kernels_api.h"

/* These are types, functions, etc. required for VITERBI */
#include "viterbi/utils.h"
#include "viterbi/viterbi_decoder_generic.h"
#include "viterbi/base.h"





/* File pointers to the computer vision, radar and Viterbi decoding traces */
FILE *cv_trace = NULL;
FILE *rad_trace = NULL;
FILE *vit_trace = NULL;

char* keras_python_file;

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




status_t init_cv_kernel(char* trace_filename, char* py_file)
{
  cv_trace = fopen(trace_filename,"r");
  if (!cv_trace)
  {
    printf("Error: unable to open trace file %s\n", trace_filename);
    return error;
  }

  keras_python_file = py_file;

  return success;
}

status_t init_rad_kernel(char* trace_filename)
{
  rad_trace = fopen(trace_filename,"r");
  if (!rad_trace)
  {
    printf("Error: unable to open trace file %s\n", trace_filename);
    return error;
  }

  return success;
}


/* This is the initialization of the Viterbi trace-stream
 * The trace format is:
 *  <n> = number of dictionary entries (message types)
 * For each dictionary entry:
 *  n1 n2 n3 n4 n5 : OFDM parms: 
 *  m1 m2 m3 m4 m5 : FRAME parms:
 *  x1 x2 x3 ...   : The message bits (input to decode routine)
 * Then after all dictionary entries
 *  t1_0 t2_0 t3_0 : Message per-lane (t1 = left, t2 = middle, t3 = right) for time step 0
 *  t1_1 t2_1 t3_1 : Message per-lane (t1 = left, t2 = middle, t3 = right) for time step 1
 */

status_t init_vit_kernel(char* trace_filename)
{
  vit_trace = fopen(trace_filename,"r");
  if (!vit_trace)
  {
    printf("Error: unable to open trace file %s\n", trace_filename);
    return error;
  }

  // Read in the trace message dictionary from the trace file
  // Read the number of messages
  fscanf(vit_trace, "%u\n", &num_dictionary_items);
  the_viterbi_trace_dict = (vit_dict_entry_t*)calloc(num_dictionary_items, sizeof(vit_dict_entry_t));
  if (the_viterbi_trace_dict == NULL) 
  {
    printf("ERROR : Cannot allocate Viterbi Trace Dictionary memory space");
    return error;
  }

  // Read in each dictionary item
  for (int i = 0; i < num_dictionary_items; i++) 
  {
    fscanf(vit_trace, "%u\n", &the_viterbi_trace_dict[i].msg_id);

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
    for (int ci = 0; ci < num_in_bits; ci++) { 
      unsigned c;
      fscanf(vit_trace, "%u ", &c); 
      the_viterbi_trace_dict[i].in_bits[ci] = (uint8_t)c;
    }
  }

  return success;
}

bool_t eof_cv_kernel()
{
  return feof(cv_trace);
}

bool_t eof_rad_kernel()
{
  return feof(rad_trace);
}

bool_t eof_vit_kernel()
{
  return feof(vit_trace);
}

label_t iterate_cv_kernel()
{
  /* Call Keras python functions */

  Py_Initialize();

  PyObject* pName = PyString_FromString(keras_python_file);

  PyObject* pModule = PyImport_Import(pName);


  /* 1) Read the next image frame from the trace */
  /* fread( ... ); */


  /* 2) Conduct object detection on the image frame */


  /* 3) Return the label corresponding to the recognized object */

  return no_label;
}

distance_t iterate_rad_kernel()
{
  /* 1) Read the next waveform from the trace */
  /* fread( ... ); */


  /* 2) Conduct distance estimation on the waveform */


  /* 3) Return the estimated distance */

  return 0.0;
}
/* Each time-step of the trace, we read in the 
 * trace values for the left, middle and right lanes
 * (i.e. which message if the autonomous car is in the 
 *  left, middle or right lane).
 * WE MUST KNOW WHICH LANE'S TRACE TO USE THIS TIME STEP
 *  otherwise we can report inconsistent state; should
 *  the car's lane be an input or a global variable?
 * We currntly assume a global: 
 *    GLOBAL_CURRENT_LANE = 0, 1, 2 (left, middle right)
 */
extern int GLOBAL_CURRENT_LANE;

message_t iterate_vit_kernel()
{
  unsigned tr_msg_vals[3];
  fscanf(vit_trace, "%u %u %u\n", &tr_msg_vals[0], &tr_msg_vals[1], &tr_msg_vals[2]); // Read next trace indicator

  unsigned tr_val = tr_msg_vals[GLOBAL_CURRENT_LANE];  // Theproper message for this time step and car-lane
  
  char the_msg[1600]; // Return from decode; large enough for any of our messages
  message_t msg;      // The output from this routine

  vit_dict_entry_t* trace_msg; // Will hold msg input data for decode, based on trace input
  
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
  result = decode(&(trace_msg->ofdm_p), &(trace_msg->frame_p), &(trace_msg->in_bits[0]));

  // descramble the output - put it in result
  int psdusize = trace_msg->frame_p.psdu_size;
  descrambler(result, psdusize, the_msg, NULL /*descram_ref*/, NULL /*msg*/);

  // Can check contents of "the_msg" to determine which message;
  //   here we "cheat" and return the message indicated by the trace.
  
  return msg;
}

vehicle_state_t plan_and_control(label_t label, distance_t distance, message_t message, vehicle_state_t vehicle_state)
{
  vehicle_state_t new_vehicle_state = vehicle_state;

  if ((label != no_label) && (distance <= THRESHOLD_1))
  {
    /* Stop!!! */
    new_vehicle_state.speed = 0;
  }
  else if ((label != no_label) && (distance <= THRESHOLD_2))
  {
    /* Slow down! */
    new_vehicle_state.speed -= 10;
    if (new_vehicle_state.speed < 0)
    {
      new_vehicle_state.speed = 0;
    }
  }
  else if ((label == no_label) && (distance > THRESHOLD_3))
  {
    /* Maintain speed */
  }



  return new_vehicle_state;
}
