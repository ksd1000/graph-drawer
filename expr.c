/*******************************************************************************
 *License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>*
 *This is free software: you are free to change and redistribute it.           *
 *******************************************************************************/
#define _GNU_SOURCE
#include "expr.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <limits.h>
#include <err.h>
#include <errno.h>
#include <setjmp.h>
#define NDEBUG
#include <assert.h>
#define printval(x) fprintf(stderr,#x ":%lu\n",x)
#define printvall(x) fprintf(stderr,#x ":%ld\n",x)
#define printvald(x) fprintf(stderr,#x ":%lf\n",x)
#define SYMDIM(sp) (*(size_t *)((sp)->str+(sp)->strlen+1))
#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif
#define cast(X,T) expr_cast(X,T)
#define cknp(ep,v,act) \
({\
	if(!(v)){\
		ep->error=EXPR_EMEM;\
		act;\
	}\
})
#define max(a,b) ({\
	__auto_type _a=(a);\
	__auto_type _b=(b);\
	_a>_b?_a:_b;\
})
#define maxc(a,const_expr) ({\
	__auto_type _a=(a);\
	_a>(const_expr)?_a:(const_expr);\
})
#define min(a,b) ({\
	__auto_type _a=(a);\
	__auto_type _b=(b);\
	_a<_b?_a:_b;\
})
#define minc(a,const_expr) ({\
	__auto_type _a=(a);\
	_a<(const_expr)?_a:(const_expr);\
})
#define mincc(c1,c2) ((c1)<(c2)?(c1):(c2))
#define SUMCASES EXPR_SUM:\
		case EXPR_INT:\
		case EXPR_PROD:\
		case EXPR_SUP:\
		case EXPR_INF:\
		case EXPR_ANDN:\
		case EXPR_ORN:\
		case EXPR_XORN:\
		case EXPR_GCDN:\
		case EXPR_LCMN:\
		case EXPR_FOR:\
		case EXPR_LOOP
#define MDCASES EXPR_MD:\
		case EXPR_ME:\
		case EXPR_MEP
#define SRCCASES EXPR_COPY:\
		case EXPR_ADD:\
		case EXPR_SUB:\
		case EXPR_MUL:\
		case EXPR_DIV:\
		case EXPR_MOD:\
		case EXPR_POW:\
		case EXPR_AND:\
		case EXPR_OR:\
		case EXPR_XOR:\
		case EXPR_SHL:\
		case EXPR_SHR:\
		case EXPR_GT:\
		case EXPR_GE:\
		case EXPR_LT:\
		case EXPR_LE:\
		case EXPR_SGE:\
		case EXPR_SLE:\
		case EXPR_SEQ:\
		case EXPR_SNE:\
		case EXPR_EQ:\
		case EXPR_NE:\
		case EXPR_ANDL:\
		case EXPR_ORL:\
		case EXPR_XORL:\
		case EXPR_NEXT:\
		case EXPR_DIFF:\
		case EXPR_OFF
#define BRANCHCASES EXPR_IF:\
		case EXPR_DON:\
		case EXPR_DOW:\
		case EXPR_WHILE
#define HOTCASES EXPR_DO:\
		case EXPR_HOT:\
		case EXPR_EP:\
		case EXPR_WIF
#define expr_equal(_a,_b) ({\
	double a,b,absa,absb,absamb;\
	int _r;\
	a=(_a);\
	b=(_b);\
	absa=a<0.0?-a:a;\
	absb=b<0.0?-b:b;\
	absamb=a<b?b-a:a-b;\
	if(absa>absb){\
		if(absa<=1.0)\
			_r=(absamb<=DBL_EPSILON);\
		else\
			_r=(absamb<=DBL_EPSILON*absa);\
	}else {\
		if(absb<=1.0)\
			_r=(absamb<=DBL_EPSILON);\
		else\
			_r=(absamb<=DBL_EPSILON*absb);\
	}\
	_r;\
})
#define expr_space(c) ({\
	int _r;\
	switch(c){\
		case '\t':\
		case '\r':\
		case '\v':\
		case '\f':\
		case '\n':\
		case '\b':\
		case ' ':\
			_r=1;\
			break;\
		default:\
			_r=0;\
			break;\
	}\
	_r;\
})
#define expr_operator(c) ({\
	int _r;\
	switch(c){\
		case '+':\
		case '-':\
		case '*':\
		case '/':\
		case '%':\
		case '^':\
		case '(':\
		case ')':\
		case ',':\
		case ';':\
		case '<':\
		case '>':\
		case '=':\
		case '!':\
		case '&':\
		case '|':\
		case '#':\
		case '[':\
		case ']':\
		case '{':\
		case '}':\
			_r=1;\
			break;\
		default:\
			_r=0;\
			break;\
	}\
	_r;\
})
#define LOGIC(a,b,_s) (((a)!=0.0) _s ((b)!=0.0))
#define LOGIC_BIT(_a,_b,_op_cal,_op_zero,_zero_val) \
	if(_expdiff>52L){\
		_r=EXPR_EDSIGN(&_a) _op_zero EXPR_EDSIGN(&_b)?-( _zero_val):(_zero_val);\
	}else {\
		_x2=(EXPR_EDBASE(&_b)|(1UL<<52UL))>>_expdiff;\
		_x1=EXPR_EDBASE(&_a)|(1UL<<52UL);\
		_x1 _op_cal _x2;\
		if(_x1){\
			_x2=63UL-__builtin_clzl(_x1);\
			_x1&=~(1UL<<_x2);\
			_x2=52UL-_x2;\
			if(EXPR_EDEXP(&_a)<_x2){\
				_r=EXPR_EDSIGN(&_a) _op_zero EXPR_EDSIGN(&_b)?-( _zero_val):(_zero_val);\
			}else {\
				EXPR_EDBASE(&_a)=_x1<<_x2;\
				EXPR_EDEXP(&_a)-=_x2;\
				EXPR_EDSIGN(&_a) _op_cal EXPR_EDSIGN(&_b);\
				_r=_a;\
			}\
		}else {\
			_r=EXPR_EDSIGN(&_a)?-0.0:0.0;\
			EXPR_EDSIGN(&_r) _op_cal EXPR_EDSIGN(&_b);\
		}\
	}
#define and2(__a,__b) ({\
	uint64_t _x2,_x1;\
	int64_t _expdiff;\
	double _a,_b,_r;\
	_a=(__a);\
	_b=(__b);\
	_expdiff=EXPR_EDEXP(&_a)-EXPR_EDEXP(&_b);\
	if(_expdiff<0L){\
		_expdiff=-_expdiff;\
		LOGIC_BIT(_b,_a,&=,&&,0.0)\
	}else {\
		LOGIC_BIT(_a,_b,&=,&&,0.0)\
	}\
	_r;\
})
#define or2(__a,__b) ({\
	uint64_t _x2,_x1;\
	int64_t _expdiff;\
	double _a,_b,_r;\
	_a=(__a);\
	_b=(__b);\
	_expdiff=EXPR_EDEXP(&_a)-EXPR_EDEXP(&_b);\
	if(_expdiff<0L){\
		_expdiff=-_expdiff;\
		LOGIC_BIT(_b,_a,|=,||,_b>=0.0?_b:-_b)\
	}else {\
		LOGIC_BIT(_a,_b,|=,||,_a>=0.0?_a:-_a)\
	}\
	_r;\
})
#define xor2(__a,__b) ({\
	uint64_t _x2,_x1;\
	int64_t _expdiff;\
	double _a,_b,_r;\
	_a=(__a);\
	_b=(__b);\
	_expdiff=EXPR_EDEXP(&_a)-EXPR_EDEXP(&_b);\
	if(_expdiff<0L){\
		_expdiff=-_expdiff;\
		LOGIC_BIT(_b,_a,^=,^,_b>=0.0?_b:-_b)\
	}else {\
		LOGIC_BIT(_a,_b,^=,^,_a>=0.0?_a:-_a)\
	}\
	_r;\
})
#define not(_x) xor2(9007199254740991.0/* 2^53-1*/,(_x))

struct expr_jmpbuf {
	struct expr_inst **ipp;
	struct expr_inst *ip;
	jmp_buf jb;
};
static const char *eerror[]={
	[0]="Unknown error",
	[EXPR_ESYMBOL]="Unknown symbol",
	[EXPR_EPT]="Parentheses do not match",
	[EXPR_EFP]="Function and keyword must be followed by a \'(\'",
	[EXPR_ENVP]="No value in parenthesis",
	[EXPR_ENEA]="No enough or too much argument",
	[EXPR_ENUMBER]="Bad number",
	[EXPR_ETNV]="Target is not variable",
	[EXPR_EEV]="Empty value",
	[EXPR_EUO]="Unexpected operator",
	[EXPR_EZAFP]="Zero-argument function must be followed by \'()\'",
	[EXPR_EDS]="Defined symbol",
	[EXPR_EVMD]="Not a multi-demension function with dim 0",
	[EXPR_EMEM]="Cannot allocate memory",
	[EXPR_EUSN]="Unexpected symbol or number",
	[EXPR_ENC]="Not a constant expression",
	[EXPR_ECTA]="Cannot take address",
	[EXPR_ESAF]="Static assertion failed"
};
//const static char ntoc[]={"0123456789abcdefg"};
const char *expr_error(int error){
	switch(error){
		default:
			error=0;
		case 0 ... sizeof(eerror)/sizeof(*eerror)-1:
			return eerror[error];
	}
}
//#define MEMORY_LEAK_CHECK
#ifdef MEMORY_LEAK_CHECK
static volatile _Atomic int count=0;
static void __attribute__((destructor)) show(void){
	warnx("MEMORY LEAK COUNT:%d",count);
}
#endif
static void *xmalloc_nullable(size_t size){
#ifdef MEMORY_LEAK_CHECK
	void *r=malloc(size);
	if(!r)return NULL;
	++count;
	return r;
#else
	return malloc(size);
#endif
}
static void xfree(void *p){
	if(!p)abort();
	free(p);
#ifdef MEMORY_LEAK_CHECK
	--count;
#endif
}
static void *xrealloc_nullable(void *old,size_t size){
#ifdef MEMORY_LEAK_CHECK
	void *r=old?realloc(old,size):malloc(size);
	if(!r)return NULL;
	if(!old)++count;
	return r;
#else
	return realloc(old,size);
#endif
}
static int xasprintf_nullable(char **restrict strp, const char *restrict fmt,...){
	va_list ap;
	int r;
	va_start(ap,fmt);
	r=vasprintf(strp,fmt,ap);
#ifdef MEMORY_LEAK_CHECK
	++count;
#endif
	va_end(ap);
	return r;
}
static void *xmalloc(size_t size){
	void *r;
	if(size>=SSIZE_MAX){
		warnx("IN xmalloc(size=%zu)\n"
			"CANNOT ALLOCATE MEMORY",size);
		goto ab;
	}
	r=malloc(size);
	if(!r){
		warn("IN xmalloc(size=%zu)\n"
			"CANNOT ALLOCATE MEMORY",size);
ab:
		warnx("ABORTING");
		abort();
	}
#ifdef MEMORY_LEAK_CHECK
	++count;
#endif
	return r;
}
static void *xrealloc(void *old,size_t size){
	void *r;
	if(size>=SSIZE_MAX){
		warnx("IN xrealloc(old=%p,size=%zu)\n"
			"CANNOT REALLOCATE MEMORY",old,size);
		goto ab;
	}
	r=old?realloc(old,size):malloc(size);
	if(!r){
		warn("IN xrealloc(old=%p,size=%zu)\n"
			"CANNOT REALLOCATE MEMORY",old,size);
ab:
		warnx("ABORTING");
		abort();
	}
#ifdef MEMORY_LEAK_CHECK
	if(!old)++count;
#endif
	return r;
}
/*static int xasprintf(char **restrict strp, const char *restrict fmt,...){
	int r;
	va_list ap;
	va_start(ap,fmt);
	r=vasprintf(strp,fmt,ap);
	if(r<0){
		warn("IN xasprintf(strp=%p)\n"
			"CANNOT ALLOCATE MEMORY",strp);
		warnx("ABORTING");
		abort();
	}
	va_end(ap);
#ifdef MEMORY_LEAK_CHECK
	++count;
#endif
	return r;
}*/
uint64_t expr_gcd64(uint64_t x,uint64_t y){
	uint64_t r;
	int r1;
	r=__builtin_ctzl(x);
	r1=__builtin_ctzl(y);
	r=r<r1?r:r1;
	x>>=r;
	y>>=r;
	r1=(x<y);
	while(x&&y){
		if(r1^=1)x%=y;
		else y%=x;
	}
	return (x|y)<<r;
}
#define gcd2(__x,__y) ({\
	double _x,_y;\
	int r1;\
	_x=(__x);\
	_y=(__y);\
	r1=(fabs(_x)<fabs(_y));\
	while(_x!=0.0&&_y!=0.0){\
		if(r1^=1)_x=fmod(_x,_y);\
		else _y=fmod(_y,_x);\
	}\
	_x!=0.0?_x:_y;\
})
#define lcm2(__x,__y) ({\
	double _a,_b;\
	_a=(__x);\
	_b=(__y);\
	_a*_b/gcd2(_a,_b);\
})
double expr_gcd2(double x,double y){
	return gcd2(x,y);
}
double expr_lcm2(double x,double y){
	return lcm2(x,y);
}
void expr_mirror(double *buf,size_t size){
	double *out=buf+size-1,swapbuf;
	while(out>buf){
		swapbuf=*out;
		*out=*buf;
		*buf=swapbuf;
		--out;
		++buf;
	}
}
void expr_fry(double *v,size_t n){
	size_t r;
	double swapbuf;
	double *endp;
	switch(n){
		case 0:
		case 1:
			return;
		default:
			r=n>>1ul;
			expr_fry(v,r);
			expr_fry(v+r,r);
			break;
	}
	r=((size_t)v+(size_t)__builtin_frame_address(0))&511;
	for(endp=v+n-1;v<endp;++v,--endp){
		r=(r*121+37)&1023;
		if(__builtin_parity(r)){
			swapbuf=*v;
			*v=*endp;
			*endp=swapbuf;
		}
	}
}

__attribute__((noinline))
void *expr_sort3(double *restrict v,size_t n,void *(*allocator)(size_t)){
	struct dnode {
		struct dnode *lt,*gt;
		size_t eq;
		double val;
	} *top,*dnp,*d0,**sp;
	double *restrict p;
	double *endp=v+n;
	size_t depth,dep;
	if(allocator){
		dnp=allocator(n*sizeof(struct dnode));
		if(!dnp)return NULL;
		expr_fry(v,n);
	}else {
		expr_fry(v,n);
		dnp=alloca(n*sizeof(struct dnode));
	}
	top=dnp;
	p=v;
	depth=1;
	dep=1;
	goto create;
	for(;p<endp;++p){
		dep=1;
		for(struct dnode *p1=dnp;;){
			if(*p==p1->val){
				++p1->eq;
				break;
			}else if((*p>p1->val)){
				if(p1->gt){
					p1=p1->gt;
					++dep;
					continue;
				}
				p1->gt=++top;
				++dep;
				goto create;
			}else {
				if(p1->lt){
					p1=p1->lt;
					++dep;
					continue;
				}
				p1->lt=++top;
				++dep;
				goto create;
			}
		}
		continue;
create:
		top->lt=NULL;
		top->gt=NULL;
		top->eq=1;
		top->val=*p;
		if(dep>depth)depth=dep;
	}
	sp=alloca(depth*sizeof(struct dnode *));
	*(sp++)=dnp;
	d0=dnp;
	for(top=dnp;;){
		if(top==dnp->lt){
no_lt:
			do
				*(v++)=dnp->val;
			while(--dnp->eq);
			if(v==endp)break;
			if(!dnp->gt)goto no_gt;
			top=dnp;
			*(sp++)=dnp;
			dnp=dnp->gt;
			continue;
		}else if(top==dnp->gt){
no_gt:
			top=dnp;
			dnp=*(--sp);
			continue;
		}else {
			if(!dnp->lt)goto no_lt;
			top=dnp;
			*(sp++)=dnp;
			dnp=dnp->lt;
			continue;
		}
	}
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-local-addr"
	return d0;//return for free
}
#pragma GCC diagnostic pop
void expr_sort_old(double *restrict v,size_t n){
	double swapbuf;
	for(size_t i=0;i<n;++i){
		for(size_t j=i+1;j<n;++j){
			if(v[j]<v[i]){
				swapbuf=v[i];
				v[i]=v[j];
				v[j]=swapbuf;
			}
		}
	}
}
void expr_sort(double *v,size_t n){
	switch(n){
		case 0:
			return;
		case 1 ... 14:
			expr_sort_old(v,n);
			return;
		default:
			expr_sort3(v,n,NULL);
			return;
	}
}

double expr_and2(double x,double y){
	return and2(x,y);
}
double expr_or2(double x,double y){
	return or2(x,y);
}
double expr_xor2(double x,double y){
	return xor2(x,y);
}
double expr_not(double x){
	return not(x);
}
#define expr_add2(a,b) ((a)+(b))
#define expr_mul2(a,b) ((a)*(b))
#define CALMD(_symbol)\
	double ret=*(args++);\
	while(--n){\
		ret= _symbol (ret,*args);\
		++args;\
	}\
	return ret
