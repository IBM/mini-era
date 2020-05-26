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
#include <limits.h>

#include <fcntl.h>
#include <pthread.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "utils.h"
#include "verbose.h"

#ifdef COMPILE_TO_ESP
#include "contig.h"
#include "mini-era.h"
#endif

#include "scheduler.h"
#include "accelerators.h" // include AFTER scheduler.h -- needs types form scheduler.h

// Forward declarations
void release_accelerator_for_task(task_metadata_block_t* task_metadata_block);

accel_selct_policy_t global_scheduler_selection_policy;

#define total_metadata_pool_blocks  32
task_metadata_block_t master_metadata_pool[total_metadata_pool_blocks];
int free_metadata_pool[total_metadata_pool_blocks];
int free_metadata_blocks = total_metadata_pool_blocks;
unsigned allocated_metadata_blocks[NUM_JOB_TYPES];
unsigned freed_metadata_blocks[NUM_JOB_TYPES];

pthread_mutex_t free_metadata_mutex; // Used to guard access to altering the free-list metadata information, etc.
pthread_mutex_t accel_alloc_mutex;   // Used to guard access to altering the accelerator allocations

//pthread_mutex_t metadata_mutex[total_metadata_pool_blocks]; // Used to guard access to altering metadata conditional variables
//pthread_cond_t  metadata_condv[total_metadata_pool_blocks]; // These phthreads conditional variables are used to "signal" a thread to do work

pthread_t metadata_threads[total_metadata_pool_blocks];

typedef struct bi_ll_struct { int clt_block_id;  struct bi_ll_struct* next; } blockid_linked_list_t;

blockid_linked_list_t critical_live_tasks_list[total_metadata_pool_blocks];
blockid_linked_list_t* critical_live_task_head = NULL;
//blockid_linked_list_t* critical_live_task_tail = NULL;
int free_critlist_pool[total_metadata_pool_blocks];
int free_critlist_entries = total_metadata_pool_blocks;
int total_critical_tasks = 0;


const char* task_job_str[NUM_JOB_TYPES] = { "NO-JOB",
					    "FFT-TASK",
					    "VITERBI-TASK" };

const char* task_criticality_str[NUM_TASK_CRIT_LEVELS] = { "NO-TASK",
							   "BASE-TASK",
							   "ELEVATED-TASK",
							   "CRITICAL-TASK" };

const char* task_status_str[NUM_TASK_STATUS] = {"TASK-FREE",
						"TASK-ALLOCATED",
						"TASK-QUEUED",
						"TASK-RUNNING",
						"TASK-DONE"};

const char* accel_type_str[NUM_ACCEL_TYPES] = { "CPU-ACCELERATOR",
						"FFT-HWR-ACCEL",
						"VITERBI-HWR-ACCEL",
						"NO-ACCELERATOR"};

const char* scheduler_selection_policy_str[NUM_SELECTION_POLICIES] = { "Select_Accelerator_Type_and_Wait_Available",
								       "Fastest_to_Slowest_First_Available" } ;

int accelerator_in_use_by[NUM_ACCEL_TYPES-1][MAX_ACCEL_OF_EACH_TYPE];
int num_accelerators_of_type[NUM_ACCEL_TYPES-1];

void print_base_metadata_block_contents(task_metadata_block_t* mb)
{
  printf("block_id = %d @ %p\n", mb->block_id, mb);
  unsigned status = mb->status;
  if (status < NUM_TASK_STATUS) {
    printf(" ** status = %s\n", task_status_str[status]);
  } else {
    printf(" ** status = %d <= NOT a legal value!\n",  mb->status);
  }
  unsigned job_type = mb->job_type;
  if (job_type < NUM_JOB_TYPES) {
    printf("    job_type = %s\n", task_job_str[job_type]);
  } else {
    printf(" ** job_type = %d <= NOT a legal value!\n", mb->job_type);
  }
  unsigned crit_level = mb->crit_level;
  if (crit_level < NUM_TASK_CRIT_LEVELS) {
    printf("    crit_level = %s\n",  task_criticality_str[crit_level]);
  } else {
    printf(" ** crit_level = %d <= NOT a legal value!\n",  mb->crit_level);
  }
  printf("    data_size  = %d\n",  mb->data_size);
  printf("    data_view  @ %p\n", &(mb->data_view));
}

void print_fft_metadata_block_contents(task_metadata_block_t* mb) {
  print_base_metadata_block_contents(mb);
}

void print_viterbi_metadata_block_contents(task_metadata_block_t* mb)
{  
  print_base_metadata_block_contents(mb);
  viterbi_data_struct_t* vdata = (viterbi_data_struct_t*)&(mb->data_view.vit_data);
  int32_t  inMem_offset = 0;
  int32_t  inData_offset = vdata->inMem_size;
  int32_t  outData_offset = inData_offset + vdata->inData_size;
  uint8_t* in_Mem  = &(vdata->theData[inMem_offset]);
  uint8_t* in_Data = &(vdata->theData[inData_offset]);
  uint8_t* out_Data = &(vdata->theData[outData_offset]);
  printf("   Viterbi Data: @ %p\n", vdata);
  printf("      n_cbps      = %d\n", vdata->n_cbps);
  printf("      n_traceback = %d\n", vdata->n_traceback);
  printf("      n_data_bits = %d\n", vdata->n_data_bits);
  printf("      psdu_size   = %d\n", vdata->psdu_size);
  printf("      in_Mem_size   = %d\n", vdata->inMem_size);
  printf("      in_Data_size  = %d\n", vdata->inData_size);
  printf("      out_Data_size = %d\n", vdata->outData_size);
  printf("      inMem_offset  = %d\n", inMem_offset);
  printf("      inData_offset  = %d\n", inData_offset);
  printf("      outData_offset = %d\n", outData_offset);
  printf("      in_Mem   @ %p\n", &(vdata->theData[inMem_offset]));
  printf("      in_Data  @ %p\n",  &(vdata->theData[inData_offset]));
  printf("      out_Data @ %p\n",  &(vdata->theData[outData_offset]));
}


void print_critical_task_list_ids() {
  blockid_linked_list_t* cli = critical_live_task_head;
  if (cli == NULL) {
    printf("Critical task list is EMPTY\n");
  } else {
    printf("Critical task list : ");
    while (cli != NULL) {
      printf(" %u", cli->clt_block_id); //, free_critlist_pool[cli->clt_block_id]);
      cli = cli->next;
    }
    printf("\n");
  }
}



