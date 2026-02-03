CC := gcc
CFLAG := -Wall -Werror -O3 -fPIC
LFLAG := -lc -lm
PLATFORM := $(shell uname -a)
all: expr.a expr-version
expr.a: expr.o expr_format.o expr_buffered.o expr_builtin.o
	ar -rcs expr.a expr.o expr_format.o expr_buffered.o expr_builtin.o
check: expr.c expr.h check.c
	$(CC) $(CFLAG) -fsanitize=address leak.c check.c expr_builtin.c -o check expr.c $(LFLAG) -g
expr.o: expr.c expr.h
	$(CC) $(CFLAG) -DPLATFORM="\"$(PLATFORM)\"" expr.c -c -o expr.o
expr_format.o: expr_format.c expr.h
	$(CC) $(CFLAG) expr_format.c -c -o expr_format.o
expr_buffered.o: expr_format.c expr.h
	$(CC) $(CFLAG) expr_buffered.c -c -o expr_buffered.o
expr_builtin.o: expr_builtin.c expr.h
	$(CC) $(CFLAG) expr_builtin.c -c -o expr_builtin.o
expr-version: expr.a expr-version.c
	$(CC) $(CFLAG) expr.a expr-version.c -o expr-version $(LFLAG)
.PHONY:
cleanall:
	make clean
.PHONY:
clean:
	rm -f check
	rm -f expr-version
	rm -f expr.o
	rm -f expr_format.o
	rm -f expr_buffered.o
	rm -f expr_builtin.o
	rm -f expr.a

