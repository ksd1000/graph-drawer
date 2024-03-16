/*******************************************************************************
 *License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>*
 *This is free software: you are free to change and redistribute it.           *
 *******************************************************************************/
#define _GNU_SOURCE
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include <time.h>
#include <float.h>
#include "expr.h"
#define printval(x) fprintf(stderr,#x ":%lu\n",x)
#define printvall(x) fprintf(stderr,#x ":%ld\n",x)
#define printvald(x) fprintf(stderr,#x ":%lf\n",x)

//#define free(v)
static const char *eerror[]={"Unknown error","Unknown symbol","Parentheses do not match","Function and keyword must be followed by a \'(\'","No value in parenthesis","No enough or too much argument","Bad number","Target is not variable","Empty value"};
const char *expr_error(int error){
	if(error<0)error=-error;
	if(error>=(sizeof(eerror)/sizeof(*eerror)))return eerror[0];
	else return eerror[error];
}
static const char spaces[]={" \t\r\f\n\v"};
static const char special[]={"+-*/%^(),<>=!&|"};
uint64_t expr_gcd64(uint64_t x,uint64_t y){
	uint64_t r,r1;
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

void expr_memrand(void *restrict m,size_t n){
#if (RAND_MAX>=UINT32_MAX)
	while(n>=4){
		*(uint32_t *)m=(uint32_t)rand();
		*(char *)&m+=4;
		n-=4;
	}
#endif
#if (RAND_MAX>=UINT16_MAX)
	while(n>=2){
		*(uint16_t *)m=(uint16_t)rand();
		*(char *)&m+=2;
		n-=2;
	}
#endif
	while(n>0){
		*(uint8_t *)m=(uint8_t)rand();
		*(char *)&m+=1;
		--n;
	}

}
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
	if(!x1)goto zero;\
	x2=63UL-__builtin_clzl(x1);\
	x1&=~(1UL<<x2);\
	x2=52UL-x2;\
	if(EXPR_EDEXP(&a)<x2)goto zero;\
	EXPR_EDBASE(&a)=x1<<x2;\
	EXPR_EDEXP(&a)-=x2;\
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
double expr_gcd2(double x,double y){
	return (double)expr_gcd64((uint64_t)(fabs(x)+DBL_EPSILON),
		(uint64_t)(fabs(y)+DBL_EPSILON));
}
double expr_lcm2(double x,double y){
	uint64_t a=(uint64_t)(fabs(x)+DBL_EPSILON),b=(uint64_t)(fabs(y)+DBL_EPSILON);
	return (double)(a*b)/expr_gcd64(a,b);
}
//double expr_rand12(void){
	//union expr_double ret;
//	if(!ddp){
//		srand48_r(time(NULL),ddp=&dd);
//	}
	//ret.ival=lrand48();
	//ret.rd.exp=1023;
	//ret.rd.sign=1;
	//return ret.val;
//	return drand48()+1.0;
//}
static double expr_rand(size_t n,double *args){
	//assert(n==2);
	return args[0]+(args[1]-args[0])*drand48();
}/*
static double expr_if(size_t n,double *args){
	//assert(n==3);
	return fabs(args[0])>DBL_EPSILON?args[1]:args[2];
}
static double expr_ifnot(size_t n,double *args){
	//assert(n==3);
	return fabs(args[0])>DBL_EPSILON?args[2]:args[1];
}*/
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
static double expr_sign(double x){
	if(x>DBL_EPSILON)return 1.0;
	else if(x<-DBL_EPSILON)return -1.0;
	else return 0.0;
}
static double expr_not(double x){
	if(fabs(x)>DBL_EPSILON)return 0.0;
	else return 1.0;
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
	if(level<1)
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
#define REGFSYM(s) {.str=#s,.un={.func=s},.type=EXPR_FUNCTION,.dim=0}
#define REGCSYM(s) {.str=#s,.un={.value=s},.type=EXPR_CONSTANT,.dim=0}
#define REGFSYM2(s,sym) {.str=s,.un={.func=sym},.type=EXPR_FUNCTION,.dim=0}
#define REGMDSYM2(s,sym,d) {.str=s,.un={.mdfunc=sym},.type=EXPR_MDFUNCTION,.dim=d}
#define REGMDEPSYM2(s,sym,d) {.str=s,.un={.mdepfunc=sym},.type=EXPR_MDEPFUNCTION,.dim=d}
#define REGCSYM2(s,val) {.str=s,.un={.value=val},.type=EXPR_CONSTANT,.dim=0}
//#define REGSYMMD(s,n) {#s,s,n}
const struct expr_builtin_keyword expr_keywords[]={
	{"sum",EXPR_SUM,5,"index_name,start_index,end_index,index_step,addend"},
	{"int",EXPR_INT,5,"integral_var_name,upper_limit,lower_limit,epsilon,integrand"},
	{"prod",EXPR_PROD,5,"index_name,start_index,end_index,index_step,factor"},
	{"pai",EXPR_PROD,5,"index_name,start_index,end_index,index_step,factor"},
	{"sup",EXPR_SUP,5,"index_name,start_index,end_index,index_step,element"},
	{"infi",EXPR_INF,5,"index_name,start_index,end_index,index_step,element"},
	{"AND",EXPR_AND,5,"index_name,start_index,end_index,index_step,element"},
	{"OR",EXPR_OR,5,"index_name,start_index,end_index,index_step,element"},
	{"XOR",EXPR_XOR,5,"index_name,start_index,end_index,index_step,element"},
	{"GCD",EXPR_GCD,5,"index_name,start_index,end_index,index_step,element"},
	{"LCM",EXPR_LCM,5,"index_name,start_index,end_index,index_step,element"},
	{"for",EXPR_FOR,5,"var_name,start_var,cond,body,value"},
	{"loop",EXPR_LOOP,5,"var_name,start_var,count,body,value"},
	{"while",EXPR_WHILE,3,"cond,body,value"},
	{"if",EXPR_IF,3,"cond,if_value,else_value"},
	{NULL}
};
const struct expr_builtin_symbol expr_bsyms[]={
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
	REGFSYM2("not",expr_not),
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

	REGMDSYM2("and",expr_and,0),
	REGMDSYM2("or",expr_or,0),
	REGMDSYM2("xor",expr_xor,0),
	REGMDSYM2("gcd",expr_gcd,0),
	REGMDSYM2("hypot",expr_hypot,0),
	REGMDSYM2("lcm",expr_lcm,0),
	REGMDSYM2("min",expr_min,0),
	REGMDSYM2("max",expr_max,0),
	REGMDSYM2("nfact",expr_nfact,2),
	REGMDSYM2("rand",expr_rand,2),

	REGMDEPSYM2("piece",expr_piece,0),
	REGMDEPSYM2("d",expr_derivate,0),
	REGMDEPSYM2("dn",expr_multi_derivate,0),
	{.str=NULL}
};/*
const struct {
	const char *str;
	double (*addr)(double);
} expr_fsyms[]={
	{"fact",expr_fact},
	{"dfact",expr_dfact},
	REGSYM(acos),
	REGSYM(acosh),
	REGSYM(asin),
	REGSYM(asinh),
	REGSYM(atan),
	REGSYM(atanh),
	REGSYM(cbrt),
	REGSYM(ceil),
	REGSYM(cos),
	REGSYM(cosh),
	REGSYM(erf),
	REGSYM(exp),
	REGSYM(exp2),
	REGSYM(expm1),
	REGSYM(fabs),
	REGSYM(floor),
	REGSYM(j0),
	REGSYM(j1),
	REGSYM(lgamma),
	REGSYM(log),
	{"ln",log},
	REGSYM(log10),
	REGSYM(log1p),
	REGSYM(log2),
	REGSYM(logb),
	REGSYM(nearbyint),
	{"nnot",expr_nnot},
	{"not",expr_not},
	REGSYM(rint),
	REGSYM(round),
	{"sign",expr_sign},
	REGSYM(sin),
	REGSYM(sinh),
	REGSYM(sqrt),
	REGSYM(tan),
	REGSYM(tanh),
	REGSYM(tgamma),
	REGSYM(trunc),
	REGSYM(y0),
	REGSYM(y1),
	{NULL,NULL}
};
const struct {
	const char *str;
	double val;
} expr_csyms[]={
	REGSYM(DBL_MAX),
	REGSYM(DBL_MIN),
	REGSYM(DBL_EPSILON),
	REGSYM(HUGE_VAL),
	REGSYM(FLT_MAX),
	REGSYM(FLT_MIN),
	REGSYM(FLT_EPSILON),
	REGSYM(HUGE_VALF),
	REGSYM(INFINITY),
	REGSYM(NAN),
	{"e",M_E},
	{"log2e",M_LOG2E},
	{"log10e",M_LOG10E},
	{"ln2",M_LN2},
	{"ln10",M_LN10},
	{"pi",M_PI},
	{"pi_2",M_PI_2},
	{"pi_4",M_PI_4},
	{"1_pi",M_1_PI},
	{"2_pi",M_2_PI},
	{"2_sqrtpi",M_2_SQRTPI},
	{"sqrt2",M_SQRT2},
	{"sqrt1_2",M_SQRT1_2},
	{NULL,0.0}
};
const struct {
	const char *str;
	double (*addr)(size_t,double *);
	unsigned int dim,unused;
} expr_mdsyms[]={
	{"and",expr_and,0},
	{"or",expr_or,0},
	{"xor",expr_xor,0},
	{"gcd",expr_gcd,0},
	{"hypot",expr_hypot,0},
	{"lcm",expr_lcm,0},
	{"min",expr_min,0},
	{"max",expr_max,0},
	{"rand",expr_rand,2},
	{NULL,NULL}
};
const struct {
	const char *str;
	double (*addr)(size_t,const struct expr *,double);
	unsigned int dim,unused;
} expr_mdepsyms[]={
	{"piece",expr_piece,0},
	{"d",expr_derivate,0},
	{"dn",expr_multi_derivate,0},
	{NULL,NULL}
};*/
static const struct expr_builtin_symbol *expr_bsym_search(const char *sym,size_t sz){
	const struct expr_builtin_symbol *p;
	//if(ep->sset&&(un.ret=expr_symset_search(ep->sset,sym,sz,type,dim)))
	/*for(i=0;i<ep->sset->size;++i){
	//for(i=0;i<1;++i){
		//printf("%p %zu found %s %p at%p\n",ep->sset,i,ep->sset->syms[i].str,ep->sset->syms[i].str,ep->sset->syms[i].addr);
		//if(!*ep->sset->syms[i].str)strcpy(ep->sset->syms[i].str,"n1");
		if(sz==strlen(ep->sset->syms[i].str)&&!memcmp(ep->sset->syms[i].str,sym,sz)){
			if(type)*type=ep->sset->syms[i].type;
			if(dim)*dim=ep->sset->syms[i].dim;
			return ep->sset->syms[i].addr;
		}
	}*/
		//return un.ret;
	/*for(i=0;expr_fsyms[i].str;++i){
		if(sz==strlen(expr_fsyms[i].str)&&!memcmp(expr_fsyms[i].str,sym,sz)){
			if(type)*type=EXPR_FUNCTION;
			return expr_fsyms[i].addr;
		}
	}
	for(i=0;expr_csyms[i].str;++i){
		if(sz==strlen(expr_csyms[i].str)&&!memcmp(expr_csyms[i].str,sym,sz)){
			if(type)*type=EXPR_CONSTANT;
			return (void *)&expr_csyms[i].val;
		}
	}
	for(i=0;expr_mdsyms[i].str;++i){
		if(sz==strlen(expr_mdsyms[i].str)&&!memcmp(expr_mdsyms[i].str,sym,sz)){
			if(type)*type=EXPR_MDFUNCTION;
			if(dim)*dim=expr_mdsyms[i].dim;
			return expr_mdsyms[i].addr;
		}
	}
	for(i=0;expr_mdepsyms[i].str;++i){
		if(sz==strlen(expr_mdepsyms[i].str)&&!memcmp(expr_mdepsyms[i].str,sym,sz)){
			if(type)*type=EXPR_MDEPFUNCTION;
			if(dim)*dim=expr_mdepsyms[i].dim;
			return expr_mdepsyms[i].addr;
		}
	}*/
	//printf("%zu %s not found\n",sz,sym);
	for(p=expr_bsyms;p->str;++p){
		if(sz==strlen(p->str)&&!memcmp(p->str,sym,sz)){
			return p;
		}
	}
	return NULL;
}
#define LISTSYM(esp) if(esp)\
	for(struct expr_symbol *p=ep->sset->syms;p;p=p->next){\
		printf("listsym %s %p at %p\n",p->str,\
		p->addr,p->str);\
	}\
	puts("");
static void *xmalloc(size_t size){
	void *r=malloc(size);
	assert(r != NULL);
	return r;
}
static void *xrealloc(void *old,size_t size){
	//void *r=malloc(size);
	void *r=realloc(old,size);
	//printf("realloc(%p,%zu)=%p\n",old,size,r);
	/*void *r=malloc(size);
	if(old){
		memcpy(r,old,size-16);
		free(old);
	}*/
	assert(r != NULL);
	return r;
}
void expr_free(struct expr *restrict ep){
	struct expr_inst *ip;
	if(!ep)return;
	if(ep->data){
		ip=ep->data;
		for(size_t i=ep->size;i>0;--i){
			switch(ip->op){
				case EXPR_SUM:
				case EXPR_INT:
				case EXPR_PROD:
				case EXPR_SUP:
				case EXPR_INF:
				case EXPR_AND:
				case EXPR_OR:
				case EXPR_XOR:
				case EXPR_GCD:
				case EXPR_LCM:
				case EXPR_FOR:
				case EXPR_LOOP:
					//expr_symset_free(ip->un.es->ep->sset);
					expr_free(ip->un.es->ep);
					expr_free(ip->un.es->from);
					expr_free(ip->un.es->to);
					expr_free(ip->un.es->step);
					free(ip->un.es);
					break;
				case EXPR_CALLMD:
				case EXPR_CALLMDEP:
					free(ip->un.em->args);
					for(unsigned int i=0;i<ip->un.em->dim;++i)
						expr_free(ip->un.em->eps+i);
					free(ip->un.em->eps);
					free(ip->un.em);
					break;
				case EXPR_IF:
				case EXPR_WHILE:
					free(ip->un.eb->cond);
					free(ip->un.eb->body);
					free(ip->un.eb->value);
					free(ip->un.eb);
					break;
				case EXPR_CALLHOT:
					expr_free(ip->un.hotfunc);
					break;
				default:
					break;
			}
			++ip;
		}
		free(ep->data);
	}
	if(ep->vars)free(ep->vars);
	if(ep->freeable)free(ep);
}
#define EXTEND_DATA if(ep->size>=ep->length){\
	ep->data=xrealloc(ep->data,\
		(ep->length+=16)*sizeof(struct expr_inst));\
	}
/*
__attribute__((noinline))
//void expr_addcall(struct expr *restrict ep,double *dst,double (*func)(double)){
	struct expr_inst *ip;
	EXTEND_DATA
	ip=ep->data+ep->size++;
	ip->op=EXPR_CALL;
	ip->dst=dst;
	ip->un.func=func;
}
__attribute__((noinline))
//void expr_addcallmdop(struct expr *restrict ep,double *dst,struct expr_mdinfo *em,unsigned int op){
	struct expr_inst *ip;
	EXTEND_DATA
	ip=ep->data+ep->size++;
	ip->op=op;
	ip->dst=dst;
	ip->un.em=em;
}
__attribute__((noinline))
//void expr_addsumop(struct expr *restrict ep,double *dst,struct expr_suminfo *es,unsigned int op){
	struct expr_inst *ip;
	EXTEND_DATA
	ip=ep->data+ep->size++;
	ip->op=op;
	ip->dst=dst;
	ip->un.es=es;
}
__attribute__((noinline))
//void expr_addconst(struct expr *restrict ep,double *dst,double *val){
	struct expr_inst *ip;
	EXTEND_DATA
	ip=ep->data+ep->size++;
	ip->op=EXPR_CONST;
	ip->dst=dst;
	ip->un.value=*val;
	ip->assign_level=0;
}*/
__attribute__((noinline))
struct expr_inst *expr_addop(struct expr *restrict ep,double *dst,void *src,unsigned int op){
	struct expr_inst *ip;
	EXTEND_DATA
	ip=ep->data+ep->size++;
	ip->op=op;
	ip->dst=dst;
	ip->un.src=(double *)src;
	//printvald(*(double *)&src);
	ip->assign_level=0;
	return ip;
}
/*
//static void expr_addassign(struct expr *restrict ep,double *dst){
	struct expr_inst *ip;
	EXTEND_DATA
	ip=ep->data+ep->size++;
	ip->op=EXPR_COPY;
	ip->assign_level=1;
	ip->dst=dst;
}*/
double *expr_newvar(struct expr *restrict ep){
	if(ep->vsize>=ep->vlength){
		//LISTSYM(ep->sset)
		ep->vars=xrealloc(ep->vars,
			(ep->vlength+=16)*sizeof(double));
		//LISTSYM(ep->sset)
	}
	return ep->vars+ep->vsize++;
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
static const char *expr_getsym(const char *c){
	while(*c&&!strchr(special,*c))
		++c;
	return c;
}
static int expr_atod2(const char *str,double *dst){
	int ret;
	ret=sscanf(str,"%lf",dst);
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
			//puts(*saveptr);
			if(!*saveptr)return NULL;
		}
		++(*saveptr);
	}
	return str;
}
static char **expr_sep(struct expr *restrict ep,char *e){
	char *p,*p1,*p2,**p3=NULL;
	size_t len=0,s;
	if(*e=='('){
		p1=(char *)expr_findpair(e);
		if(p1){
			if(!p1[1]){
				*p1=0;
				++e;
			}
		}else {
			ep->error=EXPR_EPT;
			return NULL;
		}
	}
	//puts(e);
	if(!*e){
		ep->error=EXPR_ENVP;
		return NULL;
	}
	for(p=expr_tok(e,&p2);p;p=expr_tok(NULL,&p2)){
		p1=xmalloc((s=strlen(p))+1);
		p1[s]=0;
		memcpy(p1,p,s);
		//puts(p1);
		p3=xrealloc(p3,(++len+1)*sizeof(char *));
		p3[len-1]=p1;
	}
	p3[len]=NULL;
	return p3;
}
static void expr_free2(char **buf){
	char **p=buf;
	while(*p){
		free(*p);
		++p;
	}
	free(buf);
}
static struct expr_mdinfo *expr_getmdinfo(struct expr *restrict ep,char *e,const char *asym,void *func,unsigned int dim){
	char **v=expr_sep(ep,e);
	char **p;
	size_t i;
	struct expr_mdinfo *em;
	if(!v){
		return NULL;
	}
	p=v;
	while(*p){
		//puts(*p);
		++p;
	}
	i=p-v;
	if(dim&&i!=dim){
		ep->error=EXPR_ENEA;
		goto err1;
	}
	em=xmalloc(sizeof(struct expr_mdinfo));
	em->dim=i;