task_metadata_block_t* get_task_metadata_block(scheduler_jobs_t task_type, task_criticality_t crit_level)
{
  pthread_mutex_lock(&free_metadata_mutex);
  TDEBUG(printf("in get_task_metadata_block with %u free_metadata_blocks\n", free_metadata_blocks));
  if (free_metadata_blocks < 1) {
    // Out of metadata blocks -- all in use, cannot enqueue new tasks!
    return NULL;
  }
  int bi = free_metadata_pool[free_metadata_blocks - 1];
  TDEBUG(printf(" BEFORE_GET : MB %d : free_metadata_pool : ", bi);
	 for (int i = 0; i < total_metadata_pool_blocks; i++) {
	   printf("%d ", free_metadata_pool[i]);
	 }
	 printf("\n"));
  if ((bi < 0) || (bi > total_metadata_pool_blocks)) {
    printf("ERROR : free_metadata_pool[%u -1] = %d   with %d free_metadata_blocks\n", free_metadata_blocks, bi, free_metadata_blocks);
    for (int i = 0; i < total_metadata_pool_blocks; i++) {
      printf("  free_metadata_pool[%2u] = %d\n", i, free_metadata_pool[i]);
    }
    exit(-16);
  }
  free_metadata_pool[free_metadata_blocks - 1] = -1;
  free_metadata_blocks -= 1;
  // For neatness (not "security") we'll clear the meta-data in the block (not the data data,though)
  master_metadata_pool[bi].job_type = task_type;
  master_metadata_pool[bi].status = TASK_ALLOCATED;
  master_metadata_pool[bi].crit_level = crit_level;
  master_metadata_pool[bi].data_size = 0;
  master_metadata_pool[bi].accelerator_type = no_accelerator_t;
  master_metadata_pool[bi].accelerator_id   = -1;
  master_metadata_pool[bi].atFinish = NULL;  // NO finish call-back function
  
  gettimeofday(&master_metadata_pool[bi].sched_timings.get_start, NULL);
  master_metadata_pool[bi].sched_timings.idle_sec += master_metadata_pool[bi].sched_timings.get_start.tv_sec - master_metadata_pool[bi].sched_timings.idle_start.tv_sec;
  master_metadata_pool[bi].sched_timings.idle_usec += master_metadata_pool[bi].sched_timings.get_start.tv_usec - master_metadata_pool[bi].sched_timings.idle_start.tv_usec;
  
  if (crit_level > 1) { // is this a "critical task"
    /* int ci = total_critical_tasks; // Set index for crit_task block_id in pool */
    /* critical_live_tasks_list[ci].clt_block_id = bi;  // Set the critical task block_id indication */
    // Select the next available critical-live-task-list entry ID 
    int li = free_critlist_pool[free_critlist_entries - 1];
    free_critlist_pool[free_critlist_entries - 1] = -1; // clear it a(as it is no longer free)
    free_critlist_entries -= 1;
    // Now li indicates the critical_live_tasks_list[] index to use
    // Now set up the revisions to the critical live tasks list
    critical_live_tasks_list[li].clt_block_id = bi;   // point this entry to the master_metadata_pool block id
    critical_live_tasks_list[li].next = critical_live_task_head;     // Insert as head of critical tasks list
    critical_live_task_head = &(critical_live_tasks_list[li]);
    total_critical_tasks += 1;
  }
  DEBUG(printf("  returning block %u\n", bi);
	print_critical_task_list_ids());
  TDEBUG(printf(" AFTER_GET : MB %u : free_metadata_pool : ", bi);
	 for (int i = 0; i < total_metadata_pool_blocks; i++) {
	   printf("%d ", free_metadata_pool[i]);
	 }
	 printf("\n"));
  allocated_metadata_blocks[task_type]++;
  //printf("MB%u got allocated : %u %u\n", bi, task_type, crit_level);
  pthread_mutex_unlock(&free_metadata_mutex);
  
  return &(master_metadata_pool[bi]);
}





void free_task_metadata_block(task_metadata_block_t* mb)
{
  pthread_mutex_lock(&free_metadata_mutex);

  int bi = mb->block_id;
  //printf("MB%u getting freed : %u %u\n", bi, mb->job_type, mb->crit_level);
  TDEBUG(printf("in free_task_metadata_block for block %u with %u free_metadata_blocks\n", bi, free_metadata_blocks);//);
	 printf(" BEFORE_FREE : MB %u : free_metadata_pool : ", bi);
	 for (int i = 0; i < total_metadata_pool_blocks; i++) {
	   printf("%d ", free_metadata_pool[i]);
	 }
	 printf("\n"));

  if (free_metadata_blocks < total_metadata_pool_blocks) {
    free_metadata_pool[free_metadata_blocks] = bi;
    free_metadata_blocks += 1;
    if (master_metadata_pool[bi].crit_level > 1) { // is this a critical tasks?
      // Remove task form critical list, free critlist entry, etc.
      blockid_linked_list_t* lcli = NULL;
      blockid_linked_list_t* cli = critical_live_task_head;
      //while ((cli != NULL) && (critical_live_tasks_list[cli->clt_block_id].clt_block_id != bi)) {
      while ((cli != NULL) && (cli->clt_block_id != bi)) {
	lcli = cli;  // The "previoud" block; NULL == "head"
	cli = cli->next;	
      }
      if (cli == NULL) {
	printf("ERROR: Critical task NOT on the critical_live_task_list :\n");
	print_base_metadata_block_contents(mb);
	exit(-6);
      }
      // We've found the critical task in critical_live_tasks_list - cli points to it
      int cti = cli->clt_block_id;
      //printf(" freeing critlist_pool %u to %u\n", free_critlist_entries - 1, cti);
      free_critlist_pool[free_critlist_entries] = cti; // Enable this crit-list entry for new use
      free_critlist_entries += 1; // Update the count of available critlist entries in the pool
      cli->clt_block_id = -1; // clear the clt_block_id indicator (we're done with it)
      // And remove the cli entry from the critical_lvet_tasks linked list
      if (lcli == NULL) {
	critical_live_task_head = cli->next;
      } else {
	lcli->next = cli->next;
      }
      cli->next = NULL;
      total_critical_tasks -= 1;
    }
    master_metadata_pool[bi].atFinish = NULL; // Ensure this is now set to NULL (safety safety)
    // For neatness (not "security") we'll clear the meta-data in the block (not the data data, though)
    freed_metadata_blocks[master_metadata_pool[bi].job_type]++;
    master_metadata_pool[bi].job_type = NO_TASK_JOB; // unset
    master_metadata_pool[bi].status = TASK_FREE;   // free
    gettimeofday(&master_metadata_pool[bi].sched_timings.idle_start, NULL);
    master_metadata_pool[bi].sched_timings.done_sec += master_metadata_pool[bi].sched_timings.idle_start.tv_sec - master_metadata_pool[bi].sched_timings.done_start.tv_sec;
    master_metadata_pool[bi].sched_timings.done_usec += master_metadata_pool[bi].sched_timings.idle_start.tv_usec - master_metadata_pool[bi].sched_timings.done_start.tv_usec;
    master_metadata_pool[bi].crit_level = NO_TASK; // lowest/free?
    master_metadata_pool[bi].data_size = 0;
  } else {
    printf("ERROR : We are freeing a metadata block when we already have max metadata blocks free...\n");
    printf("   THE FREE Metadata Blocks list:\n");
    for (int ii = 0; ii < free_metadata_blocks; ii++) {
      printf("        free[%2u] = %u\n", ii, free_metadata_pool[ii]);
    }
    DEBUG(printf("    THE Being-Freed Meta-Data Block:\n");
	  print_base_metadata_block_contents(mb));
    exit(-5);
  }
  TDEBUG(printf(" AFTER_FREE : MB %u : free_metadata_pool : ", bi);
	 for (int i = 0; i < total_metadata_pool_blocks; i++) {
	   printf("%d ", free_metadata_pool[i]);
	 }
	 printf("\n"));
  pthread_mutex_unlock(&free_metadata_mutex);
}




int
get_task_status(int task_id) {
  return master_metadata_pool[task_id].status;
}


void mark_task_done(task_metadata_block_t* task_metadata_block)
{
	printf("MB%u in mark_task_done\n", task_metadata_block->block_id);
  // First release the accelerator
  release_accelerator_for_task(task_metadata_block);

  // Then, mark the task as "DONE" with execution
  task_metadata_block->status = TASK_DONE;
  gettimeofday(&task_metadata_block->sched_timings.done_start, NULL);
  task_metadata_block->sched_timings.running_sec += task_metadata_block->sched_timings.done_start.tv_sec - task_metadata_block->sched_timings.running_start.tv_sec;
  task_metadata_block->sched_timings.running_usec += task_metadata_block->sched_timings.done_start.tv_usec - task_metadata_block->sched_timings.queued_start.tv_usec;

  // And finally, call the call-back if there is one... (which might clear out the metadata_block entirely)
  if (task_metadata_block->atFinish != NULL) {
    // And finally, call the atFinish call-back routine specified in the MetaData Block
    task_metadata_block->atFinish(task_metadata_block);
  }
}

