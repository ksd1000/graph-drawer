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
#ifndef _SSIZE_T_DEFINED_
#define _SSIZE_T_DEFINED_
typedef ptrdiff_t ssize_t;
#ifndef SSIZE_MAX
#define SSIZE_MAX PTRDIFF_MAX
#endif
#endif
#endif

enum expr_op :int {
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
EXPR_DO1,
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
EXPR_IP,
EXPR_IPP,
EXPR_TO,
EXPR_TO1,
EXPR_END
};

#define EXPR_VOID ((void *)-1UL)
#define EXPR_VOID_NR ((void *)-2UL)

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
#define EXPR_EVD 18
#define EXPR_EPM 19
#define EXPR_EIN 20
#define EXPR_EBS 21
#define EXPR_EVZP 22

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
#define EXPR_SF_UNSAFE 16

#define EXPR_SF_PEP 12
#define EXPR_SF_PMASK (~12)
//expr initial flag
#define EXPR_IF_NOOPTIMIZE 1
#define EXPR_IF_INSTANT_FREE 2

#define EXPR_IF_NOBUILTIN 4
#define EXPR_IF_NOKEYWORD 8
#define EXPR_IF_PROTECT 16
#define EXPR_IF_INJECTION_B 32
#define EXPR_IF_INJECTION_S 64
#define EXPR_IF_KEEPSYMSET 128
#define EXPR_IF_DETACHSYMSET 256

#define EXPR_IF_EXTEND_MASK (\
		EXPR_IF_INSTANT_FREE\
		)
#define EXPR_IF_INJECTION (EXPR_IF_INJECTION_B|EXPR_IF_INJECTION_S)
#define EXPR_IF_SETABLE (EXPR_IF_INJECTION|EXPR_IF_NOBUILTIN|EXPR_IF_NOKEYWORD|EXPR_IF_PROTECT)

//expr keyword flag
#define EXPR_KF_SUBEXPR 1
#define EXPR_KF_SEPCOMMA 2
#define EXPR_KF_NOPROTECT 4

#define EXPR_EDBASE(d) (((union expr_double *)(d))->rd.base)
#define EXPR_EDEXP(d) (((union expr_double *)(d))->rd.exp)
#define EXPR_EDSIGN(d) (((union expr_double *)(d))->rd.sign)
#define EXPR_EDIVAL(d) (((union expr_double *)(d))->ival)

#define EXPR_RPUSH(_sp) (*((_sp)++))
#define EXPR_RPOP(_sp) (*(--(_sp)))

#define EXPR_PUSH(_sp) (*(--(_sp)))
#define EXPR_POP(_sp) (*((_sp)++))

#define expr_cast(x,type) \
	({\
		union {\
			__typeof(x) _x;\
			__typeof(type) _o;\
		} _un;\
		_un._x=(x);\
		_un._o;\
	})
#define EXPR_SYMSET_DEPTHUNIT (2*sizeof(void *))

#define expr_symset_foreach(_sp,_esp,_stack) \
	for(struct expr_symbol *_sp=(_esp)->syms;_sp;_sp=NULL)for(struct {struct expr_symbol **sp;void *stack;unsigned int index;int end;} __inloop={(_stack),__inloop.sp,0,0,};!({for(;;){\
		if(__inloop.index>=EXPR_SYMNEXT){\
			if(__inloop.sp==__inloop.stack){\
				__inloop.end=1;\
				break;\
			}\
			_sp=EXPR_RPOP(__inloop.sp);\
			__inloop.index=(unsigned int)(size_t)EXPR_RPOP(__inloop.sp);\
			continue;\
		}\
		break;\
	}\
	__inloop.end;\
	});({for(;__inloop.index<EXPR_SYMNEXT;){\
		if(!_sp->next[__inloop.index]){\
			++__inloop.index;\
			continue;\
		}\
		EXPR_RPUSH(__inloop.sp)=(void *)(size_t)(__inloop.index+1);\
		EXPR_RPUSH(__inloop.sp)=_sp;\
		_sp=_sp->next[__inloop.index];\
		__inloop.index=0;\
		break;\
	}\
	}))if(__inloop.index)continue;else

struct expr_libinfo {
	const char *version;
	const char *compiler_version;
	const char *date;
	const char *time;
};
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
		void *uaddr;
	} un;
	size_t dim;
};
struct expr_vmdinfo {
	struct expr *fromep,*toep,*stepep,*ep;
	size_t max;
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
		void **uaddr2;
		struct expr_inst **instaddr2;
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
		size_t zu;
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
	size_t size,length,depth;
	unsigned int freeable,unused;
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

struct expr_callback {
	void (*before)(const struct expr *restrict ep,struct expr_inst *ip,void *arg);
	void (*after)(const struct expr *restrict ep,struct expr_inst *ip,void *arg);
	void *arg;
};

extern const struct expr_builtin_symbol expr_symbols[];
extern const struct expr_builtin_keyword expr_keywords[];
extern const struct expr_libinfo expr_libinfo[1];

extern void *(*expr_allocator)(size_t);
extern void *(*expr_reallocator)(void *,size_t);
extern void (*expr_deallocator)(void *);
extern void (*expr_contractor)(void *,size_t);
extern size_t expr_allocate_max;
//default=malloc,realloc,free,expr_contract,0x10000000000UL
extern const size_t expr_page_size;

long expr_syscall(long arg0,long arg1,long arg2,long arg3,long arg4,long arg5,long num);
