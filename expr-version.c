
/*******************************************************************************
 *License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>*
 *This is free software: you are free to change and redistribute it.           *
 *******************************************************************************/
#define _GNU_SOURCE
#include "expr.h"
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
void show(const struct expr_libinfo *el){
	fprintf(stdout,"expr version:%s\n",el->version);
	fprintf(stdout,"compiler:%s\n",el->compiler_version);
	fprintf(stdout,"compiled date:%s\n",el->date);
	fprintf(stdout,"compiled time:%s\n",el->time);
}
const struct option ops[]={
	{"abort",0,NULL,'a'},
	{"explode",2,NULL,'e'},
	{"segv",0,NULL,'s'},
	{"trap",0,NULL,'t'},
	{"help",0,NULL,'h'},
	{NULL},
};
void show_help(void){
	fputs("usage:\n"
			"\t--abort ,-a\tcall abort\n"
			"\t--explode[==size] ,-e\tcall expr_explode\n"
			"\t--segv ,-s\twrite 0 to NULL\n"
			"\t--trap ,-t\tcall __builtin_trap\n"
			"\t--help ,-h\tshow this help\n"
			,stdout);
	exit(EXIT_SUCCESS);
}
int main(int argc,char **argv){
	opterr=1;
	for(;;){
		switch(getopt_long(argc,argv,"ae::sth",ops,NULL)){
			case 'h':
				show_help();
			case 'e':
				if(optarg)
					expr_allocate_max=atol(optarg);
				expr_explode();
			case 't':
				__builtin_trap();
			case 's':
				*(volatile char *)NULL=0;
			case 'a':
				abort();
			case -1:
				goto break2;
			case '?':
				return EXIT_FAILURE;
				break;
		}
	}
break2:
	show(expr_libinfo);
	return EXIT_SUCCESS;
}
