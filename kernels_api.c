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

#include <Python.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "kernels_api.h"

#ifdef VERBOSE
char* lane_names[NUM_LANES] = {"LHazard", "Left", "Center", "Right", "RHazard" };
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
#include "viterbi/utils.h"
#include "viterbi/viterbi_decoder_generic.h"
#include "radar/calc_fmcw_dist.h"


PyObject *pName, *pModule, *pFunc, *pFunc_load;
PyObject *pArgs, *pValue, *pretValue;
#define PY_SSIZE_T_CLEAN

char *python_module = "mio";
char *python_func = "predict";	  
char *python_func_load = "loadmodel";	  

/* File pointer to the input trace */
FILE *input_trace = NULL;


/* These are some top-level defines needed for CV kernel */
#define IMAGE_SIZE  32  // What size are these?
typedef struct {
  unsigned int image_id;
  label_t  object;
  unsigned image_data[IMAGE_SIZE];
} cv_dict_entry_t;

/** The CV kernel uses a different method to select appropriate inputs; dictionary not needed
unsigned int     num_cv_dictionary_items = 0;
cv_dict_entry_t* the_cv_object_dict;
**/
unsigned label_match[NUM_OBJECTS+1] = {0, 0, 0, 0, 0, 0};  // Times CNN matched dictionary
unsigned label_lookup[NUM_OBJECTS+1] = {0, 0, 0, 0, 0, 0}; // Times we used CNN for object classification
  

/* These are some top-level defines needed for RADAR */
typedef struct {
  unsigned int return_id;
  float distance;
  float return_data[2 * RADAR_N];
} radar_dict_entry_t;

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
typedef struct {
  unsigned int msg_num;
  unsigned int msg_id;
  ofdm_param   ofdm_p;
  frame_param  frame_p;
  uint8_t      in_bits[MAX_ENCODED_BITS];
} vit_dict_entry_t;

uint8_t descramble[1600]; // I think this covers our max use cases
uint8_t actual_msg[1600];

unsigned int      num_viterbi_dictionary_items = 0;
vit_dict_entry_t* the_viterbi_trace_dict;

unsigned vit_msgs_behavior = 0; // 0 = default
unsigned total_msgs = 0; // Total messages decoded during the full run
  

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
  if (fscanf(dictF, "%u\n", &num_radar_dictionary_items)) ;
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
    if (fscanf(dictF, "%u %f", &entry_id, &entry_dist)) ;
    DEBUG(printf("  Reading rad dictionary entry %u : %u %f\n", di, entry_id, entry_dist));
    the_radar_return_dict[di].return_id = entry_id;
    the_radar_return_dict[di].distance =  entry_dist;
    for (int i = 0; i < 2*RADAR_N; i++) {
      float fin;
      if (fscanf(dictF, "%f", &fin)) ;
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
  if (fscanf(dictF, "%u\n", &num_viterbi_dictionary_items)) ;
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
    if (fscanf(dictF, "%d %d\n", &mnum, &mid)) ;
    DEBUG(printf(" V_MSG: num %d Id %d\n", mnum, mid));
    if (mnum != i) {
      printf("ERROR : Check Viterbi Dictionary : i = %d but Mnum = %d  (Mid = %d)\n", i, mnum, mid);
      exit(-5);
    }
    the_viterbi_trace_dict[i].msg_id = mnum;
    the_viterbi_trace_dict[i].msg_id = mid;

    int in_bpsc, in_cbps, in_dbps, in_encoding, in_rate; // OFDM PARMS
    if (fscanf(dictF, "%d %d %d %d %d\n", &in_bpsc, &in_cbps, &in_dbps, &in_encoding, &in_rate)) ;
    DEBUG(printf("  OFDM: %d %d %d %d %d\n", in_bpsc, in_cbps, in_dbps, in_encoding, in_rate));
    the_viterbi_trace_dict[i].ofdm_p.encoding   = in_encoding;
    the_viterbi_trace_dict[i].ofdm_p.n_bpsc     = in_bpsc;
    the_viterbi_trace_dict[i].ofdm_p.n_cbps     = in_cbps;
    the_viterbi_trace_dict[i].ofdm_p.n_dbps     = in_dbps;
    the_viterbi_trace_dict[i].ofdm_p.rate_field = in_rate;

    int in_pdsu_size, in_sym, in_pad, in_encoded_bits, in_data_bits;
    if (fscanf(dictF, "%d %d %d %d %d\n", &in_pdsu_size, &in_sym, &in_pad, &in_encoded_bits, &in_data_bits)) ;
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
      if (fscanf(dictF, "%u ", &c)) ;
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

status_t init_cv_kernel(char* py_file, char* dict_fn)
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

  /* 2) Conduct object detection on the image frame */
  // Call Keras Code  
  label_t object = run_object_classification(tr_val); 
  //label_t object = the_cv_object_dict[tr_val].object;

  //unsigned * inputs = the_cv_object_dict[tr_val].image_data;
  //DEBUG(printf("  Using obj %u : object %u\n", tr_val, the_cv_object_dict[tr_val].object));
  
  /* 3) Return the label corresponding to the recognized object */
  DEBUG(printf("  Returning d_object %u %s : object %u %s\n", d_object, object_names[d_object], object, object_names[object]));
  if (d_object == object) {
    label_match[d_object]++;
    label_match[NUM_OBJECTS]++;
  }
  label_lookup[NUM_OBJECTS]++;
  label_lookup[d_object]++;
  return d_object;
}

