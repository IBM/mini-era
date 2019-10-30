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

#ifdef USE_SIM_ENVIRON
 #include "sim_environs.h"
#else
 #include "read_trace.h"
#endif

extern unsigned time_step;

char* lane_names[NUM_LANES] = {"LHazard", "Left", "Center", "Right", "RHazard" };
char* message_names[NUM_MESSAGES] = {"Safe_L_or_R", "Safe_R_only", "Safe_L_only", "Unsafe_L_or_R" };
char* object_names[NUM_OBJECTS] = {"Nothing", "Car", "Truck", "Person", "Bike" };

float MAX_OBJECT_SIZE = 50.0; // Max size of an object

#ifdef VERBOSE
bool_t output_viz_trace = true;
#else
bool_t output_viz_trace = false;
#endif

unsigned total_obj; // Total non-'N' obstacle objects across all lanes this time step
unsigned total_v2v_obj; // Total non-'N' obstacle objects across all lanes this time step
unsigned obj_in_lane[NUM_LANES]; // Number of obstacle objects in each lane this time step (at least one, 'n')
unsigned lane_dist[NUM_LANES][MAX_OBJ_IN_LANE]; // The distance to each obstacle object in each lane
char     lane_obj[NUM_LANES][MAX_OBJ_IN_LANE]; // The type of each obstacle object in each lane

char     nearest_obj[NUM_LANES]  = { 'N', 'N', 'N', 'N', 'N' };
float    nearest_dist[NUM_LANES] = { INF_DISTANCE, INF_DISTANCE, INF_DISTANCE, INF_DISTANCE, INF_DISTANCE };

unsigned hist_total_objs[NUM_LANES * MAX_OBJ_IN_LANE];
unsigned hist_total_v2v_objs[NUM_LANES * MAX_OBJ_IN_LANE];

unsigned rand_seed = 0; // Only used if -r <N> option set

float IMPACT_DISTANCE = 50.0; // Minimum distance at which an obstacle "impacts" MyCar (collision case)


/* These are types, functions, etc. required for VITERBI */
//#include "viterbi/utils.h"
#include "viterbi/viterbi_decoder_generic.h"
//#include "radar/calc_fmcw_dist.h"


PyObject *pName, *pModule, *pFunc, *pFunc_load;
PyObject *pArgs, *pValue, *pretValue;
#define PY_SSIZE_T_CLEAN

char *python_module = "mio";
char *python_func = "predict";	  
char *python_func_load = "loadmodel";	  



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
/*   unsigned int return_id; */
/*   float distance; */
/*   float return_data[2 * RADAR_N]; */
/* } radar_dict_entry_t; */

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
/* typedef struct { */
/*   unsigned int msg_num; */
/*   unsigned int msg_id; */
/*   ofdm_param   ofdm_p; */
/*   frame_param  frame_p; */
/*   uint8_t      in_bits[MAX_ENCODED_BITS]; */
/* } vit_dict_entry_t; */

uint8_t descramble[1600]; // I think this covers our max use cases
uint8_t actual_msg[1600];

unsigned int      num_viterbi_dictionary_items = 0;
vit_dict_entry_t* the_viterbi_trace_dict;

unsigned vit_msgs_behavior = 6; // 6 = default : 1 short global + 1 long per V2V Comm Vehicle 
unsigned occ_map_behavior  = 2; // 2 = default : form a shared occupancy map
unsigned v2v_msgs_senders  = 0; // 0 = default : V2V Vehiucles are only Cars and Trucks

unsigned total_msgs = 0; // Total messages decoded during the full run
unsigned bad_decode_msgs = 0; // Total messages decoded incorrectly during the full run


mymap_input_t     global_mymap_inputs[NUM_LANES];
occupancy_map_t   global_occupancy_map;
occupancy_map_t   global_other_maps[MAX_OBJ_IN_LANE * NUM_LANES];
unsigned          num_other_maps = 0;

unsigned hist_occ_map_pct[101];



extern void descrambler(uint8_t* in, int psdusize, char* out_msg, uint8_t* ref, uint8_t *msg);




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
  DEBUG(printf("  Read %u dict entries with %u values across them all\n", num_radar_dictionary_items, tot_dict_values));
  if (!feof(dictF)) {
    printf("NOTE: Did not hit eof on the radar dictionary file %s\n", "radar_dictionary.dfn");
  }
  fclose(dictF);

  return success;
}


