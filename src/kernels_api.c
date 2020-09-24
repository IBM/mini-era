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

#ifndef BYPASS_KERAS_CV_CODE
#include <Python.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include <sys/time.h>

#include "kernels_api.h"

#ifdef USE_SIM_ENVIRON
 #include "sim_environs.h"
#else
 #include "read_trace.h"
#endif

extern unsigned time_step;

char* lane_names[NUM_LANES] = {"LHazard", "Left", "Center", "Right", "RHazard" };
char* message_names[NUM_MESSAGES] = {"Safe_L_or_R", "Safe_R_only", "Safe_L_only", "Unsafe_L_or_R" };
char* object_names[NUM_OBJECTS] = {"Nothing", "Car", "Truck", "Person", "Bike" };


#ifdef VERBOSE_MODE
bool_t output_viz_trace = true;
#else
bool_t output_viz_trace = false;
#endif
unsigned fft_logn_samples = 14; // Defaults to 16k samples

unsigned total_obj; // Total non-'N' obstacle objects across all lanes this time step
unsigned obj_in_lane[NUM_LANES]; // Number of obstacle objects in each lane this time step (at least one, 'n')
unsigned lane_dist[NUM_LANES][MAX_OBJ_IN_LANE]; // The distance to each obstacle object in each lane
char     lane_obj[NUM_LANES][MAX_OBJ_IN_LANE]; // The type of each obstacle object in each lane

char     nearest_obj[NUM_LANES]  = { 'N', 'N', 'N', 'N', 'N' };
float    nearest_dist[NUM_LANES] = { INF_DISTANCE, INF_DISTANCE, INF_DISTANCE, INF_DISTANCE, INF_DISTANCE };

unsigned hist_total_objs[NUM_LANES * MAX_OBJ_IN_LANE];

unsigned rand_seed = 0; // Only used if -r <N> option set

float IMPACT_DISTANCE = 50.0; // Minimum distance at which an obstacle "impacts" MyCar (collision case)


#ifndef BYPASS_KERAS_CV_CODE
PyObject *pName, *pModule, *pFunc, *pFunc_load;
PyObject *pArgs, *pValue, *pretValue;
#define PY_SSIZE_T_CLEAN

char *python_module = "mio";
char *python_func = "predict";	  
char *python_func_load = "loadmodel";	  
#endif


/* These are some top-level defines needed for CV kernel */
/* #define IMAGE_SIZE  32  // What size are these? */
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
/* typedef struct { */
/*   unsigned int index; */
/*   unsigned int return_id; */
/*   float distance; */
/*   float return_data[2 * RADAR_N]; */
/* } radar_dict_entry_t; */

#define MAX_RDICT_ENTRIES      12   // This should be updated eventually...
unsigned int         crit_fft_samples_set = 0; // The FFT set used for radara returns.
unsigned int         num_radar_samples_sets = 0;
unsigned int         radar_dict_items_per_set = 0;
radar_dict_entry_t** the_radar_return_dict;
unsigned int         radar_log_nsamples_per_dict_set[MAX_RDICT_SAMPLE_SETS];

unsigned radar_total_calc = 0;
unsigned hist_pct_errs[MAX_RDICT_SAMPLE_SETS][MAX_RDICT_ENTRIES][5];// = {0, 0, 0, 0, 0}; // One per distance, plus global?
unsigned hist_distances[MAX_RDICT_SAMPLE_SETS][MAX_RDICT_ENTRIES];
char*    hist_pct_err_label[5] = {"   0%", "<  1%", "< 10%", "<100%", ">100%"};
unsigned radar_inputs_histogram[MAX_RDICT_SAMPLE_SETS][MAX_RDICT_ENTRIES];

// This is the declarations, etc. for H264 kernels.
extern void  init_h264_decode(int argc, char **argv);
extern char* do_h264_decode();
extern void  do_post_h264_decode();
extern void  do_closeout_h264_decode();

int do_h264_argc = 3;
char * do_h264_argv[3] = {"do_h264_decode", "data/test.264", "data/test_dec.yuv"};

// This is the support variables, declarations, etc. for LZ4 code
unsigned lz4_msg_compares = 0;
unsigned lz4_mlen_miscompare = 0;
unsigned lz4_mtext_miscompare = 0;

#define LZ4_MLEN_HIST_BUCKETS  25
#define LZ4_MLEN_HIST_SCALE    (MAX_LZ4_MSG_SIZE/LZ4_MLEN_HIST_BUCKETS)
unsigned lz4_len_histogram[LZ4_MLEN_HIST_BUCKETS];

#ifdef INT_TIME
struct timeval l_compr_stop, l_compr_start;
uint64_t l_compr_sec  = 0LL;
uint64_t l_compr_usec = 0LL;

struct timeval l_decmp_stop, l_decmp_start;
uint64_t l_decmp_sec  = 0LL;
uint64_t l_decmp_usec = 0LL;

