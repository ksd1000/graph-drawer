/*******************************************************************************
 *License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>*
 *This is free software: you are free to change and redistribute it.           *
 *******************************************************************************/

/*
 * History: 2024-02 expr.h
 * Header file of libexpr and libexpr-lc
 */

#ifndef _EXPR_H_
#define _EXPR_H_

#ifdef __BEGIN_DECLS
__BEGIN_DECLS
#endif

#ifndef _EXPR_DEFS_H_
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <limits.h>
#endif

#define expr_static_assert(cond) _Static_assert((cond),"static assertion " #cond " failed")

expr_static_assert((!0)==1);

#define expr_issigned(T) ((__typeof__(T))-1<(__typeof__(T))0)

#if !defined(ssize_t)&&!defined(_SSIZE_T_DEFINED_)&&!defined(_SSIZE_T_DEFINED)&&!defined(SSIZE_T_DEFINED)&&!defined(HAVE_SSIZE_T)&&!defined(__ssize_t_defined)&&!defined(_SYS_TYPES_H_)&&!defined(_SYS_TYPES_H)&&!defined(SYS_TYPES_H)

typedef ptrdiff_t ssize_t;
#define _SSIZE_T_DEFINED_ 1
#define _SSIZE_T_DEFINED 1
#define __ssize_t_defined 1

#endif

expr_static_assert(expr_issigned(ssize_t));
expr_static_assert(sizeof(ssize_t)==sizeof(size_t));

#ifndef SSIZE_MAX
#define SSIZE_MAX ((ssize_t)(SIZE_MAX>>1))
#endif

#ifndef SSIZE_MIN
#define SSIZE_MIN ((ssize_t)((SIZE_MAX>>1)+1))
#endif

expr_static_assert(sizeof(ssize_t)==sizeof(ptrdiff_t));
expr_static_assert(sizeof(size_t)==sizeof(ptrdiff_t));
expr_static_assert(sizeof(void *)==sizeof(ptrdiff_t));

#ifndef expr_noinline
#define expr_noinline __attribute__((noinline))
#endif


#ifndef expr_t
#define expr_t double
#define expr_ti int64_t
#define expr_tu uint64_t
#define EXPR_FMTd "f"
#define EXPR_FMTu "f"
#define EXPR_FMTo "\xa8"
#define EXPR_FMTx "a"
#define EXPR_FMTX "A"
#define EXPR_FAIL NAN
#define EXPR_INFINITY INFINITY
#define EXPR_ZERO 0.0
#define EXPR_ONE 1.0
#define EXPR_TWO 2.0
#define EXPR_IMAXO 9007199254740991.0
#define EXPR_EDBASE(d) (((union expr_double *)(d))->rd.base)
#define EXPR_EDEXP(d) (((union expr_double *)(d))->rd.exp)
#define EXPR_EDSIGN(d) (((union expr_double *)(d))->rd.sign)
#define EXPR_EDIVAL(d) (((union expr_double *)(d))->ival)
#define EXPR_EDUVAL(d) (((union expr_double *)(d))->uval)
#define expr_tand(__a,__b) expr_internal_fand(__a,__b)
#define expr_tor(__a,__b) expr_internal_for(__a,__b)
#define expr_txor(__a,__b) expr_internal_fxor(__a,__b)
#define expr_tnot(_x) expr_internal_fxor((_x),EXPR_IMAXO)
#define expr_tabs(_x) expr_internal_fabs(_x)
#define expr_tpow(__a,__b) pow(__a,__b)
#define expr_tmod(__a,__b) fmod(__a,__b)
#define expr_tscan(_str,_size,_outsize,_mtl) expr_internal_strtod_mtl_force(_str,_size,_outsize,_mtl)
#endif

typedef ssize_t (*expr_writer)(intptr_t fd,const void *buf,size_t size);
typedef ssize_t (*expr_reader)(intptr_t fd,void *buf,size_t size);
typedef void *(*expr_allocate_t)(size_t,intptr_t);
typedef void *(*expr_reallocate_t)(void *,size_t,intptr_t);
typedef void (*expr_deallocate_t)(void *,intptr_t);
typedef intptr_t (*expr_test_t)(const void *buf,intptr_t arg,size_t size);
struct expr_memtool {
	expr_allocate_t allocate;
	expr_reallocate_t reallocate;
	expr_deallocate_t deallocate;
	expr_test_t test;
	intptr_t arg;
};
#ifndef EXPR_EXPAND_SIZE
#define EXPR_EXPAND_SIZE 16
#endif
void *expr_allocator_default(size_t size,intptr_t arg);
void *expr_reallocator_default(void *old,size_t size,intptr_t arg);
void expr_deallocator_default(void *old,intptr_t arg);
extern struct expr_memtool expr_defmtl[1];
extern size_t expr_allocate_max;

#define expr_internal_strlen(s) ({const char *_s=(s);(size_t)((const char *)expr_internal_memchr2(_s,0)-_s);})

#define expr_globals_define() \
size_t expr_allocate_max=SSIZE_MAX;\
void *expr_allocator_default(size_t size,intptr_t arg){\
	(void)arg;\
	return size>=expr_allocate_max?NULL:malloc(size);\
}\
void *expr_reallocator_default(void *old,size_t size,intptr_t arg){\
	(void)arg;\
	return size>=expr_allocate_max?NULL:realloc(old,size);\
}\
void expr_deallocator_default(void *old,intptr_t arg){\
	(void)arg;\
	free(old);\
}\
struct expr_memtool expr_defmtl[1]={{\
	.allocate=expr_allocator_default,\
	.reallocate=expr_reallocator_default,\
	.deallocate=expr_deallocator_default,\
	.test=NULL,\
	.arg=0,\
}}

#if (defined(EXPR_SETJMP_IMPL)&&(EXPR_SETJMP_IMPL))
#include <setjmp.h>
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
#else
struct expr_internal_jmpbuf {
	struct expr_inst **ipp;
	struct expr_inst *ip;
	union {
		double val;
		uint64_t u64;
		int64_t i64;
		intptr_t iptr;
	} un;
	intptr_t *jb[1];
};
#endif


#if (defined(EXPR_INLIB)&&(EXPR_INLIB))
#define EXPR_BLOCKWARNING 1
#endif

#if (defined(EXPR_BLOCKWARNING)&&(EXPR_BLOCKWARNING))

#ifdef __clang__
#pragma GCC diagnostic ignored "-Wunknown-warning-option"
#else
#pragma GCC diagnostic ignored "-Wpragmas"
#endif

#pragma GCC diagnostic ignored "-Wzero-length-bounds"
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#pragma GCC diagnostic ignored "-Wpointer-arith"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wcast-function-type-mismatch"
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#pragma GCC diagnostic ignored "-Wclobbered"
#pragma GCC diagnostic ignored "-Wcast-function-type"
#pragma GCC diagnostic ignored "-Waddress"
#pragma GCC diagnostic ignored "-Wextra-semi"
#pragma GCC diagnostic ignored "-Wc2y-extensions"
#pragma GCC diagnostic ignored "-Wgnu-auto-type"
#pragma GCC diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wnull-dereference"
#pragma GCC diagnostic ignored "-Wconversion"

#pragma GCC diagnostic ignored "-Wgnu-statement-expression-from-macro-expansion"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wflexible-array-extensions"

#endif

#if (defined(EXPR_INLIB)&&(EXPR_INLIB))
#ifndef EXPR_DEBUG
#define EXPR_DEBUG 0
#endif

#if (defined(EXPR_DEBUG)&&(EXPR_DEBUG))
#include <stdio.h>
#define debug(fmt,...) ((void)fprintf(stderr,"[DEBUG]%s:%d: " fmt "\n",__func__,__LINE__,##__VA_ARGS__))
//the debug() must not be provided by expr-format.c
#else
#define debug(fmt,...) ((void)0)
#endif

