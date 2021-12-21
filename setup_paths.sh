#!/bin/bash
####### THESE VARIABLES NEED TO BE SET! #########
export HPVM_DIR=/home/espuser/hpvm-release/hpvm
export APPROXHPVM_DIR=/home/espuser/hpvm-release/approxhpvm-nvdla
export RISCV_BIN_DIR=/home/espuser/riscv/bin
export PYTHONPATH=/home/espuser/mini-era/cv/yolo

####### THESE VARIABLES SHOULD NOT NEED ANY MODIFICATION! #########
SCRIPT=$(readlink -f "$BASH_SOURCE")
SCRIPT_PATH=$(dirname "$SCRIPT")
export MINIERA_DIR=$SCRIPT_PATH
export LLVM_BUILD_DIR=$HPVM_DIR/build
export HPVM_BENCH_DIR=$HPVM_DIR/test/benchmarks
export LLVM_SRC_ROOT=$APPROXHPVM_DIR/llvm
export LLVM_BUILD_ROOT=$APPROXHPVM_DIR/build
#export LIBRARY_PATH=/software/cuda-9.1/lib64/:$LIBRARY_PATH
export LD_LIBRARY_PATH=$LLVM_SRC_ROOT/lib/Transforms/HPVM2NVDLA/nvdla:$LD_LIBRARY_PATH
#export LD_LIBRARY_PATH=$LLVM_SRC_ROOT/lib/Transforms/HPVM2NVDLA/nvdla:/software/cuda-9.1/lib64/:$LD_LIBRARY_PATH
export PATH=$HPVM_DIR/build/bin:$PATH
export TOP=$MINIERA_DIR/sw/umd
export LD_LIBRARY_PATH=/home/espuser/anaconda3/lib:$LD_LIBRARY_PATH