static unsigned DMA_WORD_PER_BEAT(unsigned _st)
{
  return (sizeof(void *) / _st);
}

#define  NUM_VIT_ACCEL  4
#ifdef HW_VIT
// These are Viterbi Harware Accelerator Variables, etc.
char*    vitAccelName[NUM_VIT_ACCEL] = {"/dev/vitdodec.0", "/dev/vitdodec.1", "/dev/vitdodec.2", "/dev/vitdodec.3"};

int vitHW_fd[NUM_VIT_ACCEL];
contig_handle_t vitHW_mem[NUM_VIT_ACCEL];
vitHW_token_t *vitHW_lmem[NUM_VIT_ACCEL];   // Pointer to local view of contig memory
vitHW_token_t *vitHW_li_mem[NUM_VIT_ACCEL]; // Pointer to input memory block
vitHW_token_t *vitHW_lo_mem[NUM_VIT_ACCEL]; // Pointer to output memory block
size_t vitHW_in_len[NUM_VIT_ACCEL];
size_t vitHW_out_len[NUM_VIT_ACCEL];
size_t vitHW_in_size[NUM_VIT_ACCEL];
size_t vitHW_out_size[NUM_VIT_ACCEL];
size_t vitHW_out_offset[NUM_VIT_ACCEL];
size_t vitHW_size[NUM_VIT_ACCEL];

struct vitdodec_access vitHW_desc[NUM_VIT_ACCEL];


static void init_vit_parameters(int vn)
{
  size_t vitHW_in_words_adj;
  size_t vitHW_out_words_adj;
  //printf("Doing init_vit_parameters\n");
  if (DMA_WORD_PER_BEAT(sizeof(vitHW_token_t)) == 0) {
    vitHW_in_words_adj  = 24852;
    vitHW_out_words_adj = 18585;
  } else {
    vitHW_in_words_adj  = round_up(24852, DMA_WORD_PER_BEAT(sizeof(vitHW_token_t)));
    vitHW_out_words_adj = round_up(18585, DMA_WORD_PER_BEAT(sizeof(vitHW_token_t)));
  }
  vitHW_in_len[vn] = vitHW_in_words_adj;
  vitHW_out_len[vn] =  vitHW_out_words_adj;
  vitHW_in_size[vn] = vitHW_in_len[vn] * sizeof(vitHW_token_t);
  vitHW_out_size[vn] = vitHW_out_len[vn] * sizeof(vitHW_token_t);
  vitHW_out_offset[vn] = vitHW_in_len[vn];
  vitHW_size[vn] = (vitHW_out_offset[vn] * sizeof(vitHW_token_t)) + vitHW_out_size[vn];
}
#endif // HW_VIT


// Right now default to max of 16k-samples
unsigned fft_logn_samples = 14; // Defaults to 16k samples

#define NUM_FFT_ACCEL 4
#ifdef HW_FFT
// These are FFT Hardware Accelerator Variables, etc.
char* fftAccelName[NUM_FFT_ACCEL] = {"/dev/fft.0", "/dev/fft.1", "/dev/fft.2", "/dev/fft.3"};

int fftHW_fd[NUM_FFT_ACCEL];
contig_handle_t fftHW_mem[NUM_FFT_ACCEL];

fftHW_token_t* fftHW_lmem[NUM_FFT_ACCEL];  // Pointer to local version (mapping) of fftHW_mem
fftHW_token_t* fftHW_li_mem[NUM_FFT_ACCEL]; // Pointer to input memory block
fftHW_token_t* fftHW_lo_mem[NUM_FFT_ACCEL]; // Pointer to output memory block
size_t fftHW_in_len[NUM_FFT_ACCEL];
size_t fftHW_out_len[NUM_FFT_ACCEL];
size_t fftHW_in_size[NUM_FFT_ACCEL];
size_t fftHW_out_size[NUM_FFT_ACCEL];
size_t fftHW_out_offset[NUM_FFT_ACCEL];
size_t fftHW_size[NUM_FFT_ACCEL];
struct fftHW_access fftHW_desc[NUM_FFT_ACCEL];


/* User-defined code */
static void init_fft_parameters(unsigned n)
{
  size_t fftHW_in_words_adj;
  size_t fftHW_out_words_adj;
  int len = 1 << fft_logn_samples;
  if (DMA_WORD_PER_BEAT(sizeof(fftHW_token_t)) == 0) {
    fftHW_in_words_adj  = 2 * len;
    fftHW_out_words_adj = 2 * len;
  } else {
    fftHW_in_words_adj = round_up(2 * len, DMA_WORD_PER_BEAT(sizeof(fftHW_token_t)));
    fftHW_out_words_adj = round_up(2 * len, DMA_WORD_PER_BEAT(sizeof(fftHW_token_t)));
  }
  fftHW_in_len[n] = fftHW_in_words_adj;
  fftHW_out_len[n] =  fftHW_out_words_adj;
  fftHW_in_size[n] = fftHW_in_len[n] * sizeof(fftHW_token_t);
  fftHW_out_size[n] = fftHW_out_len[n] * sizeof(fftHW_token_t);
  fftHW_out_offset[n] = 0;
  fftHW_size[n] = (fftHW_out_offset[n] * sizeof(fftHW_token_t)) + fftHW_out_size[n];
}
#endif // HW_FFT



// NOTE: This is executed by a metadata_block pthread
void
execute_task_on_accelerator(task_metadata_block_t* task_metadata_block)
{
  TDEBUG(printf("In execute_task_on_accelerator for MB %d with Accel Type %s and Number %u\n", task_metadata_block->block_id, accel_type_str[task_metadata_block->accelerator_type], task_metadata_block->accelerator_id));
  switch(task_metadata_block->accelerator_type) {
  case no_accelerator_t: {
    printf("ERROR -- called execute_task_on_accelerator for NO_ACCELERATOR_T with block:\n");
    print_base_metadata_block_contents(task_metadata_block);
    exit(-11);
  } break;
  case cpu_accel_t: {
    switch(task_metadata_block->job_type) {
    case FFT_TASK:
      DEBUG(printf("Executing Task for MB %d on CPU_FFT_ACCELERATOR\n", task_metadata_block->block_id));
      execute_cpu_fft_accelerator(task_metadata_block);
      break;
    case VITERBI_TASK:
      DEBUG(printf("Executing Task for MB %d on CPU_VITERBI_ACCELERATOR\n", task_metadata_block->block_id));
      execute_cpu_viterbi_accelerator(task_metadata_block);
      break;
    default:
      printf("ERROR : execute_task_on_accelerator called for unknown task type: %u\n", task_metadata_block->job_type);
      exit(-13);
    }
  } break;
  case fft_hwr_accel_t: {
    DEBUG(printf("Executing Task for MB %d on HWR_FFT_ACCELERATOR\n", task_metadata_block->block_id));
    execute_hwr_fft_accelerator(task_metadata_block);
  } break;
  case vit_hwr_accel_t: {
    DEBUG(printf("Executing Task for MB %d on HWR_VITERBI_ACCELERATOR\n", task_metadata_block->block_id));
    execute_hwr_viterbi_accelerator(task_metadata_block);
  } break;
  default:
    printf("ERROR : Unknown accelerator type in execute_task_on_accelerator with block:\n");
    print_base_metadata_block_contents(task_metadata_block);
    exit(-12);
  }
  TDEBUG(printf("DONE Executing Task for MB %d\n", task_metadata_block->block_id));
}