struct timeval l_iterate_stop, l_iterate_start;
uint64_t l_iterate_sec  = 0LL;
uint64_t l_iterate_usec = 0LL;

#endif

status_t init_rad_kernel(char* dict_fn)
{
  DEBUG(printf("In init_rad_kernel...\n"));

  init_calculate_peak_dist(fft_logn_samples);

  // Read in the radar distances dictionary file
  FILE *dictF = fopen(dict_fn,"r");
  if (!dictF)
  {
    printf("Error: unable to open dictionary file %s\n", dict_fn);
    fclose(dictF);
    return error;
  }
  // Read the number of definitions
  if (fscanf(dictF, "%u %u\n", &num_radar_samples_sets, &radar_dict_items_per_set) != 2) {
    printf("ERROR reading the number of Radar Dictionary sets and items per set\n");
    exit(-2);
  }
  DEBUG(printf("  There are %u dictionary sets of %u entries each\n", num_radar_samples_sets, radar_dict_items_per_set));
  the_radar_return_dict = (radar_dict_entry_t**)calloc(num_radar_samples_sets, sizeof(radar_dict_entry_t*));
  if (the_radar_return_dict == NULL) {
    printf("ERROR : Cannot allocate Radar Trace Dictionary memory space\n");
    fclose(dictF);
    return error;
  }
  for (int si = 0; si < num_radar_samples_sets; si++) {
    the_radar_return_dict[si] = (radar_dict_entry_t*)calloc(radar_dict_items_per_set, sizeof(radar_dict_entry_t));
    if (the_radar_return_dict[si] == NULL) {
      printf("ERROR : Cannot allocate Radar Trace Dictionary memory space for set %u\n", si);
    fclose(dictF);
      return error;
    }
  }
  unsigned tot_dict_values = 0;
  unsigned tot_index = 0;
  for (int si = 0; si < num_radar_samples_sets; si++) {
    if (fscanf(dictF, "%u\n", &(radar_log_nsamples_per_dict_set[si])) != 1) {
      printf("ERROR reading the number of Radar Dictionary samples for set %u\n", si);
      exit(-2);
    }
    DEBUG(printf("  Dictionary set %u entries should all have %u log_nsamples\n", si, radar_log_nsamples_per_dict_set[si]));
    for (int di = 0; di < radar_dict_items_per_set; di++) {
      unsigned entry_id;
      unsigned entry_log_nsamples;
      float entry_dist;
      unsigned entry_dict_values = 0;
      if (fscanf(dictF, "%u %u %f", &entry_id, &entry_log_nsamples, &entry_dist) != 3) {
	printf("ERROR reading Radar Dictionary set %u entry %u header\n", si, di);
	exit(-2);
      }
      if (radar_log_nsamples_per_dict_set[si] != entry_log_nsamples) {
	printf("ERROR reading Radar Dictionary set %u entry %u header : Mismatch in log2 samples : %u vs %u\n", si, di, entry_log_nsamples, radar_log_nsamples_per_dict_set[si]);
	exit(-2);
      }
	
      DEBUG(printf("  Reading rad dictionary set %u entry %u : %u %u %f\n", si, di, entry_id, entry_log_nsamples, entry_dist));
      the_radar_return_dict[si][di].index = tot_index++;  // Set, and increment total index
      the_radar_return_dict[si][di].set = si;
      the_radar_return_dict[si][di].index_in_set = di;
      the_radar_return_dict[si][di].return_id = entry_id;
      the_radar_return_dict[si][di].log_nsamples = entry_log_nsamples;
      the_radar_return_dict[si][di].distance =  entry_dist;
      for (int i = 0; i < 2*(1<<entry_log_nsamples); i++) {
	float fin;
	if (fscanf(dictF, "%f", &fin) != 1) {
	  printf("ERROR reading Radar Dictionary set %u entry %u data entries\n", si, di);
	  exit(-2);
	}
	the_radar_return_dict[si][di].return_data[i] = fin;
	tot_dict_values++;
	entry_dict_values++;
      }
      DEBUG(printf("    Read in dict set %u entry %u with %u total values\n", si, di, entry_dict_values));
    } // for (int di across radar dictionary entries per set
    DEBUG(printf("   Done reading in Radar dictionary set %u\n", si));
  } // for (si across radar dictionary sets)
  DEBUG(printf("  Read %u sets with %u entries totalling %u values across them all\n", num_radar_samples_sets, radar_dict_items_per_set, tot_dict_values));
  if (!feof(dictF)) {
    printf("NOTE: Did not hit eof on the radar dictionary file %s\n", dict_fn);
    while(!feof(dictF)) {
      char c;
      if (fscanf(dictF, "%c", &c) != 1) {
	printf("Couldn't read final character\n");
      } else {
	printf("Next char is %c = %u = 0x%x\n", c, c, c);
      }
    }
    //if (!feof(dictF)) { printf("and still no EOF\n"); } 
  }
  fclose(dictF);

  // Initialize hist_pct_errs values
  for (int si = 0; si < num_radar_samples_sets; si++) {
    for (int di = 0; di < radar_dict_items_per_set; di++) {
      hist_distances[si][di] = 0;
      for (int i = 0; i < 5; i++) {
	hist_pct_errs[si][di][i] = 0;
      }
    }
  }

  //Clear the inputs (injected) histogram
  for (int i = 0; i < MAX_RDICT_SAMPLE_SETS; i++) {
    for (int j = 0; j < MAX_RDICT_ENTRIES; j++) {
      radar_inputs_histogram[i][j] = 0;
    }
  }

  return success;
}



