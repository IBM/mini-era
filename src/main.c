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
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

//#include <Python.h>
#include <math.h>

#include "kernels_api.h"

#include "fft-1d.h"

#include "calc_fmcw_dist.h"
#include "visc.h"
#define BYPASS_KERAS_CV_CODE  // INITIAL  BRING-UP

#define TIME
char * cv_dict  = "traces/objects_dictionary.dfn";
char * rad_dict = "traces/radar_dictionary.dfn";
char * vit_dict = "traces/vit_dictionary.dfn";



void print_usage(char * pname) {
  printf("Usage: %s <OPTIONS>\n", pname);
  printf(" OPTIONS:\n");
  printf("    -t <trace> : defines the input trace file to use\n");
  printf("    -v <N>     : defines Viterbi messaging behavior:\n");
  printf("               :      0 = One short message per time step\n");
  printf("               :      1 = One long  message per time step\n");
  printf("               :      2 = One short message per obstacle per time step\n");
  printf("               :      3 = One long  message per obstacle per time step\n");
  printf("               :      4 = One short msg per obstacle + 1 per time step\n");
  printf("               :      5 = One long  msg per obstacle + 1 per time step\n");
}




char* lane_names[NUM_LANES] = {"LHazard", "Left", "Center", "Right", "RHazard" };
#ifdef VERBOSE
char* message_names[NUM_MESSAGES] = {"Safe_L_or_R", "Safe_R_only", "Safe_L_only", "Unsafe_L_or_R" };
#endif
char* object_names[NUM_OBJECTS] = {"Nothing", "Bike", "Car", "Person", "Truck" };

/* These are globals for the trace read parsing routines */
#define MAX_TR_LINE_SZ   256

char in_line_buf[MAX_TR_LINE_SZ];
int last_i = 0;
int in_tok = 0;
int in_lane = 0;

#define MAX_OBJ_IN_LANE  16

unsigned total_obj; // Total non-'N' obstacle objects across all lanes this time step
unsigned obj_in_lane[NUM_LANES]; // Number of obstacle objects in each lane this time step (at least one, 'n')
unsigned lane_dist[NUM_LANES][MAX_OBJ_IN_LANE]; // The distance to each obstacle object in each lane
char     lane_obj[NUM_LANES][MAX_OBJ_IN_LANE]; // The type of each obstacle object in each lane

char     nearest_obj[NUM_LANES]  = { 'N', 'N', 'N', 'N', 'N' };
unsigned nearest_dist[NUM_LANES] = { INF_DISTANCE, INF_DISTANCE, INF_DISTANCE, INF_DISTANCE, INF_DISTANCE };

unsigned hist_total_objs[NUM_LANES * MAX_OBJ_IN_LANE];




/* These are types, functions, etc. required for VITERBI */
#include "viterbi_decoder_generic.h"

#ifndef BYPASS_KERAS_CV_CODE
PyObject *pName, *pModule, *pFunc, *pFunc_load;
PyObject *pArgs, *pValue, *pretValue;
#define PY_SSIZE_T_CLEAN

char *python_module = "mio";
char *python_func = "predict";	  
char *python_func_load = "loadmodel";	  
#endif
/* File pointer to the input trace */
FILE *input_trace = NULL;


/* These are some top-level defines needed for CV kernel */
//#define IMAGE_SIZE  32  // What size are these?
/* typedef struct { */
/*   unsigned int image_id; */
/*   label_t  object; */
/*   unsigned image_data[IMAGE_SIZE]; */
/* } cv_dict_entry_t; */

/** The CV kernel uses a different method to select appropriate inputs; dictionary not needed
unsigned int     num_cv_dictionary_items = 0;
cv_dict_entry_t* the_cv_object_dict;
**/
unsigned label_match[NUM_OBJECTS+1] = {0, 0, 0, 0, 0, 0};  // Times CNN matched dictionary
unsigned label_lookup[NUM_OBJECTS+1] = {0, 0, 0, 0, 0, 0}; // Times we used CNN for object classification
unsigned label_mismatch[NUM_OBJECTS][NUM_OBJECTS] = {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}};
  

/* These are some top-level defines needed for RADAR */
unsigned int        num_radar_dictionary_items = 0;
radar_dict_entry_t* the_radar_return_dict;

unsigned radar_inf_errs = 0;
unsigned radar_inf_noerr = 0;
unsigned radar_zero_errs = 0;
unsigned radar_zero_noerr = 0;
unsigned radar_total_calc = 0;
unsigned hist_pct_errs[5] = {0, 0, 0, 0, 0};
char*    hist_pct_err_label[5] = {"   0%", "<  1%", "< 10%", "<100%", ">100%"};

/* These are some top-level defines needed for VITERBI */

uint8_t descramble[1600]; // I think this covers our max use cases
uint8_t actual_msg[1600];

unsigned int      num_viterbi_dictionary_items = 0;
vit_dict_entry_t* the_viterbi_trace_dict;

unsigned vit_msgs_behavior = 0; // 0 = default
unsigned total_msgs = 0; // Total messages decoded during the full run
unsigned bad_decode_msgs = 0; // Total messages decoded incorrectly during the full run
  

extern void descrambler(uint8_t* in, int psdusize, char* out_msg, uint8_t* ref, uint8_t *msg);





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


