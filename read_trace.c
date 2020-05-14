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
#include <string.h>
#include "kernels_api.h"
#include "sim_environs.h"

/* File pointer to the input trace */
FILE *input_trace = NULL;

/* These are globals for the trace read parsing routines */
#define MAX_TR_LINE_SZ   256

char in_line_buf[MAX_TR_LINE_SZ];
int last_i = 0;
int in_tok = 0;
int in_lane = 0;


status_t init_trace_reader(char* trace_filename)
{
  DEBUG(printf("In init_trace_reader...\n"));
  /* Now open the mini-ERA trace */
  input_trace = fopen(trace_filename,"r");
  if (!input_trace)
  {
    printf("Error: unable to open trace file %s\n", trace_filename);
    return error;
  }

  return success;
}

void
get_object_token(char c)
{
  //DEBUG(printf("  get_object_token TK %u c %c last_i %u for %s\n", in_tok, c, last_i, &in_line_buf[last_i]));
  if (in_tok == 0) { // 0 => the object character
    char objc; 
    sscanf(&in_line_buf[last_i], "%c", &objc);
    lane_obj[in_lane][obj_in_lane[in_lane]] = objc;
    //if (obj_in_lane[in_lane] == 0) { // LAST is nearest -- but should be safer than that!
    nearest_obj[in_lane] = objc;
    if (objc != 'N') {
      total_obj++;
    }
  } else { // a distance
    printf("ERROR : trace syntax is weird!\n");
    printf(" LINE : %s\n", in_line_buf);
    printf(" TOKN : %u hit %c from %s\n", last_i, c, &in_line_buf[last_i]);
    exit(-3);
  }
  in_tok = 1 - in_tok; // Flip to expect distance token
}

void
get_distance_token(char c)
{
  //DEBUG(printf("  get_distance_token TK %u c %c last_i %u for %s\n", in_tok, c, last_i, &in_line_buf[last_i]));
  if (in_tok == 1) { // 0 => the distance value
    unsigned distv;
    sscanf(&in_line_buf[last_i], "%u", &distv);
    lane_dist[in_lane][obj_in_lane[in_lane]] = distv;
    //if (obj_in_lane[in_lane] == 0) {
    nearest_dist[in_lane] = distv;
    obj_in_lane[in_lane]++;
  } else { // a distance
    printf("ERROR : trace syntax is weird!\n");
    printf(" LINE : %s\n", in_line_buf);
    printf(" TOKN : %u hit %c from %s\n", last_i, c, &in_line_buf[last_i]);
    exit(-4);
  }
  in_tok = 1 - in_tok; // Flip to expect object char token
}


bool_t read_next_trace_record(vehicle_state_t vs)
{
  DEBUG(printf("In read_next_trace_record\n"));
  if (feof(input_trace)) { 
    printf("ERROR : invocation of read_next_trace_record indicates feof\n");
    exit(-1);
  }

  total_obj = 0;
  for (int i = 0; i < NUM_LANES; i++) {
    obj_in_lane[i] = 0;
    nearest_obj[i]  = 'N';
    nearest_dist[i] = INF_DISTANCE;
  }
  
  /* 1) Read the next entry (line, epoch) from the trace */
  void* fres = fgets(in_line_buf, MAX_TR_LINE_SZ, input_trace);
  if (fres == NULL) { // If fgets returns NULL then we hit EOF
    //printf(" FGETS returned NULL - feof = %u\n", feof(input_trace));
    return false; // Indicate we didn't read from the trace (EOF)
  }
  
  if ((strlen(in_line_buf) > 0) &&
      (in_line_buf[strlen (in_line_buf) - 1] == '\n')) {
    in_line_buf[strlen (in_line_buf) - 1] = '\0';
  }
  DEBUG(printf("IN_LINE : %s\n", in_line_buf));
  if (output_viz_trace) {
    if (!vs.active) {
      printf("  VizTrace: %d,%s\n", -vs.lane, in_line_buf);
    } else {
      printf("  VizTrace: %d,%s\n", vs.lane, in_line_buf);
    }      
  }

  last_i = 0;
  in_tok = 0;
  in_lane = 1;
  for (int i = 0; i < 256; i++) { // Scan the input line
    // Find the token seperators
    char c = in_line_buf[i];
    //DEBUG(printf("TR_CHAR '%c'\n", c));
    switch(c) {
    case ':':
      in_line_buf[i] = '\0';
      get_object_token(c);
      last_i = i+1;
      break;
    case ',':
      in_line_buf[i] = '\0';
      get_distance_token(c);
      last_i = i+1;
      in_lane++;
      break;
    case ' ':
      in_line_buf[i] = '\0';
      get_distance_token(c);
      last_i = i+1;
      break;
    case '\0':
    case '\n':
      in_line_buf[i] = '\0';
      get_distance_token(c);
      last_i = i+1;
      i = 256;
      break;
    }
  }


#ifdef SUPER_VERBOSE
  for (int i = 1; i < (NUM_LANES-1); i++) {
    printf("  Lane %u %8s : ", i, lane_names[i]);
    if (obj_in_lane[i] > 0) {
      for (int j = 0; j < obj_in_lane[i]; j++) {
	if (j > 0) {
	  printf(", ");
	}
	printf("%c:%u", lane_obj[i][j], lane_dist[i][j]);
      }
      printf("\n");
    } else {
      printf("%c:%u\n", 'N', (unsigned)INF_DISTANCE);
    }
  }
#endif
  return true;
}

bool_t eof_trace_reader()
{
  bool_t res = feof(input_trace);
  DEBUG(printf("In eof_trace_reader feof = %u\n", res));
  return res;
}

void closeout_trace_reader()
{
  fclose(input_trace);
}

