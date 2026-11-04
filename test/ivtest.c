#define _GNU_SOURCE
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include "expr.h"
expr_globals_define();
// Remove if libexpr-lc is linked.
static ssize_t write_warped(intptr_t fd,const void *buf,size_t size){
	return write((int)fd,buf,size);
}
char buf[12341234];
struct expr_buffered_file buffer[1]={EXPR_BUFFERED_INITIALIZER(write_warped,STDOUT_FILENO,buf,sizeof(buf))};
#define handy_printf(fmt,...) ({\
	struct expr_finst *fip;\
	assert(expr_mkinstf(&fip,fmt,sizeof(fmt)-1)>0);\
	expr_apwritei(expr_buffered_write_iolbf_warped,(intptr_t)buffer,fip,##__VA_ARGS__);\
	expr_defmtl->deallocate(fip,expr_defmtl->arg);\
})
int main(void){
	handy_printf("hello\n");
	handy_printf("six=%7:id\n",6);
	handy_printf("six=%8l%:id\n",6);
	handy_printf("e=%f\n",M_E);
	handy_printf("City1:%s City2:%s\n","Kawasaki","Osaka");
	handy_printf("buf has %.2:zM size\n",sizeof(buf));
	// buffer is flushed by iolbf here.
	return EXIT_SUCCESS;
}
