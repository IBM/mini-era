#!/bin/bash
./erav3c  -m0 > data/Results/ttt3m0
./erav3c  -m1 > data/Results/ttt3m1

grep xmit_out_res data/Results/ttt3m0  > data/Results/ttt3m0_xmit_out_res.raw 
grep xmit_out_res data/Results/ttt3m1  > data/Results/ttt3m1_xmit_out_res.raw 

perl data/Results/threshold_compare_final_results.pl data/Results/grm0_xmit_out_res.awkd data/Results/ttt3m0_xmit_out_res.raw 0.000001 
perl data/Results/threshold_compare_final_results.pl data/Results/grm1_xmit_out_res.awkd data/Results/ttt3m1_xmit_out_res.raw 0.000001 
