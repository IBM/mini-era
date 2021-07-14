include ./hardware_config

CPU ?= ariane
ARCH ?= riscv
EXE_EXTENSION=
ifdef DO_CROSS_COMPILATION
 CROSS_COMPILE ?= riscv64-unknown-linux-gnu-
 TOOLCHAIN_PREFIX ?= $(RISCV_BIN_DIR)/riscv64-unknown-linux-gnu-
 EXE_EXTENSION=-RV
endif

ifdef COMPILE_TO_ESP
 ESP_ROOT ?= $(realpath ../../esp)
 ESP_DRIVERS ?= $(ESP_ROOT)/soft/common/drivers
 ESP_DRV_LINUX  = $(ESP_DRIVERS)/linux
endif

CC = gcc -std=c99


INCDIR ?=
INCDIR += -I./include
ifdef COMPILE_TO_ESP
 INCDIR += -I$(ESP_DRIVERS)/common/include
 INCDIR += -I$(ESP_DRIVERS)/linux/include
endif

CFLAGS ?= -O2 -g
CFLAGS += $(INCDIR)
CFLAGS += -DINT_TIME
ifdef COMPILE_TO_ESP
 CFLAGS += -DCOMPILE_TO_ESP
endif
#  -- ALWAYS use this one! --   ifdef CONFIG_ESP_INTERFACE
CFLAGS += -DUSE_ESP_INTERFACE
#   endif

SW_STR = -SW
FA_STR =
VA_STR =
ifdef CONFIG_FFT_EN
 SW_STR = -HW
 FA_STR = -F$(CONFIG_FFT_ACCEL_VER)
 CFLAGS += -DHW_FFT
 CFLAGS += -DUSE_FFT_FX=$(CONFIG_FFT_FX)
 CFLAGS += -DUSE_FFT_ACCEL_VERSION=$(CONFIG_FFT_ACCEL_VER)
 CFLAGS += -DFFT_DEV_BASE='"$(FFT_DEVICE_BASE)"'
endif
ifdef CONFIG_FFT_BITREV
 CFLAGS += -DHW_FFT_BITREV
endif

ifdef CONFIG_VITERBI_EN
 SW_STR = -HW
 VA_STR = -V
 CFLAGS += -DHW_VIT
endif

ifdef CONFIG_KERAS_CV_BYPASS
 CFLAGS += -DBYPASS_KERAS_CV_CODE
endif

ifdef CONFIG_CV_CNN_EN
 SW_STR = -HW
 CA_STR = -C
 CFLAGS += -DHW_CV -DENABLE_NVDLA
 NVDLA_MODULE = hpvm-mod.nvdla
endif

ifdef CONFIG_VERBOSE
CFLAGS += -DVERBOSE
endif

ifdef CONFIG_GDB
CFLAGS += -g
endif

LDLIBS ?=
ifdef COMPILE_TO_ESP
 LDLIBS += -L$(ESP_BUILD_DRIVERS)/contig_alloc
 LDLIBS += -L$(ESP_BUILD_DRIVERS)/test
 LDLIBS += -L$(ESP_BUILD_DRIVERS)/libesp
endif
LDLIBS += -L./

LDFLAGS ?=
LDFLAGS += -lm
LDFLAGS += -lpthread
ifdef COMPILE_TO_ESP
 LDFLAGS += -lrt
 LDFLAGS += -lesp
 LDFLAGS += -ltest
 LDFLAGS += -lcontig
endif