#define etozu0(E) ((E)>=EXPR_IMAXO?(size_t)EXPR_IMAXO:((E)<=0.0?(size_t)0:(size_t)(E)))
#define etoi0(E) ((E)>=(expr_t)INT_MAX?INT_MAX:((E)<=(expr_t)INT_MIN?INT_MIN:(int)(E)))
#define etoi(E) ({expr_t _E=(E);etoi0(_E);})
#define STACK_SIZEOFSSET(esp) ({size_t depth=(esp)->depth;depth<2?0:(depth-1)*EXPR_SYMSET_DEPTHUNIT;})
#define STACK_DEFAULT(_stack,esp) \
	void *_stack;\
	size_t _stack##_size;\
	_stack##_size=STACK_SIZEOFSSET(esp);\
	_stack=alloca(_stack##_size)
#define SYMDIM(sp) (*expr_symbol_dim(sp))
#define HOTLEN(sp) expr_symbol_hotlen(sp)

#if !(defined(EXPR_LIBC_FMOD_IMPL)&&(EXPR_LIBC_FMOD_IMPL))
#undef fmod
#define fmod expr_internal_fmod
#endif

#if !(defined(EXPR_LIBC_POW_IMPL)&&(EXPR_LIBC_POW_IMPL))
#undef pow
#define pow expr_internal_pow
#endif

#undef fabs
#define fabs expr_internal_fabs

#if !(defined(EXPR_LIBC_STRING_IMPL)&&(EXPR_LIBC_STRING_IMPL))
#undef memchr
#undef memset
#undef memcpy
#undef memmove
#undef memrchr
#undef memcmp
#undef memmem
#undef memrmem
#undef strend
#undef strlen
#undef strnlen
#define memchr expr_internal_memchr
#define memset expr_internal_memset
#define memcpy expr_internal_memcpy
#define memmove expr_internal_memmove
#define memrchr expr_internal_memrchr
#define memcmp expr_internal_memcmp
#define strend(s) expr_internal_memchr2(s,0)
#define strlen expr_internal_strlen
#define strnlen(s,n) ({const char *_s=(s);size_t _n=(n);const char *_p=expr_internal_memchr(_s,0,_n);_p?(size_t)(_p-_s):_n;})
#define memmem expr_fake_memmem
#define memrmem expr_fake_memrmem
#else
#define strend(s) ({const char *_s=(s);_s+strlen(_s);})
#endif

#define warn(fmt,...) fprintf(stderr,fmt "\n",##__VA_ARGS__)

#define addo(V,A) __builtin_add_overflow((V),(A),&(V))
#define mulo(V,A) __builtin_mul_overflow((V),(A),&(V))
#define addo3(V,A,D) __builtin_add_overflow((V),(A),(D))
#define mulo3(V,A,D) __builtin_mul_overflow((V),(A),(D))

#define likely(cond) expr_likely(cond)
#define unlikely(cond) expr_unlikely(cond)
#define arrsize(arr) (sizeof(arr)/sizeof(*arr))
#define align(x) expr_align(x)
#define align_up(x) expr_align_up(x)
#define assume(cond) expr_assume(cond)
#define cast(X,T) expr_cast(X,T)
#define extget expr_extarg_get


#define BUFSIZE_INITIAL EXPR_BUFSIZE_INITIAL

#define xmalloc expr_xmalloc
#define xrealloc expr_xrealloc
#define xfree expr_xfree

#ifndef alloca
#define alloca(size) __builtin_alloca(size)
#endif

#define ctz64 expr_ctz64
#define clz64 expr_clz64
#define ffs64 expr_ffs64
#define ctz32 expr_ctz32
#define clz32 expr_clz32
#define ffs32 expr_ffs32
#define ctzz expr_ctzz
#define clzz expr_clzz
#define ffsz expr_ffsz

#define le16(x) expr_le(x,16)
#define le32(x) expr_le(x,32)
#define le64(x) expr_le(x,64)

#endif


#if (defined(__LONG_WIDTH__)&&(__LONG_WIDTH__==64))
#define expr_ctz64 __builtin_ctzl
#define expr_clz64 __builtin_clzl
#define expr_ffs64 __builtin_ffsl
#elif ((defined(__LLONG_WIDTH__)&&(__LLONG_WIDTH__==64))||(defined(__LONG_LONG_WIDTH__)&&(__LONG_LONG_WIDTH__==64)))
#define expr_ctz64 __builtin_ctzll
#define expr_clz64 __builtin_clzll
#define expr_ffs64 __builtin_ffsll
#elif (defined(__INT_WIDTH__)&&(__INT_WIDTH__==64))
#define expr_ctz64 __builtin_ctz
#define expr_clz64 __builtin_clz
#define expr_ffs64 __builtin_ffs
#else
#define expr_ctz64 __builtin_ctzg
#define expr_clz64 __builtin_clzg
#define expr_ffs64 __builtin_ffsg
#endif

#if (defined(__LONG_WIDTH__)&&(__LONG_WIDTH__==32))
#define expr_ctz32 __builtin_ctzl
#define expr_clz32 __builtin_clzl
#define expr_ffs32 __builtin_ffsl
#elif ((defined(__LLONG_WIDTH__)&&(__LLONG_WIDTH__==32))||(defined(__LONG_LONG_WIDTH__)&&(__LONG_LONG_WIDTH__==32)))
#define expr_ctz32 __builtin_ctzll
#define expr_clz32 __builtin_clzll
#define expr_ffs32 __builtin_ffsll
#elif (defined(__INT_WIDTH__)&&(__INT_WIDTH__==32))
#define expr_ctz32 __builtin_ctz
#define expr_clz32 __builtin_clz
#define expr_ffs32 __builtin_ffs
#else
#define expr_ctz32 __builtin_ctzg
#define expr_clz32 __builtin_clzg
#define expr_ffs32 __builtin_ffsg
#endif

#if (defined(__SIZE_WIDTH__)&&(__SIZE_WIDTH__==64))
#define expr_ctzz expr_ctz64
#define expr_clzz expr_clz64
#define expr_ffsz expr_ffs64
#elif (defined(__SIZE_WIDTH__)&&(__SIZE_WIDTH__==32))
#define expr_ctzz expr_ctz32
#define expr_clzz expr_clz32
#define expr_ffsz expr_ffs32
#else
#define expr_ctzz __builtin_ctzg
#define expr_clzz __builtin_clzg
#define expr_ffsz __builtin_ffsg
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
EXPR_IEQ,
EXPR_INE,
EXPR_EQ,
EXPR_NE,
EXPR_ANDL,
EXPR_ORL,
EXPR_XORL,
EXPR_NEXT,
EXPR_DIFF,
EXPR_OFF,
EXPR_GCD,
EXPR_LCM,
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

#define EXPR_SYMSET_INITIALIZER(mtl) {NULL,0,0,0,0,0,0,0,0,0,0,0,(mtl),0,0}
#define EXPR_MUTEX_INITIALIZER ((uint32_t)(0))

#ifndef EXPR_SYMLEN
#define EXPR_SYMLEN 64
#endif
//the value limits the length of symbol name in the errinfo.
//in some older versions, the value meant the maximal support length of a symbol name, which is "unlimited" now.

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
#define EXPR_EPURE 20
#define EXPR_ETNP 21
#define EXPR_EVZP 22
#define EXPR_EANT 23
#define EXPR_EUDE 24
#define EXPR_EREC 25

#define EXPR_CONSTANT 0
#define EXPR_VARIABLE 1
#define EXPR_FUNCTION 2
#define EXPR_MDFUNCTION 3
#define EXPR_MDEPFUNCTION 4
#define EXPR_ZAFUNCTION 5
#define EXPR_HOTFUNCTION 6
#define EXPR_ALIAS 7

//expr symbol flag
#define EXPR_SPURE 1
//a non-hot function has the PURE flag means the output value of it
//depends only by the input value,and no any side effect.for HOTFUNCTION
//and ALIAS it means a size argument is following the char * argument.
#define EXPR_SWRITEIP 2
//for MDEPFUNCTION,if it is called,the ->ip will be set to current ip.
#define EXPR_SPACKAGE 4
//for VARIABLE,means the value of it is an address of a
//multi-dimension function.for CONSTANT it means the target is a package.
#define EXPR_SNONBUILTIN 8
//for VARIABLE,means the value of it is an address of a
//multi-dimension function with expression argument.
//for CONSTANT with SF_PACKAGE it means the target is a non-builtin package.
#define EXPR_SUNSAFE 16
//a non-hot function has the flag means it may allow user to read/write
//the memory freely or make a system call.they are disabled in protected
//mode.
#define EXPR_SALLOWADDR 32
//only for a unsafe function,it means it is able and only able to accept
//a address of a VARIABLE as its argument in protected mode.

#define EXPR_SPMD EXPR_SPACKAGE
#define EXPR_SPME EXPR_SNONBUILTIN
#define EXPR_SPFUNC 12
#define EXPR_SPMASK (~12)

//expr keyword flag
#define EXPR_KSUBEXPR 1
#define EXPR_KSEPCOMMA 2
#define EXPR_KNOPROTECT 4

#define EXPR_RPUSH(_sp) (*((_sp)++))
#define EXPR_RPOP(_sp) (*(--(_sp)))
//R:reverse
#define EXPR_PUSH(_sp) (*(--(_sp)))
#define EXPR_POP(_sp) (*((_sp)++))

#define EXPR_BLANKS \
		'\t':\
		case '\r':\
		case '\v':\
		case '\f':\
		case '\n':\
		case '\b':\
		case ' '
#define expr_blank(c) ({\
	int _r;\
	switch(c){\
		case EXPR_BLANKS:\
			_r=1;\
			break;\
		default:\
			_r=0;\
			break;\
	}\
	_r;\
})
#define expr_equal(_a,_b) ({\
	double a,b,absa,absb,absamb;\
	int _r;\
	a=(_a);\
	b=(_b);\
	absa=a<0.0?-a:a;\
	absb=b<0.0?-b:b;\
	absamb=a<b?b-a:a-b;\
	if(absa>absb){\
		if(absa<=1.0)\
			_r=(absamb<=DBL_EPSILON);\
		else\
			_r=(absamb<=DBL_EPSILON*absa);\
	}else {\
		if(absb<=1.0)\
			_r=(absamb<=DBL_EPSILON);\
		else\
			_r=(absamb<=DBL_EPSILON*absb);\
	}\
	_r;\
})
#define EXPR_OPERATORS \
		'+':\
		case '-':\
		case '*':\
		case '/':\
		case '%':\
		case '^':\
		case '(':\
		case ')':\
		case ',':\
		case ';':\
		case '<':\
		case '>':\
		case '=':\
		case '!':\
		case '&':\
		case '|':\
		case '#':\
		case '[':\
		case ']':\
		case '{':\
		case '}'
#define expr_operator(c) ({\
	int _r;\
	switch(c){\
		case EXPR_OPERATORS:\
			_r=1;\
			break;\
		default:\
			_r=0;\
			break;\
	}\
	_r;\
})

#define expr_bswap(x,N) __builtin_bswap##N((uint##N##_t)(x))
#define expr_bswap16(x) __builtin_bswap16(x)
#define expr_bswap32(x) __builtin_bswap32(x)
#define expr_bswap64(x) __builtin_bswap64(x)

#define expr_gcd2(__x,__y) ({\
	expr_t _x,_y;\
	int r1;\
	_x=(__x);\
	_y=(__y);\
	r1=(expr_tabs(_x)<expr_tabs(_y));\
	while(expr_likely(_x!=EXPR_ZERO&&_y!=EXPR_ZERO)){\
		if(r1^=1)_x=expr_tmod(_x,_y);\
		else _y=expr_tmod(_y,_x);\
	}\
	assume(_x==EXPR_ZERO||_y==EXPR_ZERO);\
	_x!=EXPR_ZERO?_x:_y;\
})

#define expr_lcm2(__x,__y) ({\
	expr_t _a,_b;\
	_a=(__x);\
	_b=(__y);\
	_a*_b/expr_gcd2(_a,_b);\
})

#define expr_cast(x,type) \
	({\
		union {\
			__typeof__(x) _x;\
			__typeof__(type) _o;\
			intmax_t _im;\
		} _cast_un;\
		if(sizeof(type)>sizeof(__typeof__(x)))\
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

#define expr_allocator(size) ((xmtl)->allocate((size),(xmtl)->arg))
#define expr_reallocator(old,size) ((xmtl)->reallocate((old),(size),(xmtl)->arg))
#define expr_deallocator(old) ((xmtl)->deallocate((old),(xmtl)->arg))
// WARNING: xmtl will be evaluated twice, the caller should ensure no side effect.

#define expr_xmalloc(size) ({\
	expr_allocator(size);\
})
#define expr_xrealloc(old,size) ({\
	void *__old=(old);\
	__old?\
		 expr_reallocator(__old,(size)):\
		 expr_allocator(size);\
})
#define expr_xfree(old) ({\
	expr_deallocator(old);\
})

#define expr_align(x) (((x)+(EXPR_ALIGN-1))&~(EXPR_ALIGN-1))
#define expr_align_up(x) (((x)+(EXPR_ALIGN))&~(EXPR_ALIGN-1))
#define EXPR_ALIGN (sizeof(void *))
#define EXPR_MAGIC48_A UINT64_C(0x5deece66d)
#define EXPR_MAGIC48_B UINT64_C(0xb)

#define expr_ltod48(_l) (expr_cast((((_l)&INT64_C(0xffffffffffff))<<4)|INT64_C(0x3ff0000000000000),double)-1.0)
#define expr_ltol48(_l) ((int32_t)(((_l)&INT64_C(0xffffffff0000))>>17))
#define expr_ltom48(_l) ((int32_t)(((_l)&INT64_C(0xffffffff0000))>>16))
#define expr_lto48(_l) ({\
	int64_t __l=(_l);\
	(int64_t)((__l>>32)|((__l&INT64_C(0xffff0000)))|((__l&INT64_C(0xffff))<<32));\
})

#define expr_next48v(_val) ((int64_t)(((EXPR_MAGIC48_A)*(uint64_t)(_val)+(EXPR_MAGIC48_B))&UINT64_C(0xffffffffffff)))
#define expr_next48(_seedp) ({\
	int64_t *_next48_seed=(_seedp);\
	*_next48_seed=expr_next48v(*_next48_seed);\
})
#define expr_seed48(__val) (0x330e|(((__val)&INT64_C(0xffffffff))<<16))
#define expr_next48l32(_seedp) (expr_next48(_seedp)&INT64_C(0xffffffff))

