
/*******************************************************************************
 *License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>*
 *This is free software: you are free to change and redistribute it.           *
 *******************************************************************************/
#define _GNU_SOURCE
#include "expr.h"
#include <stdio.h>
#include <stdlib.h>
void show(const struct expr_libinfo *el){
	fprintf(stdout,"expr version:%s\n",el->version);
	fprintf(stdout,"compiler:%s\n",el->compiler_version);
	fprintf(stdout,"compiled date:%s\n",el->date);
	fprintf(stdout,"compiled time:%s\n",el->time);
}
int main(int argc,char **argv){
	show(expr_libinfo);
	return EXIT_SUCCESS;
}