distance_t iterate_rad_kernel(vehicle_state_t vs)
{
  DEBUG(printf("In iterate_rad_kernel\n"));

  unsigned tr_val = nearest_dist[vs.lane] / RADAR_BUCKET_DISTANCE;  // The proper message for this time step and car-lane
  
  distance_t ddist = the_radar_return_dict[tr_val].distance;
  distance_t dist;      // The output from this routine

  // We have to make a working copy of the inputs -- I think the calculate_peak_dist_from_fmcw alters the input data space
  float inputs[2*RADAR_N];
  float * ref_in = the_radar_return_dict[tr_val].return_data;
  for (int ii = 0; ii < 2*RADAR_N; ii++) {
    inputs[ii] = ref_in[ii];
  }
  
  DEBUG(printf("  Using dist tr_val %u : in meters %f\n", tr_val, the_radar_return_dict[tr_val].distance));
  
  /* 2) Conduct distance estimation on the waveform */
  DEBUG(printf("  Calling calculate_peak_dist_from_fmcw\n"));
  dist = calculate_peak_dist_from_fmcw(inputs);

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
  /* 3) Return the estimated distance */
  float tdist = 1.0*nearest_dist[vs.lane];
  DEBUG(printf("  Returning distance %f (vs %f and %f)\n", tdist, dist, ddist));

  //return dist;
  return tdist; // ddist;
}


/* Each time-step of the trace, we read in the 
 * trace values for the left, middle and right lanes
 * (i.e. which message if the autonomous car is in the 
 *  left, middle or right lane).
 */
