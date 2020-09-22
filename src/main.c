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

#include <getopt.h>

#include "kernels_api.h"

#define TIME

#ifdef INT_TIME
extern uint64_t bitrev_sec;
extern uint64_t bitrev_usec;

extern uint64_t fft_sec;
extern uint64_t fft_usec;

extern uint64_t cdfmcw_sec;
extern uint64_t cdfmcw_usec;

extern uint64_t dodec_sec;
extern uint64_t dodec_usec;

extern uint64_t depunc_sec;
extern uint64_t depunc_usec;
#endif

char h264_dict[256]; 
char cv_dict[256]; 
char rad_dict[256];
char vit_dict[256];

// The "effective" testing IEEE 802.11p messages dictionary...
unsigned use_xmit_message = 0;
char* xmit_msg;

#define XMIT_LIBRARY_SIZE      4
msg_library_entry_t xmit_msg_library[XMIT_LIBRARY_SIZE+1] = {
  {1500, "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"},
  {1500, "012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789"},
  {1500, "We hold these truths to be self-evident, that all men are created equal, that they are endowed by their Creator with certain unalienable Rights, that among these are Life, Liberty and the pursuit of Happiness.--That to secure these rights, Governments are instituted among Men, deriving their just powers from the consent of the governed, --That whenever any Form of Government becomes destructive of these ends, it is the Right of the People to alter or to abolish it, and to institute new Government, laying its foundation on such principles and organizing its powers in such form, as to them shall seem most likely to effect their Safety and Happiness. Prudence, indeed, will dictate that Governments long established should not be changed for light and transient causes; and accordingly all experience hath shewn, that mankind are more disposed to suffer, while evils are sufferable, than to right themselves by abolishing the forms to which they are accustomed. But when a long train of abuses and usurpations, pursuing invariably the same Object evinces a design to reduce them under absolute Despotism, it is their right, it is their duty, to throw off such Government, and to provide new Guards for their future security.--Such has been the patient sufferance of these Colonies; and such is now the necessity which constrains them to alter their former Systems of Government. The history of the present King of Great Britain is a history of repeated injuries and usurpations, all having etc."},
  { 4, "Msg0"},
  { 1500, "-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------" }
};

uint32_t xmit_msg_len = 0;

bool show_main_output = true;
bool show_xmit_output = false;
bool do_add_pre_pad = false;
bool show_recv_output = true;


bool_t bypass_h264_functions = false; // This is a global-disable of executing H264 execution functions...

bool_t all_obstacle_lanes_mode = false;
unsigned time_step;
unsigned pandc_repeat_factor = 1;

