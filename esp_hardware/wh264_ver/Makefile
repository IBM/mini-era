include .config

CPU ?= ariane
ARCH ?= riscv
ifdef DO_CROSS_COMPILATION
CROSS_COMPILE ?= riscv64-unknown-linux-gnu-
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

CFLAGS ?= -O3
CFLAGS += $(INCDIR)
CFLAGS += -DINT_TIME
ifdef COMPILE_TO_ESP
CFLAGS += -DCOMPILE_TO_ESP
endif
#  -- ALWAYS use this one! --   ifdef CONFIG_ESP_INTERFACE
CFLAGS += -DUSE_ESP_INTERFACE
#   endif

CONFIG_WHICH_FFT_TYPE ?= 1
ifdef CONFIG_FFT_EN
CFLAGS += -DHW_FFT -DUSE_FFT_ACCEL_TYPE=$(CONFIG_WHICH_FFT_TYPE)
endif

CFLAGS += -DUSE_FFT_FX=$(CONFIG_FFT_FX)
ifdef CONFIG_FFT_BITREV
CFLAGS += -DHW_FFT_BITREV
endif
ifdef CONFIG_VITERBI_EN
CFLAGS += -DHW_VIT
endif
ifdef CONFIG_KERAS_CV_BYPASS
CFLAGS += -DBYPASS_KERAS_CV_CODE
endif
ifdef CONFIG_VERBOSE
CFLAGS += -DVERBOSE
endif

LDLIBS ?=
ifdef COMPILE_TO_ESP
LDLIBS += -L$(ESP_BUILD_DRIVERS)/contig_alloc
LDLIBS += -L$(ESP_BUILD_DRIVERS)/test
LDLIBS += -L$(ESP_BUILD_DRIVERS)/libesp
endif

LDFLAGS ?=
LDFLAGS += -lm
LDFLAGS += -lrt
LDFLAGS += -lpthread
ifdef COMPILE_TO_ESP
LDFLAGS += -lesp
LDFLAGS += -ltest
LDFLAGS += -lcontig
endif

SRC = 	src/calculate_dist_from_fmcw.c \
	src/cavlc.c \
	src/decode.c \
	src/descrambler_function.c \
	src/do_h264_decode.c \
	src/fft.c \
	src/framealloc.c \
	src/getopt.c \
	src/interpred.c \
	src/intrapred.c \
	src/kernels_api.c \
	src/main.c \
	src/mathfunc.c \
	src/nalu.c \
	src/octave.c \
	src/parset.c \
	src/read_trace.c \
	src/residual.c \
	src/sim_environs.c \
	src/slice.c \
	src/timer.c \
	src/viterbi_flat.c \
	src/viterbi_parms.c \
	src/vlc.c

SRC_T = $(SRC) src/read_trace.c
SRC_S = $(SRC) src/sim_environs.c

HDR = $(wildcard include/*.h)
OBJ_T = $(SRC_T:src/%.c=obj_t/%.o)
OBJ_S = $(SRC_S:src/%.c=obj_s/%.o)

VPATH = ./src

TARGET_BASE = mini-era
ifdef CONFIG_FFT_EN
 ifdef CONFIG_VITERBI_EN
  TARGET_T +=$(TARGET_BASE)_fft$(CONFIG_WHICH_FFT_TYPE)_vit
 else
  TARGET_T +=$(TARGET_BASE)_fft$(CONFIG_WHICH_FFT_TYPE)
 endif
else
 ifdef CONFIG_VITERBI_EN
  TARGET_T =$(TARGET_BASE)_vit
 else
  TARGET_T = $(TARGET_BASE)
 endif
endif
TARGET_S = sim-$(TARGET_T)

ALL_OBJ_DIR=obj_t obj_s

all: esp-libs $(ALL_OBJ_DIR) $(TARGET_T) $(TARGET_S)

ifdef COMPILE_TO_ESP
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

esp-libs: esp-build
	  CROSS_COMPILE=$(CROSS_COMPILE) DRIVERS=$(ESP_DRV_LINUX) $(MAKE) -C $(ESP_BUILD_DRIVERS)/contig_alloc/ libcontig.a
	  cd $(ESP_BUILD_DRIVERS)/test; CROSS_COMPILE=$(CROSS_COMPILE) BUILD_PATH=$$PWD $(MAKE) -C $(ESP_DRV_LINUX)/test
	  cd $(ESP_BUILD_DRIVERS)/libesp; CROSS_COMPILE=$(CROSS_COMPILE) BUILD_PATH=$$PWD $(MAKE) -C $(ESP_DRV_LINUX)/libesp
	  cd $(ESP_BUILD_DRIVERS)/utils; CROSS_COMPILE=$(CROSS_COMPILE) BUILD_PATH=$$PWD $(MAKE) -C $(ESP_DRV_LINUX)/utils
else
esp-libs:

endif
.PHONY: esp-build-clean esp-build-distclean esp-libs


obj_t/%.o: %.c
	$(CROSS_COMPILE)$(CC) $(CFLAGS) -c $< -o $@

obj_s/%.o: %.c
	$(CROSS_COMPILE)$(CC) $(CFLAGS) -DUSE_SIM_ENVIRON -c $< -o $@

$(TARGET_T): obj_t $(TARGET_T).exe
	echo "Done building $@"

$(TARGET_S): obj_s $(TARGET_S).exe
	echo "Done building $@"


$(TARGET_T).exe: $(OBJ_T)
	$(CROSS_COMPILE)$(CC) $(LDLIBS) $^ -o $@ $(LDFLAGS)

$(TARGET_S).exe: $(OBJ_S)
	$(CROSS_COMPILE)$(CC) $(LDLIBS) $^ -o $@ $(LDFLAGS)

clean:
	$(RM) $(OBJ_T) $(OBJ_S)

clobber: clean
	$(RM) $(TARGET_T).exe $(TARGET_S).exe
	$(RM) -r obj_t obj_s


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
