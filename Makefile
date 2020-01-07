CC = gcc

COPTF0 = -O0
COPTF2 = -O2
COPTF3 = -O3

CFLAGS = -pedantic -Wall -g $(COPTF2)
#CFLAGS += -L/usr/lib/python2.7/config-x86_64-linux-gnu -L/usr/lib -lpython2.7 -lpthread -ldl  -lutil -lm  -Xlinker -export-dynamic -Wl,-O1 -Wl,-Bsymbolic-functions
CFLAGS +=  -Xlinker -export-dynamic

INCLUDES =  
#PYTHONINCLUDES = $(shell /usr/bin/python-config --cflags)
PYTHONINCLUDES = -I/usr/include/python3.6m
LFLAGS = -Lviterbi -Lradar 
#LFLAGS += 
#LIBS = -lviterbi -lfmcwdist -lpthread -ldl -lutil -lm -lpython2.7
LIBS = -lviterbi -lfmcwdist -lpthread -ldl -lutil -lm 
FLIBS = -lviterbiF -lfmcwdist -lpthread -ldl -lutil -lm 
#PYTHONLIBS = $(shell /usr/bin/python-config --ldflags)
PYTHONLIBS = -lpython3.6m

OBJDIR = obj
C_OBJDIR = obj_c
FC_OBJDIR = obj_fc

OBJ_V_DIR = obj_v
C_OBJ_V_DIR = obj_cv
FC_OBJ_V_DIR = obj_fcv


S_OBJDIR = objs
C_S_OBJDIR = objs_c
FC_S_OBJDIR = objs_fc

S_OBJ_V_DIR = objs_v
C_S_OBJ_V_DIR = objs_cv
FC_S_OBJ_V_DIR = objs_fcv


# The full mini-era target, etc.
TARGET = main
SRC = kernels_api.c main.c
SRC2 = kernels_api.c tmain.c
T_SRC = $(SRC) read_trace.c
OBJ = $(T_SRC:%.c=$(OBJDIR)/%.o)
OBJ_V = $(T_SRC:%.c=$(OBJ_V_DIR)/%.o)

T_TARGET = t-main
T_SRC2 = $(SRC2) read_trace.c
OBJ2 = $(T_SRC2:%.c=$(OBJDIR)/%.o)
OBJ2_V = $(T_SRC2:%.c=$(OBJ_V_DIR)/%.o)

S_TARGET = sim_main
S_SRC = $(SRC) sim_environs.c
S_SRC2 = $(SRC2) sim_environs.c
S_OBJ = $(S_SRC:%.c=$(S_OBJDIR)/%.o)
S_OBJ_V = $(S_SRC:%.c=$(S_OBJ_V_DIR)/%.o)

TS_TARGET = t-sim_main
TS_SRC = $(SRC) sim_environs.c
TS_OBJ2 = $(S_SRC2:%.c=$(S_OBJDIR)/%.o)
TS_OBJ2_V = $(S_SRC2:%.c=$(S_OBJ_V_DIR)/%.o)

# The C-code only target (it bypasses KERAS Python code)
C_TARGET = cmain
C_OBJ = $(T_SRC:%.c=$(C_OBJDIR)/%.o)
C_OBJ_V = $(T_SRC:%.c=$(C_OBJ_V_DIR)/%.o)
FC_OBJ = $(T_SRC:%.c=$(FC_OBJDIR)/%.o)
FC_OBJ_V = $(T_SRC:%.c=$(FC_OBJ_V_DIR)/%.o)

TC_TARGET = t-cmain
TC_OBJ = $(T_SRC2:%.c=$(C_OBJDIR)/%.o)
TC_OBJ_V = $(T_SRC2:%.c=$(C_OBJ_V_DIR)/%.o)
TFC_OBJ = $(T_SRC2:%.c=$(FC_OBJDIR)/%.o)
TFC_OBJ_V = $(T_SRC2:%.c=$(FC_OBJ_V_DIR)/%.o)

C_S_TARGET = csim_main
C_S_OBJ = $(S_SRC:%.c=$(C_S_OBJDIR)/%.o)
C_S_OBJ_V = $(S_SRC:%.c=$(C_S_OBJ_V_DIR)/%.o)
FC_S_OBJ = $(S_SRC:%.c=$(FC_S_OBJDIR)/%.o)
FC_S_OBJ_V = $(S_SRC:%.c=$(FC_S_OBJ_V_DIR)/%.o)

