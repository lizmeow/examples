CC = gcc

CFLAGS = -O2 -std=gnu99

all: spellcheck

spellcheck : spellcheck.o cvector.o cmap.o
	$(CC) spellcheck.o cvector.o cmap.o -o spellcheck

spellcheck.o : spellcheck.c cvector.h cmap.h
	$(CC) $(CFLAGS) -c spellcheck.c

cvector.o : cvector.c cvector.h
	$(CC) $(CFLAGS) -c cvector.c

cmap.o : cmap.c cmap.h
	$(CC) $(CFLAGS) -c cmap.c

clean:
	rm -fr spellcheck core *.o

.PHONY: clean all