void*
metadata_thread_wait_for_task(void* void_parm_ptr)
{
  task_metadata_block_t* task_metadata_block = (task_metadata_block_t*)void_parm_ptr;
  int bi = task_metadata_block->block_id;
  DEBUG(printf("In metadata_thread_wait_for_task for thread for metadata block %d\n", bi));
  // I think we do this once, then can wait_cond many times
  pthread_mutex_lock(&(task_metadata_block->metadata_mutex));
  do {
    TDEBUG(printf("MB_THREAD %d calling pthread_cond_wait\n", bi));
    // This will cause the thread to wait for a triggering signal through metadata_condv[bi]
    pthread_cond_wait(&(task_metadata_block->metadata_condv), &(task_metadata_block->metadata_mutex));

    TDEBUG(printf("MB_THREAD %d calling execute_task_on_accelerator...\n", bi));
    // Now we have been "triggered" -- so we invoke the appropriate accelerator
    execute_task_on_accelerator(task_metadata_block);
  } while (1); // We will loop here forever, until the main program exits....

  // We should never fall out, but in case we do, clean up
  pthread_mutex_unlock(&(task_metadata_block->metadata_mutex));
}



status_t initialize_scheduler()
{
  DEBUG(printf("In initialize...\n"));
  pthread_mutex_init(&free_metadata_mutex, NULL);
  pthread_mutex_init(&accel_alloc_mutex, NULL);
  struct timeval init_time;
  gettimeofday(&init_time, NULL);
  for (int i = 0; i < total_metadata_pool_blocks; i++) {
    master_metadata_pool[i].block_id = i; // Set the master pool's block_ids
    // Clear the (full-run, aggregate) timing data spaces
    gettimeofday( &(master_metadata_pool[i].sched_timings.idle_start), NULL);
    // Scheduler timings 
    master_metadata_pool[i].sched_timings.idle_sec = 0;
    master_metadata_pool[i].sched_timings.idle_usec = 0;
    master_metadata_pool[i].sched_timings.get_sec = 0;
    master_metadata_pool[i].sched_timings.get_usec = 0;
    master_metadata_pool[i].sched_timings.queued_sec = 0;
    master_metadata_pool[i].sched_timings.queued_usec = 0;
    master_metadata_pool[i].sched_timings.running_sec = 0;
    master_metadata_pool[i].sched_timings.running_usec = 0;
    master_metadata_pool[i].sched_timings.done_sec = 0;
    master_metadata_pool[i].sched_timings.done_usec = 0;
    // FFT task timings
    master_metadata_pool[i].fft_timings.calc_sec = 0;
    master_metadata_pool[i].fft_timings.calc_usec = 0;
    master_metadata_pool[i].fft_timings.fft_sec = 0;
    master_metadata_pool[i].fft_timings.fft_usec = 0;
    master_metadata_pool[i].fft_timings.fft_br_sec = 0;
    master_metadata_pool[i].fft_timings.fft_br_usec = 0;
    master_metadata_pool[i].fft_timings.fft_cvtin_sec = 0;
    master_metadata_pool[i].fft_timings.fft_cvtin_usec = 0;
    master_metadata_pool[i].fft_timings.fft_cvtout_sec = 0;
    master_metadata_pool[i].fft_timings.fft_cvtout_usec = 0;
    master_metadata_pool[i].fft_timings.cdfmcw_sec = 0;
    master_metadata_pool[i].fft_timings.cdfmcw_usec = 0;
    // Viterbi task timings
    master_metadata_pool[i].vit_timings.dodec_sec = 0;
    master_metadata_pool[i].vit_timings.dodec_usec = 0;
    master_metadata_pool[i].vit_timings.depunc_sec = 0;
    master_metadata_pool[i].vit_timings.depunc_usec = 0;

    pthread_mutex_init(&(master_metadata_pool[i].metadata_mutex), NULL);
    pthread_cond_init(&(master_metadata_pool[i].metadata_condv), NULL);

    free_metadata_pool[i] = i;    // Set up all blocks are free
    free_critlist_pool[i] = i;    // Set up all critlist blocks are free
  }

  // Now initialize the per-metablock threads
  // For portability (as per POSIX documentation) explicitly create threads in joinable state
  pthread_attr_t  pt_attr;
  pthread_attr_init(&pt_attr);
  pthread_attr_setdetachstate(&pt_attr, PTHREAD_CREATE_JOINABLE);
  for (int i = 0; i < total_metadata_pool_blocks; i++) {
    if (pthread_create(&metadata_threads[i], &pt_attr, metadata_thread_wait_for_task, &(master_metadata_pool[i]))) {
      printf("ERROR: Scheduler failed to create thread for metadata block: %d\n", i);
      exit(-10);
    }
    master_metadata_pool[i].thread_id = metadata_threads[i];
  }

  for (int ti = 0; ti < NUM_JOB_TYPES; ti++) {
    allocated_metadata_blocks[ti] = 0;
    freed_metadata_blocks[ti] = 0;
  }
  
  // I'm hard-coding these for now.
  num_accelerators_of_type[cpu_accel_t] = 10;  // also tested with 1 -- works.
  num_accelerators_of_type[fft_hwr_accel_t] = NUM_FFT_ACCEL;
  num_accelerators_of_type[vit_hwr_accel_t] = NUM_VIT_ACCEL;

  for (int i = 0; i < NUM_ACCEL_TYPES-1; i++) {
    for (int j = 0; j < MAX_ACCEL_OF_EACH_TYPE; j++) {
      accelerator_in_use_by[i][j] = -1; // NOT a valid metadata block ID; -1 indicates "Not in Use"
    }
  }

#ifdef HW_FFT
  // This initializes the FFT Accelerator Pool
  for (int fi = 0; fi < NUM_FFT_ACCEL; fi++) {
    DEBUG(printf("Init FFT parameters on acclerator %u\n", fi));
    init_fft_parameters(fi);

    DEBUG(printf(" Acclerator %u opening FFT device %s\n", fi, fftAccelName[fi]));
    fftHW_fd[fi] = open(fftAccelName[fi], O_RDWR, 0);
    if (fftHW_fd[fi] < 0) {
      fprintf(stderr, "Error: cannot open %s", fftAccelName[fi]);
      exit(EXIT_FAILURE);
    }

    printf(" Allocate hardware buffer of size %u\n", fftHW_size[fi]);
    fftHW_lmem[fi] = contig_alloc(fftHW_size[fi], &(fftHW_mem[fi]));
    if (fftHW_lmem[fi] == NULL) {
      fprintf(stderr, "Error: cannot allocate %zu contig bytes", fftHW_size[fi]);
      exit(EXIT_FAILURE);
    }

    fftHW_li_mem[fi] = &(fftHW_lmem[fi][0]);
    fftHW_lo_mem[fi] = &(fftHW_lmem[fi][fftHW_out_offset[fi]]);
    printf(" Set fftHW_li_mem = %p  AND fftHW_lo_mem = %p\n", fftHW_li_mem[fi], fftHW_lo_mem[fi]);

    fftHW_desc[fi].esp.run = true;
    fftHW_desc[fi].esp.coherence = ACC_COH_NONE;
    fftHW_desc[fi].esp.p2p_store = 0;
    fftHW_desc[fi].esp.p2p_nsrcs = 0;
    //fftHW_desc[fi].esp.p2p_srcs = {"", "", "", ""};
    fftHW_desc[fi].esp.contig = contig_to_khandle(fftHW_mem[fi]);

    // Always use BIT-REV in HW for now -- simpler interface, etc.
    fftHW_desc[fi].do_bitrev  = FFTHW_DO_BITREV;

    //fftHW_desc[fi].len      = fftHW_len;
    fftHW_desc[fi].log_len    = fft_logn_samples; 
    fftHW_desc[fi].src_offset = 0;
    fftHW_desc[fi].dst_offset = 0;
  }
#endif

#ifdef HW_VIT
  // This initializes the Viterbi Accelerator Pool
  for (int vi = 0; vi < NUM_VIT_ACCEL; vi++) {
    DEBUG(printf("Init Viterbi parameters on acclerator %u\n", vi));
    init_vit_parameters(vi);

    printf(" Accelerator %u opening Vit-Do-Decode device %s\n", vi, vitAccelName[vi]);
    vitHW_fd[vi] = open(vitAccelName[vi], O_RDWR, 0);
    if(vitHW_fd < 0) {
      fprintf(stderr, "Error: cannot open %s", vitAccelName[vi]);
      exit(EXIT_FAILURE);
    }

    vitHW_lmem[vi] = contig_alloc(vitHW_size[vi], &(vitHW_mem[vi]));
    if (vitHW_lmem[vi] == NULL) {
      fprintf(stderr, "Error: cannot allocate %zu contig bytes", vitHW_size[vi]);
      exit(EXIT_FAILURE);
    }
    vitHW_li_mem[vi] = &(vitHW_lmem[vi][0]);
    vitHW_lo_mem[vi] = &(vitHW_lmem[vi][vitHW_out_offset[vi]]);
    printf(" Set vitHW_li_mem = %p  AND vitHW_lo_mem = %p\n", vitHW_li_mem[vi], vitHW_lo_mem[vi]);

    vitHW_desc[vi].esp.run = true;
    vitHW_desc[vi].esp.coherence = ACC_COH_NONE;
    vitHW_desc[vi].esp.p2p_store = 0;
    vitHW_desc[vi].esp.p2p_nsrcs = 0;
    vitHW_desc[vi].esp.contig = contig_to_khandle(vitHW_mem[vi]);
  }
#endif
  DEBUG(printf("DONE with initialize -- returning success\n"));
  return success;
}


