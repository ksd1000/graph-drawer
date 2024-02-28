CC := gcc
CFLAG := -Wall -Ofast
LFLAG := -lc -lm
#kernel := $(shell uname -r)
text.o: text.h text.c.h
	$(CC) $(CFLAG) text.c -c -o text.o
text.h: bmpass gen genc text.1.h
	chmod +x ./gen ./genc
	./gen
	./genc
text.c.h: bmpass gen genc
	chmod +x ./gen ./genc
	./gen
	./genc
sbmp.o: 
	$(CC) $(CFLAG) sbmp.c -c -o sbmp.o
bmpass: sbmp.o
	$(CC) $(CFLAG) bmpass.c sbmp.o -o bmpass
.PHONY:
clean:
	rm -f text.o
	rm -f text.h
	rm -f text.c.h
	rm -f sbmp.o
	rm -f bmpass
	rm -f *.bmp *.sbmp



