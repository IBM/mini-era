#!/bin/bash
####### THESE VARIABLES NEED TO BE SET! #########
export HPVM_DIR=$HOME/work_dir/hpvm-dssoc/hpvm
export APPROXHPVM_DIR=$HOME/work_dir/approxhpvm-nvdla
export RISCV_BIN_DIR=$HOME/work_dir/riscv/bin

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
