# This Makefile compiles the HPVM-CAVA pilot project. 
# It builds HPVM-related dependencies, then the native camera pipeline ISP code.
#
# Paths to some dependencies (e.g., HPVM, LLVM) must exist in Makefile.config,
# which can be copied from Makefile.config.example for a start.

CONFIG_FILE := /home/aejjeh/work_dir/hpvm-dssoc/hpvm/test/benchmarks/include/Makefile.config

ifeq ($(wildcard $(CONFIG_FILE)),)
    $(error $(CONFIG_FILE) not found. See $(CONFIG_FILE).example)
endif
include $(CONFIG_FILE)

# Compiler Flags

DLEVEL ?= 0
LFLAGS += -lm -lrt

# Build dirs
ifeq ($(VERSION),)
    VERSION = Default
endif
SRC_DIR = src/
CAM_PIPE_SRC_DIR = $(SRC_DIR)
BUILD_DIR = build/$(TARGET)_$(VERSION)
CURRENT_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

INCLUDES +=  -I$(SRC_DIR) 

ifneq ($(CONFUSE_ROOT),)
INCLUDES += -I$(CONFUSE_ROOT)/include
LFLAGS += -L$(CONFUSE_ROOT)/lib
endif

EXE = miniera-hpvm-trace-$(VERSION)-$(TARGET)
RISCVEXE = miniera-hpvm-riscv

CAM_CFLAGS += -mf16c -flax-vector-conversions
LFLAGS += -pthread

#$(DEBUG): $(NATIVE_FULL_PATH_SRCS) $(GEM5_FULL_PATH_SRCS)
#	@echo Building benchmark for native machine with debug support.
#	#@mkdir -p $(BUILD_DIR)
#	@$(CC) $(CAM_CFLAGS) -ggdb3 $(INCLUDES) -DGEM5 -DDMA_MODE -DDMA_INTERFACE_V3 -o $(DEBUG) $^ $(LFLAGS)
#
#clean-native:
#	rm -f $(NATIVE) $(DEBUG)

