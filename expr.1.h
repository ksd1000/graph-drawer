/*******************************************************************************
 *License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>*
 *This is free software: you are free to change and redistribute it.           *
 *******************************************************************************/
#ifndef _EXPR_H_
#define _EXPR_H_

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <limits.h>

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
#define EXPR_EANT 23

#define EXPR_CONSTANT 0
#define EXPR_VARIABLE 1
#define EXPR_FUNCTION 2
#define EXPR_MDFUNCTION 3
#define EXPR_MDEPFUNCTION 4
#define EXPR_ZAFUNCTION 5
#define EXPR_HOTFUNCTION 6
#define EXPR_ALIAS 7

//expr symbol flag
#define EXPR_SF_INJECTION 1
#define EXPR_SF_WRITEIP 2
#define EXPR_SF_PMD 4
#define EXPR_SF_PME 8
#define EXPR_SF_UNSAFE 16
#define EXPR_SF_ALLOWADDR 32

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
//R:reverse
#define EXPR_PUSH(_sp) (*(--(_sp)))
#define EXPR_POP(_sp) (*((_sp)++))

#define expr_cast(x,type) \
	({\
		union {\
			__typeof(x) _x;\
			__typeof(type) _o;\
		} _cast_un;\
		_cast_un._x=(x);\
		_cast_un._o;\
	})

#define EXPR_ALIGN (sizeof(void))
#define EXPR_MAGIC48_A 0x5deece66dul
#define EXPR_MAGIC48_B 0xb

#define expr_ltod48(_l) (expr_cast((((_l)&0xffffffffffffl)<<4)|0x3ff0000000000000l,double)-1.0)
#define expr_ltol48(_l) ((int32_t)(((_l)&0xffffffff0000l)>>17))
#define expr_ltom48(_l) ((int32_t)(((_l)&0xffffffff0000l)>>16))

#define expr_next48v(_val) (((EXPR_MAGIC48_A)*(_val)+(EXPR_MAGIC48_B))&0xffffffffffffl)
#define expr_next48(_seedp) ({\
	long *restrict _next48_seed=(_seedp);\
	*_next48_seed=expr_next48v(*_next48_seed);\
})
#define expr_seed48(__val) (0x330e|(((__val)&0xffffffffl)<<16))
#define expr_next48l32(_seedp) (expr_next48(_seedp)&0xffffffffl)

#define expr_get48(_addr) ({\
	const uint16_t *_get48_addr=(_addr);\
	(long)*(uint32_t *)_get48_addr|((long)_get48_addr[2]<<32l);\
})
#define expr_set48(_addr,_val) ({\
	uint16_t *_set48_addr=(_addr);\
	long _set48_val=(_val);\
	*(uint32_t *)_set48_addr=(uint32_t)_set48_val;\
	_set48_addr[2]=(uint16_t)(_set48_val>>32l);\
})

#define expr_ssnext48(_ss) ({\
	struct expr_superseed48 *_ssnext48_ss=(_ss);\
	uint16_t *_ssnext48_end,*_ssnext48_p;\
	long _ssnext48_val,_ssnext48_r,_ssnext48_n=1;\
	_ssnext48_p=_ssnext48_ss->data;\
	_ssnext48_end=_ssnext48_p+3*_ssnext48_ss->len;\
	_ssnext48_r=expr_next48v(expr_get48(_ssnext48_p));\
	_ssnext48_val=_ssnext48_r;\
	expr_set48(_ssnext48_p,_ssnext48_r);\
	_ssnext48_p+=3;\
	while(expr_likely(_ssnext48_p<_ssnext48_end)){\
		_ssnext48_r=expr_next48v(expr_get48(_ssnext48_p));\
		_ssnext48_val+=_ssnext48_r*_ssnext48_n;\
		_ssnext48_n+=2;\
		expr_set48(_ssnext48_p,_ssnext48_r);\
		_ssnext48_p+=3;\
	}\
	_ssnext48_val&0xffffffffffffl;\
})

