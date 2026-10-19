
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
double evalfish(double input,void *arg){
	double y=*(double *)arg;
	if(input<1.0){
		return y+input;
	}else if(input<2.0){
		return y+1.0-(input-1.0)*2;
	}else if(input<3.0){
		return y-1.0+(input-2.0);
	}else if(input<4.0){
		return y+0.125*sin((input-3.0)*2.0*M_PI);
	}else if(input<5.0){
		return y+0.5*sin(((input-4.0)*0.25-0.125)*2.0*M_PI);
	}else if(input<6.0){
		return y+0.5*(input-5.0);
	}else if(input<7.0){
		return y+0.5-(input-6.0);
	}else if(input<8.0){
		return y-0.5+0.5*(input-7.0);
	}else if(input<9.0){
		return y-0.625+1.25*(input-8.0);
	}else if(input<10.0){
		return y-0.875+1.75*(input-9.0);
	}
	return NAN;
}
double evalfishx(double input,void *arg){
	double x=*(double *)arg;
	if(input<1.0){
		return x+input;
	}else if(input<2.0){
		return x+1.0;
	}else if(input<3.0){
		return x+1.0-(input-2.0);
	}else if(input<4.0){
		return x+0.25+0.125*cos((input-3.0)*2.0*M_PI);
	}else if(input<5.0){
		return x+0.5*cos(((input-4.0)*0.25-0.125)*2.0*M_PI);
	}else if(input<6.0){
		return x+1.0+0.5*(input-5.0);
	}else if(input<7.0){
		return x+1.5;
	}else if(input<8.0){
		return x+1.5-0.5*(input-7.0);
	}else if(input<9.0){
		return x+0.625+0.0625*sin(4.0*M_PI*(input-8.0));
	}else if(input<10.0){
		return x+0.875+0.0625*sin(4.0*M_PI*(1.45*(input-9.0)-0.25));
	}
	return NAN;
}
double evalfishx_rev(double input,void *arg){
	double x=*(double *)arg,z=0.0;
	return x+1.5-evalfishx(input,&z);
}
#define FBOLD 0
#define RATIO 3.875
#define RESIZE /2
void graph_draw_fish(struct graph *restrict gp,uint32_t color,int32_t bold,double x,double y,int reverse){
	volatile double current;
	if(reverse){
		graph_draw(gp,color,bold,evalfishx_rev,evalfish,&x,&y,0,10.0,RATIO/640.0,&current);
		graph_draw_point(gp,color,bold+5,x+1.25,y-0.04);
	}else {
		graph_draw(gp,color,bold,evalfishx,evalfish,&x,&y,0,10.0,RATIO/640.0,&current);
		graph_draw_point(gp,color,bold+5,x+0.25,y-0.04);
	}
}
int main(void){
	struct graph gp[1];
	intptr_t tofd;
	volatile double current;
	assert(!init_graph(gp,1280 RESIZE,720 RESIZE,32,-RATIO,RATIO,-(RATIO*720/1080),(RATIO*720/1080)));
	graph_fill(gp,0x7fffffff);
	gp->draw_value=0;
	graph_draw_grid(gp,0xffbfbfbf,0,1.0,1.0);
	graph_draw_axis(gp,0xff000000,0,1.0,1.0,8);
	graph_draw(gp,0xff00ffff,FBOLD,evalx,evalsin,NULL,NULL,-RATIO,RATIO+0.0625,RATIO/640.0,&current);
	graph_draw(gp,0xff00ff00,FBOLD,evalx,evalexp,NULL,NULL,-RATIO,RATIO+0.0625,RATIO/640.0,&current);
	graph_draw(gp,0xffff7f00,FBOLD,evalx,evallog,NULL,NULL,-RATIO,RATIO+0.0625,RATIO/640.0,&current);
	graph_draw(gp,0xff7fff00,FBOLD,evalx,evalsqrt,NULL,NULL,-RATIO,RATIO+0.0625,RATIO/640.0,&current);
	graph_draw(gp,0xff00ff7f,FBOLD,evalx,evalcosh,NULL,NULL,-RATIO,RATIO+0.0625,RATIO/640.0,&current);
	gp->connect=0;
	graph_draw_fish(gp,0xff87cefa,FBOLD+1,2.0,1.5,0);
	graph_draw_fish(gp,0xff9400d3,FBOLD+1,-3.75,-1.0,1);
	graph_draw_text_pixel(gp,0xffff0000,0,"Graph",4 RESIZE,320 RESIZE,96 RESIZE,392 RESIZE);
	graph_draw_text_pixel(gp,0xff2020ff,0,"Drawer",4 RESIZE,320 RESIZE,288 RESIZE,64 RESIZE);
	graph_draw_text_pixel(gp,0xff007f00,0,"Expression-to-image-and-audio converter",4 RESIZE,72 RESIZE,12 RESIZE,16 RESIZE);
	graph_draw_box_pixel(gp,0xffffffff,0,0,0,gp->width-1,gp->height-1);
	tofd=open("logo.bmp",O_WRONLY_CREAT);
	// the logo.png is converted by imagemagick.
	assert(!fderr(tofd));
	assert(write(tofd,gp->bm,gp->bm->size)==(ssize_t)gp->bm->size);
	close(tofd);
	graph_free(gp);
	return EXIT_SUCCESS;
}
