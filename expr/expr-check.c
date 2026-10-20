#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>
#ifdef __unix__
#include <err.h>
#else
#define err(v,fmt,...) ({fprintf(stderr,fmt,##__VA_ARGS__);exit(v)})
#endif
#define EXPR_BLOCKWARNING 1
#include "expr.h"
struct expr_symset *es;
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
	{"6**3",216},
	{"6^3",5},
	{"0&&1",0},
	{"1&&0",0},
	{"0&&0",0},
	{"1&&1",1},
	{"0||1",1},
	{"1||0",1},
	{"0||0",0},
	{"1||1",1},
	{"0^^1",1},
	{"1^^0",1},
	{"0^^0",0},
	{"1^^1",0},
	{"5-7+6",4},
	{"5-(7+6)",-8},
	{"5<<3>>2",10},
	{"5<<(3>>2)",5},
	{"(5<<3)>>2",10},
	{"5+3>>2",2},
	{"5>>3+2",5.0/32},
	{"drand48(&x0)-->x,x+2**x<3",1},
	{"drand48(&x0)-->x,x+2**x>=3",0},
	{"drand48(&x0)-->x,x+2**x>=0",1},
	{"drand48(&x0)-->x,x+2**x<0",0},
	{"!(5-2)",0},
	{"!(5-5)",1},
	{"!!(5-2)",1},
	{"!!(5-5)",0},
	{"sum(n,1,100,1,n)",5050},
	{"vmd(n,0,10000,1,n,med,0)",5000},
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
	{"!~!-!!0",0},
	{"!!~!-!!0",1},
	{"(129+127)&~127",256},
	{"(128+127)&~127",128},
	{"(127+127)&~127",128},
	{"asint(0#128)",128},
	{"asint(0#(128*2)#1)",257},
	{"asint(0#(128*2)#1)",257},
	{"static_assert(e>2.71),2",2},
	{"defined(sin)",1},
	{"typeof(exp)",EXPR_FUNCTION},
	{"flagof(cos)&SPURE",1},
	{"f=(u,v){u+v**2},f(7,4)",23},
	{"undef(defined_symbol){1},defined_symbol",2304.0},
	{"alias(x,defined_symbol);x",2304.0},
	{"alias(x,\"5+6\");x=4;x+2",6},
	{NULL,0.0}

};
const struct eproj {
	const char *e;
	int expect;
	const char *ee;
} eprojs[]={
	{"2+hshdjxjdjxhxhx",EXPR_ESYMBOL,"hshdjxjdjxhxhx"},
	{"xor(2,3,5,gg,8)",EXPR_ESYMBOL,"gg"},
	{"5->sum",EXPR_ESYMBOL,"sum"},
	{"sum(",EXPR_EPT,""},
	{"sum",EXPR_EFP,"sum"},
	{"sqrt()",EXPR_ENVP,""},
	{"$e",EXPR_ENUMBER,"e"},
	{"sum(n)",EXPR_ENEA,"sum"},
	{"266j",EXPR_EUSN,"j"},
	{"5->pi",EXPR_ETNV,"pi"},
	{"5->sqrt",EXPR_ETNV,"sqrt"},
	{"5->e",EXPR_ETNV,"e"},
	{"5->t",EXPR_ETNV,"t"},
	{"2+",EXPR_EEV,""},
	{"2->",EXPR_EEV,""},
	{"2->+",EXPR_EUO,"+"},
	{"abort(2)",EXPR_EZAFP,"abort"},
	{"abort(",EXPR_EZAFP,"abort"},
	{"abort",EXPR_EZAFP,"abort"},
	{"var(defined_symbol,5),0",EXPR_EDS,"defined_symbol"},
	{"vmd(k,1,10,1,k,cmp,0)",EXPR_EVMD,"cmp"},
	{"5-->v,&v[2]c",EXPR_EUSN,"c"},
	{"5-->v,&v[2]7",EXPR_EUSN,"7"},
	{"alloca(t,exp(t))",EXPR_ENC,"exp(t)"},
	{"static_assert(t>=2.72)",EXPR_ENC,"t>=2.72"},
	{"&e",EXPR_ECTA,"e"},
	{"static_assert(e>=2.72)",EXPR_ESAF,"e>=2.72"},
	{"static_assert(e<2.72)",EXPR_EVD,""},
	{"flag(IPROTECT),vmd(k,1,10,1,k,add,0)",EXPR_EVZP,"vmd"},
	{"undef(sin)",EXPR_EVD,""},
	{"flag(IPROTECT),flag(0)",EXPR_EPM,"flag"},
	{"flag(IPROTECT),&sin",EXPR_EPM,"&"},
	{"undef(defined_symbol),defined_symbol",EXPR_ESYMBOL,"defined_symbol"},
	{"undef(defined_symbol){defined_symbol}",EXPR_ESYMBOL,"defined_symbol"},
	{"flag(IPURE),drand48(&x0)",EXPR_EPURE,"drand48"},
	{"alias(x,y);alias(y,x);x",EXPR_EANT,"x"},
	{"alias(x,y);alias(y,z);alias(z,y);x",EXPR_EANT,"x"},
	{"error()",EXPR_EUDE,""},
	{"error(\"errname\")",EXPR_EUDE,"errname"},
	{"static_assert(2<1,\"errname\")",EXPR_EUDE,"errname"},
	{"alias(x,\"5+6\");x=4",EXPR_EVD,""},
	{"alias(x,\"5+6\");x=4;8-->x",EXPR_EDS,"5+6"},
	{"throw(EFP)",EXPR_EFP,""},
	{"throw(EPT,\"12345\")",EXPR_EPT,"12345"},
	{"throw(9999,\"customerror9999\")",9999,"customerror9999"},
	{NULL,0,NULL}
};
void errcheck(const char *e,int expect,const char *expect_errinfo){
	int error;
	char errinfo[EXPR_SYMLEN];
	printf("checking %s --- expect \"%s\"",e,expr_error(expect));
	if(expr_new(e,"t",es,EXPR_IINSTANT_FREE,&error,errinfo)){
		printf("\nerror! %s should be \"%s\" but ok\n",e,expr_error(expect));
		goto ab;
	}
	if(error!=expect){
		printf("\nerror! %s should be \"%s\" but \"%s\"\n",e,expr_error(expect),expr_error(error));
		goto ab;
	}
	if(strcmp(errinfo,expect_errinfo)){
		printf("\nbad errorinfo! should be \"%s\" but is \"%s\"\n",expect_errinfo,errinfo);
		goto ab;
	}
	printf(" ... ok errinfo=\"%s\"\n",errinfo);
	return;
ab:
	printf("ABORTING\n");
	abort();
}
void check(const char *e,double expect){
	double r;
//	struct expr ep[1];
	//static int k=0;if(k++==39)exit(0);
	printf("checking %s --- expect %lg",e,expect);
//	if(expr_init(ep,e,"t",es,EXPR_IINSTANT_FREE)<0){
//		printf("\nerror! %s:%s\n",expr_error(ep->error),ep->errinfo);
//		goto ab;
//	}
	//exit(0);
	r=expr_calc5(e,NULL,NULL,es,EXPR_INOOPTIMIZE);
	if(memcmp(&r,&expect,sizeof(double))){
		printf("\nerror! %s should be %lg but %lg\n",e,expect,r);
		goto ab;
	}
	r=expr_calc5(e,NULL,NULL,es,0);
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
#pragma GCC diagnostic ignored "-Wunused-parameter"
int main(int argc,char **argv){
	double x0=expr_cast(expr_seed48(time(NULL)),double);
	expr_setup_mtl(3);
	es=expr_builtin_symbol_converts_r(expr_defmtl,expr_symbols_all);
	assert(es);
	expr_symset_add(es,"x0",EXPR_VARIABLE,0,&x0);
	expr_symset_add(es,"defined_symbol",EXPR_CONSTANT,0,2304.0);
	setvbuf(stdout,NULL,_IONBF,0);
	for(const struct proj *p=projs;p->e;++p)
		check(p->e,p->expect);
	for(const struct eproj *p=eprojs;p->e;++p)
		errcheck(p->e,p->expect,p->ee);
	for(const struct expr_builtin_keyword *p=expr_keywords;;++p){
		if(!p->str){
			break;
		}
		if(expr_symset_search(es,p->str,p->strlen)){
			printf("conflict %s\n",p->str);
			printf("ABORTING\n");
			abort();
		}
	}
	expr_symset_free(es);
	expr_setup_mtl(0);
	return 0;
}
