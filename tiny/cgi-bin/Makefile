CC = gcc
CFLAGS = -O2 -Wall -I ..

all: adder 11-10-adder

adder: adder.c
	$(CC) $(CFLAGS) -o adder adder.c

11-10-adder: 11-10-adder.c
	$(CC) $(CFLAGS) -o 11-10-adder 11-10-adder.c

clean:
	rm -f *~ *.o adder 11-10-adder