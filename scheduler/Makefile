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

CC = gcc

INCDIR ?=
INCDIR += -I./include
ifdef COMPILE_TO_ESP
INCDIR += -I$(ESP_DRIVERS)/include
endif

CFLAGS ?= -O3
CFLAGS += $(INCDIR)
CLFAGS += -std=c99
CFLAGS += -DINT_TIME
ifdef COMPILE_TO_ESP
CFLAGS += COMPILE_TO_ESP
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

SRC_S = $(foreach f, $(wildcard src/*.c), $(shell basename $(f)))
HDR_S = $(wildcard include/*.h)
OBJ_S = $(SRC_S:.c=.o)

VPATH = ./src

TARGET=test-scheduler.exe

all: $(TARGET)

%.o: %.c
	$(CROSS_COMPILE)$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_S): $(HDR_S)

$(TARGET): $(OBJ_S)
ifdef COMPILE_TO_ESP
	CROSS_COMPILE=$(CROSS_COMPILE) $(MAKE) -C $(ESP_DRIVERS)/contig_alloc/ libcontig.a
	CROSS_COMPILE=$(CROSS_COMPILE) $(MAKE) -C $(ESP_DRIVERS)/test
	CROSS_COMPILE=$(CROSS_COMPILE) $(MAKE) -C $(ESP_DRIVERS)/libesp
endif
	$(CROSS_COMPILE)$(CC) $(LDLIBS) $^ -o $@ $(LDFLAGS)

clean:
	$(RM) $(OBJ_S) $(TARGET)

.PHONY: all clean
