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
//#include "fft-1d.h"
#include <stdint.h>
#include <math.h>
#include <limits.h>

#include "kernels_api.h"


#include "calc_fmcw_dist.h"
#include "visc.h"
#define BYPASS_KERAS_CV_CODE  // INITIAL  BRING-UP

#define TIME
char * cv_dict  = "traces/objects_dictionary.dfn";
char * rad_dict = "traces/radar_dictionary.dfn";
char * vit_dict = "traces/vit_dictionary.dfn";


// This controls whether we output the Visualizer trace
bool output_viz_trace = false;
  


void print_usage(char * pname) {
  printf("Usage: %s <OPTIONS>\n", pname);
  printf(" OPTIONS:\n");
  printf("    -h         : print this help information\n");
  printf("    -o         : output the Visualizer trace (to stdout)\n");
  printf("    -t <trace> : defines the input trace file to use\n");
  printf("    -v <N>     : defines Viterbi messaging behavior:\n");
  printf("               :      0 = One short message per time step\n");
  printf("               :      1 = One long  message per time step\n");
  //printf("               :  NOTE: Any other value currently ignored\n");
  printf("               :      2 = One short message per obstacle per time step\n");
  printf("               :      3 = One long  message per obstacle per time step\n");
  printf("               :      4 = One short msg per obstacle + 1 per time step\n");
  printf("               :      5 = One long  msg per obstacle + 1 per time step\n");
}




char* lane_names[NUM_LANES+1] = {"LHazard", "Left", "Center", "Right", "RHazard", "Unknown" };
char* message_names[NUM_MESSAGES+1] = {"Safe_L_or_R", "Safe_R_only", "Safe_L_only", "Unsafe_L_or_R", "Unknowwn" };
char* object_names[NUM_OBJECTS+1] = {"Nothing", "Bike", "Car", "Person", "Truck", "Unknown" };

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

unsigned int      num_viterbi_dictionary_items = 0;
vit_dict_entry_t* the_viterbi_trace_dict;

unsigned vit_msgs_behavior = 0; // 0 = default
unsigned total_msgs = 0; // Total messages decoded during the full run
unsigned bad_decode_msgs = 0; // Total messages decoded incorrectly during the full run
  



