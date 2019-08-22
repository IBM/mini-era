CC = gcc

CFLAGS = -pedantic -Wall -O0 -g
#CFLAGS += -L/usr/lib/python2.7/config-x86_64-linux-gnu -L/usr/lib -lpython2.7 -lpthread -ldl  -lutil -lm  -Xlinker -export-dynamic -Wl,-O1 -Wl,-Bsymbolic-functions
CFLAGS +=  -Xlinker -export-dynamic
#UNCOMMENT FOR DEBUG-MESSAGES:
CFLAGS += -DVERBOSE 

INCLUDES =  
LFLAGS = -Lviterbi -Lradar 
#LFLAGS += 
#LIBS = -lviterbi -lfmcwdist -lpthread -ldl -lutil -lm -lpython2.7
LIBS = -lviterbi -lfmcwdist -lpthread -ldl -lutil -lm 

TARGET = main
SRC = kernels_api.c
OBJ = $(SRC:%.c=%.o)

T_SRC 	= sim_environs.c
T_OBJ	= $(T_SRC:%.c=%.o)

G_SRC 	= gen_trace.c
G_OBJ	= $(G_SRC:%.c=%.o)

$(TARGET): $(OBJ) libviterbi libfmcwdist
	$(CC) $(OBJ) $(CFLAGS) $(INCLUDES) -o $(TARGET).exe $(TARGET).c $(LFLAGS) $(LIBS)


all: $(TARGET) test tracegen


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
	$(RM) $(TARGET).exe $(OBJ)
	$(RM) test  $(T_OBJ)
	$(RM) tracegen  $(G_OBJ)

allclean: clean
	cd radar; make clean
	cd viterbi; make clean

kernels_api.o: kernels_api.h 
kernels_api.o: viterbi/utils.h viterbi/viterbi_decoder_generic.h
kernels_api.o: radar/calc_fmcw_dist.h

sim_environs.o: sim_environs.h
sim_environs.o: viterbi/utils.h viterbi/viterbi_decoder_generic.h viterbi/base.h

gen_trace.o: gen_trace.h


depend:;	makedepend -fMakefile -- $(CFLAGS) -- $(SRC) $(T_SRC) $(G_SRC)
# DO NOT DELETE THIS LINE -- make depend depends on it.

