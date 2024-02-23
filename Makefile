CC := gcc
CFLAG := -Wall -Werror -Ofast
LFLAG := -lc -lm
kernel := $(shell uname -r)
lib/xgraph.a: expr.o xdraw.o texts/text.o texts/sbmp.o lib header/xdraw.h header/expr.h
	ar -rcs lib/xgraph.a expr.o xdraw.o texts/text.o texts/sbmp.o
xdraw.o: xdraw.c xdraw.h texts/text.h
	$(CC) $(CFLAG) xdraw.c -c -o xdraw.o
expr.o: expr.c expr.h
	$(CC) $(CFLAG) expr.c -c -o expr.o
header:
	mkdir header
lib:
	mkdir lib
header/xdraw.h: header
	cp xdraw.h header
header/expr.h: header
	cp expr.h header
texts/text.o:
	make -C texts
texts/sbmp.o: 
	make -C texts
texts/text.h:
	make -C texts
.PHONY:
cleanall:
	rm -rf lib header
	make clean
.PHONY:
clean:
	rm -f expr.o
	rm -f xdraw.o
	make -C texts clean