TC_S_TARGET = t-csim_main
TC_S_OBJ = $(S_SRC2:%.c=$(C_S_OBJDIR)/%.o)
TC_S_OBJ_V = $(S_SRC2:%.c=$(C_S_OBJ_V_DIR)/%.o)
TFC_S_OBJ = $(S_SRC2:%.c=$(FC_S_OBJDIR)/%.o)
TFC_S_OBJ_V = $(S_SRC2:%.c=$(FC_S_OBJ_V_DIR)/%.o)

TRGN_SRC = sim_environs.c
TRGN_OBJ = $(TRGN_SRC:%.c=obj/%.o)
TRGN_OBJ_V = $(TRGN_SRC:%.c=obj_v/%.o)

G_SRC 	= utils/gen_trace.c
G_OBJ	= $(G_SRC:%.c=obj/%.o)

$(TARGET): $(OBJDIR)  $(OBJ) libviterbi libfmcwdist
	$(CC) $(OBJ) $(CFLAGS) $(INCLUDES) $(PYTHONINCLUDES) -o $@.exe $(LFLAGS) $(LIBS) $(PYTHONLIBS)

$(T_TARGET): $(OBJDIR)  $(OBJ2) libviterbi libfmcwdist
	$(CC) $(OBJ2) $(CFLAGS) $(INCLUDES) $(PYTHONINCLUDES) -o $@.exe $(LFLAGS) $(LIBS) $(PYTHONLIBS)

$(C_TARGET): $(C_OBJDIR) $(C_OBJ) libviterbi libfmcwdist
	$(CC) $(C_OBJ) $(CFLAGS) $(INCLUDES) $(PYTHONINCLUDES) -o $@.exe $(LFLAGS) $(LIBS) $(PYTHONLIBS)

f$(C_TARGET): $(FC_OBJDIR) $(FC_OBJ) libviterbiF libfmcwdist
	$(CC) $(FC_OBJ) $(CFLAGS) $(INCLUDES) $(PYTHONINCLUDES) -o $@.exe $(LFLAGS) $(FLIBS) $(PYTHONLIBS)

$(TC_TARGET): $(C_OBJDIR) $(C_OBJ2) libviterbi libfmcwdist
	$(CC) $(C_OBJ2) $(CFLAGS) $(INCLUDES) $(PYTHONINCLUDES) -o $@.exe $(LFLAGS) $(LIBS) $(PYTHONLIBS)

f$(TC_TARGET): $(FC_OBJDIR) $(FC_OBJ2) libviterbiF libfmcwdist
	$(CC) $(FC_OBJ2) $(CFLAGS) $(INCLUDES) $(PYTHONINCLUDES) -o $@.exe $(LFLAGS) $(FLIBS) $(PYTHONLIBS)

v$(TARGET): $(OBJ_V_DIR) $(OBJ_V) libviterbi libfmcwdist
	$(CC) $(OBJ_V) $(CFLAGS) $(INCLUDES) $(PYTHONINCLUDES) -o $@.exe $(LFLAGS) $(LIBS) $(PYTHONLIBS)

v$(T_TARGET): $(OBJ_V_DIR) $(OBJ2_V) libviterbi libfmcwdist
	$(CC) $(OBJ2_V) $(CFLAGS) $(INCLUDES) $(PYTHONINCLUDES) -o $@.exe $(LFLAGS) $(LIBS) $(PYTHONLIBS)

v$(C_TARGET): $(C_OBJ_V_DIR) $(C_OBJ_V) libviterbi libfmcwdist
	$(CC) $(C_OBJ_V) $(CFLAGS) $(INCLUDES) $(PYTHONINCLUDES) -o $@.exe $(LFLAGS) $(LIBS) $(PYTHONLIBS)

vf$(C_TARGET): $(FC_OBJ_V_DIR) $(FC_OBJ_V) libviterbiF libfmcwdist
	$(CC) $(FC_OBJ_V) $(CFLAGS) $(INCLUDES) $(PYTHONINCLUDES) -o $@.exe $(LFLAGS) $(FLIBS) $(PYTHONLIBS)

