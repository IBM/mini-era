#INPUT_SIZE = SMALL
#INPUT_SIZE = MEDIUM
INPUT_SIZE = LARGE

CC = gcc

CFLAGS = -pedantic -Wall -O0 -g -DINPUT_SIZE=INPUT_SIZE_$(INPUT_SIZE)
INCLUDES =
LFLAGS =
LIBS = -lm

TARGET = main
OBJECTS = fft.o octave.o timer.o
LLVM_OBJECTS =  $(addprefix .,$(OBJECTS:=.bc)) $(addprefix .,$(TARGET:=.o.bc))


all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) $(CFLAGS) $(INCLUDES) -o $(TARGET).exe $(TARGET).c $(LFLAGS) $(LIBS)

%.o: %.c
	export LLVM_COMPILER=clang && \
	$(CC) $(CFLAGS) -o $@ -c $<

llvm: $(OBJECTS)
	export LLVM_COMPILER=clang && \
	wllvm $(OBJECTS) $(CFLAGS) $(INCLUDES) -o $(TARGET)-llvm.exe $(TARGET).c $(LFLAGS) $(LIBS)
	cd llvm
	extract-bc $(TARGET)-llvm.exe -o llvm/$(TARGET).bc

needle:
	$(MAKE) llvm CC=wllvm
	make -C llvm


clean:
	$(RM) $(TARGET)*.exe $(OBJECTS) $(LLVM_OBJECTS) *.mat
	make clean -C llvm/
