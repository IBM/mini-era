CC = gcc -std=c99

CFLAGS = -pedantic -Wall -O0 -g
INCLUDES =  
LFLAGS = -L viterbi
LIBS = -lviterbi

TARGET = main
OBJECTS = kernels_api.o

T_SRC 	= sim_environs.c
T_OBJ	= $(T_SRC:%.c=%.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) $(CFLAGS) $(INCLUDES) -o $(TARGET).exe $(TARGET).c $(LFLAGS) $(LIBS)


test: $(T_OBJ) test.c
	$(CC) $(T_OBJ) $(CFLAGS) $(INCLUDES) -o test test.c $(LFLAGS) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	$(RM) $(TARGET).exe $(OBJECTS)
