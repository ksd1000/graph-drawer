/*******************************************************************************
 *License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>*
 *This is free software: you are free to change and redistribute it.           *
 *******************************************************************************/
#define _GNU_SOURCE
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <setjmp.h>

#define _EXPR_LIB 1
#include "expr.h"

#define PHYSICAL_CONSTANT 0

#define eval(_ep,_input) expr_eval(_ep,_input)

#if defined(EXPR_ISOLATED)&&(EXPR_ISOLATED)
expr_globals;
#endif

static void *warped_xmalloc(size_t size){
	return xmalloc(size);
}
/*
static void *warped_xrealloc(void *old,size_t size){
	return xrealloc(old,size);
}
*/
static void warped_xfree(void *old){
	return xfree(old);
}
__attribute__((noinline))
int expr_sort4(double *restrict v,size_t n,void *(*allocator)(size_t),void (*deallocator)(void *)){
	struct dnode {
		struct dnode *lt,*gt;
		size_t eq;
		double val;
	} *top,*dnp,*d0,**sp,**sp0;
	double *restrict p;
	double *endp=v+n;
	size_t depth,dep;
	if(allocator){
		dnp=allocator(n*sizeof(struct dnode));
		if(!dnp)
			return -1;
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
		if(dep>depth)
			depth=dep;
	}
	if(allocator){
		sp=allocator(depth*sizeof(struct dnode *));
		if(!sp){
			deallocator(dnp);
			return -2;
		}
	}else {
		sp=alloca(depth*sizeof(struct dnode *));
	}
	sp0=sp;
	*(sp++)=dnp;
	d0=dnp;
	for(top=dnp;;){
		if(top==dnp->lt){
no_lt:
			do
				*(v++)=dnp->val;
			while(--dnp->eq);
			if(v==endp)
				break;
			if(!dnp->gt)
				goto no_gt;
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
			if(!dnp->lt)
				goto no_lt;
			top=dnp;
			*(sp++)=dnp;
			dnp=dnp->lt;
			continue;
		}
	}
	if(allocator){
		deallocator(d0);
		deallocator(sp0);
	}
	return 0;
}
static int double_compar(const double *restrict x,const double *restrict y){
	return *x==*y?0:(*x<*y?-1:1);
}
void expr_sortq(double *restrict v,size_t n){
	expr_fry(v,n);
	qsort((void *)v,n,sizeof(double),(int (*)(const void *,const void *))double_compar);
}
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
			expr_sort4(v,n,NULL,NULL);
			return;
	}
}
#define cast(x,T) expr_cast(x,T)
#define ltod(_l) expr_ltod48(_l)
#define ltol(_l) expr_ltol48(_l)
#define ltom(_l) expr_ltom48(_l)
static double expr_lrand48(double x){
	return (double)ltol(expr_next48(cast(x,int64_t *)));
}
static double expr_mrand48(double x){
	return (double)ltom(expr_next48(cast(x,int64_t *)));
}
static double expr_rand48(double x){
	return (double)(expr_next48(cast(x,int64_t *)));
}
static double expr_rand48_next(double x){
	return (double)expr_next48v((int64_t)x);
}
static double expr_drand48_next(double x){
	return ltod(expr_next48v((cast(x+1.0,int64_t)&INT64_C(0xffffffffffff0))>>4));
}
static double expr_drand48(double x){
	return ltod(expr_next48(cast(x,int64_t *)));
}
static double expr_srand48(double x){
	return cast(expr_seed48((int64_t)x),double);
}
#define state48(bits,m,retval) \
	double *a,*end=args+n-1;\
	int64_t c,found=-1;\
	int_fast32_t i;\
	if(args>=end)\
		return NAN;\
	for(i=0;likely(i!=(1<<(bits)));++i){\
		a=args;\
		c=((int64_t)(uint32_t)(int32_t)*a<<(bits))|i;\
		for(;;){\
			c=expr_next48v(c);\
			if(likely(m(c)!=(int64_t)(uint32_t)(int32_t)a[1]))\
				break;\
			++a;\
			if(unlikely(a>=end))\
				break;\
		}\
		if(likely(a<end))\
			continue;\
		if(found<0){\
			found=c;\
		}else {\
			return NAN;\
		}\
	}\
	return found<0?INFINITY:(double)(retval)
