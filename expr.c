/*******************************************************************************
 *License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>*
 *This is free software: you are free to change and redistribute it.           *
 *******************************************************************************/
#define _GNU_SOURCE
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <float.h>
#include "expr.h"
#define printval(x) fprintf(stderr,#x ":%lu\n",x)
#define printvall(x) fprintf(stderr,#x ":%ld\n",x)
#define printvald(x) fprintf(stderr,#x ":%lf\n",x)
#define NDEBUG
#include <assert.h>
//#define free(v)
static const char *eerror[]={"Unknown error","Unknown symbol","Parentheses do not match","Function and keyword must be followed by a \'(\'","No value in parenthesis","No enough or too much argument","Bad number","Target is not variable","Empty value","Unexpected operator","Zero-argument function must be followed by \'()\'","Defined symbol"};
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
double expr_gcd2(double x,double y){
	return (double)expr_gcd64((uint64_t)(fabs(x)+DBL_EPSILON),
		(uint64_t)(fabs(y)+DBL_EPSILON));
}
double expr_lcm2(double x,double y){
	uint64_t a=(uint64_t)(fabs(x)+DBL_EPSILON),b=(uint64_t)(fabs(y)+DBL_EPSILON);
	return (double)(a*b)/expr_gcd64(a,b);
}