#define expr_get48(_addr) ({\
	const uint16_t *_get48_addr=(_addr);\
	(uintptr_t)_get48_addr&3?\
	(int64_t)(*(const uint32_t *)(_get48_addr+1)<<16)|((int64_t)*_get48_addr):\
	(int64_t)*(const uint32_t *)_get48_addr|((int64_t)_get48_addr[2]<<32);\
})
#define expr_set48(_addr,_val) ({\
	uint16_t *_set48_addr=(_addr);\
	int64_t _set48_val=(_val);\
	if((uintptr_t)_set48_addr&3){\
		*(uint32_t *)(_set48_addr+1)=(uint32_t)(_set48_val>>16);\
		*_set48_addr=(uint16_t)(_set48_val);\
	}else {\
		*(uint32_t *)_set48_addr=(uint32_t)_set48_val;\
		_set48_addr[2]=(uint16_t)(_set48_val>>32);\
	}\
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
	__val&INT64_C(0xffffffffffff);\
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
	__val&INT64_C(0xffffffffffff);\
})

#define expr_symbol_hot(_esp) ({const struct expr_symbol *__esp=(_esp);(char *)(uintptr_t)(__esp->str+__esp->strlen+1);})
#define expr_symbol_hotlen(_esp) ({const struct expr_symbol *__esp=(_esp);(size_t)__esp->length-(size_t)__esp->strlen-sizeof(struct expr_symbol)-2;})
#define expr_symbol_un(_esp) ({const struct expr_symbol *__esp=(_esp);(union expr_symvalue *)(uintptr_t)(__esp->str+expr_align(__esp->strlen+1));})
#define expr_symbol_dim(_esp) ({const struct expr_symbol *__esp=(_esp);(size_t *)(uintptr_t)(__esp->str+expr_align(expr_align(__esp->strlen+1)+sizeof(union expr_symvalue)));})
#define expr_assume(cond) if(cond);else __builtin_unreachable()
#define expr_likely(cond) __builtin_expect(!!(cond),1)
#define expr_unlikely(cond) __builtin_expect(!!(cond),0)
#define expr_static_castable(value,_type) ((void)sizeof(((int (*)(_type))NULL)(value)))
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

#if __SIZEOF_POINTER__==8
#define expr_mkval(c) expr_mkval64(c)
#elif __SIZEOF_POINTER__==4
#define expr_mkval(c) expr_mkval32(c)
#elif __SIZEOF_POINTER__==2
#define expr_mkval(c) expr_mkval16(c)
#elif __SIZEOF_POINTER__==1
#define expr_mkval(c) ((uint8_t)(c))
#endif

#define expr_mkval16(c) ((uint16_t)(uint8_t)(c))*UINT16_C(0x0101)
#define expr_mkval32(c) ((uint32_t)(uint8_t)(c))*UINT32_C(0x01010101)
#define expr_mkval64(c) ((uint64_t)(uint8_t)(c))*UINT64_C(0x0101010101010101)
expr_static_assert(__builtin_constant_p(expr_mkval(0)));
#define expr_internal_memset(buf,val,size) expr_internal_memset64(buf,expr_mkval(val),size)
//#define expr_internal_memset(buf,val,size) ({if(__builtin_constant_p(val)&&!(val))expr_internal_memset64(buf,0,size);else expr_internal_memset64(buf,expr_mkval(val),size);})

#define expr_fake_memrmem(buf,size,c,c_size) ({\
	const char *___buf=(buf);\
	const char *___c=(c);\
	size_t ___c_size=(c_size);\
	size_t ___size=(size)-(___c_size-1);\
	const char *___r;\
	char ___ch=*(const char *)___c;\
	if(expr_likely(___c_size&&(ssize_t)___size>0)){\
		___ch=*___c;\
		do {\
			___r=memrchr(___buf,___ch,___size);\
			if(!___r||!memcmp(___r,___c,___c_size))\
				break;\
			___size=(size_t)(___r-___buf);\
		}while(expr_likely(___size));\
	}else\
		___r=NULL;\
	(void *)(uintptr_t)___r;\
})
#define expr_fake_memmem(buf,size,c,c_size) ({\
	const char *___r=(buf);\
	const char *___c=(c);\
	size_t ___c_size=(c_size);\
	size_t ___size=(size);\
	const char *___end=___r+___size;\
	char ___ch;\
	___size-=(___c_size-1);\
	if(expr_likely(___c_size&&(ssize_t)___size>0)){\
		___ch=*___c;\
		do {\
			___r=memchr(___r,___ch,___size);\
			if(!___r||!memcmp(___r,___c,___c_size))\
				break;\
			++___r;\
			___size=(size_t)(___end-___r);\
		}while(expr_likely(___size));\
	}else\
		___r=NULL;\
	(void *)(uintptr_t)___r;\
})
#define expr_ntable(c) ({\
	unsigned char __c=(unsigned char)(c);\
	switch(__c){\
		case '0' ... '9':\
			__c-=(unsigned char)'0';\
			break;\
		case 'A' ... 'Z':\
			__c-=(unsigned char)('A'-10);\
			break;\
		case 'a' ... 'z':\
			__c-=(unsigned char)('a'-10);\
			break;\
		default:\
			__c=(unsigned char)127;\
			break;\
	}\
	__c;\
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
	const struct expr_memtool *restrict mtl;
	union {
		uint64_t bit;
		struct {
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
	};
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

#define EXPR_FPLUS 230
#define EXPR_FSPACE 229
#define EXPR_FMINUS 228
#define EXPR_FSHARP 227
#define EXPR_FZERO 226
#define EXPR_FEQ 225
#define EXPR_FSAVED 224
#define EXPR_FADDR 223
#define EXPR_FCAP 222

expr_static_assert(offsetof(struct expr_writeflag,bit)+sizeof(uint64_t)==sizeof(struct expr_writeflag));
union expr_argf {
	void *addr;
	intptr_t sint;
	uintptr_t uint;
	double dbl;
	size_t zint;
	ssize_t tint;
	intptr_t *siaddr;
	uintptr_t *uiaddr;
	double *daddr;
	size_t *zaddr;
	ssize_t *taddr;
	const char *str;
	union expr_argf *aaddr;
	char ch;
#if __SIZEOF_POINTER__<8
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
typedef const union expr_argf *(*expr_argfetch)(ptrdiff_t index,const struct expr_writeflag *flag,intptr_t addr);
struct expr_writefmt {
	ssize_t (*converter)(expr_writer writer,intptr_t fd,const union expr_argf *arg,struct expr_writeflag *flag);
	uint8_t op[7];
	uint8_t type:2,no_arg:1,digit_check:1,setcap:1,unused:3;
};
struct expr_ftable {
	const struct expr_writefmt *fmts;
	uint8_t c[256];
	char fph,digit,bitwidth,readarg,readarg2;
	unsigned char fph_set:1,
		      digit_set:1,
		      bitwidth_set:1,
		      readarg_set:1,
		      readarg2_set:1,
		      unused_bf:sizeof(unsigned char)*8-5;
	char unused[2];
};
#define EXPR_TRUNC_CODE (SSIZE_MIN+128)

struct expr_memfd {
	void *buf;
	size_t size;
};

#define expr_iprintf(fmt,...) expr_idprintf(STDOUT_FILENO,fmt,##__VA_ARGS__)
#define expr_ieprintf(fmt,...) expr_idprintf(STDERR_FILENO,fmt,##__VA_ARGS__)
#define expr_idprintf(fd,fmt,...) expr_idwprintf(fd,write,fmt,##__VA_ARGS__)
#define expr_ibprintf(fd,fmt,...) expr_idwprintf(fd,expr_buffered_write,fmt,##__VA_ARGS__)
#define expr_ilprintf(fd,fmt,...) expr_idwprintf(fd,expr_buffered_write_iolbf_warped,fmt,##__VA_ARGS__)
#define expr_idwprintf(fd,writer,fmt,...) expr_apwritef((expr_writer)(uintptr_t)(writer),(intptr_t)(fd),fmt,sizeof(fmt)-1,##__VA_ARGS__)

#define EXPR_BZERO 1
#define EXPR_BTRUNC 2
#define EXPR_BEMPTY 4
#define EXPR_BEMEM 8
#define EXPR_BCALLBACK_PDMIN 8
#define EXPR_BTRUNC_NOREWRITE 1024

#define EXPR_BUFSIZE_INITIAL 512
struct expr_buffered_file {
	union {
		expr_reader reader;
		expr_writer writer;
		const void *uaddr;
	};
	intptr_t fd;
	void *buf;
	size_t index,length,dynamic,written;
	const struct expr_memtool *restrict mtl;
	int flag;
	int unused;
};
#define EXPR_BUFFERED_INITIALIZER_R(_wrer,_fd,_buf,_len,_mtl) {\
	.uaddr=(_wrer),\
	.fd=(_fd),\
	.buf=(_buf),\
	.length=(intptr_t)(_buf)?(_len):0,\
	.dynamic=(intptr_t)(_buf)?0:(_len),\
	.index=0,\
	.written=0,\
	.mtl=(_mtl),\
	.flag=0,\
	.unused=0,\
}
#define EXPR_BUFFERED_INITIALIZER(_wrer,_fd,_buf,_len) EXPR_BUFFERED_INITIALIZER_R(_wrer,_fd,_buf,_len,expr_defmtl)
#define expr_buffered_init_r(fp,_wrer,_fd,_buf,_len,_mtl) ({\
	struct expr_buffered_file *__fp=(fp);\
	__fp->uaddr=(const void *)(uintptr_t)(_wrer);\
	__fp->fd=(_fd);\
	if((__fp->buf=(_buf))){\
		__fp->length=(_len);\
		__fp->dynamic=0;\
	}else {\
		__fp->length=0;\
		__fp->dynamic=(_len);\
	}\
	__fp->mtl=(_mtl);\
	__fp->index=0;\
	__fp->written=0;\
	__fp->flag=0;\
})
#define expr_buffered_init(fp,_wrer,_fd,_buf,_len) expr_buffered_init_r(fp,_wrer,_fd,_buf,_len,expr_defmtl)
#define expr_buffered_drop(fp) ((fp)->index=0)
#define expr_buffered_rdrop(fp) ({\
	struct expr_buffered_file *__fp=(fp);\
	__fp->index=0;\
	__fp->written=0;\
})