static double expr_and(size_t n,double *args){
	CALMD(and2);
}
static double expr_or(size_t n,double *args){
	CALMD(or2);
}
static double expr_xor(size_t n,double *args){
	CALMD(xor2);
}
static double expr_gcd(size_t n,double *args){
	CALMD(gcd2);
}
static double expr_lcm(size_t n,double *args){
	CALMD(lcm2);
}
static double expr_add(size_t n,double *args){
	CALMD(expr_add2);
}
static double expr_mul(size_t n,double *args){
	CALMD(expr_mul2);
}
static double expr_cmp(size_t n,double *args){
	return (double)memcmp(args,args+1,sizeof(double));
}
void expr_contract(void *buf,size_t size){
	char *p=(char *)buf,*endp=(char *)buf+size-1;
	while(p<=endp){
		*p=0;
		p+=PAGE_SIZE;
	}
	if(p!=endp)
		*endp=0;
}
__attribute__((noreturn)) void expr_explode(void){
	void *r;
	size_t sz=((size_t)SSIZE_MAX+1)/2;
	do {
		while((r=xmalloc_nullable(sz))){
			expr_contract(r,sz);
		}
		sz>>=1ul;
	}while(sz);
	abort();
}
static double expr_lrand48(void){
	return (double)lrand48();
}
static double expr_mrand48(void){
	return (double)mrand48();
}
static double expr_med(size_t n,double *args){
	expr_sort(args,n);
	return n&1ul?args[n>>1ul]:(n>>=1ul,(args[n]+args[n-1])/2);
}
static double expr_hmed(size_t n,double *args){
	xfree(expr_sort3(args,n,xmalloc));
	return n&1ul?args[n>>1ul]:(n>>=1ul,(args[n]+args[n-1])/2);
}
static double expr_med_old(size_t n,double *args){
	expr_sort_old(args,n);
	return n&1ul?args[n>>1ul]:(n>>=1ul,(args[n]+args[n-1])/2);
}
static double expr_gmed(size_t n,double *args){
	expr_sort(args,n);
	return n&1ul?args[n>>1ul]:(n>>=1ul,sqrt(args[n]*args[n-1]));
}
static double expr_hgmed(size_t n,double *args){
	xfree(expr_sort3(args,n,xmalloc));
	return n&1ul?args[n>>1ul]:(n>>=1ul,sqrt(args[n]*args[n-1]));
}
static double expr_gmed_old(size_t n,double *args){
	expr_sort_old(args,n);
	return n&1ul?args[n>>1ul]:(n>>=1ul,sqrt(args[n]*args[n-1]));
}
static double expr_mode0(size_t n,double *args,int heap){
	double max,cnt;
	double *endp=args+n;
	size_t maxn=1;
	if(heap)
		xfree(expr_sort3(args,n,xmalloc));
	else
		expr_sort(args,n);
	cnt=max=*(args++);
	for(size_t cn=1;;++args){
		if(*args==cnt){
			++cn;
			continue;
		}
		if(cn>maxn){
			maxn=cn;
			max=cnt;
		}
		cn=1;
		cnt=*args;
		if(args==endp)break;
	}
	return max;
}
static double expr_mode(size_t n,double *args){
	return expr_mode0(n,args,0);
}
static double expr_hmode(size_t n,double *args){
	return expr_mode0(n,args,1);
}
static double expr_sign(double x){
	if(x>0.0)return 1.0;
	else if(x<-0.0)return -1.0;
	else return 0.0;
}
struct s_eb {
	uint64_t eb:63;
	uint64_t sign:1;
};
static double expr_bfry(size_t n,const struct expr *args,double input){
	expr_fry(cast(expr_eval(args,input),double *),(size_t)fabs(expr_eval(args+1,input)));
	return 0.0;
}
static double expr_bmirror(size_t n,const struct expr *args,double input){
	expr_mirror(cast(expr_eval(args,input),double *),(size_t)fabs(expr_eval(args+1,input)));
	return 0.0;
}
static double expr_bsort(size_t n,const struct expr *args,double input){
	expr_sort(cast(expr_eval(args,input),double *),(size_t)fabs(expr_eval(args+1,input)));
	return 0.0;
}
static double expr_bhsort(size_t n,const struct expr *args,double input){
	void *r;
	r=expr_sort3(cast(expr_eval(args,input),double *),(size_t)fabs(expr_eval(args+1,input)),xmalloc_nullable);
	if(r){
		xfree(r);
		return 0.0;
	}else {
		return -1.0;
	}
}
static double expr_bxsort(size_t n,const struct expr *args,double input){
	xfree(expr_sort3(cast(expr_eval(args,input),double *),(size_t)fabs(expr_eval(args+1,input)),xmalloc));
	return 0.0;
}
static double expr_bsort_old(size_t n,const struct expr *args,double input){
	expr_sort_old(cast(expr_eval(args,input),double *),(size_t)fabs(expr_eval(args+1,input)));
	return 0.0;
}
static double expr_bcontract(size_t n,const struct expr *args,double input){
	expr_contract(cast(expr_eval(args,input),void *),(size_t)fabs(expr_eval(args+1,input)));
	return 0.0;
}
static double expr_bassert(size_t n,const struct expr *args,double input){
	double x=expr_eval(args,input);
	if(x==0.0){
		const char *p,*e;
		e=args->ip->un.em->e;
		p=strchr(e,'(');
		if(p){
		}else p=e;
		warnx("assertion %s failed.",p);
		warnx("ABORTING");
		abort();
	}
	return x;
}
static double expr_ldr(size_t n,const struct expr *args,double input){
	union {
		double *r;
		double dr;
	} un;
	un.dr=expr_eval(args,input);
	return un.r[(size_t)expr_eval(++args,input)];
}
static double expr_str(size_t n,const struct expr *args,double input){
	union {
		double *r;
		double dr;
	} un;
	un.dr=expr_eval(args,input);
	un.r+=(size_t)expr_eval(++args,input);
	return *un.r=expr_eval(++args,input);
}
static double expr_memset(size_t n,const struct expr *args,double input){
	union {
		double *r;
		double dr;
	} un;
	double *endp;
	double val;
	un.dr=expr_eval(args,input);
	val=expr_eval(++args,input);
	endp=un.r+(size_t)expr_eval(++args,input);
	while(un.r<endp){
		*un.r=val;
		++un.r;
	}
	return val;
}
static double expr_bzero(size_t n,const struct expr *args,double input){
	union {
		double *r;
		double dr;
	} un;
	un.dr=expr_eval(args,input);
	memset(un.r,0,(size_t)expr_eval(++args,input));
	return 0.0;
}
double expr_isfinite(double x){
	return EXPR_EDEXP(&x)!=2047?1.0:0.0;
}
double expr_isinf(double x){
	return !EXPR_EDBASE(&x)&&EXPR_EDEXP(&x)==2047?1.0:0.0;
}
double expr_isnan(double x){
	return EXPR_EDBASE(&x)&&EXPR_EDEXP(&x)==2047?1.0:0.0;
}
static double expr_malloc(double x){
	union {
		double *r;
		double dr;
	} un;
	un.r=xmalloc_nullable((size_t)fabs(x));
	return un.dr;
}
static double expr_xmalloc(double x){
	union {
		double *r;
		double dr;
	} un;
	un.r=xmalloc((size_t)fabs(x));
	return un.dr;
}
static double expr_new(double x){
	union {
		double *r;
		double dr;
	} un;
	un.r=xmalloc_nullable((size_t)fabs(x)*sizeof(double));
	return un.dr;
}
static double expr_xnew(double x){
	union {
		double *r;
		double dr;
	} un;
	un.r=xmalloc((size_t)fabs(x)*sizeof(double));
	return un.dr;
}
static double expr_xfree(double x){
	union {
		void *r;
		double dr;
	} un;
	un.dr=x;
	xfree(un.r);
	return 0.0;
}
static double expr_errno(void){
	return (double)errno;
}
static double expr_serrno(double x){
	errno=(__typeof(errno))x;
	return x;
}
#define RMEM(sym,type)\
static double expr_r##sym(double x){\
	union {\
		type *r;\
		double dr;\
	} un;\
	un.dr=x;\
	return (double)*un.r;\
}
#define ZMEM(sym,type)\
static double expr_z##sym(double x){\
	union {\
		type *r;\
		double dr;\
	} un;\
	un.dr=x;\
	*un.r=(type)0;\
	return 0.0;\
}
#define WMEM(sym,type)\
static double expr_w##sym(size_t n,const struct expr *args,double input){\
	union {\
		type *r;\
		double dr;\
	} un;\
	double v;\
	un.dr=expr_eval(args,input);\
	v=expr_eval(args+1,input);\
	*un.r=(type)v;\
	return v;\
}
RMEM(8,int8_t)
RMEM(16,int16_t)
RMEM(32,int32_t)
RMEM(64,int64_t)
RMEM(m,intmax_t)
RMEM(p,intptr_t)
RMEM(z,ssize_t)
RMEM(8u,uint8_t)
RMEM(16u,uint16_t)
RMEM(32u,uint32_t)
RMEM(64u,uint64_t)
RMEM(mu,uintmax_t)
RMEM(pu,uintptr_t)
RMEM(zu,size_t)
RMEM(f,float)
RMEM(l,long double)
ZMEM(8,int8_t)
ZMEM(16,int16_t)
ZMEM(32,int32_t)
ZMEM(64,int64_t)
ZMEM(m,intmax_t)
ZMEM(p,intptr_t)
ZMEM(z,ssize_t)
ZMEM(8u,uint8_t)
ZMEM(16u,uint16_t)
ZMEM(32u,uint32_t)
ZMEM(64u,uint64_t)
ZMEM(mu,uintmax_t)
ZMEM(pu,uintptr_t)
ZMEM(zu,size_t)
ZMEM(f,float)
ZMEM(l,long double)
WMEM(8,int8_t)
WMEM(16,int16_t)
WMEM(32,int32_t)
WMEM(64,int64_t)
WMEM(m,intmax_t)
WMEM(p,intptr_t)
WMEM(z,ssize_t)
WMEM(8u,uint8_t)
WMEM(16u,uint16_t)
WMEM(32u,uint32_t)
WMEM(64u,uint64_t)
WMEM(mu,uintmax_t)
WMEM(pu,uintptr_t)
WMEM(zu,size_t)
WMEM(f,float)
WMEM(l,long double)
#define REGRMEM(s) REGFSYM2_NI("r" #s,expr_r##s)
#define REGZMEM(s) REGFSYM2_NI("z" #s,expr_z##s)
#define REGWMEM(s) REGMDEPSYM2_NI("w" #s,expr_w##s,2ul)
static double expr_dexp(double x){
	return (double)EXPR_EDEXP(&x);
}
static double expr_dbase(double x){
	return (double)EXPR_EDBASE(&x);
}
static double expr_frame(void){
	union {
		void *r;
		double d;
	} un;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wframe-address"
	un.r=__builtin_frame_address(1);
#pragma GCC diagnostic pop
	return un.d;
}
static double expr_asdouble(double x){
	union {
		int64_t d;
		uint64_t u;
		double v;
	} un;
	if(x<0.0)un.d=(int64_t)x;
	else un.u=(uint64_t)x;
	return un.v;
}
static double expr_asint(double x){
	union {
		int64_t d;
		double v;
	} un;
	un.v=x;
	return (double)un.d;
}
static double expr_asuint(double x){
	union {
		uint64_t u;
		double v;
	} un;
	un.v=x;
	return (double)un.u;
}
static double expr_popcount(double x){
	return (double)__builtin_popcountl(EXPR_EDIVAL(&x));
}
static double expr_popcountb(double x){
	return (double)__builtin_popcountl(EXPR_EDBASE(&x));
}
static double expr_popcounte(double x){
	return (double)__builtin_popcountl(EXPR_EDEXP(&x));
}
static double expr_system(double x){
	return (double)system(cast(x,const char *));
}
static double expr_exit(double x){
	exit((int)x);
}
static double expr_exitif(double x){
	if(x!=0.0)exit((int)x);
	return x;
}
static double expr_fact(double x){
	double sum=1.0;
	x=floor(x);
	while(x>0.0){
		sum*=x;
		x-=1.0;
	}
	return sum;
}
static double expr_dfact(double x){
	double sum=1.0;
	x=floor(x);
	while(x>0.0){
		sum*=x;
		x-=2.0;
	}
	return sum;
}
static double expr_nfact(size_t n,double *args){
	double sum=1.0,x=args[0];
	x=floor(x);
	while(x>0.0){
		sum*=x;
		x-=args[1];
	}
	return sum;
}
static double expr_piece(size_t n,const struct expr *args,double input){
	const struct expr *arg0=args;
	--n;
	while(args-arg0<n){
		if(expr_eval(args++,input)!=0.0){
			return expr_eval(args,input);
		}else {
			++args;
		}

	}
	return expr_eval(arg0+n,input);
}
static double expr_derivate(size_t n,const struct expr *args,double input){
	double epsilon=(n>=2?expr_eval(args+1,input):FLT_EPSILON);
	return (expr_eval(args,input+epsilon)-
		expr_eval(args,input-epsilon))/epsilon/2;
}
double expr_multilevel_derivate(const struct expr *ep,double input,long level,double epsilon){
	if(level<1l)
		return expr_eval(ep,input);
	else return (expr_multilevel_derivate(
		ep,input+epsilon,level-1,epsilon
		)-expr_multilevel_derivate(
			ep,input-epsilon,level-1,epsilon
			))/epsilon/2;
}
static double expr_multi_derivate(size_t n,const struct expr *args,double input){
	double epsilon=(n>=3?expr_eval(args+2,input):FLT_EPSILON);
	double level=(n>=2?expr_eval(args+1,input):1.0);
	return expr_multilevel_derivate(args,input,(long)level,epsilon);
}
static double expr_root(size_t n,const struct expr *args,double input){
	//root(expression)
	//root(expression,from)
	//root(expression,from,to)
	//root(expression,from,to,step)
	//root(expression,from,to,step,epsilon)
	double epsilon=FLT_EPSILON,from=0.0,to=INFINITY,step=FLT_EPSILON,swapbuf;
	switch(n){
		case 5:
			epsilon=fabs(expr_eval(args+4,input));
		case 4:
			step=fabs(expr_eval(args+3,input));
		case 3:
			to=expr_eval(args+2,input);
		case 2:
			from=expr_eval(args+1,input);
		case 1:
			break;
		default:
			return NAN;
	}
	if(from>to){
		swapbuf=from;
		from=to;
		to=swapbuf;
	}
	for(;from<=to;from+=step){
		if(fabs(expr_eval(args,from))<=epsilon)
			return from;
	}
	return INFINITY;
}
static double expr_root2(size_t n,const struct expr *args,double input){
	//root2(expression)
	//root2(expression,from)
	//root2(expression,from,to)
	//root2(expression,from,to,step)
	//root2(expression,from,to,step,epsilon)
	double epsilon=0.0,from=0.0,to=INFINITY,step=1.0,swapbuf;
	int neg,trunc=0;
	switch(n){
		case 5:
			epsilon=fabs(expr_eval(args+4,input));
		case 4:
			step=fabs(expr_eval(args+3,input));
		case 3:
			to=expr_eval(args+2,input);
		case 2:
			from=expr_eval(args+1,input);
		case 1:
			break;
		default:
			return NAN;
	}
	if(from>to){
		swapbuf=from;
		from=to;
		to=swapbuf;
		trunc=1;
	}
	swapbuf=expr_eval(args,from);
	neg=(swapbuf<-0.0);
	if(fabs(swapbuf)<=epsilon)goto end;
	for(from+=step;from<=to;from+=step){
		if(from+step==from){
			goto end1;
		}
		if(fabs(swapbuf=expr_eval(args,from))<=epsilon)goto end;
		if((swapbuf<-0.0)==neg)continue;
		from-=step;
		if(step<=epsilon)goto end;
		do {
			step/=2.0;
		}while((expr_eval(args,from+step)<0.0)!=neg);
		from+=step;
	}
	return INFINITY;
end1:
	if(!trunc)swapbuf=expr_eval(args,from);
end:
	if(!trunc&&fabs(swapbuf)>epsilon)
		return from+2.0*step;
	return from;
}
static double expr_rooti(size_t n,const struct expr *args,double input){
	//root2(expression)
	//root2(expression,from)
	//root2(expression,from,epsilon)
	//root2(expression,from,epsilon,depsilon)
	double epsilon=DBL_EPSILON,depsilon=FLT_EPSILON,from=1.0,swapbuf;
	switch(n){
		case 4:
			depsilon=fabs(expr_eval(args+3,input));
		case 3:
			epsilon=fabs(expr_eval(args+2,input));
		case 2:
			from=expr_eval(args+1,input);
		case 1:
			break;
		default:
			return NAN;
	}
	for(;;){
		swapbuf=expr_multilevel_derivate(args,from,1,depsilon);
		if(swapbuf==0.0){
			//if(fabs(expr_eval(args,from))<=epsilon)
			return from;
			//break;
		}
		swapbuf=from-expr_eval(args,from)/swapbuf;
		if(fabs(from-swapbuf)<=epsilon)return swapbuf;
		from=swapbuf;
	}
	return INFINITY;
}
static double expr_andl(size_t n,const struct expr *args,double input){
	for(const struct expr *ep=args;ep-args<n;++ep){
		if(expr_eval(ep,input)==0.0)
			return 0.0;
	}
	return 1.0;
}
static double expr_orl(size_t n,const struct expr *args,double input){
	for(const struct expr *ep=args;ep-args<n;++ep){
		if(expr_eval(ep,input)!=0.0)
			return 1.0;
	}
	return 0.0;
}
static double expr_max(size_t n,double *args){
	double ret=DBL_MIN;
	while(n>0){
		if(*args>ret)ret=*args;
		--n;
		++args;
	}
	return ret;
}
static double expr_min(size_t n,double *args){
	double ret=DBL_MAX;
	while(n>0){
		if(*args<ret)ret=*args;
		--n;
		++args;
	}
	return ret;
}

