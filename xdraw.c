/*******************************************************************************
 *License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>*
 *This is free software: you are free to change and redistribute it.           *
 *******************************************************************************/
#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "xdraw.h"

static void setpixel(struct graph *restrict gp,uint32_t color,int32_t x,int32_t y){
	if(x<gp->width&&x>=0&&y<gp->height&&y>=0)
		memcpy(gp->buf+y*gp->byte_width+x*gp->bpp,&color,gp->bpp);
}

static void setpixel_bold(struct graph *restrict gp,uint32_t color,int32_t x,int32_t y,int32_t bold){
	if(gp->drawing){
		if(gp->lastx==x&&gp->lasty==y)
			return;
		else {
			gp->lastx=x;
			gp->lasty=y;
		}
	}
	if(bold>0){
		for(int32_t px=x-bold;px<=x+bold;++px){
			if(px>=gp->width||px<0)continue;
			for(int32_t py=y-bold;py<=y+bold;++py){
				if(py>=gp->height||py<0)continue;
				if((int32_t)hypot(fabs((double)(px-x)),fabs((double)(py-y)))<=bold)
					setpixel(gp,color,px,py);
			}
		}
	}else
	setpixel(gp,color,x,y);

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
static void graph_draw_vline(struct graph *restrict gp,uint32_t color,int32_t bold,int32_t start,int32_t end,int32_t x){
	for(;start<=end;++start)
		setpixel_bold(gp,color,x,start,bold);
}
static void graph_draw_hline(struct graph *restrict gp,uint32_t color,int32_t bold,int32_t start,int32_t end,int32_t y){
	for(;start<=end;++start)
		setpixel_bold(gp,color,start,y,bold);
}
void graph_draw_point(struct graph *restrict gp,uint32_t color,int32_t bold,double x,double y){
	setpixel_bold(gp,color,xtop(x),ytop(y),bold);
}
void graph_draw_axis(struct graph *restrict gp,uint32_t color,int32_t bold,double gapx,double gapy,uint32_t gapline_len){
	int32_t px,py,ax=xtop(0),ay=ytop(0);
	double v;
	graph_draw_hline(gp,color,bold,0,gp->width,ax);
	graph_draw_vline(gp,color,bold,0,gp->height,ay);
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
		graph_draw_hline(gp,color,bold,ay-gapline_len,ay+gapline_len,py);

	}
	for(px=gp->width-1,py=ay;py-ay<=gapline_len;--px,++py)
		setpixel_bold(gp,color,px,py,bold);
	for(px=gp->width-1,py=ay;ay-py<=gapline_len;--px,--py)
		setpixel_bold(gp,color,px,py,bold);
	for(py=gp->height-1,px=ax;px-ax<=gapline_len;++px,--py)
		setpixel_bold(gp,color,px,py,bold);
	for(py=gp->height-1,px=ax;ax-px<=gapline_len;--px,--py)
		setpixel_bold(gp,color,px,py,bold);
}
void graph_draw_grid(struct graph *restrict gp,uint32_t color,int32_t bold,double gapx,double gapy){
	int32_t px,py,ax=xtop(0),ay=ytop(0);
	double v;
	graph_draw_hline(gp,color,bold,0,gp->width,ax);
	graph_draw_vline(gp,color,bold,0,gp->height,ay);
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
