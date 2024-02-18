#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include "xdraw.h"
#define GAP (M_PI/2)
#define SIZE 16
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
	int e;
	struct expr_symset *esp=new_expr_symset();
	expr_symset_add(esp,"a",rsin);
	expr_symset_add(esp,"b",rcos);
	struct expr *ep=new_expr("a(t)","t",esp,&e),*xep=new_expr("b(t)","t",esp,NULL);
	if(!ep){
		printf("expr error:%s\n",expr_error(e));
		return -1;
	}
	init_graph(&g,1024,1024,24,-SIZE,SIZE,-SIZE,SIZE);
	graph_fill(&g,0xffffff);
	graph_draw_grid(&g,0x7f7f7f,0,GAP,GAP);
	graph_draw_axis(&g,0,1,GAP,GAP,16);
	graph_draw(&g,0x00ff00,2,pow1,sin,-SIZE,SIZE,0.0001);
	graph_drawe(&g,0xff0000,2,xep,ep,0,64,0.0001);
	FILE *fout = fopen("test.bmp", "wb");
	fwrite(graph_tobmp(&g),1,graph_size(&g),fout);
	fclose(fout);
	graph_free(&g);
	expr_free(ep);
	expr_symset_free(esp);
	expr_free(xep);
}
