/*******************************************************************************
 *License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>*
 *This is free software: you are free to change and redistribute it.           *
 *******************************************************************************/
#ifndef _EXPR_H_
#define _EXPR_H_
#include <stdint.h>
enum expr_op {
EXPR_COPY=0,
EXPR_INPUT,
EXPR_CONST,
EXPR_CALL,
EXPR_ADD,
EXPR_SUB,
EXPR_MUL,
EXPR_DIV,
EXPR_MOD,
EXPR_POW,
EXPR_AND,
EXPR_OR,
EXPR_XOR,
EXPR_SHL,
EXPR_SHR,
EXPR_NEG,
EXPR_NOT,
EXPR_NOTL,
EXPR_IF,
EXPR_WHILE,
EXPR_SUM,
EXPR_INT,
EXPR_PROD,
EXPR_SUP,
EXPR_INF,
EXPR_ANDN,
EXPR_ORN,
EXPR_XORN,
EXPR_GCDN,
EXPR_LCMN,
EXPR_LOOP,
EXPR_FOR,
EXPR_CALLZA,
EXPR_CALLMD,
EXPR_CALLME,
EXPR_CALLHOT,
EXPR_GT,
EXPR_GE,
EXPR_LT,
EXPR_LE,
EXPR_SEQ,
EXPR_EQ,
EXPR_NE,
EXPR_ANDL,
EXPR_ORL,
EXPR_XORL,
EXPR_END
};
#define EXPR_SYMLEN 64

#define EXPR_ESYMBOL 1
#define EXPR_EPT 2
#define EXPR_EFP 3
#define EXPR_ENVP 4
#define EXPR_ENEA 5
#define EXPR_ENUMBER 6
#define EXPR_ETNV 7
#define EXPR_EEV 8
#define EXPR_EUO 9
#define EXPR_EZAFP 10
#define EXPR_EDS 11

#define EXPR_CONSTANT 0
#define EXPR_VARIABLE 1
#define EXPR_FUNCTION 2
#define EXPR_MDFUNCTION 3
#define EXPR_MDEPFUNCTION 4
#define EXPR_HOTFUNCTION 5
#define EXPR_ZAFUNCTION 6

#define EXPR_SF_INJECTION 1

