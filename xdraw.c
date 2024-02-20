/*******************************************************************************
 *License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>*
 *This is free software: you are free to change and redistribute it.           *
 *******************************************************************************/
#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "xdraw.h"
//void graph_setpixel(struct graph *restrict gp,uint32_t color,int32_t x,int32_t y){
//}
#define min(a,b) (a<b?a:b)
#define max(a,b) (a>b?a:b)
#define inrange(x,y) ((x)>=0&&(x)<gp->width&&(y)>=0&&(y)<gp->height)
#define setpixel(px,py)\
	memcpy(gp->buf+py*gp->byte_width+px*gp->bpp,&color,gp->bpp)
void graph_setpixel_bold(struct graph *restrict gp,uint32_t color,int32_t bold,int32_t x,int32_t y){
	int32_t px,endx,py,endy;
	px=x-bold;
//	if(px>=gp->width)return;
	py=y-bold;
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
			for(;py<=endy;++py){
				if((int32_t)hypot(fabs((double)(px-x)),fabs((double)(py-y)))<=bold){
					if(inrange(px,py))setpixel(px,py);
					//graph_setpixel(gp,color,px,py);
				}
			}
		}
	}else if(inrange(x,y))
	setpixel(x,y);
	//graph_setpixel(gp,color,x,y);
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
int init_graph(struct graph *restrict gp,int32_t width,int32_t height,uint16_t bpp,double minx,double maxx,double miny,double maxy){
	gp->byte_width=((width*bpp+31)>>5)<<2;
	if((gp->buf=malloc(54+gp->byte_width*height))==NULL)return -1;
	gp->width=width;
	gp->height=height;
	gp->minx=minx;
	gp->maxx=maxx;
	gp->miny=miny;
	gp->maxy=maxy;
	gp->drawing=0;
	gp->connect=1;
	//gp->lastx=gp->lasty=-1;
	gp->bpp=bpp/8;
	memset(gp->buf,0,54);
	memcpy(gp->buf,"BM",2);
	*(uint32_t *)(gp->buf+2)=54+gp->byte_width*height;
	*(uint32_t *)(gp->buf+10)=54;
	*(uint32_t *)(gp->buf+14)=40;
	*(uint32_t *)(gp->buf+18)=width;
	*(uint32_t *)(gp->buf+22)=height;
	*(uint16_t *)(gp->buf+26)=1;
	*(uint16_t *)(gp->buf+28)=bpp;
	*(uint32_t *)(gp->buf+34)=gp->byte_width*height;
	gp->buf+=54;
	return 0;
}
void graph_free(struct graph *restrict gp){
	free(gp->buf-54);
}
#define xtop(x) (((x)-gp->minx)/(gp->maxx-gp->minx)*gp->width)
#define ytop(y) (((y)-gp->miny)/(gp->maxy-gp->miny)*gp->height)
#define START_DRAWING gp->drawing=1;\
	gp->lastx=gp->lasty=-1;
#define END_DRAWING gp->drawing=0;
static void graph_draw_vline(struct graph *restrict gp,uint32_t color,int32_t bold,int32_t start,int32_t end,int32_t x){
	
	//puts(__func__);
	for(;start<=end;++start)
		graph_setpixel_bold(gp,color,bold,x,start);
}
static void graph_draw_hline(struct graph *restrict gp,uint32_t color,int32_t bold,int32_t start,int32_t end,int32_t y){
	
	//puts(__func__);
	for(;start<=end;++start)
		graph_setpixel_bold(gp,color,bold,start,y);
}
void graph_draw_point(struct graph *restrict gp,uint32_t color,int32_t bold,double x,double y){
	int32_t px=xtop(x),py=ytop(y);
	if(gp->drawing){
		if(gp->lastx==px&&gp->lasty==py){
			return;
		}else {
			if(gp->lastx!=-1&&gp->lasty!=-1&&gp->connect){
				if(!graph_connect_pixel(gp,color,bold,gp->lastx,gp->lasty,px,py)){
					gp->lastx=px;
					gp->lasty=py;
					return;
				}
			}
			gp->lastx=px;
			gp->lasty=py;
		}

	}
	graph_setpixel_bold(gp,color,bold,px,py);
}
void graph_draw(struct graph *restrict gp,uint32_t color,int32_t bold,double (*x)(double),double (*y)(double),double from,double to,double step){
	START_DRAWING
	for(;from<=to;from+=step){
		graph_draw_point(gp,color,bold,x(from),y(from));
	}
	END_DRAWING
}
void graph_drawe(struct graph *restrict gp,uint32_t color,int32_t bold,struct expr *restrict xep,struct expr *restrict yep,double from,double to,double step){
	START_DRAWING
	for(;from<=to;from+=step){
		graph_draw_point(gp,color,bold,expr_compute(xep,from),expr_compute(yep,from));
	}
	END_DRAWING
}
static int32_t muldiv(int32_t m1,int32_t m2,int32_t f){
	return m1/f*m2+m1%f*m2/f;
}
static int32_t muldiv_up(int32_t m1,int32_t m2,int32_t f){
	int32_t mod=m1%f;
	return m1/f*m2+mod*m2/f+!!mod;
}
static void graph_connect_pixel_x(struct graph *restrict gp,uint32_t color,int32_t bold,int32_t x1,int32_t y1,int32_t x2,int32_t y2){
	int32_t dx,dy,idx;
	//puts(__func__);
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
	//puts(__func__);
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
	
	//puts(__func__);
	for(;n>=0;++x1,++y1,--n){
//		printf("draw at %d %d\n",x1,y1);
		graph_setpixel_bold(gp,color,bold,x1,y1);
	}
}
static void graph_connect_pixel_135(struct graph *restrict gp,uint32_t color,int32_t bold,int32_t x1,int32_t y1,int32_t n){
	
	//puts(__func__);
	for(;n>=0;++x1,--y1,--n){
		graph_setpixel_bold(gp,color,bold,x1,y1);
	}
}

