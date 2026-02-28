/*******************************************************************************
 *License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>*
 *This is free software: you are free to change and redistribute it.           *
 *******************************************************************************/
#ifndef _EXPR_H_
#define _EXPR_H_

#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <limits.h>
#include <setjmp.h>

#if defined(_EXPR_LIB)&&(_EXPR_LIB)
#include <stdlib.h>
#define expr_globals \
void *(*expr_allocator)(size_t)=malloc;\
void *(*expr_reallocator)(void *,size_t)=realloc;\
void (*expr_deallocator)(void *)=free;\
size_t expr_allocate_max=SSIZE_MAX;\
size_t expr_bufsize_initial=512

#ifndef EXPR_DEBUG
#define EXPR_DEBUG 0
#endif

#ifdef __clang__
#pragma GCC diagnostic ignored "-Wunknown-warning-option"
#endif

#if (EXPR_DEBUG)
#define debug(fmt,...) ((void)fprintf(stderr,"[DEBUG]%s:%d: " fmt "\n",__func__,__LINE__,##__VA_ARGS__))
#else
#define debug(fmt,...) ((void)0)
#endif

#define warn(fmt,...) fprintf(stderr,fmt "\n",##__VA_ARGS__)

#pragma GCC diagnostic ignored "-Wzero-length-bounds"
#pragma GCC diagnostic ignored "-Wstrict-aliasing"

#define addo(V,A) __builtin_add_overflow((V),(A),&(V))
#define mulo(V,A) __builtin_mul_overflow((V),(A),&(V))

#define likely(cond) expr_likely(cond)
#define unlikely(cond) expr_unlikely(cond)
#define arrsize(arr) (sizeof(arr)/sizeof(*arr))

#define align(x) (((x)+(EXPR_ALIGN-1))&~(EXPR_ALIGN-1))

#define bufsize_initial expr_bufsize_initial

#define xmalloc expr_xmalloc
#define xrealloc expr_xrealloc
#define xfree expr_xfree

#ifndef alloca
#define alloca(size) __builtin_alloca(size)
#endif

#if (defined(__LONG_WIDTH__)&&(__LONG_WIDTH__==64))
#define ctz64 __builtin_ctzl
#define clz64 __builtin_clzl
#elif ((defined(__LLONG_WIDTH__)&&(__LLONG_WIDTH__==64))||(defined(__LONG_LONG_WIDTH__)&&(__LONG_LONG_WIDTH__==64)))
#define ctz64 __builtin_ctzll
#define clz64 __builtin_clzll
#elif (defined(__INT_WIDTH__)&&(__INT_WIDTH__==64))
#define ctz64 __builtin_ctz
#define clz64 __builtin_clz
#else
#define ctz64 __builtin_ctzg
#define clz64 __builtin_clzg
#endif

#define le16(x) expr_le(x,16)
#define le32(x) expr_le(x,32)
#define le64(x) expr_le(x,64)

#endif

#ifndef _SSIZE_T_DEFINED_
#define _SSIZE_T_DEFINED_
typedef ptrdiff_t ssize_t;
#endif

#ifndef SSIZE_MAX
#define SSIZE_MAX PTRDIFF_MAX
#endif

#define expr_static_assert(cond) _Static_assert((cond),"static assertion " #cond " failed")
expr_static_assert(sizeof(ssize_t)==sizeof(ptrdiff_t));
expr_static_assert(sizeof(size_t)==sizeof(ptrdiff_t));
expr_static_assert(sizeof(void *)==sizeof(ptrdiff_t));
expr_static_assert(sizeof(void *)>=sizeof(double));

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
EXPR_NEX0,
EXPR_DIF0,
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
EXPR_TO,
EXPR_TO1,
EXPR_HMD,
EXPR_RET,
EXPR_SVC,
EXPR_SVC0,
EXPR_SVCP,
EXPR_SVCP0,
EXPR_END
};

#define EXPR_VOID ((void *)-1)
#define EXPR_VOID_NR ((void *)-2)

#define EXPR_SYMSET_INITIALIZER {NULL,0,0,0,0}
#define EXPR_MUTEX_INITIALIZER UINT32_C(0]

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
#define EXPR_EUDE 24

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
//a non-hot function has the INJECTION flag means the output value of it
//depends only by the input value,and no any side effect.for HOTFUNCTION
//and ALIAS it means a size argument is following the char * argument.
#define EXPR_SF_WRITEIP 2
//for MDEPFUNCTION,if it is called,the ->ip first-argument expression will
//be set to current ip.
#define EXPR_SF_PMD 4
//for CONSTANT and VARIABLE,means the value of it is an address of a
//multi-dimension function.
#define EXPR_SF_PME 8
//for CONSTANT and VARIABLE,means the value of it is an address of a
//multi-dimension function with expression argument.
#define EXPR_SF_UNSAFE 16
//a non-hot function has the flag means it may allow user to read/write
//the memory freely or make a system call.they are disabled in protected
//mode.
#define EXPR_SF_ALLOWADDR 32
//only for a unsafe function,it means it is able and only able to accept
//a address of a VARIABLE as its argument in protected mode.

#define EXPR_SF_PFUNC 12
#define EXPR_SF_PMASK (~12)
//expr initial flag
#define EXPR_IF_NOOPTIMIZE 1
#define EXPR_IF_INSTANT_FREE 2

#define EXPR_IF_INJECTION 4
#define EXPR_IF_NOKEYWORD 8
#define EXPR_IF_PROTECT 16
#define EXPR_IF_KEEPSYMSET 128
#define EXPR_IF_DETACHSYMSET 256
#define EXPR_IF_UNSAFE 512

#define EXPR_IF_EXTEND_MASK (\
		EXPR_IF_INSTANT_FREE\
		)
#define EXPR_IF_SETABLE (EXPR_IF_INJECTION|EXPR_IF_NOKEYWORD|EXPR_IF_PROTECT)

//expr keyword flag
#define EXPR_KF_SUBEXPR 1
#define EXPR_KF_SEPCOMMA 2
#define EXPR_KF_NOPROTECT 4

#define EXPR_EDBASE(d) (((union expr_double *)(d))->rd.base)
#define EXPR_EDEXP(d) (((union expr_double *)(d))->rd.exp)
#define EXPR_EDSIGN(d) (((union expr_double *)(d))->rd.sign)
#define EXPR_EDIVAL(d) (((union expr_double *)(d))->ival)
#define EXPR_EDUVAL(d) (((union expr_double *)(d))->uval)

#define EXPR_RPUSH(_sp) (*((_sp)++))
#define EXPR_RPOP(_sp) (*(--(_sp)))
//R:reverse
#define EXPR_PUSH(_sp) (*(--(_sp)))
#define EXPR_POP(_sp) (*((_sp)++))

#define expr_bswap(x,N) __builtin_bswap##N((uint##N##_t)(x))

#define expr_cast(x,type) \
	({\
		union {\
			__typeof(x) _x;\
			__typeof(type) _o;\
			intmax_t _im;\
		} register _cast_un;\
		if(sizeof(type)>sizeof(__typeof(x)))\
	 		_cast_un._im=0;\
		_cast_un._x=(x);\
		_cast_un._o;\
	})