message_t iterate_vit_kernel(vehicle_state_t vs)
{
  hist_total_objs[total_obj]++;
  
  /* unsigned tr_msg_vals[NUM_LANES] = { 1, 1, 0, 2, 2}; // Defaults for all lanes clear : LH = only R; LL,CL,RL = L or R; RH = only L */
  /* if ((nearest_obj[2] != 'N') && (nearest_dist[2] < VIT_CLEAR_THRESHOLD)) {  */
  /*   // Some object is in the Center lane at distance 0 or 1 */
  /*   tr_msg_vals[1] = 3; // Unsafe to move from left  lane to right or left. */
  /*   tr_msg_vals[3] = 3; // Unsafe to move from right lane to right or left. */
  /* } */
  /* if ((nearest_obj[1] != 'N') && (nearest_dist[1] < VIT_CLEAR_THRESHOLD)) {  */
  /*   // Some object is in the Left lane at distance 0 or 1 */
  /*   tr_msg_vals[2] += 1; // Unsafe to move from center lane to the left. */
  /* } */
  /* if ((nearest_obj[3] != 'N') && (nearest_dist[3] < VIT_CLEAR_THRESHOLD)) {  */
  /*   // Some object is in the Right lane at distance 0 or 1 */
  /*   tr_msg_vals[2] += 2; // Unsafe to move from center lane to the right. */
  /* } */
  
  /* DEBUG(printf("  Tr_Msgs: %u %u %u\n", tr_msg_vals[0], tr_msg_vals[1], tr_msg_vals[2])); */
  /* unsigned tr_val = tr_msg_vals[vs.lane];  // The proper message for this time step and car-lane */

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
  char the_msg[1600]; // Return from decode; large enough for any of our messages
  message_t msg;      // The output from this routine

  vit_dict_entry_t* trace_msg; // Will hold msg input data for decode, based on trace input

    // Here we will simulate multiple cases, based on global vit_msgs_behavior
  int num_msgs = 1;   // the number of messages to send this time step (1 is default)
  int msg_offset = 0; // 0 = short messages, 4 = long messages
  switch(vit_msgs_behavior) {
  case 0: break;
  case 1: msg_offset = 4; break;
  case 2: num_msgs = total_obj; break;
  case 3: num_msgs = total_obj; msg_offset = 4; break;
  case 4: num_msgs = total_obj + 1; break;
  case 5: num_msgs = total_obj + 1; msg_offset = 4; break;
  }

  switch(tr_val) {
  case 0: // safe_to_move_right_or_left
    trace_msg = &(the_viterbi_trace_dict[0 + msg_offset]);
    msg = safe_to_move_right_or_left;  // Cheating - already know output result.
    DEBUG(printf("  Using msg %u : %s = safe_to_move_right_or_left\n", msg, message_names[msg]));
    break;
  case 1: // safe_to_move_right
    trace_msg = &(the_viterbi_trace_dict[1 + msg_offset]);
    msg = safe_to_move_right_only;  // Cheating - already know output result.
    DEBUG(printf("  Using msg %u : %s = safe_to_move_right_only\n", msg, message_names[msg]));
    break;
  case 2: // safe_to_move_left
    trace_msg = &(the_viterbi_trace_dict[2 + msg_offset]);
    msg = safe_to_move_left_only;  // Cheating - already know output result.
    DEBUG(printf("  Using msg %u : %s = safe_to_move_left_only\n", msg, message_names[msg]));
    break;
  case 3: // unsafe_to_move_left_or_right
    trace_msg = &(the_viterbi_trace_dict[3 + msg_offset]);
    msg = unsafe_to_move_left_or_right;  // Cheating - already know output result.
    DEBUG(printf("  Using msg %u : %s = unsafe_to_move_right_or_left\n", msg, message_names[msg]));
    break;
  }

  // Send each message (here they are all the same) through the viterbi decoder
  uint8_t *result;
  for (int mi = 0; mi < num_msgs; mi++) {
    DEBUG(printf("  Calling the viterbi decode routine for %s message %u\n", (msg_offset == 0) ? "short" : "long", mi));
    result = decode(&(trace_msg->ofdm_p), &(trace_msg->frame_p), &(trace_msg->in_bits[0]));
    total_msgs++;
    // descramble the output - put it in result
    int psdusize = trace_msg->frame_p.psdu_size;
    DEBUG(printf("  Calling the viterbi descrambler routine\n"));
    descrambler(result, psdusize, the_msg, NULL /*descram_ref*/, NULL /*msg*/);

    // Here we could llllook at the message string and do something,
    //  but we already know the right answeer...
  }
  
  // Can check contents of "the_msg" to determine which message;
  //   here we "cheat" and return the message indicated by the trace.
  DEBUG(printf("The iterate_vit_kernel is returning msg %u\n", msg));
  
  return msg;
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
