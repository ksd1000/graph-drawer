/*******************************************************************************
 *License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>*
 *This is free software: you are free to change and redistribute it.           *
 *******************************************************************************/
#define _GNU_SOURCE
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>
#include <float.h>
#include <limits.h>
#include <err.h>
#include "expr.h"
#define printval(x) fprintf(stderr,#x ":%lu\n",x)
#define printvall(x) fprintf(stderr,#x ":%ld\n",x)
#define printvald(x) fprintf(stderr,#x ":%lf\n",x)
#define SYMDIM(sp) (*(size_t *)((sp)->str+(sp)->strlen+1))
#define NDEBUG
#include <assert.h>
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
	[EXPR_EVMD]="Not a multi-demension function with dim 0"
};
const char *expr_error(int error){
	if(error<0)error=-error;
	if(error>=(sizeof(eerror)/sizeof(*eerror)))return eerror[0];
	else return eerror[error];
}
static __attribute__((always_inline)) int expr_equal(double a,double b){
	double absa,absb,absamb;
	absa=a<0.0?-a:a;
	absb=b<0.0?-b:b;
	absamb=a<b?b-a:a-b;
	if(absa>absb){
		if(absa<=1.0)
			return absamb<=DBL_EPSILON;
		return absamb<=DBL_EPSILON*absa;
	}else {
		if(absb<=1.0)
			return absamb<=DBL_EPSILON;
		return absamb<=DBL_EPSILON*absb;
	}
}
static int expr_space(char c){
	switch(c){
		case '\t':
		case '\r':
		case '\v':
		case '\f':
		case '\n':
		case '\b':
		case ' ':
			return 1;
		default:
			return 0;
	}
}
static int expr_operator(char c){
	switch(c){
		case '+':
		case '-':
		case '*':
		case '/':
		case '%':
		case '^':
		case '(':
		case ')':
		case ',':
		case '<':
		case '>':
		case '=':
		case '!':
		case '&':
		case '|':
			return 1;
		default:
			return 0;
	}
}
//static const char spaces[]={" \t\r\f\n\v"};
//static const char special[]={"+-*/%^(),<>=!&|"};
const static char ntoc[]={"0123456789abcdefg"};

//#define MEMORY_LEAK_CHECK

#ifdef MEMORY_LEAK_CHECK
static volatile _Atomic int count=0;
static void xfree(void *p){
	free(p);
	--count;
}
static void __attribute__((destructor)) show(void){
	warnx("MEMORY LEAK COUNT:%d",count);
}
#define free(p) xfree(p)
#endif
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
	r=realloc(old,size);
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
static int xasprintf(char **restrict strp, const char *restrict fmt,...){
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
}
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
double expr_gcd2(double x,double y){
	int r1=(fabs(x)<fabs(y));
	while(x&&y){
		if(r1^=1)x=fmod(x,y);
		else y=fmod(y,x);
	}
	return x?x:y;
}
double expr_lcm2(double x,double y){
	return x*y/expr_gcd2(x,y);
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
struct dnode {
	struct dnode *lt,*gt;
	size_t eq;
	double val;
};
static void expr_sort3_write(double **dst,const struct dnode *dnp){
	size_t eq;
	if(dnp->lt)
		expr_sort3_write(dst,dnp->lt);
	eq=dnp->eq;
	do
		*((*dst)++)=dnp->val;
	while(--eq);
	if(dnp->gt)
		expr_sort3_write(dst,dnp->gt);
}
__attribute__((noinline))
void *expr_sort3(double *v,size_t n,void *(*allocator)(size_t)){
	struct dnode *top,*dnp;
	double *restrict p;
	double *endp=v+n;
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
	goto create;
	for(;p<endp;++p){
		for(struct dnode *p1=dnp;;){
			if(*p==p1->val){
				++p1->eq;
				break;
			}else if((*p>p1->val)){
				if(p1->gt){
					p1=p1->gt;
					continue;
				}
				p1->gt=++top;
				goto create;
			}else {
				if(p1->lt){
					p1=p1->lt;
					continue;
				}
				p1->lt=++top;
				goto create;
			}
		}
		continue;
create:
		top->lt=NULL;
		top->gt=NULL;
		top->eq=1;
		top->val=*p;
	}
	expr_sort3_write(&v,dnp);
	return dnp;
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
			expr_sort3(v,n,NULL);
			return;
	}
}
#define CALLOGIC(a,b,_s) ((fabs(a)>DBL_EPSILON) _s (fabs(b)>DBL_EPSILON))
#define CALBLOGIC(_sign_cal,_sign_zero,_zero_val) \
	uint64_t x2,x1;\
	int64_t expdiff=EXPR_EDEXP(&a)-EXPR_EDEXP(&b);\
	double swapbuf;\
	if(expdiff<0L){\
		swapbuf=a;\
		a=b;\
		b=swapbuf;\
		expdiff=-expdiff;\
	}\
	if(expdiff>52L)goto zero;\
	x2=(EXPR_EDBASE(&b)|(1UL<<52UL))>>expdiff;\
	x1=EXPR_EDBASE(&a)|(1UL<<52UL);\
	x1 _sign_cal x2;\
	if(x1){\
		x2=63UL-__builtin_clzl(x1);\
		x1&=~(1UL<<x2);\
		x2=52UL-x2;\
		if(EXPR_EDEXP(&a)<x2)goto zero;\
		EXPR_EDBASE(&a)=x1<<x2;\
		EXPR_EDEXP(&a)-=x2;\
	}else {\
		a=0.0;\
	}\
	EXPR_EDSIGN(&a) _sign_cal EXPR_EDSIGN(&b);\
	return a;\
zero:\
	return EXPR_EDSIGN(&a) _sign_zero EXPR_EDSIGN(&b)?-( _zero_val):(_zero_val)
double expr_and2(double a,double b){
	CALBLOGIC(&=,&&,0.0);
}
double expr_or2(double a,double b){
	CALBLOGIC(|=,||,a>=0.0?a:-a);
}
double expr_xor2(double a,double b){
	CALBLOGIC(^=,^,a>=0.0?a:-a);
}
static double expr_rand(size_t n,double *args){
	//assert(n==2);
	return args[0]+(args[1]-args[0])*drand48();
}
#define expr_add2(a,b) ((a)+(b))
#define expr_mul2(a,b) ((a)*(b))
#define CALMDLOGIC(_symbol)\
	double ret=*(args++);\
	while(--n>0){\
		ret= _symbol (ret,*args);\
		++args;\
	}\
	return ret