#ifdef COMPILE_TO_ESP
#include "fixed_point.h"
#endif
#include "calc_fmcw_dist.h"

#ifdef HW_FFT
unsigned int fft_rev(unsigned int v)
{
  unsigned int r = v;
  int s = sizeof(v) * CHAR_BIT - 1;

  for (v >>= 1; v; v >>= 1) {
    r <<= 1;
    r |= v & 1;
    s--;
  }
  r <<= s;
  return r;
}

void fft_bit_reverse(float *w, unsigned int n, unsigned int bits)
{
  unsigned int i, s, shift;

  s = sizeof(i) * CHAR_BIT - 1;
  shift = s - bits + 1;

  for (i = 0; i < n; i++) {
    unsigned int r;
    float t_real, t_imag;

    r = fft_rev(i);
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
}

static void fft_in_hw(int *fd, struct fftHW_access *desc)
{
  if (ioctl(*fd, FFTHW_IOC_ACCESS, *desc)) {
    perror("IOCTL:");
    exit(EXIT_FAILURE);
  }
}
#endif

void
execute_hwr_fft_accelerator(task_metadata_block_t* task_metadata_block)
{
  int fn = task_metadata_block->accelerator_id;
  TDEBUG(printf("In execute_hwr_fft_accelerator on FFT_HWR Accel %u : MB %d  CL %d\n", fn, task_metadata_block->block_id, task_metadata_block->crit_level));
#ifdef HW_FFT
  float * data = (float*)(task_metadata_block->data_view.fft_data);
  // convert input from float to fixed point
  for (int j = 0; j < 2 * (1 << fft_logn_samples); j++) {
    fftHW_lmem[fn][j] = float2fx(data[j], FX_IL);
  }

  // Call the FFT Accelerator
  //    NOTE: Currently this is blocking-wait for call to complete
  fft_in_hw(&(fftHW_fd[fn]), &(fftHW_desc[fn]));

  // convert output from fixed point to float
  for (int j = 0; j < 2 * (1 << fft_logn_samples); j++) {
    data[j] = (float)fx2float(fftHW_lmem[fn][j], FX_IL);
  }

  //TDEBUG
  (printf("MB_THREAD %u calling mark_task_done...\n", task_metadata_block->block_id));
  mark_task_done(task_metadata_block);

#else
  printf("ERROR : This executable DOES NOT support Hardware-FFT execution!\n");
  exit(-2);
#endif
}




#ifdef HW_VIT
static void do_decoding_hw(int *fd, struct vitdodec_access *desc)
{
  if (ioctl(*fd, VITDODEC_IOC_ACCESS, *desc)) {
    perror("IOCTL:");
    exit(EXIT_FAILURE);
  }
}
#endif


void
execute_hwr_viterbi_accelerator(task_metadata_block_t* task_metadata_block)
{
  int vn = task_metadata_block->accelerator_id;
  TDEBUG(printf("In execute_hwr_viterbi_accelerator on FFT_HWR Accel %u : MB %d  CL %d\n", vn, task_metadata_block->block_id, task_metadata_block->crit_level));
  viterbi_data_struct_t* vdata = (viterbi_data_struct_t*)&(task_metadata_block->data_view.vit_data);
  int32_t  in_cbps = vdata->n_cbps;
  int32_t  in_ntraceback = vdata->n_traceback;
  int32_t  in_data_bits = vdata->n_data_bits;
  int32_t  inMem_offset = 0;
  int32_t  inData_offset = vdata->inMem_size;
  int32_t  outData_offset = inData_offset + vdata->inData_size;
  uint8_t* in_Mem  = &(vdata->theData[inMem_offset]);
  uint8_t* in_Data = &(vdata->theData[inData_offset]);
  uint8_t* out_Data = &(vdata->theData[outData_offset]);

#ifdef HW_VIT
  vitHW_desc[vn].cbps = in_cbps;
  vitHW_desc[vn].ntraceback = in_ntraceback;
  vitHW_desc[vn].data_bits = in_data_bits;

  uint8_t* hwrInMem  = vitHW_li_mem[vn];
  uint8_t* hwrOutMem = vitHW_lo_mem[vn];
  for (int ti = 0; ti < 70; ti ++) {
    hwrInMem[ti] = in_Mem[ti];
  }
  hwrInMem[70] = 0;
  hwrInMem[71] = 0;
  int imi = 72;
  for (int ti = 0; ti < MAX_ENCODED_BITS; ti ++) {
    hwrInMem[imi++] = in_Data[ti];
  }
  for (int ti = 0; ti < (MAX_ENCODED_BITS * 3 / 4); ti ++) {
    out_Data[ti] = 0;
  }

#ifdef INT_TIME
  gettimeofday(&(task_metadata_block->vit_timings.dodec_start), NULL);
#endif
  do_decoding_hw(&(vitHW_fd[vn]), &(vitHW_desc[vn]));

#ifdef INT_TIME
  struct timeval dodec_stop;
  gettimeofday(&(dodec_stop), NULL);
  task_metadata_block->vit_timings.dodec_sec  += dodec_stop.tv_sec  - task_metadata_block->vit_timings.dodec_start.tv_sec;
  task_metadata_block->vit_timings.dodec_usec += dodec_stop.tv_usec - task_metadata_block->vit_timings.dodec_start.tv_usec;
#endif
  // Copy output data from HWR memory to Metadata Block Memory.
  for (int ti = 0; ti < (MAX_ENCODED_BITS * 3 / 4); ti ++) {

    out_Data[ti] = hwrOutMem[ti];
  }
  DEBUG(printf("MB%u at end of HWR VITERBI:\n    out_Data : ", task_metadata_block->block_id);
  for (int ti = 0; ti < 80 /*(MAX_ENCODED_BITS * 3 / 4)*/; ti ++) {
    printf("%u ", out_Data[ti]);
  });

  //TDEBUG
  (printf("MB_THREAD %u calling mark_task_done...\n", task_metadata_block->block_id));
  mark_task_done(task_metadata_block);

#else // HW_VIT
  printf("ERROR : This executable DOES NOT support Viterbi Hardware execution!\n");
  exit(-3);
#endif // HW_VIT
}


void
release_accelerator_for_task(task_metadata_block_t* task_metadata_block)
{
  unsigned mdb_id     = task_metadata_block->block_id;
  unsigned accel_type = task_metadata_block->accelerator_type;
  unsigned accel_id   = task_metadata_block->accelerator_id;
  pthread_mutex_lock(&accel_alloc_mutex);

  //printf("MB%u RELEASE  accelerator %u %u for %d cl %u\n", mdb_id, accel_type, accel_id, accelerator_in_use_by[accel_type][accel_id], task_metadata_block->crit_level);
  DEBUG(printf(" RELEASE accelerator %u  %u  = %d  : ", accel_type, accel_id, accelerator_in_use_by[accel_type][accel_id]);
	  for (int ai = 0; ai < num_accelerators_of_type[fft_hwr_accel_t]; ai++) {
		  printf("%u %d : ", ai, accelerator_in_use_by[accel_type][ai]);
	  }
	  printf("\n"));
  if (accelerator_in_use_by[accel_type][accel_id] != mdb_id) {
    printf("ERROR - in release_accelerator_for_task for ACCEL %s Num %d but BLOCK_ID Mismatch: %d vs %d\n", accel_type_str[accel_type], accel_id, accelerator_in_use_by[accel_type][accel_id], mdb_id);
    printf("  this occurred on finish of block:\n");
    print_base_metadata_block_contents(task_metadata_block);
    printf("Accelerators Info:\n");
    for (int ai = 0; ai < num_accelerators_of_type[fft_hwr_accel_t]; ai++) {
	    printf(" accelerator_in_use_by[ %u ][ %u ] = %d\n", accel_type, ai, accelerator_in_use_by[accel_type][ai]);
    }
  } else {
    accelerator_in_use_by[accel_type][accel_id] = -1; // Indicates "Not in Use"
  }
  pthread_mutex_unlock(&accel_alloc_mutex);
}




#ifdef HW_FFT
#define FFT_HW_THRESHOLD 25    // 75% chance of using HWR
#else
#define FFT_HW_THRESHOLD 101   // 0% chance of using HWR
#endif

#ifdef HW_VIT
#define VITERBI_HW_THRESHOLD 25   // 75% chance to use Viterbi Hardware
#else
#define VITERBI_HW_THRESHOLD 101  // 0% chance to use Viterbi Hardware
#endif

// This is a basic accelerator selection policy:
//   This one selects an accelerator type (HWR or CPU) randomly
//   If an accelerators of that type is not available, it waits until it is.

void
pick_accel_and_wait_for_available(task_metadata_block_t* task_metadata_block)
{
  int proposed_accel = no_accelerator_t;
  int accel_type     = no_accelerator_t;
  int accel_id       = -1;
  switch(task_metadata_block->job_type) {
  case FFT_TASK: {
    // Scheduler should now run this either on CPU or FFT:
    int num = (rand() % (100)); // Return a value from [0,99]
    if (num >= FFT_HW_THRESHOLD) {
      // Execute on hardware
      proposed_accel = fft_hwr_accel_t;
    } else {
      // Execute in CPU (softwware)
      proposed_accel = cpu_accel_t;
    }
  } break;
  case VITERBI_TASK: {
    // Scheduler should now run this either on CPU or VITERBI:
    int num = (rand() % (100)); // Return a value from [0,99]
    if (num >= VITERBI_HW_THRESHOLD) {
      // Execute on hardware
      proposed_accel = vit_hwr_accel_t;
    } else {
      // Execute in CPU (softwware)
      proposed_accel = cpu_accel_t;
    }
  } break;
  default:
    printf("ERROR : request_execution called for unknown task type: %u\n", task_metadata_block->job_type);
    exit(-15);
  }
  // Okay, here we should have a good task to schedule...
  // Creating a "busy spin loop" where we constantly try to allocate
  //  This metablock to an accelerator, until one gets free...
  do {
    int i = 0;
    while ((i < num_accelerators_of_type[proposed_accel]) && (accel_id < 0)) {
      if (accelerator_in_use_by[proposed_accel][i] == -1) { // Not in use -- available
        accel_type = proposed_accel;
        accel_id = i;
      }
      i++;
    }
  } while (accel_type == no_accelerator_t);
  task_metadata_block->accelerator_type = accel_type;
  task_metadata_block->accelerator_id = accel_id;
}


// This is a basic accelerator selection policy:
//   This one selects a hardware (if implemented) and then if none available, 
//   tries for a CPU, and then repeats this scan until one becomes available.
void
fastest_to_slowest_first_available(task_metadata_block_t* task_metadata_block)
{
  int proposed_accel = no_accelerator_t;
  int accel_type     = no_accelerator_t;
  int accel_id       = -1;
  switch(task_metadata_block->job_type) {
  case FFT_TASK: {
    // Scheduler should now run this either on CPU or FFT:
    do {
      int i = 0;
     #ifdef HW_FFT
      proposed_accel = fft_hwr_accel_t;
      while ((i < num_accelerators_of_type[proposed_accel]) && (accel_id < 0)) {
	if (accelerator_in_use_by[proposed_accel][i] == -1) { // Not in use -- available
	  accel_type = proposed_accel;
	  accel_id = i;
	}
	i++;
      } // while (loop through HWR FFT accelerators)
     #endif
      if (accel_id < 0) { // Didn't find one
	i = 0;
	proposed_accel = cpu_accel_t;
	while ((i < num_accelerators_of_type[proposed_accel]) && (accel_id < 0)) {
	  if (accelerator_in_use_by[proposed_accel][i] == -1) { // Not in use -- available
	    accel_type = proposed_accel;
	    accel_id = i;
	  }
	  i++;
	} // while (loop through CPU FFT accelerators)
      } // if (accel_id < 0) 
    } while (accel_type == no_accelerator_t);
  } break;
  case VITERBI_TASK: {
    do {
      int i = 0;
     #ifdef HW_VIT
      proposed_accel = vit_hwr_accel_t;
      while ((i < num_accelerators_of_type[proposed_accel]) && (accel_id < 0)) {
	if (accelerator_in_use_by[proposed_accel][i] == -1) { // Not in use -- available
	  accel_type = proposed_accel;
	  accel_id = i;
	}
	i++;
      } // while (loop through HWR VITERBI accelerators)
     #endif
      if (accel_id < 0) { // Didn't find one
	i = 0;
	proposed_accel = cpu_accel_t;
	while ((i < num_accelerators_of_type[proposed_accel]) && (accel_id < 0)) {
	  if (accelerator_in_use_by[proposed_accel][i] == -1) { // Not in use -- available
	    accel_type = proposed_accel;
	    accel_id = i;
	  }
	  i++;
	} // while (loop through CPU VITERBI accelerators)
      } // if (accel_id < 0) 
    } while (accel_type == no_accelerator_t);
  } break;
 default:
    printf("ERROR : request_execution called for unknown task type: %u\n", task_metadata_block->job_type);
    exit(-15);
  }
  // Okay, here we should have a good task to schedule...
  // Creating a "busy spin loop" where we constantly try to allocate
  //  This metablock to an accelerator, until one gets free...
  do {
    int i = 0;
    while ((i < num_accelerators_of_type[proposed_accel]) && (accel_id < 0)) {
      if (accelerator_in_use_by[proposed_accel][i] == -1) { // Not in use -- available
        accel_type = proposed_accel;
        accel_id = i;
      }
      i++;
    }
  } while (accel_type == no_accelerator_t);
  task_metadata_block->accelerator_type = accel_type;
  task_metadata_block->accelerator_id = accel_id;
}


// This routine selects an available accelerator for the given job, 
//  The accelerator is selected according to a policy
//  The policies are implemented in separate functions.
void
select_target_accelerator(accel_selct_policy_t policy, task_metadata_block_t* task_metadata_block)
{
  switch(policy) { 
  case SELECT_ACCEL_AND_WAIT_POLICY:
    pick_accel_and_wait_for_available(task_metadata_block);
    break;
  case FAST_TO_SLOW_FIRST_AVAIL_POLICY:
    fastest_to_slowest_first_available(task_metadata_block);
    break;
  default:
    printf("ERROR : unknown scheduler accelerator selection policy: %u\n", policy);
    exit(-15);
  }
}



void
request_execution(task_metadata_block_t* task_metadata_block)
{
  task_metadata_block->status = TASK_QUEUED; // queued
  gettimeofday(&task_metadata_block->sched_timings.queued_start, NULL);
  task_metadata_block->sched_timings.get_sec += task_metadata_block->sched_timings.queued_start.tv_sec - task_metadata_block->sched_timings.get_start.tv_sec;
  task_metadata_block->sched_timings.get_usec += task_metadata_block->sched_timings.queued_start.tv_usec - task_metadata_block->sched_timings.get_start.tv_usec;

  // Select the target accelerator to execute the task
  select_target_accelerator(global_scheduler_selection_policy, task_metadata_block);

  unsigned int accel_type = task_metadata_block->accelerator_type;
  unsigned int accel_id = task_metadata_block->accelerator_id;
  if (accel_type < no_accelerator_t) {
    // Mark the requested accelerator as "In-USE" by this metadata block
    if (accelerator_in_use_by[accel_type][accel_id] != -1) {
      printf("ERROR : request_execution is trying to allocate ACCEL %s %u which is already allocated to Block %u\n", accel_type_str[accel_type], accel_id, accelerator_in_use_by[accel_type][accel_id]);
      exit(-14);
    }
    int bi = task_metadata_block->block_id; // short name for the block_id
    accelerator_in_use_by[accel_type][accel_id] = bi;
    //printf("MB%u ALLOCATE accelerator %u %u to  %d cl %u\n", bi, accel_type, accel_id, bi, task_metadata_block->crit_level);
    DEBUG(printf("MB%u ALLOC accelerator %u  %u to %d  : ", bi, accel_type, accel_id, bi);
	    for (int ai = 0; ai < num_accelerators_of_type[fft_hwr_accel_t]; ai++) {
		    printf("%u %d : ", ai, accelerator_in_use_by[accel_type][ai]);
	    }
	    printf("\n"));
    task_metadata_block->status = TASK_RUNNING; // running

    gettimeofday(&master_metadata_pool[bi].sched_timings.running_start, NULL);
    master_metadata_pool[bi].sched_timings.queued_sec += master_metadata_pool[bi].sched_timings.running_start.tv_sec - master_metadata_pool[bi].sched_timings.queued_start.tv_sec;
    master_metadata_pool[bi].sched_timings.queued_usec += master_metadata_pool[bi].sched_timings.running_start.tv_usec - master_metadata_pool[bi].sched_timings.queued_start.tv_usec;

    TDEBUG(printf("Kicking off accelerator task for Metadata Block %u : Task %s %s on Accel %s %u\n", bi, task_job_str[task_metadata_block->job_type], task_criticality_str[task_metadata_block->crit_level], accel_type_str[task_metadata_block->accelerator_type], task_metadata_block->accelerator_id));

    // Lock the mutex associated to the conditional variable
    pthread_mutex_lock(&(task_metadata_block->metadata_mutex));

    // Signal the conditional variable -- triggers the target thread execution of accelerator
    pthread_cond_signal(&(task_metadata_block->metadata_condv));

    // And now we unlock because we are done here...
    pthread_mutex_unlock(&(task_metadata_block->metadata_mutex));

  } else {
    printf("Cannot allocate execution resources for metadata block:\n");
    print_base_metadata_block_contents(task_metadata_block);
  }
}


/********************************************************************************
 * Here are the wait routines -- for critical tasks or all tasks to finish
 ********************************************************************************/
void wait_all_critical()
{
  // Loop through the critical tasks list and check whether they are all in status "done"
  blockid_linked_list_t* cli = critical_live_task_head;
  while (cli != NULL) {
    if (master_metadata_pool[cli->clt_block_id].status != TASK_DONE) {
      // This task is not finished yet.. wait for it
      //  So start polling from the start of the list again.
      cli = critical_live_task_head;
    } else {
      cli = cli->next;
    }
  }
}

void wait_all_tasks_finish()
{
  // Spin loop : check whether all blocks are free...
  printf("Waiting for ALL tasks to finish: free = %u and total = %u\n", free_metadata_blocks, total_metadata_pool_blocks);
  while (free_metadata_blocks != total_metadata_pool_blocks) {
    ; // Nothing really to do, but wait.
  }
}



// This is called at the end of run/life to shut down the scheduler
//  This will also output a bunch of stats abdout timings, etc.

void shutdown_scheduler()
{
  // Kill (cancel) the per-metablock threads
  for (int i = 0; i < total_metadata_pool_blocks; i++) {
    pthread_cancel(metadata_threads[i]);
  }
  // Clean out the pthread mutex and conditional variables
  pthread_mutex_destroy(&free_metadata_mutex);
  pthread_mutex_destroy(&accel_alloc_mutex);
  for (int i = 0; i < total_metadata_pool_blocks; i++) {
	  pthread_mutex_destroy(&(master_metadata_pool[i].metadata_mutex));
	  pthread_cond_destroy(&(master_metadata_pool[i].metadata_condv));
  }

  printf("\nScheduler block allocation/free statistics:\n");
  for (int ti = 0; ti < NUM_JOB_TYPES; ti++) {
    printf("  For %12s Scheduler allocated %9u blocks and freed %9u blocks\n", task_job_str[ti], allocated_metadata_blocks[ti], freed_metadata_blocks[ti]);
  }
  printf(" During FULL run,  Scheduler allocated %9u blocks and freed %9u blocks in total\n", allocated_metadata_blocks[NO_TASK_JOB], freed_metadata_blocks[NO_TASK_JOB]);
  
  printf("\nPer-MetaData-Block Scheduler Timing Data:\n");
  {
    uint64_t total_idle_usec    = 0;
    uint64_t total_get_usec     = 0;
    uint64_t total_queued_usec  = 0;
    uint64_t total_running_usec = 0;
    uint64_t total_done_usec    = 0;
    for (int bi = 0; bi < total_metadata_pool_blocks; bi++) {
      uint64_t this_idle_usec = (uint64_t)(master_metadata_pool[bi].sched_timings.idle_sec) * 1000000 + (uint64_t)(master_metadata_pool[bi].sched_timings.idle_usec);
      uint64_t this_get_usec = (uint64_t)(master_metadata_pool[bi].sched_timings.get_sec) * 1000000 + (uint64_t)(master_metadata_pool[bi].sched_timings.get_usec);
      uint64_t this_queued_usec = (uint64_t)(master_metadata_pool[bi].sched_timings.queued_sec) * 1000000 + (uint64_t)(master_metadata_pool[bi].sched_timings.queued_usec);
      uint64_t this_running_usec = (uint64_t)(master_metadata_pool[bi].sched_timings.running_sec) * 1000000 + (uint64_t)(master_metadata_pool[bi].sched_timings.running_usec);
      uint64_t this_done_usec = (uint64_t)(master_metadata_pool[bi].sched_timings.done_sec) * 1000000 + (uint64_t)(master_metadata_pool[bi].sched_timings.done_usec);
      printf(" Block %3u : IDLE %15lu GET %15lu QUE %15lu RUN %15lu DONE %15lu usec\n", bi, this_idle_usec, this_get_usec, this_queued_usec, this_running_usec,  total_done_usec);

      total_idle_usec    += this_idle_usec;
      total_get_usec     += this_get_usec;
      total_queued_usec  += this_queued_usec;
      total_running_usec += this_running_usec;
      total_done_usec    += this_done_usec;
    }
    double avg;
    printf("\nScheduler Timings: Aggregate Across all Metadata Blocks\n");
    avg = (double)total_idle_usec/(double)total_metadata_pool_blocks;
    printf("  Metablocks_IDLE total run time:    %15lu usec : %16.2lf (average)\n", total_idle_usec, avg);
    avg = (double)total_get_usec/(double)total_metadata_pool_blocks;
    printf("  Metablocks_GET total run time:     %15lu usec : %16.2lf (average)\n", total_get_usec, avg);
    avg = (double)total_queued_usec/(double)total_metadata_pool_blocks;
    printf("  Metablocks_QUEUED total run time:  %15lu usec : %16.2lf (average)\n", total_queued_usec, avg);
    avg = (double)total_running_usec/(double)total_metadata_pool_blocks;
    printf("  Metablocks_RUNNING total run time: %15lu usec : %16.2lf (average)\n", total_running_usec, avg);
    avg = (double)total_done_usec/(double)total_metadata_pool_blocks;
    printf("  Metablocks_DONE total run time:    %15lu usec : %16.2lf (average)\n", total_done_usec, avg);
  }

  printf("\nPer-MetaData-Block Job Timing Data:\n");
  printf("\n  Per-MetaData-Block FFT Timing Data:\n");
  {
    // The FFT Tasks Timing Info
    uint64_t total_calc_usec = 0;
    uint64_t total_fft_usec = 0;
    uint64_t total_fft_br_usec = 0;
    uint64_t total_bitrev_usec = 0;
    uint64_t total_fft_cvtin_usec = 0;
    uint64_t total_fft_cvtout_usec = 0;
    uint64_t total_cdfmcw_usec = 0;
    for (int bi = 0; bi < total_metadata_pool_blocks; bi++) {
      uint64_t this_calc_usec = (uint64_t)(master_metadata_pool[bi].fft_timings.calc_sec) * 1000000 + (uint64_t)(master_metadata_pool[bi].fft_timings.calc_usec);
      uint64_t this_fft_usec = (uint64_t)(master_metadata_pool[bi].fft_timings.fft_sec) * 1000000 + (uint64_t)(master_metadata_pool[bi].fft_timings.fft_usec);
      uint64_t this_fft_br_usec = (uint64_t)(master_metadata_pool[bi].fft_timings.fft_br_sec) * 1000000 + (uint64_t)(master_metadata_pool[bi].fft_timings.fft_br_usec);
      uint64_t this_bitrev_usec = (uint64_t)(master_metadata_pool[bi].fft_timings.bitrev_sec) * 1000000 + (uint64_t)(master_metadata_pool[bi].fft_timings.bitrev_usec);
      uint64_t this_fft_cvtin_usec = (uint64_t)(master_metadata_pool[bi].fft_timings.fft_cvtin_sec) * 1000000 + (uint64_t)(master_metadata_pool[bi].fft_timings.fft_cvtin_usec);
      uint64_t this_fft_cvtout_usec = (uint64_t)(master_metadata_pool[bi].fft_timings.fft_cvtout_sec) * 1000000 + (uint64_t)(master_metadata_pool[bi].fft_timings.fft_cvtout_usec);
      uint64_t this_cdfmcw_usec = (uint64_t)(master_metadata_pool[bi].fft_timings.cdfmcw_sec) * 1000000 + (uint64_t)(master_metadata_pool[bi].fft_timings.cdfmcw_usec);
      printf("Block %3u : FFT calc %15lu fft %15lu fft_br %15lu br %15lu cvtin %15lu cvto %15lu fmcw %15lu usec\n", bi, this_calc_usec, this_fft_usec, this_fft_br_usec, this_bitrev_usec, this_fft_cvtin_usec, this_fft_cvtout_usec, this_cdfmcw_usec);
      total_calc_usec       += this_calc_usec;
      total_fft_usec        += this_fft_usec;
      total_fft_br_usec     += this_fft_br_usec;
      total_bitrev_usec     += this_bitrev_usec;
      total_fft_cvtin_usec  += this_fft_cvtin_usec;
      total_fft_cvtout_usec += this_fft_cvtout_usec;
      total_cdfmcw_usec     += this_cdfmcw_usec;
    }
    
    printf("\nAggregate FFT Tasks Total Timing Data: %u finished FFT tasks\n", freed_metadata_blocks[FFT_TASK]);
    double avg;
    avg = (double)total_calc_usec / (double) freed_metadata_blocks[FFT_TASK];
    printf("     fft-total run time   %15lu usec : %16.3lf average usec\n", total_calc_usec, avg);
    avg = (double)total_fft_br_usec / (double) freed_metadata_blocks[FFT_TASK];
    printf("     bit-reverse run time %15lu usec : %16.3lf average usec\n", total_fft_br_usec, avg);
    avg = (double)total_bitrev_usec / (double) freed_metadata_blocks[FFT_TASK];
    printf("     bit-rev run time     %15lu usec : %16.3lf average usec\n", total_bitrev_usec, avg);
    avg = (double)total_fft_cvtin_usec / (double) freed_metadata_blocks[FFT_TASK];
    printf("     fft_cvtin run time   %15lu usec : %16.3lf average usec\n", total_fft_cvtin_usec, avg);
    avg = (double)total_fft_usec / (double) freed_metadata_blocks[FFT_TASK];
    printf("     fft-comp run time    %15lu usec : %16.3lf average usec\n", total_fft_usec, avg);
    avg = (double)total_fft_cvtout_usec / (double) freed_metadata_blocks[FFT_TASK];
    printf("     fft_cvtout run time  %15lu usec : %16.3lf average usec\n", total_fft_cvtout_usec, avg);
    avg = (double)total_cdfmcw_usec / (double) freed_metadata_blocks[FFT_TASK];
    printf("     calc-dist run time   %15lu usec : %16.3lf average usec\n", total_cdfmcw_usec, avg);
    
    printf("\n  Per-MetaData-Block VITERBI Timing Data: %u finished VITERBI tasks\n", freed_metadata_blocks[VITERBI_TASK]);
    // The Viterbi Task Timing Info
    uint64_t total_depunc_usec = 0;
    uint64_t total_dodec_usec = 0;
    for (int bi = 0; bi < total_metadata_pool_blocks; bi++) {
      uint64_t this_depunc_usec = (uint64_t)(master_metadata_pool[bi].vit_timings.depunc_sec) * 1000000 + (uint64_t)(master_metadata_pool[bi].vit_timings.depunc_usec);
      uint64_t this_dodec_usec = (uint64_t)(master_metadata_pool[bi].vit_timings.dodec_sec) * 1000000 + (uint64_t)(master_metadata_pool[bi].vit_timings.dodec_usec);
      printf("Block %3u : VITERBI depunc %15lu dodecode %15lu usec\n", bi, this_depunc_usec, this_dodec_usec);

      total_depunc_usec += this_depunc_usec;
      total_dodec_usec += this_dodec_usec;
    }
    printf("\nAggregate VITERBI Tasks Total Timing Data:\n");
    avg = (double)total_depunc_usec / (double) freed_metadata_blocks[VITERBI_TASK];
    printf("     depuncture  run time   %15lu usec : %16.3lf average usec\n", total_depunc_usec, avg);
    avg = (double)total_dodec_usec / (double) freed_metadata_blocks[VITERBI_TASK];
    printf("     do-decoding run time   %15lu usec : %16.3lf average usec\n", total_dodec_usec, avg);

  }

  // Clean up any hardware accelerator stuff
 #ifdef HW_VIT
  for (int vi = 0; vi < NUM_VIT_ACCEL; vi++) {
    contig_free(vitHW_mem[vi]);
    close(vitHW_fd[vi]);
  }
#endif

 #ifdef HW_FFT
  for (int fi = 0; fi < NUM_FFT_ACCEL; fi++) {
    contig_free(fftHW_mem[fi]);
    close(fftHW_fd[fi]);
  }
 #endif
}



