/*******************************************************************************
 *License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>*
 *This is free software: you are free to change and redistribute it.           *
 *******************************************************************************/

// compile command: gcc nolibc.c -I ../expr ../expr/expr-internal.c ../expr/expr-buffered.c ../expr/expr-format.c ../expr/expr-alloc.c ../expr/expr-core.c -nostdlib -o nolibc

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syscall.h>
#include "expr.h"
#define strlen expr_internal_strlen
ssize_t warped_write(intptr_t fd,const void *buf,size_t size){
	return (ssize_t)expr_internal_syscall(SYS_write,fd,buf,size);
}
char wbuf[BUFSIZ];
struct expr_buffered_file printf_buf[1]={EXPR_BUFFERED_INITIALIZER_R(warped_write,STDOUT_FILENO,wbuf,BUFSIZ,NULL)};
char buf[1024*1024*16];
struct expr_memtool mtl[1];
#define expr_defmtl mtl
#if defined(__x86_64__)
__attribute__((force_align_arg_pointer))
#endif
int main(int argc,char **argv){
	struct expr *ep;
	expr_setup_heapmtl(mtl,buf,sizeof(buf));
	expr_ilprintf(printf_buf,"hello\n",argc,argv);
	ep=expr_new2_r("sum(n,1,100,1,n)",NULL,mtl);
	if(!ep)
		return EXIT_FAILURE;
	expr_ilprintf(printf_buf,"sum(n,1,100,1,n)=%f\n",expr_eval(ep,0.0));
	expr_free(ep);//the expr_free() is not necessary here.
	return EXIT_SUCCESS;
}
__attribute__((noreturn)) void _start(void){
	int r=main(0,NULL);
	expr_internal_syscall(SYS_exit,r);
	__builtin_unreachable();
}