#define expr_buffered_ainit_r(fp,_len,_mtl) ({\
	struct expr_buffered_file *__fp=(fp);\
	__fp->mtl=(_mtl);\
	__fp->buf=__fp->mtl->allocate(EXPR_BUFSIZE_INITIAL,__fp->mtl->arg);\
	if(expr_likely(__fp->buf)){\
		__fp->length=EXPR_BUFSIZE_INITIAL;\
		__fp->dynamic=(_len);\
		__fp->uaddr=NULL;\
		__fp->fd=0;\
		__fp->index=0;\
		__fp->written=0;\
		__fp->flag=0;\
	}\
	__fp->buf?0:-1;\
})
struct expr;
struct expr_symset;
struct expr_suminfo {
	struct expr *fromep,*toep,*stepep,*ep;
	volatile expr_t index;
};
struct expr_branchinfo {
	struct expr *cond,*body,*value;
};
struct expr_mdinfo {
	struct expr *eps;
	expr_t *args;
	char *e;
	union {
		expr_t (*func)(expr_t *,size_t);
		expr_t (*funcep)(const struct expr *,size_t,expr_t);
		void *uaddr;
	} un;
	size_t dim;
};
struct expr_hmdinfo {
	struct expr *eps;
	expr_t *args;
	struct expr *hotfunc;
	size_t dim;
};
struct expr_vmdinfo {
	struct expr *fromep,*toep,*stepep,*ep;
	size_t max;
	expr_t (*func)(expr_t *,size_t);
	expr_t *args;
	volatile expr_t index;
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
		expr_t *dst;
		double *ddst;
		expr_ti *idst;
		intptr_t *pdst;
		void *uaddr;
		void **uaddr2;
		expr_t **dst2;
		double **ddst2;
		struct expr_inst **instaddr2;
		struct expr_rawdouble *rdst;
		expr_t (**md2)(expr_t *,size_t);
		expr_t (**me2)(const struct expr *,size_t,expr_t);
	} dst;
	union {
		expr_t *src;
		double *dsrc;
		expr_ti *isrc;
		intptr_t *psrc;
		expr_t **src2;
		double **dsrc2;
		void *uaddr;
		void **uaddr2;
		expr_t value;
		double dvalue;
		intptr_t pvalue;
		size_t zu;
		struct expr *hotfunc;
		struct expr **hotfunc2;
		expr_t (*func)(expr_t);
		expr_t (**func2)(expr_t);
		expr_t (*zafunc)(void);
		expr_t (**zafunc2)(void);
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
	expr_t value;
	double dvalue;
	expr_ti ivalue;
	expr_tu uvalue;
	size_t size;
	ssize_t off;
	expr_t *addr;
	double *daddr;
	void *uaddr;
	const char *hot; //for builtin symbols only
	expr_t (*func)(expr_t);
	expr_t (*zafunc)(void);
	expr_t (*mdfunc)(expr_t *,size_t);
	expr_t (*mdepfunc)(const struct expr *,size_t,expr_t);
	const struct expr_builtin_symbol *bsyms;
	const struct expr_symset *sset;
};
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
		       //expr_symset_[re]insert() or expr_symset_remove()
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
	const struct expr_memtool *restrict mtl;
	uint32_t freeable,mutex;
};
struct expr_symset_infile {
	uint64_t size;
	uint32_t maxlen;
	char data[];
}__attribute__((packed));
#define EXPR_RDESTRUCTOR 1
struct expr_resource {
	struct expr_resource *next;
	union {
		void *uaddr;
		expr_t *addr;
		double *daddr;
		char *str;
		struct expr *ep;
	} un;
	int type,flag;
};

union expr_double {
	double val;
	int64_t ival;
	uint64_t uval;
	struct expr_rawdouble rd;
};

struct expr_callback {
	int (*before)(const struct expr *restrict ep,struct expr_inst *ip,intptr_t arg);
	int (*after)(const struct expr *restrict ep,struct expr_inst *ip,intptr_t arg);
	intptr_t arg;
};
typedef int (*expr_recursive_callback)(struct expr *restrict ep,void *arg);

#if (defined(EXPR_AREA_MINSIZE)&&(EXPR_AREA_MINSIZE))

#ifndef EXPR_AREA_ESSENTIAL_ONLY
#define EXPR_AREA_ESSENTIAL_ONLY 1
#endif

#ifndef EXPR_AREA_IGNORE_AZERO
#define EXPR_AREA_IGNORE_AZERO 1
#endif

#ifndef EXPR_AREA_NOMONOTONIC
#define EXPR_AREA_NOMONOTONIC 1
#endif

#ifndef EXPR_AREA_FORCEALIGN
#define EXPR_AREA_FORCEALIGN 1
#endif

#ifndef EXPR_AREA_FORCELAZY
#define EXPR_AREA_FORCELAZY 1
#endif

#ifndef EXPR_AREA_FORCENONAIL
#define EXPR_AREA_FORCENONAIL 1
#endif

#else

#ifndef EXPR_AREA_ESSENTIAL_ONLY
#define EXPR_AREA_ESSENTIAL_ONLY 0
#endif

#ifndef EXPR_AREA_IGNORE_AZERO
#define EXPR_AREA_IGNORE_AZERO 0
#endif

#ifndef EXPR_AREA_NOMONOTONIC
#define EXPR_AREA_NOMONOTONIC 0
#endif

#ifndef EXPR_AREA_FORCEALIGN
#define EXPR_AREA_FORCEALIGN 0
#endif

#ifndef EXPR_AREA_FORCELAZY
#define EXPR_AREA_FORCELAZY 0
#endif

#ifndef EXPR_AREA_FORCENONAIL
#define EXPR_AREA_FORCENONAIL 0
#endif

#endif

#if (!defined(__BIG_ENDIAN__)||!(__BIG_ENDIAN__))
#define expr_areaunit_bitfield(master,minor) struct {size_t master:sizeof(size_t)*8-1,minor:1;}
#else
#define expr_areaunit_bitfield(master,minor) struct {size_t minor:1,master:sizeof(size_t)*8-1;}
#endif
struct expr_zoneunit {
	union {
		expr_areaunit_bitfield(prev,tail);
		size_t unprev;
	};
	union {
		expr_areaunit_bitfield(size,deallocated);
		size_t unsize;
	};
};
struct expr_area {
	struct expr_zoneunit *data;
	struct expr_zoneunit *tail;
#if !(EXPR_AREA_NOMONOTONIC)
	unsigned int monotonic_allocate,
		     monotonic_allocate_fail,
		     monotonic_deallocate,
		     monotonic_expand_nail,
		     monotonic_expand_move,
		     monotonic_expand_fail,
		     monotonic_shrink,
		     monotonic_create,
		     monotonic_combine,
		     monotonic_retail_up,
		     monotonic_retail_down;
#else
	int unused[3];
#endif
	int flag;
};
//expr_static_assert(!(sizeof(struct expr_area)%sizeof(struct expr_zoneunit)));
struct expr_areainfo {
	size_t size;
	size_t unit_count;
	size_t leak;
	size_t leak_count;
	size_t leak_max;
	size_t free;
	size_t free_count;
	size_t free_max;
	size_t tail_index;
	size_t tail_size;
	size_t max;
};
#define EXPR_AZERO 1
#define EXPR_ANOALIGN 2
#define EXPR_ALAZY 4
#define EXPR_ANAIL 8
#define EXPR_ADYNAMICALIGN 16
#define EXPR_ALAZYEX 32
#define EXPR_ASTARTADDRALIGN 64

#define EXPR_ALAZY_ALL (EXPR_ALAZY|EXPR_ALAZYEX)
#define EXPR_ADEFAULT (EXPR_ALAZY_ALL|EXPR_ASTARTADDRALIGN)
#define EXPR_AALIGN_SHIFT 7
// if the EXPR_ADYNAMICALIGN is set:
// (flag>>EXPR_AALIGN_SHIFT) must be nonzero, or the undefined behaviour(divide by 0) will occur.
// if (flag>>EXPR_AALIGN_SHIFT) cannot be divided by sizeof(struct expr_zoneunit), the align of memory may be broken.
#define expr_zoneof(heap) ((void *)((uintptr_t)(heap)+sizeof(struct expr_area)))
#define expr_memlen(p) ({expr_static_castable((p),const void *);((const struct expr_zoneunit *)((uintptr_t)(p)-sizeof(struct expr_zoneunit)))->size;})

#define EXPR_HEAPMTL_INITIALIZER(heap) {\
	.allocate=expr_heapmtl_malloc,\
	.reallocate=expr_heapmtl_realloc,\
	.deallocate=expr_heapmtl_dealloc,\
	.test=NULL,\
	.arg=(intptr_t)(heap),\
}

struct expr_poolarea {
	struct expr_area area[1];
	struct expr_poolarea *prev,*next;
	char zone[];
};
struct expr_pool {
	struct expr_poolarea *head,*tail;
	void *(*sys_allocate)(size_t size,intptr_t arg);
	void (*sys_deallocate)(void *old,intptr_t arg);
	intptr_t sys_arg;
	size_t allocate_size;
	int flag,unused;
};
#define expr_pool_init(pool,allocate,deallocate,arg,area_size,area_flag) ({\
	struct expr_pool *_pool=(pool);\
	_pool->head=NULL;\
	_pool->tail=NULL;\
	_pool->sys_allocate=(allocate);\
	_pool->sys_deallocate=(deallocate);\
	_pool->sys_arg=(arg);\
	_pool->allocate_size=(area_size);\
	_pool->flag=(area_flag);\
})

#define EXPR_POOL_INITIALIZER(pool,allocate,deallocate,arg,area_size,area_flag) {\
	.head=NULL,\
	.tail=NULL,\
	.sys_allocate=(allocate),\
	.sys_deallocate=(deallocate),\
	.sys_arg=(arg),\
	.allocate_size=(area_size),\
	.flag=(area_flag),\
}

struct expr_strtod_workspace {
	uint64_t dbuf[96];
	union {
		char iabuf[1536];
		struct {
			uint64_t dfrac[96];
			uint64_t workspace[96];
		};
	};
};

#define expr_symbols_all \
	expr_symbols_default,\
	expr_symbols_expr,\
	expr_symbols_common,\
	expr_symbols_syscall,\
	expr_symbols_superseed48,\
	expr_symbols_string,\
	expr_symbols_symset,\
	expr_symbols_memory,\
	NULL
#define expr_symbols_ess \
	expr_symbols_default,\
	expr_symbols_packages,\
	NULL

//#undef __aarch64__
//#undef __unix__
//#undef __linux__

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
#define EXPR_CLOB0 "x1","memory","cc"
#define EXPR_CLOB1 "x1","memory","cc"
#define EXPR_CLOB2 "memory","cc"
#define EXPR_CLOB3 "memory","cc"
#define EXPR_CLOB4 "memory","cc"
#define EXPR_CLOB5 "memory","cc"
#define EXPR_CLOB6 "memory","cc"
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
#define EXPR_CLOB0 "rcx","r11","memory","cc"
#define EXPR_CLOB1 "rcx","r11","memory","cc"
#define EXPR_CLOB2 "rcx","r11","memory","cc"
#define EXPR_CLOB3 "rcx","r11","memory","cc"
#define EXPR_CLOB4 "rcx","r11","memory","cc"
#define EXPR_CLOB5 "rcx","r11","memory","cc"
#define EXPR_CLOB6 "rcx","r11","memory","cc"
#define EXPR_ARCHN "x86_64"
#else
#define EXPR_SYSAM 0
#define EXPR_ARCHN "unknown_arch"
#if !(defined(EXPR_NOSYSCALL)&&(EXPR_NOSYSCALL))
#pragma message("internal syscall is not implemented")
#endif
#endif

#endif

