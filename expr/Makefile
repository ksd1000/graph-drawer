CC := gcc
CFLAG := -Wall -Werror -O3 -fPIC
LFLAG := -lc -lm
all: expr.o expr-version
expr.o: expr_global.o expr_core.o expr_format.o expr_buffered.o expr_builtin.o
	ld -r -o expr.o expr_global.o expr_core.o expr_format.o expr_buffered.o expr_builtin.o
check: expr_core.c expr_global.c expr.h check.c expr_builtin.c
	$(CC) $(CFLAG) -fsanitize=address leak.c check.c expr_builtin.c expr_global.c -o check expr_core.c $(LFLAG) -g
expr_global.o: expr_global.c expr.h
	$(CC) $(CFLAG) expr_global.c -c -o expr_global.o
expr_core.o: expr_core.c expr.h
	$(CC) $(CFLAG) expr_core.c -c -o expr_core.o
expr_format.o: expr_format.c expr.h
	$(CC) $(CFLAG) expr_format.c -c -o expr_format.o
expr_buffered.o: expr_format.c expr.h
	$(CC) $(CFLAG) expr_buffered.c -c -o expr_buffered.o
expr_builtin.o: expr_builtin.c expr.h
	$(CC) $(CFLAG) expr_builtin.c -c -o expr_builtin.o
expr-version: expr.o expr-version.c
	$(CC) $(CFLAG) expr-version.c expr.o -o expr-version $(LFLAG)
.PHONY:
genh:
	sh genexprh
.PHONY:
cleanall:
	make clean
.PHONY:
clean:
	rm -f check
	rm -f expr-version
	rm -f expr_global.o
	rm -f expr_core.o
	rm -f expr_format.o
	rm -f expr_buffered.o
	rm -f expr_builtin.o
	rm -f expr.o

