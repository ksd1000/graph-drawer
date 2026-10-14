//compile command: gcc compare.c -I ../expr ../expr/expr-core.c ../expr/expr-internal.c ../expr/expr-alloc.c -o compare -I ../../expr -lm -O3

#define _GNU_SOURCE
#include "expr.h"
#undef expr_init
#include <stdio.h>
#include <assert.h>
#include <time.h>

#include <stdlib.h>
#include <malloc.h>
//avoid being included again in expr_z.h

char internal_heap[1024*1024*16];
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

char *expression="x0+5+3+7+x0";

double dtime(void){
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME,&ts);
	return (double)ts.tv_sec+ts.tv_nsec/1000000000.0;
}
void area_show(struct expr_area *a){
	printf("heap monotonic: "
	"allocate=%u,"
	"allocate_fail=%u,"
	"deallocate=%u,"
	"expand_nail=%u,"
	"expand_move=%u,"
	"expand_fail=%u,"
	"shrink=%u,"
	"create=%u,"
	"combine=%u,"
	"retail_up=%u,"
	"retail_down=%u\n",
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
	//assert((!ai.leak&&!ai.leak_count));
}
struct expr_var_list vars={0};
struct expr_var *a;
struct expr_symset *es;
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
int main(void){
	unsigned int count=50000000;
	double ct,st,r=0;
	double x0=4;
	assert(!expr_setup_heapmtl4(mtl,internal_heap,sizeof(internal_heap),EXPR_ADEFAULT));
	assert(a=expr_var(&vars,"x0",2));
	a->value=4;
	assert(es=expr_symset_new_r(mtl));
	assert(expr_symset_add(es,"x0",EXPR_VARIABLE,0,&x0));

	printf("eval %s %u times\n",expression,count);

	expr_area_reset_monotonic((struct expr_area *)mtl->arg);
	rct r=(double)eval_z(count);
	printf("zserge_expr uses %.2lf ms,result=%lf\n",(ct-st)*1000,r);
	area_show((struct expr_area *)mtl->arg);

	expr_area_reset_monotonic((struct expr_area *)mtl->arg);
	rct r=eval(count,EXPR_INOOPTIMIZE);
	printf("expr(no optimize) uses %.2lf ms,result=%lf\n",(ct-st)*1000,r);
	area_show((struct expr_area *)mtl->arg);

	expr_area_reset_monotonic((struct expr_area *)mtl->arg);
	rct r=eval(count,0);
	printf("expr(optimized) uses %.2lf ms,result=%lf\n",(ct-st)*1000,r);
	area_show((struct expr_area *)mtl->arg);

	count=500000;

	printf("init+eval(once)+free %u times\n",count);

	rct for(unsigned int i=0;i<count;++i)eval_z(1);
	printf("zserge_expr uses %.2lf ms\n",(ct-st)*1000);

	rct for(unsigned int i=0;i<count;++i)eval(1,EXPR_INOOPTIMIZE);
	printf("expr(no optimize) uses %.2lf ms\n",(ct-st)*1000);

	rct for(unsigned int i=0;i<count;++i)eval(1,0);
	printf("expr(optimized) uses %.2lf ms\n",(ct-st)*1000);

	expr_symset_free(es);
	area_showcur((struct expr_area *)mtl->arg);
	return 0;
}