static double expr_hypot(size_t n,double *args){
	double ret=0;
	while(n>0){
		ret+=*args**args;
		--n;
		++args;
	}
	return sqrt(ret);
}
//regs
//#define REGSYM(s) {#s,s}
#define REGZASYM(s) {.strlen=sizeof(#s)-1,.str=#s,.un={.zafunc=s},.type=EXPR_ZAFUNCTION,.flag=0}
#define REGFSYM(s) {.strlen=sizeof(#s)-1,.str=#s,.un={.func=s},.type=EXPR_FUNCTION,.flag=EXPR_SF_INJECTION}
#define REGCSYM(s) {.strlen=sizeof(#s)-1,.str=#s,.un={.value=s},.type=EXPR_CONSTANT}
#define REGFSYM2(s,sym) {.strlen=sizeof(s)-1,.str=s,.un={.func=sym},.type=EXPR_FUNCTION,.flag=EXPR_SF_INJECTION}
#define REGFSYM2_NI(s,sym) {.strlen=sizeof(s)-1,.str=s,.un={.func=sym},.type=EXPR_FUNCTION,.flag=0}
#define REGZASYM2(s,sym) {.strlen=sizeof(s)-1,.str=s,.un={.zafunc=sym},.type=EXPR_ZAFUNCTION,.flag=0}
#define REGMDSYM2(s,sym,d) {.strlen=sizeof(s)-1,.str=s,.un={.mdfunc=sym},.dim=d,.type=EXPR_MDFUNCTION,.flag=EXPR_SF_INJECTION}
#define REGMDSYM2_NI(s,sym,d) {.strlen=sizeof(s)-1,.str=s,.un={.mdfunc=sym},.dim=d,.type=EXPR_MDFUNCTION,.flag=0}
#define REGMDEPSYM2(s,sym,d) {.strlen=sizeof(s)-1,.str=s,.un={.mdepfunc=sym},.dim=d,.type=EXPR_MDEPFUNCTION,.flag=EXPR_SF_INJECTION}
#define REGMDEPSYM2_NI(s,sym,d) {.strlen=sizeof(s)-1,.str=s,.un={.mdepfunc=sym},.dim=d,.type=EXPR_MDEPFUNCTION,.flag=0}
#define REGMDEPSYM2_NIW(s,sym,d) {.strlen=sizeof(s)-1,.str=s,.un={.mdepfunc=sym},.dim=d,.type=EXPR_MDEPFUNCTION,.flag=EXPR_SF_WRITEIP}
#define REGCSYM2(s,val) {.strlen=sizeof(s)-1,.str=s,.un={.value=val},.type=EXPR_CONSTANT}
#define REGKEY(s,op,dim,desc) {s,op,0,sizeof(s)-1,desc}
#define REGKEYS(s,op,dim,desc) {s,op,EXPR_KF_SUBEXPR,sizeof(s)-1,desc}
#define REGKEYC(s,op,dim,desc) {s,op,EXPR_KF_SEPCOMMA,sizeof(s)-1,desc}
#define REGKEYSC(s,op,dim,desc) {s,op,EXPR_KF_SUBEXPR|EXPR_KF_SEPCOMMA,sizeof(s)-1,desc}
const struct expr_builtin_keyword expr_keywords[]={
	REGKEYSC("sum",EXPR_SUM,5,"sum(index_name,start_index,end_index,index_step,addend)"),
	REGKEYSC("int",EXPR_INT,5,"int(integral_var_name,upper_limit,lower_limit,epsilon,integrand)"),
	REGKEYSC("prod",EXPR_PROD,5,"prod(index_name,start_index,end_index,index_step,factor)"),
	REGKEYSC("pai",EXPR_PROD,5,"pai(index_name,start_index,end_index,index_step,factor)"),
	REGKEYSC("sup",EXPR_SUP,5,"sup(index_name,start_index,end_index,index_step,element)"),
	REGKEYSC("infi",EXPR_INF,5,"infi(index_name,start_index,end_index,index_step,element)"),
	REGKEYSC("andn",EXPR_ANDN,5,"andn(index_name,start_index,end_index,index_step,element)"),
	REGKEYSC("orn",EXPR_ORN,5,"orn(index_name,start_index,end_index,index_step,element)"),
	REGKEYSC("xorn",EXPR_XORN,5,"xorn(index_name,start_index,end_index,index_step,element)"),
	REGKEYSC("gcdn",EXPR_GCDN,5,"gcdn(index_name,start_index,end_index,index_step,element)"),
	REGKEYSC("lcmn",EXPR_LCMN,5,"lcmn(index_name,start_index,end_index,index_step,element)"),
	REGKEYSC("for",EXPR_FOR,5,"for(var_name,start_var,cond,body,value)"),
	REGKEYSC("loop",EXPR_LOOP,5,"loop(var_name,start_var,count,body,value)"),
	REGKEYSC("vmd",EXPR_VMD,7,"vmd(index_name,start_index,end_index,index_step,element,md_symbol,[constant_expression max_dim])"),
	REGKEYS("do",EXPR_DO,1,"do(body) do{body}"),
	REGKEYSC("if",EXPR_IF,3,"if(cond,if_value,else_value) if(cond){body}[[else]{value}]"),
	REGKEYSC("while",EXPR_WHILE,3,"while(cond,body) while(cond){body}"),
	REGKEYSC("dowhile",EXPR_DOW,3,"dowhile(cond,body) dowhile(cond){body}"),
	REGKEYSC("don",EXPR_DON,3,"don(cond,body) don(cond){body}"),
	REGKEYC("const",EXPR_CONST,2,"const(name,[constant_expression value])"),
	REGKEYC("var",EXPR_MUL,2,"var(name,[constant_expression initial_value])"),
	REGKEY("double",EXPR_BL,1,"double(constant_expression count)"),
	REGKEY("byte",EXPR_COPY,1,"byte(constant_expression count)"),
	REGKEY("jmpbuf",EXPR_INPUT,1,"jmpbuf(constant_expression count)"),
	REGKEYC("alloca",EXPR_ALO,2,"alloca(nmemb,[constant_expression size])"),
	REGKEY("setjmp",EXPR_SJ,1,"setjmp(jmp_buf)"),
	REGKEYC("longjmp",EXPR_LJ,2,"longjmp(jmp_buf,val)"),
	REGKEYC("eval",EXPR_EVAL,2,"eval(ep,[input])"),
	REGKEYC("decl",EXPR_ADD,2,"decl(name,[constant_expression flag])"),
	REGKEY("static_assert",EXPR_SUB,1,"static_assert(constant_expression cond)"),
	{NULL}
};
const struct expr_builtin_symbol expr_symbols[]={
	REGCSYM(DBL_MAX),
	REGCSYM(DBL_MIN),
	REGCSYM(DBL_TRUE_MIN),
	REGCSYM(DBL_EPSILON),
	REGCSYM2("DBL_SHIFT",(double)__builtin_ctzl(sizeof(double))),
	REGCSYM2("DBL_SIZE",(double)sizeof(double)),
	REGCSYM(EXPR_SF_INJECTION),
	REGCSYM(EXPR_SF_WRITEIP),
	REGCSYM(EXPR_SF_PMD),
	REGCSYM(EXPR_SF_PME),
	REGCSYM(EXPR_SF_PEP),
	REGCSYM(FLT_MAX),
	REGCSYM(FLT_MIN),
	REGCSYM(FLT_EPSILON),
	REGCSYM(HUGE_VAL),
	REGCSYM(HUGE_VALF),
	REGCSYM(INFINITY),
	REGCSYM2("JBLEN",(double)sizeof(struct expr_jmpbuf)),
	REGCSYM(NAN),
	REGCSYM2("NULL",(double)(size_t)NULL),
	REGCSYM2("1_pi",M_1_PI),
	REGCSYM2("2_pi",M_2_PI),
	REGCSYM2("2_sqrtpi",M_2_SQRTPI),
	REGCSYM2("C",0.577215664901532860606512090082402431042),
	REGCSYM2("c",299792458.0),
	REGCSYM2("e",M_E),
	REGCSYM2("e0",8.8541878128e-12),
	REGCSYM2("e1",1.602176634e-19),
	REGCSYM2("G",6.67430e-11),
	REGCSYM2("h",6.62607015e-34),
	REGCSYM2("inf",INFINITY),
	REGCSYM2("k",1.380649e-23),
	REGCSYM2("log2e",M_LOG2E),
	REGCSYM2("log10e",M_LOG10E),
	REGCSYM2("ln2",M_LN2),
	REGCSYM2("ln10",M_LN10),
	REGCSYM2("N_A",6.02214076e+23),
	REGCSYM2("nan",NAN),
	REGCSYM2("pi",M_PI),
	REGCSYM2("pi_2",M_PI_2),
	REGCSYM2("pi_4",M_PI_4),
	REGCSYM2("R",8.31446261815234),
	REGCSYM2("sqrt2",M_SQRT2),
	REGCSYM2("sqrt1_2",M_SQRT1_2),
	REGCSYM2("sqrc",89875517873681764.0),
	REGCSYM2("u0",1.25663706212e-6),

	REGFSYM2("abs",fabs),
	REGFSYM(acos),
	REGFSYM(acosh),
	REGFSYM2("arccos",acos),
	REGFSYM2("arcosh",acosh),
	REGFSYM2("asdouble",expr_asdouble),
	REGFSYM2("asint",expr_asint),
	REGFSYM2("asuint",expr_asuint),
	REGFSYM(asin),
	REGFSYM(asinh),
	REGFSYM2("arcsin",asin),
	REGFSYM2("arsinh",asinh),
	REGFSYM(atan),
	REGFSYM(atanh),
	REGFSYM2("arctan",atan),
	REGFSYM2("artanh",atanh),
	REGFSYM(cbrt),
	REGFSYM(ceil),
	REGFSYM(cos),
	REGFSYM(cosh),
	REGFSYM2("dbase",expr_dbase),
	REGFSYM2("dexp",expr_dexp),
	REGFSYM2("dfact",expr_dfact),
	REGFSYM(erf),
	REGFSYM(exp),
	REGFSYM(exp2),
	REGFSYM(expm1),
	REGFSYM(fabs),
	REGFSYM2("fact",expr_fact),
	REGFSYM(floor),
	REGFSYM2("isfinite",expr_isfinite),
	REGFSYM2("isinf",expr_isinf),
	REGFSYM2("isnan",expr_isnan),
	REGFSYM(j0),
	REGFSYM(j1),
	REGFSYM(lgamma),
	REGFSYM(log),
	REGFSYM2("ln",log),
	REGFSYM(log10),
	REGFSYM(log1p),
	REGFSYM(log2),
	REGFSYM(logb),
	REGFSYM(nearbyint),
	REGFSYM2("popcount",expr_popcount),
	REGFSYM2("popcountb",expr_popcountb),
	REGFSYM2("popcounte",expr_popcounte),
	REGFSYM(rint),
	REGFSYM(round),
	REGFSYM2("sign",expr_sign),
	REGFSYM(sin),
	REGFSYM(sinh),
	REGFSYM(sqrt),
	REGFSYM(tan),
	REGFSYM(tanh),
	REGFSYM(tgamma),
	REGFSYM(trunc),
	REGFSYM(y0),
	REGFSYM(y1),

	REGZASYM2("abort",(double (*)(void))abort),
	REGZASYM2("errno",expr_errno),
	REGZASYM(drand48),
	REGZASYM2("frame",expr_frame),
	REGZASYM2("lrand48",expr_lrand48),
	REGZASYM2("mrand48",expr_mrand48),
	REGZASYM2("explode",(double (*)(void))expr_explode),

	REGFSYM2_NI("exit",expr_exit),
	REGFSYM2_NI("exitif",expr_exitif),
	REGFSYM2_NI("malloc",expr_malloc),
	REGFSYM2_NI("xmalloc",expr_xmalloc),
	REGFSYM2_NI("new",expr_new),
	REGFSYM2_NI("xnew",expr_xnew),
	REGFSYM2_NI("free",expr_xfree),
	REGFSYM2_NI("serrno",expr_serrno),
	REGFSYM2_NI("system",expr_system),

	REGMDSYM2("add",expr_add,0),
	REGMDSYM2("and",expr_and,0),
	REGMDSYM2("or",expr_or,0),
	REGMDSYM2("xor",expr_xor,0),
	REGMDSYM2("cmp",expr_cmp,2),
	REGMDSYM2("gcd",expr_gcd,0),
	REGMDSYM2("hgmed",expr_hgmed,0),
	REGMDSYM2("hmed",expr_hmed,0),
	REGMDSYM2("hmode",expr_hmode,0),
	REGMDSYM2("hypot",expr_hypot,0),
	REGMDSYM2("lcm",expr_lcm,0),
	REGMDSYM2("max",expr_max,0),
	REGMDSYM2("med",expr_med,0),
	REGMDSYM2("med_old",expr_med_old,0),
	REGMDSYM2("gmed",expr_gmed,0),
	REGMDSYM2("gmed_old",expr_gmed_old,0),
	REGMDSYM2("min",expr_min,0),
	REGMDSYM2("mode",expr_mode,0),
	REGMDSYM2("mul",expr_mul,0),
	REGMDSYM2("nfact",expr_nfact,2ul),

	REGMDEPSYM2_NIW("assert",expr_bassert,1ul),
	REGMDEPSYM2_NI("ldr",expr_ldr,2ul),
	REGMDEPSYM2_NI("str",expr_str,3ul),
	REGMDEPSYM2_NI("bzero",expr_bzero,2ul),
	REGMDEPSYM2_NI("contract",expr_bcontract,2ul),
	REGMDEPSYM2_NI("fry",expr_bfry,2ul),
	REGMDEPSYM2_NI("memset",expr_memset,3ul),
	REGMDEPSYM2_NI("mirror",expr_bmirror,2ul),
	REGMDEPSYM2_NI("sort",expr_bsort,2ul),
	REGMDEPSYM2_NI("hsort",expr_bhsort,2ul),
	REGMDEPSYM2_NI("xsort",expr_bxsort,2ul),
	REGMDEPSYM2_NI("sort_old",expr_bsort_old,2ul),

	REGRMEM(8),
	REGRMEM(16),
	REGRMEM(32),
	REGRMEM(64),
	REGRMEM(m),
	REGRMEM(p),
	REGRMEM(z),
	REGRMEM(8u),
	REGRMEM(16u),
	REGRMEM(32u),
	REGRMEM(64u),
	REGRMEM(mu),
	REGRMEM(pu),
	REGRMEM(zu),
	REGRMEM(f),
	REGRMEM(l),
	REGZMEM(8),
	REGZMEM(16),
	REGZMEM(32),
	REGZMEM(64),
	REGZMEM(m),
	REGZMEM(p),
	REGZMEM(z),
	REGZMEM(8u),
	REGZMEM(16u),
	REGZMEM(32u),
	REGZMEM(64u),
	REGZMEM(mu),
	REGZMEM(pu),
	REGZMEM(zu),
	REGZMEM(f),
	REGZMEM(l),
	REGWMEM(8),
	REGWMEM(16),
	REGWMEM(32),
	REGWMEM(64),
	REGWMEM(m),
	REGWMEM(p),
	REGWMEM(z),
	REGWMEM(8u),
	REGWMEM(16u),
	REGWMEM(32u),
	REGWMEM(64u),
	REGWMEM(mu),
	REGWMEM(pu),
	REGWMEM(zu),
	REGWMEM(f),
	REGWMEM(l),

	REGMDEPSYM2("andl",expr_andl,0),
	REGMDEPSYM2("orl",expr_orl,0),
	REGMDEPSYM2("piece",expr_piece,0),
	REGMDEPSYM2("d",expr_derivate,0),
	REGMDEPSYM2("dn",expr_multi_derivate,0),
	REGMDEPSYM2("root",expr_root,0),
	REGMDEPSYM2("root2",expr_root2,0),
	REGMDEPSYM2("rooti",expr_rooti,0),
	{.str=NULL}
};
const struct expr_builtin_symbol *expr_builtin_symbol_search(const char *sym,size_t sz){
	for(const struct expr_builtin_symbol *p=expr_symbols;p->str;++p){
		if(sz==p->strlen&&!memcmp(p->str,sym,sz)){
			return p;
		}
	}
	return NULL;
}
const struct expr_builtin_symbol *expr_builtin_symbol_rsearch(void *addr){
	for(const struct expr_builtin_symbol *p=expr_symbols;p->str;++p){
		if(p->un.uaddr==addr){
			return p;
		}
	}
	return NULL;
}
static size_t expr_strcopy(const char *s,size_t sz,char *buf){
	const char *s0=s,*p,*s1;
	char *buf0=buf,v;
	while(s-s0<sz)switch(*s){
		case '\\':
			switch(s[1]){
				case '\\':
					*(buf++)='\\';
					s+=2;
					break;
				case 'a':
					*(buf++)='\a';
					s+=2;
					break;
				case 'b':
					*(buf++)='\b';
					s+=2;
					break;
				case 'c':
					*(buf++)='\0';
					s+=2;
					break;
				case 'e':
					*(buf++)='\033';
					s+=2;
					break;
				case 'f':
					*(buf++)='\f';
					s+=2;
					break;
				case 'n':
					*(buf++)='\n';
					s+=2;
					break;
				case 'r':
					*(buf++)='\r';
					s+=2;
					break;
				case 't':
					*(buf++)='\t';
					s+=2;
					break;
				case 'v':
					*(buf++)='\v';
					s+=2;
					break;
				case 'x':
					s1=s;
					p=s+=2;
					while(p-s0<sz&&((*p>='0'&&*p<='9')
						||(*p>='a'&&*p<='f')
						||(*p>='A'&&*p<='F')
						)&&p-s<2)++p;
					if(p==s)goto fail;
					v=0;
					while(s<p){
						v<<=4;
						switch(*s){
							case '0' ... '9':
								v+=*s-'0';
								break;
							case 'a' ... 'f':
								v+=*s-'a'+10;
								break;
							case 'A' ... 'F':
								v+=*s-'A'+10;
								break;
							default:
								goto fail;
						}
						++s;
					}
					*(buf++)=v;
					break;
				default:
					s1=s;
					p=s+=1;
					while(p-s0<sz&&*p>='0'&&*p<='7'&&p-s<3)
						++p;;
					if(p==s)goto fail;
					v=0;
					while(s<p){
						v<<=3;
						v+=*s-'0';
						++s;
					}
					*(buf++)=v;
					break;
fail:
					*(buf++)=s1[1];
					s=s1+2;
					break;
			}
			break;
		default:
			*(buf++)=*(s++);
			break;
	}
	return buf-buf0;
}
static const char *expr_findpair_dmark(const char *c,const char *endp){
	++c;
	for(;c<endp;++c){
		if(*c=='\"'&&c[-1]!='\\')
			return c;
	}
	return NULL;
}
size_t expr_strscan(const char *s,size_t sz,char *buf){
	char *buf0=buf;
	const char *p,*endp=s+sz;
	if(!sz||*s!='\"')return 0;
	for(;;){
	while(s<endp&&*s!='\"'&&!expr_space(*s))++s;
	if(s>=endp)return buf-buf0;
	++s;
	p=s;
	do {
		p=strchr(p+1,'\"');
	}while(p&&p>s&&p[-1]=='\\');
	if(!p||p<=s)return buf-buf0;
	buf+=expr_strcopy(s,p-s,buf);
	s=p+1;
	}
}
char *expr_astrscan(const char *s,size_t sz,size_t *outsz){
	char *buf;
	buf=xmalloc_nullable(sz);
	if(!buf)return NULL;
	*outsz=expr_strscan(s,sz,buf);
	buf[*outsz]=0;
	if(!*buf){
		xfree(buf);
		return NULL;
	}
	return buf;
}
static void expr_freesuminfo(struct expr_suminfo *p){
	expr_free(p->ep);
	expr_free(p->fromep);
	expr_free(p->toep);
	expr_free(p->stepep);
	xfree(p);
}
static void expr_freevmdinfo(struct expr_vmdinfo *p){
	expr_free(p->ep);
	expr_free(p->fromep);
	expr_free(p->toep);
	expr_free(p->stepep);
	if(p->args)
		xfree(p->args);
	xfree(p);
}
static void expr_freebranchinfo(struct expr_branchinfo *p){
	expr_free(p->cond);
	expr_free(p->body);
	expr_free(p->value);
	xfree(p);
}
static void expr_freemdinfo(struct expr_mdinfo *p){
	for(size_t i=0;i<p->dim;++i)
		expr_free(p->eps+i);
	xfree(p->eps);
	if(p->args)xfree(p->args);
	if(p->e)xfree((void *)p->e);
	xfree(p);
}
static void expr_freedata(struct expr_inst *restrict data,size_t size){
	struct expr_inst *ip=data,*endp=data+size;
	for(;ip<endp;++ip){
		switch(ip->op){
			case SUMCASES:
				expr_freesuminfo(ip->un.es);
				break;
			case MDCASES:
			case EXPR_PMD:
			case EXPR_PME:
			case EXPR_PMEP:
				expr_freemdinfo(ip->un.em);
				break;
			case EXPR_VMD:
				expr_freevmdinfo(ip->un.ev);
				break;
			case BRANCHCASES:
				expr_freebranchinfo(ip->un.eb);
				break;
			case HOTCASES:
				expr_free(ip->un.hotfunc);
				break;
			default:
				break;
		}
	}
	xfree(data);
}
void expr_free(struct expr *restrict ep){
	struct expr *ep0=(struct expr *)ep;
	struct expr_resource *erp,*erp1;
start:
	if(ep->data)expr_freedata(ep->data,ep->size);
	if(ep->vars){
		for(size_t i=0;i<ep->vsize;++i)
			xfree(ep->vars[i]);
		xfree(ep->vars);
	}
	for(erp=ep->res;erp;){
		if(erp->un.uaddr)switch(erp->type){
			case EXPR_HOTFUNCTION:
				expr_free(erp->un.ep);
				break;
			default:
				xfree(erp->un.uaddr);
				break;
		}
		erp1=erp;
		erp=erp->next;
		xfree(erp1);
	}
	switch(ep->freeable){
		case 1:
			xfree(ep0);
			break;
		case 2:
			++ep;
			goto start;
		default:
			break;
	}
}
#define EXTEND_SIZE 1
__attribute__((noinline))
struct expr_inst *expr_addop(struct expr *restrict ep,double *dst,void *src,enum expr_op op,int flag){
	struct expr_inst *ip;
	if(ep->size>=ep->length){
		ep->data=xrealloc(ep->data,
		(ep->length+=EXTEND_SIZE)*sizeof(struct expr_inst));
	}
	ip=ep->data+ep->size++;
	ip->op=op;
	ip->dst.dst=dst;
	ip->un.uaddr=src;
	ip->flag=flag;
	return ip;
}
static struct expr_inst *expr_addread(struct expr *restrict ep,double *dst,double *src){
	return expr_addop(ep,dst,src,EXPR_READ,0);
}
static struct expr_inst *expr_addwrite(struct expr *restrict ep,double *dst,double *src){
	return expr_addop(ep,dst,src,EXPR_WRITE,0);
}
static struct expr_inst *expr_addoff(struct expr *restrict ep,double *dst,double *src){
	return expr_addop(ep,dst,src,EXPR_OFF,0);
}
static struct expr_inst *expr_addcopy(struct expr *restrict ep,double *dst,double *src){
	return expr_addop(ep,dst,src,EXPR_COPY,0);
}
static struct expr_inst *expr_addcall(struct expr *restrict ep,double *dst,double (*func)(double),int flag){
	return expr_addop(ep,dst,func,EXPR_BL,flag);
}
static struct expr_inst *expr_addlj(struct expr *restrict ep,double *dst,double *src){
	return expr_addop(ep,dst,src,EXPR_LJ,0);
}
static struct expr_inst *expr_addsj(struct expr *restrict ep,double *dst){
	return expr_addop(ep,dst,NULL,EXPR_SJ,0);
}
static struct expr_inst *expr_addneg(struct expr *restrict ep,double *dst){
	return expr_addop(ep,dst,NULL,EXPR_NEG,0);
}
static struct expr_inst *expr_addnot(struct expr *restrict ep,double *dst){
	return expr_addop(ep,dst,NULL,EXPR_NOT,0);
}
static struct expr_inst *expr_addnotl(struct expr *restrict ep,double *dst){
	return expr_addop(ep,dst,NULL,EXPR_NOTL,0);
}
static struct expr_inst *expr_addinput(struct expr *restrict ep,double *dst){
	return expr_addop(ep,dst,NULL,EXPR_INPUT,0);
}
static struct expr_inst *expr_addend(struct expr *restrict ep,double *dst){
	return expr_addop(ep,dst,NULL,EXPR_END,0);
}
static struct expr_inst *expr_addza(struct expr *restrict ep,double *dst,double (*zafunc)(void),int flag){
	return expr_addop(ep,dst,zafunc,EXPR_ZA,flag);
}
static struct expr_inst *expr_addmd(struct expr *restrict ep,double *dst,struct expr_mdinfo *em,int flag){
	return expr_addop(ep,dst,em,EXPR_MD,flag);
}
static struct expr_inst *expr_addme(struct expr *restrict ep,double *dst,struct expr_mdinfo *em,int flag){
	return expr_addop(ep,dst,em,EXPR_ME,flag);
}
static struct expr_inst *expr_addmep(struct expr *restrict ep,double *dst,struct expr_mdinfo *em,int flag){
	return expr_addop(ep,dst,em,EXPR_MEP,flag);
}
static struct expr_inst *expr_addhot(struct expr *restrict ep,double *dst,struct expr *hot,int flag){
	return expr_addop(ep,dst,hot,EXPR_HOT,flag);
}
static struct expr_inst *expr_addconst(struct expr *restrict ep,double *dst,double val){
	struct expr_inst *r=expr_addop(ep,dst,NULL,EXPR_CONST,0);
	r->un.value=val;
	return r;
}
static struct expr_inst *expr_addalo(struct expr *restrict ep,double *dst,ssize_t zd){
	struct expr_inst *r=expr_addop(ep,dst,NULL,EXPR_ALO,0);
	r->un.zd=zd;
	return r;
}
static struct expr_resource *expr_newres(struct expr *restrict ep){
	struct expr_resource *p;
	if(!ep->res){
		return ep->res=xmalloc_nullable(sizeof(struct expr_resource));
	}
	p=ep->tail?ep->tail:ep->res;
	while(p->next)p=p->next;
	p->next=xmalloc_nullable(sizeof(struct expr_resource));
	if(!p->next)return NULL;
	p=p->next;
	p->next=NULL;
	p->type=EXPR_CONSTANT;
	ep->tail=p;
	return p;
}
static double *expr_newvar(struct expr *restrict ep){
	double *r=xmalloc_nullable(sizeof(double)),**p;
	if(!r)return NULL;
	if(ep->vsize>=ep->vlength){
		p=xrealloc_nullable(ep->vars,
			(ep->vlength+=EXTEND_SIZE)*sizeof(double *));
		if(!p){
			xfree(r);
			return NULL;
		}
		ep->vars=p;
	}
	*(ep->vars+ep->vsize++)=r;
	*r=NAN;
	return r;
}
static int expr_detach(struct expr *restrict ep){
	if(!ep->sset_shouldfree){
		if(!ep->sset)ep->sset=new_expr_symset();
		else ep->sset=expr_symset_clone(ep->sset);
		if(!ep->sset)return -1;
		ep->sset_shouldfree=1;
	}
	return 0;
}
static int expr_createconst(struct expr *restrict ep,const char *symbol,size_t symlen,double val){
	cknp(ep,expr_detach(ep)>=0,return -1);
	return expr_symset_addl(ep->sset,symbol,symlen,EXPR_CONSTANT,val)?
	0:-1;
}
static int expr_createsvar(struct expr *restrict ep,const char *symbol,size_t symlen,double val){
	struct expr_resource *r;
	cknp(ep,expr_detach(ep)>=0,return -1);
	r=expr_newres(ep);
	cknp(ep,r,return -1);
	r->un.addr=xmalloc_nullable(sizeof(double));
	cknp(ep,r->un.addr,return -1);
	*r->un.addr=val;
	return expr_symset_addl(ep->sset,symbol,symlen,EXPR_VARIABLE,r->un.addr)?
	0:-1;
}
static double *expr_createvar(struct expr *restrict ep,const char *symbol,size_t symlen){
	double *r=expr_newvar(ep);
	if(!r)return NULL;
	cknp(ep,expr_detach(ep)>=0,return NULL);
	expr_symset_addl(ep->sset,symbol,symlen,EXPR_VARIABLE,r);
	return r;
}
static const char *expr_findpair(const char *c,const char *endp){
	size_t lv=0;
	if(*c!='(')goto err;
	while(c<endp){
		switch(*c){
			case '(':
				++lv;
				break;
			case ')':
				--lv;
				if(!lv)return c;
				break;
			default:
				break;
		}
		++c;
	}
err:
	return NULL;
}
static const char *expr_findpair_bracket(const char *c,const char *endp){
	size_t lv=0;
	if(*c!='[')goto err;
	while(c<endp){
		switch(*c){
			case '[':
				++lv;
				break;
			case ']':
				--lv;
				if(!lv)return c;
				break;
			default:
				break;
		}
		++c;
	}
err:
	return NULL;
}
static const char *expr_findpair_brace(const char *c,const char *endp){
	size_t lv=0;
	if(*c!='{')goto err;
	while(c<endp){
		switch(*c){
			case '{':
				++lv;
				break;
			case '}':
				--lv;
				if(!lv)return c;
				break;
			default:
				break;
		}
		++c;
	}
err:
	return NULL;
}
static const char *expr_unfindpair(const char *e,const char *c){
	size_t lv=0;
	if(*c!=')')goto err;
	while(c>=e){
		switch(*c){
			case ')':
				++lv;
				break;
			case '(':
				--lv;
				if(!lv)return c;
				break;
			default:
				break;
		}
		--c;
	}
err:
	return NULL;
}
static const char *expr_getsym(const char *c,const char *endp){
	while(c<endp&&!expr_operator(*c))
		++c;
	return c;
}
static const char *expr_getsym_expo(const char *c,const char *endp){
	const char *c0=c;
	while(c<endp&&!expr_operator(*c))
		++c;
	if(c+1<endp&&c-c0>=2&&(*c=='-'||*c=='+')&&(c[-1]=='e'||c[-1]=='E')&&((c[-2]<='9'&&c[-2]>=0)||c[-2]=='.')){
		return expr_getsym(c+1,endp);
	}
	return c;
}
static int expr_atod2(const char *str,double *dst){
	int ret;
	char c;
	ret=sscanf(str,"%lf%c",dst,&c);
	return ret;
}
static int expr_atod(const char *str,size_t sz,double *dst){
	int ret;
	char *p0=xmalloc_nullable(sz+1),*p;
	p=p0?p0:alloca(sz+1);
	p[sz]=0;
	memcpy(p,str,sz);
	ret=expr_atod2(p,dst);
	if(p0)xfree(p0);
	return ret;
}
static char *expr_tok(char *restrict str,char **restrict saveptr){
	char *s0=(char *)str;
	int instr=0;
	if(str){
		*saveptr=str;
	}else if(!**saveptr)return NULL;
	else {
		str=*saveptr;
	}
	while(**saveptr){
		switch(**saveptr){
			case '\"':
				if(*saveptr>s0&&(*saveptr)[-1]=='\\')
					break;
				instr^=1;
				break;
			case ',':
				if(instr)break;
				**saveptr=0;
				++(*saveptr);
				return str;
			case '(':
				*saveptr=(char *)expr_findpair(*saveptr,*saveptr+strlen(*saveptr));
				if(!*saveptr)return NULL;
			default:
				break;
		}
		++(*saveptr);
	}
	return str;
}
static void expr_free2(char **buf){
	for(char **p=buf;*p;++p){
		xfree(*p);
	}
	xfree(buf);
}
static char **expr_sep(struct expr *restrict ep,const char *pe,size_t esz){
	char *p,*p1,*p2,**p3=NULL,/*p5,*/*e,*p6;
	void *p7;
	size_t len=0,s/*,sz*/;
	p6=e=xmalloc_nullable(esz+1);
	cknp(ep,e,return NULL);
	memcpy(e,pe,esz);
	e[esz]=0;
	if(*e=='('){
		p1=(char *)expr_findpair(e,e+esz);
		if(p1){
			if(!p1[1]){
				*p1=0;
				++e;
			}
		}else {
			ep->error=EXPR_EPT;
			goto fail;
		}
	}
	if(!*e){
		ep->error=EXPR_ENVP;
		goto fail;
	}
	for(p=expr_tok(e,&p2);p;p=expr_tok(NULL,&p2)){
		s=strlen(p);
		/*if(*p=='\"'&&(p5=expr_astrscan(p,s,&sz))){
			for(char *p4=p5;p4-p5<sz;++p4){
				p1=xmalloc_nullable(5);
				cknp(ep,p1,goto fail);
				p1[0]='0';
				p1[1]='x';
				p1[2]=ntoc[*p4>>4];
				p1[3]=ntoc[*p4&15];
				p1[4]=0;
				p7=xrealloc_nullable(p3,(len+2)*sizeof(char *));
				cknp(ep,p7,goto fail);
				p3=p7;
				p3[len++]=p1;
			}
			xfree(p5);
		}else */if(*p=='{'){
			long from,to,istep=1;
			size_t diff;
			double f,t,step;
			int r;
			char c;
			r=sscanf(p+1,"%ld..%ld%c",&from,&to,&c);
			if(r!=3||c!='}'){
				r=sscanf(p+1,"%lf:%lf:%lf%c",&f,&step,&t,&c);
				if(r!=4||c!='}'){
					r=sscanf(p+1,"%ld..%ld..%ld%c",&from,&istep,&to,&c);
					if(r!=4||c!='}')goto normal;
					if(istep<0l)istep=-istep;
					goto integer;
				}
				if(step<0.0)step=-step;
				if(f<=t)
				do {
					cknp(ep,xasprintf_nullable(&p1,"%.64lg",f)>=0,goto fail);
					p7=xrealloc_nullable(p3,(len+2)*sizeof(char *));
					cknp(ep,p7,goto fail);
					p3=p7;
					p3[len++]=p1;
					f+=step;
				}while(f<=t);
				else
				do {
					cknp(ep,xasprintf_nullable(&p1,"%.64lg",f)>=0,goto fail);
					p7=xrealloc_nullable(p3,(len+2)*sizeof(char *));
					cknp(ep,p7,goto fail);
					p3=p7;
					p3[len++]=p1;
					f-=step;
				}while(f>=t);
				continue;
			}
integer:
			diff=(from>to?from-to:to-from);
			p7=xrealloc_nullable(p3,(len+diff/istep+1+1)*sizeof(char *));
			cknp(ep,p7,goto fail);
			p3=p7;
			if(from<=to)
			do {
				cknp(ep,xasprintf_nullable(&p1,"%ld",from)>=0,goto fail);
				p3[len++]=p1;
				from+=istep;
			}while(from<=to);
			else
			do {
				cknp(ep,xasprintf_nullable(&p1,"%ld",from)>=0,goto fail);
				p3[len++]=p1;
				from-=istep;
			}while(from>=to);
		}else{
normal:
			p1=xmalloc_nullable(s+1);
			cknp(ep,p1,goto fail);
			p1[s]=0;
			memcpy(p1,p,s);
			p7=xrealloc_nullable(p3,(len+2)*sizeof(char *));
			cknp(ep,p7,goto fail);
			p3=p7;
			p3[len++]=p1;
		}
	}
	if(p3)p3[len]=NULL;
	xfree(p6);
	return p3;
fail:
	if(p3){
		p3[len]=NULL;
		expr_free2(p3);
	}
	xfree(p6);
	return NULL;
}
static struct expr_mdinfo *expr_getmdinfo(struct expr *restrict ep,const char *e0,size_t sz,const char *e,size_t esz,const char *asym,size_t asymlen,void *func,size_t dim,int ifep){
	char **v,**p;
	char *pe;
	char *v1=NULL;
	size_t i;
	struct expr_mdinfo *em;
	if(dim==1){
		v1=xmalloc_nullable(esz+1);
		cknp(ep,v1,return NULL);
		memcpy(v1,e,esz);
		v1[esz]=0;
		v=&v1;
		i=1;
	}else {
		v=expr_sep(ep,e,esz);
		if(!v){
			memcpy(ep->errinfo,e0,sz);
			return NULL;
		}
		p=v;
		while(*p)++p;
		i=p-v;
		if(dim&&i!=dim){
			memcpy(ep->errinfo,e0,sz);
			ep->error=EXPR_ENEA;
			goto err1;
		}
	}
	em=xmalloc_nullable(sizeof(struct expr_mdinfo));
	cknp(ep,em,goto err1);
	em->e=NULL;
	em->args=NULL;
	em->dim=i;
	em->un.func=func;
	em->eps=xmalloc_nullable(em->dim*sizeof(struct expr));
	cknp(ep,em->eps,goto err15);
	switch(ifep){
		case 0:
			em->args=xmalloc_nullable(em->dim*sizeof(double));
			cknp(ep,em->args,goto err175);
			break;
		default:
			sz+=esz;
			pe=xmalloc_nullable(sz+1);
			cknp(ep,pe,goto err175);
			memcpy(pe,e0,sz);
			pe[sz]=0;
			em->e=pe;
		case 1:
		break;
	}
	for(i=0;i<em->dim;++i){
		if(init_expr7(em->eps+i,v[i],strlen(v[i]),asym,asymlen,ep->sset,ep->iflag)<0){
			for(ssize_t k=i-1;k>=0;--k)
				expr_free(em->eps+k);
			goto err2;
		}
	}
	if(v1)xfree(v1);
	else expr_free2(v);
	return em;
err2:
	if(em->args)xfree(em->args);
	if(em->e)xfree((void *)em->e);
	ep->error=em->eps[i].error;
	memcpy(ep->errinfo,em->eps[i].errinfo,EXPR_SYMLEN);
err175:
	xfree(em->eps);
err15:
	xfree(em);
err1:
	if(v1)xfree(v1);
	else expr_free2(v);
	return NULL;
}
static int expr_seizeres(struct expr *restrict dst,struct expr *restrict src){
	struct expr_resource *drp;
	for(struct expr_resource *srp=src->res;srp;srp=srp->next){
		drp=expr_newres(dst);
		cknp(dst,drp,return -1);
		drp->type=srp->type;
		drp->un.uaddr=srp->un.uaddr;
		srp->un.uaddr=NULL;
	}
	return 0;
}
static struct expr *new_expr10(const char *e,size_t len,const char *asym,size_t asymlen,struct expr_symset *esp,int flag,int n,int *error,char errinfo[EXPR_SYMLEN],struct expr *parent);
static double expr_consteval(const char *e,size_t len,const char *asym,size_t asymlen,struct expr_symset *sset,struct expr *restrict parent){
	struct expr *ep;
	double r;
	ep=new_expr10(e,len,asym,asymlen,sset,0,1,&parent->error,parent->errinfo,parent);
	if(!ep)return NAN;
	if(!expr_isconst(ep)){
		expr_free(ep);
		parent->error=EXPR_ENC;
		memcpy(parent->errinfo,e,minc(len,EXPR_SYMLEN));
		return NAN;
	}
	r=expr_eval(ep,0.0);
	expr_seizeres(parent,ep);
	expr_free(ep);
	return r;
}
static struct expr_vmdinfo *expr_getvmdinfo(struct expr *restrict ep,const char *e0,size_t sz,const char *e,size_t esz,const char *asym,size_t asymlen,int *flag){
	char **v=expr_sep(ep,e,esz);
	char **p;
	struct expr_vmdinfo *ev;
	struct expr_symset *sset;
	union {
		const struct expr_symbol *es;
		const struct expr_builtin_symbol *ebs;
	} sym;
	size_t ssz;
	ssize_t max;
	double (*fp)(size_t n,double *args);
	if(!v){
		memcpy(ep->errinfo,e0,sz);
		return NULL;
	}
	p=v;
	while(*p){
		++p;
	}
	switch(p-v){
		case 6:
			max=0;
			break;
		case 7:
			max=(ssize_t)expr_consteval(v[6],strlen(v[6]),asym,asymlen,ep->sset,ep);
			if(ep->error)goto err0;
			if(max<0)max=-max;
			break;
		default:
			memcpy(ep->errinfo,e0,sz);
			ep->error=EXPR_ENEA;
			goto err0;
	}
	ssz=strlen(v[5]);
	if(ep->sset&&(sym.es=expr_symset_search(ep->sset,v[5],ssz))){
		if(sym.es->type!=EXPR_MDFUNCTION||SYMDIM(sym.es)){
evmd:
			memcpy(ep->errinfo,v[5],ssz);
			ep->error=EXPR_EVMD;
			goto err0;
		}
		fp=sym.es->un.mdfunc;
		*flag=sym.es->flag;
	}else if((sym.ebs=expr_builtin_symbol_search(v[5],ssz))){
		if(sym.ebs->type!=EXPR_MDFUNCTION||sym.ebs->dim)
			goto evmd;
		fp=sym.ebs->un.mdfunc;
		*flag=sym.ebs->flag;
	}else {
		memcpy(ep->errinfo,v[5],ssz);
		ep->error=EXPR_ESYMBOL;
		goto err0;
	}
	ev=xmalloc_nullable(sizeof(struct expr_vmdinfo));
	cknp(ep,ev,goto err0);
	if(max>0){
		ev->args=xmalloc_nullable(max*sizeof(double));
		cknp(ep,ev->args,goto err05);
		ev->max=max;
	}else {
		ev->args=NULL;
		ev->max=0;
	}
	ev->func=fp;
	sset=new_expr_symset();
	cknp(ep,sset,goto err075);
	expr_symset_add(sset,v[0],EXPR_VARIABLE,&ev->index);
	expr_symset_copy(sset,ep->sset);
	ev->fromep=new_expr8(v[1],strlen(v[1]),asym,asymlen,ep->sset,ep->iflag,&ep->error,ep->errinfo);
	if(!ev->fromep)goto err1;
	ev->toep=new_expr8(v[2],strlen(v[2]),asym,asymlen,sset,ep->iflag,&ep->error,ep->errinfo);
	if(!ev->toep)goto err2;
	ev->stepep=new_expr8(v[3],strlen(v[3]),asym,asymlen,sset,ep->iflag,&ep->error,ep->errinfo);
	if(!ev->stepep)goto err3;
	ev->ep=new_expr8(v[4],strlen(v[4]),asym,asymlen,sset,ep->iflag,&ep->error,ep->errinfo);
	if(!ev->ep)goto err4;

