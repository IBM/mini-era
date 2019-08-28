CC = gcc

CFLAGS = -pedantic -Wall -O0 -g
##CFLAGS += -L/usr/lib/python2.7/config-x86_64-linux-gnu -L/usr/lib -lpython2.7 -lpthread -ldl  -lutil -lm  -Xlinker -export-dynamic -Wl,-O1 -Wl,-Bsymbolic-functions
#CFLAGS +=  -Xlinker -export-dynamic

INCLUDES =  
#PYTHONINCLUDES = -I/usr/include/python3.6m
#LFLAGS = -Lviterbi -Lradar 
##LFLAGS += 
##LIBS = -lviterbi -lfmcwdist -lpthread -ldl -lutil -lm -lpython2.7
#LIBS = -lviterbi -lfmcwdist -lpthread -ldl -lutil -lm 
LIBS = -lm
#PYTHONLIBS = -lpython3.6m

T_SRC 	= sim_environs.c
T_OBJ	= $(T_SRC:%.c=obj/%.o)
T_OBJ_V = $(T_SRC:%.c=obj_v/%.o)

G_SRC 	= gen_trace.c
G_OBJ	= $(G_SRC:%.c=obj/%.o)

all: test vtest tracegen


test: $(T_OBJ) test.c
	$(CC) $(T_OBJ) $(CFLAGS) $(INCLUDES) -o $@ test.c $(LFLAGS) $(LIBS)

vtest: $(T_OBJ_V) test.c
	$(CC) $(T_OBJ_V) $(CFLAGS) $(INCLUDES) -o $@ test.c $(LFLAGS) $(LIBS)

tracegen: $(G_OBJ)
	$(CC) $(G_OBJ) $(CFLAGS) $(INCLUDES) -o $@ 

obj/%.o: %.c
	$(CC) $(CFLAGS) $(PYTHONINCLUDES) -o $@ $(PYTHONLIBS) -c $<

obj_v/%.o: %.c
	$(CC) $(CFLAGS) $(PYTHONINCLUDES) -DVERBOSE -o $@ $(PYTHONLIBS) -c $<

clean:
	$(RM) test   $(T_OBJ)
	$(RM) vtest  $(T_OBJ_V)
	$(RM) tracegen  $(G_OBJ)


obj/sim_environs.o: sim_environs.h

obj_v/sim_environs.o: sim_environs.h

obj/gen_trace.o: gen_trace.h


depend:;	makedepend -fMakefile -- $(CFLAGS) -- $(SRC) $(T_SRC) $(G_SRC)
# DO NOT DELETE THIS LINE -- make depend depends on it.

