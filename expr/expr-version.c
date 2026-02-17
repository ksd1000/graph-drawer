
/*******************************************************************************
 *License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>*
 *This is free software: you are free to change and redistribute it.           *
 *******************************************************************************/
#define _GNU_SOURCE
#include "expr.h"
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <ctype.h>
const struct expr_libinfo expr_libinfo[1]={{
	.version="pre-release",
	.compiler_version=
#ifdef __VERSION__
		__VERSION__,
#else
		"Unknown",
#endif
	.platform=EXPR_ARCHN " "
#ifdef __linux__
		"Linux",
#else
		"Unknown",
#endif
	.date=__DATE__,
	.time=__TIME__,
	.license="GPL v3+",
}};
void show(const struct expr_libinfo *el){
	fprintf(stdout,"expr version:%s\n",el->version);
	fprintf(stdout,"compiler:%s\n",el->compiler_version);
	fprintf(stdout,"platform:%s\n",el->platform);
	fprintf(stdout,"compiled date:%s\n",el->date);
	fprintf(stdout,"compiled time:%s\n",el->time);
	fprintf(stdout,"license:%s\n",el->license);
}
size_t all=0;
#ifdef __unix__
#include <sys/sysinfo.h>
double freemem(void){
	struct sysinfo si;
	sysinfo(&si);
	return (double)si.freeram/si.totalram;
}
#else
double freemem(void){
	return 1.0;
}
#endif
const char hchars[]={"BKMGTPEZY"};
double human(size_t sz,unsigned int *i){
	unsigned int l=0;
	double v=(double)sz;
	while(v>=1024&&hchars[l+1]){
		v/=1024;
		++l;
	}
	*i=l;
	return v;
}
void printprog(size_t size,size_t c,size_t n){
	unsigned int i,i1;
	double v,v1;
	v=human(size,&i);
	v1=human(all,&i1);
	fprintf(stdout,"allocated %.2lf %c / %.2lf %c, contracting [%6.2lf%%], %6.2lf%% memory free         \r",v,(int)hchars[i],v1,(int)hchars[i1],100.0*(double)c/n,100.0*freemem());
	fflush(stdout);
}
void contract_hook(void *buf,size_t size){
	char *p=(char *)buf,*endp=(char *)buf+size-1;
	size_t n=(size+expr_page_size-1)/expr_page_size,c=0;
	size_t old=0;
	while(p<=endp){
		*p=0;
		p+=expr_page_size;
		++c;
		if(!old||10000*(c-old)>=n){
			old=c;
			printprog(size,c,n);
		}
	}
	if(p!=endp)
		*endp=0;
	printprog(size,1,1);
}
void *alloc_hook(size_t size){
	void *r;
	r=malloc(size);
	if(!r)
		return NULL;
	all+=size;
	return r;
}
const struct option ops[]={
	{"abort",0,NULL,'a'},
	{"explode",2,NULL,'e'},
	{"segv",0,NULL,'s'},
	{"trap",0,NULL,'t'},
	{"unreachable",0,NULL,'u'},
	{"help",0,NULL,'h'},
	{"table",0,NULL,'T'},
	{NULL},
};
void show_help(void){
	fputs("usage:\n"
			"\t--abort ,-a\tcall abort\n"
			"\t--explode[==size] ,-e\tcall expr_explode\n"
			"\t--segv ,-s\twrite 0 to NULL\n"
			"\t--trap ,-t\tcall __builtin_trap\n"
			"\t--unreachable ,-u\tcall __builtin_unreachable\n"
			"\t--table ,-T\twrite the table for writef()\n"
			"\t--help ,-h\tshow this help\n"
			,stdout);
	exit(EXIT_SUCCESS);
}
void table_write(void){
	const struct expr_writefmt *f;
	uint8_t c,nextline=0;
	for(uint8_t i=0;i<expr_writefmts_default_size;){
		f=expr_writefmts_default+i;
		++i;
		for(uint8_t j=0;j<6;++j){
			if(!(c=f->op[j]))
				continue;
			nextline=1;
			switch(isprint(c)){
				case 0:
					fprintf(stdout,"[(uint8_t)\'\\x%x\']=%d,",(int)c,(int)i);
					break;
				default:
					fprintf(stdout,"[\'%c\']=%d,",(int)c,(int)i);
					break;
			}
		}
		if(nextline){
			nextline=0;
			fprintf(stdout,"\n");
		}
	}
	exit(EXIT_SUCCESS);
}
int main(int argc,char **argv){
	opterr=1;
	for(;;){
		switch(getopt_long(argc,argv,"ae::sthuT",ops,NULL)){
			case 'h':
				show_help();
			case 'T':
				table_write();
			case 'e':
				expr_allocator=alloc_hook;
				expr_contractor=contract_hook;
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
			case 'u':
				goto bu;
		}
	}
break2:
	show(expr_libinfo);
	return EXIT_SUCCESS;
bu:
	__builtin_unreachable();
}
