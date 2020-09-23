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

#include <string.h>
#include <stdint.h>
#include <sys/time.h>

#include "global.h"
#include "nalu.h"
#include "decode.h"

#ifdef DEBUG_MODE
#define DEBUG(x) x
#else
#define DEBUG(x)
#endif

#ifdef INT_TIME
struct timeval h_decode_stop, h_decode_start;
uint64_t h_decode_sec  = 0LL;
uint64_t h_decode_usec = 0LL;
#endif


#if _N_HLS_
extern FILE *prediction_test;
extern FILE* construction_test;
extern FILE *trace_bit;
extern FILE* debug_test;
#endif

FILE *bitstr;
char AnnexbFileName[100];
char* h264_gold_results_file;
FILE *h264_test_results_out;
StorablePicture Pic[MAX_REFERENCE_PICTURES];
StorablePictureInfo Pic_info[MAX_REFERENCE_PICTURES];

// Give a couple different possible outputs
char* frame_image0 = (char*)0xfeedbeef;
char* frame_image1 = (char*)0xdeadf00d;

unsigned do_h264_decode_invokes = 0;

NALU_t PINGPONGbuffer[MAX_H264_DECODE_PASSES];

void init_h264_decode(int argc, char **argv)
{
  if(argc != 3)
  {
    puts("Too much argument!");
    exit(-1);
  }


  if(argc == 3)
  {
    sprintf(AnnexbFileName, "%s", argv[1]);
  }
  else
    strcpy(AnnexbFileName,"input/test.264");


  h264_gold_results_file = argv[2];
  
  bitstr=fopen(AnnexbFileName,"rb");
  if(bitstr==NULL)
  {
    puts("cannot find the corresponding file.");
    exit(-1);
  }

#if _N_HLS_
  prediction_test=fopen("pred_test.txt","w");
  construction_test=fopen("construc_test.txt","w");
  trace_bit=fopen("trace.txt","w");
  debug_test=fopen("debug_test.txt","w");
#endif


  h264_test_results_out=fopen("testresult.yuv","wb");
  if (h264_test_results_out == NULL) {
    printf("ERROR: Cannot open output results testing file...\n");
  }

  for (int i = 0; i < MAX_H264_DECODE_PASSES; i++) { 
    PINGPONGbuffer[i].nal_unit_type=0;
    DEBUG(printf(" initializing PPB %u\n", i));
    if (GetAnnexbNALU (&PINGPONGbuffer[i], bitstr)==0) {
      printf("ERROR : Only got to %u out of %u H264_DECODE_PASSES\n", i, MAX_H264_DECODE_PASSES);
      exit(-1);
    }

    PINGPONGbuffer[i].len = EBSPtoRBSP (PINGPONGbuffer[i].buf, PINGPONGbuffer[i].len, 1);
    RBSPtoSODB(&PINGPONGbuffer[i],PINGPONGbuffer[i].len-1);
    DEBUG(printf("  On pass %u : PPB.len = %u\n", i, PINGPONGbuffer[i].len));
  }


#if _N_HLS_
  fclose(prediction_test);
  fclose(construction_test);
  fclose(trace_bit);
  fclose(debug_test);
#endif
  fclose(bitstr);
}


