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

//compile command: gcc -I ../expr -I ../tinyexpr -I ../graph-drawer/expr ../graph-drawer/fun/compare.c expr/libexpr.a -lm -o compare -Wall -Wextra -O3

const char *expression="x0+5+5+5+5+3+(0.75+1.5)+7.0123456891011121314151617181920+x1";
//tinyexpr does not allow bitwise and/or.

double dtime(void){
	struct timespec ts;
	clock_gettime(CLOCK_THREAD_CPUTIME_ID,&ts);
	return (double)ts.tv_sec+ts.tv_nsec/1000000000.0;
}
size_t mem_used=0,mem_max=0,mem_cur=0,mem_old=0;
void *expr_heapmtl_malloc_hook(size_t size,intptr_t arg){
	void *r;
	//printf("malloc=%zu\n",size);
	r=expr_area_malloc((struct expr_area *)arg,size);
	if(r){
		size_t n;
		n=expr_memlen(r);
		mem_used+=n;
		if(n>mem_max)
			mem_max=n;
	//printf("memlen=%zu\n",expr_memlen(r));
	}
	return r;
}
void *expr_heapmtl_realloc_hook(void *old,size_t size,intptr_t arg){
	size_t olds,new;
	void *r;
	olds=expr_memlen(old);
	r=expr_area_realloc((struct expr_area *)arg,old,size);
	if(r){
		new=expr_memlen(r);
		if(new>olds){
			size_t n;
			n=new-olds;
			mem_used+=n;
			if(n>mem_max)
				mem_max=n;
			}
	}
	return r;
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
	"rd=%u\n\n",
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
size_t area_size(struct expr_area *a){
	struct expr_areainfo ai;
	assert(!expr_area_summary(a,&ai));
	return ai.leak;
}
struct expr_var_list vars={0};
struct expr_var *a=NULL;
double tx0=4,tx1=6;
volatile double x0=4,x1=6;
te_variable tvars[]={{"x0",&tx0,TE_VARIABLE,NULL},{"x1",&tx1,TE_VARIABLE,NULL}};
struct expr_symset es[1]={EXPR_SYMSET_INITIALIZER(mtl)};
__attribute__((noinline)) double gcc_eval_once(void){
	return x0+5+5+5+5+3+(0.75+1.5)+7.0123456891011121314151617181920+x1;
}
double eval_g(unsigned int count){
	volatile double r=0;
	while(count--)
		r=gcc_eval_once();
	mem_cur=area_size((struct expr_area *)mtl->arg);
	return r;
}
double eval_t(struct te_expr *ep,unsigned int count){
	int err=0;
	int fr=0;
	double r=0;
	if(!ep){
		ep=te_compile(expression,tvars,2,&err);
		fr=1;
	}
	assert(ep);
	while(count--)
		r=te_eval(ep);
	mem_cur=area_size((struct expr_area *)mtl->arg);
	if(fr)
		te_free(ep);
	return r;
}
float eval_z(struct expr_z *ep,unsigned int count){
	float r=0;
	int fr=0;
	if(!ep){
		ep=expr_create(expression,strlen(expression),&vars,NULL);
		fr=1;
	}
	assert(ep);
	while(count--)
		r=expr_eval_z(ep);
	mem_cur=area_size((struct expr_area *)mtl->arg);
	if(fr)
		expr_destroy(ep,NULL);
	return r;
}
double eval(struct expr *ep,unsigned int count,int flag){
	double r=0;
	int fr=0;
	if(!ep){
		ep=expr_new4_r(expression,NULL,es,flag,mtl);
		fr=1;
	}
	assert(ep);
	while(count--)
		r=expr_eval(ep,0.0);
	mem_cur=area_size((struct expr_area *)mtl->arg);
	if(fr)
		expr_free(ep);
	return r;
}
#define rct for(ct=0.0,st=dtime();ct==0.0;ct=dtime())
int main(int argc,char **argv){
	unsigned int count=50000000;
	double ct,st,r=0;
	struct expr *ep;
	struct expr_z *ezp;
	te_expr *tp;
	int err;
	if(argc>1)
		expression=argv[1];
	assert(!expr_setup_heapmtl4(mtl,internal_heap,sizeof(internal_heap),EXPR_ADEFAULT));
	mtl->allocate=expr_heapmtl_malloc_hook;
	mtl->reallocate=expr_heapmtl_realloc_hook;
	assert((a=expr_var(&vars,"x0",2)));
	a->value=4;
	assert((a=expr_var(&vars,"x1",2)));
	a->value=6;
	assert(expr_symset_add(es,"x0",EXPR_VARIABLE,0,&x0));
	assert(expr_symset_add(es,"x1",EXPR_VARIABLE,0,&x1));
	mem_old=area_size((struct expr_area *)mtl->arg);
	printf("eval %s %u times\n",expression,count);
#define test_eval(p,init,free,func,name) \
	mem_used=0;\
	p=init;\
	assert(p);\
	mem_max=0;\
	expr_area_reset_monotonic((struct expr_area *)mtl->arg);\
	rct r=(double)(func);\
	free;\
	printf(name " uses %.2lf ms,result=%.16lf,mem=%zu/%zu/%zu\n",(ct-st)*1000,r,mem_cur-mem_old,mem_used,mem_max);\
	area_show((struct expr_area *)mtl->arg)
	test_eval(ezp,expr_create(expression,strlen(expression),&vars,NULL),expr_destroy(ezp,NULL),eval_z(ezp,count),"zserge_expr");
	test_eval(tp,te_compile(expression,tvars,2,&err),te_free(tp),eval_t(tp,count),"tinyexpr");
	test_eval(ep,expr_new4_r(expression,NULL,es,EXPR_INOOPTIMIZE|EXPR_INOSHRINK,mtl),expr_free(ep),eval(ep,count,EXPR_INOOPTIMIZE|EXPR_INOSHRINK),"expr(no optimize)");
	test_eval(ep,expr_new4_r(expression,NULL,es,0,mtl),expr_free(ep),eval(ep,count,0),"expr(optimized)");
	test_eval(ezp,(void *)1,,eval_g(count),"gcc/clang");


	count=500000;
	printf("init+free %u times\n",count);

#define test_ief(func,name) \
	rct for(unsigned int i=0;i<count;++i)(func);\
	printf(name " uses %.2lf ms\n",(ct-st)*1000)
	test_ief(eval_z(NULL,0),"zserge_expr");
	test_ief(eval_t(NULL,0),"tinyexpr");
	test_ief(eval(NULL,0,EXPR_INOOPTIMIZE|EXPR_INOSHRINK|EXPR_ISCANCACHE),"expr(no optimize)");
	test_ief(eval(NULL,0,0),"expr(optimized)");

	printf("\ninit+eval(once)+free %u times\n",count);

	test_ief(eval_z(NULL,1),"zserge_expr");
	test_ief(eval_t(NULL,1),"tinyexpr");
	test_ief(eval(NULL,1,EXPR_INOOPTIMIZE|EXPR_INOSHRINK|EXPR_ISCANCACHE),"expr(no optimize)");
	test_ief(eval(NULL,1,0),"expr(optimized)");

	expr_symset_free(es);
	expr_destroy(NULL,&vars);
	area_showcur((struct expr_area *)mtl->arg);
	return 0;
}

