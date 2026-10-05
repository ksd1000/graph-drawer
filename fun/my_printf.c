/*******************************************************************************
 *License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>*
 *This is free software: you are free to change and redistribute it.           *
 *******************************************************************************/
#define _GNU_SOURCE
#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include "expr.h"
ssize_t linebuf(intptr_t fd,const void *buf,size_t size){
	return expr_buffered_write_flushat((struct expr_buffered_file *)fd,buf,size,"\n",1);
}
char wbuf[BUFSIZ];
expr_globals_define();
struct expr_buffered_file printf_buf[1]={EXPR_BUFFERED_INITIALIZER((expr_writer)write,STDOUT_FILENO,NULL,100000)};
struct mapl2e {
	const char *libc;
	const char *expr;
};
const struct mapl2e maps[]={
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
		while(memchr("+ -#0=?&i0123456789*.:",*fmt,22))
			copy_once;
		for(c=fmt;;++c){
			if(!*c)
				goto end;
			if(!memchr("hlzt",*c,4)&&expr_writefmts_table_default[(uint8_t)*c])
				break;
		}
		if(memchr("diouxXcfFgGeEaA",*c,15)){
			d=c-fmt;
			for(const struct mapl2e *p=maps;p->libc;++p){
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
	char *estyle=malloc(strlen(fmt)*2);
	size_t len=convert_from_libc_printf_style(fmt,estyle);
	r=(int)expr_vapwritef(estyle,len,linebuf,(intptr_t)printf_buf,ap);
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
