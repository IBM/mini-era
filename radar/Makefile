CC = gcc -O2
CFLAGS = -pedantic -Wall -O0 -g -I ../fft/fft-1d

AR      = ar rcs

L_SRC = calculate_dist_from_fmcw.c 
L_OBJ = $(L_SRC:%.c=%.o) ../fft/fft-1d/fft.o

SRC = 	main.c
OBJ =   $(SRC:%.c=%.o)

all:	libfmcwdist.a $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -I ../fft/fft-1d -o test_calc_dist_from_fmcw.exe -L. -lfmcwdist -lm 
#	$(CC) ../fft/fft-1d/fft.o $(CFLAGS) calculate_dist_from_fmcw.c -L /mnt/hgfs/WinDownloads/DSSOC/era/mini-era/fft/fft-1d/ -lm -I ../fft/fft-1d -o calculate_dist_from_fmcw.exe

libfmcwdist.a:	$(L_OBJ)
	$(AR) $@ $(L_OBJ)

../fft/fft-1d/fft.o:
	cd ../fft/fft-1d && make

clean:
	-rm *.exe *.dat ../fft/fft-1d/fft.o *.o libfmcwdist.a 
