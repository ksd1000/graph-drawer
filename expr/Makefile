# The Makefile is deprecated, use cmake instead.
CC := gcc
CFLAG := -Wall -Werror -O3 -fPIC
LFLAG := -lc -lm
all: expr.o expr-version
expr.o: expr-global.o expr-core.o expr-format.o expr-buffered.o expr-builtin.o
	ld -r -o expr.o expr-global.o expr-core.o expr-format.o expr-buffered.o expr-builtin.o
check: expr-core.c expr-global.c expr.h check.c expr-builtin.c
	$(CC) $(CFLAG) -fsanitize=address leak.c check.c expr-builtin.c expr-global.c -o check expr-core.c $(LFLAG) -g
expr-global.o: expr-global.c expr.h
	$(CC) $(CFLAG) expr-global.c -c -o expr-global.o
expr-core.o: expr-core.c expr.h
	$(CC) $(CFLAG) expr-core.c -c -o expr-core.o
expr-format.o: expr-format.c expr.h
	$(CC) $(CFLAG) expr-format.c -c -o expr-format.o
expr-buffered.o: expr-format.c expr.h
	$(CC) $(CFLAG) expr-buffered.c -c -o expr-buffered.o
expr-builtin.o: expr-builtin.c expr.h
	$(CC) $(CFLAG) expr-builtin.c -c -o expr-builtin.o
expr-version: expr.o expr-version.c
	$(CC) $(CFLAG) expr-version.c expr.o -o expr-version $(LFLAG)
.PHONY:
genh:
	./genexprh
.PHONY:
cleanall:
	make clean
.PHONY:
clean:
	rm -f check
	rm -f expr-version
	rm -f expr-global.o
	rm -f expr-core.o
	rm -f expr-format.o
	rm -f expr-buffered.o
	rm -f expr-builtin.o
	rm -f expr.o