## BEGIN HPVM MAKEFILE
LANGUAGE=hpvm
SRCDIR_OBJS=read_trace.ll
OBJS_SRC=$(wildcard $(SRC_DIR)/*.c)
HPVM_OBJS=main.hpvm.ll
APP = $(EXE)
APP_CUDALDFLAGS=-lm -lstdc++
APP_CFLAGS= $(INCLUDES) -DDMA_MODE -DDMA_INTERFACE_V3
APP_CXXFLAGS=-ffast-math -O0 -I/opt/opencv/include
APP_LDFLAGS=$(LFLAGS)
OPT_FLAGS = -tti -targetlibinfo -tbaa -scoped-noalias -assumption-cache-tracker -profile-summary-info -forceattrs -inferattrs -ipsccp -globalopt -domtree -mem2reg -deadargelim -domtree -basicaa -aa -simplifycfg -pgo-icall-prom -basiccg -globals-aa -prune-eh -always-inline -functionattrs -domtree -sroa -early-cse -lazy-value-info -jump-threading -correlated-propagation -simplifycfg -domtree -basicaa -aa -libcalls-shrinkwrap -tailcallelim -simplifycfg -reassociate -domtree -loops -loop-simplify -lcssa-verification -lcssa -basicaa -aa -scalar-evolution -loop-rotate -licm -loop-unswitch -simplifycfg -domtree -basicaa -aa -loops -loop-simplify -lcssa-verification -lcssa -scalar-evolution -indvars -loop-idiom -loop-deletion -memdep -memcpyopt -sccp -domtree -demanded-bits -bdce -basicaa -aa -lazy-value-info -jump-threading -correlated-propagation -domtree -basicaa -aa -memdep -dse -loops -loop-simplify -lcssa-verification -lcssa -aa -scalar-evolution -licm -postdomtree -adce -simplifycfg -domtree -basicaa -aa -barrier -basiccg -rpo-functionattrs -globals-aa -float2int -domtree -loops -loop-simplify -lcssa-verification -lcssa -basicaa -aa -scalar-evolution -loop-rotate -loop-accesses -lazy-branch-prob -lazy-block-freq -opt-remark-emitter -loop-distribute -loop-simplify -lcssa-verification -lcssa -branch-prob -block-freq -scalar-evolution -basicaa -aa -loop-accesses -demanded-bits -lazy-branch-prob -lazy-block-freq -opt-remark-emitter -loop-vectorize -loop-simplify -scalar-evolution -aa -loop-accesses -loop-load-elim -basicaa -aa -simplifycfg -domtree -basicaa -aa -loops -scalar-evolution -alignment-from-assumptions -strip-dead-prototypes -domtree -loops -branch-prob -block-freq -loop-simplify -lcssa-verification -lcssa -basicaa -aa -scalar-evolution -branch-prob -block-freq -loop-sink -instsimplify 

CFLAGS = -O1 $(APP_CFLAGS) $(PLATFORM_CFLAGS)
OBJS_CFLAGS = -O1 $(APP_CFLAGS) $(PLATFORM_CFLAGS)
CXXFLAGS = $(APP_CXXFLAGS) $(PLATFORM_CXXFLAGS)
LDFLAGS= $(APP_LDFLAGS) $(PLATFORM_LDFLAGS)

LIBCLC_LIB_PATH = $(LLVM_SRC_ROOT)/../libclc/built_libs
HPVM_RT_PATH = $(LLVM_SRC_ROOT)/../build/tools/hpvm/projects/hpvm-rt

HPVM_RT_LIB = $(HPVM_RT_PATH)/hpvm-rt.bc
#LIBCLC_NVPTX_LIB = $(LIBCLC_LIB_PATH)/nvptx--nvidiacl.bc
LIBCLC_NVPTX_LIB = $(LIBCLC_LIB_PATH)/nvptx64--nvidiacl.bc
#LIBCLC_NVPTX_LIB = nvptx64--nvidiacl.bc

LLVM_34_AS = $(LLVM_34_ROOT)/build/bin/llvm-as

TESTGEN_OPTFLAGS = -load LLVMGenHPVM.so -genhpvm -globaldce
KERNEL_GEN_FLAGS = -O3 -target nvptx64-nvidia-nvcl

ifeq ($(TARGET),x86)
  DEVICE = SPIR_TARGET
  HPVM_OPTFLAGS = -load LLVMBuildDFG.so -load LLVMLocalMem.so -load LLVMDFG2LLVM_SPIR.so -load LLVMDFG2LLVM_CPU.so -load LLVMClearDFG.so -localmem -dfg2llvm-spir -dfg2llvm-cpu -clearDFG
  CFLAGS += -DOPENCL_CPU
  HPVM_OPTFLAGS += -hpvm-timers-cpu -hpvm-timers-spir
else ifeq ($(TARGET),seq)
  DEVICE = CPU_TARGET
  HPVM_OPTFLAGS = -load LLVMBuildDFG.so -load LLVMDFG2LLVM_CPU.so -load LLVMClearDFG.so -dfg2llvm-cpu -clearDFG
  HPVM_OPTFLAGS += -hpvm-timers-cpu
else ifeq ($(TARGET),fpga)
  DEVICE = FPGA_TARGET
  HPVM_OPTFLAGS = -load LLVMBuildDFG.so -load LLVMLocalMem.so -load LLVMDFG2LLVM_FPGA.so -load LLVMDFG2LLVM_CPU.so -load LLVMClearDFG.so -localmem -dfg2llvm-fpga -dfg2llvm-cpu -clearDFG
  CFLAGS += -DOPENCL_CPU
  HPVM_OPTFLAGS += -hpvm-timers-cpu -hpvm-timers-fpga
else
  DEVICE = GPU_TARGET
  HPVM_OPTFLAGS = -load LLVMBuildDFG.so -load LLVMLocalMem.so -load LLVMDFG2LLVM_NVPTX.so -load LLVMDFG2LLVM_CPU.so -load LLVMClearDFG.so -localmem -dfg2llvm-nvptx -dfg2llvm-cpu -clearDFG
  HPVM_OPTFLAGS += -hpvm-timers-cpu -hpvm-timers-ptx
endif
  TESTGEN_OPTFLAGS += -hpvm-timers-gen

CFLAGS += -DDEVICE=$(DEVICE)
CXXFLAGS += -DDEVICE=$(DEVICE)

#ifeq ($(TIMER),x86)
#  HPVM_OPTFLAGS += -hpvm-timers-cpu
#else ifeq ($(TIMER),ptx)
#  HPVM_OPTFLAGS += -hpvm-timers-ptx
#else ifeq ($(TIMER),gen)
#  TESTGEN_OPTFLAGS += -hpvm-timers-gen
#else ifeq ($(TIMER),spir)
#  TESTGEN_OPTFLAGS += -hpvm-timers-spir
#else ifeq ($(TIMER),fpga)
#  TESTGEN_OPTFLAGS += -hpvm-timers-fpga
#else ifeq ($(TIMER),no)
#else
#  ifeq ($(TARGET),x86)
#    HPVM_OPTFLAGS += -hpvm-timers-cpu -hpvm-timers-spir
#  else ifeq ($(TARGET),seq)
#    HPVM_OPTFLAGS += -hpvm-timers-cpu
#  else ifeq ($(TARGET),fpga)
#    HPVM_OPTFLAGS += -hpvm-timers-cpu -hpvm-timers-fpga
#  else ifeq ($(TARGET),seqcpu)
#    HPVM_OPTFLAGS += -hpvm-timers-cpu -hpvm-timers-spir
#  else ifeq ($(TARGET),seqgpu)
#    HPVM_OPTFLAGS += -hpvm-timers-cpu -hpvm-timers-ptx
#  else
#    HPVM_OPTFLAGS += -hpvm-timers-cpu -hpvm-timers-ptx
#  endif
#  TESTGEN_OPTFLAGS += -hpvm-timers-gen
#endif

# Add BUILDDIR as a prefix to each element of $1
INBUILDDIR=$(addprefix $(BUILD_DIR)/,$(1))

# Add SRCDIR as a prefix to each element of $1
#INSRCDIR=$(addprefix $(SRCDIR)/,$(1))

PYTHON_LLVM_40_34 = ../llvm-40-34.py

.PRECIOUS: $(BUILD_DIR)/%.ll

OBJS = $(call INBUILDDIR,$(SRCDIR_OBJS))
TEST_OBJS = $(call INBUILDDIR,$(HPVM_OBJS))
KERNEL = $(TEST_OBJS).kernels.ll

ifeq ($(TARGET),x86)
  SPIR_ASSEMBLY = $(TEST_OBJS).kernels.bc
else ifeq ($(TARGET),seq)
else ifeq ($(TARGET),fpga)
  AOC_CL = $(TEST_OBJS).kernels.cl
  AOCL_ASSEMBLY = $(TEST_OBJS).kernels.aocx
  BOARD = a10gx
  ifeq ($(EMULATION),1)
    EXE = cava-hpvm-emu
    AOC_EMU = -march=emulator
    BUILD_DIR = build/$(TARGET)-emu
  endif
else
  KERNEL_LINKED = $(BUILD_DIR)/$(APP).kernels.linked.ll
  #KERNEL = $(TEST_OBJS).kernels.ll
  PTX_ASSEMBLY = $(TEST_OBJS).nvptx.s
endif

HOST_LINKED = $(BUILD_DIR)/$(APP).linked.ll
HOST = $(BUILD_DIR)/$(APP).host.ll

ifeq ($(OPENCL_PATH),)
FAILSAFE=no_opencl
else 
FAILSAFE=
endif

# Targets
default: $(FAILSAFE) $(BUILD_DIR) $(EXE)
riscv: $(FAILSAFE) $(BUILD_DIR) $(RISCVEXE)
#default: $(FAILSAFE) $(BUILD_DIR) $(PTX_ASSEMBLY) $(SPIR_ASSEMBLY) $(AOC_CL) $(AOCL_ASSEMBLY) $(EXE)

$(PTX_ASSEMBLY) : $(KERNEL_LINKED)
	$(CC) $(KERNEL_GEN_FLAGS) -S $< -o $@

$(KERNEL_LINKED) : $(KERNEL)
	$(LLVM_LINK) $(LIBCLC_NVPTX_LIB) -S $< -o $@

$(SPIR_ASSEMBLY) : $(KERNEL)
	python $(PYTHON_LLVM_40_34) $< $(BUILD_DIR)/kernel_34.ll
	$(LLVM_34_AS) $(BUILD_DIR)/kernel_34.ll -o $@

$(AOCL_ASSEMBLY) : $(AOC_CL)
	aoc --report $(AOC_EMU) $(AOC_CL) -o $(AOCL_ASSEMBLY) -board=$(BOARD)

$(AOC_CL) : $(KERNEL)
	llvm-cbe --debug $(KERNEL)

$(RISCVEXE) : $(HOST_LINKED)
	#$(OPT) 
	$(CXX) --target=riscv64 -march=rv64g -mabi=lp64d $< -c -o test.o
	/home/aejjeh/work_dir/riscv/bin/riscv64-unknown-linux-gnu-g++ test.o -o $@ -lm -lrt -lpthread -Wl,--eh-frame-hdr -mabi=lp64d -march=rv64g	
	rm test.o

$(EXE) : $(HOST_LINKED)
	$(CXX) -O3 $(LDFLAGS) $< -o $@

$(HOST_LINKED) : $(HOST) $(OBJS) $(HPVM_RT_LIB)
	$(LLVM_LINK) $^ -S -o $@

$(HOST) $(KERNEL): $(BUILD_DIR)/$(HPVM_OBJS)
	$(OPT) -debug $(HPVM_OPTFLAGS) -S $< -o $(HOST)
#	mv *.ll $(BUILD_DIR) 
#	$(OPT) -debug-only=DFG2LLVM_SPIR,DFG2LLVM_CPU,DFG2LLVM_FPGA,GENHPVM $(HPVM_OPTFLAGS) -S $< -o $(HOST)
#$(OBJS): $(OBJS_SRC)
#	$(CC) $(OBJS_CFLAGS) -emit-llvm -S -o $@ $<

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.ll : $(SRC_DIR)/%.c
	$(CC) $(OBJS_CFLAGS) -emit-llvm -S -o $@ $<

$(BUILD_DIR)/main.ll : $(SRC_DIR)/main.c
	$(CC) $(CFLAGS) -emit-llvm -S -o $@ $<

#$(BUILD_DIR)/main.opt.ll : $(BUILD_DIR)/main.ll
#	$(OPT) $(OPT_FLAGS) $< -S -o $@

$(BUILD_DIR)/main.hpvm.ll : $(BUILD_DIR)/main.ll
	$(OPT) -debug-only=genhpvm $(TESTGEN_OPTFLAGS) $< -S -o $@

## END HPVM MAKEFILE