status_t init_rad_kernel(char* dict_fn)
{
  DEBUG(printf("In init_rad_kernel...\n"));
  // Read in the radar distances dictionary file
  FILE *dictF = fopen(dict_fn,"r");
  if (!dictF)
  {
    printf("Error: unable to open dictionary file %s\n", dict_fn);
    return error;
  }
  // Read the number of definitions
  if (fscanf(dictF, "%u\n", &num_radar_dictionary_items)) 
	  ;
  DEBUG(printf("  There are %u dictionary entries\n", num_radar_dictionary_items));
  the_radar_return_dict = (radar_dict_entry_t*)calloc(num_radar_dictionary_items, sizeof(radar_dict_entry_t));
  if (the_radar_return_dict == NULL) 
  {
    printf("ERROR : Cannot allocate Radar Trace Dictionary memory space");
    return error;
  }

  unsigned tot_dict_values = 0;
  for (int di = 0; di < num_radar_dictionary_items; di++) {
    unsigned entry_id;
    float entry_dist;
    unsigned entry_dict_values = 0;
    if (fscanf(dictF, "%u %f", &entry_id, &entry_dist)) 
	    ;
    DEBUG(printf("  Reading rad dictionary entry %u : %u %f\n", di, entry_id, entry_dist));
    the_radar_return_dict[di].return_id = entry_id;
    the_radar_return_dict[di].distance =  entry_dist;
    for (int i = 0; i < 2*RADAR_N; i++) {
      float fin;
      if (fscanf(dictF, "%f", &fin)) 
	      ;
      the_radar_return_dict[di].return_data[i] = fin;
      tot_dict_values++;
      entry_dict_values++;
    }
    DEBUG(printf("    Read in dict entry %u with %u total values\n", di, entry_dict_values));
  }
  DEBUG(printf("  Read %u dict entrie with %u values across them all\n", num_radar_dictionary_items, tot_dict_values));
  if (!feof(dictF)) {
    printf("NOTE: Did not hit eof on the radar dictionary file %s\n", "radar_dictionary.dfn");
  }
  fclose(dictF);

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

status_t init_vit_kernel(char* dict_fn)
{
  DEBUG(printf("In init_vit_kernel...\n"));
  // Read in the object images dictionary file
  FILE *dictF = fopen(dict_fn,"r");
  if (!dictF)
  {
    printf("Error: unable to open viterbi dictionary definitiond file %s\n", "vit_dictionary.dfn");
    return error;
  }

  // Read in the trace message dictionary from the trace file
  // Read the number of messages
  if (fscanf(dictF, "%u\n", &num_viterbi_dictionary_items)) 
	  ;
  DEBUG(printf("  There are %u dictionary entries\n", num_viterbi_dictionary_items));
  the_viterbi_trace_dict = (vit_dict_entry_t*)calloc(num_viterbi_dictionary_items, sizeof(vit_dict_entry_t));
  if (the_viterbi_trace_dict == NULL) 
  {
    printf("ERROR : Cannot allocate Viterbi Trace Dictionary memory space");
    return error;
  }

  // Read in each dictionary item
  for (int i = 0; i < num_viterbi_dictionary_items; i++) 
  {
    DEBUG(printf("  Reading vit dictionary entry %u\n", i)); //the_viterbi_trace_dict[i].msg_id));

    int mnum, mid;
    if (fscanf(dictF, "%d %d\n", &mnum, &mid)) 
	    ;
    DEBUG(printf(" V_MSG: num %d Id %d\n", mnum, mid));
    if (mnum != i) {
      printf("ERROR : Check Viterbi Dictionary : i = %d but Mnum = %d  (Mid = %d)\n", i, mnum, mid);
      exit(-5);
    }
    the_viterbi_trace_dict[i].msg_id = mnum;
    the_viterbi_trace_dict[i].msg_id = mid;

    int in_bpsc, in_cbps, in_dbps, in_encoding, in_rate; // OFDM PARMS
    if (fscanf(dictF, "%d %d %d %d %d\n", &in_bpsc, &in_cbps, &in_dbps, &in_encoding, &in_rate)) 
	    ;
    DEBUG(printf("  OFDM: %d %d %d %d %d\n", in_bpsc, in_cbps, in_dbps, in_encoding, in_rate));
    the_viterbi_trace_dict[i].ofdm_p.encoding   = in_encoding;
    the_viterbi_trace_dict[i].ofdm_p.n_bpsc     = in_bpsc;
    the_viterbi_trace_dict[i].ofdm_p.n_cbps     = in_cbps;
    the_viterbi_trace_dict[i].ofdm_p.n_dbps     = in_dbps;
    the_viterbi_trace_dict[i].ofdm_p.rate_field = in_rate;

    int in_pdsu_size, in_sym, in_pad, in_encoded_bits, in_data_bits;
    if (fscanf(dictF, "%d %d %d %d %d\n", &in_pdsu_size, &in_sym, &in_pad, &in_encoded_bits, &in_data_bits)) 
	    ;
    DEBUG(printf("  FRAME: %d %d %d %d %d\n", in_pdsu_size, in_sym, in_pad, in_encoded_bits, in_data_bits));
    the_viterbi_trace_dict[i].frame_p.psdu_size      = in_pdsu_size;
    the_viterbi_trace_dict[i].frame_p.n_sym          = in_sym;
    the_viterbi_trace_dict[i].frame_p.n_pad          = in_pad;
    the_viterbi_trace_dict[i].frame_p.n_encoded_bits = in_encoded_bits;
    the_viterbi_trace_dict[i].frame_p.n_data_bits    = in_data_bits;

    int num_in_bits = in_encoded_bits + 10; // strlen(str3)+10; //additional 10 values
    DEBUG(printf("  Reading %u in_bits\n", num_in_bits));
    for (int ci = 0; ci < num_in_bits; ci++) { 
      unsigned c;
      if (fscanf(dictF, "%u ", &c)) 
	      ;
      DEBUG(printf("%u ", c));
      the_viterbi_trace_dict[i].in_bits[ci] = (uint8_t)c;
    }
    DEBUG(printf("\n"));
  }
  fclose(dictF);

  for (int i = 0; i < NUM_LANES * MAX_OBJ_IN_LANE; i++) {
    hist_total_objs[i] = 0;
  }

  DEBUG(printf("DONE with init_vit_kernel -- returning success\n"));
  return success;
}

status_t init_cv_kernel(char* dict_fn)
{
  DEBUG(printf("In the init_cv_kernel routine\n"));
  /** The CV kernel uses a different method to select appropriate inputs; dictionary not needed
  // Read in the object images dictionary file
  FILE *dictF = fopen(dict_fn,"r");
  if (!dictF)
  {
    printf("Error: unable to open dictionary file %s\n", dict_fn);
    return error;
  }
  // Read the number of definitions
  fscanf(dictF, "%u\n", &num_cv_dictionary_items);
  DEBUG(printf("  There are %u dictionary entries\n", num_cv_dictionary_items));
  the_cv_object_dict = (cv_dict_entry_t*)calloc(num_cv_dictionary_items, sizeof(cv_dict_entry_t));
  if (the_cv_object_dict == NULL) 
  {
    printf("ERROR : Cannot allocate Cv Trace Dictionary memory space");
    return error;
  }

  for (int di = 0; di < num_cv_dictionary_items; di++) {
    unsigned entry_id;
    unsigned object_id;
    fscanf(dictF, "%u %u", &entry_id, &object_id);
    DEBUG(printf("  Reading cv dictionary entry %u : %u %u\n", di, entry_id, object_id));
    the_cv_object_dict[di].image_id = entry_id;
    the_cv_object_dict[di].object   = object_id;
    for (int i = 0; i < IMAGE_SIZE; i++) {
      unsigned fin;
      fscanf(dictF, "%u", &fin);
      the_cv_object_dict[di].image_data[i] = fin;
    }
  }
  fclose(dictF);
  **/
  // Initialization to run Keras CNN code 
#ifndef BYPASS_KERAS_CV_CODE
  Py_Initialize();
  pName = PyUnicode_DecodeFSDefault(python_module);
  pModule = PyImport_Import(pName);
  Py_DECREF(pName);

  if (pModule == NULL) {
     PyErr_Print();
     printf("Failed to load Python program, perhaps pythonpath needs to be set; export PYTHONPATH=your_mini_era_dir/cv/CNN_MIO_KERAS");
     return 1;
  } else {
    pFunc_load = PyObject_GetAttrString(pModule, python_func_load);

    if (pFunc_load && PyCallable_Check(pFunc_load)) {
       PyObject_CallObject(pFunc_load, NULL);
    }
    else {
        if (PyErr_Occurred())
        PyErr_Print();
        printf("Cannot find python function - loadmodel");
    }
    Py_XDECREF(pFunc_load);
  }
  DEBUG(printf("CV Kernel Init done\n"));
#endif  
  return success;
}


bool_t eof_trace_reader()
{
  bool_t res = feof(input_trace);
  DEBUG(printf("In eof_trace_reader feof = %u\n", res));
  return res;
}


label_t run_object_classification_syscall(unsigned tr_val) 
{
  DEBUG(printf("Entered run_object_classification...\n"));
  label_t object;	
#ifdef BYPASS_KERAS_CV_CODE
  object = (label_t)tr_val;
#else
  char shell_cmd[100];
  snprintf(shell_cmd, sizeof(shell_cmd), "sh utils/cnn_shell.sh %u", tr_val);
  DEBUG(printf("  Invoking CV CNN using `%s`\n", shell_cmd));
  FILE *testing = popen(shell_cmd, "r");
  if (testing == NULL)
  {
    printf("FAIL to open CV kernel !\n");
    return 1;
  }
  char pbuffer[100];
  while (fgets(pbuffer, 100, testing) != NULL)
  {
    //printf(pbuffer);
  }
  DEBUG(printf("Label Prediction done \n"));
  DEBUG(printf("pbuffer : %s\n", pbuffer));
  int val = atoi(pbuffer);   //the last thing printed by the Keras code is the predicted label 
  object = (label_t)val;
  pclose(testing);
  DEBUG(printf("run_object_classification returning %u = %u\n", val, object));
#endif
  return object;  
}

label_t run_object_classification(unsigned tr_val) 
{
  DEBUG(printf("Entered run_object_classification...\n"));
  label_t object = (label_t)tr_val;
#ifndef BYPASS_KERAS_CV_CODE
  if (pModule != NULL) {
          pFunc = PyObject_GetAttrString(pModule, python_func);
  
	  if (pFunc && PyCallable_Check(pFunc)) {
	     pArgs = PyTuple_New(1);
	     pValue = PyLong_FromLong(tr_val);
	     if (!pValue) {
		Py_DECREF(pArgs);
		Py_DECREF(pFunc);
		Py_DECREF(pModule);
		fprintf(stderr, "Trying to run CNN kernel: Cannot convert C argument into python\n");
		return 1;
	      }
	      PyTuple_SetItem(pArgs, 0, pValue);
	      pretValue = PyObject_CallObject(pFunc, pArgs);
	      Py_DECREF(pArgs);
	      if (pretValue != NULL) {
		DEBUG(printf("Predicted label from Python program: %ld\n", PyLong_AsLong(pretValue)));
	  	int val = PyLong_AsLong(pretValue);    
	  	object = (label_t)val;
          	DEBUG(printf("run_object_classification returning %u = %u\n", val, object));
		Py_DECREF(pretValue);
	      }
	      else {
		Py_DECREF(pFunc);
		Py_DECREF(pModule);
		PyErr_Print();
		printf("Trying to run CNN kernel : Python function call failed\n");
		return 1;
	       }
	   }
	   else {
	      if (PyErr_Occurred())
	      PyErr_Print();
	      printf("Cannot find python function");
	    }
        Py_XDECREF(pFunc);
    //Py_DECREF(pModule);
   }
   
	  
#endif
  return object;  
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
    printf(" FGETS returned NULL - feof = %u\n", feof(input_trace));
    return false; // Indicate we didn't read from the trace (EOF)
  }
  
  if ((strlen(in_line_buf) > 0) &&
      (in_line_buf[strlen (in_line_buf) - 1] == '\n')) {
    in_line_buf[strlen (in_line_buf) - 1] = '\0';
  }
  DEBUG(printf("IN_LINE : %s\n", in_line_buf));
  DEBUG(printf("  VizTrace: %u,%s\n", vs.lane, in_line_buf));
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
      printf("%c:%u\n", 'N', INF_DISTANCE);
    }
  }