static double expr_lrand48_next(double *args,size_t n){
	state48(17,ltol,ltol(expr_next48v(found)));
}
static double expr_lrand48_state(double *args,size_t n){
	state48(17,ltol,found);
}
static double expr_mrand48_next(double *args,size_t n){
	state48(16,ltom,ltom(expr_next48v(found)));
}
static double expr_mrand48_state(double *args,size_t n){
	state48(16,ltom,found);
}
static double expr_ssdrand48(double x){
	return ltod(expr_ssnext48(cast(x,struct expr_superseed48 *)));
}
static double expr_ssrand48(double x){
	return (double)(expr_ssnext48(cast(x,struct expr_superseed48 *)));
}
static double expr_sslrand48(double x){
	return (double)ltol(expr_ssnext48(cast(x,struct expr_superseed48 *)));
}
static double expr_ssmrand48(double x){
	return (double)ltom(expr_ssnext48(cast(x,struct expr_superseed48 *)));
}
static double expr_ssnext48_b(double x){
	return (double)expr_ssgetnext48(cast(x,const struct expr_superseed48 *));
}
static double expr_ssdnext48(double x){
	return ltod(expr_ssgetnext48(cast(x,const struct expr_superseed48 *)));
}
double expr_exp_old(double x){
	size_t n;
	double r,y;
	int frac;
	if(x<0){
		x=-x;
		frac=1;
	}else
		frac=0;
	n=x;
	x-=n;
	r=1.0;
	if(n){
		y=M_E;
		for(size_t b=1;;b<<=1){
			if(b&n){
				r*=y;
				n&=~b;
			}
			if(!n)
				break;
			y*=y;
		}
	}
	if(x!=0.0){
		unsigned int ff;
		double z;
		ff=2;
		z=1+x;
		y=x;
		for(;;){
			y*=x/ff;
			if(!y)
				break;
			z+=y;
			++ff;
		}
		r*=z;
	}
	return frac?1/r:r;
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
static double expr_and(double *args,size_t n){
	CALMD(expr_and2);
}
static double expr_or(double *args,size_t n){
	CALMD(expr_or2);
}
static double expr_xor(double *args,size_t n){
	CALMD(expr_xor2);
}
static double expr_gcd(double *args,size_t n){
	CALMD(expr_gcd2);
}
static double expr_lcm(double *args,size_t n){
	CALMD(expr_lcm2);
}
static double expr_add(double *args,size_t n){
	CALMD(expr_add2);
}
static double expr_mul(double *args,size_t n){
	CALMD(expr_mul2);
}
static double expr_cmp(double *args,size_t n){
	return (double)memcmp(args,args+1,sizeof(double));
}
static double expr_pow_old(double x,uintmax_t y){
	double r;
	size_t b;
	r=1.0;
	for(unsigned int i=0;;++i){
		b=((uintmax_t)1)<<i;
		if(b&y){
			r*=x;
			y&=~b;
		}
		if(unlikely(!y))
			break;
		x=x*x;
	}
	return r;
}
#define SIZE_BITS (8*sizeof(size_t))
static double expr_pow_old_n(double *args,size_t n){
	return expr_pow_old(*args,args[1]);
}
static double expr_strtol(double *args,size_t n){
	return (double)strtol(cast(*args,const char *),NULL,(int)args[1]);
}
static double expr_qmed(double *args,size_t n){
	expr_sortq(args,n);
	return n&1ul?args[n>>1ul]:(n>>=1ul,(args[n]+args[n-1])/2);
}
static double expr_med(double *args,size_t n){
	expr_sort(args,n);
	return n&1ul?args[n>>1ul]:(n>>=1ul,(args[n]+args[n-1])/2);
}
static double expr_hmed(double *args,size_t n){
	expr_sort4(args,n,warped_xmalloc,warped_xfree);
	return n&1ul?args[n>>1ul]:(n>>=1ul,(args[n]+args[n-1])/2);
}
static double expr_med_old(double *args,size_t n){
	expr_sort_old(args,n);
	return n&1ul?args[n>>1ul]:(n>>=1ul,(args[n]+args[n-1])/2);
}
static double expr_qgmed(double *args,size_t n){
	expr_sort(args,n);
	return n&1ul?args[n>>1ul]:(n>>=1ul,sqrt(args[n]*args[n-1]));
}
static double expr_gmed(double *args,size_t n){
	expr_sort(args,n);
	return n&1ul?args[n>>1ul]:(n>>=1ul,sqrt(args[n]*args[n-1]));
}
static double expr_hgmed(double *args,size_t n){
	expr_sort4(args,n,warped_xmalloc,warped_xfree);
	return n&1ul?args[n>>1ul]:(n>>=1ul,sqrt(args[n]*args[n-1]));
}
static double expr_gmed_old(double *args,size_t n){
	expr_sort_old(args,n);
	return n&1ul?args[n>>1ul]:(n>>=1ul,sqrt(args[n]*args[n-1]));
}
static double expr_mode0(size_t n,double *args,int heap){
	double max,cnt;
	double *endp=args+n;
	size_t maxn=1;
	switch(heap){
		case 0:
			expr_sort(args,n);
			break;
		case 2:
			expr_sortq(args,n);
			break;
		default:
			expr_sort4(args,n,warped_xmalloc,warped_xfree);
			break;
	}
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
		if(args==endp)
			break;
	}
	return max;
}
static double expr_mode(double *args,size_t n){
	return expr_mode0(n,args,0);
}
static double expr_qmode(double *args,size_t n){
	return expr_mode0(n,args,2);
}
static double expr_hmode(double *args,size_t n){
	return expr_mode0(n,args,1);
}
static double expr_sign(double x){
	if(x>0.0)
		return 1.0;
	else if(x<-0.0)
		return -1.0;
	else
		return 0.0;
}
struct s_eb {
	uint64_t eb:63;
	uint64_t sign:1;
};
static double bfry(const struct expr *args,size_t n,double input){
	expr_fry(cast(eval(args,input),double *),(size_t)fabs(eval(args+1,input)));
	return 0.0;
}
static double bmirror(const struct expr *args,size_t n,double input){
	expr_mirror(cast(eval(args,input),double *),(size_t)fabs(eval(args+1,input)));
	return 0.0;
}
static double bsort(const struct expr *args,size_t n,double input){
	expr_sort(cast(eval(args,input),double *),(size_t)fabs(eval(args+1,input)));
	return 0.0;
}
static double bhsort(const struct expr *args,size_t n,double input){
	int r;
	r=expr_sort4(cast(eval(args,input),double *),(size_t)fabs(eval(args+1,input)),warped_xmalloc,warped_xfree);
	if(!r){
		return 0.0;
	}else {
		return -1.0;
	}
}
static double bxsort(const struct expr *args,size_t n,double input){
	expr_sort4(cast(eval(args,input),double *),(size_t)fabs(eval(args+1,input)),warped_xmalloc,warped_xfree);
	return 0.0;
}
static double bsort_old(const struct expr *args,size_t n,double input){
	expr_sort_old(cast(eval(args,input),double *),(size_t)fabs(eval(args+1,input)));
	return 0.0;
}
static double bcontract(const struct expr *args,size_t n,double input){
	expr_contract(cast(eval(args,input),void *),(size_t)fabs(eval(args+1,input)));
	return 0.0;
}
static double bassert(const struct expr *args,size_t n,double input){
	double x=eval(args,input);
	if(x==0.0){
		const char *p,*e;
		e=(*args->parent->ipp)->un.em->e;
		p=strchr(e,'(');
		if(unlikely(!p))
			p=e;
		warn("assertion %s failed",p);
		warn("ABORTING");
		abort();
	}
	return x;
}
static double expr_ldr(const struct expr *args,size_t n,double input){
	union {
		double *r;
		double dr;
	} un;
	un.dr=eval(args,input);
	return un.r[(ptrdiff_t)eval(++args,input)];
}
static double expr_str(const struct expr *args,size_t n,double input){
	union {
		double *r;
		double dr;
	} un;
	un.dr=eval(args,input);
	un.r+=(ptrdiff_t)eval(++args,input);
	return *un.r=eval(++args,input);
}
static double expr_memset(const struct expr *args,size_t n,double input){
	union {
		double *r;
		double dr;
	} un;
	double *endp;
	double val;
	un.dr=eval(args,input);
	val=eval(++args,input);
	endp=un.r+(ptrdiff_t)eval(++args,input);
	while(un.r<endp){
		*un.r=val;
		++un.r;
	}
	return val;
}
static double expr_bzero(const struct expr *args,size_t n,double input){
	union {
		double *r;
		double dr;
	} un;
	un.dr=eval(args,input);
	memset(un.r,0,(size_t)eval(++args,input));
	return 0.0;
}
static double expr_isfinite_b(double x){
	return EXPR_EDEXP(&x)!=2047?1.0:0.0;
}
static double expr_isinf_b(double x){
	return !EXPR_EDBASE(&x)&&EXPR_EDEXP(&x)==2047?1.0:0.0;
}
static double expr_isnan_b(double x){
	return EXPR_EDBASE(&x)&&EXPR_EDEXP(&x)==2047?1.0:0.0;
}
static double expr_longjmp_out(double *args,size_t n){
	int val;
	val=n<2?0:(int)args[1];
	longjmp(cast(*args,void *),val);
}
static double expr_destruct(double *args,size_t n){
	struct expr *ep;
	double *a;
	ep=cast(*args,struct expr *);
	a=alloca((n-1)*sizeof(double));
	if(n>1){
		memcpy(a,args+1,(n-1)*sizeof(double));
	}
	expr_free(ep);
	switch(n){
		case 1:
			break;
		case 2:
			cast(*a,double (*)(void))();
			break;
		default:
			cast(*a,double (*)(double *,size_t))(a+1,n-2);
			break;
	}
	warn("function for destruct() returns");
	warn("ABORTING");
	abort();
}
static double expr_malloc(double x){
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
	un.r=xmalloc((size_t)fabs(x)*sizeof(double));
	return un.dr;
}
static double expr_realloc(double *args,size_t n){
	union {
		void *r;
		double dr;
	} un;
	un.dr=args[0];
	un.r=xrealloc(un.r,(size_t)args[1]);
	return un.dr;
}
static double expr_bxfree(double x){
	union {
		void *r;
		double dr;
	} un;
	un.dr=x;
	xfree(un.r);
	return un.dr;
}

