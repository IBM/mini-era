include ./.config

CPU ?= ariane
ARCH ?= riscv
ifdef DO_CROSS_COMPILATION
CROSS_COMPILE ?= riscv64-unknown-linux-gnu-
endif

ifdef COMPILE_TO_ESP
ESP_ROOT ?= ../../esp
ESP_DRIVERS ?= $(ESP_ROOT)/soft/$(CPU)/drivers
endif

CC = gcc -std=c99


INCDIR ?=
INCDIR += -I./include
ifdef COMPILE_TO_ESP
INCDIR += -I$(ESP_DRIVERS)/include
endif

CFLAGS ?= -O2
CFLAGS += $(INCDIR)
CFLAGS += -DINT_TIME
ifdef COMPILE_TO_ESP
CFLAGS += -DCOMPILE_TO_ESP
endif
#  -- ALWAYS use this one! --   ifdef CONFIG_ESP_INTERFACE
CFLAGS += -DUSE_ESP_INTERFACE
#   endif
ifdef CONFIG_FFT_EN
CFLAGS += -DHW_FFT
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
LDLIBS += -L$(ESP_DRIVERS)/contig_alloc
LDLIBS += -L$(ESP_DRIVERS)/test
LDLIBS += -L$(ESP_DRIVERS)/libesp
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

SRC_T = $(foreach f, $(wildcard src/*.c), $(shell basename $(f)))
SRC_D = $(wildcard src/*.c)
HDR_T = $(wildcard include/*.h)
OBJ_T = $(SRC_T:%.c=obj_t/%.o)
OBJ_S = $(SRC_T:%.c=obj_s/%.o)

VPATH = ./src

TARGET=mini-era.exe
STARGET=mini-era-sim.exe

all: obj_t obj_s $(TARGET) $(STARGET)

obj_t/%.o: %.c
	$(CROSS_COMPILE)$(CC) $(CFLAGS) -c $< -o $@

obj_s/%.o: %.c
	$(CROSS_COMPILE)$(CC) $(CFLAGS) -DUSE_SIM_ENVIRON -c $< -o $@

$(OBJ_T): $(HDR_T)

$(TARGET): $(OBJ_T)
ifdef COMPILE_TO_ESP
	CROSS_COMPILE=$(CROSS_COMPILE) $(MAKE) -C $(ESP_DRIVERS)/contig_alloc/ libcontig.a
	CROSS_COMPILE=$(CROSS_COMPILE) $(MAKE) -C $(ESP_DRIVERS)/test
	CROSS_COMPILE=$(CROSS_COMPILE) $(MAKE) -C $(ESP_DRIVERS)/libesp
endif
	$(CROSS_COMPILE)$(CC) $(LDLIBS) $^ -o $@ $(LDFLAGS)

$(STARGET): $(OBJ_S)
ifdef COMPILE_TO_ESP
	CROSS_COMPILE=$(CROSS_COMPILE) $(MAKE) -C $(ESP_DRIVERS)/contig_alloc/ libcontig.a
	CROSS_COMPILE=$(CROSS_COMPILE) $(MAKE) -C $(ESP_DRIVERS)/test
	CROSS_COMPILE=$(CROSS_COMPILE) $(MAKE) -C $(ESP_DRIVERS)/libesp
endif
	$(CROSS_COMPILE)$(CC) $(LDLIBS) $^ -o $@ $(LDFLAGS)

clean:
	$(RM) $(OBJ_T) $(OBJ_S) $(TARGET) $(STARGET)
	$(RM) -r obj_t obj_s

clobber: clean


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
