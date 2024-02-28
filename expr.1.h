/*******************************************************************************
 *License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>*
 *This is free software: you are free to change and redistribute it.           *
 *******************************************************************************/
#ifndef _EXPR_H_
#define _EXPR_H_
enum {
EXPR_COPY=0,
EXPR_CONST,
EXPR_CALL,
EXPR_ADD,
EXPR_SUB,
EXPR_MUL,
EXPR_DIV,
EXPR_MOD,
EXPR_POW,
EXPR_NEG,
EXPR_IF,
EXPR_WHILE,
EXPR_SUM,
EXPR_INT,
EXPR_PROD,
EXPR_SUP,
EXPR_INF,
EXPR_AND,
EXPR_OR,
EXPR_XOR,
EXPR_GCD,
EXPR_LCM,
EXPR_NEST,
EXPR_LOOP,
EXPR_FOR,
EXPR_CALLMD,
EXPR_CALLMDEP,
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
#define EXPR_EEV 8

#define EXPR_CONSTANT 0
#define EXPR_VARIABLE 1
#define EXPR_FUNCTION 2
#define EXPR_MDFUNCTION 3
#define EXPR_MDEPFUNCTION 4
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
	unsigned int dim,unused;
};
union expr_inst_op2{
	volatile double *src;
	double value;
	double (*func)(double);
	struct expr_suminfo *es;
	struct expr_branchinfo *eb;
	struct expr_mdinfo *em;
};
struct expr_inst {
	double *dst;
	union expr_inst_op2 un;
	unsigned int op,assign_level;
};
union expr_symbol_value {
	double value;
	volatile double *addr;
	double (*func)(double);
	double (*mdfunc)(size_t,double *);
	double (*mdepfunc)(size_t,
		const struct expr *,double);
};
struct expr_symbol {
	union expr_symbol_value un;
	struct expr_symbol *next;
	int type;
	unsigned int dim;
	char str[EXPR_SYMLEN];
};
struct expr_builtin_symbol {
	union expr_symbol_value un;
	const char *str;
	int type;
	unsigned int dim;
};
struct expr_symset {
	struct expr_symbol *syms;
	//size_t size,length;
	int freeable;
};
struct expr {
	double *vars;
	double (*imm)(double);
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
extern const struct expr_builtin_symbol expr_bsyms[];
