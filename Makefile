CC = gcc -std=c99

CFLAGS = -pedantic -Wall -O0 -g
#CFLAGS += -L/usr/lib/python2.7/config-x86_64-linux-gnu -L/usr/lib -lpython2.7 -lpthread -ldl  -lutil -lm  -Xlinker -export-dynamic -Wl,-O1 -Wl,-Bsymbolic-functions
CFLAGS += -lpthread -ldl -lutil -lm -lpython2.7 -Xlinker -export-dynamic

INCLUDES =  
LFLAGS = -L viterbi
LIBS = -lviterbi

TARGET = main
OBJECTS = kernels_api.o

T_SRC 	= sim_environs.c
T_OBJ	= $(T_SRC:%.c=%.o)

all: $(TARGET) 

$(TARGET): $(OBJECTS) libviterbi
	$(CC) $(OBJECTS) $(CFLAGS) $(INCLUDES) -o $(TARGET).exe $(TARGET).c $(LFLAGS) $(LIBS)


test: $(T_OBJ) test.c
	$(CC) $(T_OBJ) $(CFLAGS) $(INCLUDES) -o test test.c $(LFLAGS) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

libviterbi: 
	cd viterbi; make

clean:
	$(RM) $(TARGET).exe $(OBJECTS)