v$(TC_TARGET): $(C_OBJ_V_DIR) $(C_OBJ2_V) libviterbi libfmcwdist
	$(CC) $(C_OBJ2_V) $(CFLAGS) $(INCLUDES) $(PYTHONINCLUDES) -o $@.exe $(LFLAGS) $(LIBS) $(PYTHONLIBS)

vf$(TC_TARGET): $(FC_OBJ_V_DIR) $(FC_OBJ2_V) libviterbiF libfmcwdist
	$(CC) $(FC_OBJ2_V) $(CFLAGS) $(INCLUDES) $(PYTHONINCLUDES) -o $@.exe $(LFLAGS) $(FLIBS) $(PYTHONLIBS)


$(S_TARGET): $(S_OBJDIR)  $(S_OBJ) libviterbi libfmcwdist
	$(CC) $(S_OBJ) $(CFLAGS) $(INCLUDES) $(PYTHONINCLUDES) -o $@.exe $(LFLAGS) $(LIBS) $(PYTHONLIBS)

$(TS_TARGET): $(S_OBJDIR)  $(S_OBJ2) libviterbi libfmcwdist
	$(CC) $(S_OBJ2) $(CFLAGS) $(INCLUDES) $(PYTHONINCLUDES) -o $@.exe $(LFLAGS) $(LIBS) $(PYTHONLIBS)

$(C_S_TARGET): $(C_S_OBJDIR) $(C_S_OBJ) libviterbi libfmcwdist
	$(CC) $(C_S_OBJ) $(CFLAGS) $(INCLUDES) $(PYTHONINCLUDES) -o $@.exe $(LFLAGS) $(LIBS) $(PYTHONLIBS)

f$(C_S_TARGET): $(FC_S_OBJDIR) $(FC_S_OBJ) libviterbi libfmcwdist
	$(CC) $(FC_S_OBJ) $(CFLAGS) $(INCLUDES) $(PYTHONINCLUDES) -o $@.exe $(LFLAGS) $(FLIBS) $(PYTHONLIBS)

$(TC_S_TARGET): $(C_S_OBJDIR) $(C_S_OBJ2) libviterbi libfmcwdist
	$(CC) $(C_S_OBJ2) $(CFLAGS) $(INCLUDES) $(PYTHONINCLUDES) -o $@.exe $(LFLAGS) $(LIBS) $(PYTHONLIBS)

f$(TC_S_TARGET): $(FC_S_OBJDIR) $(FC_S_OBJ2) libviterbi libfmcwdist
	$(CC) $(FC_S_OBJ2) $(CFLAGS) $(INCLUDES) $(PYTHONINCLUDES) -o $@.exe $(LFLAGS) $(FLIBS) $(PYTHONLIBS)

v$(S_TARGET): $(S_OBJ_V_DIR) $(S_OBJ_V) libviterbi libfmcwdist
	$(CC) $(S_OBJ_V) $(CFLAGS) $(INCLUDES) $(PYTHONINCLUDES) -o $@.exe $(LFLAGS) $(LIBS) $(PYTHONLIBS)

v$(TS_TARGET): $(S_OBJ_V_DIR) $(S_OBJ2_V) libviterbi libfmcwdist
	$(CC) $(S_OBJ2_V) $(CFLAGS) $(INCLUDES) $(PYTHONINCLUDES) -o $@.exe $(LFLAGS) $(LIBS) $(PYTHONLIBS)

v$(C_S_TARGET): $(C_S_OBJ_V_DIR) $(C_S_OBJ_V) libviterbi libfmcwdist
	$(CC) $(C_S_OBJ_V) $(CFLAGS) $(INCLUDES) $(PYTHONINCLUDES) -o $@.exe $(LFLAGS) $(LIBS) $(PYTHONLIBS)

vf$(C_S_TARGET): $(FC_S_OBJ_V_DIR) $(FC_S_OBJ_V) libviterbi libfmcwdist
	$(CC) $(FC_S_OBJ_V) $(CFLAGS) $(INCLUDES) $(PYTHONINCLUDES) -o $@.exe $(LFLAGS) $(FLIBS) $(PYTHONLIBS)

