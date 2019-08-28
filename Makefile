CC = gcc

CFLAGS = -pedantic -Wall -O0 -g
#CFLAGS += -L/usr/lib/python2.7/config-x86_64-linux-gnu -L/usr/lib -lpython2.7 -lpthread -ldl  -lutil -lm  -Xlinker -export-dynamic -Wl,-O1 -Wl,-Bsymbolic-functions
CFLAGS +=  -Xlinker -export-dynamic

INCLUDES =  
PYTHONINCLUDES = -I/usr/include/python3.6m
LFLAGS = -Lviterbi -Lradar 
#LFLAGS += 
#LIBS = -lviterbi -lfmcwdist -lpthread -ldl -lutil -lm -lpython2.7
LIBS = -lviterbi -lfmcwdist -lpthread -ldl -lutil -lm 
PYTHONLIBS = -lpython3.6m
# The full mini-era target, etc.
TARGET = main
SRC = kernels_api.c main.c
OBJ = $(SRC:%.c=obj/%.o)
OBJ_V = $(SRC:%.c=obj_v/%.o)

# The C-code only target (it bypasses KERAS Python code)
C_TARGET = cmain
C_OBJ = $(SRC:%.c=obj_c/%.o)
C_OBJ_V = $(SRC:%.c=obj_cv/%.o)

T_SRC 	= sim_environs.c
T_OBJ	= $(T_SRC:%.c=obj/%.o)
T_OBJ_V = $(T_SRC:%.c=obj_v/%.o)

G_SRC 	= utils/gen_trace.c
G_OBJ	= $(G_SRC:%.c=obj/%.o)

$(TARGET): $(OBJ) libviterbi libfmcwdist
	$(CC) $(OBJ) $(CFLAGS) $(INCLUDES) $(PYTHONINCLUDES) -o $@.exe $(LFLAGS) $(LIBS) $(PYTHONLIBS)

$(C_TARGET): $(C_OBJ) libviterbi libfmcwdist
	$(CC) $(C_OBJ) $(CFLAGS) $(INCLUDES) $(PYTHONINCLUDES) -o $@.exe $(LFLAGS) $(LIBS) $(PYTHONLIBS)

v$(TARGET): $(OBJ_V) libviterbi libfmcwdist
	$(CC) $(OBJ_V) $(CFLAGS) $(INCLUDES) $(PYTHONINCLUDES) -o $@.exe $(LFLAGS) $(LIBS) $(PYTHONLIBS)

v$(C_TARGET): $(C_OBJ_V) libviterbi libfmcwdist
	$(CC) $(C_OBJ_V) $(CFLAGS) $(INCLUDES) $(PYTHONINCLUDES) -o $@.exe $(LFLAGS) $(LIBS) $(PYTHONLIBS)


all: $(TARGET) $(C_TARGET) v$(TARGET) v$(C_TARGET) test vtest tracegen


test: $(T_OBJ) test.c
	$(CC) $(T_OBJ) $(CFLAGS) $(INCLUDES) -o $@ test.c $(LFLAGS) $(LIBS)

vtest: $(T_OBJ_V) test.c
	$(CC) $(T_OBJ_V) $(CFLAGS) $(INCLUDES) -o $@ test.c $(LFLAGS) $(LIBS)

tracegen: $(G_OBJ)
	$(CC) $(G_OBJ) $(CFLAGS) $(INCLUDES) -o $@ 

libviterbi:
	cd viterbi; make

libfmcwdist:
	cd radar; make


obj/%.o: %.c
	$(CC) $(CFLAGS) $(PYTHONINCLUDES) -o $@ $(PYTHONLIBS) -c $<

obj_v/%.o: %.c
	$(CC) $(CFLAGS) $(PYTHONINCLUDES) -DVERBOSE -o $@ $(PYTHONLIBS) -c $<

obj_c/%.o: %.c
	$(CC) $(CFLAGS) $(PYTHONINCLUDES) -DVERBOSE -DBYPASS_KERAS_CV_CODE -o $@ $(PYTHONLIBS) -c $<

obj_cv/%.o: %.c
	$(CC) $(CFLAGS) $(PYTHONINCLUDES) -DVERBOSE -DBYPASS_KERAS_CV_CODE -o $@ $(PYTHONLIBS) -c $<

clean:
	$(RM) $(TARGET).exe $(OBJ)
	$(RM) $(C_TARGET).exe $(C_OBJ)
	$(RM) v$(TARGET).exe $(OBJ_V)
	$(RM) v$(C_TARGET).exe $(C_OBJ_V)
	$(RM) test  $(T_OBJ)
	$(RM) tracegen  $(G_OBJ)

allclean: clean
	cd radar; make clean
	cd viterbi; make clean

obj/kernels_api.o: kernels_api.h 
obj/kernels_api.o: viterbi/utils.h viterbi/viterbi_decoder_generic.h
obj/kernels_api.o: radar/calc_fmcw_dist.h

obj_c/kernels_api.o: kernels_api.h 
obj_c/kernels_api.o: viterbi/utils.h viterbi/viterbi_decoder_generic.h
obj_c/kernels_api.o: radar/calc_fmcw_dist.h

objv/kernels_api.o: kernels_api.h 
objv/kernels_api.o: viterbi/utils.h viterbi/viterbi_decoder_generic.h
objv/kernels_api.o: radar/calc_fmcw_dist.h

obj_cv/kernels_api.o: kernels_api.h 
obj_cv/kernels_api.o: viterbi/utils.h viterbi/viterbi_decoder_generic.h
obj_cv/kernels_api.o: radar/calc_fmcw_dist.h

obj/sim_environs.o: sim_environs.h

obj_v/sim_environs.o: sim_environs.h

#obj/sim_environs.o: viterbi/utils.h viterbi/viterbi_decoder_generic.h viterbi/base.h

obj/gen_trace.o: gen_trace.h


depend:;	makedepend -fMakefile -- $(CFLAGS) -- $(SRC) $(T_SRC) $(G_SRC)
# DO NOT DELETE THIS LINE -- make depend depends on it.

