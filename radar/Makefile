CC = gcc -O2
CFLAGS = -pedantic -Wall -O0 -g

AR      = ar rcs

L_SRC = calculate_dist_from_fmcw.c 
L_OBJ = $(L_SRC:%.c=%.o)

SRC = 	main.c
OBJ =   $(SRC:%.c=%.o)

all:	libfmcwdist.a $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o test_calc_dist_from_fmcw.exe -L. -lfmcwdist -lm 

libfmcwdist.a:	$(L_OBJ)
	$(AR) $@ $(L_OBJ)

clean:
	-rm *.exe *.dat *.o libfmcwdist.a 
