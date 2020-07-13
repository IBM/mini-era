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

#include <fcntl.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/time.h>


#include "contig.h"
#include "get_init_time.h"
#include "getopt.h"


typedef enum {false, true} bool_t;

uint64_t accel_sec  = 0;
uint64_t accel_usec = 0;

#define ACCEL_DEVNAME  "/dev/inittime.0"

int accelHW_fd;
contig_handle_t accelHW_mem;

accelHW_token_t* accelHW_lmem;  // Pointer to local version (mapping) of accelHW_mem
accelHW_token_t* accelHW_li_mem; // Pointer to input memory block
accelHW_token_t* accelHW_lo_mem; // Pointer to output memory block
size_t accelHW_in_words_adj;
size_t accelHW_out_words_adj;
size_t accelHW_in_len;
size_t accelHW_out_len;
size_t accelHW_in_size;
size_t accelHW_out_size;
size_t accelHW_out_offset;
size_t accelHW_size;

struct accelHW_access accelHW_desc;

static unsigned DMA_WORD_PER_BEAT(unsigned _st)
{
        return (sizeof(void *) / _st);
}


static void init_accel_parameters(unsigned len)
{
  if (DMA_WORD_PER_BEAT(sizeof(accelHW_token_t)) == 0) {
    accelHW_in_words_adj  = len;
    accelHW_out_words_adj = len;
  } else {
    accelHW_in_words_adj = round_up(len, DMA_WORD_PER_BEAT(sizeof(accelHW_token_t)));
    accelHW_out_words_adj = round_up(len, DMA_WORD_PER_BEAT(sizeof(accelHW_token_t)));
  }
  accelHW_in_len = accelHW_in_words_adj;
  accelHW_out_len =  accelHW_out_words_adj;
  accelHW_in_size = accelHW_in_len * sizeof(accelHW_token_t);
  accelHW_out_size = accelHW_out_len * sizeof(accelHW_token_t);
  accelHW_out_offset = 0;
  accelHW_size = (accelHW_out_offset * sizeof(accelHW_token_t)) + accelHW_out_size;
}

static void call_into_hw(int *fd, struct accelHW_access *desc)
{
  //contig_copy_to(*mem, 0, inMemory, size);

  if (ioctl(*fd, ACCELHW_IOC_ACCESS, *desc)) {
    perror("IOCTL:");
    exit(EXIT_FAILURE);
  }

  //contig_copy_from(inMemory, *mem, 0, out_size);
}


void show_usage(char * pname) {
  printf("Usage: %s <OPTIONS>\n", pname);
  printf(" OPTIONS:\n");
  printf("    -h        : print this helpful usage info\n");
  printf("    -r <N>    : Repeat call <N> times.\n");
  printf("    -M <N>    : Use a mem_size of <N> ( 0 to 131072)\n");
}


	 
int main(int argc, char *argv[])
{
  unsigned repeats = 1;
  unsigned mem_size = 131072;

  int opt;
  // put ':' in the starting of the
  // string so that program can
  // distinguish between '?' and ':'
  while((opt = getopt(argc, argv, ":hr:M:")) != -1) {
    switch(opt) {
    case 'h':
      show_usage(argv[0]);
      exit(0);
    case 'r':
      repeats = atoi(optarg);
      break;
    case 'M':
      mem_size = atoi(optarg);
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

  if (repeats < 1) {
    printf("ERROR : Repeats must be apositive integer\n");
    show_usage(argv[0]);
    exit(-1);
  }
    
  if (mem_size > (128*1024)) {
    printf("ERROR : Max mem_size is %u (you specified %u)\n", 128*1024, mem_size);
    show_usage(argv[0]);
    exit(-1);
  }

  printf("Running %s :\n", argv[0]);
  printf("  mem_size = %u and repeats = %u\n", mem_size, repeats);

  init_accel_parameters(mem_size);

  accelHW_fd = open(ACCEL_DEVNAME, O_RDWR, 0);
  if (accelHW_fd < 0) {
    fprintf(stderr, "Error: cannot open %s", ACCEL_DEVNAME);
    exit(EXIT_FAILURE);
  }

  printf("Allocate hardware buffer of size %zu\n", accelHW_size);
  accelHW_lmem = contig_alloc(accelHW_size, &accelHW_mem);
  if (accelHW_lmem == NULL) {
    fprintf(stderr, "Error: cannot allocate %zu contig bytes", accelHW_size);
    exit(EXIT_FAILURE);
  }

  accelHW_li_mem = &(accelHW_lmem[0]);
  accelHW_lo_mem = &(accelHW_lmem[accelHW_out_offset]);
  printf("Set accelHW_li_mem = %p  AND accelHW_lo_mem = %p\n", accelHW_li_mem, accelHW_lo_mem);

  accelHW_desc.esp.run = true;
  accelHW_desc.esp.coherence = ACC_COH_NONE;
  accelHW_desc.esp.p2p_store = 0;
  accelHW_desc.esp.p2p_nsrcs = 0;
  //accelHW_desc.esp.p2p_srcs = {"", "", "", ""};
  accelHW_desc.esp.contig = contig_to_khandle(accelHW_mem);

  accelHW_desc.mem_size   = mem_size;
  accelHW_desc.src_offset = 0;
  accelHW_desc.dst_offset = 0;

  struct timeval accel_stop, accel_start;

  for (int ri = 0; ri < repeats; ri++) {
    gettimeofday(&accel_start, NULL);
    call_into_hw(&accelHW_fd, &accelHW_desc);
    gettimeofday(&accel_stop, NULL);
    accel_sec  += accel_stop.tv_sec  - accel_start.tv_sec;
    accel_usec += accel_stop.tv_usec - accel_start.tv_usec;
  }

  uint64_t total_time = (uint64_t) (accel_sec * 1000000) + (uint64_t) (accel_usec);
  printf("  accel_time = %lu usec (for %u repeats with %u mem_size)\n", total_time, repeats, mem_size);
  printf("\nDone.\n");

  contig_free(accelHW_mem);
  close(accelHW_fd);
  return 0;
}