status_t init_h264_kernel(char* dict_fn)
{
  DEBUG(printf("In init_h264_kernel...\n"));

  // Read in the object images dictionary file
  FILE *dictF = fopen(dict_fn,"r");
  if (!dictF)
  {
    printf("Error: unable to open h264 dictionary definition file %s\n", dict_fn);
    return error;
  }
  // Read in the trace message dictionary from the trace file
  // Read the Entry ID
  unsigned dict_entry_id;
  if (fscanf(dictF, "%u\n", &dict_entry_id) != 1) {
    printf("ERROR reading the H264 Dictionary ID\n");
    exit(-2);
  }    
  DEBUG(printf("  The H264dictionary id is %u\n", dict_entry_id));
  /**
  the_h264_trace_dict = (h264_dict_entry_t*)calloc(num_h264_dictionary_items, sizeof(h264_dict_entry_t));
  if (the_h264_trace_dict == NULL) 
  {
    printf("ERROR : Cannot allocate H264 Trace Dictionary memory space\n");
    fclose(dictF);
    return error;
  }

  // Read in each dictionary item
  for (int i = 0; i < num_h264_dictionary_items; i++) 
  {
    DEBUG(printf("  Reading h264 dictionary entry %u\n", i)); //the_h264_trace_dict[i].msg_id));

    int mnum, mid;
    if (fscanf(dictF, "%d %d\n", &mnum, &mid) != 2) {
      printf("Error reading h264 kernel dictionary enry %u header: Message_number and Message_id\n", i);
      exit(-6);
    }
    DEBUG(printf(" V_MSG: num %d Id %d\n", mnum, mid));
    if (mnum != i) {
      printf("ERROR : Check H264 Dictionary : i = %d but Mnum = %d  (Mid = %d)\n", i, mnum, mid);
      exit(-5);
    }
    the_h264_trace_dict[i].msg_num = mnum;
    the_h264_trace_dict[i].msg_id = mid;

    int in_bpsc, in_cbps, in_dbps, in_encoding, in_rate; // OFDM PARMS
    if (fscanf(dictF, "%d %d %d %d %d\n", &in_bpsc, &in_cbps, &in_dbps, &in_encoding, &in_rate) != 5) {
      printf("Error reading h264 kernel dictionary entry %u bpsc, cbps, dbps, encoding and rate info\n", i);
      exit(-2);
    }

    DEBUG(printf("  OFDM: %d %d %d %d %d\n", in_bpsc, in_cbps, in_dbps, in_encoding, in_rate));
    the_h264_trace_dict[i].ofdm_p.encoding   = in_encoding;
    the_h264_trace_dict[i].ofdm_p.n_bpsc     = in_bpsc;
    the_h264_trace_dict[i].ofdm_p.n_cbps     = in_cbps;
    the_h264_trace_dict[i].ofdm_p.n_dbps     = in_dbps;
    the_h264_trace_dict[i].ofdm_p.rate_field = in_rate;

    int in_pdsu_size, in_sym, in_pad, in_encoded_bits, in_data_bits;
    if (fscanf(dictF, "%d %d %d %d %d\n", &in_pdsu_size, &in_sym, &in_pad, &in_encoded_bits, &in_data_bits) != 5) {
      printf("Error reading h264 kernel dictionary entry %u psdu num_sym, pad, n_encoded_bits and n_data_bits\n", i);
      exit(-2);
    }
    DEBUG(printf("  FRAME: %d %d %d %d %d\n", in_pdsu_size, in_sym, in_pad, in_encoded_bits, in_data_bits));
    the_h264_trace_dict[i].frame_p.psdu_size      = in_pdsu_size;
    the_h264_trace_dict[i].frame_p.n_sym          = in_sym;
    the_h264_trace_dict[i].frame_p.n_pad          = in_pad;
    the_h264_trace_dict[i].frame_p.n_encoded_bits = in_encoded_bits;
    the_h264_trace_dict[i].frame_p.n_data_bits    = in_data_bits;

    int num_in_bits = in_encoded_bits + 10; // strlen(str3)+10; //additional 10 values
    DEBUG(printf("  Reading %u in_bits\n", num_in_bits));
    for (int ci = 0; ci < num_in_bits; ci++) { 
      unsigned c;
      if (fscanf(dictF, "%u ", &c) != 1) {
	printf("Error reading h264 kernel dictionary entry %u data\n", i);
	exit(-6);
      }
      #ifdef SUPER_VERBOSE
      printf("%u ", c);
      #endif
      the_h264_trace_dict[i].in_bits[ci] = (uint8_t)c;
    }
    DEBUG(printf("\n"));
  }
  **/
  fclose(dictF);

  // Now initialize the H264 specific workload stuff...
  init_h264_decode(do_h264_argc, do_h264_argv);
  DEBUG(printf("DONE with init_h264_kernel -- returning success\n"));
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
  // Now we do one execution call, to make sure everything is "primed"
  /*label_t label =*/ execute_cv_kernel(0, NULL);
#endif  
  return success;
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
  DEBUG(printf("Entered run_object_classification... tr_val = %u\n", tr_val));
  label_t object = (label_t)tr_val;
#ifndef BYPASS_KERAS_CV_CODE
  if (pModule != NULL) {
    DEBUG(printf("  Starting call to pModule...\n"));
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


label_t iterate_cv_kernel(vehicle_state_t vs)
{
  DEBUG(printf("In iterate_cv_kernel\n"));

  unsigned tr_val = 0; // Default nothing
  switch(nearest_obj[vs.lane]) {
    case 'N' : tr_val = no_label; break;
    case 'B' : tr_val = bicycle; break;
    case 'C' : tr_val = car; break;
    case 'P' : tr_val = pedestrian; break;
    case 'T' : tr_val = truck; break;
    default: printf("ERROR : Unknown object type in cv trace: '%c'\n", nearest_obj[vs.lane]); exit(-2);
  }
  label_t d_object = (label_t)tr_val;

  return d_object;
}


label_t execute_cv_kernel(label_t in_tr_val, char* found_frame_ptr)
{
  DEBUG(printf("In execute_cv_kernel with in_tr_val %u and frame_ptr %p\n", in_tr_val, found_frame_ptr));
  /* 2) Conduct object detection on the image frame */
  DEBUG(printf("  Calling run_object_detection with in_tr_val tr_val %u %s\n", in_tr_val, object_names[in_tr_val]));
  // Call Keras Code
  label_t object = run_object_classification((unsigned)in_tr_val); 
  //label_t object = the_cv_object_dict[tr_val].object;

  DEBUG(printf("  Returning object %u %s : tr_val %u %s\n", object, object_names[object], in_tr_val, object_names[in_tr_val]));
  return object;
}

void post_execute_cv_kernel(label_t tr_val, label_t cv_object)
{
  if (cv_object == tr_val) {
    label_match[cv_object]++;
    label_match[NUM_OBJECTS]++;
  } else {
    label_mismatch[tr_val][cv_object]++;
  }
  label_lookup[NUM_OBJECTS]++;
  label_lookup[cv_object]++;
}



radar_dict_entry_t* iterate_rad_kernel(vehicle_state_t vs)
{
  DEBUG(printf("In iterate_rad_kernel\n"));
  unsigned tr_val = nearest_dist[vs.lane] / RADAR_BUCKET_DISTANCE;  // The proper message for this time step and car-lane
  radar_inputs_histogram[crit_fft_samples_set][tr_val]++;
  //printf("Incrementing radar_inputs_histogram[%u][%u] to %u\n", crit_fft_samples_set, tr_val, radar_inputs_histogram[crit_fft_samples_set][tr_val]);
  return &(the_radar_return_dict[crit_fft_samples_set][tr_val]);
}
  


distance_t execute_rad_kernel(float * inputs)
{
  DEBUG(printf("In execute_rad_kernel\n"));

  /* 2) Conduct distance estimation on the waveform */
  DEBUG(printf("  Calling calculate_peak_dist_from_fmcw\n"));
  distance_t dist = calculate_peak_dist_from_fmcw(inputs);
  DEBUG(printf("  Returning distance = %.1f\n", dist));
  return dist;
}


void post_execute_rad_kernel(unsigned set, unsigned index, distance_t tr_dist, distance_t dist)
{
  // Get an error estimate (Root-Squared?)
  float error;
  radar_total_calc++;
  hist_distances[set][index]++;
  //printf("Setting hist_distances[%u][%u] to %u\n", set, index, hist_distances[set][index]);
  if ((tr_dist >= 500.0) && (dist > 10000.0)) {
    error = 0.0;
  } else {
    error = (tr_dist - dist);
  }
  float abs_err = fabs(error);
  float pct_err;
  if (tr_dist != 0.0) {
    pct_err = abs_err/tr_dist;
  } else {
    pct_err = abs_err;
  }
  
  DEBUG(printf("%f vs %f : ERROR : %f   ABS_ERR : %f PCT_ERR : %f\n", tr_dist, dist, error, abs_err, pct_err));
  //printf("IDX: %u :: %f vs %f : ERROR : %f   ABS_ERR : %f PCT_ERR : %f\n", index, tr_dist, dist, error, abs_err, pct_err);
  if (pct_err == 0.0) {
    hist_pct_errs[set][index][0]++;
  } else if (pct_err < 0.01) {
    hist_pct_errs[set][index][1]++;
  } else if (pct_err < 0.1) {
    printf("RADAR_LT010_ERR : %f vs %f : ERROR : %f   PCT_ERR : %f\n", tr_dist, dist, error, pct_err);
    hist_pct_errs[set][index][2]++;
  } else if (pct_err < 1.00) {
    printf("RADAR_LT100_ERR : %f vs %f : ERROR : %f   PCT_ERR : %f\n", tr_dist, dist, error, pct_err);
    hist_pct_errs[set][index][3]++;
  } else {
    printf("RADAR_GT100_ERR : %f vs %f : ERROR : %f   PCT_ERR : %f\n", tr_dist, dist, error, pct_err);
    hist_pct_errs[set][index][4]++;
  }
}


/* Each time-step of the trace, we read in the 
 * trace values for the left, middle and right lanes
 * (i.e. which message if the autonomous car is in the 
 *  left, middle or right lane).
 */
#define VIT_CLEAR_THRESHOLD  THRESHOLD_1
message_t iterate_vit_kernel(vehicle_state_t vs)
{
  DEBUG(printf("In iterate_vit_kernel in lane %u = %s\n", vs.lane, lane_names[vs.lane]));
  hist_total_objs[total_obj]++;
  unsigned tr_val = 0; // set a default to avoid compiler messages
  switch (vs.lane) {
  case lhazard:
    {
      unsigned nd_1 = RADAR_BUCKET_DISTANCE * (unsigned)(nearest_dist[1] / RADAR_BUCKET_DISTANCE); // floor by bucket...
      DEBUG(printf("  Lane %u : obj in %u is %c at %u\n", vs.lane, vs.lane+1, nearest_obj[vs.lane+1], nd_1));
      if ((nearest_obj[1] != 'N') && (nd_1 < VIT_CLEAR_THRESHOLD)) {  
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
      unsigned ndp1 = RADAR_BUCKET_DISTANCE * (unsigned)(nearest_dist[vs.lane+1] / RADAR_BUCKET_DISTANCE); // floor by bucket...
      unsigned ndm1 = RADAR_BUCKET_DISTANCE * (unsigned)(nearest_dist[vs.lane-1] / RADAR_BUCKET_DISTANCE); // floor by bucket...
      tr_val = 0;
      DEBUG(printf("  Lane %u : obj in %u is %c at %.1f : obj in %u is %c at %.1f\n", vs.lane, 
		   vs.lane-1, nearest_obj[vs.lane-1], nearest_dist[vs.lane-1],
		   vs.lane+1, nearest_obj[vs.lane+1], nearest_dist[vs.lane+1]));
      if ((nearest_obj[vs.lane-1] != 'N') && (ndm1 < VIT_CLEAR_THRESHOLD)) {
	// Some object is in the Left lane at distance 0 or 1
	DEBUG(printf("    Marking unsafe to move left\n"));
	tr_val += 1; // Unsafe to move from this lane to the left.
      }
      if ((nearest_obj[vs.lane+1] != 'N') && (ndp1 < VIT_CLEAR_THRESHOLD)) {
	// Some object is in the Right lane at distance 0 or 1
	DEBUG(printf("    Marking unsafe to move right\n"));
	tr_val += 2; // Unsafe to move from this lane to the right.
      }
    }
    break;
  case rhazard:
    {
      unsigned nd_3 = RADAR_BUCKET_DISTANCE * (unsigned)(nearest_dist[3] / RADAR_BUCKET_DISTANCE); // floor by bucket...
      DEBUG(printf("  Lane %u : obj in %u is %c at %u\n", vs.lane, vs.lane-1, nearest_obj[vs.lane-1], nd_3));
      if ((nearest_obj[3] != 'N') && (nd_3 < VIT_CLEAR_THRESHOLD)) {
	// Some object is in the right lane within threshold distance
	tr_val = 3; // Unsafe to move from center lane to the right.
      } else {
	tr_val = 2;
      }
    }
    break;
  }

  DEBUG(printf("Viterbi final message for lane %u %s = %u\n", vs.lane, lane_names[vs.lane], tr_val));	
  return tr_val;
}


/* Each time-step of the trace, we read in the 
 * trace values for the left, middle and right lanes
 * (i.e. which message if the autonomous car is in the 
 *  left, middle or right lane).
 */

h264_dict_entry_t* iterate_h264_kernel(vehicle_state_t vs)
{
  DEBUG(printf("In iterate_h264_kernel in lane %u = %s\n", vs.lane, lane_names[vs.lane]));
  return NULL;
}

void execute_h264_kernel(h264_dict_entry_t* in_dict, char* frame_ptr)
{
  frame_ptr = do_h264_decode();
  return;
}

void post_execute_h264_kernel(message_t tr_msg, message_t dec_msg)
{
  do_post_h264_decode();
  return;
}


/* #undef DEBUG */
/* #define DEBUG(x) x */

vehicle_state_t plan_and_control(label_t label, distance_t distance, message_t message, vehicle_state_t vehicle_state)
{
  DEBUG(printf("In the plan_and_control routine : label %u %s distance %.1f (T1 %.1f T1 %.1f T3 %.1f) message %u\n", 
	       label, object_names[label], distance, THRESHOLD_1, THRESHOLD_2, THRESHOLD_3, message));
  vehicle_state_t new_vehicle_state = vehicle_state;
  if (!vehicle_state.active) {
    // Our car is broken and burning, no plan-and-control possible.
    return vehicle_state;
  }
  
  if (//(label != no_label) && // For safety, assume every return is from SOMETHING we should not hit!
      ((distance <= THRESHOLD_1)
       #ifdef USE_SIM_ENVIRON
       || ((vehicle_state.speed < car_goal_speed) && (distance <= THRESHOLD_2))
       #endif
       )) {
    if (distance <= IMPACT_DISTANCE) {
      printf("WHOOPS: We've suffered a collision on time_step %u!\n", time_step);
      //fprintf(stderr, "WHOOPS: We've suffered a collision on time_step %u!\n", time_step);
      new_vehicle_state.speed = 0.0;
      new_vehicle_state.active = false; // We should add visualizer stuff for this!
      return new_vehicle_state;
    }
    
    // Some object ahead of us that needs to be avoided.
    DEBUG(printf("  In lane %s with %c (%u) at %.1f (trace: %.1f)\n", lane_names[vehicle_state.lane], nearest_obj[vehicle_state.lane], label, distance, nearest_dist[vehicle_state.lane]));
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
	#ifdef USE_SIM_ENVIRON
	if (vehicle_state.speed > car_decel_rate) {
	  new_vehicle_state.speed = vehicle_state.speed - car_decel_rate; // was / 2.0;
	  DEBUG(printf("   In %s with No_Safe_Move -- SLOWING DOWN from %.2f to %.2f\n", lane_names[vehicle_state.lane], vehicle_state.speed, new_vehicle_state.speed));
	} else {
	  DEBUG(printf("   In %s with No_Safe_Move -- Going < 15.0 so STOPPING!\n", lane_names[vehicle_state.lane]));
	  new_vehicle_state.speed = 0.0;
	}
	#else
	DEBUG(printf("   In %s with No_Safe_Move : STOPPING\n", lane_names[vehicle_state.lane]));
	new_vehicle_state.speed = 0.0;
	#endif
	break; /* Stop!!! */
    default:
      printf(" ERROR  In %s with UNDEFINED MESSAGE: %u\n", lane_names[vehicle_state.lane], message);
      //exit(-6);
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
    #ifdef USE_SIM_ENVIRON
    if ((vehicle_state.speed < car_goal_speed) &&  // We are going slower than we want to, and
	//((label == no_label) ||      // There is no object ahead of us -- don't need; NOTHING is at INF_DISTANCE
	(distance >= THRESHOLD_2)) { // Any object is far enough away 
      if (vehicle_state.speed <= (car_goal_speed - car_accel_rate)) {
	new_vehicle_state.speed += 15.0;
      } else {
	new_vehicle_state.speed = car_goal_speed;
      }
      DEBUG(printf("  Going %.2f : slower than target speed %.2f : Speeding up to %.2f\n", vehicle_state.speed, 50.0, new_vehicle_state.speed));
    }
    #endif
  } // else clause


  return new_vehicle_state;
}
/* #undef DEBUG */
/* #define DEBUG(x) */


void closeout_h264_kernel()
{
  do_closeout_h264_decode();
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
  printf("\nHistogram of Radar Distances:\n");
  printf("    %3s | %3s | %8s | %9s \n", "Set", "Idx", "Distance", "Occurs");
  for (int si = 0; si < num_radar_samples_sets; si++) {
    for (int di = 0; di < radar_dict_items_per_set; di++) {
      printf("    %3u | %3u | %8.3f | %9u \n", si, di, the_radar_return_dict[si][di].distance, hist_distances[si][di]);
    }
  }

  printf("\nHistogram of Radar Distance ABS-PCT-ERROR:\n");
  unsigned totals[] = {0, 0, 0, 0, 0};
  
  for (int si = 0; si < num_radar_samples_sets; si++) {
    for (int di = 0; di < radar_dict_items_per_set; di++) {
      printf("    Set %u Entry %u Id %u Distance %f Occurs %u Histogram:\n", si, di, the_radar_return_dict[si][di].index, the_radar_return_dict[si][di].distance, hist_distances[si][di]);
      for (int i = 0; i < 5; i++) {
	printf("    %7s | %9u \n", hist_pct_err_label[i], hist_pct_errs[si][di][i]);
	totals[i] += hist_pct_errs[si][di][i];
      }
    }
  }

  printf("\n  TOTALS Histogram of Radar Distance ABS-PCT-ERROR:\n");
  for (int i = 0; i < 5; i++) {
    printf("  %7s | %9u \n", hist_pct_err_label[i], totals[i]);
  }


  printf("\nHistogram of Radar Task Inputs Used:\n");
  printf("    %3s | %5s | %9s \n", "Set", "Entry", "NumOccurs");
  for (int si = 0; si < num_radar_samples_sets; si++) {
    for (int di = 0; di < radar_dict_items_per_set; di++) {
      printf("    %3u | %3u | %9u \n", si, di, radar_inputs_histogram[si][di]);
    }
  }
  printf("\n");
}



//
// This supports the IEEE 802.11p Transmit/Receive (Software-Defined Radio) Code
//  That code is in the sdr subdirectory.
//

#ifdef USE_XMIT_PIPE
status_t init_xmit_kernel()
{
  DEBUG(printf("In the init_xmit_kernel routine\n"));
  xmit_pipe_init();
  return success;
}

void iterate_xmit_kernel()
{
  DEBUG(printf("In iterate_xmit_kernel\n"));

  return;
}


void execute_xmit_kernel(int in_msg_len, char * in_msg, int* n_out, float* out_real, float* out_imag)
{
  DEBUG(printf("\nIn execute_xmit_kernel\n"));
  do_xmit_pipeline(in_msg_len, in_msg, n_out, out_real, out_imag);
  return;
}

void post_execute_xmit_kernel()
{
  DEBUG(printf("In post_execute_xmit_kernel\n"));

  return;
}



void closeout_xmit_kernel()
{
  // Nothing to do?
  return;
}

#endif



#ifdef USE_RECV_PIPE
#ifndef USE_XMIT_PIPE // We are in file-driven receive pipe only mode
#define MAX_XMIT_OUTPUTS   41800   // Really 41782 I think

extern char     recv_in_fname[256];
extern uint32_t xmit_msg_len;
extern int      xmit_num_out;
extern float    xmit_out_real[MAX_XMIT_OUTPUTS];
extern float    xmit_out_imag[MAX_XMIT_OUTPUTS];
#endif

status_t init_recv_kernel()
{
  DEBUG(printf("In init_recv_kernel...\n"));
 #ifndef USE_XMIT_PIPE
  // Read in the encoded message data
  FILE *inF = fopen(recv_in_fname, "r");
  if (!inF) {
    printf("Error: unable to open receiver-pipeline input encoded message file %s\n", recv_in_fname);
    return error;
  }

  // Read in the encoded message data
  // Read the length and number of encoded complex values
  if (fscanf(inF, "%u %d\n", &xmit_msg_len, &xmit_num_out) != 2) {
    printf("ERROR reading the encoded msg length and number of complex values\n");
    fclose(inF);
    exit(-2);
  }    
  DEBUG(printf("  The message is %u bytes and %u complex encoded values\n", xmit_msg_len, xmit_num_out));
  for (int i = 0; i < xmit_num_out; i++) {
    if (fscanf(inF, "%f %f", &xmit_out_real[i], &xmit_out_imag[i]) != 2) {
      printf("ERROR reading the complex input %d values\n", i);
      fclose(inF);
      exit(-2);
    }
  }
  fclose(inF);
  #endif

  return success;
}

void iterate_recv_kernel()
{
  DEBUG(printf("In iterate_recv_kernel\n"));

  return;
}

void execute_recv_kernel(int in_msg_len, int n_in, float* in_real, float* in_imag, int* out_msg_len, char* out_msg)
{
  DEBUG(printf("\nIn execute_recv_kernel\n"));
  do_recv_pipeline(in_msg_len, n_in, in_real, in_imag, out_msg_len, out_msg);
  return;
}

void post_execute_recv_kernel()
{
  DEBUG(printf("In post_execute_recv_kernel\n"));

  return;
}


void closeout_recv_kernel()
{
  /* printf("\nHistogram of Recvar Distances:\n"); */
  /* for (int di = 0; di < num_recvar_dictionary_items; di++) { */
  /*   printf("    %3u | %8.3f | %9u \n", di, the_recvar_return_dict[di].distance, hist_distances[di]); */
  /* } */

  return;
}


// This code supports the execution of the LZ4 Compression code
#include "lz4.h"

// This does any required initialization by the underlying code, etc.
//  Would also read in the dictionary, etc. (if there is one)
status_t init_lz4_kernel(char* dict_fn)
{
  return success;
}

// This routine will select/create an input mesage to be encoded (and then decoded)
//  The vehicle state is sent in (in case that is useful in the processing)
//  The "gen_mlen" is the bytes of the generated
//  The gen_msg is the generated message (bytes)
void iterate_lz4_kernel(vehicle_state_t vs, int* gen_mlen, unsigned char* gen_msg)
{
 #ifdef INT_TIME
  gettimeofday(&l_iterate_start, NULL);
 #endif // INT_TIME
  // Here, we will set up a random message of up to 100000 bytes
  int num = (rand() % (MAX_LZ4_MSG_SIZE)) + 1; // Return a value from [1,MAX_LZ4_MSG_SIZE]
  *gen_mlen = num;
  int idx = (rand() % (256)); // Return a value from [0,255] (byte values)
  for (int i = 0; i < num; i++) {
    gen_msg[i] = idx;
    idx = (idx ^ i)&0xFF;
    //printf("GEN_MSG : %7u : %02x\n", i, gen_msg[i]);
  }
  for (int i = num; i < MAX_LZ4_MSG_SIZE; i++) {
    gen_msg[i] = 0x0;
  }
  int hidx = num / LZ4_MLEN_HIST_SCALE;
  //printf("LZ4_MLEN : %7u : %2u  (%u)\n", num, hidx, LZ4_MLEN_HIST_SCALE);
  lz4_len_histogram[hidx]++;
 #ifdef INT_TIME
  gettimeofday(&l_iterate_stop, NULL);
  l_iterate_sec  += l_iterate_stop.tv_sec  - l_iterate_start.tv_sec;
  l_iterate_usec += l_iterate_stop.tv_usec - l_iterate_start.tv_usec;
 #endif
}


void execute_lz4_compress_kernel(int in_mlen, unsigned char* in_msg, int* enc_mlen, unsigned char* enc_msg)
{
 #ifdef INT_TIME
  gettimeofday(&l_compr_start, NULL);
 #endif // INT_TIME
  int r_bytes = LZ4_compress_default((char*)in_msg, (char*)enc_msg, in_mlen, MAX_LZ4_MSG_SIZE+1);
  *enc_mlen = r_bytes;
 #ifdef INT_TIME
  gettimeofday(&l_compr_stop, NULL);
  l_compr_sec  += l_compr_stop.tv_sec  - l_compr_start.tv_sec;
  l_compr_usec += l_compr_stop.tv_usec - l_compr_start.tv_usec;
 #endif
}

void execute_lz4_decompress_kernel(int in_mlen, unsigned char* in_msg, int* dec_mlen, unsigned char* dec_msg)
{
 #ifdef INT_TIME
  gettimeofday(&l_decmp_start, NULL);
 #endif // INT_TIME
  int r_bytes = LZ4_decompress_safe((char*)in_msg, (char*)dec_msg, in_mlen, MAX_LZ4_MSG_SIZE);
  *dec_mlen = r_bytes;
 #ifdef INT_TIME
  gettimeofday(&l_decmp_stop, NULL);
  l_decmp_sec  += l_decmp_stop.tv_sec  - l_decmp_start.tv_sec;
  l_decmp_usec += l_decmp_stop.tv_usec - l_decmp_start.tv_usec;
 #endif
}

void post_execute_lz4_kernel(int in_mlen, unsigned char* in_msg, int dec_mlen, unsigned char* dec_msg)
{
  lz4_msg_compares++;
  if (in_mlen != dec_mlen) {
    printf("LZ4 ERROR : length mis-match IN %u DEC %u\n", in_mlen, dec_mlen);
    lz4_mlen_miscompare++;
    return;
  }
  for (int i = 0; i < in_mlen; i++) {
    if (in_msg[i] != dec_msg[i]) {
      printf("LZ4 ERROR : content mis-match at %u : IN %02x DEC %02x\n", i, in_msg[i], dec_msg[i]);
      lz4_mtext_miscompare++;
    }
  }
}

void closeout_lz4_kernel()
{
  int total_miscmp = lz4_mlen_miscompare + lz4_mtext_miscompare;
  printf("\nLZ4 Analysis: %u miscompares across %u LZ4 comp/decomp passes\n", total_miscmp, lz4_msg_compares);
  printf("            : %u mlen and %u mtext\n", lz4_mlen_miscompare, lz4_mtext_miscompare);
  printf("  Histogram of Input Message Lengths (into %u buckets of size %u)\n", LZ4_MLEN_HIST_BUCKETS, LZ4_MLEN_HIST_SCALE);
  printf("     %19s : %s\n", "Bucket Size Range", "Occurences");
  for (int i = 0; i < LZ4_MLEN_HIST_BUCKETS; i++) {
    printf("%7u to %7u : %u\n", i*LZ4_MLEN_HIST_SCALE, (i+1)*LZ4_MLEN_HIST_SCALE-1, lz4_len_histogram[i]);
  }

}

#endif