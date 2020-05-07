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

#define total_metadata_pool_blocks  32
task_metadata_block_t master_metadata_pool[total_metadata_pool_blocks];
int free_metadata_pool[total_metadata_pool_blocks];
int free_metadata_blocks = total_metadata_pool_blocks;

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

void print_base_metadata_block_contents(task_metadata_block_t* mb)
{
  printf("metadata_block_id = %d @ %p\n", mb->metadata.metadata_block_id, mb);
  unsigned status = mb->metadata.status;
  if (status < NUM_TASK_STATUS) {
    printf(" ** status = %s\n", task_status_str[status]);
  } else {
    printf(" ** status = %d <= NOT a legal value!\n",  mb->metadata.status);
  }
  unsigned job_type = mb->metadata.job_type;
  if (job_type < NUM_JOB_TYPES) {
    printf("    job_type = %s\n", task_job_str[job_type]);
  } else {
    printf(" ** job_type = %d <= NOT a legal value!\n", mb->metadata.job_type);
  }
  unsigned crit_level = mb->metadata.crit_level;
  if (crit_level < NUM_TASK_CRIT_LEVELS) {
    printf("    crit_level = %s\n",  task_criticality_str[crit_level]);
  } else {
    printf(" ** crit_level = %d <= NOT a legal value!\n",  mb->metadata.crit_level);
  }
  printf("    data_size  = %d\n",  mb->metadata.data_size);
  printf("    data @ %p\n", mb->metadata.data);
}

void print_fft_metadata_block_contents(task_metadata_block_t* mb) {
  print_base_metadata_block_contents(mb);
}

void print_viterbi_metadata_block_contents(task_metadata_block_t* mb)
{  
  print_base_metadata_block_contents(mb);
  viterbi_data_struct_t* vdata = (viterbi_data_struct_t*)(mb->metadata.data);
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
  printf("in get_task_metadata_block with %u free_metadata_blocks\n", free_metadata_blocks);
  if (free_metadata_blocks < 1) {
    // Out of metadata blocks -- all in use, cannot enqueue new tasks!
    return NULL;
  }
  int bi = free_metadata_pool[free_metadata_blocks - 1];
  free_metadata_pool[free_metadata_blocks - 1] = -1;
  free_metadata_blocks -= 1;
  // For neatness (not "security") we'll clear the meta-data in the block (not the data data,though)
  master_metadata_pool[bi].metadata.job_type = task_type;
  master_metadata_pool[bi].metadata.status = TASK_ALLOCATED;
  master_metadata_pool[bi].metadata.crit_level = crit_level;
  master_metadata_pool[bi].metadata.data_size = 0;
  if (crit_level > 1) { // is this a "critical task"
    /* int ci = total_critical_tasks; // Set index for crit_task block_id in pool */
    /* critical_live_tasks_list[ci].clt_block_id = bi;  // Set the critical task block_id indication */
    // Select the next available critical-live-task-list entry ID 
    int li = free_critlist_pool[free_critlist_entries - 1];
    free_critlist_pool[free_critlist_entries - 1] = -1; // clear it a(as it is no longer free)
    free_critlist_entries -= 1;
    // Now li indicates the critical_live_tasks_list[] index to use
    // Now set up the revisions to the critical live tasks list
    critical_live_tasks_list[li].clt_block_id = bi;   // point this entry to the master_metatdata_pool block id
    critical_live_tasks_list[li].next = critical_live_task_head;     // Insert as head of critical tasks list
    critical_live_task_head = &(critical_live_tasks_list[li]);
    total_critical_tasks += 1;
  }
  printf("  returning block %u\n", bi);
  print_critical_task_list_ids();
  return &(master_metadata_pool[bi]);
}