#endif
  return true;
}

// This prepares the input for the execute_cv_kernel call
label_t iterate_cv_kernel(vehicle_state_t vs)
{
  DEBUG(printf("In iterate_cv_kernel\n"));

  unsigned tr_val = 0; // Default nothing
  switch(nearest_obj[vs.lane]) {
    case 'N' : tr_val = no_label; break;
    case 'B' : tr_val = bus; break;
    case 'C' : tr_val = car; break;
    case 'P' : tr_val = pedestrian; break;
    case 'T' : tr_val = truck; break;
    default: printf("ERROR : Unknown object type in cv trace: '%c'\n", nearest_obj[vs.lane]); exit(-2);
  }
  label_t d_object = (label_t)tr_val;
  return d_object;
}

label_t execute_cv_kernel(label_t tr_val)
{
  /* 2) Conduct object detection on the image frame */
  // Call Keras Code  
  label_t object = run_object_classification(tr_val); 
  //label_t object = the_cv_object_dict[tr_val].object;
  return object;
}

void post_execute_cv_kernel(label_t d_object, label_t object)
{
  /* 3) Return the label corresponding to the recognized object */
  DEBUG(printf("  Returning d_object %u %s : object %u %s\n", d_object, object_names[d_object], object, object_names[object]));
  if (d_object == object) {
    label_match[d_object]++;
    label_match[NUM_OBJECTS]++;
  } else {
    label_mismatch[d_object][object]++;
  }
  label_lookup[NUM_OBJECTS]++;
  label_lookup[d_object]++;
}



radar_dict_entry_t* iterate_rad_kernel(vehicle_state_t vs)
{
  DEBUG(printf("In iterate_rad_kernel\n"));

  unsigned tr_val = nearest_dist[vs.lane] / RADAR_BUCKET_DISTANCE;  // The proper message for this time step and car-lane
  
  //distance_t ddist = 1.0 * the_radar_return_dict[tr_val].distance;

  /* // We have to make a working copy of the inputs -- I think the calculate_peak_dist_from_fmcw alters the input data space */
  /* float * ref_in = the_radar_return_dict[tr_val].return_data; */
  /* for (int ii = 0; ii < 2*RADAR_N; ii++) { */
  /*   inputs[ii] = ref_in[ii]; */
  /* } */

  /* DEBUG(printf("  Using dist tr_val %u : in meters %f\n", tr_val, ddist)); */
  return &(the_radar_return_dict[tr_val]);
}