static double bsyscall(double *args,size_t n){
	intptr_t num;
	intptr_t a[6];
	unsigned int i,argt;
	if(n<1)
		return -1;
	if(n>7)
		n=7;
	num=(intptr_t)*args;
	argt=(unsigned int)(num>>32);
	num&=0x0ffffffff;
	for(i=1;i<(int)n;++i){
		a[i-1]=argt&(1u<<i)?cast(args[i],intptr_t):(intptr_t)args[i];
	}
	for(;i<7;++i){
		a[i-1]=0;
	}
	num=expr_internal_syscall6(num,a[0],a[1],a[2],a[3],a[4],a[5]);
	return (argt&1u)?cast(num,double):(double)num;
}
static double systype(double x){
	const char *p;
	size_t len;
	unsigned int r=0;
	p=cast(x,const char *);
	len=strlen(p);
	if(len>7)
		len=7;
	for(size_t i=0;i<len;++i){
		switch(p[i]){
			case '*':
				r|=1u<<(unsigned int)i;
				break;
			default:
				break;
		}
	}
	return (double)((int64_t)r<<32);
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
#define RMEMC(sym,type)\
static double expr_r##sym##_c(double x){\
	union {\
		type *r;\
		double dr;\
	} un;\
	un.dr=x;\
	return cast(*un.r,double);\
}
#define BMEM(sym,type,s,op)\
static double expr_##s##sym(const struct expr *args,size_t n,double input){\
	union {\
		type *r;\
		double dr;\
	} un;\
	double v;\
	un.dr=eval(args,input);\
	v=eval(args+1,input);\
	*un.r op (type)v;\
	return v;\
}\
static double expr_##s##sym##_c(const struct expr *args,size_t n,double input){\
	union {\
		type *r;\
		double dr;\
	} un;\
	double v;\
	un.dr=eval(args,input);\
	v=eval(args+1,input);\
	*un.r op cast(v,type);\
	return v;\
}
#define LMEM(sym,type,s,op)\
static double expr_##s##sym(const struct expr *args,size_t n,double input){\
	union {\
		type *r;\
		double dr;\
	} un;\
	double v;\
	un.dr=eval(args,input);\
	v=eval(args+1,input);\
	return (double)(*un.r op (type)v);\
}\
static double expr_##s##sym##_c(const struct expr *args,size_t n,double input){\
	union {\
		type *r;\
		double dr;\
	} un;\
	double v;\
	un.dr=eval(args,input);\
	v=eval(args+1,input);\
	return (double)(*un.r op cast(v,type));\
}
#define FMEM(sym,type) RMEM(sym,type) RMEMC(sym,type) ZMEM(sym,type) BMEM(sym,type,w,=) BMEM(sym,type,a,+=) BMEM(sym,type,s,-=) BMEM(sym,type,m,*=) BMEM(sym,type,d,/=) LMEM(sym,type,e,==) LMEM(sym,type,l,<) LMEM(sym,type,g,>)
#define MEM(sym,type) FMEM(sym,type) BMEM(sym,type,c,&=) BMEM(sym,type,o,|=) BMEM(sym,type,x,^=) BMEM(sym,type,sl,<<=) BMEM(sym,type,sr,>>=)
MEM(8,int8_t);
MEM(16,int16_t);
MEM(32,int32_t);
MEM(64,int64_t);
MEM(m,intmax_t);
MEM(p,intptr_t);
MEM(z,ssize_t);
MEM(8u,uint8_t);
MEM(16u,uint16_t);
MEM(32u,uint32_t);
MEM(64u,uint64_t);
MEM(mu,uintmax_t);
MEM(pu,uintptr_t);
MEM(zu,size_t);
FMEM(f,float);
FMEM(d,double);
FMEM(l,long double);
#define REGMEM2(s,t) REGMDEPSYM2_NI("_" #t #s,expr_##t##s,2ul),REGMDEPSYM2_NI("_" #t #s "c",expr_##t##s##_c,2ul)
#define REGRMEM(s) REGFSYM2_NI("_r" #s,expr_r##s)
#define REGRMEMC(s) REGFSYM2_NI("_r" #s "c",expr_r##s##_c)
#define REGZMEM(s) REGFSYM2_NI("_z" #s,expr_z##s)
#define REGFMEM(_s) REGRMEM(_s),REGRMEMC(_s),REGZMEM(_s),REGMEM2(_s,w),REGMEM2(_s,a),REGMEM2(_s,s),REGMEM2(_s,m),REGMEM2(_s,d),REGMEM2(_s,e),REGMEM2(_s,l),REGMEM2(_s,g)
#define REGMEM(_s) REGFMEM(_s) ,REGMEM2(_s,c),REGMEM2(_s,o),REGMEM2(_s,x),REGMEM2(_s,sl),REGMEM2(_s,sr)
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
	if(x<0.0)
		un.d=(int64_t)x;
	else
		un.u=(uint64_t)x;
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
	if(x!=0.0)
		exit((int)x);
	return x;
}
static double expr_ctz(double x){
	uint64_t base=EXPR_EDBASE(&x);
	if(base){
		return (double)(__builtin_ctzg(base)+(EXPR_EDEXP(&x)-(1023L+52L)));
	}else
		return (double)(EXPR_EDEXP(&x)-1023L);
}
static double expr_clz(double x){
	return (double)(EXPR_EDEXP(&x)-1023L);
}
static double expr_ffs(double x){
	int64_t base=EXPR_EDBASE(&x);
	if(base){
		return (double)(__builtin_ffsl(base)+(EXPR_EDEXP(&x)-(1023L+52L)));
	}else
		return (double)(EXPR_EDEXP(&x)-1023L+1L);
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
static double expr_nfact(double *args,size_t n){
	double sum=1.0,x=args[0];
	x=floor(x);
	while(x>0.0){
		sum*=x;
		x-=args[1];
	}
	return sum;
}
static double expr_piece(const struct expr *args,size_t n,double input){
	const struct expr *arg0=args;
	--n;
	while(args-arg0<n){
		if(eval(args++,input)!=0.0){
			return eval(args,input);
		}else {
			++args;
		}

	}
	return eval(arg0+n,input);
}
static double expr_assign(const struct expr *args,size_t n,double input){
	double *p;
	const struct expr *endp=args+(n&~1ul);
	while(args<endp){
		p=expr_cast(eval(args++,input),__typeof(p));
		*p=(__typeof(*p))eval(args++,input);
	}
	return input;
}
static double expr_assigni(const struct expr *args,size_t n,double input){
	int64_t *p;
	const struct expr *endp=args+(n&~1ul);
	while(args<endp){
		p=expr_cast(eval(args++,input),__typeof(p));
		*p=(__typeof(*p))eval(args++,input);
	}
	return input;
}
static double expr_assignu(const struct expr *args,size_t n,double input){
	uint64_t *p;
	const struct expr *endp=args+(n&~1ul);
	while(args<endp){
		p=expr_cast(eval(args++,input),__typeof(p));
		*p=(__typeof(*p))eval(args++,input);
	}
	return input;
}
static double expr_derivate(const struct expr *args,size_t n,double input){
	double epsilon=(n>=2?eval(args+1,input):FLT_EPSILON);
	return (eval(args,input+epsilon)-
		eval(args,input-epsilon))/epsilon/2;
}
double expr_multilevel_derivate(const struct expr *ep,double input,long level,double epsilon){
	if(level<1)
		return eval(ep,input);
	else return (expr_multilevel_derivate(
		ep,input+epsilon,level-1,epsilon
		)-expr_multilevel_derivate(
			ep,input-epsilon,level-1,epsilon
			))/epsilon/2;
}
static double expr_multi_derivate(const struct expr *args,size_t n,double input){
	double epsilon=(n>=3?eval(args+2,input):FLT_EPSILON);
	double level=(n>=2?eval(args+1,input):1.0);
	return expr_multilevel_derivate(args,input,(long)level,epsilon);
}
static double expr_root(const struct expr *args,size_t n,double input){
	//root(expression)
	//root(expression,from)
	//root(expression,from,to)
	//root(expression,from,to,step)
	//root(expression,from,to,step,epsilon)
	double epsilon=FLT_EPSILON,from=0.0,to=INFINITY,step=FLT_EPSILON,swapbuf;
	switch(n){
		case 5:
			epsilon=fabs(eval(args+4,input));
		case 4:
			step=fabs(eval(args+3,input));
		case 3:
			to=eval(args+2,input);
		case 2:
			from=eval(args+1,input);
		case 1:
			break;
		default:
			return NAN;
	}
	if(unlikely(from>to)){
		swapbuf=from;
		from=to;
		to=swapbuf;
	}
	for(;likely(from<=to);from+=step){
		if(unlikely(fabs(eval(args,from))<=epsilon))
			return from;
	}
	return INFINITY;
}
static double expr_findbound2(const struct expr *args,size_t n,double input){
	//root2(expression)
	//root2(expression,from)
	//root2(expression,from,to)
	double from,to,swapbuf;
	int fcond,tcond,cond;
	from=eval(args+1,input);
	to=eval(args+2,input);
	fcond=(eval(args,from)!=0.0);
	tcond=(eval(args,to)!=0.0);
	if(fcond==tcond)
		return INFINITY;
	for(;;){
		swapbuf=(from+to)/2;
		if(unlikely(swapbuf==from||swapbuf==to))
			return from;
		cond=(eval(args,swapbuf)!=0.0);
		if(cond==fcond){
			from=swapbuf;
		}else {
			to=swapbuf;
		}
		//printf("(%.24lf,%.24lf)\n",from,to);
		//printf(":(%.24lf,%.24lf)\n",eval(args,from),eval(args,to));
	}
}
static double expr_root2(const struct expr *args,size_t n,double input){
	//root2(expression)
	//root2(expression,from)
	//root2(expression,from,to)
	//root2(expression,from,to,step)
	//root2(expression,from,to,step,epsilon)
	double epsilon=0.0,from=0.0,to=INFINITY,step=1.0,swapbuf;
	int neg,trunc;
	switch(n){
		case 5:
			epsilon=fabs(eval(args+4,input));
		case 4:
			step=fabs(eval(args+3,input));
		case 3:
			to=eval(args+2,input);
		case 2:
			from=eval(args+1,input);
		case 1:
			break;
		default:
			return NAN;
	}
	if(unlikely(from>to)){
		swapbuf=from;
		from=to;
		to=swapbuf;
		trunc=1;
	}else
		trunc=0;
	swapbuf=eval(args,from);
	if(unlikely(fabs(swapbuf)<=epsilon))
		goto end;
	neg=(swapbuf<-0.0);
	for(from+=step;from<=to;from+=step){
		if(unlikely(from+step==from)){
			goto end1;
		}
		if(unlikely(fabs(swapbuf=eval(args,from))<=epsilon))
			goto end;
		if((swapbuf<-0.0)==neg)
			continue;
		from-=step;
		if(unlikely(step<=epsilon))
			goto end;
		do {
			step/=2.0;
		}while((eval(args,from+step)<0.0)!=neg);
		from+=step;
	}
	return INFINITY;
end1:
	if(!trunc)
		swapbuf=eval(args,from);
end:
	if(!trunc&&fabs(swapbuf)>epsilon)
		return from+2.0*step;
	return from;
}
static double expr_rooti(const struct expr *args,size_t n,double input){
	//root2(expression)
	//root2(expression,from)
	//root2(expression,from,epsilon)
	//root2(expression,from,epsilon,depsilon)
	double epsilon=DBL_EPSILON,depsilon=FLT_EPSILON,from=1.0,swapbuf;
	switch(n){
		case 4:
			depsilon=fabs(eval(args+3,input));
		case 3:
			epsilon=fabs(eval(args+2,input));
		case 2:
			from=eval(args+1,input);
		case 1:
			break;
		default:
			return NAN;
	}
	for(;;){
		swapbuf=expr_multilevel_derivate(args,from,1,depsilon);
		if(unlikely(swapbuf==0.0)){
			//if(fabs(eval(args,from))<=epsilon)
			return from;
			//break;
		}
		swapbuf=from-eval(args,from)/swapbuf;
		if(unlikely(fabs(from-swapbuf)<=epsilon))
			return swapbuf;
		from=swapbuf;
	}
	return INFINITY;
}
static double expr_andl(const struct expr *args,size_t n,double input){
	for(const struct expr *ep=args;ep-args<n;++ep){
		if(eval(ep,input)==0.0)
			return 0.0;
	}
	return 1.0;
}
static double expr_orl(const struct expr *args,size_t n,double input){
	for(const struct expr *ep=args;ep-args<n;++ep){
		if(eval(ep,input)!=0.0)
			return 1.0;
	}
	return 0.0;
}
static double expr_max(double *args,size_t n){
	double ret=*args++;
	--n;
	while(n>0){
		if(*args>ret)
			ret=*args;
		--n;
		++args;
	}
	return ret;
}
static double expr_min(double *args,size_t n){
	double ret=*args++;
	--n;
	while(n>0){
		if(*args<ret)
			ret=*args;
		--n;
		++args;
	}
	return ret;
}
static double expr_hypot(double *args,size_t n){
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
#define REGFSYM_U(s) {.strlen=sizeof(#s)-1,.str=#s,.un={.func=s},.type=EXPR_FUNCTION,.flag=EXPR_SF_INJECTION|EXPR_SF_UNSAFE}
#define REGCSYM(s) {.strlen=sizeof(#s)-1,.str=#s,.un={.value=s},.type=EXPR_CONSTANT}
#define REGCSYM_E(s) {.strlen=sizeof(#s)-1,.str=#s,.un={.value=EXPR_##s},.type=EXPR_CONSTANT}
#define REGFSYM2(s,sym) {.strlen=sizeof(s)-1,.str=s,.un={.func=sym},.type=EXPR_FUNCTION,.flag=EXPR_SF_INJECTION}
#define REGFSYM2_NI(s,sym) {.strlen=sizeof(s)-1,.str=s,.un={.func=sym},.type=EXPR_FUNCTION,.flag=EXPR_SF_UNSAFE}
#define REGFSYM2_U(s,sym) {.strlen=sizeof(s)-1,.str=s,.un={.func=sym},.type=EXPR_FUNCTION,.flag=EXPR_SF_INJECTION|EXPR_SF_UNSAFE}
#define REGFSYM2_NIU(s,sym) {.strlen=sizeof(s)-1,.str=s,.un={.func=sym},.type=EXPR_FUNCTION,.flag=EXPR_SF_UNSAFE}
#define REGFSYM2_UA(s,sym) {.strlen=sizeof(s)-1,.str=s,.un={.func=sym},.type=EXPR_FUNCTION,.flag=EXPR_SF_ALLOWADDR|EXPR_SF_UNSAFE}
#define REGZASYM2(s,sym) {.strlen=sizeof(s)-1,.str=s,.un={.zafunc=sym},.type=EXPR_ZAFUNCTION,.flag=0}
#define REGZASYM2_U(s,sym) {.strlen=sizeof(s)-1,.str=s,.un={.zafunc=sym},.type=EXPR_ZAFUNCTION,.flag=EXPR_SF_UNSAFE}
#define REGMDSYM2(s,sym,d) {.strlen=sizeof(s)-1,.str=s,.un={.mdfunc=sym},.dim=d,.type=EXPR_MDFUNCTION,.flag=EXPR_SF_INJECTION}
#define REGMDSYM2_U(s,sym,d) {.strlen=sizeof(s)-1,.str=s,.un={.mdfunc=sym},.dim=d,.type=EXPR_MDFUNCTION,.flag=EXPR_SF_INJECTION|EXPR_SF_UNSAFE}
#define REGMDSYM2_NIU(s,sym,d) {.strlen=sizeof(s)-1,.str=s,.un={.mdfunc=sym},.dim=d,.type=EXPR_MDFUNCTION,.flag=EXPR_SF_UNSAFE}
//#define REGMDSYM2_NI(s,sym,d) {.strlen=sizeof(s)-1,.str=s,.un={.mdfunc=sym},.dim=d,.type=EXPR_MDFUNCTION,.flag=0}
#define REGMDEPSYM2(s,sym,d) {.strlen=sizeof(s)-1,.str=s,.un={.mdepfunc=sym},.dim=d,.type=EXPR_MDEPFUNCTION,.flag=EXPR_SF_INJECTION}
#define REGMDEPSYM2_NI(s,sym,d) {.strlen=sizeof(s)-1,.str=s,.un={.mdepfunc=sym},.dim=d,.type=EXPR_MDEPFUNCTION,.flag=EXPR_SF_UNSAFE}
#define REGMDEPSYM2_U(s,sym,d) {.strlen=sizeof(s)-1,.str=s,.un={.mdepfunc=sym},.dim=d,.type=EXPR_MDEPFUNCTION,.flag=EXPR_SF_INJECTION|EXPR_SF_UNSAFE}
#define REGMDEPSYM2_NIW(s,sym,d) {.strlen=sizeof(s)-1,.str=s,.un={.mdepfunc=sym},.dim=d,.type=EXPR_MDEPFUNCTION,.flag=EXPR_SF_WRITEIP|EXPR_SF_UNSAFE}
#define REGCSYM2(s,val) {.strlen=sizeof(s)-1,.str=s,.un={.value=val},.type=EXPR_CONSTANT}
const struct expr_builtin_symbol expr_symbols[]={
	REGCSYM(DBL_MAX),
	REGCSYM(DBL_MIN),
	REGCSYM(DBL_TRUE_MIN),
	REGCSYM(DBL_EPSILON),

	REGCSYM_E(CONSTANT),
	REGCSYM_E(VARIABLE),
	REGCSYM_E(FUNCTION),
	REGCSYM_E(MDFUNCTION),
	REGCSYM_E(MDEPFUNCTION),
	REGCSYM_E(HOTFUNCTION),
	REGCSYM_E(ZAFUNCTION),

	REGCSYM_E(IF_NOOPTIMIZE),
	REGCSYM_E(IF_INSTANT_FREE),
	REGCSYM_E(IF_INJECTION),
//	REGCSYM_E(IF_NOBUILTIN),
	REGCSYM_E(IF_NOKEYWORD),
	REGCSYM_E(IF_PROTECT),
//	REGCSYM_E(IF_INJECTION_B),
//	REGCSYM_E(IF_INJECTION_S),
	REGCSYM_E(IF_KEEPSYMSET),
	REGCSYM_E(IF_DETACHSYMSET),
	REGCSYM_E(IF_UNSAFE),
	REGCSYM_E(IF_EXTEND_MASK),
	REGCSYM_E(IF_SETABLE),

	REGCSYM_E(SF_INJECTION),
	REGCSYM_E(SF_WRITEIP),
	REGCSYM_E(SF_PMD),
	REGCSYM_E(SF_PME),
	REGCSYM_E(SF_PFUNC),
	REGCSYM_E(SF_UNSAFE),
	REGCSYM_E(SF_ALLOWADDR),

	REGCSYM_E(ESYMBOL),
	REGCSYM_E(EPT),
	REGCSYM_E(EFP),
	REGCSYM_E(ENVP),
	REGCSYM_E(ENEA),
	REGCSYM_E(ENUMBER),
	REGCSYM_E(ETNV),
	REGCSYM_E(EEV),
	REGCSYM_E(EUO),
	REGCSYM_E(EZAFP),
	REGCSYM_E(EDS),
	REGCSYM_E(EVMD),
	REGCSYM_E(EMEM),
	REGCSYM_E(EUSN),
	REGCSYM_E(ENC),
	REGCSYM_E(ECTA),
	REGCSYM_E(ESAF),
	REGCSYM_E(EVD),
	REGCSYM_E(EPM),
	REGCSYM_E(EIN),
	REGCSYM_E(EBS),
	REGCSYM_E(EVZP),
	REGCSYM_E(EANT),
	REGCSYM_E(EUDE),

	REGCSYM(FLT_MAX),
	REGCSYM(FLT_MIN),
	REGCSYM(FLT_EPSILON),
	REGCSYM(HUGE_VAL),
	REGCSYM(HUGE_VALF),
	REGCSYM(INFINITY),
	REGCSYM(NAN),
	REGCSYM2("NULL",(double)(uintptr_t)NULL),
	REGCSYM2("1_pi",M_1_PI),
	REGCSYM2("2_pi",M_2_PI),
	REGCSYM2("2_sqrtpi",M_2_SQRTPI),
	REGCSYM2("C",0.577215664901532860606512090082402431042),
	REGCSYM2("e",M_E),
	REGCSYM2("inf",INFINITY),
	REGCSYM2("log2e",M_LOG2E),
	REGCSYM2("log10e",M_LOG10E),
	REGCSYM2("ln2",M_LN2),
	REGCSYM2("ln10",M_LN10),
	REGCSYM2("nan",NAN),
	REGCSYM2("pi",M_PI),
	REGCSYM2("pi_2",M_PI_2),
	REGCSYM2("pi_4",M_PI_4),
	REGCSYM2("sqrt2",M_SQRT2),
	REGCSYM2("sqrt1_2",M_SQRT1_2),
	REGCSYM2("DBL_SHIFT",(double)__builtin_ctzg(sizeof(double))),
	REGCSYM2("DBL_SIZE",(double)sizeof(double)),
	REGCSYM2("jmpbuf",(double)sizeof(struct expr_internal_jmpbuf)),
	REGCSYM2("INSTLEN",(double)sizeof(struct expr_inst)),
	REGCSYM2("IPP_OFF",(double)offsetof(struct expr,ipp)),
	REGCSYM2("SIZE_OFF",(double)offsetof(struct expr,size)),
#if PHYSICAL_CONSTANT
	REGCSYM2("c",299792458.0),
	REGCSYM2("e0",8.8541878128e-12),
	REGCSYM2("e1",1.602176634e-19),
	REGCSYM2("G",6.67430e-11),
	REGCSYM2("h",6.62607015e-34),
	REGCSYM2("k",1.380649e-23),
	REGCSYM2("N_A",6.02214076e+23),
	REGCSYM2("R",8.31446261815234),
	REGCSYM2("sqrc",89875517873681764.0),
	REGCSYM2("u0",1.25663706212e-6),
#endif

#ifdef __linux__
	REGCSYM(__linux__),
#endif

#ifdef __unix__
	REGCSYM(__unix__),
#endif

#ifdef __aarch64__
	REGCSYM(__aarch64__),
#endif

#ifdef __x86_64__
	REGCSYM(__x86_64__),
#endif
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
	REGFSYM2("clz",expr_clz),
	REGFSYM(cos),
	REGFSYM(cosh),
	REGFSYM2("ctz",expr_ctz),
	REGFSYM2("dbase",expr_dbase),
	REGFSYM2("dexp",expr_dexp),
	REGFSYM2("dfact",expr_dfact),
	REGFSYM(erf),
	REGFSYM(exp),
	REGFSYM(exp2),
	REGFSYM(expm1),
	REGFSYM2("exp_old",expr_exp_old),
	REGFSYM(fabs),
	REGFSYM2("fact",expr_fact),
	REGFSYM2("ffs",expr_ffs),
	REGFSYM(floor),
	REGFSYM2("isfinite",expr_isfinite_b),
	REGFSYM2("isinf",expr_isinf_b),
	REGFSYM2("isnan",expr_isnan_b),
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
	REGFSYM2_UA("rand48",expr_rand48),
	REGFSYM2_UA("drand48",expr_drand48),
	REGFSYM2_UA("lrand48",expr_lrand48),
	REGFSYM2_UA("mrand48",expr_mrand48),
	REGFSYM2_NIU("ssrand48",expr_ssrand48),
	REGFSYM2_NIU("ssdrand48",expr_ssdrand48),
	REGFSYM2_NIU("sslrand48",expr_sslrand48),
	REGFSYM2_NIU("ssmrand48",expr_ssmrand48),
	REGFSYM2_NIU("ssnext48",expr_ssnext48_b),
	REGFSYM2_NIU("ssdnext48",expr_ssdnext48),
	REGFSYM2("srand48",expr_srand48),
	REGFSYM2("drand48_next",expr_drand48_next),
	REGFSYM2("rand48_next",expr_rand48_next),
	REGMDSYM2("lrand48_next",expr_lrand48_next,0),
	REGMDSYM2("lrand48_state",expr_lrand48_state,0),
	REGMDSYM2("mrand48_next",expr_mrand48_next,0),
	REGMDSYM2("mrand48_state",expr_mrand48_state,0),

	REGZASYM2_U("abort",(double (*)(void))abort),
//	REGZASYM(drand48),
	REGZASYM2_U("explode",(double (*)(void))expr_explode),
	REGZASYM2_U("frame",expr_frame),
//	REGZASYM2("lrand48",expr_lrand48),
//	REGZASYM2("mrand48",expr_mrand48),
	REGZASYM2_U("trap",(double (*)(void))expr_trap),
	REGZASYM2_U("ubehavior",(double (*)(void))expr_ubehavior),

	REGFSYM2_NI("exit",expr_exit),
	REGFSYM2_NI("exitif",expr_exitif),
	REGFSYM2_NI("malloc",expr_malloc),
	REGMDSYM2_NIU("realloc",expr_realloc,2),
	REGFSYM2_NI("new",expr_new),
	REGFSYM2_NI("free",expr_bxfree),
	REGFSYM2_NI("system",expr_system),

	REGMDSYM2("add",expr_add,0),
	REGMDSYM2("and",expr_and,0),
	REGMDSYM2("or",expr_or,0),
	REGMDSYM2("xor",expr_xor,0),
	REGMDSYM2("cmp",expr_cmp,2),
	REGMDSYM2("gcd",expr_gcd,0),
	REGMDSYM2_U("hgmed",expr_hgmed,0),
	REGMDSYM2_U("hmed",expr_hmed,0),
	REGMDSYM2_U("hmode",expr_hmode,0),
	REGMDSYM2("hypot",expr_hypot,0),
	REGMDSYM2("lcm",expr_lcm,0),
	REGMDSYM2_U("long",expr_strtol,2),
	REGMDSYM2("max",expr_max,0),
	REGMDSYM2_U("med",expr_med,0),
	REGMDSYM2("med_old",expr_med_old,0),
	REGMDSYM2_U("gmed",expr_gmed,0),
	REGMDSYM2("gmed_old",expr_gmed_old,0),
	REGMDSYM2("min",expr_min,0),
	REGMDSYM2_U("mode",expr_mode,0),
	REGMDSYM2("mul",expr_mul,0),
	REGMDSYM2("nfact",expr_nfact,2ul),
	REGMDSYM2("pow_old_n",expr_pow_old_n,2),
	REGMDSYM2("qmed",expr_qmed,0),
	REGMDSYM2("qgmed",expr_qgmed,0),
	REGMDSYM2("qmode",expr_qmode,0),

	REGMDSYM2_NIU("syscall",bsyscall,0),
	REGFSYM_U(systype),
	REGCSYM2("sysp0",INT64_C(1)<<32),
	REGCSYM2("sysp1",INT64_C(1)<<33),
	REGCSYM2("sysp2",INT64_C(1)<<34),
	REGCSYM2("sysp3",INT64_C(1)<<35),
	REGCSYM2("sysp4",INT64_C(1)<<36),
	REGCSYM2("sysp5",INT64_C(1)<<37),
	REGCSYM2("sysp6",INT64_C(1)<<38),

	REGMDEPSYM2_NIW("assert",bassert,1ul),
	REGMDEPSYM2_NI("ldr",expr_ldr,2ul),
	REGMDEPSYM2_NI("str",expr_str,3ul),
	REGMDEPSYM2_NI("bzero",expr_bzero,2ul),
	REGMDEPSYM2_NI("contract",bcontract,2ul),
	REGMDEPSYM2_NI("fry",bfry,2ul),
	REGMDEPSYM2_NI("memset",expr_memset,3ul),
	REGMDEPSYM2_NI("mirror",bmirror,2ul),
	REGMDEPSYM2_NI("sort",bsort,2ul),
	REGMDEPSYM2_NI("hsort",bhsort,2ul),
	REGMDEPSYM2_NI("xsort",bxsort,2ul),
	REGMDEPSYM2_NI("sort_old",bsort_old,2ul),
	REGMDSYM2_NIU("destruct",expr_destruct,0),
	REGMDSYM2_NIU("longjmp_out",expr_longjmp_out,0),

	REGMEM(8),
	REGMEM(16),
	REGMEM(32),
	REGMEM(64),
	REGMEM(m),
	REGMEM(p),
	REGMEM(z),
	REGMEM(8u),
	REGMEM(16u),
	REGMEM(32u),
	REGMEM(64u),
	REGMEM(mu),
	REGMEM(pu),
	REGMEM(zu),
	REGFMEM(f),
	REGFMEM(d),
	REGFMEM(l),

	REGMDEPSYM2_NI("assign",expr_assign,0),
	REGMDEPSYM2_NI("assigni",expr_assigni,0),
	REGMDEPSYM2_NI("assignu",expr_assignu,0),
	REGMDEPSYM2("andl",expr_andl,0),
	REGMDEPSYM2("findbound2",expr_findbound2,3),
	REGMDEPSYM2("orl",expr_orl,0),
	REGMDEPSYM2("piece",expr_piece,0),
	REGMDEPSYM2("d",expr_derivate,0),
	REGMDEPSYM2_U("dn",expr_multi_derivate,0),
	REGMDEPSYM2("root",expr_root,0),
	REGMDEPSYM2("root2",expr_root2,0),
	REGMDEPSYM2("rooti",expr_rooti,0),
	{.str=NULL}
};
#define arrsize(arr) (sizeof(arr)/sizeof(*arr))
const size_t expr_symbols_size=arrsize(expr_symbols)-1;