v$(TC_S_TARGET): $(C_S_OBJ_V_DIR) $(C_S_OBJ2_V) libviterbi libfmcwdist
	$(CC) $(C_S_OBJ2_V) $(CFLAGS) $(INCLUDES) $(PYTHONINCLUDES) -o $@.exe $(LFLAGS) $(LIBS) $(PYTHONLIBS)

vf$(TC_S_TARGET): $(FC_S_OBJ_V_DIR) $(FC_S_OBJ2_V) libviterbi libfmcwdist
	$(CC) $(FC_S_OBJ2_V) $(CFLAGS) $(INCLUDES) $(PYTHONINCLUDES) -o $@.exe $(LFLAGS) $(FLIBS) $(PYTHONLIBS)


all: objdirs $(TARGET) $(T_TARGET) $(C_TARGET) f$(C_TARGET) v$(TC_TARGET) f$(TC_TARGET) v$(TARGET) v$(C_TARGET) vf$(C_TARGET) v$(T_TARGET) v$(TC_TARGET) vf$(TC_TARGET) $(S_TARGET) $(C_S_TARGET) f$(C_S_TARGET) v$(S_TARGET) v$(C_S_TARGET) vf$(C_S_TARGET) $(TS_TARGET) $(TC_S_TARGET) f$(TC_S_TARGET) v$(TS_TARGET) v$(TC_S_TARGET) vf$(TC_S_TARGET) util_prog


objdirs: $(OBJDIR) $(C_OBJDIR) $(FC_OBJDIR) $(OBJ_V_DIR) $(C_OBJ_V_DIR) $(FC_OBJ_V_DIR) $(S_OBJDIR) $(C_S_OBJDIR) $(FC_S_OBJDIR) $(S_OBJ_V_DIR) $(C_S_OBJ_V_DIR) $(FC_S_OBJ_V_DIR)


util_prog:
	cd utils; make all

test: 
	cd utils; make test

vtest: 
	cd utils; make vtest

tracegen: 
	cd utils; make tracegen

libviterbi:
	cd viterbi; make lib

libviterbiF:
	cd viterbi; make lib

libfmcwdist:
	cd radar; make

$(OBJDIR):
	mkdir $(OBJDIR)

$(C_OBJDIR):
	mkdir $(C_OBJDIR)

$(FC_OBJDIR):
	mkdir $(FC_OBJDIR)

$(OBJ_V_DIR):
	mkdir $(OBJ_V_DIR)

$(C_OBJ_V_DIR):
	mkdir $(C_OBJ_V_DIR)

$(FC_OBJ_V_DIR):
	mkdir $(FC_OBJ_V_DIR)

$(S_OBJDIR):
	mkdir $(S_OBJDIR)

$(C_S_OBJDIR):
	mkdir $(C_S_OBJDIR)

$(FC_S_OBJDIR):
	mkdir $(FC_S_OBJDIR)

$(S_OBJ_V_DIR):
	mkdir $(S_OBJ_V_DIR)

$(C_S_OBJ_V_DIR):
	mkdir $(C_S_OBJ_V_DIR)

$(FC_S_OBJ_V_DIR):
	mkdir $(FC_S_OBJ_V_DIR)



$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) $(PYTHONINCLUDES) -o $@ $(PYTHONLIBS) -c $<

$(OBJ_V_DIR)/%.o: %.c
	$(CC) $(CFLAGS) $(PYTHONINCLUDES) -DVERBOSE -o $@ $(PYTHONLIBS) -c $<

$(C_OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) $(PYTHONINCLUDES) -DBYPASS_KERAS_CV_CODE -o $@ $(PYTHONLIBS) -c $<

$(C_OBJ_V_DIR)/%.o: %.c
	$(CC) $(CFLAGS) $(PYTHONINCLUDES) -DVERBOSE -DBYPASS_KERAS_CV_CODE -o $@ $(PYTHONLIBS) -c $<

$(FC_OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) $(PYTHONINCLUDES) -DBYPASS_KERAS_CV_CODE -o $@ $(PYTHONLIBS) -c $<

$(FC_OBJ_V_DIR)/%.o: %.c
	$(CC) $(CFLAGS) $(PYTHONINCLUDES) -DVERBOSE -DBYPASS_KERAS_CV_CODE -o $@ $(PYTHONLIBS) -c $<


