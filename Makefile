CC := gcc
CFLAG := -Wall -Ofast -fPIC
LFLAG := -lc -lm
kernel := $(shell uname -r)
xgraph.tar.gz: header/expr.h header/xdraw.h lib/xgraph.a lib/xgraph.so check
	tar -czf xgraph.tar.gz header lib
check: expr.c expr.h check.c
	$(CC) $(CFLAG) check.c -o check -DMEMORY_LEAK_CHECK expr.c $(LFLAG)
lib/xgraph.a: expr.o xdraw.o texts/text.o texts/sbmp.o
	mkdir -p lib
	ar -rcs lib/xgraph.a expr.o xdraw.o texts/text.o texts/sbmp.o
lib/xgraph.so: expr.o xdraw.o texts/text.o texts/sbmp.o
	mkdir -p lib
	$(CC) $(CFLAG) -shared -o lib/xgraph.so expr.o xdraw.o texts/text.o texts/sbmp.o
xdraw.o: xdraw.c texts/text.h xdraw.h expr.h
	$(CC) $(CFLAG) xdraw.c -c -o xdraw.o
expr.o: expr.c expr.h
	$(CC) $(CFLAG) expr.c -c -o expr.o

header/xdraw.h: xdraw.h header/expr.h
	mkdir -p header
	cp xdraw.h header
header/expr.h: expr.h
	mkdir -p header
	cp expr.h header
expr.h: expr.1.h expr.2.h expr.c
	chmod +x ./genexprh ./decdump
	./genexprh
xdraw.h: xdraw.1.h xdraw.2.h xdraw.c expr.h
	chmod +x ./decdump ./genxdrawh
	./genxdrawh
texts/text.o:
	make -C texts
texts/sbmp.o: 
	make -C texts
texts/text.h:
	make -C texts
.PHONY:
cleanall:
	make clean
.PHONY:
clean:
	rm -f xgraph.tar.gz
	rm -rf lib header
	rm -f check
	rm -f expr.o
	rm -f xdraw.o
	make -C texts clean




