#!/bin/bash

module load cuda-toolkit/9.1
export CUDA_INCLUDE_PATH=/software/cuda-9.1/include
export CUDNN_PATH=/software/cuda-9.1/lib64/
export LLVM_SRC_ROOT=$APPROXHPVM_DIR/llvm
export LLVM_BUILD_ROOT=$APPROXHPVM_DIR/build
export LIBRARY_PATH=/software/cuda-9.1/lib64/:$LIBRARY_PATH
export LD_LIBRARY_PATH=$LLVM_SRC_ROOT/lib/Transforms/HPVM2NVDLA/nvdla:/software/cuda-9.1/lib64/:$LD_LIBRARY_PATH