#define expr_isfinite(x) ({\
	union expr_double __r;\
	__r.val=(x);\
	__r.rd.exp!=2047;\
})
#define expr_isinf(x) ({\
	union expr_double __r;\
	__r.val=(x);\
	!__r.rd.base&&__r.rd.exp==2047;\
})
#define expr_isnan(x) ({\
	union expr_double __r;\
	__r.val=(x);\
	__r.rd.base&&__r.rd.exp==2047;\
})

#define expr_xmalloc(size) ({\
	size_t __sz=(size);\
	unlikely(__sz>expr_allocate_max)?\
		NULL:\
		expr_allocator(__sz);\
})
#define expr_xrealloc(old,size) ({\
	void *__old=(old);\
	size_t __sz=(size);\
	unlikely(__sz>expr_allocate_max)?\
		NULL:\
		(__old?\
			 expr_reallocator(__old,__sz):\
			 expr_allocator(__sz));\
})
#define expr_xfree(old) ({\
	void *__old=(old);\
	if(unlikely(!__old))\
		__builtin_trap();\
	expr_deallocator(__old);\
})

#define EXPR_ALIGN (sizeof(void *))
#define EXPR_MAGIC48_A 0x5deece66dul
#define EXPR_MAGIC48_B 0xb

#define expr_ltod48(_l) (expr_cast((((_l)&UINT64_C(0xffffffffffff))<<4)|UINT64_C(0x3ff0000000000000),double)-1.0)
#define expr_ltol48(_l) ((int32_t)(((_l)&UINT64_C(0xffffffff0000))>>17))
#define expr_ltom48(_l) ((int32_t)(((_l)&UINT64_C(0xffffffff0000))>>16))

#define expr_next48v(_val) (((EXPR_MAGIC48_A)*(_val)+(EXPR_MAGIC48_B))&UINT64_C(0xffffffffffff))
#define expr_next48(_seedp) ({\
	int64_t *_next48_seed=(_seedp);\
	*_next48_seed=expr_next48v(*_next48_seed);\
})
#define expr_seed48(__val) (0x330e|(((__val)&UINT64_C(0xffffffff))<<16))
#define expr_next48l32(_seedp) (expr_next48(_seedp)&UINT64_C(0xffffffff))

#define expr_get48(_addr) ({\
	const uint16_t *_get48_addr=(_addr);\
	(int64_t)*(uint32_t *)_get48_addr|((int64_t)_get48_addr[2]<<32);\
})
#define expr_set48(_addr,_val) ({\
	uint16_t *_set48_addr=(_addr);\
	int64_t _set48_val=(_val);\
	*(uint32_t *)_set48_addr=(uint32_t)_set48_val;\
	_set48_addr[2]=(uint16_t)(_set48_val>>32);\
})

#define expr_ssnext48(_ss,_sz) ({\
	expr_superseed48 *__ss=(_ss);\
	expr_superseed48 *__end=__ss+(_sz);\
	int64_t __val,__r,__n=1;\
	__val=expr_next48v(expr_get48(*__ss));\
	expr_set48(*__ss,__val);\
	++__ss;\
	while(__ss<__end){\
		__r=expr_next48v(expr_get48(*__ss));\
		__val+=__r*__n;\
		__n+=2;\
		expr_set48(*__ss,__r);\
		++__ss;\
	}\
	__val&UINT64_C(0xffffffffffff);\
})

#define expr_ssgetnext48(_ss,_sz) ({\
	const expr_superseed48 *__ss=(_ss);\
	const expr_superseed48 *__end=__ss+(_sz);\
	int64_t __val,__r,__n=1;\
	__val=expr_next48v(expr_get48(*__ss));\
	++__ss;\
	while(__ss<__end){\
		__r=expr_next48v(expr_get48(*__ss));\
		__val+=__r*__n;\
		__n+=2;\
		++__ss;\
	}\
	__val&UINT64_C(0xffffffffffff);\
})

#define expr_symbol_hot(_esp) ({const struct expr_symbol *restrict __esp=(_esp);(char *)(__esp->str+__esp->strlen+1);})
#define expr_symbol_hotlen(_esp) ({const struct expr_symbol *restrict __esp=(_esp);(size_t)__esp->length-(size_t)__esp->strlen-sizeof(struct expr_symbol)-2;})
#define expr_symbol_un(_esp) ({const struct expr_symbol *restrict __esp=(_esp);(union expr_symvalue *)(__esp->str+__esp->strlen+1);})
#define expr_symbol_dim(_esp) ({const struct expr_symbol *restrict __esp=(_esp);(size_t *)(__esp->str+__esp->strlen+1+sizeof(union expr_symvalue));})
#define expr_assume(cond) if(cond);else __builtin_unreachable()
#define expr_likely(cond) __builtin_expect(!!(cond),1)
#define expr_unlikely(cond) __builtin_expect(!!(cond),0)
#define expr_static_castable(value,_type) ((void)__builtin_constant_p(((int (*)(_type))NULL)(value)))
#define EXPR_SYMSET_DEPTHUNIT (2*sizeof(void *))

#define expr_symbol_foreach5(_sp,_esp,_stack,_atindex,_label) \
	for(unsigned int __inloop_index=0;;({goto expr_combine(expr_symset_foreach_label_,_label);}))if(0){\
		expr_static_assert(__builtin_constant_p((_atindex)));\
		expr_static_assert(__builtin_constant_p((_atindex)<EXPR_SYMNEXT));\
		expr_static_assert(__builtin_constant_p((_atindex)>=EXPR_SYMNEXT));\
		expr_static_castable((_stack),void *);\
		expr_static_castable((_esp),const struct expr_symbol *);\
	expr_combine(expr_symset_foreach_label_,_label):break;}else\
	for(struct expr_symbol *_sp=(struct expr_symbol *)(_esp);expr_likely(_sp);({goto expr_combine(expr_symset_foreach_label_,_label);}))\
	for(struct expr_symbol **__inloop_stack=(struct expr_symbol **)(_stack),**__inloop_sp=__inloop_stack;;({\
		if(expr_unlikely(__inloop_index>=EXPR_SYMNEXT)){\
			if(expr_unlikely(__inloop_sp==__inloop_stack)){\
				goto expr_combine(expr_symset_foreach_label_,_label);\
			}\
			_sp=EXPR_RPOP(__inloop_sp);\
			__inloop_index=(unsigned int)(uintptr_t)EXPR_RPOP(__inloop_sp);\
		}else if(!_sp->next[__inloop_index]){\
			++__inloop_index;\
		}else {\
			EXPR_RPUSH(__inloop_sp)=(void *)(uintptr_t)(__inloop_index+1);\
			EXPR_RPUSH(__inloop_sp)=_sp;\
			_sp=_sp->next[__inloop_index];\
			__inloop_index=0;\
		}\
	}))if(expr_likely(__inloop_index!=(_atindex)))continue;else

#define expr_combine0(A,B) A##B
#define expr_combine(A,B) expr_combine0(A,B)
#define expr_mstring(X) #X
#define expr_symbol_foreach4(_sp,_esp,_stack,_atindex) expr_symbol_foreach5(_sp,_esp,_stack,_atindex,__LINE__)
#define expr_symset_foreach4(_sp,_esp,_stack,_atindex) expr_symbol_foreach4(_sp,(_esp)->syms,_stack,_atindex)

