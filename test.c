#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include "xdraw.h"
double pow1(double x){
	return x;
}
double rsin(double x){
	return x*sin(x);
}
double rcos(double x){
	return x*cos(x);
}
int main(){
	struct graph g;
	int fd[3];
	pid_t pid;
	char pbuf[16];
	init_graph(&g,1024,1024,24,-4,4,-4,4);
	graph_fill(&g,0xffffff);
	graph_draw_axis(&g,0,1,M_PI,1,16);
	graph_draw(&g,0x00ff00,3,pow1,sin,-7,7,0.0001);
	graph_draw(&g,0xff0000,2,rcos,rsin,0,32,0.0001);
	FILE *fout = fopen("test.bmp", "wb");
	fwrite(graph_tobmp(&g),1,graph_size(&g),fout);
	fclose(fout);
	graph_free(&g);
}
