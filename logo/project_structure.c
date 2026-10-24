
/*******************************************************************************
 *License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>*
 *This is free software: you are free to change and redistribute it.           *
 *******************************************************************************/
// compile command: gcc -I ../graph-drawer/graph ../graph-drawer/logo.c graph/libgraph.a -lm -o logo -Wall -Wextra
#define _GNU_SOURCE
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include "graph.h"
#ifdef __unix__
#define REAL_UNIX
#endif
#include "fake_unix.h"
#pragma GCC diagnostic ignored "-Wunused-result"
#define FBOLD 0
#define RATIO 1.0
#define RESIZE /2
void graph_draw_boxedtext(struct graph *restrict gp,uint32_t color,int32_t bold,const char *text,int32_t x,int32_t y){
	int32_t l;
	l=graph_draw_text_pixel(gp,color,bold,text,4 RESIZE,80 RESIZE,x+60 RESIZE,y+12 RESIZE);
	graph_draw_box_pixel(gp,color,0,x,y,x+l+100 RESIZE,y+96 RESIZE);
}
int main(void){
	struct graph gp[1];
	intptr_t tofd;
	assert(!init_graph(gp,1280 RESIZE,720 RESIZE,32,-RATIO,RATIO,-(RATIO*720/1080),(RATIO*720/1080)));
	graph_fill(gp,0xefffffff);
	graph_draw_box_pixel(gp,0xff00ff00,0,0,0,gp->width-1,gp->height-1);
	//graph_draw_box_pixel(gp,0xff00ff00,0,20 RESIZE,540 RESIZE,660 RESIZE,680 RESIZE);
	graph_draw_boxedtext(gp,0xff00cf00,0,"Graph Drawer",70 RESIZE,560 RESIZE);
	graph_draw_boxedtext(gp,0xff00cf00,0,"Expression evaluator lib",180 RESIZE,400 RESIZE);
	graph_draw_boxedtext(gp,0xff00cf00,0,"Graph generator lib",180 RESIZE,250 RESIZE);
	graph_draw_boxedtext(gp,0xff00cf00,0,"Frontend program",180 RESIZE,100 RESIZE);
	graph_connect_pixel(gp,0xff00cf00,0,120 RESIZE,560 RESIZE,120 RESIZE,140 RESIZE);
	graph_connect_pixel(gp,0xff00cf00,0,180 RESIZE,440 RESIZE,120 RESIZE,440 RESIZE);
	graph_connect_pixel(gp,0xff00cf00,0,180 RESIZE,290 RESIZE,120 RESIZE,290 RESIZE);
	graph_connect_pixel(gp,0xff00cf00,0,180 RESIZE,140 RESIZE,120 RESIZE,140 RESIZE);
	graph_draw_text_pixel(gp,0xffff00ff,0,"Structure of the Graph Drawer project",4 RESIZE,72 RESIZE,12 RESIZE,16 RESIZE);
	tofd=open("project_structure.bmp",O_WRONLY_CREAT);
	// the project_structure.png is converted by imagemagick.
	assert(!fderr(tofd));
	assert(write(tofd,gp->bm,gp->bm->size)==(ssize_t)gp->bm->size);
	close(tofd);
	graph_free(gp);
	return EXIT_SUCCESS;
}