SRC_T = $(foreach f, $(wildcard src/*.c), $(shell basename $(f)))
SRC_D = $(wildcard src/*.c)
HDR_T = $(wildcard include/*.h)
OBJ_T = $(SRC_T:%.c=obj_t/%.o)
OBJ_S = $(SRC_T:%.c=obj_s/%.o)

VPATH = ./src

TARGET=mini-era$(EXE_EXTENSION)$(SW_STR)$(FA_STR)$(VA_STR)$(CA_STR).exe
STARGET=sim-mini-era$(EXE_EXTENSION)$(SW_STR)$(FA_STR)$(VA_STR)$(CA_STR).exe

all: esp-libs obj_t obj_s $(TARGET) $(STARGET)

ESP_BUILD_DRIVERS     = esp-build/drivers

esp-build:
	@mkdir -p $(ESP_BUILD_DRIVERS)/contig_alloc
	@mkdir -p $(ESP_BUILD_DRIVERS)/esp
	@mkdir -p $(ESP_BUILD_DRIVERS)/esp_cache
	@mkdir -p $(ESP_BUILD_DRIVERS)/libesp
	@mkdir -p $(ESP_BUILD_DRIVERS)/probe
	@mkdir -p $(ESP_BUILD_DRIVERS)/test
	@mkdir -p $(ESP_BUILD_DRIVERS)/utils/baremetal
	@mkdir -p $(ESP_BUILD_DRIVERS)/utils/linux
	@ln -sf $(ESP_DRV_LINUX)/contig_alloc/* $(ESP_BUILD_DRIVERS)/contig_alloc
	@ln -sf $(ESP_DRV_LINUX)/esp/* $(ESP_BUILD_DRIVERS)/esp
	@ln -sf $(ESP_DRV_LINUX)/esp_cache/* $(ESP_BUILD_DRIVERS)/esp_cache
	@ln -sf $(ESP_DRV_LINUX)/driver.mk $(ESP_BUILD_DRIVERS)
	@ln -sf $(ESP_DRV_LINUX)/include $(ESP_BUILD_DRIVERS)
	@ln -sf $(ESP_DRV_LINUX)/../common $(ESP_BUILD_DRIVERS)/../common

esp-build-clean:

esp-build-distclean: esp-build-clean
	$(QUIET_CLEAN)$(RM) -rf esp-build

ifdef COMPILE_TO_ESP
esp-libs: esp-build
	  CROSS_COMPILE=$(CROSS_COMPILE) DRIVERS=$(ESP_DRV_LINUX) $(MAKE) -C $(ESP_BUILD_DRIVERS)/contig_alloc/ libcontig.a
	  cd $(ESP_BUILD_DRIVERS)/test; CROSS_COMPILE=$(CROSS_COMPILE) BUILD_PATH=$$PWD $(MAKE) -C $(ESP_DRV_LINUX)/test
	  cd $(ESP_BUILD_DRIVERS)/libesp; CROSS_COMPILE=$(CROSS_COMPILE) BUILD_PATH=$$PWD $(MAKE) -C $(ESP_DRV_LINUX)/libesp
	  cd $(ESP_BUILD_DRIVERS)/utils; CROSS_COMPILE=$(CROSS_COMPILE) BUILD_PATH=$$PWD $(MAKE) -C $(ESP_DRV_LINUX)/utils
else
esp-libs: 

endif
.PHONY: esp-build-clean esp-build-distclean esp-libs


#------------------------------------------------------------------------------------------
#------------------------------------------------------------------------------------------

CUR_DIR = $(dir $(realpath $(firstword $(MAKEFILE_LIST))))
$(info $$CUR_DIR is [${CUR_DIR}])

ROOT := $(CUR_DIR)/sw/umd
TOP := $(ROOT)
$(info $$ROOT is [${ROOT}])

ESP_NVDLA_DIR = esp_hardware/nvdla
#INCLUDES +=  -I$(SRC_DIR) -I$(ESP_NVDLA_DIR) -I$(TOP)/core/include
INC_DIR +=  -I$(ESP_NVDLA_DIR) -I$(ROOT)/core/include

NVDLA_RUNTIME_DIR = $(ROOT)
NVDLA_RUNTIME = $(NVDLA_RUNTIME_DIR)/out
$(info $$NVDLA_RUNTIME is [${NVDLA_RUNTIME}])
$(info $$NVDLA_RUNTIME_DIR is [${NVDLA_RUNTIME_DIR}])

ifdef CONFIG_CV_CNN_EN
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

$(info $$ALLMODULE_OBJS is [${ALLMODULE_OBJS}])
$(info $$MODULE_OBJS is [${MODULE_OBJS}])
endif


#-----------------------------------------------------------------------------------------------
$(NVDLA_RUNTIME):
	@echo -e ${YEL}Compiling NVDLA Runtime Library${NC}
	@cd $(NVDLA_RUNTIME_DIR) && make ROOT=$(ROOT) runtime


$(NVDLA_MODULE): $(NVDLA_RUNTIME)
	#@echo -e ${YEL}Compiling HPVM Module for NVDLA${NC}
	#@cd $(NVDLA_MAKE_DIR) && python gen_me_hpvm_mod.py && cp $(NVDLA_RES_DIR)/$(NVDLA_MODULE) $(CUR_DIR)

#-----------------------------------------------------------------------------------------------

obj_t/%.o: %.c
	$(CROSS_COMPILE)$(CC) $(CFLAGS) -c $< -o $@

obj_s/%.o: %.c
	$(CROSS_COMPILE)$(CC) $(CFLAGS) -DUSE_SIM_ENVIRON -c $< -o $@

$(OBJ_T): $(HDR_T)

$(TARGET): $(OBJ_T) $(NVDLA_MODULE) $(ALLMODULE_OBJS)
	#$(CROSS_COMPILE)$(CC) -fPIC $< -c -o me_test.o
	$(CROSS_COMPILE)$(LD) -r $(ALLMODULE_OBJS) $(OBJ_T) -o wnvdla_test.o
	$(CROSS_COMPILE)$(CXX) $(LDLIBS) wnvdla_test.o -o $@ $(LDFLAGS) $(NVDLA_FLAGS)
	#$(CROSS_COMPILE)$(CC) $(LDLIBS) $^ -o $@ $(LDFLAGS)
	rm wnvdla_test.o

$(STARGET): $(OBJ_S) $(NVDLA_MODULE) $(ALLMODULE_OBJS)
	#$(CROSS_COMPILE)$(CC) -fPIC $< -c -o me_test.o
	$(CROSS_COMPILE)$(LD) -r $(ALLMODULE_OBJS) $(OBJ_S) -o wnvdla_test.o
	$(CROSS_COMPILE)$(CXX) $(LDLIBS) wnvdla_test.o -o $@ $(LDFLAGS) $(NVDLA_FLAGS)
	#$(CROSS_COMPILE)$(CC) $(LDLIBS) $^ -o $@ $(LDFLAGS)
	rm wnvdla_test.o

clean:
	$(RM) $(OBJ_T) $(OBJ_S)
	$(RM) -r obj_t obj_s
	if [ -f "$(NVDLA_MODULE)" ]; then rm $(NVDLA_MODULE); fi
	if [ -d "$(NVDLA_RUNTIME)" ]; then rm -rf $(NVDLA_RUNTIME); fi

clobber: clean
	$(RM) -rf esp-build
	$(RM) $(TARGET) $(STARGET)


obj_t:
	mkdir $@

obj_s:
	mkdir $@

.PHONY: all clean

#depend:;	makedepend -fMakefile -- $(CFLAGS) -- $(SRC_D)
# DO NOT DELETE THIS LINE -- make depend depends on it.

src/read_trace.o: ./include/kernels_api.h ./include/verbose.h
src/read_trace.o: ./include/base_types.h ./include/calc_fmcw_dist.h
src/read_trace.o: ./include/utils.h
src/read_trace.o: ./include/sim_environs.h
src/getopt.o: ./include/getopt.h
src/descrambler_function.o: ./include/base.h ./include/utils.h
src/descrambler_function.o: ./include/viterbi_standalone.h
src/viterbi_flat.o: ./include/base.h ./include/utils.h
src/viterbi_flat.o: ./include/viterbi_flat.h ./include/verbose.h
src/viterbi_flat.o: ./include/viterbi_standalone.h
src/viterbi_flat.o: ./include/base_types.h
src/sim_environs.o: ./include/kernels_api.h ./include/verbose.h
src/sim_environs.o: ./include/base_types.h ./include/calc_fmcw_dist.h
src/sim_environs.o: ./include/utils.h
src/sim_environs.o: ./include/sim_environs.h
src/cpu_vit_accel.o: ./include/base.h ./include/utils.h
src/cpu_vit_accel.o: ./include/viterbi_flat.h ./include/verbose.h
src/cpu_vit_accel.o: ./include/viterbi_standalone.h
src/viterbi_standalone.o: ./include/base.h ./include/utils.h
src/viterbi_standalone.o: ./include/viterbi_flat.h
src/viterbi_standalone.o: ./include/verbose.h
src/main.o: ./include/getopt.h ./include/verbose.h
src/main.o: ./include/base_types.h
src/main.o: ./include/kernels_api.h ./include/calc_fmcw_dist.h
src/main.o: ./include/utils.h ./include/sim_environs.h
src/calculate_dist_from_fmcw.o: ./include/fft-1d.h
src/calculate_dist_from_fmcw.o: ./include/calc_fmcw_dist.h
src/calculate_dist_from_fmcw.o: ./include/base_types.h
src/cpu_fft_accel.o: ./include/base_types.h ./include/fft-1d.h
src/cpu_fft_accel.o: ./include/calc_fmcw_dist.h
src/timer.o: ./include/timer.h
src/kernels_api.o: ./include/kernels_api.h
src/kernels_api.o: ./include/verbose.h ./include/base_types.h
src/kernels_api.o: ./include/calc_fmcw_dist.h
src/kernels_api.o: ./include/utils.h ./include/read_trace.h
src/kernels_api.o: ./include/viterbi_flat.h ./include/base.h
