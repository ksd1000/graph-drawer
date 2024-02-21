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
#include "expr.h"
#define printval(x) fprintf(stderr,#x ":%lu\n",x)
#define printvall(x) fprintf(stderr,#x ":%ld\n",x)
#define printvald(x) fprintf(stderr,#x ":%lf\n",x)
static const char *eerror[]={"Unknown error","Unknown symbol","Parentheses do not match","Function and keyword must be followed by a \'(\'","No value in parenthesis","No enough or too much argument","Bad number","Target is not variable"};
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
	return (double)expr_gcd64((uint64_t)(x+DBL_EPSILON),
		(uint64_t)(y+DBL_EPSILON));
}
double expr_lcm2(double x,double y){
	uint64_t a=(uint64_t)(x+DBL_EPSILON),b=(uint64_t)(y+DBL_EPSILON);
	return (double)(a*b)/expr_gcd64(a,b);
}
double expr_rand12(void){
	double ret;
	expr_memrand(&ret,sizeof(double));
	EXPR_EDEXP(&ret)=1023;
	EXPR_EDSIGN(&ret)=0;
	return ret;
}
static double expr_rand(size_t n,double *args){
	//assert(n==2);
	return args[0]+(args[1]-args[0])*(expr_rand12()-1.0);
}
static double expr_if(size_t n,double *args){
	//assert(n==3);
	return fabs(args[0])>DBL_EPSILON?args[1]:args[2];
}
static double expr_ifnot(size_t n,double *args){
	//assert(n==3);
	return fabs(args[0])>DBL_EPSILON?args[2]:args[1];
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
static double expr_piece(ssize_t n,double *args){
	while(--n>0){
		if(fabs(*(args++))>DBL_EPSILON){
			return *args;
		}else {
			--n;
			++args;
		}

	}
	return *args;
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
#define REGSYM(s) {#s,s}
#define REGSYMMD(s,n) {#s,s,n}
static const struct {
	const char *str;
	double (*addr)(double);
} syms[]={
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
static const struct {
	const char *str;
	double val;
} csyms[]={
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
static const struct {
	const char *str;
	double (*addr)(size_t,double *);
	unsigned int dim,unused;
} mdsyms[]={
	{"and",expr_and,0},
	{"or",expr_or,0},
	{"xor",expr_xor,0},
	{"gcd",expr_gcd,0},
	{"hypot",expr_hypot,0},
	{"if",expr_if,3},
	{"ifnot",expr_ifnot,3},
	{"lcm",expr_lcm,0},
	{"min",expr_min,0},
	{"max",expr_max,0},
	{"piece",(double (*)(size_t, double *))expr_piece,0},
	{"rand",expr_rand,2},
	{NULL,NULL}
};
static double (*expr_sym2addr(struct expr *restrict ep,const char *sym,size_t sz,int *type,unsigned int *dim))(double){
	size_t i;
	if(sz>=EXPR_SYMLEN)return NULL;
	if(ep->sset)
	for(i=0;i<ep->sset->size;++i){
		if(sz==strlen(ep->sset->syms[i].str)&&!memcmp(ep->sset->syms[i].str,sym,sz)){
			if(type)*type=ep->sset->syms[i].type;
			if(dim)*dim=ep->sset->syms[i].dim;
			return ep->sset->syms[i].addr;
		}
	}
	for(i=0;syms[i].str;++i){
		if(sz==strlen(syms[i].str)&&!memcmp(syms[i].str,sym,sz)){
			if(type)*type=EXPR_FUNCTION;
			return syms[i].addr;
		}
	}
	for(i=0;csyms[i].str;++i){
		if(sz==strlen(csyms[i].str)&&!memcmp(csyms[i].str,sym,sz)){
			if(type)*type=EXPR_PARAMETER;
			return (double (*)(double))&csyms[i].val;
		}
	}
	for(i=0;mdsyms[i].str;++i){
		if(sz==strlen(mdsyms[i].str)&&!memcmp(mdsyms[i].str,sym,sz)){
			if(type)*type=EXPR_MDFUNCTION;
			if(dim)*dim=mdsyms[i].dim;
			return (double (*)(double))mdsyms[i].addr;
		}
	}
	return NULL;
}
const char *expr_error(int error){
	if(error<0)error=-error;
	if(error>=(sizeof(eerror)/sizeof(*eerror)))return eerror[0];
	else return eerror[error];
}
static void *xmalloc(size_t size){
	void *r=malloc(size);
	assert(r != NULL);
	return r;
}
static void *xrealloc(void *old,size_t size){
	void *r=realloc(old,size);
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
				case EXPR_NEST:
					expr_free(ip->un.es->ep);
					expr_free(ip->un.es->from);
					expr_free(ip->un.es->to);
					expr_free(ip->un.es->step);
					//expr_symset_free(ip->un.es->sset);
					free(ip->un.es);
					break;
				case EXPR_CALLMD:
					for(unsigned int i=0;i<ip->un.em->dim;++i)
						expr_free(ip->un.em->eps[i]);
					free(ip->un.em->eps);
					free(ip->un.em->args);
					free(ip->un.em);
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
void expr_addcall(struct expr *restrict ep,double *dst,double (*func)(double)){
	struct expr_inst *ip;
	EXTEND_DATA
	ip=ep->data+ep->size++;
	ip->op=EXPR_CALL;
	ip->dst=dst;
	ip->un.func=func;
}
__attribute__((noinline))
void expr_addcallmdop(struct expr *restrict ep,double *dst,struct expr_mdinfo *em,unsigned int op){
	struct expr_inst *ip;
	EXTEND_DATA
	ip=ep->data+ep->size++;
	ip->op=op;
	ip->dst=dst;
	ip->un.em=em;
}
__attribute__((noinline))
void expr_addsumop(struct expr *restrict ep,double *dst,struct expr_suminfo *es,unsigned int op){
	struct expr_inst *ip;
	EXTEND_DATA
	ip=ep->data+ep->size++;
	ip->op=op;
	ip->dst=dst;
	ip->un.es=es;
}
__attribute__((noinline))
void expr_addop(struct expr *restrict ep,double *dst,double *src,unsigned int op){
	struct expr_inst *ip;
	EXTEND_DATA
	ip=ep->data+ep->size++;
	ip->op=op;
	ip->dst=dst;
	ip->un.src=src;
	ip->assign_level=0;
	//printval((uint64_t)ip->dst);
	//printval((uint64_t)ip->un.src);
	//printval((uint64_t)ip->op);
//	puts("");
}
static void expr_addassign(struct expr *restrict ep,double *dst){
	struct expr_inst *ip;
	EXTEND_DATA
	ip=ep->data+ep->size++;
	ip->op=EXPR_COPY;
	ip->assign_level=1;
	ip->dst=dst;
//	printval((uint64_t)ip->dst);
//	printval((uint64_t)ip->op);
//	puts("");
}
double *expr_newvar(struct expr *restrict ep){
	if(ep->vsize>=ep->vlength){
		ep->vars=xrealloc(ep->vars,
		(ep->vlength+=16)*sizeof(double));
	}
	return ep->vars+ep->vsize++;
}
void expr_symset_add4(struct expr_symset *restrict ep,const char *sym,void *addr,int type,int dim){
	struct expr_symbol *esp;
	size_t len=strlen(sym);
	if(len>EXPR_SYMLEN)return;
	if(ep->size>=ep->length){
		ep->syms=xrealloc(ep->syms,
		(ep->length+=16)*sizeof(double));
	}
	esp=ep->syms+ep->size++;
	memcpy(esp->str,sym,len);
	if(len<EXPR_SYMLEN)esp->str[len]=0;
	esp->addr=addr;
	esp->type=type;
	esp->dim=dim;
}

void expr_symset_add(struct expr_symset *restrict ep,const char *sym,void *addr,int type){
	expr_symset_add4(ep,sym,addr,type,0);
}
struct expr_symset *expr_symset_clone(struct expr_symset *restrict ep){
	struct expr_symset *es=new_expr_symset();
	if(ep)
	for(size_t i=0;i<ep->size;++i){
		expr_symset_add(es,ep->syms[i].str,
		ep->syms[i].addr,ep->syms[i].type);
	}
	return es;
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
static struct expr_mdinfo *expr_getmdinfo(struct expr *restrict ep,char *e,const char *asym,double (*func)(size_t,double *),unsigned int dim){
	char **v=expr_sep(ep,e);
	char **p;
	size_t i;
	struct expr_mdinfo *em;
	int error;
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

	em->func=func;
	em->eps=xmalloc(em->dim*sizeof(struct expr *));
	em->args=xmalloc(em->dim*sizeof(double));
	for(i=0;i<em->dim;++i){
		em->eps[i]=new_expr(v[i],asym,ep->sset,&error);
		if(!em->eps[i]){
			for(ssize_t k=i-1;k>=0;--k)
				expr_free(em->eps[k]);
			goto err2;
		}
	}
	expr_free2(v);
	return em;
err2:
	free(em->eps);
	free(em->args);
	ep->error=error;
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
	sset=expr_symset_clone(ep->sset);
	expr_symset_add(sset,v[0],&es->index,EXPR_PARAMETER);
	es->from=new_expr(v[1],asym,ep->sset,&error);
	if(!es->from)goto err1;
	es->to=new_expr(v[2],asym,sset,&error);
	if(!es->to)goto err2;
	es->step=new_expr(v[3],asym,sset,&error);
	if(!es->step)goto err3;
	es->ep=new_expr(v[4],asym,sset,&error);
	expr_free2(v);
	expr_symset_free(sset);
	//assert(es->ep);
	if(!es->ep)goto err4;
	//puts("getsi");
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
	ep->error=error;
err0:
	expr_free2(v);
	return NULL;
}
	
static double *expr_scan(struct expr *restrict ep,const char *e,const char *asym);
static double *expr_getval(struct expr *restrict ep,const char *e,const char **_p,const char *asym,unsigned int assign_level){
	const char *p,*p2,*e0=e;
	double (*p1)(double);
	char *buf;
	double *v0=NULL,*v1;
	union {
	double v;
	struct expr_suminfo *es;
	struct expr_mdinfo *em;
	} un;
	int type;
	unsigned int dim;

	//fprintf(stderr,"getval %u: %s\n",assign_level,e0);
	for(;*e;++e){
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
		else if(*e==')'&&!expr_unfindpair(e0,e))goto pterr;
		p=expr_getsym(e);
	//	fprintf(stderr,"unknown sym %ld %s\n",p-e,e);
		if(p==e)goto symerr;
		type=-1;
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
		else KEYWORD_SET("nest",EXPR_NEST);
		if(type!=-1){
			if(*p!='('){
				ep->error=EXPR_EFP;
				//assert(0);
				return NULL;
			}
			e=p;
			p=expr_findpair(e);
			if(!p)goto pterr;
			buf=xmalloc(p-e+2);
			buf[p-e+1]=0;
			memcpy(buf,e,p-e+1);
			un.es=expr_getsuminfo(ep,buf,asym);
			free(buf);
			//assert(es);
			if(!un.es)return NULL;
			v0=expr_newvar(ep);
			expr_addsumop(ep,v0,un.es,type);
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
		p1=expr_sym2addr(ep,e,p-e,&type,&dim);
		//fprintf(stderr,"find sym %ld %s\n",p-e,e);
		if(p1){
			if(type==EXPR_FUNCTION){
				if(*p!='('){
					ep->error=EXPR_EFP;
					//assert(0);
					return NULL;
				}
				v0=expr_getval(ep,p,&e,asym,0);
				//assert(v0);
				if(!v0)return NULL;
				expr_addcall(ep,v0,p1);
			}else if(type==EXPR_PARAMETER){

				v0=expr_newvar(ep);
				expr_addcopy(ep,v0,(double *)p1);
				if(assign_level){
					expr_addassign(ep,(double *)p1);
					assign_level=0;
				}
				e=p;
			}else if(type==EXPR_MDFUNCTION){
				if(*p!='('){
					ep->error=EXPR_EFP;
					//assert(0);
					return NULL;
				}
				e=p;
				p=expr_findpair(e);
				if(!p)goto pterr;
				buf=xmalloc(p-e+2);
				buf[p-e+1]=0;
				memcpy(buf,e,p-e+1);
				un.em=expr_getmdinfo(ep,buf,asym,
					(double (*)(size_t,double *))p1
					,dim);
				free(buf);
				//assert(es);
				if(!un.em)return NULL;
				v0=expr_newvar(ep);
				expr_addcallmd(ep,v0,un.em);
				e=p+1;
			}else goto symerr;
			break;
		}
		if(expr_atod(e,p-e,&un.v)==1){
			v1=expr_newvar(ep);
			v0=expr_newvar(ep);
			expr_addcopy(ep,v0,v1);
			*v1=un.v;
			e=p;
			break;
		}
symerr:
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
	esp->length=esp->size=0;
	esp->freeable=0;
}
struct expr_symset *new_expr_symset(void){
	struct expr_symset *ep=xmalloc(sizeof(struct expr_symset));
	init_expr_symset(ep);
	ep->freeable=1;
	return ep;
}
void expr_symset_free(struct expr_symset *restrict esp){
	if(!esp)return;
	if(esp->syms)free(esp->syms);
	if(esp->freeable)free(esp);
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
	ep->sset=esp;
	ep->error=0;
	ep->freeable=0;
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
struct expr *new_expr(const char *e,const char *asym,struct expr_symset *esp,int *error){

	struct expr *ep=xmalloc(sizeof(struct expr));
	if(init_expr(ep,e,asym,esp)){
		if(error)*error=ep->error;
		free(ep);
		return NULL;
	}
	ep->freeable=1;
	return ep;
}
double expr_compute(struct expr *restrict ep,double input){
	struct expr_inst *ip=ep->data;
	double step,sum,from,to,y;
	int neg,inited=0;
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
#define CALLOGIC(a,b,_s) (fabs(a)>DBL_EPSILON _s fabs(b)>DBL_EPSILON)
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
				from=expr_compute(ip->un.es->from,input);\
				to=expr_compute(ip->un.es->to,input);\
				if(from>to){\
					step=from;\
					from=to;\
					to=step;\
					neg=1;\
				}\
				step=expr_compute(ip->un.es->step,input);\
				if(step<0)step=-step;\
				sum= _init ;\
				for(ip->un.es->index=from;\
					ip->un.es->index<=to;\
					ip->un.es->index+=step){\
					y=expr_compute(ip->un.es->ep,input) ;\
					_do ;\
				}\
				if(neg)sum= _neg ;\
				*ip->dst=sum;\
				break
			CALSUM(EXPR_SUM,sum+=y,0,-sum);
			CALSUM(EXPR_INT,sum+=step*y,0,-sum);
			CALSUM(EXPR_PROD,sum*=y,1,1/sum);
			CALSUM(EXPR_SUP,if(y>sum)sum=y,DBL_MIN,sum);
			CALSUM(EXPR_INF,if(y<sum)sum=y,DBL_MAX,sum);
			CALSUM(EXPR_AND,sum=inited?expr_and2(sum,y):y;inited=1,DBL_MAX,sum);
			CALSUM(EXPR_OR,sum=inited?expr_or2(sum,y):y;inited=1,DBL_MAX,sum);
			CALSUM(EXPR_XOR,sum=inited?expr_xor2(sum,y):y;inited=1,DBL_MAX,sum);
			case EXPR_NEST:
				ip->un.es->index=
				expr_compute(ip->un.es->from,input);
				to=expr_compute(ip->un.es->to,input);
				expr_compute(ip->un.es->step,input);
//				printvald(from);
//				printvald(to);
//				printvald(step);
				if(to<0)to=-to;
				for(;to>DBL_EPSILON;to-=1.0){
					expr_compute(ip->un.es->step,input);
					ip->un.es->index=
					expr_compute(ip->un.es->ep,input);
				}
				*ip->dst=ip->un.es->index;
				break;
			case EXPR_CALLMD:
				for(unsigned int i=0;i<ip->un.em->dim;++i)
					ip->un.em->args[i]=
						expr_compute(
						ip->un.em->eps[i],input);
				
				*ip->dst=ip->un.em->func(ip->un.em->dim,ip->un.em->args);
				break;
			default:
				break;
		}
		++ip;
	}
	return *ip->dst;
}
