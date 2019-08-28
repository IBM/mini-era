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

OBJDIR = obj
OBJ_V_DIR = obj_v

T_SRC 	= sim_environs.c
T_OBJ	= $(T_SRC:%.c=$(OBJDIR)/%.o)
T_OBJ_V = $(T_SRC:%.c=$(OBJ_V_DIR)/%.o)

G_SRC 	= gen_trace.c
G_OBJ	= $(G_SRC:%.c=obj/%.o)

all: test vtest tracegen


test: $(OBJDIR) $(T_OBJ) test.c
	$(CC) $(T_OBJ) $(CFLAGS) $(INCLUDES) -o $@ test.c $(LFLAGS) $(LIBS)

vtest: $(OBJ_V_DIR) $(T_OBJ_V) test.c
	$(CC) $(T_OBJ_V) $(CFLAGS) $(INCLUDES) -o $@ test.c $(LFLAGS) $(LIBS)

tracegen: $(OBJDIR) $(G_OBJ)
	$(CC) $(G_OBJ) $(CFLAGS) $(INCLUDES) -o $@ 

obj:
	mkdir obj

obj_v:
	mkdir obj_v


obj/%.o: %.c
	$(CC) $(CFLAGS) $(PYTHONINCLUDES) -o $@ $(PYTHONLIBS) -c $<

obj_v/%.o: %.c
	$(CC) $(CFLAGS) $(PYTHONINCLUDES) -DVERBOSE -o $@ $(PYTHONLIBS) -c $<

clean:
	$(RM) test   $(T_OBJ)
	$(RM) vtest  $(T_OBJ_V)
	$(RM) tracegen  $(G_OBJ)

allclean: clean
	$(RM) -rf $(OBJDIR)
	$(RM) -rf $(OBJ_V_DIR)

obj/sim_environs.o: sim_environs.h

obj_v/sim_environs.o: sim_environs.h

obj/gen_trace.o: gen_trace.h


depend:;	makedepend -fMakefile -- $(CFLAGS) -- $(SRC) $(T_SRC) $(G_SRC)
# DO NOT DELETE THIS LINE -- make depend depends on it.

