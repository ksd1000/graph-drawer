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

static const char *eerror[]={"Unknown error","Unknown symbol","Parentheses do not match","Function and keyword must be followed by a \'(\'","No value in parenthesis","No enough argument","Bad number"};
#define REGSYM(s) {#s,s}
static const struct {
	const char *str;
	double (*addr)(double);
} syms[]={
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
	REGSYM(rint),
	REGSYM(round),
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
double expr_max(size_t n,double *args){
	double ret=DBL_MIN;
	while(n>0){
		//printf("%lf\n",*args);
		if(*args>ret)ret=*args;
		--n;
		++args;
	}
	return ret;
}
double expr_min(size_t n,double *args){
	double ret=DBL_MAX;
	while(n>0){
		//printf("%lf\n",*args);
		if(*args<ret)ret=*args;
		--n;
		++args;
	}
	return ret;
}
double expr_hypot(size_t n,double *args){
	double ret=0;
	while(n>0){
		//printf("%lf\n",*args);
		ret+=*args**args;
		--n;
		++args;
	}
	return sqrt(ret);
}
static const struct {
	const char *str;
	double (*addr)(size_t,double *);
} mdsyms[]={
	{"min",expr_min},
	{"max",expr_max},
	{"hypot",expr_hypot},
	{NULL,NULL}
};
static double (*expr_sym2addr(struct expr *restrict ep,const char *sym,size_t sz,int *type))(double){
	size_t i;
	if(sz>=EXPR_SYMLEN)return NULL;
	if(ep->sset)
	for(i=0;i<ep->sset->size;++i){
		if(sz==strlen(ep->sset->syms[i].str)&&!memcmp(ep->sset->syms[i].str,sym,sz)){
			if(type)*type=ep->sset->syms[i].type;
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
			return (double (*)(double))mdsyms[i].addr;
		}
	}
	return NULL;
}
const char *expr_error(int error){
	if(error>6||error<0)return eerror[0];
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
		while(ip->op!=EXPR_END){
			switch(ip->op){
				case EXPR_SUM:
				case EXPR_INT:
				case EXPR_PROD:
					expr_free(ip->un.es->ep);
					expr_free(ip->un.es->from);
					expr_free(ip->un.es->to);
					expr_free(ip->un.es->step);
					expr_symset_free(ip->un.es->sset);
					free(ip->un.es);
					break;
				case EXPR_CALLMD:
					for(size_t i=0;i<ip->un.em->dim;++i)
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
	free(ep);
}
#define EXTEND_DATA if(ep->size>=ep->length){\
	ep->data=xrealloc(ep->data,\
		(ep->length+=16)*sizeof(struct expr_inst));\
	}
__attribute__((noinline)) void expr_addcall(struct expr *restrict ep,double *dst,double (*func)(double)){
	struct expr_inst *ip;
	EXTEND_DATA
	ip=ep->data+ep->size++;
	ip->op=EXPR_CALL;
	ip->dst=dst;
	ip->un.func=func;
}
__attribute__((noinline)) void expr_addcallmd(struct expr *restrict ep,double *dst,struct expr_mdinfo *em){
	struct expr_inst *ip;
	EXTEND_DATA
	ip=ep->data+ep->size++;
	ip->op=EXPR_CALLMD;
	ip->dst=dst;
	ip->un.em=em;
}
__attribute__((noinline)) void expr_addsumop(struct expr *restrict ep,double *dst,struct expr_suminfo *es,unsigned int op){
	struct expr_inst *ip;
	EXTEND_DATA
	ip=ep->data+ep->size++;
	ip->op=op;
	ip->dst=dst;
	ip->un.es=es;
}
__attribute__((noinline)) void expr_addop(struct expr *restrict ep,double *dst,double *src,unsigned int op){
	struct expr_inst *ip;
	EXTEND_DATA
	ip=ep->data+ep->size++;
	ip->op=op;
	ip->dst=dst;
	ip->un.src=src;
}
static double *expr_newvar(struct expr *restrict ep){
	if(ep->vsize>=ep->vlength){
		ep->vars=xrealloc(ep->vars,
		(ep->vlength+=16)*sizeof(double));
	}
	return ep->vars+ep->vsize++;
}
void expr_symset_add(struct expr_symset *restrict ep,const char *sym,void *addr,int type){
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
static const char special[]={"+-*/^ (),"};
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

static char **expr_sep(struct expr *restrict ep,char *e){
	char *p,*p1,*p2,**p3=NULL;
	size_t len=0,s;
	for(;;){
		while(*e==' ')++e;
		if(*e!='(')break;
		p1=(char *)expr_findpair(e);
		if(p1){
			p2=p1+1;
			while(*p2==' ')++p2;

			if(!*p2){
				*p1=0;
				++e;
			}else break;
		}else {
			ep->error=EXPR_EPT;
			return NULL;
		}
	}

	while(*e==' ')++e;
	if(!*e){
		ep->error=EXPR_ENVP;
		return NULL;
	}
	for(p=strtok_r(e,",",&p2);p;p=strtok_r(NULL,",",&p2)){
		p1=xmalloc((s=strlen(p))+1);
		p1[s]=0;
		memcpy(p1,p,s);
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
static struct expr_mdinfo *expr_getmdinfo(struct expr *restrict ep,char *e,const char *asym,double (*func)(size_t,double *)){
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
	em=xmalloc(sizeof(struct expr_mdinfo));
	em->dim=p-v;
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
	expr_free2(v);
	return NULL;
}
static struct expr_suminfo *expr_getsuminfo(struct expr *restrict ep,char *e,const char *asym){
	char **v=expr_sep(ep,e);
	char **p;
	struct expr_suminfo *es;
	int error;
	if(!v){
		return NULL;
	}
	p=v;
	while(*p){
		//puts(*p);
		++p;
	}
	if(p-v<5){
		ep->error=EXPR_ENEA;
		goto err0;
	}
	es=xmalloc(sizeof(struct expr_suminfo));
//	sum(sym_index,from,to,step,expression)
	es->sset=expr_symset_clone(ep->sset);
	expr_symset_add(es->sset,v[0],&es->index,EXPR_PARAMETER);
	/*if(expr_atod2(v[1],&es->from)<1||expr_atod2(v[2],&es->to)<1||expr_atod2(v[3],&es->step)<1){
		expr_free2(v);
		ep->error=EXPR_ENUMBER;
		return NULL;
	}*/
	es->from=new_expr(v[1],asym,ep->sset,&error);
	if(!es->from)goto err1;
	es->to=new_expr(v[2],asym,ep->sset,&error);
	if(!es->to)goto err2;
	es->step=new_expr(v[3],asym,ep->sset,&error);
	if(!es->step)goto err3;
	es->ep=new_expr(v[4],asym,es->sset,&error);
	//assert(es->ep);
	if(!es->ep)goto err4;
	expr_free2(v);
	//puts("getsi");
	return es;
err4:
	expr_symset_free(es->sset);
	free(es);
	expr_free2(v);
err3:
	expr_free(es->step);
err2:
	expr_free(es->to);
err1:
	expr_free(es->from);
	ep->error=error;
err0:
	expr_free2(v);
	return NULL;
}
	
static double *expr_scan(struct expr *restrict ep,const char *e,const char *asym);
static double *expr_getval(struct expr *restrict ep,const char *e,const char **_p,const char *asym){
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
	//fprintf(stderr,"getval %s\n",e0);
	for(;*e;++e){
		if(*e==' ')continue;
		if(*e=='('){
			p=expr_findpair(e);
			if(!p){
pterr:
				ep->error=EXPR_EPT;
				//assert(0);
				return NULL;
			}
			p2=e;
			while(*p2=='('||*p2==' ')++p2;
			if(*p2==')'){
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
		//fprintf(stderr,"unknown sym %ld %s\n",p-e,e);
		type=-1;
		if(p-e==3&&!memcmp(e,"sum",p-e))type=EXPR_SUM;
		else if(p-e==3&&!memcmp(e,"int",p-e))type=EXPR_INT;
		else if(
			(p-e==3&&!memcmp(e,"pai",p-e))||
			(p-e==4&&!memcmp(e,"prod",p-e))
			)	type=EXPR_PROD;
		//if(type==-1&&(!strncmp(e,"prod",p-e)||!strncmp(e,"pai",p-e)))type=EXPR_PROD;
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
		p1=expr_sym2addr(ep,e,p-e,&type);
		//fprintf(stderr,"find sym %ld %s\n",p-e,e);
		if(p1){
			if(type==EXPR_FUNCTION){
				if(*p!='('){
					ep->error=EXPR_EFP;
					//assert(0);
					return NULL;
				}
				v0=expr_getval(ep,p,&e,asym);
				//assert(v0);
				if(!v0)return NULL;
				expr_addcall(ep,v0,p1);
			}else if(type==EXPR_PARAMETER){
				v0=expr_newvar(ep);
				expr_addcopy(ep,v0,(double *)p1);
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
					(double (*)(size_t,double *))p1);
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
//	fprintf(stderr,"p %p\n",p);
//	fprintf(stderr,"p %p\n",p);
	p->next=xmalloc(sizeof(struct expr_vnode));
//	fprintf(stderr,"p->next %p\n",p->next);
	p->next->v=v;
	p->next->op=op;
	p->next->next=NULL;
	return vp;
}
static void expr_vnunion(struct expr *restrict ep,struct expr_vnode *ev){
	struct expr_vnode *p;
	expr_addop(ep,ev->v,ev->next->v,ev->next->op);
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
	double *v0=NULL,*v1;
	const char *e0=e;
	int op,neg=0;
	struct expr_vnode *ev=NULL,*p;
	//fprintf(stderr,"scan %s\n",e);
	if(*e=='-'){
		neg=1;
		++e;
	}
	v0=expr_getval(ep,e,&e,asym);
	if(!v0)goto err;
	ev=expr_vn(v0,0);
more:
	op=-1;
	for(;*e;++e){
		switch(*e){
			case '+':
				op=EXPR_ADD;
				++e;
				goto end1;
			case '-':
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
			case '^':
				op=EXPR_POW;
				++e;
				goto end1;
			case ' ':
				continue;
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
	v1=NULL;
	if(op!=-1){
		v1=expr_getval(ep,e,&e,asym);
		//assert(!ep->error);
		if(ep->error)goto err;
		if(v1){
			expr_vnadd(ev,v1,op);
			goto more;
		}
	}
	for(p=ev;p;p=p->next){
		while(p->next&&p->next->op==EXPR_POW){
			expr_vnunion(ep,p);
		}
	}
	for(p=ev;p;p=p->next){
		while(p->next&&(p->next->op==EXPR_MUL||p->next->op==EXPR_DIV)){
			expr_vnunion(ep,p);
		}
	}
	if(neg)expr_addneg(ep,v0);
	while(ev->next)
		expr_vnunion(ep,ev);
	expr_vnfree(ev);
	return v0;
err:
	expr_vnfree(ev);
	return NULL;
}
struct expr_symset *new_expr_symset(void){
	struct expr_symset *ep=xmalloc(sizeof(struct expr_symset));
	ep->syms=NULL;
	ep->length=ep->size=0;
	return ep;
}
void expr_symset_free(struct expr_symset *restrict esp){
	if(!esp)return;
	if(esp->syms)free(esp->syms);
	free(esp);
}
struct expr *new_expr(const char *e,const char *asym,struct expr_symset *esp,int *error){
	double *p;
	struct expr *ep=xmalloc(sizeof(struct expr));
	ep->data=NULL;
	ep->vars=NULL;
	ep->sset=esp;
	ep->error=0;
	ep->length=ep->size=ep->vlength=ep->vsize=0;
	if(e&&*e){
		p=expr_scan(ep,e,asym);
		if(p)
			expr_addend(ep,p);
		else {
			if(error)
				*error=ep->error;
			expr_free(ep);
			return NULL;
		}
	}
	return ep;
}
double expr_compute(struct expr *restrict ep,double input){
	struct expr_inst *ip=ep->data;
	double step,sum,from,to;
	int neg;
	while(ip->op!=EXPR_END){
		switch(ip->op){
			case EXPR_COPY:
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
			case EXPR_POW:
				*ip->dst=pow(*ip->dst,*ip->un.src);
				break;
			case EXPR_NEG:
				*ip->dst=-*ip->dst;
				break;
			case EXPR_SUM:
				from=expr_compute(ip->un.es->from,input);
				to=expr_compute(ip->un.es->to,input);
				if(from>to){
					step=from;
					from=to;
					to=step;
				}
				step=expr_compute(ip->un.es->step,input);
				if(step<0)step=-step;
				sum=0;
				for(ip->un.es->index=from;
					ip->un.es->index<=to;
					ip->un.es->index+=step){
					sum+=expr_compute(ip->un.es->ep,input);
				}
				*ip->dst=sum;
				break;
			case EXPR_PROD:
				from=expr_compute(ip->un.es->from,input);
				to=expr_compute(ip->un.es->to,input);
				if(from>to){
					step=from;
					from=to;
					to=step;
				}
				step=expr_compute(ip->un.es->step,input);
				if(step<0)step=-step;
				sum=1;
				for(ip->un.es->index=from;
					ip->un.es->index<=to;
					ip->un.es->index+=step){
					sum*=expr_compute(ip->un.es->ep,input);
				}
				*ip->dst=sum;
				break;
			case EXPR_INT:
				neg=0;
				from=expr_compute(ip->un.es->from,input);
				to=expr_compute(ip->un.es->to,input);
				if(from>to){
					step=from;
					from=to;
					to=step;
					neg=1;
				}
				step=expr_compute(ip->un.es->step,input);
				if(step<0)step=-step;
				sum=0;
				for(ip->un.es->index=from+step/2;
					ip->un.es->index<=to;
					ip->un.es->index+=step){
					sum+=step*expr_compute(ip->un.es->ep,
						input);
				}
				*ip->dst=neg?-sum:sum;
				break;
			case EXPR_CALLMD:
				for(size_t i=0;i<ip->un.em->dim;++i)
					ip->un.em->args[i]=
						expr_compute(
						ip->un.em->eps[i],input);
				
				*ip->dst=ip->un.em->func(ip->un.em->dim,ip->un.em->args);
			default:
				break;
		}
		++ip;
	}
	return *ip->dst;
}
