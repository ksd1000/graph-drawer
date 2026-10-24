#define _GNU_SOURCE
#include "expr.h"
#undef expr_init
#include <stdio.h>
#include <assert.h>
#include <time.h>
#include <getopt.h>
#include <err.h>

#include <stdlib.h>
#include <malloc.h>
//avoid being included again in expr_z.h

struct expr_memtool expr_defmtl[1];
#define malloc(s) expr_defmtl->allocate(s,expr_defmtl->arg)
#define calloc(s,n) expr_area_calloc((struct expr_area *)expr_defmtl->arg,(s)*(n))
#define realloc(o,s) ({void *_o=(o);(_o)?expr_defmtl->reallocate(_o,s,expr_defmtl->arg):expr_defmtl->allocate(s,expr_defmtl->arg);})
#define free(o) ({void *_o=(o);if(_o)expr_defmtl->deallocate(_o,expr_defmtl->arg);})

#define expr_eval expr_eval_z
#define expr expr_z
#include "expr_z.h"
//the expr_z.h is renamed form the expr.h form zserge_expr
//link: https://raw.githubusercontent.com/zserge/expr/refs/heads/master/expr.h
#undef expr_eval
#undef expr

#include "tinyexpr.h"
#include "tinyexpr.c"
//link: https://raw.githubusercontent.com/codeplea/tinyexpr/refs/heads/master/tinyexpr.h
//and https://raw.githubusercontent.com/codeplea/tinyexpr/refs/heads/master/tinyexpr.c
//patch the malloc by include

//compile: gcc -I ../expr -I ../tinyexpr -I ../graph-drawer/expr ../graph-drawer/test/speedtest.c ../graph-drawer/expr/expr-{core,internal,alloc}.c -o speedtest -Wall -Wextra -O3 -lm

long atol2(const char *str){
	long r;
	char *c;
	r=strtol(str,&c,10);
	if(c==str||*c)
		errx(EXIT_FAILURE,"invaild integer: %s",str);
	return r;
}
const char *expression="x0+5+5+5+5+3+(0.75+1.5)+7.0123456891011121314151617181920+x1";
struct expr_var_list vars={0};
struct expr_var *a=NULL;
double tx0=4,tx1=6;
volatile double x0=4,x1=6;
te_variable tvars[]={{"x0",&tx0,TE_VARIABLE,NULL},{"x1",&tx1,TE_VARIABLE,NULL}};
struct expr_symset es[1]={EXPR_SYMSET_INITIALIZER(expr_defmtl)};
__attribute__((noinline)) double gcc_eval_once(void){
	return x0+5+5+5+5+3+(0.75+1.5)+7.0123456891011121314151617181920+x1;
}
volatile double r=0;
void eval_g(unsigned int count){
	while(count--)
		r=gcc_eval_once();
}
void eval_z(unsigned int count){
	struct expr_z *ep;
	ep=expr_create(expression,strlen(expression),&vars,NULL);
	assert(ep);
	while(count--)
		expr_eval_z(ep);
	expr_destroy(ep,NULL);
}
void eval_t(unsigned int count){
	int err;
	te_expr *ep;
	ep=te_compile(expression,tvars,2,&err);
	assert(ep);
	while(count--)
		te_eval(ep);
	te_free(ep);
}
void eval_noop(unsigned int count){
	struct expr *ep;
	ep=expr_new4(expression,"t",es,EXPR_INOOPTIMIZE);
	assert(ep);
	while(count--)
		expr_eval(ep,0.0);
	expr_free(ep);
}
void eval_default(unsigned int count){
	struct expr *ep;
	ep=expr_new4(expression,"t",es,0);
	assert(ep);
	while(count--)
		expr_eval(ep,0.0);
	expr_free(ep);
}
void (*eval)(unsigned int)=eval_default;
char heap[8192];
int main(int argc,char **argv){
	size_t count=0;
	opterr=1;
	for(;;){
		switch(getopt(argc,argv,"n:oeztg")){
			case 'n':
				count=(size_t)atol2(optarg);
				break;
			case 'o':
				eval=eval_noop;
				break;
			case 'e':
				eval=eval_default;
				break;
			case 't':
				eval=eval_t;
				break;
			case 'z':
				eval=eval_z;
				break;
			case 'g':
				eval=eval_g;
				break;
			case -1:
				goto break2;
			default:
				break;
		}
	}
break2:
	;
	expr_setup_heapmtl(expr_defmtl,heap,sizeof(heap));
	assert((a=expr_var(&vars,"x0",2)));
	a->value=4;
	assert((a=expr_var(&vars,"x1",2)));
	a->value=6;
	assert(expr_symset_add(es,"x0",EXPR_VARIABLE,0,&x0));
	assert(expr_symset_add(es,"x1",EXPR_VARIABLE,0,&x1));
	eval(count);
	expr_symset_free(es);
	expr_destroy(NULL,&vars);
	return EXIT_SUCCESS;
}