/* This is the initialization of the Viterbi dictionary data, etc.
 * The format is:
 *  <n> = number of dictionary entries (message types)
 * For each dictionary entry:
 *  n1 n2 n3 n4 n5 : OFDM parms: 
 *  m1 m2 m3 m4 m5 : FRAME parms:
 *  x1 x2 x3 ...   : The message bits (input to decode routine)
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
      #ifdef SUPER_VERBOSE
      printf("%u ", c);
      #endif
      the_viterbi_trace_dict[i].in_bits[ci] = (uint8_t)c;
    }
    DEBUG(printf("\n"));
  }
  fclose(dictF);

  for (int i = 0; i < NUM_LANES * MAX_OBJ_IN_LANE; i++) {
    hist_total_objs[i] = 0;
    hist_total_v2v_objs[i] = 0;
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

status_t init_mymap_kernel(char* dict_fn)
{
  DEBUG(printf("In the init_mymap_kernel routine\n"));
  return success;
}

status_t init_cbmap_kernel(char* dict_fn)
{
  DEBUG(printf("In the init_cbmap_kernel routine\n"));
  for (int i = 0; i <= 100; i++) {
    hist_occ_map_pct[i] = 0;
  }
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


label_t execute_cv_kernel(label_t in_tr_val)
{
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

  return &(the_radar_return_dict[tr_val]);
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


void post_execute_rad_kernel(distance_t tr_dist, distance_t dist)
{
  // Get an error estimate (Root-Squared?)
  radar_total_calc++;
  if (tr_dist == INFINITY) {
    if (dist < 500.0) { // 100000.0) {
      DEBUG(printf("%f vs %f => INF_PCT_ERR\n", tr_dist, dist));
      radar_inf_errs++;
    } else {
      radar_inf_noerr++;
    }      
  } else if (dist == 0.0) {
    if (dist != 0.0) {
      DEBUG(printf("%f vs %f => INF_PCT_ERR\n", tr_dist, dist));
      radar_zero_errs++;
    } else {
      radar_zero_noerr++;
    }
  } else {
    float error   = (tr_dist - dist);
    DEBUG(float abs_err = fabs(error));
    float pct_err = error/dist;
    DEBUG(printf("%f vs %f : ERROR : %f   ABS_ERR : %f PCT_ERR : %f\n", tr_dist, dist, error, abs_err, pct_err));
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
  DEBUG(printf("In iterate_vit_kernel in lane %u = %s\n", vs.lane, lane_names[vs.lane]));
  hist_total_objs[total_obj]++;
  if (occ_map_behavior > 1) { // Only update when occ_map_behavior uses V2V communications
    hist_total_v2v_objs[total_v2v_obj]++;
  }
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
  case 6: break;
  default: printf("ERROR: Unrecognized vit_msgs_behavior value: %u\n", vit_msgs_behavior); exit(-5);
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

/* This routine determines a V2V message for communicating occupancy grid maps
 * Currently, this just picks a default full-payload message (1500 bytes) */
vit_dict_entry_t* get_v2v_message()
{
  DEBUG(printf("In get_v2v_message\n"));
  unsigned tr_val = 4; // set a default to avoid compiler messages
  vit_dict_entry_t* trace_msg; // Will hold msg input data for decode, based on trace input
  trace_msg = &(the_viterbi_trace_dict[tr_val]);
  DEBUG(printf(" VIT: Using msg %u Id %u : %s \n", trace_msg->msg_num, trace_msg->msg_id, message_names[trace_msg->msg_id]));
  return trace_msg;
}

