# This Makefile compiles the HPVM-CAVA pilot project. 
# It builds HPVM-related dependencies, then the native camera pipeline ISP code.
#
# Paths to some dependencies (e.g., HPVM, LLVM) must exist in Makefile.config,
# which can be copied from Makefile.config.example for a start.

ifeq ($(HPVM_DIR),)
    $(error HPVM_DIR must be set!)
endif

CONFIG_FILE := $(HPVM_DIR)/test/benchmarks/include/Makefile.config

ifeq ($(wildcard $(CONFIG_FILE)),)
    $(error $(CONFIG_FILE) not found. See $(CONFIG_FILE).example)
endif
include $(CONFIG_FILE)

# Compiler Flags

LFLAGS += -lm -lrt

ifeq ($(TARGET),)
	TARGET = seq
endif


# Build dirs
SRC_DIR = src/
BUILD_DIR = build/$(TARGET)

INCLUDES +=  -I$(SRC_DIR) 

EXE = miniera-hpvm-$(TARGET)
RISCVEXE = miniera-hpvm-riscv
EPOCHSEXE = miniera-hpvm-epochs

LFLAGS += -pthread


## BEGIN HPVM MAKEFILE
SRCDIR_OBJS=read_trace.ll
OBJS_SRC=$(wildcard $(SRC_DIR)/*.c)
HPVM_OBJS=main.hpvm.ll
APP = $(EXE)
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

ifeq ($(OPENCL_PATH),)
FAILSAFE=no_opencl
else 
FAILSAFE=
endif

RED='\033[0;31m'
NC='\033[0m'

# Targets
default: $(FAILSAFE) $(BUILD_DIR) $(EXE)
riscv: $(FAILSAFE) $(BUILD_DIR) $(RISCVEXE)
epochs: $(FAILSAFE) $(BUILD_DIR) $(EPOCHSEXE)

$(EPOCHSEXE) : $(EPOCHS_LINKED)
	@echo -e -n ${RED}Cross-compiling for RISCV: ${NC}
	@echo -e ${RED}1\) Use Clang to generate object file${NC}
	$(CXX) --target=riscv64 -march=rv64g -mabi=lp64d $< -c -o test.o
	@echo -e ${RED}Cross-compiling for RISCV: 2\) Use GCC cross-compiler to link the binary \(linking with ESP libraries required\)${NC}
	$(RISCV_BIN_DIR)/riscv64-unknown-linux-gnu-g++ test.o -o $@ -LESP/lib -lm -lrt -lpthread -lesp -ltest -lcontig -mabi=lp64d -march=rv64g	
	rm test.o

$(EPOCHS_LINKED) : $(EPOCHS_HOST) $(OBJS) $(HPVM_RT_LIB)
	@echo -e ${RED}Compile hook function for accelerator: fft${NC}
	$(CC) -S -O0 -emit-llvm $(SRC_DIR)/fft_hook_impl.c -IESP/include -o $(BUILD_DIR)/fft_hook.ll
	@echo -e ${RED}Compile hook function for accelerator: viterbi${NC}
	$(CC) -S -O0 -emit-llvm $(SRC_DIR)/viterbi_hook_impl.c -IESP/include -o $(BUILD_DIR)/viterbi_hook.ll
	@echo -e ${RED}Link main application with hook function, other necessary object files, and HPVM runtime library${NC}
	$(LLVM_LINK) $^ $(BUILD_DIR)/fft_hook.ll $(BUILD_DIR)/viterbi_hook.ll -S -o $@

$(EPOCHS_HOST) : $(HOST)
	@echo -e ${RED}Generate .ll file for accelerator spec: fft_spec.c${NC}
	$(CC) $(CFLAGS) -emit-llvm -S $(SRC_DIR)/fft_spec.c -o $(BUILD_DIR)/fft_spec.ll
	@echo -e ${RED}Accelerator code-gen for: fft_spec.ll${NC}
	$(OPT) -S -load LLVMesp_codegen.so -esp_codegen $< --targetspec $(BUILD_DIR)/fft_spec.ll:fft -o $@ 
	@echo -e ${RED}Generate .ll file for accelerator spec: viterbi_spec.c${NC}
	$(CC) $(CFLAGS) -emit-llvm -S $(SRC_DIR)/viterbi_spec.c -o $(BUILD_DIR)/viterbi_spec.ll
	@echo -e ${RED}Accelerator code-gen for: viterbi_spec.ll${NC}
	$(OPT) -S -load LLVMesp_codegen.so -esp_codegen $@ --targetspec $(BUILD_DIR)/viterbi_spec.ll:do_decoding -o $@ 

$(RISCVEXE) : $(HOST_LINKED)
	@echo -e -n ${RED}Cross-compiling for RISCV: ${NC}
	@echo -e ${RED}1\) Use Clang to generate object file${NC}
	$(CXX) --target=riscv64 -march=rv64g -mabi=lp64d $< -c -o test.o
	@echo -e ${RED}Then gcc cross-compiler is used to link the binary${NC}
	$(RISCV_BIN_DIR)/riscv64-unknown-linux-gnu-g++ test.o -o $@ -lm -lrt -lpthread -Wl,--eh-frame-hdr -mabi=lp64d -march=rv64g	
	rm test.o

$(EXE) : $(HOST_LINKED)
	@echo -e ${RED}Generating native executable${NC}
	$(CXX) -O3 $(LDFLAGS) $< -o $@

$(HOST_LINKED) : $(HOST) $(OBJS) $(HPVM_RT_LIB)
	@echo -e ${RED}Link main application with other necessary object files and HPVM runtime library${NC}
	$(LLVM_LINK) $^ -S -o $@

$(HOST) $(KERNEL): $(BUILD_DIR)/$(HPVM_OBJS)
	@echo -e ${RED}Build HPVM DFG and compile for CPU Backend${NC}
	$(OPT) -load LLVMBuildDFG.so -load LLVMDFG2LLVM_CPU.so -load LLVMClearDFG.so -dfg2llvm-cpu -clearDFG -S $< -o $(HOST)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.ll : $(SRC_DIR)/%.c
	@echo -e ${RED}Building other source files${NC}
	$(CC) $(OBJS_CFLAGS) -emit-llvm -S -o $@ $<

$(BUILD_DIR)/main.ll : $(SRC_DIR)/main.c
	@echo -e ${RED}Compiling main application code into LLVM${NC}
	$(CC) $(CFLAGS) -emit-llvm -S -o $@ $<

$(BUILD_DIR)/main.hpvm.ll : $(BUILD_DIR)/main.ll
	@echo -e ${RED}Run GenHPVM on module to convert HPVM-C function calls into HPVM intrinsics${NC}
	$(OPT) -load LLVMGenHPVM.so -genhpvm -globaldce $< -S -o $@

clean:
	if [ -d "$(BUILD_DIR)" ]; then rm -r $(BUILD_DIR); fi
	if [ -f "$(EXE)" ]; then rm $(EXE); fi
	if [ -f "$(EPOCHSEXE)" ]; then rm $(EPOCHSEXE); fi
	if [ -f "$(RISCVEXE)" ]; then rm $(RISCVEXE); fi
## END HPVM MAKEFILE
