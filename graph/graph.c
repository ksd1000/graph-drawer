/*******************************************************************************
 *License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>*
 *This is free software: you are free to change and redistribute it.           *
 *******************************************************************************/
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <float.h>
#ifdef __unix__
#include <pthread.h>
#include <alloca.h>
#endif
#include "graph.h"
#include "texts/text.h"
#define muldiv(m1,m2,f) ((uint64_t)(m1)*(m2)/(f))
#define muldiv_up(m1,m2,f) ({\
	int64_t _p=(m1)*(m2);\
	int32_t _f=(f);\
	_p/_f+!!(_p%_f);\
})
#define xisnan(x) (isnan(*(x)))
#define max(a,b) ({\
	__auto_type _a=(a);\
	__auto_type _b=(b);\
	_a>_b?_a:_b;\
})
#define min(a,b) ({\
	__auto_type _a=(a);\
	__auto_type _b=(b);\
	_a<_b?_a:_b;\
})
#define manhattan(x1,y1,x2,y2) (abs((x1)-(x2))+abs((y1)-(y2)))
//#define free(v)
//#define printf(v, ... ) abort()
static int shouldconnect(const struct graph *restrict gp,int32_t x1,int32_t y1,int32_t x2,int32_t y2){
	int32_t dmax;
	switch(manhattan(x1,y1,x2,y2)){
		case 0:
		case 1:
			return 0;
		case 2:
			return (x1==y1||x2==y2);
		default:
			dmax=gp->width+gp->height;
			//printf("dx=%u,dy=%u,max=%u\n",abs(x2-x1),abs(y2-y1),dmax);
			if(
				((x1<0||x1>=gp->width)||
				(y1<0||y1>=gp->height))&&
				((x2<0||x2>=gp->width)||
				(y2<0||y2>=gp->height))&&
				(abs(x2-x1)>=dmax||abs(y2-y1)>=dmax)
				)
				return 0;
			return 1;
	}
}
#define xtop(x) ((int32_t)(((x)-gp->minx)/(gp->maxx-gp->minx)*gp->width))
#define ytop(y) ((int32_t)(((y)-gp->miny)/(gp->maxy-gp->miny)*gp->height))
#define inrange(x,y) ((x)>=0&&(x)<gp->width&&(y)>=0&&(y)<gp->height)
#define setpixel(px,py)\
	bm_setpixel6(gp->buf,gp->bpp,gp->byte_width,px,py,color)
