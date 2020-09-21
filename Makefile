CC = gcc -std=c99
MFILE = Makefile

COPTF0 = -O0
COPTF2 = -O2
COPTF3 = -O3

CFLAGS = -pedantic -Wall -g $(COPTF2)
#CFLAGS += -L/usr/lib/python2.7/config-x86_64-linux-gnu -L/usr/lib -lpython2.7 -lpthread -ldl  -lutil -lm  -Xlinker -export-dynamic -Wl,-O1 -Wl,-Bsymbolic-functions
CFLAGS +=  -Xlinker -export-dynamic

INCLUDES = -Iinclude -Iinclude/radar -Iinclude/h264 -Iinclude/viterbi
CFLAGS += $(INCLUDES)

#PYTHONINCLUDES = $(shell /usr/bin/python-config --cflags)
PYTHONINCLUDES = -I/usr/include/python3.6m


LFLAGS = 
#LFLAGS += 
#LIBS = -lviterbi -lfmcwdist -lpthread -ldl -lutil -lm -lpython2.7
#LIBS = -lpthread -ldl -lutil -lm
LIBS = -lm
#PYTHONLIBS = $(shell /usr/bin/python-config --ldflags)
PYTHONLIBS = -lpython3.6m
CPYTHONLIBS =

include Makefile.targets



depend:;	makedepend -f$(MFILE) -- $(CFLAGS) -- $(SRC) $(TRGN_SRC) $(G_SRC)
# DO NOT DELETE THIS LINE -- make depend depends on it.