char* do_h264_decode()
{
 #ifdef INT_TIME
  gettimeofday(&h_decode_start, NULL);
 #endif
  memset(Pic, 0, MAX_REFERENCE_PICTURES*sizeof(StorablePicture));
  memset(Pic_info, 0, MAX_REFERENCE_PICTURES*sizeof(StorablePictureInfo));

  DEBUG(printf("In do_h264_decode invocation %u\n", do_h264_decode_invokes));

  //int poc = 0;
  for (int pass = 0; pass < MAX_H264_DECODE_PASSES; pass++) {
    DEBUG(printf(" do_h264_decode invocation %u pass %u\n", do_h264_decode_invokes, pass));

    decode_main(&PINGPONGbuffer[pass],Pic,Pic_info);

    // Checkif we found a "Picture" (frame)
    if (PINGPONGbuffer[pass].nal_unit_type==5 || PINGPONGbuffer[pass].nal_unit_type==1 ) {
      DEBUG(printf("  SHOULD have a frame picture -- can exit decode loop now on pass %u\n", pass));
      /* printf("  ...Checking for writing...\n"); */
      /* for(int j=0;j<MAX_REFERENCE_PICTURES;j++) { */
      /* 	for(int i=0;i<MAX_REFERENCE_PICTURES;i++) { */
      /* 	  printf("    for j %u PIC %u memocc %u order_num %u\n", j, i, Pic[i].memoccupied, Pic[i].Picorder_num); */
      /* 	  if(Pic[i].memoccupied && Pic[i].Picorder_num==poc) { */
      /* 	    //write_out_pic(&Pic[i], h264_test_results_out); */
      /* 	    printf("    FOUND frame picture -- should be done at pass %u\n", pass);	     */
      /* 	    poc++; */
      do_h264_decode_invokes++;
      return frame_image0;
      /*     //return &Pic[i]; */
      /*   } */
      /*  } */
      /* } */
    }
  }
  do_h264_decode_invokes++;
  printf("Note: do_decode hit the fall-through case (probably undesired behavior)\n");
 #ifdef INT_TIME
  gettimeofday(&h_decode_stop, NULL);
  h_decode_sec  += h_decode_stop.tv_sec  - h_decode_start.tv_sec;
  h_decode_usec += h_decode_stop.tv_usec - h_decode_start.tv_usec;
 #endif
  return frame_image1;
}

void write_out_pic(StorablePicture *pic,FILE * p_out)
{
  int i,j;

  for(i=0;i<FrameHeightInSampleL;i++)
    for(j=0; j<PicWidthInSamplesL; j++)
    {
      fputc(pic->Sluma[j][i],p_out);
    }
  for(i=0;i<FrameHeightInSampleC;i++)
    for(j=0; j<PicWidthInSamplesC; j++)
    {
      fputc(pic->SChroma_0[j][i],p_out);
    }
  for(i=0;i<FrameHeightInSampleC;i++)
    for(j=0 ;j <PicWidthInSamplesC ;  j++)
    {
      fputc(pic->SChroma_1[j][i],p_out);
    }

}


void do_post_h264_decode()
{
  DEBUG(printf("In do_post_h264_decode...\n"));
  /**
  int poc = 1;

    if (PINGPONGbuffer[pass].nal_unit_type==5 || PINGPONGbuffer[pass].nal_unit_type==1 ) {
      printf(" ...Checking for writing...\n");
      for(int j=0;j<MAX_REFERENCE_PICTURES;j++) {
        for(int i=0;i<MAX_REFERENCE_PICTURES;i++) {
	  printf("j %u : PIC %u memocc %u order_num %u\n", j, i, Pic[i].memoccupied, Pic[i].Picorder_num);
          if(Pic[i].memoccupied && Pic[i].Picorder_num==poc) {
            write_out_pic(&Pic[i], h264_test_results_out);
            printf("writing %d\n",poc );
            poc++;
          }
        }
      }
    }
  **/
}

void do_closeout_h264_decode()
{
  DEBUG(printf("In do_closeout_h264_decode...\n"));
  /*
  int poc = 0;

    if (PINGPONGbuffer[pass].nal_unit_type==5 || PINGPONGbuffer[pass].nal_unit_type==1 ) {
      printf("  ...Checking for writing...\n");
      for(int j=0;j<MAX_REFERENCE_PICTURES;j++) {
        for(int i=0;i<MAX_REFERENCE_PICTURES;i++) {
	  printf("j %u PIC %u memocc %u order_num %u\n", j, i, Pic[i].memoccupied, Pic[i].Picorder_num);
          if(Pic[i].memoccupied && Pic[i].Picorder_num==poc) {
            write_out_pic(&Pic[i], h264_test_results_out);
            printf("   writing %d\n",poc );
            poc++;
          }
        }
      }
    }
  */
  fclose(h264_test_results_out);
  /*
  sprintf(AnnexbFileName,"diff -q testresult.yuv %s", h264_gold_results_file);

  if (!system(AnnexbFileName))
    printf("PASSED\n");
  else
    printf("FAILED\n");
  */
}
