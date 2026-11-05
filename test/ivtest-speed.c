#define _GNU_SOURCE
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <err.h>
#include "expr.h"
expr_globals_define();
// Remove if libexpr-lc is linked.
ssize_t write_warped(intptr_t fd,const void *buf,size_t size);
ssize_t write_warped(intptr_t fd,const void *buf,size_t size){
	return write((int)fd,buf,size);
}
ssize_t dropper(intptr_t fd,const void *buf,size_t size);
ssize_t dropper(intptr_t fd,const void *buf,size_t size){
	(void)fd;
	(void)buf;
	return (ssize_t)size;
}
//#define writer write_warped
#define writer dropper
char buf[12341234];
#define FMT \
	"hello\n"\
	"100%%+100%%=200%%\n"\
	"six=%7:id\n"\
	"six=%8l%:id\n"\
	"e=%f\n"\
	"City1:%s City2:%s\n"\
	"buf has %.2:zM size\n"
#define ARG 6,6,M_E,"Kawasaki","Osaka",sizeof(buf)
/*
#define FMT \
	"hello\n"\
	"six=%7:id\n"\
	"City1:%s City2:%s\n"
#define ARG 6,"Kawasaki","Osaka"
*/
void fmode(size_t count){
	while(count--)
		expr_apwritef(writer,0,FMT,sizeof(FMT)-1,ARG);
}
void imode(size_t count){
	struct expr_finst *fip;
	assert(expr_mkinstf(&fip,FMT,sizeof(FMT)-1,0)>0);
	while(count--)
		expr_apwritei(writer,0,fip,ARG);
	expr_defmtl->deallocate(fip,expr_defmtl->arg);
}
void (*mode)(size_t)=fmode;
long atol2(const char *str){
	long r;
	char *c;
	r=strtol(str,&c,10);
	if(c==str||*c)
		errx(EXIT_FAILURE,"invaild integer: %s",str);
	return r;
}
int main(int argc,char **argv){
	size_t count=0;
	opterr=1;
	for(;;){
		switch(getopt(argc,argv,"fin:")){
			case 'n':
				count=(size_t)atol2(optarg);
				break;
			case 'f':
				mode=fmode;
				break;
			case 'i':
				mode=imode;
				break;
			case -1:
				goto break2;
			default:
				break;
		}
	}
break2:
	expr_setup_heapmtl(expr_defmtl,buf,sizeof(buf));
	mode(count);
	return EXIT_SUCCESS;
}
