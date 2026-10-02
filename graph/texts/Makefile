CC := gcc
CFLAG := -Wall -O3 -fPIC
LFLAG := -lc -lm
#kernel := $(shell uname -r)
all: text.o sbmp.o
text.o: text.c
	$(CC) $(CFLAG) text.c -c -o text.o
data: text.h text.c.h
text.h: bmpass gen genc text.1.h
	chmod +x ./gen ./genc
	./gen
	./genc
text.c.h: bmpass gen genc text.1.h
	chmod +x ./gen ./genc
	./gen
	./genc
sbmp.o: sbmp.c
	$(CC) $(CFLAG) sbmp.c -c -o sbmp.o
bmpass: sbmp.o bmpass.c
	$(CC) $(CFLAG) bmpass.c sbmp.o -o bmpass
.PHONY:
clean:
	rm -f text.o
#	rm -f text.h
#	rm -f text.c.h
	rm -f sbmp.o
	rm -f bmpass
	rm -f *.bmp *.sbmp



