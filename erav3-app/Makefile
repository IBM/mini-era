CC = gcc -std=c99
MFILE = Makefile

COPTF0 = -O0
COPTF2 = -O2
COPTF3 = -O3

CFLAGS = -pedantic -Wall -g $(COPTF2)
#CFLAGS += -L/usr/lib/python2.7/config-x86_64-linux-gnu -L/usr/lib -lpython2.7 -lpthread -ldl  -lutil -lm  -Xlinker -export-dynamic -Wl,-O1 -Wl,-Bsymbolic-functions
#CFLAGS +=  -lm -Xlinker -export-dynamic

INCLUDES =


LFLAGS = 
#LFLAGS += 
#LIBS = -lviterbi -lfmcwdist -lpthread -ldl -lutil -lm -lpython2.7
LIBS     = -lm

OBJDIR      = obj
V_OBJDIR    = obj_v
X_OBJDIR    = obj_x
VX_OBJDIR   = obj_vx
R_OBJDIR    = obj_r
VR_OBJDIR   = obj_vr
RE_OBJDIR   = obj_re
VRE_OBJDIR  = obj_vre

# The full mini-era target, etc.
TARGET    =  erav3c
X_TARGET   = xmit_erav3c
R_TARGET   = recv_erav3c
RE_TARGET  = recv_erav3c_esp
V_TARGET   = verbose_erav3c
VX_TARGET  = verbose_xmit_erav3c
VR_TARGET  = verbose_recv_erav3c
VRE_TARGET = verbose_recv_erav3c_esp

XMIT_SRC = xmit_pipe.c \
	   crc.c

RECV_SRC = recv_pipe.c\
	   complex_ops.c \
	   delay.c \
	   gr_equalizer.c \
	   ofdm.c \
	   sync_long.c \
	   fir.c \
	   sync_short.c \
	   viterbi_flat.c \
	   descrambler_function.c \
	   simple_dft.c

COMMON_SRC = fft-1d.c 

SRC    = main.c kernels_api.c $(XMIT_SRC) $(RECV_SRC) $(COMMON_SRC)
X_SRC  = xmit_main.c kernels_api.c $(XMIT_SRC) $(COMMON_SRC)
R_SRC  = recv_main.c kernels_api.c $(RECV_SRC) $(COMMON_SRC)
OBJ    = $(SRC:%.c=$(OBJDIR)/%.o)
OBJ_V  = $(SRC:%.c=$(V_OBJDIR)/%.o)
X_OBJ    = $(X_SRC:%.c=$(X_OBJDIR)/%.o)
X_OBJ_V  = $(X_SRC:%.c=$(VX_OBJDIR)/%.o)
R_OBJ    = $(R_SRC:%.c=$(R_OBJDIR)/%.o)
R_OBJ_V  = $(R_SRC:%.c=$(VR_OBJDIR)/%.o)
RE_OBJ   = $(R_SRC:%.c=$(RE_OBJDIR)/%.o)
RE_OBJ_V = $(R_SRC:%.c=$(VRE_OBJDIR)/%.o)

#$(info $$SRC is [${SRC}])
#$(info $$OBJ is [${OBJ}])


all: $(TARGET) $(V_TARGET) $(X_TARGET) $(VX_TARGET) $(R_TARGET) $(VR_TARGET) $(RE_TARGET) $(VRE_TARGET)

$(TARGET): $(OBJDIR)  $(OBJ) 
	$(CC) $(OBJ) $(CFLAGS) $(INCLUDES) -o $@ $(LFLAGS) $(LIBS)

$(V_TARGET): $(V_OBJDIR)  $(OBJ_V) 
	$(CC) $(OBJ_V) $(CFLAGS) $(INCLUDES) -DDEBUG_MODE -o $@ $(LFLAGS) $(LIBS)


$(X_TARGET): $(X_OBJDIR)  $(X_OBJ) 
	$(CC) $(X_OBJ) $(CFLAGS) $(INCLUDES) -o $@ $(LFLAGS) $(LIBS)