#define expr_symset_foreach(_sp,_esp,_stack) expr_symset_foreach4(_sp,_esp,_stack,0)

#define expr_fake_memrchr(buf,c,size) ({\
	const void *__buf=(buf);\
	const char *__end=__buf+(size);\
	void *__out=NULL;\
	char __c=(char)(c);\
	while(--__end>(const char *)__buf){\
		if(expr_unlikely(*__end==__c)){\
			__out=(void *)__end;\
			break;\
		}\
	}\
	__out;\
})
#define expr_fake_memrmem(buf,size,c,c_size) ({\
	const void *___buf=(buf);\
	const void *___c=(c);\
	size_t ___size=(size);\
	size_t ___c_size=(c_size);\
	const char *___end=___buf+___size;\
	const char *___r;\
	char ___ch=*(const char *)___c;\
	if(expr_likely(___size)){\
		do {\
			___r=memrchr(___buf,___ch,___size);\
			if(!___r||(___end-___r)<___c_size){\
				___r=NULL;\
				break;\
			}\
			if(!memcmp(___r,___c,___c_size))\
				break;\
			___size=___r-(const char *)___buf;\
		}while(expr_likely(___size));\
	}else\
		___r=NULL;\
	(void *)___r;\
})

struct expr_libinfo {
	const char *version;
	const char *compiler_version;
	const char *platform;
	const char *date;
	const char *time;
	const char *license;
};
#define EXPR_FLAGTYPE_ADDR 0
#define EXPR_FLAGTYPE_SIGNED 1
#define EXPR_FLAGTYPE_UNSIGNED 2
#define EXPR_FLAGTYPE_DOUBLE 3
struct expr_writeflag {
	size_t width;
	ssize_t digit;
	uint64_t bit[0];
#if (!defined(__BIG_ENDIAN__)||!(__BIG_ENDIAN__))
	uint64_t unused:35,
		 op:8,
		 argsize:8,
		 type:2,
		 addr:1,
		 width_set:1,
		 digit_set:1,
		 saved:1,
		 cap:1,
		 eq:1,
		 sharp:1,
		 minus:1,
		 zero:1,
		 space:1,
		 plus:1;
#else
	uint64_t plus:1,
		 space:1,
		 zero:1,
		 minus:1,
		 sharp:1,
		 eq:1,
		 cap:1,
		 saved:1,
		 digit_set:1,
		 width_set:1,
		 addr:1,
		 type:2,
		 argsize:8,
		 op:8,
		 unused:35;
#endif
};
#define EXPR_FMTC_EXIT 0
#define EXPR_FMTC_WRITESIZE 255
#define EXPR_FMTC_WRITESIZEDF 254
#define EXPR_FMTC_JUMP_INDEX 253
#define EXPR_FMTC_LOOP 252
#define EXPR_FMTC_LOOP_FORWARD 251
#define EXPR_FMTC_DELIM 250
#define EXPR_FMTC_TAIL 249
#define EXPR_FMTC_SETARR 248
#define EXPR_FMTC_EXITIF 247
#define EXPR_FMTC_READ_FMTC 246
#define EXPR_FMTC_JUMP 245
#define EXPR_FMTC_READ_CONVERTER 244
#define EXPR_FMTC_INC 243
#define EXPR_FMTC_DEC 242
#define EXPR_FMTC_PUSH 241
#define EXPR_FMTC_CLEAR 240
#define EXPR_FMTC_SAVE 239
#define EXPR_FMTC_EVAL 238
#define EXPR_FMTC_FLUSH 237