	expr_free2(v);
	expr_symset_free(sset);
	return ev;
err4:
	expr_free(ev->stepep);
err3:
	expr_free(ev->toep);
err2:
	expr_free(ev->fromep);
err1:
	expr_symset_free(sset);
err075:
	if(ev->args)xfree(ev->args);
err05:
	xfree(ev);
err0:
	expr_free2(v);
	return NULL;
}
static struct expr_suminfo *expr_getsuminfo(struct expr *restrict ep,const char *e0,size_t sz,const char *e,size_t esz,const char *asym,size_t asymlen){
	char **v=expr_sep(ep,e,esz);
	char **p;
	struct expr_suminfo *es;
	struct expr_symset *sset;
	if(!v){
		memcpy(ep->errinfo,e0,sz);
		return NULL;
	}
	p=v;
	while(*p){
		++p;
	}
	if(p-v!=5){
		memcpy(ep->errinfo,e0,sz);
		ep->error=EXPR_ENEA;
		goto err0;
	}
	es=xmalloc_nullable(sizeof(struct expr_suminfo));
	cknp(ep,es,goto err0);
//	sum(sym_index,from,to,step,expression)
	sset=new_expr_symset();
	cknp(ep,sset,goto err05);
	expr_symset_add(sset,v[0],EXPR_VARIABLE,&es->index);
	expr_symset_copy(sset,ep->sset);
	es->fromep=new_expr8(v[1],strlen(v[1]),asym,asymlen,ep->sset,ep->iflag,&ep->error,ep->errinfo);
	if(!es->fromep)goto err1;
	es->toep=new_expr8(v[2],strlen(v[2]),asym,asymlen,sset,ep->iflag,&ep->error,ep->errinfo);
	if(!es->toep)goto err2;
	es->stepep=new_expr8(v[3],strlen(v[3]),asym,asymlen,sset,ep->iflag,&ep->error,ep->errinfo);
	if(!es->stepep)goto err3;
	es->ep=new_expr8(v[4],strlen(v[4]),asym,asymlen,sset,ep->iflag,&ep->error,ep->errinfo);
	if(!es->ep)goto err4;
	expr_free2(v);
	expr_symset_free(sset);
	return es;
err4:
	expr_free(es->stepep);
err3:
	expr_free(es->toep);
err2:
	expr_free(es->fromep);
err1:
	expr_symset_free(sset);
err05:
	xfree(es);
err0:
	expr_free2(v);
	return NULL;
}
struct branch {
	const char *cond;
	const char *body;
	const char *value;
	size_t scond;
	size_t sbody;
	size_t svalue;
};
static struct expr_branchinfo *expr_getbranchinfo(struct expr *restrict ep,const char *e0,size_t sz,const char *e,size_t esz,struct branch *b,const char *asym,size_t asymlen){
	char **v=NULL;
	char **p;
	struct expr_branchinfo *eb;
	int dim3;
	if(b){
		eb=xmalloc_nullable(sizeof(struct expr_branchinfo));
		cknp(ep,eb,goto err0);
	//	while(cond,body,value)
		eb->cond=new_expr8(b->cond,b->scond,asym,asymlen,ep->sset,ep->iflag,&ep->error,ep->errinfo);
		if(!eb->cond)goto err1;
		if(b->sbody){
			eb->body=new_expr8(b->body,b->sbody,asym,asymlen,ep->sset,ep->iflag,&ep->error,ep->errinfo);
			if(!eb->body)goto err2;
		}else {
			eb->body=new_expr_const(NAN);
			if(!eb->body){
				ep->error=EXPR_EMEM;
				goto err2;
			}
		}
		if(b->svalue){
			eb->value=new_expr8(b->value,b->svalue,asym,asymlen,ep->sset,ep->iflag,&ep->error,ep->errinfo);
			if(!eb->value)goto err3;
		}else {
			eb->value=new_expr_const(NAN);
			if(!eb->value){
				ep->error=EXPR_EMEM;
				goto err3;
			}
		}
		return eb;
	}
	v=expr_sep(ep,e,esz);
	if(!v){
		memcpy(ep->errinfo,e0,sz);
		return NULL;
	}
	p=v;
	while(*p){
		++p;
	}
	switch(p-v){
		case 3:
			dim3=1;
			break;
		case 2:
			dim3=0;
			break;
		default:
			memcpy(ep->errinfo,e0,sz);
			ep->error=EXPR_ENEA;
			goto err0;
	}
	eb=xmalloc_nullable(sizeof(struct expr_branchinfo));
	cknp(ep,eb,goto err0);
//	while(cond,body,value)
	eb->cond=new_expr8(v[0],strlen(v[0]),asym,asymlen,ep->sset,ep->iflag,&ep->error,ep->errinfo);
	if(!eb->cond)goto err1;
	eb->body=new_expr8(v[1],strlen(v[1]),asym,asymlen,ep->sset,ep->iflag,&ep->error,ep->errinfo);
	if(!eb->body)goto err2;
	if(dim3){
		eb->value=new_expr8(v[2],strlen(v[2]),asym,asymlen,ep->sset,ep->iflag,&ep->error,ep->errinfo);
		if(!eb->value)goto err3;
	}else {
		eb->value=new_expr_const(NAN);
		if(!eb->value){
			ep->error=EXPR_EMEM;
			goto err3;
		}
	}
	expr_free2(v);
	return eb;
err3:
	expr_free(eb->body);
err2:
	expr_free(eb->cond);
err1:
	xfree(eb);
err0:
	if(v)expr_free2(v);
	return NULL;
}
static double *expr_scan(struct expr *restrict ep,const char *e,const char *endp,const char *asym,size_t asymlen);
static double *expr_getvalue(struct expr *restrict ep,const char *e,const char *endp,const char **_p,const char *asym,size_t asymlen){
	const char *p,*p2;
	double *v0=NULL,*v1;
	int r0,builtin=0;
	union {
		double v;
		void *uaddr;
		struct expr *ep;
		struct expr_suminfo *es;
		struct expr_branchinfo *eb;
		struct expr_mdinfo *em;
		struct expr_vmdinfo *ev;
		char **vv1;
	} un;
	union {
		const struct expr_symbol *es;
		const struct expr_builtin_symbol *ebs;
		struct expr_resource *er;
		char **vv;
		struct branch *b;
	} sym;
	union {
		const union expr_symvalue *sv;
		struct expr_symbol *es;
	} sv;
	int type,flag;
	size_t dim=0;
	if(*e=='+')++e;
	if(e>=endp)goto eev;
	switch(*e){
		case 0:
eev:
			ep->error=EXPR_EEV;
			return NULL;
		case '(':
			p=expr_findpair(e,endp);
			if(!p){
pterr:
				ep->error=EXPR_EPT;
				return NULL;
			}
			if(p==e+1){
envp:
				ep->error=EXPR_ENVP;
				return NULL;
			}
			v0=expr_scan(ep,e+1,p,asym,asymlen);
			if(!v0)return NULL;
			e=p+1;
			goto vend;
		case '_':
			if(e+9>=endp||memcmp(e+1,"_builtin_",9))
				break;
			e+=10;
			p=expr_getsym(e,endp);
			if(p==e){
				if(e<endp&&expr_operator(*e)){
					*ep->errinfo=*e;
					ep->error=EXPR_EUO;
					return NULL;
				}
				goto symerr;
			}
			builtin=1;
			break;
		case 'd':
			if(e+2>endp||e[1]!='o'||e[2]!='{')
				break;
			e+=2;
			dim=1;
		case '{':
			r0=0;
block:
			p=expr_findpair_brace(e,endp);
			if(!p)goto pterr;
			if(p==e+1)goto envp;
			v0=expr_newvar(ep);
			cknp(ep,v0,return NULL);
			un.ep=new_expr8(e+1,p-e-1,asym,asymlen,ep->sset,ep->iflag,&ep->error,ep->errinfo);
			if(!un.ep)return NULL;
			if(r0){
				sym.er=expr_newres(ep);
				cknp(ep,sym.er,expr_free(un.ep);return NULL);
				sym.er->un.ep=un.ep;
				sym.er->type=EXPR_HOTFUNCTION;
				expr_addconst(ep,v0,un.v);
			}else {
				expr_addop(ep,v0,un.ep,dim?EXPR_DO:EXPR_EP,0);
			}
			e=p+1;
			goto vend;
		case '[':
			p=expr_findpair_bracket(e,endp);
			if(!p)goto pterr;
			if(p==e+1)goto envp;
			v1=expr_scan(ep,e+1,p,asym,asymlen);
			if(!v1)return NULL;
			v0=expr_newvar(ep);
			cknp(ep,v0,return NULL);
			expr_addread(ep,v0,v1);
			e=p+1;
			goto vend;
		case '\'':
			if(e+2>=endp||e[2]!='\'')break;
			v0=expr_newvar(ep);
			cknp(ep,v0,return NULL);
			expr_addconst(ep,v0,(double)((unsigned char *)e)[1]);
			e+=3;
			goto vend;
		case '\"':
			p=expr_findpair_dmark(e,endp);
			if(!p)goto pterr;
			un.uaddr=expr_astrscan(e,endp-e,&dim);
			if(!un.uaddr)break;
			sym.er=expr_newres(ep);
			cknp(ep,sym.er,xfree(un.uaddr);return NULL);
			sym.er->un.str=un.uaddr;
			v0=expr_newvar(ep);
			cknp(ep,v0,return NULL);
			expr_addconst(ep,v0,un.v);
			e=p+1;
			goto vend;
		case '&':
			if(e+1<endp)switch(e[1]){
				case '#':
					v0=expr_newvar(ep);
					cknp(ep,v0,return NULL);
					if(ep->parent)
						un.ep=ep->parent;
					else
						un.ep=(struct expr *)ep;
					expr_addconst(ep,v0,un.v);
					e+=2;
					goto vend;
				case '{':
					r0=1;
					++e;
					goto block;
				default:
					break;
			}
			++e;
			if(e+9<endp&&!memcmp(e+1,"_builtin_",9)){
				builtin=1;
				e+=10;
			}
			p=expr_getsym(e,endp);
			if(p==e){
				ep->error=EXPR_ECTA;
				*ep->errinfo=*e;
				return NULL;
			}
			if(builtin||!ep->sset||!(sym.es=expr_symset_search(ep->sset,e,p-e))){
				sym.ebs=expr_builtin_symbol_search(e,p-e);
				if(sym.ebs){
					type=sym.ebs->type;
					un.uaddr=sym.ebs->un.uaddr;
				}else {
ecta:
					ep->error=EXPR_ECTA;
					memcpy(ep->errinfo,e,minc(p-e,EXPR_SYMLEN));
					return NULL;
				}
			}else {
				type=sym.es->type;
				un.uaddr=sym.es->un.uaddr;
			}
			switch(type){
				case EXPR_CONSTANT:
				case EXPR_HOTFUNCTION:
					goto ecta;
				default:
					break;
			}
			v0=expr_newvar(ep);
			cknp(ep,v0,return NULL);
			expr_addconst(ep,v0,un.v);
			e=p;
			goto vend;
		case '0' ... '9':
			goto number;
		case '.':
			switch(e[1]){
			case '0' ... '9':
				goto number;
			default:
				break;
			}
		default:
			break;
	}
	p=expr_getsym(e,endp);
	if(!builtin){
		if(asym&&p-e==asymlen&&!memcmp(e,asym,p-e)){
			v0=expr_newvar(ep);
			cknp(ep,v0,return NULL);
			expr_addinput(ep,v0);
			e=p;
			goto vend;
		}
		if(ep->sset&&(sym.es=expr_symset_search(ep->sset,e,p-e))){
			type=sym.es->type;
			switch(type){
				case EXPR_MDFUNCTION:
				case EXPR_MDEPFUNCTION:
					dim=SYMDIM(sym.es);
				default:
					break;
			}
			sv.sv=&sym.es->un;
			flag=sym.es->flag;
			goto found;
		}
	}
	if((sym.ebs=expr_builtin_symbol_search(e,p-e))){
		type=sym.ebs->type;
		sv.sv=&sym.ebs->un;
		dim=sym.ebs->dim;
		flag=sym.ebs->flag;
		goto found;
	}
	for(const struct expr_builtin_keyword *kp=expr_keywords;
			kp->str;++kp){
		if(p-e!=kp->strlen||memcmp(e,kp->str,p-e))
			continue;
		if(*p!='('){
			memcpy(ep->errinfo,e,p-e);
			ep->error=EXPR_EFP;
			return NULL;
		}
		p2=e;
		e=p;
		p=expr_findpair(e,endp);
		if(!p)goto pterr;
		flag=0;
		switch(kp->op){
			case EXPR_CONST:
				sym.vv=expr_sep(ep,e,p-e+1);
				if(!sym.vv)return NULL;
				for(un.vv1=sym.vv;*un.vv1;++un.vv1);
				switch(un.vv1-sym.vv){
					case 1:
						un.v=0.0;
						break;
					case 2:
						un.v=expr_consteval(sym.vv[1],strlen(sym.vv[1]),asym,asymlen,ep->sset,ep);
						if(ep->error)goto c_fail;
						break;
					default:
						ep->error=EXPR_ENEA;
						memcpy(ep->errinfo,"const",mincc(5,EXPR_SYMLEN));
c_fail:
						expr_free2(sym.vv);
						return NULL;
				}
				if(ep->sset&&expr_symset_search(ep->sset,sym.vv[0],dim=strlen(sym.vv[0]))){
					ep->error=EXPR_EDS;
					memcpy(ep->errinfo,sym.vv[0],minc(dim,EXPR_SYMLEN));
					goto c_fail;
				}
				r0=expr_createconst(ep,sym.vv[0],dim,un.v);
				expr_free2(sym.vv);
				cknp(ep,!r0,return NULL);
				v0=expr_newvar(ep);
				expr_addconst(ep,v0,un.v);
				e=p+1;
				goto vend;
			case EXPR_MUL:
				sym.vv=expr_sep(ep,e,p-e+1);
				if(!sym.vv)return NULL;
				for(un.vv1=sym.vv;*un.vv1;++un.vv1);
				switch(un.vv1-sym.vv){
					case 1:
						un.v=0.0;
						break;
					case 2:
						un.v=expr_consteval(sym.vv[1],strlen(sym.vv[1]),asym,asymlen,ep->sset,ep);
						if(ep->error)goto c_fail;
						break;
					default:
						ep->error=EXPR_ENEA;
						memcpy(ep->errinfo,"var",mincc(3,EXPR_SYMLEN));
						goto c_fail;
				}
				if(ep->sset&&expr_symset_search(ep->sset,sym.vv[0],dim=strlen(sym.vv[0]))){
					ep->error=EXPR_EDS;
					memcpy(ep->errinfo,sym.vv[0],minc(dim,EXPR_SYMLEN));
					goto c_fail;
				}
				r0=expr_createsvar(ep,sym.vv[0],dim,un.v);
				expr_free2(sym.vv);
				cknp(ep,!r0,return NULL);
				v0=expr_newvar(ep);
				expr_addconst(ep,v0,un.v);
				e=p+1;
				goto vend;
			case EXPR_ADD:
				sym.vv=expr_sep(ep,e,p-e+1);
				if(!sym.vv)return NULL;
				for(un.vv1=sym.vv;*un.vv1;++un.vv1);
				switch(un.vv1-sym.vv){
					case 1:
						un.v=0.0;
						break;
					case 2:
						un.v=expr_consteval(sym.vv[1],strlen(sym.vv[1]),asym,asymlen,ep->sset,ep);
						if(ep->error)goto c_fail;
						break;
					default:
						ep->error=EXPR_ENEA;
						memcpy(ep->errinfo,"decl",mincc(4,EXPR_SYMLEN));
						goto c_fail;
				}
				cknp(ep,expr_detach(ep)>=0,goto c_fail);
				if(!ep->sset||!(sv.es=expr_symset_search(ep->sset,sym.vv[0],dim=strlen(sym.vv[0])))){
					ep->error=expr_builtin_symbol_search(sym.vv[0],dim)?
						EXPR_ETNV:EXPR_ESYMBOL;
					memcpy(ep->errinfo,sym.vv[0],minc(dim,EXPR_SYMLEN));
					goto c_fail;
				}else switch(sv.es->type){
					case EXPR_CONSTANT:
					case EXPR_VARIABLE:
						break;
					default:
						ep->error=EXPR_ETNV;
						memcpy(ep->errinfo,sym.vv[0],minc(dim,EXPR_SYMLEN));
						goto c_fail;
				}
				expr_free2(sym.vv);
				flag=sv.es->flag;
				sv.es->flag=(int)un.v;
				v0=expr_newvar(ep);
				expr_addconst(ep,v0,(double)flag);
				e=p+1;
				goto vend;
			case EXPR_INPUT:
				dim=sizeof(struct expr_jmpbuf);
				goto use_byte;
			case EXPR_COPY:
				dim=1;
				goto use_byte;
			case EXPR_BL:
				dim=sizeof(double);
use_byte:
				un.v=expr_consteval(e+1,p-e-1,asym,asymlen,ep->sset,ep);
				if(ep->error)return NULL;
				dim*=(size_t)fabs(un.v);
				v0=expr_newvar(ep);
				cknp(ep,v0,return NULL);
				un.uaddr=xmalloc_nullable(dim);
				cknp(ep,un.uaddr,return NULL);
				sym.er=expr_newres(ep);
				cknp(ep,sym.er,xfree(un.uaddr);return NULL);
				sym.er->un.uaddr=un.uaddr;
				expr_addconst(ep,v0,un.v);
				e=p+1;
				goto vend;
			case EXPR_SUB:
				un.v=expr_consteval(e+1,p-e-1,asym,asymlen,ep->sset,ep);
				if(ep->error)return NULL;
				if(un.v==0.0){
					ep->error=EXPR_ESAF;
					memcpy(ep->errinfo,e+1,minc(p-e-1,EXPR_SYMLEN));
					return NULL;
				}
				v0=expr_newvar(ep);
				cknp(ep,v0,return NULL);
				expr_addconst(ep,v0,un.v);
				e=p+1;
				goto vend;
			case EXPR_ALO:
				sym.vv=expr_sep(ep,e,p-e+1);
				if(!sym.vv)return NULL;
				for(un.vv1=sym.vv;*un.vv1;++un.vv1);
				switch(un.vv1-sym.vv){
					case 1:
						dim=1;
						break;
					case 2:
						un.v=expr_consteval(sym.vv[1],strlen(sym.vv[1]),asym,asymlen,ep->sset,ep);
						if(ep->error)goto c_fail;
						dim=(size_t)fabs(un.v);
						break;
					default:
						ep->error=EXPR_ENEA;
						memcpy(ep->errinfo,"alloca",mincc(6,EXPR_SYMLEN));
						goto c_fail;
				}
				v0=expr_scan(ep,sym.vv[0],sym.vv[0]+strlen(sym.vv[0]),asym,asymlen);
				expr_free2(sym.vv);
				if(!v0)return NULL;
				expr_addalo(ep,v0,(ssize_t)dim);
				e=p+1;
				goto vend;
			case EXPR_SJ:
				v0=expr_scan(ep,e+1,p,asym,asymlen);
				if(!v0)return NULL;
				expr_addsj(ep,v0);
				e=p+1;
				goto vend;
			case EXPR_LJ:
				sym.vv=expr_sep(ep,e,p-e+1);
				if(!sym.vv)return NULL;
				for(un.vv1=sym.vv;*un.vv1;++un.vv1);
				if(un.vv1-sym.vv!=2){
					expr_free2(sym.vv);
					ep->error=EXPR_ENEA;
					memcpy(ep->errinfo,"longjmp",mincc(7,EXPR_SYMLEN));
					return NULL;
				}
				v0=expr_scan(ep,sym.vv[0],sym.vv[0]+strlen(sym.vv[0]),asym,asymlen);
				if(!v0)goto c_fail;
				v1=expr_scan(ep,sym.vv[1],sym.vv[1]+strlen(sym.vv[1]),asym,asymlen);
				expr_free2(sym.vv);
				if(!v1)return NULL;
				expr_addlj(ep,v0,v1);
				e=p+1;
				goto vend;
			case EXPR_EVAL:
				sym.vv=expr_sep(ep,e,p-e+1);
				if(!sym.vv)return NULL;
				for(un.vv1=sym.vv;*un.vv1;++un.vv1);
				switch(un.vv1-sym.vv){
					case 2:
						v0=expr_scan(ep,sym.vv[1],sym.vv[1]+strlen(sym.vv[1]),asym,asymlen);
						if(!v0)goto c_fail;
						break;
					case 1:
						v0=expr_newvar(ep);
						cknp(ep,v0,goto c_fail);
						expr_addinput(ep,v0);
						break;
					default:
					ep->error=EXPR_ENEA;
					memcpy(ep->errinfo,"eval",mincc(4,EXPR_SYMLEN));
					goto c_fail;
				}

				v1=expr_scan(ep,sym.vv[0],sym.vv[0]+strlen(sym.vv[0]),asym,asymlen);
				expr_free2(sym.vv);
				if(!v1)return NULL;
				expr_addop(ep,v0,v1,EXPR_EVAL,0);
				e=p+1;
				goto vend;
			case BRANCHCASES:
				if(p+1<endp&&p[1]=='{'){
					sym.b=alloca(sizeof(struct branch));
					sym.b->cond=e+1;
					sym.b->scond=p-e-1;
					e=p+1;
					p=expr_findpair_brace(p+1,endp);
					if(!p)goto pterr;
					sym.b->body=e+1;
					sym.b->sbody=p-e-1;
					if(++p<endp)switch(*p){
						case 'e':
							if(p+4>=endp||memcmp(p+1,"lse{",4)){
						default:
								goto vzero;
							}
							p+=4;
						case '{':
							switch(kp->op){
								case EXPR_IF:
									break;
								default:
									ep->error=EXPR_EUO;
									*ep->errinfo='{';
									return NULL;
							}
							e=p;
							p=expr_findpair_brace(e,endp);
							if(!p)goto pterr;
							sym.b->value=e+1;
							sym.b->svalue=p-e-1;
							break;
					}else {
vzero:
						--p;
						sym.b->svalue=0;
					}
					if(!sym.b->scond){
						memcpy(ep->errinfo,kp->str,minc(kp->strlen,EXPR_SYMLEN));
						goto envp;
					}
					un.eb=expr_getbranchinfo(ep,NULL,0,NULL,0,sym.b,asym,asymlen);
				}else
				un.eb=expr_getbranchinfo(ep,p2,e-p2,e,p-e+1,NULL,asym,asymlen);
				break;
			case EXPR_DO:
				p=expr_findpair(e,endp);
				if(!p)goto pterr;
				if(p==e+1)goto envp;
				v0=expr_newvar(ep);
				cknp(ep,v0,return NULL);
				un.ep=new_expr8(e+1,p-e-1,asym,asymlen,ep->sset,ep->iflag,&ep->error,ep->errinfo);
				if(!un.ep)return NULL;
				expr_addop(ep,v0,un.ep,EXPR_DO,0);
				e=p+1;
				goto vend;
			case EXPR_VMD:
				un.ev=expr_getvmdinfo(ep,p2,e-p2,e,p-e+1,asym,asymlen,&flag);
				break;
			case SUMCASES:
				un.es=expr_getsuminfo(ep,p2,e-p2,e,p-e+1,asym,asymlen);
				break;
			default:
				abort();
		}
		if(!un.uaddr){
			return NULL;
		}
		v0=expr_newvar(ep);
		cknp(ep,v0,return NULL);
		expr_addop(ep,v0,un.uaddr,kp->op,flag);
		e=p+1;
		goto vend;
	}
	goto number;
found:
	switch(type){
		case EXPR_FUNCTION:
			if(p>=endp||*p!='('){
				memcpy(ep->errinfo,e,p-e);
				ep->error=EXPR_EFP;
				return NULL;
			}
			e=p;
			p=expr_findpair(e,endp);
			if(!p)goto pterr;
			if(p==e+1)goto envp;
			v0=expr_scan(ep,e+1,p,asym,asymlen);
			if(!v0)return NULL;
			expr_addcall(ep,v0,sv.sv->func,flag);
			e=p+1;
			break;
		case EXPR_ZAFUNCTION:
			if(p+1>=endp||*p!='('||p[1]!=')'){
				memcpy(ep->errinfo,e,p-e);
				ep->error=EXPR_EZAFP;
				return NULL;
			}
			v0=expr_newvar(ep);
			cknp(ep,v0,return NULL);
			expr_addza(ep,v0,sv.sv->zafunc,flag);
			e=p+2;
			break;
		case EXPR_HOTFUNCTION:
			if(p>=endp||*p!='('){
				memcpy(ep->errinfo,e,p-e);
				ep->error=EXPR_EFP;
				return NULL;
			}
			e=p;
			p=expr_findpair(e,endp);
			if(!p)goto pterr;
			if(p==e+1)goto envp;
			v0=expr_scan(ep,e+1,p,asym,asymlen);
			if(!v0)return NULL;
			p2=sv.sv->hotexpr+strlen(sv.sv->hotexpr);
			un.ep=new_expr8(sv.sv->hotexpr,p2-sv.sv->hotexpr,p2+1,
			strlen(p2+1),ep->sset,
			ep->iflag
			,&ep->error,ep->errinfo);
			if(!un.ep)return NULL;
			expr_addhot(ep,v0,un.ep,flag);
			e=p+1;
			break;
		case EXPR_CONSTANT:
			v0=expr_newvar(ep);
			cknp(ep,v0,return NULL);
			expr_addconst(ep,v0,sv.sv->value);
			goto treat_as_variable;
		case EXPR_VARIABLE:
			v0=expr_newvar(ep);
			cknp(ep,v0,return NULL);
			expr_addcopy(ep,v0,sv.sv->addr);
treat_as_variable:
			p2=e;
			e=p;
			if(e<endp&&*e=='('){
				switch(flag&~EXPR_SF_PMASK){
					case EXPR_SF_PMD:
						p=expr_findpair(e,endp);
						if(!p)goto pterr;
						un.em=expr_getmdinfo(ep,p2,e-p2,e,p-e+1,asym,asymlen,NULL,0,0);
						if(!un.em)return NULL;
						expr_addop(ep,v0,un.em,EXPR_PMD,0);
						e=p+1;
						goto vend;
					case EXPR_SF_PME:
						p=expr_findpair(e,endp);
						if(!p)goto pterr;
						un.em=expr_getmdinfo(ep,p2,e-p2,e,p-e+1,asym,asymlen,NULL,0,1+!!(flag&EXPR_SF_WRITEIP));
						if(!un.em)return NULL;
						expr_addop(ep,v0,un.em,(flag&EXPR_SF_WRITEIP)?EXPR_PMEP:EXPR_PME,0);
						e=p+1;
						goto vend;
					case EXPR_SF_PEP:
						p=expr_findpair(e,endp);
						if(!p)goto pterr;
						v1=expr_scan(ep,e+1,p,asym,asymlen);
						if(!v1)return NULL;
						expr_addop(ep,v1,v0,EXPR_EVAL,0);
						e=p+1;
						v0=v1;
						goto vend;
					default:
						break;
				}
				if(e+1<endp&&e[1]==')'){
					v1=expr_newvar(ep);
					cknp(ep,v1,return NULL);
					expr_addop(ep,v1,v0,EXPR_PZA,0);
					e+=2;
				}else {
					p=expr_findpair(e,endp);
					if(!p)goto pterr;
					v1=expr_scan(ep,e+1,p,asym,asymlen);
					if(!v1)return NULL;
					expr_addop(ep,v1,v0,EXPR_PBL,0);
					e=p+1;
				}
				v0=v1;
			}
			break;
		case EXPR_MDFUNCTION:
		case EXPR_MDEPFUNCTION:
			if(p>=endp||*p!='('){
				memcpy(ep->errinfo,e,p-e);
				ep->error=EXPR_EFP;
				return NULL;
			}
			p2=e;
			e=p;
			p=expr_findpair(e,endp);
			if(!p)goto pterr;
			switch(type){
				case EXPR_MDFUNCTION:
					un.em=expr_getmdinfo(ep,p2,e-p2,e,p-e+1,asym,asymlen,sv.sv->mdfunc,dim,0);
				break;
				case EXPR_MDEPFUNCTION:
					un.em=expr_getmdinfo(ep,p2,e-p2,e,p-e+1,asym,asymlen,sv.sv->mdepfunc,dim,1+!!(flag&EXPR_SF_WRITEIP));
				break;
			}
			if(!un.em){
				return NULL;
			}
			v0=expr_newvar(ep);
			cknp(ep,v0,return NULL);
			switch(type){
				case EXPR_MDFUNCTION:
					expr_addmd(ep,v0,un.em,flag);
					break;
				case EXPR_MDEPFUNCTION:
					(flag&EXPR_SF_WRITEIP?expr_addmep:expr_addme)
					(ep,v0,un.em,flag);
					break;
			}
			e=p+1;
			break;
		default:
			goto symerr;
		break;
	}
	goto vend;
number:
	p=expr_getsym_expo(e,endp);
	r0=expr_atod(e,p-e,&un.v);
	if(r0==1){
		v0=expr_newvar(ep);
		cknp(ep,v0,return NULL);
		expr_addconst(ep,v0,un.v);
		e=p;
		goto vend;
	}else if(r0>1){
		memcpy(ep->errinfo,e,minc(p-e,EXPR_SYMLEN));
		ep->error=EXPR_ENUMBER;
		return NULL;
	}
symerr:
	memcpy(ep->errinfo,e,p-e);
	ep->error=EXPR_ESYMBOL;
	return NULL;
vend:
	while(e<endp&&*e=='['){
		double *v2;
		p=expr_findpair_bracket(e,endp);
		if(!p)goto pterr;
		if(p==e+1)goto envp;
		v1=expr_scan(ep,e+1,p,asym,asymlen);
		if(!v1)return NULL;
		v2=v0;
		v0=expr_newvar(ep);
		cknp(ep,v0,return NULL);
		expr_addoff(ep,v2,v1);
		expr_addread(ep,v0,v2);
		e=p+1;
	}
	if(_p)*_p=e;
	return v0;
}
struct expr_vnode {
	struct expr_vnode *next;
	double *v;
	enum expr_op op;
	unsigned int unary;
};
static struct expr_vnode *expr_vn(double *v,enum expr_op op,unsigned int unary){
	struct expr_vnode *p;
	p=xmalloc_nullable(sizeof(struct expr_vnode));
	if(!p)return NULL;
	p->next=NULL;
	p->v=v;
	p->op=op;
	p->unary=unary;
	return p;
}
static struct expr_vnode *expr_vnadd(struct expr_vnode *vp,double *v,enum expr_op op,unsigned int unary){
	struct expr_vnode *p;
	if(!vp)return expr_vn(v,op,unary);
	for(p=vp;p->next;p=p->next);
	p->next=xmalloc_nullable(sizeof(struct expr_vnode));
	if(!p->next)return NULL;
	p->next->v=v;
	p->next->op=op;
	p->next->unary=unary;
	p->next->next=NULL;
	return vp;
}

