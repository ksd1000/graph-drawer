/*******************************************************************************
 *License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>*
 *This is free software: you are free to change and redistribute it.           *
 *******************************************************************************/
#ifndef _EXPR_H_
#define _EXPR_H_
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#ifdef __unix__
#include <sys/types.h>
#else
#ifndef EXPR_HIDE_SSIZE_WARNING
#warning "__unix__ is not defined. define ssize_t as ptrdiff_t."
#endif
typedef ptrdiff_t ssize_t;
#endif
enum expr_op {
EXPR_COPY=0,
EXPR_INPUT,
EXPR_CONST,
EXPR_BL,
EXPR_ADD,
EXPR_SUB,
EXPR_MUL,
EXPR_DIV,
EXPR_MOD,
EXPR_POW,
EXPR_AND,
EXPR_XOR,
EXPR_OR,
EXPR_SHL,
EXPR_SHR,
EXPR_GT,
EXPR_GE,
EXPR_LT,
EXPR_LE,
EXPR_SLE,
EXPR_SGE,
EXPR_SEQ,
EXPR_SNE,
EXPR_EQ,
EXPR_NE,
EXPR_ANDL,
EXPR_ORL,
EXPR_XORL,
EXPR_NEXT,
EXPR_DIFF,
EXPR_NEG,
EXPR_NOT,
EXPR_NOTL,
EXPR_TSTL,
EXPR_IF,
EXPR_WHILE,
EXPR_DO,
EXPR_DOW,
EXPR_WIF,
EXPR_DON,
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
EXPR_MEP,
EXPR_VMD,
EXPR_EP,
EXPR_EVAL,
EXPR_HOT,
EXPR_PBL,
EXPR_PZA,
EXPR_PMD,
EXPR_PME,
EXPR_PMEP,
EXPR_READ,
EXPR_WRITE,
EXPR_OFF,
EXPR_ALO,
EXPR_SJ,
EXPR_LJ,
EXPR_END
};
#define EXPR_SYMSET_INITIALIZER {NULL,0UL,0UL,0UL,0}
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
#define EXPR_EMEM 13
#define EXPR_EUSN 14
#define EXPR_ENC 15
#define EXPR_ECTA 16
#define EXPR_ESAF 17

#define EXPR_CONSTANT 0
#define EXPR_VARIABLE 1
#define EXPR_FUNCTION 2
#define EXPR_MDFUNCTION 3
#define EXPR_MDEPFUNCTION 4
#define EXPR_HOTFUNCTION 5
#define EXPR_ZAFUNCTION 6
//expr symbol flag
#define EXPR_SF_INJECTION 1
#define EXPR_SF_WRITEIP 2
#define EXPR_SF_PMD 4
#define EXPR_SF_PME 8
#define EXPR_SF_PEP 12
#define EXPR_SF_PMASK (~12)
//expr initial flag
#define EXPR_IF_NOOPTIMIZE 1
#define EXPR_IF_INSTANT_FREE 2

#define EXPR_IF_EXTEND_MASK (\
		EXPR_IF_INSTANT_FREE\
		)
//expr keyword flag
#define EXPR_KF_SUBEXPR 1
#define EXPR_KF_SEPCOMMA 2
#define EXPR_EDBASE(d) (((union expr_double *)(d))->rd.base)
#define EXPR_EDEXP(d) (((union expr_double *)(d))->rd.exp)
#define EXPR_EDSIGN(d) (((union expr_double *)(d))->rd.sign)
#define EXPR_EDIVAL(d) (((union expr_double *)(d))->ival)
#define expr_cast(x,type) \
	({\
		union {\
			__typeof(x) _x;\
			__typeof(type) _o;\
		} _un;\
		_un._x=(x);\
		_un._o;\
	})
struct expr;
struct expr_symset;
struct expr_suminfo {
	struct expr *fromep,*toep,*stepep,*ep;
	volatile double index;
};
struct expr_branchinfo {
	struct expr *cond,*body,*value;
};
struct expr_mdinfo {
	struct expr *eps;
	double *args;
	const char *e;
	union {
		double (*func)(size_t,double *);
		double (*funcep)(size_t,
			const struct expr *,double);
	} un;
	size_t dim;
};
struct expr_vmdinfo {
	struct expr *fromep,*toep,*stepep,*ep;
	ssize_t max;
	double (*func)(size_t,double *);
	double *args;
	volatile double index;
};
struct expr_rawdouble {
	uint64_t base:52;
	uint64_t exp:11;
	uint64_t sign:1;
} __attribute__((packed));
struct expr_inst {
	union {
		double *dst;
		int64_t *idst;
		void *uaddr;
		struct expr_rawdouble *rdst;
		double (**md2)(size_t,double *);
		double (**me2)(size_t,
			const struct expr *,double);
	} dst;
	union {
		double *src;
		int64_t *isrc;
		double **src2;
		void *uaddr;
		double value;
		ssize_t zd;
		struct expr *hotfunc;
		struct expr **hotfunc2;
		double (*func)(double);
		double (**func2)(double);
		double (*zafunc)(void);
		double (**zafunc2)(void);
		struct expr_suminfo *es;
		struct expr_branchinfo *eb;
		struct expr_mdinfo *em;
		struct expr_vmdinfo *ev;
	} un;
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
struct expr_symbol {
	union expr_symvalue un;
	struct expr_symbol *next[EXPR_SYMNEXT];
	unsigned int length;
	unsigned short strlen;
	unsigned char type,flag;
	char str[];
};
struct expr_builtin_symbol {
	union expr_symvalue un;
	const char *str;
	unsigned short strlen;
	short type,flag,dim;
};
struct expr_builtin_keyword {
	const char *str;
	enum expr_op op;
	unsigned short flag;
	unsigned short strlen;
	const char *desc;
};
struct expr_symset {
	struct expr_symbol *syms;
	size_t size,depth,length;
	int freeable;
};
struct expr_resource {
	struct expr_resource *next;
	union {
		void *uaddr;
		double *addr;
		char *str;
		struct expr *ep;
	} un;
	int type;
};
struct expr {
	struct expr_inst *data;
	struct expr_inst *ip;
	struct expr *parent;
	double **vars;
	struct expr_symset *sset;
	struct expr_resource *res,*tail;
	size_t size,length,vsize,vlength;
	int error;
	short iflag;
	unsigned char freeable,sset_shouldfree;
	char errinfo[EXPR_SYMLEN];
};

union expr_double {
	double val;
	int64_t ival;
	uint64_t uval;
	struct expr_rawdouble rd;
};
extern const struct expr_builtin_symbol expr_symbols[];
extern const struct expr_builtin_keyword expr_keywords[];