message_t execute_vit_kernel(vit_dict_entry_t* trace_msg, int num_msgs, vit_dict_entry_t* v2v_msg, int num_v2v_msgs)
{
  DEBUG(printf("In execute_vit_kernel with %u msgs and %u v2v_msgs\n", num_msgs, num_v2v_msgs));
  // Send each message (here they are all the same) through the viterbi decoder
  message_t msg = num_message_t;
  uint8_t *result;
  char     msg_text[1600]; // Big enough to hold largest message (1500?)

  // If there are v2v messages, send them here first
  if (num_v2v_msgs > 0) {
    for (int mi = 0; mi < num_msgs; mi++) {
      DEBUG(printf("  Calling the viterbi decode routine for message %u iter %u\n", v2v_msg->msg_num, mi));
      result = decode(&(v2v_msg->ofdm_p), &(v2v_msg->frame_p), &(v2v_msg->in_bits[0]));
      // descramble the output - put it in result
      int psdusize = v2v_msg->frame_p.psdu_size;
      DEBUG(printf("  Calling the viterbi descrambler routine\n"));
      descrambler(result, psdusize, msg_text, NULL /*descram_ref*/, NULL /*msg*/);
    }
  }
  
  for (int mi = 0; mi < num_msgs; mi++) {
    DEBUG(printf("  Calling the viterbi decode routine for message %u iter %u\n", trace_msg->msg_num, mi));
    result = decode(&(trace_msg->ofdm_p), &(trace_msg->frame_p), &(trace_msg->in_bits[0]));
    // descramble the output - put it in result
    int psdusize = trace_msg->frame_p.psdu_size;
    DEBUG(printf("  Calling the viterbi descrambler routine\n"));
    descrambler(result, psdusize, msg_text, NULL /*descram_ref*/, NULL /*msg*/);

    // Here we look at the message string and select proper message_t
    switch(msg_text[3]) {
    case '0' : msg = safe_to_move_right_or_left; break;
    case '1' : msg = safe_to_move_right_only; break;
    case '2' : msg = safe_to_move_left_only; break;
    case '3' : msg = unsafe_to_move_left_or_right; break;
    default  : msg = num_message_t; break;
    }
  }

  DEBUG(printf("The execute_vit_kernel is returning msg %u\n", msg));
  
  return msg;
}

void post_execute_vit_kernel(message_t tr_msg, message_t dec_msg)
{
  total_msgs++;
  if (dec_msg != tr_msg) {
    bad_decode_msgs++;
  }
}




/* This is a utility function used to generate the inputs to an occupancy map
 * relative to the current object lane and distance */
void generate_my_occ_map_inputs(lane_t lane, mymap_input_t* map_near_obj)
{
  DEBUG(printf("In generate_my_occ_map_inputs Lane %u\n", lane));
  int lmin = (lane == lhazard) ? 0 : (lane - 1);
  int lmax = (lane == rhazard) ? 5 : (lane + 1);
  for (int li = lmin; li < lmax; li++) {
    char obj  = nearest_obj[li];
    DEBUG(printf("Lane %u Nearest Object %c\n", li, obj));
    unsigned dist = nearest_dist[li];
    if (obj != 'N') {
      if (dist > (unsigned)MAX_DISTANCE) { dist = (uint8_t)MAX_DISTANCE; }
      label_t obj_label = no_label;
      DEBUG(printf("NOTE: Adding Lane %d nearest object '%c'\n", li, obj));
      switch(obj) {
        case 'B' : obj_label = bicycle; break;
        case 'C' : obj_label = car; break;
        case 'P' : obj_label = pedestrian; break;
        case 'T' : obj_label = truck; break;
        default: printf("ERROR : Unexpected nearest object: '%c'\n", nearest_obj[li]); exit(-3);
      }
      map_near_obj[li].obj  = (uint8_t)obj_label;
      map_near_obj[li].dist = dist;
      DEBUG(printf("   Adding Obj %u %s at Dist %u\n", obj_label, object_names[obj_label], dist));
    }
  }
}

/* This is a utility function used to generate the inputs to an occupancy map
 * relative to an "other" vehicle (i.e. from their perspecitve) */
void generate_other_occ_map_inputs(lane_t lane, distance_t fdist, mymap_input_t* map_near_obj)
{
  unsigned udist = (unsigned)fdist;
  DEBUG(printf("In generate_other_occ_map_inputs Lane %u Dist %.1f %u\n", lane, fdist, udist));
    // Clear out the inputs (results)

  int lmin = (lane == lhazard) ? 0 : (lane - 1);
  int lmax = (lane == rhazard) ? 5 : (lane + 1);
  for (int li = lmin; li < lmax; li++) {
    if (obj_in_lane[li] > 0) {
      // Spin through the obstacles in the lane and get an input map from each one.
      // We go from farthest to nearest (to the original vehicle):
      //   Prune those "behind" this vehicle, keep the nearest at or ahead of it
      int close_idx = -1;
      for (int oi = 0; oi < obj_in_lane[li]; oi++) {
	if (lane_obj[li][oi] != 'N') {
	  // We have some vehicle
	  unsigned dist = lane_dist[li][oi];
	  if (dist > udist) {
	    // This is a candidate vehicle
	    close_idx = oi;
	  }
	}
      }
      // Now we know the closest obstacle vehicle to this one... it is [li][close_idx]
      
      if (close_idx >= 0) { // We have an obstacle vehicle
	char obj      = lane_obj[li][close_idx];
	unsigned dist = lane_dist[li][close_idx] - udist; // Scale to this vehicle's distance perspective
	label_t obj_label = no_label;
	DEBUG(printf("NOTE: Adding Lane %d nearest object '%c'\n", li, obj));
	switch(obj) {
          case 'B' : obj_label = bicycle; break;
          case 'C' : obj_label = car; break;
          case 'P' : obj_label = pedestrian; break;
          case 'T' : obj_label = truck; break;
          default: printf("ERROR : Unexpected nearest object: '%c'\n", nearest_obj[li]); exit(-3);
	}
	map_near_obj[li].obj  = (uint8_t)obj_label;
	map_near_obj[li].dist = dist;
	DEBUG(printf("   Adding Other Obj %u %s at Dist %u\n", obj_label, object_names[obj_label], dist));
      }
    } // if (obj_in_lane[li] > 0)
  } // for (li in lane_min to lane_max)
}