#define EXPR_EDBASE(d) (((union expr_double *)(d))->rd.base)
#define EXPR_EDEXP(d) (((union expr_double *)(d))->rd.exp)
#define EXPR_EDSIGN(d) (((union expr_double *)(d))->rd.sign)
struct expr;
struct expr_symset;
struct expr_suminfo {
	struct expr *ep,*from,*to,*step;
	volatile double index;
};
struct expr_branchinfo {
	struct expr *cond,*body,*value;
};
struct expr_mdinfo {
	struct expr *eps;
	double *args;
	union {
		double (*func)(size_t,double *);
		double (*funcep)(size_t,
			const struct expr *,double);
	} un;
	size_t dim;
};
union expr_inst_op2{
	double *src;
	double value;
	struct expr *hotfunc;
	double (*func)(double);
	double (*zafunc)(void);
	struct expr_suminfo *es;
	struct expr_branchinfo *eb;
	struct expr_mdinfo *em;
};
struct expr_inst {
	double *dst;
	union expr_inst_op2 un;
	enum expr_op op;
	//unsigned int assign_level;
};
union expr_symbol_value {
	double value;
	double *addr;
	void *uaddr;
	double (*func)(double);
	double (*zafunc)(void);
	struct {
		char *expr;
		char *asym;
	} hot;
	struct {
		double (*func)(size_t,double *);
		size_t dim;
	} md;
	struct {
		double (*func)(size_t,
			const struct expr *,double);
		size_t dim;
	} mdep;
};
struct expr_symbol {
	union expr_symbol_value un;
	struct expr_symbol *next;
	unsigned int length,strlen;
	int type,flag;
	char str[EXPR_SYMLEN];
	char data[];
};
struct expr_builtin_symbol {
	union expr_symbol_value un;
	const char *str;
	unsigned short strlen;
	short type,flag,unused;
};
struct expr_builtin_keyword {
	const char *str;
	enum expr_op op;
	unsigned short dim;
	unsigned short strlen;
	const char *desc;
};
struct expr_symset {
	struct expr_symbol *syms;
	struct expr_symbol *tail;
	//size_t size,length;
	int freeable;
};
struct expr {
	double **vars;
	struct expr_inst *data;
	struct expr_symset *sset;
	size_t size,length,vsize,vlength;
	int error;
	short freeable,sset_shouldfree;
	char errinfo[EXPR_SYMLEN];
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
extern const struct expr_builtin_symbol expr_bsyms[];
extern const struct expr_builtin_keyword expr_keywords[];
const char *expr_error(int error);
uint64_t expr_gcd64(uint64_t x,uint64_t y);
void expr_memrand(void *restrict m,size_t n);
double expr_and2(double a,double b);
double expr_or2(double a,double b);
double expr_xor2(double a,double b);
double expr_gcd2(double x,double y);
double expr_lcm2(double x,double y);
double expr_multilevel_derivate(const struct expr *ep,double input,long level,double epsilon);
const struct expr_builtin_symbol *expr_bsym_search(const char *sym,size_t sz);
const struct expr_builtin_symbol *expr_bsym_rsearch(void *addr);
size_t expr_strscan(const char *s,size_t sz,char *buf);
char *expr_astrscan(const char *s,size_t sz,size_t *outsz);
void expr_free(struct expr *restrict ep);
struct expr_inst *expr_addop(struct expr *restrict ep,double *dst,void *src,enum expr_op op);
void init_expr_symset(struct expr_symset *restrict esp);
struct expr_symset *new_expr_symset(void);
void expr_symset_free(struct expr_symset *restrict esp);
struct expr_symbol *expr_symset_findtail(struct expr_symset *restrict esp);
struct expr_symbol *expr_symset_add(struct expr_symset *restrict esp,const char *sym,int type,...);
struct expr_symbol *expr_symset_vadd(struct expr_symset *restrict esp,const char *sym,int type,va_list ap);
const struct expr_symbol *expr_symset_search(const struct expr_symset *restrict esp,const char *sym,size_t sz);
const struct expr_symbol *expr_symset_rsearch(const struct expr_symset *restrict esp,void *addr);
void expr_symset_copy(struct expr_symset *restrict dst,const struct expr_symset *restrict src);
struct expr_symset *expr_symset_clone(const struct expr_symset *restrict ep);
int init_expr_old(struct expr *restrict ep,const char *e,const char *asym,struct expr_symset *esp);
int init_expr(struct expr *restrict ep,const char *e,const char *asym,struct expr_symset *esp);
struct expr *new_expr(const char *e,const char *asym,struct expr_symset *esp,int *error,char errinfo[EXPR_SYMLEN]);
double expr_eval(const struct expr *restrict ep,double input);

#define expr_addcopy(e,t,f) expr_addop(e,t,f,EXPR_COPY)
#define expr_addcall(e,t,f) expr_addop(e,t,f,EXPR_CALL)
#define expr_addadd(e,t,f) expr_addop(e,t,f,EXPR_ADD)
#define expr_addsub(e,t,f) expr_addop(e,t,f,EXPR_SUB)
#define expr_addmul(e,t,f) expr_addop(e,t,f,EXPR_MUL)
#define expr_adddiv(e,t,f) expr_addop(e,t,f,EXPR_DIV)
#define expr_addpow(e,t,f) expr_addop(e,t,f,EXPR_POW)
#define expr_addneg(e,t) expr_addop(e,t,NULL,EXPR_NEG)
#define expr_addnot(e,t) expr_addop(e,t,NULL,EXPR_NOT)
#define expr_addnotl(e,t) expr_addop(e,t,NULL,EXPR_NOTL)
#define expr_addinput(e,t) expr_addop(e,t,NULL,EXPR_INPUT)
//#define expr_addsum(e,t,es) expr_addop(e,t,es,EXPR_SUM)
#define expr_addend(e,t) expr_addop(e,t,NULL,EXPR_END)
#define expr_addcallza(e,t,em) expr_addop(e,t,em,EXPR_CALLZA)
#define expr_addcallmd(e,t,em) expr_addop(e,t,em,EXPR_CALLMD)
#define expr_addcallmdep(e,t,em) expr_addop(e,t,em,EXPR_CALLME)
#define expr_addcallhot(e,t,em) expr_addop(e,t,em,EXPR_CALLHOT)
//#define expr_addconst(e,t,v) expr_addop(e,t,v,EXPR_CONST)
//#define expr_addconst(e,t,v) expr_addop(e,t,*(void **)(v),EXPR_CONST)
#define expr_addconst(e,t,v) (expr_addop(e,t,NULL,EXPR_CONST)->un.value=(v))
#define expr_compute expr_eval
#define expr_evaluate expr_eval
#define expr_calculate expr_eval
#endif