$(S_OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) $(PYTHONINCLUDES) -DUSE_SIM_ENVIRON -o $@ $(PYTHONLIBS) -c $<

$(S_OBJ_V_DIR)/%.o: %.c
	$(CC) $(CFLAGS) $(PYTHONINCLUDES) -DUSE_SIM_ENVIRON -DVERBOSE -o $@ $(PYTHONLIBS) -c $<

$(C_S_OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) $(PYTHONINCLUDES) -DUSE_SIM_ENVIRON -DBYPASS_KERAS_CV_CODE -o $@ $(PYTHONLIBS) -c $<

$(C_S_OBJ_V_DIR)/%.o: %.c
	$(CC) $(CFLAGS) $(PYTHONINCLUDES) -DUSE_SIM_ENVIRON -DVERBOSE -DBYPASS_KERAS_CV_CODE -o $@ $(PYTHONLIBS) -c $<

$(FC_S_OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) $(PYTHONINCLUDES) -DUSE_SIM_ENVIRON -DBYPASS_KERAS_CV_CODE -o $@ $(PYTHONLIBS) -c $<

$(FC_S_OBJ_V_DIR)/%.o: %.c
	$(CC) $(CFLAGS) $(PYTHONINCLUDES) -DUSE_SIM_ENVIRON -DVERBOSE -DBYPASS_KERAS_CV_CODE -o $@ $(PYTHONLIBS) -c $<

clean:
	$(RM) $(TARGET).exe $(OBJ) $(T_TARGET).exe $(OBJ2)
	$(RM) $(C_TARGET).exe $(C_OBJ)
	$(RM) $(FC_TARGET).exe $(FC_OBJ)
	$(RM) v$(TARGET).exe $(OBJ_V)
	$(RM) v$(C_TARGET).exe $(C_OBJ_V)
	$(RM) fv$(C_TARGET).exe $(FC_OBJ_V)
	$(RM) $(S_TARGET).exe $(S_OBJ)
	$(RM) $(C_S_TARGET).exe $(C_S_OBJ)
	$(RM) $(FC_S_TARGET).exe $(FC_S_OBJ)
	$(RM) v$(S_TARGET).exe $(S_OBJ_V)
	$(RM) v$(C_S_TARGET).exe $(C_S_OBJ_V)
	$(RM) fv$(C_S_TARGET).exe $(FC_S_OBJ_V)
	$(RM) test  $(TRGN_OBJ)
	$(RM) tracegen  $(G_OBJ)

allclean: clean
	$(RM) -rf $(OBJDIR)
	$(RM) -rf $(OBJ_V_DIR)
	$(RM) -rf $(C_OBJDIR)
	$(RM) -rf $(C_OBJ_V_DIR)
	$(RM) -rf $(S_OBJDIR)
	$(RM) -rf $(S_OBJ_V_DIR)
	$(RM) -rf $(C_S_OBJDIR)
	$(RM) -rf $(C_S_OBJ_V_DIR)
	cd utils; make allclean
	cd radar; make clean
	cd viterbi; make allclean


$(OBJDIR)/kernels_api.o: kernels_api.h read_trace.h
$(OBJDIR)/kernels_api.o: viterbi/utils.h viterbi/viterbi_decoder_generic.h
$(OBJDIR)/kernels_api.o: radar/calc_fmcw_dist.h
$(OBJDIR)/read_trace.o: kernels_api.h read_trace.h

$(C_OBJDIR)/kernels_api.o: kernels_api.h read_trace.h
$(C_OBJDIR)/kernels_api.o: viterbi/utils.h viterbi/viterbi_decoder_generic.h
$(C_OBJDIR)/kernels_api.o: radar/calc_fmcw_dist.h
$(C_OBJDIR)/read_trace.o: kernels_api.h read_trace.h

$(FC_OBJDIR)/kernels_api.o: kernels_api.h read_trace.h
$(FC_OBJDIR)/kernels_api.o: viterbi/utils.h viterbi/viterbi_flat.h
$(FC_OBJDIR)/kernels_api.o: radar/calc_fmcw_dist.h
$(FC_OBJDIR)/read_trace.o: kernels_api.h read_trace.h