#if defined(EXPR_SYSIN)&&!(defined(EXPR_NOSYSCALL)&&(EXPR_NOSYSCALL))
#define expr_internal_syscalln(N,num,a0,a1,a2,a3,a4,a5,a6) expr_internal_syscall_reg##N(num,a0,a1,a2,a3,a4,a5,a6,EXPR_SYSID,EXPR_SYSA0,EXPR_SYSA1,EXPR_SYSA2,EXPR_SYSA3,EXPR_SYSA4,EXPR_SYSA5,EXPR_SYSA6,EXPR_SYSRE,EXPR_SYSE0,EXPR_SYSE1,EXPR_SYSE2,EXPR_SYSE3,EXPR_SYSE4,EXPR_SYSE5,EXPR_SYSE6,EXPR_SYSE7)
#else
#define expr_internal_syscall_void_reg7(num,a0,a1,a2,a3,a4,a5,a6,ID,A0,A1,A2,A3,A4,A5,A6,RE,E0,E1,E2,E3,E4,E5,E6,E7) ((void)(num),(void)(a0),(void)(a1),(void)(a2),(void)(a3),(void)(a4),(void)(a5),(void)(a6),(intptr_t)0)
#define expr_internal_syscall_void_reg6(num,a0,a1,a2,a3,a4,a5,a6,ID,A0,A1,A2,A3,A4,A5,A6,RE,E0,E1,E2,E3,E4,E5,E6,E7) ((void)(num),(void)(a0),(void)(a1),(void)(a2),(void)(a3),(void)(a4),(void)(a5),(intptr_t)0)
#define expr_internal_syscall_void_reg5(num,a0,a1,a2,a3,a4,a5,a6,ID,A0,A1,A2,A3,A4,A5,A6,RE,E0,E1,E2,E3,E4,E5,E6,E7) ((void)(num),(void)(a0),(void)(a1),(void)(a2),(void)(a3),(void)(a4),(intptr_t)0)
#define expr_internal_syscall_void_reg4(num,a0,a1,a2,a3,a4,a5,a6,ID,A0,A1,A2,A3,A4,A5,A6,RE,E0,E1,E2,E3,E4,E5,E6,E7) ((void)(num),(void)(a0),(void)(a1),(void)(a2),(void)(a3),(intptr_t)0)
#define expr_internal_syscall_void_reg3(num,a0,a1,a2,a3,a4,a5,a6,ID,A0,A1,A2,A3,A4,A5,A6,RE,E0,E1,E2,E3,E4,E5,E6,E7) ((void)(num),(void)(a0),(void)(a1),(void)(a2),(intptr_t)0)
#define expr_internal_syscall_void_reg2(num,a0,a1,a2,a3,a4,a5,a6,ID,A0,A1,A2,A3,A4,A5,A6,RE,E0,E1,E2,E3,E4,E5,E6,E7) ((void)(num),(void)(a0),(void)(a1),(intptr_t)0)
#define expr_internal_syscall_void_reg1(num,a0,a1,a2,a3,a4,a5,a6,ID,A0,A1,A2,A3,A4,A5,A6,RE,E0,E1,E2,E3,E4,E5,E6,E7) ((void)(num),(void)(a0),(intptr_t)0)
#define expr_internal_syscall_void_reg0(num,a0,a1,a2,a3,a4,a5,a6,ID,A0,A1,A2,A3,A4,A5,A6,RE,E0,E1,E2,E3,E4,E5,E6,E7) ((void)(num),(intptr_t)0)
#define expr_internal_syscalln(N,num,a0,a1,a2,a3,a4,a5,a6) expr_internal_syscall_void_reg##N(num,a0,a1,a2,a3,a4,a5,a6,EXPR_SYSID,EXPR_SYSA0,EXPR_SYSA1,EXPR_SYSA2,EXPR_SYSA3,EXPR_SYSA4,EXPR_SYSA5,EXPR_SYSA6,EXPR_SYSRE,EXPR_SYSE0,EXPR_SYSE1,EXPR_SYSE2,EXPR_SYSE3,EXPR_SYSE4,EXPR_SYSE5,EXPR_SYSE6,EXPR_SYSE7)
#endif

#define expr_internal_syscall_sel(_n,_0,_1,_2,_3,_4,_5,_6,_name,...) _name
#define expr_internal_syscall(...) expr_internal_syscall_sel(__VA_ARGS__,expr_internal_syscall7,expr_internal_syscall6,expr_internal_syscall5,expr_internal_syscall4,expr_internal_syscall3,expr_internal_syscall2,expr_internal_syscall1,expr_internal_syscall0)(__VA_ARGS__)

#define expr_internal_syscall0(num) expr_internal_syscalln(0,num,,,,,,,)
#define expr_internal_syscall1(num,a0) expr_internal_syscalln(1,num,a0,,,,,,)
#define expr_internal_syscall2(num,a0,a1) expr_internal_syscalln(2,num,a0,a1,,,,,)
#define expr_internal_syscall3(num,a0,a1,a2) expr_internal_syscalln(3,num,a0,a1,a2,,,,)
#define expr_internal_syscall4(num,a0,a1,a2,a3) expr_internal_syscalln(4,num,a0,a1,a2,a3,,,)
#define expr_internal_syscall5(num,a0,a1,a2,a3,a4) expr_internal_syscalln(5,num,a0,a1,a2,a3,a4,,)
#define expr_internal_syscall6(num,a0,a1,a2,a3,a4,a5) expr_internal_syscalln(6,num,a0,a1,a2,a3,a4,a5,)
#define expr_internal_syscall7(num,a0,a1,a2,a3,a4,a5,a6) expr_internal_syscalln(7,num,a0,a1,a2,a3,a4,a5,a6,)

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
		:EXPR_CLOB7);\
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
		:EXPR_CLOB6);\
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
		:EXPR_CLOB5);\
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
		:EXPR_CLOB4);\
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
		:EXPR_CLOB3);\
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
		:EXPR_CLOB2);\
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
		:EXPR_CLOB1);\
		expr_combine(__r_,RE);\
})
#define expr_internal_syscall_reg0(num,a0,a1,a2,a3,a4,a5,a6,ID,A0,A1,A2,A3,A4,A5,A6,RE,E0,E1,E2,E3,E4,E5,E6,E7) ({\
		expr_internal_regvarr(ID)=(intptr_t)(num);\
		E0;\
		__asm__ volatile(\
		EXPR_SYSIN \
		:"=r"(expr_combine(__r_,RE))\
		:"r"(expr_combine(__r_,ID))\
		:EXPR_CLOB0);\
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

#if (defined(EXPR_EVAL_MINSIZE)&&(EXPR_EVAL_MINSIZE))

#ifndef EXPR_VAEVAL
#define EXPR_VAEVAL 0
#endif

#ifndef EXPR_DEBUGEVAL
#define EXPR_DEBUGEVAL 0
#endif

#else

#ifndef EXPR_VAEVAL
#define EXPR_VAEVAL 1
#endif

#ifndef EXPR_DEBUGEVAL
#define EXPR_DEBUGEVAL 1
#endif

#endif

#ifndef EXPR_EVAL2
#define EXPR_EVAL2 1
#endif

#ifndef EXPR_EVAL1
#define EXPR_EVAL1 0
#endif

#define EXPR_RECURSIVE_DEFAULT 8

//expr initial flag
#define EXPR_INOOPTIMIZE 1
#define EXPR_IINSTANT_FREE 2
#define EXPR_IPURE 4
#define EXPR_INOKEYWORD 8
#define EXPR_IPROTECT 16
#define EXPR_IREMOVEBLANK 32
#define EXPR_IRECURSIVELIMIT 64
#define EXPR_IKEEPSYMSET 128
#define EXPR_IDETACHSYMSET 256
#define EXPR_IUNSAFE 512
#define EXPR_INOSHRINK 1024
#define EXPR_ISCANCACHE 2048
#define EXPR_IABSORBSYMSET 4096


#define EXPR_IEXTEND_MASK (EXPR_IINSTANT_FREE|EXPR_IABSORBSYMSET)
#define EXPR_ISETABLE (EXPR_IPURE|EXPR_INOKEYWORD|EXPR_IPROTECT|EXPR_ISCANCACHE)

#define EXPR_IDEFAULT (EXPR_IPROTECT)

struct expr {
	struct expr_inst *data;
	size_t size,length;
	struct expr *parent;
	struct expr_symset *sset;
	struct expr_resource *res,*tail;
	expr_t **vars;
	size_t vsize,vlength;
	const struct expr_memtool *restrict mtl;
	// the restrict in field may be ignored, might work in the future
	union {
		expr_t args[EXPR_SYSAM];
		union {
			struct {
				struct expr_inst endinst[1];
				expr_t val;
			} end[1];
			struct expr_inst **ipp;
		};
		char errinfo[expr_align(EXPR_SYMLEN)];
	};
	int error;
	unsigned int recursive,iflag;
	unsigned int input_count:8,noinput:1,freeable:1,sset_shouldfree:1,isconst:1,unused:sizeof(unsigned int)*8-12;
};

struct expr_extarg {
	unsigned int recursive;
	unsigned int argc:8,unused:sizeof(unsigned int)*8-8;
};
#define EXPR_EXTARG_DEFAULT {\
	.recursive=EXPR_RECURSIVE_DEFAULT,\
	.argc=0,\
	.unused=0,\
}

#define expr_extarg_get(argp,field) (((const struct expr_extarg *)(argp))->field)