	em->un.func=func;
	em->eps=xmalloc(em->dim*sizeof(struct expr));
	em->args=xmalloc(em->dim*sizeof(double));
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
	free(em->args);
	ep->error=em->eps[i].error;
	memcpy(ep->errinfo,em->eps[i].errinfo,EXPR_SYMLEN);
	free(em->eps);
	free(em);
err1:
	expr_free2(v);
	return NULL;
}
static struct expr_suminfo *expr_getsuminfo(struct expr *restrict ep,char *e,const char *asym){
	char **v=expr_sep(ep,e);
	char **p;
	struct expr_suminfo *es;
	struct expr_symset *sset;
	int error;
	char ef[EXPR_SYMLEN];
	if(!v){
		return NULL;
	}
	p=v;
	while(*p){
		//puts(*p);
		++p;
	}
	if(p-v!=5){
		ep->error=EXPR_ENEA;
		goto err0;
	}
	es=xmalloc(sizeof(struct expr_suminfo));
//	sum(sym_index,from,to,step,expression)
	//sset=new_expr_symset();
	//puts("bef addv0");
	//expr_symset_add(sset,"draw",&es->index,EXPR_VARIABLE);
	sset=new_expr_symset();
	expr_symset_add(sset,v[0],EXPR_VARIABLE,&es->index);
	expr_symset_copy(sset,ep->sset);
	//printf("es->index %p\n",&es->index);
	es->from=new_expr(v[1],asym,ep->sset,&error,ef);
	if(!es->from)goto err1;
	es->to=new_expr(v[2],asym,sset,&error,ef);
	if(!es->to)goto err2;
	es->step=new_expr(v[3],asym,sset,&error,ef);
	if(!es->step)goto err3;
	//sset=expr_symset_clone(ep->sset);