void graph_setpixel_bold(struct graph *restrict gp,uint32_t color,int32_t bold,int32_t x,int32_t y){
	int32_t px,endx,py,py1,endy;
	px=x-bold;
//	if(px>=gp->width)return;
	py1=y-bold;
//	if(py>=gp->height)return;
	endx=x+bold;
//	if(endx<0)return;
	endy=y+bold;
//	if(endy<0)return;
//	if(!inrange(px,py)||!inrange(endx,endy))return;
	if(bold>0){
//		if(px<0)px=0;
//		if(py<0)py=0;
//		if(endx>=gp->width)endx=gp->width-1;
//		if(endy>=gp->height)endy=gp->height-1;
		for(;px<=endx;++px){
			for(py=py1;py<=endy;++py){
				if((int32_t)hypot(fabs((double)(px-x)),fabs((double)(py-y)))<=bold){
					//printf("%d,%d,%d,%d,%d\n",px,py,endx,endy,inrange(px,py));
					if(inrange(px,py))setpixel(px,py);
					//graph_setpixel(gp,color,px,py);
				}
			}
		}
	}else if(inrange(x,y))
		setpixel(x,y);
	//graph_setpixel(gp,color,x,y);
}
static int32_t graph_drawchar(struct graph *restrict gp,uint32_t color,int32_t bold,int c,int32_t height,int32_t x,int32_t y,int test){
	const struct sbmp *text=text_getsbmp(c);
	int32_t md1,md2;
	int32_t width;
	//printf("%c\n",c);
	//printf("at %d,%d height:%d text->height:%d\n",x,y,height,text->height);
	if(!text){
		return 0;
	}
	width=muldiv(text->width,height,text->height);
	if(test)goto end;
	if(!sbmp_decompress(text,(struct sbmp *)gp->textbuf))
		text=(void *)gp->textbuf;
	if(height>text->height){
	for(int32_t xi=x;(md1=muldiv(xi-x,text->width,width))<text->width&&xi-x<width&&xi<gp->width;++xi){
	for(int32_t yi=y;(md2=muldiv(yi-y,text->height,height))<text->height&&yi-y<height&&yi<gp->height;++yi){
		//printf("at %d,%d\n",xi,yi);
		if(!sbmp_tstpixel(text,md1,md2))continue;
		graph_setpixel_bold(gp,color,bold,xi,yi);
	}
	}
	}else if(height<text->height){
	for(int32_t xi=0;(md1=muldiv(xi,width,text->width))<gp->width&&xi<text->width&&md1-x<width;++xi){
	for(int32_t yi=0;(md2=muldiv(yi,height,text->height))<gp->height&&yi<text->height&&md2-y<height;++yi){
		if(!sbmp_tstpixel(text,xi,yi))continue;
		//printf("at %d,%d\n",xi,yi);
		graph_setpixel_bold(gp,color,bold,x+md1,y+md2);
	}
	}
	}else {
	for(int32_t xi=x;xi-x<width&&xi<gp->width;++xi){
	for(int32_t yi=y;yi-y<height&&yi<gp->height;++yi){
		//printf("at %d,%d\n",xi,yi);
		if(!sbmp_tstpixel(text,xi-x,yi-y))continue;
		graph_setpixel_bold(gp,color,bold,xi,yi);
	}
	}
	}
end:
	return width;
}
int32_t graph_draw_text_pixel(struct graph *restrict gp,uint32_t color,int32_t bold,const char *s,int32_t gap,int32_t height,int32_t x,int32_t y){
	if(!*s)return x;
	for(;*s;++s){
		x+=graph_drawchar(gp,color,bold,*s,height,x,y,0)+gap;
	}
	return x-gap;
}
int32_t graph_textlen(struct graph *restrict gp,const char *s,int32_t gap,int32_t height){
	int32_t x=0;
	if(!*s)return x;
	for(;*s;++s){
		x+=graph_drawchar(gp,0,0,*s,height,0,0,1)+gap;
	}
	return x-gap;
}
int32_t graph_text_height(void){
	return TEXT_HEIGHT;
}
int32_t graph_draw_text(struct graph *restrict gp,uint32_t color,int32_t bold,const char *s,int32_t gap,int32_t height,double x,double y){
	if(xisnan(&x)||xisnan(&y))return -1;
	return graph_draw_text_pixel(gp,color,bold,s,gap,height,xtop(x),ytop(y));
}
int32_t graph_xtop(const struct graph *restrict gp,double x){
	if(xisnan(&x))return INT32_MAX;
	return xtop(x);
}
int32_t graph_ytop(const struct graph *restrict gp,double y){
	if(xisnan(&y))return INT32_MAX;
	return ytop(y);
}
void graph_fill(struct graph *restrict gp,uint32_t color){
	char *p,*buf=gp->buf;
	int32_t x,y;
	for(y=0;y<gp->height;++y,buf+=gp->byte_width){
		p=buf;
		for(x=0;x<gp->width;p+=gp->bpp,++x){
			memcpy(p,&color,gp->bpp);
		}
	}
}
int init_graph_frombmp(struct graph *restrict gp,const void *bmp,size_t size,double minx,double maxx,double miny,double maxy){
	int r;
#define inbmp ((const struct bitmap *)bmp)
	if(bm_check(inbmp,size)<0)
		return -1;
	r=init_graph(gp,inbmp->width,inbmp->height,inbmp->bpp,minx,maxx,miny,maxy);
	if(r<0)
		return r;
	memcpy(gp->buf,bmp+inbmp->pindex,size-inbmp->pindex);
	return 0;
#undef inbmp
}
int init_graph(struct graph *restrict gp,int32_t width,int32_t height,uint16_t bpp,double minx,double maxx,double miny,double maxy){
	gp->byte_width=((width*bpp+31)>>5)<<2;
	if(xisnan(&minx)||xisnan(&maxx)||xisnan(&miny)||xisnan(&maxy))return -1;
	if(width<=0||height<=0||bpp<8||bpp&7||minx>=maxx||miny>=maxy)
		return -1;
	gp->hsize=54;
	if((gp->bm=malloc(gp->hsize+gp->byte_width*height))==NULL)
		return -2;
	if((gp->textbuf=malloc(TEXT_MAXOSIZE))==NULL){
		free(gp->buf);
		return -2;
	}
	gp->width=width;
	gp->height=height;
	gp->minx=minx;
	gp->maxx=maxx;
	gp->miny=miny;
	gp->maxy=maxy;
	gp->connect=1;
	gp->arrow=1;
	gp->draw_value=1;
	gp->bpp=bpp/8;
	bm_init(gp->bm,width,height,bpp);
	gp->buf=gp->bm->data;
	return 0;
}
void graph_free(struct graph *restrict gp){
	free(gp->buf-gp->hsize);
	free(gp->textbuf);
}