/*********************************************************************************
 * The following is code to support the initialization of the simulation run
 *********************************************************************************/

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
      //DEBUG(printf("%u ", c));
      the_viterbi_trace_dict[i].in_bits[ci] = (uint8_t)c;
    }
    //DEBUG(printf("\n"));
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
  //DEBUG(
  if (output_viz_trace) {
    printf("  VizTrace: %u,%s\n", vs.lane, in_line_buf);//);
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


void execute_cv_kernel(/* 0 */ label_t* in_tr_val, size_t in_tr_val_size, /* 1 */
		       /* 2 */ label_t* out_label, size_t out_label_size  /* 3 */)
{
  __visc__hint(DEVICE);
  __visc__attributes(2, in_tr_val, out_label, 1, out_label);

  /* 2) Conduct object detection on the image frame */
  // Should replace with nodes generated from DNN compiled from Keras here?
  *out_label = run_object_classification(*in_tr_val);
  
  __visc__return(1, out_label_size);
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



/*********************************************************************************
 * The following is code to support the RADAR kernel
 *********************************************************************************/
radar_dict_entry_t* iterate_rad_kernel(vehicle_state_t vs)
{
  DEBUG(printf("In iterate_rad_kernel\n"));

  unsigned tr_val = nearest_dist[vs.lane] / RADAR_BUCKET_DISTANCE;  // The effective "radar bucket" distance

  /* DEBUG(printf("  Using dist tr_val %u : in meters %f\n", tr_val, ddist)); */
  return &(the_radar_return_dict[tr_val]);
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


void fft(/* 0 */ float * data, size_t data_size_bytes, /* 1 */ 
	 /* 2 */ unsigned int N,
	 /* 3 */ unsigned int logn,
	 /* 4 */ int sign)
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
}


void get_dist_from_fft(/* 0 */ float * distance, size_t dist_size, /* 1 */ 
		       /* 2 */ unsigned int input_N,
		       /* 3 */ float* data, size_t data_size_bytes /* 4 */ )
{
  float max_psd   = 0.0;
  unsigned int max_index = data_size_bytes; /* A too large value */
  
  float temp;
  for (int i = 0; i < input_N; i++) {
    temp = (pow(data[2*i],2) + pow(data[2*i+1],2))/100.0;
    if (temp > max_psd) {
      max_psd = temp;
      max_index = i;
    }
  }

  float dist = INFINITY;
  if (max_psd > 1e-10*pow(8192,2)) {
    dist = ((float)(max_index*((float)RADAR_fs)/((float)(input_N))))*0.5*RADAR_c/((float)(RADAR_alpha));
  }
  *distance = dist;
}


void execute_rad_kernel(float * inputs, size_t input_size_bytes,
			unsigned int input_N, unsigned int in_logn, int in_sign,
			distance_t * distance, size_t dist_size)
{
  __visc__hint(DEVICE);
  __visc__attributes(2, inputs, distance, 1, distance);

  /* 2) Conduct distance estimation on the waveform */
  // The fft routine reads from inputs and writes results in the same memory
  fft (inputs, input_size_bytes, input_N, in_logn, in_sign);

  // get_dist_from_fft takes fft output and puts a distance into distance
  get_dist_from_fft(distance, dist_size, input_N, inputs, input_size_bytes);

  // Return the SIZE -- the pointer is transferred by a __visc__bind
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


/*********************************************************************************
 * The following is code to support the RADAR kernel
 *********************************************************************************/

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
    {
      unsigned nd_1 = RADAR_BUCKET_DISTANCE * (nearest_dist[1] / RADAR_BUCKET_DISTANCE); // floor by bucket...
      if ((nearest_obj[1] != 'N') && (nd_1 /*nearest_dist[1]*/ < VIT_CLEAR_THRESHOLD)) {  
	// Some object is in the left lane within threshold distance
	tr_val = 3; // Unsafe to move from lhazard lane into the left lane 
      } else {
	tr_val = 1;
      }
    }
    break;
  case left:
  case center:
  case right:
    {
      unsigned ndp1 = RADAR_BUCKET_DISTANCE * (nearest_dist[vs.lane+1] / RADAR_BUCKET_DISTANCE); // floor by bucket...
      unsigned ndm1 = RADAR_BUCKET_DISTANCE * (nearest_dist[vs.lane-1] / RADAR_BUCKET_DISTANCE); // floor by bucket...
      tr_val = 0;
      DEBUG(printf("  Lane %u : obj in %u is %c at %u : obj in %u is %c at %u\n", vs.lane, 
		   vs.lane-1, nearest_obj[vs.lane-1], nearest_dist[vs.lane-1],
		   vs.lane+1, nearest_obj[vs.lane+1], nearest_dist[vs.lane+1]));
      if ((nearest_obj[vs.lane-1] != 'N') && (ndm1 /*nearest_dist[vs.lane-1]*/ < VIT_CLEAR_THRESHOLD)) {
	// Some object is in the Left lane at distance 0 or 1
	DEBUG(printf("    Marking unsafe to move left\n"));
	tr_val += 1; // Unsafe to move from this lane to the left.
      }
      if ((nearest_obj[vs.lane+1] != 'N') && (ndp1 /*nearest_dist[vs.lane+1]*/ < VIT_CLEAR_THRESHOLD)) {
	// Some object is in the Right lane at distance 0 or 1
	DEBUG(printf("    Marking unsafe to move right\n"));
	tr_val += 2; // Unsafe to move from this lane to the right.
      }
    }
    break;
  case rhazard:
    {
      unsigned nd_3 = RADAR_BUCKET_DISTANCE * (nearest_dist[3] / RADAR_BUCKET_DISTANCE); // floor by bucket...
      if ((nearest_obj[3] != 'N') && (nd_3 /*nearest_dist[3]*/ < VIT_CLEAR_THRESHOLD)) {
	// Some object is in the right lane within threshold distance
	tr_val = 3; // Unsafe to move from center lane to the right.
      } else {
	tr_val = 2;
      }
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
  DEBUG(printf(" VIT: Using msg %u Id %u : %s \n", trace_msg->msg_num, trace_msg->msg_id, message_names[trace_msg->msg_id]));
  return trace_msg;
}


// #include "viterbi_decoder_generic.c"
/*
 * Copyright 1995 Phil Karn, KA9Q
 * Copyright 2008 Free Software Foundation, Inc.
 * 2014 Added SSE2 implementation Bogdan Diaconescu
 *
 * This file is part of GNU Radio
 *
 * GNU Radio is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * GNU Radio is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Radio; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

/*
 * Viterbi decoder for K=7 rate=1/2 convolutional code
 * Some modifications from original Karn code by Matt Ettus
 * Major modifications by adding SSE2 code by Bogdan Diaconescu
 */
#include "base.h"
#include "viterbi_decoder_generic.h"
//#include "viterbi_standalone.h"

// GLOBAL VARIABLES

// Position in circular buffer where the current decoded byte is stored
//static int d_store_pos = 0;
// Metrics for each state
//static unsigned char d_mmresult[64] __attribute__((aligned(16)));
// Paths for each state
//static unsigned char d_ppresult[TRACEBACK_MAX][64] __attribute__((aligned(16)));



uint8_t* depuncture(ofdm_param *ofdm,   size_t ofdm_size,
		    frame_param *frame, size_t frame_size,
		    int ntraceback,
		    int k_val,
		    unsigned char *d_depuncture_pattern,  size_t depunc_ptn_size,
		    uint8_t *in,        size_t in_size)
{
  int count;
  int n_cbps = ofdm->n_cbps;
  uint8_t *depunctured;
  //printf("Depunture call...\n");
  if (ntraceback == 5) {
    count = frame->n_sym * n_cbps;
    depunctured = in;
  } else {
    depunctured = d_depunctured;
    count = 0;
    for(int i = 0; i < frame->n_sym; i++) {
      for(int k = 0; k < n_cbps; k++) {
	while (d_depuncture_pattern[count % (2 * k_val)] == 0) {
	  depunctured[count] = 2;
	  count++;
	}

	// Insert received bits
	depunctured[count] = in[i * n_cbps + k];
	count++;

	while (d_depuncture_pattern[count % (2 * k_val)] == 0) {
	  depunctured[count] = 2;
	  count++;
	}
      }
    }
  }
  //printf("  depuncture count = %u\n", count);
  return depunctured;
}

/* The basic Viterbi decoder operation, called a "butterfly"
 * operation because of the way it looks on a trellis diagram. Each
 * butterfly involves an Add-Compare-Select (ACS) operation on the two nodes
 * where the 0 and 1 paths from the current node merge at the next step of
 * the trellis.
 *
 * The code polynomials are assumed to have 1's on both ends. Given a
 * function encode_state() that returns the two symbols for a given
 * encoder state in the low two bits, such a code will have the following
 * identities for even 'n' < 64:
 *
 * 	encode_state(n) = encode_state(n+65)
 *	encode_state(n+1) = encode_state(n+64) = (3 ^ encode_state(n))
 *
 * Any convolutional code you would actually want to use will have
 * these properties, so these assumptions aren't too limiting.
 *
 * Doing this as a macro lets the compiler evaluate at compile time the
 * many expressions that depend on the loop index and encoder state and
 * emit them as immediate arguments.
 * This makes an enormous difference on register-starved machines such
 * as the Intel x86 family where evaluating these expressions at runtime
 * would spill over into memory.
 */

// INPUTS/OUTPUTS:  All are 64-entry (bytes) arrays randomly accessed.
//    symbols : INPUT        : Array [  4 bytes ] 
//    mm0     : INPUT/OUTPUT : Array [ 64 bytes ]
//    mm1     : INPUT/OUTPUT : Array [ 64 bytes ]
//    pp0     : INPUT/OUTPUT : Array [ 64 bytes ] 
//    pp1     : INPUT/OUTPUT : Array [ 64 bytes ]
// d_branchtab27_generic[1].c[] : INPUT : Array [2].c[32] {GLOBAL}
//

void viterbi_butterfly2_generic(unsigned char *symbols,        size_t size_symbols,
				unsigned char *d_brtab27[2],   size_t size_brtab27,
				unsigned char *mm0,            size_t size_mm0,
				unsigned char *mm1,            size_t size_mm1,
				unsigned char *pp0,            size_t size_pp0,
				unsigned char *pp1,            size_t size_pp1 )
{
  //unsigned char *d_brtab27[2] = {&(d_branchtab27_generic[0].c[0]), &(d_branchtab27_generic[1].c[0])};

  // These are used to "virtually" rename the uses below (for symmetry; reduces code size)
  //  Really these are functionally "offset pointers" into the above arrays....
  unsigned char *metric0, *metric1;
  unsigned char *path0, *path1;

  // Operate on 4 symbols (2 bits) at a time

  unsigned char m0[16], m1[16], m2[16], m3[16], decision0[16], decision1[16], survivor0[16], survivor1[16];
  unsigned char metsv[16], metsvm[16];
  unsigned char shift0[16], shift1[16];
  unsigned char tmp0[16], tmp1[16];
  unsigned char sym0v[16], sym1v[16];
  unsigned short simd_epi16;
  unsigned int   first_symbol;
  unsigned int   second_symbol;

  // Set up for the first two symbols (0 and 1)
  metric0 = mm0;
  path0 = pp0;
  metric1 = mm1;
  path1 = pp1;
  first_symbol = 0;
  second_symbol = first_symbol+1;
  for (int j = 0; j < 16; j++) {
    sym0v[j] = symbols[first_symbol];
    sym1v[j] = symbols[second_symbol];
  }

  for (int s = 0; s < 2; s++) { // iterate across the 2 symbol groups
    // This is the basic viterbi butterfly for 2 symbols (we need therefore 2 passes for 4 total symbols)
    for (int i = 0; i < 2; i++) {
      if (symbols[first_symbol] == 2) {
	for (int j = 0; j < 16; j++) {
          //metsvm[j] = d_branchtab27_generic[1].c[(i*16) + j] ^ sym1v[j];
	  metsvm[j] = d_brtab27[1][(i*16) + j] ^ sym1v[j];
	  metsv[j] = 1 - metsvm[j];
	}
      }
      else if (symbols[second_symbol] == 2) {
	for (int j = 0; j < 16; j++) {
          //metsvm[j] = d_branchtab27_generic[0].c[(i*16) + j] ^ sym0v[j];
	  metsvm[j] = d_brtab27[0][(i*16) + j] ^ sym0v[j];
	  metsv[j] = 1 - metsvm[j];
	}
      }
      else {
	for (int j = 0; j < 16; j++) {
          //metsvm[j] = (d_branchtab27_generic[0].c[(i*16) + j] ^ sym0v[j]) + (d_branchtab27_generic[1].c[(i*16) + j] ^ sym1v[j]);
	  metsvm[j] = (d_brtab27[0][(i*16) + j] ^ sym0v[j]) + (d_brtab27[1][(i*16) + j] ^ sym1v[j]);
	  metsv[j] = 2 - metsvm[j];
	}
      }

      for (int j = 0; j < 16; j++) {
	m0[j] = metric0[(i*16) + j] + metsv[j];
	m1[j] = metric0[((i+2)*16) + j] + metsvm[j];
	m2[j] = metric0[(i*16) + j] + metsvm[j];
	m3[j] = metric0[((i+2)*16) + j] + metsv[j];
      }

      for (int j = 0; j < 16; j++) {
	decision0[j] = ((m0[j] - m1[j]) > 0) ? 0xff : 0x0;
	decision1[j] = ((m2[j] - m3[j]) > 0) ? 0xff : 0x0;
	survivor0[j] = (decision0[j] & m0[j]) | ((~decision0[j]) & m1[j]);
	survivor1[j] = (decision1[j] & m2[j]) | ((~decision1[j]) & m3[j]);
      }

      for (int j = 0; j < 16; j += 2) {
	simd_epi16 = path0[(i*16) + j];
	simd_epi16 |= path0[(i*16) + (j+1)] << 8;
	simd_epi16 <<= 1;
	shift0[j] = simd_epi16;
	shift0[j+1] = simd_epi16 >> 8;

	simd_epi16 = path0[((i+2)*16) + j];
	simd_epi16 |= path0[((i+2)*16) + (j+1)] << 8;
	simd_epi16 <<= 1;
	shift1[j] = simd_epi16;
	shift1[j+1] = simd_epi16 >> 8;
      }
      for (int j = 0; j < 16; j++) {
	shift1[j] = shift1[j] + 1;
      }

      for (int j = 0, k = 0; j < 16; j += 2, k++) {
	metric1[(2*i*16) + j] = survivor0[k];
	metric1[(2*i*16) + (j+1)] = survivor1[k];
      }
      for (int j = 0; j < 16; j++) {
	tmp0[j] = (decision0[j] & shift0[j]) | ((~decision0[j]) & shift1[j]);
      }

      for (int j = 0, k = 8; j < 16; j += 2, k++) {
	metric1[((2*i+1)*16) + j] = survivor0[k];
	metric1[((2*i+1)*16) + (j+1)] = survivor1[k];
      }
      for (int j = 0; j < 16; j++) {
	tmp1[j] = (decision1[j] & shift0[j]) | ((~decision1[j]) & shift1[j]);
      }

      for (int j = 0, k = 0; j < 16; j += 2, k++) {
	path1[(2*i*16) + j] = tmp0[k];
	path1[(2*i*16) + (j+1)] = tmp1[k];
      }
      for (int j = 0, k = 8; j < 16; j += 2, k++) {
	path1[((2*i+1)*16) + j] = tmp0[k];
	path1[((2*i+1)*16) + (j+1)] = tmp1[k];
      }
    }

    // Set up for the second two symbols (2 and 3)
    metric0 = mm1;
    path0 = pp1;
    metric1 = mm0;
    path1 = pp0;
    first_symbol = 2;
    second_symbol = first_symbol+1;
    for (int j = 0; j < 16; j++) {
      sym0v[j] = symbols[first_symbol];
      sym1v[j] = symbols[second_symbol];
    }
  }
}

//  Find current best path
// 
// INPUTS/OUTPUTS:  
//    RET_VAL     : (ignored)
//    mm0         : INPUT/OUTPUT  : Array [ 64 ]
//    pp0         : INPUT/OUTPUT  : Array [ 64 ] 
//    ntraceback  : INPUT         : int (I think effectively const for given run type; here 5 I think)
//    outbuf      : OUTPUT        : 1 byte
//    d_store_pos : GLOBAL IN/OUT : int (position in circular traceback buffer?)
//    d_mmresult  : GLOBAL OUTPUT : Array [ 64 bytes ] 
//    d_ppresult  : GLOBAL OUTPUT : Array [ntraceback][ 64 bytes ]

void viterbi_get_output_generic(unsigned char *mm0,                                    size_t mm0_size,
				unsigned char *pp0,                                    size_t pp0_size,
				int ntraceback,
				int* store_pos,                                      size_t size_store_pos,
				unsigned char*  mmresult __attribute__((aligned(16))), size_t mmres_size,
				unsigned char ppresult[TRACEBACK_MAX][64] __attribute__((aligned(16))), size_t ppres_size,
				unsigned char *outbuf,                                 size_t outbuf_size)
{
  int i;
  int bestmetric, minmetric;
  int beststate = 0;
  int pos = 0;
  int j;

  // circular buffer with the last ntraceback paths
  *store_pos = (*store_pos + 1) % ntraceback;

  for (i = 0; i < 4; i++) {
    for (j = 0; j < 16; j++) {
      mmresult[(i*16) + j] = mm0[(i*16) + j];
      ppresult[*store_pos][(i*16) + j] = pp0[(i*16) + j];
    }
  }

  // Find out the best final state
  bestmetric = mmresult[beststate];
  minmetric = mmresult[beststate];

  for (i = 1; i < 64; i++) {
    if (mmresult[i] > bestmetric) {
      bestmetric = mmresult[i];
      beststate = i;
    }
    if (mmresult[i] < minmetric) {
      minmetric = mmresult[i];
    }
  }

  // Trace back
  for (i = 0, pos = *store_pos; i < (ntraceback - 1); i++) {
    // Obtain the state from the output bits
    // by clocking in the output bits in reverse order.
    // The state has only 6 bits
    beststate = ppresult[pos][beststate] >> 2;
    pos = (pos - 1 + ntraceback) % ntraceback;
  }

  // Store output byte
  *outbuf = ppresult[pos][beststate];

  for (i = 0; i < 4; i++) {
    for (j = 0; j < 16; j++) {
      pp0[(i*16) + j] = 0;
      mm0[(i*16) + j] = mm0[(i*16) + j] - minmetric;
    }
  }

  //return bestmetric;
}


void viterbi_reset(ofdm_param *ofdm,   size_t ofdm_size,
		   unsigned char*  d_metric0_generic __attribute__ ((aligned(16))), size_t d_m0_size,
		   unsigned char*  d_metric1_generic __attribute__ ((aligned(16))), size_t d_m1_size,
		   unsigned char*  d_path0_generic __attribute__ ((aligned(16))),   size_t d_p0_size,
		   unsigned char*  d_path1_generic __attribute__ ((aligned(16))),   size_t d_p1_size,
		   unsigned char*  d_mmresult __attribute__((aligned(16))),         size_t mmres_size,
		   unsigned char d_ppresult[TRACEBACK_MAX][64] __attribute__((aligned(16))),         size_t ppres_size,
		   d_branchtab27_t* d_branchtab27_generic,                          size_t d_brt_size,
		   int* d_ntraceback,                    size_t ntrbk_size,
		   int* d_k,                             size_t d_k_size,
		   unsigned char *d_depuncture_pattern,  size_t depunc_ptn_size
		   )
{
  int i, j;

  for (i = 0; i < 4; i++) {
    d_metric0_generic[i] = 0;
    d_path0_generic[i] = 0;
  }

  int polys[2] = { 0x6d, 0x4f };
  for(i=0; i < 32; i++) {
    d_branchtab27_generic[0].c[i] = (polys[0] < 0) ^ PARTAB[(2*i) & abs(polys[0])] ? 1 : 0;
    d_branchtab27_generic[1].c[i] = (polys[1] < 0) ^ PARTAB[(2*i) & abs(polys[1])] ? 1 : 0;
  }

  for (i = 0; i < 64; i++) {
    d_mmresult[i] = 0;
    for (j = 0; j < TRACEBACK_MAX; j++) {
      d_ppresult[j][i] = 0;
    }
  }

  switch(ofdm->encoding) {
  case BPSK_1_2:
  case QPSK_1_2:
  case QAM16_1_2:
    *d_ntraceback = 5;
    d_depuncture_pattern = PUNCTURE_1_2;
    *d_k = 1;
    break;
  case QAM64_2_3:
    *d_ntraceback = 9;
    d_depuncture_pattern = PUNCTURE_2_3;
    *d_k = 2;
    break;
  case BPSK_3_4:
  case QPSK_3_4:
  case QAM16_3_4:
  case QAM64_3_4:
    *d_ntraceback = 10;
    d_depuncture_pattern = PUNCTURE_3_4;
    *d_k = 3;
    break;
  }
}


/* This is the main "decode" function; it prepares data and repeatedly
 * calls the viterbi butterfly2 routine to do steps of decoding.
 */
// INPUTS/OUTPUTS:  
//    ofdm   : INPUT        : Struct (see utils.h) [enum, char, int, int, int]
//    frame  : INPUT/OUTPUT : Struct (see utils.h) [int, int, int, int]
//    in     : INPUT/OUTPUT : Array [ MAX_ENCODED_BITS == 24780 ]

void viterbi_decode(ofdm_param *ofdm,   size_t ofdm_size,
		    frame_param *frame, size_t frame_size,
		    uint8_t *in,        size_t in_size,
		    uint8_t* l_decoded, size_t decd_size)
{
  // Local memories
  unsigned char d_metric0_generic[64] __attribute__ ((aligned(16)));
  unsigned char d_metric1_generic[64] __attribute__ ((aligned(16)));
  unsigned char d_path0_generic[64] __attribute__ ((aligned(16)));
  unsigned char d_path1_generic[64] __attribute__ ((aligned(16)));
  // These are initialized to zero once...
  for (int i = 0; i < 64; i++) {
    d_metric0_generic[i] = 0;
    d_metric1_generic[i] = 0;
    d_path0_generic[i] = 0;
    d_path1_generic[i] = 0;
  }

  // Metrics for each state
  unsigned char d_mmresult[64] __attribute__((aligned(16)));
  // Paths for each state
  unsigned char d_ppresult[TRACEBACK_MAX][64] __attribute__((aligned(16)));

  d_branchtab27_t d_branchtab27_generic[2];
  
  int d_store_pos = 0;
  int d_ntraceback;
  int d_k;
  unsigned char *d_depuncture_pattern;

  viterbi_reset( ofdm,                  ofdm_size,
		 d_metric0_generic,     64,
		 d_metric1_generic,     64,
		 d_path0_generic,       64,
		 d_path1_generic,       64,
		 d_mmresult,            64,
		 d_ppresult,            TRACEBACK_MAX * 64,
		 d_branchtab27_generic, 2*sizeof(d_branchtab27_t),
		 &d_ntraceback,         sizeof(int),
		 &d_k,                  sizeof(int),
		 d_depuncture_pattern,  6);

  uint8_t *depunctured = depuncture(ofdm,      ofdm_size,
				    frame,     frame_size,
				    d_ntraceback, d_k,
				    d_depuncture_pattern,  6,
				    in,        in_size);
	
  int in_count = 0;
  int out_count = 0;
  int n_decoded = 0;

  //printf("uint8_t DECODER_VERIF_DATA[7000][4][64] = {\n");
  //printf("void set_viterbi_decode_verif_data() {\n");

  int viterbi_butterfly_calls = 0;
  while (n_decoded < frame->n_data_bits) {
    //printf("n_decoded = %d vs %d = frame->n_data_bits\n", n_decoded, frame->n_data_bits);
    if ((in_count % 4) == 0) { //0 or 3
      //printf(" Viterbi_Butterfly Call,%d,n_decoded,%d,n_data_bits,%d,in_count,%d,%d\n", viterbi_butterfly_calls, n_decoded, frame->n_data_bits, in_count, (in_count & 0xfffffffc));
      unsigned char *d_brtab27[2] = {&(d_branchtab27_generic[0].c[0]), &(d_branchtab27_generic[1].c[0])};
      viterbi_butterfly2_generic(&depunctured[in_count & 0xfffffffc],   4,
				 d_brtab27,                            64,
				 d_metric0_generic,                    64,
				 d_metric1_generic,                    64,
				 d_path0_generic,                      64,
				 d_path1_generic,                      64 );
      viterbi_butterfly_calls++; // Do not increment until after the comparison code.

      if ((in_count > 0) && (in_count % 16) == 8) { // 8 or 11
	unsigned char c;
	viterbi_get_output_generic(d_metric0_generic,     64,
				   d_path0_generic,       64,
				   d_ntraceback,
				   &d_store_pos,          sizeof(int),
				   d_mmresult,            64,
				   d_ppresult,            TRACEBACK_MAX * 64,
				   &c,                    1);
	//std::cout << "OUTPUT: " << (unsigned int)c << std::endl; 
	if (out_count >= d_ntraceback) {
	  for (int i= 0; i < 8; i++) {
	    l_decoded[(out_count - d_ntraceback) * 8 + i] = (c >> (7 - i)) & 0x1;
	    //printf("l_decoded[ %u ] written\n", (out_count - d_ntraceback) * 8 + i);
	    n_decoded++;
	  }
	}
	out_count++;
      }
    }
    in_count++;
  }
  //printf("};\n");
  //return l_decoded;
}




void viterbi_descrambler(uint8_t* in,   size_t in_size,
			 int psdusize,
			 char* out_msg, size_t out_msg_size)
{
  uint32_t output_length = (psdusize)+2; //output is 2 more bytes than psdu_size
  uint32_t msg_length = (psdusize)-28;
  uint8_t out[output_length];
  int state = 0; //start
  // find the initial state of LFSR (linear feedback shift register: 7 bits) from first 7 input bits
  for(int i = 0; i < 7; i++) {
    if(*(in+i)) {
      state |= 1 << (6 - i);
    }
  }
  //init o/p array to zeros
  for (int i=0; i<output_length; i++ ) {
    out[i] = 0;
  }

  out[0] = state; //initial value
  int feedback;
  int bit;
  int index = 0;
  int mod = 0;
  for(int i = 7; i < (psdusize*8)+16; i++) { // 2 bytes more than psdu_size -> convert to bits
    feedback = ((!!(state & 64))) ^ (!!(state & 8));
    bit = feedback ^ (*(in+i) & 0x1);
    index = i/8;
    mod =  i%8;
    int comp1, comp2, val, comp3;
    comp1 = (bit << mod);
    val = out[index];
    comp2 = val | comp1;
    out[index] =  comp2;
    comp3 = out[index];
    state = ((state << 1) & 0x7e) | feedback;
  }
  
  for (int i = 0; i< msg_length; i++) {
    out_msg[i] = out[i+26];
  }
  out_msg[msg_length] = '\0';
}


void viterbi_analyze_msg_text(char* msg_txt,       size_t msg_txt_size,
			      message_t* out_message,  size_t out_message_size)
{
  // Check contents of "msg_txt" to determine which message_t;
  switch(msg_txt[3]) {
  case '0' : *out_message = safe_to_move_right_or_left; break;
  case '1' : *out_message = safe_to_move_right_only; break;
  case '2' : *out_message = safe_to_move_left_only; break;
  case '3' : *out_message = unsafe_to_move_left_or_right; break;
  default  : *out_message = num_messages; break;
  }
}



void viterbi_decode_to_message_t(ofdm_param *ofdm_ptr,    size_t ofdm_size,
				 frame_param *frame_ptr,  size_t frame_size,
				 uint8_t *input_bits,     size_t in_bits_size,
				 uint8_t* l_decoded,      size_t decd_size,
				 char* out_msg_txt,       size_t out_msg_txt_size,
				 message_t* out_message,  size_t out_message_size)
{
  /* First we do the base viterbi decode ; resulting decoded bits are put into l_decoded */
  viterbi_decode(ofdm_ptr,   sizeof(ofdm_param),
		 frame_ptr,  sizeof(frame_param),
		 input_bits, MAX_ENCODED_BITS,
		 l_decoded,  MAX_ENCODED_BITS * 3 / 4);
  
  /* Now descramble the output from the decoder to get to plain-text message */
  int psdusize = frame_ptr->psdu_size;
  DEBUG(printf("  Calling the viterbi_descrambler routine\n"));
  viterbi_descrambler(l_decoded,   MAX_ENCODED_BITS * 3 / 4, 
		      psdusize,    
		      out_msg_txt, 1600);

  /* This analyzes the message text and sets the out_message message_t indicator */
  viterbi_analyze_msg_text(out_msg_txt, out_msg_txt_size,
			   out_message,  out_message_size);
  
}


void execute_vit_kernel(/*  0 */ ofdm_param* ofdm_ptr,    size_t ofdm_parm_size,   /*  1 */
			/*  2 */ frame_param* frame_ptr,  size_t frame_parm_size,  /*  3 */
			/*  4 */ uint8_t* input_bits,     size_t input_bits_size,  /*  5 */
			/*  6 */ char* out_msg_txt,       size_t out_msg_txt_size, /*  7 */
			/*  8 */ message_t* out_message,  size_t out_message_size, /*  9 */
			/* 10 */ int num_msgs_to_decode)
{
  __visc__hint(DEVICE);
  __visc__attributes(5, ofdm_ptr, frame_ptr, input_bits, out_msg_txt, out_message, 
		     2, out_message, out_msg_txt);
  
  uint8_t l_decoded[MAX_ENCODED_BITS * 3 / 4]; // Intermediate value
  // We will decode num_msgs_to_decode of the same message (for now) and return the last one.
  DEBUG(printf("Decoding %u messages\n", num_msgs_to_decode));
  for (int mi = 0; mi < num_msgs_to_decode; mi++) {
    viterbi_decode_to_message_t(ofdm_ptr,    sizeof(ofdm_param),
				frame_ptr,   sizeof(frame_param),
				input_bits,  MAX_ENCODED_BITS,
				l_decoded,   MAX_ENCODED_BITS * 3 / 4,
				out_msg_txt, 1600,
				out_message, sizeof(message_t));
  }
  
  // Return the SIZE -- the pointer is transferred by a __visc__bind
  __visc__return(2, out_message_size, out_msg_txt_size);
}


void post_execute_vit_kernel(message_t tr_msg, message_t dec_msg)
{
  total_msgs++;
  if (dec_msg != tr_msg) {
    bad_decode_msgs++;
  }
}


/*********************************************************************************
 * The following is code to support the PLAN-AND-CONTROL kernel
 *********************************************************************************/
void plan_and_control(/* 0 */ label_t* label,                 size_t size_label,    /* 1 */ 
		      /* 2 */ distance_t* distance,           size_t size_distance, /* 3 */ 
		      /* 4 */ message_t* message,             size_t size_message,  /* 5 */ 
		      /* 6 */ vehicle_state_t* vehicle_state, size_t size_vehicle_state /* 7 */ )
{
  __visc__hint(CPU_TARGET);
  __visc__attributes(4, label, distance, message, vehicle_state,
		     1, vehicle_state);

  DEBUG(printf("In the plan_and_control routine : label %u %s distance %.1f (T1 %.1f T1 %.1f T3 %.1f) message %u\n", 
	       *label, object_names[*label], *distance, THRESHOLD_1, THRESHOLD_2, THRESHOLD_3, *message));
  vehicle_state_t new_vehicle_state = *vehicle_state;
  
  if ((*label != no_label) && (*distance <= THRESHOLD_1)) {
    switch (*message) {
      case safe_to_move_right_or_left   :
	/* Bias is move right, UNLESS we are in the Right lane and would then head into the RHazard Lane */
	if (vehicle_state->lane < right) { 
	  DEBUG(printf("   In %s with Safe_L_or_R : Moving Right\n", lane_names[vehicle_state->lane]));
	  new_vehicle_state.lane += 1;
	} else {
	  DEBUG(printf("   In %s with Safe_L_or_R : Moving Left\n", lane_names[vehicle_state->lane]));
	  new_vehicle_state.lane -= 1;
	}	  
	break; // prefer right lane
      case safe_to_move_right_only      :
	DEBUG(printf("   In %s with Safe_R_only : Moving Right\n", lane_names[vehicle_state->lane]));
	new_vehicle_state.lane += 1;
	break;
      case safe_to_move_left_only       :
	DEBUG(printf("   In %s with Safe_L_Only : Moving Left\n", lane_names[vehicle_state->lane]));
	new_vehicle_state.lane -= 1;
	break;
      case unsafe_to_move_left_or_right :
	DEBUG(printf("   In %s with No_Safe_Move : STOPPING\n", lane_names[vehicle_state->lane]));
	new_vehicle_state.speed = 0;
	break; /* Stop!!! */
    default:
      printf(" ERROR  In %s with UNDEFINED MESSAGE: %u\n", lane_names[vehicle_state->lane], *message);
      //exit(-6);
    }
  } else {
    // No obstacle-inspired lane change, so try now to occupy the center lane
    switch (vehicle_state->lane) {
    case lhazard:
    case left:
      if ((*message == safe_to_move_right_or_left) ||
	  (*message == safe_to_move_right_only)) {
	DEBUG(printf("  In %s with Can_move_Right: Moving Right\n", lane_names[vehicle_state->lane]));
	new_vehicle_state.lane += 1;
      }
      break;
    case center:
      // No need to alter, already in the center
      break;
    case right:
    case rhazard:
      if ((*message == safe_to_move_right_or_left) ||
	  (*message == safe_to_move_left_only)) {
	DEBUG(printf("  In %s with Can_move_Left : Moving Left\n", lane_names[vehicle_state->lane]));
	new_vehicle_state.lane -= 1;
      }
      break;
    }
  } // else clause


 /** For now we'll ignore other thresholds, etc.
  // Slow down!
  new_vehicle_state.speed -= 10;
  if (new_vehicle_state.speed < 0)
  {
  new_vehicle_state.speed = 0;
  }
  }
  else if ((*label == no_label) && (*distance > THRESHOLD_3))
  {
  // Maintain speed 
  }
  **/
  
  *vehicle_state = new_vehicle_state;

  __visc__return(1, size_vehicle_state);
  //return new_vehicle_state;
}


/*********************************************************************************
 * The following is code that runs to close-out the full simulation run
 *********************************************************************************/
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



/********************************************************************/
/*** HPVM Internal node Functions - Determine the graph structure ***/
/********************************************************************/

// We create a wrapper node per leaf node - this is an implementation
// requirement for the FPGA backend . The CPU backend also supports this,
// so it does not cause a portability issue.

void execute_cv_wrapper(/* 0 */ label_t* in_tr_val, size_t in_tr_val_size, /* 1 */
			/* 2 */ label_t* out_label, size_t out_label_size  /* 3 */)
{
  __visc__hint(CPU_TARGET);
  __visc__attributes(2, in_tr_val, out_label, 1, out_label);

  // Create a 0-D (specified by 1st argument) HPVM node -- a single node 
  // associated with node function execute_cv_kernel
  //void* EXEC_CV_node = __visc__createNodeND(1, execute_cv_kernel, (size_t)1); // 1-D with 1 instance
  void* EXEC_CV_node = __visc__createNodeND(0, execute_cv_kernel);

  // Binds inputs of current node with specified node
  // - destination node
  // - argument position in argument list of function of source node
  // - argument position in argument list of function of destination node
  // - streaming (1) or non-streaming (0)
  __visc__bindIn(EXEC_CV_node,  0,  0, 0); // in_tr_val -> EXEC_CV_node:in_tr_val
  __visc__bindIn(EXEC_CV_node,  1,  1, 0); // in_tr_val_size -> EXEC_CV_node:in_tr_val_size
  __visc__bindIn(EXEC_CV_node,  2,  2, 0); // out_label -> EXEC_CV_node:out_label
  __visc__bindIn(EXEC_CV_node,  3,  3, 0); // out_label_size -> EXEC_CV_node:out_label_size

  // Similar to bindIn, but for the output. Output of a node is a struct, and
  // we consider the fields in increasing ordering.
  __visc__bindOut(EXEC_CV_node, 0, 0, 0);
}


void execute_rad_wrapper(float * inputs, size_t input_size_bytes,
			unsigned int input_N, unsigned int in_logn, int in_sign,
			distance_t * distance, size_t dist_size)
{
  __visc__hint(CPU_TARGET);
  __visc__attributes(2, inputs, distance, 1, distance);

  // Create a 0-D (specified by 1st argument) HPVM node -- a single node 
  // associated with node function execute_rad_kernel
  //void* EXEC_RAD_node = __visc__createNodeND(1, execute_rad_kernel, (size_t)1); // 1-D with 1 instance
  void* EXEC_RAD_node = __visc__createNodeND(0, execute_rad_kernel);

  // Binds inputs of current node with specified node
  // - destination node
  // - argument position in argument list of function of source node
  // - argument position in argument list of function of destination node
  // - streaming (1) or non-streaming (0)
  __visc__bindIn(EXEC_RAD_node,  0,  0, 0); // inputs -> EXEC_RAD_node:inputs
  __visc__bindIn(EXEC_RAD_node,  1,  1, 0); // input_size_bytes -> EXEC_RAD_node:input_size_bytes
  __visc__bindIn(EXEC_RAD_node,  2,  2, 0); // input_N -> EXEC_RAD_node:input_N
  __visc__bindIn(EXEC_RAD_node,  3,  3, 0); // in_logn -> EXEC_RAD_node:in_logn
  __visc__bindIn(EXEC_RAD_node,  4,  4, 0); // in_sign -> EXEC_RAD_node:in_sign
  __visc__bindIn(EXEC_RAD_node,  5,  5, 0); // distance -> EXEC_RAD_node:distance
  __visc__bindIn(EXEC_RAD_node,  6,  6, 0); // dist_size -> EXEC_RAD_node:dist_size

  // Similar to bindIn, but for the output. Output of a node is a struct, and
  // we consider the fields in increasing ordering.
  __visc__bindOut(EXEC_RAD_node, 0, 0, 0);
}

void execute_vit_wrapper(/*  0 */ ofdm_param* ofdm_ptr,    size_t ofdm_parm_size,   /*  1 */
			 /*  2 */ frame_param* frame_ptr,  size_t frame_parm_size,  /*  3 */
			 /*  4 */ uint8_t* input_bits,     size_t input_bits_size,  /*  5 */
			 /*  6 */ char* out_msg_txt,       size_t out_msg_txt_size, /*  7 */
			 /*  8 */ message_t* out_message,  size_t out_message_size, /*  9 */
			 /* 10 */ int num_msgs_to_decode)
{
  __visc__hint(CPU_TARGET);
  __visc__attributes(5, ofdm_ptr, frame_ptr, input_bits, out_msg_txt, out_message, 
		     //2, out_msg_txt, out_message);
		     1, out_message);
  
  // Create a 0-D (specified by 1st argument) HPVM node -- a single node 
  // associated with node function execute_rad_kernel
  void* EXEC_VIT_node = __visc__createNodeND(0, execute_vit_kernel);

  // Binds inputs of current node with specified node
  // - destination node
  // - argument position in argument list of function of source node
  // - argument position in argument list of function of destination node
  // - streaming (1) or non-streaming (0)
  __visc__bindIn(EXEC_VIT_node,  0,  0, 0); // ofdm_ptr -> EXEC_VIT_node::ofdm_ptr
  __visc__bindIn(EXEC_VIT_node,  1,  1, 0); // ofdm_parm_size -> EXEC_VIT_node::ofdm_parm_size
  __visc__bindIn(EXEC_VIT_node,  2,  2, 0); // frame_ptr -> EXEC_VIT_node::frame_ptr
  __visc__bindIn(EXEC_VIT_node,  3,  3, 0); // frame_parm_size  -> EXEC_VIT_node::frame_parm_size
  __visc__bindIn(EXEC_VIT_node,  4,  4, 0); // input_bits -> EXEC_VIT_node::input_bits
  __visc__bindIn(EXEC_VIT_node,  5,  5, 0); // input_bits_size -> EXEC_VIT_node::input_bits_size
  __visc__bindIn(EXEC_VIT_node,  6,  6, 0); // out_msg_txt -> EXEC_VIT_node::out_msg_txt
  __visc__bindIn(EXEC_VIT_node,  7,  7, 0); // out_msg_txt_size -> EXEC_VIT_node::out_msg_txt_size
  __visc__bindIn(EXEC_VIT_node,  8,  8, 0); // out_message -> EXEC_VIT_node::out_message
  __visc__bindIn(EXEC_VIT_node,  9,  9, 0); // out_message_size -> EXEC_VIT_node::out_message_size
  __visc__bindIn(EXEC_VIT_node, 10, 10, 0); // num_msgs_to_decode -> EXEC_VIT_node::num_msgs_to_decode

  // Similar to bindIn, but for the output. Output of a node is a struct, and
  // we consider the fields in increasing ordering.
  __visc__bindOut(EXEC_VIT_node, 0, 0, 0);
}




// Type definition for struct used to pass arguments to the HPVM dataflow graph
// using the hpvm launch operation

typedef struct __attribute__((__packed__)) {
  float * radar_data;       size_t bytes_radar_data;          /*  0,  1 */
  unsigned int radar_N;				              /*  2 */
  unsigned int radar_logn;			              /*  3 */
  int radar_sign;				              /*  4 */
  float * radar_distance;   size_t bytes_radar_distance;      /*  5,  6 */
  
  ofdm_param* ofdm_ptr;     size_t bytes_ofdm_parm;           /*  7,  8 */
  frame_param* frame_ptr;   size_t bytes_frame_parm;          /*  9, 10 */
  uint8_t* vit_in_bits;     size_t bytes_vit_in_bits;	      /* 11, 12 */
  char* vit_out_msg_txt;    size_t bytes_vit_out_msg_txt;     /* 13, 14 */
  message_t* vit_out_msg;   size_t bytes_vit_out_msg;	      /* 15, 16 */
  int   num_msgs_to_decode;				      /* 17 */

  label_t* label;                 size_t bytes_label;         /* 18, 19 */
  vehicle_state_t* vehicle_state; size_t bytes_vehicle_state; /* 20, 21 */

} RootIn;


/*** ROOT Node - Top Level of the Graph Hierarchy ***/
void miniERARoot(/*  0 */ float * radar_data, size_t bytes_radar_data, /* 1 */    // RADAR
		 /*  2 */ unsigned int radar_N,
		 /*  3 */ unsigned int radar_logn,
		 /*  4 */ int radar_sign,
		 /*  5 */ float * radar_distance,  size_t bytes_radar_distance     /*  6 */, // Viterbi
		 /*  7 */ ofdm_param* ofdm_ptr,    size_t bytes_ofdm_parm,         /*  8 */
		 /*  9 */ frame_param* frame_ptr,  size_t bytes_frame_parm,        /* 10 */
		 /* 11 */ uint8_t* vit_in_bits,    size_t bytes_vit_in_bits,       /* 12 */
		 /* 13 */ char* vit_out_msg_txt,   size_t bytes_vit_out_msg_txt,   /* 14 */
		 /* 15 */ message_t* vit_out_msg,  size_t bytes_vit_out_msg,       /* 16 */
		 /* 17 */ int num_msgs_to_decode,
		 
		 /* 18 */ label_t* label,                 size_t bytes_label,        /* 19 */ // Plan-and-Control
		 /* 20 */ vehicle_state_t* vehicle_state, size_t bytes_vehicle_state /* 21 */
		 )
{
  //Specifies compilation target for current node
  __visc__hint(CPU_TARGET);

  // Specifies pointer arguments that will be used as "in" and "out" arguments
  // - count of "in" arguments
  // - list of "in" argument , and similar for "out"
  __visc__attributes(11, radar_data, radar_N, radar_logn, radar_sign, radar_distance, // - count of "in" arguments, list of "in" arguments
		         ofdm_ptr, frame_ptr, vit_in_bits, vit_out_msg_txt, vit_out_msg,
		         vehicle_state,
		     1, vehicle_state);          // - count of "out" arguments, list of "out" arguments
  //                 3, radar_distance, vit_out_msg_txt, vit_out_msg);          // - count of "out" arguments, list of "out" arguments

  // FFT Node
  //void* EXEC_RAD_node = __visc__createNodeND(0, execute_rad_kernel);
  void* RAD_wrap_node = __visc__createNodeND(0, execute_rad_wrapper);

  // Viterbi Node
  void* VIT_wrap_node = __visc__createNodeND(0, execute_vit_wrapper);

  // CV Nodes
  void* CV_wrap_node = __visc__createNodeND(0, execute_cv_wrapper);

  // Plan and Control Node
  void* PLAN_CTL_node = __visc__createNodeND(0, plan_and_control);

  // BindIn binds inputs of current node with specified node
  // - destination node
  // - argument position in argument list of function of source node
  // - argument position in argument list of function of destination node
  // - streaming (1) or non-streaming (0)

  // Edge transfers data between nodes within the same level of hierarchy.
  // - source and destination dataflow nodes
  // - edge type, all-all (1) or one-one(0)
  // - source position (in output struct of source node, i.e. __visc__return() statement)
  // - destination position (in argument list of destination node function parameters)
  // - streaming (1) or non-streaming (0)

  // scale_fxp inputs
  __visc__bindIn(RAD_wrap_node,  0,  0, 0); // radar_data -> RAD_wrap_node:radar_data
  __visc__bindIn(RAD_wrap_node,  1,  1, 0); // bytes_radar_data -> RAD_wrap_node:bytes_radar_data
  __visc__bindIn(RAD_wrap_node,  2,  2, 0); // radar_N -> RAD_wrap_node:radar_N
  __visc__bindIn(RAD_wrap_node,  3,  3, 0); // radar_logn -> RAD_wrap_node:radar_logn
  __visc__bindIn(RAD_wrap_node,  4,  4, 0); // radar_sign -> RAD_wrap_node:radar_sign
  __visc__bindIn(RAD_wrap_node,  5,  5, 0); // radar_distance -> RAD_wrap_node:radar_distance
  __visc__bindIn(RAD_wrap_node,  6,  6, 0); // bytes_radar_dist -> RAD_wrap_node:bytes_radar_dist

  __visc__bindIn(VIT_wrap_node,  7,  0, 0); // ofdm_ptr -> VIT_wrap_node:ofdm_ptr
  __visc__bindIn(VIT_wrap_node,  8,  1, 0); // bytes_ofdm_parm -> VIT_wrap_node:bytes_ofdm_parm
  __visc__bindIn(VIT_wrap_node,  9,  2, 0); // frame_ptr -> VIT_wrap_node:frame_ptr
  __visc__bindIn(VIT_wrap_node, 10,  3, 0); // bytes_frame_parm -> VIT_wrap_node:bytes_frame_parm
  __visc__bindIn(VIT_wrap_node, 11,  4, 0); // vit_in_bits -> VIT_wrap_node:vit_in_bits
  __visc__bindIn(VIT_wrap_node, 12,  5, 0); // bytes_vit_in_bits -> VIT_wrap_node:bytes_vit_in_bits
  __visc__bindIn(VIT_wrap_node, 13,  6, 0); // vit_out_msg_txt -> VIT_wrap_node:vit_out_msg_txt
  __visc__bindIn(VIT_wrap_node, 14,  7, 0); // bytes_vit_out_msg_txt -> VIT_wrap_node:bytes_vit_out_msg_txt
  __visc__bindIn(VIT_wrap_node, 15,  8, 0); // vit_out_msg -> VIT_wrap_node:vit_out_msg
  __visc__bindIn(VIT_wrap_node, 16,  9, 0); // bytes_vit_out_msg -> VIT_wrap_node:bytes_vit_out_msg
  __visc__bindIn(VIT_wrap_node, 17, 10, 0); // num_msgs_to_decode -> VIT_wrap_node:num_msgs_to_decode

  __visc__bindIn(CV_wrap_node, 18, 0, 0); // label -> CV_wrap_node:in_tr_val
  __visc__bindIn(CV_wrap_node, 19, 1, 0); // label_size -> CV_wrap_node:in_tr_val_size
  __visc__bindIn(CV_wrap_node, 18, 2, 0); // label -> CV_wrap_node:out_label
  __visc__bindIn(CV_wrap_node, 19, 3, 0); // label_size -> CV_wrap_node:out_label_size

  // Use bindIn for the label (pointer) and edge for the size_t (which also creates node->node flow dependence)
  __visc__bindIn(PLAN_CTL_node, 18,  0, 0); // out_label -> PLAN_CTL_node::label
  __visc__edge(CV_wrap_node, PLAN_CTL_node, 1, 0, 1, 0); // RAD_wrap_node::out_label_size output -> PLAN_CTL_node::size_label

  // Use bindIn for the distance (pointer) and edge for the size_t (which also creates node->node flow dependence)
  __visc__bindIn(PLAN_CTL_node,  5,  2, 0); // radar_distance -> PLAN_CTL_node:radar_distance
  __visc__edge(RAD_wrap_node, PLAN_CTL_node, 1, 0, 3, 0); // RAD_wrap_node::dist_size ouput -> PLAN_CTL_node::distance input

  // Use bindIn for the vit_out_msg (pointer) and edge for the size_t (which also creates node->node flow dependence)
  __visc__bindIn(PLAN_CTL_node, 15,  4, 0); // vit_out_msg -> PLAN_CTL_node::message input  
  __visc__edge(VIT_wrap_node, PLAN_CTL_node, 1, 0, 5, 0); // VIT_wrap_node::out_message -> PLAN_CTL_node::message input

  __visc__bindIn(PLAN_CTL_node, 20,  6, 0); // vehicle_state -> PLAN_CTL_node::vehicle_state
  __visc__bindIn(PLAN_CTL_node, 21,  7, 0); // bytes_vehicle_state -> PLAN_CTL_node::size_vehicle_state
  //__visc__edge(PLAN_CTL_node, PLAN_CTL_node, 1, 0, 7, 0); // PLAN_CTL_node::size_vehicle_stat -> PLAN_CTL_node::size_vehicle_state
  
  // Similar to bindIn, but for the output. Output of a node is a struct, and
  // we consider the fields in increasing ordering.
  __visc__bindOut(PLAN_CTL_node, 0, 0, 0);  // Output is just the new vehicle_state
}


int main(int argc, char *argv[])
{
  vehicle_state_t vehicle_state;

  char* trace_file; 
  int opt; 

  // put ':' in the starting of the 
  // string so that program can  
  // distinguish between '?' and ':'
  while ((opt = getopt(argc, argv, ":hot:v:")) != -1) {  
    switch(opt) {  
    case 'h':
      print_usage(argv[0]);
      exit(0);
    case 'o':
      output_viz_trace = true;
      break;
    case 't':
      trace_file = optarg;
      printf("Using trace file: %s\n", trace_file);
      break;
    case 'v':
      {
	int inval = atoi(optarg);
 	//if ((inval == 0) || (inval == 1)) {
	vit_msgs_behavior = inval;
	//}
	printf("Using viterbi behavior %u\n", vit_msgs_behavior);
      }
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
  
  ofdm_param  xfer_ofdm;
  frame_param xfer_frame;
  uint8_t     xfer_vit_inputs[MAX_ENCODED_BITS];
  char        vit_msg_txt[1600];
  message_t   vit_message;

  llvm_visc_track_mem(&xfer_ofdm, sizeof(ofdm_param));
  llvm_visc_track_mem(&xfer_frame, sizeof(frame_param));
  llvm_visc_track_mem(xfer_vit_inputs, MAX_ENCODED_BITS);

  llvm_visc_track_mem(vit_msg_txt, 1600);
  llvm_visc_track_mem(&vit_message, sizeof(message_t));

  label_t cv_infer_label;
  llvm_visc_track_mem(&cv_infer_label, sizeof(label_t));
  llvm_visc_track_mem(&vehicle_state, sizeof(vehicle_state_t));
  
  int num_vit_msgs = 1;   // the number of messages to send this time step (1 is default)

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
    num_vit_msgs = 1;   // the number of messages to send this time step (1 is default) 
    switch(vit_msgs_behavior) {
    case 2: num_vit_msgs = total_obj; break;
    case 3: num_vit_msgs = total_obj; break;
    case 4: num_vit_msgs = total_obj + 1; break;
    case 5: num_vit_msgs = total_obj + 1; break;
    }


    // EXECUTE the kernels using the now known inputs 
    cv_infer_label = cv_tr_label; // Set default output to match the input (in case we bypass CNN call)

    // Set up HPVM DFG inputs in the rootArgs struct.
    rootArgs->radar_data       = radar_input;
    rootArgs->bytes_radar_data = 8*RADAR_N;
  
    rootArgs->radar_N = RADAR_N;
    rootArgs->radar_logn = RADAR_LOGN;
    rootArgs->radar_sign = -1;

    rootArgs->radar_distance       = &radar_distance;
    rootArgs->bytes_radar_distance = sizeof(float);

    /* Viterbi part 0 copy over required inputs */
    xfer_ofdm.encoding   = vdentry_p->ofdm_p.encoding;
    xfer_ofdm.rate_field = vdentry_p->ofdm_p.rate_field;
    xfer_ofdm.n_bpsc     = vdentry_p->ofdm_p.n_bpsc;
    xfer_ofdm.n_cbps     = vdentry_p->ofdm_p.n_cbps;
    xfer_ofdm.n_dbps     = vdentry_p->ofdm_p.n_dbps;

    xfer_frame.psdu_size      = vdentry_p->frame_p.psdu_size;
    xfer_frame.n_sym          = vdentry_p->frame_p.n_sym;
    xfer_frame.n_encoded_bits = vdentry_p->frame_p.n_encoded_bits;
    xfer_frame.n_data_bits    = vdentry_p->frame_p.n_data_bits;

    for (int bi = 0; bi <  MAX_ENCODED_BITS; bi++) {
      xfer_vit_inputs[bi] = vdentry_p->in_bits[bi];
    }

    rootArgs->ofdm_ptr           = &xfer_ofdm;
    rootArgs->bytes_ofdm_parm    = sizeof(ofdm_param);
    rootArgs->frame_ptr          = &xfer_frame;
    rootArgs->bytes_frame_parm   = sizeof(frame_param);
    rootArgs->vit_in_bits        = xfer_vit_inputs;
    rootArgs->bytes_vit_in_bits  = MAX_ENCODED_BITS;
    rootArgs->vit_out_msg_txt       = vit_msg_txt;
    rootArgs->bytes_vit_out_msg_txt = 1600;
    rootArgs->vit_out_msg        = &vit_message;
    rootArgs->bytes_vit_out_msg  = sizeof(message_t);
    rootArgs->num_msgs_to_decode = num_vit_msgs;
    
    
    rootArgs->label               = &cv_infer_label;
    rootArgs->bytes_label         = sizeof(label_t);
    rootArgs->vehicle_state       = &vehicle_state;
    rootArgs->bytes_vehicle_state = sizeof(vehicle_state_t);

    // Launch the DFG to do:
    //            the CV CNN analysis,
    //        AND the radar computation
    //        AND the Viterbi computation...
    //        AND the Plan-and-Control function

    void* doExecPlanControlDFG = __visc__launch(0, miniERARoot, (void*) rootArgs);
    __visc__wait(doExecPlanControlDFG);

    // Request data from graph.
    llvm_visc_request_mem(&radar_distance, sizeof(float));
    //llvm_visc_request_mem(&vit_msg_txt, 1600); 
    llvm_visc_request_mem(&vit_message, sizeof(message_t));
    llvm_visc_request_mem(&vehicle_state, sizeof(vehicle_state_t));
    
    DEBUG(printf("New vehicle state: lane %u speed %.1f\n\n", vehicle_state.lane, vehicle_state.speed));

    // POST-EXECUTE each kernels to gather stats, etc.
    post_execute_cv_kernel(cv_tr_label, cv_infer_label);
    post_execute_rad_kernel(rd_dist, radar_distance);
    for (int mi = 0; mi < num_vit_msgs; mi++) {
      post_execute_vit_kernel(vdentry_p->msg_id, vit_message);
    }
    

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

  llvm_visc_untrack_mem(&xfer_ofdm);
  llvm_visc_untrack_mem(&xfer_frame);
  llvm_visc_untrack_mem(xfer_vit_inputs);

  llvm_visc_untrack_mem(vit_msg_txt);
  llvm_visc_untrack_mem(&vit_message);

  llvm_visc_track_mem(&cv_infer_label, sizeof(label_t));
  llvm_visc_untrack_mem(&vehicle_state);

  __visc__cleanup();

  printf("\nDone.\n");
  return 0;
}