$(VX_TARGET): $(VX_OBJDIR)  $(X_OBJ_V) 
	$(CC) $(X_OBJ_V) $(CFLAGS) $(INCLUDES) -DDEBUG_MODE -o $@ $(LFLAGS) $(LIBS)


$(R_TARGET): $(R_OBJDIR)  $(R_OBJ) 
	$(CC) $(R_OBJ) $(CFLAGS) $(INCLUDES) -o $@ $(LFLAGS) $(LIBS)

$(VR_TARGET): $(VR_OBJDIR)  $(R_OBJ_V) 
	$(CC) $(R_OBJ_V) $(CFLAGS) $(INCLUDES) -DDEBUG_MODE -o $@ $(LFLAGS) $(LIBS)


$(RE_TARGET): $(RE_OBJDIR)  $(RE_OBJ) 
	$(CC) $(RE_OBJ) $(CFLAGS) $(INCLUDES) -o $@ $(LFLAGS) $(LIBS)

$(VRE_TARGET): $(VRE_OBJDIR)  $(RE_OBJ_V) 
	$(CC) $(RE_OBJ_V) $(CFLAGS) $(INCLUDES) -DDEBUG_MODE -o $@ $(LFLAGS) $(LIBS)



$(OBJDIR):
	mkdir $@

$(V_OBJDIR):
	mkdir $@

$(R_OBJDIR):
	mkdir $@

$(VR_OBJDIR):
	mkdir $@

$(RE_OBJDIR):
	mkdir $@

$(VRE_OBJDIR):
	mkdir $@

$(X_OBJDIR):
	mkdir $@

$(VX_OBJDIR):
	mkdir $@



$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) $(PYTHONINCLUDES) -DUSE_XMIT_PIPE -DUSE_RECV_PIPE -o $@ $(PYTHONLIBS) -c $<

$(V_OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) $(PYTHONINCLUDES) -DUSE_XMIT_PIPE -DUSE_RECV_PIPE -DDEBUG_MODE -o $@ $(PYTHONLIBS) -c $<

$(X_OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) $(PYTHONINCLUDES) -DUSE_XMIT_PIPE -o $@ $(PYTHONLIBS) -c $<

$(VX_OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) $(PYTHONINCLUDES) -DUSE_XMIT_PIPE -DDEBUG_MODE -o $@ $(PYTHONLIBS) -c $<

$(R_OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) $(PYTHONINCLUDES) -DUSE_RECV_PIPE -o $@ $(PYTHONLIBS) -c $<

$(VR_OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) $(PYTHONINCLUDES) -DUSE_RECV_PIPE -DDEBUG_MODE -o $@ $(PYTHONLIBS) -c $<

$(RE_OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) $(PYTHONINCLUDES) -DUSE_ESP_INTERFACE -DUSE_RECV_PIPE -o $@ $(PYTHONLIBS) -c $<

$(VRE_OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) $(PYTHONINCLUDES) -DUSE_ESP_INTERFACE -DUSE_RECV_PIPE -DDEBUG_MODE -o $@ $(PYTHONLIBS) -c $<


clean:
	$(RM) $(TARGET)    $(OBJ)    $(V_TARGET)   $(V_OBJ) 
	$(RM) $(X_TARGET)  $(X_OBJ)  $(VX_TARGET)  $(VX_OBJ) 
	$(RM) $(R_TARGET)  $(R_OBJ)  $(VR_TARGET)  $(VR_OBJ) 
	$(RM) $(RE_TARGET) $(RE_OBJ) $(VRE_TARGET) $(VRE_OBJ) 

clobber: clean
	$(RM) -rf $(OBJDIR)    $(V_OBJDIR)
	$(RM) -rf $(X_OBJDIR)  $(VX_OBJDIR)
	$(RM) -rf $(R_OBJDIR)  $(VR_OBJDIR)
	$(RM) -rf $(RE_OBJDIR) $(VRE_OBJDIR)

allclean: clobber


depend:;	makedepend -f$(MFILE) -- $(CFLAGS) -- $(SRC)
# DO NOT DELETE THIS LINE -- make depend depends on it.


