#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "expr.h"
#include <time.h>
#include <err.h>
#include <assert.h>
#include <sys/random.h>
#include <sys/time.h>
double dtime(void){
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC,&ts);
	return (double)ts.tv_sec+ts.tv_nsec/1000000000.0;
}
/*
void add_common_symbols(struct expr_symset *es);
volatile sig_atomic_t vssuc;
volatile sig_atomic_t vsi;
void psig(int sig){
	fprintf(stderr,"final %d/%d\n",vssuc,vsi);
}
*/
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
/*
struct expr_symbol *esp;
char target[64]={"mustin"};
void readmode(void){
//	ssize_t r;
//	r=read(STDIN_FILENO,buf,expr_allocate_max);
//	fprintf(stderr,"read return %zd\n",expr_symset_read(es,buf,r));
	fprintf(stderr,"read return %zd\n",expr_symset_readfd(es,(ssize_t (*)(intptr_t,void *,size_t))read,STDIN_FILENO));
	(esp=expr_symset_search(es,target,strlen(target)))?
		fprintf(stderr,"found %s=%zd\n",esp->str,(ssize_t)expr_symbol_un(esp)->value):fputs("fail\n",stderr);

}
*/
double area_free(const struct expr_area *a){
	return 1.0-(double)(((size_t)a->tail-(size_t)a->data)+sizeof(struct expr_zoneunit))/(double)a->data->prev;
}
const char pool[]={"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_@"};
int64_t seed=0;
void randstr(char *buf,size_t sz){
	int64_t state;
	while(sz--){
		state=expr_next48(&seed);
		state=expr_lto48(state);
		//state=expr_bswap64(state)>>16;
		*(buf++)=pool[(size_t)state%(sizeof(pool)-1)];
	}
	*buf=0;
}
struct expr_memtool expr_defmtl[1];
#define rct for(ct=0.0,st=dtime();ct==0.0;ct=dtime())
//int argc,char **argv
int main(void){
	size_t bufsize=UINT64_C(1024)*1024*1024*4;
	char *heap=malloc(bufsize);
	assert(bufsize);
	expr_setup_heapmtl(expr_defmtl,heap,bufsize);
	struct expr_symset *esp=expr_symset_new();
	assert(esp);
	char buf[32+1];
	size_t count=1000000;
	printf("adding %zu elements.\n",count);
	size_t symsize=8;
	size_t ok=0;
	seed=expr_seed48((int64_t)(dtime()*10));
	double ct,st;
	rct for(size_t i=0;i<count;++i){
		randstr(buf,symsize);
		if(!(i%1000000))
			printf("adding %s index=%zu mem_free(sys)=%.2lf%% mem_free(heap)=%.2lf%%\n",buf,i,100.0*freemem(),100.0*area_free(((const struct expr_area *)expr_defmtl->arg)));
		if(expr_symset_addl(esp,buf,symsize,EXPR_ALIAS,EXPR_SPURE,"hehe",4))
			++ok;
		else {
			if(((const struct expr_area *)expr_defmtl->arg)->monotonic_allocate_fail){
				
				printf("no enough memory at index=%zu\n",i);
				break;
			}
		}
	}
	printf("%zu ok. time=%.4lfms\n",ok,100*(ct-st));
#define pes(es) fprintf(stdout,"size:%zu,size_m:%zu,length:%zu,length_m:%zu,alength:%zu,alength_m:%zu,removed:%zu,removed_m:%zu,depth:%zu,depth_n:%zu,depth_nm:%zu " "\n",es->size,es->size_m,es->length,es->length_m,es->alength,es->alength_m,es->removed,es->removed_m,es->depth,es->depth_n,es->depth_nm)
	pes(esp);
	fprintf(stderr,"sqrt(depth*length)=%.2lf\tEXPR_SYMNEXT=%d\n",expr_internal_sqrt((double)esp->depth*(double)esp->length),EXPR_SYMNEXT);
	free(heap);
	return EXIT_SUCCESS;
}
/*
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
	for(i=1;suc<n;++i){
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
	}
	expr_symset_add(es,"mustin",EXPR_CONSTANT,0,-1.0);
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
		fprintf(stderr,"found %s=%zd\n",esp->str,(ssize_t)expr_symbol_un(esp)->value):fputs("fail\n",stderr);
	fprintf(stderr,"searching time: %lg s\n",dtime()-st);
nosearch:
	pes("before corrected");
	for(int i=0;i<0;++i){
		expr_symset_tryrecombine(es,in);
		pes("before recombined");
	}
	fprintf(stderr,"ok deallocating\n");
	fprintf(stderr,"\ndeallocated\n");
	return 0;
}
*/
