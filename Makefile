CC = gcc -std=c99

CFLAGS = -pedantic -Wall -O0 -g
#CFLAGS += -L/usr/lib/python2.7/config-x86_64-linux-gnu -L/usr/lib -lpython2.7 -lpthread -ldl  -lutil -lm  -Xlinker -export-dynamic -Wl,-O1 -Wl,-Bsymbolic-functions
CFLAGS +=  -Xlinker -export-dynamic
#UNCOMMENT FOR DEBUG-MESSAGES:
CFLAGS += -DVERBOSE 

INCLUDES =  
LFLAGS = -Lviterbi -Lradar 
#LFLAGS += 
LIBS = -lviterbi -lfmcwdist -lpthread -ldl -lutil -lm -lpython2.7

TARGET = main
OBJECTS = kernels_api.o

T_SRC 	= sim_environs.c
T_OBJ	= $(T_SRC:%.c=%.o)

G_SRC 	= gen_trace.c
G_OBJ	= $(G_SRC:%.c=%.o)

all: $(TARGET) 

$(TARGET): $(OBJECTS) libviterbi libfmcwdist
	$(CC) $(OBJECTS) $(CFLAGS) $(INCLUDES) -o $(TARGET).exe $(TARGET).c $(LFLAGS) $(LIBS)


test: $(T_OBJ) test.c
	$(CC) $(T_OBJ) $(CFLAGS) $(INCLUDES) -o $@ test.c $(LFLAGS) $(LIBS)

tracegen: $(G_OBJ)
	$(CC) $(G_OBJ) $(CFLAGS) $(INCLUDES) -o $@ 

libviterbi: 
	cd viterbi; make

libfmcwdist:
	cd radar; make


%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<


clean:
	$(RM) $(TARGET).exe $(OBJECTS)

allclean: clean
	cd radar; make clean
	cd viterbi; make clean