void execute_rad_kernel(float * inputs, size_t input_size_bytes, unsigned int N, unsigned int logn, int sign, float * distance, size_t dist_size)
{
  __visc__hint(CPU_TARGET);
  __visc__attributes(2, inputs, distance, 1, distance);

  /* 2) Conduct distance estimation on the waveform */
  //DEBUG(printf("  Calling calculate_peak_dist_from_fmcw\n"));
  calculate_peak_dist_from_fmcw(inputs, input_size_bytes, RADAR_N, RADAR_LOGN, -1, distance, dist_size);

  __visc__return(1, dist_size);
}

void post_execute_rad_kernel(distance_t ddist, distance_t dist)
{
  // Get an error estimate (Root-Squared?)
  radar_total_calc++;
  if (dist == INFINITY) {
    if (ddist < 500.0) { // 100000.0) {
      DEBUG(printf("%f vs %f => INF_PCT_ERR\n", dist, ddist));
      radar_inf_errs++;
    } else {
      radar_inf_noerr++;
    }      
  } else if (ddist == 0.0) {
    if (ddist != 0.0) {
      DEBUG(printf("%f vs %f => INF_PCT_ERR\n", dist, ddist));
      radar_zero_errs++;
    } else {
      radar_zero_noerr++;
    }
  } else {
    float error   = (dist - ddist);
    DEBUG(float abs_err = fabs(error));
    float pct_err = error/ddist;
    DEBUG(printf("%f vs %f : ERROR : %f   ABS_ERR : %f PCT_ERR : %f\n", dist, ddist, error, abs_err, pct_err));
    if (pct_err == 0.0) {
      hist_pct_errs[0]++;
    } else if (pct_err < 0.01) {
      hist_pct_errs[1]++;
    } else if (pct_err < 0.1) {
      hist_pct_errs[2]++;
    } else if (pct_err < 1.00) {
      hist_pct_errs[3]++;
    } else {
      hist_pct_errs[4]++;
    }
  }
}


/* Each time-step of the trace, we read in the 
 * trace values for the left, middle and right lanes
 * (i.e. which message if the autonomous car is in the 
 *  left, middle or right lane).
 */
vit_dict_entry_t* iterate_vit_kernel(vehicle_state_t vs)
{
  hist_total_objs[total_obj]++;
  unsigned tr_val = 0; // set a default to avoid compiler messages
  switch (vs.lane) {
  case lhazard:
    if ((nearest_obj[1] != 'N') && (nearest_dist[1] < VIT_CLEAR_THRESHOLD)) {  
      // Some object is in the left lane within threshold distance
      tr_val = 3; // Unsafe to move from lhazard lane into the left lane 
    } else {
      tr_val = 1;
    }
    break;
  case left:
  case center:
  case right:
    tr_val = 0;
    DEBUG(printf("  Lane %u : obj in %u is %c at %u : obj in %u is %c at %u\n", vs.lane, 
		 vs.lane-1, nearest_obj[vs.lane-1], nearest_dist[vs.lane-1],
		 vs.lane+1, nearest_obj[vs.lane+1], nearest_dist[vs.lane+1]));
    if ((nearest_obj[vs.lane-1] != 'N') && (nearest_dist[vs.lane-1] < VIT_CLEAR_THRESHOLD)) {
      // Some object is in the Left lane at distance 0 or 1
      DEBUG(printf("    Marking unsafe to move left\n"));
      tr_val += 1; // Unsafe to move from this lane to the left.
    }
    if ((nearest_obj[vs.lane+1] != 'N') && (nearest_dist[vs.lane+1] < VIT_CLEAR_THRESHOLD)) {
      // Some object is in the Right lane at distance 0 or 1
      DEBUG(printf("    Marking unsafe to move right\n"));
      tr_val += 2; // Unsafe to move from this lane to the right.
    }
    break;
  case rhazard:
    if ((nearest_obj[3] != 'N') && (nearest_dist[3] < VIT_CLEAR_THRESHOLD)) {
      // Some object is in the right lane within threshold distance
      tr_val = 3; // Unsafe to move from center lane to the right.
    } else {
      tr_val = 2;
    }
    break;
  }

  DEBUG(printf("Viterbi final message for lane %u %s = %u\n", vs.lane, lane_names[vs.lane], tr_val));	

  vit_dict_entry_t* trace_msg; // Will hold msg input data for decode, based on trace input

  // Here we determine short or long messages, based on global vit_msgs_behavior
  int msg_offset = 0; // 0 = short messages, 4 = long messages
  switch(vit_msgs_behavior) {
  case 0: break;
  case 1: msg_offset = 4; break;
  case 2: break;
  case 3: msg_offset = 4; break;
  case 4: break;
  case 5: msg_offset = 4; break;
  }

  switch(tr_val) {
  case 0: // safe_to_move_right_or_left
    trace_msg = &(the_viterbi_trace_dict[0 + msg_offset]);
    break;
  case 1: // safe_to_move_right
    trace_msg = &(the_viterbi_trace_dict[1 + msg_offset]);
    break;
  case 2: // safe_to_move_left
    trace_msg = &(the_viterbi_trace_dict[2 + msg_offset]);
    break;
  case 3: // unsafe_to_move_left_or_right
    trace_msg = &(the_viterbi_trace_dict[3 + msg_offset]);
    break;
  }
  //DEBUG(printf("  Using msg %u : %s = safe_to_move_right_or_left\n", msg, message_names[msg]));
  return trace_msg;
}

void execute_vit_kernel(ofdm_param* ofdm_ptr,    size_t ofdm_parms_size,
			frame_param* frame_ptr,  size_t frame_parm_size,
			uint8_t* input_bits,     size_t input_bits_size,
			char* out_msg_txt,       size_t out_msg_txt_size,
			message_t* out_message,  size_t out_message_size)
{
  // Send the input_bits message through the viterbi decoder
  uint8_t *result;
  DEBUG(printf("  Calling the viterbi decode routine...\n"));
  // descramble the output - put it in l_decoded
  uint8_t l_decoded[MAX_ENCODED_BITS * 3 / 4]; // Intermediate value
  viterbi_decode(ofdm_ptr, frame_ptr, input_bits, l_decoded);
  int psdusize = frame_ptr->psdu_size;
  DEBUG(printf("  Calling the viterbi descrambler routine\n"));
  descrambler(l_decoded, psdusize, out_msg_txt, NULL /*descram_ref*/, NULL /*msg*/);

  // Check contents of "out_msg_txt" to determine which message_t;
  switch(out_msg_txt[3]) {
  case '0' : *out_message = safe_to_move_right_or_left; break;
  case '1' : *out_message = safe_to_move_right_only; break;
  case '2' : *out_message = safe_to_move_left_only; break;
  case '3' : *out_message = unsafe_to_move_left_or_right; break;
  default  : *out_message = num_messages; break;
  }
}


