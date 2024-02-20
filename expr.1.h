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
EXPR_CALLMD,
EXPR_END
};
#define EXPR_SYMLEN 256
#define EXPR_ESYMBOL 1
#define EXPR_EPT 2
#define EXPR_EFP 3
#define EXPR_ENVP 4
#define EXPR_ENEA 5
#define EXPR_ENUMBER 6

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
	struct expr_symset *sset;
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
	unsigned int op;
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
};
struct expr {
	double *vars;;
	struct expr_inst *data;
	struct expr_symset *sset;
	size_t size,length,vsize,vlength;
	int error;
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