//global functions of expr-alloc.c :
int expr_area_init4(struct expr_area *restrict area,void *zone,size_t size,int flag);
void expr_area_dealloc(struct expr_area *restrict area,void *old);
void *expr_area_realloc4(struct expr_area *restrict area,void *old,size_t size,int flag);
void expr_area_wipe(struct expr_area *restrict area);
void expr_area_wipe_monotonic(struct expr_area *restrict area);
void expr_area_reset_monotonic(struct expr_area *restrict area);
int expr_area_resize(struct expr_area *restrict area,size_t size);
int expr_area_summary(const struct expr_area *restrict area,struct expr_areainfo *restrict info);
void *expr_heapmtl_malloc(size_t size,intptr_t arg);
void *expr_heapmtl_realloc(void *old,size_t size,intptr_t arg);
void expr_heapmtl_dealloc(void *old,intptr_t arg);
int expr_setup_heapmtl4(struct expr_memtool *restrict mtl,void *heap,size_t size,int flag);
//global externs of expr-alloc.c :
//global functions of expr-buffered.c :
ssize_t expr_buffered_write(struct expr_buffered_file *restrict fp,const void *buf,size_t size);
ssize_t expr_buffered_write_warped(intptr_t fd,const void *buf,size_t size);
ssize_t expr_buffered_read(struct expr_buffered_file *restrict fp,void *buf,size_t size);
ssize_t expr_buffered_write_flushatc(struct expr_buffered_file *restrict fp,const void *buf,size_t size,int c);
ssize_t expr_buffered_write_flushatt(struct expr_buffered_file *restrict fp,const void *buf,size_t size,expr_test_t test,intptr_t arg);
ssize_t expr_buffered_write_flushat(struct expr_buffered_file *restrict fp,const void *buf,size_t size,const void *c,size_t c_size);
ssize_t expr_buffered_write_sflushatc(struct expr_buffered_file *restrict fp,const void *buf,size_t size,int c);
ssize_t expr_buffered_write_sflushatt(struct expr_buffered_file *restrict fp,const void *buf,size_t size,expr_test_t test,intptr_t arg);
ssize_t expr_buffered_write_sflushat(struct expr_buffered_file *restrict fp,const void *buf,size_t size,const void *c,size_t c_size);
ssize_t expr_buffered_write_sync(struct expr_buffered_file *restrict fp,const void *buf,size_t size);
ssize_t expr_buffered_flush(struct expr_buffered_file *restrict fp);
ssize_t expr_buffered_rdropall(struct expr_buffered_file *restrict fp);
ssize_t expr_buffered_close(struct expr_buffered_file *restrict fp);
void expr_buffered_rclose(struct expr_buffered_file *restrict fp);
ssize_t expr_buffered_readline(struct expr_buffered_file *restrict fp,int c,void *savep);
ssize_t expr_buffered_write_iolbf(struct expr_buffered_file *restrict fp,const void *buf,size_t size);
ssize_t expr_buffered_write_iolbf_warped(intptr_t fd,const void *buf,size_t size);
//global externs of expr-buffered.c :
//global functions of expr-core.c :
const char *expr_error(int error);
const struct expr_builtin_keyword *expr_keyword_search(const char *str,size_t len,const struct expr_builtin_keyword *restrict kp,size_t n);
void expr_free2mtl_r(struct expr *restrict ep,int flag,const struct expr_memtool *restrict mtl);
void expr_free2(struct expr *restrict ep,int flag);
void expr_free(struct expr *restrict ep);
const char *expr_findpair(const char *c,const char *endp,int oth);
int expr_isconst(const struct expr *restrict ep);
void expr_init_const_r(struct expr *restrict ep,expr_t val,const struct expr_memtool *restrict mtl);
struct expr *expr_new_const_r(expr_t val,const struct expr_memtool *restrict mtl);
void expr_shink(struct expr *restrict ep);
int expr_init8_r(struct expr *restrict ep,const char *e,size_t len,const char *asym,size_t asymlen,struct expr_symset *esp,int flag,const void *extarg,const struct expr_memtool *restrict mtl);
struct expr *expr_new9_r(const char *e,size_t len,const char *asym,size_t asymlen,struct expr_symset *esp,int flag,int *error,char errinfo[EXPR_SYMLEN],const void *extarg,const struct expr_memtool *restrict mtl);
expr_t expr_calc5_r(const char *e,int *error,char errinfo[EXPR_SYMLEN],struct expr_symset *esp,int flag,const struct expr_memtool *restrict mtl);
size_t expr_varofep(const struct expr *restrict ep,expr_t *v);
int expr_recursive(struct expr *restrict ep,expr_recursive_callback callback,void *arg);
int expr_optimize(struct expr *restrict ep);
int expr_optimize_recursive(struct expr *restrict ep);
expr_t expr_eval(const struct expr *restrict ep,expr_t input);
expr_t expr_vevalx(const struct expr *restrict ep,va_list ap);
expr_t expr_evalx(const struct expr *restrict ep,...);
expr_t expr_eval1(const struct expr *restrict ep);
expr_t expr_eval_callback(const struct expr *restrict ep,expr_t input,const struct expr_callback *ec);
//global externs of expr-core.c :
extern const struct expr_builtin_keyword expr_keywords[];
extern const size_t expr_keywords_size;
extern const struct expr_extarg expr_extarg_default[1];
//global functions of expr-format.c :
ssize_t expr_writec_r(expr_writer writer,intptr_t fd,size_t count,int c,const struct expr_memtool *restrict mtl);
ssize_t expr_converter_common(expr_writer writer,intptr_t fd,const void *buf,size_t size,const struct expr_writeflag *flag);
ssize_t expr_vwriteft_r(expr_writer writer,intptr_t fd,const char *restrict fmt,size_t fmtlen,expr_argfetch arg,intptr_t addr,const struct expr_ftable *restrict table,const struct expr_memtool *restrict mtl);
ssize_t expr_writeft_r(expr_writer writer,intptr_t fd,const char *restrict fmt,size_t fmtlen,const union expr_argf *restrict args,size_t arglen,const struct expr_ftable *restrict table,const struct expr_memtool *restrict mtl);
ssize_t expr_vapwriteft_r(expr_writer writer,intptr_t fd,const char *restrict fmt,size_t fmtlen,const struct expr_ftable *restrict table,const struct expr_memtool *restrict mtl,va_list ap);
ssize_t expr_apwriteft_r(expr_writer writer,intptr_t fd,const char *restrict fmt,size_t fmtlen,const struct expr_ftable *restrict table,const struct expr_memtool *restrict mtl,...);
ssize_t expr_snwriteft_r(char *buf,size_t buflen,const char *restrict fmt,size_t fmtlen,const union expr_argf *restrict args,size_t arglen,const struct expr_ftable *restrict table,const struct expr_memtool *restrict mtl);
ssize_t expr_vsnprintft_r(char *buf,size_t buflen,const char *restrict fmt,size_t fmtlen,const struct expr_ftable *restrict table,const struct expr_memtool *restrict mtl,va_list ap);
ssize_t expr_snprintft_r(char *buf,size_t buflen,const char *restrict fmt,size_t fmtlen,const struct expr_ftable *restrict table,const struct expr_memtool *restrict mtl,...);
size_t expr_sscanf_r(const char *str,size_t len,const char *fmt,size_t fmtlen,void *const *addr,size_t addrlen,const struct expr_memtool *restrict mtl);
//global externs of expr-format.c :
extern const struct expr_writefmt expr_writefmts_default[];
extern const uint8_t expr_writefmts_default_size;
extern const struct expr_ftable expr_ftable_default[1];
//global functions of expr-internal.c :
size_t expr_strscan(const char *restrict s,size_t sz,char *restrict buf,size_t outsz);
void expr_contract(void *buf,size_t size);
void expr_explode_r(void (*contractor)(void *,size_t),size_t max,const struct expr_memtool *restrict mtl);
size_t expr_extint_left(uint64_t *buf,size_t size,uint64_t bits);
size_t expr_extint_right(uint64_t *buf,size_t size,uint64_t bits);
size_t expr_extint_add(uint64_t *buf,uint64_t addend);
size_t expr_extint_addz(uint64_t *buf,size_t size,uint64_t addend);
size_t expr_extint_add2(uint64_t *buf,const uint64_t *addend_buf,size_t size);
size_t expr_extint_orindex(uint64_t *buf,size_t size,uint64_t index);
size_t expr_extint_sub(uint64_t *buf,uint64_t subtractor);
void expr_extint_subnrv(uint64_t *buf,uint64_t subtractor);
size_t expr_extint_subz(uint64_t *buf,size_t size,uint64_t subtractor);
size_t expr_extint_sub2(uint64_t *buf,size_t size,const uint64_t *subtractor_buf,size_t subsize);
int expr_extint_cmp(const uint64_t *restrict buf,const uint64_t *restrict buf1,size_t size,size_t *index);
int expr_extint_cmpz(const uint64_t *restrict buf,size_t size,const uint64_t *restrict buf1,size_t buf1size);
int expr_extint_cmpsub(uint64_t *restrict buf,size_t size,const uint64_t *restrict buf1,size_t buf1size,size_t *outsize);
size_t expr_extint_mul(uint64_t *buf,size_t size,uint32_t factor,uint64_t *workspace);
size_t expr_extint_mul2(uint64_t *buf,size_t bufsize,const uint64_t *factor_buf,size_t factor_size,uint64_t *workspace);
size_t expr_extint_div(uint64_t *buf,size_t size,uint32_t divisor,uint32_t *mod);
size_t expr_extint_div2(uint64_t *buf,size_t size,uint64_t *divisor_buf,size_t divisor_size,uint64_t *out,size_t *outsize);
size_t expr_extint_mulnp(uint64_t *buf,size_t size,uint32_t factor,uint32_t power,uint64_t *workspace);
size_t expr_extint_ascii(uint64_t *buf,size_t size,const char *chars,char *outbuf,uint32_t base);
size_t expr_extint_ascii_rev(uint64_t *buf,size_t size,const char *chars,char *outbuf,uint32_t base);
size_t expr_extint_ascii_convert(uint64_t *buf,const char *inbuf,size_t inbuf_size,uint32_t base,uint64_t *workspace);
ssize_t expr_internal_strtou(const char *restrict nptr,size_t nsize,size_t *restrict end_index,int base);
ssize_t expr_internal_strtoz(const char *restrict nptr,size_t nsize,size_t *restrict end_index,int base);
double expr_internal_strtod4(const char *restrict nptr,size_t nsize,size_t *restrict end_index,struct expr_strtod_workspace *restrict ws);
double expr_internal_strtod_mtl_force(const char *restrict nptr,size_t nsize,size_t *restrict end_index,const struct expr_memtool *restrict mtl);
double expr_internal_trunc(double x);
double expr_internal_fmod(double x,double y);
double expr_internal_fand(double x,double y);
double expr_internal_for(double x,double y);
double expr_internal_fxor(double x,double y);
double expr_internal_sqrt(double x);
uint64_t expr_internal_sqrt128(uint64_t low,uint64_t high);
double expr_internal_pow(double x,double y);
double expr_internal_fabs(double x);
void expr_internal_memset64(void *buf,uintptr_t val,size_t size);
void expr_internal_memcpy(void *dst,const void *src,size_t size);
void expr_internal_memswap(void *dst,void *src,size_t size);
void expr_internal_memmove(void *dst,const void *src,size_t size);
void *expr_internal_memchr(const void *buf,int c,size_t size);
void *expr_internal_memrchr(const void *buf,int c,size_t size);
int expr_internal_memcmp(const void *s0,const void *s1,size_t size);
void *expr_internal_memchr2(const void *buf,int c);
struct expr_symbol *expr_builtin_symbol_add(struct expr_symset *restrict esp,const struct expr_builtin_symbol *p);
ssize_t expr_builtin_symbol_xaddalls(struct expr_symset *restrict esp,const struct expr_builtin_symbol **symsp,const struct expr_builtin_symbol *syms,...);
ssize_t expr_builtin_symbol_addalls(struct expr_symset *restrict esp,const struct expr_builtin_symbol *syms,...);
void expr_symset_init_r(struct expr_symset *restrict esp,const struct expr_memtool *restrict mtl);
struct expr_symset *expr_symset_new_r(const struct expr_memtool *restrict mtl);
void expr_symset_freemtl(struct expr_symset *restrict esp,const struct expr_memtool *restrict mtl);
void expr_symset_freemtl_s(struct expr_symset *restrict esp,void *stack,const struct expr_memtool *restrict mtl);
void expr_symset_free(struct expr_symset *restrict esp);
void expr_symset_free_s(struct expr_symset *restrict esp,void *stack);
void expr_symset_wipe(struct expr_symset *restrict esp);
void expr_symset_wipe_s(struct expr_symset *restrict esp,void *stack);
struct expr_symbol **expr_symset_findtail(struct expr_symset *restrict esp,const char *sym,size_t symlen,size_t *depth);
struct expr_symbol *expr_symset_add(struct expr_symset *restrict esp,const char *sym,int type,int flag,...);
struct expr_symbol *expr_symset_addl(struct expr_symset *restrict esp,const char *sym,size_t symlen,int type,int flag,...);
struct expr_symbol *expr_symset_vadd(struct expr_symset *restrict esp,const char *sym,int type,int flag,va_list ap);
struct expr_symbol *expr_symset_vaddl(struct expr_symset *restrict esp,const char *sym,size_t symlen,int type,int flag,va_list ap);
struct expr_symbol *expr_symset_addcopy(struct expr_symset *restrict esp,const struct expr_symbol *restrict es);
struct expr_symbol **expr_symset_search0(const struct expr_symset *restrict esp,const char *sym,size_t sz);
struct expr_symbol *expr_symset_search(const struct expr_symset *restrict esp,const char *sym,size_t sz);
struct expr_symbol *expr_symset_searchd(const struct expr_symset *restrict esp,const char *sym,size_t sz,size_t *depth);
int expr_symbol_check(const struct expr_symbol *restrict esp);
int expr_symset_insert(struct expr_symset *restrict esp,struct expr_symbol *restrict es);
void expr_symset_reinsert(struct expr_symset *restrict esp,struct expr_symbol *restrict sp1);
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
size_t expr_symset_copy(struct expr_symset *restrict dst,const struct expr_symset *restrict src);
size_t expr_symset_copy_s(struct expr_symset *restrict dst,const struct expr_symset *restrict src,void *stack);
struct expr_symset *expr_symset_clone(const struct expr_symset *restrict esp);
struct expr_symset *expr_symset_clone_s(const struct expr_symset *restrict esp,void *stack);
struct expr_symbol *expr_symbol_create_r(const char *sym,int type,int flag,const struct expr_memtool *restrict mtl,...);
struct expr_symbol *expr_symbol_createl_r(const char *sym,size_t symlen,int type,int flag,const struct expr_memtool *restrict mtl,...);
struct expr_symbol *expr_symbol_vcreate_r(const char *sym,int type,int flag,const struct expr_memtool *restrict mtl,va_list ap);
struct expr_symbol *expr_symbol_vcreatel_r(const char *sym,size_t symlen,int type,int flag,const struct expr_memtool *restrict mtl,va_list ap);
//global externs of expr-internal.c :
extern const uint8_t expr_number_table[256];
//global functions of expr-pool.c :
void *expr_pool_malloc(struct expr_pool *restrict pool,size_t size);
void expr_pool_dealloc(struct expr_pool *restrict pool,void *old);
void *expr_pool_realloc(struct expr_pool *restrict pool,void *old,size_t size);
void expr_pool_wipe(struct expr_pool *restrict pool);
void *expr_poolmtl_malloc(size_t size,intptr_t arg);
void *expr_poolmtl_realloc(void *old,size_t size,intptr_t arg);
void expr_poolmtl_dealloc(void *old,intptr_t arg);
void expr_setup_poolmtl(struct expr_memtool *restrict mtl,struct expr_pool *pool);
//global externs of expr-pool.c :
//global functions of expr-symext.c :
const struct expr_builtin_symbol *expr_builtin_symbol_search(const struct expr_builtin_symbol *syms,const char *sym,size_t sz);
const struct expr_builtin_symbol *expr_builtin_symbol_rsearch(const struct expr_builtin_symbol *syms,void *addr);
struct expr_symset *expr_builtin_symbol_converts_r(const struct expr_memtool *restrict mtl,const struct expr_builtin_symbol *syms,...);
struct expr_symset *expr_builtin_symbol_convert_r(const struct expr_builtin_symbol *syms,const struct expr_memtool *restrict mtl);
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
void expr_symset_save(struct expr_symset *restrict esp,void *buf);
void expr_symset_save_s(struct expr_symset *restrict esp,void *buf,void *stack);
void expr_symset_move(struct expr_symset *restrict esp,ptrdiff_t off);
void expr_symset_move_s(struct expr_symset *restrict esp,ptrdiff_t off,void *stack);
ssize_t expr_symset_write(const struct expr_symset *restrict esp,expr_writer writer,intptr_t fd);
ssize_t expr_symset_write_s(const struct expr_symset *restrict esp,expr_writer writer,intptr_t fd,void *stack);
ssize_t expr_symset_read(struct expr_symset *restrict esp,const void *buf,size_t size);
ssize_t expr_symset_readfd(struct expr_symset *restrict esp,expr_reader reader,intptr_t fd);
//global externs of expr-symext.c :
//global functions of expr-builtin.c :
uint64_t expr_gcd64(uint64_t x,uint64_t y);
void expr_mirror(double *buf,size_t size);
void expr_memfry48(void *restrict buf,size_t size,size_t n,int64_t seed);
void expr_fry(double *restrict v,size_t n);
int expr_sort4_r(double *restrict v,size_t n,const struct expr_memtool *restrict mtl);
void expr_sortq(double *restrict v,size_t n);
void expr_sort_old(double *restrict v,size_t n);
void expr_sort(double *v,size_t n);
double expr_exp_old(double x);
double expr_multilevel_derivate(const struct expr *ep,double input,long level,double epsilon);
double expr_and2(double x,double y);
double expr_or2(double x,double y);
double expr_xor2(double x,double y);
double expr_not(double x);
//global externs of expr-builtin.c :
extern const struct expr_builtin_symbol expr_symbols_default[];
extern const struct expr_builtin_symbol expr_symbols_packages[];
extern const struct expr_builtin_symbol expr_symbols_expr[];
extern const struct expr_builtin_symbol expr_symbols_common[];
extern const struct expr_builtin_symbol expr_symbols_syscall[];
extern const struct expr_builtin_symbol expr_symbols_superseed48[];
extern const struct expr_builtin_symbol expr_symbols_memory[];
extern const struct expr_builtin_symbol expr_symbols_symset[];
extern const struct expr_builtin_symbol expr_symbols_string[];
//global functions of expr-global.c :
double expr_internal_strtod(const char *restrict nptr,size_t nsize,size_t *restrict end_index);
__attribute__((noreturn)) void expr_explode(void);
__attribute__((noreturn)) void expr_trap(void);
__attribute__((noreturn)) void expr_ubehavior(void);
ssize_t expr_file_readfd_r(expr_reader reader,intptr_t fd,size_t tail,void *savep,const struct expr_memtool *restrict mtl);
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
int expr_setup_mtl(int flag);
void *expr_heapmtl_malloc_nonnull(size_t size,intptr_t arg);
void *expr_heapmtl_realloc_nonnull(void *old,size_t size,intptr_t arg);
int expr_setup_heapmtl4_nonnull(struct expr_memtool *restrict mtl,void *heap,size_t size,int flag);
//global externs of expr-global.c :
extern const struct expr_libinfo expr_libinfo_current[1];
extern int expr_mtl_setup;
#if !(defined(EXPR_INLIB)&&(EXPR_INLIB))
#define expr_free2mtl(ep,flag) expr_free2mtl_r(ep,flag,expr_defmtl)
#define expr_init_const(ep,val) expr_init_const_r(ep,val,expr_defmtl)
#define expr_new_const(val) expr_new_const_r(val,expr_defmtl)
#define expr_init8(ep,e,len,asym,asymlen,esp,flag,extarg) expr_init8_r(ep,e,len,asym,asymlen,esp,flag,extarg,expr_defmtl)
#define expr_new9(e,len,asym,asymlen,esp,flag,error,errinfo,extarg) expr_new9_r(e,len,asym,asymlen,esp,flag,error,errinfo,extarg,expr_defmtl)
#define expr_calc5(e,error,errinfo,esp,flag) expr_calc5_r(e,error,errinfo,esp,flag,expr_defmtl)
#define expr_writec(writer,fd,count,c) expr_writec_r(writer,fd,count,c,expr_defmtl)
#define expr_vwriteft(writer,fd,fmt,fmtlen,arg,addr,table) expr_vwriteft_r(writer,fd,fmt,fmtlen,arg,addr,table,expr_defmtl)
#define expr_writeft(writer,fd,fmt,fmtlen,args,arglen,table) expr_writeft_r(writer,fd,fmt,fmtlen,args,arglen,table,expr_defmtl)
#define expr_vapwriteft(writer,fd,fmt,fmtlen,table,ap) expr_vapwriteft_r(writer,fd,fmt,fmtlen,table,expr_defmtl,ap)
#define expr_apwriteft(writer,fd,fmt,fmtlen,table,...) expr_apwriteft_r(writer,fd,fmt,fmtlen,table,expr_defmtl,##__VA_ARGS__)
#define expr_snwriteft(buf,buflen,fmt,fmtlen,args,arglen,table) expr_snwriteft_r(buf,buflen,fmt,fmtlen,args,arglen,table,expr_defmtl)
#define expr_vsnprintft(buf,buflen,fmt,fmtlen,table,ap) expr_vsnprintft_r(buf,buflen,fmt,fmtlen,table,expr_defmtl,ap)
#define expr_snprintft(buf,buflen,fmt,fmtlen,table,...) expr_snprintft_r(buf,buflen,fmt,fmtlen,table,expr_defmtl,##__VA_ARGS__)
#define expr_sscanf(str,len,fmt,fmtlen,addr,addrlen) expr_sscanf_r(str,len,fmt,fmtlen,addr,addrlen,expr_defmtl)
#define expr_symset_init(esp) expr_symset_init_r(esp,expr_defmtl)
#define expr_symset_new() expr_symset_new_r(expr_defmtl)
#define expr_symbol_create(sym,type,flag,...) expr_symbol_create_r(sym,type,flag,expr_defmtl,##__VA_ARGS__)
#define expr_symbol_createl(sym,symlen,type,flag,...) expr_symbol_createl_r(sym,symlen,type,flag,expr_defmtl,##__VA_ARGS__)
#define expr_symbol_vcreate(sym,type,flag,ap) expr_symbol_vcreate_r(sym,type,flag,expr_defmtl,ap)
#define expr_symbol_vcreatel(sym,symlen,type,flag,ap) expr_symbol_vcreatel_r(sym,symlen,type,flag,expr_defmtl,ap)
#define expr_builtin_symbol_converts(syms,...) expr_builtin_symbol_converts_r(expr_defmtl,syms,##__VA_ARGS__)
#define expr_builtin_symbol_convert(syms) expr_builtin_symbol_convert_r(syms,expr_defmtl)
#define expr_sort4(v,n) expr_sort4_r(v,n,expr_defmtl)
#define expr_file_readfd(reader,fd,tail,savep) expr_file_readfd_r(reader,fd,tail,savep,expr_defmtl)