#define EXPR_FMTC_MAX 255
#define EXPR_FMTC_MIN 237
expr_static_assert(offsetof(struct expr_writeflag,bit)+sizeof(uint64_t)==sizeof(struct expr_writeflag));
typedef ssize_t (*expr_writer)(intptr_t fd,const void *buf,size_t size);
typedef ssize_t (*expr_reader)(intptr_t fd,void *buf,size_t size);
union expr_argf {
	void *addr;
	intptr_t sint;
	uintptr_t uint;
	double dbl;
	size_t z;
	ssize_t t;
	intptr_t *siaddr;
	uintptr_t *uiaddr;
	double *daddr;
	size_t *zaddr;
	ssize_t *taddr;
	const char *str;
	union expr_argf *aaddr;
#if ___POINTER_WIDTH__<64
#define expr_umaxf_t uint64_t
#define expr_imaxf_t int64_t
#else
#define expr_umaxf_t uintptr_t
#define expr_imaxf_t intptr_t
#endif
	expr_umaxf_t umax;
	expr_umaxf_t *umaddr;
	expr_imaxf_t smax;
	expr_imaxf_t *smaddr;
};
expr_static_assert(sizeof(union expr_argf)==sizeof(double));
expr_static_assert(sizeof(union expr_argf)==sizeof(expr_umaxf_t));
struct expr_writefmt {
	ssize_t (*converter)(expr_writer writer,intptr_t fd,const union expr_argf *arg,struct expr_writeflag *flag);
	uint8_t op[7];
	uint8_t type:2,no_arg:1,digit_check:1,setcap:1,unused:3;
};
struct expr_buffered_file {
	intptr_t fd;
	union {
		expr_reader reader;
		expr_writer writer;
	} un;
	void *buf;
	size_t index,length,dynamic,written;
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
struct expr_hmdinfo {
	struct expr *eps;
	double *args;
	struct expr *hotfunc;
	size_t dim;
};
struct expr_vmdinfo {
	struct expr *fromep,*toep,*stepep,*ep;
	size_t max;
	double (*func)(double *,size_t);
	double *args;
	volatile double index;
};
typedef uint16_t expr_superseed48[3];
struct expr_rawdouble {
#if (!defined(__BIG_ENDIAN__)||!(__BIG_ENDIAN__))
	uint64_t base:52;
	uint64_t exp:11;
	uint64_t sign:1;
#else
	uint64_t sign:1;
	uint64_t exp:11;
	uint64_t base:52;
#endif
}__attribute__((packed));
struct expr_inst {
	union {
		double *dst;
		int64_t *idst;
		intptr_t *pdst;
		void *uaddr;
		void **uaddr2;
		double **dst2;
		struct expr_inst **instaddr2;
		struct expr_rawdouble *rdst;
		double (**md2)(double *,size_t);
		double (**me2)(const struct expr *,size_t,double);
	} dst;
	union {
		double *src;
		int64_t *isrc;
		intptr_t *psrc;
		double **src2;
		void *uaddr;
		void **uaddr2;
		double value;
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
		struct expr_hmdinfo *eh;
	} un;
	enum expr_op op;
	int flag;
};
union expr_symvalue {
	double value;
	int64_t ivalue;
	uint64_t uvalue;
	size_t size;
	ssize_t off;
	double *addr;
	void *uaddr;
	const char *hot; //for builtin symbols only
	double (*func)(double);
	double (*zafunc)(void);
	double (*mdfunc)(double *,size_t);
	double (*mdepfunc)(const struct expr *,size_t,double);
};
_Static_assert(EXPR_SYMLEN<=64,"EXPR_SYMLEN is more than 6 bits");
#define EXPR_SYMBOL_DEPTHM1_WIDTH 54
struct expr_symbol {
	struct expr_symbol *next[EXPR_SYMNEXT];
	struct expr_symbol **tail;
	size_t length,strlen;
	uint64_t type:3,flag:6,saved:1,depthm1:EXPR_SYMBOL_DEPTHM1_WIDTH;
	char str[];
};
struct expr_symbol_infile {
	uint32_t length;
#if (!defined(__BIG_ENDIAN__)||!(__BIG_ENDIAN__))
#define expr_le(x,N) ((uint##N##_t)(x))
	uint16_t strlen:6,type:3,flag:6,unused:1;
#else
#define expr_le(x,N) expr_bswap(x,N)
	uint16_t unused:1,flag:6,type:3,strlen:6;
#endif
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
	uint32_t freeable,mutex;
};
struct expr_symset_infile {
	uint64_t size;
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
	int type,unused;
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
struct expr_internal_jmpbuf {
	struct expr_inst **ipp;
	struct expr_inst *ip;
	union {
		double val;
		uint64_t u64;
		int64_t i64;
		intptr_t iptr;
	} un;
	jmp_buf jb;
};

extern const struct expr_writefmt expr_writefmts_default[];
extern const uint8_t expr_writefmts_default_size;
extern const uint8_t expr_writefmts_table_default[256];
extern const uint8_t expr_number_table[256];

extern const struct expr_builtin_symbol expr_symbols[];
extern const size_t expr_symbols_size;

extern const struct expr_builtin_keyword expr_keywords[];

extern void *(*expr_allocator)(size_t);
extern void *(*expr_reallocator)(void *,size_t);
extern void (*expr_deallocator)(void *);
extern void (*expr_contractor)(void *,size_t);
extern size_t expr_allocate_max;
extern int expr_symset_allow_heap_stack;

extern size_t expr_bufsize_initial;
extern const size_t expr_page_size;

#define expr_internal_regvarr(_name) register intptr_t expr_combine(__r_,_name) __asm__(#_name)
#define expr_internal_regvar(_name) register intptr_t _name __asm__(#_name)

#ifndef EXPR_SYSIN

#if defined(__aarch64__)
#define EXPR_SYSIN "svc #0"
#define EXPR_SYSID x8
#define EXPR_SYSRE x0
#define EXPR_SYSA0 x0
#define EXPR_SYSA1 x1
#define EXPR_SYSA2 x2
#define EXPR_SYSA3 x3
#define EXPR_SYSA4 x4
#define EXPR_SYSA5 x5
#define EXPR_SYSAM 6
#define EXPR_SYSE0 expr_internal_regvarr(x0)
#define EXPR_SYSE1
#define EXPR_SYSE2
#define EXPR_SYSE3
#define EXPR_SYSE4
#define EXPR_SYSE5
#define EXPR_SYSE6
#define EXPR_SYSE7
#define EXPR_ARCHN "aarch64"
#elif defined(__x86_64__)
#define EXPR_SYSIN "syscall"
#define EXPR_SYSID rax
#define EXPR_SYSRE rax
#define EXPR_SYSA0 rdi
#define EXPR_SYSA1 rsi
#define EXPR_SYSA2 rdx
#define EXPR_SYSA3 r10
#define EXPR_SYSA4 r8
#define EXPR_SYSA5 r9
#define EXPR_SYSAM 6
#define EXPR_SYSE0
#define EXPR_SYSE1
#define EXPR_SYSE2
#define EXPR_SYSE3
#define EXPR_SYSE4
#define EXPR_SYSE5
#define EXPR_SYSE6
#define EXPR_SYSE7
#define EXPR_ARCHN "x86_64"
#else
#define EXPR_SYSAM 0
#define EXPR_ARCHN "unknown_arch"
#pragma message("internal syscall is not implemented")
#endif

#endif

#ifdef EXPR_SYSIN
#define expr_internal_syscall(N,num,a0,a1,a2,a3,a4,a5,a6) expr_internal_syscall_reg##N(num,a0,a1,a2,a3,a4,a5,a6,EXPR_SYSID,EXPR_SYSA0,EXPR_SYSA1,EXPR_SYSA2,EXPR_SYSA3,EXPR_SYSA4,EXPR_SYSA5,EXPR_SYSA6,EXPR_SYSRE,EXPR_SYSE0,EXPR_SYSE1,EXPR_SYSE2,EXPR_SYSE3,EXPR_SYSE4,EXPR_SYSE5,EXPR_SYSE6,EXPR_SYSE7)
#else
#define expr_internal_syscall(N,num,a0,a1,a2,a3,a4,a5,a6) ((intptr_t)0)
#endif

#define expr_internal_syscall0(num) expr_internal_syscall(0,num,,,,,,,)
#define expr_internal_syscall1(num,a0) expr_internal_syscall(1,num,a0,,,,,,)
#define expr_internal_syscall2(num,a0,a1) expr_internal_syscall(2,num,a0,a1,,,,,)
#define expr_internal_syscall3(num,a0,a1,a2) expr_internal_syscall(3,num,a0,a1,a2,,,,)
#define expr_internal_syscall4(num,a0,a1,a2,a3) expr_internal_syscall(4,num,a0,a1,a2,a3,,,)
#define expr_internal_syscall5(num,a0,a1,a2,a3,a4) expr_internal_syscall(5,num,a0,a1,a2,a3,a4,,)
#define expr_internal_syscall6(num,a0,a1,a2,a3,a4,a5) expr_internal_syscall(6,num,a0,a1,a2,a3,a4,a5,)
#define expr_internal_syscall7(num,a0,a1,a2,a3,a4,a5,a6) expr_internal_syscall(7,num,a0,a1,a2,a3,a4,a5,a6,)

#define expr_internal_syscall_reg7(num,a0,a1,a2,a3,a4,a5,a6,ID,A0,A1,A2,A3,A4,A5,A6,RE,E0,E1,E2,E3,E4,E5,E6,E7) ({\
		expr_internal_regvarr(ID)=(intptr_t)(num);\
		expr_internal_regvarr(A0)=(intptr_t)(a0);\
		expr_internal_regvarr(A1)=(intptr_t)(a1);\
		expr_internal_regvarr(A2)=(intptr_t)(a2);\
		expr_internal_regvarr(A3)=(intptr_t)(a3);\
		expr_internal_regvarr(A4)=(intptr_t)(a4);\
		expr_internal_regvarr(A5)=(intptr_t)(a5);\
		expr_internal_regvarr(A6)=(intptr_t)(a6);\
		E7;\
		__asm__ volatile(\
		EXPR_SYSIN \
		:"=r"(expr_combine(__r_,RE))\
		:"r"(expr_combine(__r_,A0)),"r"(expr_combine(__r_,A1)),"r"(expr_combine(__r_,A2)),"r"(expr_combine(__r_,A3)),"r"(expr_combine(__r_,A4)),"r"(expr_combine(__r_,A5)),"r"(expr_combine(__r_,A6)),"r"(expr_combine(__r_,ID))\
		:"memory");\
		expr_combine(__r_,RE);\
})
#define expr_internal_syscall_reg6(num,a0,a1,a2,a3,a4,a5,a6,ID,A0,A1,A2,A3,A4,A5,A6,RE,E0,E1,E2,E3,E4,E5,E6,E7) ({\
		expr_internal_regvarr(ID)=(intptr_t)(num);\
		expr_internal_regvarr(A0)=(intptr_t)(a0);\
		expr_internal_regvarr(A1)=(intptr_t)(a1);\
		expr_internal_regvarr(A2)=(intptr_t)(a2);\
		expr_internal_regvarr(A3)=(intptr_t)(a3);\
		expr_internal_regvarr(A4)=(intptr_t)(a4);\
		expr_internal_regvarr(A5)=(intptr_t)(a5);\
		E6;\
		__asm__ volatile(\
		EXPR_SYSIN \
		:"=r"(expr_combine(__r_,RE))\
		:"r"(expr_combine(__r_,A0)),"r"(expr_combine(__r_,A1)),"r"(expr_combine(__r_,A2)),"r"(expr_combine(__r_,A3)),"r"(expr_combine(__r_,A4)),"r"(expr_combine(__r_,A5)),"r"(expr_combine(__r_,ID))\
		:"memory");\
		expr_combine(__r_,RE);\
})
#define expr_internal_syscall_reg5(num,a0,a1,a2,a3,a4,a5,a6,ID,A0,A1,A2,A3,A4,A5,A6,RE,E0,E1,E2,E3,E4,E5,E6,E7) ({\
		expr_internal_regvarr(ID)=(intptr_t)(num);\
		expr_internal_regvarr(A0)=(intptr_t)(a0);\
		expr_internal_regvarr(A1)=(intptr_t)(a1);\
		expr_internal_regvarr(A2)=(intptr_t)(a2);\
		expr_internal_regvarr(A3)=(intptr_t)(a3);\
		expr_internal_regvarr(A4)=(intptr_t)(a4);\
		E5;\
		__asm__ volatile(\
		EXPR_SYSIN \
		:"=r"(expr_combine(__r_,RE))\
		:"r"(expr_combine(__r_,A0)),"r"(expr_combine(__r_,A1)),"r"(expr_combine(__r_,A2)),"r"(expr_combine(__r_,A3)),"r"(expr_combine(__r_,A4)),"r"(expr_combine(__r_,ID))\
		:"memory");\
		expr_combine(__r_,RE);\
})
#define expr_internal_syscall_reg4(num,a0,a1,a2,a3,a4,a5,a6,ID,A0,A1,A2,A3,A4,A5,A6,RE,E0,E1,E2,E3,E4,E5,E6,E7) ({\
		expr_internal_regvarr(ID)=(intptr_t)(num);\
		expr_internal_regvarr(A0)=(intptr_t)(a0);\
		expr_internal_regvarr(A1)=(intptr_t)(a1);\
		expr_internal_regvarr(A2)=(intptr_t)(a2);\
		expr_internal_regvarr(A3)=(intptr_t)(a3);\
		E4;\
		__asm__ volatile(\
		EXPR_SYSIN \
		:"=r"(expr_combine(__r_,RE))\
		:"r"(expr_combine(__r_,A0)),"r"(expr_combine(__r_,A1)),"r"(expr_combine(__r_,A2)),"r"(expr_combine(__r_,A3)),"r"(expr_combine(__r_,ID))\
		:"memory");\
		expr_combine(__r_,RE);\
})
#define expr_internal_syscall_reg3(num,a0,a1,a2,a3,a4,a5,a6,ID,A0,A1,A2,A3,A4,A5,A6,RE,E0,E1,E2,E3,E4,E5,E6,E7) ({\
		expr_internal_regvarr(ID)=(intptr_t)(num);\
		expr_internal_regvarr(A0)=(intptr_t)(a0);\
		expr_internal_regvarr(A1)=(intptr_t)(a1);\
		expr_internal_regvarr(A2)=(intptr_t)(a2);\
		E3;\
		__asm__ volatile(\
		EXPR_SYSIN \
		:"=r"(expr_combine(__r_,RE))\
		:"r"(expr_combine(__r_,A0)),"r"(expr_combine(__r_,A1)),"r"(expr_combine(__r_,A2)),"r"(expr_combine(__r_,ID))\
		:"memory");\
		expr_combine(__r_,RE);\
})
#define expr_internal_syscall_reg2(num,a0,a1,a2,a3,a4,a5,a6,ID,A0,A1,A2,A3,A4,A5,A6,RE,E0,E1,E2,E3,E4,E5,E6,E7) ({\
		expr_internal_regvarr(ID)=(intptr_t)(num);\
		expr_internal_regvarr(A0)=(intptr_t)(a0);\
		expr_internal_regvarr(A1)=(intptr_t)(a1);\
		E2;\
		__asm__ volatile(\
		EXPR_SYSIN \
		:"=r"(expr_combine(__r_,RE))\
		:"r"(expr_combine(__r_,A0)),"r"(expr_combine(__r_,A1)),"r"(expr_combine(__r_,ID))\
		:"memory");\
		expr_combine(__r_,RE);\
})
#define expr_internal_syscall_reg1(num,a0,a1,a2,a3,a4,a5,a6,ID,A0,A1,A2,A3,A4,A5,A6,RE,E0,E1,E2,E3,E4,E5,E6,E7) ({\
		expr_internal_regvarr(ID)=(intptr_t)(num);\
		expr_internal_regvarr(A0)=(intptr_t)(a0);\
		E1;\
		__asm__ volatile(\
		EXPR_SYSIN \
		:"=r"(expr_combine(__r_,RE))\
		:"r"(expr_combine(__r_,A0)),"r"(expr_combine(__r_,ID))\
		:"memory");\
		expr_combine(__r_,RE);\
})
#define expr_internal_syscall_reg0(num,a0,a1,a2,a3,a4,a5,a6,ID,A0,A1,A2,A3,A4,A5,A6,RE,E0,E1,E2,E3,E4,E5,E6,E7) ({\
		expr_internal_regvarr(ID)=(intptr_t)(num);\
		E0;\
		__asm__ volatile(\
		EXPR_SYSIN \
		:"=r"(expr_combine(__r_,RE))\
		:"r"(expr_combine(__r_,ID))\
		:"memory");\
		expr_combine(__r_,RE);\
})

