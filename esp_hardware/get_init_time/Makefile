
CPU ?= ariane
ARCH ?= riscv
CROSS_COMPILE ?= riscv64-unknown-linux-gnu-

ESP_ROOT ?= ../../esp
ESP_DRIVERS ?= $(ESP_ROOT)/soft/$(CPU)/drivers

CC = gcc -std=c99


INCDIR ?=
INCDIR += -I./include
INCDIR += -I$(ESP_DRIVERS)/include

CFLAGS ?= -O2
CFLAGS += $(INCDIR)
CFLAGS += -DINT_TIME

#CFLAGS += -DVERBOSE

LDLIBS ?=
LDLIBS += -L$(ESP_DRIVERS)/contig_alloc
LDLIBS += -L$(ESP_DRIVERS)/test
LDLIBS += -L$(ESP_DRIVERS)/libesp

LDFLAGS ?=
LDFLAGS += -lm
LDFLAGS += -lrt
LDFLAGS += -lpthread
LDFLAGS += -lesp
LDFLAGS += -ltest
LDFLAGS += -lcontig

SRC_T = $(foreach f, $(wildcard src/*.c), $(shell basename $(f)))
SRC_D = $(wildcard src/*.c)
HDR_T = $(wildcard include/*.h)
OBJ_T = $(SRC_T:%.c=obj_t/%.o)

VPATH = ./src

TARGET=get_init_time.exe

all: obj_t $(TARGET)

obj_t/%.o: %.c
	$(CROSS_COMPILE)$(CC) $(CFLAGS) -c $< -o $@

obj_s/%.o: %.c
	$(CROSS_COMPILE)$(CC) $(CFLAGS) -DUSE_SIM_ENVIRON -c $< -o $@

$(OBJ_T): $(HDR_T)

$(TARGET): $(OBJ_T)
	CROSS_COMPILE=$(CROSS_COMPILE) $(MAKE) -C $(ESP_DRIVERS)/contig_alloc/ libcontig.a
	CROSS_COMPILE=$(CROSS_COMPILE) $(MAKE) -C $(ESP_DRIVERS)/test
	CROSS_COMPILE=$(CROSS_COMPILE) $(MAKE) -C $(ESP_DRIVERS)/libesp
	$(CROSS_COMPILE)$(CC) $(LDLIBS) $^ -o $@ $(LDFLAGS)


clean:
	$(RM) $(OBJ_T) $(TARGET)
	$(RM) -r obj_t

clobber: clean


obj_t:
	mkdir $@

.PHONY: all clean


#depend:;	makedepend -fMakefile -- $(CFLAGS) -- $(SRC_D)
# DO NOT DELETE THIS LINE -- make depend depends on it.