#define expr_calc4_r(e,error,errinfo,esp,mtl) expr_calc5_r(e,error,errinfo,esp,EXPR_INOOPTIMIZE|EXPR_IDEFAULT,mtl)
#define expr_calc3_r(e,error,errinfo,mtl) expr_calc5_r(e,error,errinfo,NULL,EXPR_INOOPTIMIZE|EXPR_IDEFAULT,mtl)
#define expr_calc2_r(e,flag,mtl) expr_calc5_r(e,NULL,NULL,NULL,flag,mtl)
#define expr_calc_r(e,mtl) expr_calc5_r(e,NULL,NULL,NULL,EXPR_INOOPTIMIZE|EXPR_IDEFAULT,mtl)

#define expr_calc5(e,error,errinfo,esp,flag) expr_calc5_r(e,error,errinfo,esp,flag,expr_defmtl)
#define expr_calc4(e,error,errinfo,esp) expr_calc4_r(e,error,errinfo,esp,expr_defmtl)
#define expr_calc3(e,error,errinfo) expr_calc3_r(e,error,errinfo,expr_defmtl)
#define expr_calc2(e,flag) expr_calc2_r(e,flag,expr_defmtl)
#define expr_calc(e) expr_calc_r(e,expr_defmtl)

#define expr_init7_r(ep,e,len,asym,asymlen,esp,flag,mtl) expr_init8_r(ep,e,len,asym,asymlen,esp,flag,expr_extarg_default,mtl)
#define expr_init6_r(ep,e,len,asym,asymlen,flag,mtl) expr_init8_r(ep,e,len,asym,asymlen,NULL,flag,expr_extarg_default,mtl)
#define expr_init5_r(ep,e,len,asym,asymlen,mtl) expr_init8_r(ep,e,len,asym,asymlen,NULL,0,expr_extarg_default,mtl)
#define expr_init_r(ep,e,asym,esp,flag,mtl) ({\
	const char *_e=(e),*_asym=(asym);\
	expr_init8_r(ep,_e,_e?expr_internal_strlen(_e):0,_asym,_asym?expr_internal_strlen(_asym):0,esp,flag,expr_extarg_default,mtl);\
})
#define expr_init4_r(ep,e,asym,flag,mtl) expr_init_r(ep,e,asym,NULL,flag,mtl)
#define expr_init3_r(ep,e,asym,mtl) expr_init_r(ep,e,asym,NULL,EXPR_IDEFAULT,mtl)
#define expr_init2_r(ep,e,mtl) expr_init_r(ep,e,NULL,NULL,EXPR_IDEFAULT,mtl)

