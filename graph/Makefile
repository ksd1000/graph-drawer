CC := gcc
CFLAG := -Wall -O3 -fPIC
LFLAG := -lc -lm
all: graph.a
graph.a: graph.o texts/text.o texts/sbmp.o
	ar -rcs graph.a graph.o texts/text.o texts/sbmp.o
graph.o: graph.c graph.h
	$(CC) $(CFLAG) graph.c -c -o graph.o
texts/text.o:
	make -C texts
texts/sbmp.o: 
	make -C texts
texts/text.h:
	make -C texts
.PHONY:
cleanall:
	make clean
	make -C texts clean
.PHONY:
clean:
	rm -f graph.o
	rm -f graph.a




