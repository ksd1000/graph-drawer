/*******************************************************************************
 *License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>*
 *This is free software: you are free to change and redistribute it.           *
 *******************************************************************************/
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#define EXPR_BLOCKWARNING 1
#include "expr.h"
void wdaprint(const struct expr_finst *restrict fip){
#define wdashow(field,fmt,cast) \
	switch(fip->read_##field){\
		case 0:\
			printf(fmt,(cast)fip->flag->field);\
			break;\
		case 1:\
			printf("*");\
			break;\
		case 2:\
			printf("**");\
			break;\
		case 3:\
			printf("?");\
			break;\
		default:\
			abort();\
	}
#define flshow(field,ch) if(fip->flag->field)printf("%c",(int)(ch))
	flshow(plus,'+');
	flshow(space,' ');
	flshow(zero,'0');
	flshow(minus,'-');
	flshow(sharp,'#');
	flshow(eq,'=');
	flshow(cap,'!');
	flshow(saved,'?');
	flshow(addr,'&');
	printf("|");
	// make the space visible easily
	wdashow(width,"%zu",size_t);
	printf(".");
	wdashow(digit,"%zd",size_t);
	printf(":");
	wdashow(argsize,"%zu",size_t);
}
void dump(const struct expr_finst *restrict fip){
	for(;;++fip){
		switch(fip->op){
			case EXPR_FIMM:
				printf("immstr%s: \"",fip->shouldfree?"*":"");
				fwrite(fip->imm_str,1,fip->imm_len,stdout);
				// "%.*s" does not work properly in libc printf on
				// this mobile device(but properly on computer),
				// for the imm_str may be not terminated by 0
				// see bugs/libc-printf.c
				// ---
				// CLOSED
				// it is not a bug. in fact,it was caused by the author attempted to pass the length of the string with "%*s". but the behavior of passing a string with no terminating zero to "%.*s" is still undefined in C standard, so here will keep using fwrite.
				printf("\"\n");
				break;
			case EXPR_FCON:
				printf("conv: %%");
				wdaprint(fip);
				printf(" %c\n",(int)fip->flag->op);
				break;
			case EXPR_FCOL:
				printf("conv(l): %%");
				wdaprint(fip);
				printf(" %c\n",(int)fip->flag->op);
				break;
			case EXPR_FCOA:
				printf("conv(a): %%");
				wdaprint(fip);
				printf(" %c\n",(int)fip->flag->op);
				break;
			case EXPR_FCOZ:
				printf("conv(z): %%");
				wdaprint(fip);
				printf(" %c\n",(int)fip->flag->op);
				break;
			case EXPR_FCTL:
				printf("control: %%");
				wdaprint(fip);
				printf(" %c\n",(int)fip->flag->op);
				break;
			case EXPR_FEND:
				printf("end\n");
				fflush(stdout);
				return;
			default:
				abort();
				// not necessary to use __builtin_unreachable()
				// for here is not in libexpr.
		}
	}
}
const struct option ops[]={
	{"no-optimize",0,NULL,'n'},
	{"help",0,NULL,'h'},
	{NULL,0,NULL,0},
};
void show_help(const char *a0){
	fprintf(stdout,"usage: %s [options] format\n"
			"\t--no-optimize, -n\tdo not optimize\n"
			"\t--help, -h\tshow this help\n"
			"version: %s\n"
			,a0,expr_libinfo_current->version);
	exit(EXIT_SUCCESS);
}
int main(int argc,char **argv){
	char *e=NULL;
	struct expr_finst *fip;
	ssize_t r;
	int flag=0;
	if(argc<2)
		show_help(*argv);
	opterr=1;
	for(;;){
		switch(getopt_long(argc,argv,"nh",ops,NULL)){
			case 'n':
				flag|=EXPR_INOOPTIMIZE;
				break;
			case 'h':
				show_help(*argv);
				break;
			case -1:
				goto break2;
			case '?':
				exit(EXIT_FAILURE);
				break;
		}
	}
break2:
	if(optind>=argc){
		fprintf(stderr,"format not found");
		return EXIT_FAILURE;
	}
	e=argv[optind];
	r=expr_mkinstf(&fip,e,strlen(e),flag);
	if(r<0){
		fprintf(stderr,"mkinst(): %zd",r);
		return EXIT_FAILURE;
	}
	dump(fip);
	expr_ifree(fip);
	return EXIT_SUCCESS;
}
