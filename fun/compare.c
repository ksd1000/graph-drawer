#define _GNU_SOURCE
#include "expr.h"
#undef expr_init
#include <stdio.h>
#include <assert.h>
#include <time.h>

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
	struct expr *ep=expr_new4(expression,NULL,es,flag);
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
	assert(a=expr_var(&vars,"x0",2));
	a->value=4;
	assert(es=expr_symset_new());
	assert(expr_symset_add(es,"x0",EXPR_VARIABLE,0,&x0));
	printf("eval %s %u times\n",expression,count);
	rct r=(double)eval_z(count);
	printf("zserge_expr uses %.2lf ms,result=%lf\n",(ct-st)*1000,r);
	rct r=eval(count,EXPR_INOOPTIMIZE);
	printf("expr(no optimize) uses %.2lf ms,result=%lf\n",(ct-st)*1000,r);
	rct r=eval(count,0);
	printf("expr(optimized) uses %.2lf ms,result=%lf\n",(ct-st)*1000,r);
	count=500000;
	printf("init+eval(once)+free %u times\n",count);
	rct for(unsigned int i=0;i<count;++i)eval_z(1);
	printf("zserge_expr uses %.2lf ms\n",(ct-st)*1000);
	rct for(unsigned int i=0;i<count;++i)eval(1,EXPR_INOOPTIMIZE);
	printf("expr(no optimize) uses %.2lf ms\n",(ct-st)*1000);
	rct for(unsigned int i=0;i<count;++i)eval(1,0);
	printf("expr(optimized) uses %.2lf ms\n",(ct-st)*1000);
	return 0;
}

