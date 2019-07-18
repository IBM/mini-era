CC = gcc

CFLAGS = -pedantic -Wall -O0 -g
INCLUDES =  
LFLAGS = 
LIBS = 

TARGET = main
OBJECTS = kernels_api.o

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) $(CFLAGS) $(INCLUDES) -o $(TARGET).exe $(TARGET).c $(LFLAGS) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	$(RM) $(TARGET).exe $(OBJECTS)