void post_execute_vit_kernel(message_t tr_msg, message_t dec_msg)
{
  total_msgs++;
  if (dec_msg != tr_msg) {
    bad_decode_msgs++;
  }
}


vehicle_state_t plan_and_control(label_t label, distance_t distance, message_t message, vehicle_state_t vehicle_state)
{
  DEBUG(printf("In the plan_and_control routine : label %u %s distance %.1f (T1 %.1f T1 %.1f T3 %.1f) message %u\n", 
	       label, object_names[label], distance, THRESHOLD_1, THRESHOLD_2, THRESHOLD_3, message));
  vehicle_state_t new_vehicle_state = vehicle_state;
  
  if ((label != no_label) && (distance <= THRESHOLD_1))
  {
    switch (message) {
      case safe_to_move_right_or_left   :
	/* Bias is move right, UNLESS we are in the Right lane and would then head into the RHazard Lane */
	if (vehicle_state.lane < right) { 
	  DEBUG(printf("   In %s with Safe_L_or_R : Moving Right\n", lane_names[vehicle_state.lane]));
	  new_vehicle_state.lane += 1;
	} else {
	  DEBUG(printf("   In %s with Safe_L_or_R : Moving Left\n", lane_names[vehicle_state.lane]));
	  new_vehicle_state.lane -= 1;
	}	  
	break; // prefer right lane
      case safe_to_move_right_only      :
	DEBUG(printf("   In %s with Safe_R_only : Moving Right\n", lane_names[vehicle_state.lane]));
	new_vehicle_state.lane += 1;
	break;
      case safe_to_move_left_only       :
	DEBUG(printf("   In %s with Safe_L_Only : Moving Left\n", lane_names[vehicle_state.lane]));
	new_vehicle_state.lane -= 1;
	break;
      case unsafe_to_move_left_or_right :
	DEBUG(printf("   In %s with No_Safe_Move : STOPPING\n", lane_names[vehicle_state.lane]));
	new_vehicle_state.speed = 0;
	break; /* Stop!!! */
    default:
      printf(" ERROR  In %s with UNDEFINED MESSAGE: %u\n", lane_names[vehicle_state.lane], message);
      exit(-6);
    }
  } else {
    // No obstacle-inspired lane change, so try now to occupy the center lane
    switch (vehicle_state.lane) {
    case lhazard:
    case left:
      if ((message == safe_to_move_right_or_left) ||
	  (message == safe_to_move_right_only)) {
	DEBUG(printf("  In %s with Can_move_Right: Moving Right\n", lane_names[vehicle_state.lane]));
	new_vehicle_state.lane += 1;
      }
      break;
    case center:
      // No need to alter, already in the center
      break;
    case right:
    case rhazard:
      if ((message == safe_to_move_right_or_left) ||
	  (message == safe_to_move_left_only)) {
	DEBUG(printf("  In %s with Can_move_Left : Moving Left\n", lane_names[vehicle_state.lane]));
	new_vehicle_state.lane -= 1;
      }
      break;
    }
  } // else clause


  /** For now we'll igfnore other thresholds, etc.
  else if ((label != no_label) && (distance <= THRESHOLD_2))
  {
  switch (message) {
  case safe_to_move_right_or_left   : new_vehicle_state.lane += 1; break; // prefer right lane
  case safe_to_move_right_only      : new_vehicle_state.lane += 1; break;
  case safe_to_move_left_only       : new_vehicle_state.lane -= 1; break;
  case unsafe_to_move_left_or_right : new_vehicle_state.speed = 0; break; // Stop!!!
  }
  // Slow down!
  new_vehicle_state.speed -= 10;
  if (new_vehicle_state.speed < 0)
  {
  new_vehicle_state.speed = 0;
  }
  }
  else if ((label == no_label) && (distance > THRESHOLD_3))
  {
  // Maintain speed 
  }
  **/


  return new_vehicle_state;
}


void closeout_trace_reader()
{
  fclose(input_trace);
}


void closeout_cv_kernel()
{
  float label_correct_pctg = (100.0*label_match[NUM_OBJECTS])/(1.0*label_lookup[NUM_OBJECTS]);
  printf("\nFinal CV CNN Accuracy: %u correct of %u classifications = %.2f%%\n", label_match[NUM_OBJECTS], label_lookup[NUM_OBJECTS], label_correct_pctg);
  for (int i = 0; i < NUM_OBJECTS; i++) {
    label_correct_pctg = (100.0*label_match[i])/(1.0*label_lookup[i]);
    printf("  CV CNN Accuracy for %10s : %u correct of %u classifications = %.2f%%\n", object_names[i], label_match[i], label_lookup[i], label_correct_pctg);
  }

  unsigned errs = label_lookup[NUM_OBJECTS] - label_match[NUM_OBJECTS];
  if (errs > 0) {
    printf("\nAnalysis of the %u mis-identifications:\n", errs);
    for (int i = 0; i < NUM_OBJECTS; i++) {
      for (int j = 0; j < NUM_OBJECTS; j++) {
	if (label_mismatch[i][j] != 0) {
	  printf("  Mislabeled %10s as %10s on %u occasions\n", object_names[i], object_names[j], label_mismatch[i][j]);
	}
      }
    }
  }

#ifndef BYPASS_KERAS_CV_CODE
    Py_DECREF(pModule);
    Py_Finalize();
#endif   
}

void closeout_rad_kernel()
{
  printf("\nHistogram of Radar Distance ABS-PCT-ERROR:\n");
  for (int i = 0; i < 5; i++) {
    printf("%7s | %9u \n", hist_pct_err_label[i], hist_pct_errs[i]);
  }
  printf("%7s | %9u \n", "Inf_Err", radar_inf_errs);
  printf("%7s | %9u \n", "Inf_OK", radar_inf_noerr);
  printf("%7s | %9u \n", "Inf_Err", radar_zero_errs);
  printf("%7s | %9u \n", "ZeroOk", radar_zero_noerr);
  printf("%7s | %9u \n", "Total", radar_total_calc);
}

