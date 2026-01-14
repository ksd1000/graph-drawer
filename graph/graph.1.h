/*******************************************************************************
 *License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>*
 *This is free software: you are free to change and redistribute it.           *
 *******************************************************************************/
#ifndef _GRAPH_H_
#define _GRAPH_H_
#include <stdint.h>
#include "bitmap.h"
struct graph {
	char *buf;
	struct bitmap *bm;
	char *textbuf;
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

