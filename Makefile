# This Makefile compiles the HPVM-CAVA pilot project. 
# It builds HPVM-related dependencies, then the native camera pipeline ISP code.
#
# Paths to some dependencies (e.g., HPVM, LLVM) must exist in Makefile.config,
# which can be copied from Makefile.config.example for a start.

ifeq ($(HPVM_DIR),)
    $(error HPVM_DIR is undefined! setup_paths.sh needs to be sourced before running make!)
endif

CC = $(LLVM_BUILD_DIR)/bin/clang
#PLATFORM_CFLAGS = -I$(OPENCL_PATH)/include/CL/ -I$(HPVM_BENCH_DIR)/include
PLATFORM_CFLAGS = -I$(HPVM_BENCH_DIR)/include

CXX = $(LLVM_BUILD_DIR)/bin/clang++
#PLATFORM_CXXFLAGS = -I$(OPENCL_PATH)/include/CL/ -I$(HPVM_BENCH_DIR)/include
PLATFORM_CXXFLAGS = -I$(HPVM_BENCH_DIR)/include

LINKER = $(LLVM_BUILD_DIR)/bin/clang++
#PLATFORM_LDFLAGS = -lm -lpthread -lrt -lOpenCL -L$(OPENCL_LIB_PATH)
PLATFORM_LDFLAGS = -lm -lpthread -lrt -lOpenCL

LLVM_LIB_PATH = $(LLVM_BUILD_DIR)/lib
LLVM_BIN_PATH = $(LLVM_BUILD_DIR)/bin

OPT = $(LLVM_BIN_PATH)/opt
LLVM_LINK = $(LLVM_BIN_PATH)/llvm-link
LLVM_AS = $(LLVM_BIN_PATH)/llvm-as
LIT = $(LLVM_BIN_PATH)/llvm-lit
OCLBE = $(LLVM_BIN_PATH)/llvm-cbe

ifeq ($(TARGET),)
        TARGET = seq
endif

CUR_DIR = $(dir $(realpath $(firstword $(MAKEFILE_LIST))))

# Compiler Flags

LFLAGS += -lm -lrt

# Build dirs
SRC_DIR = src/
ESP_NVDLA_DIR = esp_hardware/nvdla
BUILD_DIR = build/$(TARGET)

INCLUDES +=  -I$(SRC_DIR) -I$(ESP_NVDLA_DIR) -I$(TOP)/core/include

EXE = miniera-hpvm-seq
RISCVEXE = miniera-hpvm-riscv
EPOCHSEXE = miniera-hpvm-epochs

LFLAGS += -pthread

