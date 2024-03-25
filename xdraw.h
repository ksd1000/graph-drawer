/*******************************************************************************
 *License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>*
 *This is free software: you are free to change and redistribute it.           *
 *******************************************************************************/
#ifndef _XDRAW_H_
#define _XDRAW_H_
#include <stdint.h>
#include "expr.h"
struct graph {
	char *buf,*textbuf;
	double minx,maxx,miny,maxy;
	int32_t width,height,lastx,lasty;
	uint32_t byte_width,hsize;
	uint16_t bpp;
	char connect:1;
	char drawing:1;
	char arrow:1;
	char draw_value:1;
	//connect decides if the continuous points will be connected
};

void graph_setpixel_bold(struct graph *restrict gp,uint32_t color,int32_t bold,int32_t x,int32_t y);
int32_t graph_draw_text_pixel(struct graph *restrict gp,uint32_t color,int32_t bold,const char *s,int32_t gap,int32_t height,int32_t x,int32_t y);
int32_t graph_textlen(struct graph *restrict gp,const char *s,int32_t gap,int32_t height);
int32_t graph_text_height(void);
int32_t graph_draw_text(struct graph *restrict gp,uint32_t color,int32_t bold,const char *s,int32_t gap,int32_t height,double x,double y);
int32_t graph_xtop(const struct graph *restrict gp,double x);
int32_t graph_ytop(const struct graph *restrict gp,double y);
void graph_fill(struct graph *restrict gp,uint32_t color);
int init_graph_frombmp(struct graph *restrict gp,void *bmp,size_t size,double minx,double maxx,double miny,double maxy);
int init_graph(struct graph *restrict gp,int32_t width,int32_t height,uint16_t bpp,double minx,double maxx,double miny,double maxy);
void graph_free(struct graph *restrict gp);
void graph_draw_point6(struct graph *restrict gp,uint32_t color,int32_t bold,double x,double y,int32_t last[2]);
void graph_draw_point(struct graph *restrict gp,uint32_t color,int32_t bold,double x,double y);
double graph_pixelstep(const struct graph *restrict gp);
void graph_draw(struct graph *restrict gp,uint32_t color,int32_t bold,double (*x)(double),double (*y)(double),double from,double to,double step,volatile double *current);
void graph_draw_mt(struct graph *restrict gp,uint32_t color,int32_t bold,double (*x)(double),double (*y)(double),double from,double to,double step,volatile double *currents,int thread);
void graph_drawep(struct graph *restrict gp,uint32_t color,int32_t bold,const struct expr *restrict xep,const struct expr *restrict yep,double from,double to,double step,volatile double *current);
void graph_drawep_mt(struct graph *restrict gp,uint32_t color,int32_t bold,const struct expr *restrict xeps,const struct expr *restrict yeps,double from,double to,double step,volatile double *currents,int thread);
int graph_raycast(const struct graph *restrict gp,int32_t *restrict x1,int32_t *restrict y1,int32_t *restrict x2,int32_t *restrict y2);
int graph_connect_pixel(struct graph *restrict gp,uint32_t color,int32_t bold,int32_t x1,int32_t y1,int32_t x2,int32_t y2);
int graph_connect(struct graph *restrict gp,uint32_t color,int32_t bold,double x1,double y1,double x2,double y2);
void graph_draw_axis(struct graph *restrict gp,uint32_t color,int32_t bold,double gapx,double gapy,uint32_t gapline_len);
void graph_draw_grid(struct graph *restrict gp,uint32_t color,int32_t bold,double gapx,double gapy);

#define graph_bmpsize(gp) (*(uint32_t *)((gp)->buf-54+2))
#define graph_getbmp(gp) ((gp)->buf-54)
#define graph_colorat(gp,x,y) ((gp)->buf[(y)*(gp)->byte_width+(x)*(gp)->bpp])
#endif
