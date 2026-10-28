/*******************************************************************************
 *License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>*
 *This is free software: you are free to change and redistribute it.           *
 *******************************************************************************/
#define _GNU_SOURCE
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include "expr.h"
ssize_t write_warp(intptr_t fd,const void *buf,size_t len){
	FILE *fp;
	switch(fd){
		case 0:
			fp=stdin;
			break;
		case 1:
			fp=stdout;
			break;
		case 2:
			fp=stderr;
			break;
		default:
			fp=(FILE *)fd;
			break;
	}
	return (ssize_t)fwrite(buf,1,len,fp);
}
char wbuf[BUFSIZ];
expr_globals_define();
struct expr_buffered_file printf_buf[1]={EXPR_BUFFERED_INITIALIZER(write_warp,1,NULL,100000)};
// stdout is not a compile-time constant. use 1 map to it.
__attribute__((destructor)) void pbexit(void){
	expr_buffered_close(printf_buf);
}
struct maplc2e {
	const char *libc;
	const char *expr;
};
static const struct maplc2e blens[]={
	{"hh",":4"},
	{"h",":4"},
	{"",":4"},
#if __SIZEOF_LONG__==8
	{"l",":8"},
#else
	{"l",":4"},
#endif
	{"z",":8"},
	{"t",":8"},
	{"ll",":8"},
	{NULL},
};
size_t convert_from_libc_printf_style(const char *fmt,char *out){
	const char *c;
	size_t d,len;
	char *out0=out;
#define copy_once {*(out++)=*(fmt++);if(!*fmt)goto end;}
	for(;;){
		if(*fmt!='%'){
			copy_once;
			continue;
		}
		copy_once;
		while(memchr("+ -#0=&0123456789*.:",*fmt,20))
			copy_once;
		for(c=fmt;;++c){
			if(!*c)
				goto end;
			if(!memchr("hlzt",*c,4)&&expr_ftable_default->c[(uint8_t)*c])
				break;
		}
		if(memchr("diouxXcfFgGeEaA",*c,15)){
			d=c-fmt;
			for(const struct maplc2e *p=blens;p->libc;++p){
				len=strlen(p->libc);
				if(len!=d)
					continue;
				if(d&&memcmp(p->libc,fmt,d))
					continue;
				fmt=c;
				out=stpcpy(out,p->expr);
				break;
			}
		}
		copy_once;
	}
end:
	*out=0;
	return out-out0;
}
int my_vprintf(const char *fmt,va_list ap){
	int r;
	char *estyle=malloc(strlen(fmt)*2+1);
	size_t len=convert_from_libc_printf_style(fmt,estyle);
	//printf("convert result:%s\n",estyle);
	r=(int)expr_vapwritef(expr_buffered_write_iolbf_warped,(intptr_t)printf_buf,estyle,len,ap);
	free(estyle);
	return r;
}
__attribute__((format(printf,1,2))) int my_printf(const char *fmt,...){
	va_list ap;
	int r;
	va_start(ap,fmt);
	r=my_vprintf(fmt,ap);
	va_end(ap);
	return r;
}
int main(int argc,char **argv){
	my_printf("e is %lf and -one is %d,two is %zu\n",M_E,-1,2ul);
	my_printf("%e\n",M_E);
	my_printf("%g\n",M_E);
	my_printf("the string is:%s\n","This is a string");
}