/* Each time-step, we form our occupancy map (anew) 
 * from radar data taken from our lane, and one left and one right
 */
void iterate_mymap_kernel(vehicle_state_t vs, mymap_input_t* map_near_obj)
{
  DEBUG(printf("In iterate_mymap_kernel with occ_map_behavior = %u and in lane %u = %s\n", occ_map_behavior, vs.lane, lane_names[vs.lane]));
  if (occ_map_behavior > 0) {
    for (int di = 0; di < (int)MAX_DISTANCE; di++) {
      for (int li = 0; li < NUM_LANES; li++) {
	global_occupancy_map.map[di][li] = 0;  // 0 = Not Occupied
      }
    }
    
    // Clear the inputs (global memory space)
    for (int li = 0; li < NUM_LANES; li++) {
      map_near_obj[li].obj  = no_label;
      map_near_obj[li].dist = MAX_DISTANCE;
    }    
    
    // Note that my car's distance is always 0
    generate_my_occ_map_inputs(vs.lane, map_near_obj);
  }
}

/* Utility function to build an occupancy map from occupancy map inputs.
 * This map is from the perspective of the object itself... so it is at distance 0 */

void build_occupancy_map_from_inputs(lane_t lane, unsigned dist, mymap_input_t* map_near_obj, occupancy_map_t* themap)
{
  unsigned max_dist = (unsigned)MAX_DISTANCE;
  unsigned max_size = (unsigned)MAX_OBJECT_SIZE;
  // Set up my occupancy map from MY data
  int lmin = (lane == lhazard) ? 0 : (lane - 1);
  int lmax = (lane == rhazard) ? 5 : (lane + 1);
  themap->my_lane     = lane;
  themap->my_distance = dist;	// My location is always distance 0 (zero)
  for (int li = lmin; li < lmax; li++) {
    label_t  obj  = map_near_obj[li].obj;
    unsigned dist = map_near_obj[li].dist;
    if (obj != no_label) {
      // Mark all map locations for "size" with the object label...
      DEBUG(printf(" Lane %u %s Object %d %s at Dist %u (size %u)\n", li, lane_names[li], obj, object_names[obj], dist, max_size));
      for (int os = 0; os < max_size; os++) {
	int didx = dist+os;
	if (didx < max_dist) {
	  themap->map[didx][li] = obj; // This is prob = 1 and value = label
	  //printf("MAP[%3u][%u] = %u\n", didx, li, obj);
	}
      }
    }
  }
}

void execute_mymap_kernel(vehicle_state_t vs, mymap_input_t* map_near_obj, occupancy_map_t* mymap)
{
  if (occ_map_behavior > 0) {
    build_occupancy_map_from_inputs(vs.lane, 0, map_near_obj, mymap);
  }
}

void post_execute_mymap_kernel()
{
  // Do nothing for now?
}


/* Each time-step, we form a consensus occupancy map using inputs
 * from other (obstacle) vehicles.
 */