void closeout_vit_kernel()
{
  // Nothing to do?

  printf("\nHistogram of Total Objects:\n");
  unsigned sum = 0;
  for (int i = 0; i < NUM_LANES * MAX_OBJ_IN_LANE; i++) {
    if (hist_total_objs[i] != 0) {
      printf("%3u | %9u \n", i, hist_total_objs[i]);
      sum += i*hist_total_objs[i];
    }
  }
  double avg_objs = (1.0 * sum)/(1.0 * radar_total_calc); // radar_total_calc == total time steps
  printf("There were %.3lf obstacles per time step (average)\n", avg_objs);
  double avg_msgs = (1.0 * total_msgs)/(1.0 * radar_total_calc); // radar_total_calc == total time steps
  printf("There were %.3lf messages per time step (average)\n", avg_msgs);
}


/* -*-Mode: C;-*- */

/** #include fft-1d/fft.c **/
/**BeginCopyright************************************************************
 *
 * $HeadURL: https://pastec.gtri.gatech.edu/svn/svn-dpc/INNC/projects/PERFECT-TAV-ES/suite/required/fft-1d/src/fft.c $
 * $Id: fft.c 8546 2014-04-02 21:36:22Z tallent $
 *
 *---------------------------------------------------------------------------
 * Part of PERFECT Benchmark Suite (hpc.pnnl.gov/projects/PERFECT/)
 *---------------------------------------------------------------------------
 *
 * Copyright ((c)) 2014, Battelle Memorial Institute
 * Copyright ((c)) 2014, Georgia Tech Research Corporation
 * All rights reserved.
 *
 * 1. Battelle Memorial Institute (hereinafter Battelle) and Georgia Tech
 *    Research Corporation (GTRC) hereby grant permission to any person
 *    or entity lawfully obtaining a copy of this software and associated
 *    documentation files (hereinafter "the Software") to redistribute
 *    and use the Software in source and binary forms, with or without
 *    modification.  Such person or entity may use, copy, modify, merge,
 *    publish, distribute, sublicense, and/or sell copies of the
 *    Software, and may permit others to do so, subject to the following
 *    conditions:
 * 
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimers.
 * 
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in
 *      the documentation and/or other materials provided with the
 *      distribution.
 * 
 *    * Other than as used herein, neither the name Battelle Memorial
 *      Institute nor Battelle may be used in any form whatsoever without
 *      the express written consent of Battelle.
 * 
 *      Other than as used herein, neither the name Georgia Tech Research
 *      Corporation nor GTRC may not be used in any form whatsoever
 *      without the express written consent of GTRC.
 * 
 *    * Redistributions of the software in any form, and publications
 *      based on work performed using the software should include the
 *      following citation as a reference:
 * 
 *      Kevin Barker, Thomas Benson, Dan Campbell, David Ediger, Roberto
 *      Gioiosa, Adolfy Hoisie, Darren Kerbyson, Joseph Manzano, Andres
 *      Marquez, Leon Song, Nathan R. Tallent, and Antonino Tumeo.
 *      PERFECT (Power Efficiency Revolution For Embedded Computing
 *      Technologies) Benchmark Suite Manual. Pacific Northwest National
 *      Laboratory and Georgia Tech Research Institute, December 2013.
 *      http://hpc.pnnl.gov/projects/PERFECT/
 *
 * 2. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *    FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 *    BATTELLE, GTRC, OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 *    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 *    HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 *    STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *    ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 *    OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 **EndCopyright*************************************************************/

static unsigned int
_rev (unsigned int v)
{
  unsigned int r = v; 
  int s = sizeof(v) * CHAR_BIT - 1; 

  for (v >>= 1; v; v >>= 1)
  {   
    r <<= 1;
    r |= v & 1;
    s--;
  }
  r <<= s; 

  return r;
}


static float *
bit_reverse (float * w, size_t w_size_bytes, unsigned int N, unsigned int bits)
{
  unsigned int i, s, shift;
  s = sizeof(i) * CHAR_BIT - 1;
  shift = s - bits + 1;

  for (i = 0; i < N; i++) {
    unsigned int r;
    float t_real, t_imag;
    r = _rev (i);
    r >>= shift;

    if (i < r) {
      t_real = w[2 * i];
      t_imag = w[2 * i + 1];
      w[2 * i] = w[2 * r];
      w[2 * i + 1] = w[2 * r + 1];
      w[2 * r] = t_real;
      w[2 * r + 1] = t_imag;
    }
  }

  return w;
}


int
fft (float * data, size_t data_size_bytes, unsigned int N, unsigned int logn, int sign)
{
  unsigned int transform_length;
  unsigned int a, b, i, j, bit;
  float theta, t_real, t_imag, w_real, w_imag, s, t, s2, z_real, z_imag;

  transform_length = 1;

  /* bit reversal */
  bit_reverse (data, data_size_bytes, N, logn);

  /* calculation */
  for (bit = 0; bit < logn; bit++) {
    w_real = 1.0;
    w_imag = 0.0;

    theta = 1.0 * sign * M_PI / (float) transform_length;

    s = sin (theta);
    t = sin (0.5 * theta);
    s2 = 2.0 * t * t;

    for (a = 0; a < transform_length; a++) {
      for (b = 0; b < N; b += 2 * transform_length) {
	i = b + a;
	j = b + a + transform_length;

	z_real = data[2*j  ];
	z_imag = data[2*j+1];

	t_real = w_real * z_real - w_imag * z_imag;
	t_imag = w_real * z_imag + w_imag * z_real;

	/* write the result */
	data[2*j  ]  = data[2*i  ] - t_real;
	data[2*j+1]  = data[2*i+1] - t_imag;
	data[2*i  ] += t_real;
	data[2*i+1] += t_imag;
      }

      /* adjust w */
      t_real = w_real - (s * w_imag + s2 * w_real);
      t_imag = w_imag + (s * w_real - s2 * w_imag);
      w_real = t_real;
      w_imag = t_imag;

    }

    transform_length *= 2;
  }

  return 0;
}


typedef struct {
  float max_psd;
  unsigned int max_index;
} avg_max_t;


avg_max_t
calc_avg_max(float* data, size_t data_size_bytes)
{
  avg_max_t ret_val;
  ret_val.max_psd   = 0;
  ret_val.max_index = data_size_bytes; /* A too large value */
  
  unsigned int i;
  float temp;
  for (i=0; i < RADAR_N; i++) {
    temp = (pow(data[2*i],2) + pow(data[2*i+1],2))/100.0;
    if (temp > ret_val.max_psd) {
      ret_val.max_psd = temp;
      ret_val.max_index = i;
    }
  }
  return ret_val;
}


