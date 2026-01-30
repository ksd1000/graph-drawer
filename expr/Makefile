CC := gcc
CFLAG := -Wall -Werror -O3 -fPIC
LFLAG := -lc -lm
PLATFORM := $(shell uname -a)
all: expr.o expr-version
check: expr.c expr.h check.c
	$(CC) $(CFLAG) -fsanitize=address leak.c check.c -o check expr.c $(LFLAG) -g
expr.o: expr.c expr.h
	$(CC) $(CFLAG) -DPLATFORM="\"$(PLATFORM)\"" expr.c -c -o expr.o
expr-version: expr.o expr-version.c
	$(CC) $(CFLAG) expr.o expr-version.c -o expr-version $(LFLAG)
.PHONY:
cleanall:
	make clean
.PHONY:
clean:
	rm -f check
	rm -f expr-version
	rm -f expr.o