int graph_connect_pixel(struct graph *restrict gp,uint32_t color,int32_t bold,int32_t x1,int32_t y1,int32_t x2,int32_t y2){
	//color=0;bold=0;
	if(!inrange(x1,y1)||!inrange(x2,y2))return -1;
	if(x1==x2&&y1==y2){
		graph_setpixel_bold(gp,color,bold,x1,y1);
		return 0;
	}
	else if(x1==x2){
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
void graph_connect(struct graph *restrict gp,uint32_t color,int32_t bold,double x1,double y1,double x2,double y2){
//	printf("%lf,%lf %lf,%lf\n",x1,y1,x2,y2);
	graph_connect_pixel(gp,color,bold,xtop(x1),ytop(y1),xtop(x2),ytop(y2));
}
void graph_draw_axis(struct graph *restrict gp,uint32_t color,int32_t bold,double gapx,double gapy,uint32_t gapline_len){
	int32_t px,py,ax=xtop(0),ay=ytop(0);
	double v;
	graph_draw_hline(gp,color,bold,0,gp->width,ay);
	graph_draw_vline(gp,color,bold,0,gp->height,ax);
	for(v=gapx;;v+=gapx){
		px=xtop(v);
		if(px+bold>=gp->width)break;
		graph_draw_vline(gp,color,bold,ay-gapline_len,ay+gapline_len,px);

	}
	for(v=-gapx;;v-=gapx){
		px=xtop(v);
		if(px-bold<0)break;
		graph_draw_vline(gp,color,bold,ay-gapline_len,ay+gapline_len,px);

	}
	for(v=gapy;;v+=gapy){
		py=ytop(v);
		if(py+bold>=gp->height)break;
		graph_draw_hline(gp,color,bold,ax-gapline_len,ax+gapline_len,py);

	}
	for(v=-gapy;;v-=gapy){
		py=ytop(v);
		if(py-bold<0)break;
		graph_draw_hline(gp,color,bold,ax-gapline_len,ax+gapline_len,py);

	}
	for(px=gp->width-1,py=ay;py-ay<=gapline_len;--px,++py)
		graph_setpixel_bold(gp,color,bold,px,py);
	for(px=gp->width-1,py=ay;ay-py<=gapline_len;--px,--py)
		graph_setpixel_bold(gp,color,bold,px,py);
	for(py=gp->height-1,px=ax;px-ax<=gapline_len;++px,--py)
		graph_setpixel_bold(gp,color,bold,px,py);
	for(py=gp->height-1,px=ax;ax-px<=gapline_len;--px,--py)
		graph_setpixel_bold(gp,color,bold,px,py);
}
void graph_draw_grid(struct graph *restrict gp,uint32_t color,int32_t bold,double gapx,double gapy){
	int32_t px,py,ax=xtop(0),ay=ytop(0);
	double v;
	graph_draw_hline(gp,color,bold,0,gp->width,ay);
	graph_draw_vline(gp,color,bold,0,gp->height,ax);
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
