#define _GNU_SOURCE
#include "expr.h"
#undef expr_init
#include <stdio.h>
#include <assert.h>
#include <time.h>

#include <stdlib.h>
#include <malloc.h>
//avoid being included again in expr_z.h

char internal_heap[1024*1024*8];
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

//compile command: gcc -I  ../../expr -I ../expr -I ../../tinyexpr compare.c (graph-drawer build path)/expr/libexpr.a -lm -Wall -Wextra -O3 -o compare

const char *expression="x0+5+3+(0.75+1.5)+7.01234568910111213141516+x1";
//tinyexpr does not allow bitwise and/or.

double dtime(void){
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME,&ts);
	return (double)ts.tv_sec+ts.tv_nsec/1000000000.0;
}
void area_show(struct expr_area *a){
	printf("heap monotonic: "
	"a=%u,"
	"af=%u,"
	"d=%u,"
	"en=%u,"
	"em=%u,"
	"ef=%u,"
	"s=%u,"
	"c=%u,"
	"co=%u,"
	"ru=%u,"
	"rd=%u\n",
	a->monotonic_allocate,
	a->monotonic_allocate_fail,
	a->monotonic_deallocate,
	a->monotonic_expand_nail,
	a->monotonic_expand_move,
	a->monotonic_expand_fail,
	a->monotonic_shrink,
	a->monotonic_create,
	a->monotonic_combine,
	a->monotonic_retail_up,
	a->monotonic_retail_down
	);
	expr_area_reset_monotonic(a);
}
void area_showcur(struct expr_area *a){
	struct expr_areainfo ai;
	assert(!expr_area_summary(a,&ai));
	printf("heap current: "
	"size=%zu,"
	"unit_count=%zu,"
	"free=%zu,"
	"free_count=%zu,"
	"free_max=%zu,"
	"leak=%zu,"
	"leak_count=%zu,"
	"leak_max=%zu,"
	"tail_size=%zu,"
	"tail_index=%zu,"
	"max=%zu\n",
	ai.size,
	ai.unit_count,
	ai.free,
	ai.free_count,
	ai.free_max,
	ai.leak,
	ai.leak_count,
	ai.leak_max,
	ai.tail_size,
	ai.tail_index,
	ai.max
	);
	assert((!ai.leak&&!ai.leak_count));
}
struct expr_var_list vars={0};
struct expr_var *a;
double tx0=4,tx1=6;
double x0=4,x1=6;
te_variable tvars[]={{"x0",&tx0,TE_VARIABLE,NULL},{"x1",&tx1,TE_VARIABLE,NULL}};
struct expr_symset es[1]={EXPR_SYMSET_INITIALIZER(mtl)};
double eval_t(unsigned int count){
	int err;
	struct te_expr *ep=te_compile(expression,tvars,2,&err);
	double r=0;
	assert(ep);
	while(count--)
		r=te_eval(ep);
	te_free(ep);
	return r;
}
float eval_z(unsigned int count){
	struct expr_z *ep=expr_create(expression,strlen(expression),&vars,NULL);
	float r=0;
	assert(ep);
	while(count--)
		r=expr_eval_z(ep);
	expr_destroy(ep,NULL);
	return r;
}
double eval(unsigned int count,int flag){
	struct expr *ep=expr_new4_r(expression,NULL,es,flag,mtl);
	double r=0;
	assert(ep);
	while(count--)
		r=expr_eval(ep,0.0);
	expr_free(ep);
	return r;
}
#define rct for(ct=0.0,st=dtime();ct==0.0;ct=dtime())
int main(int argc,char **argv){
	unsigned int count=50000000;
	double ct,st,r=0;
	if(argc>1)
		expression=argv[1];
	assert(!expr_setup_heapmtl4(mtl,internal_heap,sizeof(internal_heap),EXPR_ADEFAULT));
	assert((a=expr_var(&vars,"x0",2)));
	a->value=4;
	assert((a=expr_var(&vars,"x1",2)));
	a->value=6;
	assert(expr_symset_add(es,"x0",EXPR_VARIABLE,0,&x0));
	assert(expr_symset_add(es,"x1",EXPR_VARIABLE,0,&x1));

	printf("eval %s %u times\n",expression,count);
#define test_eval(func,name) \
	expr_area_reset_monotonic((struct expr_area *)mtl->arg);\
	rct r=(double)(func);\
	printf(#name " uses %.2lf ms,result=%.16lf\n",(ct-st)*1000,r);\
	area_show((struct expr_area *)mtl->arg)
	test_eval(eval_z(count),"zserge_expr");
	test_eval(eval_t(count),"tinyexpr");
	test_eval(eval(count,EXPR_INOOPTIMIZE),"expr(no optimize)");
	test_eval(eval(count,0),"expr(optimized)");


	count=500000;
	printf("init+free %u times\n",count);

#define test_ief(func,name) \
	rct for(unsigned int i=0;i<count;++i)(func);\
	printf(#name " uses %.2lf ms\n",(ct-st)*1000)
	test_ief(eval_z(0),"zserge_expr");
	test_ief(eval_t(0),"tinyexpr");
	test_ief(eval(0,EXPR_INOOPTIMIZE),"expr(no optimize)");
	test_ief(eval(0,0),"expr(optimized)");

	printf("init+eval(once)+free %u times\n",count);

#define test_ief(func,name) \
	rct for(unsigned int i=0;i<count;++i)(func);\
	printf(#name " uses %.2lf ms\n",(ct-st)*1000)
	test_ief(eval_z(1),"zserge_expr");
	test_ief(eval_t(1),"tinyexpr");
	test_ief(eval(1,EXPR_INOOPTIMIZE),"expr(no optimize)");
	test_ief(eval(1,0),"expr(optimized)");

	expr_symset_free(es);
	expr_destroy(NULL,&vars);
	area_showcur((struct expr_area *)mtl->arg);
	return 0;
}

