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

/* File pointers to the computer vision, radar and Viterbi decoding traces */
FILE *cv_trace = NULL;
FILE *rad_trace = NULL;
FILE *vit_trace = NULL;

char* keras_python_file;

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

status_t init_vit_kernel(char* trace_filename)
{
  vit_trace = fopen(trace_filename,"r");
  if (!vit_trace)
  {
    printf("Error: unable to open trace file %s\n", trace_filename);
    return error;
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

message_t iterate_vit_kernel()
{
  /* 1) Read the next OFDM symbol from the trace */
  /* fread( ... ); */


  /* 2) Conduct Viterbi decoding and extract original message */


  /* 3) Return message */

  return no_message;
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