void free_task_metadata_block(task_metadata_block_t* mb)
{
  int bi = mb->metadata.metadata_block_id;
  printf("in free_task_metadata_block for block %u with %u free_metadata_blocks\n", bi, free_metadata_blocks);
  if (free_metadata_blocks < total_metadata_pool_blocks) {
    free_metadata_pool[free_metadata_blocks] = bi;
    free_metadata_blocks += 1;
    if (master_metadata_pool[bi].metadata.crit_level > 1) { // is this a critical tasks?
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
      free_critlist_pool[free_critlist_entries - 1] = cti; // Enable this crit-list entry for new use
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
    // For neatness (not "security") we'll clear the meta-data in the block (not the data data,though)
    master_metadata_pool[bi].metadata.job_type = NO_TASK_JOB; // unset
    master_metadata_pool[bi].metadata.status = TASK_FREE;   // free
    master_metadata_pool[bi].metadata.crit_level = NO_TASK; // lowest/free?
    master_metadata_pool[bi].metadata.data_size = 0;
  } else {
    printf("ERROR : We are freeing a metadata block when we already have max metadata blocks free...\n");
    printf("   THE FREE Metadata Blocks list:\n");
    for (int ii = 0; ii < free_metadata_blocks; ii++) {
      printf("        free[%2u] = %u\n", ii, free_metadata_pool[ii]);
    }
    printf("    THE Being-Freed Metat-Data Block:\n");
    print_base_metadata_block_contents(mb);
    exit(-5);
  }
}

int
get_task_status(int task_id) {
  return master_metadata_pool[task_id].metadata.status;
}

static unsigned DMA_WORD_PER_BEAT(unsigned _st)
{
        return (sizeof(void *) / _st);
}

#ifdef HW_VIT
// These are Viterbi Harware Accelerator Variables, etc.
#define  NUM_VIT_ACCEL  4
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

#ifdef HW_FFT
// These are FFT Hardware Accelerator Variables, etc.
#define NUM_FFT_ACCEL 4
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

status_t initialize_scheduler()
{
	DEBUG(printf("In initialize...\n"));
	for (int i = 0; i < total_metadata_pool_blocks; i++) {
	  master_metadata_pool[i].metadata.metadata_block_id = i; // Set the master pool's block_ids
	  free_metadata_pool[i] = i;     // Set up all blocks are free
	  free_critlist_pool[i] = -1;    // Set all critlist entries are unallocated
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
execute_hwr_fft_accelerator(int fn, task_metadata_block_t* task_metadata_block)
{
  DEBUG(printf("In execute_hwr_fft_accelerator: MB %d  CL %d\n", task_metadata_block->metadata.metadata_block_id, task_metadata_block->metadata.crit_level ));
#ifdef HW_FFT
  float * data = (float*)(task_metadata_block->metadata.data);
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

  task_metadata_block->metadata.status = TASK_DONE; // done

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


#ifdef INT_TIME
extern struct timeval dodec_stop, dodec_start;
extern uint64_t dodec_sec;
extern uint64_t dodec_usec;
#endif

void
execute_hwr_viterbi_accelerator(int vn, task_metadata_block_t* task_metadata_block)
//int n_cbps, int n_traceback, int n_data_bits, uint8_t* inMem, uint8_t* inDat, uint8_t* outData)
{
  DEBUG(printf("In execute_hwr_viterbi_accelerator\n"));
  viterbi_data_struct_t* vdata = (viterbi_data_struct_t*)(task_metadata_block->metadata.data);
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
  gettimeofday(&dodec_start, NULL);
#endif
  do_decoding_hw(&(vitHW_fd[vn]), &(vitHW_desc[vn]));

#ifdef INT_TIME
  gettimeofday(&dodec_stop, NULL);
  dodec_sec  += dodec_stop.tv_sec  - dodec_start.tv_sec;
  dodec_usec += dodec_stop.tv_usec - dodec_start.tv_usec;
#endif

  task_metadata_block->metadata.status = TASK_DONE; // done

#else // HW_VIT
  printf("ERROR : This executable DOES NOT support Viterbi Hardware execution!\n");
  exit(-3);
#endif // HW_VIT
}


void shutdown_scheduler()
{
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

/* void */
/* schedule_fft(task_metadata_block_t* task_metadata_block) */
/* { */
/*   // Scheduler should now run this either on CPU or FFT: */
/*   int num = (rand() % (100)); // Return a value from [0,99] */
/*   if (num >= FFT_HW_THRESHOLD) { */
/*     // Execute on hardware */
/*     printf("SCHED: executing FFT on Hardware : %u > %u\n", num, FFT_HW_THRESHOLD); */
/*     execute_hwr_fft_accelerator(0, data);  // only using FFT HW 0 for now -- still blocking, too */
/*   } else { */
/*     // Execute in CPU (softwware) */
/*     printf("SCHED: executing FFT on CPU Software : %u < %u\n", num, FFT_HW_THRESHOLD); */
/*     execute_cpu_fft_accelerator(data); */
/*   } */
/* } */

/* void */
/* schedule_viterbi(int n_data_bits, int n_cbps, int n_traceback, uint8_t* inMem, uint8_t* inData, uint8_t* outData) */
/* { */
/*   // Scheduler should now run this either on CPU or VITERBI: */
/*   int num = (rand() % (100)); // Return a value from [0,99] */
/*   if (num >= VITERBI_HW_THRESHOLD) { */
/*     // Execute on hardware */
/*     printf("SCHED: executing VITERBI on Hardware : %u > %u\n", num, VITERBI_HW_THRESHOLD); */
/*     execute_hwr_viterbi_accelerator(0, n_cbps, n_traceback, n_data_bits, inMem, inData, outData);  // only using VITERBI HW 0 for now -- still blocking, too */
/*   } else { */
/*     // Execute in CPU (softwware) */
/*     printf("SCHED: executing VITERBI on CPU Software : %u < %u\n", num, VITERBI_HW_THRESHOLD); */
/*     execute_cpu_viterbi_accelerator(n_cbps, n_traceback, n_data_bits, inMem, inData, outData);  */
/*   } */
/* } */




void
request_execution(task_metadata_block_t* task_metadata_block)
{
  task_metadata_block->metadata.status = TASK_QUEUED; // queued
  switch(task_metadata_block->metadata.job_type) {
  case FFT_TASK:
    {
      // Scheduler should now run this either on CPU or FFT:
      task_metadata_block->metadata.status = TASK_RUNNING; // running
      int num = (rand() % (100)); // Return a value from [0,99]
      if (num >= FFT_HW_THRESHOLD) {
	// Execute on hardware
	printf("SCHED: executing FFT on Hardware : %u > %u\n", num, FFT_HW_THRESHOLD);
	execute_hwr_fft_accelerator(0, task_metadata_block); // data);  // only using FFT HW 0 for now -- still blocking, too
      } else {
	// Execute in CPU (softwware)
	printf("SCHED: executing FFT on CPU Software : %u < %u\n", num, FFT_HW_THRESHOLD);
	execute_cpu_fft_accelerator(task_metadata_block); // data);
      }
      //}
    }
    break;
  case VITERBI_TASK:
    {
      // Scheduler should now run this either on CPU or VITERBI:
      task_metadata_block->metadata.status = TASK_RUNNING; // running
      int num = (rand() % (100)); // Return a value from [0,99]
      if (num >= VITERBI_HW_THRESHOLD) {
	// Execute on hardware
	printf("SCHED: executing VITERBI on Hardware : %u > %u\n", num, VITERBI_HW_THRESHOLD);
	/* viterbi_data_struct_t* vdata = (viterbi_data_struct_t*)(task_metadata_block->metadata.data); */
	/* int32_t  in_ncbps = vdata->n_cbps; */
	/* int32_t  in_ntraceback = vdata->n_traceback; */
	/* int32_t  in_ndata_bits = vdata->n_data_bits; */
	/* int32_t  inMem_offset = 0; */
	/* int32_t  inData_offset = vdata->inMem_size; */
	/* int32_t  outData_offset = inData_offset + vdata->inData_size; */
	/* uint8_t* in_Mem  = &(vdata->theData[inMem_offset]); */
	/* uint8_t* in_Data = &(vdata->theData[inData_offset]); */
	/* uint8_t* out_Data = &(vdata->theData[outData_offset]); */
	/* execute_hwr_viterbi_accelerator(0, in_ncbps, in_ntraceback, in_ndata_bits, in_Mem, in_Data, out_Data);  // only using VITERBI HW 0 for now -- still blocking, too */
	execute_hwr_viterbi_accelerator(0, task_metadata_block); // only using VITERBI HW 0 for now -- still blocking, too
      } else {
	// Execute in CPU (softwware)
	printf("SCHED: executing VITERBI on CPU Software : %u < %u\n", num, VITERBI_HW_THRESHOLD);
	execute_cpu_viterbi_accelerator(task_metadata_block);
      }
    }
    break;
  default:
    printf("ERROR : request_execution called for unknown task type: %u\n", task_metadata_block->metadata.job_type);
  }


  /* // For now, since this is blocking, etc. we can return the MetaData Block here */
  /* //   In reality, this should happen when we detect a task has non-blockingly-finished... */
  /* free_task_metadata_block(task_metadata_block); */
}



void wait_all_critical()
{
  // Loop through the critical tasks list and check whether they are all in status "done"
  
}