#define expr_ssgetnext48(_ss) ({\
	const struct expr_superseed48 *_ssgetnext48_ss=(_ss);\
	const uint16_t *_ssgetnext48_end,*_ssgetnext48_p;\
	long _ssgetnext48_val,_ssgetnext48_r,_ssgetnext48_n=1;\
	_ssgetnext48_p=_ssgetnext48_ss->data;\
	_ssgetnext48_end=_ssgetnext48_p+3*_ssgetnext48_ss->len;\
	_ssgetnext48_r=expr_next48v(expr_get48(_ssgetnext48_p));\
	_ssgetnext48_val=_ssgetnext48_r;\
	_ssgetnext48_p+=3;\
	while(expr_likely(_ssgetnext48_p<_ssgetnext48_end)){\
		_ssgetnext48_r=expr_next48v(expr_get48(_ssgetnext48_p));\
		_ssgetnext48_val+=_ssgetnext48_r*_ssgetnext48_n;\
		_ssgetnext48_n+=2;\
		_ssgetnext48_p+=3;\
	}\
	_ssgetnext48_val&0xffffffffffffl;\
})
#define expr_symset_hot(_esp) ({const struct expr_symbol *restrict __esp=(_esp);(char *)(__esp->str+__esp->strlen+1);})
#define expr_symset_hotlen(_esp) ({const struct expr_symbol *restrict __esp=(_esp);(size_t)__esp->length-(size_t)__esp->strlen-sizeof(struct expr_symbol)-2;})
#define expr_symset_un(_esp) ({const struct expr_symbol *restrict __esp=(_esp);(union expr_symvalue *)(__esp->str+__esp->strlen+1);})
#define expr_symset_dim(_esp) ({const struct expr_symbol *restrict __esp=(_esp);(size_t *)(__esp->str+__esp->strlen+1+sizeof(union expr_symvalue));})
#define expr_assume(cond) if(cond);else __builtin_unreachable()
#define expr_likely(cond) __builtin_expect(!!(cond),1)
#define expr_unlikely(cond) __builtin_expect(!!(cond),0)
#define expr_static_castable(value,_type) __builtin_constant_p(((void (*)(_type))NULL)(value))
#define EXPR_SYMSET_DEPTHUNIT (2*sizeof(void *))

#define expr_symbol_foreach4(_sp,_esp,_stack,_atindex) \
	for(unsigned int __inloop_end=0,__inloop_index=0;({\
		_Static_assert(__builtin_constant_p((_atindex)),"_atindex should be constant");\
		_Static_assert(__builtin_constant_p((_atindex)<EXPR_SYMNEXT),"_atindex should be constant");\
		_Static_assert(__builtin_constant_p((_atindex)>=EXPR_SYMNEXT),"_atindex should be constant");\
		expr_static_castable((_stack),void *);\
		expr_static_castable((_esp),const struct expr_symbol *);\
		!__inloop_end;\
	});)\
	for(struct expr_symbol *_sp=(struct expr_symbol *)(_esp);!__inloop_end;)\
	if(expr_unlikely(!_sp)){__inloop_end=1;break;}else\
	for(struct expr_symbol **__inloop_stack=(struct expr_symbol **)(_stack),**__inloop_sp=__inloop_stack;expr_likely(!({for(;;){\
		if((_atindex)<EXPR_SYMNEXT){\
			if(expr_unlikely(__inloop_index>=EXPR_SYMNEXT)){\
				if(expr_unlikely(__inloop_sp==__inloop_stack)){\
					__inloop_end=1;\
					break;\
				}\
				_sp=EXPR_RPOP(__inloop_sp);\
				__inloop_index=(unsigned int)(size_t)EXPR_RPOP(__inloop_sp);\
				continue;\
			}\
		}\
		break;\
	}\
	__inloop_end;\
	}));({for(;;){\
		if((_atindex)>=EXPR_SYMNEXT){\
			if(expr_unlikely(__inloop_index>=EXPR_SYMNEXT)){\
				if(expr_unlikely(__inloop_sp==__inloop_stack)){\
					__inloop_end=1;\
					break;\
				}\
				_sp=EXPR_RPOP(__inloop_sp);\
				__inloop_index=(unsigned int)(size_t)EXPR_RPOP(__inloop_sp);\
				break;\
			}\
		}\
		if(!_sp->next[__inloop_index]){\
			++__inloop_index;\
		}else {\
			EXPR_RPUSH(__inloop_sp)=(void *)(size_t)(__inloop_index+1);\
			EXPR_RPUSH(__inloop_sp)=_sp;\
			_sp=_sp->next[__inloop_index];\
			__inloop_index=0;\
		}\
		break;\
	}}))if(expr_likely(__inloop_index!=(_atindex)))continue;else

