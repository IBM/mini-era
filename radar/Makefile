CC = gcc -O2
CFLAGS = -pedantic -Wall -O0 -g -I ../fft/fft-1d
MFILE = Makefile

AR      = ar rcs

L_OBJ_DIR = obj
LG_OBJ_DIR = obj_g
LT_OBJ_DIR = obj_t

L_SRC = calculate_dist_from_fmcw.c 
L_OBJ = $(L_SRC:%.c=$(L_OBJ_DIR)/%.o) ../fft/fft-1d/obj/fft.o
LG_OBJ = $(L_SRC:%.c=$(LG_OBJ_DIR)/%.o) ../fft/fft-1d/obj_g/fft.o
LT_OBJ = $(L_SRC:%.c=$(LT_OBJ_DIR)/%.o) ../fft/fft-1d/obj_t/fft.o

SRC = 	main.c
OBJ =   $(SRC:%.c=%.o)

all:	libfmcwdist.a libfmcwdist_gp.a libfmcwdist_t.a $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -I ../fft/fft-1d -o test_calc_dist_from_fmcw.exe -L. -lfmcwdist -lm 
#	$(CC) ../fft/fft-1d/fft.o $(CFLAGS) calculate_dist_from_fmcw.c -L /mnt/hgfs/WinDownloads/DSSOC/era/mini-era/fft/fft-1d/ -lm -I ../fft/fft-1d -o calculate_dist_from_fmcw.exe

libfmcwdist.a:	$(L_OBJ_DIR) $(L_OBJ)
	$(AR) $@ $(L_OBJ)

libfmcwdist_gp.a:	$(LG_OBJ_DIR) $(LG_OBJ)
	$(AR) $@ $(LG_OBJ)

libfmcwdist_t.a:	$(LT_OBJ_DIR) $(LT_OBJ)
	$(AR) $@ $(LT_OBJ)

../fft/fft-1d/obj/fft.o:
	cd ../fft/fft-1d && make -f $(MFILE) all

../fft/fft-1d/obj_g/fft.o:
	cd ../fft/fft-1d && make -f $(MFILE) all

../fft/fft-1d/obj_t/fft.o:
	cd ../fft/fft-1d && make -f $(MFILE) all

$(L_OBJ_DIR):
	mkdir $@

$(LG_OBJ_DIR):
	mkdir $@

$(LT_OBJ_DIR):
	mkdir $@

clean:
	-rm *.exe *.dat *.o libfmcwdist*.a 
	-rm -rf $(L_OBJ_DIR)
	-rm -rf $(LG_OBJ_DIR)
	-rm -rf $(LT_OBJ_DIR)
	cd ../fft/fft-1d; make -f $(MFILE) clean

$(L_OBJ_DIR)/%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

$(LG_OBJ_DIR)/%.o: %.c
	$(CC) $(CFLAGS) -g -pg -o $@ -c $<

$(LT_OBJ_DIR)/%.o: %.c
	$(CC) $(CFLAGS) -DINT_TIME -o $@ -c $<