static double expr_and(size_t n,double *args){
	CALMDLOGIC(expr_and2);
}
static double expr_or(size_t n,double *args){
	CALMDLOGIC(expr_or2);
}
static double expr_xor(size_t n,double *args){
	CALMDLOGIC(expr_xor2);
}
static double expr_gcd(size_t n,double *args){
	CALMDLOGIC(expr_gcd2);
}
static double expr_lcm(size_t n,double *args){
	CALMDLOGIC(expr_lcm2);
}
static double expr_add(size_t n,double *args){
	CALMDLOGIC(expr_add2);
}
static double expr_mul(size_t n,double *args){
	CALMDLOGIC(expr_mul2);
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
	free(expr_sort3(args,n,xmalloc));
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
	free(expr_sort3(args,n,xmalloc));
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
		free(expr_sort3(args,n,xmalloc));
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
	if(x>DBL_EPSILON)return 1.0;
	else if(x<-DBL_EPSILON)return -1.0;
	else return 0.0;
}
#define expr_not(x) expr_xor2(x,9007199254740991.0/* 2^53-1*/)
static double expr_notfunc(double x){
	return expr_not(x);
}
static double expr_nnot(double x){
	if(fabs(x)>DBL_EPSILON)return 1.0;
	else return 0.0;
}
static double expr_fact(double x){
	double sum=1.0;
	x=floor(x);
	while(x>DBL_EPSILON){
		sum*=x;
		x-=1.0;
	}
	return sum;
}
static double expr_dfact(double x){
	double sum=1.0;
	x=floor(x);
	while(x>DBL_EPSILON){
		sum*=x;
		x-=2.0;
	}
	return sum;
}
static double expr_nfact(size_t n,double *args){
	double sum=1.0,x=args[0];
	x=floor(x);
	while(x>DBL_EPSILON){
		sum*=x;
		x-=args[1];
	}
	return sum;
}
static double expr_piece(size_t n,const struct expr *args,double input){
	const struct expr *arg0=args;
	--n;
	while(args-arg0<n){
		if(fabs(expr_eval(args++,input))>DBL_EPSILON){
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
	return expr_multilevel_derivate(args,input,(long)(level+DBL_EPSILON),epsilon);
}

static double expr_strlen(size_t n,const struct expr *args,double input){
	return (double)n;
}
static double expr_strchr(size_t n,const struct expr *args,double input){
	double c=expr_eval(args,input);
	const struct expr *s0=++args;
	while(--n){
		if(expr_eval(args,input)==c)
			return (double)(args-s0);
		++args;
	}
	return -1.0;
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
	double epsilon=DBL_EPSILON,from=1.0,swapbuf;
	switch(n){
		case 3:
			epsilon=fabs(expr_eval(args+4,input));
		case 2:
			from=expr_eval(args+1,input);
		case 1:
			break;
		default:
			return NAN;
	}
	for(;;){
		swapbuf=expr_multilevel_derivate(args,from,1,epsilon);
		if(fabs(swapbuf)==0.0){
			if(fabs(expr_eval(args,from))<=epsilon)
				return from;
			break;
		}
		swapbuf=from-expr_eval(args,from)/swapbuf;
		if(fabs(from-swapbuf)<=epsilon)return swapbuf;
		from=swapbuf;
	}
	return INFINITY;
}
static double expr_andl(size_t n,const struct expr *args,double input){
	for(const struct expr *ep=args;ep-args<n;++ep){
		if(fabs(expr_eval(ep,input))<=DBL_EPSILON)
			return 0.0;
	}
	return 1.0;
}
static double expr_orl(size_t n,const struct expr *args,double input){
	for(const struct expr *ep=args;ep-args<n;++ep){
		if(fabs(expr_eval(ep,input))>DBL_EPSILON)
			return 1.0;
	}
	return 0.0;
}
static double expr_max(size_t n,double *args){
	double ret=DBL_MIN;
	while(n>0){
		//printf("%lf\n",*args);
		if(*args>ret)ret=*args;
		--n;
		++args;
	}
	return ret;
}
static double expr_min(size_t n,double *args){
	double ret=DBL_MAX;
	while(n>0){
		//printf("%lf\n",*args);
		if(*args<ret)ret=*args;
		--n;
		++args;
	}
	return ret;
}

static double expr_hypot(size_t n,double *args){
	double ret=0;
	while(n>0){
		//printf("%lf\n",*args);
		ret+=*args**args;
		--n;
		++args;
	}
	return sqrt(ret);
}
//#define REGSYM(s) {#s,s}
#define REGZASYM(s) {.strlen=sizeof(#s)-1,.str=#s,.un={.zafunc=s},.type=EXPR_ZAFUNCTION,.flag=0}
#define REGFSYM(s) {.strlen=sizeof(#s)-1,.str=#s,.un={.func=s},.type=EXPR_FUNCTION,.flag=EXPR_SF_INJECTION}
#define REGCSYM(s) {.strlen=sizeof(#s)-1,.str=#s,.un={.value=s},.type=EXPR_CONSTANT}
#define REGFSYM2(s,sym) {.strlen=sizeof(s)-1,.str=s,.un={.func=sym},.type=EXPR_FUNCTION,.flag=EXPR_SF_INJECTION}
#define REGZASYM2(s,sym) {.strlen=sizeof(s)-1,.str=s,.un={.zafunc=sym},.type=EXPR_ZAFUNCTION,.flag=0}
#define REGMDSYM2(s,sym,d) {.strlen=sizeof(s)-1,.str=s,.un={.mdfunc=sym},.dim=d,.type=EXPR_MDFUNCTION}
#define REGMDEPSYM2(s,sym,d) {.strlen=sizeof(s)-1,.str=s,.un={.mdepfunc=sym},.dim=d,.type=EXPR_MDEPFUNCTION}
#define REGCSYM2(s,val) {.strlen=sizeof(s)-1,.str=s,.un={.value=val},.type=EXPR_CONSTANT}
#define REGKEY(s,op,dim,desc) {s,op,dim,sizeof(s)-1,desc}
const struct expr_builtin_keyword expr_keywords[]={
	REGKEY("sum",EXPR_SUM,5,"index_name,start_index,end_index,index_step,addend"),
	REGKEY("int",EXPR_INT,5,"integral_var_name,upper_limit,lower_limit,epsilon,integrand"),
	REGKEY("prod",EXPR_PROD,5,"index_name,start_index,end_index,index_step,factor"),
	REGKEY("pai",EXPR_PROD,5,"index_name,start_index,end_index,index_step,factor"),
	REGKEY("sup",EXPR_SUP,5,"index_name,start_index,end_index,index_step,element"),
	REGKEY("infi",EXPR_INF,5,"index_name,start_index,end_index,index_step,element"),
	REGKEY("andn",EXPR_ANDN,5,"index_name,start_index,end_index,index_step,element"),
	REGKEY("orn",EXPR_ORN,5,"index_name,start_index,end_index,index_step,element"),
	REGKEY("xorn",EXPR_XORN,5,"index_name,start_index,end_index,index_step,element"),
	REGKEY("gcdn",EXPR_GCDN,5,"index_name,start_index,end_index,index_step,element"),
	REGKEY("lcmn",EXPR_LCMN,5,"index_name,start_index,end_index,index_step,element"),
	REGKEY("for",EXPR_FOR,5,"var_name,start_var,cond,body,value"),
	REGKEY("loop",EXPR_LOOP,5,"var_name,start_var,count,body,value"),
	REGKEY("while",EXPR_WHILE,3,"cond,body,value"),
	REGKEY("if",EXPR_IF,3,"cond,if_value,else_value"),
	REGKEY("vmd",EXPR_VMD,7,"index_name,start_index,end_index,index_step,element,md_symbol,max_dim"),
	{NULL}
};
const struct expr_builtin_symbol expr_bsyms[]={
	REGFSYM2("abs",fabs),
	REGFSYM(acos),
	REGFSYM(acosh),
	REGFSYM(asin),
	REGFSYM(asinh),
	REGFSYM(atan),
	REGFSYM(atanh),
	REGFSYM(cbrt),
	REGFSYM(ceil),
	REGFSYM(cos),
	REGFSYM(cosh),
	REGFSYM2("dfact",expr_dfact),
	REGFSYM(erf),
	REGFSYM(exp),
	REGFSYM(exp2),
	REGFSYM(expm1),
	REGFSYM(fabs),
	REGFSYM2("fact",expr_fact),
	REGFSYM(floor),
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
	REGFSYM2("nnot",expr_nnot),
	REGFSYM2("not",expr_notfunc),
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
	REGZASYM(drand48),
	REGZASYM2("lrand48",expr_lrand48),
	REGZASYM2("mrand48",expr_mrand48),

	REGCSYM(DBL_MAX),
	REGCSYM(DBL_MIN),
	REGCSYM(DBL_EPSILON),
	REGCSYM(HUGE_VAL),
	REGCSYM(FLT_MAX),
	REGCSYM(FLT_MIN),
	REGCSYM(FLT_EPSILON),
	REGCSYM(HUGE_VALF),
	REGCSYM(INFINITY),
	REGCSYM2("inf",INFINITY),
	REGCSYM(NAN),
	REGCSYM2("nan",NAN),
	REGCSYM2("e",M_E),
	REGCSYM2("log2e",M_LOG2E),
	REGCSYM2("log10e",M_LOG10E),
	REGCSYM2("ln2",M_LN2),
	REGCSYM2("ln10",M_LN10),
	REGCSYM2("pi",M_PI),
	REGCSYM2("pi_2",M_PI_2),
	REGCSYM2("pi_4",M_PI_4),
	REGCSYM2("1_pi",M_1_PI),
	REGCSYM2("2_pi",M_2_PI),
	REGCSYM2("2_sqrtpi",M_2_SQRTPI),
	REGCSYM2("sqrt2",M_SQRT2),
	REGCSYM2("sqrt1_2",M_SQRT1_2),

	REGMDSYM2("add",expr_add,0),
	REGMDSYM2("and",expr_and,0),
	REGMDSYM2("or",expr_or,0),
	REGMDSYM2("xor",expr_xor,0),
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
	REGMDSYM2("rand",expr_rand,2ul),

	REGMDEPSYM2("andl",expr_andl,0),
	REGMDEPSYM2("orl",expr_orl,0),
	REGMDEPSYM2("piece",expr_piece,0),
	REGMDEPSYM2("d",expr_derivate,0),
	REGMDEPSYM2("dn",expr_multi_derivate,0),
	REGMDEPSYM2("strchr",expr_strchr,0),
	REGMDEPSYM2("strlen",expr_strlen,0),
	REGMDEPSYM2("root",expr_root,0),
	REGMDEPSYM2("root2",expr_root2,0),
	REGMDEPSYM2("rooti",expr_rooti,0),
	{.str=NULL}
};
const struct expr_builtin_symbol *expr_bsym_search(const char *sym,size_t sz){
	for(const struct expr_builtin_symbol *p=expr_bsyms;p->str;++p){
		//printf("sz=%zu,ps=%u %s\n",sz,p->strlen,p->str);
		if(sz==p->strlen&&!memcmp(p->str,sym,sz)){
			return p;
		}
	}
	return NULL;
}
const struct expr_builtin_symbol *expr_bsym_rsearch(void *addr){
	for(const struct expr_builtin_symbol *p=expr_bsyms;p->str;++p){
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
size_t expr_strscan(const char *s,size_t sz,char *buf){
	char *buf0=buf;
	const char *p,*endp=s+sz;
	if(!sz||*s!='\"')return 0;
	for(;;){
	while(s<endp&&*s!='\"'&&!expr_space(*s))++s;
	if(!(s<endp))return buf-buf0;
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
	buf=xmalloc(sz);
	*outsz=expr_strscan(s,sz,buf);
	buf[*outsz]=0;
	if(!*buf){
		free(buf);
		return NULL;
	}
	return buf;
}
void expr_free(struct expr *restrict ep){
	struct expr_inst *ip,*endp;
	if(ep->data){
		ip=ep->data;
		endp=ip+ep->size;
		for(;ip<endp;++ip){
			switch(ip->op){
				case EXPR_SUM:
				case EXPR_INT:
				case EXPR_PROD:
				case EXPR_SUP:
				case EXPR_INF:
				case EXPR_ANDN:
				case EXPR_ORN:
				case EXPR_XORN:
				case EXPR_GCDN:
				case EXPR_LCMN:
				case EXPR_FOR:
				case EXPR_LOOP:
					expr_free(ip->un.es->ep);
					expr_free(ip->un.es->from);
					expr_free(ip->un.es->to);
					expr_free(ip->un.es->step);
					free(ip->un.es);
					break;
				case EXPR_MD:
					free(ip->un.em->args);
				case EXPR_ME:
					for(size_t i=0;i<ip->un.em->dim;++i)
						expr_free(ip->un.em->eps+i);
					free(ip->un.em->eps);
					free(ip->un.em);
					break;
				case EXPR_VMD:
					expr_free(ip->un.ev->ep);
					expr_free(ip->un.ev->from);
					expr_free(ip->un.ev->to);
					expr_free(ip->un.ev->step);
					if(ip->un.ev->args)
						free(ip->un.ev->args);
					free(ip->un.ev);
					break;
				case EXPR_IF:
				case EXPR_WHILE:
					expr_free(ip->un.eb->cond);
					expr_free(ip->un.eb->body);
					expr_free(ip->un.eb->value);
					free(ip->un.eb);
					break;
				case EXPR_HOT:
					expr_free(ip->un.hotfunc);
					break;
				default:
					break;
			}
		}
		free(ep->data);
	}
	if(ep->vars){
		for(size_t i=0;i<ep->vsize;++i)
			free(ep->vars[i]);
		free(ep->vars);
	}
	if(ep->freeable)free(ep);
}
#define EXTEND_SIZE 1
#define EXTEND_DATA if(ep->size>=ep->length){\
	ep->data=xrealloc(ep->data,\
		(ep->length+=EXTEND_SIZE)*sizeof(struct expr_inst));\
	}
__attribute__((noinline))
struct expr_inst *expr_addop(struct expr *restrict ep,double *dst,void *src,enum expr_op op,int flag){
	struct expr_inst *ip;
	EXTEND_DATA
	ip=ep->data+ep->size++;
	ip->op=op;
	ip->dst=dst;
	ip->un.uaddr=src;
	ip->flag=flag;
	return ip;
}
static struct expr_inst *expr_addcopy(struct expr *restrict ep,double *dst,double *src){
	return expr_addop(ep,dst,src,EXPR_COPY,0);
}
static struct expr_inst *expr_addcall(struct expr *restrict ep,double *dst,double (*func)(double),int flag){
	return expr_addop(ep,dst,func,EXPR_CALL,flag);
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
static struct expr_inst *expr_addhot(struct expr *restrict ep,double *dst,struct expr *hot,int flag){
	return expr_addop(ep,dst,hot,EXPR_HOT,flag);
}
static struct expr_inst *expr_addconst(struct expr *restrict ep,double *dst,double val){
	struct expr_inst *r=expr_addop(ep,dst,NULL,EXPR_CONST,0);
	r->un.value=val;
	return r;
}
static double *expr_newvar(struct expr *restrict ep){
	double *r=xmalloc(sizeof(double));
	if(ep->vsize>=ep->vlength){
		ep->vars=xrealloc(ep->vars,
			(ep->vlength+=EXTEND_SIZE)*sizeof(double *));
	}
	*(ep->vars+ep->vsize++)=r;
	*r=NAN;
	return r;
}
static double *expr_createvar(struct expr *restrict ep,const char *symbol,size_t symlen){
	double *r=expr_newvar(ep);
	if(!ep->sset_shouldfree){
		if(!ep->sset)ep->sset=new_expr_symset();
		else ep->sset=expr_symset_clone(ep->sset);
		ep->sset_shouldfree=1;
	}
	expr_symset_addl(ep->sset,symbol,symlen,EXPR_VARIABLE,r);
	return r;
}
static const char *expr_findpair(const char *c){
	size_t lv=0;
	if(*c!='(')goto err;
	while(*c){
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
	char *p=xmalloc(sz+1);
	p[sz]=0;
	memcpy(p,str,sz);
	ret=expr_atod2(p,dst);
	free(p);
	return ret;
}
static char *expr_tok(char *restrict str,char **restrict saveptr){
	if(str){
	//	puts(str);
		*saveptr=str;
	}else if(!**saveptr)return NULL;
	else {
		str=*saveptr;
	}
	while(**saveptr){
		if(**saveptr==','){
			**saveptr=0;
			++(*saveptr);
			return str;
		}
		if(**saveptr=='('){
			*saveptr=(char *)expr_findpair(*saveptr);
			if(!*saveptr)return NULL;
		}
		++(*saveptr);
	}
	return str;
}
static void expr_free2(char **buf){
	char **p=buf;
	while(*p){
		free(*p);
		++p;
	}
	free(buf);
}
static char **expr_sep(struct expr *restrict ep,const char *pe,size_t esz){
	char *p,*p1,*p2,**p3=NULL,*p5,*e,*p6;
	size_t len=0,s,sz;
	p6=e=xmalloc(esz+1);
	memcpy(e,pe,esz);
	e[esz]=0;
	if(*e=='('){
		p1=(char *)expr_findpair(e);
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
		if(*p=='\"'&&(p5=expr_astrscan(p,s,&sz))){
			for(char *p4=p5;p4-p5<sz;++p4){
				p1=xmalloc(5);
				p1[0]='0';
				p1[1]='x';
				p1[2]=ntoc[*p4>>4];
				p1[3]=ntoc[*p4&15];
				p1[4]=0;
				p3=xrealloc(p3,(++len+1)*sizeof(char *));
				p3[len-1]=p1;
			}
			free(p5);
		}else if(*p=='{'){
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
					xasprintf(&p1,"%.64lg",f);
					p3=xrealloc(p3,(++len+1)*sizeof(char *));
					p3[len-1]=p1;
					f+=step;
				}while(f<=t);
				else
				do {
					xasprintf(&p1,"%.64lg",f);
					p3=xrealloc(p3,(++len+1)*sizeof(char *));
					p3[len-1]=p1;
					f-=step;
				}while(f>=t);
				continue;
			}
integer:
			diff=(from>to?from-to:to-from);
			p3=xrealloc(p3,(len+diff/istep+1+1)*sizeof(char *));
			if(from<=to)
			do {
				xasprintf(&p1,"%ld",from);
				p3[++len-1]=p1;
				from+=istep;
			}while(from<=to);
			else
			do {
				xasprintf(&p1,"%ld",from);
				//p3=xrealloc(p3,(++len+1)*sizeof(char *));
				p3[++len-1]=p1;
				from-=istep;
			}while(from>=to);
		}else{
normal:
			p1=xmalloc(s+1);
			p1[s]=0;
			memcpy(p1,p,s);
			p3=xrealloc(p3,(++len+1)*sizeof(char *));
			p3[len-1]=p1;
		}
	}
	if(p3)p3[len]=NULL;
	free(p6);
	return p3;
fail:
	free(p6);
	return NULL;
}

static struct expr_mdinfo *expr_getmdinfo(struct expr *restrict ep,const char *e0,size_t sz,const char *e,size_t esz,const char *asym,void *func,size_t dim,int ifep){
	char **v=expr_sep(ep,e,esz);
	char **p;
	size_t i;
	struct expr_mdinfo *em;
	if(!v){
		return NULL;
	}
	p=v;
	while(*p){
		++p;
	}
	i=p-v;
	if(dim&&i!=dim){
		memcpy(ep->errinfo,e0,sz);
		ep->error=EXPR_ENEA;
		goto err1;
	}
	em=xmalloc(sizeof(struct expr_mdinfo));
	em->dim=i;

	em->un.func=func;
	em->eps=xmalloc(em->dim*sizeof(struct expr));
	em->args=NULL;
	if(!ifep)em->args=xmalloc(em->dim*sizeof(double));
	for(i=0;i<em->dim;++i){
		if(init_expr(em->eps+i,v[i],asym,ep->sset)<0){
			for(ssize_t k=i-1;k>=0;--k)
				expr_free(em->eps+k);
			goto err2;
		}
	}
	expr_free2(v);
	return em;
err2:
	if(em->args)free(em->args);
	ep->error=em->eps[i].error;
	memcpy(ep->errinfo,em->eps[i].errinfo,EXPR_SYMLEN);
	free(em->eps);
	free(em);
err1:
	expr_free2(v);
	return NULL;
}
static struct expr_vmdinfo *expr_getvmdinfo(struct expr *restrict ep,const char *e0,size_t sz,const char *e,size_t esz,const char *asym){
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
	char c;
	if(!v){
		return NULL;
	}
	p=v;
	while(*p){
		++p;
	}
	if(p-v!=7){
		memcpy(ep->errinfo,e0,sz);
		ep->error=EXPR_ENEA;
		goto err0;
	}
	if(sscanf(v[6],"%zd%c",&max,&c)!=1){
		strcpy(ep->errinfo,v[6]);
		ep->error=EXPR_ENUMBER;
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
	}else if((sym.ebs=expr_bsym_search(v[5],ssz))){
		if(sym.ebs->type!=EXPR_MDFUNCTION||sym.ebs->dim)goto evmd;
		fp=sym.ebs->un.mdfunc;
	}else {
		memcpy(ep->errinfo,v[5],ssz);
		ep->error=EXPR_ESYMBOL;
		goto err0;
	}
	ev=xmalloc(sizeof(struct expr_vmdinfo));
	if(max>0){
		ev->args=xmalloc(max*sizeof(double));
		ev->max=max;
	}else {
		ev->args=NULL;
		ev->max=0;
	}
	ev->func=fp;
	sset=new_expr_symset();
	expr_symset_add(sset,v[0],EXPR_VARIABLE,&ev->index);
	expr_symset_copy(sset,ep->sset);
	ev->from=new_expr(v[1],asym,ep->sset,&ep->error,ep->errinfo);
	if(!ev->from)goto err1;
	ev->to=new_expr(v[2],asym,sset,&ep->error,ep->errinfo);
	if(!ev->to)goto err2;
	ev->step=new_expr(v[3],asym,sset,&ep->error,ep->errinfo);
	if(!ev->step)goto err3;
	ev->ep=new_expr(v[4],asym,sset,&ep->error,ep->errinfo);
	if(!ev->ep)goto err4;

	expr_free2(v);
	expr_symset_free(sset);
	return ev;
err4:
	expr_free(ev->step);
err3:
	expr_free(ev->to);
err2:
	expr_free(ev->from);
err1:
	free(ev);
	expr_symset_free(sset);
err0:
	expr_free2(v);
	return NULL;
}
static struct expr_suminfo *expr_getsuminfo(struct expr *restrict ep,const char *e0,size_t sz,const char *e,size_t esz,const char *asym){
	char **v=expr_sep(ep,e,esz);
	char **p;
	struct expr_suminfo *es;
	struct expr_symset *sset;
	if(!v){
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
	es=xmalloc(sizeof(struct expr_suminfo));
//	sum(sym_index,from,to,step,expression)
	sset=new_expr_symset();
	expr_symset_add(sset,v[0],EXPR_VARIABLE,&es->index);
	expr_symset_copy(sset,ep->sset);
	es->from=new_expr(v[1],asym,ep->sset,&ep->error,ep->errinfo);
	if(!es->from)goto err1;
	es->to=new_expr(v[2],asym,sset,&ep->error,ep->errinfo);
	if(!es->to)goto err2;
	es->step=new_expr(v[3],asym,sset,&ep->error,ep->errinfo);
	if(!es->step)goto err3;
	es->ep=new_expr(v[4],asym,sset,&ep->error,ep->errinfo);
	if(!es->ep)goto err4;
	expr_free2(v);
	expr_symset_free(sset);
	return es;
err4:
	expr_free(es->step);
err3:
	expr_free(es->to);
err2:
	expr_free(es->from);
err1:
	free(es);
	expr_symset_free(sset);
err0:
	expr_free2(v);
	return NULL;
}
static struct expr_branchinfo *expr_getbranchinfo(struct expr *restrict ep,const char *e0,size_t sz,const char *e,size_t esz,const char *asym){
	char **v=expr_sep(ep,e,esz);
	char **p;
	struct expr_branchinfo *eb;
	if(!v){
		return NULL;
	}
	p=v;
	while(*p){
		++p;
	}
	if(p-v!=3){
		memcpy(ep->errinfo,e0,sz);
		ep->error=EXPR_ENEA;
		goto err0;
	}
	eb=xmalloc(sizeof(struct expr_branchinfo));
//	while(cond,body,value)
	eb->cond=new_expr(v[0],asym,ep->sset,&ep->error,ep->errinfo);
	if(!eb->cond)goto err1;
	eb->body=new_expr(v[1],asym,ep->sset,&ep->error,ep->errinfo);
	if(!eb->body)goto err2;
	eb->value=new_expr(v[2],asym,ep->sset,&ep->error,ep->errinfo);
	if(!eb->value)goto err3;
	expr_free2(v);
	return eb;
err3:
	expr_free(eb->body);
err2:
	expr_free(eb->cond);
err1:
	free(eb);
err0:
	expr_free2(v);
	return NULL;
}
static double *expr_scan(struct expr *restrict ep,const char *e,size_t sz,const char *asym,size_t asymlen);
static double *expr_getvalue(struct expr *restrict ep,const char *e,const char *endp,const char **_p,const char *asym,size_t asymlen){
	const char *p,*p2;//*e0=e;
	double *v0=NULL;
	//char *buf;
	int r0;
	union {
		double v;
		void *uaddr;
		struct expr *ep;
		struct expr_suminfo *es;
		struct expr_branchinfo *eb;
		struct expr_mdinfo *em;
		struct expr_vmdinfo *ev;
	} un;
	union {
		const struct expr_symbol *es;
		const struct expr_builtin_symbol *ebs;
	} sym;
	const union expr_symvalue *sv;
	int type,flag;
	size_t dim=0;
	for(;;++e){
		if(e>=endp)goto eev;
		switch(*e){
			case 0:
eev:
				ep->error=EXPR_EEV;
				return NULL;
			case '(':
				p=expr_findpair(e);
				if(!p){
pterr:
					ep->error=EXPR_EPT;
					return NULL;
				}
				p2=e;
				if(*(++p2)==')'){
					ep->error=EXPR_ENVP;
					return NULL;
				}
				v0=expr_scan(ep,e+1,p-e-1,asym,asymlen);
				if(!v0)return NULL;
				e=p+1;
				goto vend;
			case '+':
				continue;
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
		if(p==e){
			if(e<endp&&expr_operator(*e)){
				*ep->errinfo=*e;
				ep->error=EXPR_EUO;
				return NULL;
			}
			goto symerr;
		}
		for(const struct expr_builtin_keyword *kp=expr_keywords;
				kp->str;++kp){
			if(p-e!=kp->strlen||memcmp(e,kp->str,p-e))
				continue;
			if(*p!='('){
				memcpy(ep->errinfo,e,p-e);
				ep->error=EXPR_EFP;
				//assert(0);
				return NULL;
			}
			p2=e;
			e=p;
			p=expr_findpair(e);
			if(!p)goto pterr;
			switch(kp->op){
				case EXPR_IF:
				case EXPR_WHILE:
					un.eb=expr_getbranchinfo(ep,p2,e-p2,e,p-e+1,asym);
					break;
				case EXPR_VMD:
					un.ev=expr_getvmdinfo(ep,p2,e-p2,e,p-e+1,asym);
					break;
				default:
					un.es=expr_getsuminfo(ep,p2,e-p2,e,p-e+1,asym);
					break;
			}
			if(!un.uaddr){
				/*if(ep->error==EXPR_ENEA)
					memcpy(ep->errinfo,p2,e-p2);*/
				return NULL;
			}
			v0=expr_newvar(ep);
			expr_addop(ep,v0,un.uaddr,kp->op,0);
			e=p+1;
			goto vend;
		}
		if(asym&&p-e==asymlen&&!memcmp(e,asym,p-e)){
			v0=expr_newvar(ep);
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
			sv=&sym.es->un;
			flag=sym.es->flag;
		}else if((sym.ebs=expr_bsym_search(e,p-e))){
			type=sym.ebs->type;
			sv=&sym.ebs->un;
			dim=sym.ebs->dim;
			flag=sym.ebs->flag;
		}else goto number;
		switch(type){
			case EXPR_FUNCTION:
				if(*p!='('){
					memcpy(ep->errinfo,e,p-e);
					ep->error=EXPR_EFP;
					return NULL;
				}
				v0=expr_getvalue(ep,p,endp,&e,asym,asymlen);
				if(!v0)return NULL;
				expr_addcall(ep,v0,sv->func,flag);
				break;
			case EXPR_ZAFUNCTION:
				if(*p!='('||p+1>=endp||p[1]!=')'){
					memcpy(ep->errinfo,e,p-e);
					ep->error=EXPR_EZAFP;
					return NULL;
				}
				v0=expr_newvar(ep);
				expr_addza(ep,v0,sv->zafunc,flag);
				e=p+2;
				break;
			case EXPR_HOTFUNCTION:
				if(*p!='('){
					memcpy(ep->errinfo,e,p-e);
					ep->error=EXPR_EFP;
					return NULL;
				}
				v0=expr_getvalue(ep,p,endp,&e,asym,asymlen);
				if(!v0)return NULL;
				un.ep=new_expr(sv->hotexpr,sv->hotexpr+
					strlen(sv->hotexpr)+1
					,ep->sset,&ep->error,ep->errinfo);
				if(!un.ep)return NULL;
				expr_addhot(ep,v0,un.ep,flag);
				break;
			case EXPR_CONSTANT:
				v0=expr_newvar(ep);
				expr_addconst(ep,v0,sv->value);
				e=p;
				break;
			case EXPR_VARIABLE:
				v0=expr_newvar(ep);
				expr_addcopy(ep,v0,(void *)sv->addr);
				e=p;
				break;
			case EXPR_MDFUNCTION:
			case EXPR_MDEPFUNCTION:
				if(*p!='('){
					memcpy(ep->errinfo,e,p-e);
					ep->error=EXPR_EFP;
					return NULL;
				}
				p2=e;
				e=p;
				p=expr_findpair(e);
				if(!p)goto pterr;
				switch(type){
					case EXPR_MDFUNCTION:
						un.em=expr_getmdinfo(ep,p2,e-p2,e,p-e+1,asym,sv->mdfunc,dim,0);
					break;
					case EXPR_MDEPFUNCTION:
						un.em=expr_getmdinfo(ep,p2,e-p2,e,p-e+1,asym,sv->mdepfunc,dim,1);
					break;
				}
				if(!un.em){
					/*if(ep->error==EXPR_ENEA)
					memcpy(ep->errinfo,p2,e-p2);*/
					return NULL;
				}
				v0=expr_newvar(ep);
				switch(type){
					case EXPR_MDFUNCTION:
						expr_addmd(ep,v0,un.em,flag);
						break;
					case EXPR_MDEPFUNCTION:
						expr_addme(ep,v0,un.em,flag);
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
			expr_addconst(ep,v0,un.v);
			e=p;
			goto vend;
		}else if(r0>1){
			memcpy(ep->errinfo,e,p-e);
			ep->error=EXPR_ENUMBER;
			return NULL;
		}
symerr:
		memcpy(ep->errinfo,e,p-e);
		ep->error=EXPR_ESYMBOL;
		return NULL;
	}
vend:
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
	p=xmalloc(sizeof(struct expr_vnode));
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
	p->next=xmalloc(sizeof(struct expr_vnode));
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
			//if(!p->unary)continue;
			goto redo;
		case 4:
			//printf("is %d\n",p->unary);
			switch(p->unary&3){
				case 1:
					expr_addneg(ep,p->v);
					break;
				default:
					continue;
			}
			r=1;
			p->unary>>=2;
			//if(!p->unary)continue;
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

	free(p);
}
static void expr_vnfree(struct expr_vnode *vp){
	struct expr_vnode *p;
	while(vp){
		p=vp->next;
		free(vp);
		vp=p;
	}
}
static double *expr_scan(struct expr *restrict ep,const char *e,size_t sz,const char *asym,size_t asymlen){
	double *v1;
	const char *e0=e,*endp=e+sz;
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
	//v1=NULL;
	v1=expr_getvalue(ep,e,endp,&e,asym,asymlen);
	if(!v1)goto err;
	ev=expr_vnadd(ev,v1,op,unary);
	if(e>=endp)goto end2;
	op=EXPR_END;
	for(;e-e0<sz;){
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
					e+=2;
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
					if(p1-e==asymlen&&!memcmp(e,asym,p1-e))
						goto tnv;
					if(ep->sset)
					esp=expr_symset_search(ep->sset,e,p1-e);
					if(!esp){
						if(expr_bsym_search(e,p1-e))
							goto tnv;
						ep->error=EXPR_ESYMBOL;
						memcpy(ep->errinfo,e,p1-e);
						goto err;
					}
					if(esp->type!=EXPR_VARIABLE){
tnv:
						ep->error=EXPR_ETNV;
						memcpy(ep->errinfo,e,p1-e);
						goto err;
					}
					expr_addcopy(ep,esp->un.addr,v1);
					e=p1;
					continue;
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
					||(expr_bsym_search(e,p1-e)
					||(ep->sset&&expr_symset_search(ep->sset,e,p1-e)))){
						ep->error=EXPR_EDS;
						memcpy(ep->errinfo,e,p1-e);
						goto err;
					}
					v2=expr_createvar(ep,e,p1-e);
					expr_addcopy(ep,v2,v1);
					e=p1;
					continue;
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
			case ',':
				op=EXPR_COPY;
				++e;
				goto end1;
			case ')':
				if(!expr_unfindpair(e0,e)){
					ep->error=EXPR_EPT;
					goto err;
				}
			default:
				goto euo;
		}
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
	free(esp);
}
void expr_symset_free(struct expr_symset *restrict esp){
	if(esp->syms)
		expr_symbol_free(esp->syms);
	if(esp->freeable)free(esp);
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
		*sum=r;
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
		//printf("diff:%d\n",r);
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
	memset(&ep->next,0,sizeof(ep->next));
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
			free(ep);
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
	struct expr_symbol *restrict *ep=expr_symset_findtail(esp,es->str,es->strlen,&depth);
	if(ep){
		*ep=xmalloc(es->length);
	}else {
		return NULL;
	}
	memcpy(*ep,es,es->length);
	memset(&(*ep)->next,0,sizeof((*ep)->next));
	++esp->size;
	esp->length+=es->length;
	if(depth>esp->depth)esp->depth=depth;
	return *ep;
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
static size_t expr_strcpy_nospace(char *restrict s1,const char *restrict s2){
	const char *s20=s2,*s10=s1;
	int instr=0;
	for(;*s2;++s2){
		if(*s2=='\"'&&s2>s20&&s2[-1]!='\\')instr^=1;
		if(!instr&&expr_space(*s2))continue;
		*(s1++)=*s2;
	}
	*s1=0;
	return s1-s10;
}
static void expr_optimize(struct expr *restrict ep);
int init_expr5(struct expr *restrict ep,const char *e,const char *asym,struct expr_symset *esp,int flag){
	double *p;
	size_t r;
	char *ebuf;
	/*ep->data=NULL;
	ep->vars=NULL;
	ep->sset_shouldfree=0;
	ep->error=0;
	ep->freeable=0;
	memset(ep->errinfo,0,EXPR_SYMLEN);
	ep->length=ep->size=ep->vlength=ep->vsize=0;*/
	memset(ep,0,sizeof(struct expr));
	ep->sset=esp;
	ep->iflag=(short)flag;
	if(e){
		ebuf=xmalloc(strlen(e)+1);
		r=expr_strcpy_nospace(ebuf,e);
		//ebuf[r]='k';
		p=expr_scan(ep,ebuf,r,asym,asym?strlen(asym):0);
		free(ebuf);
		if(ep->sset_shouldfree){
			expr_symset_free(ep->sset);
		}
		if(p){
			expr_addend(ep,p);
		}else {
			expr_free(ep);
			return -1;
		}
	}
	if(!(flag&EXPR_IF_NOOPTIMIZE))
		expr_optimize(ep);
	if(flag&EXPR_IF_INSTANT_FREE)
		expr_free(ep);
	return 0;
}
int init_expr(struct expr *restrict ep,const char *e,const char *asym,struct expr_symset *esp){
	return init_expr5(ep,e,asym,esp,0);
}
struct expr *new_expr6(const char *e,const char *asym,struct expr_symset *esp,int flag,int *error,char errinfo[EXPR_SYMLEN]){

	struct expr *ep=xmalloc(sizeof(struct expr));
	if(init_expr5(ep,e,asym,esp,flag)){
		if(error)*error=ep->error;
		if(errinfo)memcpy(errinfo,ep->errinfo,EXPR_SYMLEN);
		free(ep);
		return NULL;
	}
	if(flag&EXPR_IF_INSTANT_FREE)
		free(ep);
	else
		ep->freeable=1;
	return ep;
}
struct expr *new_expr(const char *e,const char *asym,struct expr_symset *esp,int *error,char errinfo[EXPR_SYMLEN]){
	return new_expr6(e,asym,esp,0,error,errinfo);
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
static int expr_vcheck_ep(struct expr_inst *ip0,double *v);
static int expr_varofep(const struct expr *restrict ep,double *v){
	for(size_t i=0;i<ep->vsize;++i){
		if(ep->vars[i]==v){
			return 1;
		}
	}
	return 0;
}
static void expr_writeconsts(struct expr *restrict ep){
	for(struct expr_inst *ip=ep->data;ip->op!=EXPR_END;++ip){
		if(ip->op==EXPR_CONST&&ip->dst&&
			expr_varofep(ep,ip->dst)
			){
			*ip->dst=ip->un.value;
		}
	}
}
static void expr_optimize_completed(struct expr *restrict ep){
	struct expr_inst *cip=ep->data;
	for(struct expr_inst *ip=cip;ip-ep->data<ep->size;++ip){
		if(ip->dst){
			if(ip>cip)
				memcpy(cip,ip,sizeof(struct expr_inst));
			++cip;
		}
	}
	ep->size=cip-ep->data;
	//expr_writeconsts(ep);
}
static int expr_modified(const struct expr *restrict ep,double *v){
	if(!expr_varofep(ep,v))
		return 1;
	for(struct expr_inst *ip=ep->data;ip->op!=EXPR_END;++ip){
		if(ip->dst==v&&ip->op!=EXPR_CONST){
			return 1;
		}
	}
	return 0;
}
static int expr_usesrc(enum expr_op op);

static struct expr_inst *expr_findconst(const struct expr *restrict ep,struct expr_inst *ip){
	double *s=ip->dst;
	for(--ip;ip>=ep->data;--ip){
		if(expr_usesrc(ip->op)&&ip->un.src==s)break;
		if(ip->dst!=s)continue;
		if(ip->op==EXPR_CONST)return ip;
		else break;
	}
	return NULL;
}
static void expr_optimize_contadd(struct expr *restrict ep){
	double sum;
	double *nv;
	struct expr_inst *rip;
	for(struct expr_inst *ip=ep->data;ip->op!=EXPR_END;++ip){
		if(!ip->dst
			||!expr_modified(ep,ip->dst)
			||(ip->op!=EXPR_ADD&&ip->op!=EXPR_SUB)
			||expr_modified(ep,ip->un.src)
			)continue;
		sum=ip->op==EXPR_ADD?*ip->un.src:-*ip->un.src;
		for(struct expr_inst *ip1=ip+1;ip1->op!=EXPR_END;++ip1){
			if(ip1->dst!=ip->dst)continue;
			if(ip1->op!=EXPR_ADD
				&&ip1->op!=EXPR_SUB
				)break;
			if(!expr_modified(ep,ip1->un.src)){
				if(ip1->op==EXPR_ADD)
					sum+=*ip1->un.src;
				else
					sum-=*ip1->un.src;
				ip1->dst=NULL;
			}
		}
		rip=expr_findconst(ep,ip);
		if(rip){
			rip->un.value+=sum;
			ip->dst=NULL;
			expr_writeconsts(ep);
		}else {
			nv=expr_newvar(ep);
			ip->un.src=nv;
			if(sum<0.0){
				*nv=-sum;
				ip->op=EXPR_SUB;
			}
			else {
				*nv=sum;
				ip->op=EXPR_ADD;
			}
		}
	}
	expr_optimize_completed(ep);
}
static void expr_optimize_contsh(struct expr *restrict ep){
	double sum;
	double *nv;
	struct expr_inst *rip;
	for(struct expr_inst *ip=ep->data;ip->op!=EXPR_END;++ip){
		if(!ip->dst
			||!expr_modified(ep,ip->dst)
			||(ip->op!=EXPR_SHL&&ip->op!=EXPR_SHR)
			||expr_modified(ep,ip->un.src)
			)continue;
		sum=ip->op==EXPR_SHL?*ip->un.src:-*ip->un.src;
		for(struct expr_inst *ip1=ip+1;ip1->op!=EXPR_END;++ip1){
			if(ip1->dst!=ip->dst)continue;
			if(ip1->op!=EXPR_SHL
				&&ip1->op!=EXPR_SHR
				)break;
			if(!expr_modified(ep,ip1->un.src)){
				if(ip1->op==EXPR_SHL)
					sum+=*ip1->un.src;
				else
					sum-=*ip1->un.src;
				ip1->dst=NULL;
			}
		}
		rip=expr_findconst(ep,ip);
		if(rip){
			EXPR_EDEXP(&rip->un.value)+=(int64_t)sum;
			ip->dst=NULL;
			expr_writeconsts(ep);
		}else {
			nv=expr_newvar(ep);
			ip->un.src=nv;
			if(sum<0.0){
				*nv=-sum;
				ip->op=EXPR_SHR;
			}
			else {
				*nv=sum;
				ip->op=EXPR_SHL;
			}
		}
	}
	expr_optimize_completed(ep);
}
static void expr_optimize_contmul(struct expr *restrict ep,enum expr_op op){
	double sum;
	double *nv;
	struct expr_inst *rip;
	for(struct expr_inst *ip=ep->data;ip->op!=EXPR_END;++ip){
		if(!ip->dst
			||!expr_modified(ep,ip->dst)
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
					continue;
				default:
					break;
			}
			sum=*ip->un.src;
		}
		for(struct expr_inst *ip1=ip+!rip;ip1->op!=EXPR_END;++ip1){
			if(ip1->dst!=ip->dst)continue;
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
					sum=expr_and2(sum,*ip1->un.src);
					break;
				case EXPR_OR:
					sum=expr_or2(sum,*ip1->un.src);
					break;
				case EXPR_XOR:
					sum=expr_xor2(sum,*ip1->un.src);
					break;
				case EXPR_POW:
					sum=pow(sum,*ip1->un.src);
					break;
				case EXPR_COPY:
					sum=*ip1->un.src;
					break;
				case EXPR_GT:
					sum=sum>*ip->un.src?
						1.0:
						0.0;
					break;
				case EXPR_LT:
					sum=sum<*ip->un.src?
						1.0:
						0.0;
					break;
				case EXPR_SGE:
					sum=sum>=*ip->un.src?
						1.0:
						0.0;
					break;
				case EXPR_SLE:
					sum=sum<=*ip->un.src?
						1.0:
						0.0;
					break;
				case EXPR_GE:
					sum=sum>=*ip->un.src-DBL_EPSILON?
						1.0:
						0.0;
					break;
				case EXPR_LE:
					sum=sum<=*ip->un.src+DBL_EPSILON?
						1.0:
						0.0;
					break;
				case EXPR_SEQ:
					sum=sum==*ip->un.src?
						1.0:
						0.0;
					break;
				case EXPR_SNE:
					sum=sum!=*ip->un.src?
						1.0:
						0.0;
					break;
/*				case EXPR_EQ:
					sum=fabs(sum-*ip->un.src)<=DBL_EPSILON?
						1.0:
						0.0;
					break;
				case EXPR_NE:
					sum=fabs(sum-*ip->un.src)>DBL_EPSILON?
						1.0:
						0.0;
					break;*/
				case EXPR_EQ:
					sum=expr_equal(sum,*ip->un.src)?
						1.0:
						0.0;
					break;
				case EXPR_NE:
					sum=!expr_equal(sum,*ip->un.src)?
						1.0:
						0.0;
					break;
				case EXPR_ANDL:
					sum=CALLOGIC(sum,*ip->un.src,&&)?
						1.0:
						0.0;
					break;
				case EXPR_ORL:
					sum=CALLOGIC(sum,*ip->un.src,||)?
						1.0:
						0.0;
					break;
				case EXPR_XORL:
					sum=CALLOGIC(sum,*ip->un.src,^)?
						1.0:
						0.0;
					break;
				default:
					abort();
			}
				ip1->dst=NULL;
			}
		}
		if(rip){
			rip->un.value=sum;
			ip->dst=NULL;
			expr_writeconsts(ep);
		}else {
			nv=expr_newvar(ep);
			*nv=sum;
			ip->un.src=nv;
		}
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
				if(fabs(*ip->un.src)<=DBL_EPSILON){
					ip->op=EXPR_CONST;
					ip->un.value=0.0;
					expr_writeconsts(ep);
					r=1;
				}
				break;
			case EXPR_ORL:
				if(fabs(*ip->un.src)>DBL_EPSILON){
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
					ip->dst=NULL;
					r=1;
				}
				break;
		}
	}
	return r;
}
static void expr_optimize_const(struct expr *restrict ep){
	for(struct expr_inst *ip=ep->data;ip->op!=EXPR_END;++ip){
		if(ip->op==EXPR_CONST&&!expr_modified(ep,ip->dst)){
			*ip->dst=ip->un.value;
			ip->dst=NULL;
		}
	}
	expr_optimize_completed(ep);
}
static int expr_side(enum expr_op op){
	switch(op){
		case EXPR_COPY:
		case EXPR_INPUT:
		case EXPR_CONST:
		case EXPR_ADD:
		case EXPR_SUB:
		case EXPR_MUL:
		case EXPR_DIV:
		case EXPR_MOD:
		case EXPR_POW:
		case EXPR_AND:
		case EXPR_OR:
		case EXPR_XOR:
		case EXPR_SHL:
		case EXPR_SHR:
		case EXPR_NEG:
		case EXPR_NOT:
		case EXPR_NOTL:
		case EXPR_TSTL:
		case EXPR_GT:
		case EXPR_GE:
		case EXPR_LT:
		case EXPR_LE:
		case EXPR_SGE:
		case EXPR_SLE:
		case EXPR_SEQ:
		case EXPR_SNE:
		case EXPR_EQ:
		case EXPR_NE:
		case EXPR_ANDL:
		case EXPR_ORL:
		case EXPR_XORL:
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
		case EXPR_IF:
		case EXPR_WHILE:
		case EXPR_SUM:
		case EXPR_INT:
		case EXPR_PROD:
		case EXPR_SUP:
		case EXPR_INF:
		case EXPR_ANDN:
		case EXPR_ORN:
		case EXPR_XORN:
		case EXPR_GCDN:
		case EXPR_LCMN:
		case EXPR_LOOP:
		case EXPR_FOR:
		case EXPR_ZA:
		case EXPR_MD:
		case EXPR_ME:
		case EXPR_VMD:
			return 1;
		default:
			return 0;
	}
}
static int expr_usesrc(enum expr_op op){
	switch(op){
		case EXPR_COPY:
		case EXPR_ADD:
		case EXPR_SUB:
		case EXPR_MUL:
		case EXPR_DIV:
		case EXPR_MOD:
		case EXPR_POW:
		case EXPR_AND:
		case EXPR_OR:
		case EXPR_XOR:
		case EXPR_SHL:
		case EXPR_SHR:
		case EXPR_GT:
		case EXPR_GE:
		case EXPR_LT:
		case EXPR_LE:
		case EXPR_SGE:
		case EXPR_SLE:
		case EXPR_SEQ:
		case EXPR_SNE:
		case EXPR_EQ:
		case EXPR_NE:
		case EXPR_ANDL:
		case EXPR_ORL:
		case EXPR_XORL:
			return 1;
		default:
			return 0;
	}
}
static int expr_usesum(enum expr_op op){
	switch(op){
		case EXPR_SUM:
		case EXPR_INT:
		case EXPR_PROD:
		case EXPR_SUP:
		case EXPR_INF:
		case EXPR_ANDN:
		case EXPR_ORN:
		case EXPR_XORN:
		case EXPR_GCDN:
		case EXPR_LCMN:
		case EXPR_FOR:
		case EXPR_LOOP:
				return 1;
		default:
				return 0;
	}
}
static int expr_usemd(enum expr_op op){
	switch(op){
		case EXPR_MD:
		case EXPR_ME:
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
		case EXPR_IF:
		case EXPR_WHILE:
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
			||(ip1->dst==v&&!ov)
		){
			return 1;
		}
		if(ip1->dst==v&&ov){
			return 0;
		}
		if(ip1->op==EXPR_END){
			return 0;
		}
	}
	return 0;
}
static int expr_vcheck_ep(struct expr_inst *ip0,double *v){
	if(expr_vused(ip0,v))return 1;
	for(struct expr_inst *ip=ip0;ip->op!=EXPR_END;++ip){
		if(expr_usesum(ip->op)&&(
			expr_vcheck_ep(ip->un.es->from->data,v)||
			expr_vcheck_ep(ip->un.es->to->data,v)||
			expr_vcheck_ep(ip->un.es->step->data,v)||
			expr_vcheck_ep(ip->un.es->ep->data,v)
			))return 1;
		if(expr_usevmd(ip->op)&&(
			expr_vcheck_ep(ip->un.ev->from->data,v)||
			expr_vcheck_ep(ip->un.ev->to->data,v)||
			expr_vcheck_ep(ip->un.ev->step->data,v)||
			expr_vcheck_ep(ip->un.ev->ep->data,v)
			))return 1;
		if(expr_usebranch(ip->op)&&(
			expr_vcheck_ep(ip->un.eb->cond->data,v)||
			expr_vcheck_ep(ip->un.eb->body->data,v)||
			expr_vcheck_ep(ip->un.eb->value->data,v)
			))return 1;
		if(expr_usemd(ip->op)){
			for(size_t i=0;i<ip->un.em->dim;++i){
				if(expr_vcheck_ep(ip->un.em->eps[i].data,v))
					return 1;
			}
		}
		if(ip->op==EXPR_HOT&&(
			expr_vcheck_ep(ip->un.hotfunc->data,v)
			))return 1;
	}
	return 0;
}

static void expr_optimize_unused(struct expr *restrict ep){
	for(struct expr_inst *ip=ep->data;ip->op!=EXPR_END;++ip){

		if(!expr_varofep(ep,ip->dst)
			||expr_side(ip->op))continue;
		if(/*!expr_used(ip)&&*/!expr_vcheck_ep(ip+1,ip->dst)){
			ip->dst=NULL;
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
			if(ip1->dst!=ip->dst)continue;
			if(ip1->op!=EXPR_CONST)break;
			switch(ip->op){
				case EXPR_NEG:
					ip1->un.value=-ip1->un.value;
					break;
				case EXPR_NOT:
					ip1->un.value=expr_not(ip1->un.value);
					break;
				case EXPR_NOTL:
					ip1->un.value=fabs(ip1->un.value)<=DBL_EPSILON?
						1.0:0.0;
					break;
				case EXPR_TSTL:
					ip1->un.value=fabs(ip1->un.value)>DBL_EPSILON?
						1.0:0.0;
					break;
				default:
					continue;
			}
			expr_writeconsts(ep);
			ip->dst=NULL;
			break;
		}
	}
	expr_optimize_completed(ep);
}

static int expr_injection_optype(enum expr_op op){
	switch(op){
		case EXPR_CALL:
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
			//printf("in %zd\n",ip-ep->data);
			if(ip->op==EXPR_ZA){
				ip->un.value=ip->un.zafunc();
			//printf("in %lf\n",ip->un.value);
				ip->op=EXPR_CONST;
				expr_writeconsts(ep);
				r=1;
				continue;
			}
		for(struct expr_inst *ip1=ip-1;ip1>=ep->data;--ip1){
			//printf("at [%zd] in %zd\n",ip1-ep->data,ip-ep->data);
			if(ip1->dst!=ip->dst)continue;
			if(ip1->op!=EXPR_CONST)break;
			switch(ip->op){
				case EXPR_CALL:
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
			r=1;
			ip->dst=NULL;
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
			!expr_varofep(ep,ip->dst)
			||expr_vcheck_ep(ip+1,ip->dst))continue;
		ip2=NULL;
		for(struct expr_inst *ip1=ip+1;ip1->op!=EXPR_END;++ip1){
			if(ip1->dst==ip->dst)goto fail;
			if(!expr_usesrc(ip1->op)||ip1->un.src!=ip->dst)continue;
			if(ip2){
fail:
				ip2=NULL;
				break;
			}else
				ip2=ip1;
		}
		if(ip2){
			ip2->un.src=ip->un.src;
			ip->dst=NULL;
		}
	}
	expr_optimize_completed(ep);
}
/*static int expr_optimize_copy2const(struct expr *restrict ep){
	int r=0;
	for(struct expr_inst *ip=ep->data;ip->op!=EXPR_END;++ip){
		if(ip->op!=EXPR_COPY
			//||
			//!expr_varofep(ep,ip->dst)||
			//!expr_varofep(ep,ip->un.src)
			)continue;
		if(!expr_modified(ep,ip->un.src)){
			ip->op=EXPR_CONST;
			ip->un.value=*ip->un.src;
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
	if(lip&&lip->op==EXPR_COPY&&lip->dst==ip->dst&&
		expr_varofep(ep,lip->dst)
		){
		ip->dst=lip->un.src;
		lip->dst=NULL;
	}
	expr_optimize_completed(ep);
}
static void expr_optimize_strongorder_and_notl(struct expr *restrict ep){
	for(struct expr_inst *ip=ep->data;ip->op!=EXPR_END;++ip){
		if(ip[1].dst!=ip->dst)continue;
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
		(++ip)->dst=NULL;
	}
	expr_optimize_completed(ep);
}
static void expr_optimize_contneg(struct expr *restrict ep){
	for(struct expr_inst *ip=ep->data;ip->op!=EXPR_END;++ip){
		if(ip[1].dst!=ip->dst)continue;
		switch(ip->op){
			case EXPR_NOT:
			case EXPR_NEG:
				if(ip[1].op==ip->op){
					ip->dst=NULL;
					(++ip)->dst=NULL;
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
						(ip++)->dst=NULL;
						break;
					default:
						break;
				}
				break;
			case EXPR_NOTL:
				switch(ip[1].op){
					case EXPR_NOT:
from_tstl_to_not:
						if(ip[2].dst==ip->dst)
						switch(ip[2].op){
							case EXPR_NOTL:
								ip->un.value=0;
								goto notl_tstl;
							case EXPR_TSTL:
								ip->un.value=1;
notl_tstl:
								ip->op=EXPR_CONST;
								(++ip)->dst=NULL;
								(++ip)->dst=NULL;
								expr_writeconsts(ep);
							default:
								break;
						}
						continue;
					case EXPR_NEG:

from_tstl_to_neg:
						if(ip[2].dst==ip->dst)
						switch(ip[2].op){
							case EXPR_NOTL:
							case EXPR_TSTL:
								(++ip)->dst=NULL;
							default:
								break;
						}
						continue;
					case EXPR_NOTL:
						ip->op=EXPR_TSTL;
					case EXPR_TSTL:
						(++ip)->dst=NULL;
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

	r|=expr_optimize_zero(ep);
	//r|=expr_optimize_copy2const(ep);
	expr_optimize_copyadd(ep);
	expr_optimize_unused(ep);
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
	step=fabs(expr_eval(ev->step,input));
	from=expr_eval(ev->from,input);
	to=expr_eval(ev->to,input);
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
double expr_eval(const struct expr *restrict ep,double input){
	double step,sum,from,to,y;
	int neg;
	for(struct expr_inst *ip=ep->data;;++ip){
		assert(ip->op>=EXPR_COPY);
		assert(ip->op<=EXPR_END);
		switch(ip->op){
			case EXPR_COPY:
				*ip->dst=*ip->un.src;
				break;
			case EXPR_INPUT:
				*ip->dst=input;
				break;
			case EXPR_CALL:
				*ip->dst=ip->un.func(*ip->dst);
				break;
			case EXPR_CONST:
				*ip->dst=ip->un.value;
				break;
			case EXPR_ADD:
				*ip->dst+=*ip->un.src;
				break;
			case EXPR_SUB:
				*ip->dst-=*ip->un.src;
				break;
			case EXPR_MUL:
				*ip->dst*=*ip->un.src;
				break;
			case EXPR_DIV:
				*ip->dst/=*ip->un.src;
				break;
			case EXPR_MOD:
				*ip->dst=fmod(*ip->dst,*ip->un.src);
				break;
			case EXPR_POW:
				*ip->dst=pow(*ip->dst,*ip->un.src);
				break;
			case EXPR_AND:
				*ip->dst=expr_and2(*ip->dst,*ip->un.src);
				break;
			case EXPR_OR:
				*ip->dst=expr_or2(*ip->dst,*ip->un.src);
				break;
			case EXPR_XOR:
				*ip->dst=expr_xor2(*ip->dst,*ip->un.src);
				break;
			case EXPR_SHL:
				EXPR_EDEXP(ip->dst)+=(int64_t)*ip->un.src;
				break;
			case EXPR_SHR:
				EXPR_EDEXP(ip->dst)-=(int64_t)*ip->un.src;
				break;
			case EXPR_NEG:
				*ip->dst=-*ip->dst;
				break;
			case EXPR_NOT:
				*ip->dst=expr_not(*ip->dst);
				break;
			case EXPR_NOTL:
				*ip->dst=fabs(*ip->dst)<=DBL_EPSILON?
					1.0:
					0.0;
				break;
			case EXPR_TSTL:
				*ip->dst=fabs(*ip->dst)>DBL_EPSILON?
					1.0:
					0.0;
				break;
			case EXPR_GT:
				*ip->dst=*ip->dst>*ip->un.src?
					1.0:
					0.0;
				break;
			case EXPR_LT:
				*ip->dst=*ip->dst<*ip->un.src?
					1.0:
					0.0;
				break;
			case EXPR_SGE:
				*ip->dst=*ip->dst>=*ip->un.src?
					1.0:
					0.0;
				break;
			case EXPR_SLE:
				*ip->dst=*ip->dst<=*ip->un.src?
					1.0:
					0.0;
				break;
			case EXPR_GE:
				*ip->dst=*ip->dst>=*ip->un.src-DBL_EPSILON?
					1.0:
					0.0;
				break;
			case EXPR_LE:
				*ip->dst=*ip->dst<=*ip->un.src+DBL_EPSILON?
					1.0:
					0.0;
				break;

			case EXPR_SEQ:
				*ip->dst=*ip->dst==*ip->un.src?
					1.0:
					0.0;
				break;
			case EXPR_SNE:
				*ip->dst=*ip->dst!=*ip->un.src?
					1.0:
					0.0;
				break;
/*			case EXPR_EQ:
				*ip->dst=fabs(*ip->dst-*ip->un.src)<=DBL_EPSILON?
					1.0:
					0.0;
				break;
			case EXPR_NE:
				*ip->dst=fabs(*ip->dst-*ip->un.src)>DBL_EPSILON?
					1.0:
					0.0;
				break;*/
			case EXPR_EQ:
				*ip->dst=expr_equal(*ip->dst,*ip->un.src)?
					1.0:
					0.0;
				break;
			case EXPR_NE:
				*ip->dst=!expr_equal(*ip->dst,*ip->un.src)?
					1.0:
					0.0;
				break;
//#define CALLOGIC(a,b,_s) ((fabs(a)>DBL_EPSILON) _s (fabs(b)>DBL_EPSILON))
			case EXPR_ANDL:
				*ip->dst=CALLOGIC(*ip->dst,*ip->un.src,&&)?
					1.0:
					0.0;
				break;
			case EXPR_ORL:
				*ip->dst=CALLOGIC(*ip->dst,*ip->un.src,||)?
					1.0:
					0.0;
				break;
			case EXPR_XORL:
				*ip->dst=CALLOGIC(*ip->dst,*ip->un.src,^)?
					1.0:
					0.0;
				break;
				
#define CALSUM(_op,_do,_init,_neg)\
			case _op :\
				neg=0;\
				from=expr_eval(ip->un.es->from,input);\
				to=expr_eval(ip->un.es->to,input);\
				if(from>to){\
					step=from;\
					from=to;\
					to=step;\
					neg=1;\
				}\
				step=expr_eval(ip->un.es->step,input);\
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
				*ip->dst=sum;\
				break
			CALSUM(EXPR_SUM,sum+=y,sum=0.0,-sum);
			CALSUM(EXPR_INT,sum+=step*y,sum=0.0;from+=step/2.0,-sum);
			CALSUM(EXPR_PROD,sum*=y,sum=1.0,1.0/sum);
			CALSUM(EXPR_SUP,if(y>sum)sum=y,sum=DBL_MIN,sum);
			CALSUM(EXPR_INF,if(y<sum)sum=y,sum=DBL_MAX,sum);
			CALSUM(EXPR_ANDN,sum=sum!=DBL_MAX?expr_and2(sum,y):y,sum=DBL_MAX,sum);
			CALSUM(EXPR_ORN,sum=sum!=0.0?expr_or2(sum,y):y,sum=0.0,sum);
			CALSUM(EXPR_XORN,sum=sum!=0.0?expr_xor2(sum,y):y,sum=0.0,sum);
			CALSUM(EXPR_GCDN,sum=sum!=DBL_MAX?expr_gcd2(sum,y):y,sum=DBL_MAX,sum);
			CALSUM(EXPR_LCMN,sum=sum!=1.0?expr_lcm2(sum,y):y,sum=1.0,sum);

			case EXPR_FOR:
				ip->un.es->index=
				expr_eval(ip->un.es->from,input);//init
				to=expr_eval(ip->un.es->to,input);//cond
				if(to<0.0)to=-to;
				while(to>DBL_EPSILON){
					expr_eval(ip->un.es->step,input);//every time
					to=expr_eval(ip->un.es->to,input);//cond
					if(to<0.0)to=-to;
				}
				*ip->dst=expr_eval(ip->un.es->ep,input);
				break;
			case EXPR_LOOP:
				ip->un.es->index=
				expr_eval(ip->un.es->from,input);//init
				to=expr_eval(ip->un.es->to,input);//times
				if(to<0)to=-to;
				for(;to>DBL_EPSILON;to-=1.0){
					expr_eval(ip->un.es->step,input);//every time
				}
				*ip->dst=expr_eval(ip->un.es->ep,input);
				break;
			case EXPR_IF:
				*ip->dst=
				fabs(expr_eval(ip->un.eb->cond,input))>DBL_EPSILON?
				expr_eval(ip->un.eb->body,input):
				expr_eval(ip->un.eb->value,input);
				break;
			case EXPR_WHILE:
				while(fabs(expr_eval(ip->un.eb->cond,input))>DBL_EPSILON)
				expr_eval(ip->un.eb->body,input);
				*ip->dst=
				expr_eval(ip->un.eb->value,input);
				break;
			case EXPR_ZA:
				*ip->dst=ip->un.zafunc();
				break;
			case EXPR_MD:
				for(size_t i=0;i<ip->un.em->dim;++i)
					ip->un.em->args[i]=
						expr_eval(
						ip->un.em->eps+i,input);
				
				*ip->dst=ip->un.em->un.func(ip->un.em->dim,ip->un.em->args);
				break;
			case EXPR_ME:
				*ip->dst=ip->un.em->un.funcep(ip->un.em->dim,ip->un.em->eps,input);
				break;
			case EXPR_VMD:
				*ip->dst=expr_vmdeval(ip->un.ev,input);
				break;
			case EXPR_HOT:
				*ip->dst=expr_eval(ip->un.hotfunc,*ip->dst);
				break;
			case EXPR_END:
				return *ip->dst;
		}
	}
}