static int expr_do_unary(struct expr *restrict ep,struct expr_vnode *ev,int prec){
	int r=0;;
	for(struct expr_vnode *p=ev;p;p=p->next){
redo:
	switch(prec){
		case 2:
			switch(p->unary&3){
				case 2:
					expr_addnotl(ep,p->v);
					break;
				case 3:
					expr_addnot(ep,p->v);
					break;
				default:
					continue;
			}
			r=1;
			p->unary>>=2;
			goto redo;
		case 4:
			switch(p->unary&3){
				case 1:
					expr_addneg(ep,p->v);
					break;
				default:
					continue;
			}
			r=1;
			p->unary>>=2;
			goto redo;
		default:
			continue;
	}
	}
	return r;
}
static void expr_vnunion(struct expr *restrict ep,struct expr_vnode *ev){
	struct expr_vnode *p;

	expr_addop(ep,ev->v,ev->next->v,ev->next->op,0);
	p=ev->next;
	ev->next=ev->next->next;

	xfree(p);
}
static void expr_vnfree(struct expr_vnode *vp){
	struct expr_vnode *p;
	while(vp){
		p=vp->next;
		xfree(vp);
		vp=p;
	}
}
//scan mark
static double *expr_scan(struct expr *restrict ep,const char *e,const char *endp,const char *asym,size_t asymlen){
	double *v1;
	const char *e0=e,*e1;
	enum expr_op op=0;
	unsigned int unary;
	struct expr_vnode *ev=NULL,*p;
	do {
	unary=0;
redo:
	switch(*e){
		case '-':
		case '!':
		case '~':
			if(unary&0xc0000000){
				if(e<endp&&expr_operator(*e)){
euo:
					*ep->errinfo=*e;
					ep->error=EXPR_EUO;
				}else {
eev:
					ep->error=EXPR_EEV;
				}
				goto err;
			}
		default:
			break;
	}
	switch(*e){
		case '-':
			unary=(unary<<2)|1;
			++e;
			if(e>=endp)goto eev;
			goto redo;
		case '!':
			unary=(unary<<2)|2;
			++e;
			if(e>=endp)goto eev;
			goto redo;
		case '~':
			unary=(unary<<2)|3;
			++e;
			if(e>=endp)goto eev;
			goto redo;
	}
	v1=expr_getvalue(ep,e,endp,&e,asym,asymlen);
	if(!v1)goto err;
	p=expr_vnadd(ev,v1,op,unary);
	cknp(ep,p,goto err);
	ev=p;
	if(e>=endp)goto end2;
	op=EXPR_END;
rescan:
	switch(*e){
		case '>':
			++e;
			if(e<endp&&*e=='='){
				++e;
				if(e<endp&&*e=='='){
					++e;
					op=EXPR_GE;
				}else op=EXPR_SGE;
			}else if(e<endp&&*e=='>'){
				op=EXPR_SHR;
				++e;
			}else if(e<endp&&*e=='<'){
				op=EXPR_SNE;
				++e;
			}else op=EXPR_GT;
			goto end1;
		case '<':
			++e;
			if(e<endp&&*e=='='){
				++e;
				if(e<endp&&*e=='='){
					++e;
					op=EXPR_LE;
				}else op=EXPR_SLE;
			}else if(e<endp&&*e=='<'){
				op=EXPR_SHL;
				++e;
			}else if(e<endp&&*e=='>'){
				op=EXPR_SNE;
				++e;
			}else op=EXPR_LT;
			goto end1;
		case '=':
			++e;
			if(e<endp&&*e=='='){
				op=EXPR_EQ;
				++e;
			}else op=EXPR_SEQ;
			goto end1;
		case '!':
			++e;
			if(e<endp&&*e=='='){
				++e;
				op=EXPR_NE;
			}else op=EXPR_SNE;
			goto end1;
		case '&':
			++e;
			if(e<endp&&*e=='&'){
				op=EXPR_ANDL;
				++e;
			}
			else op=EXPR_AND;
			goto end1;
		case '|':
			++e;
			if(e<endp&&*e=='|'){
				op=EXPR_ORL;
				++e;
			}
			else op=EXPR_OR;
			goto end1;
		case '+':
			op=EXPR_ADD;
			++e;
			goto end1;
		case '-':
			if(e+1<endp&&e[1]=='>'){
				const struct expr_symbol *esp=NULL;
				const char *p1;
				double *v2;
				e+=2;
				p1=expr_getsym(e,endp);
				if(p1==e){
					switch(*e){
						case '[':
						p1=expr_findpair_bracket(e,endp);
						if(!p1)goto pterr;
						if(p1==e+1){
envp:
							ep->error=EXPR_ENVP;
							goto err;
						}
						if(p1+1<endp&&p1[1]=='['){
							e1=e;
							goto multi_dim;
						}
						v2=expr_scan(ep,e+1,p1,asym,asymlen);
						if(!v2)goto err;
						expr_addwrite(ep,v1,v2);
						e=p1+1;
						goto bracket_end;
						case '(':
						p1=expr_findpair(e,endp);
						if(p1+1>=endp||p1[1]!='[')
							break;
						e1=e;
						goto multi_dim;
						case '&':
						if(e+1>=endp)
							break;
						p1=expr_getsym(e+1,endp);
						if(p1==e+1||*p1!='[')break;
						e1=e;
						--p1;
						goto multi_dim;
						default:
						break;
					}
					if(expr_operator(*e)){
						*ep->errinfo=*e;
						ep->error=EXPR_EUO;
					}else {
						ep->error=EXPR_EEV;
					}
				goto err;
				}
				if(p1-e==asymlen&&!memcmp(e,asym,p1-e))
					goto tnv;
				if(ep->sset)
				esp=expr_symset_search(ep->sset,e,p1-e);
				if(!esp){
					if(expr_builtin_symbol_search(e,p1-e))
						goto tnv;
					ep->error=EXPR_ESYMBOL;
					memcpy(ep->errinfo,e,p1-e);
					goto err;
				}
				e1=e;
				e=p1;
				if(*e=='['){
					double *v3;
					p1=expr_findpair_bracket(e,endp);
					if(!p1)goto pterr;
					if(p1==e+1)goto envp;
					while(p1+1<endp&&p1[1]=='['){
multi_dim:
							e=p1+1;
							p1=expr_findpair_bracket(e,endp);
							if(!p1)goto pterr;
							if(p1==e+1)goto envp;
					}
					v3=expr_getvalue(ep,e1,e,NULL,asym,asymlen);
					cknp(ep,v3,goto err);
					v2=expr_scan(ep,e+1,p1,asym,asymlen);
					if(!v2)goto err;
					expr_addoff(ep,v3,v2);
					expr_addwrite(ep,v1,v3);
					e=p1+1;
					goto bracket_end;
				}
				if(esp->type!=EXPR_VARIABLE){
tnv:
					ep->error=EXPR_ETNV;
					memcpy(ep->errinfo,e,p1-e);
					goto err;
				}
				expr_addcopy(ep,esp->un.addr,v1);
bracket_end:
				if(e>=endp)continue;
				goto rescan;
			}else if(e+2<endp&&e[1]=='-'&&e[2]=='>'){
				const char *p1;
				double *v2;
				e+=3;
				p1=expr_getsym(e,endp);
				if(p1==e){
					if(*e&&expr_operator(*e)){
						*ep->errinfo=*e;
						ep->error=EXPR_EUO;
					}else {
						ep->error=EXPR_EEV;
					}
				goto err;
				}
				if((p1-e==asymlen&&!memcmp(e,asym,p1-e))
				//||expr_builtin_symbol_search(e,p1-e)
				||(ep->sset&&expr_symset_search(ep->sset,e,p1-e))
				){
					ep->error=EXPR_EDS;
					memcpy(ep->errinfo,e,p1-e);
					goto err;
				}
				v2=expr_createvar(ep,e,p1-e);
				cknp(ep,v2,goto err);
				expr_addcopy(ep,v2,v1);
				e=p1;
				if(e>=endp)continue;
				goto rescan;
			}else {
				op=EXPR_SUB;
				++e;
			}
			goto end1;
		case '*':
			op=EXPR_MUL;
			++e;
			goto end1;
		case '/':
			op=EXPR_DIV;
			++e;
			goto end1;
		case '%':
			op=EXPR_MOD;
			++e;
			goto end1;
		case '^':
			++e;
			if(e<endp&&*e=='^'){
				++e;
				if(e<endp&&*e=='^'){
					op=EXPR_XORL;
					++e;
				}else {
					op=EXPR_XOR;
				}
			}else op=EXPR_POW;
			goto end1;
		case '#':
			++e;
			if(e<endp&&*e=='#'){
				op=EXPR_DIFF;
				++e;
			}
			else op=EXPR_NEXT;
			goto end1;
		case ';':
			if(e+1==endp)continue;
		case ',':
			op=EXPR_COPY;
			++e;
			goto end1;
		case ')':
			if(!expr_unfindpair(e0,e)){
pterr:
				ep->error=EXPR_EPT;
				goto err;
			}
		default:
			if(expr_operator(*e))goto euo;
			ep->error=EXPR_EUSN;
			memcpy(ep->errinfo,e,minc(expr_getsym(e,endp)-e,EXPR_SYMLEN));
			goto err;
	}
end1:
		if(e>=endp&&op!=EXPR_END)goto eev;
		continue;	
	}while(op!=EXPR_END);
end2:
#define SETPREC1(a)\
	for(p=ev;p;p=p->next){\
		while(p->next&&p->next->op==(a)){\
			expr_vnunion(ep,p);\
		}\
	}
#define SETPREC2(a,b)\
	for(p=ev;p;p=p->next){\
		while(p->next&&(\
			p->next->op==(a)\
			||p->next->op==(b)\
			)){\
			expr_vnunion(ep,p);\
		}\
	}
#define SETPREC3(a,b,c)\
	for(p=ev;p;p=p->next){\
		while(p->next&&(\
			p->next->op==(a)\
			||p->next->op==(b)\
			||p->next->op==(c)\
			)){\
			expr_vnunion(ep,p);\
		}\
	}
#define SETPREC4(a,b,c,d)\
	for(p=ev;p;p=p->next){\
		while(p->next&&(\
			p->next->op==(a)\
			||p->next->op==(b)\
			||p->next->op==(c)\
			||p->next->op==(d)\
			)){\
			expr_vnunion(ep,p);\
		}\
	}
#define SETPREC6(a,b,c,d,e,f)\
	for(p=ev;p;p=p->next){\
		while(p->next&&(\
			p->next->op==(a)\
			||p->next->op==(b)\
			||p->next->op==(c)\
			||p->next->op==(d)\
			||p->next->op==(e)\
			||p->next->op==(f)\
			)){\
			expr_vnunion(ep,p);\
		}\
	}
	expr_do_unary(ep,ev,2);
	SETPREC1(EXPR_POW)
	expr_do_unary(ep,ev,4);
	while(expr_do_unary(ep,ev,2)&&expr_do_unary(ep,ev,4));
	SETPREC3(EXPR_MUL,EXPR_DIV,EXPR_MOD)
	SETPREC2(EXPR_ADD,EXPR_SUB)
	SETPREC2(EXPR_SHL,EXPR_SHR)
	SETPREC1(EXPR_NEXT)
	SETPREC1(EXPR_DIFF)
	SETPREC6(EXPR_LT,EXPR_LE,EXPR_GT,EXPR_GE,EXPR_SLE,EXPR_SGE)
	SETPREC4(EXPR_SEQ,EXPR_SNE,EXPR_EQ,EXPR_NE)
	SETPREC1(EXPR_AND)
	SETPREC1(EXPR_XOR)
	SETPREC1(EXPR_OR)
	SETPREC1(EXPR_ANDL)
	SETPREC1(EXPR_XORL)
	SETPREC1(EXPR_ORL)
	for(p=ev;p->next;p=p->next);
	v1=p->v;
	expr_vnfree(ev);
	return v1;
err:
	if(ev)expr_vnfree(ev);
	return NULL;
}
void init_expr_symset(struct expr_symset *restrict esp){
	/*esp->syms=NULL;
	esp->size=0;
	esp->depth=0;
	esp->length=0;
	esp->freeable=0;*/
	memset(esp,0,sizeof(struct expr_symset));
}
struct expr_symset *new_expr_symset(void){
	struct expr_symset *ep=xmalloc(sizeof(struct expr_symset));
	init_expr_symset(ep);
	ep->freeable=1;
	return ep;
}