#define expr_init7(ep,e,len,asym,asymlen,esp,flag) expr_init7_r(ep,e,len,asym,asymlen,esp,flag,expr_defmtl)
#define expr_init6(ep,e,len,asym,asymlen,flag) expr_init6_r(ep,e,len,asym,asymlen,flag,expr_defmtl)
#define expr_init5(ep,e,len,asym,asymlen) expr_init5_r(ep,e,len,asym,asymlen,expr_defmtl)
#define expr_init(ep,e,asym,esp,flag) expr_init_r(ep,e,asym,esp,flag,expr_defmtl)
#define expr_init4(ep,e,asym,flag) expr_init4_r(ep,e,asym,flag,expr_defmtl)
#define expr_init3(ep,e,asym) expr_init3_r(ep,e,asym,expr_defmtl)
#define expr_init2(ep,e) expr_init2_r(ep,e,expr_defmtl)

#define expr_new8_r(e,len,asym,asymlen,esp,flag,error,errinfo,mtl) expr_new9_r(e,len,asym,asymlen,esp,flag,error,errinfo,expr_extarg_default,mtl)
#define expr_new6_r(e,len,asym,asymlen,esp,flag,mtl) expr_new9_r(e,len,asym,asymlen,esp,flag,NULL,NULL,expr_extarg_default,mtl)
#define expr_new_r(e,asym,esp,flag,error,errinfo,mtl) ({\
	const char *_e=(e),*_asym=(asym);\
	expr_new9_r(_e,_e?expr_internal_strlen(_e):0,_asym,_asym?expr_internal_strlen(_asym):0,esp,flag,error,errinfo,expr_extarg_default,mtl);\
})
#define expr_new4_r(e,asym,esp,flag,mtl) expr_new_r(e,asym,esp,flag,NULL,NULL,mtl)
#define expr_new3_r(e,asym,flag,mtl) expr_new_r(e,asym,NULL,flag,NULL,NULL,mtl)
#define expr_new2_r(e,asym,mtl) expr_new_r(e,asym,NULL,EXPR_IDEFAULT,NULL,NULL,mtl)
#define expr_new1_r(e,mtl) expr_new_r(e,NULL,NULL,EXPR_IDEFAULT,NULL,NULL,mtl)

#define expr_new8(e,len,asym,asymlen,esp,flag,error,errinfo) expr_new8_r(e,len,asym,asymlen,esp,flag,error,errinfo,expr_defmtl)
#define expr_new6(e,len,asym,asymlen,esp,flag,error,errinfo) expr_new6_r(e,len,asym,asymlen,esp,flag,error,errinfo,expr_defmtl)
#define expr_new(e,asym,esp,flag,error,errinfo) expr_new_r(e,asym,esp,flag,error,errinfo,expr_defmtl)
#define expr_new4(e,asym,esp,flag) expr_new4_r(e,asym,esp,flag,expr_defmtl)
#define expr_new3(e,asym,flag) expr_new3_r(e,asym,flag,expr_defmtl)
#define expr_new2(e,asym) expr_new2_r(e,asym,expr_defmtl)
#define expr_new1(e) expr_new1_r(e,expr_defmtl)

#define expr_vwritef_r(writer,fd,fmt,fmtlen,arg,addr,mtl) expr_vwriteft_r(writer,fd,fmt,fmtlen,arg,addr,expr_ftable_default,mtl)

#define expr_vwritef(writer,fd,fmt,fmtlen,arg,addr) expr_vwritef_r(writer,fd,fmt,fmtlen,arg,addr,expr_defmtl)

#define expr_writef_r(writer,fd,fmt,fmtlen,args,arglen,mtl) expr_writeft_r(writer,fd,fmt,fmtlen,args,arglen,expr_ftable_default,mtl)

#define expr_writef(writer,fd,fmt,fmtlen,args,arglen) expr_writef_r(writer,fd,fmt,fmtlen,args,arglen,expr_defmtl)

#define expr_vapwritef_r(writer,fd,fmt,fmtlen,mtl,ap) expr_vapwriteft_r(writer,fd,fmt,fmtlen,expr_ftable_default,mtl,ap)
#define expr_apwritef_r(writer,fd,fmt,fmtlen,mtl,...) expr_apwriteft_r(writer,fd,fmt,fmtlen,expr_ftable_default,mtl,##__VA_ARGS__)

#define expr_vapwritef(writer,fd,fmt,fmtlen,ap) expr_vapwritef_r(writer,fd,fmt,fmtlen,expr_defmtl,ap)
#define expr_apwritef(writer,fd,fmt,fmtlen,...) expr_apwritef_r(writer,fd,fmt,fmtlen,expr_defmtl,##__VA_ARGS__)

#define expr_snwritef_r(buf,buflen,fmt,fmtlen,args,arglen,mtl) expr_snwriteft_r(buf,buflen,fmt,fmtlen,args,arglen,expr_ftable_default,mtl)

#define expr_snwritef(buf,buflen,fmt,fmtlen,args,arglen) expr_snwritef_r(buf,buflen,fmt,fmtlen,args,arglen,expr_defmtl)

#define expr_aswriteft_r(strp,fmt,fmtlen,args,arglen,table,mtl) ({\
	struct expr_buffered_file _fp[1];\
	const struct expr_memtool *restrict _mtl=(mtl);\
	ssize_t _r;\
	if(expr_likely(!expr_buffered_ainit_r(_fp,SIZE_MAX,_mtl))){\
		_r=expr_writeft_r(expr_buffered_write_warped,(intptr_t)_fp,fmt,fmtlen,args,arglen,table,_mtl);\
		if(expr_likely(_r>=0)){\
			((char *)_fp->buf)[_r]=0;\
		/* buffer expands if the writer touches its limit*/\
		*(strp)=_fp->buf;\
		}else {\
			expr_buffered_close(_fp);\
			*(strp)=NULL;\
		}\
	}else {\
		_r=SSIZE_MIN;\
		*(strp)=NULL;\
	}\
	_r;\
})

#define expr_aswriteft(strp,fmt,fmtlen,args,arglen,table,mtl) expr_aswriteft_r(strp,fmt,fmtlen,args,arglen,table,mtl)

#define expr_aswritef_r(strp,fmt,fmtlen,args,arglen,mtl) expr_aswriteft_r(strp,fmt,fmtlen,args,arglen,expr_ftable_default,mtl)

#define expr_aswritef(strp,fmt,fmtlen,args,arglen) expr_aswritef_r(strp,fmt,fmtlen,args,arglen,expr_defmtl)

#define expr_vsnprintf_r(buf,buflen,fmt,fmtlen,mtl,ap) expr_vsnprintft_r(buf,buflen,fmt,fmtlen,expr_ftable_default,mtl,ap)
#define expr_snprintf_r(buf,buflen,fmt,fmtlen,mtl,...) expr_snprintft_r(buf,buflen,fmt,fmtlen,expr_ftable_default,mtl,##__VA_ARGS__)

#define expr_vsnprintf(buf,buflen,fmt,fmtlen,ap) expr_vsnprintf_r(buf,buflen,fmt,fmtlen,expr_defmtl,ap)
#define expr_snprintf(buf,buflen,fmt,fmtlen,...) expr_snprintf_r(buf,buflen,fmt,fmtlen,expr_defmtl,##__VA_ARGS__)

#define expr_common_asprintft_r(strp,fmt,fmtlen,table,mtl,called,...) ({\
	struct expr_buffered_file _fp[1];\
	const struct expr_memtool *restrict _mtl=(mtl);\
	ssize_t _r;\
	if(expr_likely(!expr_buffered_ainit_r(_fp,SIZE_MAX,_mtl))){\
		_r=called(expr_buffered_write_warped,(intptr_t)_fp,fmt,fmtlen,table,_mtl,##__VA_ARGS__);\
		if(expr_likely(_r>=0)){\
			((char *)_fp->buf)[_r]=0;\
		/* buffer expands if the writer touches its limit*/\
		*(strp)=_fp->buf;\
		}else {\
			expr_buffered_close(_fp);\
			*(strp)=NULL;\
		}\
	}else {\
		_r=SSIZE_MIN;\
		*(strp)=NULL;\
	}\
	_r;\
})

#define expr_vasprintft_r(strp,fmt,fmtlen,table,mtl,ap) expr_common_asprintft_r(strp,fmt,fmtlen,table,mtl,expr_vapwriteft_r,ap)
#define expr_asprintft_r(strp,fmt,fmtlen,table,mtl,...) expr_common_asprintft_r(strp,fmt,fmtlen,table,mtl,expr_apwriteft_r,##__VA_ARGS__)

#define expr_vasprintf_r(strp,fmt,fmtlen,mtl,ap) expr_vasprintft_r(strp,fmt,fmtlen,expr_ftable_default,mtl,ap)
#define expr_asprintf_r(strp,fmt,fmtlen,mtl,...) expr_asprintft_r(strp,fmt,fmtlen,expr_ftable_default,mtl,##__VA_ARGS__)

#define expr_vasprintft(strp,fmt,fmtlen,table,ap) expr_vasprintft_r(strp,fmt,fmtlen,table,expr_defmtl,ap)
#define expr_asprintft(strp,fmt,fmtlen,table,...) expr_asprintft_r(strp,fmt,fmtlen,table,expr_defmtl,##__VA_ARGS__)

#define expr_vasprintf(strp,fmt,fmtlen,ap) expr_vasprintft_r(strp,fmt,fmtlen,expr_ftable_default,expr_defmtl,ap)
#define expr_asprintf(strp,fmt,fmtlen,...) expr_asprintft_r(strp,fmt,fmtlen,expr_ftable_default,expr_defmtl,##__VA_ARGS__)

#define expr_builtin_symbol_xaddall(esp,symsp,syms) expr_builtin_symbol_xaddalls(esp,symsp,syms,NULL)
#define expr_builtin_symbol_addall(esp,syms) expr_builtin_symbol_addalls(esp,syms,NULL)

#define expr_area_init(area,zone,size) expr_area_init4(area,zone,size,EXPR_ADEFAULT)
#define expr_setup_heapmtl(mtl,heap,size) expr_setup_heapmtl4(mtl,heap,size,EXPR_ADEFAULT)

#endif

// visible also with EXPR_INLIB set, for expr-alloc.c use.
#define expr_area_malloc3(area,size,flag) expr_area_realloc4(area,NULL,size,flag)
#define expr_area_malloc(area,size) ({\
	struct expr_area *restrict __areap=(area);\
	expr_area_malloc3(__areap,size,__areap->flag);\
})
#define expr_area_realloc(area,old,size) ({\
	struct expr_area *restrict __areap=(area);\
	expr_area_realloc4(__areap,old,size,__areap->flag);\
})
#define expr_area_calloc(area,size) ({\
	struct expr_area *restrict __areap=(area);\
	expr_area_malloc3(__areap,size,__areap->flag|EXPR_AZERO);\
})

#ifdef __END_DECLS
__END_DECLS
#endif

#endif