void iterate_cbmap_kernel(vehicle_state_t vs)
{
  DEBUG(printf("In iterate_cbmap_kernel with occ_map_behavior = %u\n", occ_map_behavior));
  if (occ_map_behavior > 1) {
    num_other_maps = 0; // reset the number of other maps this time step
    mymap_input_t othermap_inputs[NUM_LANES];
    for (int li = 0; li < NUM_LANES; li++) {
      DEBUG(printf("Checking Lane %u with %u objects:\n", li, obj_in_lane[li]));
      if (obj_in_lane[li] > 0) {
	// Spin through the obstacles in the lane and get an input map from each one.
	for (int oi = 0; oi < obj_in_lane[li]; oi++) {
	  DEBUG(printf("   Lane %u Object %u is %c at %u Dist\n", li, oi, lane_obj[li][oi], lane_dist[li][oi]));
	  char objc = lane_obj[li][oi];
	  int does_v2v = ((objc != 'N') // We have an obstacle object (not "Nothing")
			  && ((v2v_msgs_senders > 1) // Every obstacle communicates V2V
			      || ((v2v_msgs_senders > 0) && (objc != 'P')) // Every obstacle but a Pedestrian
			      || ((objc == 'C') || objc == 'T')) ); // The obstacle is a Car or Truck
	  if (does_v2v) {
	    // Determine the inputs for this "other" car's occupancy map
	    for (int li = 0; li < NUM_LANES; li++) {
	      othermap_inputs[li].obj  = no_label;
	      othermap_inputs[li].dist = MAX_DISTANCE;
	    }    
	    // Generate the inputs into othermap_inputs
	    generate_other_occ_map_inputs(li, lane_dist[li][oi], othermap_inputs);

	    // Now Generate an occupancy map from this object's perspective
	    DEBUG(printf("Generating OtherMap %u from lane %u and Dist %u\n", num_other_maps, li, lane_dist[li][oi]));
	    for (int di = 0; di < (int)MAX_DISTANCE; di++) {
	      for (int li = 0; li < NUM_LANES; li++) {
		global_other_maps[num_other_maps].map[di][li] = 0;  // 0 = Not Occupied
	      }
	    }
	    global_other_maps[num_other_maps].my_lane     = li;
	    global_other_maps[num_other_maps].my_distance = lane_dist[li][oi];
	    //DEBUG(printf("2 Generating OtherMap %u from lane %u and Dist %u\n", num_other_maps, global_other_maps[num_other_maps].my_lane, global_other_maps[num_other_maps].my_distance));

	    // Now generate a map from this object's point of view (from global distance data)
	    build_occupancy_map_from_inputs(li, lane_dist[li][oi], othermap_inputs, &global_other_maps[num_other_maps]);
	    //DEBUG(printf("3 Generating OtherMap %u from lane %u and Dist %u\n", num_other_maps, global_other_maps[num_other_maps].my_lane, global_other_maps[num_other_maps].my_distance));
	    num_other_maps++;
	    // The objects are ordered (per lane) from farthest to nearest...
	    /*printf("OTHER: Lane %u Dist %u : \n", li, lane_dist[li][oi]);
	      for (int i = 0; i < obj_in_lane[li]; i++) {
	      printf("   obj %u of %u : Obj %u at Dist %u\n", i, obj_in_lane[li], lane_obj[li][i], lane_dist[li][i]);
	      }*/
	  } // if (does_v2v)
	} // for (oi loops through objects_in_lane)
      } // if (obj_in_lane[li] > 0)
    } // for (li loops through the lanes)
  } // if (occ_map_behavior > 1) 
}

/* This routine takes my occupancy map and a set of other vehicle occupancy maps 
 * (from those vehicles' perspective) and combines to make a more complete map */