## BEGIN HPVM MAKEFILE
SRCDIR_OBJS=read_trace.ll
OBJS_SRC=$(wildcard $(SRC_DIR)/*.c)
HPVM_OBJS=main.hpvm.ll
APP = miniera-hpvm-$(TARGET)
APP_CFLAGS= -ffast-math $(INCLUDES)
APP_LDFLAGS=$(LFLAGS)

CFLAGS = -O1 $(APP_CFLAGS) $(PLATFORM_CFLAGS)
OBJS_CFLAGS = -O1 $(APP_CFLAGS) $(PLATFORM_CFLAGS)
LDFLAGS= $(APP_LDFLAGS) $(PLATFORM_LDFLAGS)

HPVM_RT_PATH = $(LLVM_BUILD_DIR)/tools/hpvm/projects/hpvm-rt
HPVM_RT_LIB = $(HPVM_RT_PATH)/hpvm-rt.bc

DEVICE = CPU_TARGET
CFLAGS += -DDEVICE=$(DEVICE)
CXXFLAGS += -DDEVICE=$(DEVICE)

# Add BUILDDIR as a prefix to each element of $1
INBUILDDIR=$(addprefix $(BUILD_DIR)/,$(1))

.PRECIOUS: $(BUILD_DIR)/%.ll

OBJS = $(call INBUILDDIR,$(SRCDIR_OBJS))
TEST_OBJS = $(call INBUILDDIR,$(HPVM_OBJS))

KERNEL = $(TEST_OBJS).kernels.ll
HOST = $(BUILD_DIR)/$(APP).host.ll

EPOCHS_HOST = $(BUILD_DIR)/$(APP).host.epochs.ll

HOST_LINKED = $(BUILD_DIR)/$(APP).linked.ll
EPOCHS_LINKED = $(BUILD_DIR)/$(APP).linked.epochs.ll

NVDLA_MODULE = hpvm-mod.nvdla
NVDLA_DIR = $(APPROXHPVM_DIR)/llvm/test/VISC/DNN_Benchmarks/benchmarks/miniera-hpvm

LINKER=ld

YEL='\033[0;33m'
NC='\033[0m'

# Targets
.PHONY: default riscv epochs clean
default: $(BUILD_DIR) $(EXE)
riscv: $(BUILD_DIR) $(RISCVEXE)
#epochs: $(FAILSAFE) $(BUILD_DIR) $(EPOCHSEXE)
epochs: check-env $(NVDLA_MODULE) $(FAILSAFE) $(BUILD_DIR) $(EPOCHSEXE) 
#------------------------------------------------------------------------------------------
epochs: CFLAGS += -DENABLE_NVDLA

ROOT := $(CUR_DIR)/sw/umd

NVDLA_RUNTIME_DIR = $(CUR_DIR)/sw/umd/
NVDLA_RUNTIME = $(CUR_DIR)/sw/umd/out

TOOLCHAIN_PREFIX ?= $(RISCV_BIN_DIR)/riscv64-unknown-linux-gnu-

ifeq ($(TOOLCHAIN_PREFIX),)
$(error Toolchain prefix missing)
endif

MODULE := nvdla_runtime

include $(ROOT)/make/macros.mk

BUILDOUT ?= $(ROOT)/out/apps/runtime
BUILDDIR := $(BUILDOUT)/$(MODULE)
TEST_BIN := $(BUILDDIR)/$(MODULE)

MODULE_COMPILEFLAGS := -W -Wall -Wno-multichar -Wno-unused-parameter -Wno-unused-function -Werror-implicit-function-declaration
MODULE_CFLAGS := --std=c99
MODULE_CPPFLAGS := --std=c++11 -fexceptions -fno-rtti
NVDLA_FLAGS := -pthread -L$(ROOT)/external/ -ljpeg -L$(ROOT)/out/core/src/runtime/libnvdla_runtime -lnvdla_runtime  -Wl,-rpath=.

include esp_hardware/nvdla/rules.mk
#-----------------------------------------------------------------------------------------------

check-env:
ifndef APPROXHPVM_DIR
	$(error APPROXHPVM_DIR is undefined! setup_paths.sh needs to be sourced before running make!)
endif
ifndef MINIERA_DIR
	$(error MINIERA_DIR is undefined! setup_paths.sh needs to be sourced before running make!)
endif
ifndef RISCV_BIN_DIR
	$(error RISCV_BIN_DIR is undefined! setup_paths.sh needs to be sourced before running make!)
endif

$(NVDLA_MODULE):
	@echo -e ${YEL}Compiling NVDLA Runtime Library${NC}
	@cd $(NVDLA_RUNTIME_DIR) && make runtime
	@echo -e ${YEL}Compiling HPVM Module for NVDLA${NC}
	@cd $(APPROXHPVM_DIR)/llvm/test/VISC/DNN_Benchmarks/benchmarks/miniera-hpvm && make && cp $(NVDLA_MODULE) $(CUR_DIR)

$(EPOCHSEXE) : $(EPOCHS_LINKED) $(ALLMODULE_OBJS)
	@echo -e -n ${YEL}Cross-compiling for RISCV: ${NC}
	@echo -e ${YEL}1\) Use Clang to generate object file${NC}
	$(CXX) --target=riscv64 -march=rv64g -mabi=lp64d $< -c -o test.o
	$(RISCV_BIN_DIR)/riscv64-unknown-linux-gnu-ld -r $(ALLMODULE_OBJS) test.o -o epochs_test.o
	@echo -e ${YEL}Cross-compiling for RISCV: 2\) Use GCC cross-compiler to link the binary \(linking with ESP libraries required\)${NC}
	$(RISCV_BIN_DIR)/riscv64-unknown-linux-gnu-g++ epochs_test.o -o $@ -LESP/lib -lm -lrt -lpthread -lesp -ltest -lcontig -mabi=lp64d -march=rv64g	$(NVDLA_FLAGS)
	rm test.o epochs_test.o

$(EPOCHS_LINKED) : $(EPOCHS_HOST) $(OBJS) $(HPVM_RT_LIB)
	@echo -e ${YEL}Compile hook function for accelerator: fft${NC}
	$(CC) -S -O0 -emit-llvm $(SRC_DIR)/fft_hook_impl.c -IESP/include -o $(BUILD_DIR)/fft_hook.ll
	@echo -e ${YEL}Compile hook function for accelerator: viterbi${NC}
	$(CC) -S -O0 -emit-llvm $(SRC_DIR)/viterbi_hook_impl.c -IESP/include -o $(BUILD_DIR)/viterbi_hook.ll
	@echo -e ${YEL}Link main application with hook function, other necessary object files, and HPVM runtime library${NC}
	$(LLVM_LINK) $^ $(BUILD_DIR)/fft_hook.ll $(BUILD_DIR)/viterbi_hook.ll -S -o $@

$(EPOCHS_HOST) : $(HOST)
	@echo -e ${YEL}Generate .ll file for accelerator spec: fft_spec.c${NC}
	$(CC) $(CFLAGS) -emit-llvm -S $(SRC_DIR)/fft_spec.c -o $(BUILD_DIR)/fft_spec.ll
	@echo -e ${YEL}Accelerator code-gen for: fft_spec.ll${NC}
	$(OPT) -S -load LLVMesp_codegen.so -esp_codegen $< --targetspec $(BUILD_DIR)/fft_spec.ll:fft -o $@ 
	@echo -e ${YEL}Generate .ll file for accelerator spec: viterbi_spec.c${NC}
	$(CC) $(CFLAGS) -emit-llvm -S $(SRC_DIR)/viterbi_spec.c -o $(BUILD_DIR)/viterbi_spec.ll
	@echo -e ${YEL}Accelerator code-gen for: viterbi_spec.ll${NC}
	$(OPT) -S -load LLVMesp_codegen.so -esp_codegen $@ --targetspec $(BUILD_DIR)/viterbi_spec.ll:do_decoding -o $@ 

$(RISCVEXE) : $(HOST_LINKED)
	@echo -e -n ${YEL}Cross-compiling for RISCV: ${NC}
	@echo -e ${YEL}1\) Use Clang to generate object file${NC}
	$(CXX) --target=riscv64 -march=rv64g -mabi=lp64d $< -c -o test.o
	@echo -e ${YEL}Then gcc cross-compiler is used to link the binary${NC}
	$(RISCV_BIN_DIR)/riscv64-unknown-linux-gnu-g++ test.o -o $@ -lm -lrt -lpthread -Wl,--eh-frame-hdr -mabi=lp64d -march=rv64g	
	rm test.o

$(EXE) : $(HOST_LINKED)
	@echo -e ${YEL}Generating native executable${NC}
	$(CXX) -O3 $(LDFLAGS) $< -o $@

$(HOST_LINKED) : $(HOST) $(OBJS) $(HPVM_RT_LIB)
	@echo -e ${YEL}Link main application with other necessary object files and HPVM runtime library${NC}
	$(LLVM_LINK) $^ -S -o $@

$(HOST) $(KERNEL): $(BUILD_DIR)/$(HPVM_OBJS)
	@echo -e ${YEL}Build HPVM DFG and compile for CPU Backend${NC}
	$(OPT) -load LLVMBuildDFG.so -load LLVMDFG2LLVM_CPU.so -load LLVMClearDFG.so -dfg2llvm-cpu -clearDFG -S $< -o $(HOST)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.ll : $(SRC_DIR)/%.c
	@echo -e ${YEL}Building other source files${NC}
	$(CC) $(OBJS_CFLAGS) -emit-llvm -S -o $@ $<

$(BUILD_DIR)/main.ll : $(SRC_DIR)/main.c
	@echo -e ${YEL}Compiling main application code into LLVM${NC}
	$(CC) $(CFLAGS) -emit-llvm -S -o $@ $<

$(BUILD_DIR)/main.hpvm.ll : $(BUILD_DIR)/main.ll
	@echo -e ${YEL}Run GenHPVM on module to convert HPVM-C function calls into HPVM intrinsics${NC}
	$(OPT) -load LLVMGenHPVM.so -genhpvm -globaldce $< -S -o $@

clean:
	if [ -d "$(BUILD_DIR)" ]; then rm -r $(BUILD_DIR); fi
	if [ -f "$(EXE)" ]; then rm $(EXE); fi
	if [ -f "$(EPOCHSEXE)" ]; then rm $(EPOCHSEXE); fi
	if [ -f "$(RISCVEXE)" ]; then rm $(RISCVEXE); fi
	if [ -f "$(NVDLA_MODULE)" ]; then rm $(NVDLA_MODULE); fi
	if [ -d "$(NVDLA_RUNTIME)" ]; then rm -rf $(NVDLA_RUNTIME); fi
	if [ -d "output" ]; then rm -r output; fi


## END HPVM MAKEFILE
