#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "expr.h"
#include <time.h>
#include <err.h>
//#define printf(f,...) 1
void add_common_symbols(struct expr_symset *es);
double dtime(void);
struct expr_symset es[1];
volatile sig_atomic_t vssuc;
volatile sig_atomic_t vsi;
void randomize(double *buf,size_t sz){
	if(!sz)sz=1;
	do
		*(buf++)=drand48();
	while(--sz);
}
double rv[10000000];
int main(int argc,char **argv){
	double t3,t,dt;
	size_t from=1,to=UINT_MAX,times=10000;
	switch(argc){
		case 4:
			times=atol(argv[3]);
		case 3:
			to=atol(argv[2]);
		case 2:
			from=atol(argv[1]);
		default:
			break;
	}
	srand48(time(NULL)+(intptr_t)main);
	srand(time(NULL)+(intptr_t)main);
	srandom(time(NULL)+(intptr_t)main);
	for(size_t i=from;i<=to;++i,printf("\n")){
		t3=0.0;
		int r;
		for(size_t j=0;j<times;++j){
			randomize(rv,i);
			dt=dtime();
			//r=expr_sort4(rv,i,malloc,free);
			expr_sortq(rv,i);r=0;
			t3+=dtime()-dt;
			if(r<0){
				fputs("CANNOT ALLOCATE MEMORY\n",stderr);
				abort();
			}
		}
		printf("n=%zu\t expr_sort4: %zums",i,(size_t)(t3*1000));
		//continue;
		t=0.0;
		for(size_t j=0;j<times;++j){
			randomize(rv,i);
			dt=dtime();
			expr_sort_old(rv,i);
			t+=dtime()-dt;
		}
		printf("\texpr_sort_old: %zums\t1 : %lg",(size_t)(t*1000),t/t3);
	}
	return 0;
}
