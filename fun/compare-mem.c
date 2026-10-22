#define _GNU_SOURCE
#include "expr.h"
#undef expr_init
#include <stdio.h>
#include <assert.h>
#include <time.h>

#include <stdlib.h>
#include <malloc.h>
//avoid being included again in expr_z.h

char internal_heap[1024*1024];
char internal_backup[1024*1024];
struct expr_memtool mtl[1];
#define malloc(s) mtl->allocate(s,mtl->arg)
#define calloc(s,n) expr_area_calloc((struct expr_area *)mtl->arg,(s)*(n))
#define realloc(o,s) ({void *_o=(o);(_o)?mtl->reallocate(_o,s,mtl->arg):mtl->allocate(s,mtl->arg);})
#define free(o) ({void *_o=(o);if(_o)mtl->deallocate(_o,mtl->arg);})

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

//compile command: gcc -I ../expr -I ../tinyexpr -I ../graph-drawer/expr ../graph-drawer/fun/compare-mem.c expr/libexpr.a -lm -o compare-mem -Wall -Wextra -O3

struct expr_var_list vars={0};
struct expr_var *a=NULL;
double tx=4;
double x=4;
te_variable tvars[]={{"x",&x,TE_VARIABLE,NULL}};
struct expr_symset es[1]={EXPR_SYMSET_INITIALIZER(mtl)};
int test_tiny(char *e,size_t len){
	int err=0;
	char old;
	te_expr *ep;
	old=e[len];
	e[len]=0;
	ep=te_compile(e,tvars,2,&err);
	e[len]=old;
	// pass the length of expression is not allowed by tinyexpr
	if(!ep)
		return 0;
	te_free(ep);
	return 1;
}
int test_zserge(char *e,size_t len){
	struct expr_z *ep;
	ep=expr_create(e,len,&vars,NULL);
	if(!ep)
		return 0;
	expr_destroy(ep,NULL);
	return 1;
}
int test_expr(char *e,size_t len){
	return !!expr_new6_r(e,len,NULL,0,es,EXPR_IINSTANT_FREE|EXPR_INOOPTIMIZE|EXPR_INOSHRINK,mtl);
}
char buf[1024*128];
size_t get_max_available(char *e,size_t len,int (*testee)(char *e,size_t len)){
	size_t cur=1,step=len/2;
	assert(len&1);
	while(step>=2){
		printf("-------- current size=%zu,cur=%zu,step=%zu\n",cur+step,cur,step);
		memcpy(internal_heap,internal_backup,sizeof(internal_heap));
		// zserge expr may make memory leak on fail,
		// clean it.
		// do not use expr_area_wipe(), which will wipe
		// the symset together.
		if(cur+step<=len&&testee(e,cur+step)){
			cur+=step;
		}else {
			//abort();
			step=(step/2)&~(size_t)1;
		}
	}
	return cur;
}
int main(int argc,char **argv){
	ssize_t r;
	(void)argc;
	(void)argv;
	(void)expr_eval_z;
	assert(!expr_setup_heapmtl(mtl,internal_heap,sizeof(internal_heap)));
	assert((a=expr_var(&vars,"x",2)));
	a->value=4;
	assert(expr_symset_add(es,"x",EXPR_VARIABLE,0,&x));
	printf("filling x+x+x+...\n");
	r=expr_snprintf_r(buf,sizeof(buf),"%1T+%*l%s",9,mtl,sizeof(buf)/2,"x");
	// "%1T+": delimed by "+" between "x", 1: size of "x"
	// "%*l" : loop for "*"(read form arg) times.
	// "%s" : "x"
	assert((size_t)r==sizeof(buf)-1);
	memcpy(internal_backup,internal_heap,sizeof(internal_heap));
	printf("tinyexpr: %zu\n",get_max_available(buf,(size_t)r,test_tiny));
	printf("zserge expr: %zu\n",get_max_available(buf,(size_t)r,test_zserge));
	printf("graph-drawer expr: %zu\n",get_max_available(buf,(size_t)r,test_expr));
	


	expr_symset_free(es);
	expr_destroy(NULL,&vars);
	return 0;
}

