/*******************************************************************************
 *License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>*
 *This is free software: you are free to change and redistribute it.           *
 *******************************************************************************/
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <getopt.h>
#include <errno.h>
#include "expr.h"
#include "fake_unix.h"
struct expr_buffered_file rline[1]={EXPR_BUFFERED_INITIALIZER(read,STDIN_FILENO,NULL,SIZE_MAX)};
static void *memchr_warped(const void *buf,intptr_t c,size_t size){
	return (void *)(uintptr_t)memchr(buf,(int)c,size);
}
const char *readline1(size_t *outsize){
	ssize_t r;
	char *p;
	r=expr_buffered_readline(rline,memchr_warped,'\n',&p);
	if(r<0){
		warn("read()");
		return NULL;
	}
	if(outsize)
		*outsize=(size_t)r;
	return p;
}
int notty=0;
void line_start(void){
	if(!notty&&isatty(STDIN_FILENO))
		fprintf(stderr,"expr> ");
}
void line_exit(void){
	if(!notty&&isatty(STDIN_FILENO))
		fprintf(stderr,"exit\n");
}
const struct option ops[]={
	{"no-tty",0,NULL,'b'},
	{"help",0,NULL,'h'},
	{NULL,0,NULL,0},
};
void show_help(const char *a0){
	fprintf(stdout,"usage: %s [options]\n"
			"\t--no-tty, -b\tassume the stderr does not refer to a tty\n"
			"\t--help, -h\tshow this help\n"
			"version: %s\n"
			,a0,expr_libinfo_current->version);
	exit(EXIT_SUCCESS);
}
int main(int argc,char **argv){
	double r;
	const char *p;
	struct expr_symset *es;
	int error=0,flag=0;
	char errinfo[EXPR_SYMLEN];
	opterr=1;
	for(;;){
		switch(getopt_long(argc,argv,"hb",ops,NULL)){
			case 'h':
				show_help(*argv);
				break;
			case 'b':
				notty=1;
				break;
			case -1:
				goto break2;
			case '?':
				exit(EXIT_FAILURE);
				break;
		}
	}
break2:
	es=expr_symset_new();
	if(!es)
		err(EXIT_FAILURE,"cannot allocate memory");
	if(expr_builtin_symbol_addalls(es,expr_symbols_default,NULL)<0)
		err(EXIT_FAILURE,"cannot allocate memory");
	for(line_start();(p=readline1(NULL))&&*p;line_start()){
		if(!p[1])switch(*p){
			case 'q':
				goto case_q;
			default:
				break;
		}
		r=expr_calc5(p,&error,errinfo,es,flag);
		if(error){
			warnx("expression error:%s \"%s\"",expr_error(error),errinfo);
		}
		expr_printf("%=.-1f\n",r);
		error=0;
	}
case_q:
	line_exit();
	expr_buffered_rclose(rline);
	expr_symset_free(es);
	return EXIT_SUCCESS;
}
