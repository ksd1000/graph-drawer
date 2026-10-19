#define _GNU_SOURCE
#include "expr.h"
#undef expr_init
#include <stdio.h>
#include <assert.h>
#include <time.h>
#include <assert.h>
#include "graph.h"
#ifdef __unix__
#define REAL_UNIX
#endif
#include "fake_unix.h"
#pragma GCC diagnostic ignored "-Wunused-result"

#include <stdlib.h>
#include <malloc.h>
//avoid being included again in expr_z.h

char internal_heap[1024*1024*8];
struct expr_memtool mtl[1];
#define malloc(s) mtl->allocate(s,mtl->arg)
#define calloc(s,n) expr_area_calloc((struct expr_area *)mtl->arg,(s)*(n))
#define realloc(o,s) ({void *_o=(o);(_o)?mtl->reallocate(_o,s,mtl->arg):mtl->allocate(s,mtl->arg);})
#define free(o) ({void *_o=(o);if(_o)mtl->deallocate(_o,mtl->arg);})


#include "tinyexpr.h"
#include "tinyexpr.c"
//link: https://raw.githubusercontent.com/codeplea/tinyexpr/refs/heads/master/tinyexpr.h
//and https://raw.githubusercontent.com/codeplea/tinyexpr/refs/heads/master/tinyexpr.c
//patch the malloc by include

//compile command: gcc -I ../tinyexpr -I ../graph-drawer -I ../graph-drawer/expr -I ../graph-drawer/graph ../graph-drawer/fun/compare-curve.c expr/libexpr.a graph/libgraph.a -lm -o compare-curve -Wall -Wextra -O3

const char *expression="x0+(5+5+5+5+3+(0.75+1.5))+7.0123456891011121314151617181920+x1";
//tinyexpr does not allow bitwise and/or.

double dtime(void){
	struct timespec ts;
	clock_gettime(CLOCK_THREAD_CPUTIME_ID,&ts);
	return (double)ts.tv_sec+ts.tv_nsec/1000000000.0;
}
double tx0=4,tx1=6;
volatile double x0=4,x1=6;
te_variable tvars[]={{"x0",&tx0,TE_VARIABLE,NULL},{"x1",&tx1,TE_VARIABLE,NULL}};
struct expr_symset es[1]={EXPR_SYMSET_INITIALIZER(mtl)};
double eval_t(struct te_expr *ep,unsigned int count){
	int err=0;
	double r=0;
	ep=te_compile(expression,tvars,2,&err);
	assert(ep);
	while(count--)
		r=te_eval(ep);
	te_free(ep);
	return r;
}
double eval(struct expr *ep,unsigned int count,int flag){
	double r=0;
	ep=expr_new4_r(expression,NULL,es,flag,mtl);
	assert(ep);
	while(count--)
		r=expr_eval(ep,0.0);
	expr_free(ep);
	return r;
}
#define MAX_TIMES 1600
struct {
	double tiny[MAX_TIMES+1];
	double expr[MAX_TIMES+1];
	double expn[MAX_TIMES+1];
} cost;
double evalx(double input,void *arg){
	(void)arg;
	return input;
}
double evaly(double input,void *arg){
	if((size_t)input>MAX_TIMES)
		return NAN;
	return ((double *)arg)[(size_t)input];
}
#define FBOLD 1
void draw_curve(double max_time){
	struct graph gp[1];
	intptr_t tofd;
	volatile double current;
	int32_t l=0;
	int32_t y0;
	assert(!init_graph(gp,1280,720,32,-1,MAX_TIMES+0.5,-64,max_time+0.5));
	graph_fill(gp,0x7fffffff);
	gp->connect=0;
	graph_draw_grid(gp,0xffbfbfbf,0,MAX_TIMES/16,max_time/8);
	graph_draw_axis(gp,0xff000000,0,MAX_TIMES/16,max_time/8,16);
	graph_draw(gp,0xff0000ff,FBOLD,evalx,evaly,NULL,cost.tiny,0,MAX_TIMES,0.125,&current);
	l=128;
	y0=graph_ytop(gp,0.0)+32;
	l+=graph_draw_text_pixel(gp,0xff0000ff,0,"tinyexpr",4,48,l,y0)+32;
	graph_draw(gp,0xff00ff00,FBOLD,evalx,evaly,NULL,cost.expr,0,MAX_TIMES,0.125,&current);
	l+=graph_draw_text_pixel(gp,0xff00ff00,0,"gdexpr",4,48,l,y0)+32;
//	graph_draw(gp,0xffff0000,FBOLD,evalx,evaly,NULL,cost.expn,0,MAX_TIMES,0.125,&current);
//	l+=graph_draw_text_pixel(gp,0xffff0000,0,"gdexpr(NO)",4,48,l,y0)+32;
	graph_draw_text_pixel(gp,0xff000000,0,"eval times",4,32,1024,y0);
	graph_draw_text_pixel(gp,0xff000000,0,"time cost(ms)",4,32,48,680);
	graph_draw_text_pixel(gp,0xff007f00,0,expression,4,32,48,600);
//	graph_draw_text_pixel(gp,0xff2020ff,0,"Drawer",4,320,288,64);
//	graph_draw_text_pixel(gp,0xff007f00,0,"Expression-to-image-and-audio converter",4,72,12,16);
	graph_draw_box_pixel(gp,0xffffffff,0,0,0,gp->width-1,gp->height-1);
	tofd=open("curve.bmp",O_WRONLY_CREAT);
	// the curve.png is converted by imagemagick.
	assert(!fderr(tofd));
	assert(write(tofd,gp->bm,gp->bm->size)==(ssize_t)gp->bm->size);
	close(tofd);
	graph_free(gp);
}
#define rct for(ct=0.0,st=dtime();ct==0.0;ct=dtime())
int main(int argc,char **argv){
	unsigned int count=8192;
	double ct,st,tt,et/*,ent*/;
	double max_time=0.0;
	if(argc>1)
		expression=argv[1];
	assert(!expr_setup_heapmtl4(mtl,internal_heap,sizeof(internal_heap),EXPR_ADEFAULT));
	assert(expr_symset_add(es,"x0",EXPR_VARIABLE,0,&x0));
	assert(expr_symset_add(es,"x1",EXPR_VARIABLE,0,&x1));

#define test_ief(func,tdest) \
	rct for(unsigned int i=0;i<count;++i)(func);\
	tdest=(ct-st)*1000.0
#define test_once(times)\
		test_ief(eval_t(NULL,times),tt);\
		test_ief(eval(NULL,times,EXPR_ISCANCACHE),et)
//		test_ief(eval(NULL,times,EXPR_INOOPTIMIZE|EXPR_INOSHRINK|EXPR_ISCANCACHE),ent)
	test_once(0);
	test_once(1);
	for(unsigned int times=0;times<=MAX_TIMES;++times){
		test_once(times);
		cost.tiny[times]=tt;
		cost.expr[times]=et;
//		cost.expn[times]=ent;
//		printf("times=%-6u,t=%-8.2lfms %-2s e=%-8.2lfms %-2s en=%-8.2lfms\n",times,tt,tt<et?"<":">",et,et<ent?"<":">",ent);
		printf("times=%-6u,t=%-8.2lfms %-2s e=%-8.2lfms\n",times,tt,tt<et?"<":">",et);
	}
	for(unsigned int i=0;i<=(MAX_TIMES+1)*2;++i){
		ct=((double *)&cost)[i];
		if(ct>max_time)
			max_time=ct;
	}
		printf("max_time=%.2lf\n",max_time);
	expr_symset_free(es);
	draw_curve(max_time);
	return 0;
}

