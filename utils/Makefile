CC = gcc -std=c99

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
OBJ_DBG_DIR = obj_dbg
OBJVDIR = objv
OBJV_DBG_DIR = objv_dbg

T_SRC 	= sim_environs.c gen_trace.c
T_OBJ	= $(T_SRC:%.c=$(OBJDIR)/%.o)
T_OBJ_DBG = $(T_SRC:%.c=$(OBJ_DBG_DIR)/%.o)
T_OBJV	= $(T_SRC:%.c=$(OBJVDIR)/%.o)
T_OBJV_DBG = $(T_SRC:%.c=$(OBJV_DBG_DIR)/%.o)


all: tracegen debug_tracegen  vistracegen debug_vistracegen 


tracegen: $(OBJDIR) $(T_OBJ)
	$(CC) $(T_OBJ) $(CFLAGS) $(INCLUDES) -o $@ $(LFLAGS) $(LIBS)

debug_tracegen: $(OBJ_DBG_DIR) $(T_OBJ_DBG)
	$(CC) $(T_OBJ_DBG) $(CFLAGS) $(INCLUDES) -o $@ $(LFLAGS) $(LIBS)

vistracegen: $(OBJVDIR) $(T_OBJV)
	$(CC) $(T_OBJV) $(CFLAGS) $(INCLUDES) -o $@ $(LFLAGS) $(LIBS)

debug_vistracegen: $(OBJV_DBG_DIR) $(T_OBJV_DBG)
	$(CC) $(T_OBJV_DBG) $(CFLAGS) $(INCLUDES) -o $@ $(LFLAGS) $(LIBS)

obj:
	mkdir obj

obj_dbg:
	mkdir obj_dbg


objv:
	mkdir objv

objv_dbg:
	mkdir objv_dbg


obj/%.o: %.c
	$(CC) $(CFLAGS) $(PYTHONINCLUDES) -o $@ $(PYTHONLIBS) -c $<

obj_dbg/%.o: %.c
	$(CC) $(CFLAGS) $(PYTHONINCLUDES) -DVERBOSE -o $@ $(PYTHONLIBS) -c $<


objv/%.o: %.c
	$(CC) $(CFLAGS) $(PYTHONINCLUDES) -DFOR_VIS -o $@ $(PYTHONLIBS) -c $<

objv_dbg/%.o: %.c
	$(CC) $(CFLAGS) $(PYTHONINCLUDES) -DFOR_VIS -DVERBOSE -o $@ $(PYTHONLIBS) -c $<

clean:
	$(RM) tracegen           $(T_OBJ)
	$(RM) vistracegen        $(T_OBJV)
	$(RM) debug_tracegen     $(T_OBJ_DBG)
	$(RM) debug_vistracegen  $(T_OBJV_DBG)

allclean: clean
	$(RM) -rf $(OBJDIR)
	$(RM) -rf $(OBJ_DBG_DIR)

obj/sim_environs.o: sim_environs.h

obj_dbg/sim_environs.o: sim_environs.h

depend:;	makedepend -fMakefile -- $(CFLAGS) -- $(SRC) $(T_SRC) $(G_SRC)
# DO NOT DELETE THIS LINE -- make depend depends on it.