static void expr_symbol_free(struct expr_symbol *restrict esp){
	for(int i=0;i<EXPR_SYMNEXT;++i){
		if(!esp->next[i])continue;
		expr_symbol_free(esp->next[i]);
	}
	xfree(esp);
}
void expr_symset_free(struct expr_symset *restrict esp){
	if(esp->syms)
		expr_symbol_free(esp->syms);
	if(esp->freeable)xfree(esp);
}
void expr_symset_wipe(struct expr_symset *restrict esp){
	expr_symset_free(esp);
	init_expr_symset(esp);
}
static int expr_firstdiff(const char *restrict s1,const char *restrict s2,size_t len){
	int r;
	do {
		r=(unsigned int)*(s1++)-(unsigned int)*(s2++);
		if(r)break;
	}while(--len);
	return r;
}
static int expr_strdiff(const char *restrict s1,size_t len1,const char *restrict s2,size_t len2,int *sum){
	int r;
	if(len1==len2){
		r=memcmp(s1,s2,len1);
		if(!r)return 0;
		*sum=expr_firstdiff(s1,s2,len1);
		return 1;
	}
	*sum=expr_firstdiff(s1,s2,len1<len2?len1:len2);
	return 1;
}
#define modi(d,m){\
	int tmpvar;\
	tmpvar=(d)%(m);\
	if((d)>=0||!tmpvar)\
		(d)=tmpvar;\
	else\
		(d)=((d)+((-(d))/(m)+!!tmpvar)*(m))%(m);\
}
static struct expr_symbol **expr_symset_findtail(struct expr_symset *restrict esp,const char *sym,size_t symlen,size_t *depth){
	struct expr_symbol *p;
	size_t dep;
	int r;
	if(!esp->syms){
		*depth=1;
		return &esp->syms;
	}
	dep=2;
	for(p=esp->syms;;++dep){
		if(!expr_strdiff(sym,symlen,p->str,p->strlen,&r))return NULL;
		modi(r,EXPR_SYMNEXT)
		if(p->next[r]){
			p=p->next[r];
		}else {
			*depth=dep;
			return p->next+r;
		}
	}
}
struct expr_symbol *expr_symset_add(struct expr_symset *restrict esp,const char *sym,int type,...){
	va_list ap;
	struct expr_symbol *r;
	va_start(ap,type);
	r=expr_symset_vadd(esp,sym,type,ap);
	va_end(ap);
	return r;
}
struct expr_symbol *expr_symset_addl(struct expr_symset *restrict esp,const char *sym,size_t symlen,int type,...){
	va_list ap;
	struct expr_symbol *r;
	va_start(ap,type);
	r=expr_symset_vaddl(esp,sym,symlen,type,ap);
	va_end(ap);
	return r;
}
struct expr_symbol *expr_symset_vadd(struct expr_symset *restrict esp,const char *sym,int type,va_list ap){
	return expr_symset_vaddl(esp,sym,strlen(sym),type,ap);
}
struct expr_symbol *expr_symset_vaddl(struct expr_symset *restrict esp,const char *sym,size_t symlen,int type,va_list ap){
	struct expr_symbol *ep,**next;
	size_t len,len_expr,len_asym,depth;
	char *asymp;
	const char *p,*p1;
	if(!symlen)return NULL;
	if(symlen>=EXPR_SYMLEN)symlen=EXPR_SYMLEN-1;
	next=expr_symset_findtail(esp,sym,symlen,&depth);
	if(!next)return NULL;
	len=sizeof(struct expr_symbol)+symlen+1;
	ep=xmalloc(len);
	memset(ep->next,0,sizeof(ep->next));
	ep->length=len;
	memcpy(ep->str,sym,symlen);
	ep->str[symlen]=0;
	ep->strlen=symlen;
	switch(type){
		case EXPR_CONSTANT:
			ep->un.value=va_arg(ap,double);
			break;
		case EXPR_VARIABLE:
			ep->un.addr=va_arg(ap,double *);
			break;
		case EXPR_FUNCTION:
			ep->un.func=va_arg(ap,double (*)(double));
			break;
		case EXPR_MDFUNCTION:
			ep->un.mdfunc=va_arg(ap,double (*)(size_t,double *));
			ep->length+=sizeof(size_t);
			ep=xrealloc(ep,ep->length);
			SYMDIM(ep)
			=va_arg(ap,size_t);
			break;
		case EXPR_MDEPFUNCTION:
			ep->un.mdepfunc=va_arg(ap,double (*)(size_t,
				const struct expr *,double));
			ep->length+=sizeof(size_t);
			ep=xrealloc(ep,ep->length);
			SYMDIM(ep)
			=va_arg(ap,size_t);
			break;
		case EXPR_HOTFUNCTION:
			p=va_arg(ap,const char *);
			p1=va_arg(ap,const char *);
			len_expr=strlen(p);
			len_asym=strlen(p1)+2;
			ep->length+=len_expr+len_asym;
			ep=xrealloc(ep,ep->length);
			ep->un.hotexpr=ep->str+symlen+1;
			asymp=ep->un.hotexpr+len_expr+1;
			memcpy(ep->un.hotexpr,p,len_expr);
			ep->un.hotexpr[len_expr]=0;
			memcpy(asymp,p1,len_asym);
			asymp[len_asym]=0;
			break;
		case EXPR_ZAFUNCTION:
			ep->un.zafunc=va_arg(ap,double (*)(void));
			break;
		default:
			xfree(ep);
			return NULL;
	}

	ep->type=type;
	ep->flag=0;
	++esp->size;
	esp->length+=ep->length;
	if(depth>esp->depth)esp->depth=depth;
	*next=ep;
	return ep;
}
struct expr_symbol *expr_symset_addcopy(struct expr_symset *restrict esp,const struct expr_symbol *restrict es){
	size_t depth;
	struct expr_symbol *restrict *tail=expr_symset_findtail(esp,es->str,es->strlen,&depth);
	struct expr_symbol *ep;
	if(tail){
		ep=xmalloc(es->length);
	}else {
		return NULL;
	}
	memcpy(ep,es,es->length);
	memset(ep->next,0,sizeof(ep->next));
	switch(es->type){
		case EXPR_HOTFUNCTION:
			ep->un.hotexpr=ep->str+(es->un.hotexpr-es->str);
			break;
		default:
			break;
	}
	*tail=ep;
	++esp->size;
	esp->length+=es->length;
	if(depth>esp->depth)esp->depth=depth;
	return ep;
}
struct expr_symbol *expr_symset_search(const struct expr_symset *restrict esp,const char *sym,size_t sz){
	int r;
	for(struct expr_symbol *p=esp->syms;p;){
		if(!expr_strdiff(sym,sz,p->str,p->strlen,&r)){
			return p;
		}
		modi(r,EXPR_SYMNEXT)
		p=p->next[r];
	}
	return NULL;
}
static struct expr_symbol *expr_symset_rsearch_symbol(struct expr_symbol *esp,void *addr){
	struct expr_symbol *p;
	if(esp->un.uaddr==addr)return esp;
	for(int i=0;i<EXPR_SYMNEXT;++i){
		if(!esp->next[i])continue;
		p=expr_symset_rsearch_symbol(esp->next[i],addr);
		if(p)return p;
	}
	return NULL;
}
struct expr_symbol *expr_symset_rsearch(const struct expr_symset *restrict esp,void *addr){
	if(!esp->syms)return NULL;
	return expr_symset_rsearch_symbol(esp->syms,addr);
}
static void expr_symset_copy_symbol(struct expr_symset *restrict dst,const struct expr_symbol *restrict src){
	expr_symset_addcopy(dst,src);
	for(int i=0;i<EXPR_SYMNEXT;++i){
		if(!src->next[i])continue;
		expr_symset_copy_symbol(dst,src->next[i]);
	}
}
void expr_symset_copy(struct expr_symset *restrict dst,const struct expr_symset *restrict src){
	if(src&&src->syms)
		expr_symset_copy_symbol(dst,src->syms);
}
struct expr_symset *expr_symset_clone(const struct expr_symset *restrict ep){
	struct expr_symset *es=new_expr_symset();
	expr_symset_copy(es,ep);
	return es;
}
static char *expr_stpcpy_nospace(char *restrict s1,const char *restrict s2,const char *endp){
	const char *s20=s2;
	int instr=0;
	for(;s2<endp;++s2){
		if(*s2=='\"'&&s2>s20&&s2[-1]!='\\')instr^=1;
		if(!instr&&expr_space(*s2))continue;
		*(s1++)=*s2;
	}
	*s1=0;
	return s1;
}

static int expr_usesrc(enum expr_op op);
/*
static void expr_remove_unused_vars(struct expr *restrict ep){
	size_t i,ci;
	double *v;
	for(i=0;i<ep->vsize;++i){
		v=ep->vars[i];
		for(struct expr_inst *ip=ep->data;;++ip){
			if(ip->dst.uaddr==v||
			(expr_usesrc(ip->op)&&ip->un.uaddr==v))
				goto force_continue;
			if(ip->op==EXPR_END)
				break;
		}
		free(v);
		ep->vars[i]=NULL;
force_continue:
		continue;
	}
	for(i=0,ci=0;i<ep->vsize;++i){
		if(!ep->vars[i]){
			continue;
		}
		if(ci==i){
			++ci;
			continue;
		}
		ep->vars[ci++]=ep->vars[i];
	}
	ep->vsize=ci;
}
a bug cannot fix
*/
static void expr_optimize(struct expr *restrict ep);
static int expr_constexpr(const struct expr *restrict ep,double *except);
int expr_isconst(const struct expr *restrict ep){
	return expr_constexpr(ep,NULL);
}
int init_expr_const(struct expr *restrict ep,double val){
	double *v;
	memset(ep,0,sizeof(struct expr));
	v=expr_newvar(ep);
	if(!v){
		expr_free(ep);
		ep->error=EXPR_EMEM;
		return -1;
	}
	*v=val;
	expr_addend(ep,v);
	return 0;
}
struct expr *new_expr_const(double val){
	struct expr *r=xmalloc_nullable(sizeof(struct expr));
	if(!r)return NULL;
	if(init_expr_const(r,val)<0){
		xfree(r);
		return NULL;
	}
	r->freeable=1;
	return r;
}
static int init_expr8(struct expr *restrict ep,const char *e,size_t len,const char *asym,size_t asymlen,struct expr_symset *esp,int flag,struct expr *parent){
	union {
		double *p;
		double v;
	} un;
	char *ebuf,*r,*p0;
	/*ep->data=NULL;
	ep->vars=NULL;
	ep->sset_shouldfree=0;
	ep->error=0;
	ep->freeable=0;
	memset(ep->errinfo,0,EXPR_SYMLEN);
	ep->length=ep->size=ep->vlength=ep->vsize=0;*/
	memset(ep,0,sizeof(struct expr));
	ep->sset=esp;
	ep->parent=parent;
	ep->iflag=flag&~EXPR_IF_EXTEND_MASK;
	if(e){
		p0=xmalloc_nullable(len+1);
		ebuf=p0?p0:alloca(len+1);
		r=expr_stpcpy_nospace(ebuf,e,e+len);
		un.p=expr_scan(ep,ebuf,r,asym,asym?asymlen:0);
		if(p0)xfree(p0);
		if(ep->sset_shouldfree){
			expr_symset_free(ep->sset);
			ep->sset=esp;
		}
		if(un.p){
			expr_addend(ep,un.p);
		}else {
			expr_free(ep);
			ep->errinfo[EXPR_SYMLEN-1]=0;
			return -1;
		}
	}
	if(!(flag&EXPR_IF_NOOPTIMIZE)){
		expr_optimize(ep);
		if(expr_isconst(ep)){
			un.v=expr_eval(ep,0.0);
			expr_free(ep);
			if(init_expr_const(ep,un.v)<0)
				return -1;
		}
	}
	if(flag&EXPR_IF_INSTANT_FREE){
		expr_free(ep);
	}
	//expr_remove_unused_vars(ep);
	return 0;
}
int init_expr7(struct expr *restrict ep,const char *e,size_t len,const char *asym,size_t asymlen,struct expr_symset *esp,int flag){
	return init_expr8(ep,e,len,asym,asymlen,esp,flag,NULL);
}
int init_expr5(struct expr *restrict ep,const char *e,const char *asym,struct expr_symset *esp,int flag){
	return init_expr7(ep,e,e?strlen(e):0,asym,asym?strlen(asym):0,esp,flag);
}
int init_expr(struct expr *restrict ep,const char *e,const char *asym,struct expr_symset *esp){
	return init_expr5(ep,e,asym,esp,0);
}
static struct expr *new_expr10(const char *e,size_t len,const char *asym,size_t asymlen,struct expr_symset *esp,int flag,int n,int *error,char errinfo[EXPR_SYMLEN],struct expr *parent){
	struct expr *ep,*ep0;
	if(n<1)n=1;
	ep=ep0=xmalloc_nullable(n*sizeof(struct expr));
	if(!ep){
		if(error)*error=EXPR_EMEM;
		if(errinfo)memset(errinfo,0,EXPR_SYMLEN);
		return NULL;
	}
	do if(init_expr8(ep,e,len,asym,asymlen,esp,flag,parent)<0){
		if(error)*error=ep->error;
		if(errinfo)memcpy(errinfo,ep->errinfo,EXPR_SYMLEN);
		if(!(flag&EXPR_IF_INSTANT_FREE))while(--ep>=ep0){
			expr_free(ep);
		}
		xfree(ep0);
		return NULL;
	}else {
		ep->freeable=2;
		++ep;
	}while(--n);
	ep[-1].freeable=1;
	if(flag&EXPR_IF_INSTANT_FREE)
		xfree(ep0);
	return ep0;
}
struct expr *new_expr9(const char *e,size_t len,const char *asym,size_t asymlen,struct expr_symset *esp,int flag,int n,int *error,char errinfo[EXPR_SYMLEN]){
	return new_expr10(e,len,asym,asymlen,esp,flag,n,error,errinfo,NULL);
}
struct expr *new_expr7(const char *e,const char *asym,struct expr_symset *esp,int flag,int n,int *error,char errinfo[EXPR_SYMLEN]){
	return new_expr9(e,e?strlen(e):0,asym,asym?strlen(asym):0,esp,flag,n,error,errinfo);
}
struct expr *new_expr8(const char *e,size_t len,const char *asym,size_t asymlen,struct expr_symset *esp,int flag,int *error,char errinfo[EXPR_SYMLEN]){
	return new_expr9(e,len,asym,asymlen,esp,flag,1,error,errinfo);
}
struct expr *new_expr6(const char *e,const char *asym,struct expr_symset *esp,int flag,int *error,char errinfo[EXPR_SYMLEN]){
	return new_expr7(e,asym,esp,flag,1,error,errinfo);
}
struct expr *new_expr(const char *e,const char *asym,struct expr_symset *esp,int *error,char errinfo[EXPR_SYMLEN]){
	return new_expr7(e,asym,esp,0,1,error,errinfo);
}
double expr_calc5(const char *e,const char *asym,double input,struct expr_symset *esp,int flag){
	struct expr ep[1];
	double r;
	flag&=~EXPR_IF_INSTANT_FREE;
	if(init_expr5(ep,e,asym,esp,flag)<0)
		return NAN;
	r=expr_eval(ep,input);
	expr_free(ep);
	return r;
}
double expr_calc4(const char *e,const char *asym,double input,struct expr_symset *esp){
	return expr_calc5(e,asym,input,esp,0);
}
double expr_calc3(const char *e,const char *asym,double input){
	return expr_calc5(e,asym,input,NULL,0);
}
double expr_calc(const char *e){
	return expr_calc5(e,NULL,0.0,NULL,0);
}
static size_t expr_varofep(const struct expr *restrict ep,double *v){
	for(size_t i=0;i<ep->vsize;++i){
		if(ep->vars[i]==v){
			return i+1;
		}
	}
	return 0;
}
static void expr_writeconsts(struct expr *restrict ep){
	for(struct expr_inst *ip=ep->data;ip->op!=EXPR_END;++ip){
		if(ip->op==EXPR_CONST&&ip->dst.dst&&
			expr_varofep(ep,ip->dst.dst)
			){
			*ip->dst.dst=ip->un.value;
		}
	}
}