void
calculate_peak_dist_from_fmcw(float* inputs, size_t data_size_bytes, unsigned int N, unsigned int logn, int sign, float * distance, size_t dist_size)
{
  /* __visc__hint(CPU_TARGET); */
  /* __visc__attributes(2, data, distance, 1, distance); */

  fft (inputs, data_size_bytes, N, logn, sign);

  avg_max_t avg_max = calc_avg_max(inputs, data_size_bytes);

  float dist = INFINITY;
  if (avg_max.max_psd > 1e-10*pow(8192,2)) {
    dist = ((float)(avg_max.max_index*((float)RADAR_fs)/((float)(RADAR_N))))*0.5*RADAR_c/((float)(RADAR_alpha));
    //DEBUG(printf("Max distance is %.3f\nMax PSD is %4E\nMax index is %d\n", distance, max_psd, max_index));
    /* *distance = distance; */
    /* } else { */
    /*   *distance = INFINITY; */
  }
  *distance = dist;
  //__visc__return(1, dist_size)
}




typedef struct __attribute__((__packed__)) {
  float * data;       size_t bytes_data;
  unsigned int N;
  unsigned int logn;
  int sign;
  float * distance;   size_t bytes_distance;
} RootIn;


void miniERARoot(/* 0 */ float * data, size_t bytes_data, /* 1 */
		 /* 2 */ unsigned int N,
		 /* 3 */ unsigned int logn,
		 /* 4 */ int sign,
		 /* 5 */ float * distance, size_t bytes_distance /* 6 */) {

  
  //Specifies compilation target for current node
  __visc__hint(CPU_TARGET);

  // Specifies pointer arguments that will be used as "in" and "out" arguments
  // - count of "in" arguments
  // - list of "in" argument , and similar for "out"
  __visc__attributes(5, data, N, logn, sign, distance, // - count of "in" arguments, list of "in" arguments
		     1, distance);		       // - count of "out" arguments, list of "out" arguments

  // FFT Node
  void* EXEC_RAD_node = __visc__createNodeND(0, execute_rad_kernel);

  // Viterbi Node
  //void* EXEC_VIT_node = __visc__createNodeND(0, viterbi_node_function);

  // CV Nodes
  // nodes generated from DNN compiled from Keras here

  // Plan and Control Node
  //void* PC_node = __visc__createNodeND(0, planAndControl_node_function);

  // BindIn binds inputs of current node with specified node
  // - destination node
  // - argument position in argument list of function of source node
  // - argument position in argument list of function of destination node
  // - streaming (1) or non-streaming (0)

  // scale_fxp inputs
  __visc__bindIn(EXEC_RAD_node, 0, 0, 0); // data -> EXEC_RAD_node:data
  __visc__bindIn(EXEC_RAD_node, 1, 1, 0); // bytes_data -> EXEC_RAD_node:bytes_data
  __visc__bindIn(EXEC_RAD_node, 2, 2, 0); // N -> EXEC_RAD_node:N
  __visc__bindIn(EXEC_RAD_node, 3, 3, 0); // logn -> EXEC_RAD_node:logn
  __visc__bindIn(EXEC_RAD_node, 4, 4, 0); // sign -> EXEC_RAD_node:sign
  __visc__bindIn(EXEC_RAD_node, 5, 5, 0); // distance -> EXEC_RAD_node:distance
  __visc__bindIn(EXEC_RAD_node, 6, 6, 0); // bytes_dist -> EXEC_RAD_node:bytes_dist

  // Edge transfers data between nodes within the same level of hierarchy.
  // - source and destination dataflow nodes
  // - edge type, all-all (1) or one-one(0)
  // - source position (in output struct of source node)
  // - destination position (in argument list of destination node)
  // - streaming (1) or non-streaming (0)

  //__visc__edge(EXEC_RAD_node, PC_node, 1, , , 0);
  //__visc__edge(EXEC_VIT_node, PC_node, 1, , , 0);
  //__visc__edge(/* last of Keras nodes */, PC_node, 1, , , 0); // tensor result
  //__visc__edge(/* last of Keras nodes */, PC_node, 1, , , 0); // size of tensor

  // Similar to bindIn, but for the output. Output of a node is a struct, and
  // we consider the fields in increasing ordering.
  __visc__bindOut(EXEC_RAD_node, 0, 0, 0);
}


