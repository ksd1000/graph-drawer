#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "expr.h"
#include <time.h>
#include <assert.h>
#include <err.h>
struct expr_symset es=EXPR_SYMSET_INITIALIZER;
const struct proj {
	const char *e;
	double expect;
} projs[]={
	{"+7",7},
	{"-7",-7},
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
	{"med({0..10})",5},
	{"vmd(n,0,10000,1,n,med,0)",5000},
	{"sum(n,1,100,1,n)",5050},
	{"if(3,5,7)",5},
	{"if(0,5,7)",7},
	{"0-->m,while(m<7626,(1+m)->m),m",7626},
	{"0-->m,while(m<=7626,(1+m)->m),m",7627},
	{"-!3",-0.0},
	{"!-3",0},
	{"!-~3",0},
	{"!~-3",0},
	{"-!~3",-0.0},
	{"!!3",1},
	{"!!!3",0},
	{"!!0",0},
	{"!!!0",1},
	{"!~!-!!t",0},
	{"!!~!-!!t",1},
	{"(129+127)&~127",256},
	{"(128+127)&~127",128},
	{"(127+127)&~127",128},
	{"asint(t#128)",128},
	{"asint(t#(128*2)#1)",257},
	{"asint(0#(128*2)#1)",257},
	{"static_assert(e>2.71)",1},
	{NULL}

};
const struct eproj {
	const char *e;
	int expect;
} eprojs[]={
	{"2+hshdjxjdjxhxhx",EXPR_ESYMBOL},
	{"xor(2,3,5,gg,8)",EXPR_ESYMBOL},
	{"5->sum",EXPR_ESYMBOL},
	{"sum(",EXPR_EPT},
	{"sum",EXPR_EFP},
	{"sqrt()",EXPR_ENVP},
	{"sum(n)",EXPR_ENEA},
	{"266j",EXPR_ENUMBER},
	{"5->pi",EXPR_ETNV},
	{"5->sqrt",EXPR_ETNV},
	{"5->t",EXPR_ETNV},
	{"2+",EXPR_EEV},
	{"2->",EXPR_EEV},
	{"2->+",EXPR_EUO},
	{"drand48(2)",EXPR_EZAFP},
	{"drand48(",EXPR_EZAFP},
	{"drand48",EXPR_EZAFP},
	{"5-->defined_symbol",EXPR_EDS},
	{"5-->t",EXPR_EDS},
	{"vmd(k,1,10,1,k,cmp,0)",EXPR_EVMD},
	{"med({1..1000000000000000})",EXPR_EMEM},
	{"5-->v,&v[2]c",EXPR_EUSN},
	{"5-->v,&v[2]7",EXPR_EUSN},
	{"alloca(t,exp(t))",EXPR_ENC},
	{"static_assert(t>=2.72)",EXPR_ENC},
	{"&e",EXPR_ECTA},
	{"static_assert(e>=2.72)",EXPR_ESAF},
	{NULL}
};
void errcheck(const char *e,int expect){
	int error;
	printf("checking %s --- expect \"%s\"",e,expr_error(expect));
	if(new_expr6(e,"t",&es,EXPR_IF_INSTANT_FREE,&error,NULL)){
		printf("\nerror! %s should be \"%s\" but ok\n",e,expr_error(expect));
		goto ab;
	}else if(error!=expect){
		printf("\nerror! %s should be \"%s\" but \"%s\"\n",e,expr_error(expect),expr_error(error));
		goto ab;
	}
	printf(" ... ok\n");
	return;
ab:
	printf("ABORTING\n");
	abort();
}
void check(const char *e,double expect){
	double r;
	struct expr ep[1];
	//static int k=0;if(k++==39)exit(0);
	printf("checking %s --- expect %lg",e,expect);
	init_expr5(ep,e,"t",NULL,EXPR_IF_INSTANT_FREE);
	r=expr_calc5(e,"t",0,NULL,EXPR_IF_NOOPTIMIZE);
	if(memcmp(&r,&expect,sizeof(double))){
		printf("\nerror! %s should be %lg but %lg\n",e,expect,r);
		goto ab;
	}
	r=expr_calc5(e,"t",0,NULL,0);
	if(memcmp(&r,&expect,sizeof(double))){
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
	srand48(time(NULL)+getpid());
	expr_calc5("t+2","t",3,NULL,0);
	expr_symset_add(&es,"defined_symbol",EXPR_CONSTANT,2304.0);
	for(const struct proj *p=projs;p->e;++p)
		check(p->e,p->expect);
	for(const struct eproj *p=eprojs;p->e;++p)
		errcheck(p->e,p->expect);
	new_expr7("t^3+sin(t)+sum(n,0,100,1,sin(n*t))","t",NULL,EXPR_IF_INSTANT_FREE,1250,NULL,NULL);
	assert(es.size==1);
	assert(es.depth==1);
	expr_symset_wipe(&es);
	for(const struct expr_builtin_keyword *p=expr_keywords;;++p){
		if(!p->str){
			break;
		}
		if(expr_builtin_symbol_search(p->str,p->strlen)){
			printf("conflict %s\n",p->str);
			printf("ABORTING\n");
			abort();
		}
	}
	return 0;
}
