CC := gcc
CFLAG := -Wall -Werror -Ofast
LFLAG := -lc -lm
kernel := $(shell uname -r)
xgraph.a: expr.o xdraw.o texts/text.o texts/sbmp.o
	ar -rcs xgraph.a expr.o xdraw.o texts/text.o texts/sbmp.o
xdraw.o: xdraw.c xdraw.h texts/text.h
	$(CC) $(CFLAG) xdraw.c -c -o xdraw.o
expr.o: expr.c expr.h
	$(CC) $(CFLAG) expr.c -c -o expr.o
texts/text.o:
	make -C texts
texts/sbmp.o: 
	make -C texts
texts/text.h:
	make -C texts
.PHONY:
clean:
	rm -f xgraph.a
	rm -f expr.o
	rm -f xdraw.o
	make -C texts clean




