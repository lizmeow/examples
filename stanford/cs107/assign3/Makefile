CC = gcc

CFLAGS = -O2 -std=gnu99

all: vectortest hashsettest

vectortest : vectortest.o vector.o
	$(CC) vectortest.o vector.o -o vectortest

vectortest.o : vectortest.c vector.h
	$(CC) $(CFLAGS) -c vectortest.c

vector.o : vector.c vector.h vector_internal.h
	$(CC) $(CFLAGS) -c vector.c

hashsettest : hashsettest.o hashset.o vector.o
	$(CC) hashsettest.o hashset.o vector.o -o hashsettest

hashsettest.o : hashsettest.c hashset.h
	$(CC) $(CFLAGS) -c hashsettest.c

hashset.o : hashset.c hashset.h hashset_internal.h vector.h
	$(CC) $(CFLAGS) -c hashset.c

clean:
	rm -fr vectortest hashsettest core *.o

.PHONY: clean all