$(OBJ_V_DIR)/kernels_api.o: kernels_api.h read_trace.h
$(OBJ_V_DIR)/kernels_api.o: viterbi/utils.h viterbi/viterbi_decoder_generic.h
$(OBJ_V_DIR)/kernels_api.o: radar/calc_fmcw_dist.h
$(OBJ_V_DIR)/read_trace.o: kernels_api.h read_trace.h

$(C_OBJ_V_DIR)/kernels_api.o: kernels_api.h read_trace.h
$(C_OBJ_V_DIR)/kernels_api.o: viterbi/utils.h viterbi/viterbi_decoder_generic.h
$(C_OBJ_V_DIR)/kernels_api.o: radar/calc_fmcw_dist.h
$(C_OBJ_V_DIR)/read_trace.o: kernels_api.h read_trace.h

$(FC_OBJ_V_DIR)/kernels_api.o: kernels_api.h read_trace.h
$(FC_OBJ_V_DIR)/kernels_api.o: viterbi/utils.h viterbi/viterbi_flat.h
$(FC_OBJ_V_DIR)/kernels_api.o: radar/calc_fmcw_dist.h
$(FC_OBJ_V_DIR)/read_trace.o: kernels_api.h read_trace.h


$(S_OBJDIR)/kernels_api.o: kernels_api.h sim_environs.h
$(S_OBJDIR)/kernels_api.o: viterbi/utils.h viterbi/viterbi_decoder_generic.h
$(S_OBJDIR)/kernels_api.o: radar/calc_fmcw_dist.h
$(S_OBJDIR)/sim_environs.o: kernels_api.h sim_environs.h

$(C_S_OBJDIR)/kernels_api.o: kernels_api.h sim_environs.h
$(C_S_OBJDIR)/kernels_api.o: viterbi/utils.h viterbi/viterbi_decoder_generic.h
$(C_S_OBJDIR)/kernels_api.o: radar/calc_fmcw_dist.h
$(C_S_OBJDIR)/sim_environs.o: kernels_api.h sim_environs.h

$(FC_S_OBJDIR)/kernels_api.o: kernels_api.h sim_environs.h
$(FC_S_OBJDIR)/kernels_api.o: viterbi/utils.h viterbi/viterbi_flat.h
$(FC_S_OBJDIR)/kernels_api.o: radar/calc_fmcw_dist.h
$(FC_S_OBJDIR)/sim_environs.o: kernels_api.h sim_environs.h

$(S_OBJ_V_DIR)/kernels_api.o: kernels_api.h sim_environs.h
$(S_OBJ_V_DIR)/kernels_api.o: viterbi/utils.h viterbi/viterbi_decoder_generic.h
$(S_OBJ_V_DIR)/kernels_api.o: radar/calc_fmcw_dist.h
$(S_OBJ_V_DIR)/sim_environs.o: kernels_api.h sim_environs.h

$(C_S_OBJ_V_DIR)/kernels_api.o: kernels_api.h sim_environs.h
$(C_S_OBJ_V_DIR)/kernels_api.o: viterbi/utils.h viterbi/viterbi_decoder_generic.h
$(C_S_OBJ_V_DIR)/kernels_api.o: radar/calc_fmcw_dist.h
$(C_S_OBJ_V_DIR)/sim_environs.o: kernels_api.h sim_environs.h

$(FC_S_OBJ_V_DIR)/kernels_api.o: kernels_api.h sim_environs.h
$(FC_S_OBJ_V_DIR)/kernels_api.o: viterbi/utils.h viterbi/viterbi_flat.h
$(FC_S_OBJ_V_DIR)/kernels_api.o: radar/calc_fmcw_dist.h
$(FC_S_OBJ_V_DIR)/sim_environs.o: kernels_api.h sim_environs.h

obj/sim_environs.o: utils/sim_environs.h

obj_v/sim_environs.o: utils/sim_environs.h

#obj/sim_environs.o: viterbi/utils.h viterbi/viterbi_decoder_generic.h viterbi/base.h

obj/gen_trace.o: gen_trace.h


depend:;	makedepend -fMakefile -- $(CFLAGS) -- $(SRC) $(TRGN_SRC) $(G_SRC)
# DO NOT DELETE THIS LINE -- make depend depends on it.