#define expr_symset_foreach4(_sp,_esp,_stack,_atindex) expr_symbol_foreach4(_sp,(_esp)->syms,_stack,_atindex)

#define expr_symset_foreach(_sp,_esp,_stack) expr_symset_foreach4(_sp,_esp,_stack,0)

#define expr_breakforeach ({__inloop_end=1;break;})

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
		double (*func)(double *,size_t);
		double (*funcep)(const struct expr *,size_t,double);
		void *uaddr;
	} un;
	size_t dim;
};
struct expr_vmdinfo {
	struct expr *fromep,*toep,*stepep,*ep;
	size_t max;
	double (*func)(double *,size_t);
	double *args;
	volatile double index;
};
struct expr_superseed48 {
	size_t len;
	uint16_t data[];//size=3*len
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
		double (**md2)(double *,size_t);
		double (**me2)(const struct expr *,size_t,double);
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
	size_t size;
	ssize_t off;
	double *addr;
	void *uaddr;
	double (*func)(double);
	double (*zafunc)(void);
	double (*mdfunc)(double *,size_t);
	double (*mdepfunc)(const struct expr *,size_t,double);
};
_Static_assert(EXPR_SYMLEN<=64,"EXPR_SYMLEN is more than 6 bits");

struct expr_symbol {
	struct expr_symbol *next[EXPR_SYMNEXT];
	struct expr_symbol **tail;
	uint32_t length;
	uint32_t strlen:6,type:3,flag:6,saved:1,depthm1:16;
	char str[];
}__attribute__((packed));
struct expr_symbol_infile {
	uint32_t length;
	uint16_t strlen:6,type:3,flag:6,unused:1;
	char str[];
}__attribute__((packed));
#define EXPR_SYMBOL_EXTRA (sizeof(struct expr_symbol)-sizeof(struct expr_symbol_infile))
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
	size_t size;
	size_t size_m;//amount of all symbols with removed
	size_t removed;//amount of removed symbols starting from the
		       //latest expr_symset_vaddl(),expr_symset_addcopy(),
		       //expr_symset_insert() or expr_symset_remove()
		       //that increases this->depth.
		       //expr_symset_wipe(this) will set it to 0.
	size_t removed_m;//amount of all removed symbols
	size_t length;
	size_t length_m;//m:monotonic
	size_t alength;
	size_t alength_m;
	size_t depth;
	size_t depth_n;//amount of symbol at the deepest depth,
		       //expr_symset_remove() may reduce this
		       //value after removing a such symbol.
		       //this value reaches 0 means that the
		       //real depth of this symbol set is lower
		       //than this->depth.
	size_t depth_nm;//like depth_n but monotonic
	//the this->depth is the maximal depth this reaches,it equals to the
	//real depth if no symbol was removed by expr_symset_remove(this,.),
	//one can use the expr_symset_depth() to get the real depth and run
	//expr_symset_correct(this) to correct it,which can save stack's
	//memory to prevent the signal SIGSEGV or SIGKILL(by OOM) if there
	//are symbols be removed frequently or you can add it to the source
	//code of expr_symset_remove() to ensure this->depth equals to the
	//real depth. but it is not suggested,for it will cost a lot of cpu
	//time to travel through every symbol to get the real depth.
	//this will be set to 0 when an expr_symset_wipe(this) is called.
	unsigned int freeable,unused;
};
struct expr_symset_infile {
	size_t size;
	uint32_t maxlen;
	char data[];
}__attribute__((packed));
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
	char extra_data[];
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
extern int expr_symset_allow_heap_stack;
extern long expr_seed_default;
//default=malloc,realloc,free,expr_contract,0x1000000000UL,NULL
extern const size_t expr_page_size;

long expr_syscall(long arg0,long arg1,long arg2,long arg3,long arg4,long arg5,long num);
