/*******************************************************************************
 *License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>*
 *This is free software: you are free to change and redistribute it.           *
 *******************************************************************************/
#ifndef _EXPR_H_
#define _EXPR_H_
#include <stdint.h>
#include <stddef.h>
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
EXPR_ZA,
EXPR_MD,
EXPR_ME,
EXPR_VMD,
EXPR_HOT,
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
#ifndef EXPR_SYMNEXT
#define EXPR_SYMNEXT 14
#endif
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
#define EXPR_EVMD 12

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
	struct expr *from,*to,*step,*ep;
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
struct expr_vmdinfo {
	struct expr *from,*to,*step,*ep,*max;
	double (*func)(size_t,double *);
	volatile double index;
};
union expr_inst_op2{
	double *src;
	void *uaddr;
	double value;
	struct expr *hotfunc;
	double (*func)(double);
	double (*zafunc)(void);
	struct expr_suminfo *es;
	struct expr_branchinfo *eb;
	struct expr_mdinfo *em;
	struct expr_vmdinfo *ev;
};
struct expr_inst {
	double *dst;
	union expr_inst_op2 un;
	enum expr_op op;
	int flag;
};
union expr_symvalue {
	double value;
	long ivalue;
	unsigned long uvalue;
	double *addr;
	void *uaddr;
	double (*func)(double);
	double (*zafunc)(void);
	char *hotexpr;
	double (*mdfunc)(size_t,double *);
	double (*mdepfunc)(size_t,
		const struct expr *,double);
};
//_Static_assert(sizeof(union expr_symvalue)==8,"symbol_value size error");
struct expr_symbol {
	union expr_symvalue un;
	struct expr_symbol *next[EXPR_SYMNEXT];
	unsigned int length;
	unsigned short strlen;
	char type,flag;
	char str[];
}/* __attribute__((packed))*/;
//_Static_assert(sizeof(struct expr_symbol)-EXPR_SYMNEXT*sizeof(struct expr_symbol *)==16,"symbol size error");
struct expr_builtin_symbol {
	union expr_symvalue un;
	const char *str;
	unsigned short strlen;
	short type,flag,dim;
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
	size_t size,depth,length;
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
