#define _GNU_SOURCE
#include "expr.h"
#undef expr_init
#include <stdio.h>
#include <assert.h>
#include <time.h>
#include <getopt.h>
#include <err.h>
#include <time.h>
#include "graph.h"

#include <unistd.h>
#include <fcntl.h>
#include <unistd.h>
#include <err.h>
#include <pthread.h>
#define fderr(fd) ((fd)<0)
#define ERRFD (-1)
#define O_WRONLY_CREAT O_WRONLY|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR

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

//compile: gcc -I ../expr -I ../tinyexpr -I ../graph-drawer/expr -I ../graph-drawer/graph ../graph-drawer/test/speedtest.c ../graph-drawer/expr/expr-{core,internal,alloc}.c graph/libgraph.a -o speedtest -Wall -Wextra -O3 -lm
//
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
void eval_g(size_t count){
	while(count--)
		r=gcc_eval_once();
}
void eval_z(size_t count){
	struct expr_z *ep;
	ep=expr_create(expression,strlen(expression),&vars,NULL);
	assert(ep);
	while(count--)
		expr_eval_z(ep);
	expr_destroy(ep,NULL);
}
void eval_t(size_t count){
	int err;
	te_expr *ep;
	ep=te_compile(expression,tvars,2,&err);
	assert(ep);
	while(count--)
		te_eval(ep);
	te_free(ep);
}
void eval_noop(size_t count){
	struct expr *ep;
	ep=expr_new4(expression,"t",es,EXPR_INOOPTIMIZE);
	assert(ep);
	while(count--)
		expr_eval(ep,0.0);
	expr_free(ep);
}
void eval_default(size_t count){
	struct expr *ep;
	ep=expr_new4(expression,"t",es,0);
	assert(ep);
	while(count--)
		expr_eval1(ep);
	expr_free(ep);
}
void (*eval)(size_t)=eval_default;
char heap[8192];
double dtime(void){
	struct timespec ts;
	clock_gettime(CLOCK_THREAD_CPUTIME_ID,&ts);
	return (double)(ts.tv_sec*1000000000+ts.tv_nsec);
}
#define rct(v) for(double _ct=0.0,_st=dtime();_ct==0.0||(((v)=_ct-_st),0);_ct=dtime())
double avg(void (*fp)(size_t),size_t count,size_t n){
	double sum=0.0;
	double n0=(double)n;
	double r=0.0;
	assert(n);
	do {
		rct(r){
			fp(count);
		}
		assert(r>=0.0);
		sum+=r;
	}while(--n);
	return sum/n0;
}
int output=0;
size_t avgn=1;
#define MAXCOUNT 2048
struct result {
	double zserge[MAXCOUNT+1];
	double tiny[MAXCOUNT+1];
	double expr[MAXCOUNT+1];
	double expn[MAXCOUNT+1];
	double gcc[MAXCOUNT+1];
};
struct result res[1];
double maxt=0.0;
void timecost(void (*fp)(size_t),size_t count,double *out){
	double t=avg(fp,count,avgn);
	if(out&&count<=MAXCOUNT){
		if(t>maxt){
			//fprintf(stderr,"max time: %lf -> %lf ,count: %zu\n",maxt,t,count);
			maxt=t;
		}
		out[count]=t;
	}
}
void timecost_write(void (*fp)(size_t),size_t count,double *out){
	for(size_t i=0;i<=count;++i)
		timecost(fp,i,NULL);
	for(size_t i=0;i<=count;++i)
		timecost(fp,i,out);
}
void correct(double *res,size_t count){
	for(size_t i=1;i<count;++i){
		if((res[i]>res[i-1]&&res[i]>res[i+1])||(res[i]<res[i-1]&&res[i]<res[i+1])){
			double d;
			d=res[i-1]+res[i+1];
			res[i]=d/2;
		}
	}
}
double evalx(double input,void *arg){
	(void)arg;
	return input;
}
double evaly(double input,void *arg){
	if((size_t)input>MAXCOUNT)
		return NAN;
	return ((double *)arg)[(size_t)input];
}
#define FBOLD 1
void draw_curve(const char *file,size_t count){
	struct graph gp[1];
	intptr_t tofd;
	volatile double current;
	int32_t l=0;
	int32_t y0;
	assert(!init_graph(gp,1280,720,32,-1,(double)count+0.5,-maxt/16,maxt+0.5));
	graph_fill(gp,0xefffffff);
	gp->connect=1;
	graph_draw_grid(gp,0xffbfbfbf,0,(double)count/16,maxt/8);
	graph_draw_axis(gp,0xff000000,0,(double)count/16,maxt/8,16);
	l=600-64;
	y0=graph_ytop(gp,0.0)+1;
#define DRAW(color,name,addr) \
	graph_draw_text_pixel(gp,color,0,name,4,32,72,l);\
	l-=32;\
	graph_draw(gp,color,FBOLD,evalx,evaly,NULL,addr,0,MAXCOUNT,1,&current)
	DRAW(0xff00ffff,"graph-drawer expr(no optimize)",res->expn);
	DRAW(0xff00ff00,"graph-drawer expr(optimized)",res->expr);
	DRAW(0xffff0000,"zserge expr",res->zserge);
	DRAW(0xff0000ff,"tinyexpr",res->tiny);
	DRAW(0xffa0522d,"gcc/clang",res->gcc);

	graph_draw_text_pixel(gp,0xff000000,0,"eval times",4,32,1024,y0-16);
	graph_draw_text_pixel(gp,0xff000000,0,"time cost(ns)",4,32,48,680);
	graph_draw_text_pixel(gp,0xff007f00,0,expression,4,32,48,600);
	graph_draw_box_pixel(gp,0xffffffff,0,0,0,gp->width-1,gp->height-1);
	tofd=open(file,O_WRONLY_CREAT);
	assert(!fderr(tofd));
	assert(write(tofd,gp->bm,gp->bm->size)==(ssize_t)gp->bm->size);
	close(tofd);
	graph_free(gp);
}
int main(int argc,char **argv){
	size_t count=0;
	const char *file=NULL;
	opterr=1;
	for(;;){
		switch(getopt(argc,argv,"a:n:Neztgo::")){
			case 'a':
				avgn=(size_t)atol2(optarg);
				break;
			case 'n':
				count=(size_t)atol2(optarg);
				break;
			case 'N':
				eval=eval_noop;
				break;
			case 'o':
				output=1;
				if(optarg)
					file=optarg;
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
	if(output){
		timecost_write(eval_z,count,res->zserge);
		timecost_write(eval_t,count,res->tiny);
		timecost_write(eval,count,res->expr);
		timecost_write(eval_noop,count,res->expn);
		timecost_write(eval_g,count,res->gcc);
		correct(res->zserge,count);
		correct(res->tiny,count);
		correct(res->expr,count);
		correct(res->expn,count);
		correct(res->gcc,count);
		if(file){
			draw_curve(file,count);
		}else {
			printf("eval-times,zserge-expr,tinyexpr,graph-drawer-expr\n");
			for(size_t i=0;i<=count;++i)
				printf("%zu,%.16lf,%.16lf,%.16lf\n",i,res->zserge[i],res->tiny[i],res->expr[i]);
		}
		fprintf(stderr,"max time: %lf\n",maxt);
	}else
		eval(count);
	expr_symset_free(es);
	expr_destroy(NULL,&vars);
	return EXIT_SUCCESS;
}
