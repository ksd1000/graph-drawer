/*******************************************************************************
 *License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>*
 *This is free software: you are free to change and redistribute it.           *
 *******************************************************************************/
#ifndef _XDRAW_H_
#define _XDRAW_H_
#include <stdint.h>
#include "expr.h"
struct graph {
	char *buf;
	double minx,maxx,miny,maxy;
	volatile double current;
	int32_t width,height,lastx,lasty;
	uint32_t byte_width;
	uint16_t bpp;
	char connect:1;
	char drawing:1;
	char arrow:1;
	char draw_value:1;
	//connect decides if the continuous points will be connected
};

void graph_setpixel_bold(struct graph *restrict gp,uint32_t color,int32_t bold,int32_t x,int32_t y);
int32_t graph_draw_text_pixel(struct graph *restrict gp,uint32_t color,int32_t bold,const char *s,int32_t gap,int32_t height,int32_t x,int32_t y);
int32_t graph_draw_text(struct graph *restrict gp,uint32_t color,int32_t bold,const char *s,int32_t gap,int32_t height,double x,double y);
int32_t graph_xtop(struct graph *restrict gp,double x);
int32_t graph_ytop(struct graph *restrict gp,double y);
void graph_fill(struct graph *restrict gp,uint32_t color);
int init_graph(struct graph *restrict gp,int32_t width,int32_t height,uint16_t bpp,double minx,double maxx,double miny,double maxy);
void graph_free(struct graph *restrict gp);
void graph_draw_point(struct graph *restrict gp,uint32_t color,int32_t bold,double x,double y);
void graph_draw(struct graph *restrict gp,uint32_t color,int32_t bold,double (*x)(double),double (*y)(double),double from,double to,double step);
void graph_drawep(struct graph *restrict gp,uint32_t color,int32_t bold,struct expr *restrict xep,struct expr *restrict yep,double from,double to,double step);
int graph_raycast(struct graph *restrict gp,int32_t *restrict x1,int32_t *restrict y1,int32_t *restrict x2,int32_t *restrict y2);
int graph_connect_pixel(struct graph *restrict gp,uint32_t color,int32_t bold,int32_t x1,int32_t y1,int32_t x2,int32_t y2);
int graph_connect(struct graph *restrict gp,uint32_t color,int32_t bold,double x1,double y1,double x2,double y2);
void graph_draw_axis(struct graph *restrict gp,uint32_t color,int32_t bold,double gapx,double gapy,uint32_t gapline_len);
void graph_draw_grid(struct graph *restrict gp,uint32_t color,int32_t bold,double gapx,double gapy);

#define graph_size(gp) (*(uint32_t *)((gp)->buf-54+2))
#define graph_tobmp(gp) ((gp)->buf-54)
#define graph_colorat(gp,x,y) ((gp)->buf[(y)*(gp)->byte_width+(x)*(gp)->bpp])
#endif
