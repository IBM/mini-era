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

ifeq ($(TARGET),)
	TARGET = seq
endif


# Build dirs
SRC_DIR = src/
BUILD_DIR = build/$(TARGET)
CURRENT_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

INCLUDES +=  -I$(SRC_DIR) 

ifneq ($(CONFUSE_ROOT),)
INCLUDES += -I$(CONFUSE_ROOT)/include
LFLAGS += -L$(CONFUSE_ROOT)/lib
endif

EXE = miniera-hpvm-$(TARGET)
RISCVEXE = miniera-hpvm-riscv
EPOCHSEXE = miniera-hpvm-epochs

LFLAGS += -pthread


## BEGIN HPVM MAKEFILE
LANGUAGE=hpvm
SRCDIR_OBJS=read_trace.ll
OBJS_SRC=$(wildcard $(SRC_DIR)/*.c)
HPVM_OBJS=main.hpvm.ll
APP = $(EXE)
APP_CFLAGS= -ffast-math $(INCLUDES) -DDMA_MODE -DDMA_INTERFACE_V3
APP_LDFLAGS=$(LFLAGS)

CFLAGS = -O1 $(APP_CFLAGS) $(PLATFORM_CFLAGS)
OBJS_CFLAGS = -O1 $(APP_CFLAGS) $(PLATFORM_CFLAGS)
LDFLAGS= $(APP_LDFLAGS) $(PLATFORM_LDFLAGS)

HPVM_RT_PATH = $(LLVM_SRC_ROOT)/../build/tools/hpvm/projects/hpvm-rt

HPVM_RT_LIB = $(HPVM_RT_PATH)/hpvm-rt.bc

TESTGEN_OPTFLAGS = -load LLVMGenHPVM.so -genhpvm -globaldce

DEVICE = CPU_TARGET
HPVM_OPTFLAGS = -load LLVMBuildDFG.so -load LLVMDFG2LLVM_CPU.so -load LLVMClearDFG.so -dfg2llvm-cpu -clearDFG

CFLAGS += -DDEVICE=$(DEVICE)
CXXFLAGS += -DDEVICE=$(DEVICE)

# Add BUILDDIR as a prefix to each element of $1
INBUILDDIR=$(addprefix $(BUILD_DIR)/,$(1))

.PRECIOUS: $(BUILD_DIR)/%.ll

OBJS = $(call INBUILDDIR,$(SRCDIR_OBJS))
TEST_OBJS = $(call INBUILDDIR,$(HPVM_OBJS))
KERNEL = $(TEST_OBJS).kernels.ll

HOST_LINKED = $(BUILD_DIR)/$(APP).linked.ll
HOST = $(BUILD_DIR)/$(APP).host.ll
EPOCHS_HOST = $(BUILD_DIR)/$(APP).host.epochs.ll
EPOCHS_LINKED = $(BUILD_DIR)/$(APP).linked.epochs.ll

ifeq ($(OPENCL_PATH),)
FAILSAFE=no_opencl
else 
FAILSAFE=
endif

# Targets
default: $(FAILSAFE) $(BUILD_DIR) $(EXE)
riscv: $(FAILSAFE) $(BUILD_DIR) $(RISCVEXE)
epochs: $(FAILSAFE) $(BUILD_DIR) $(EPOCHSEXE)

$(EPOCHSEXE) : $(EPOCHS_LINKED)
	$(CXX) --target=riscv64 -march=rv64g -mabi=lp64d $< -c -o test.o
	/home/aejjeh/work_dir/riscv/bin/riscv64-unknown-linux-gnu-g++ test.o -o $@ -L$(ESP_DIR)/contig_alloc -L$(ESP_DIR)/libesp -L$(ESP_DIR)/test -lm -lrt -lpthread -lesp -ltest -lcontig -Wl,--eh-frame-hdr -mabi=lp64d -march=rv64g	
	rm test.o

$(EPOCHS_LINKED) : $(EPOCHS_HOST) $(OBJS) $(HPVM_RT_LIB)
	$(CC) -S -O0 -emit-llvm $(SRC_DIR)/fft_hook_impl.c -I$(ESP_DIR)/include -o $(BUILD_DIR)/fft_hook.ll
	$(LLVM_LINK) $^ $(BUILD_DIR)/fft_hook.ll -S -o $@

$(EPOCHS_HOST) : $(HOST)
	$(CC) $(CFLAGS) -emit-llvm -S $(SRC_DIR)/fft_spec.c -o $(BUILD_DIR)/fft_spec.ll
	$(OPT) -S -load $(ESP_MATCHER_PATH)/LLVMesp_codegen.so -esp_codegen $< --targetspec $(BUILD_DIR)/fft_spec.ll:fft -o $@ 

$(RISCVEXE) : $(HOST_LINKED)
	$(CXX) --target=riscv64 -march=rv64g -mabi=lp64d $< -c -o test.o
	/home/aejjeh/work_dir/riscv/bin/riscv64-unknown-linux-gnu-g++ test.o -o $@ -lm -lrt -lpthread -Wl,--eh-frame-hdr -mabi=lp64d -march=rv64g	
	rm test.o

$(EXE) : $(HOST_LINKED)
	$(CXX) -O3 $(LDFLAGS) $< -o $@

$(HOST_LINKED) : $(HOST) $(OBJS) $(HPVM_RT_LIB)
	$(LLVM_LINK) $^ -S -o $@

$(HOST) $(KERNEL): $(BUILD_DIR)/$(HPVM_OBJS)
	$(OPT) $(HPVM_OPTFLAGS) -S $< -o $(HOST)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.ll : $(SRC_DIR)/%.c
	$(CC) $(OBJS_CFLAGS) -emit-llvm -S -o $@ $<

$(BUILD_DIR)/main.ll : $(SRC_DIR)/main.c
	$(CC) $(CFLAGS) -emit-llvm -S -o $@ $<

$(BUILD_DIR)/main.hpvm.ll : $(BUILD_DIR)/main.ll
	$(OPT) $(TESTGEN_OPTFLAGS) $< -S -o $@

clean:
	if [ -d "$(BUILD_DIR)" ]; then rm -r $(BUILD_DIR); fi
	if [ -f "$(EXE)" ]; then rm $(EXE); fi
	if [ -f "$(EPOCHSEXE)" ]; then rm $(EPOCHSEXE); fi
	if [ -f "$(RISCVEXE)" ]; then rm $(RISCVEXE); fi
## END HPVM MAKEFILE