static void expr_optimize_completed(struct expr *restrict ep){
	struct expr_inst *cip=ep->data;
	for(struct expr_inst *ip=cip;ip-ep->data<ep->size;++ip){
		if(ip->dst.dst){
			if(ip>cip)
				memcpy(cip,ip,sizeof(struct expr_inst));
			++cip;
		}
	}
	ep->size=cip-ep->data;
}
static int expr_modified(const struct expr *restrict ep,double *v){
	if(!expr_varofep(ep,v))
		return 1;
	for(struct expr_inst *ip=ep->data;ip->op!=EXPR_END;++ip){
		if(ip->dst.dst==v&&ip->op!=EXPR_CONST){
			return 1;
		}
	}
	return 0;
}
static struct expr_inst *expr_findconst(const struct expr *restrict ep,struct expr_inst *ip){
	double *s=ip->dst.dst;
	for(--ip;ip>=ep->data;--ip){
		if(expr_usesrc(ip->op)&&ip->un.src==s)break;
		if(ip->dst.dst!=s)continue;
		if(ip->op==EXPR_CONST)return ip;
		else break;
	}
	return NULL;
}
static void expr_optimize_contadd(struct expr *restrict ep){
	double sum;
	struct expr_inst *rip;
	for(struct expr_inst *ip=ep->data;ip->op!=EXPR_END;++ip){
		if(!expr_modified(ep,ip->dst.dst)
			||(ip->op!=EXPR_ADD&&ip->op!=EXPR_SUB)
			||expr_modified(ep,ip->un.src)
			)continue;
		sum=ip->op==EXPR_ADD?*ip->un.src:-*ip->un.src;
		for(struct expr_inst *ip1=ip+1;ip1->op!=EXPR_END;++ip1){
			if(ip1->dst.dst!=ip->dst.dst)continue;
			if(ip1->op!=EXPR_ADD
				&&ip1->op!=EXPR_SUB
				)break;
			if(!expr_modified(ep,ip1->un.src)){
				if(ip1->op==EXPR_ADD)
					sum+=*ip1->un.src;
				else
					sum-=*ip1->un.src;
				ip1->dst.dst=NULL;
			}
		}
		rip=expr_findconst(ep,ip);
		if(rip){
			rip->un.value+=sum;
			ip->dst.dst=NULL;
			expr_writeconsts(ep);
		}else {
			/*if(sum<0.0){
				*ip->un.src=-sum;
				ip->op=EXPR_SUB;
			}
			else */{
				*ip->un.src=sum;
				ip->op=EXPR_ADD;
			}
		}
		continue;
	}
	expr_optimize_completed(ep);
}
static void expr_optimize_contsh(struct expr *restrict ep){
	double sum;
	struct expr_inst *rip;
	for(struct expr_inst *ip=ep->data;ip->op!=EXPR_END;++ip){
		if(!expr_modified(ep,ip->dst.dst)
			||(ip->op!=EXPR_SHL&&ip->op!=EXPR_SHR)
			||expr_modified(ep,ip->un.src)
			)continue;
		sum=ip->op==EXPR_SHL?*ip->un.src:-*ip->un.src;
		for(struct expr_inst *ip1=ip+1;ip1->op!=EXPR_END;++ip1){
			if(ip1->dst.dst!=ip->dst.dst)continue;
			if(ip1->op!=EXPR_SHL
				&&ip1->op!=EXPR_SHR
				)break;
			if(!expr_modified(ep,ip1->un.src)){
				if(ip1->op==EXPR_SHL)
					sum+=*ip1->un.src;
				else
					sum-=*ip1->un.src;
				ip1->dst.dst=NULL;
			}
		}
		rip=expr_findconst(ep,ip);
		if(rip){
			EXPR_EDEXP(&rip->un.value)+=(int64_t)sum;
			ip->dst.dst=NULL;
			expr_writeconsts(ep);
		}else {
			/*if(sum<0.0){
				*ip->un.src=-sum;
				ip->op=EXPR_SHR;
			}
			else */{
				*ip->un.src=sum;
				ip->op=EXPR_SHL;
			}
		}
		continue;
	}
	expr_optimize_completed(ep);
}
static void expr_optimize_contmul(struct expr *restrict ep,enum expr_op op){
	double sum;
	struct expr_inst *rip;
	for(struct expr_inst *ip=ep->data;ip->op!=EXPR_END;++ip){
		if(!ip->dst.dst
			||!expr_modified(ep,ip->dst.dst)
			||ip->op!=op
			||expr_modified(ep,ip->un.src)
			)continue;
		rip=expr_findconst(ep,ip);
		if(rip){
			sum=rip->un.value;
		}else {
			switch(op){
				case EXPR_POW:
				case EXPR_MOD:
				case EXPR_LT:
				case EXPR_LE:
				case EXPR_SLE:
				case EXPR_SGE:
				case EXPR_GT:
				case EXPR_GE:
				case EXPR_EQ:
				case EXPR_SEQ:
				case EXPR_SNE:
				case EXPR_NE:
				case EXPR_NEXT:
				case EXPR_DIFF:
				case EXPR_OFF:
					continue;
				default:
					break;
			}
			sum=*ip->un.src;
		}
		for(struct expr_inst *ip1=ip+!rip;ip1->op!=EXPR_END;++ip1){
			if(ip1->dst.dst!=ip->dst.dst)continue;
			if(ip1->op!=op)break;
			if(!expr_modified(ep,(double *)ip1->un.src)){
			switch(op){
				case EXPR_ADD:
					sum+=*ip1->un.src;
					break;
				case EXPR_SUB:
					if(rip)sum-=*ip1->un.src;
					else sum+=*ip1->un.src;
					break;
				case EXPR_MUL:
					sum*=*ip1->un.src;
					break;
				case EXPR_DIV:
					if(rip)sum/=*ip1->un.src;
					else sum*=*ip1->un.src;
					break;
				case EXPR_MOD:
					sum=fmod(sum,*ip1->un.src);
					break;
				case EXPR_AND:
					sum=and2(sum,*ip1->un.src);
					break;
				case EXPR_OR:
					sum=or2(sum,*ip1->un.src);
					break;
				case EXPR_XOR:
					sum=xor2(sum,*ip1->un.src);
					break;
				case EXPR_POW:
					sum=pow(sum,*ip1->un.src);
					break;
				case EXPR_COPY:
					sum=*ip1->un.src;
					break;
				case EXPR_GT:
					sum=sum>*ip1->un.src?
						1.0:
						0.0;
					break;
				case EXPR_LT:
					sum=sum<*ip1->un.src?
						1.0:
						0.0;
					break;
				case EXPR_SGE:
					sum=sum>=*ip1->un.src?
						1.0:
						0.0;
					break;
				case EXPR_SLE:
					sum=sum<=*ip1->un.src?
						1.0:
						0.0;
					break;
				case EXPR_GE:
					sum=sum>=*ip1->un.src
					||expr_equal(sum,*ip1->un.src)?
						1.0:
						0.0;
					break;
				case EXPR_LE:
					sum=sum<=*ip1->un.src
					||expr_equal(sum,*ip1->un.src)?
						1.0:
						0.0;
					break;
				case EXPR_SEQ:
					sum=sum==*ip1->un.src?
						1.0:
						0.0;
					break;
				case EXPR_SNE:
					sum=sum!=*ip1->un.src?
						1.0:
						0.0;
					break;
				case EXPR_EQ:
					sum=expr_equal(sum,*ip1->un.src)?
						1.0:
						0.0;
					break;
				case EXPR_NE:
					sum=!expr_equal(sum,*ip1->un.src)?
						1.0:
						0.0;
					break;
				case EXPR_ANDL:
					sum=LOGIC(sum,*ip1->un.src,&&)?
						1.0:
						0.0;
					break;
				case EXPR_ORL:
					sum=LOGIC(sum,*ip1->un.src,||)?
						1.0:
						0.0;
					break;
				case EXPR_XORL:
					sum=LOGIC(sum,*ip1->un.src,^)?
						1.0:
						0.0;
					break;
				case EXPR_NEXT:
					EXPR_EDIVAL(&sum)+=(int64_t)*ip1->un.src;
					break;
				case EXPR_DIFF:
					sum=(double)(EXPR_EDIVAL(&sum)-*ip->un.isrc);
					break;
				default:
					abort();
			}
				ip1->dst.dst=NULL;
			}
		}
		if(rip){
			rip->un.value=sum;
			ip->dst.dst=NULL;
			expr_writeconsts(ep);
		}else {
			*ip->un.src=sum;
		}
		continue;
	}
	expr_optimize_completed(ep);
}
static double expr_zero_element(enum expr_op op){
	switch(op){
		case EXPR_ADD:
		case EXPR_SUB:
		case EXPR_OR:
		case EXPR_XOR:
		case EXPR_SHL:
		case EXPR_SHR:
		case EXPR_NEXT:
		case EXPR_OFF:
			return 0.0;
		case EXPR_MUL:
		case EXPR_DIV:
		case EXPR_MOD:
		case EXPR_POW:
			return 1.0;
		default:
			return -1.0;
	}
}
static int expr_optimize_zero(struct expr *restrict ep){
	double ze;
	int r=0;
	for(struct expr_inst *ip=ep->data;ip->op!=EXPR_END;++ip){
		switch(ip->op){
			case EXPR_ANDL:
				if(*ip->un.src==0.0){
					ip->op=EXPR_CONST;
					ip->un.value=0.0;
					expr_writeconsts(ep);
					r=1;
				}
				break;
			case EXPR_ORL:
				if(*ip->un.src!=0.0){
					ip->op=EXPR_CONST;
					ip->un.value=1.0;
					expr_writeconsts(ep);
					r=1;
				}
				break;
			default:
				ze=expr_zero_element(ip->op);
				if(ze<0.0
					||expr_modified(ep,ip->un.src)
					)continue;
				if(ze==*ip->un.src){
					ip->dst.dst=NULL;
					r=1;
				}
				break;
		}
	}
	return r;
}
//oconst
static void expr_optimize_const(struct expr *restrict ep){
	for(struct expr_inst *ip=ep->data;ip->op!=EXPR_END;++ip){
		if(ip->op==EXPR_CONST&&!expr_modified(ep,ip->dst.dst)){
			/*if(!(ip->flag&EXPR_SF_INJECTION)){
				*ip->dst.dst=ip->un.value;
			}*/
			ip->dst.dst=NULL;
		}
	}
	expr_optimize_completed(ep);
}
static int expr_side(enum expr_op op){
	switch(op){
		case SRCCASES:
		case EXPR_INPUT:
		case EXPR_CONST:
		case EXPR_NEG:
		case EXPR_NOT:
		case EXPR_NOTL:
		case EXPR_TSTL:
		case EXPR_ALO:
			return 0;
		default:
			return 1;
	}
}
static int expr_override(enum expr_op op){
	switch(op){
		case EXPR_COPY:
		case EXPR_INPUT:
		case EXPR_CONST:
		case BRANCHCASES:
		case SUMCASES:
		case EXPR_ZA:
		case EXPR_PZA:
		case EXPR_EP:
		case MDCASES:
		case EXPR_VMD:
		case EXPR_READ:
			return 1;
		default:
			return 0;
	}
}
static int expr_usesrc(enum expr_op op){
	switch(op){
		case SRCCASES:
		case EXPR_READ:
		case EXPR_WRITE:
		case EXPR_LJ:
		case EXPR_PBL:
		case EXPR_PZA:
		case EXPR_EVAL:
			return 1;
		default:
			return 0;
	}
}

static int expr_usesum(enum expr_op op){
	switch(op){
		case SUMCASES:
				return 1;
		default:
				return 0;
	}
}

static int expr_usemd(enum expr_op op){
	switch(op){
		case MDCASES:
		case EXPR_PMD:
		case EXPR_PME:
		case EXPR_PMEP:
				return 1;
		default:
				return 0;
	}
}
static int expr_usevmd(enum expr_op op){
	switch(op){
		case EXPR_VMD:
				return 1;
		default:
				return 0;
	}
}

static int expr_usebranch(enum expr_op op){
	switch(op){
		case BRANCHCASES:
				return 1;
		default:
				return 0;
	}
}
static int expr_usehot(enum expr_op op){
	switch(op){
		case HOTCASES:
				return 1;
		default:
				return 0;
	}
}
static int expr_vused(struct expr_inst *ip1,double *v){
	int ov;
	for(;;++ip1){
		ov=expr_override(ip1->op);
		if((expr_usesrc(ip1->op)&&ip1->un.src==v)
			||(ip1->dst.dst==v&&!ov)
		){
			return 1;
		}
		if(ip1->dst.dst==v&&ov){
			return 0;
		}
		if(ip1->op==EXPR_END){
			return 0;
		}
	}
	return 0;
}
static int expr_constexpr(const struct expr *restrict ep,double *except){
	for(struct expr_inst *ip=ep->data;;++ip){
		if(!expr_varofep(ep,ip->dst.dst)&&ip->dst.dst!=except)return 0;
		switch(ip->op){
			case EXPR_BL:
			case EXPR_ZA:
			case EXPR_HOT:
				if(ip->flag&EXPR_SF_INJECTION)
					break;
			case EXPR_INPUT:
			case MDCASES:
			case EXPR_VMD:
			case SUMCASES:
			case BRANCHCASES:
			case EXPR_DO:
			case EXPR_EP:
			case EXPR_WIF:
			case EXPR_EVAL:
			case EXPR_READ:
			case EXPR_WRITE:
			case EXPR_ALO:
			case EXPR_SJ:
			case EXPR_LJ:
			case EXPR_PBL:
			case EXPR_PZA:
			case EXPR_PMD:
			case EXPR_PME:
			case EXPR_PMEP:
				return 0;
			case SRCCASES:
				if(!expr_varofep(ep,ip->un.src)&&
					ip->un.src!=except)
					return 0;
				break;
			case EXPR_END:
				return 1;
			default:
				break;
		}
	}
}
#define CALSUM(_op,_do,_init,_neg,dest)\
			case _op :\
				neg=0;\
				from=expr_eval(ip->un.es->fromep,input);\
				to=expr_eval(ip->un.es->toep,input);\
				if(from>to){\
					step=from;\
					from=to;\
					to=step;\
					neg=1;\
				}\
				step=expr_eval(ip->un.es->stepep,input);\
				if(step<0){\
					step=-step;\
					neg^=1;\
				}\
				_init ;\
				for(ip->un.es->index=from;\
					ip->un.es->index<=to;\
					ip->un.es->index+=step){\
					y=expr_eval(ip->un.es->ep,input) ;\
					_do ;\
				}\
				if(neg)sum= _neg ;\
				*dest=sum;\
				break
#define CALSUM_INSWITCH(dest) CALSUM(EXPR_SUM,sum+=y,sum=0.0,-sum,dest);\
			CALSUM(EXPR_INT,sum+=step*y,sum=0.0;from+=step/2.0,-sum,dest);\
			CALSUM(EXPR_PROD,sum*=y,sum=1.0,1.0/sum,dest);\
			CALSUM(EXPR_SUP,if(y>sum)sum=y,sum=DBL_MIN,sum,dest);\
			CALSUM(EXPR_INF,if(y<sum)sum=y,sum=DBL_MAX,sum,dest);\
			CALSUM(EXPR_ANDN,sum=sum!=DBL_MAX?and2(sum,y):y,sum=DBL_MAX,sum,dest);\
			CALSUM(EXPR_ORN,sum=sum!=0.0?or2(sum,y):y,sum=0.0,sum,dest);\
			CALSUM(EXPR_XORN,sum=sum!=0.0?xor2(sum,y):y,sum=0.0,sum,dest);\
			CALSUM(EXPR_GCDN,sum=sum!=DBL_MAX?gcd2(sum,y):y,sum=DBL_MAX,sum,dest);\
			CALSUM(EXPR_LCMN,sum=sum!=1.0?lcm2(sum,y):y,sum=1.0,sum,dest);\
\
			case EXPR_FOR:\
				ip->un.es->index=\
				expr_eval(ip->un.es->fromep,input);\
				to=expr_eval(ip->un.es->toep,input);\
				if(to<0.0)to=-to;\
				while(to!=0.0){\
					expr_eval(ip->un.es->stepep,input);\
					to=expr_eval(ip->un.es->toep,input);\
					if(to<0.0)to=-to;\
				}\
				*dest=expr_eval(ip->un.es->ep,input);\
				break;\
			case EXPR_LOOP:\
				ip->un.es->index=\
				expr_eval(ip->un.es->fromep,input);\
				to=expr_eval(ip->un.es->toep,input);\
				if(to<0)to=-to;\
				for(;to>0.0;to-=1.0){\
					expr_eval(ip->un.es->stepep,input);\
				}\
				*dest=expr_eval(ip->un.es->ep,input);\
				break
#define CALMD_INSWITCH(dest) case EXPR_MD:\
				ap=ip->un.em->args;\
				endp=ap+ip->un.em->dim;\
				epp=ip->un.em->eps;\
				for(;ap<endp;++ap)\
					*ap=expr_eval(epp++,input);\
				*dest=ip->un.em->un.func(ip->un.em->dim,ip->un.em->args);\
				break;\
			case EXPR_MEP:\
				ip->un.em->eps->ip=ip;\
			case EXPR_ME:\
				*dest=ip->un.em->un.funcep(ip->un.em->dim,ip->un.em->eps,input);\
				break
static double expr_vmdeval(struct expr_vmdinfo *restrict ev,double input);
static int expr_optimize_constexpr(struct expr *restrict ep){
	double result;
	union {
		struct {
			double _sum,_step,_from,_to,_y;
			int _neg;
		} s0;
		struct {
			double *_ap,*_endp;
			struct expr *_epp,*_endp1;
		} s1;
	} un;
#define sum (un.s0._sum)
#define from (un.s0._from)
#define to (un.s0._to)
#define step (un.s0._step)
#define y (un.s0._y)
#define neg (un.s0._neg)
#define ap (un.s1._ap)
#define endp (un.s1._endp)
#define endp1 (un.s1._endp1)
#define epp (un.s1._epp)
	static const double input=0.0;
	struct expr *hep;
	int r=0;
	for(struct expr_inst *ip=ep->data;ip->op!=EXPR_END;++ip){
		switch(ip->op){
			case SUMCASES:
				if(!(expr_constexpr(ip->un.es->fromep,NULL)&&
				expr_constexpr(ip->un.es->toep,(double *)&ip->un.es->index)&&
				expr_constexpr(ip->un.es->stepep,(double *)&ip->un.es->index)&&
				expr_constexpr(ip->un.es->ep,(double *)&ip->un.es->index)))
					continue;
				switch(ip->op){
					CALSUM_INSWITCH((&result));
					default:
						abort();
				}
				expr_freesuminfo(ip->un.es);
				ip->op=EXPR_CONST;
				ip->un.value=result;
				expr_writeconsts(ep);
				r=1;
				break;
			case MDCASES:
				if(!(ip->flag&EXPR_SF_INJECTION))
					continue;
				epp=ip->un.em->eps;
				endp1=epp+ip->un.em->dim;
				for(;epp<endp1;++epp){
					if(!expr_constexpr(epp,NULL))
						goto force_continue;
				}
				switch(ip->op){
					CALMD_INSWITCH((&result));
					default:
						abort();
				}
				expr_freemdinfo(ip->un.em);
				ip->op=EXPR_CONST;
				ip->flag=0;
				ip->un.value=result;
				expr_writeconsts(ep);
				r=1;
				break;
			case EXPR_VMD:
				if(!(ip->flag&EXPR_SF_INJECTION))
					continue;
				if(!(expr_constexpr(ip->un.ev->fromep,NULL)&&
				expr_constexpr(ip->un.ev->toep,(double *)&ip->un.ev->index)&&
				expr_constexpr(ip->un.ev->stepep,(double *)&ip->un.ev->index)&&
				expr_constexpr(ip->un.ev->ep,(double *)&ip->un.ev->index)))
					continue;
				result=expr_vmdeval(ip->un.ev,input);
				expr_freevmdinfo(ip->un.ev);
				ip->op=EXPR_CONST;
				ip->flag=0;
				ip->un.value=result;
				expr_writeconsts(ep);
				r=1;
				break;
			case EXPR_WHILE:
				if(!expr_constexpr(ip->un.eb->cond,NULL)){
					if(expr_constexpr(ip->un.eb->body,NULL)){
wif:
						hep=ip->un.eb->cond;
						expr_free(ip->un.eb->body);
						expr_free(ip->un.eb->value);
						xfree(ip->un.eb);
						ip->op=EXPR_WIF;
						ip->un.hotfunc=hep;
						ip->flag=0;
						r=1;
						break;
					}
					continue;
				}
				if(expr_eval(ip->un.eb->cond,input)!=0.0){
endless:
					hep=ip->un.eb->body;
					expr_free(ip->un.eb->cond);
					expr_free(ip->un.eb->value);
					xfree(ip->un.eb);
					ip->op=EXPR_DO;
					ip->un.hotfunc=hep;
					ip->flag=0;
					r=1;
					break;
				}
				hep=ip->un.eb->value;
				expr_free(ip->un.eb->cond);
				expr_free(ip->un.eb->body);
				goto free_eb;
			case EXPR_DON:
				if(!expr_constexpr(ip->un.eb->cond,NULL))
					continue;
				switch((size_t)expr_eval(ip->un.eb->cond,input)){
					case 0:
						hep=ip->un.eb->value;
						expr_free(ip->un.eb->cond);
						expr_free(ip->un.eb->body);
						goto free_eb;
					default:
						continue;
				}
				break;
			case EXPR_DOW:
				if(!expr_constexpr(ip->un.eb->cond,NULL)){
					if(expr_constexpr(ip->un.eb->body,NULL))
						goto wif;
				}
					continue;
				if(expr_eval(ip->un.eb->cond,input)!=0.0)
					goto endless;
				break;
			case EXPR_IF:
				if(!expr_constexpr(ip->un.eb->cond,NULL))
					continue;
				result=expr_eval(ip->un.eb->cond,input);
				expr_free(ip->un.eb->cond);
				if(result!=0.0){
					hep=ip->un.eb->body;
					expr_free(ip->un.eb->value);
				}else {
					hep=ip->un.eb->value;
					expr_free(ip->un.eb->body);
				}
free_eb:
				xfree(ip->un.eb);
				ip->op=EXPR_EP;
				ip->un.hotfunc=hep;
				ip->flag=0;
				r=1;
			case EXPR_HOT:
			case EXPR_EP:
			case EXPR_WIF:
				if(!expr_constexpr(ip->un.hotfunc,NULL))
					continue;
				result=expr_eval(ip->un.hotfunc,input);
				expr_free(ip->un.hotfunc);
				ip->op=EXPR_CONST;
				ip->un.value=result;
				expr_writeconsts(ep);
				ip->flag=0;
				r=1;
				break;
			default:
				break;
		}
force_continue:
		continue;
	}
	return r;
#undef sum
#undef from
#undef to
#undef step
#undef y
#undef neg
#undef ap
#undef endp
#undef endp1
#undef epp
}
static int expr_vcheck_ep(struct expr *restrict ep,struct expr_inst *ip0,double *v){
	if(expr_vused(ip0,v))return 1;
	for(struct expr_inst *ip=ip0;;++ip){
		if(ip->op==EXPR_LJ)return 1;
		if(expr_usesum(ip->op)&&(
			expr_vcheck_ep(ip->un.es->fromep,ip->un.es->fromep->data,v)||
			expr_vcheck_ep(ip->un.es->toep,ip->un.es->toep->data,v)||
			expr_vcheck_ep(ip->un.es->stepep,ip->un.es->stepep->data,v)||
			expr_vcheck_ep(ip->un.es->ep,ip->un.es->ep->data,v)
			))return 1;
		if(expr_usevmd(ip->op)&&(
			expr_vcheck_ep(ip->un.ev->fromep,ip->un.ev->fromep->data,v)||
			expr_vcheck_ep(ip->un.ev->toep,ip->un.ev->toep->data,v)||
			expr_vcheck_ep(ip->un.ev->stepep,ip->un.ev->stepep->data,v)||
			expr_vcheck_ep(ip->un.ev->ep,ip->un.ev->ep->data,v)
			))return 1;
		if(expr_usebranch(ip->op)&&(
			expr_vcheck_ep(ip->un.eb->cond,ip->un.eb->cond->data,v)||
			expr_vcheck_ep(ip->un.eb->body,ip->un.eb->body->data,v)||
			expr_vcheck_ep(ip->un.eb->value,ip->un.eb->value->data,v)
			))return 1;
		if(expr_usemd(ip->op)){
			for(size_t i=0;i<ip->un.em->dim;++i){
				if(expr_vcheck_ep(ip->un.em->eps+i,ip->un.em->eps[i].data,v))
					return 1;
			}
		}
		if(expr_usehot(ip->op)&&(
			expr_vcheck_ep(ip->un.hotfunc,ip->un.hotfunc->data,v)
			))return 1;
		if(ip->op==EXPR_END){
			if(ip->dst.dst==v)
				return 1;
			else break;
		}
	}
	for(struct expr_resource *rp=ep->res;rp;rp=rp->next){
		if(!rp->un.uaddr)
			continue;
		switch(rp->type){
			case EXPR_HOTFUNCTION:
				if(expr_vcheck_ep(rp->un.ep,rp->un.ep->data,v))
					return 1;
			default:
				break;
		}
	}
	return 0;
}
/*static int expr_optimize_mulpow2n(struct expr *restrict ep){
	int r=0;
	for(struct expr_inst *ip=ep->data;ip->op!=EXPR_END;++ip){
		switch(ip->op){
			case EXPR_MUL:
			case EXPR_DIV:
				break;
			default:
				continue;
		}
		if(expr_modified(ep,ip->un.src)||
			EXPR_EDSIGN(ip->un.src)||
			EXPR_EDBASE(ip->un.src))
			continue;
		switch(ip->op){
			case EXPR_MUL:
				*ip->un.src=(double)(EXPR_EDEXP(ip->un.src)-1023);
				break;
			case EXPR_DIV:
				*ip->un.src=-(double)(EXPR_EDEXP(ip->un.src)-1023);
				break;
			default:
				abort();
		}
		ip->op=EXPR_SHL;
		r=1;
	}
	return r;
}*/
//2 bugs
static void expr_optimize_unused(struct expr *restrict ep){
	for(struct expr_inst *ip=ep->data;ip->op!=EXPR_END;++ip){

		if(!expr_varofep(ep,ip->dst.dst)
			||expr_side(ip->op))continue;
		if(!expr_vcheck_ep(ep,ip+1,ip->dst.dst)){
			ip->dst.dst=NULL;
		}
	}
	expr_optimize_completed(ep);
}
static void expr_optimize_constneg(struct expr *restrict ep){
	for(struct expr_inst *ip=ep->data;ip->op!=EXPR_END;++ip){
		switch(ip->op){
			case EXPR_NEG:
			case EXPR_NOT:
			case EXPR_NOTL:
			case EXPR_TSTL:
				break;
			default:
				continue;
		}
		for(struct expr_inst *ip1=ip-1;ip>=ep->data;--ip1){
			if(ip1->dst.dst!=ip->dst.dst)continue;
			if(ip1->op!=EXPR_CONST)break;
			switch(ip->op){
				case EXPR_NEG:
					ip1->un.value=-ip1->un.value;
					break;
				case EXPR_NOT:
					ip1->un.value=not(ip1->un.value);
					break;
				case EXPR_NOTL:
					ip1->un.value=(ip1->un.value==0.0)?
						1.0:0.0;
					break;
				case EXPR_TSTL:
					ip1->un.value=(ip1->un.value!=0.0)?
						1.0:0.0;
					break;
				default:
					continue;
			}
			expr_writeconsts(ep);
			ip->dst.dst=NULL;
			break;
		}
	}
	expr_optimize_completed(ep);
}

