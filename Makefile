CC := gcc
CFLAG := -Wall -O3 -fPIC
LFLAG := -lc -lm
#kernel := $(shell uname -r)
all: xgraph.a xgraph.so expr/expr-version
xgraph.a: graph/graph.a expr/expr.o
	cp graph/graph.a xgraph.a
	ar -rcs xgraph.a expr/expr.o
xgraph.so: xgraph.a
	$(CC) $(CFLAG) -shared -o xgraph.so xgraph.a
expr/expr.o: expr
	make -C expr
expr/expr-version: expr
	make -C expr
graph/graph.a: graph
	make -C graph
.PHONY:
cleanall:
	make -C expr cleanall
	make -C graph cleanall
	make clean0
.PHONY:
clean:
	make -C expr clean
	make -C graph clean
	make clean0
.PHONY:
clean0:
	rm -rf xgraph.a xgraph.so