static double expr_rand(size_t n,double *args){
	//assert(n==2);
	return args[0]+(args[1]-args[0])*drand48();
}
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
#define REGFSYM(s) {.str=#s,.un={.func=s},.type=EXPR_FUNCTION,.flag=EXPR_SF_INJECTION}
#define REGCSYM(s) {.str=#s,.un={.value=s},.type=EXPR_CONSTANT}
#define REGFSYM2(s,sym) {.str=s,.un={.func=sym},.type=EXPR_FUNCTION,.flag=EXPR_SF_INJECTION}
#define REGMDSYM2(s,sym,d) {.str=s,.un={.md={.func=sym,.dim=d}},.type=EXPR_MDFUNCTION}
#define REGMDEPSYM2(s,sym,d) {.str=s,.un={.mdep={.func=sym,.dim=d}},.type=EXPR_MDEPFUNCTION}
#define REGCSYM2(s,val) {.str=s,.un={.value=val},.type=EXPR_CONSTANT}
//#define REGSYMMD(s,n) {#s,s,n}
const struct expr_builtin_keyword expr_keywords[]={
	{"sum",EXPR_SUM,5,"index_name,start_index,end_index,index_step,addend"},
	{"int",EXPR_INT,5,"integral_var_name,upper_limit,lower_limit,epsilon,integrand"},
	{"prod",EXPR_PROD,5,"index_name,start_index,end_index,index_step,factor"},
	{"pai",EXPR_PROD,5,"index_name,start_index,end_index,index_step,factor"},
	{"sup",EXPR_SUP,5,"index_name,start_index,end_index,index_step,element"},
	{"infi",EXPR_INF,5,"index_name,start_index,end_index,index_step,element"},
	{"AND",EXPR_ANDN,5,"index_name,start_index,end_index,index_step,element"},
	{"OR",EXPR_ORN,5,"index_name,start_index,end_index,index_step,element"},
	{"XOR",EXPR_XORN,5,"index_name,start_index,end_index,index_step,element"},
	{"GCD",EXPR_GCDN,5,"index_name,start_index,end_index,index_step,element"},
	{"LCM",EXPR_LCMN,5,"index_name,start_index,end_index,index_step,element"},
	{"for",EXPR_FOR,5,"var_name,start_var,cond,body,value"},
	{"loop",EXPR_LOOP,5,"var_name,start_var,count,body,value"},
	{"while",EXPR_WHILE,3,"cond,body,value"},
	{"if",EXPR_IF,3,"cond,if_value,else_value"},
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
};
const struct expr_builtin_symbol *expr_bsym_search(const char *sym,size_t sz){
	const struct expr_builtin_symbol *p;
	for(p=expr_bsyms;p->str;++p){
		if(sz==strlen(p->str)&&!memcmp(p->str,sym,sz)){
			return p;
		}
	}
	return NULL;
}
const struct expr_builtin_symbol *expr_bsym_rsearch(void *addr){
	const struct expr_builtin_symbol *p;
	for(p=expr_bsyms;p->str;++p){
		if(p->un.uaddr==addr){
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
				case EXPR_ANDN:
				case EXPR_ORN:
				case EXPR_XORN:
				case EXPR_GCDN:
				case EXPR_LCMN:
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
					free(ip->un.em->args);
				case EXPR_CALLMDEP:
					for(size_t i=0;i<ip->un.em->dim;++i)
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
__attribute__((noinline))
struct expr_inst *expr_addop(struct expr *restrict ep,double *dst,void *src,enum expr_op op){
	struct expr_inst *ip;
	EXTEND_DATA
	ip=ep->data+ep->size++;
	ip->op=op;
	ip->dst=dst;
	ip->un.src=(double *)src;
	//printvald(*(double *)&src);
	return ip;
}

static double *expr_newvar(struct expr *restrict ep){
	double *r;
	if(ep->vsize>=ep->vlength){
		ep->vars=xrealloc(ep->vars,
			(ep->vlength+=16)*sizeof(double));
	}
	r=ep->vars+ep->vsize++;
	*r=NAN;
	return r;
}
static double *expr_createvar(struct expr *restrict ep,const char *symbol){
	double *r=expr_newvar(ep);
	if(!ep->sset_shouldfree){
		if(!ep->sset)ep->sset=new_expr_symset();
		else ep->sset=expr_symset_clone(ep->sset);
		ep->sset_shouldfree=1;
	}
	expr_symset_add(ep->sset,symbol,EXPR_VARIABLE,r);
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
static const char *expr_getsym(const char *c){
	while(*c&&!strchr(special,*c))
		++c;
	return c;
}
static const char *expr_getsym_expo(const char *c){
	const char *c0=c;
	while(*c&&!strchr(special,*c))
		++c;
	if(c-c0>=2&&(*c=='-'||*c=='+')&&(c[-1]=='e'||c[-1]=='E')&&((c[-2]<='9'&&c[-2]>=0)||c[-2]=='.')){
		return expr_getsym(c+1);
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
static struct expr_mdinfo *expr_getmdinfo(struct expr *restrict ep,char *e,const char *asym,void *func,size_t dim,int ifep){
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
static struct expr_suminfo *expr_getsuminfo(struct expr *restrict ep,char *e,const char *asym){
	char **v=expr_sep(ep,e);
	char **p;
	struct expr_suminfo *es;
	struct expr_symset *sset;
//	int error;
//	char ef[EXPR_SYMLEN];
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
	es->from=new_expr(v[1],asym,ep->sset,&ep->error,ep->errinfo);
	if(!es->from)goto err1;
	es->to=new_expr(v[2],asym,sset,&ep->error,ep->errinfo);
	if(!es->to)goto err2;
	es->step=new_expr(v[3],asym,sset,&ep->error,ep->errinfo);
	if(!es->step)goto err3;
	//sset=expr_symset_clone(ep->sset);

	es->ep=new_expr(v[4],asym,sset,&ep->error,ep->errinfo);
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
err0:
	expr_free2(v);
	return NULL;
}
static struct expr_branchinfo *expr_getbranchinfo(struct expr *restrict ep,char *e,const char *asym){
	char **v=expr_sep(ep,e);
	char **p;
	struct expr_branchinfo *eb;
//	int error;
//	char ef[EXPR_SYMLEN];
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
	eb->cond=new_expr(v[0],asym,ep->sset,&ep->error,ep->errinfo);
	//assert(eb->cond);
	if(!eb->cond)goto err1;
	eb->body=new_expr(v[1],asym,ep->sset,&ep->error,ep->errinfo);
	//assert(eb->body);
	if(!eb->body)goto err2;
	eb->value=new_expr(v[2],asym,ep->sset,&ep->error,ep->errinfo);
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
//	ep->error=error;
//	memcpy(ep->errinfo,ef,EXPR_SYMLEN);
err0:
	expr_free2(v);
	return NULL;
}
static double *expr_scan(struct expr *restrict ep,const char *e,const char *asym);
static double *expr_getvalue(struct expr *restrict ep,const char *e,const char **_p,const char *asym){
	const char *p,*p2;//*e0=e
	char *buf;
	double *v0=NULL;
	int r0;
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
		//fprintf(stderr,"unknown sym %ld %s\n",p-e,e);
		if(p==e){
			if(*e&&strchr(special,*e)){
				*ep->errinfo=*e;
				ep->error=EXPR_EUO;
				return NULL;
			}
			goto symerr;
		}
		type=-1;
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
		if(asym&&p-e==strlen(asym)&&!memcmp(e,asym,p-e)){
			v0=expr_newvar(ep);
			expr_addinput(ep,v0);
			//fprintf(stderr,"asym %ld %s\n",p-e,e);
			e=p;
			break;
		}
		//p1=expr_sym2addr(ep,e,p-e,&type,&dim);
		//fprintf(stderr,"find sym %ld %s\n",p-e,e);
		if(ep->sset&&(sym.es=expr_symset_search(ep->sset,e,p-e))){
			type=sym.es->type;
			sv=&sym.es->un;
		}else if((sym.ebs=expr_bsym_search(e,p-e))){
			type=sym.ebs->type;
			sv=&sym.ebs->un;
		}else goto number;
			if(type==EXPR_FUNCTION){
				if(*p!='('){
					memcpy(ep->errinfo,e,p-e);
					ep->error=EXPR_EFP;
					//assert(0);
					return NULL;
				}
				v0=expr_getvalue(ep,p,&e,asym);
				//assert(v0);
				if(!v0)return NULL;
				expr_addcall(ep,v0,sv->func);
			}else if(type==EXPR_ZAFUNCTION){
				if(*p!='('||p[1]!=')'){
					memcpy(ep->errinfo,e,p-e);
					ep->error=EXPR_EZAFP;
					//assert(0);
					return NULL;
				}
				v0=expr_newvar(ep);
				//assert(v0);
				//if(!v0)return NULL;
				expr_addcallza(ep,v0,sv->zafunc);
				e=p+2;
			}else if(type==EXPR_HOTFUNCTION){
				if(*p!='('){
					memcpy(ep->errinfo,e,p-e);
					ep->error=EXPR_EFP;
					//assert(0);
					return NULL;
				}
				v0=expr_getvalue(ep,p,&e,asym);
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
				/*if(assign_level){
					//struct expr_inst *ip=
					expr_addcopy(ep,(void *)sv->addr,NULL)
					->assign_level=1;
					assign_level=0;
				}*/
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
				if(type==EXPR_MDEPFUNCTION)
					un.em=expr_getmdinfo(ep,buf,asym,
					sv->mdep.func,sv->mdep.dim,1);
				else	
					un.em=expr_getmdinfo(ep,buf,asym,
					sv->md.func,sv->md.dim,0);
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
		p=expr_getsym_expo(e);
		r0=expr_atod(e,p-e,&un.v);
		if(r0==1){
			//v1=expr_newvar(ep);
			v0=expr_newvar(ep);
			expr_addconst(ep,v0,un.v);
			//*v1=un.v;
			e=p;
			break;
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

	if(_p)*_p=e;
	/*if(assign_level){
		ep->error=EXPR_ETNV;
		return NULL;
	}*/
	return v0;
}
struct expr_vnode {
	struct expr_vnode *next;
	double *v;
	enum expr_op op;
};
static struct expr_vnode *expr_vn(double *v,enum expr_op op){
	struct expr_vnode *p;
	p=xmalloc(sizeof(struct expr_vnode));
	p->next=NULL;
	p->v=v;
	p->op=op;
	return p;
}
static struct expr_vnode *expr_vnadd(struct expr_vnode *vp,double *v,enum expr_op op){
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
	/*if(ev->next->op==EXPR_ASSIGN){
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
	}*/
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
	enum expr_op op=0;
	int neg=0;
	struct expr_vnode *ev=NULL,*p;
	//fprintf(stderr,"scan %s\n",e);
	if(*e=='-'){
		neg=1;
		++e;
	}
	do {
//	v0=expr_getvalue(ep,e,&e,asym,0);
//	if(!v0)goto err;
//	ev=expr_vn(v0,0);
	v1=NULL;
		//printvall((long)op);
		v1=expr_getvalue(ep,e,&e,asym);
		//assert(!ep->error);
		if(!v1)goto err;
		ev=expr_vnadd(ev,v1,op);
	op=-1;
	for(;*e;){
		switch(*e){
			case '>':
				if(e[1]=='='){
					op=EXPR_GE;
					++e;
				}else if(e[1]=='>'){
					op=EXPR_SHR;
					++e;
				}else if(e[1]=='<'){
					op=EXPR_NE;
					++e;
				}else op=EXPR_GT;
				++e;
				goto end1;
			case '<':
				if(e[1]=='='){
					op=EXPR_LE;
					++e;
				}else if(e[1]=='<'){
					op=EXPR_SHL;
					++e;
				}else if(e[1]=='>'){
					op=EXPR_NE;
					++e;
				}else op=EXPR_LT;
				++e;
				goto end1;
			case '=':
				if(e[1]=='='){
					op=EXPR_EQ;
					++e;
				}else op=EXPR_SEQ;
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
					op=EXPR_ANDL;
					++e;
				}
				else op=EXPR_AND;
				++e;
				goto end1;
			case '|':
				if(e[1]=='|'){
					op=EXPR_ORL;
					++e;
				}
				else op=EXPR_OR;
				++e;
				goto end1;
			case '+':
				op=EXPR_ADD;
				++e;
				goto end1;
			case '-':
				if(e[1]=='>'){
					const struct expr_symbol *esp=NULL;
					const char *p1=expr_getsym(e+=2);
					if(p1==e){
						if(*e&&strchr(special,*e)){
							*ep->errinfo=*e;
							ep->error=EXPR_EUO;
						}else {
							ep->error=EXPR_EEV;
						}
					goto err;
					}
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
				}else if(e[1]&&e[1]=='-'&&e[2]=='>'){
					const char *p1=expr_getsym(e+=3);
					double *v2;
					char *v2sym;
					if(p1==e){
						if(*e&&strchr(special,*e)){
							*ep->errinfo=*e;
							ep->error=EXPR_EUO;
						}else {
							ep->error=EXPR_EEV;
						}
					goto err;
					}
					if(expr_bsym_search(e,p1-e)
					||(ep->sset&&expr_symset_search(ep->sset,e,p1-e))){
						ep->error=EXPR_EDS;
						memcpy(ep->errinfo,e,p1-e);
						goto err;
					}
					v2sym=xmalloc(p1-e+1);
					memcpy(v2sym,e,p1-e);
					v2sym[p1-e]=0;
					v2=expr_createvar(ep,v2sym);
					free(v2sym);
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
				if(e[1]=='^'){
					if(e[2]=='^'){
						op=EXPR_XORL;
						++e;
					}else {
						op=EXPR_XOR;
					}
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
	//SETPREC1(EXPR_ASSIGN)
	SETPREC1(EXPR_POW)
	SETPREC3(EXPR_MUL,EXPR_DIV,EXPR_MOD)
	if(neg)expr_addneg(ep,ev->v);
	SETPREC2(EXPR_ADD,EXPR_SUB)
	SETPREC2(EXPR_SHL,EXPR_SHR)
	SETPREC4(EXPR_LT,EXPR_LE,EXPR_GT,EXPR_GE)
	SETPREC3(EXPR_SEQ,EXPR_EQ,EXPR_NE)
	SETPREC1(EXPR_AND)
	SETPREC1(EXPR_XOR)
	SETPREC1(EXPR_OR)
	SETPREC1(EXPR_ANDL)
	SETPREC1(EXPR_XORL)
	SETPREC1(EXPR_ORL)
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
		/*switch(p->type){
			case EXPR_HOTFUNCTION:
				if(p->un.hot.expr){
					free(p->un.hot.expr);
					p->un.hot.expr=NULL;
				}
				if(p->un.hot.asym){
					free(p->un.hot.asym);
					p->un.hot.asym=NULL;
				}
				break;
			default:
				break;
		}*/
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
	va_list ap;
	struct expr_symbol *r;
	va_start(ap,type);
	r=expr_symset_vadd(esp,sym,type,ap);
	va_end(ap);
	return r;
}
struct expr_symbol *expr_symset_vadd(struct expr_symset *restrict esp,const char *sym,int type,va_list ap){
	struct expr_symbol *ep=expr_symset_findtail(esp),**next;
	size_t len,len_expr,len_asym;
	const char *p,*p1;
	/*if(ep->size>=ep->length){
		ep->syms=xrealloc(ep->syms,
		(ep->length+=16)*sizeof(double));
	}
	ep=ep->syms+ep->size++;*/
	len=sizeof(struct expr_symbol);
	if(ep){
		ep->next=xmalloc(sizeof(struct expr_symbol));
		next=&ep->next;
	}else {
		esp->syms=xmalloc(sizeof(struct expr_symbol));
		next=&esp->syms;
	}
	ep=*next;
	ep->length=len;
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
			ep->un.md.func=va_arg(ap,double (*)(size_t,double *));
			ep->un.md.dim=va_arg(ap,size_t);
			break;
		case EXPR_MDEPFUNCTION:
			ep->un.mdep.func=va_arg(ap,double (*)(size_t,
				const struct expr *,double));
			ep->un.mdep.dim=va_arg(ap,size_t);
			break;
		case EXPR_HOTFUNCTION:
			p=va_arg(ap,const char *);
			len=len_expr=strlen(p);
			p1=va_arg(ap,const char *);
			len+=len_asym=strlen(p1)+2;
			ep->length+=len;
			ep=xrealloc(ep,ep->length);
			*next=ep;
			ep->un.hot.expr=ep->hot_expr_asym;
			ep->un.hot.asym=ep->hot_expr_asym+len_expr+1;
			memcpy(ep->un.hot.expr,p,len_expr);
			ep->un.hot.expr[len_expr]=0;
			memcpy(ep->un.hot.asym,p1,len_asym);
			ep->un.hot.asym[len_asym]=0;
			break;
		case EXPR_ZAFUNCTION:
			ep->un.zafunc=va_arg(ap,double (*)(void));
			break;
		default:
			return NULL;
	}
	len=strlen(sym);
	if(len>=EXPR_SYMLEN)len=EXPR_SYMLEN-1;
	memcpy(ep->str,sym,len+1);
	ep->type=type;
	ep->flag=0;
	ep->next=NULL;
	//printf("add:%s,%s(%zu) into %p\n",sym,ep->str,len,&ep->str);
	return ep;
}
static struct expr_symbol *expr_symset_addcopy(struct expr_symset *restrict esp,const struct expr_symbol *restrict es){
	struct expr_symbol *ep=expr_symset_findtail(esp);
	if(ep){
		ep->next=xmalloc(es->length);
		ep=ep->next;
	}else {
		esp->syms=xmalloc(es->length);
		ep=esp->syms;
	}
	memcpy(ep,es,es->length);
	ep->next=NULL;
	return ep;
}
const struct expr_symbol *expr_symset_search(const struct expr_symset *restrict esp,const char *sym,size_t sz){
	for(struct expr_symbol *p=esp->syms;p;p=p->next){
		//puts(p->str);
		if(sz==strlen(p->str)&&!memcmp(p->str,sym,sz)){
			return p;
		}
	}
	return NULL;
}
const struct expr_symbol *expr_symset_rsearch(const struct expr_symset *restrict esp,void *addr){
	for(struct expr_symbol *p=esp->syms;p;p=p->next){
		if(p->un.uaddr==addr){
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
		//if(p->type==EXPR_HOTFUNCTION)continue;
		expr_symset_addcopy(dst,p);
		
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
int init_expr_old(struct expr *restrict ep,const char *e,const char *asym,struct expr_symset *esp){
	double *p;
	char *ebuf;
	ep->data=NULL;
	ep->vars=NULL;
	ep->sset=esp;
	ep->sset_shouldfree=0;
	ep->error=0;
	ep->freeable=0;
	memset(ep->errinfo,0,EXPR_SYMLEN);
	ep->length=ep->size=ep->vlength=ep->vsize=0;
	
	if(e){
		ebuf=xmalloc(strlen(e)+1);
		expr_strcpy_nospace(ebuf,e);
		p=expr_scan(ep,ebuf,asym);
		free(ebuf);
		if(ep->sset_shouldfree){
			free(ep->sset);
			ep->sset=esp;
		}
		if(p){
			expr_addend(ep,p);
		}else {
			return -1;
		}
	}
	return 0;
}
static void expr_optimize(struct expr *restrict ep);
int init_expr(struct expr *restrict ep,const char *e,const char *asym,struct expr_symset *esp){
	if(!init_expr_old(ep,e,asym,esp))
		expr_optimize(ep);
	else return -1;
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
static void expr_writeconsts(struct expr *restrict ep){
	for(struct expr_inst *ip=ep->data;ip->op!=EXPR_END;++ip){
		if(ip->op==EXPR_CONST&&
			ip->dst>=ep->vars&&ip->dst<ep->vars+ep->vsize
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
	expr_writeconsts(ep);
}
static int expr_modified(const struct expr *restrict ep,double *v){
	if(v<ep->vars||v>=ep->vars+ep->vsize)
		return 1;
	for(struct expr_inst *ip=ep->data;ip->op!=EXPR_END;++ip){
		//printf("checking vars[%zd] at %p on %p\n",(ssize_t)(v-ep->vars),ip->un.src,ep->vars);
		if(ip->dst==v&&ip->op!=EXPR_CONST){
			return 1;
		}
		//printf("ok\n");
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
				case EXPR_GT:
				case EXPR_GE:
				case EXPR_EQ:
				case EXPR_SEQ:
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
				case EXPR_EQ:
					sum=fabs(sum-*ip->un.src)<=DBL_EPSILON?
						1.0:
						0.0;
					break;
				case EXPR_NE:
					sum=fabs(sum-*ip->un.src)>DBL_EPSILON?
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
			/*case EXPR_GT:
				if(*ip->un.src!=-INFINITY)break;
			case EXPR_GE:
				if(*ip->un.src!=-INFINITY&&
				*ip->un.src!=DBL_MIN)break;
			case EXPR_LT:
				if(*ip->un.src!=INFINITY)break;
			case EXPR_LE:
				if(*ip->un.src!=INFINITY&&
				*ip->un.src!=DBL_MAX)break;*/
			case EXPR_ANDL:
				if(fabs(*ip->un.src)<=DBL_EPSILON){
					ip->op=EXPR_CONST;
					ip->un.value=0.0;
					r=1;
				}
				break;
			//case EXPR_XORL:
			case EXPR_ORL:
				if(fabs(*ip->un.src)>DBL_EPSILON){
					ip->op=EXPR_CONST;
					ip->un.value=1.0;
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
		//printf("before checking vars[%zd]\n",(ssize_t)(ip->dst-ep->vars));
		if(ip->op==EXPR_CONST&&!expr_modified(ep,ip->dst)){
			//*ip->dst=ip->un.value;
			//not necessary
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
		case EXPR_GT:
		case EXPR_GE:
		case EXPR_LT:
		case EXPR_LE:
		case EXPR_SEQ:
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
		case EXPR_SEQ:
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
		case EXPR_CALLZA:
		case EXPR_CALLMD:
		case EXPR_CALLMDEP:
			return 1;
		default:
			return 0;
	}
}
static int expr_used(struct expr *restrict ep,struct expr_inst *ip){
	struct expr_inst *ip1;
	int ov;
	for(ip1=ip+1;ip1<ep->data+ep->size;++ip1){
		ov=expr_override(ip1->op);
		if((expr_usesrc(ip1->op)&&ip1->un.src==ip->dst)
			||(ip1->dst==ip->dst&&!ov)
				)break;
		if(ip1->dst==ip->dst&&ov){
			ip1=ep->data+ep->size;
			break;
		}
	}
	return ip1!=ep->data+ep->size;
}
static void expr_optimize_unused(struct expr *restrict ep){
	for(struct expr_inst *ip=ep->data;ip->op!=EXPR_END;++ip){
		/*printf("%d,%d,%d,%d [%zu]\n",ip->dst<ep->vars
			,ip->dst>=ep->vars+ep->vsize
			,expr_side(ip->op)
			,expr_used(ep,ip),ip-ep->data);*/
		if(ip->dst<ep->vars
			||ip->dst>=ep->vars+ep->vsize
			||expr_side(ip->op))continue;
		if(!expr_used(ep,ip)){
			ip->dst=NULL;
		}
	}
	expr_optimize_completed(ep);
}
static void expr_optimize_constneg(struct expr *restrict ep){
	for(struct expr_inst *ip=ep->data;ip->op!=EXPR_END;++ip){
		if(ip->op!=EXPR_NEG)continue;
		for(struct expr_inst *ip1=ip-1;ip>=ep->data;--ip1){
			if(ip1->dst!=ip->dst)continue;
			if(ip1->op!=EXPR_CONST)break;
			ip1->un.value=-ip1->un.value;
			ip->dst=NULL;
			break;
		}
	}
	expr_optimize_completed(ep);
}
static int expr_injection_symtype(int type){
	switch(type){
		case EXPR_FUNCTION:
		case EXPR_ZAFUNCTION:
		//case EXPR_HOTFUNCTION:
			return 1;
		default:
			return 0;
	}
}
static int expr_optimize_injection(struct expr *restrict ep){
	union {
		const struct expr_symbol *es;
		const struct expr_builtin_symbol *ebs;
	} sym;
	int r=0;
	for(struct expr_inst *ip=ep->data;ip->op!=EXPR_END;++ip){
		switch(ip->op){
			case EXPR_CALL:
			case EXPR_CALLZA:
			//case EXPR_CALLHOT: not work
				break;
			default:
				continue;
		}
			if(!ep->sset
				||!(sym.es=expr_symset_rsearch(ep->sset,ip->un.func))
				||!expr_injection_symtype(sym.es->type)
				||!(sym.es->flag&EXPR_SF_INJECTION)
			){
				if(!(sym.ebs=expr_bsym_rsearch(ip->un.func))
				||!expr_injection_symtype(sym.ebs->type)
				||!(sym.ebs->flag&EXPR_SF_INJECTION)
				)continue;
			}
			//printf("in %zd\n",ip-ep->data);
			if(ip->op==EXPR_CALLZA){
				ip->un.value=ip->un.zafunc();
			//printf("in %lf\n",ip->un.value);
				ip->op=EXPR_CONST;
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
				/*case EXPR_CALLHOT:
					ip1->un.value=expr_eval(ip->un.hotfunc,ip1->un.value);
					break;*/
				default:
					abort();
			}
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
			ip->dst<ep->vars||
			ip->dst>=ep->vars+ep->vsize
			)continue;
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
			//ip->dst<ep->vars||
			//ip->dst>=ep->vars+ep->vsize||
			//ip->un.src<ep->vars||
			//ip->un.src>=ep->vars+ep->vsize
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
		lip->dst>=ep->vars&&lip->dst<ep->vars+ep->vsize
		){
		ip->dst=lip->un.src;
		lip->dst=NULL;
	}
	expr_optimize_completed(ep);
}
static int expr_optimize_once(struct expr *restrict ep){
	int r=0;
	expr_optimize_const(ep);
	expr_optimize_constneg(ep);
	r|=expr_optimize_injection(ep);

	//expr_optimize_contmul(ep,EXPR_COPY);
/*	expr_optimize_contmul(ep,EXPR_OR);
	expr_optimize_contmul(ep,EXPR_XOR);
	expr_optimize_contmul(ep,EXPR_AND);
	expr_optimize_contsh(ep);
	expr_optimize_contadd(ep);
	expr_optimize_contmul(ep,EXPR_MOD);
	expr_optimize_contmul(ep,EXPR_DIV);
	expr_optimize_contmul(ep,EXPR_MUL);
	expr_optimize_contmul(ep,EXPR_POW);*/

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
	expr_optimize_contmul(ep,EXPR_SEQ);
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
//	return;
	if((ep->size<s||r)&&ep->size>1){
		s=ep->size;
		goto again;
	}

}
double expr_eval(const struct expr *restrict ep,double input){
	double step,sum,from,to,y;
	int neg;
	for(struct expr_inst *ip=ep->data;;++ip){
		assert(ip->op>=0);
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

			case EXPR_SEQ:
				*ip->dst=*ip->dst==*ip->un.src?
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
			case EXPR_CALLZA:
				*ip->dst=ip->un.zafunc();
				break;
			case EXPR_CALLMD:
				for(size_t i=0;i<ip->un.em->dim;++i)
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
			case EXPR_END:
				return *ip->dst;
		}
	}
}