static int expr_injection_optype(enum expr_op op){
	switch(op){
		case EXPR_BL:
		case EXPR_ZA:
		case EXPR_HOT:
			return 1;
		default:
			return 0;
	}
}
static int expr_isinjection(struct expr *restrict ep,struct expr_inst *ip){
	return expr_injection_optype(ip->op)
		&&(ip->flag&EXPR_SF_INJECTION);
}
static int expr_optimize_injection(struct expr *restrict ep){
	int r=0;
	for(struct expr_inst *ip=ep->data;ip->op!=EXPR_END;++ip){
			if(!expr_isinjection(ep,ip))continue;
			if(ip->op==EXPR_ZA){
				ip->un.value=ip->un.zafunc();
				ip->op=EXPR_CONST;
				ip->flag=0;
				expr_writeconsts(ep);
				r=1;
				continue;
			}
		for(struct expr_inst *ip1=ip-1;ip1>=ep->data;--ip1){
			if(ip1->dst.dst!=ip->dst.dst)continue;
			if(ip1->op==EXPR_INPUT&&
				!expr_vcheck_ep(ep,ip+1,ip->dst.dst))
				goto delete;
			if(ip1->op!=EXPR_CONST)break;
			switch(ip->op){
				case EXPR_BL:
					ip1->un.value=ip->un.func(ip1->un.value);
					break;
				case EXPR_HOT:
					ip1->un.value=expr_eval(ip->un.hotfunc,ip1->un.value);
					expr_free(ip->un.hotfunc);
					break;
				default:
					abort();
			}
			expr_writeconsts(ep);
delete:
			r=1;
			ip->dst.dst=NULL;
			break;
		}
	}
	expr_optimize_completed(ep);
	return r;
}
static void expr_optimize_copyadd(struct expr *restrict ep){
	struct expr_inst *ip2;
	for(struct expr_inst *ip=ep->data;ip->op!=EXPR_END;++ip){
		if(ip->op!=EXPR_COPY||
			!expr_varofep(ep,ip->dst.dst)
			||expr_vcheck_ep(ep,ip+1,ip->dst.dst))continue;
		ip2=NULL;
		for(struct expr_inst *ip1=ip+1;ip1->op!=EXPR_END;++ip1){
			if(ip1->dst.dst==ip->dst.dst)goto fail;
			if(!expr_usesrc(ip1->op)||ip1->un.src!=ip->dst.dst)continue;
			if(ip2){
fail:
				ip2=NULL;
				break;
			}else
				ip2=ip1;
		}
		if(ip2){
			ip2->un.src=ip->un.src;
			ip->dst.dst=NULL;
		}
	}
	expr_optimize_completed(ep);
}
/*static int expr_optimize_copy2const(struct expr *restrict ep){
	int r=0;
	for(struct expr_inst *ip=ep->data;ip->op!=EXPR_END;++ip){
		if(ip->op!=EXPR_COPY
			//||
			//!expr_varofep(ep,ip->dst.dst)||
			//!expr_varofep(ep,ip->un.src)
			)continue;
		if(!expr_modified(ep,ip->un.src)){
			ip->op=EXPR_CONST;
			ip->un.value=*ip->un.src;
			expr_writeconsts(ep);
			r=1;
		}
	}
	expr_optimize_completed(ep);
	return r;
}*/
//a bug cannot fix
static void expr_optimize_copyend(struct expr *restrict ep){
	struct expr_inst *lip=NULL;
	struct expr_inst *ip=ep->data;
	for(;ip->op!=EXPR_END;++ip){
		lip=ip;
	}
	if(lip&&lip->op==EXPR_COPY&&lip->dst.dst==ip->dst.dst&&
		expr_varofep(ep,lip->dst.dst)
		){
		ip->dst.dst=lip->un.src;
		lip->dst.dst=NULL;
	}
	expr_optimize_completed(ep);
}
static void expr_optimize_strongorder_and_notl(struct expr *restrict ep){
	for(struct expr_inst *ip=ep->data;ip->op!=EXPR_END;++ip){
		if(ip[1].dst.dst!=ip->dst.dst)continue;
		switch(ip[1].op){
			case EXPR_NOTL:
			case EXPR_TSTL:
				break;
			default:
				continue;
		}
		switch(ip->op){
			case EXPR_LT:
			case EXPR_GT:
			case EXPR_SLE:
			case EXPR_SGE:
			case EXPR_SEQ:
			case EXPR_SNE:
			case EXPR_EQ:
			case EXPR_NE:
				break;
			default:
				++ip;
				continue;
		}
		if(ip[1].op==EXPR_NOTL)switch(ip->op){
			case EXPR_LT:
				ip->op=EXPR_SGE;
				break;
			case EXPR_GT:
				ip->op=EXPR_SLE;
				break;
			case EXPR_SLE:
				ip->op=EXPR_GT;
				break;
			case EXPR_SGE:
				ip->op=EXPR_LT;
				break;
			case EXPR_SEQ:
				ip->op=EXPR_SNE;
				break;
			case EXPR_SNE:
				ip->op=EXPR_SEQ;
				break;
			case EXPR_EQ:
				ip->op=EXPR_NE;
				break;
			case EXPR_NE:
				ip->op=EXPR_EQ;
				break;
			default:
				abort();
		}
		(++ip)->dst.dst=NULL;
	}
	expr_optimize_completed(ep);
}
static void expr_optimize_contneg(struct expr *restrict ep){
	for(struct expr_inst *ip=ep->data;ip->op!=EXPR_END;++ip){
		if(ip[1].dst.dst!=ip->dst.dst)continue;
		switch(ip->op){
			case EXPR_NOT:
			case EXPR_NEG:
				if(ip[1].op==ip->op){
					ip->dst.dst=NULL;
					(++ip)->dst.dst=NULL;
				}
				break;
			case EXPR_TSTL:
				switch(ip[1].op){
					case EXPR_NOT:
						goto from_tstl_to_not;
					case EXPR_NEG:
						goto from_tstl_to_neg;
					case EXPR_NOTL:
					case EXPR_TSTL:
						(ip++)->dst.dst=NULL;
						break;
					default:
						break;
				}
				break;
			case EXPR_NOTL:
				switch(ip[1].op){
					case EXPR_NOT:
from_tstl_to_not:
						if(ip[2].dst.dst==ip->dst.dst)
						switch(ip[2].op){
							case EXPR_NOTL:
								ip->un.value=0;
								goto notl_tstl;
							case EXPR_TSTL:
								ip->un.value=1;
notl_tstl:
								ip->op=EXPR_CONST;
								(++ip)->dst.dst=NULL;
								(++ip)->dst.dst=NULL;
								expr_writeconsts(ep);
							default:
								break;
						}
						continue;
					case EXPR_NEG:

from_tstl_to_neg:
						if(ip[2].dst.dst==ip->dst.dst)
						switch(ip[2].op){
							case EXPR_NOTL:
							case EXPR_TSTL:
								(++ip)->dst.dst=NULL;
							default:
								break;
						}
						continue;
					case EXPR_NOTL:
						ip->op=EXPR_TSTL;
					case EXPR_TSTL:
						(++ip)->dst.dst=NULL;
						break;
					default:
						break;
				}
				break;
			default:
				break;
		}
	}
	expr_optimize_completed(ep);
}
static int expr_optimize_once(struct expr *restrict ep){
	int r=0;
	expr_optimize_const(ep);
	expr_optimize_constneg(ep);
	//expr_optimize_notl(ep);
	expr_optimize_contneg(ep);
	expr_optimize_strongorder_and_notl(ep);
	r|=expr_optimize_injection(ep);
	expr_optimize_contmul(ep,EXPR_POW);
	expr_optimize_contmul(ep,EXPR_MUL);
	expr_optimize_contmul(ep,EXPR_DIV);
	expr_optimize_contmul(ep,EXPR_MOD);
	expr_optimize_contadd(ep);
	expr_optimize_contsh(ep);
	expr_optimize_contmul(ep,EXPR_NEXT);
	expr_optimize_contmul(ep,EXPR_DIFF);
	expr_optimize_contmul(ep,EXPR_LT);
	expr_optimize_contmul(ep,EXPR_LE);
	expr_optimize_contmul(ep,EXPR_GT);
	expr_optimize_contmul(ep,EXPR_GE);
	expr_optimize_contmul(ep,EXPR_SLE);
	expr_optimize_contmul(ep,EXPR_SGE);
	expr_optimize_contmul(ep,EXPR_SEQ);
	expr_optimize_contmul(ep,EXPR_SNE);
	expr_optimize_contmul(ep,EXPR_EQ);
	expr_optimize_contmul(ep,EXPR_NE);
	expr_optimize_contmul(ep,EXPR_AND);
	expr_optimize_contmul(ep,EXPR_XOR);
	expr_optimize_contmul(ep,EXPR_OR);
	expr_optimize_contmul(ep,EXPR_ANDL);
	expr_optimize_contmul(ep,EXPR_XORL);
	expr_optimize_contmul(ep,EXPR_ORL);
	//expr_optimize_contmul(ep,EXPR_COPY);

	//r|=expr_optimize_mulpow2n(ep);
	r|=expr_optimize_zero(ep);
	//r|=expr_optimize_copy2const(ep);
	expr_optimize_copyadd(ep);
	expr_optimize_unused(ep);
	r|=expr_optimize_constexpr(ep);
	expr_optimize_copyend(ep);
	return r;
}
static void expr_optimize(struct expr *restrict ep){
	size_t s=ep->size;
	int r;
	expr_writeconsts(ep);
again:
	r=expr_optimize_once(ep);
	if((ep->size<s||r)&&ep->size>1){
		s=ep->size;
		goto again;
	}

}
__attribute__((noinline))
static double expr_vmdeval(struct expr_vmdinfo *restrict ev,double input){
	ssize_t max=ev->max;
	double *args,*ap,from,to,step;
	step=fabs(expr_eval(ev->stepep,input));
	from=expr_eval(ev->fromep,input);
	to=expr_eval(ev->toep,input);
	if(max<=0){
		if(step==0.0)return NAN;
		max=1;
		for(double from1=from,from2;;++max){
			if(from<to){
				from2=from1;
				from1+=step;
				if(from1>to)break;
			}else if(from>to){
				from2=from1;
				from1-=step;
				if(from1<to)break;
			}else {
				break;
			}
			if(from2==from1)return NAN;
		}
	}
	args=ev->args?ev->args:alloca(max*sizeof(double));
	ap=args;
	ev->index=from;
	for(;max;--max){
		*(ap++)=expr_eval(ev->ep,input);
		if(from<to){
			ev->index+=step;
			if(ev->index>to)break;
		}else if(from>to){
			ev->index-=step;
			if(ev->index<to)break;
		}else {
			break;
		}
	}
	return ev->func(ap-args,args);
}
__attribute__((noinline))
double expr_eval(const struct expr *restrict ep,double input){
	union {
		struct {
			double _sum,_step,_from,_to,_y;
			int _neg;
		} s0;
		struct {
			double *_ap,*_endp;
			struct expr *_epp;
		} s1;
		struct {
			union {
				double d;
				struct expr_jmpbuf *jp;
			} un;
			int val;
		} s2;
		struct expr_jmpbuf *jp;
		void *up;
		double d;
		size_t index;
	} un;
#define sum (un.s0._sum)
#define from (un.s0._from)
#define to (un.s0._to)
#define step (un.s0._step)
#define y (un.s0._y)
#define neg (un.s0._neg)
#define ap (un.s1._ap)
#define endp (un.s1._endp)
#define epp (un.s1._epp)
	for(struct expr_inst *ip=ep->data;;++ip){
		assert(ip->op>=EXPR_COPY);
		assert(ip->op<=EXPR_END);
		switch(ip->op){
			case EXPR_COPY:
				*ip->dst.dst=*ip->un.src;
				break;
			case EXPR_INPUT:
				*ip->dst.dst=input;
				break;
			case EXPR_BL:
				*ip->dst.dst=ip->un.func(*ip->dst.dst);
				break;
			case EXPR_CONST:
				*ip->dst.dst=ip->un.value;
				break;
			case EXPR_ADD:
				*ip->dst.dst+=*ip->un.src;
				break;
			case EXPR_SUB:
				*ip->dst.dst-=*ip->un.src;
				break;
			case EXPR_MUL:
				*ip->dst.dst*=*ip->un.src;
				break;
			case EXPR_DIV:
				*ip->dst.dst/=*ip->un.src;
				break;
			case EXPR_MOD:
				*ip->dst.dst=fmod(*ip->dst.dst,*ip->un.src);
				break;
			case EXPR_POW:
				*ip->dst.dst=pow(*ip->dst.dst,*ip->un.src);
				break;
			case EXPR_AND:
				*ip->dst.dst=and2(*ip->dst.dst,*ip->un.src);
				break;
			case EXPR_OR:
				*ip->dst.dst=or2(*ip->dst.dst,*ip->un.src);
				break;
			case EXPR_XOR:
				*ip->dst.dst=xor2(*ip->dst.dst,*ip->un.src);
				break;
			case EXPR_SHL:
				ip->dst.rdst->exp+=(int64_t)*ip->un.src;
				break;
			case EXPR_SHR:
				ip->dst.rdst->exp-=(int64_t)*ip->un.src;
				break;
			case EXPR_NEXT:
				*ip->dst.idst+=(int64_t)*ip->un.src;
				break;
			case EXPR_DIFF:
				*ip->dst.dst=(double)(*ip->dst.idst-*ip->un.isrc);
				break;
			case EXPR_OFF:
				*ip->dst.idst+=(int64_t)*ip->un.src*(int64_t)sizeof(double);
				break;
			case EXPR_NEG:
				*ip->dst.dst=-*ip->dst.dst;
				break;
			case EXPR_NOT:
				*ip->dst.dst=not(*ip->dst.dst);
				break;
			case EXPR_NOTL:
				*ip->dst.dst=(*ip->dst.dst==0.0)?
					1.0:
					0.0;
				break;
			case EXPR_TSTL:
				*ip->dst.dst=(*ip->dst.dst!=0.0)?
					1.0:
					0.0;
				break;
			case EXPR_GT:
				*ip->dst.dst=*ip->dst.dst>*ip->un.src?
					1.0:
					0.0;
				break;
			case EXPR_LT:
				*ip->dst.dst=*ip->dst.dst<*ip->un.src?
					1.0:
					0.0;
				break;
			case EXPR_SGE:
				*ip->dst.dst=*ip->dst.dst>=*ip->un.src?
					1.0:
					0.0;
				break;
			case EXPR_SLE:
				*ip->dst.dst=*ip->dst.dst<=*ip->un.src?
					1.0:
					0.0;
				break;
			case EXPR_GE:
				*ip->dst.dst=*ip->dst.dst>=*ip->un.src
				||expr_equal(*ip->dst.dst,*ip->un.src)?
					1.0:
					0.0;
				break;
			case EXPR_LE:
				*ip->dst.dst=*ip->dst.dst<=*ip->un.src
				||expr_equal(*ip->dst.dst,*ip->un.src)?
					1.0:
					0.0;
				break;

			case EXPR_SEQ:
				*ip->dst.dst=*ip->dst.dst==*ip->un.src?
					1.0:
					0.0;
				break;
			case EXPR_SNE:
				*ip->dst.dst=*ip->dst.dst!=*ip->un.src?
					1.0:
					0.0;
				break;
			case EXPR_EQ:
				*ip->dst.dst=expr_equal(*ip->dst.dst,*ip->un.src)?
					1.0:
					0.0;
				break;
			case EXPR_NE:
				*ip->dst.dst=!expr_equal(*ip->dst.dst,*ip->un.src)?
					1.0:
					0.0;
				break;
			case EXPR_ANDL:
				*ip->dst.dst=LOGIC(*ip->dst.dst,*ip->un.src,&&)?
					1.0:
					0.0;
				break;
			case EXPR_ORL:
				*ip->dst.dst=LOGIC(*ip->dst.dst,*ip->un.src,||)?
					1.0:
					0.0;
				break;
			case EXPR_XORL:
				*ip->dst.dst=LOGIC(*ip->dst.dst,*ip->un.src,^)?
					1.0:
					0.0;
				break;
			CALSUM_INSWITCH(ip->dst.dst);
			case EXPR_IF:
				*ip->dst.dst=
				expr_eval(ip->un.eb->cond,input)!=0.0?
				expr_eval(ip->un.eb->body,input):
				expr_eval(ip->un.eb->value,input);
				break;
			case EXPR_WHILE:
				while(expr_eval(ip->un.eb->cond,input)!=0.0)
					expr_eval(ip->un.eb->body,input);
				break;
			case EXPR_WIF:
				while(expr_eval(ip->un.hotfunc,input)!=0.0);
				break;
			case EXPR_DOW:
				do
					expr_eval(ip->un.eb->body,input);
				while(expr_eval(ip->un.eb->cond,input)!=0.0);
				break;
			case EXPR_DO:
				for(;;)expr_eval(ip->un.hotfunc,input);
			case EXPR_DON:
				un.index=(size_t)expr_eval(ip->un.eb->cond,input);
				while(un.index){
					expr_eval(ip->un.eb->body,input);
					--un.index;
				}
				break;
			case EXPR_ZA:
				*ip->dst.dst=ip->un.zafunc();
				break;
			CALMD_INSWITCH(ip->dst.dst);
			case EXPR_VMD:
				*ip->dst.dst=expr_vmdeval(ip->un.ev,input);
				break;
			case EXPR_EP:
				*ip->dst.dst=expr_eval(ip->un.hotfunc,input);
				break;
			case EXPR_EVAL:
				*ip->dst.dst=expr_eval(*ip->un.hotfunc2,*ip->dst.dst);
				break;
			case EXPR_HOT:
				*ip->dst.dst=expr_eval(ip->un.hotfunc,*ip->dst.dst);
				break;
			case EXPR_PBL:
				*ip->dst.dst=(*ip->un.func2)(*ip->dst.dst);
				break;
			case EXPR_PZA:
				*ip->dst.dst=(*ip->un.zafunc2)();
				break;
			case EXPR_PMD:
				ap=ip->un.em->args;
				endp=ap+ip->un.em->dim;
				epp=ip->un.em->eps;
				for(;ap<endp;++ap)
					*ap=expr_eval(epp++,input);
				*ip->dst.dst=(*ip->dst.md2)(ip->un.em->dim,ip->un.em->args);
				break;
			case EXPR_PMEP:
				ip->un.em->eps->ip=ip;
			case EXPR_PME:
				*ip->dst.dst=(*ip->dst.me2)(ip->un.em->dim,ip->un.em->eps,input);
				break;
			case EXPR_READ:
				*ip->dst.dst=**ip->un.src2;
				break;
			case EXPR_WRITE:
				**ip->un.src2=*ip->dst.dst;
				break;
			case EXPR_ALO:
				un.up=alloca((ssize_t)*ip->dst.dst*ip->un.zd);
				*ip->dst.dst=un.d;
				break;
			case EXPR_SJ:
				un.d=*ip->dst.dst;
				un.jp->ipp=&ip;
				un.jp->ip=ip;
				*ip->dst.dst=(double)setjmp(un.jp->jb);
				break;
			case EXPR_LJ:
				un.s2.val=(int)*ip->un.src;
				un.s2.un.d=*ip->dst.dst;//ip cannot be used after here
				*un.s2.un.jp->ipp=un.s2.un.jp->ip;
				longjmp(un.s2.un.jp->jb,un.s2.val);
			case EXPR_END:
				return *ip->dst.dst;
		}
	}
#undef sum
#undef from
#undef to
#undef step
#undef y
#undef neg
#undef ap
#undef endp
#undef epp
}
