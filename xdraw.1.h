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
	int32_t width,height,lastx,lasty;
	uint32_t byte_width;
	uint16_t bpp;
	char drawing,connect;
};

