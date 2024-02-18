#ifndef _XDRAW_H_
#define _XDRAW_H_
#include <stdint.h>
#include "expr.h"
struct graph {
	char *buf;
	double minx,maxx,miny,maxy;
	int32_t width,height;
	uint32_t byte_width;
	uint16_t bpp;

};
#define graph_size(gp) (*(uint32_t *)((gp)->buf-54+2))
#define graph_tobmp(gp) ((gp)->buf-54)
void graph_fill(struct graph *restrict gp,uint32_t color);
int init_graph(struct graph *restrict gp,int32_t width,int32_t height,uint16_t bpp,double minx,double maxx,double miny,double maxy);
void graph_free(struct graph *restrict gp);
void graph_draw(struct graph *restrict gp,uint32_t color,int32_t bold,double (*x)(double),double (*y)(double),double from,double to,double step);
void graph_drawe(struct graph *restrict gp,uint32_t color,int32_t bold,struct expr *restrict xep,struct expr *restrict yep,double from,double to,double step);
void graph_draw_axis(struct graph *restrict gp,uint32_t color,int32_t bold,double gapx,double gapy,uint32_t gapline_len);
void graph_draw_point(struct graph *restrict gp,uint32_t color,int32_t bold,double x,double y);
void graph_draw_grid(struct graph *restrict gp,uint32_t color,int32_t bold,double gapx,double gapy);
#endif
