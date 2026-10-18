
/*******************************************************************************
 *License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>*
 *This is free software: you are free to change and redistribute it.           *
 *******************************************************************************/
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include "graph.h"
#ifdef __unix__
#define REAL_UNIX
#endif
#include "fake_unix.h"
#pragma GCC diagnostic ignored "-Wunused-result"
double evalx(double input,void *arg){
	(void)arg;
	return input;
}
double evalsin(double input,void *arg){
	(void)arg;
	return sin(input);
}
double evalexp(double input,void *arg){
	(void)arg;
	return exp(input);
}
double evallog(double input,void *arg){
	(void)arg;
	return log(input);
}
double evalsqrt(double input,void *arg){
	(void)arg;
	return sqrt(input);
}
double evalcosh(double input,void *arg){
	(void)arg;
	return cosh(input);
}
#define FBOLD 0
#define RATIO 4.0
int main(void){
	struct graph gp[1];
	intptr_t tofd;
	volatile double current;
	assert(!init_graph(gp,1280,720,24,-RATIO,RATIO,-(RATIO*720/1080),(RATIO*720/1080)));
	graph_fill(gp,0xffffff);
	gp->draw_value=0;
	graph_draw_grid(gp,0xbfbfbf,0,1.0,1.0);
	graph_draw_axis(gp,0x000000,0,1.0,1.0,8);
	graph_draw(gp,0x00ffff,FBOLD,evalx,evalsin,NULL,NULL,-4,4.25,8.0/1280.0,&current);
	graph_draw(gp,0x00ff00,FBOLD,evalx,evalexp,NULL,NULL,-4,4.25,8.0/1280.0,&current);
	graph_draw(gp,0xff7f00,FBOLD,evalx,evallog,NULL,NULL,-4,4.25,8.0/1280.0,&current);
	graph_draw(gp,0x7fff00,FBOLD,evalx,evalsqrt,NULL,NULL,-4,4.25,8.0/1280.0,&current);
	graph_draw(gp,0x00ff7f,FBOLD,evalx,evalcosh,NULL,NULL,-4,4.25,8.0/1280.0,&current);
	//graph_drawchar(gp,0xff0000,0,'g',32,0,128,0);
	graph_draw_text_pixel(gp,0xff0000,0,"Graph",4,320,96,392);
	graph_draw_text_pixel(gp,0x0000ff,0,"Drawer",4,320,288,64);
	graph_draw_text_pixel(gp,0x006400,0,"Expression-to-image-and-audio converter",4,72,12,16);
	tofd=open("logo.bmp",O_WRONLY_CREAT);
	assert(!fderr(tofd));
	assert(write(tofd,gp->bm,gp->bm->size)==(ssize_t)gp->bm->size);
	close(tofd);
	graph_free(gp);
	return EXIT_SUCCESS;
}
