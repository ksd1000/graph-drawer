/*******************************************************************************
 *License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>*
 *This is free software: you are free to change and redistribute it.           *
 *******************************************************************************/
#define _GNU_SOURCE
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "expr.h"
#define EXTEND_DATA if(ep->size>=ep->length){\
	ep->data=realloc(ep->data,\
		(ep->length+=16)*sizeof(struct expr_inst));\
	}
static const char *eerror[]={"Unknown error","Unknown symbol","Parentheses do not match","Function must be followed by a \'(\'","No value in parenthesis"};
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
static double (*expr_sym2addr(struct expr *restrict ep,const char *sym,size_t sz,int *type))(double){
	size_t i;
	if(sz>EXPR_SYMLEN)return NULL;
	if(ep->sset)
	for(i=0;i<ep->sset->size;++i){
		if(!strncmp(ep->sset->syms[i].str,sym,sz)){
			if(type)*type=ep->sset->syms[i].type;
			return ep->sset->syms[i].addr;
		}
	}
	for(i=0;syms[i].str;++i){
		if(!strncmp(syms[i].str,sym,sz)){
			if(type)*type=EXPR_FUNCTION;
			return syms[i].addr;
		}
	}
	return NULL;
}
const char *expr_error(int error){
	if(error>4||error<0)return eerror[0];
	else return eerror[error];
}
static void *xmalloc(size_t size){
	void *r=malloc(size);
	assert(r != NULL);
	return r;
}
void expr_free(struct expr *restrict ep){
	if(!ep)return;
	if(ep->data)free(ep->data);
	if(ep->vars)free(ep->vars);
	free(ep);
}
__attribute__((noinline)) void expr_addcall(struct expr *restrict ep,double *dst,double (*func)(double)){
	struct expr_inst *ip;
	EXTEND_DATA
	ip=ep->data+ep->size++;
	ip->op=EXPR_CALL;
	ip->dst=dst;
	ip->un.func=func;
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
		ep->vars=realloc(ep->vars,
		(ep->vlength+=16)*sizeof(double));
	}
	return ep->vars+ep->vsize++;
}
void expr_symset_add(struct expr_symset *restrict ep,const char *sym,void *addr,int type){
	struct expr_symbol *esp;
	size_t len=strlen(sym);
	if(len>EXPR_SYMLEN)return;
	if(ep->size>=ep->length){
		ep->syms=realloc(ep->syms,
		(ep->length+=16)*sizeof(double));
	}
	esp=ep->syms+ep->size++;
	memcpy(esp->str,sym,len);
	if(len<EXPR_SYMLEN)esp->str[len]=0;
	esp->addr=addr;
	esp->type=type;
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
static const char special[]={"+-*/^ ()"};
static const char *expr_getsym(const char *c){
	while(*c&&!strchr(special,*c))
		++c;
	return c;
}
static int expr_atod(const char *str,size_t sz,double *dst){
	int ret;
	char *p=xmalloc(sz+1);
	p[sz]=0;
	memcpy(p,str,sz);
	ret=sscanf(p,"%lf",dst);
	free(p);
	return ret;
}
static double *expr_scan(struct expr *restrict ep,const char *e,const char *asym);
static double *expr_getval(struct expr *restrict ep,const char *e,const char **_p,const char *asym){
	const char *p,*p2,*e0=e;
	double (*p1)(double);
	char *buf;
	double *v0=NULL,*v1;
	double v;
	int type;
	//printf("getval %s\n",e);
	for(;*e;++e){
		if(*e==' ')continue;
		if(*e=='('){
			p=expr_findpair(e);
			if(!p){
pterr:
				ep->error=EXPR_EPT;
				return NULL;
			}
			p2=e;
			while(*p2=='('||*p2==' ')++p2;
			if(*p2==')'){
				ep->error=EXPR_ENVP;
				return NULL;
			}
			buf=xmalloc(p-e);
			buf[p-e-1]=0;
			memcpy(buf,e+1,p-e-1);
			v0=expr_scan(ep,buf,asym);
			free(buf);
			if(!v0)return NULL;
			e=p+1;
			break;
		}else if(*e=='+')continue;
		else if(*e==')'&&!expr_unfindpair(e0,e))goto pterr;
		p=expr_getsym(e);
		if(!memcmp(e,asym,p-e)){
			v0=expr_newvar(ep);
			expr_addcopy(ep,v0,NULL);
			e=p;
			break;
		}
		p1=expr_sym2addr(ep,e,p-e,&type);
		if(p1){
			if(type==EXPR_FUNCTION){
				if(*p!='('){
					ep->error=EXPR_EFP;
					return NULL;
				}
				v0=expr_getval(ep,p,&e,asym);
				if(!v0)return NULL;
				expr_addcall(ep,v0,p1);
			}else if(type==EXPR_PARAMETER){
				v0=expr_newvar(ep);
				expr_addcopy(ep,v0,(double *)p1);
				e=p;
			}else goto symerr;
			break;
		}
		if(expr_atod(e,p-e,&v)==1){
			v1=expr_newvar(ep);
			v0=expr_newvar(ep);
			expr_addcopy(ep,v0,v1);
			*v1=v;
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
	//printf("vp %p\n",vp);
	if(!vp)return expr_vn(v,op);
	for(p=vp;p->next;p=p->next);
//	printf("p %p\n",p);
//	printf("p %p\n",p);
	p->next=xmalloc(sizeof(struct expr_vnode));
//	printf("p->next %p\n",p->next);
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
//	printf("scan %s\n",e);
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
	ep->error=0;
	v1=NULL;
	if(op!=-1){
		v1=expr_getval(ep,e,&e,asym);
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
			default:
				break;
		}
		++ip;
	}
	return *ip->dst;
}