static void graph_draw_vline(struct graph *restrict gp,uint32_t color,int32_t bold,int32_t start,int32_t end,int32_t x){
	for(;start<=end;++start)
		graph_setpixel_bold(gp,color,bold,x,start);
}
static void graph_draw_hline(struct graph *restrict gp,uint32_t color,int32_t bold,int32_t start,int32_t end,int32_t y){
	
	for(;start<=end;++start)
		graph_setpixel_bold(gp,color,bold,start,y);
}
void graph_draw_point6(struct graph *restrict gp,uint32_t color,int32_t bold,double x,double y,int32_t last[2]){
	//printf("x=%lf,y=%lf,xisnan(y)=%lu\n",x,y,xisnan(&y));return;
	//if(x==INFINITY||y==INFINITY)return;
	if(xisnan(&x)||xisnan(&y)){
		last[0]=last[1]=-1;
		return;
	}
	int32_t px=xtop(x),py=ytop(y);
	if(last){
		if((last[0]==px&&last[1]==py)){
			return;
		}else {
			if(gp->connect&&
				last[0]!=-1&&
				last[1]!=-1&&
				shouldconnect(gp,last[0],last[1],px,py)
				){
				if(!graph_connect_pixel(gp,color,bold,last[0],last[1],px,py)){
					last[0]=px;
					last[1]=py;
					return;
				}
			}
			last[0]=px;
			last[1]=py;
		}

	}
	graph_setpixel_bold(gp,color,bold,px,py);
}
void graph_draw_point(struct graph *restrict gp,uint32_t color,int32_t bold,double x,double y){
	//if(x==INFINITY||y==INFINITY)return;
	if(xisnan(&x)||xisnan(&y))return;
	graph_setpixel_bold(gp,color,bold,xtop(x),ytop(y));
}
double graph_pixelstep(const struct graph *restrict gp){
	double x,y;
	x=(gp->maxx-gp->miny)/(gp->width+1);
	y=(gp->maxy-gp->miny)/(gp->height+1);
	return min(x,y);
}