void execute_cbmap_kernel(vehicle_state_t vs, occupancy_map_t* the_occ_map, occupancy_map_t* cbmap_inputs, int num_inputs)
{
  DEBUG(printf("In execute_cbmap_kernel with occ_map_behavior %u and %u other-vehicle input occupancy maps\n", occ_map_behavior, num_inputs));
  if (occ_map_behavior > 1) {
    // Combine the various occupancy maps to fill out our knowledge...
    unsigned max_dist = (unsigned)MAX_DISTANCE;
    //unsigned my_dist = the_occ_map->my_distance;
    DEBUG(for (int mi = 0; mi < num_inputs; mi++) {  // Span the set of input maps
	printf(" OTHER MAP %2u : Lane %u  Dist %u\n", mi, cbmap_inputs[mi].my_lane, cbmap_inputs[mi].my_distance);
      });
    for (int mi = 0; mi < num_inputs; mi++) {  // Span the set of input maps
      //printf(" OTHER MAP %2u : Lane %u  Dist %.1f\n", mi, cbmap_inputs[mi].my_lane, cbmap_inputs[mi].my_distance);
      unsigned other_dist = cbmap_inputs[mi].my_distance;
      //DEBUG(printf(" other_dist = %u\n", other_dist));
      for (int di = other_dist; di < max_dist; di++) { // Span the set of my occupancy map entries represented there
	unsigned cb_dist = di - other_dist; // Back into the perspective of other-vehicle
	for (int li = 0; li < NUM_LANES; li++) { // Span the set of map entries (lanes)
	  if (cbmap_inputs[mi].map[cb_dist][li] != 0) {
	    // We have some obstacle in that map location
	    if (the_occ_map->map[di][li] != 0) {
	      // We already have an obstacle there in the main occupancy map	    
	      if (the_occ_map->map[di][li] != cbmap_inputs[mi].map[cb_dist][li]) {
		// The occupancy maps don't agree on what obstacle is there...
		printf("HOLD : occupancy map confusion at occ_map[%u][%u] : Local %u and Other %u [%u]\n",
		       di, li, the_occ_map->map[di][li], cbmap_inputs[mi].map[cb_dist][li], cb_dist);
	      }
	    } else {
	      the_occ_map->map[di][li] = cbmap_inputs[mi].map[cb_dist][li];
	      DEBUG(printf("  Adding %u into occ_map[%u][%u] from Other input %u at [%u][%u]\n", cbmap_inputs[mi].map[cb_dist][li], di, li, mi, cb_dist, li));
	    }
	  } // if (other-map entry shows an object)
	} // for (li loops over the lanes) 
      } // for (di loops over distances in my map)
    } // for (mi loops over other vehicle occupancy maps)
  } // if (occ_map_behavior > 1)
}

void post_execute_cbmap_kernel(occupancy_map_t* the_occ_map)
{
  if (occ_map_behavior > 0) {
    unsigned max_dist = (unsigned)MAX_DISTANCE;
    unsigned spaces = max_dist * NUM_LANES;
    unsigned occupied_spaces = 0;
    for (int di = 0; di < max_dist; di++) { // Span the set of my occupancy map entries represented there
      for (int li = 0; li < NUM_LANES; li++) { // Span the set of map entries (lanes)
	if (the_occ_map->map[di][li] != 0) {
	  occupied_spaces++;
	}
      }
    }
    unsigned avg_occupied = (100 * occupied_spaces)/(spaces); // "percent" between 0 and 100
    hist_occ_map_pct[avg_occupied]++;
  }
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
  printf("\nHistogram of Total Objects:\n");
  unsigned sum = 0;
  for (int i = 0; i < NUM_LANES * MAX_OBJ_IN_LANE; i++) {
    if (hist_total_objs[i] != 0) {
      printf("%3u | %9u \n", i, hist_total_objs[i]);
      sum += i*hist_total_objs[i];
    }
  }

  printf("\nHistogram of Total V2V Communicating Objects:\n");
  unsigned v2v_sum = 0;
  for (int i = 0; i < NUM_LANES * MAX_OBJ_IN_LANE; i++) {
    if (hist_total_v2v_objs[i] != 0) {
      printf("%3u | %9u \n", i, hist_total_v2v_objs[i]);
      v2v_sum += i*hist_total_v2v_objs[i];
    }
  }

  double avg_objs = (1.0 * sum)/(1.0 * radar_total_calc); // radar_total_calc == total time steps
  printf("There were %.3lf obstacles per time step (average)\n", avg_objs);
  double avg_v2v_objs = (1.0 * v2v_sum)/(1.0 * radar_total_calc); // radar_total_calc == total time steps
  printf("There were %.3lf V2V Communicating obstacles per time step (average)\n", avg_v2v_objs);
  double avg_msgs = (1.0 * total_msgs)/(1.0 * radar_total_calc); // radar_total_calc == total time steps
  printf("There were %.3lf messages per time step (average)\n", avg_msgs);
  printf("There were %.3lf V2V messages per time step (average)\n", avg_v2v_objs);
  printf("There were %u bad decodes of the %u messages\n", bad_decode_msgs, total_msgs);
}


void closeout_mymap_kernel()
{
  // Nothing to do?
}

void closeout_cbmap_kernel()
{
  // Nothing to do?
  printf("\nHistogram of Occupancy Map Percent Occupied Spaces:\n");
  printf("%7s | %9s \n", "Percent", "Occurrences");
  for (int i = 0; i <= 100; i++) {
    if (hist_occ_map_pct[i] != 0) {
      printf("%7u | %9u \n", i, hist_occ_map_pct[i]);
    }
  }
}

