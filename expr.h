/*******************************************************************************
 *License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>*
 *This is free software: you are free to change and redistribute it.           *
 *******************************************************************************/
#ifndef _EXPR_H_
#define _EXPR_H_
enum {
EXPR_COPY=0,
EXPR_CALL,
EXPR_ADD,
EXPR_SUB,
EXPR_MUL,
EXPR_DIV,
EXPR_MOD,
EXPR_POW,
EXPR_NEG,
EXPR_SUM,
EXPR_INT,
EXPR_PROD,
EXPR_SUP,
EXPR_INF,
EXPR_AND,
EXPR_OR,
EXPR_XOR,
EXPR_NEST,
EXPR_CALLMD,
EXPR_GT,
EXPR_GE,
EXPR_LT,
EXPR_LE,
EXPR_EQ,
EXPR_NE,
EXPR_ANDL,
EXPR_ORL,
EXPR_XORL,
EXPR_ASSIGN,
EXPR_END
};
#define EXPR_SYMLEN 256
#define EXPR_ESYMBOL 1
#define EXPR_EPT 2
#define EXPR_EFP 3
#define EXPR_ENVP 4
#define EXPR_ENEA 5
#define EXPR_ENUMBER 6
#define EXPR_ETNV 7

#define EXPR_FUNCTION 0
#define EXPR_PARAMETER 1
#define EXPR_MDFUNCTION 2
#define EXPR_EDBASE(d) (((union expr_double *)(d))->rd.base)
#define EXPR_EDEXP(d) (((union expr_double *)(d))->rd.exp)
#define EXPR_EDSIGN(d) (((union expr_double *)(d))->rd.sign)
struct expr;
struct expr_symset;
struct expr_suminfo {
	struct expr *ep,*from,*to,*step;
	double index;
};
struct expr_mdinfo {
	struct expr **eps;
	double *args;
	double (*func)(size_t,double *);
	unsigned int dim,unused;
};
struct expr_inst {
	double *dst;
	union {
		volatile double *src;
		double (*func)(double);
		struct expr_suminfo *es;
		struct expr_mdinfo *em;
	} un;
	unsigned int op,assign_level;
};
struct expr_symbol {
	void *addr;
	int type;
	unsigned int dim;
	char str[EXPR_SYMLEN];
};
struct expr_symset {
	struct expr_symbol *syms;
	size_t size,length;
	int freeable;
};
struct expr {
	double *vars;;
	struct expr_inst *data;
	struct expr_symset *sset;
	size_t size,length,vsize,vlength;
	int error,freeable;
};
struct expr_rawdouble {
	uint64_t base:52;
	uint64_t exp:11;
	uint64_t sign:1;
} __attribute__((packed));
union expr_double {
	double val;
	uint64_t ival;
	struct expr_rawdouble rd;
};

uint64_t expr_gcd64(uint64_t x,uint64_t y);
void expr_memrand(void *restrict m,size_t n);
double expr_and2(double a,double b);
double expr_or2(double a,double b);
double expr_xor2(double a,double b);
double expr_gcd2(double x,double y);
double expr_lcm2(double x,double y);
double expr_rand12(void);
const char *expr_error(int error);
void expr_free(struct expr *restrict ep);
void expr_addcall(struct expr *restrict ep,double *dst,double (*func)(double));
void expr_addcallmdop(struct expr *restrict ep,double *dst,struct expr_mdinfo *em,unsigned int op);
void expr_addsumop(struct expr *restrict ep,double *dst,struct expr_suminfo *es,unsigned int op);
void expr_addop(struct expr *restrict ep,double *dst,double *src,unsigned int op);
double *expr_newvar(struct expr *restrict ep);
void expr_symset_add4(struct expr_symset *restrict ep,const char *sym,void *addr,int type,int dim);
void expr_symset_add(struct expr_symset *restrict ep,const char *sym,void *addr,int type);
struct expr_symset *expr_symset_clone(struct expr_symset *restrict ep);
void init_expr_symset(struct expr_symset *restrict esp);
struct expr_symset *new_expr_symset(void);
void expr_symset_free(struct expr_symset *restrict esp);
int init_expr(struct expr *restrict ep,const char *e,const char *asym,struct expr_symset *esp);
struct expr *new_expr(const char *e,const char *asym,struct expr_symset *esp,int *error);
double expr_compute(struct expr *restrict ep,double input);

#define expr_addcopy(e,t,f) expr_addop(e,t,f,EXPR_COPY)
#define expr_addadd(e,t,f) expr_addop(e,t,f,EXPR_ADD)
#define expr_addsub(e,t,f) expr_addop(e,t,f,EXPR_SUB)
#define expr_addmul(e,t,f) expr_addop(e,t,f,EXPR_MUL)
#define expr_adddiv(e,t,f) expr_addop(e,t,f,EXPR_DIV)
#define expr_addpow(e,t,f) expr_addop(e,t,f,EXPR_POW)
#define expr_addneg(e,t) expr_addop(e,t,NULL,EXPR_NEG)
#define expr_addsum(e,t,es) expr_addsumop(e,t,es,EXPR_SUM)
#define expr_addint(e,t,es) expr_addsumop(e,t,es,EXPR_INT)
#define expr_addend(e,t) expr_addop(e,t,NULL,EXPR_END)
#define expr_addcallmd(e,t,em) expr_addcallmdop(e,t,em,EXPR_CALLMD)
#endif
