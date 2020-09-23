//============================================================================//
//    H.264 High Level Synthesis Benchmark
//    Copyright (c) <2016>
//    <University of Illinois at Urbana-Champaign>
//    All rights reserved.
//
//    Developed by:
//
//    <ES CAD Group>
//        <University of Illinois at Urbana-Champaign>
//        <http://dchen.ece.illinois.edu/>
//
//    <Hardware Research Group>
//        <Advanced Digital Sciences Center>
//        <http://adsc.illinois.edu/>
//============================================================================//

#include <stdint.h>
#include <sys/time.h>

#ifdef INT_TIME
struct timeval h_decmain_stop, h_decmain_start;
uint64_t h_decmain_sec  = 0LL;
uint64_t h_decmain_usec = 0LL;

struct timeval h_dcase7_stop, h_dcase7_start;
uint64_t h_dcase7_sec  = 0LL;
uint64_t h_dcase7_usec = 0LL;

struct timeval h_dcase8_stop, h_dcase8_start;
uint64_t h_dcase8_sec  = 0LL;
uint64_t h_dcase8_usec = 0LL;

struct timeval h_dcase51_stop, h_dcase51_start;
uint64_t h_dcase51_sec  = 0LL;
uint64_t h_dcase51_usec = 0LL;

struct timeval h_dProcSH_stop, h_dProcSH_start;
uint64_t h_dProcSH_sec  = 0LL;
uint64_t h_dProcSH_usec = 0LL;

struct timeval h_dSlHdr_stop, h_dSlHdr_start;
uint64_t h_dSlHdr_sec  = 0LL;
uint64_t h_dSlHdr_usec = 0LL;

struct timeval h_dProcSl_stop, h_dProcSl_start;
uint64_t h_dProcSl_sec  = 0LL;
uint64_t h_dProcSl_usec = 0LL;

#endif

#include "global.h"
#include "parset.h"
#include "decode.h"
#include "slice.h"



void decode_main(NALU_t* nalu, StorablePicture pic[MAX_REFERENCE_PICTURES], StorablePictureInfo pic_info[MAX_REFERENCE_PICTURES])
{
  //#pragma HLS dataflow
  extern seq_parameter_set_rbsp_t SPS_GLOBAL;
  extern pic_parameter_set_rbsp_t PPS_GLOBAL;
  extern ImageParameters img_inst;
  extern slice_header_rbsp_t sliceHeader_inst;
  extern char intra_pred_mode[PicWidthInMBs*4][FrameHeightInMbs*4];
  extern unsigned char nz_coeff_luma[PicWidthInMBs*4][FrameHeightInMbs*4];
  extern unsigned char nz_coeff_chroma[2][PicWidthInMBs*2][FrameHeightInMbs*2];
  extern unsigned char Mb_prediction_type[PicWidthInMBs][FrameHeightInMbs];

#if _N_HLS_
  fprintf(trace_bit,"reading NAL unit:length %d\t refidc %d\t type %d\n",nalu->len, nalu->nal_reference_idc,nalu->nal_unit_type);
#endif // _N_HLS_

 #ifdef INT_TIME
  gettimeofday(&h_decmain_start, NULL);
 #endif
  switch(nalu->nal_unit_type)
  {
    case 7:
      ProcessSPS(&SPS_GLOBAL, nalu);
#if _N_HLS_
      fprintf(trace_bit,"\n");
#endif // _N_HLS_
     #ifdef INT_TIME
      gettimeofday(&h_dcase7_stop, NULL);
      h_dcase7_sec  += h_dcase7_stop.tv_sec  - h_decmain_start.tv_sec;
      h_dcase7_usec += h_dcase7_stop.tv_usec - h_decmain_start.tv_usec;
     #endif
      break;
    case 8:
      ProcessPPS(&PPS_GLOBAL,nalu);
#if _N_HLS_
      fprintf(trace_bit,"\n");
#endif // _N_HLS_
     #ifdef INT_TIME
      gettimeofday(&h_dcase8_stop, NULL);
      h_dcase8_sec  += h_dcase8_stop.tv_sec  - h_decmain_start.tv_sec;
      h_dcase8_usec += h_dcase8_stop.tv_usec - h_decmain_start.tv_usec;
     #endif
      break;
    case 5:
    case 1:
      ProcessSH(&SPS_GLOBAL,&PPS_GLOBAL,&sliceHeader_inst,&img_inst,nalu);
     #ifdef INT_TIME
      gettimeofday(&h_dProcSH_stop, NULL);
      h_dProcSH_sec  += h_dProcSH_stop.tv_sec  - h_decmain_start.tv_sec;
      h_dProcSH_usec += h_dProcSH_stop.tv_usec - h_decmain_start.tv_usec;
     #endif
      intepretSLiceHeader(&sliceHeader_inst,&SPS_GLOBAL,&PPS_GLOBAL,nalu->nal_unit_type,nalu->nal_reference_idc,&img_inst,pic);
     #ifdef INT_TIME
      gettimeofday(&h_dSlHdr_stop, NULL);
      h_dSlHdr_sec  += h_dSlHdr_stop.tv_sec  - h_dProcSH_stop.tv_sec;
      h_dSlHdr_usec += h_dSlHdr_stop.tv_usec - h_dProcSH_stop.tv_usec;
     #endif

      ProcessSlice
        (
         nalu,
         pic,
         pic_info,
         Mb_prediction_type,
         intra_pred_mode,
         nz_coeff_luma,
         nz_coeff_chroma,
         PPS_GLOBAL.constrained_intra_pred_flag,
         &sliceHeader_inst,
         &img_inst
        );
     #ifdef INT_TIME
      gettimeofday(&h_dProcSl_stop, NULL);
      h_dProcSl_sec  += h_dProcSl_stop.tv_sec  - h_dProcSH_stop.tv_sec;
      h_dProcSl_usec += h_dProcSl_stop.tv_usec - h_dProcSH_stop.tv_usec;
     #endif


#if _N_HLS_
      fprintf(trace_bit,"\n");
#endif // _N_HLS_
     #ifdef INT_TIME
      gettimeofday(&h_dcase51_stop, NULL);
      h_dcase51_sec  += h_dcase51_stop.tv_sec  - h_decmain_start.tv_sec;
      h_dcase51_usec += h_dcase51_stop.tv_usec - h_decmain_start.tv_usec;
     #endif
      break;
    default:
      break;
  }
 #ifdef INT_TIME
  gettimeofday(&h_decmain_stop, NULL);
  h_decmain_sec  += h_decmain_stop.tv_sec  - h_decmain_start.tv_sec;
  h_decmain_usec += h_decmain_stop.tv_usec - h_decmain_start.tv_usec;
 #endif
}
