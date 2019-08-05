CC = gcc -std=c99

CFLAGS = -pedantic -Wall -O0 -g
INCLUDES =  
LFLAGS = -L viterbi
LIBS = -lviterbi

TARGET = main
OBJECTS = kernels_api.o

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) $(CFLAGS) $(INCLUDES) -o $(TARGET).exe $(TARGET).c $(LFLAGS) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	$(RM) $(TARGET).exe $(OBJECTS)