void print_usage(char * pname) {
  printf("Usage: %s <OPTIONS>\n", pname);
  printf(" OPTIONS:\n");
  printf("    -h         : print this helpful usage info\n");
  printf("    -o         : print the Visualizer output traace information during the run\n");
  printf("    -R <file>  : defines the input Radar dictionary file <file> to use\n");
  printf("    -V <file>  : defines the input Viterbi dictionary file <file> to use\n");
  printf("    -C <file>  : defines the input CV/CNN dictionary file <file> to use\n");
  printf("    -H <file>  : defines the input H264 dictionary file <file> to use\n");
  printf("    -b         : Bypass (do not use) the H264 functions in this run.\n");
  printf("    -t <trace> : defines the input trace file <trace> to use\n");
  printf("    -p <N>     : defines the plan-and-control repeat factor (calls per time step -- default is 1)\n");
  printf("    -f <N>     : defines which Radar Dictionary Set is used for Critical FFT Tasks\n");
  printf("               :      Each Set of Radar Dictionary Entries Can use a different sample size, etc.\n");
  printf("    -n <N>     : defines number of Viterbi messages per time step behavior:\n");
  printf("               :      0 = One message per time step\n");
  printf("               :      1 = One message per obstacle per time step\n");
  printf("               :      2 = One msg per obstacle + 1 per time step\n");
  printf("    -v <N>     : defines Viterbi message size:\n");
  printf("               :      0 = Short messages (4 characters)\n");
  printf("               :      1 = Medium messages (500 characters)\n");
  printf("               :      2 = Long messages (1000 characters)\n");
  printf("               :      3 = Max-sized messages (1500 characters)\n");
  printf("    -M <N>     : Use message <N> (0 = 1500 'x' chars, 1 = 1500 '0123456789', 2 = quote)\n");
  printf("    -T \"S\"     : Use message S (No longer than 1500 chars)\n");
  printf("    -S <0|1>   : 0=disable 1=enable output of Messages (Xmit and Recv) per time step\n");
}


	 
int main(int argc, char *argv[])
{
  vehicle_state_t vehicle_state;
  label_t label;
  distance_t distance;
  message_t message;
  char* trace_file = "";
  int opt;
  int set_last_message = 0;

  rad_dict[0] = '\0';
  vit_dict[0] = '\0';
  cv_dict[0]  = '\0';
  h264_dict[0]  = '\0';

  // put ':' in the starting of the
  // string so that program can
  // distinguish between '?' and ':'
  while((opt = getopt(argc, argv, ":hAbot:v:n:r:W:R:V:C:H:f:p:S:M:T:")) != -1) {
    switch(opt) {
    case 'h':
      print_usage(argv[0]);
      exit(0);
    case 'A':
      all_obstacle_lanes_mode = true;
      break;
    case 'o':
      output_viz_trace = true;
      break;
    case 'R':
      snprintf(rad_dict, 255, "%s", optarg);
      break;
    case 'C':
      snprintf(cv_dict, 255, "%s", optarg);
      break;
    case 'H':
      snprintf(h264_dict, 255, "%s", optarg);
      break;
    case 'V':
      snprintf(vit_dict, 255, "%s", optarg);
      break;
    case 'b':
      bypass_h264_functions = true;
      break;
    case 'p':
      pandc_repeat_factor = atoi(optarg);
      printf("Using Plan-Adn-Control repeat factor %u\n", pandc_repeat_factor);
      break;
    case 'f':
      crit_fft_samples_set = atoi(optarg);
      printf("Using Radar Dictionary samples set %u for the critical FFT tasks\n", crit_fft_samples_set);
      break;
    case 't':
      trace_file = optarg;
      printf("Using trace file: %s\n", trace_file);
      break;
    case 'v':
      vit_msgs_size = atoi(optarg);
      if (vit_msgs_size >= VITERBI_MSG_LENGTHS) {
	printf("ERROR: Specified viterbi message size (%u) is larger than max (%u) : from the -v option\n", vit_msgs_size, VITERBI_MSG_LENGTHS);
	exit(-1);
      } else {
	printf("Using viterbi message size %u = %s\n", vit_msgs_size, vit_msgs_size_str[vit_msgs_size]);
      }
      break;
    case 'n':
      vit_msgs_per_step = atoi(optarg);
      if (vit_msgs_size >= VITERBI_MSG_LENGTHS) {
	printf("ERROR: Specified viterbi messages per time step behavior (%u) is larger than max (%u) : from the -n option\n", vit_msgs_size, VITERBI_MSG_LENGTHS);
	exit(-1);
      } else {
	printf("Using viterbi messages per step behavior %u = %s\n", vit_msgs_per_step, vit_msgs_per_step_str[vit_msgs_per_step]);
      }
      break;
    case 'S':
      //show_main_output = (atoi(optarg) != 0);
      show_xmit_output = (atoi(optarg) != 0);
      show_recv_output = (atoi(optarg) != 0);
      break;
    case 'M':
      use_xmit_message = atoi(optarg);
      //printf("Setting use_xmit_message to %u\n", use_xmit_message);
      break;
    case 'T': {
      use_xmit_message = XMIT_LIBRARY_SIZE;
      int mlen = strlen(optarg);
      xmit_msg_library[XMIT_LIBRARY_SIZE].msg_len = (mlen > 1500) ? 1500 : mlen;
      strncpy(xmit_msg_library[XMIT_LIBRARY_SIZE].msg_text, optarg, 1500);
      set_last_message = 1;
      printf("Setting to use a custom %u byte message...\n", xmit_msg_library[XMIT_LIBRARY_SIZE].msg_len);
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

  if (pandc_repeat_factor == 0) {
    printf("ERROR - Pland-and-Control repeat factor must be >= 1 : %u specified (with '-p' option)\n", pandc_repeat_factor);
    exit(-1);
  }

  if (rad_dict[0] == '\0') {
    sprintf(rad_dict, "data/norm_radar_16k_dictionary.dfn");
  }
  if (vit_dict[0] == '\0') {
    sprintf(vit_dict, "data/vit_dictionary.dfn");
  }
  if (cv_dict[0] == '\0') {
    sprintf(cv_dict, "data/objects_dictionary.dfn");
  }
  if (h264_dict[0] == '\0') {
    sprintf(h264_dict, "data/h264_dictionary.dfn");
  }

  printf("\nDictionaries:\n");
  printf("   CV/CNN : %s\n", cv_dict);
  printf("   Radar  : %s\n", rad_dict);
  printf("   Viterbi: %s\n", vit_dict);

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


  char cv_py_file[] = "../cv/keras_cnn/lenet.py";

  printf("Doing initialization tasks...\n");
  /* Trace Reader initialization */
  if (!init_trace_reader(trace_file))
  {
    printf("Error: the trace reader couldn't be initialized properly.\n");
    return 1;
  }

  /* Kernels initialization */
  if (bypass_h264_functions) {
    printf("Bypassing the H264 Functionality in this run...\n");
  } else {
    printf("Initializing the H264 kernel...\n");
    if (!init_h264_kernel(h264_dict))
      {
	printf("Error: the H264 decoding kernel couldn't be initialized properly.\n");
	return 1;
      }
  }

  printf("Initializing the CV kernel...\n");
  if (!init_cv_kernel(cv_py_file, cv_dict))
  {
    printf("Error: the computer vision kernel couldn't be initialized properly.\n");
    return 1;
  }
  printf("Initializing the Radar kernel...\n");
  if (!init_rad_kernel(rad_dict))
  {
    printf("Error: the radar kernel couldn't be initialized properly.\n");
    return 1;
  }
  printf("Initializing the Viterbi kernel...\n");
  if (!init_vit_kernel(vit_dict))
  {
    printf("Error: the Viterbi decoding kernel couldn't be initialized properly.\n");
    return 1;
  }

  printf("Set show_main_output = %u\n", show_main_output);
  printf("Set show_xmit_output = %u\n", show_xmit_output);
  printf("Set show_recv_output = %u\n", show_recv_output);

  if (use_xmit_message >= XMIT_LIBRARY_SIZE) {
    if ((set_last_message) && (use_xmit_message == XMIT_LIBRARY_SIZE)) {
      printf("Using a Custom XMIT message..\n");
    } else {
      printf("ERROR - Specified an illegal message identifier: %u (There are %u in library)\n", use_xmit_message, XMIT_LIBRARY_SIZE);
      exit(-1);
    }
  } else {
    printf("Using XMIT message %u (0 ='x', 1 = '0123456789')\n", use_xmit_message);
  }
  printf("RECV message is taken from XMIT output...\n");

  xmit_msg_len = xmit_msg_library[use_xmit_message].msg_len;
  xmit_msg = xmit_msg_library[use_xmit_message].msg_text;
  if (show_main_output) {
    printf("\nXMIT_MSG:\n'%s'\n\n", xmit_msg);
  }

  printf("Initializing the XMIT kernel...\n");
  if (!init_xmit_kernel()) {
    printf("Error: the computer vision kernel couldn't be initialized properly.\n");
    return 1;
  }
  printf("Initializing the Receive kernel...\n");
  if (!init_recv_kernel()) {
    printf("Error: the receive kernel couldn't be initialized properly.\n");
    return 1;
  }

  if (crit_fft_samples_set >= num_radar_samples_sets) {
    printf("ERROR : Selected FFT Tasks from Radar Dictionary Set %u but there are only %u sets in the dictionary %s\n", crit_fft_samples_set, num_radar_samples_sets, rad_dict);
    exit(-1);
  }
    
  /* We assume the vehicle starts in the following state:
   *  - Lane: center
   *  - Speed: 50 mph
   */
  vehicle_state.active  = true;
  vehicle_state.lane    = center;
  vehicle_state.speed   = 50;
  DEBUG(printf("Vehicle starts with the following state: active: %u lane %u speed %.1f\n", vehicle_state.active, vehicle_state.lane, vehicle_state.speed));

/*** MAIN LOOP -- iterates until all the traces are fully consumed ***/
  time_step = 0;
 #ifdef TIME
  struct timeval stop_prog, start_prog;

  struct timeval stop_iter_rad, start_iter_rad;
  struct timeval stop_iter_vit, start_iter_vit;
  struct timeval stop_iter_cv , start_iter_cv;
  struct timeval stop_iter_h264 , start_iter_h264;

  uint64_t iter_rad_sec = 0LL;
  uint64_t iter_vit_sec = 0LL;
  uint64_t iter_cv_sec  = 0LL;
  uint64_t iter_h264_sec  = 0LL;

  uint64_t iter_rad_usec = 0LL;
  uint64_t iter_vit_usec = 0LL;
  uint64_t iter_cv_usec  = 0LL;
  uint64_t iter_h264_usec  = 0LL;

  struct timeval stop_exec_rad, start_exec_rad;
  struct timeval stop_exec_vit, start_exec_vit;
  struct timeval stop_exec_cv , start_exec_cv;
  struct timeval stop_exec_h264 , start_exec_h264;

  uint64_t exec_rad_sec = 0LL;
  uint64_t exec_vit_sec = 0LL;
  uint64_t exec_cv_sec  = 0LL;
  uint64_t exec_h264_sec  = 0LL;

  uint64_t exec_rad_usec = 0LL;
  uint64_t exec_vit_usec = 0LL;
  uint64_t exec_cv_usec  = 0LL;
  uint64_t exec_h264_usec  = 0LL;

  struct timeval stop_exec_pandc , start_exec_pandc;
  uint64_t exec_pandc_sec  = 0LL;
  uint64_t exec_pandc_usec  = 0LL;

  struct timeval stop_exec_xmit, start_exec_xmit;
  struct timeval stop_exec_recv, start_exec_recv;

  uint64_t exec_xmit_sec  = 0LL;
  uint64_t exec_recv_sec = 0LL;

  uint64_t exec_xmit_usec  = 0LL;
  uint64_t exec_recv_usec = 0LL;
 #endif // TIME

  printf("Starting the main loop...\n");
  /* The input trace contains the per-epoch (time-step) input data */
 #ifdef TIME
  gettimeofday(&start_prog, NULL);
 #endif
  
  read_next_trace_record(vehicle_state);
  while (!eof_trace_reader()) {
    DEBUG(printf("Vehicle_State: Lane %u %s Speed %.1f\n", vehicle_state.lane, lane_names[vehicle_state.lane], vehicle_state.speed));

    /* The computer vision kernel performs object recognition on the
     * next image, and returns the corresponding label. 
     * This process takes place locally (i.e. within this car).
     */
   #ifdef TIME
    gettimeofday(&start_iter_h264, NULL);
   #endif
    h264_dict_entry_t* hdep = 0x0;
    if (!bypass_h264_functions) {
      hdep = iterate_h264_kernel(vehicle_state);
    }
   #ifdef TIME
    gettimeofday(&stop_iter_h264, NULL);
    iter_h264_sec  += stop_iter_h264.tv_sec  - start_iter_h264.tv_sec;
    iter_h264_usec += stop_iter_h264.tv_usec - start_iter_h264.tv_usec;
   #endif

    /* The computer vision kernel performs object recognition on the
     * next image, and returns the corresponding label. 
     * This process takes place locally (i.e. within this car).
     */
   #ifdef TIME
    gettimeofday(&start_iter_cv, NULL);
   #endif
    label_t cv_tr_label = iterate_cv_kernel(vehicle_state);
   #ifdef TIME
    gettimeofday(&stop_iter_cv, NULL);
    iter_cv_sec  += stop_iter_cv.tv_sec  - start_iter_cv.tv_sec;
    iter_cv_usec += stop_iter_cv.tv_usec - start_iter_cv.tv_usec;
   #endif

    /* The radar kernel performs distance estimation on the next radar
     * data, and returns the estimated distance to the object.
     */
   #ifdef TIME
    gettimeofday(&start_iter_rad, NULL);
   #endif
    radar_dict_entry_t* rdentry_p = iterate_rad_kernel(vehicle_state);
   #ifdef TIME
    gettimeofday(&stop_iter_rad, NULL);
    iter_rad_sec  += stop_iter_rad.tv_sec  - start_iter_rad.tv_sec;
    iter_rad_usec += stop_iter_rad.tv_usec - start_iter_rad.tv_usec;
   #endif
    distance_t rdict_dist = rdentry_p->distance;
    float * ref_in = rdentry_p->return_data;
    float radar_inputs[2*RADAR_N];
    for (int ii = 0; ii < 2*RADAR_N; ii++) {
      radar_inputs[ii] = ref_in[ii];
    }

    /* The Viterbi decoding kernel performs Viterbi decoding on the next
     * OFDM symbol (message), and returns the extracted message.
     * This message can come from another car (including, for example,
     * its 'pose') or from the infrastructure (like speed violation or
     * road construction warnings). For simplicity, we define a fix set
     * of message classes (e.g. car on the right, car on the left, etc.)
     */
   #ifdef TIME
    gettimeofday(&start_iter_vit, NULL);
   #endif
    vit_dict_entry_t* vdentry_p = iterate_vit_kernel(vehicle_state);
   #ifdef TIME
    gettimeofday(&stop_iter_vit, NULL);
    iter_vit_sec  += stop_iter_vit.tv_sec  - start_iter_vit.tv_sec;
    iter_vit_usec += stop_iter_vit.tv_usec - start_iter_vit.tv_usec;
   #endif

    // Here we will simulate multiple cases, based on global vit_msgs_behavior
    int num_vit_msgs = 1;   // the number of messages to send this time step (1 is default) 
    switch(vit_msgs_per_step) {
    case 1: num_vit_msgs = total_obj; break;
    case 2: num_vit_msgs = total_obj + 1; break;
    }

    // EXECUTE the kernels using the now known inputs 
   #ifdef TIME
    gettimeofday(&start_exec_h264, NULL);
   #endif
    char* found_frame_ptr = 0x0;
    if (!bypass_h264_functions) {
      execute_h264_kernel(hdep, found_frame_ptr);
    } else {
      found_frame_ptr = (char*)0xAD065BED;
    }
   #ifdef TIME
    gettimeofday(&stop_exec_h264, NULL);
    exec_h264_sec  += stop_exec_h264.tv_sec  - start_exec_h264.tv_sec;
    exec_h264_usec += stop_exec_h264.tv_usec - start_exec_h264.tv_usec;

    gettimeofday(&start_exec_cv, NULL);
   #endif
    label = execute_cv_kernel(cv_tr_label, found_frame_ptr);
   #ifdef TIME
    gettimeofday(&stop_exec_cv, NULL);
    exec_cv_sec  += stop_exec_cv.tv_sec  - start_exec_cv.tv_sec;
    exec_cv_usec += stop_exec_cv.tv_usec - start_exec_cv.tv_usec;

    gettimeofday(&start_exec_rad, NULL);
   #endif
    distance = execute_rad_kernel(radar_inputs);
   #ifdef TIME
    gettimeofday(&stop_exec_rad, NULL);
    exec_rad_sec  += stop_exec_rad.tv_sec  - start_exec_rad.tv_sec;
    exec_rad_usec += stop_exec_rad.tv_usec - start_exec_rad.tv_usec;

    gettimeofday(&start_exec_vit, NULL);
   #endif
    message = execute_vit_kernel(vdentry_p, num_vit_msgs);
   #ifdef TIME
    gettimeofday(&stop_exec_vit, NULL);
    exec_vit_sec  += stop_exec_vit.tv_sec  - start_exec_vit.tv_sec;
    exec_vit_usec += stop_exec_vit.tv_usec - start_exec_vit.tv_usec;
   #endif

    /* The xmit kernel does a transmission pass */
    int xmit_num_out;
    float xmit_out_real[MAX_XMIT_OUTPUTS];
    float xmit_out_imag[MAX_XMIT_OUTPUTS];
   #ifdef TIME
    gettimeofday(&start_exec_xmit, NULL);
   #endif
    execute_xmit_kernel(xmit_msg_len, xmit_msg, &xmit_num_out, xmit_out_real, xmit_out_imag);
   #ifdef TIME
    gettimeofday(&stop_exec_xmit, NULL);
    exec_xmit_sec  += stop_exec_xmit.tv_sec  - start_exec_xmit.tv_sec;
    exec_xmit_usec += stop_exec_xmit.tv_usec - start_exec_xmit.tv_usec;
   #endif
    if (show_xmit_output) {
      printf("\nXMIT Pipe Final output:\n");
      for (int i = 0; i < xmit_num_out; i++) {
        printf(" xmit_out_res %6u : %11.8f + %11.8f i\n", i, xmit_out_real[i], xmit_out_imag[i]);
      }
      printf("\n");
    }
    
    /* The receive kernel does a receive of one message */
    int  recv_msg_len;
    char recv_msg[MAX_MESSAGE_LEN];
   #ifdef TIME
    gettimeofday(&start_exec_recv, NULL);
   #endif
    execute_recv_kernel(xmit_msg_len, xmit_num_out, xmit_out_real, xmit_out_imag, &recv_msg_len, recv_msg);    
   #ifdef TIME
    gettimeofday(&stop_exec_recv, NULL);
    exec_recv_sec  += stop_exec_recv.tv_sec  - start_exec_recv.tv_sec;
    exec_recv_usec += stop_exec_recv.tv_usec - start_exec_recv.tv_usec;
   #endif

    if (show_recv_output) {
      printf("Iteration %u : RECV_MSG:\n'%s'\n", time_step, recv_msg);
    }

    // POST-EXECUTE each kernel to gather stats, etc.
    if (!bypass_h264_functions) {
      post_execute_h264_kernel();
    }
    post_execute_cv_kernel(cv_tr_label, label);
    post_execute_rad_kernel(rdentry_p->set, rdentry_p->index_in_set, rdict_dist, distance);
    for (int mi = 0; mi < num_vit_msgs; mi++) {
      post_execute_vit_kernel(vdentry_p->msg_id, message);
    }

    /* The plan_and_control() function makes planning and control decisions
     * based on the currently perceived information. It returns the new
     * vehicle state.
     */
    DEBUG(printf("Time Step %3u : Calling Plan and Control %u times with message %u and distance %.1f\n", pandc_repeat_factor, time_step, message, distance));
    vehicle_state_t new_vehicle_state;
   #ifdef TIME
    gettimeofday(&start_exec_pandc, NULL);
   #endif
    for (int prfi = 0; prfi <= pandc_repeat_factor; prfi++) {
      new_vehicle_state = plan_and_control(label, distance, message, vehicle_state);
    }
   #ifdef TIME
    gettimeofday(&stop_exec_pandc, NULL);
    exec_pandc_sec  += stop_exec_pandc.tv_sec  - start_exec_pandc.tv_sec;
    exec_pandc_usec += stop_exec_pandc.tv_usec - start_exec_pandc.tv_usec;
   #endif
    vehicle_state = new_vehicle_state;

    DEBUG(printf("New vehicle state: lane %u speed %.1f\n\n", vehicle_state.lane, vehicle_state.speed));

   #ifdef TIME
    time_step++;
   #endif

    read_next_trace_record(vehicle_state);
  }

 #ifdef TIME
  gettimeofday(&stop_prog, NULL);
 #endif

  /* All the trace/simulation-time has been completed -- Quitting... */
  printf("\nRun completed %u time steps\n\n", time_step);

  if (!bypass_h264_functions) {
    closeout_h264_kernel();
  }
  closeout_cv_kernel();
  closeout_rad_kernel();
  closeout_vit_kernel();

  closeout_xmit_kernel();
  closeout_recv_kernel();

 #ifdef TIME
  {
    uint64_t total_exec = (uint64_t) (stop_prog.tv_sec - start_prog.tv_sec) * 1000000 + (uint64_t) (stop_prog.tv_usec - start_prog.tv_usec);
    uint64_t iter_rad   = (uint64_t) (iter_rad_sec)  * 1000000 + (uint64_t) (iter_rad_usec);
    uint64_t iter_vit   = (uint64_t) (iter_vit_sec)  * 1000000 + (uint64_t) (iter_vit_usec);
    uint64_t iter_cv    = (uint64_t) (iter_cv_sec)   * 1000000 + (uint64_t) (iter_cv_usec);
    uint64_t iter_h264  = (uint64_t) (iter_h264_sec) * 1000000 + (uint64_t) (iter_h264_usec);
    uint64_t exec_rad   = (uint64_t) (exec_rad_sec)  * 1000000 + (uint64_t) (exec_rad_usec);
    uint64_t exec_vit   = (uint64_t) (exec_vit_sec)  * 1000000 + (uint64_t) (exec_vit_usec);
    uint64_t exec_cv    = (uint64_t) (exec_cv_sec)   * 1000000 + (uint64_t) (exec_cv_usec);
    uint64_t exec_h264  = (uint64_t) (exec_h264_sec) * 1000000 + (uint64_t) (exec_h264_usec);
    uint64_t exec_pandc = (uint64_t) (exec_pandc_sec) * 1000000 + (uint64_t) (exec_pandc_usec);
    printf("\nProgram total execution time     %lu usec\n", total_exec);
    printf("  iterate_rad_kernel run time    %lu usec\n", iter_rad);
    printf("  iterate_vit_kernel run time    %lu usec\n", iter_vit);
    printf("  iterate_h264_kernel run time   %lu usec\n", iter_h264);
    printf("  iterate_cv_kernel run time     %lu usec\n", iter_cv);
    printf("  execute_rad_kernel run time    %lu usec\n", exec_rad);
    printf("  execute_vit_kernel run time    %lu usec\n", exec_vit);
    printf("  execute_h264_kernel run time   %lu usec\n", exec_h264);
    printf("  execute_cv_kernel run time     %lu usec\n", exec_cv);
    printf("  plan_and_control run time      %lu usec at %u factor\n", exec_pandc, pandc_repeat_factor);
  }
 #endif // TIME
 #ifdef INT_TIME
  // These are timings taken from called routines...
  printf("\n");
  uint64_t fft    = (uint64_t) (fft_sec)  * 1000000 + (uint64_t) (fft_usec);
  printf("  fft-total   run time    %lu usec\n", fft);
  uint64_t bitrev    = (uint64_t) (bitrev_sec)  * 1000000 + (uint64_t) (bitrev_usec);
  printf("  bit-reverse run time    %lu usec\n", bitrev);
  uint64_t cdfmcw    = (uint64_t) (cdfmcw_sec)  * 1000000 + (uint64_t) (cdfmcw_usec);
  printf("  calc-dist   run time    %lu usec\n", cdfmcw);

  printf("\n");
  uint64_t depunc    = (uint64_t) (depunc_sec)  * 1000000 + (uint64_t) (depunc_usec);
  printf("  depuncture  run time    %lu usec\n", depunc);
  uint64_t dodec    = (uint64_t) (dodec_sec)  * 1000000 + (uint64_t) (dodec_usec);
  printf("  do-decoding run time    %lu usec\n", dodec);
 #endif // INT_TIME

  printf("\nDone.\n");
  return 0;
}