int main(int argc, char *argv[])
{
  vehicle_state_t vehicle_state;
  /* label_t label; */
  /* distance_t distance; */
  /* message_t message; */

  char* trace_file; 
  int opt; 
      
  // put ':' in the starting of the 
  // string so that program can  
  // distinguish between '?' and ':'
  while((opt = getopt(argc, argv, ":ht:v:")) != -1) {  
    switch(opt) {  
    case 'h':
      print_usage(argv[0]);
      exit(0);
    case 't':
      trace_file = optarg;
      printf("Using trace file: %s\n", trace_file);
      break;
    case 'v':
      vit_msgs_behavior = atoi(optarg);
      printf("Using viterbi behavior %u\n", vit_msgs_behavior);
      break;
    case ':':
      printf("option needs a value\n");
      break;  
    case '?':
      printf("unknown option: %c\n", optopt); 
    break;
    }  
  }  
      
  // optind is for the extra arguments 
  // which are not parsed 
  for(; optind < argc; optind++){      
    printf("extra arguments: %s\n", argv[optind]);  
  } 
  
  
  /* We plan to use three separate trace files to drive the three different kernels
   * that are part of mini-ERA (CV, radar, Viterbi). All these three trace files
   * are required to have the same base name, using the file extension to indicate
   * which kernel the trace corresponds to (cv, rad, vit).
   */
  /* if (argc != 2) */
  /* { */
  /*   printf("Usage: %s <trace_basename>\n\n", argv[0]); */
  /*   printf("Where <trace_basename> is the basename of the trace files to load:\n"); */
  /*   printf("  <trace_basename>.cv  : trace to feed the computer vision kernel\n"); */
  /*   printf("  <trace_basename>.rad : trace to feed the radar (FFT-1D) kernel\n"); */
  /*   printf("  <trace_basename>.vit : trace to feed the Viterbi decoding kernel\n"); */

  /*   return 1; */
  /* } */


  /* Trace filename construction */
  /* char * trace_file = argv[1]; */
  printf("Input trace file: %s\n", trace_file);


  /* Trace Reader initialization */
  if (!init_trace_reader(trace_file))
  {
    printf("Error: the trace reader couldn't be initialized properly.\n");
    return 1;
  }

  /* Kernels initialization */
  if (!init_cv_kernel(cv_dict))
  {
    printf("Error: the computer vision kernel couldn't be initialized properly.\n");
    return 1;
  }
  if (!init_rad_kernel(rad_dict))
  {
    printf("Error: the radar kernel couldn't be initialized properly.\n");
    return 1;
  }
  if (!init_vit_kernel(vit_dict))
  {
    printf("Error: the Viterbi decoding kernel couldn't be initialized properly.\n");
    return 1;
  }

  /* We assume the vehicle starts in the following state:
   *  - Lane: center
   *  - Speed: 50 mph
   */
  vehicle_state.lane  = center;
  vehicle_state.speed = 50;
  DEBUG(printf("Vehicle starts with the following state: lane %u speed %.1f\n", vehicle_state.lane, vehicle_state.speed));
  /*** MAIN LOOP -- iterates until all the traces are fully consumed ***/
  #ifdef TIME
         int loop=0;
         struct timeval stop, start;
  #endif


  // Allocate struct to pass DFG inputs
  RootIn* rootArgs = (RootIn*) malloc(sizeof(RootIn));

  /* Initialize the HPVM environment, etc. (Only done once) */
  __visc__init();

  /* Declare memories used in the while loop but tracked by HPVM */
  // Memory tracking is required for pointer arguments.
  // Nodes can be scheduled on different targets, and 
  // dataflow edge implementation needs to request data.
  // The pair (pointer, size) is inserted in memory tracker using this call
  float radar_input[2*RADAR_N];
  distance_t radar_distance;
  llvm_visc_track_mem(radar_input, 8*RADAR_N);
  llvm_visc_track_mem(&radar_distance, sizeof(float));


  /* The input trace contains the per-epoch (time-step) input data */
  read_next_trace_record(vehicle_state);
  while (!eof_trace_reader())
  {
    DEBUG(printf("Vehicle_State: Lane %u %s Speed %.1f\n", vehicle_state.lane, lane_names[vehicle_state.lane], vehicle_state.speed));

    // Iterate the various kernels (PREP their states for execution, get inputs, etc.)
    /* The computer vision kernel performs object recognition on the
     * next image, and returns the corresponding label. 
     * This process takes place locally (i.e. within this car).
     */
    label_t cv_tr_label = iterate_cv_kernel(vehicle_state);


    /* The radar kernel performs distance estimation on the next radar
     * data, and returns the estimated distance to the object.
     */
    radar_dict_entry_t* rdentry_p = iterate_rad_kernel(vehicle_state);
    distance_t rd_dist = rdentry_p->distance;
    float * ref_in = rdentry_p->return_data;
    for (int ii = 0; ii < 2*RADAR_N; ii++) {
      radar_input[ii] = ref_in[ii];
    }

    /* The Viterbi decoding kernel performs Viterbi decoding on the next
     * OFDM symbol (message), and returns the extracted message.
     * This message can come from another car (including, for example,
     * its 'pose') or from the infrastructure (like speed violation or
     * road construction warnings). For simplicity, we define a fix set
     * of message classes (e.g. car on the right, car on the left, etc.)
     */
    vit_dict_entry_t* vdentry_p = iterate_vit_kernel(vehicle_state);

    // Here we will simulate multiple cases, based on global vit_msgs_behavior
    int num_vit_msgs = 1;   // the number of messages to send this time step (1 is default)
    switch(vit_msgs_behavior) {
    case 2: num_vit_msgs = total_obj; break;
    case 3: num_vit_msgs = total_obj; break;
    case 4: num_vit_msgs = total_obj + 1; break;
    case 5: num_vit_msgs = total_obj + 1; break;
    }


    // EXECUTE the kernels using the now known inputs 
    label_t cv_infer_label = execute_cv_kernel(cv_tr_label);
    // Set up HPVM DFG inputs in the rootArgs struct.
    rootArgs->data = radar_input;
    rootArgs->bytes_data = 8*RADAR_N;
  
    rootArgs->N = RADAR_N;
    rootArgs->logn = RADAR_LOGN;
    rootArgs->sign = -1;

    rootArgs->distance   = &radar_distance;
    rootArgs->bytes_distance = sizeof(float);
    
    // Launch the DFG to do the radar computation
    //distance_t radar_dist  = execute_rad_kernel(radar_input, 8*RADAR_N, RADAR_N, RADAR_LOGN, -1, distance, sizeof(float));
    void* radarExecDFG = __visc__launch(0, miniERARoot, (void*) rootArgs);
    __visc__wait(radarExecDFG);

    // Request data from graph.    
    llvm_visc_request_mem(&radar_distance, sizeof(float));

    
    message_t vit_message;
    char out_msg_text[1600];
    execute_vit_kernel(&(vdentry_p->ofdm_p),  sizeof(ofdm_param),
		       &(vdentry_p->frame_p), sizeof(frame_param),
		       vdentry_p->in_bits,    MAX_ENCODED_BITS,
		       out_msg_text,          1600,
		       &vit_message,          sizeof(message_t));
    
    // POST-EXECUTE each kernels to gather stats, etc.
    post_execute_cv_kernel(cv_tr_label, cv_infer_label);
    post_execute_rad_kernel(rd_dist, radar_distance);
    for (int mi = 0; mi < num_vit_msgs; mi++) {
      post_execute_vit_kernel(vdentry_p->msg_id, vit_message);
    }
    

    /* The plan_and_control() function makes planning and control decisions
     * based on the currently perceived information. It returns the new
     * vehicle state.
     */
    vehicle_state = plan_and_control(cv_infer_label, radar_distance, vit_message, vehicle_state);
    DEBUG(printf("New vehicle state: lane %u speed %.1f\n\n", vehicle_state.lane, vehicle_state.speed));
    
    #ifdef TIME  
          loop++;
          if (loop == 1) { 
  	  gettimeofday(&start, NULL);
	  }
    #endif	  
    read_next_trace_record(vehicle_state);
  }

  #ifdef TIME
  	gettimeofday(&stop, NULL);
  #endif 

  /* All the traces have been fully consumed. Quitting... */
  closeout_cv_kernel();
  closeout_rad_kernel();
  closeout_vit_kernel();

  #ifdef TIME
  	printf("Program run time in milliseconds %f\n", (double) (stop.tv_sec - start.tv_sec) * 1000 + (double) (stop.tv_usec - start.tv_usec) / 1000);
  #endif

  // Remove tracked pointers.
  llvm_visc_untrack_mem(radar_input);
  llvm_visc_untrack_mem(&radar_distance);

  __visc__cleanup();

  printf("\nDone.\n");
  return 0;
}