#ifdef __unix__
struct mtarg {
	struct graph *restrict gp;
	uint32_t color;
	int32_t bold;
	double (*x)(double,void *);
	double (*y)(double,void *);
	void *arg;
	double from,to,step;
	volatile double *current;
};
static void *graph_drawthread(struct mtarg *mt){
	graph_draw(mt->gp,mt->color,mt->bold,mt->x,mt->y,mt->arg,mt->from,mt->to,mt->step,mt->current);
	pthread_exit(NULL);
}
#endif
void graph_draw(struct graph *restrict gp,uint32_t color,int32_t bold,double (*x)(double,void *),double (*y)(double,void *),void *arg,double from,double to,double step,volatile double *current){
	int32_t last[2]={-1,-1};
	double cur_null,toms=to-step;
	if(!current)current=&cur_null;
	for(;from<=toms;from+=step){
		graph_draw_point6(gp,color,bold,x(from,arg),y(from,arg),last);
		*current=from;
	}
	*current=to;
	graph_draw_point6(gp,color,bold,x(to,arg),y(to,arg),last);
	*current=DBL_MAX;
}
void graph_draw_mt(struct graph *restrict gp,uint32_t color,int32_t bold,double (*x)(double,void *),double (*y)(double,void *),void *arg,double from,double to,double step,volatile double *currents,int thread){
#ifdef __unix__
	pthread_t *pts;
	struct mtarg *mts;
	double last,gap,lpg;
	if(thread<2){
		graph_draw(gp,color,bold,x,y,arg,from,to,step,currents);
		return;
	}
	pts=alloca(thread*sizeof(pthread_t));
	mts=alloca(thread*sizeof(struct mtarg));
	last=from;
	gap=(to-from)/thread;
	for(int i=0;i<thread;++i){
		mts[i].gp=gp;
		mts[i].color=color;
		mts[i].bold=bold;
		mts[i].x=x;
		mts[i].y=y;
		mts[i].arg=arg;
		lpg=last+gap;
		if(lpg>to)lpg=to;
		mts[i].from=last;
		mts[i].to=lpg;
		last=lpg;
		mts[i].step=step;
		mts[i].current=currents?currents+i:NULL;
		pthread_create(pts+i,NULL,(void *(*)(void *))graph_drawthread,mts+i);
	}
	for(int i=0;i<thread;++i){
		pthread_join(pts[i],NULL);
	}
	return;
#else
	if(thread<2){
		graph_draw(gp,color,bold,x,y,arg,from,to,step,currents);
		return;
	}
	fprintf(stderr,"multi_thread is not implemented\n");
	abort();
#endif
}
static void graph_connect_pixel_x(struct graph *restrict gp,uint32_t color,int32_t bold,int32_t x1,int32_t y1,int32_t x2,int32_t y2){
	int32_t dx,dy,idx;
	idx=x2-x1;
	dy=y2-y1;
	if(dy<0){
		dy=-dy;
		for(dx=idx;dx>=0;--dx)
			graph_setpixel_bold(gp,color,bold,x1+dx,y1-muldiv_up(dx,dy,idx));
		return;
	}
	for(dx=idx;dx>=0;--dx)
		graph_setpixel_bold(gp,color,bold,x1+dx,y1+muldiv(dx,dy,idx));
	return;
}
static void graph_connect_pixel_y(struct graph *restrict gp,uint32_t color,int32_t bold,int32_t x1,int32_t y1,int32_t x2,int32_t y2){
	int32_t dx,dy,idy;
	idy=y2-y1;
	dx=x2-x1;
	if(dx<0){
		dx=-dx;
		for(dy=idy;dy>=0;--dy)
			graph_setpixel_bold(gp,color,bold,x1-muldiv_up(dy,dx,idy),y1+dy);
		return;
	}
	for(dy=idy;dy>=0;--dy)
		graph_setpixel_bold(gp,color,bold,x1+muldiv(dy,dx,idy),y1+dy);
	return;
}
static void graph_connect_pixel_45(struct graph *restrict gp,uint32_t color,int32_t bold,int32_t x1,int32_t y1,int32_t n){
	
	for(;n>=0;++x1,++y1,--n){
//		printf("draw at %d %d\n",x1,y1);
		graph_setpixel_bold(gp,color,bold,x1,y1);
	}
}
static void graph_connect_pixel_135(struct graph *restrict gp,uint32_t color,int32_t bold,int32_t x1,int32_t y1,int32_t n){
	
	for(;n>=0;++x1,--y1,--n){
		graph_setpixel_bold(gp,color,bold,x1,y1);
	}
}
int graph_raycast(const struct graph *restrict gp,int32_t *restrict x1,int32_t *restrict y1,int32_t *restrict x2,int32_t *restrict y2){
	char i1,i2;
	int32_t iv,ix1=-1,iy1=-1,ix2=-1,iy2=-1,swapbuf
		;
#define x1 (*x1)
#define y1 (*y1)
#define x2 (*x2)
#define y2 (*y2)
	/*if(y1>=max){
		y1=gp->height-1;
		x1=x2;
	}
	if(y1<=-max){
		y1=0;
		x1=x2;
	}
	if(x1>=max){
		x1=gp->width-1;
		y1=y2;
	}
	if(x1<=-max){
		x1=0;
		y1=y2;
	}
	if(y2>=max){
		y2=gp->height-1;
		x2=x1;
	}
	if(y2<=-max){
		y2=0;
		x2=x1;
	}
	if(x2>=max){
		x2=gp->width-1;
		y2=y1;
	}
	if(x2<=-max){
		x2=0;
		y2=y1;
	}*/
	i1=inrange(x1,y1);
	i2=inrange(x2,y2);
	if(i1&&i2)return 0;
	if(!i1&&!i2&&(
		((y1<0&&y2<0)||(y1>=gp->height&&y2>=gp->height)) ||
		((x1<0&&x2<0)||(x1>=gp->width&&x2>=gp->width))
		))return -1;
	if(x1==x2){
		//puts("vertical");
		if(x1<0||x1>=gp->width)return -1;

		if(y2>y1){
			if(!i1)y1=0;
			if(!i2)y2=gp->height-1;
		}else {
			if(!i1)y1=gp->height-1;
			if(!i2)y2=0;
		}
		return 0;
	}
	else if(y1==y2){
		//puts("horizonal");
		if(y1<0||y1>=gp->height)return -1;

		if(x2>x1){
			if(!i1)x1=0;
			if(!i2)x2=gp->width-1;
		}else {
			if(!i1)x1=gp->width-1;
			if(!i2)x2=0;
		}
		return 0;
	}
	iv=y1+muldiv(y2-y1,-x1,x2-x1);
	if(iv>=0&&iv<gp->height){
		//puts("west");
		ix1=0;
		iy1=iv;
	}
	iv=y1+muldiv(y2-y1,gp->width-1-x1,x2-x1);
	if(iv>=0&&iv<gp->height){
		//puts("east");
		if(ix1==-1){
			ix1=gp->width-1;
			iy1=iv;
		}else {
			ix2=gp->width-1;
			iy2=iv;
		}
	}
	iv=x1+muldiv(x2-x1,-y1,y2-y1);
	if(iv>=0&&iv<gp->width){
		//puts("south");
		if(ix1==-1){
			ix1=iv;
			iy1=0;
		}else {
			ix2=iv;
			iy2=0;
		}
	}
	iv=x1+muldiv(x2-x1,gp->height-1-y1,y2-y1);
	if(iv>=0&&iv<gp->width){
		//puts("north");
		if(ix1==-1){
			ix1=iv;
			iy1=gp->height-1;
		}else {
			ix2=iv;
			iy2=gp->height-1;
		}
	}
	if(ix1==-1)return -1;
	if(ix2==-1){
		x1=x2;
		y1=y2;
		//puts("rtangle");
		return 0;
	}
	if(!i1&&!i2){
		x1=ix1;
		y1=iy1;
		x2=ix2;
		y2=iy2;
		return 0;
	}
	if(i2){
		swapbuf=x1;x1=x2;x2=swapbuf;
		swapbuf=y1;y1=y2;y2=swapbuf;
	}
	//printf("%d,%d,%d,%d\n",ix1,iy1,ix2,iy2);
	if(x2>x1){
		if(ix2>=x1){
			x2=ix2;
			y2=iy2;
			return 0;
		}else if(ix1>=x1){
			x2=ix1;
			y2=iy1;
			return 0;
		}
	}
	if(x2<x1){
		if(ix2<=x1){
			x2=ix2;
			y2=iy2;
			return 0;
		}else if(ix1<=x1){
			x2=ix1;
			y2=iy1;
			return 0;
		}
	}
	if(y2>y1){
		if(iy2>=y1){
			x2=ix2;
			y2=iy2;
			return 0;
		}else if(iy1>=y1){
			x2=ix1;
			y2=iy1;
			return 0;
		}
	}
	if(y2<y1){
		if(iy2<=y1){
			x2=ix2;
			y2=iy2;
			return 0;
		}else if(iy1<=y1){
			x2=ix1;
			y2=iy1;
			return 0;
		}
	}

	return -1;
#undef x1
#undef y1
#undef x2
#undef y2
}
int graph_connect_pixel(struct graph *restrict gp,uint32_t color,int32_t bold,int32_t x1,int32_t y1,int32_t x2,int32_t y2){
	//color=0;bold=0;
		//printf("bef %d,%d to %d,%d\n",x1,y1,x2,y2);
	//printf("at %d,%d to %d,%d\t%d\n",x1,y1,x2,y2,abs(x1-x2)+abs(y1-y2));
	//printf("from %d,%d to %d,%d %d\n",x1,y1,x2,y2,manhattan(x1,y1,x2,y2));
	if(graph_raycast(gp,&x1,&y1,&x2,&y2)){
		return -1;
	}
	//printf("to %d,%d to %d,%d %d\n",x1,y1,x2,y2,manhattan(x1,y1,x2,y2));
	if(x1==x2&&y1==y2){
		graph_setpixel_bold(gp,color,bold,x1,y1);
		return 0;
	}
	if(x1==x2){
		graph_draw_vline(gp,color,bold,min(y1,y2),max(y1,y2),x1);
		return 0;
	}
	else if(y1==y2){
		graph_draw_hline(gp,color,bold,min(x1,x2),max(x1,x2),y1);
		return 0;
	}
	else if((x1-x2)==(y1-y2)){
		if(x1<x2)
		graph_connect_pixel_45(gp,color,bold,x1,y1,x2-x1);
		else
		graph_connect_pixel_45(gp,color,bold,x2,y2,x1-x2);
		return 0;
	}
	else if((x1-x2)==(y2-y1)){
		if(x1<x2)
		graph_connect_pixel_135(gp,color,bold,x1,y1,x2-x1);
		else
		graph_connect_pixel_135(gp,color,bold,x2,y2,x1-x2);
		return 0;
	}
	else if(abs(x1-x2)<abs(y1-y2)){
		if(y1>y2)
		graph_connect_pixel_y(gp,color,bold,x2,y2,x1,y1);
		else
		graph_connect_pixel_y(gp,color,bold,x1,y1,x2,y2);
		return 0;
	}
	else {
		if(x1>x2)
		graph_connect_pixel_x(gp,color,bold,x2,y2,x1,y1);
		else
		graph_connect_pixel_x(gp,color,bold,x1,y1,x2,y2);
	}
	return 0;
}
int graph_connect(struct graph *restrict gp,uint32_t color,int32_t bold,double x1,double y1,double x2,double y2){
	//printf("at %lf,%lf %lf,%lf\n",x1,y1,x2,y2);
	//printf("at %d,%d,%d,%d\n",xtop(x1),ytop(y1),xtop(x2),ytop(y2));
	if(xisnan(&x1)||xisnan(&y1)||xisnan(&x2)||xisnan(&y2))return -1;
	return graph_connect_pixel(gp,color,bold,xtop(x1),ytop(y1),xtop(x2),ytop(y2));
}
#define DRAWXVAL if(gp->draw_value){\
	snprintf(vb,16,"%.5lg",v);\
	if(!strchr(vb,'e')&&!(p=strstr(vb,".00"))){\
		*p=0;\
	}\
	ih=h=gapline_len*3/2;\
	while((len=graph_textlen(gp,vb,1,h))>gapxi&&h>=4)\
		h=muldiv(h,gapxi,len);\
	--h;\
	/*graph_draw_text_pixel(gp,color,0,vb,1,h,px+bold+4,ay-gapline_len*5/2+(ih-h));*/\
}
#define DRAWYVAL if(gp->draw_value){\
	snprintf(vb,16,"%.5lg",v);\
	if(!strchr(vb,'e')&&!(p=strstr(vb,".00"))){\
		*p=0;\
	}\
	/*graph_draw_text_pixel(gp,color,0,vb,1,gapline_len*3/2,ax+bold+4,py);*/\
}
void graph_draw_axis(struct graph *restrict gp,uint32_t color,int32_t bold,double gapx,double gapy,uint32_t gapline_len){
	int32_t px,py,ax=xtop(0),ay=ytop(0);
	double v;
	int32_t gapxi=xtop(gapx),len,h/*,ih*/;
	char vb[32];
	char *p;
	graph_draw_hline(gp,color,bold,0,gp->width,ay);
	graph_draw_vline(gp,color,bold,0,gp->height,ax);
	if(gapx>DBL_EPSILON){
	for(v=gapx;;v+=gapx){
		px=xtop(v);
		if(px+bold>=gp->width)
			break;
		graph_draw_vline(gp,color,bold,ay-gapline_len,ay+gapline_len,px);
		DRAWXVAL
	}
	for(v=-gapx;;v-=gapx){
		px=xtop(v);
		if(px-bold<0)
			break;
		graph_draw_vline(gp,color,bold,ay-gapline_len,ay+gapline_len,px);
		DRAWXVAL
	}
	}
	if(gapy>DBL_EPSILON){
	for(v=gapy;;v+=gapy){
		py=ytop(v);
		if(py+bold>=gp->height)
			break;
		graph_draw_hline(gp,color,bold,ax-gapline_len,ax+gapline_len,py);
		DRAWYVAL
	}
	for(v=-gapy;;v-=gapy){
		py=ytop(v);
		if(py-bold<0)break;
		graph_draw_hline(gp,color,bold,ax-gapline_len,ax+gapline_len,py);
		DRAWYVAL

	}
	}
	if(gp->arrow){
	for(py=gp->height-1,px=ax;px-ax<=gapline_len;++px,--py)
		graph_setpixel_bold(gp,color,bold,px,py);
	for(py=gp->height-1,px=ax;ax-px<=gapline_len;--px,--py)
		graph_setpixel_bold(gp,color,bold,px,py);
	for(px=gp->width-1,py=ay;py-ay<=gapline_len;--px,++py)
		graph_setpixel_bold(gp,color,bold,px,py);
	for(px=gp->width-1,py=ay;ay-py<=gapline_len;--px,--py)
		graph_setpixel_bold(gp,color,bold,px,py);
	}
}
void graph_draw_grid(struct graph *restrict gp,uint32_t color,int32_t bold,double gapx,double gapy){
	int32_t px,py,ax=xtop(0),ay=ytop(0);
	double v;
	graph_draw_hline(gp,color,bold,0,gp->width,ay);
	graph_draw_vline(gp,color,bold,0,gp->height,ax);
	if(gapx>DBL_EPSILON){
		for(v=gapx;;v+=gapx){
			px=xtop(v);
			if(px+bold>=gp->width)break;
			graph_draw_vline(gp,color,bold,0,gp->height-1,px);
		}
		for(v=-gapx;;v-=gapx){
			px=xtop(v);
			if(px-bold<0)break;
			graph_draw_vline(gp,color,bold,0,gp->height-1,px);
		}
	}
	if(gapy>DBL_EPSILON){
		for(v=gapy;;v+=gapy){
			py=ytop(v);
			if(py+bold>=gp->height)break;
			graph_draw_hline(gp,color,bold,0,gp->width-1,py);
		}
		for(v=-gapy;;v-=gapy){
			py=ytop(v);
			if(py-bold<0)break;
			graph_draw_hline(gp,color,bold,0,gp->width-1,py);
		}
	}
}