	es->ep=new_expr(v[4],asym,sset,&error,ef);
	//printf("sset %p 1:%s 2:%s\n",es->ep->sset,es->ep->sset->syms[0].str,es->ep->sset->syms[1].str);
	if(!es->ep)goto err4;
	//printf("sset %p 1:%s 2:%s\n",sset,sset->syms[0].str,sset->syms[1].str);
	expr_free2(v);
	//assert(es->ep);
	//puts("getsi");
	expr_symset_free(sset);
	return es;
err4:
	//puts(v[0]);
	//printf("%s,%s\n",sset->syms[0].str,sset->syms[1].str);
	//assert(0);
	expr_free(es->step);
err3:
	expr_free(es->to);
err2:
	expr_free(es->from);
err1:
	free(es);
	expr_symset_free(sset);
	ep->error=error;
	memcpy(ep->errinfo,ef,EXPR_SYMLEN);
err0:
	expr_free2(v);
	return NULL;
}
static struct expr_branchinfo *expr_getbranchinfo(struct expr *restrict ep,char *e,const char *asym){
	char **v=expr_sep(ep,e);
	char **p;
	struct expr_branchinfo *eb;
	int error;
	char ef[EXPR_SYMLEN];
	//assert(v);
	if(!v){
		return NULL;
	}
	p=v;
	while(*p){
		//puts(*p);
		++p;
	}
	//assert(p-v==3);
	if(p-v!=3){
		ep->error=EXPR_ENEA;
		goto err0;
	}
	eb=xmalloc(sizeof(struct expr_branchinfo));
//	while(cond,body,value)
	eb->cond=new_expr(v[0],asym,ep->sset,&error,ef);
	//assert(eb->cond);
	if(!eb->cond)goto err1;
	eb->body=new_expr(v[1],asym,ep->sset,&error,ef);
	//assert(eb->body);
	if(!eb->body)goto err2;
	eb->value=new_expr(v[2],asym,ep->sset,&error,ef);
	//assert(eb->value);
	if(!eb->value)goto err3;
	expr_free2(v);
	//assert(0);
	return eb;
err3:
	expr_free(eb->body);
err2:
	expr_free(eb->cond);
err1:
	free(eb);
	ep->error=error;
	memcpy(ep->errinfo,ef,EXPR_SYMLEN);
err0:
	expr_free2(v);
	return NULL;
}
static double *expr_scan(struct expr *restrict ep,const char *e,const char *asym);
static double *expr_getval(struct expr *restrict ep,const char *e,const char **_p,const char *asym,unsigned int assign_level){
	const char *p,*p2;//*e0=e
	char *buf;
	double *v0=NULL;
	union {
		double v;
		struct expr *ep;
		struct expr_suminfo *es;
		struct expr_branchinfo *eb;
		struct expr_mdinfo *em;
	} un;
	union {
		const struct expr_symbol *es;
		const struct expr_builtin_symbol *ebs;
	} sym;
	const union expr_symbol_value *sv;
	int type;
	unsigned int dim;

	//fprintf(stderr,"getval %u: %s\n",assign_level,e0);
	for(;;++e){
		if(!*e){
			ep->error=EXPR_EEV;
			return NULL;
		}
		if(*e=='('){
			p=expr_findpair(e);
			if(!p){
pterr:
				ep->error=EXPR_EPT;
				//assert(0);
				return NULL;
			}
			p2=e;
			if(*(++p2)==')'){
				ep->error=EXPR_ENVP;
				//assert(0);
				return NULL;
			}
			buf=xmalloc(p-e);
			buf[p-e-1]=0;
			memcpy(buf,e+1,p-e-1);
			v0=expr_scan(ep,buf,asym);
			free(buf);
			//assert(v0);
			if(!v0)return NULL;
			e=p+1;
			break;
		}else if(*e=='+')continue;
		//else if(*e==')'&&!expr_unfindpair(e0,e))goto pterr;
		p=expr_getsym(e);
	//	fprintf(stderr,"unknown sym %ld %s\n",p-e,e);
		if(p==e)goto symerr;
		type=-1;
/*
#define KEYWORD_SET(key,val) if(p-e==sizeof( key )-1&&!memcmp(e,( key ),p-e))type=( val )
		KEYWORD_SET("sum",EXPR_SUM);
		else KEYWORD_SET("int",EXPR_INT);
		else KEYWORD_SET("pai",EXPR_PROD);
		else KEYWORD_SET("prod",EXPR_PROD);
		else KEYWORD_SET("sup",EXPR_SUP);
		else KEYWORD_SET("infi",EXPR_INF);
		else KEYWORD_SET("AND",EXPR_AND);
		else KEYWORD_SET("OR",EXPR_OR);
		else KEYWORD_SET("XOR",EXPR_XOR);
		else KEYWORD_SET("GCD",EXPR_GCD);
		else KEYWORD_SET("LCM",EXPR_LCM);
		else KEYWORD_SET("for",EXPR_FOR);
		else KEYWORD_SET("loop",EXPR_LOOP);
		else KEYWORD_SET("while",EXPR_WHILE);
		else KEYWORD_SET("if",EXPR_IF);*/
		for(const struct expr_builtin_keyword *kp=expr_keywords;kp->str;++kp){
			if(p-e==strlen(kp->str)&&!memcmp(e,kp->str,p-e)){
				type=kp->op;
			}
		}
		if(type!=-1){
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
			buf=xmalloc(p-e+2);
			buf[p-e+1]=0;
			memcpy(buf,e,p-e+1);
			switch(type){
				case EXPR_IF:
				case EXPR_WHILE:
					un.eb=expr_getbranchinfo(ep,buf,asym);
					break;
				default:
					un.es=expr_getsuminfo(ep,buf,asym);
					break;
			}
			free(buf);
			//assert(es);
			if(!un.es){
				if(ep->error==EXPR_ENEA)
					memcpy(ep->errinfo,p2,e-p2);
				return NULL;
			}
			v0=expr_newvar(ep);
			expr_addop(ep,v0,un.es,type);
			e=p+1;
			break;
		}
		if(p-e==strlen(asym)&&!memcmp(e,asym,p-e)){
			v0=expr_newvar(ep);
			expr_addcopy(ep,v0,NULL);
			//fprintf(stderr,"asym %ld %s\n",p-e,e);
			e=p;
			break;
		}
		//p1=expr_sym2addr(ep,e,p-e,&type,&dim);
		//fprintf(stderr,"find sym %ld %s\n",p-e,e);
		if(ep->sset&&(sym.es=expr_symset_search(ep->sset,e,p-e))){
			type=sym.es->type;
			dim=sym.es->dim;
			sv=&sym.es->un;
		}else if((sym.ebs=expr_bsym_search(e,p-e))){
			type=sym.ebs->type;
			dim=sym.ebs->dim;
			sv=&sym.ebs->un;
		}else goto number;
			if(type==EXPR_FUNCTION){
				if(*p!='('){
					memcpy(ep->errinfo,e,p-e);
					ep->error=EXPR_EFP;
					//assert(0);
					return NULL;
				}
				v0=expr_getval(ep,p,&e,asym,0);
				//assert(v0);
				if(!v0)return NULL;
				expr_addcall(ep,v0,sv->func);
			}else if(type==EXPR_HOTFUNCTION){
				if(*p!='('){
					memcpy(ep->errinfo,e,p-e);
					ep->error=EXPR_EFP;
					//assert(0);
					return NULL;
				}
				v0=expr_getval(ep,p,&e,asym,0);
				//assert(v0);
				if(!v0)return NULL;
				un.ep=new_expr(sv->hot.expr,sv->hot.asym
					,ep->sset,&ep->error,ep->errinfo);
				if(!un.ep)return NULL;
				expr_addcallhot(ep,v0,un.ep);
			}else if(type==EXPR_CONSTANT){
				v0=expr_newvar(ep);
				expr_addconst(ep,v0,sv->value);
				//printf("%p %lf\n",p1,*(double *)p1);
				//v0=p1;
				e=p;
			}else if(type==EXPR_VARIABLE){
				v0=expr_newvar(ep);
				expr_addcopy(ep,v0,(void *)sv->addr);
				//v0=p1;
				if(assign_level){
					//struct expr_inst *ip=
					expr_addcopy(ep,(void *)sv->addr,NULL)
					->assign_level=1;
					assign_level=0;
				}
				e=p;
			}else if(type==EXPR_MDFUNCTION
				||type==EXPR_MDEPFUNCTION){
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
				buf=xmalloc(p-e+2);
				buf[p-e+1]=0;
				memcpy(buf,e,p-e+1);
				un.em=expr_getmdinfo(ep,buf,asym,
					sv->mdfunc,dim);
				free(buf);
				//assert(es);
				if(!un.em){
					if(ep->error==EXPR_ENEA)
					memcpy(ep->errinfo,p2,e-p2);
					return NULL;
				}
				v0=expr_newvar(ep);
				switch(type){
					case EXPR_MDFUNCTION:
						expr_addcallmd(ep,v0,un.em);
						break;
					case EXPR_MDEPFUNCTION:
						expr_addcallmdep(ep,v0,un.em);
						break;
				}
				e=p+1;
			}else goto symerr;
			break;
number:
		if(expr_atod(e,p-e,&un.v)==1){
			//v1=expr_newvar(ep);
			v0=expr_newvar(ep);
			expr_addconst(ep,v0,un.v);
			//*v1=un.v;
			e=p;
			break;
		}
symerr:
		memcpy(ep->errinfo,e,p-e);
		ep->error=EXPR_ESYMBOL;
		return NULL;
	}

	if(_p)*_p=e;
	if(assign_level){
		ep->error=EXPR_ETNV;
		return NULL;
	}
	return v0;
}
struct expr_vnode {
	struct expr_vnode *next;
	double *v;
	int op;
};
static struct expr_vnode *expr_vn(double *v,int op){
	struct expr_vnode *p;
	p=xmalloc(sizeof(struct expr_vnode));
	p->next=NULL;
	p->v=v;
	p->op=op;
	return p;
}
static struct expr_vnode *expr_vnadd(struct expr_vnode *vp,double *v,int op){
	struct expr_vnode *p;
	//fprintf(stderr,"vp %p\n",vp);
	if(!vp)return expr_vn(v,op);
	for(p=vp;p->next;p=p->next);
//	//fprintf(stderr,"p %p\n",p);
//	//fprintf(stderr,"p %p\n",p);
	p->next=xmalloc(sizeof(struct expr_vnode));
//	//fprintf(stderr,"p->next %p\n",p->next);
	p->next->v=v;
	p->next->op=op;
	p->next->next=NULL;
	return vp;
}
static void expr_vnunion(struct expr *restrict ep,struct expr_vnode *ev){
	struct expr_vnode *p;
	expr_addop(ep,ev->v,ev->next->v,ev->next->op);
	if(ev->next->op==EXPR_ASSIGN){
		for(size_t i=0;i<ep->size;++i){
			if(ep->data[i].op==EXPR_COPY&&ep->data[i].assign_level){
				//printval((uint64_t)ep->data[i].un.src);
				//puts("");
				//printval((uint64_t)e);
				ep->data[i].un.src=ev->v;
				ep->data[i].assign_level=0;
				break;
			}
		}
	}
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
static double *expr_scan(struct expr *restrict ep,const char *e,const char *asym){
	double *v1;
	const char *e0=e;
	int op=0,neg=0;
	struct expr_vnode *ev=NULL,*p;
	//fprintf(stderr,"scan %s\n",e);
	if(*e=='-'){
		neg=1;
		++e;
	}
	do {
//	v0=expr_getval(ep,e,&e,asym,0);
//	if(!v0)goto err;
//	ev=expr_vn(v0,0);
	v1=NULL;
		//printvall((long)op);
		v1=expr_getval(ep,e,&e,asym,op==EXPR_ASSIGN);
		//assert(!ep->error);
		if(!v1)goto err;
		ev=expr_vnadd(ev,v1,op);
	op=-1;
	for(;*e;++e){
		switch(*e){
			case '>':
				if(e[1]=='='){
					op=EXPR_GE;
					++e;
				}else op=EXPR_GT;
				++e;
				goto end1;
			case '<':
				if(e[1]=='='){
					op=EXPR_LE;
					++e;
				}else op=EXPR_LT;
				++e;
				goto end1;
			case '=':
				if(e[1]=='='){
					++e;
				}
				op=EXPR_EQ;
				++e;
				goto end1;
			case '!':
				if(e[1]=='='){
					++e;
					op=EXPR_NE;
				}
				++e;
				goto end1;
			case '&':
				if(e[1]=='&'){
					++e;
				}
				op=EXPR_ANDL;
				++e;
				goto end1;
			case '|':
				if(e[1]=='|'){
					++e;
				}
				op=EXPR_ORL;
				++e;
				goto end1;
			case '+':
				op=EXPR_ADD;
				++e;
				goto end1;
			case '-':
				if(e[1]=='>'){
					op=EXPR_ASSIGN;
					++e;
				}else
				op=EXPR_SUB;
				++e;
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
				if(e[1]=='^'){
					op=EXPR_XORL;
					++e;
				}else op=EXPR_POW;
				++e;
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
				goto end1;
		}
	}
end1:
		continue;	
	}while(op!=-1);
	for(p=ev;p;p=p->next){
		while(p->next&&p->next->op==EXPR_ASSIGN){
			expr_vnunion(ep,p);
		}
	}
	for(p=ev;p;p=p->next){
		while(p->next&&p->next->op==EXPR_POW){
			expr_vnunion(ep,p);
		}
	}
	for(p=ev;p;p=p->next){
		while(p->next&&(
			p->next->op==EXPR_MUL
			||p->next->op==EXPR_DIV
			||p->next->op==EXPR_MOD
			)){
			expr_vnunion(ep,p);
		}
	}
	if(neg)expr_addneg(ep,ev->v);
	for(p=ev;p;p=p->next){
		while(p->next&&(
			p->next->op==EXPR_ADD
			||p->next->op==EXPR_SUB
			)){
			expr_vnunion(ep,p);
		}
	}
	for(p=ev;p;p=p->next){
		while(p->next&&(
			p->next->op==EXPR_GT
			||p->next->op==EXPR_GE
			||p->next->op==EXPR_LT
			||p->next->op==EXPR_LE
			||p->next->op==EXPR_EQ
			||p->next->op==EXPR_NE
			)){
			expr_vnunion(ep,p);
		}
	}
	for(p=ev;p;p=p->next){
		while(p->next&&(
			p->next->op==EXPR_AND
			||p->next->op==EXPR_OR
			||p->next->op==EXPR_XOR
			)){
			expr_vnunion(ep,p);
		}
	}
	while(ev->next)
		expr_vnunion(ep,ev);
	v1=ev->v;
	expr_vnfree(ev);
	return v1;
err:
	expr_vnfree(ev);
	return NULL;
}
void init_expr_symset(struct expr_symset *restrict esp){
	esp->syms=NULL;
	//esp->length=esp->size=0;
	esp->freeable=0;
}
struct expr_symset *new_expr_symset(void){
	struct expr_symset *ep=xmalloc(sizeof(struct expr_symset));
	init_expr_symset(ep);
	ep->freeable=1;
	return ep;
}
void expr_symset_free(struct expr_symset *restrict esp){
	struct expr_symbol *p;
	if(!esp)return;
	while((p=esp->syms)){
		esp->syms=p->next;
		switch(p->type){
			case EXPR_HOTFUNCTION:
				free(p->un.hot.expr);
				free(p->un.hot.asym);
				break;
			default:
				break;
		}
		free(p);
	}
	if(esp->freeable)free(esp);
}
struct expr_symbol *expr_symset_findtail(const struct expr_symset *restrict esp){
	struct expr_symbol *p=esp->syms;
	while(p){
		if(!p->next)return p;
		p=p->next;
	}
	return NULL;
}
struct expr_symbol *expr_symset_add(struct expr_symset *restrict esp,const char *sym,int type,...){
	struct expr_symbol *ep=expr_symset_findtail(esp);
	size_t len=strlen(sym);
	va_list ap;
	const char *p;
	if(len>EXPR_SYMLEN)len=EXPR_SYMLEN;
	/*if(ep->size>=ep->length){
		ep->syms=xrealloc(ep->syms,
		(ep->length+=16)*sizeof(double));
	}
	ep=ep->syms+ep->size++;*/
	if(ep){
		ep->next=xmalloc(sizeof(struct expr_symbol));
		ep=ep->next;
	}else {
		esp->syms=xmalloc(sizeof(struct expr_symbol));
		ep=esp->syms;
	}
	memcpy(ep->str,sym,len);
	if(len<EXPR_SYMLEN)ep->str[len]=0;
	va_start(ap,type);
	switch(type){
		case EXPR_CONSTANT:
			ep->un.value=va_arg(ap,double);
			break;
		case EXPR_VARIABLE:
			ep->un.addr=va_arg(ap,volatile double *);
			break;
		case EXPR_FUNCTION:
			ep->un.func=va_arg(ap,double (*)(double));
			break;
		case EXPR_MDFUNCTION:
			ep->un.mdfunc=va_arg(ap,double (*)(size_t,double *));
			ep->dim=va_arg(ap,unsigned int);
			break;
		case EXPR_MDEPFUNCTION:
			ep->un.mdepfunc=va_arg(ap,double (*)(size_t,
			const struct expr *,double));
			ep->dim=va_arg(ap,unsigned int);
			break;
		case EXPR_HOTFUNCTION:
			p=va_arg(ap,const char *);
			len=strlen(p);
			memcpy(ep->un.hot.expr=xmalloc(len+1),p,len);

			p=va_arg(ap,const char *);
			len=strlen(p);
			memcpy(ep->un.hot.asym=xmalloc(len+1),p,len);
			break;
		default:
			break;
	}
	ep->type=type;
	ep->next=NULL;
	//printf("add:%s,%s(%zu,%p) into %p\n",sym,ep->str,len,addr,&ep->str);
	return ep;
}

const struct expr_symbol *expr_symset_search(const struct expr_symset *restrict esp,const char *sym,size_t sz){
	for(struct expr_symbol *p=esp->syms;p;p=p->next){
	//for(i=0;i<1;++i){
		//printf("%p %zu found %s %p at%p\n",ep->sset,i,ep->sset->syms[i].str,ep->sset->syms[i].str,ep->sset->syms[i].addr);
		//if(!*ep->sset->syms[i].str)strcpy(ep->sset->syms[i].str,"n1");
		if(sz==strlen(p->str)&&!memcmp(p->str,sym,sz)){
			return p;
		}
	}
	return NULL;
}
void expr_symset_copy(struct expr_symset *restrict dst,const struct expr_symset *restrict src){
	if(src)
	for(struct expr_symbol *p=src->syms;p;p=p->next){
		//printf("copy %s %p\n",src->syms[i].str,
		//src->syms[i].addr);
		memcpy(&expr_symset_add(dst,p->str,
		p->type,NULL,p->dim)->un,&p->un,sizeof(union expr_symbol_value));
		
	}
}
struct expr_symset *expr_symset_clone(const struct expr_symset *restrict ep){
	struct expr_symset *es=new_expr_symset();
	expr_symset_copy(es,ep);
	return es;
}
static void expr_strcpy_nospace(char *restrict s1,const char *restrict s2){
	for(;*s2;++s2){
		if(strchr(spaces,*s2))continue;
		*(s1++)=*s2;
	}
	*s1=0;
}
int init_expr(struct expr *restrict ep,const char *e,const char *asym,struct expr_symset *esp){
	double *p;
	char *ebuf;
	ep->data=NULL;
	ep->vars=NULL;
	ep->imm=NULL;
	ep->sset=esp;
	ep->error=0;
	ep->freeable=0;
	memset(ep->errinfo,0,EXPR_SYMLEN);
	ep->length=ep->size=ep->vlength=ep->vsize=0;
	
	if(e&&*e){
		ebuf=xmalloc(strlen(e)+1);
		expr_strcpy_nospace(ebuf,e);
		p=expr_scan(ep,ebuf,asym);
		free(ebuf);
		if(p)
			expr_addend(ep,p);
		else {
			return -1;
		}
	}
	return 0;
}
struct expr *new_expr(const char *e,const char *asym,struct expr_symset *esp,int *error,char errinfo[EXPR_SYMLEN]){

	struct expr *ep=xmalloc(sizeof(struct expr));
	if(init_expr(ep,e,asym,esp)){
		*error=ep->error;
		if(errinfo)memcpy(errinfo,ep->errinfo,EXPR_SYMLEN);
		free(ep);
		return NULL;
	}
	ep->freeable=1;
	return ep;
}
double expr_eval(const struct expr *restrict ep,double input){
	struct expr_inst *ip=ep->data;
	double step,sum,from,to,y;
	int neg;
	if(ep->imm)return ep->imm(input);
	ip=ep->data;
	while(ip->op!=EXPR_END){
		switch(ip->op){
			case EXPR_COPY:
			case EXPR_ASSIGN:
				//printval((uint64_t)ip->op);
				//printval((uint64_t)ip->dst);
				//printval((uint64_t)ip->un.src);
				//fprintf(stderr,"%lf to %lf\n",*ip->dst,*ip->un.src);
				if(ip->un.src)
					*ip->dst=*ip->un.src;
				else
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
			case EXPR_NEG:
				*ip->dst=-*ip->dst;
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
			case EXPR_EQ:
				*ip->dst=fabs(*ip->dst-*ip->un.src)<=DBL_EPSILON?
					1.0:
					0.0;
				break;
			case EXPR_NE:
				*ip->dst=fabs(*ip->dst-*ip->un.src)>DBL_EPSILON?
					1.0:
					0.0;
				break;
#define CALLOGIC(a,b,_s) ((fabs(a)>DBL_EPSILON) _s (fabs(b)>DBL_EPSILON))
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
				if(step<0)step=-step;\
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
			CALSUM(EXPR_SUM,sum+=y,sum=0,-sum);
			CALSUM(EXPR_INT,sum+=step*y,sum=0,-sum);
			CALSUM(EXPR_PROD,sum*=y,sum=1,1/sum);
			CALSUM(EXPR_SUP,if(y>sum)sum=y,sum=DBL_MIN,sum);
			CALSUM(EXPR_INF,if(y<sum)sum=y,sum=DBL_MAX,sum);
			CALSUM(EXPR_AND,sum=sum!=DBL_MAX?expr_and2(sum,y):y,sum=DBL_MAX,sum);
			CALSUM(EXPR_OR,sum=sum!=0.0?expr_or2(sum,y):y,sum=0.0,sum);
			CALSUM(EXPR_XOR,sum=sum!=0.0?expr_xor2(sum,y):y,sum=0.0,sum);
			CALSUM(EXPR_GCD,sum=sum!=DBL_MAX?expr_gcd2(sum,y):y,sum=DBL_MAX,sum);
			CALSUM(EXPR_LCM,sum=sum!=1.0?expr_lcm2(sum,y):y,sum=1.0,sum);

			case EXPR_FOR:
				ip->un.es->index=
				expr_eval(ip->un.es->from,input);//init
//				printvald(from);
//				printvald(to);
//				printvald(step);
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
				//printvald(from);
//				printvald(to);
//				printvald(step);
				to=expr_eval(ip->un.es->to,input);//times
				if(to<0)to=-to;
				for(;to>DBL_EPSILON;to-=1.0){
				//printf("ip->un.es->index %p val:%lf\n",&ip->un.es->index,ip->un.es->index);
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
			case EXPR_CALLMD:
				for(unsigned int i=0;i<ip->un.em->dim;++i)
					ip->un.em->args[i]=
						expr_eval(
						ip->un.em->eps+i,input);
				
				*ip->dst=ip->un.em->un.func(ip->un.em->dim,ip->un.em->args);
				break;
			case EXPR_CALLMDEP:
				*ip->dst=ip->un.em->un.funcep(ip->un.em->dim,ip->un.em->eps,input);
				break;
			case EXPR_CALLHOT:
				*ip->dst=expr_eval(ip->un.hotfunc,*ip->dst);
				break;
			default:
				abort();
				break;
		}
		++ip;
	}
	return *ip->dst;
}
