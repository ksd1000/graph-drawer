#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "expr/expr.h"
#include <time.h>
#include <err.h>
#include <assert.h>
#include <sys/random.h>
void add_common_symbols(struct expr_symset *es);
double dtime(void);
struct expr_symset es[1]={EXPR_SYMSET_INITIALIZER};
volatile sig_atomic_t vssuc;
volatile sig_atomic_t vsi;
void psig(int sig){
	fprintf(stderr,"final %d/%d\n",vssuc,vsi);
}
const char pool[]={"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_@"};
void randstr(char *buf,size_t sz){
	if(!sz)sz=1;
	do
		*(buf++)=pool[lrand48()%sizeof(pool)];
	while(--sz);
	*buf=0;
}
#ifdef __unix__
#include <sys/sysinfo.h>
double freemem(void){
	struct sysinfo si;
	sysinfo(&si);
	return (double)si.freeram/si.totalram;
}
size_t freememc(void){
	struct sysinfo si;
	sysinfo(&si);
	return si.freeram;
}
#else
double freemem(void){
	return 0;
}
#endif
char *buf=NULL;
char *p=NULL;
int freed=0;
__attribute__((constructor)) void bufst(void){
	buf=malloc(expr_allocate_max);
	assert(buf);
	p=buf;
}
__attribute__((destructor)) void bufend(void){
	if(!freed)
		free(buf);
}
void *alloc_hook(size_t sz){
	sz=(sz+7UL)&~7UL;
	void *r=p;
	p+=sz;
	assert(p<=buf+expr_allocate_max);
	return r;
}
void free_hook(void *r){
	return;
	if(r==buf)
		freed=1;
	free(r);
}
struct expr_symbol *esp;
char target[64]={"mustin"};
void readmode(void){
//	ssize_t r;
//	r=read(STDIN_FILENO,buf,expr_allocate_max);
//	fprintf(stderr,"read return %zd\n",expr_symset_read(es,buf,r));
	fprintf(stderr,"read return %zd\n",expr_symset_readfd(es,(ssize_t (*)(intptr_t,void *,size_t))read,STDIN_FILENO));
	(esp=expr_symset_search(es,target,strlen(target)))?
		fprintf(stderr,"found %s=%zd\n",esp->str,(ssize_t)expr_symset_un(esp)->value):fputs("fail\n",stderr);

}
ssize_t getrandom(void* __buffer, size_t __buffer_size, unsigned int __flags);
int main(int argc,char **argv){
	char buf[32];
	double st;
	size_t k,suc=0,l=0,count=argc>2?atol(argv[2]):1000,n=argc>1?atol(argv[1]):1000;
	size_t i;
	unsigned int in;
	if(argc>1&&!strcmp(argv[1],"r")){
		readmode();
		return 0;
	}
	if(argc>3)strcpy(target,argv[3]);
	expr_symset_init(es);
	//add_common_symbols(es);
	fputs("creating\n",stderr);
	st=dtime();
	getrandom(&in,4,GRND_NONBLOCK);
	srand48(in);
	signal(SIGABRT,psig);
	expr_allocator=alloc_hook;
	expr_deallocator=free_hook;
	for(i=1;suc<n;++i){
		//sfprintf(stderr,buf,"x%zu",i);
		randstr(buf,4+in%8);
		if(expr_symset_add(es,buf,EXPR_CONSTANT,0,(double)i))++suc;
		vssuc=suc;
		vsi=i;
		if(i-l>=50000){
			fprintf(stderr,"added %zu/%zuth %-32s\r",suc,i,buf);
			if(freememc()<1024*1024*20){
				fprintf(stderr,"\nout of memory\n");
				break;
			}
			l=i;
			getrandom(&in,4,GRND_NONBLOCK);
			srand48(in);
		}
	//	if(es->size%8000000==7999999){
			//expr_symset_save(es,alloc_hook(es->alength));
			//expr_symset_save(es,alloc_hook(es->alength));
			//printf("saved\n");
			//break;
	//	}
//		if(drand48()<0.6)
//			expr_symset_remove(es,buf,strlen(buf));
	}
	expr_symset_add(es,"mustin",EXPR_CONSTANT,0,-1.0);
	//fprintf(stderr,"\nsaving ...");
	//fprintf(stderr,"ok\n");
	if(!count){
		fprintf(stdout ,"NEXT:%d size:%zu depth:%zu length:%zu depth*length:%lg\n",EXPR_SYMNEXT,es->size,es->depth,es->length,(double)es->depth*es->length);
		goto nosearch;
	}
	fprintf(stderr,"added %zu/%zuth %-32s\n",suc,i-1,buf);
	fprintf(stderr,"size:%zu depth:%zu length:%zu depth*length:%lg\n",es->size,es->depth,es->length,(double)es->depth*es->length);
	fprintf(stderr,"creating time: %lg s\n",dtime()-st);
	fputs("searching\n",stderr);
	i=0;
	k=strlen(target);
	fprintf(stderr,"%s\n",target);
	st=dtime();
	while(--count)
		expr_symset_search(es,target,k);
	(esp=expr_symset_search(es,target,k))?
		fprintf(stderr,"found %s=%zd\n",esp->str,(ssize_t)expr_symset_un(esp)->value):fputs("fail\n",stderr);
	fprintf(stderr,"searching time: %lg s\n",dtime()-st);
nosearch:
#define pes(fmt) fprintf(stderr,"size:%zu,size_m:%zu,length:%zu,length_m:%zu,alength:%zu,alength_m:%zu,removed:%zu,removed_m:%zu,depth:%zu,depth_n:%zu,depth_nm:%zu " fmt "\n",es->size,es->size_m,es->length,es->length_m,es->alength,es->alength_m,es->removed,es->removed_m,es->depth,es->depth_n,es->depth_nm)
	pes("before corrected");
	/*
	*/
	//expr_symset_correct(es);
	//expr_symset_save(es,alloc_hook(es->alength));
	//fprintf(stderr,"write return %zd\n",expr_symset_write(es,(ssize_t (*)(intptr_t,const void *,size_t))write,STDOUT_FILENO));
	for(int i=0;i<0;++i){
		expr_symset_tryrecombine(es,in);
		pes("before recombined");
	}
	fprintf(stderr,"ok deallocating\n");
	/*
	n=0;
	st=0;
	expr_symset_foreach(sp,es,alloc_hook((es->depth-1)*EXPR_SYMSET_DEPTHUNIT)){
		d=100.0*++n/(double)es->size;
		if(d-st>=1){
			fprintf(stderr,"[%-6.2lf%%]\r",d);
			st=d;
		}
	}
	*/
	//expr_symset_free(es);
	fprintf(stderr,"\ndeallocated\n");
	return 0;
}
