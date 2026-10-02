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
#include <stdarg.h>
#define EXPR_BLOCKWARNING 1
#include "expr.h"
#ifdef __unix__
#define REAL_UNIX
const char *sysname(unsigned int id);
#else
#define sysname(id) ("unknown")
#endif

#include "fake_unix.h"
#define BUFSIZE 4096
//dump
#define PREFIX_SIZE 128
#define likely(cond) expr_likely(cond)
#define unlikely(cond) expr_unlikely(cond)

void add_common_symbols(struct expr_symset *);
static ssize_t linebuf(intptr_t fd,const void *buf,size_t size){
	return expr_buffered_write_sflushat((struct expr_buffered_file *)fd,buf,size,"\n",1);
}
char printfbuf[BUFSIZ];
struct expr_buffered_file printff=EXPR_BUFFERED_INITIALIZER(0,(expr_writer)write,printfbuf,BUFSIZ);
static void __attribute__((destructor)) ffend(void){
	expr_buffered_close(&printff);
}
double d_printf(double *args,size_t n){
	const char *fmt=expr_cast(*args,const char *);
	size_t flen=strlen(fmt);
	ssize_t r;
	r=expr_writef(fmt,flen,linebuf,(intptr_t)&printff,(const union expr_argf *)args+1,n-1);
	return (double)r;
}
double d_printl(double *args,size_t n){
	size_t len=(size_t)args[1];
	ssize_t r;
	r=expr_buffered_write_flushat(&printff,expr_cast(*args,const void *),len,"\n",1);
	return (double)r;
}
double d_printc(double *args,size_t n){
	ssize_t r;
	r=expr_writec(linebuf,(intptr_t)&printff,(size_t)args[1],(int)args[0]);
	return (double)r;
}
double d_sscanf(double *args,size_t n){
	const char *s,*fmt;
	size_t slen,flen;
	if(expr_unlikely(n<2))
		return 0;
	s=expr_cast(*args,const char *);
	fmt=expr_cast(args[1],const char *);
	slen=strlen(s);
	flen=strlen(fmt);
	return (double)expr_sscanf(s,slen,fmt,flen,(void **)(args+2),n-2);
}
char scanfbuf[BUFSIZ];
struct expr_buffered_file scanff=EXPR_BUFFERED_INITIALIZER(0,read,scanfbuf,BUFSIZ);
double d_scanf(double *args,size_t n){
	const char *s,*fmt;
	size_t slen,flen;
	slen=expr_buffered_readline(&scanff,'\n',&s);
	if(expr_unlikely((ssize_t)slen<0))
		return -1;
	fmt=expr_cast(args[0],const char *);
	flen=strlen(fmt);
	return (double)expr_sscanf(s,slen,fmt,flen,(void **)(args+1),n-1);
}
static void *xmalloc(size_t size){
	void *r;
	r=malloc(size);
	if(unlikely(!r)){
		warn("IN xmalloc(size=%zu)\n"
			"CANNOT ALLOCATE MEMORY",size);
		warnx("ABORTING");
		abort();
	}
	return r;
}
static void *xrealloc(void *old,size_t size){
	void *r;
	r=realloc(old,size);
	if(unlikely(!r)){
		warn("IN xrealloc(old=%p,size=%zu)\n"
			"CANNOT REALLOCATE MEMORY",old,size);
		warnx("ABORTING");
		abort();
	}
	return r;
}
char prefix[PREFIX_SIZE]={[0 ... (PREFIX_SIZE-1)]=' '};
unsigned long level=0;
void writeprefix(void){
	unsigned long n;
	n=level*4;
	while(n>=PREFIX_SIZE){
		fwrite(prefix,1,PREFIX_SIZE,stdout);
		n-=PREFIX_SIZE;
	}
	if(n)
		fwrite(prefix,1,n,stdout);
}
int xprintf(const char *fmt,...){
	va_list ap;
	int r;
	va_start(ap,fmt);
	writeprefix();
	r=vprintf(fmt,ap);
	va_end(ap);
	return r;
}
int addr2sym(const struct expr *restrict ep,const struct expr_symset *restrict esp,char buf[EXPR_SYMLEN],void *addr){
	union {
		const struct expr_symbol *es;
	} sym;
	if(!addr)return -1;
	if(addr==EXPR_VOID||addr==EXPR_VOID_NR){
		strcpy(buf,"EXPR_VOID");
		return 0;
	}
	sym.es=NULL;
	if(ep->sset)sym.es=expr_symset_rsearch(esp,addr);
	if(sym.es){
		strcpy(buf,sym.es->str);
		return 0;
	}
	return -1;
}
#define idex(_ps,_indexof) \
const void **_ps=NULL;\
size_t _ps##_size=0;\
size_t _indexof(const struct expr *ep){\
	size_t i=0;\
	for(;i<_ps##_size;++i){\
		if(_ps[i]==ep)\
			return i;\
	}\
	_ps=_ps?xrealloc(_ps,(i+1)*sizeof(void *)):xmalloc((i+1)*sizeof(void *));\
	_ps[i]=ep;\
	_ps##_size=i+1;\
	return i;\
}\
__attribute__((destructor)) void _ps##atend(void){\
	if(_ps)\
		free(_ps);\
}
idex(eps,indexof);
idex(ps,indexofp);
idex(ups,indexofu);
enum odtype {NUL,MEM,ADDR,SUM,BRANCH,HOT,MD,VMD,HMD,VAL,ZVAL,MEMF,FLAG};
char subexpr[]={[NUL]=0,[MEM]=0,[ADDR]=0,[SUM]=1,[BRANCH]=1,[HOT]=1,[MD]=1,[VMD]=1,[HMD]=1,[VAL]=0,[ZVAL]=0,[MEMF]=0,[FLAG]=0,};
#define hassubexpr(_op) subexpr[ii[_op].st]
struct inst_info {
	const char *name;
	enum odtype st;
};
const struct inst_info ii[]={
[EXPR_COPY]={.name="copy",.st=MEM,},
[EXPR_INPUT]={.name="input",.st=NUL,},
[EXPR_CONST]={.name="const",.st=VAL,},
[EXPR_BL]={.name="bl",.st=ADDR,},
[EXPR_ADD]={.name="add",.st=MEM,},
[EXPR_SUB]={.name="sub",.st=MEM,},
[EXPR_MUL]={.name="mul",.st=MEM,},
[EXPR_DIV]={.name="div",.st=MEM,},
[EXPR_MOD]={.name="mod",.st=MEM,},
[EXPR_POW]={.name="pow",.st=MEM,},
[EXPR_AND]={.name="and",.st=MEM,},
[EXPR_XOR]={.name="xor",.st=MEM,},
[EXPR_OR]={.name="or",.st=MEM,},
[EXPR_SHL]={.name="shl",.st=MEM,},
[EXPR_SHR]={.name="shr",.st=MEM,},
[EXPR_GT]={.name="gt",.st=MEM,},
[EXPR_GE]={.name="ge",.st=MEM,},
[EXPR_LT]={.name="lt",.st=MEM,},
[EXPR_LE]={.name="le",.st=MEM,},
[EXPR_SLE]={.name="sle",.st=MEM,},
[EXPR_SGE]={.name="sge",.st=MEM,},
[EXPR_SEQ]={.name="seq",.st=MEM,},
[EXPR_SNE]={.name="sne",.st=MEM,},
[EXPR_EQ]={.name="eq",.st=MEM,},
[EXPR_NE]={.name="ne",.st=MEM,},
[EXPR_ANDL]={.name="andl",.st=MEM,},
[EXPR_ORL]={.name="orl",.st=MEM,},
[EXPR_XORL]={.name="xorl",.st=MEM,},
[EXPR_NEXT]={.name="next",.st=MEM,},
[EXPR_DIFF]={.name="diff",.st=MEM,},
[EXPR_NEG]={.name="neg",.st=NUL,},
[EXPR_NOT]={.name="not",.st=NUL,},
[EXPR_NOTL]={.name="notl",.st=NUL,},
[EXPR_TSTL]={.name="tstl",.st=NUL,},
[EXPR_NEX0]={.name="nex0",.st=NUL,},
[EXPR_DIF0]={.name="dif0",.st=NUL,},
[EXPR_IF]={.name="if",.st=BRANCH,},
[EXPR_WHILE]={.name="while",.st=BRANCH,},
[EXPR_DO]={.name="do",.st=HOT,},
[EXPR_DOW]={.name="dow",.st=BRANCH,},
[EXPR_WIF]={.name="wif",.st=HOT,},
[EXPR_DON]={.name="don",.st=BRANCH,},
[EXPR_SUM]={.name="sum",.st=SUM,},
[EXPR_INT]={.name="int",.st=SUM,},
[EXPR_PROD]={.name="prod",.st=SUM,},
[EXPR_SUP]={.name="sup",.st=SUM,},
[EXPR_INF]={.name="inf",.st=SUM,},
[EXPR_ANDN]={.name="andn",.st=SUM,},
[EXPR_ORN]={.name="orn",.st=SUM,},
[EXPR_XORN]={.name="xorn",.st=SUM,},
[EXPR_GCDN]={.name="gcdn",.st=SUM,},
[EXPR_LCMN]={.name="lcmn",.st=SUM,},
[EXPR_FOR]={.name="for",.st=SUM,},
[EXPR_ZA]={.name="za",.st=ADDR,},
[EXPR_MD]={.name="md",.st=MD,},
[EXPR_ME]={.name="me",.st=MD,},
[EXPR_MEP]={.name="mep",.st=MD,},
[EXPR_VMD]={.name="vmd",.st=VMD},
[EXPR_DO1]={.name="do1",.st=HOT,},
[EXPR_EP]={.name="ep",.st=HOT,},
[EXPR_EVAL]={.name="eval",.st=MEM,},
[EXPR_HOT]={.name="hot",.st=HOT,},
[EXPR_PBL]={.name="pbl",.st=MEM,},
[EXPR_PZA]={.name="pza",.st=MEM,},
[EXPR_PMD]={.name="pmd",.st=MD,},
[EXPR_PME]={.name="pme",.st=MD,},
[EXPR_PMEP]={.name="pmep",.st=MD,},
[EXPR_READ]={.name="read",.st=MEM,},
[EXPR_WRITE]={.name="write",.st=MEM,},
[EXPR_OFF]={.name="off",.st=MEM,},
[EXPR_ALO]={.name="alo",.st=ZVAL,},
[EXPR_SJ]={.name="sj",.st=NUL,},
[EXPR_LJ]={.name="lj",.st=MEM,},
[EXPR_IP]={.name="ip",.st=ZVAL,},
[EXPR_TO]={.name="to",.st=NUL,},
[EXPR_TO1]={.name="to1",.st=NUL,},
[EXPR_HMD]={.name="hmd",.st=HMD,},
[EXPR_RET]={.name="ret",.st=MEM,},
[EXPR_SVC]={.name="svc",.st=FLAG,},
[EXPR_SVC0]={.name="svc0",.st=FLAG,},
[EXPR_SVCP]={.name="svcp",.st=MEMF,},
[EXPR_SVCP0]={.name="svcp0",.st=MEMF,},
[EXPR_END]={.name="end",.st=NUL,},
};
const char *adst(const double *dst){
	static char abuf[64+EXPR_SYMLEN];
	if(dst==EXPR_VOID||dst==EXPR_VOID_NR)
		strcpy(abuf,"[void]");
	else
		sprintf(abuf,"%lg",*dst);
	return abuf;
}
ssize_t varindex(const struct expr *restrict ep,double *v){
	for(size_t i=0;i<ep->vsize;++i){
		if(ep->vars[i]==v)return i;
	}
	return -1;
}
const char *symbolof(const struct expr *restrict ep,void *addr){
	union {
		const struct expr_symbol *es;
	} sym;
	sym.es=NULL;
	if(ep->sset)
		sym.es=expr_symset_rsearch(ep->sset,addr);
	if(sym.es){
		return sym.es->str;
	}
	return NULL;
}
const char *od(const struct expr *restrict ep,void *addr){
	static char abuf[64+EXPR_SYMLEN];
	ssize_t index;
	const char *p;
	if(addr==EXPR_VOID||addr==EXPR_VOID_NR){
		strcpy(abuf,"[void]");
		return abuf;
	}
	p=symbolof(ep,addr);
	if(p){
		strcpy(abuf,p);
		return abuf;
	}
	index=varindex(ep,addr);
	if(index>=0){
		sprintf(abuf,"[%zd]",index);
		return abuf;
	}
	if((double *)addr>=ep->un.args&&(double *)addr<ep->un.args+EXPR_SYSAM){
		sprintf(abuf,"r[%zd]",(double *)addr-ep->un.args);
		return abuf;
	}
	sprintf(abuf,"u[%zd]",indexofu(addr));
	return abuf;
}
const char *ainst(const struct expr *restrict ep,struct expr_inst *ip){
	static char abuf[128+EXPR_SYMLEN];
	const char *p;
	int r=sprintf(abuf,"%-5s %-6s",ii[ip->op].name,od(ep,ip->dst.uaddr));
	switch(ii[ip->op].st){
		case NUL:
			break;
		case MEM:
			sprintf(abuf+r," %-5s=%.3lg",od(ep,ip->un.uaddr),*ip->un.src);
			break;
		case ADDR:
			sprintf(abuf+r," %-5s",od(ep,ip->un.uaddr));
			break;
		case MEMF:
			sprintf(abuf+r," %-5s %s(%d)",od(ep,ip->un.uaddr),sysname((unsigned int)*ip->dst.dst),ip->flag);
			break;
		case FLAG:
			sprintf(abuf+r," %s(%d)",sysname((unsigned int)*ip->dst.dst),ip->flag);
			break;
		case SUM:
			sprintf(abuf+r," sum[%zu,%zu,%zu,%zu]",indexof(ip->un.es->fromep),indexof(ip->un.es->toep),indexof(ip->un.es->stepep),indexof(ip->un.es->ep));
			break;
		case BRANCH:
			sprintf(abuf+r," branch[%zu,%zu,%zu]",indexof(ip->un.eb->cond),indexof(ip->un.eb->body),indexof(ip->un.eb->value));
			break;
		case MD:
			p=symbolof(ep,ip->un.em->un.uaddr);
			if(p)
				sprintf(abuf+r," %s(%zu)",p,ip->un.em->dim);
			else
				sprintf(abuf+r," (mdinfo)heap[%zu]",indexofp(ip->un.uaddr));
			break;
		case VMD:
			sprintf(abuf+r," vmd[%zu,%zu,%zu,%zu](%zu)",indexof(ip->un.ev->fromep),indexof(ip->un.ev->toep),indexof(ip->un.ev->stepep),indexof(ip->un.ev->ep),ip->un.ev->max);
			break;
		case HMD:
			sprintf(abuf+r," expr[%zu](%zu)",indexof(ip->un.eh->hotfunc),ip->un.eh->dim);
			break;
		case HOT:
			sprintf(abuf+r," expr[%zu]",indexof(ip->un.hotfunc));
			break;
		case VAL:
			sprintf(abuf+r," %lg",*ip->dst.dst=ip->un.value);
			break;
		case ZVAL:
			sprintf(abuf+r," %zu",ip->un.zu);
			break;
		default:
			sprintf(abuf+r," (unknown)heap[%zu]",indexofp(ip->dst.uaddr));
			break;
	}
	return abuf;
}
size_t line=0;
void list(const struct expr *restrict ep,const struct expr_symset *restrict esp){
	size_t d;
#define list1(x) list((x),ep->sset)
	printf("={\n");
	//xprintf("[%zu],[%zu]{\n",ep->size,ep->vsize);
	++level;
	for(struct expr_inst *ip=ep->data;ip-ep->data<ep->size;++ip){
	xprintf("%zu:[%zu]->[%zd] %s\n",line++,indexof(ep),ip-ep->data,ainst(ep,ip));
	switch(ii[ip->op].st){
		case SUM:
			++level;
			//xprintf("sum=\n");
			xprintf("->from=expr[%zu]",indexof(ip->un.es->fromep));
			list1(ip->un.es->fromep);
			xprintf("->to=expr[%zu]",indexof(ip->un.es->toep));
			list1(ip->un.es->toep);
			xprintf("->step=expr[%zu]",indexof(ip->un.es->stepep));
			list1(ip->un.es->stepep);
			xprintf("->ep=expr[%zu]",indexof(ip->un.es->ep));
			list1(ip->un.es->ep);
			--level;
			break;
		case BRANCH:
			++level;
			//xprintf("branch=\n");
			xprintf("->cond=expr[%zu]",indexof(ip->un.eb->cond));
			list1(ip->un.eb->cond);
			xprintf("->body=expr[%zu]",indexof(ip->un.eb->body));
			list1(ip->un.eb->body);
			xprintf("->value=expr[%zu]",indexof(ip->un.eb->value));
			list1(ip->un.eb->value);
			--level;
			break;
		case MD:
			++level;
			//xprintf("md(%zu)=\n",d=ip->un.em->dim);
			d=ip->un.em->dim;
			for(size_t i=0;i<d;++i){
				xprintf("->eps[%zu]=expr[%zu]",i,indexof(ip->un.em->eps+i));
				list1(ip->un.em->eps+i);
			}
			--level;
			break;
		case VMD:
			++level;
			xprintf("->max=%zu\n",ip->un.ev->max);
			xprintf("->from=expr[%zu]",indexof(ip->un.ev->fromep));
			list1(ip->un.ev->fromep);
			xprintf("->to=expr[%zu]",indexof(ip->un.ev->toep));
			list1(ip->un.ev->toep);
			xprintf("->step=expr[%zu]",indexof(ip->un.ev->stepep));
			list1(ip->un.ev->stepep);
			xprintf("->ep=expr[%zu]",indexof(ip->un.ev->ep));
			list1(ip->un.ev->ep);
			--level;
			break;
		case HMD:
			++level;
			//xprintf("hmd(%zu)=\n",d=ip->un.eh->dim);
			d=ip->un.eh->dim;
			for(size_t i=0;i<d;++i){
				xprintf("->eps[%zu]=expr[%zu]",i,indexof(ip->un.eh->eps+i));
				list1(ip->un.eh->eps+i);
			}
			--level;
			break;
		case HOT:
			++level;
			xprintf("->hot=expr[%zu]",indexof(ip->un.hotfunc));
			list1(ip->un.hotfunc);
			--level;
			break;
		default:
			break;
	}
	}
	--level;
	xprintf("}\n",ep->size,ep->vsize);
}
void *readall(intptr_t fd,size_t *len);
void printdouble(double val){
	char *buf,*p;
	if(asprintf(&buf,"%.4096lf",val)<0){
		warn("asprintf");
		return;
	}
	p=strchr(buf,'.');
	if(p){
		p+=strlen(p);
		while(*(--p)=='0')*p=0;
		if(*p=='.')*p=0;

	}
	puts(buf);
	free(buf);
}
struct expr_symset *es=NULL;
char *rbuf=NULL;
double atod2(const char *str){
	double r;
	int error=0;
	char err[EXPR_SYMLEN];
	r=expr_calc5(str,&error,err,NULL,EXPR_IF_PROTECT|EXPR_IF_NOKEYWORD);
	if(error)
		errx(EXIT_FAILURE,"invaild expression: %s (%s:%s)",str,expr_error(error),err);
	return r;
}
void atend(void){
	if(es)
		expr_symset_free(es);
	if(rbuf)
		expr_deallocator(rbuf);
}
#define STACK_SIZE (128*1024)
size_t sstack[STACK_SIZE]={0};
size_t *ssp=sstack;
void push(size_t n){
	if(ssp-sstack>=STACK_SIZE)
		return;
	*(ssp++)=n;
}
size_t pop(void){
	if(ssp<sstack)
		return -1;
	return *(--ssp);
}
int gchr=0;
static int stop(void){
	return gchr?getchar():0;
}
void callee(const struct expr *ep,struct expr_inst *ip,void *arg){
	static char abuf[128];
	sprintf(abuf,"%zu:expr[%zu]->data[%td]",line++,indexof(ep),ip-ep->data);
	if(hassubexpr(ip->op)){
		printf("%-26s %-32s begin {\n",abuf,ainst(ep,ip));
		push(line-1);
		stop();
	}else
		printf("%-26s %-28s ...",abuf,ainst(ep,ip));
}
void callee_end(const struct expr *ep,struct expr_inst *ip,void *arg){
	static char abuf[128];
	if(hassubexpr(ip->op)){
		sprintf(abuf,"%zu:expr[%zu]->data[%td]",pop(),indexof(ep),ip-ep->data);
		printf("%-57s } ok dst=%s\n",abuf,adst(ip->dst.dst));
	}else {
		printf(" ok dst=%s\n",adst(ip->dst.dst));
	}
	stop();
}
struct expr_callback ec={
	.before=callee,
	.after=callee_end,
	.arg=NULL,
};
double callbackmode(struct expr *ep,double input){
	line=0;
	return expr_callback(ep,input,&ec);
}
double stepmode(struct expr *ep,double input){
	struct expr_inst *ip=ep->data;
	double r;
	do{
		printf("%-26s dst=%lg\n",ainst(ep,ip),*ip->dst.dst);
	}while(!expr_step(ep,input,&r,&ip));
	return r;
}
const struct option ops[]={
	{"safe",0,NULL,'p'},
	{"no-optimize",0,NULL,'n'},
	{"no-builtin",0,NULL,'N'},
	{"dump",0,NULL,'D'},
	{"quiet",0,NULL,'q'},
	{"count",1,NULL,0xff01},
	{"injection",0,NULL,'i'},
	{"step",0,NULL,'s'},
	{"callback",0,NULL,'c'},
	{"keep",0,NULL,'k'},
	{"detach",0,NULL,'d'},
	{"getchar",0,NULL,'g'},
	{"help",0,NULL,'h'},
	{"file",1,NULL,'f'},
	{NULL,0,NULL,0},
};
void show_help(const char *a0){
	fprintf(stdout,"usage: %s [options] expression\n"
			"\t--safe, -p\twork on protected mode\n"
			"\t--no-optimize, -n\tdo not optimize\n"
			"\t--no-builtin, -N\tdo not use builtin symbols\n"
			"\t--dump, -D\tdump mode(do not evaluate)\n"
			"\t--quiet, -q\tdo not print the result\n"
			"\t--count count\tevaluate how many times,default 1\n"
			"\t--injection, -i\tuse injective function only\n"
			"\t--step, -s\tstep mode\n"
			"\t--callback, -c\tcallback mode\n"
			"\t--keep, -k\tkeep symbol sets,use with -d to make symbols visible\n"
			"\t--detach, -d\tdetach each symbol sets,use with -k to make symbols visible\n"
			"\t--getchar, -g\tcall getchar() on each instructions if -s/-c is set\n"
			"\t--file, -f file\tread expression from file\n"
			"\t--help, -h\tshow this help\n"
			"extend function:\n"
			"\tscanf(format,...)\n"
			"\tsscanf(str,format,...)\n"
			"\tprintf(format,...)\n"
			"\tprintl(buf,len)\n"
			"\tprintc(c,n)\n"
			"\tdestructor(val)\n"
			"example:\n"
			"\t%s 3**2+sin(pi/6)\n"
			,a0,a0);
	exit(EXIT_SUCCESS);
}
int main(int argc,char **argv){
	char *e=NULL;
	int flag=0;
	int dump=0;
	int adbt=0;
	int nobt=0;
	int r0;
	intptr_t fd;
	enum {NORMAL,STEP,CALLBACK} mode=NORMAL;
	size_t count=1;
	double r;
	struct expr ep[1];
	jmp_buf jb;
	volatile double show_result=1.0;
	atexit(atend);
	setvbuf(stdout,NULL,_IONBF,0);
	if(argc<2)
		show_help(*argv);
	opterr=1;
	for(;;){
		switch(getopt_long(argc,argv,"pnDt::Nisckdgf:hq",ops,NULL)){
			case 'p':
				flag|=EXPR_IF_PROTECT;
				break;
			case 'n':
				flag|=EXPR_IF_NOOPTIMIZE;
				break;
			case 'N':
				nobt=1;
				break;
			case 'i':
				flag|=EXPR_IF_INJECTION;
				break;
			case 'k':
				flag|=EXPR_IF_KEEPSYMSET;
				break;
			case 'd':
				flag|=EXPR_IF_DETACHSYMSET;
				break;
			case 'g':
				gchr=1;
				break;
			case 'D':
				dump=1;
				break;
			case 'q':
				show_result=0.0;
				break;
			case 's':
				mode=STEP;
				break;
			case 'c':
				mode=CALLBACK;
				break;
			case 0xff01:
				count=(size_t)atod2(optarg);
				break;
			case 'h':
				show_help(*argv);
				break;
			case 'f':
				fd=open(optarg,O_RDONLY);
				if(fderr(fd))
					err(EXIT_FAILURE,"cannot open file");
				rbuf=readall(fd,NULL);
				close(fd);
				if(!rbuf)
					err(EXIT_FAILURE,"cannot read");
				e=rbuf;
				--optind;
				goto break3;
			case -1:
				goto break2;
			case '?':
				exit(EXIT_FAILURE);
				break;
		}
	}
break2:
	if(optind>=argc){
		errx(EXIT_FAILURE,"expression not found");
	}
	if(!e)
		e=argv[optind];
	if(!strcmp(e,"-")){
		rbuf=readall((intptr_t)stdin,NULL);
		if(!rbuf)
			err(EXIT_FAILURE,"cannot read expression from stdin");
		e=rbuf;
	}
break3:
	es=expr_symset_new();
	if(!es)
		err(EXIT_FAILURE,"cannot allocate memory");
	add_common_symbols(es);
	printff.fd=STDOUT_FILENO;
	scanff.fd=STDIN_FILENO;
	expr_symset_add(es,"ret",EXPR_HOTFUNCTION,0,"(ep,val){reset(end);([ep]#([ep#SIZE_OFF]##(0#1))*INSTLEN)-->end;(end#-INSTLEN)->[[ep#IPP_OFF]];val->[[end]]}");
	expr_symset_add(es,"destructor",EXPR_HOTFUNCTION,0,"(val){destruct(&#,&longjmp_out,&outbuf,val)}");
	expr_symset_add(es,"calc_show_result",EXPR_VARIABLE,0,&show_result);
	expr_symset_add(es,"outbuf",EXPR_VARIABLE,0,jb);
	expr_symset_add(es,"argc",EXPR_CONSTANT,0,(double)(argc-optind));
	expr_symset_add(es,"argv",EXPR_CONSTANT,0,expr_cast(argv+optind,double));
	expr_symset_add(es,"scanf",EXPR_MDFUNCTION,EXPR_SF_UNSAFE,d_scanf,(size_t)0);
	expr_symset_add(es,"sscanf",EXPR_MDFUNCTION,EXPR_SF_UNSAFE,d_sscanf,(size_t)0);
	expr_symset_add(es,"printf",EXPR_MDFUNCTION,EXPR_SF_UNSAFE,d_printf,(size_t)0);
	expr_symset_add(es,"printl",EXPR_MDFUNCTION,EXPR_SF_UNSAFE,d_printl,(size_t)2);
	expr_symset_add(es,"printc",EXPR_MDFUNCTION,EXPR_SF_UNSAFE,d_printc,(size_t)2);
	if(adbt||!nobt)
		expr_builtin_symbol_addalls(es,expr_symbols_ess);
	if(expr_init(ep,e,"t",es,flag)<0){
		if(*ep->errinfo)
			errx(EXIT_FAILURE,"expression error:%s \"%s\"",expr_error(ep->error),ep->errinfo);
		else
			errx(EXIT_FAILURE,"expression error:%s",expr_error(ep->error));
	}
	if(dump)
		list(ep,es);
	else {
		if((r0=setjmp(jb))){
			warnx("expression destructed:%d",r0);
			return EXIT_SUCCESS;
		}
		do {
			switch(mode){
				case NORMAL:
					r=expr_eval(ep,0);
					break;
				case STEP:
					r=stepmode(ep,0);
					break;
				case CALLBACK:
					r=callbackmode(ep,0);
					break;
			}
		}while(--count);
		if(show_result!=0.0)
			printdouble(r);
	}
	expr_free(ep);
	return EXIT_SUCCESS;
}