#define expr_internal_syscall_ncase_inswitch0(dest,cast,num,a0,a1,a2,a3,a4,a5,a6) case 0:(dest)=(cast)expr_internal_syscall0(num);break
#define expr_internal_syscall_ncase_inswitch1(dest,cast,num,a0,a1,a2,a3,a4,a5,a6) case 1:(dest)=(cast)expr_internal_syscall1(num,a0);break
#define expr_internal_syscall_ncase_inswitch2(dest,cast,num,a0,a1,a2,a3,a4,a5,a6) case 2:(dest)=(cast)expr_internal_syscall2(num,a0,a1);break
#define expr_internal_syscall_ncase_inswitch3(dest,cast,num,a0,a1,a2,a3,a4,a5,a6) case 3:(dest)=(cast)expr_internal_syscall3(num,a0,a1,a2);break
#define expr_internal_syscall_ncase_inswitch4(dest,cast,num,a0,a1,a2,a3,a4,a5,a6) case 4:(dest)=(cast)expr_internal_syscall4(num,a0,a1,a2,a3);break
#define expr_internal_syscall_ncase_inswitch5(dest,cast,num,a0,a1,a2,a3,a4,a5,a6) case 5:(dest)=(cast)expr_internal_syscall5(num,a0,a1,a2,a3,a4);break
#define expr_internal_syscall_ncase_inswitch6(dest,cast,num,a0,a1,a2,a3,a4,a5,a6) case 6:(dest)=(cast)expr_internal_syscall6(num,a0,a1,a2,a3,a4,a5);break
#define expr_internal_syscall_ncase_inswitch7(dest,cast,num,a0,a1,a2,a3,a4,a5,a6) case 7:(dest)=(cast)expr_internal_syscall7(num,a0,a1,a2,a3,a4,a5,a6);break
#if (EXPR_SYSAM==0)
#define expr_internal_syscall_ncase_inswitch(dest,cast,num,a0,a1,a2,a3,a4,a5,a6) expr_internal_syscall_ncase_inswitch0(dest,cast,num,a0,a1,a2,a3,a4,a5,a6)
#elif (EXPR_SYSAM==1)
#define expr_internal_syscall_ncase_inswitch(dest,cast,num,a0,a1,a2,a3,a4,a5,a6) expr_internal_syscall_ncase_inswitch0(dest,cast,num,a0,a1,a2,a3,a4,a5,a6);expr_internal_syscall_ncase_inswitch1(dest,cast,num,a0,a1,a2,a3,a4,a5,a6)
#elif (EXPR_SYSAM==2)
#define expr_internal_syscall_ncase_inswitch(dest,cast,num,a0,a1,a2,a3,a4,a5,a6) expr_internal_syscall_ncase_inswitch0(dest,cast,num,a0,a1,a2,a3,a4,a5,a6);expr_internal_syscall_ncase_inswitch1(dest,cast,num,a0,a1,a2,a3,a4,a5,a6);expr_internal_syscall_ncase_inswitch2(dest,cast,num,a0,a1,a2,a3,a4,a5,a6)
#elif (EXPR_SYSAM==3)
#define expr_internal_syscall_ncase_inswitch(dest,cast,num,a0,a1,a2,a3,a4,a5,a6) expr_internal_syscall_ncase_inswitch0(dest,cast,num,a0,a1,a2,a3,a4,a5,a6);expr_internal_syscall_ncase_inswitch1(dest,cast,num,a0,a1,a2,a3,a4,a5,a6);expr_internal_syscall_ncase_inswitch2(dest,cast,num,a0,a1,a2,a3,a4,a5,a6);expr_internal_syscall_ncase_inswitch3(dest,cast,num,a0,a1,a2,a3,a4,a5,a6)
#elif (EXPR_SYSAM==4)
#define expr_internal_syscall_ncase_inswitch(dest,cast,num,a0,a1,a2,a3,a4,a5,a6) expr_internal_syscall_ncase_inswitch0(dest,cast,num,a0,a1,a2,a3,a4,a5,a6);expr_internal_syscall_ncase_inswitch1(dest,cast,num,a0,a1,a2,a3,a4,a5,a6);expr_internal_syscall_ncase_inswitch2(dest,cast,num,a0,a1,a2,a3,a4,a5,a6);expr_internal_syscall_ncase_inswitch3(dest,cast,num,a0,a1,a2,a3,a4,a5,a6);expr_internal_syscall_ncase_inswitch4(dest,cast,num,a0,a1,a2,a3,a4,a5,a6)
#elif (EXPR_SYSAM==5)
#define expr_internal_syscall_ncase_inswitch(dest,cast,num,a0,a1,a2,a3,a4,a5,a6) expr_internal_syscall_ncase_inswitch0(dest,cast,num,a0,a1,a2,a3,a4,a5,a6);expr_internal_syscall_ncase_inswitch1(dest,cast,num,a0,a1,a2,a3,a4,a5,a6);expr_internal_syscall_ncase_inswitch2(dest,cast,num,a0,a1,a2,a3,a4,a5,a6);expr_internal_syscall_ncase_inswitch3(dest,cast,num,a0,a1,a2,a3,a4,a5,a6);expr_internal_syscall_ncase_inswitch4(dest,cast,num,a0,a1,a2,a3,a4,a5,a6);expr_internal_syscall_ncase_inswitch5(dest,cast,num,a0,a1,a2,a3,a4,a5,a6)
#elif (EXPR_SYSAM==6)
#define expr_internal_syscall_ncase_inswitch(dest,cast,num,a0,a1,a2,a3,a4,a5,a6) expr_internal_syscall_ncase_inswitch0(dest,cast,num,a0,a1,a2,a3,a4,a5,a6);expr_internal_syscall_ncase_inswitch1(dest,cast,num,a0,a1,a2,a3,a4,a5,a6);expr_internal_syscall_ncase_inswitch2(dest,cast,num,a0,a1,a2,a3,a4,a5,a6);expr_internal_syscall_ncase_inswitch3(dest,cast,num,a0,a1,a2,a3,a4,a5,a6);expr_internal_syscall_ncase_inswitch4(dest,cast,num,a0,a1,a2,a3,a4,a5,a6);expr_internal_syscall_ncase_inswitch5(dest,cast,num,a0,a1,a2,a3,a4,a5,a6);expr_internal_syscall_ncase_inswitch6(dest,cast,num,a0,a1,a2,a3,a4,a5,a6)
#elif (EXPR_SYSAM==7)
#define expr_internal_syscall_ncase_inswitch(dest,cast,num,a0,a1,a2,a3,a4,a5,a6) expr_internal_syscall_ncase_inswitch0(dest,cast,num,a0,a1,a2,a3,a4,a5,a6);expr_internal_syscall_ncase_inswitch1(dest,cast,num,a0,a1,a2,a3,a4,a5,a6);expr_internal_syscall_ncase_inswitch2(dest,cast,num,a0,a1,a2,a3,a4,a5,a6);expr_internal_syscall_ncase_inswitch3(dest,cast,num,a0,a1,a2,a3,a4,a5,a6);expr_internal_syscall_ncase_inswitch4(dest,cast,num,a0,a1,a2,a3,a4,a5,a6);expr_internal_syscall_ncase_inswitch5(dest,cast,num,a0,a1,a2,a3,a4,a5,a6);expr_internal_syscall_ncase_inswitch6(dest,cast,num,a0,a1,a2,a3,a4,a5,a6);expr_internal_syscall_ncase_inswitch7(dest,cast,num,a0,a1,a2,a3,a4,a5,a6)
#else
#define expr_internal_syscall_ncase_inswitch(dest,cast,num,a0,a1,a2,a3,a4,a5,a6) ((intptr_t)0)
#pragma message("only 0-7 arguments syscall is supported")
#endif
#define expr_internal_syscall_eval(dest,cast,N,num,a0,a1,a2,a3,a4,a5,a6) ({\
	switch(N){\
		expr_internal_syscall_ncase_inswitch(dest,cast,num,a0,a1,a2,a3,a4,a5,a6);\
		__builtin_unreachable();\
	}\
})
struct expr {
	struct expr_inst *data;
	struct expr_inst **ipp;
	size_t size;
	struct expr *parent;
	double **vars;
	struct expr_symset *sset;
	struct expr_resource *res,*tail;
	size_t length,vsize,vlength;
	union {
		double args[EXPR_SYSAM];
		struct {
			struct expr_inst endinst[1];
			double val;
		} end[1];
	} un;
	int error;
	short iflag;
	uint8_t freeable:2,sset_shouldfree:1,isconst:1,unused:4;
	char errinfo[EXPR_SYMLEN];
	char extra_data[];
};
ssize_t expr_writec(expr_writer writer,intptr_t fd,size_t count,int c);
ssize_t expr_converter_common(expr_writer writer,intptr_t fd,const void *buf,size_t size,const struct expr_writeflag *flag);
ssize_t expr_vwritef(const char *restrict fmt,size_t fmtlen,expr_writer writer,intptr_t fd,const union expr_argf *(*arg)(ptrdiff_t index,const struct expr_writeflag *flag,void *addr),void *addr);
ssize_t expr_vwritef_r(const char *restrict fmt,size_t fmtlen,expr_writer writer,intptr_t fd,const union expr_argf *(*arg)(ptrdiff_t index,const struct expr_writeflag *flag,void *addr),void *addr,const struct expr_writefmt *restrict fmts,const uint8_t *restrict table);
ssize_t expr_writef(const char *restrict fmt,size_t fmtlen,expr_writer writer,intptr_t fd,const union expr_argf *restrict args,size_t arglen);
ssize_t expr_writef_r(const char *restrict fmt,size_t fmtlen,expr_writer writer,intptr_t fd,const union expr_argf *restrict args,size_t arglen,const struct expr_writefmt *restrict fmts,const uint8_t *restrict table);
ssize_t expr_vapwritef_r(const char *restrict fmt,size_t fmtlen,expr_writer writer,intptr_t fd,const struct expr_writefmt *restrict fmts,const uint8_t *restrict table,va_list ap);
ssize_t expr_apwritef_r(const char *restrict fmt,size_t fmtlen,expr_writer writer,intptr_t fd,const struct expr_writefmt *restrict fmts,const uint8_t *restrict table,...);
ssize_t expr_vapwritef(const char *restrict fmt,size_t fmtlen,expr_writer writer,intptr_t fd,va_list ap);
ssize_t expr_apwritef(const char *restrict fmt,size_t fmtlen,expr_writer writer,intptr_t fd,...);
ssize_t expr_buffered_write(struct expr_buffered_file *restrict fp,const void *buf,size_t size);
ssize_t expr_buffered_read(struct expr_buffered_file *restrict fp,void *buf,size_t size);
ssize_t expr_buffered_write_flushatc(struct expr_buffered_file *restrict fp,const void *buf,size_t size,int c);
ssize_t expr_buffered_write_flushat(struct expr_buffered_file *restrict fp,const void *buf,size_t size,void *c,size_t c_size);
ssize_t expr_buffered_flush(struct expr_buffered_file *restrict fp);
ssize_t expr_buffered_drop(struct expr_buffered_file *restrict fp);
ssize_t expr_buffered_rdrop(struct expr_buffered_file *restrict fp);
ssize_t expr_buffered_rdropall(struct expr_buffered_file *restrict fp);
ssize_t expr_buffered_close(struct expr_buffered_file *restrict fp);
void expr_buffered_rclose(struct expr_buffered_file *restrict fp);
ssize_t expr_file_readfd(expr_reader reader,intptr_t fd,size_t tail,void *savep);
int expr_sort4(double *restrict v,size_t n,void *(*allocator)(size_t),void (*deallocator)(void *));
void expr_sortq(double *restrict v,size_t n);
void expr_sort_old(double *restrict v,size_t n);
void expr_sort(double *v,size_t n);
double expr_exp_old(double x);
double expr_multilevel_derivate(const struct expr *ep,double input,long level,double epsilon);
const char *expr_error(int error);
uint64_t expr_gcd64(uint64_t x,uint64_t y);
double expr_gcd2(double x,double y);
double expr_lcm2(double x,double y);
void expr_mirror(double *buf,size_t size);
void expr_memswap(void *restrict s1,void *restrict s2,size_t size);
void expr_memfry48(void *restrict buf,size_t size,size_t n,int64_t seed);
void expr_fry(double *restrict v,size_t n);
void expr_contract(void *buf,size_t size);
__attribute__((noreturn)) void expr_explode(void);
__attribute__((noreturn)) void expr_trap(void);
__attribute__((noreturn)) void expr_ubehavior(void);
double expr_and2(double x,double y);
double expr_or2(double x,double y);
double expr_xor2(double x,double y);
double expr_not(double x);
void expr_mutex_lock(uint32_t *lock);
int expr_mutex_trylock(uint32_t *lock);
void expr_mutex_unlock(uint32_t *lock);
void expr_mutex_spinlock(uint32_t *lock);
void expr_mutex_spinunlock(uint32_t *lock);
intptr_t expr_warped_syscall0(int num);
intptr_t expr_warped_syscall1(int num,intptr_t a0);
intptr_t expr_warped_syscall2(int num,intptr_t a0,intptr_t a1);
intptr_t expr_warped_syscall3(int num,intptr_t a0,intptr_t a1,intptr_t a2);
intptr_t expr_warped_syscall4(int num,intptr_t a0,intptr_t a1,intptr_t a2,intptr_t a3);
intptr_t expr_warped_syscall5(int num,intptr_t a0,intptr_t a1,intptr_t a2,intptr_t a3,intptr_t a4);
intptr_t expr_warped_syscall6(int num,intptr_t a0,intptr_t a1,intptr_t a2,intptr_t a3,intptr_t a4,intptr_t a5);
intptr_t expr_warped_syscall7(int num,intptr_t a0,intptr_t a1,intptr_t a2,intptr_t a3,intptr_t a4,intptr_t a5,intptr_t a6);
const struct expr_builtin_symbol *expr_builtin_symbol_search(const struct expr_builtin_symbol *syms,const char *sym,size_t sz);
const struct expr_builtin_symbol *expr_builtin_symbol_rsearch(const struct expr_builtin_symbol *syms,void *addr);
struct expr_symbol *expr_builtin_symbol_add(struct expr_symset *restrict esp,const struct expr_builtin_symbol *p);
size_t expr_builtin_symbol_addall(struct expr_symset *restrict esp,const struct expr_builtin_symbol *syms);
struct expr_symset *expr_builtin_symbol_convert(const struct expr_builtin_symbol *syms);
size_t expr_strscan(const char *restrict s,size_t sz,char *restrict buf,size_t outsz);
char *expr_astrscan(const char *s,size_t sz,size_t *restrict outsz);
void expr_free(struct expr *restrict ep);
void init_expr_symset(struct expr_symset *restrict esp);
struct expr_symset *new_expr_symset(void);
void expr_symset_free(struct expr_symset *restrict esp);
void expr_symset_free_s(struct expr_symset *restrict esp,void *stack);
void expr_symset_wipe(struct expr_symset *restrict esp);
void expr_symset_wipe_s(struct expr_symset *restrict esp,void *stack);
void expr_symset_save(struct expr_symset *restrict esp,void *buf);
void expr_symset_save_s(struct expr_symset *restrict esp,void *buf,void *stack);
void expr_symset_move(struct expr_symset *restrict esp,ptrdiff_t off);
void expr_symset_move_s(struct expr_symset *restrict esp,ptrdiff_t off,void *stack);
ssize_t expr_symset_write(const struct expr_symset *restrict esp,expr_writer writer,intptr_t fd);
ssize_t expr_symset_write_s(const struct expr_symset *restrict esp,expr_writer writer,intptr_t fd,void *stack);
ssize_t expr_symset_read(struct expr_symset *restrict esp,const void *buf,size_t size);
ssize_t expr_symset_readfd(struct expr_symset *restrict esp,expr_reader reader,intptr_t fd);
struct expr_symbol **expr_symset_findtail(struct expr_symset *restrict esp,const char *sym,size_t symlen,size_t *depth);
struct expr_symbol *expr_symbol_create(const char *sym,int type,int flag,...);
struct expr_symbol *expr_symbol_createl(const char *sym,size_t symlen,int type,int flag,...);
struct expr_symbol *expr_symbol_vcreate(const char *sym,int type,int flag,va_list ap);
struct expr_symbol *expr_symset_add(struct expr_symset *restrict esp,const char *sym,int type,int flag,...);
struct expr_symbol *expr_symset_addl(struct expr_symset *restrict esp,const char *sym,size_t symlen,int type,int flag,...);
struct expr_symbol *expr_symset_vadd(struct expr_symset *restrict esp,const char *sym,int type,int flag,va_list ap);
struct expr_symbol *expr_symbol_vcreatel(const char *sym,size_t symlen,int type,int flag,va_list ap);
struct expr_symbol *expr_symset_vaddl(struct expr_symset *restrict esp,const char *sym,size_t symlen,int type,int flag,va_list ap);
struct expr_symbol *expr_symset_addcopy(struct expr_symset *restrict esp,const struct expr_symbol *restrict es);
struct expr_symbol **expr_symset_search0(const struct expr_symset *restrict esp,const char *sym,size_t sz);
struct expr_symbol *expr_symset_search(const struct expr_symset *restrict esp,const char *sym,size_t sz);
struct expr_symbol *expr_symset_searchd(const struct expr_symset *restrict esp,const char *sym,size_t sz,size_t *depth);
int expr_symbol_check(const struct expr_symbol *restrict esp);
int expr_symset_insert(struct expr_symset *restrict esp,struct expr_symbol *restrict es);
int expr_symset_remove(struct expr_symset *restrict esp,const char *sym,size_t sz);
int expr_symset_remove_s(struct expr_symset *restrict esp,const char *sym,size_t sz,void *stack);
void expr_symset_removes(struct expr_symset *restrict esp,struct expr_symbol *sp);
void expr_symset_removes_s(struct expr_symset *restrict esp,struct expr_symbol *sp,void *stack);
void expr_symset_removea(struct expr_symset *restrict esp,struct expr_symbol *const *spa,size_t n);
void expr_symset_removea_s(struct expr_symset *restrict esp,struct expr_symbol *const *spa,size_t n,void *stack);
int expr_symset_detach(struct expr_symset *restrict esp,const char *sym,size_t sz);
int expr_symset_detach_s(struct expr_symset *restrict esp,const char *sym,size_t sz,void *stack);
void expr_symset_detachs(struct expr_symset *restrict esp,struct expr_symbol *sp);
void expr_symset_detachs_s(struct expr_symset *restrict esp,struct expr_symbol *sp,void *stack);
void expr_symset_detacha(struct expr_symset *restrict esp,struct expr_symbol *const *spa,size_t n);
void expr_symset_detacha_s(struct expr_symset *restrict esp,struct expr_symbol *const *spa,size_t n,void *stack);
int expr_symset_recombine(struct expr_symset *restrict esp,long seed);
int expr_symset_recombine_s(struct expr_symset *restrict esp,int64_t seed,void *stack);
int expr_symset_tryrecombine(struct expr_symset *restrict esp,long seed);
struct expr_symbol *expr_symset_rsearch(const struct expr_symset *restrict esp,void *addr);
struct expr_symbol *expr_symset_rsearch_s(const struct expr_symset *restrict esp,void *addr,void *stack);
size_t expr_symset_depth(const struct expr_symset *restrict esp);
size_t expr_symset_depth_s(const struct expr_symset *restrict esp,void *stack);
void expr_symset_correct(struct expr_symset *restrict esp);
void expr_symset_correct_s(struct expr_symset *restrict esp,void *stack);
size_t expr_symset_length(const struct expr_symset *restrict esp);
size_t expr_symset_length_s(const struct expr_symset *restrict esp,void *stack);
size_t expr_symset_size(const struct expr_symset *restrict esp);
size_t expr_symset_size_s(const struct expr_symset *restrict esp,void *stack);
size_t expr_symset_copy(struct expr_symset *restrict dst,const struct expr_symset *restrict src);
size_t expr_symset_copy_s(struct expr_symset *restrict dst,const struct expr_symset *restrict src,void *stack);
struct expr_symset *expr_symset_clone(const struct expr_symset *restrict ep);
struct expr_symset *expr_symset_clone_s(const struct expr_symset *restrict ep,void *stack);
int expr_isconst(const struct expr *restrict ep);
void init_expr_const(struct expr *restrict ep,double val);
struct expr *new_expr_const(double val);
int init_expr7(struct expr *restrict ep,const char *e,size_t len,const char *asym,size_t asymlen,struct expr_symset *esp,int flag);
int init_expr5(struct expr *restrict ep,const char *e,const char *asym,struct expr_symset *esp,int flag);
int init_expr(struct expr *restrict ep,const char *e,const char *asym,struct expr_symset *esp);
struct expr *new_expr9(const char *e,size_t len,const char *asym,size_t asymlen,struct expr_symset *esp,int flag,int n,int *error,char errinfo[EXPR_SYMLEN]);
struct expr *new_expr7(const char *e,const char *asym,struct expr_symset *esp,int flag,int n,int *error,char errinfo[EXPR_SYMLEN]);
struct expr *new_expr8(const char *e,size_t len,const char *asym,size_t asymlen,struct expr_symset *esp,int flag,int *error,char errinfo[EXPR_SYMLEN]);
struct expr *new_expr6(const char *e,const char *asym,struct expr_symset *esp,int flag,int *error,char errinfo[EXPR_SYMLEN]);
struct expr *new_expr(const char *e,const char *asym,struct expr_symset *esp,int *error,char errinfo[EXPR_SYMLEN]);
double expr_calc5(const char *e,int *error,char errinfo[EXPR_SYMLEN],struct expr_symset *esp,int flag);
double expr_calc4(const char *e,int *error,char errinfo[EXPR_SYMLEN],struct expr_symset *esp);
double expr_calc3(const char *e,int *error,char errinfo[EXPR_SYMLEN]);
double expr_calc2(const char *e,int flag);
double expr_calc(const char *e);
int expr_optimize(struct expr *restrict ep);
double expr_eval(const struct expr *restrict ep,double input);
int expr_step(const struct expr *restrict ep,double input,double *restrict output,struct expr_inst **restrict saveip);
double expr_callback(const struct expr *restrict ep,double input,const struct expr_callback *ec);
#endif
