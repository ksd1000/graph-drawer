#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "expr.h"
#include <time.h>
#include <err.h>
const struct proj {
	const char *e;
	double expect;
} projs[]={
	{"1+5",6},
	{"5+3*8",29},
	{"(5+3)*8",64},
	{"134|45.5&7.5",135.5},
	{"134|(45.5&7.5)",135.5},
	{"(134|45.5)&7.5",7.5},
	{"6^3",216},
	{"6^^3",5},
	{"0&&1",0},
	{"1&&0",0},
	{"0&&0",0},
	{"1&&1",1},
	{"0||1",1},
	{"1||0",1},
	{"0||0",0},
	{"1||1",1},
	{"0^^^1",1},
	{"1^^^0",1},
	{"0^^^0",0},
	{"1^^^1",0},
	{"5-7+6",4},
	{"5-(7+6)",-8},
	{"5<<3>>2",10},
	{"5<<(3>>2)",5},
	{"(5<<3)>>2",10},
	{"5+3>>2",2},
	{"5>>3+2",5.0/32},
	{"drand48()-->x,x+2^x<3",1},
	{"drand48()-->x,x+2^x>=3",0},
	{"drand48()-->x,x+2^x>=0",1},
	{"drand48()-->x,x+2^x<0",0},
	{"!(5-2)",0},
	{"!(5-5)",1},
	{"!!(5-2)",1},
	{"!!(5-5)",0},
	{"med({0..10000})",5000},
	{"vmd(n,0,10000,1,n,med,0)",5000},
	{"sum(n,1,100,1,n)",5050},
	{"if(3,5,7)",5},
	{"if(0,5,7)",7},
	{"0-->m,while(m<7626,(1+m)->m,m)",7626},
	{"0-->m,while(m<=7626,(1+m)->m,m)",7627},
	{"-!3",-0.0},
	{"!-3",0},
	{"!-~3",0},
	{"!~-3",0},
	{"-!~3",-0.0},
	{"!!3",1},
	{"!!!3",0},
	{"!!0",0},
	{"!!!0",1},
	{"(129+127)&~127",256},
	{"(128+127)&~127",128},
	{"(127+127)&~127",128},
	{NULL,}

};
void check(const char *e,double expect){
	double r;
	printf("checking %s --- expect %lg",e,expect);
	r=expr_calc5(e,NULL,0,NULL,EXPR_IF_NOOPTIMIZE);
	if(r!=expect){
		printf("\nerror! %s should be %lg but %lg\n",e,expect,r);
		goto ab;
	}
	r=expr_calc5(e,NULL,0,NULL,0);
	if(r!=expect){
		printf("\noptimization error! %s should be %lg but %lg\n",e,expect,r);
		goto ab;
	}
	printf(" ... ok\n");
	return;
ab:
	printf("ABORTING\n");
	abort();
}
int main(int argc,char **argv){
	for(const struct proj *p=projs;p->e;++p)
		check(p->e,p->expect);
	return 0;
}
