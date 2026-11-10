#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>
#ifdef __unix__
#include <err.h>
#else
#define err(v,fmt,...) ({fprintf(stderr,fmt,##__VA_ARGS__);exit(v)})
#endif
#define EXPR_BLOCKWARNING 1
#include "expr.h"
#undef warn
#define warn(fmt,...) fprintf(stderr,fmt "\n",##__VA_ARGS__)
#define check_abort() ({warn("==expr-check==ABORTING");fflush(stderr);abort();})
static struct expr_symset *es;
static const struct proj {
	const char *e;
	double expect;
} projs[]={
	{"+7",7},
	{"-7",-7},
	{"1+5",6},
	{"5+3*8",29},
	{"(5+3)*8",64},
	{"134|45.5&7.5",135.5},
	{"134|(45.5&7.5)",135.5},
	{"(134|45.5)&7.5",7.5},
	{"6**3",216},
	{"6^3",5},
	{"0&&1",0},
	{"1&&0",0},
	{"0&&0",0},
	{"1&&1",1},
	{"0||1",1},
	{"1||0",1},
	{"0||0",0},
	{"1||1",1},
	{"0^^1",1},
	{"1^^0",1},
	{"0^^0",0},
	{"1^^1",0},
	{"5-7+6",4},
	{"5-(7+6)",-8},
	{"5<<3>>2",10},
	{"5<<(3>>2)",5},
	{"(5<<3)>>2",10},
	{"5+3>>2",2},
	{"5>>3+2",5.0/32},
	{"drand48(&x0)-->x,x+2**x<3",1},
	{"drand48(&x0)-->x,x+2**x>=3",0},
	{"drand48(&x0)-->x,x+2**x>=0",1},
	{"drand48(&x0)-->x,x+2**x<0",0},
	{"!(5-2)",0},
	{"!(5-5)",1},
	{"!!(5-2)",1},
	{"!!(5-5)",0},
	{"sum(n,1,100,1,n)",5050},
	{"vmd(n,0,10000,1,n,med,0)",5000},
	{"if(3,5,7)",5},
	{"if(0,5,7)",7},
	{"0-->m,while(m<7626){(1+m)->m,0},m",7626},
	{"0-->m,while(m<=7626){(1+m)->m,0},m",7627},
	{"-!3",-0.0},
	{"!-3",0},
	{"!-~3",0},
	{"!~-3",0},
	{"-!~3",-0.0},
	{"!!3",1},
	{"!!!3",0},
	{"!!0",0},
	{"!!!0",1},
	{"!~!-!!0",0},
	{"!!~!-!!0",1},
	{"(129+127)&~127",256},
	{"(128+127)&~127",128},
	{"(127+127)&~127",128},
	{"asint(0#128)",128},
	{"asint(0#(128*2)#1)",257},
	{"asint(0#(128*2)#1)",257},
	{"static_assert(e>2.71),2",2},
	{"defined(sin)",1},
	{"typeof(exp)",EXPR_FUNCTION},
	{"flagof(cos)&SPURE",1},
	{"f=(u,v){u+v**2},f(7,4)",23},
	{"undef(defined_symbol){1},defined_symbol",2304.0},
	{"alias(x,defined_symbol);x",2304.0},
	{"alias(x,\"5+6\");x=4;x+2",6},
	{NULL,0.0}

};
static const struct eproj {
	const char *e;
	int expect;
	const char *ee;
} eprojs[]={
	{"2+hshdjxjdjxhxhx",EXPR_ESYMBOL,"hshdjxjdjxhxhx"},
	{"xor(2,3,5,gg,8)",EXPR_ESYMBOL,"gg"},
	{"5->sum",EXPR_ESYMBOL,"sum"},
	{"sum(",EXPR_EPT,""},
	{"sum",EXPR_EFP,"sum"},
	{"sqrt()",EXPR_ENVP,""},
	{"$e",EXPR_ENUMBER,"e"},
	{"sum(n)",EXPR_ENEA,"sum"},
	{"266j",EXPR_EUSN,"j"},
	{"5->pi",EXPR_ETNV,"pi"},
	{"5->sqrt",EXPR_ETNV,"sqrt"},
	{"5->e",EXPR_ETNV,"e"},
	{"5->t",EXPR_ETNV,"t"},
	{"2+",EXPR_EEV,""},
	{"2->",EXPR_EEV,""},
	{"2->+",EXPR_EUO,"+"},
	{"abort(2)",EXPR_EZAFP,"abort"},
	{"abort(",EXPR_EZAFP,"abort"},
	{"abort",EXPR_EZAFP,"abort"},
	{"var(defined_symbol,5),0",EXPR_EDS,"defined_symbol"},
	{"vmd(k,1,10,1,k,cmp,0)",EXPR_EVMD,"cmp"},
	{"5-->v,&v[2]c",EXPR_EUSN,"c"},
	{"5-->v,&v[2]7",EXPR_EUSN,"7"},
	{"alloca(t,exp(t))",EXPR_ENC,"exp(t)"},
	{"static_assert(t>=2.72)",EXPR_ENC,"t>=2.72"},
	{"&e",EXPR_ECTA,"e"},
	{"static_assert(e>=2.72)",EXPR_ESAF,"e>=2.72"},
	{"static_assert(e<2.72)",EXPR_EVD,""},
	{"flag(IPROTECT),vmd(k,1,10,1,k,add,0)",EXPR_EVZP,"vmd"},
	{"undef(sin)",EXPR_EVD,""},
	{"flag(IPROTECT),flag(0)",EXPR_EPM,"flag"},
	{"flag(IPROTECT),&sin",EXPR_EPM,"&"},
	{"undef(defined_symbol),defined_symbol",EXPR_ESYMBOL,"defined_symbol"},
	{"undef(defined_symbol){defined_symbol}",EXPR_ESYMBOL,"defined_symbol"},
	{"flag(IPURE),drand48(&x0)",EXPR_EPURE,"drand48"},
	{"alias(x,y);alias(y,x);x",EXPR_EANT,"x"},
	{"alias(x,y);alias(y,z);alias(z,y);x",EXPR_EANT,"x"},
	{"error()",EXPR_EUDE,""},
	{"error(\"errname\")",EXPR_EUDE,"errname"},
	{"static_assert(2<1,\"errname\")",EXPR_EUDE,"errname"},
	{"alias(x,\"5+6\");x=4",EXPR_EVD,""},
	{"alias(x,\"5+6\");x=4;8-->x",EXPR_EDS,"5+6"},
	{"throw(EFP)",EXPR_EFP,""},
	{"throw(EPT,\"12345\")",EXPR_EPT,"12345"},
	{"throw(9999,\"customerror9999\")",9999,"customerror9999"},
	{"1+(1+(1+(1+(1+(1+(1+(1+(t))))))))",EXPR_EREC,""},
	{"byte(0x10000000)",EXPR_EMEM,""},
	{NULL,0,NULL}
};
static void errcheck(const char *e,int expect,const char *expect_errinfo){
	int error;
	int flag=EXPR_IRECURSIVELIMIT;
	char errinfo[EXPR_SYMLEN];
	printf("checking %s --- expect \"%s\"",e,expr_error(expect));
	if(expr_new(e,"t",es,flag|EXPR_IINSTANT_FREE,&error,errinfo)){
		printf("\nerror! %s should be \"%s\" but ok\n",e,expr_error(expect));
		goto ab;
	}
	if(error!=expect){
		printf("\nerror! %s should be \"%s\" but \"%s\"\n",e,expr_error(expect),expr_error(error));
		goto ab;
	}
	if(strcmp(errinfo,expect_errinfo)){
		printf("\nbad errorinfo! should be \"%s\" but is \"%s\"\n",expect_errinfo,errinfo);
		goto ab;
	}
	printf(" ... ok errinfo=\"%s\"\n",errinfo);
	return;
ab:
	check_abort();
}
static void check(const char *e,double expect){
	double r;
//	struct expr ep[1];
	//static int k=0;if(k++==39)exit(0);
	printf("checking %s --- expect %lg",e,expect);
//	if(expr_init(ep,e,"t",es,EXPR_IINSTANT_FREE)<0){
//		printf("\nerror! %s:%s\n",expr_error(ep->error),ep->errinfo);
//		goto ab;
//	}
	//exit(0);
	r=expr_calc5(e,NULL,NULL,es,EXPR_INOOPTIMIZE);
	if(memcmp(&r,&expect,sizeof(double))){
		printf("\nerror! %s should be %lg but %lg\n",e,expect,r);
		goto ab;
	}
	r=expr_calc5(e,NULL,NULL,es,0);
	if(memcmp(&r,&expect,sizeof(double))){
		printf("\noptimization error! %s should be %lg but %lg\n",e,expect,r);
		goto ab;
	}
	printf(" ... ok\n");
	return;
ab:
	check_abort();
}
#define REDZONE 1024
static void ckal(void *buf,size_t size){
	(void)size;
	assert(!((uintptr_t)buf%sizeof(struct expr_zoneunit)));
}
static void areacheck(size_t size,size_t off,int shouldfail){
	char *buf;
	struct expr_memtool mtl[1];
	buf=malloc(size+off+REDZONE*2);
	assert(buf);
	memset(buf,0xcc,size+off+REDZONE*2);
	if(shouldfail){
		if(expr_setup_heapmtl4(mtl,buf+off+REDZONE,size,EXPR_ADEFAULT|EXPR_AZERO)<0)
			goto end;
		printf("%zu,%zu should fail\n",size,off);
		check_abort();
	}
	assert(!expr_setup_heapmtl4(mtl,buf+off+REDZONE,size,EXPR_ADEFAULT|EXPR_AZERO));
	expr_explode_r(ckal,SIZE_MAX,mtl);
	for(size_t i=0;i<REDZONE+off;++i){
		assert(buf[i]==(char)0xcc);
	}
	for(size_t i=REDZONE+off+size;i<size+off+REDZONE*2;++i){
		assert(buf[i]==(char)0xcc);
	}
end:
	free(buf);
}
#define xmtl expr_defmtl
static void checkfmt(const char *expect,const char *fmt,...){
	char *p;
	va_list ap;
	ssize_t r;
	printf("checking \"%s\" --- expect \"%s\"\n",fmt,expect);
	va_start(ap,fmt);
	r=expr_vasprintf(&p,fmt,strlen(fmt),ap);
	va_end(ap);
	assert(r>=0);
	if(strcmp(p,expect)){
		printf("error result: \"%s\"\n",p);
		check_abort();
	}
	expr_xfree(p);
}
static void checkfmti(const char *expect,const char *fmt,...){
	static char buf[9999];
	va_list ap;
	struct expr_finst *fip;
	assert(expr_mkinstf(&fip,fmt,strlen(fmt),0)>0);
	va_start(ap,fmt);
	assert(expr_vsnprinti(buf,9999,fip,ap)<9999);
	va_end(ap);
	expr_ifree(fip);
	if(strcmp(buf,expect)){
		printf("error result(i): \"%s\"\n",buf);
		check_abort();
	}
}
#define checkfmt2(...) checkfmt(__VA_ARGS__);checkfmti(__VA_ARGS__)
static void checkfmts(void){
	checkfmt2("   1234","%7d",(expr_it)1234);
	checkfmt2("1234  ","%-6d",(expr_it)1234);
	checkfmt2("1231 ","%-5.*S",(size_t)4,"123123");
	checkfmt2("76b3e4","%.3h","\x76\xb3\xe4");
	checkfmt2("k","%c",expr_mkval64('k'));
	checkfmt2("100%+200%=k=300%","100%%+200%%=%c=300%%",expr_mkval64('k'));
	checkfmt2("372u,372u,372u","%3l%1T,%1tu%u",(expr_ut)372);
	checkfmt2("{5459,-833,-9999,67}","{%4k%1T,%:id}",(int []){5459,-833,-9999,67});
	checkfmt2("{\"asd\",\"fhl\",\"HELLO\"}","{%3k%3T\",\"\"%.5s\"}",(const char *[]){"asd\0aa","fhl\0aaa","HELLO","NOPRINT"});
	checkfmt2("null","%p",NULL);
	checkfmt2("NULL","%P",NULL);
	checkfmt2("1.5m","%=.-1m",(expr_ut)1024*1536);
	checkfmt2("1.7M","%.1M",(expr_ut)1024*1792);
	checkfmt2("aa\\1b\\0021\\2\\68\\n","%.10E","aa\1b\2\61\2\68\n");
	printf("fotmat check ok\n");
}
static int keyword_diff(const char *str,size_t len,const struct expr_builtin_keyword *restrict s){
	int r=(int)len-(int)s->strlen;
	if(r)
		return r;
	return memcmp(str,s->str,len);
}
struct clist {
	expr_list_t node;
	char c;
};
#define xmalloc expr_xmalloc
#define xfree expr_xfree
static struct clist *mkclist(const char *str){
	struct clist *h,*p;
	assert(*str);
	h=xmalloc(sizeof(struct clist));
	assert(h);
	h->c=*str;
	expr_list_ring(h->node);
	while(*(++str)){
		if(*str==h->c){
			assert(expr_list_isring(h->node));
			return h;
		}
		p=xmalloc(sizeof(struct clist));
		assert(p);
		p->c=*str;
		expr_list_rinsert_prev(h->node,p->node);
	}
	expr_list_rsethead(h->node);
	assert(!expr_list_isring(h->node));
	return h;
};
static void checkclist(struct clist *p,const char *expect){
	struct clist *p0=p;
	const char *e0=expect;
	printf("checking list --- expect \"%s\"\n",expect);
	if(expr_list_isring(p->node)){
		while(*expect){
			if(expect>e0&&*expect==p0->c){
				if(p->node!=p0->node){
					printf("error ring: \"%c\"\n",(int)(unsigned char)p->c);
					check_abort();
				}
				break;
			}
			if(p->c!=*expect){
				printf("error result: \"%c\"\n",(int)(unsigned char)p->c);
				check_abort();
			}
			p=expr_host(p->node->next,struct clist,node);
			++expect;
		}
		if(!*expect){
			printf("should be ring but not\n");
			check_abort();
		}
	}else {
		assert(!p->node->prev);
		while(*expect){
			assert(p);
			if(p->c!=*expect){
				printf("error result: \"%c\"\n",(int)(unsigned char)p->c);
				check_abort();
			}
			p=expr_host(p->node->next,struct clist,node);
			++expect;
		}
		assert(!p);
	}
	expr_list_uforeach0_rw(v,p0->node,next){
		xfree(expr_host(v,struct clist,node));
	}
}
static void killclist(struct expr_list *p){
	expr_list_detach_poison(p);
	assert(expr_list_ispoisoned(p));
	xfree(expr_host(p,struct clist,node));
}
#define hostof(p) expr_host(p,struct clist,node)
#define CKCLIST(str,after) for(struct clist *p=mkclist(str);p;checkclist(p,after),p=NULL)
static void checklist(void){
	struct clist *tmp;
	CKCLIST("012345","012345");
	CKCLIST("0123450","0123450");
	CKCLIST("012345","01245"){
		killclist(p->node->next->next->next);
	}
	CKCLIST("012345","01234"){
		killclist(p->node->next->next->next->next->next);
	}
	CKCLIST("012345","12345"){
		p=hostof(p->node->next);
		killclist(p->node->prev);
	}
	CKCLIST("0123450","012345"){
		expr_list_rsethead(p->node);
	}
	CKCLIST("012345","0123450"){
		expr_list_link(p->node->next->next->next->next->next,p->node);
	}
	CKCLIST("012345","234501"){
		tmp=hostof(p->node->next->next);
		expr_list_sethead(p->node,p->node->next->next->next->next->next,tmp->node);
		p=tmp;
	}
	CKCLIST("012345","234501"){
		p=hostof(expr_list_ucmul(p->node,p->node->next->next->next->next->next,2));
	}
	CKCLIST("012345","0123745"){
		tmp=mkclist("7");
		expr_list_insert_prev(p->node->next->next->next->next,tmp->node);
	}
	CKCLIST("012345","0123745"){
		tmp=mkclist("7");
		expr_list_insert_next(p->node->next->next->next,tmp->node);
	}
	CKCLIST("0123450","01237450"){
		tmp=mkclist("7");
		expr_list_rinsert_prev(p->node->next->next->next->next,tmp->node);
	}
	CKCLIST("0123450","01237450"){
		tmp=mkclist("7");
		expr_list_rinsert_next(p->node->next->next->next,tmp->node);
	}
	CKCLIST("012345","256789"){
		expr_list_foreach_ro(v,p->node,next)
			++hostof(v)->c;
		expr_list_foreach0_ro(v,p->node,next)
			++hostof(v)->c;
		expr_list_uforeach_ro(v,p->node,next)
			++hostof(v)->c;
		expr_list_uforeach0_ro(v,p->node,next)
			++hostof(v)->c;
	}
	CKCLIST("0123450","2567892"){
		expr_list_rforeach_ro(v,p->node,next)
			++hostof(v)->c;
		expr_list_rforeach0_ro(v,p->node,next)
			++hostof(v)->c;
		expr_list_uforeach_ro(v,p->node,next)
			++hostof(v)->c;
		expr_list_uforeach0_ro(v,p->node,next)
			++hostof(v)->c;
	}
	CKCLIST("012345","256789"){
		expr_list_foreach_rw(v,p->node,next)
			++hostof(v)->c;
		expr_list_foreach0_rw(v,p->node,next)
			++hostof(v)->c;
		expr_list_uforeach_rw(v,p->node,next)
			++hostof(v)->c;
		expr_list_uforeach0_rw(v,p->node,next)
			++hostof(v)->c;
	}
	CKCLIST("0123450","2567892"){
		expr_list_rforeach_rw(v,p->node,next)
			++hostof(v)->c;
		expr_list_rforeach0_rw(v,p->node,next)
			++hostof(v)->c;
		expr_list_uforeach_rw(v,p->node,next)
			++hostof(v)->c;
		expr_list_uforeach0_rw(v,p->node,next)
			++hostof(v)->c;
	}
	printf("list check ok\n");
}
#pragma GCC diagnostic ignored "-Wunused-parameter"
static char buf[8192*1024];
int main(int argc,char **argv){
	double x0=expr_cast(expr_seed48(time(NULL)),double);
	expr_setup_heapmtl(expr_defmtl,buf,sizeof(buf));
	es=expr_builtin_symbol_converts_r(expr_defmtl,expr_symbols_all);
	assert(es);
	expr_symset_addl(es,"x0",2,EXPR_VARIABLE,0,&x0);
	expr_symset_addl(es,"defined_symbol",14,EXPR_CONSTANT,0,2304.0);
	setvbuf(stdout,NULL,_IONBF,0);
	printf("checking area[%zu-%zu] + %zu-%zu",(size_t)0,(size_t)4096,(size_t)0,sizeof(struct expr_zoneunit)-1);
	for(size_t i=0;i<=4096;++i)
		for(size_t j=0;j<sizeof(struct expr_zoneunit);++j)
			areacheck(i,j,i<sizeof(struct expr_area)+sizeof(struct expr_zoneunit)+(j?sizeof(struct expr_zoneunit)-j:0));
	printf("\nchecking keyword\n");
	for(const struct expr_builtin_keyword *p=expr_keywords;;++p){
		if(!p->str){
			break;
		}
		if(p[1].str&&keyword_diff(p->str,(size_t)p->strlen,p+1)>=0){
			printf("keyword %s >= %s\n",p->str,p[1].str);
			check_abort();
		}
		assert(expr_keyword_search(p->str,(size_t)p->strlen,expr_keywords,expr_keywords_size)==p);
		assert(!expr_keyword_search("notkey",6,expr_keywords,expr_keywords_size));
		if(expr_symset_search(es,p->str,p->strlen)){
			printf("conflict %s\n",p->str);
			check_abort();
		}
	}
	for(const struct proj *p=projs;p->e;++p)
		check(p->e,p->expect);
	for(const struct eproj *p=eprojs;p->e;++p)
		errcheck(p->e,p->expect,p->ee);
	expr_symset_free(es);
	checkfmts();
	checklist();
#define areap ((const struct expr_area *)(expr_defmtl->arg))
	assert(areap->monotonic_create==areap->monotonic_combine);
	assert(areap->monotonic_deallocate==areap->monotonic_allocate+areap->monotonic_expand_move);
	return 0;
}
