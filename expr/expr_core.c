/*******************************************************************************
 *License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>*
 *This is free software: you are free to change and redistribute it.           *
 *******************************************************************************/
#define _GNU_SOURCE
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <setjmp.h>

#define _EXPR_LIB 1
#include "expr.h"

#define UNABLE_GETADDR_IN_PROTECTED_MODE 1


#define printval(x) warn(#x ":%lu",(unsigned long)(x))
#define printvalx(x) warn(#x ":%lx",(unsigned long)(x))
#define printvali(x) warn(#x ":%d",(int)(x))
#define printvall(x) warn(#x ":%ld",(long)(x))
#define printvald(x) warn(#x ":%lf",(double)(x))
#define trap (warn("\nat file %s line %d",__FILE__,__LINE__),__builtin_trap())

#define STACK_SIZEOFSSET(esp) ({size_t depth=(esp)->depth;depth<2?0:(depth-1)*EXPR_SYMSET_DEPTHUNIT;})
#define STACK_DEFAULT(_stack,esp) \
	__attribute__((cleanup(xfree_stack))) void *_stack##_heap;\
	void *_stack;\
	size_t _stack##_size;\
	_stack##_size=STACK_SIZEOFSSET(esp);\
	if(unlikely(!_stack##_size)){\
		_stack##_heap=NULL;\
		_stack=NULL;\
	}else if(expr_symset_allow_heap_stack){\
		_stack##_heap=xmalloc(_stack##_size);\
		if(unlikely(!_stack##_heap))\
			_stack=alloca(_stack##_size);\
		else \
			_stack=_stack##_heap;\
	}else {\
		_stack##_heap=NULL;\
		_stack=alloca(_stack##_size);\
	};((void)0)
#define SYMDIM(sp) (*expr_symbol_dim(sp))
#define HOTLEN(sp) expr_symbol_hotlen(sp)
#define assume(cond) expr_assume(cond)
#define likely(cond) expr_likely(cond)
#define unlikely(cond) expr_unlikely(cond)
#define cast(X,T) expr_cast(X,T)
#define eval(_ep,_input) expr_eval(_ep,_input)
#define align(x) (((x)+(EXPR_ALIGN-1))&~(EXPR_ALIGN-1))

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif


#define seterr(_ep,_eperror) ({(_ep)->error=(_eperror);\
		debug("error %s occur",expr_error(_eperror));})


#define cknp(ep,v,act) \
({\
	if(unlikely(!(v))){\
		seterr(ep,EXPR_EMEM);\
		act;\
	}\
})
/*
#define max(a,b) ({\
	__auto_type _a=(a);\
	__auto_type _b=(b);\
	_a>_b?_a:_b;\
})
#define maxc(a,const_expr) ({\
	expr_static_assert(__builtin_constant_p(const_expr));\
	__auto_type _a=(a);\
	_a>(const_expr)?_a:(const_expr);\
})
#define min(a,b) ({\
	__auto_type _a=(a);\
	__auto_type _b=(b);\
	_a<_b?_a:_b;\
})
*/
#define minc(a,const_expr) ({\
	expr_static_assert(__builtin_constant_p(const_expr));\
	__auto_type _a=(a);\
	_a<(const_expr)?_a:(const_expr);\
})
#define mincc(c1,c2) ((c1)<(c2)?(c1):(c2))
#define SUMCASES EXPR_SUM:\
		case EXPR_INT:\
		case EXPR_PROD:\
		case EXPR_SUP:\
		case EXPR_INF:\
		case EXPR_ANDN:\
		case EXPR_ORN:\
		case EXPR_XORN:\
		case EXPR_GCDN:\
		case EXPR_LCMN:\
		case EXPR_FOR:\
		case EXPR_LOOP
#define MDCASES EXPR_MD:\
		case EXPR_ME:\
		case EXPR_MEP
#define MDCASES_WITHP MDCASES:\
		case EXPR_PMD:\
		case EXPR_PME:\
		case EXPR_PMEP
#define SRCCASES_NOCOPY_SWAPABLE EXPR_ADD:\
		case EXPR_MUL:\
		case EXPR_AND:\
		case EXPR_OR:\
		case EXPR_XOR:\
		case EXPR_SEQ:\
		case EXPR_SNE:\
		case EXPR_EQ:\
		case EXPR_NE:\
		case EXPR_ANDL:\
		case EXPR_ORL:\
		case EXPR_XORL
#define SRCCASES_NOCOPY EXPR_ADD:\
		case EXPR_SUB:\
		case EXPR_MUL:\
		case EXPR_DIV:\
		case EXPR_MOD:\
		case EXPR_POW:\
		case EXPR_AND:\
		case EXPR_OR:\
		case EXPR_XOR:\
		case EXPR_SHL:\
		case EXPR_SHR:\
		case EXPR_GT:\
		case EXPR_GE:\
		case EXPR_LT:\
		case EXPR_LE:\
		case EXPR_SGE:\
		case EXPR_SLE:\
		case EXPR_SEQ:\
		case EXPR_SNE:\
		case EXPR_EQ:\
		case EXPR_NE:\
		case EXPR_ANDL:\
		case EXPR_ORL:\
		case EXPR_XORL:\
		case EXPR_NEXT:\
		case EXPR_DIFF:\
		case EXPR_OFF
#define SRCCASES EXPR_COPY:\
		case SRCCASES_NOCOPY
#define UNARYCASES EXPR_NEG:\
		case EXPR_NOT:\
		case EXPR_NOTL:\
		case EXPR_TSTL:\
		case EXPR_NEX0:\
		case EXPR_DIF0
#define BRANCHCASES EXPR_IF:\
		case EXPR_DON:\
		case EXPR_DOW:\
		case EXPR_WHILE
#define HOTCASES EXPR_DO:\
		case EXPR_HOT:\
		case EXPR_DO1:\
		case EXPR_EP:\
		case EXPR_WIF
#define SVCCASES EXPR_SVC:\
		case EXPR_SVCP:\
		case EXPR_SVC0:\
		case EXPR_SVCP0

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
#define EXPR_SPACES \
		'\t':\
		case '\r':\
		case '\v':\
		case '\f':\
		case '\n':\
		case '\b':\
		case ' '
#define expr_space(c) ({\
	int _r;\
	switch(c){\
		case EXPR_SPACES:\
			_r=1;\
			break;\
		default:\
			_r=0;\
			break;\
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
#define serrinfo(ei,s,n) ({\
	char *_ei=(ei);\
	size_t _n=minc((n),EXPR_SYMLEN-1);\
	memcpy(_ei,(s),_n);_ei[_n]=0;\
})
#define serrinfoc(ei,s) memcpy(ei,s,mincc(sizeof(s)-1,EXPR_SYMLEN))
#define expr_void(addr) ({\
	int _r;\
	switch((ssize_t)addr){\
		case -1:\
		case -2:\
			_r=1;\
			break;\
		default:\
			_r=0;\
			break;\
	}\
	_r;\
})
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#define LOGIC(a,b,_s) (((a)!=0.0) _s ((b)!=0.0))
#define LOGIC_BIT(_a,_b,_op_cal,_op_zero,_zero_val) \
	if(_expdiff>52){\
		_r=EXPR_EDSIGN(&_a) _op_zero EXPR_EDSIGN(&_b)?-( _zero_val):(_zero_val);\
	}else {\
		_x2=(EXPR_EDBASE(&_b)|(UINT64_C(1)<<52))>>_expdiff;\
		_x1=EXPR_EDBASE(&_a)|(UINT64_C(1)<<52);\
		_x1 _op_cal _x2;\
		if(_x1){\
			_x2=63-__builtin_clzl(_x1);\
			_x1&=~(UINT64_C(1)<<_x2);\
			_x2=52-_x2;\
			if(EXPR_EDEXP(&_a)<_x2){\
				_r=EXPR_EDSIGN(&_a) _op_zero EXPR_EDSIGN(&_b)?-( _zero_val):(_zero_val);\
			}else {\
				EXPR_EDBASE(&_a)=_x1<<_x2;\
				EXPR_EDEXP(&_a)-=_x2;\
				EXPR_EDSIGN(&_a) _op_cal EXPR_EDSIGN(&_b);\
				_r=_a;\
			}\
		}else {\
			_r=EXPR_EDSIGN(&_a)?-0.0:0.0;\
			EXPR_EDSIGN(&_r) _op_cal EXPR_EDSIGN(&_b);\
		}\
	}
#define and2(__a,__b) ({\
	uint64_t _x2,_x1;\
	int64_t _expdiff;\
	double _a,_b,_r;\
	_a=(__a);\
	_b=(__b);\
	_expdiff=EXPR_EDEXP(&_a)-EXPR_EDEXP(&_b);\
	if(_expdiff<0){\
		_expdiff=-_expdiff;\
		LOGIC_BIT(_b,_a,&=,&&,0.0)\
	}else {\
		LOGIC_BIT(_a,_b,&=,&&,0.0)\
	}\
	_r;\
})
#define or2(__a,__b) ({\
	uint64_t _x2,_x1;\
	int64_t _expdiff;\
	double _a,_b,_r;\
	_a=(__a);\
	_b=(__b);\
	_expdiff=EXPR_EDEXP(&_a)-EXPR_EDEXP(&_b);\
	if(_expdiff<0){\
		_expdiff=-_expdiff;\
		LOGIC_BIT(_b,_a,|=,||,_b>=0.0?_b:-_b)\
	}else {\
		LOGIC_BIT(_a,_b,|=,||,_a>=0.0?_a:-_a)\
	}\
	_r;\
})
#define xor2(__a,__b) ({\
	uint64_t _x2,_x1;\
	int64_t _expdiff;\
	double _a,_b,_r;\
	_a=(__a);\
	_b=(__b);\
	_expdiff=EXPR_EDEXP(&_a)-EXPR_EDEXP(&_b);\
	if(_expdiff<0){\
		_expdiff=-_expdiff;\
		LOGIC_BIT(_b,_a,^=,^,_b>=0.0?_b:-_b)\
	}else {\
		LOGIC_BIT(_a,_b,^=,^,_a>=0.0?_a:-_a)\
	}\
	_r;\
})
#define not(_x) xor2(9007199254740991.0/* 2^53-1*/,(_x))
#define nex0(x) (cast((int64_t)(x),double))
#define dif0(x) ((double)cast((x),int64_t))
#pragma GCC diagnostic pop
static const char *eerror[]={
	[0]="Unknown error",
	[EXPR_ESYMBOL]="Unknown symbol",
	[EXPR_EPT]="Parentheses do not match",
	[EXPR_EFP]="Function and keyword must be followed by a \'(\'",
	[EXPR_ENVP]="No value in parenthesis",
	[EXPR_ENEA]="No enough or too much argument",
	[EXPR_ENUMBER]="Bad number",
	[EXPR_ETNV]="Target is not variable",
	[EXPR_EEV]="Empty value",
	[EXPR_EUO]="Unexpected operator",
	[EXPR_EZAFP]="Zero-argument function must be followed by \'()\'",
	[EXPR_EDS]="Defined symbol",
	[EXPR_EVMD]="Not a multi-demension function with dim 0",
	[EXPR_EMEM]="Cannot allocate memory",
	[EXPR_EUSN]="Unexpected symbol or number",
	[EXPR_ENC]="Not a constant expression",
	[EXPR_ECTA]="Cannot take address",
	[EXPR_ESAF]="Static assertion failed",
	[EXPR_EVD]="Void value must be dropped",
	[EXPR_EPM]="In protected mode",
	[EXPR_EIN]="Injective function only",
	[EXPR_EBS]="Cannot remove a builtin symbol",
	[EXPR_EVZP]="Variable-length multi-demension function without a maximal length is not allowed in protected mode",
	[EXPR_EANT]="Alias is not terminated",
	[EXPR_EUDE]="User-defined error",
};
const char *expr_error(int error){
	switch(error){
		default:
			error=0;
		case 0 ... sizeof(eerror)/sizeof(*eerror)-1:
			return eerror[error];
	}
}

expr_weaks;

void (*expr_contractor)(void *,size_t)=expr_contract;
int expr_symset_allow_heap_stack=0;
const size_t expr_page_size=PAGE_SIZE;

#define free (use xfree() instead!)
#define malloc (use xmalloc() instead!)
#define realloc (use xrealloc() instead!)
#define asprintf (use xasprintf_nullable() instead!)
#define vasprintf (use expr_vasprintf() instead!)
static inline void *xautoadd(void **restrict old,size_t *restrict size,size_t *restrict length,size_t n,size_t extend){
	void *r;
	size_t old_size=*size,new_length;
	if(old_size<*length){
		++(*size);
		return (uint8_t *)*old+old_size*n;
	}
	new_length=*length+extend;
	r=xrealloc(*old,new_length*n);
	if(unlikely(!r)){
		return NULL;
	}
	*old=r;
	*length=new_length;
	++(*size);
	return (uint8_t *)r+old_size*n;
}
static inline void xfree_stack(void **restrict p){
	if(!*p)
		return;
	expr_deallocator(*p);
}

uint64_t expr_gcd64(uint64_t x,uint64_t y){
	uint64_t r;
	int r1;
	r=__builtin_ctzl(x);
	r1=__builtin_ctzl(y);
	r=r<r1?r:r1;
	x>>=r;
	y>>=r;
	r1=(x<y);
	while(likely(x&&y)){
		if(r1^=1)
			x%=y;
		else
			y%=x;
	}
	assume(!x||!y);
	return (x|y)<<r;
}
#define gcd2(__x,__y) ({\
	double _x,_y;\
	int r1;\
	_x=(__x);\
	_y=(__y);\
	r1=(fabs(_x)<fabs(_y));\
	while(likely(_x!=0.0&&_y!=0.0)){\
		if(r1^=1)_x=fmod(_x,_y);\
		else _y=fmod(_y,_x);\
	}\
	assume(_x==0.0||_y==0.0);\
	_x!=0.0?_x:_y;\
})
#define lcm2(__x,__y) ({\
	double _a,_b;\
	_a=(__x);\
	_b=(__y);\
	_a*_b/gcd2(_a,_b);\
})
double expr_gcd2(double x,double y){
	return gcd2(x,y);
}
double expr_lcm2(double x,double y){
	return lcm2(x,y);
}
void expr_mirror(double *buf,size_t size){
	double *out=buf+size-1,swapbuf;
	while(likely(out>buf)){
		swapbuf=*out;
		*out=*buf;
		*buf=swapbuf;
		--out;
		++buf;
	}
}
void expr_memswap(void *restrict s1,void *restrict s2,size_t size){
	register union {
		int64_t swapbuf;
		int32_t swapbuf32;
		int16_t swapbuf16;
		int8_t swapbuf8;
	} un;
	while(size>=8){
		un.swapbuf=*(int64_t *)s1;
		*(int64_t *)s1=*(int64_t *)s2;
		*(int64_t *)s2=un.swapbuf;
		size-=8;
		++(*(int64_t **)&s1);
		++(*(int64_t **)&s2);
	}
	if(size>=4){
		un.swapbuf32=*(int32_t *)s1;
		*(int32_t *)s1=*(int32_t *)s2;
		*(int32_t *)s2=un.swapbuf32;
		size-=4;
		++(*(int32_t **)&s1);
		++(*(int32_t **)&s2);
	}
	if(size>=2){
		un.swapbuf16=*(int16_t *)s1;
		*(int16_t *)s1=*(int16_t *)s2;
		*(int16_t *)s2=un.swapbuf16;
		size-=2;
		++(*(int16_t **)&s1);
		++(*(int16_t **)&s2);
	}
	if(size){
		un.swapbuf8=*(int8_t *)s1;
		*(int8_t *)s1=*(int8_t *)s2;
		*(int8_t *)s2=un.swapbuf8;
	}
}
void expr_memfry48(void *restrict buf,size_t size,size_t n,long seed){
	size_t r;
	seed=expr_seed48(seed);
	for(size_t i=0;i<n;++i){
		r=expr_ltol48(expr_next48(&seed))%n;
		if(likely(r!=i))
			expr_memswap((char *)buf+i*size,(char *)buf+r*size,size);
		else if(i<n-1){
			expr_memswap((char *)buf+i*size,(char *)buf+(i+1)*size,size);
		}
	}
}
void expr_fry(double *restrict v,size_t n){
	size_t r;
	double swapbuf;
	double *endp;
	switch(n){
		case 0:
		case 1:
			return;
		default:
			r=n>>1;
			expr_fry(v,r);
			expr_fry(v+r,r);
			break;
	}
	r=((uintptr_t)v+(uintptr_t)__builtin_frame_address(0))&511;
	for(endp=v+n-1;likely(v<endp);++v,--endp){
		r=(r*121+37)&1023;
		if(__builtin_parity(r)){
			swapbuf=*v;
			*v=*endp;
			*endp=swapbuf;
		}
	}
}

void expr_contract(void *buf,size_t size){
	char *p=(char *)buf,*endp=(char *)buf+size-1;
	while(p<=endp){
		*p=0;
		p+=PAGE_SIZE;
	}
	if(p!=endp)
		*endp=0;
}
__attribute__((noreturn)) void expr_explode(void){
	void *r;
	size_t sz=expr_allocate_max;
	do {
		while((r=xmalloc(sz))){
			expr_contractor(r,sz);
			//if do not contract,
			//the virtual memory
			//has not physical
			//memory and the OOM
			//will not be called.
		}
		sz>>=1;
	}while(sz);
	abort();
//the abort() is usually unreachable,should
//be killed by kernel before sz reaches 0
}
__attribute__((noreturn)) void expr_trap(void){
	__builtin_trap();
}
__attribute__((noreturn)) void expr_ubehavior(void){
	__builtin_unreachable();
}
double expr_and2(double x,double y){
	return and2(x,y);
}
double expr_or2(double x,double y){
	return or2(x,y);
}
double expr_xor2(double x,double y){
	return xor2(x,y);
}
double expr_not(double x){
	return not(x);
}
#ifdef EXPR_SYSIN
#define SYSCALL_DEFINED 1
#else
#define SYSCALL_DEFINED 0
#endif

#if (SYSCALL_DEFINED)&&(__linux__)

#ifndef _MUTEX_H_
#define _MUTEX_H_
#include <stdatomic.h>
#include <sys/syscall.h>
#include <linux/futex.h>
typedef _Atomic(uint32_t) mutex_t;
#define mutex_lock(lock) ({\
	mutex_t *_lock=(lock);\
	uint32_t _r;\
	while(expr_unlikely(_r=atomic_fetch_add(_lock,1))){\
		mutex_wait(_lock,_r+1);\
	}\
})

#define mutex_spinlock(lock) ({\
	mutex_t *__lock=(lock);\
	while(mutex_trylock(__lock));\
})

#define mutex_trylock(lock) ({\
	uint32_t __e=0;\
	!atomic_compare_exchange_strong((lock),&__e,1);\
})

#define mutex_unlock(lock) ({\
	mutex_t *_lock=(lock);\
	if(expr_unlikely(atomic_exchange(_lock,0)>=2)){\
		mutex_wake(_lock,INT32_MAX);\
	}\
})

#define mutex_spinunlock(lock) ({\
	atomic_store((lock),0);\
})

#define mutex_atomicl(lock,_label) for(mutex_lock(lock);;({mutex_unlock(lock);goto expr_combine(__atomic_label_,_label);}))if(0){expr_combine(__atomic_label_,_label):break;}else
#define mutex_atomic(lock) mutex_atomicl(lock,__LINE__)
#define mutex_spinatomicl(lock,_label) for(mutex_spinlock(lock);;({mutex_spinunlock(lock);goto expr_combine(__atomic_label_,_label);}))if(0){expr_combine(__atomic_label_,_label):break;}else
#define mutex_spinatomic(lock) mutex_spinatomicl(lock,__LINE__)

#define mutex_wait(lock,val) expr_internal_syscall6(SYS_futex,(intptr_t)(lock),FUTEX_WAIT,(val),0,0,0)
#define mutex_wake(lock,val) expr_internal_syscall6(SYS_futex,(intptr_t)(lock),FUTEX_WAKE,(val),0,0,0)
#endif

#endif

void expr_mutex_lock(uint32_t *lock){
#ifdef _MUTEX_H_
	mutex_lock((mutex_t *)lock);
#endif
}
int expr_mutex_trylock(uint32_t *lock){
#ifdef _MUTEX_H_
	return mutex_trylock((mutex_t *)lock);
#else
	return 0;
#endif
}
void expr_mutex_unlock(uint32_t *lock){
#ifdef _MUTEX_H_
	mutex_unlock((mutex_t *)lock);
#endif
}
void expr_mutex_spinlock(uint32_t *lock){
#ifdef _MUTEX_H_
	mutex_spinlock((mutex_t *)lock);
#endif
}
void expr_mutex_spinunlock(uint32_t *lock){
#ifdef _MUTEX_H_
	mutex_spinunlock((mutex_t *)lock);
#endif
}

intptr_t expr_warped_syscall0(int num){
	return expr_internal_syscall0(num);
}
#ifdef EXPR_SYSA0
intptr_t expr_warped_syscall1(int num,intptr_t a0){
	return expr_internal_syscall1(num,a0);
}
#ifdef EXPR_SYSA1
intptr_t expr_warped_syscall2(int num,intptr_t a0,intptr_t a1){
	return expr_internal_syscall2(num,a0,a1);
}
#ifdef EXPR_SYSA2
intptr_t expr_warped_syscall3(int num,intptr_t a0,intptr_t a1,intptr_t a2){
	return expr_internal_syscall3(num,a0,a1,a2);
}
#ifdef EXPR_SYSA3
intptr_t expr_warped_syscall4(int num,intptr_t a0,intptr_t a1,intptr_t a2,intptr_t a3){
	return expr_internal_syscall4(num,a0,a1,a2,a3);
}
#ifdef EXPR_SYSA4
intptr_t expr_warped_syscall5(int num,intptr_t a0,intptr_t a1,intptr_t a2,intptr_t a3,intptr_t a4){
	return expr_internal_syscall5(num,a0,a1,a2,a3,a4);
}
#ifdef EXPR_SYSA5
intptr_t expr_warped_syscall6(int num,intptr_t a0,intptr_t a1,intptr_t a2,intptr_t a3,intptr_t a4,intptr_t a5){
	return expr_internal_syscall6(num,a0,a1,a2,a3,a4,a5);
}
#ifdef EXPR_SYSA6
intptr_t expr_warped_syscall7(int num,intptr_t a0,intptr_t a1,intptr_t a2,intptr_t a3,intptr_t a4,intptr_t a5,intptr_t a6){
	return expr_internal_syscall7(num,a0,a1,a2,a3,a4,a5,a6);
}
#endif
#endif
#endif
#endif
#endif
#endif
#endif

#define REGKEY(s,op,dim,desc) {s,op,0,sizeof(s)-1,desc}
#define REGKEYS(s,op,dim,desc) {s,op,EXPR_KF_SUBEXPR,sizeof(s)-1,desc}
#define REGKEYC(s,op,dim,desc) {s,op,EXPR_KF_SEPCOMMA,sizeof(s)-1,desc}
#define REGKEYSC(s,op,dim,desc) {s,op,EXPR_KF_SUBEXPR|EXPR_KF_SEPCOMMA,sizeof(s)-1,desc}
#define REGKEYN(s,op,dim,desc) {s,op,EXPR_KF_NOPROTECT,sizeof(s)-1,desc}
#define REGKEYCN(s,op,dim,desc) {s,op,EXPR_KF_SEPCOMMA|EXPR_KF_NOPROTECT,sizeof(s)-1,desc}
#define REGKEYSN(s,op,dim,desc) {s,op,EXPR_KF_SUBEXPR|EXPR_KF_NOPROTECT,sizeof(s)-1,desc}
#define REGKEYSCN(s,op,dim,desc) {s,op,EXPR_KF_SUBEXPR|EXPR_KF_SEPCOMMA|EXPR_KF_NOPROTECT,sizeof(s)-1,desc}
const struct expr_builtin_keyword expr_keywords[]={
	REGKEYSC("sum",EXPR_SUM,5,"sum(index_name,start_index,end_index,index_step,addend)"),
	REGKEYSC("int",EXPR_INT,5,"int(integral_var_name,upper_limit,lower_limit,epsilon,integrand)"),
	REGKEYSC("prod",EXPR_PROD,5,"prod(index_name,start_index,end_index,index_step,factor)"),
	REGKEYSC("sup",EXPR_SUP,5,"sup(index_name,start_index,end_index,index_step,element)"),
	REGKEYSC("infi",EXPR_INF,5,"infi(index_name,start_index,end_index,index_step,element)"),
	REGKEYSC("andn",EXPR_ANDN,5,"andn(index_name,start_index,end_index,index_step,element)"),
	REGKEYSC("orn",EXPR_ORN,5,"orn(index_name,start_index,end_index,index_step,element)"),
	REGKEYSC("xorn",EXPR_XORN,5,"xorn(index_name,start_index,end_index,index_step,element)"),
	REGKEYSC("gcdn",EXPR_GCDN,5,"gcdn(index_name,start_index,end_index,index_step,element)"),
	REGKEYSC("lcmn",EXPR_LCMN,5,"lcmn(index_name,start_index,end_index,index_step,element)"),
	REGKEYSC("for",EXPR_FOR,5,"for(var_name,start_var,cond,body,value)"),
	REGKEYSC("loop",EXPR_LOOP,5,"loop(var_name,start_var,count,body,value)"),
	REGKEYSC("vmd",EXPR_VMD,7,"vmd(index_name,start_index,end_index,index_step,element,md_symbol,[constant_expression max_dim])"),
	REGKEYS("do",EXPR_DO,1,"do(body) do{body}"),
	REGKEYSC("if",EXPR_IF,3,"if(cond,if_value,else_value) if(cond){body}[{else_body}]"),
	REGKEYSC("while",EXPR_WHILE,3,"while(cond,body) while(cond){body}"),
	REGKEYSC("dowhile",EXPR_DOW,3,"dowhile(cond,body) dowhile(cond){body}"),
	REGKEYSC("don",EXPR_DON,3,"don(cond,body) don(cond){body}"),
	REGKEYSC("const",EXPR_CONST,2,"const(name,[constant_expression value]) name=(value)"),
	REGKEYSC("var",EXPR_MUL,2,"var(name,[constant_expression initial_value])"),
	REGKEYSCN("double",EXPR_BL,1,"double(constant_expression count)"),
	REGKEYSCN("byte",EXPR_COPY,1,"byte(constant_expression count)"),
	REGKEYCN("alloca",EXPR_ALO,2,"alloca(nmemb,[constant_expression size])"),
	REGKEYN("setjmp",EXPR_SJ,1,"setjmp(jmp_buf)"),
	REGKEYCN("longjmp",EXPR_LJ,2,"longjmp(jmp_buf,val)"),
	REGKEYCN("eval",EXPR_EVAL,2,"eval(ep,input)"),
	REGKEYSCN("decl",EXPR_ADD,2,"decl(symbol,[constant_expression flag])"),
	REGKEYS("static_assert",EXPR_SUB,1,"static_assert(constant_expression cond)"),
	REGKEYSN("ip",EXPR_IP,1,"ip([mode])"),
	REGKEYN("to",EXPR_TO,1,"to(val)"),
	REGKEYN("to1",EXPR_TO1,1,"to(val)"),
	REGKEYSC("hot",EXPR_HOT,2,"hot(function_name,(args...){expression}) function_name=(args...){expression}"),
	REGKEY("defined",EXPR_DIV,1,"defined(symbol)"),
	REGKEY("typeof",EXPR_AND,1,"typeof(symbol)"),
	REGKEY("flagof",EXPR_OR,1,"flagof(symbol)"),
	REGKEY("undef",EXPR_XOR,1,"undef([symbol])[{scope}]"),
	REGKEY("reset",EXPR_NOTL,1,"reset([symbol])[{scope}]"),
	REGKEY("static_if",EXPR_ANDL,3,"static_if(constant_expression cond){body}[{else_body}]"),
	REGKEY("flag",EXPR_ORL,1,"flag([constant_expression new_flag])[{scope}]"),
	REGKEY("scope",EXPR_XORL,1,"scope(){scope}"),
	REGKEYS("fix",EXPR_NEXT,1,"fix(expression value)"),
	REGKEYS("constant",EXPR_DIFF,1,"constant(expression value)"),
	REGKEYS("try",EXPR_NEG,1,"try([new_symbol]){body}"),
	REGKEYC("alias",EXPR_NOT,2,"alias(alias_name,target)"),
	REGKEYCN("return",EXPR_RET,2,"return(ep,val)"),
	REGKEY("error",EXPR_TSTL,2,"error(const string)"),
	REGKEYCN("svcp",EXPR_SVCP,3,"svc(sys_num,arg[],constant_expression n)"),
	REGKEYCN("svc",EXPR_SVC,EXPR_SYSAM+1,"svcn(sys_num,...)"),
	{NULL}
};
const struct expr_builtin_symbol *expr_builtin_symbol_search(const struct expr_builtin_symbol *syms,const char *sym,size_t sz){
	for(;syms->str;++syms){
		if(unlikely(sz==syms->strlen&&!memcmp(syms->str,sym,sz))){
			return syms;
		}
	}
	return NULL;
}
const struct expr_builtin_symbol *expr_builtin_symbol_rsearch(const struct expr_builtin_symbol *syms,void *addr){
	for(;syms->str;++syms){
		if(unlikely(syms->un.uaddr==addr)){
			return syms;
		}
	}
	return NULL;
}
struct expr_symbol *expr_builtin_symbol_add(struct expr_symset *restrict esp,const struct expr_builtin_symbol *p){
	switch(p->type){
		case EXPR_CONSTANT:
			return expr_symset_addl(esp,p->str,p->strlen,EXPR_CONSTANT,p->flag,p->un.value);
		case EXPR_VARIABLE:
			return expr_symset_addl(esp,p->str,p->strlen,EXPR_VARIABLE,p->flag,p->un.addr);
		case EXPR_FUNCTION:
			return expr_symset_addl(esp,p->str,p->strlen,EXPR_FUNCTION,p->flag,p->un.func);
		case EXPR_MDFUNCTION:
			return expr_symset_addl(esp,p->str,p->strlen,EXPR_MDFUNCTION,p->flag,p->un.mdfunc,(size_t)p->dim);
		case EXPR_MDEPFUNCTION:
			return expr_symset_addl(esp,p->str,p->strlen,EXPR_MDEPFUNCTION,p->flag,p->un.mdepfunc,(size_t)p->dim);
		case EXPR_ZAFUNCTION:
			return expr_symset_addl(esp,p->str,p->strlen,EXPR_ZAFUNCTION,p->flag,p->un.zafunc);
		case EXPR_HOTFUNCTION:
			return expr_symset_addl(esp,p->str,p->strlen,EXPR_HOTFUNCTION,0,p->un.hot);
		case EXPR_ALIAS:
			return expr_symset_addl(esp,p->str,p->strlen,EXPR_ALIAS,0,p->un.hot);
		default:
			__builtin_unreachable();
	}
}
size_t expr_builtin_symbol_addall(struct expr_symset *restrict esp,const struct expr_builtin_symbol *syms){
	size_t r=0;
	for(;syms->str;++syms){
		if(likely(expr_builtin_symbol_add(esp,syms)))
			++r;
	}
	return r;
}
struct expr_symset *expr_builtin_symbol_convert(const struct expr_builtin_symbol *syms){
	struct expr_symset *esp=new_expr_symset();
	if(unlikely(!esp))
		return NULL;
	for(;syms->str;++syms){
		struct expr_symbol *r;
		if(unlikely(!(r=expr_builtin_symbol_add(esp,syms)))){
			trap;
			expr_symset_free(esp);
			return NULL;
		}
	}
	return esp;
}
const uint8_t expr_number_table[256]={
[0 ... '0'-1]=127,
['0']=0,
['1']=1,
['2']=2,
['3']=3,
['4']=4,
['5']=5,
['6']=6,
['7']=7,
['8']=8,
['9']=9,
['9'+1 ... 'A'-1]=127,
['A']=10,
['B']=11,
['C']=12,
['D']=13,
['E']=14,
['F']=15,
['F'+1 ... 'a'-1]=127,
['a']=10,
['b']=11,
['c']=12,
['d']=13,
['e']=14,
['f']=15,
['f'+1 ... 255]=127,
};
size_t expr_strscan(const char *restrict s,size_t sz,char *restrict buf,size_t outsz){
	const char *p,*endp=s+sz;
	char *buf0=(char *)buf;
	uint8_t v,v1;
	char *oend;
	oend=buf0+outsz;
	while(s<endp&&buf<oend)switch(*s){
		case '\\':
			if(unlikely(s+1>=endp))
				goto dflt;
			switch(s[1]){
				case '\\':
					*(buf++)='\\';
					s+=2;
					break;
				case 'a':
					*(buf++)='\a';
					s+=2;
					break;
				case 'b':
					*(buf++)='\b';
					s+=2;
					break;
				case 'c':
					*(buf++)='\0';
					s+=2;
					break;
				case 'e':
					*(buf++)='\033';
					s+=2;
					break;
				case 'f':
					*(buf++)='\f';
					s+=2;
					break;
				case 'n':
					*(buf++)='\n';
					s+=2;
					break;
				case 'r':
					*(buf++)='\r';
					s+=2;
					break;
				case 't':
					*(buf++)='\t';
					s+=2;
					break;
				case 'v':
					*(buf++)='\v';
					s+=2;
					break;
#define scanoux(base,maxlen) \
					v=0;\
					while(p<endp){\
						v1=expr_number_table[(uint8_t)*p];\
						if(unlikely(v1>=base))\
							break;\
						v=v*base+v1;\
						++p;\
						if(p-s>=maxlen){\
							if(base==16){\
								*(buf++)=v;\
								if(unlikely(buf>=oend))\
									goto no_enough_size;\
								s=p;\
								goto x_start;\
							}else\
								break;\
						}\
					}\
					if(unlikely(p==s))\
						goto fail;\
					*(buf++)=v;\
					s=p
				case 'B':
					s+=2;
					p=s;
					scanoux(2,8);
					break;
				case 'u':
					s+=2;
					p=s;
					scanoux(10,3);
					break;
				case 'x':
					s+=2;
					p=s;
x_start:
					scanoux(16,2);
					break;
				default:
					++s;
					p=s;
					scanoux(8,3);
					break;
fail:
					break;
			}
			break;
		default:
dflt:
			*(buf++)=*(s++);
			break;
	}
no_enough_size:
	return buf-buf0;
}
static inline const char *findpair_dmark(const char *c,const char *endp){
	for(++c;c<endp;++c){
		if(*c=='\"'&&c[-1]!='\\')
			return c;
	}
	return NULL;
}

char *expr_astrscan(const char *s,size_t sz,size_t *restrict outsz){
	char *buf;
	buf=xmalloc(sz+1);
	if(!buf)
		return NULL;
	*outsz=expr_strscan(s,sz,buf,sz);
	buf[*outsz]=0;
	return buf;
}
static inline void freesuminfo(struct expr_suminfo *p){
	expr_free(p->ep);
	expr_free(p->fromep);
	expr_free(p->toep);
	expr_free(p->stepep);
	xfree(p);
}
static inline void freevmdinfo(struct expr_vmdinfo *p){
	expr_free(p->ep);
	expr_free(p->fromep);
	expr_free(p->toep);
	expr_free(p->stepep);
	if(p->args)
		xfree(p->args);
	xfree(p);
}
static inline void freebranchinfo(struct expr_branchinfo *p){
	expr_free(p->cond);
	expr_free(p->body);
	expr_free(p->value);
	xfree(p);
}
static inline void freemdinfo(struct expr_mdinfo *p){
	for(size_t i=0;i<p->dim;++i)
		expr_free(p->eps+i);
	xfree(p->eps);
	if(p->args)
		xfree(p->args);
	if(p->e)
		xfree((void *)p->e);
	xfree(p);
}
static inline void freehmdinfo(struct expr_hmdinfo *p){
	for(size_t i=0;i<p->dim;++i)
		expr_free(p->eps+i);
	xfree(p->eps);
	expr_free(p->hotfunc);
	xfree(p->args);
	xfree(p);
}
static inline void expr_freedata(struct expr_inst *restrict data,size_t size){
	struct expr_inst *ip=data,*endp=data+size;
	for(;ip<endp;++ip){
		switch(ip->op){
			case SUMCASES:
				freesuminfo(ip->un.es);
				break;
			case MDCASES_WITHP:
				freemdinfo(ip->un.em);
				break;
			case EXPR_VMD:
				freevmdinfo(ip->un.ev);
				break;
			case BRANCHCASES:
				freebranchinfo(ip->un.eb);
				break;
			case HOTCASES:
				expr_free(ip->un.hotfunc);
				break;
			case EXPR_HMD:
				freehmdinfo(ip->un.eh);
				break;
			default:
				break;
		}
	}
	xfree(data);
}
static inline void expr_free_keepres(struct expr *restrict ep){
	if(!ep->isconst){
		if(likely(ep->data))
			expr_freedata(ep->data,ep->size);
		if(likely(ep->vars)){
			for(size_t i=0;i<ep->vsize;++i)
				if(likely(ep->vars[i]))
					xfree(ep->vars[i]);
			xfree(ep->vars);
		}
	}
	if(ep->sset_shouldfree){
		expr_symset_free(ep->sset);
	}
}
void expr_free(struct expr *restrict ep){
	struct expr *ep0=(struct expr *)ep;
	struct expr_resource *erp,*erp1;
start:
	expr_free_keepres(ep);
	for(erp=ep->res;erp;){
		if(erp->un.uaddr)switch(erp->type){
			case EXPR_HOTFUNCTION:
				expr_free(erp->un.ep);
				break;
			default:
				xfree(erp->un.uaddr);
				break;
		}
		erp1=erp;
		erp=erp->next;
		xfree(erp1);
	}
	switch(ep->freeable){
		case 1:
			xfree(ep0);
			break;
		case 2:
			++ep;
			goto start;
		default:
			break;
	}
}
static inline void setunsafe(struct expr *restrict ep){
	struct expr *p;
	if(ep->parent){
		p=ep->parent;
		do {
			p->iflag|=EXPR_IF_UNSAFE;
		}while((p=p->parent));
	}
	ep->iflag|=EXPR_IF_UNSAFE;
}
#define EXTEND_SIZE 16
static inline struct expr_inst *expr_addop(struct expr *restrict ep,void *dst,void *src,enum expr_op op,int flag){
	struct expr_inst *ip;
	ip=xautoadd((void **)&ep->data,&ep->size,&ep->length,sizeof(struct expr_inst),EXTEND_SIZE*sizeof(struct expr_inst));
	if(unlikely(!ip))
		return NULL;
	ip->op=op;
	ip->dst.uaddr=dst;
	ip->un.uaddr=src;
	ip->flag=flag;
	return ip;
}
static inline struct expr_inst *expr_addread(struct expr *restrict ep,double *dst,double *src){
	return expr_addop(ep,dst,src,EXPR_READ,0);
}
static inline struct expr_inst *expr_addwrite(struct expr *restrict ep,double *dst,double *src){
	return expr_addop(ep,dst,src,EXPR_WRITE,0);
}
static inline struct expr_inst *expr_addoff(struct expr *restrict ep,double *dst,double *src){
	return expr_addop(ep,dst,src,EXPR_OFF,0);
}
static inline struct expr_inst *expr_addcopy(struct expr *restrict ep,double *dst,double *src){
	return expr_addop(ep,dst,src,EXPR_COPY,0);
}
static inline struct expr_inst *expr_addcall(struct expr *restrict ep,double *dst,double (*func)(double),int flag){
	return expr_addop(ep,dst,func,EXPR_BL,flag);
}
static inline struct expr_inst *expr_addlj(struct expr *restrict ep,double *dst,double *src){
	return expr_addop(ep,dst,src,EXPR_LJ,0);
}
static inline struct expr_inst *expr_addun(struct expr *restrict ep,double *dst,enum expr_op op){
	return expr_addop(ep,dst,NULL,op,0);
}
#define expr_addsj(ep,dst) expr_addun(ep,dst,EXPR_SJ)
#define expr_addneg(ep,dst) expr_addun(ep,dst,EXPR_NEG)
#define expr_addnot(ep,dst) expr_addun(ep,dst,EXPR_NOT)
#define expr_addnotl(ep,dst) expr_addun(ep,dst,EXPR_NOTL)
#define expr_addinput(ep,dst) expr_addun(ep,dst,EXPR_INPUT)
#define expr_addto(ep,dst) expr_addun(ep,dst,EXPR_TO)
#define expr_addto1(ep,dst) expr_addun(ep,dst,EXPR_TO1)
#define expr_addend(ep,dst) expr_addun(ep,dst,EXPR_END)
#define expr_addnex0(ep,dst) expr_addun(ep,dst,EXPR_NEX0)
#define expr_adddif0(ep,dst) expr_addun(ep,dst,EXPR_DIF0)
static inline struct expr_inst *expr_addip(struct expr *restrict ep,double *dst,size_t mode){
	return expr_addop(ep,dst,(void *)mode,EXPR_IP,0);
}
static inline struct expr_inst *expr_addza(struct expr *restrict ep,double *dst,double (*zafunc)(void),int flag){
	return expr_addop(ep,dst,zafunc,EXPR_ZA,flag);
}
static inline struct expr_inst *expr_addmd(struct expr *restrict ep,double *dst,struct expr_mdinfo *em,int flag){
	return expr_addop(ep,dst,em,EXPR_MD,flag);
}
static inline struct expr_inst *expr_addme(struct expr *restrict ep,double *dst,struct expr_mdinfo *em,int flag){
	return expr_addop(ep,dst,em,EXPR_ME,flag);
}
static inline struct expr_inst *expr_addmep(struct expr *restrict ep,double *dst,struct expr_mdinfo *em,int flag){
	return expr_addop(ep,dst,em,EXPR_MEP,flag);
}
static inline struct expr_inst *expr_addhot(struct expr *restrict ep,double *dst,struct expr *hot,int flag){
	return expr_addop(ep,dst,hot,EXPR_HOT,flag);
}
static inline struct expr_inst *expr_addsvc(struct expr *restrict ep,double *dst,int flag){
	return expr_addop(ep,dst,NULL,EXPR_SVC,flag);
}
static inline struct expr_inst *expr_addsvcp(struct expr *restrict ep,double *dst,double *src,int flag){
	return expr_addop(ep,dst,src,EXPR_SVCP,flag);
}
/*
static inline struct expr_inst *expr_addep(struct expr *restrict ep,double *dst,struct expr *hot,int flag){
	return expr_addop(ep,dst,hot,EXPR_EP,flag);
}
*/
static inline struct expr_inst *expr_addhmd(struct expr *restrict ep,double *dst,struct expr_hmdinfo *eh,int flag){
	return expr_addop(ep,dst,eh,EXPR_HMD,flag);
}
static inline struct expr_inst *expr_addconst(struct expr *restrict ep,double *dst,double val){
	return expr_addop(ep,dst,cast(val,void *),EXPR_CONST,0);
}
static inline struct expr_inst *expr_addconst_i(struct expr *restrict ep,double *dst,double val){
//	the address of a variable may be used so it cannot be optimized out.
	return expr_addop(ep,dst,cast(val,void *),EXPR_CONST,EXPR_SF_INJECTION);
}
static inline struct expr_inst *expr_addalo(struct expr *restrict ep,double *dst,size_t zu){
	return expr_addop(ep,dst,cast(zu,void *),EXPR_ALO,0);
}
static struct expr_resource *expr_newres(struct expr *restrict ep){
	struct expr_resource *p;
	if(!ep->res){
		p=xmalloc(sizeof(struct expr_resource));
		if(unlikely(!p))
			return NULL;
		p->next=NULL;
		p->type=EXPR_CONSTANT;
		ep->res=p;
		return p;
	}
	p=ep->tail?ep->tail:ep->res;
	while(p->next)p=p->next;
	p->next=xmalloc(sizeof(struct expr_resource));
	if(unlikely(!p->next))
		return NULL;
	p=p->next;
	p->next=NULL;
	p->type=EXPR_CONSTANT;
	ep->tail=p;
	return p;
}
static double *expr_newvar(struct expr *restrict ep){
	double *r=xmalloc(sizeof(double)),**p;
	if(unlikely(!r))
		return NULL;
	p=xautoadd((void **)&ep->vars,&ep->vsize,&ep->vlength,sizeof(double *),EXTEND_SIZE*sizeof(double *));
	if(unlikely(!p))
		return NULL;
	*p=r;
	*r=NAN;
	return r;
}
static int expr_detach(struct expr *restrict ep){
	struct expr_symset *esp;
	if(!ep->sset_shouldfree){
		if(!ep->sset)
			esp=new_expr_symset();
		else
			esp=expr_symset_clone(ep->sset);
		if(unlikely(!esp))
			return -1;
		ep->sset=esp;
		ep->sset_shouldfree=1;
	}
	return 0;
}
static int expr_createconst(struct expr *restrict ep,const char *symbol,size_t symlen,double val){
	if(unlikely(!symlen)){
		seterr(ep,EXPR_EEV);
		return -1;
	}
	cknp(ep,expr_detach(ep)>=0,return -1);
	return expr_symset_addl(ep->sset,symbol,symlen,EXPR_CONSTANT,0,val)?
	0:-1;
}
static int expr_createsvar(struct expr *restrict ep,const char *symbol,size_t symlen,double val){
	if(unlikely(!symlen)){
		seterr(ep,EXPR_EEV);
		return -1;
	}
	struct expr_resource *r;
	cknp(ep,expr_detach(ep)>=0,return -1);
	r=expr_newres(ep);
	cknp(ep,r,return -1);
	r->un.addr=xmalloc(sizeof(double));
	cknp(ep,r->un.addr,return -1);
	*r->un.addr=val;
	return expr_symset_addl(ep->sset,symbol,symlen,EXPR_VARIABLE,0,r->un.addr)?0:-1;
}
static int expr_createvar4(struct expr *restrict ep,const char *symbol,size_t symlen,double val){
	double *r;
	if(unlikely(!symlen)){
		seterr(ep,EXPR_EEV);
		return -1;
	}
	r=expr_newvar(ep);
	if(unlikely(!r))
		return -1;
	cknp(ep,expr_detach(ep)>=0,return -1);
	*r=val;
	return expr_symset_addl(ep->sset,symbol,symlen,EXPR_VARIABLE,0,r)?0:-1;
}
static double *expr_createvar(struct expr *restrict ep,const char *symbol,size_t symlen){
	double *r;
	if(unlikely(!symlen)){
		seterr(ep,EXPR_EEV);
		return NULL;
	}
	r=expr_newvar(ep);
	if(unlikely(!r))
		return NULL;
	cknp(ep,expr_detach(ep)>=0,return NULL);
	return expr_symset_addl(ep->sset,symbol,symlen,EXPR_VARIABLE,0,r)?r:NULL;
}
static int expr_createhot(struct expr *restrict ep,const char *symbol,size_t symlen,const char *hotexpr,size_t hotlen,int flag){
	if(unlikely(!symlen)){
		seterr(ep,EXPR_EEV);
		return -1;
	}
	cknp(ep,expr_detach(ep)>=0,return -1);
	return expr_symset_addl(ep->sset,symbol,symlen,EXPR_HOTFUNCTION,flag|EXPR_SF_INJECTION,hotexpr,hotlen)?
	0:-1;
}
static inline const char *findpair(const char *c,const char *endp){
	size_t lv=0;
	if(*c!='(')
		goto err;
	while(c<endp){
		switch(*c){
			case '(':
				++lv;
				break;
			case ')':
				--lv;
				if(!lv)
					return c;
				break;
			default:
				break;
		}
		++c;
	}
err:
	return NULL;
}
static inline const char *findpair_bracket(const char *c,const char *endp){
	size_t lv=0;
	if(*c!='[')
		goto err;
	while(c<endp){
		switch(*c){
			case '[':
				++lv;
				break;
			case ']':
				--lv;
				if(!lv)
					return c;
				break;
			default:
				break;
		}
		++c;
	}
err:
	return NULL;
}
static inline const char *findpair_brace(const char *c,const char *endp){
	size_t lv=0;
	if(*c!='{')
		goto err;
	while(c<endp){
		switch(*c){
			case '{':
				++lv;
				break;
			case '}':
				--lv;
				if(!lv)
					return c;
				break;
			default:
				break;
		}
		++c;
	}
err:
	return NULL;
}
static inline const char *unfindpair(const char *e,const char *c){
	size_t lv=0;
	if(*c!=')')
		goto err;
	while(c>=e){
		switch(*c){
			case ')':
				++lv;
				break;
			case '(':
				--lv;
				if(!lv)
					return c;
				break;
			default:
				break;
		}
		--c;
	}
err:
	return NULL;
}
static inline const char *getsym_p(const char *c,const char *endp){
	const char *p;
	while(c<endp){
		if(*c=='('){
			p=findpair(c,endp);
			if(p)
				return p+1;
		}
		if(expr_operator(*c))
			break;
		++c;
	}
	return c;
}
static inline const char *getsym(const char *c,const char *endp){
	while(c<endp&&!expr_operator(*c))
		++c;
	return c;
}
static inline const char *getsym_expo(const char *c,const char *endp){
	const char *c0=c;
	while(c<endp&&!expr_operator(*c))
		++c;
	if(c+1<endp&&c-c0>=2&&(*c=='-'||*c=='+')&&(c[-1]=='e'||c[-1]=='E')&&((c[-2]<='9'&&c[-2]>=0)||c[-2]=='.')){
		return getsym(c+1,endp);
	}
	return c;
}
static inline int atod(const char *str,size_t sz,double *dst){
	char *c;
	*dst=strtod(str,&c);
	if(unlikely(c==str))
		return 0;
	else if(unlikely(c-str!=sz))
		return 2;
	else return 1;
}
static char *expr_tok(char *restrict str,char **restrict saveptr){
	char *s0=(char *)str;
	int instr=0;
	if(str){
		*saveptr=str;
	}else if(!**saveptr)
		return NULL;
	else {
		str=*saveptr;
	}
	while(**saveptr){
		switch(**saveptr){
			case '\"':
				if(*saveptr>s0&&(*saveptr)[-1]=='\\')
					break;
				instr^=1;
				break;
			case ',':
				if(instr)
					break;
				**saveptr=0;
				++(*saveptr);
				return str;
			case '(':
				*saveptr=(char *)findpair(*saveptr,*saveptr+strlen(*saveptr));
				if(unlikely(!*saveptr))
					return NULL;
			default:
				break;
		}
		++(*saveptr);
	}
	return str;
}
static inline void expr_free2(char **buf){
	for(char **p=buf;*p;++p){
		xfree(*p);
	}
	xfree(buf);
}
static char **expr_sep(struct expr *restrict ep,const char *pe,size_t esz){
	char *p,*p1,*p2,**p3=NULL,/*p5,*/*e,*p6;
	void *p7;
	size_t len=0,s/*,sz*/;
	p6=e=xmalloc(esz+1);
	cknp(ep,e,return NULL);
	memcpy(e,pe,esz);
	e[esz]=0;
	if(*e=='('){
		p1=(char *)findpair(e,e+esz);
		if(p1){
			if(!p1[1]){
				*p1=0;
				++e;
			}
		}else {
			seterr(ep,EXPR_EPT);
			goto fail;
		}
	}
	if(unlikely(!*e)){
		seterr(ep,EXPR_ENVP);
		goto fail;
	}
	for(p=expr_tok(e,&p2);p;p=expr_tok(NULL,&p2)){
		s=strlen(p);
		p1=xmalloc(s+1);
		cknp(ep,p1,goto fail);
		p1[s]=0;
		memcpy(p1,p,s);
		p7=xrealloc(p3,(len+2)*sizeof(char *));
		cknp(ep,p7,goto fail);
		p3=p7;
		p3[len++]=p1;
	}
	if(likely(p3))
		p3[len]=NULL;
	xfree(p6);
	return p3;
fail:
	if(likely(p3)){
		p3[len]=NULL;
		expr_free2(p3);
	}
	xfree(p6);
	return NULL;
}
static int init_expr8(struct expr *restrict ep,const char *e,size_t len,const char *asym,size_t asymlen,struct expr_symset *esp,int flag,struct expr *restrict parent);
static struct expr_mdinfo *getmdinfo(struct expr *restrict ep,const char *e0,size_t sz,const char *e,size_t esz,const char *asym,size_t asymlen,void *func,size_t dim,int ifep){
	char **v,**p;
	char *pe;
	char *v1=NULL;
	size_t i;
	struct expr_mdinfo *em;
	if(dim==1){
		v1=xmalloc(esz+1);
		cknp(ep,v1,return NULL);
		memcpy(v1,e,esz);
		v1[esz]=0;
		v=&v1;
		i=1;
	}else {
		v=expr_sep(ep,e,esz);
		if(unlikely(!v)){
			serrinfo(ep->errinfo,e0,sz);
			return NULL;
		}
		p=v;
		while(*p)++p;
		i=p-v;
		if(unlikely(dim&&i!=dim)){
			serrinfo(ep->errinfo,e0,sz);
			seterr(ep,EXPR_ENEA);
			goto err1;
		}
	}
	em=xmalloc(sizeof(struct expr_mdinfo));
	cknp(ep,em,goto err1);
	em->e=NULL;
	em->args=NULL;
	em->dim=i;
	em->un.func=func;
	em->eps=xmalloc(em->dim*sizeof(struct expr));
	cknp(ep,em->eps,goto err15);
	switch(ifep){
		case 0:
			em->args=xmalloc(em->dim*sizeof(double));
			cknp(ep,em->args,goto err175);
			break;
		default:
			sz+=esz;
			pe=xmalloc(sz+1);
			cknp(ep,pe,goto err175);
			memcpy(pe,e0,sz);
			pe[sz]=0;
			em->e=pe;
		case 1:
		break;
	}
	for(i=0;i<em->dim;++i){
		if(unlikely(init_expr8(em->eps+i,v[i],strlen(v[i]),asym,asymlen,ep->sset,ep->iflag,ep)<0)){
			for(ssize_t k=i-1;k>=0;--k)
				expr_free(em->eps+k);
			goto err2;
		}
	}
	if(v1)
		xfree(v1);
	else
		expr_free2(v);
	return em;
err2:
	if(em->args)
		xfree(em->args);
	if(em->e)
		xfree((void *)em->e);
	seterr(ep,em->eps[i].error);
	memcpy(ep->errinfo,em->eps[i].errinfo,EXPR_SYMLEN);
err175:
	xfree(em->eps);
err15:
	xfree(em);
err1:
	if(v1)
		xfree(v1);
	else
		expr_free2(v);
	return NULL;
}
static void expr_seizeres(struct expr *restrict dst,struct expr *restrict src){
	struct expr_resource **drp;
	drp=&dst->res;
	while(*drp)
		drp=&(*drp)->next;
	*drp=src->res;
	src->res=NULL;

}
static struct expr *new_expr10(const char *e,size_t len,const char *asym,size_t asymlen,struct expr_symset *esp,int flag,int n,int *error,char errinfo[EXPR_SYMLEN],struct expr *restrict parent);
static struct expr *new_expr8p(const char *e,size_t len,const char *asym,size_t asymlen,struct expr_symset *esp,int flag,int *error,char errinfo[EXPR_SYMLEN],struct expr *restrict parent){
	return new_expr10(e,len,asym,asymlen,esp,flag,1,error,errinfo,parent);
}
static double consteval(const char *e,size_t len,const char *asym,size_t asymlen,struct expr_symset *sset,struct expr *restrict parent){
	struct expr *ep;
	double r;
	ep=new_expr10(e,len,asym,asymlen,sset,parent->iflag&~EXPR_IF_NOOPTIMIZE,1,&parent->error,parent->errinfo,parent);
	if(unlikely(!ep))
		return NAN;
	if(unlikely(!expr_isconst(ep))){
		expr_free(ep);
		seterr(parent,EXPR_ENC);
		serrinfo(parent->errinfo,e,len);
		return NAN;
	}
	r=eval(ep,0.0);
	expr_seizeres(parent,ep);
	expr_free(ep);
	return r;
}
static double nonconsteval(const char *e,size_t len,const char *asym,size_t asymlen,struct expr_symset *sset,struct expr *restrict parent){
	struct expr *ep;
	double r;
	ep=new_expr10(e,len,asym,asymlen,sset,parent->iflag&~EXPR_IF_NOOPTIMIZE,1,&parent->error,parent->errinfo,parent);
	if(unlikely(!ep))
		return NAN;
	r=eval(ep,0.0);
	expr_seizeres(parent,ep);
	expr_free(ep);
	return r;
}
static double constcheck(const char *e,size_t len,const char *asym,size_t asymlen,struct expr_symset *sset,struct expr *restrict parent){
	struct expr *ep;
	double r;
	ep=new_expr10(e,len,asym,asymlen,sset,parent->iflag&~EXPR_IF_NOOPTIMIZE,1,&parent->error,parent->errinfo,parent);
	if(unlikely(!ep))
		return NAN;
	r=expr_isconst(ep)?1.0:0.0;;
	expr_seizeres(parent,ep);
	expr_free(ep);
	return r;
}
static struct expr_vmdinfo *getvmdinfo(struct expr *restrict ep,const char *e0,size_t sz,const char *e,size_t esz,const char *asym,size_t asymlen,int *flag){
	char **v=expr_sep(ep,e,esz);
	char **p;
	struct expr_vmdinfo *ev;
	struct expr_symset *sset;
	union {
		const struct expr_symbol *es;
	} sym;
	size_t ssz,s0;
	size_t max;
	double (*fp)(double *args,size_t n);
	if(unlikely(!v)){
		serrinfo(ep->errinfo,e0,sz);
		return NULL;
	}
	p=v;
	while(*p){
		++p;
	}
	switch(p-v){
		case 6:
			max=0;
			break;
		case 7:
			max=(size_t)consteval(v[6],strlen(v[6]),asym,asymlen,ep->sset,ep);
			if(unlikely(ep->error))
				goto err0;
			break;
		default:
			serrinfo(ep->errinfo,e0,sz);
			seterr(ep,EXPR_ENEA);
			goto err0;
	}
	ssz=strlen(v[5]);
	if(ep->sset&&(sym.es=expr_symset_search(ep->sset,v[5],ssz))){
		if(unlikely(sym.es->type!=EXPR_MDFUNCTION||SYMDIM(sym.es))){
			serrinfo(ep->errinfo,v[5],ssz);
			seterr(ep,EXPR_EVMD);
			goto err0;
		}
		fp=expr_symbol_un(sym.es)->mdfunc;
		*flag=sym.es->flag;
	}else {
		serrinfo(ep->errinfo,v[5],ssz);
		seterr(ep,EXPR_ESYMBOL);
		goto err0;
	}
	if(ep->iflag&EXPR_IF_PROTECT){
		if(*flag&EXPR_SF_UNSAFE){
			seterr(ep,EXPR_EPM);
			serrinfo(ep->errinfo,v[5],ssz);
			goto err0;
		}
		if(!max){
			seterr(ep,EXPR_EVZP);
			serrinfoc(ep->errinfo,"vmd");
			goto err0;
		}
		ev=xmalloc(sizeof(struct expr_vmdinfo));
		cknp(ep,ev,goto err0);
		ev->args=xmalloc(max*sizeof(double));
		cknp(ep,ev->args,goto err05);
		ev->max=max;
	}else {
		if(*flag&EXPR_SF_UNSAFE){
			setunsafe(ep);
		}
		ev=xmalloc(sizeof(struct expr_vmdinfo));
		cknp(ep,ev,goto err0);
		if(max){
			ev->args=xmalloc(max*sizeof(double));
			cknp(ep,ev->args,goto err05);
			ev->max=max;
		}else {
			ev->args=NULL;
			ev->max=0;
			setunsafe(ep);
		}
	}
	sset=expr_symset_clone(ep->sset);
	cknp(ep,sset,goto err075);
	s0=strlen(v[0]);
	expr_symset_remove(sset,v[0],s0);
	cknp(ep,expr_symset_addl(sset,v[0],s0,EXPR_VARIABLE,0,&ev->index),goto err1);
	ev->fromep=new_expr8p(v[1],strlen(v[1]),asym,asymlen,ep->sset,ep->iflag,&ep->error,ep->errinfo,ep);
	if(unlikely(!ev->fromep))
		goto err1;
	ev->toep=new_expr8p(v[2],strlen(v[2]),asym,asymlen,sset,ep->iflag,&ep->error,ep->errinfo,ep);
	if(unlikely(!ev->toep))
		goto err2;
	ev->stepep=new_expr8p(v[3],strlen(v[3]),asym,asymlen,sset,ep->iflag,&ep->error,ep->errinfo,ep);
	if(unlikely(!ev->stepep))
		goto err3;
	ev->ep=new_expr8p(v[4],strlen(v[4]),asym,asymlen,sset,ep->iflag,&ep->error,ep->errinfo,ep);
	if(unlikely(!ev->ep))
		goto err4;

	expr_free2(v);
	expr_symset_free(sset);
/*#define setsset(_ep) if((_ep)->sset==sset)(_ep)->sset=ep->sset
	setsset(ev->toep);
	setsset(ev->stepep);
	setsset(ev->ep);*/
	ev->func=fp;
	return ev;
err4:
	expr_free(ev->stepep);
err3:
	expr_free(ev->toep);
err2:
	expr_free(ev->fromep);
err1:
	expr_symset_free(sset);
err075:
	if(ev->args)
		xfree(ev->args);
err05:
	xfree(ev);
err0:
	expr_free2(v);
	return NULL;
}
static struct expr_suminfo *getsuminfo(struct expr *restrict ep,const char *e0,size_t sz,const char *e,size_t esz,const char *asym,size_t asymlen){
	char **v=expr_sep(ep,e,esz);
	char **p;
	struct expr_suminfo *es;
	struct expr_symset *sset;
	size_t s0;
	if(unlikely(!v)){
		serrinfo(ep->errinfo,e0,sz);
		return NULL;
	}
	p=v;
	while(*p){
		++p;
	}
	if(unlikely(p-v!=5)){
		serrinfo(ep->errinfo,e0,sz);
		seterr(ep,EXPR_ENEA);
		goto err0;
	}
	es=xmalloc(sizeof(struct expr_suminfo));
	cknp(ep,es,goto err0);
//	sum(sym_index,from,to,step,expression)
	sset=expr_symset_clone(ep->sset);
	cknp(ep,sset,goto err05);
	s0=strlen(v[0]);
	expr_symset_remove(sset,v[0],s0);
	cknp(ep,expr_symset_addl(sset,v[0],s0,EXPR_VARIABLE,0,&es->index),goto err1);
	es->fromep=new_expr8p(v[1],strlen(v[1]),asym,asymlen,ep->sset,ep->iflag,&ep->error,ep->errinfo,ep);
	if(unlikely(!es->fromep))
		goto err1;
	es->toep=new_expr8p(v[2],strlen(v[2]),asym,asymlen,sset,ep->iflag,&ep->error,ep->errinfo,ep);
	if(unlikely(!es->toep))
		goto err2;
	es->stepep=new_expr8p(v[3],strlen(v[3]),asym,asymlen,sset,ep->iflag,&ep->error,ep->errinfo,ep);
	if(unlikely(!es->stepep))
		goto err3;
	es->ep=new_expr8p(v[4],strlen(v[4]),asym,asymlen,sset,ep->iflag,&ep->error,ep->errinfo,ep);
	if(unlikely(!es->ep))
		goto err4;
	expr_free2(v);
	expr_symset_free(sset);
	/*setsset(es->toep);
	setsset(es->stepep);
	setsset(es->ep);*/
	return es;
err4:
	expr_free(es->stepep);
err3:
	expr_free(es->toep);
err2:
	expr_free(es->fromep);
err1:
	expr_symset_free(sset);
err05:
	xfree(es);
err0:
	expr_free2(v);
	return NULL;
}
struct branch {
	const char *cond;
	const char *body;
	const char *value;
	size_t scond;
	size_t sbody;
	size_t svalue;
};
static struct expr_branchinfo *getbranchinfo(struct expr *restrict ep,const char *e0,size_t sz,const char *e,size_t esz,struct branch *b,const char *asym,size_t asymlen){
	char **v=NULL;
	char **p;
	struct expr_branchinfo *eb;
	int dim3;
	if(b){
		eb=xmalloc(sizeof(struct expr_branchinfo));
		cknp(ep,eb,goto err0);
	//	while(cond,body,value)
		eb->cond=new_expr8p(b->cond,b->scond,asym,asymlen,ep->sset,ep->iflag,&ep->error,ep->errinfo,ep);
		if(unlikely(!eb->cond))
			goto err1;
		if(b->sbody){
			eb->body=new_expr8p(b->body,b->sbody,asym,asymlen,ep->sset,ep->iflag,&ep->error,ep->errinfo,ep);
			if(unlikely(!eb->body))
				goto err2;
		}else {
			eb->body=new_expr_const(NAN);
			cknp(ep,eb->body,goto err2);
		}
		if(b->svalue){
			eb->value=new_expr8p(b->value,b->svalue,asym,asymlen,ep->sset,ep->iflag,&ep->error,ep->errinfo,ep);
			if(unlikely(!eb->value))
				goto err3;
		}else {
			eb->value=new_expr_const(NAN);
			cknp(ep,eb->value,goto err3);
		}
		return eb;
	}
	v=expr_sep(ep,e,esz);
	if(unlikely(!v)){
		serrinfo(ep->errinfo,e0,sz);
		return NULL;
	}
	p=v;
	while(*p){
		++p;
	}
	switch(p-v){
		case 3:
			dim3=1;
			break;
		case 2:
			dim3=0;
			break;
		default:
			serrinfo(ep->errinfo,e0,sz);
			seterr(ep,EXPR_ENEA);
			goto err0;
	}
	eb=xmalloc(sizeof(struct expr_branchinfo));
	cknp(ep,eb,goto err0);
//	while(cond,body,value)
	eb->cond=new_expr8p(v[0],strlen(v[0]),asym,asymlen,ep->sset,ep->iflag,&ep->error,ep->errinfo,ep);
	if(unlikely(!eb->cond))
		goto err1;
	eb->body=new_expr8p(v[1],strlen(v[1]),asym,asymlen,ep->sset,ep->iflag,&ep->error,ep->errinfo,ep);
	if(unlikely(!eb->body))
		goto err2;
	if(dim3){
		eb->value=new_expr8p(v[2],strlen(v[2]),asym,asymlen,ep->sset,ep->iflag,&ep->error,ep->errinfo,ep);
		if(unlikely(!eb->value))
			goto err3;
	}else {
		eb->value=new_expr_const(NAN);
		cknp(ep,eb->value,goto err3);
	}
	expr_free2(v);
	return eb;
err3:
	expr_free(eb->body);
err2:
	expr_free(eb->cond);
err1:
	xfree(eb);
err0:
	if(v)
		expr_free2(v);
	return NULL;
}
static size_t vsize2(char *const *v){
	char *const *p1=v;
	for(;*p1;++p1);
	return p1-v;
}
static double *scan(struct expr *restrict ep,const char *e,const char *endp,const char *asym,size_t asymlen);
static double *gethot(struct expr *restrict ep,const char *e0,size_t sz,const char *e,const char *endp,const char *asym,size_t asymlen,const char *he,size_t helen,const char **ee,struct expr_symset *esp,int flag){
	const char *p,*hend,*pe;
	char **v,**ve;
	size_t n,len,i;
	double *v1;
	struct expr_hmdinfo *eh;
	//warn("(%s) len:%zu",he,helen);
	hend=he+helen;
#define hot_sep(_e,_p,_end,_v,_act) \
	if(unlikely(*_e!='(')){\
		seterr(ep,EXPR_EPT);\
		serrinfo(ep->errinfo,e0,sz);\
		_act;\
	}\
	_p=findpair(_e,_end);\
	if(unlikely(!_p)){\
		seterr(ep,EXPR_EPT);\
		serrinfo(ep->errinfo,e0,sz);\
		_act;\
	}\
	_v=expr_sep(ep,_e,_p-_e+1);\
	if(unlikely(!_v)){\
		serrinfo(ep->errinfo,e0,sz);\
		_act;\
	}
	hot_sep(he,p,hend,v,return NULL);
	hot_sep(e,pe,endp,ve,goto err0);
	*ee=pe+1;
	n=vsize2(v);
	if(vsize2(ve)!=n){
		seterr(ep,EXPR_ENEA);
		serrinfo(ep->errinfo,e0,sz);
		goto err05;
	}
	eh=xmalloc(sizeof(struct expr_hmdinfo));
	cknp(ep,eh,goto err05);
	eh->args=xmalloc(n*sizeof(double));
	cknp(ep,eh->args,xfree(eh);goto err05);
	eh->eps=xmalloc(n*sizeof(struct expr));
	cknp(ep,eh->eps,xfree(eh->args);xfree(eh);goto err05);
	eh->dim=n;

	v1=eh->args;
	for(char **p1=v;;){
		len=strlen(*p1);
		expr_symset_remove(esp,*p1,len);
		cknp(ep,expr_symset_addl(esp,*p1,len,EXPR_VARIABLE,0,v1),goto err1);
		++p1;
		if(!*p1)
			break;
		++v1;
	}

	he=p+1;
	if(he>=hend||*he!='{'){
		seterr(ep,EXPR_EPT);
		serrinfo(ep->errinfo,e0,sz);
		goto err1;
	}
	p=findpair_brace(he,hend);
	if(unlikely(!p)){
		seterr(ep,EXPR_EPT);
		serrinfo(ep->errinfo,e0,sz);
		goto err1;
	}
	eh->hotfunc=new_expr8p(he+1,p-he-1,asym,asymlen,esp,ep->iflag,&ep->error,ep->errinfo,ep);
	if(unlikely(!eh->hotfunc))
		goto err1;
	for(i=0;i<n;++i){
		if(unlikely(init_expr8(eh->eps+i,ve[i],strlen(ve[i]),asym,asymlen,ep->sset,ep->iflag,ep)<0)){
			for(ssize_t k=i-1;k>=0;--k)
				expr_free(eh->eps+k);
			goto err2;
		}
	}

	v1=expr_newvar(ep);
	cknp(ep,v1,goto err3);
	cknp(ep,expr_addhmd(ep,v1,eh,flag&~EXPR_SF_INJECTION),goto err3);
	expr_free2(ve);
	expr_free2(v);
	return v1;
err3:
	freehmdinfo(eh);
	goto err05;
err2:
	seterr(ep,eh->eps[i].error);
	memcpy(ep->errinfo,eh->eps[i].errinfo,EXPR_SYMLEN);
	expr_free(eh->hotfunc);
err1:
	xfree(eh->eps);
	xfree(eh->args);
	xfree(eh);
//	expr_symset_free(esp);
err05:
	expr_free2(ve);
err0:
	expr_free2(v);
	return NULL;
}
static int expr_rmsymt(struct expr *restrict ep,struct expr_symbol **buf,size_t sz,int type){
	struct expr_symbol **bufp=buf;
	struct expr_symbol **bufp_end=buf+sz;
	STACK_DEFAULT(stack,ep->sset);
	expr_symset_foreach(sp,ep->sset,stack){
		if(unlikely(bufp>=bufp_end)){
			expr_symset_removea(ep->sset,buf,bufp-buf);
			return 1;
		}
		if(sp->type!=type)
			continue;
		*(bufp++)=sp;
	}
	expr_symset_removea(ep->sset,buf,bufp-buf);
	return 0;
}
static int expr_rmsym(struct expr *restrict ep,const char *sym,size_t sz){
	int type;
	void *r;
	size_t rsz;
	if(!sz)
		return 0;
	switch(*sym){
		case '*':
			if(sz==1){
				expr_symset_wipe(ep->sset);
				return 0;
			}
			type=(int)consteval(sym+1,sz-1,NULL,0,ep->sset,ep);
			if(unlikely(ep->error))
				return -1;
			switch(type){
				case EXPR_CONSTANT:
				case EXPR_VARIABLE:
				case EXPR_FUNCTION:
				case EXPR_MDFUNCTION:
				case EXPR_MDEPFUNCTION:
				case EXPR_ZAFUNCTION:
				case EXPR_HOTFUNCTION:
				case EXPR_ALIAS:
					break;
				default:
					return 0;
			}
			r=xmalloc((rsz=ep->sset->depth)*sizeof(struct expr_symbol *));
			cknp(ep,r,return -1);
			while(expr_rmsymt(ep,r,rsz,type));
			xfree(r);
			return 0;
		default:
			return expr_symset_remove(ep->sset,sym,sz)<0;
	}
}
static const struct expr_symbol *alias_target(struct expr *restrict ep,const struct expr_symbol *alias,int symerr){
	size_t len,len0;
	const char *p,*p0;
	const struct expr_symbol *prev;
	if(alias->type!=EXPR_ALIAS){
		return alias;
	}
	p0=alias->str;
	len0=alias->strlen;
	for(size_t n=ep->sset->size-1;;--n){
		p=expr_symbol_hot(alias);
		len=HOTLEN(alias);
		prev=alias;
		alias=expr_symset_search(ep->sset,p,len);
		if(unlikely(!alias)){
			switch(symerr){
				case 0:
					return NULL;
				default:
					serrinfo(ep->errinfo,p,len);
					seterr(ep,EXPR_ESYMBOL);
					return NULL;
				case 2:
					return prev;
			}
		}
		if(likely(alias->type!=EXPR_ALIAS)){
			return alias;
		}
		if(unlikely(!n)){
			serrinfo(ep->errinfo,p0,len0);
			seterr(ep,EXPR_EANT);
			return NULL;
		}
	}
}
static double *alias_scan(struct expr *restrict ep,const struct expr_symbol *alias,const char *asym,size_t asymlen){
	size_t len;
	const char *p;
	struct expr_symset *esp,*old;
	double *v0;
	cknp(ep,expr_detach(ep)>=0,return NULL);
	esp=expr_symset_clone(ep->sset);
	cknp(ep,esp,return NULL);
	p=expr_symbol_hot(alias);
	len=HOTLEN(alias);
	expr_symset_remove(esp,p,len);
	old=ep->sset;
	ep->sset=esp;
	v0=scan(ep,p,p+len,asym,asymlen);
	ep->sset=old;
	expr_symset_free(esp);
	return v0;
}
static void *readsymbol(struct expr *restrict ep,const char *symbol,size_t symlen,int follow,int symerr){
	int flag,type;
	union {
		const struct expr_symbol *es;
		void *uaddr;
	} sym;
	if(!ep->sset||!(sym.es=expr_symset_search(ep->sset,symbol,symlen))){
		goto esymbol;
	}
//	debug("reading:%s -- ",symbol);
alias_found:
	type=sym.es->type;
	flag=sym.es->flag;
	switch(type){
		case EXPR_MDFUNCTION:
		case EXPR_MDEPFUNCTION:
		case EXPR_ZAFUNCTION:
			if(flag&EXPR_SF_UNSAFE){
				if(unlikely(ep->iflag&EXPR_IF_PROTECT))
					goto pm;
				setunsafe(ep);
			}
		case EXPR_FUNCTION:
			if(unlikely(!(flag&EXPR_SF_INJECTION)&&(ep->iflag&EXPR_IF_INJECTION)))
				goto ein;
		default:
			break;
	}
	if(type==EXPR_ALIAS&&follow){
		sym.es=alias_target(ep,sym.es,follow);
		if(follow==2)
			follow=0;
		if(sym.es){
			goto alias_found;
		}
		return NULL;
	}
//	debug("at:%p\n",expr_symbol_un(sym.es)->uaddr);
	return sym.uaddr;
pm:
	serrinfo(ep->errinfo,symbol,symlen);
	seterr(ep,EXPR_EPM);
	return NULL;
ein:
	serrinfo(ep->errinfo,symbol,symlen);
	seterr(ep,EXPR_EIN);
	return NULL;
esymbol:
	if(symerr){
		serrinfo(ep->errinfo,symbol,symlen);
		seterr(ep,EXPR_ESYMBOL);
	}
	return NULL;
}
static double *getvalue(struct expr *restrict ep,const char *e,const char *endp,const char **_p,const char *asym,size_t asymlen){
	const char *p,*p2,*p4,*e0=e;
	double *v0=NULL,*v1;
	int r0;
	union {
		double v;
		void *uaddr;
		double *daddr;
		int8_t *baddr;
		struct expr *ep;
		struct expr_suminfo *es;
		struct expr_branchinfo *eb;
		struct expr_mdinfo *em;
		struct expr_vmdinfo *ev;
		char **vv1;
		const char *ccp;
		struct expr_symset *esp;
		struct expr_symbol *ebp;
		size_t sz;
		int flag;
	} un;
	union {
		const struct expr_symbol *es;
		struct expr_resource *er;
		char **vv;
		struct branch *b;
		struct expr_symset *esp;
		void *uaddr;
	} sym;
	union {
		char **vv;
		struct expr_symbol *es;
		const struct expr_symbol *ces;
		struct expr_symset *esp;
		size_t i;
		double v;
	} sv;
	int type,flag;
	size_t dim;
	if(unlikely(e>=endp))
		goto eev;
	switch(*e){
		case 0:
eev:
			seterr(ep,EXPR_EEV);
			return NULL;
		case '(':
			p=findpair(e,endp);
			if(unlikely(!p)){
pterr:
				seterr(ep,EXPR_EPT);
				return NULL;
			}
			if(unlikely(p==e+1)){
envp:
				seterr(ep,EXPR_ENVP);
				return NULL;
			}
			v0=scan(ep,e+1,p,asym,asymlen);
			if(unlikely(!v0))
				return NULL;
			e=p+1;
			goto vend;
		case ':':
			if(e+1>=endp||e[1]!=':'){
				if(ep->iflag&EXPR_IF_NOKEYWORD)
					goto dflt;
				e+=1;
#define try_getsym \
				p=getsym(e,endp);\
				if(unlikely(p==e)){\
					*ep->errinfo=*e;\
					seterr(ep,EXPR_EUO);\
					return NULL;\
				}
				try_getsym;
				goto keyword;
			}
			try_getsym;
			goto symget;
		case 'd':
			if(e+2>endp||e[1]!='o'||e[2]!='{')
				break;
			e+=2;
			dim=1;
		case '{':
			r0=0;
			dim=0;
block:
			p=findpair_brace(e,endp);
			if(unlikely(!p))
				goto pterr;
			if(unlikely(p==e+1))
				goto envp;
			v0=expr_newvar(ep);
			cknp(ep,v0,return NULL);
			un.ep=new_expr8p(e+1,p-e-1,asym,asymlen,ep->sset,ep->iflag,&ep->error,ep->errinfo,ep);
			if(unlikely(!un.ep))
				return NULL;
			if(r0){
				sym.er=expr_newres(ep);
				cknp(ep,sym.er,expr_free(un.ep);return NULL);
				sym.er->un.ep=un.ep;
				sym.er->type=EXPR_HOTFUNCTION;
				cknp(ep,expr_addconst(ep,v0,un.v),return NULL);
			}else {
				cknp(ep,expr_addop(ep,v0,un.ep,dim?EXPR_DO:EXPR_EP,0),expr_free(un.ep);return NULL);
			}
			e=p+1;
			goto vend;
		case '[':
			p=findpair_bracket(e,endp);
			if(unlikely(!p))
				goto pterr;
			if(unlikely(ep->iflag&EXPR_IF_PROTECT)){
				seterr(ep,EXPR_EPM);
				serrinfo(ep->errinfo,e,p-e+1);
				return NULL;
			}
			setunsafe(ep);
			if(unlikely(p==e+1))
				goto envp;
			v1=scan(ep,e+1,p,asym,asymlen);
			if(unlikely(!v1))
				return NULL;
			v0=expr_newvar(ep);
			cknp(ep,v0,return NULL);
			cknp(ep,expr_addread(ep,v0,v1),return NULL);
			e=p+1;
			goto vend;
		case '\'':
			if(e+2>=endp||e[2]!='\'')
				break;
			v0=expr_newvar(ep);
			cknp(ep,v0,return NULL);
			cknp(ep,expr_addconst(ep,v0,(double)((unsigned char *)e)[1]),return NULL);
			e+=3;
			goto vend;
		case '\"':
			p=findpair_dmark(e,endp);
			if(unlikely(!p))
				goto pterr;
#if UNABLE_GETADDR_IN_PROTECTED_MODE
			if(unlikely(ep->iflag&EXPR_IF_PROTECT)){
				seterr(ep,EXPR_EPM);
				serrinfo(ep->errinfo,e,p-e+1);
				return NULL;
			}
#endif
			setunsafe(ep);
			un.uaddr=expr_astrscan(e+1,p-e-1,&dim);
			if(unlikely(!un.uaddr))
				break;
			sym.er=expr_newres(ep);
			cknp(ep,sym.er,xfree(un.uaddr);return NULL);
			sym.er->un.str=un.uaddr;
			v0=expr_newvar(ep);
			cknp(ep,v0,return NULL);
			cknp(ep,expr_addconst(ep,v0,un.v),return NULL);
			e=p+1;
			goto vend;
		case '&':
#if UNABLE_GETADDR_IN_PROTECTED_MODE
			if(unlikely(ep->iflag&EXPR_IF_PROTECT)){
				seterr(ep,EXPR_EPM);
				*ep->errinfo='&';
				return NULL;
			}
#endif
			setunsafe(ep);
			if(e+1<endp)switch(e[1]){
				case '#':
					v0=expr_newvar(ep);
					cknp(ep,v0,return NULL);
					if(ep->parent){
						un.ep=ep->parent;
						while(un.ep->parent)
							un.ep=un.ep->parent;
					}else
						un.ep=(struct expr *)ep;
					cknp(ep,expr_addconst(ep,v0,un.v),return NULL);
					e+=2;
					goto vend;
/*				case '+':
					v0=expr_newvar(ep);
					cknp(ep,v0,return NULL);
					cknp(ep,expr_addipp(ep,v0),return NULL);
					e+=2;
					goto vend;
*/
				case '{':
					r0=1;
					++e;
					goto block;
/*				case ':':
					if(ep->iflag&EXPR_IF_NOBUILTIN){
						++e;
						break;
					}
					if(e+2<endp&&e[2]==':'){
						builtin=1;
						e+=3;
						break;
					}
*/
				default:
					++e;
					break;
			}
			p=getsym(e,endp);
			if(unlikely(p==e)){
				seterr(ep,EXPR_ECTA);
				*ep->errinfo=*e;
				return NULL;
			}
			sym.uaddr=readsymbol(ep,e,p-e,1,1);
			if(!sym.uaddr)
				return NULL;
			switch(sym.es->type){
				case EXPR_CONSTANT:
				case EXPR_HOTFUNCTION:
					seterr(ep,EXPR_ECTA);
					serrinfo(ep->errinfo,e,p-e);
					return NULL;
				default:
					break;
			}
			un.uaddr=expr_symbol_un(sym.es)->uaddr;
			v0=expr_newvar(ep);
			cknp(ep,v0,return NULL);
			cknp(ep,expr_addconst_i(ep,v0,un.v),return NULL);
			e=p;
			goto vend;
		/*case '0' ... '9':
			goto number;
		case '.':
			switch(e[1]){
			case '0' ... '9':
				goto number;
			default:
				break;
			}*/
		default:
dflt:
			if(unlikely(expr_operator(*e))){
				*ep->errinfo=*e;
				seterr(ep,EXPR_EUO);
				return NULL;
			}
			break;
	}
	p=getsym(e,endp);
symget:
	if(asym&&p-e==asymlen&&!memcmp(e,asym,p-e)){
		v0=expr_newvar(ep);
		cknp(ep,v0,return NULL);
		cknp(ep,expr_addinput(ep,v0),return NULL);
		e=p;
		goto vend;
	}
	sym.uaddr=readsymbol(ep,e,p-e,2,0);
	if(!sym.uaddr){
		if(ep->error)
			return NULL;
	}else {
		type=sym.es->type;
		flag=sym.es->flag;
		goto found;
	}
keyword:
	if(ep->iflag&EXPR_IF_NOKEYWORD)
		goto number;
	for(const struct expr_builtin_keyword *kp=expr_keywords;
			kp->str;++kp){
		if(likely(p-e!=kp->strlen||memcmp(e,kp->str,p-e)))
			continue;
		if(kp->flag&EXPR_KF_NOPROTECT){
			if(unlikely(ep->iflag&EXPR_IF_PROTECT)){
				seterr(ep,EXPR_EPM);
				serrinfo(ep->errinfo,kp->str,kp->strlen);
				return NULL;
			}
			setunsafe(ep);
		}
		if(unlikely(p>=endp||*p!='(')){
			serrinfo(ep->errinfo,e,p-e);
			seterr(ep,EXPR_EFP);
			return NULL;
		}
		p2=e;
		e=p;
		p=findpair(e,endp);
		if(unlikely(!p))
			goto pterr;
		flag=0;
		switch(kp->op){
			case EXPR_CONST:
				sym.vv=expr_sep(ep,e,p-e+1);
				if(unlikely(!sym.vv))
					return NULL;
				for(un.vv1=sym.vv;*un.vv1;++un.vv1);
				switch(un.vv1-sym.vv){
					case 1:
						un.v=0.0;
						break;
					case 2:
						un.v=consteval(sym.vv[1],strlen(sym.vv[1]),asym,asymlen,ep->sset,ep);
						if(unlikely(ep->error))
							goto c_fail;
						break;
					default:
						seterr(ep,EXPR_ENEA);
						serrinfoc(ep->errinfo,"const");
c_fail:
						expr_free2(sym.vv);
						return NULL;
				}
				dim=strlen(sym.vv[0]);
				if(ep->sset&&expr_symset_search(ep->sset,sym.vv[0],dim)){
					seterr(ep,EXPR_EDS);
					serrinfo(ep->errinfo,sym.vv[0],dim);
					goto c_fail;
				}
				r0=expr_createconst(ep,sym.vv[0],dim,un.v);
				expr_free2(sym.vv);
				cknp(ep,!r0,return NULL);
				v0=EXPR_VOID;
				e=p+1;
				goto vend;
			case EXPR_MUL:
				sym.vv=expr_sep(ep,e,p-e+1);
				if(unlikely(!sym.vv))
					return NULL;
				for(un.vv1=sym.vv;*un.vv1;++un.vv1);
				switch(un.vv1-sym.vv){
					case 1:
						un.v=0.0;
						break;
					case 2:
						un.v=consteval(sym.vv[1],strlen(sym.vv[1]),asym,asymlen,ep->sset,ep);
						if(unlikely(ep->error))
							goto c_fail;
						break;
					default:
						seterr(ep,EXPR_ENEA);
						serrinfoc(ep->errinfo,"var");
						goto c_fail;
				}
				if(ep->sset&&expr_symset_search(ep->sset,sym.vv[0],dim=strlen(sym.vv[0]))){
					seterr(ep,EXPR_EDS);
					serrinfo(ep->errinfo,sym.vv[0],dim);
					goto c_fail;
				}
				r0=expr_createsvar(ep,sym.vv[0],dim,un.v);
				expr_free2(sym.vv);
				cknp(ep,!r0,return NULL);
				v0=EXPR_VOID;
				e=p+1;
				goto vend;
			case EXPR_ADD:
				sym.vv=expr_sep(ep,e,p-e+1);
				if(unlikely(!sym.vv))
					return NULL;
				for(un.vv1=sym.vv;*un.vv1;++un.vv1);
				switch(un.vv1-sym.vv){
					case 1:
						un.v=0;
						break;
					case 2:
						dim=strlen(sym.vv[0]);
						un.v=consteval(sym.vv[1],dim,asym,asymlen,ep->sset,ep);
						if(unlikely(ep->error))
							goto c_fail;
						break;
					default:
						seterr(ep,EXPR_ENEA);
						serrinfoc(ep->errinfo,"decl");
						goto c_fail;
				}
				cknp(ep,expr_detach(ep)>=0,goto c_fail);
				if(!ep->sset||!(sv.es=expr_symset_search(ep->sset,sym.vv[0],dim=strlen(sym.vv[0])))){
					seterr(ep,EXPR_ESYMBOL);
					serrinfo(ep->errinfo,sym.vv[0],dim);
					goto c_fail;
				}else {
alias_found_decl:
					switch(sv.es->type){
						case EXPR_CONSTANT:
						case EXPR_VARIABLE:
							break;
						case EXPR_ALIAS:
							sv.ces=alias_target(ep,sv.es,1);
							if(likely(sv.ces)){
								goto alias_found_decl;
							}
							goto c_fail;
						default:
							seterr(ep,EXPR_ETNV);
							serrinfo(ep->errinfo,sym.vv[0],dim);
							goto c_fail;
					}
				}
				expr_free2(sym.vv);
				flag=sv.es->flag;
				sv.es->flag=((int)un.v&~EXPR_SF_PMASK)|(flag&EXPR_SF_PMASK);
				v0=EXPR_VOID;
				e=p+1;
				goto vend;
			case EXPR_HOT:
				sym.vv=expr_sep(ep,e,p-e+1);
				if(unlikely(!sym.vv))
					return NULL;
				for(un.vv1=sym.vv;*un.vv1;++un.vv1);
				switch(un.vv1-sym.vv){
					case 2:
						break;
					default:
						seterr(ep,EXPR_ENEA);
						serrinfoc(ep->errinfo,"hot");
						goto c_fail;
				}
				dim=strlen(sym.vv[0]);
				if(unlikely(!dim))
					goto eev;
				p4=getsym(sym.vv[0],sym.vv[0]+dim);
				if(unlikely(*p4)){
					seterr(ep,EXPR_EUO);
					*ep->errinfo=*p4;
					goto c_fail;
				}
				if(ep->sset&&expr_symset_search(ep->sset,sym.vv[0],dim)){
					seterr(ep,EXPR_EDS);
					serrinfo(ep->errinfo,sym.vv[0],dim);
					goto c_fail;
				}
				r0=expr_createhot(ep,sym.vv[0],dim,sym.vv[1],strlen(sym.vv[1]),0);
				expr_free2(sym.vv);
				cknp(ep,!r0,return NULL);
				v0=EXPR_VOID;
				e=p+1;
				goto vend;
			case EXPR_INPUT:
				type=sizeof(struct expr_internal_jmpbuf);
				goto use_byte;
			case EXPR_COPY:
				type=1;
				goto use_byte;
			case EXPR_BL:
				type=sizeof(double);
use_byte:
				sym.vv=expr_sep(ep,e,p-e+1);
				if(unlikely(!sym.vv))
					return NULL;
				for(un.vv1=sym.vv;*un.vv1;++un.vv1);
				dim=un.vv1-sym.vv;
				sv.i=(size_t)consteval(sym.vv[0],strlen(sym.vv[0]),asym,asymlen,ep->sset,ep);
				if(unlikely(ep->error))
					goto c_fail;
				v0=expr_newvar(ep);
				cknp(ep,v0,goto c_fail);
				un.uaddr=xmalloc((size_t)type*sv.i);
				cknp(ep,un.uaddr,goto c_fail);
				p4=(const char *)expr_newres(ep);
				cknp(ep,p4,xfree(un.uaddr);goto c_fail);
				((struct expr_resource *)p4)->un.uaddr=un.uaddr;
				cknp(ep,expr_addconst(ep,v0,un.v),goto c_fail);
				if(--dim){
					if(dim>sv.i)
						dim=un.sz;
					if(dim){
						do {
							sv.v=consteval(sym.vv[dim],strlen(sym.vv[dim]),asym,asymlen,ep->sset,ep);
							--dim;
							if(unlikely(ep->error))
								goto c_fail;
							switch(type){
								case sizeof(double):
									un.daddr[dim]=sv.v;
									break;
								case 1:
									un.baddr[dim]=(int8_t)sv.v;
									break;
								default:
									break;
							}
						}while(dim);
					}
				}
				expr_free2(sym.vv);
				e=p+1;
				goto vend;
			case EXPR_SUB:
				un.v=consteval(e+1,p-e-1,asym,asymlen,ep->sset,ep);
				if(unlikely(ep->error))
					return NULL;
				if(unlikely(un.v==0.0)){
					seterr(ep,EXPR_ESAF);
					serrinfo(ep->errinfo,e+1,p-e-1);
					return NULL;
				}
				v0=EXPR_VOID;
				e=p+1;
				goto vend;
			case EXPR_DIV:
				if(!ep->sset||!(sym.es=expr_symset_search(ep->sset,e+1,p-e-1))){
					un.v=0.0;
				}else {
					un.v=1.0;
				}
				v0=expr_newvar(ep);
				cknp(ep,v0,return NULL);
				cknp(ep,expr_addconst(ep,v0,un.v),return NULL);
				e=p+1;
				goto vend;
#define goto_symerr {\
	serrinfo(ep->errinfo,e,p-e);\
	seterr(ep,EXPR_ESYMBOL);\
	return NULL;\
}

#define symfield(_field,_dest) \
				if(likely(ep->sset&&(sym.es=expr_symset_search(ep->sset,e+1,p-e-1)))){\
					sv.ces=alias_target(ep,sym.es,1);\
					if(likely(sv.ces)){\
						_dest=sv.ces->_field;\
					}else {\
						return NULL;\
					}\
				}else {\
					++e;\
					goto_symerr;\
				}
			case EXPR_AND:
				symfield(type,flag);
				v0=expr_newvar(ep);
				cknp(ep,v0,return NULL);
				cknp(ep,expr_addconst(ep,v0,(double)flag),return NULL);
				e=p+1;
				goto vend;
			case EXPR_OR:
				symfield(flag,flag);
				v0=expr_newvar(ep);
				cknp(ep,v0,return NULL);
				cknp(ep,expr_addconst(ep,v0,(double)flag),return NULL);
				e=p+1;
				goto vend;
			case EXPR_XOR:
				if(p+1<endp&&p[1]=='{')
					p2=findpair_brace(p+1,endp);
				else
					p2=NULL;
				switch(e[1]){
					case ')':
					case '*':
						goto found1;
					default:
						break;
				}
				if(unlikely(!ep->sset||!(sym.es=expr_symset_search(ep->sset,e+1,p-e-1)))){
					++e;
					goto_symerr;
				}
found1:
#define do_undef(_se) \
				cknp(ep,expr_detach(ep)>=0,return NULL);\
				if(p2){\
					un.esp=ep->sset?expr_symset_clone(ep->sset):new_expr_symset();\
					cknp(ep,un.esp,return NULL);\
					sym.esp=ep->sset;\
					ep->sset=un.esp;\
					if(_se){\
						if(unlikely(expr_rmsym(ep,e+1,p-e-1))){\
							++e;\
							ep->sset=sym.esp;\
							expr_symset_free(un.esp);\
							goto_symerr;\
						}\
					}else {\
						expr_rmsym(ep,e+1,p-e-1);\
					}\
					v0=scan(ep,p+2,p2,asym,asymlen);\
					ep->sset=sym.esp;\
					expr_symset_free(un.esp);\
					if(unlikely(!v0))\
						return NULL;\
					e=p2+1;\
				}else {\
					if(_se){\
						if(unlikely(expr_rmsym(ep,e+1,p-e-1))){\
							++e;\
							goto_symerr;\
						}\
					}else {\
						expr_rmsym(ep,e+1,p-e-1);\
					}\
					v0=EXPR_VOID;\
					e=p+1;\
				}\
				goto vend
				do_undef(1);
			case EXPR_NOTL:
				if(p+1<endp&&p[1]=='{')
					p2=findpair_brace(p+1,endp);
				else
					p2=NULL;
				switch(e[1]){
					case ')':
					case '*':
						goto found2;
					default:
						break;
				}
found2:
				do_undef(0);
			case EXPR_ORL:
				if(p==e+1){
					v0=expr_newvar(ep);
					cknp(ep,v0,return NULL);
					cknp(ep,expr_addconst(ep,v0,(double)ep->iflag),return NULL);
					e=p+1;
					goto vend;
				}
				un.v=consteval(e+1,p-e-1,asym,asymlen,ep->sset,ep);
				if(unlikely(ep->error))
					return NULL;
				flag=(int)un.v;
				if(flag&EXPR_IF_UNSAFE)
					setunsafe(ep);
				flag&=EXPR_IF_SETABLE;
				type=ep->iflag;
				un.flag=type;
				if(type&EXPR_IF_PROTECT){
					if(flag==(type&~EXPR_IF_PROTECT)){
						if(likely(type&EXPR_IF_UNSAFE)){
							type&=~EXPR_IF_PROTECT;
						}else {
							goto flpm;
						}
					}else {
						if(unlikely(type&EXPR_IF_SETABLE&~flag)){
flpm:
							seterr(ep,EXPR_EPM);
							serrinfoc(ep->errinfo,"flag");
							return NULL;
						}
					}
				}
				ep->iflag=(type&~EXPR_IF_SETABLE)|flag;
				++p;
				if(p<endp&&*p=='{'&&likely(p2=findpair_brace(p,endp))){
					v0=scan(ep,p+1,p2,asym,asymlen);
					if(unlikely(!v0))
						return NULL;
					e=p2+1;
					ep->iflag=un.flag;
				}else {
					v0=EXPR_VOID;
					e=p;
				}
				goto vend;
			case EXPR_XORL:
				if(unlikely(p!=e+1)){
					seterr(ep,EXPR_EZAFP);
					serrinfoc(ep->errinfo,"scope");
					return NULL;
				}
				++p;
				if(unlikely(p>=endp||*p!='{'||!(p2=findpair_brace(p,endp)))){
					goto pterr;
				}
				cknp(ep,expr_detach(ep)>=0,return NULL);
				flag=ep->iflag;
				un.esp=ep->sset?expr_symset_clone(ep->sset):new_expr_symset();
				cknp(ep,un.esp,return NULL);
				sym.esp=ep->sset;
				ep->sset=un.esp;
				v0=scan(ep,p+1,p2,asym,asymlen);
				ep->sset=sym.esp;
				expr_symset_free(un.esp);
				ep->iflag=flag;
				if(unlikely(!v0))
					return NULL;
				e=p2+1;
				goto vend;
			case EXPR_NEXT:
				un.v=nonconsteval(e+1,p-e-1,asym,asymlen,ep->sset,ep);
				if(unlikely(ep->error))
					return NULL;
				v0=expr_newvar(ep);
				cknp(ep,v0,return NULL);
				cknp(ep,expr_addconst(ep,v0,un.v),return NULL);
				e=p+1;
				goto vend;
			case EXPR_DIFF:
				un.v=constcheck(e+1,p-e-1,asym,asymlen,ep->sset,ep);
				if(unlikely(ep->error))
					return NULL;
				v0=expr_newvar(ep);
				cknp(ep,v0,return NULL);
				cknp(ep,expr_addconst(ep,v0,un.v),return NULL);
				e=p+1;
				goto vend;
			case EXPR_NEG:
				if(p+1>=endp||p[1]!='{')
					goto pterr;
				p2=findpair_brace(p+1,endp);
				if(!p2)
					goto pterr;
				if(p2==p+2)
					goto eev;
				switch(e[1]){
					case ')':
						type=0;
						break;
					default:
						if(unlikely(ep->sset&&expr_symset_search(ep->sset,e+1,p-e-1))){
							serrinfo(ep->errinfo,e+1,p-e-1);
							seterr(ep,EXPR_EDS);
							return NULL;
						}
						type=1;
						break;
				}
				un.ep=new_expr8p(p+2,p2-p-2,asym,asymlen,ep->sset,ep->iflag,&flag,NULL,ep);
				if(un.ep){
					flag=0;
					v0=expr_newvar(ep);
					cknp(ep,v0,expr_free(un.ep);return NULL);
					cknp(ep,expr_addop(ep,v0,un.ep,EXPR_EP,0),expr_free(un.ep);return NULL);
				}else
					v0=EXPR_VOID;
				if(type){
					cknp(ep,expr_detach(ep)>=0,return NULL);
					cknp(ep,!expr_createconst(ep,e+1,p-e-1,(double)flag),return NULL);
				}
				e=p2+1;
				goto vend;
			case EXPR_NOT:
				sym.vv=expr_sep(ep,e,p-e+1);
				if(unlikely(!sym.vv))
					return NULL;
				for(un.vv1=sym.vv;*un.vv1;++un.vv1);
				switch(un.vv1-sym.vv){
					case 2:
						break;
					default:
						seterr(ep,EXPR_ENEA);
						serrinfoc(ep->errinfo,"alias");
						goto c_fail;
				}
				dim=strlen(sym.vv[0]);
				if(unlikely(!dim))
					goto eev;
				p4=getsym(sym.vv[0],sym.vv[0]+dim);
				if(unlikely(*p4)){
					seterr(ep,EXPR_EUO);
					*ep->errinfo=*p4;
					goto c_fail;
				}
				if(ep->sset&&expr_symset_search(ep->sset,sym.vv[0],dim)){
					seterr(ep,EXPR_EDS);
					serrinfo(ep->errinfo,sym.vv[0],dim);
					goto c_fail;
				}
				cknp(ep,expr_detach(ep)>=0,goto c_fail);
				un.ebp=expr_symset_addl(ep->sset,sym.vv[0],dim,EXPR_ALIAS,0,sym.vv[1]);
				expr_free2(sym.vv);
				cknp(ep,un.ebp,return NULL);
				v0=EXPR_VOID;
				e=p+1;
				goto vend;
			case EXPR_TSTL:
				if(p>e+1){
					if(unlikely(e[1]!='\"'||findpair_dmark(e+1,endp)!=p-1))
						goto pterr;
					dim=p-e-3;
					if(dim)
						expr_strscan(e+2,dim,ep->errinfo,EXPR_SYMLEN);
				}
				seterr(ep,EXPR_EUDE);
				return NULL;
			case EXPR_ALO:
				sym.vv=expr_sep(ep,e,p-e+1);
				if(unlikely(!sym.vv))
					return NULL;
				for(un.vv1=sym.vv;*un.vv1;++un.vv1);
				switch(un.vv1-sym.vv){
					case 1:
						dim=1;
						break;
					case 2:
						un.v=consteval(sym.vv[1],strlen(sym.vv[1]),asym,asymlen,ep->sset,ep);
						if(unlikely(ep->error))
							goto c_fail;
						dim=(size_t)fabs(un.v);
						break;
					default:
						seterr(ep,EXPR_ENEA);
						serrinfoc(ep->errinfo,"alloca");
						goto c_fail;
				}
				v0=scan(ep,sym.vv[0],sym.vv[0]+strlen(sym.vv[0]),asym,asymlen);
				expr_free2(sym.vv);
				if(unlikely(!v0))
					return NULL;
				cknp(ep,expr_addalo(ep,v0,dim),return NULL);
				e=p+1;
				goto vend;
			case EXPR_SVCP:
				sym.vv=expr_sep(ep,e,p-e+1);
				if(unlikely(!sym.vv))
					return NULL;
				for(un.vv1=sym.vv;*un.vv1;++un.vv1);
				switch(un.vv1-sym.vv){
					case 3:
						un.v=consteval(sym.vv[2],strlen(sym.vv[2]),asym,asymlen,ep->sset,ep);
						if(unlikely(ep->error))
							goto c_fail;
						dim=(size_t)fabs(un.v);
						switch(dim){
							case 0 ... EXPR_SYSAM:
								break;
							default:
								goto svc_enea;
						}
						break;
					default:
svc_enea:
						seterr(ep,EXPR_ENEA);
						serrinfoc(ep->errinfo,"svcp");
						goto c_fail;
				}
				v0=scan(ep,sym.vv[0],sym.vv[0]+strlen(sym.vv[0]),asym,asymlen);
				if(unlikely(!v0))
					goto c_fail;
				v1=scan(ep,sym.vv[1],sym.vv[1]+strlen(sym.vv[1]),asym,asymlen);
				expr_free2(sym.vv);
				if(unlikely(!v1))
					return NULL;
				cknp(ep,expr_addsvcp(ep,v0,v1,(int)dim),return NULL);
				e=p+1;
				goto vend;
			case EXPR_SVC:
				sym.vv=expr_sep(ep,e,p-e+1);
				if(unlikely(!sym.vv))
					return NULL;
				for(un.vv1=sym.vv;*un.vv1;++un.vv1);
				dim=un.vv1-sym.vv;
				switch(dim){
					case 1 ... EXPR_SYSAM+1:
						break;
					default:
						seterr(ep,EXPR_ENEA);
						serrinfoc(ep->errinfo,"svc");
						goto c_fail;
				}
				v0=scan(ep,sym.vv[0],sym.vv[0]+strlen(sym.vv[0]),asym,asymlen);
				if(unlikely(!v0))
					goto c_fail;
				--dim;
				if(dim){
					sv.i=0;
					do {
						un.ccp=sym.vv[sv.i+1];
						v1=scan(ep,un.ccp,un.ccp+strlen(un.ccp),asym,asymlen);
						if(unlikely(!v1))
							goto c_fail;
						cknp(ep,expr_addcopy(ep,ep->un.args+sv.i,v1),goto c_fail);
						++sv.i;
					}while(sv.i<dim);
				}
				expr_free2(sym.vv);
				cknp(ep,expr_addsvc(ep,v0,(int)dim),return NULL);
				e=p+1;
				goto vend;
			case EXPR_SJ:
				v0=scan(ep,e+1,p,asym,asymlen);
				if(unlikely(!v0))
					return NULL;
				cknp(ep,expr_addsj(ep,v0),return NULL);
				e=p+1;
				goto vend;
			case EXPR_LJ:
				sym.vv=expr_sep(ep,e,p-e+1);
				if(unlikely(!sym.vv))
					return NULL;
				for(un.vv1=sym.vv;*un.vv1;++un.vv1);
				if(unlikely(un.vv1-sym.vv!=2)){
					seterr(ep,EXPR_ENEA);
					serrinfoc(ep->errinfo,"longjmp");
					goto c_fail;
				}
				v0=scan(ep,sym.vv[0],sym.vv[0]+strlen(sym.vv[0]),asym,asymlen);
				if(unlikely(!v0))
					goto c_fail;
				v1=scan(ep,sym.vv[1],sym.vv[1]+strlen(sym.vv[1]),asym,asymlen);
				expr_free2(sym.vv);
				if(unlikely(!v1))
					return NULL;
				cknp(ep,expr_addlj(ep,v0,v1),return NULL);
				e=p+1;
				goto vend;
			case EXPR_IP:
				if(p!=e+1){
					un.v=consteval(e+1,p-e-1,asym,asymlen,ep->sset,ep);
					if(unlikely(ep->error))
						return NULL;
					dim=(size_t)un.v;
					switch(dim){
						case 0 ... 3:
							break;
						default:
							seterr(ep,EXPR_EZAFP);
							serrinfoc(ep->errinfo,"ip");
							return NULL;
					}
				}else
					dim=0;
				v0=expr_newvar(ep);
				cknp(ep,v0,return NULL);
				cknp(ep,expr_addip(ep,v0,dim),return NULL);
				e=p+1;
				goto vend;
			case EXPR_TO:
				v0=scan(ep,e+1,p,asym,asymlen);
				if(unlikely(!v0))
					return NULL;
				cknp(ep,expr_addto(ep,v0),return NULL);
				e=p+1;
				goto vend;
			case EXPR_TO1:
				v0=scan(ep,e+1,p,asym,asymlen);
				if(unlikely(!v0))
					return NULL;
				cknp(ep,expr_addto1(ep,v0),return NULL);
				e=p+1;
				goto vend;
#define EVALLIKE(_op,_err) \
				sym.vv=expr_sep(ep,e,p-e+1);\
				if(unlikely(!sym.vv))\
					return NULL;\
				for(un.vv1=sym.vv;*un.vv1;++un.vv1);\
				switch(un.vv1-sym.vv){\
					case 2:\
						v0=scan(ep,sym.vv[1],sym.vv[1]+strlen(sym.vv[1]),asym,asymlen);\
						if(unlikely(!v0))\
							goto c_fail;\
						break;\
					default:\
						seterr(ep,EXPR_ENEA);\
						serrinfoc(ep->errinfo,_err);\
						goto c_fail;\
				}\
\
				v1=scan(ep,sym.vv[0],sym.vv[0]+strlen(sym.vv[0]),asym,asymlen);\
				expr_free2(sym.vv);\
				if(unlikely(!v1))\
					return NULL;\
				cknp(ep,expr_addop(ep,v0,v1,_op,0),return NULL);\
				e=p+1;\
				goto vend
			case EXPR_EVAL:
				EVALLIKE(EXPR_EVAL,"eval");
			case EXPR_RET:
				EVALLIKE(EXPR_RET,"return");
			case BRANCHCASES:
				if(p+1<endp&&p[1]=='{'){
					sym.b=alloca(sizeof(struct branch));
					sym.b->cond=e+1;
					sym.b->scond=p-e-1;
					e=p+1;
					p=findpair_brace(e,endp);
					if(unlikely(!p))
						goto pterr;
					sym.b->body=e+1;
					sym.b->sbody=p-e-1;
					if(++p<endp)switch(*p){
						default:
								goto vzero;
						case '{':
							switch(kp->op){
								case EXPR_IF:
									break;
								default:
									seterr(ep,EXPR_EUO);
									*ep->errinfo='{';
									return NULL;
							}
							e=p;
							p=findpair_brace(e,endp);
							if(unlikely(!p))
								goto pterr;
							sym.b->value=e+1;
							sym.b->svalue=p-e-1;
							break;
					}else {
vzero:
						--p;
						sym.b->svalue=0;
					}
					if(!sym.b->scond){
						serrinfo(ep->errinfo,kp->str,kp->strlen);
						goto envp;
					}
					un.eb=getbranchinfo(ep,NULL,0,NULL,0,sym.b,asym,asymlen);
				}else
					un.eb=getbranchinfo(ep,p2,e-p2,e,p-e+1,NULL,asym,asymlen);
				if(unlikely(!un.eb)){
					return NULL;
				}
				if(kp->op==EXPR_IF){
					v0=expr_newvar(ep);
					cknp(ep,v0,freebranchinfo(un.eb);return NULL);
				}else
					v0=EXPR_VOID;
				cknp(ep,expr_addop(ep,v0,un.uaddr,kp->op,flag),freebranchinfo(un.eb);return NULL);
				e=p+1;
				goto vend;
			case EXPR_ANDL:
				if(p+1>=endp||p[1]!='{')
					goto pterr;
				un.v=consteval(e+1,p-e-1,asym,asymlen,ep->sset,ep);
				if(unlikely(ep->error))
					return NULL;
				e=p+1;
				p=findpair_brace(e,endp);
				if(unlikely(!p))
					goto pterr;
				if(un.v!=0.0){
					v0=scan(ep,e+1,p,asym,asymlen);
					if(unlikely(!v0))
						return NULL;
					e=p+1;
					if(e<endp&&*e=='{'){
						p=findpair_brace(e,endp);
						if(likely(p))
							e=p+1;
					}
					goto vend;
				}
				if(p+1>=endp){
					v0=EXPR_VOID;
					e=p+1;
					goto vend;
				}
				switch(p[1]){
					case '{':
						e=p+1;
						p=findpair_brace(e,endp);
						if(unlikely(!p))
							goto pterr;
						v0=scan(ep,e+1,p,asym,asymlen);
						if(unlikely(!v0))
							return NULL;
						e=p+1;
						goto vend;
					default:
						v0=EXPR_VOID;
						e=p+1;
						goto vend;
				}
			case EXPR_DO:
				p=findpair(e,endp);
				if(unlikely(!p))
					goto pterr;
				if(unlikely(p==e+1))
					goto envp;
				un.ep=new_expr8p(e+1,p-e-1,asym,asymlen,ep->sset,ep->iflag,&ep->error,ep->errinfo,ep);
				if(unlikely(!un.ep))
					return NULL;
				cknp(ep,expr_addop(ep,v0=EXPR_VOID_NR,un.ep,EXPR_DO,0),expr_free(un.ep);return NULL);
				e=p+1;
				goto vend;
#define addinfo(_op,dal) \
				if(unlikely(!un.uaddr)){\
					return NULL;\
				}\
				v0=expr_newvar(ep);\
				cknp(ep,v0,dal(un.uaddr);return NULL);\
				cknp(ep,expr_addop(ep,v0,un.uaddr,_op,flag),dal(un.uaddr);return NULL);\
				e=p+1;\
				goto vend
			case EXPR_VMD:
				un.ev=getvmdinfo(ep,p2,e-p2,e,p-e+1,asym,asymlen,&flag);
				addinfo(EXPR_VMD,freevmdinfo);
			case SUMCASES:
				un.es=getsuminfo(ep,p2,e-p2,e,p-e+1,asym,asymlen);
				addinfo(kp->op,freesuminfo);
			default:
				__builtin_unreachable();
		}
/*		if(unlikely(!un.uaddr)){
			return NULL;
		}
		v0=expr_newvar(ep);
		cknp(ep,v0,return NULL);
		cknp(ep,expr_addop(ep,v0,un.uaddr,kp->op,flag),return NULL);
		e=p+1;
		goto vend;*/
		abort();
	}
	goto number;
found:
	switch(type){
		case EXPR_FUNCTION:
			if(flag&EXPR_SF_UNSAFE){
				if(!(flag&EXPR_SF_ALLOWADDR))
					goto fpm;
				if(p+2>=endp||p[1]!='&')
					goto fpm;
				un.ccp=getsym(p+2,endp);
				if(unlikely(p+2==un.ccp)){
					goto fpm;
				}
				if(unlikely(!ep->sset||!(un.ebp=expr_symset_search(ep->sset,p+2,un.ccp-p-2)))){
					goto fpm;
				}
				switch(un.ebp->type){
					case EXPR_VARIABLE:
						type=1;
						goto fok;
					default:
						goto fpm;
				}
			}
			type=0;
			goto fok;
fpm:
			if(unlikely(ep->iflag&EXPR_IF_PROTECT))
				goto pm;
			type=0;
			setunsafe(ep);
fok:
			if(unlikely(p>=endp||*p!='(')){
				serrinfo(ep->errinfo,e,p-e);
				seterr(ep,EXPR_EFP);
				return NULL;
			}
			e=p;
			p=findpair(e,endp);
			if(unlikely(!p))
				goto pterr;
			if(unlikely(p==e+1))
				goto envp;
			if(type){
				v0=expr_newvar(ep);
				cknp(ep,v0,return NULL);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
				cknp(ep,expr_addconst(ep,v0,expr_symbol_un(un.ebp)->value),return NULL);
#pragma GCC diagnostic pop
			}else {
				v0=scan(ep,e+1,p,asym,asymlen);
				if(unlikely(!v0))
					return NULL;
			}
			cknp(ep,expr_addcall(ep,v0,expr_symbol_un(sym.es)->func,flag),return NULL);
			e=p+1;
			goto vend;
		case EXPR_ZAFUNCTION:
			if(unlikely(p+1>=endp||*p!='('||p[1]!=')')){
				serrinfo(ep->errinfo,e,p-e);
				seterr(ep,EXPR_EZAFP);
				return NULL;
			}
			v0=expr_newvar(ep);
			cknp(ep,v0,return NULL);
			cknp(ep,expr_addza(ep,v0,expr_symbol_un(sym.es)->zafunc,flag),return NULL);
			e=p+2;
			goto vend;
		case EXPR_HOTFUNCTION:
			if(!(flag&EXPR_SF_WRITEIP)){
#define sset_push(_start,_end) \
				cknp(ep,expr_detach(ep)>=0,return NULL);\
				sym.esp=expr_symset_clone(ep->sset);\
				cknp(ep,sym.esp,return NULL);\
				expr_symset_remove(sym.esp,_start,_end-_start)
				un.ccp=expr_symbol_hot(sym.es);
				dim=HOTLEN(sym.es);
				sset_push(e,p);
				v0=gethot(ep,e,p-e,p,endp,asym,asymlen,un.ccp,dim,&e,sym.esp,flag);
				expr_symset_free(sym.esp);
				if(!v0)
					return NULL;
				goto vend;
			}
			if(unlikely(p>=endp||*p!='(')){
				serrinfo(ep->errinfo,e,p-e);
				seterr(ep,EXPR_EFP);
				return NULL;
			}
			p2=e;
			p4=p;
			e=p;
			p=findpair(e,endp);
			if(unlikely(!p))
				goto pterr;
			if(unlikely(p==e+1))
				goto envp;
			v0=scan(ep,e+1,p,asym,asymlen);
			if(unlikely(!v0))
				return NULL;
			un.ccp=expr_symbol_hot(sym.es);
			dim=HOTLEN(sym.es);
			sset_push(p2,p4);
			un.ep=new_expr8p(un.ccp,dim,asym,
			asymlen,sym.esp,ep->iflag,&ep->error,ep->errinfo,ep);
			expr_symset_free(sym.esp);
			if(unlikely(!un.ep))
				return NULL;
			cknp(ep,expr_addhot(ep,v0,un.ep,flag&~EXPR_SF_INJECTION),return NULL);
			e=p+1;
			goto vend;
		case EXPR_CONSTANT:
			v0=expr_newvar(ep);
			cknp(ep,v0,return NULL);
			cknp(ep,expr_addconst(ep,v0,expr_symbol_un(sym.es)->value),return NULL);
			goto treat_as_variable;
		case EXPR_VARIABLE:
			v0=expr_newvar(ep);
			cknp(ep,v0,return NULL);
			cknp(ep,expr_addcopy(ep,v0,expr_symbol_un(sym.es)->addr),return NULL);
treat_as_variable:
			p2=e;
			e=p;
			if(e<endp&&*e=='('){
				if(unlikely(ep->iflag&EXPR_IF_PROTECT)){
					p=findpair(e,endp);
					if(unlikely(!p))
						goto pterr;
					seterr(ep,EXPR_EPM);
					serrinfo(ep->errinfo,e0,e-e0);
					return NULL;
				}
				setunsafe(ep);
				switch(flag&~EXPR_SF_PMASK){
					case EXPR_SF_PMD:
						p=findpair(e,endp);
						if(unlikely(!p))
							goto pterr;
						un.em=getmdinfo(ep,p2,e-p2,e,p-e+1,asym,asymlen,NULL,0,0);
						addinfo(EXPR_PMD,freemdinfo);
						/*if(unlikely(!un.em))
							return NULL;
						cknp(ep,expr_addop(ep,v0,un.em,EXPR_PMD,0),return NULL);
						e=p+1;
						goto vend;*/
					case EXPR_SF_PME:
						p=findpair(e,endp);
						if(unlikely(!p))
							goto pterr;
						un.em=getmdinfo(ep,p2,e-p2,e,p-e+1,asym,asymlen,NULL,0,1+!!(flag&EXPR_SF_WRITEIP));
						addinfo((flag&EXPR_SF_WRITEIP)?EXPR_PMEP:EXPR_PME,freemdinfo);
						/*if(unlikely(!un.em))
							return NULL;
						cknp(ep,expr_addop(ep,v0,un.em,(flag&EXPR_SF_WRITEIP)?EXPR_PMEP:EXPR_PME,0),return NULL);
						e=p+1;
						goto vend;*/
					case EXPR_SF_PFUNC:
						break;
						/*
						p=findpair(e,endp);
						if(unlikely(!p))
							goto pterr;
						v1=scan(ep,e+1,p,asym,asymlen);
						if(unlikely(!v1))
							return NULL;
						cknp(ep,expr_addop(ep,v1,v0,EXPR_EVAL,0),return NULL);
						e=p+1;
						v0=v1;
						goto vend;
						*/
					default:
						goto vend;
				}
				if(e+1<endp&&e[1]==')'){
					v1=expr_newvar(ep);
					cknp(ep,v1,return NULL);
					cknp(ep,expr_addop(ep,v1,v0,EXPR_PZA,0),return NULL);
					e+=2;
				}else {
					p=findpair(e,endp);
					if(unlikely(!p))
						goto pterr;
					v1=scan(ep,e+1,p,asym,asymlen);
					if(unlikely(!v1))
						return NULL;
					cknp(ep,expr_addop(ep,v1,v0,EXPR_PBL,0),return NULL);
					e=p+1;
				}
				v0=v1;
			}
			goto vend;
		case EXPR_MDFUNCTION:
		case EXPR_MDEPFUNCTION:
			if(p>=endp||*p!='('){
				serrinfo(ep->errinfo,e,p-e);
				seterr(ep,EXPR_EFP);
				return NULL;
			}
			p2=e;
			e=p;
			p=findpair(e,endp);
			if(unlikely(!p))
				goto pterr;
			dim=SYMDIM(sym.es);
			switch(type){
				case EXPR_MDFUNCTION:
					un.em=getmdinfo(ep,p2,e-p2,e,p-e+1,asym,asymlen,expr_symbol_un(sym.es)->mdfunc,dim,0);
				break;
				case EXPR_MDEPFUNCTION:
					un.em=getmdinfo(ep,p2,e-p2,e,p-e+1,asym,asymlen,expr_symbol_un(sym.es)->mdepfunc,dim,1+!!(flag&EXPR_SF_WRITEIP));
				break;
			}
			if(unlikely(!un.em)){
				return NULL;
			}
			v0=expr_newvar(ep);
			cknp(ep,v0,freemdinfo(un.em);return NULL);
			switch(type){
				case EXPR_MDFUNCTION:
					cknp(ep,expr_addmd(ep,v0,un.em,flag),freemdinfo(un.em);return NULL);
					break;
				case EXPR_MDEPFUNCTION:
					cknp(ep,(flag&EXPR_SF_WRITEIP?expr_addmep:expr_addme)
					(ep,v0,un.em,flag),freemdinfo(un.em);return NULL);
					break;
			}
			e=p+1;
			goto vend;
		case EXPR_ALIAS:
			v0=alias_scan(ep,sym.es,asym,asymlen);
			if(unlikely(!v0))
				return NULL;
			e=p;
			goto vend;
		default:
			goto_symerr;
	}
number:
	p=getsym_expo(e,endp);
	r0=atod(e,p-e,&un.v);
	if(r0==1){
		v0=expr_newvar(ep);
		cknp(ep,v0,return NULL);
		cknp(ep,expr_addconst(ep,v0,un.v),return NULL);
		e=p;
		goto vend;
	}
	goto sym_notfound;
/*
ein:
	serrinfo(ep->errinfo,e,p-e);
	seterr(ep,EXPR_EIN);
	return NULL;
*/
sym_notfound:
	if(unlikely(p+1>=endp||*p!='='))
		goto_symerr;
	p2=e;
	++p;
	if(*p==':'){
		++p;
		type=1;
	}else
		type=0;
	e=p;
	p4=p;
	switch(*p){
		case '(':
			p=findpair(e,endp);
			if(unlikely(!p))
				goto pterr;
			if(p+1>=endp||p[1]!='{'){
				++p;
				goto constant;
			}
			if(unlikely(type)){
				seterr(ep,EXPR_EUO);
				*ep->errinfo=':';
				return NULL;
			}
			if(asym&&p-e-1==asymlen&&!memcmp(e+1,asym,asymlen)){
				flag=EXPR_SF_WRITEIP;
				e=p+2;
			}else
				flag=0;
			p=findpair_brace(p+1,endp);
			if(unlikely(!p))
				goto pterr;
			if(!flag)
				++p;
			r0=expr_createhot(ep,p2,p4-1-p2,e,p-e,flag);
			cknp(ep,!r0,return NULL);
			v0=EXPR_VOID;
			e=p;
			if(flag)
				++e;
			goto vend;
		default:
			p=getsym_p(e,endp);
constant:
			if(unlikely(e==p)){
				seterr(ep,EXPR_EEV);
				return NULL;
			}
			un.v=consteval(e,p-e,asym,asymlen,ep->sset,ep);
			if(unlikely(ep->error))
				return NULL;
			if(type)
				r0=expr_createvar4(ep,p2,e-2-p2,un.v);
			else
				r0=expr_createconst(ep,p2,e-1-p2,un.v);
			cknp(ep,!r0,return NULL);
			v0=EXPR_VOID;
			e=p;
			goto vend;
	}
pm:
	serrinfo(ep->errinfo,e,p-e);
	seterr(ep,EXPR_EPM);
	return NULL;
vend:
	if(e<endp&&*e=='['){
		if(unlikely(ep->iflag&EXPR_IF_PROTECT)){
			p=findpair_bracket(e,endp);
			if(unlikely(!p))
				goto pterr;
			seterr(ep,EXPR_EPM);
			serrinfo(ep->errinfo,e0,p-e0+1);
			return NULL;
		}
		setunsafe(ep);
		do{
			double *v2;
			p=findpair_bracket(e,endp);
			if(unlikely(!p))
				goto pterr;
			if(unlikely(p==e+1))
				goto envp;
			v1=scan(ep,e+1,p,asym,asymlen);
			if(unlikely(!v1))
				return NULL;
			v2=v0;
			v0=expr_newvar(ep);
			cknp(ep,v0,return NULL);
			cknp(ep,expr_addoff(ep,v2,v1),return NULL);
			cknp(ep,expr_addread(ep,v0,v2),return NULL);
			e=p+1;
		}while(e<endp&&*e=='[');
	}
	if(likely(_p))*_p=e;
	return v0;
}
struct vnode {
	struct vnode *next;
	double *v;
	enum expr_op op;
	uint64_t unary;
};
static struct vnode *vn(double *v,enum expr_op op,uint64_t unary){
	struct vnode *p;
	p=xmalloc(sizeof(struct vnode));
	if(unlikely(!p))
		return NULL;
	p->next=NULL;
	p->v=v;
	p->op=op;
	p->unary=unary;
	return p;
}
static struct vnode *vnadd(struct vnode *vp,double *v,enum expr_op op,uint64_t unary){
	struct vnode *p;
	if(unlikely(!vp))
		return vn(v,op,unary);
	for(p=vp;p->next;p=p->next);
	p->next=xmalloc(sizeof(struct vnode));
	if(unlikely(!p->next))
		return NULL;
	p->next->v=v;
	p->next->op=op;
	p->next->unary=unary;
	p->next->next=NULL;
	return vp;
}
static inline int do_unary_forone(struct expr *restrict ep,struct vnode *p){
redo:
	switch(p->unary&7){
		case 1:
			cknp(ep,expr_addneg(ep,p->v),return -1);
			break;
		case 2:
			cknp(ep,expr_addnotl(ep,p->v),return -1);
			break;
		case 3:
			cknp(ep,expr_addnot(ep,p->v),return -1);
			break;
		case 4:
			cknp(ep,expr_addnex0(ep,p->v),return -1);
			break;
		case 5:
			cknp(ep,expr_adddif0(ep,p->v),return -1);
			break;
		default:
			return 0;
	}
	p->unary>>=3;
	goto redo;
}
static inline int do_unary_forvar(struct expr *restrict ep,struct vnode *ev,double *v){
	for(struct vnode *p=ev;p;p=p->next){
		if(p->v==v)
			return do_unary_forone(ep,p);
	}
	return -1;
}
static int do_unary(struct expr *restrict ep,struct vnode *ev){
	int r=0;
	for(struct vnode *p=ev;p;p=p->next){
		if(unlikely(do_unary_forone(ep,p)<0))
			return -1;
		r=1;
	}
	return r;
}
static int vnunion(struct expr *restrict ep,struct vnode *ev){
	struct vnode *p;

	cknp(ep,expr_addop(ep,ev->v,ev->next->v,ev->next->op,0),return -1);
	p=ev->next;
	ev->next=ev->next->next;
	xfree(p);
	return 0;
}
static void vnfree(struct vnode *vp){
	struct vnode *p;
	while(vp){
		p=vp->next;
		xfree(vp);
		vp=p;
	}
}
//scan mark
static double *scan(struct expr *restrict ep,const char *e,const char *endp,const char *asym,size_t asymlen){
	double *v1;
	const char *e0=e,*e1;
	enum expr_op op=EXPR_COPY,op0;
	uint64_t unary;
	struct vnode *ev=NULL,*p;
	do {
	unary=0;
redo:
	switch(*e){
		case '-':
		case '!':
		case '~':
		case '#':
		case '=':
			if(unlikely(unary&UINT64_C(0xf000000000000000))){
				if(e<endp&&expr_operator(*e)){
euo:
					*ep->errinfo=*e;
					seterr(ep,EXPR_EUO);
				}else {
eev:
					seterr(ep,EXPR_EEV);
				}
				goto err;
			}
		default:
			break;
	}
	switch(*e){
		case '+':
			++e;
			if(unlikely(e>=endp))
				goto eev;
			goto redo;
		case '-':
			unary=(unary<<3)|1;
			++e;
			if(unlikely(e>=endp))
				goto eev;
			goto redo;
		case '!':
			unary=(unary<<3)|2;
			++e;
			if(unlikely(e>=endp))
				goto eev;
			goto redo;
		case '~':
			unary=(unary<<3)|3;
			++e;
			if(unlikely(e>=endp))
				goto eev;
			goto redo;
		case '#':
			unary=(unary<<3)|4;
			++e;
			if(unlikely(e>=endp))
				goto eev;
			goto redo;
		case '=':
			unary=(unary<<3)|5;
			++e;
			if(unlikely(e>=endp))
				goto eev;
			goto redo;
	}
	v1=getvalue(ep,e,endp,&e,asym,asymlen);
	if(unlikely(!v1))
		goto err;
	p=vnadd(ev,v1,op,unary);
	cknp(ep,p,goto err);
	if(unlikely(!ev))
		ev=p;
	if(unlikely(e>=endp)){
		if(v1==EXPR_VOID){
evd:
			seterr(ep,EXPR_EVD);
			goto err;
		}
		goto end2;
	}
	op0=op;
	op=EXPR_END;
rescan:
#define unary_for(V) if(unlikely(do_unary_forvar(ep,ev,(V))<0))goto err
	switch(*e){
		case '>':
			++e;
			if(e<endp&&*e=='='){
				++e;
				if(e<endp&&*e=='='){
					++e;
					op=EXPR_GE;
				}else op=EXPR_SGE;
			}else if(e<endp&&*e=='>'){
				op=EXPR_SHR;
				++e;
			}else if(e<endp&&*e=='<'){
				op=EXPR_SNE;
				++e;
			}else op=EXPR_GT;
			goto end1;
		case '<':
			++e;
			if(e<endp&&*e=='='){
				++e;
				if(e<endp&&*e=='='){
					++e;
					op=EXPR_LE;
				}else op=EXPR_SLE;
			}else if(e<endp&&*e=='<'){
				op=EXPR_SHL;
				++e;
			}else if(e<endp&&*e=='>'){
				op=EXPR_SNE;
				++e;
			}else op=EXPR_LT;
			goto end1;
		case '=':
			++e;
			if(e<endp&&*e=='='){
				op=EXPR_EQ;
				++e;
			}else op=EXPR_SEQ;
			goto end1;
		case '!':
			++e;
			if(e<endp&&*e=='='){
				++e;
				op=EXPR_NE;
			}else op=EXPR_SNE;
			goto end1;
		case '&':
			++e;
			if(e<endp&&*e=='&'){
				op=EXPR_ANDL;
				++e;
			}
			else op=EXPR_AND;
			goto end1;
		case '|':
			++e;
			if(e<endp&&*e=='|'){
				op=EXPR_ORL;
				++e;
			}
			else op=EXPR_OR;
			goto end1;
		case '+':
			op=EXPR_ADD;
			++e;
			goto end1;
		case '-':
			if(e+1<endp&&e[1]=='>'){
				const struct expr_symbol *esp=NULL;
				const char *p1;
				double *v2;
				if(unlikely(expr_void(v1)))
					goto evd;
				e+=2;
				p1=getsym(e,endp);
				if(likely(p1==e)){
					switch(*e){
						case '[':
							p1=findpair_bracket(e,endp);
							if(unlikely(!p1))
								goto pterr;
							if(unlikely(ep->iflag&EXPR_IF_PROTECT)){
								seterr(ep,EXPR_EPM);
								serrinfo(ep->errinfo,e,p1-e+1);
								goto err;
							}
							setunsafe(ep);
							if(unlikely(p1==e+1)){
envp:
								seterr(ep,EXPR_ENVP);
								goto err;
							}
							if(p1+1<endp&&p1[1]=='['){
								e1=e;
								goto multi_dim;
							}
							v2=scan(ep,e+1,p1,asym,asymlen);
							if(unlikely(!v2))
								goto err;
							unary_for(v1);
							cknp(ep,expr_addwrite(ep,v1,v2),goto err);
							e=p1+1;
							goto bracket_end;
						case '(':
							p1=findpair(e,endp);
							if(p1+1>=endp||p1[1]!='[')
								break;
							if(unlikely(ep->iflag&EXPR_IF_PROTECT)){
								p1=findpair_bracket(p1+1,endp);
								if(unlikely(!p1))
									goto pterr;
								seterr(ep,EXPR_EPM);
								serrinfo(ep->errinfo,e,p1-e+1);
								goto err;
							}
							setunsafe(ep);
							e1=e;
							goto multi_dim;
						default:
							break;
					}
					if(expr_operator(*e)){
						*ep->errinfo=*e;
						seterr(ep,EXPR_EUO);
					}else {
						seterr(ep,EXPR_EEV);
					}
				goto err;
				}
				if(unlikely(p1-e==asymlen&&!memcmp(e,asym,p1-e))){
					serrinfo(ep->errinfo,e,p1-e);
					goto tnv;
				}
				if(ep->sset)
					esp=expr_symset_search(ep->sset,e,p1-e);
				if(unlikely(!esp)){
					serrinfo(ep->errinfo,e,p1-e);
					seterr(ep,EXPR_ESYMBOL);
					goto err;
				}
				e1=e;
				e=p1;
				if(*p1=='['){
					double *v3;
					p1=findpair_bracket(e,endp);
					if(unlikely(!p1))
						goto pterr;
					if(unlikely(ep->iflag&EXPR_IF_PROTECT)){
						seterr(ep,EXPR_EPM);
						serrinfo(ep->errinfo,e1,p1-e1+1);
						goto err;
					}
					setunsafe(ep);
					if(unlikely(p1==e+1))
						goto envp;
					while(p1+1<endp&&p1[1]=='['){
multi_dim:
						e=p1+1;
						p1=findpair_bracket(e,endp);
						if(unlikely(!p1))
							goto pterr;
						if(unlikely(p1==e+1))
							goto envp;
					}
					v3=getvalue(ep,e1,e,NULL,asym,asymlen);
					cknp(ep,v3,goto err);
					v2=scan(ep,e+1,p1,asym,asymlen);
					if(unlikely(!v2))
						goto err;
					cknp(ep,expr_addoff(ep,v3,v2),goto err);
					unary_for(v1);
					cknp(ep,expr_addwrite(ep,v1,v3),goto err);
					e=p1+1;
					goto bracket_end;
				}else {
					switch(esp->type){
						case EXPR_VARIABLE:
							break;
							esp=alias_target(ep,esp,1);
							if(unlikely(!esp))
								goto err;
							if(likely(esp->type==EXPR_VARIABLE))
								break;
						case EXPR_ALIAS:
						default:
							serrinfo(ep->errinfo,e1,p1-e1);
tnv:
							seterr(ep,EXPR_ETNV);
							goto err;
					}
				}
				unary_for(v1);
				cknp(ep,expr_addcopy(ep,expr_symbol_un(esp)->addr,v1),goto err);
bracket_end:
				if(e>=endp)
					continue;
				goto rescan;
			}else if(e+2<endp&&e[1]=='-'&&e[2]=='>'){
				const char *p1;
				double *v2;
				struct expr_symbol *esp;
				if(unlikely(expr_void(v1)))
					goto evd;
				e+=3;
				p1=getsym(e,endp);
				if(unlikely(p1==e)){
					if(*e&&expr_operator(*e)){
						*ep->errinfo=*e;
						seterr(ep,EXPR_EUO);
					}else {
						seterr(ep,EXPR_EEV);
					}
				goto err;
				}

				if(unlikely(p1-e==asymlen&&!memcmp(e,asym,p1-e))){
					serrinfo(ep->errinfo,e,p1-e);
					goto tnv;
				}
				if(ep->sset&&(esp=expr_symset_search(ep->sset,e,p1-e))
				){
					seterr(ep,EXPR_EDS);
					serrinfo(ep->errinfo,e,p1-e);
					goto err;
					/*
					cknp(ep,expr_detach(ep)>=0,goto err);
					esp=expr_symset_search(ep->sset,e,p1-e);
					if(unlikely(!esp))
						goto err;//unknown error
					v2=expr_newvar(ep);
					cknp(ep,v2,goto err);
					esp->type=EXPR_VARIABLE;
					esp->flag=0;
					expr_symbol_un(esp)->addr=v2;
					*/
				}else {
					v2=expr_createvar(ep,e,p1-e);
					cknp(ep,v2,goto err);
				}
				unary_for(v1);
				cknp(ep,expr_addcopy(ep,v2,v1),goto err);
				e=p1;
				if(e>=endp)
					continue;
				goto rescan;
			}else {
				op=EXPR_SUB;
				++e;
			}
			goto end1;
		case '*':
			++e;
			if(e<endp&&*e=='*'){
				op=EXPR_POW;
				++e;
			}else
				op=EXPR_MUL;
			goto end1;
		case '/':
			op=EXPR_DIV;
			++e;
			goto end1;
		case '%':
			op=EXPR_MOD;
			++e;
			goto end1;
		case '^':
			++e;
			if(e<endp&&*e=='^'){
				++e;
				op=EXPR_XORL;
			}else
				op=EXPR_XOR;
			goto end1;
		case '#':
			++e;
			if(e<endp&&*e=='#'){
				op=EXPR_DIFF;
				++e;
			}else
				op=EXPR_NEXT;
			goto end1;
		case ';':
			if(e+1==endp)
				continue;
		case ',':
			op=EXPR_COPY;
			++e;
			goto end1;
		case ')':
			if(unlikely(!unfindpair(e0,e))){
pterr:
				seterr(ep,EXPR_EPT);
				goto err;
			}
		default:
			if(unlikely(expr_operator(*e)))
				goto euo;
			seterr(ep,EXPR_EUSN);
			serrinfo(ep->errinfo,e,getsym(e,endp)-e);
			goto err;
	}
end1:

	if(unlikely(v1==EXPR_VOID&&(op!=EXPR_COPY
		||op0!=EXPR_COPY))){
		goto evd;
	}
	if(unlikely(e>=endp&&op!=EXPR_END))
		goto eev;
	continue;	
	}while(op!=EXPR_END);
end2:
#define vnunionp cknp(ep,vnunion(ep,p)>=0,goto err)
#define SETPREC1(a)\
	for(p=ev;p;p=p->next){\
		while(p->next&&p->next->op==(a)){\
			vnunionp;\
		}\
	}
#define SETPREC2(a,b)\
	for(p=ev;p;p=p->next){\
		while(p->next&&(\
			p->next->op==(a)\
			||p->next->op==(b)\
			)){\
			vnunionp;\
		}\
	}
#define SETPREC3(a,b,c)\
	for(p=ev;p;p=p->next){\
		while(p->next&&(\
			p->next->op==(a)\
			||p->next->op==(b)\
			||p->next->op==(c)\
			)){\
			vnunionp;\
		}\
	}
#define SETPREC4(a,b,c,d)\
	for(p=ev;p;p=p->next){\
		while(p->next&&(\
			p->next->op==(a)\
			||p->next->op==(b)\
			||p->next->op==(c)\
			||p->next->op==(d)\
			)){\
			vnunionp;\
		}\
	}
#define SETPREC6(a,b,c,d,e,f)\
	for(p=ev;p;p=p->next){\
		while(p->next&&(\
			p->next->op==(a)\
			||p->next->op==(b)\
			||p->next->op==(c)\
			||p->next->op==(d)\
			||p->next->op==(e)\
			||p->next->op==(f)\
			)){\
			vnunionp;\
		}\
	}
	cknp(ep,do_unary(ep,ev)>=0,goto err);
	SETPREC1(EXPR_POW)
	SETPREC3(EXPR_MUL,EXPR_DIV,EXPR_MOD)
	SETPREC2(EXPR_NEXT,EXPR_DIFF)
	SETPREC2(EXPR_ADD,EXPR_SUB)
	SETPREC2(EXPR_SHL,EXPR_SHR)
	SETPREC6(EXPR_LT,EXPR_LE,EXPR_GT,EXPR_GE,EXPR_SLE,EXPR_SGE)
	SETPREC4(EXPR_SEQ,EXPR_SNE,EXPR_EQ,EXPR_NE)
	SETPREC1(EXPR_AND)
	SETPREC1(EXPR_XOR)
	SETPREC1(EXPR_OR)
	SETPREC1(EXPR_ANDL)
	SETPREC1(EXPR_XORL)
	SETPREC1(EXPR_ORL)
	for(p=ev;p->next;p=p->next);
	v1=p->v;
	vnfree(ev);
	return v1;
err:
	if(likely(ev))
		vnfree(ev);
	return NULL;
}
void init_expr_symset(struct expr_symset *restrict esp){
	memset(esp,0,sizeof(struct expr_symset));
}
struct expr_symset *new_expr_symset(void){
	struct expr_symset *ep=xmalloc(sizeof(struct expr_symset));
	if(!ep)
		return NULL;
	init_expr_symset(ep);
	ep->freeable=1;
	return ep;
}
static void expr_symset_freesymbol_s(struct expr_symset *restrict esp,void *stack){
	expr_symset_foreach4(sp,esp,stack,EXPR_SYMNEXT){
		if(!sp->saved){
			xfree(sp);
		}
	}
}
void expr_symset_free(struct expr_symset *restrict esp){
	STACK_DEFAULT(stack,esp);
	expr_symset_free_s(esp,stack);
}
void expr_symset_free_s(struct expr_symset *restrict esp,void *stack){
	expr_symset_freesymbol_s(esp,stack);
	if(esp->freeable)
		xfree(esp);
}
void expr_symset_wipe(struct expr_symset *restrict esp){
	STACK_DEFAULT(stack,esp);
	expr_symset_wipe_s(esp,stack);
}
void expr_symset_wipe_s(struct expr_symset *restrict esp,void *stack){
	expr_symset_freesymbol_s(esp,stack);
	esp->syms=NULL;
	esp->size=0;
	esp->removed=0;
	esp->depth=0;
	esp->depth_n=0;
	esp->length=0;
	esp->alength=0;
}
void expr_symset_save(struct expr_symset *restrict esp,void *buf){
	STACK_DEFAULT(stack,esp);
	expr_symset_save_s(esp,buf,stack);
}
void expr_symset_save_s(struct expr_symset *restrict esp,void *buf,void *stack){
	ptrdiff_t off;
	struct expr_symbol *prev;
	size_t index;
	expr_symset_foreach4(sp,(volatile struct expr_symset *)esp,stack,EXPR_SYMNEXT){
		*sp->tail=(struct expr_symbol *)buf;
		memcpy(buf,sp,sp->length);
		off=align(sp->length);
		if(!sp->saved){
			xfree(sp);
			((struct expr_symbol *)buf)->saved=1;
		}
		buf=(void *)((uintptr_t)buf+off);
	}
	expr_symset_foreach4(sp,(volatile struct expr_symset *)esp,stack,EXPR_SYMNEXT){
		if(likely(__inloop_sp!=__inloop_stack)){
			prev=*(__inloop_sp-1);
			index=*((size_t *)__inloop_sp-2)-1;
			sp->tail=prev->next+index;
		}
	}
}
void expr_symset_move(struct expr_symset *restrict esp,ptrdiff_t off){
	STACK_DEFAULT(stack,esp);
	expr_symset_move_s(esp,off,stack);
}
void expr_symset_move_s(struct expr_symset *restrict esp,ptrdiff_t off,void *stack){
	expr_symset_foreach4(sp,esp,stack,EXPR_SYMNEXT){
		if(!sp->saved)
			continue;
		*sp->tail=(struct expr_symbol *)((intptr_t)*sp->tail+off);
	}
}
#define off_sin offsetof(struct expr_symbol_infile,str)
#define off_str offsetof(struct expr_symbol,str)
ssize_t expr_symset_write(const struct expr_symset *restrict esp,expr_writer writer,intptr_t fd){
	STACK_DEFAULT(stack,esp);
	return expr_symset_write_s(esp,writer,fd,stack);
}
#define try_write(buf,size) \
	r=writer(fd,(buf),(size));\
	if(unlikely(r<0))\
		goto err2;\
	else if(r<(size))\
		goto err3;\
	((void)0)
ssize_t expr_symset_write_s(const struct expr_symset *restrict esp,expr_writer writer,intptr_t fd,void *stack){
//	uintptr_t minsp=UINTPTR_MAX;
	size_t maxlen=0,count;
	ssize_t r;
	ptrdiff_t off;
	struct expr_symset_infile tempesp[1];
	struct expr_symbol_infile *temp;
	expr_symset_foreach4(sp,esp,stack,0){
		off=sp->length;
		if(unlikely(off>maxlen))
			maxlen=off;
	}
	temp=xmalloc(align(maxlen)-EXPR_SYMBOL_EXTRA);
	if(unlikely(!temp))
		return -1;
	tempesp->size=esp->size;
	tempesp->maxlen=maxlen;
	try_write(tempesp,sizeof(struct expr_symset_infile));
	count=tempesp->size;
	if(!count)
		goto end;
	++count;
	expr_symset_foreach4(sp,esp,stack,EXPR_SYMNEXT/2){
		off=sp->length;
		maxlen=off-EXPR_SYMBOL_EXTRA;
		temp->length=maxlen;
		temp->strlen=sp->strlen;
		temp->type=sp->type;
		temp->flag=sp->flag;
		temp->unused=0;
		memcpy(temp->str,sp->str,off-off_str);
		try_write(temp,maxlen);
	}
end:
	xfree(temp);
	return 0;
err2:
	xfree(temp);
	return r;
err3:
	xfree(temp);
	return -2;
}
ssize_t expr_symset_read(struct expr_symset *restrict esp,const void *buf,size_t size){
	const struct expr_symset_infile *esi;
	const struct expr_symbol_infile *ebi;
	struct expr_symbol *sp;
	size_t count,len;
	ssize_t added;
	if(size<sizeof(struct expr_symset_infile))
		return -2;
	esi=(const struct expr_symset_infile *)buf;
	count=esi->size;
	ebi=(const struct expr_symbol_infile *)((uintptr_t)buf+sizeof(struct expr_symset_infile));
	size-=sizeof(struct expr_symset_infile);
	added=0;
	while(count){
		if(unlikely(size<=off_sin))
			break;
		if(unlikely(ebi->length<=off_sin))
			break;
		if(unlikely(ebi->length>size))
			return -((added<<2)|2);
		len=ebi->length+EXPR_SYMBOL_EXTRA;
		sp=xmalloc(len);
		if(unlikely(!sp))
			return -((added<<2)|1);
		sp->length=len;
		sp->strlen=ebi->strlen;
		sp->type=ebi->type;
		sp->flag=ebi->flag;
		sp->saved=0;
		memcpy(sp->str,ebi->str,ebi->length-off_sin);
		if(unlikely(expr_symset_insert(esp,sp)<0)){
			xfree(sp);
		}else {
			++added;
		}
		size-=ebi->length;
		ebi=(const struct expr_symbol_infile *)((uintptr_t)ebi+ebi->length);
		--count;
	}
	return unlikely(added<esi->size)?-(added<<2):added;
}
ssize_t expr_symset_readfd(struct expr_symset *restrict esp,expr_reader reader,intptr_t fd){
	struct expr_symset_infile esi[1];
	struct expr_symbol_infile *ebi;
	struct expr_symbol *sp;
	size_t count,len,maxlen;
	ssize_t added,r;
	r=reader(fd,esi,sizeof(struct expr_symset_infile));
	if(unlikely(r<0))
		return -3;
	if(unlikely(r<sizeof(struct expr_symset_infile)))
		return -2;
	count=esi->size;
	maxlen=esi->maxlen;
	if(unlikely(maxlen<=off_sin))
		return -2;
	ebi=xmalloc(maxlen);
	if(unlikely(!ebi))
		return -1;
	added=0;
	while(count){
		r=reader(fd,ebi,off_sin);
		if(unlikely(r<0))
			goto err3;
		if(unlikely(r<off_sin))
			goto err2;
		if(unlikely(ebi->length<=off_sin))
			goto err2;
		if(unlikely(ebi->length>maxlen))
			goto err2;
		len=ebi->length+EXPR_SYMBOL_EXTRA;
		if(unlikely(len<=sizeof(struct expr_symbol)))
			goto err2;
		r=reader(fd,(void *)((uintptr_t)ebi+off_sin),ebi->length-off_sin);
		if(unlikely(r<0))
			goto err3;
		if(unlikely(r<ebi->length-off_sin))
			goto err2;
		sp=xmalloc(len);
		if(unlikely(!sp))
			goto err;
		sp->length=len;
		sp->strlen=ebi->strlen;
		sp->type=ebi->type;
		sp->flag=ebi->flag;
		sp->saved=0;
		memcpy(sp->str,ebi->str,ebi->length-off_sin);
		if(unlikely(expr_symset_insert(esp,sp)<0)){
			xfree(sp);
		}else {
			++added;
		}
		--count;
	}
	xfree(ebi);
	return unlikely(added<esi->size)?-(added<<2):added;
err:
	xfree(ebi);
	return -((added<<2)|1);
err2:
	xfree(ebi);
	return -((added<<2)|2);
err3:
	xfree(ebi);
	return -((added<<2)|3);
//WARNING: the symbol value of variables or functions
//cannot be reused after a PIE-program restarted.
}
static long firstdiff(const char *restrict s1,const char *restrict s2,size_t len1,size_t len2){
	long r,r0;
	int64_t ampl=INT64_C(0x330e)*EXPR_MAGIC48_A+EXPR_MAGIC48_B;
	do {
		ampl=expr_next48v(ampl);
		r=(int)(unsigned char)*(s1++)-(int)(unsigned char)*(s2++);
		if(r)
			break;
		--len1;
		--len2;
	}while(len1&&len2);
	if(len1&&!len2){
		r=(int32_t)(unsigned char)*s1;
	}else if(!len1&&len2){
		r=-(int32_t)(unsigned char)*s2;
	}
	//r0=(r*ampl)>>48;
	r0=r*expr_ltod48(ampl);
	//return r==0?0:(r>0?r0+1:r0-1);
	return r0?r0:r;
}
static long strdiff(const char *restrict s1,size_t len1,const char *restrict s2,size_t len2,long *sum){
	long r=firstdiff(s1,s2,len1,len2);
	if(unlikely(!r))
		return 0;
	*sum=r;
	return 1;
}
#define modi(d) ({\
	long t=(d);\
	if(t<0)\
		t+=EXPR_SYMNEXT*((-t)/EXPR_SYMNEXT+1);\
	t%EXPR_SYMNEXT;\
})
struct expr_symbol **expr_symset_findtail(struct expr_symset *restrict esp,const char *sym,size_t symlen,size_t *depth){
	struct expr_symbol *p;
	size_t dep;
	long r;
	if(unlikely(!esp->syms)){
		*depth=1;
		return &esp->syms;
	}
	dep=2;
	for(p=esp->syms;;++dep){
		if(unlikely(!strdiff(sym,symlen,p->str,p->strlen,&r)))
			return NULL;
		r=modi(r);
		if(p->next[r]){
			p=p->next[r];
		}else {
			*depth=dep;
			return p->next+r;
		}
	}
}
struct expr_symbol *expr_symbol_create(const char *sym,int type,int flag,...){
	va_list ap;
	struct expr_symbol *r;
	va_start(ap,flag);
	r=expr_symbol_vcreate(sym,type,flag,ap);
	va_end(ap);
	return r;
}
struct expr_symbol *expr_symbol_createl(const char *sym,size_t symlen,int type,int flag,...){
	va_list ap;
	struct expr_symbol *r;
	va_start(ap,flag);
	r=expr_symbol_vcreatel(sym,symlen,type,flag,ap);
	va_end(ap);
	return r;
}
struct expr_symbol *expr_symbol_vcreate(const char *sym,int type,int flag,va_list ap){
	return expr_symbol_vcreatel(sym,strlen(sym),type,flag,ap);
}
struct expr_symbol *expr_symset_add(struct expr_symset *restrict esp,const char *sym,int type,int flag,...){
	va_list ap;
	struct expr_symbol *r;
	va_start(ap,flag);
	r=expr_symset_vadd(esp,sym,type,flag,ap);
	va_end(ap);
	return r;
}
struct expr_symbol *expr_symset_addl(struct expr_symset *restrict esp,const char *sym,size_t symlen,int type,int flag,...){
	va_list ap;
	struct expr_symbol *r;
	va_start(ap,flag);
	r=expr_symset_vaddl(esp,sym,symlen,type,flag,ap);
	va_end(ap);
	return r;
}
struct expr_symbol *expr_symset_vadd(struct expr_symset *restrict esp,const char *sym,int type,int flag,va_list ap){
	return expr_symset_vaddl(esp,sym,strlen(sym),type,flag,ap);
}
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
struct expr_symbol *expr_symbol_vcreatel(const char *sym,size_t symlen,int type,int flag,va_list ap){
	struct expr_symbol *ep;
	size_t len,len_expr;
	const char *p;
	char *p1;
	if(unlikely(!symlen))
		return NULL;
	if(unlikely(symlen>=EXPR_SYMLEN))
		symlen=EXPR_SYMLEN-1;
	len=sizeof(struct expr_symbol)+symlen+1;
	switch(type){
		case EXPR_CONSTANT:
		case EXPR_VARIABLE:
		case EXPR_FUNCTION:
		case EXPR_ZAFUNCTION:
			len+=sizeof(union expr_symvalue);
			break;
		case EXPR_MDFUNCTION:
		case EXPR_MDEPFUNCTION:
			len+=sizeof(union expr_symvalue)+sizeof(size_t);
			break;
		case EXPR_HOTFUNCTION:
		case EXPR_ALIAS:
			p=(const char *)va_arg(ap,const void *);
			if(flag&EXPR_SF_INJECTION)
				len_expr=va_arg(ap,size_t);
			else
				len_expr=strlen(p);
			len+=len_expr+1;
			break;
		default:
			return NULL;
	}
	ep=xmalloc(len);
	if(unlikely(!ep)){
		return NULL;
	}
	ep->length=len;
	memcpy(ep->str,sym,symlen);
	ep->str[symlen]=0;
	ep->strlen=symlen;
	switch(type){
		case EXPR_CONSTANT:
			expr_symbol_un(ep)->value=va_arg(ap,double);
			break;
		case EXPR_VARIABLE:
			expr_symbol_un(ep)->addr=va_arg(ap,double *);
			break;
		case EXPR_FUNCTION:
			expr_symbol_un(ep)->func=va_arg(ap,double (*)(double));
			break;
		case EXPR_MDFUNCTION:
			expr_symbol_un(ep)->mdfunc=va_arg(ap,double (*)(double *,size_t));
			SYMDIM(ep)=va_arg(ap,size_t);
			break;
		case EXPR_MDEPFUNCTION:
			expr_symbol_un(ep)->mdepfunc=va_arg(ap,double (*)(const struct expr *,size_t,double));
			SYMDIM(ep)=va_arg(ap,size_t);
			break;
		case EXPR_ZAFUNCTION:
			expr_symbol_un(ep)->zafunc=va_arg(ap,double (*)(void));
			break;
		case EXPR_HOTFUNCTION:
		case EXPR_ALIAS:
			p1=ep->str+symlen+1;
			memcpy(p1,p,len_expr);
			p1[len_expr]=0;
			break;
		default:
			__builtin_unreachable();
	}
	ep->type=type;
	ep->flag=flag;
	ep->saved=0;
	return ep;
}
struct expr_symbol *expr_symset_vaddl(struct expr_symset *restrict esp,const char *sym,size_t symlen,int type,int flag,va_list ap){
	struct expr_symbol *ep,**next;
	size_t depth,alen;
	next=expr_symset_findtail(esp,sym,symlen,&depth);
//	printf("adding %s\n",sym);
//	if(symlen==1)trap;
	if(unlikely(!next))
		return NULL;
	ep=expr_symbol_vcreatel(sym,symlen,type,flag,ap);
	if(unlikely(!ep))
		return NULL;
	memset(ep->next,0,sizeof(ep->next));
#define sset_inc(esp,_length,depth) \
	++(esp)->size;\
	++(esp)->size_m;\
	alen=align(_length);\
	(esp)->length+=(_length);\
	(esp)->alength+=alen;\
	(esp)->length_m+=(_length);\
	(esp)->alength_m+=alen;\
	if(unlikely((depth)>(esp)->depth)){\
		esp->depth=(depth);\
		esp->removed=0;\
		esp->depth_n=1;\
		++esp->depth_nm;\
	}else if(unlikely((depth)==(esp)->depth)){\
		++esp->depth_n;\
		++esp->depth_nm;\
	}
	sset_inc(esp,ep->length,depth);
	ep->tail=next;
	ep->depthm1=depth-1;
	*next=ep;
	return ep;
}
#pragma GCC diagnostic pop
struct expr_symbol *expr_symset_addcopy(struct expr_symset *restrict esp,const struct expr_symbol *restrict es){
	size_t depth,alen;
	struct expr_symbol **tail=expr_symset_findtail(esp,es->str,es->strlen,&depth);
	struct expr_symbol *ep;
	if(unlikely(!tail||!(ep=xmalloc(es->length)))){
		return NULL;
	}
	memcpy(ep,es,es->length);
	memset(ep->next,0,EXPR_SYMNEXT*sizeof(struct expr_symbol *));
	*tail=ep;
	ep->tail=tail;
	ep->depthm1=depth-1;
	ep->saved=0;
	sset_inc(esp,ep->length,depth);
	return ep;
}
struct expr_symbol **expr_symset_search0(const struct expr_symset *restrict esp,const char *sym,size_t sz){
	long r;
	for(struct expr_symbol *p=esp->syms;p;){
		if(unlikely(!strdiff(sym,sz,p->str,p->strlen,&r))){
			return p->tail;
		}
		r=modi(r);
		p=p->next[r];
	}
	return NULL;
}
struct expr_symbol *expr_symset_search(const struct expr_symset *restrict esp,const char *sym,size_t sz){
	long r;
	for(struct expr_symbol *p=esp->syms;p;){
		if(unlikely(!strdiff(sym,sz,p->str,p->strlen,&r))){
			return p;
		}
		r=modi(r);
		p=p->next[r];
	}
	return NULL;
}
struct expr_symbol *expr_symset_searchd(const struct expr_symset *restrict esp,const char *sym,size_t sz,size_t *depth){
	long r;
	size_t dep=1;
	for(struct expr_symbol *p=esp->syms;p;){
		if(unlikely(!strdiff(sym,sz,p->str,p->strlen,&r))){
			*depth=dep;
			return p;
		}
		r=modi(r);
		p=p->next[r];
		++dep;
	}
	return NULL;
}
int expr_symbol_check(const struct expr_symbol *restrict esp){
	ssize_t data,expect;
	const char *p;
	if(unlikely(esp->strlen>=EXPR_SYMLEN-1))
		return -1;
	if(unlikely(esp->length<=off_str))
		return -2;
	data=(esp->length-off_str)-(esp->strlen+1);
	if(unlikely(data<0))
		return -3;
	if(unlikely(esp->str[esp->strlen]))
		return -4;
	if(unlikely(strlen(esp->str)!=esp->strlen))
		return -5;
	switch(esp->type){
		case EXPR_CONSTANT:
		case EXPR_VARIABLE:
		case EXPR_FUNCTION:
		case EXPR_ZAFUNCTION:
			expect=sizeof(union expr_symvalue);
			break;
		case EXPR_MDFUNCTION:
		case EXPR_MDEPFUNCTION:
			expect=sizeof(union expr_symvalue)+sizeof(size_t);
			break;
		case EXPR_HOTFUNCTION:
		case EXPR_ALIAS:
			p=expr_symbol_hot(esp);
			expect=strlen(p);
			if(unlikely(p[expect]))
				return -6;
			++expect;
			break;
		default:
			return -7;
	}
	if(unlikely(data!=expect))
		return -8;
	return 0;
}
int expr_symset_insert(struct expr_symset *restrict esp,struct expr_symbol *restrict es){
	size_t depth,alen;
	struct expr_symbol **tail;
	if(unlikely(expr_symbol_check(es)<0)){
		return -1;
	}
	tail=expr_symset_findtail(esp,es->str,es->strlen,&depth);
	if(unlikely(!tail)){
		return -2;
	}
	memset(es->next,0,EXPR_SYMNEXT*sizeof(struct expr_symbol *));
	*tail=es;
	es->tail=tail;
	es->depthm1=depth-1;
	sset_inc(esp,es->length,depth);
	return 0;
}
static void expr_symset_reinsert(struct expr_symset *restrict esp,struct expr_symbol *restrict sp1){
	size_t depth;
	struct expr_symbol **tail;
	memset(sp1->next,0,EXPR_SYMNEXT*sizeof(struct expr_symbol *));
	tail=expr_symset_findtail(esp,sp1->str,sp1->strlen,&depth);
	*tail=sp1;
	sp1->tail=tail;
	sp1->depthm1=depth-1;
	if(unlikely(depth>esp->depth)){
		esp->depth=depth;
		esp->removed=0;
		esp->depth_n=1;
		++esp->depth_nm;
	}else if(unlikely((depth)==(esp)->depth)){
		++esp->depth_n;
		++esp->depth_nm;
	}
}
int expr_symset_remove(struct expr_symset *restrict esp,const char *sym,size_t sz){
	struct expr_symbol *r;
	size_t depth;
	r=expr_symset_searchd(esp,sym,sz,&depth);
	if(!r)
		return -1;
	expr_symset_removea(esp,&r,1);
	return 0;
}
int expr_symset_remove_s(struct expr_symset *restrict esp,const char *sym,size_t sz,void *stack){
	struct expr_symbol *r;
	size_t depth;
	r=expr_symset_searchd(esp,sym,sz,&depth);
	if(!r)
		return -1;
	expr_symset_removea_s(esp,&r,1,stack);
	return 0;
}
void expr_symset_removes(struct expr_symset *restrict esp,struct expr_symbol *sp){
	expr_symset_removea(esp,&sp,1);
}
void expr_symset_removes_s(struct expr_symset *restrict esp,struct expr_symbol *sp,void *stack){
	expr_symset_removea_s(esp,&sp,1,stack);
}
void expr_symset_removea(struct expr_symset *restrict esp,struct expr_symbol *const *spa,size_t n){
	STACK_DEFAULT(stack,esp);
	expr_symset_removea_s(esp,spa,n,stack);
}
void expr_symset_removea_s(struct expr_symset *restrict esp,struct expr_symbol *const *spa,size_t n,void *stack){
	struct expr_symbol *const *spa_end=spa+n;
	expr_symset_detacha_s(esp,spa,n,stack);
	do {
		if(!(*spa)->saved)
			xfree(*spa);
		spa++;
	}while(spa<spa_end);

}
int expr_symset_detach(struct expr_symset *restrict esp,const char *sym,size_t sz){
	struct expr_symbol *r;
	size_t depth;
	r=expr_symset_searchd(esp,sym,sz,&depth);
	if(!r)
		return -1;
	expr_symset_detacha(esp,&r,1);
	return 0;
}
int expr_symset_detach_s(struct expr_symset *restrict esp,const char *sym,size_t sz,void *stack){
	struct expr_symbol *r;
	size_t depth;
	r=expr_symset_searchd(esp,sym,sz,&depth);
	if(!r)
		return -1;
	expr_symset_detacha_s(esp,&r,1,stack);
	return 0;
}
void expr_symset_detachs(struct expr_symset *restrict esp,struct expr_symbol *sp){
	expr_symset_detacha(esp,&sp,1);
}
void expr_symset_detachs_s(struct expr_symset *restrict esp,struct expr_symbol *sp,void *stack){
	expr_symset_detacha_s(esp,&sp,1,stack);
}
void expr_symset_detacha(struct expr_symset *restrict esp,struct expr_symbol *const *spa,size_t n){
	STACK_DEFAULT(stack,esp);
	expr_symset_detacha_s(esp,spa,n,stack);
}
void expr_symset_detacha_s(struct expr_symset *restrict esp,struct expr_symbol *const *spa,size_t n,void *stack){
	size_t depthm1;
	struct expr_symbol *const *spa_cur=spa;
	struct expr_symbol *const *spa_end=spa+n;
	if(likely(esp->depth<=(1<<16))){
		depthm1=esp->depth-1;
		do {
			*(*spa_cur)->tail=NULL;
			if((*spa_cur)->depthm1==depthm1)
				--esp->depth_n;
			++spa_cur;
		}while(spa_cur<spa_end);
	}else {
		do {
			*(*spa_cur)->tail=NULL;
			++spa_cur;
		}while(spa_cur<spa_end);
	}
	esp->size-=n;
	esp->removed+=n;
	esp->removed_m+=n;
	do {
		expr_symbol_foreach4(sp1,*spa,stack,EXPR_SYMNEXT){
			if(unlikely(sp1==*spa))
				break;
			expr_symset_reinsert(esp,sp1);
		}
		depthm1=(*spa)->length;
		esp->length-=depthm1;
		esp->alength-=align(depthm1);
		++spa;
	}while(spa<spa_end);
}
int expr_symset_recombine(struct expr_symset *restrict esp,long seed){
	//the function tries to recombine the symset randomly,which sometimes
	//can reduce the depth of it. but it is not ensured,even probably make
	//a greater depth,for I have not found an algorithm to find the best
	//arrange to get the minimal depth.
	STACK_DEFAULT(stack,esp);
	return expr_symset_recombine_s(esp,seed,stack);
}
int expr_symset_recombine_s(struct expr_symset *restrict esp,long seed,void *stack){
	struct expr_symbol **ss,**ss_cur,**ss_end,**p1,**p0;
	struct expr_symbol *swapbuf;
	if(unlikely(!esp->size))
		return 0;
	ss=xmalloc(esp->size*sizeof(struct expr_symbol *));
	if(unlikely(!ss))
		return -1;
	ss_cur=ss;
	expr_symset_foreach4(sp,esp,stack,EXPR_SYMNEXT){
		*ss_cur=sp;
		++ss_cur;
	}
	for(size_t i=0,endi=esp->size,i1;i<endi;++i){
		i1=expr_ltol48(expr_next48(&seed))%endi;
		if(likely(i1!=i)){
			p0=ss+i;
			p1=ss+i1;
		}else if(likely(i<endi-1)){
			p0=ss+i;
			p1=ss+i+1;
		}else
			continue;
		swapbuf=*p0;
		*p0=*p1;
		*p1=swapbuf;
	}
	//expr_memfry48(ss,sizeof(struct expr_symbol *),esp->size,expr_seed_default);
	esp->syms=NULL;
	esp->depth=0;
	esp->depth_n=0;
	ss_end=ss_cur;
	ss_cur=ss;
	do {
		expr_symset_reinsert(esp,*ss_cur);
		++ss_cur;
	}while(ss_cur<ss_end);
	xfree(ss);
	return 0;
}
int expr_symset_tryrecombine(struct expr_symset *restrict esp,long seed){
	struct expr_symset *temp;
	struct expr_symbol *b;
	size_t d;
	temp=expr_symset_clone(esp);
	if(unlikely(!temp))
		return -1;
	if(unlikely(expr_symset_recombine(temp,seed)<0)){
		expr_symset_free(temp);
		return -1;
	}
	if(temp->depth>esp->depth){
		expr_symset_free(temp);
		return 1;
	}
	d=esp->depth;
	esp->depth=temp->depth;
	esp->depth_n=temp->depth_n;
	esp->depth_nm+=temp->depth_nm;
	b=esp->syms;
	esp->syms=temp->syms;
	temp->syms=b;
	temp->depth=d;
	expr_symset_free(temp);
	return 0;

}
struct expr_symbol *expr_symset_rsearch(const struct expr_symset *restrict esp,void *addr){
	STACK_DEFAULT(stack,esp);
	return expr_symset_rsearch_s(esp,addr,stack);
}
struct expr_symbol *expr_symset_rsearch_s(const struct expr_symset *restrict esp,void *addr,void *stack){
	expr_symset_foreach(sp,esp,stack){
		if(unlikely(expr_symbol_un(sp)->uaddr==addr))
			return sp;
	}
	return NULL;
}
size_t expr_symset_depth(const struct expr_symset *restrict esp){
	STACK_DEFAULT(stack,esp);
	return expr_symset_depth_s(esp,stack);
}
size_t expr_symset_depth_s(const struct expr_symset *restrict esp,void *stack){
	uintptr_t depth=0;
	expr_symset_foreach(sp,esp,stack)
		if(unlikely((uintptr_t)__inloop_sp>depth))
			depth=(uintptr_t)__inloop_sp;
	return depth?(depth-(uintptr_t)stack)/EXPR_SYMSET_DEPTHUNIT+1:0;
}
void expr_symset_correct(struct expr_symset *restrict esp){
	STACK_DEFAULT(stack,esp);
	expr_symset_correct_s(esp,stack);
}
void expr_symset_correct_s(struct expr_symset *restrict esp,void *stack){
	size_t depth=0,cdepth,n=0;
	expr_symset_foreach(sp,esp,stack){
		cdepth=((uintptr_t)__inloop_sp-(uintptr_t)stack)/EXPR_SYMSET_DEPTHUNIT+1;
		if(likely(cdepth<depth)){
			continue;
		}else if(cdepth==depth){
			++n;
		}else {
			depth=cdepth;
			n=1;
		}
	}
	esp->depth=depth;
	esp->depth_n=n;
	esp->removed=0;
}
size_t expr_symset_length(const struct expr_symset *restrict esp){
	STACK_DEFAULT(stack,esp);
	return expr_symset_length_s(esp,stack);
}
size_t expr_symset_length_s(const struct expr_symset *restrict esp,void *stack){
	size_t length=0;
	expr_symset_foreach(sp,esp,stack)
		length+=sp->length;
	return length;
}
size_t expr_symset_size(const struct expr_symset *restrict esp){
	STACK_DEFAULT(stack,esp);
	return expr_symset_size_s(esp,stack);
}
size_t expr_symset_size_s(const struct expr_symset *restrict esp,void *stack){
	size_t sz=0;
	expr_symset_foreach(sp,esp,stack)
		++sz;
	return sz;
}
size_t expr_symset_copy(struct expr_symset *restrict dst,const struct expr_symset *restrict src){
	STACK_DEFAULT(stack,src);
	return expr_symset_copy_s(dst,src,stack);
}
size_t expr_symset_copy_s(struct expr_symset *restrict dst,const struct expr_symset *restrict src,void *stack){
	size_t n=0;
	expr_symset_foreach(sp,src,stack){
		if(likely(expr_symset_addcopy(dst,sp)))
			++n;
	}
	return n;
}
struct expr_symset *expr_symset_clone(const struct expr_symset *restrict ep){
	struct expr_symset *es=new_expr_symset();
	if(!es)
		return NULL;
	if(ep&&expr_symset_copy(es,ep)<ep->size){
		expr_symset_free(es);
		return NULL;
	}
	return es;
}
struct expr_symset *expr_symset_clone_s(const struct expr_symset *restrict ep,void *stack){
	struct expr_symset *es=new_expr_symset();
	if(!es)
		return NULL;
	if(ep&&expr_symset_copy_s(es,ep,stack)<ep->size){
		expr_symset_free_s(es,stack);
		return NULL;
	}
	return es;
}
static char *stpcpy_nospace(char *restrict s1,const char *restrict s2,const char *endp,int subexpr){
	const char *s20=(const char *)s2,*p;
	int instr=0;
	for(;s2<endp;++s2){
		if(unlikely(*s2=='\"'&&(s2==s20||s2[-1]!='\\')))
			instr^=1;
		else if(unlikely(!subexpr&&!instr&&*s2=='#'&&(s2==s20||s2[-1]=='\n'))){
			p=memchr(s2+1,'\n',endp-s2-1);
			if(unlikely(!p))
				s2=endp;
			else
				s2=p;
			continue;
		}
		if(!instr&&expr_space(*s2))
			continue;
		*(s1++)=*s2;
	}
	*s1=0;
	return s1;
}

static int expr_usesrc(enum expr_op op);
static int expr_constexpr(const struct expr *restrict ep,double *except);
int expr_isconst(const struct expr *restrict ep){
	return ep->isconst||expr_constexpr(ep,NULL);
}
void init_expr_const(struct expr *restrict ep,double val){
	memset(ep,0,sizeof(struct expr));
	ep->un.end->val=val;
	ep->un.end->endinst->op=EXPR_END;
	ep->un.end->endinst->dst.dst=&ep->un.end->val;
	ep->un.end->endinst->un.src=NULL;
	ep->data=ep->un.end->endinst;
	ep->size=1;
	ep->isconst=1;
}
struct expr *new_expr_const(double val){
	struct expr *r=xmalloc(sizeof(struct expr));
	if(unlikely(!r))
		return NULL;
	init_expr_const(r,val);
	r->freeable=1;
	return r;
}
static int init_expr8(struct expr *restrict ep,const char *e,size_t len,const char *asym,size_t asymlen,struct expr_symset *esp,int flag,struct expr *restrict parent){
	union {
		double *p;
		double v;
	} un;
	char *r,*p0;
	double *v;
	memset(ep,0,sizeof(struct expr));
	ep->sset=esp;
	ep->parent=(struct expr *)parent;
	ep->iflag=flag&~EXPR_IF_EXTEND_MASK;
	if(ep->iflag&EXPR_IF_DETACHSYMSET){
		cknp(ep,expr_detach(ep)>=0,return -1);
	}
	p0=xmalloc(len+1);
	cknp(ep,p0,return -1);
	r=stpcpy_nospace(p0,e,e+len,!!parent);
	un.p=scan(ep,p0,r,asym,asym?asymlen:0);
	xfree(p0);
	if(ep->sset_shouldfree){
		if(!(ep->iflag&EXPR_IF_KEEPSYMSET)){
			expr_symset_free(ep->sset);
			ep->sset_shouldfree=0;
			ep->sset=NULL;
		}
	}else
		ep->sset=NULL;
	if(likely(un.p)){
		if(unlikely(expr_void(un.p))){
			v=expr_newvar(ep);
			cknp(ep,v,goto err);
			*v=0;
			cknp(ep,expr_addend(ep,v),goto err);
		}else
			cknp(ep,expr_addend(ep,un.p),goto err);
	}else {
err:
		expr_free(ep);
		ep->errinfo[EXPR_SYMLEN-1]=0;
		return -1;
	}
	if(!(flag&EXPR_IF_NOOPTIMIZE)){
		expr_optimize(ep);
	}
	if(flag&EXPR_IF_INSTANT_FREE){
		expr_free(ep);
	}
	return 0;
}
int init_expr7(struct expr *restrict ep,const char *e,size_t len,const char *asym,size_t asymlen,struct expr_symset *esp,int flag){
	return init_expr8(ep,e,len,asym,asymlen,esp,flag,NULL);
}
int init_expr5(struct expr *restrict ep,const char *e,const char *asym,struct expr_symset *esp,int flag){
	return init_expr7(ep,e,e?strlen(e):0,asym,asym?strlen(asym):0,esp,flag);
}
int init_expr(struct expr *restrict ep,const char *e,const char *asym,struct expr_symset *esp){
	return init_expr5(ep,e,asym,esp,0);
}
static struct expr *new_expr10(const char *e,size_t len,const char *asym,size_t asymlen,struct expr_symset *esp,int flag,int n,int *error,char errinfo[EXPR_SYMLEN],struct expr *restrict parent){
	struct expr *ep,*ep0;
	if(unlikely(n<1))
		n=1;
	ep=ep0=xmalloc(n*sizeof(struct expr));
	if(unlikely(!ep)){
		if(error)
			*error=EXPR_EMEM;
		if(errinfo)
			memset(errinfo,0,EXPR_SYMLEN);
		return NULL;
	}
	do if(unlikely(init_expr8(ep,e,len,asym,asymlen,esp,flag,parent)<0)){
		if(error)
			*error=ep->error;
		if(errinfo)
			memcpy(errinfo,ep->errinfo,EXPR_SYMLEN);
		if(!(flag&EXPR_IF_INSTANT_FREE))while(--ep>=ep0){
			expr_free(ep);
		}
		xfree(ep0);
		return NULL;
	}else {
		ep->freeable=2;
		++ep;
	}while(--n);
	ep[-1].freeable=1;
	if(flag&EXPR_IF_INSTANT_FREE)
		xfree(ep0);
	return ep0;
}
struct expr *new_expr9(const char *e,size_t len,const char *asym,size_t asymlen,struct expr_symset *esp,int flag,int n,int *error,char errinfo[EXPR_SYMLEN]){
	return new_expr10(e,len,asym,asymlen,esp,flag,n,error,errinfo,NULL);
}
struct expr *new_expr7(const char *e,const char *asym,struct expr_symset *esp,int flag,int n,int *error,char errinfo[EXPR_SYMLEN]){
	return new_expr9(e,e?strlen(e):0,asym,asym?strlen(asym):0,esp,flag,n,error,errinfo);
}
struct expr *new_expr8(const char *e,size_t len,const char *asym,size_t asymlen,struct expr_symset *esp,int flag,int *error,char errinfo[EXPR_SYMLEN]){
	return new_expr9(e,len,asym,asymlen,esp,flag,1,error,errinfo);
}
struct expr *new_expr6(const char *e,const char *asym,struct expr_symset *esp,int flag,int *error,char errinfo[EXPR_SYMLEN]){
	return new_expr7(e,asym,esp,flag,1,error,errinfo);
}
struct expr *new_expr(const char *e,const char *asym,struct expr_symset *esp,int *error,char errinfo[EXPR_SYMLEN]){
	return new_expr7(e,asym,esp,0,1,error,errinfo);
}
double expr_calc5(const char *e,int *error,char errinfo[EXPR_SYMLEN],struct expr_symset *esp,int flag){
	struct expr ep[1];
	double r;
	flag&=~EXPR_IF_INSTANT_FREE;
	if(unlikely(init_expr5(ep,e,NULL,esp,flag)<0)){
		if(error)
			*error=ep->error;
		if(errinfo)
			memcpy(errinfo,ep->errinfo,EXPR_SYMLEN);
		return NAN;
	}
	r=eval(ep,0.0);
	expr_free(ep);
	return r;
}
double expr_calc4(const char *e,int *error,char errinfo[EXPR_SYMLEN],struct expr_symset *esp){
	return expr_calc5(e,error,errinfo,esp,0);
}
double expr_calc3(const char *e,int *error,char errinfo[EXPR_SYMLEN]){
	return expr_calc5(e,error,errinfo,NULL,0);
}
double expr_calc2(const char *e,int flag){
	return expr_calc5(e,NULL,NULL,NULL,flag);
}
double expr_calc(const char *e){
	return expr_calc5(e,NULL,NULL,NULL,0);
}

static size_t expr_varofep(const struct expr *restrict ep,double *v){
	size_t i;
	if(unlikely(expr_void(v)))
		return SIZE_MAX;
	for(i=0;i<ep->vsize;++i){
		if(ep->vars[i]==v){
			return i+1;
		}
	}
	if(v==&ep->un.end->val){
		return SIZE_MAX;
	}
	return 0;
}
static void expr_writeconsts(struct expr *restrict ep){
	for(struct expr_inst *ip=ep->data;ip->op!=EXPR_END;++ip){
		if(ip->op==EXPR_CONST&&ip->dst.dst&&
			expr_varofep(ep,ip->dst.dst)
			){
			if(likely(!expr_void(ip->dst.uaddr)))
				*ip->dst.dst=ip->un.value;
		}
	}
}


static void expr_optimize_completed(struct expr *restrict ep){
	struct expr_inst *cip=ep->data;
	for(struct expr_inst *ip=cip;ip-ep->data<ep->size;++ip){
		if(ip->dst.dst){
			if(ip>cip)
				memcpy(cip,ip,sizeof(struct expr_inst));
			++cip;
		}
	}
	ep->size=cip-ep->data;
}
static int expr_modified(const struct expr *restrict ep,double *v){
	if(!expr_varofep(ep,v))
		return 1;
	for(struct expr_inst *ip=ep->data;ip->op!=EXPR_END;++ip){
		if(ip->dst.dst==v&&ip->op!=EXPR_CONST){
			return 1;
		}
	}
	return 0;
}
static struct expr_inst *expr_findconst(const struct expr *restrict ep,struct expr_inst *ip){
	double *s=ip->dst.dst;
	for(--ip;ip>=ep->data;--ip){
		if(expr_usesrc(ip->op)&&ip->un.src==s)
			break;
		if(ip->dst.dst!=s)
			continue;
		if(ip->op==EXPR_CONST)
			return ip;
		else break;
	}
	return NULL;
}
#define optimize_end if(r){\
	expr_optimize_completed(ep);\
	debug("optimization end (%d)",r);\
}\
return r
static int expr_optimize_contadd(struct expr *restrict ep){
	double sum;
	struct expr_inst *rip;
	int r=0;
	for(struct expr_inst *ip=ep->data;ip->op!=EXPR_END;++ip){
		if(!expr_modified(ep,ip->dst.dst)
			||(ip->op!=EXPR_ADD&&ip->op!=EXPR_SUB)
			||expr_modified(ep,ip->un.src)
			)continue;
		sum=ip->op==EXPR_ADD?*ip->un.src:-*ip->un.src;
		for(struct expr_inst *ip1=ip+1;ip1->op!=EXPR_END;++ip1){
			if(ip1->dst.dst!=ip->dst.dst)
				continue;
			if(ip1->op!=EXPR_ADD
				&&ip1->op!=EXPR_SUB
				)
				break;
			if(!expr_modified(ep,ip1->un.src)){
				if(ip1->op==EXPR_ADD)
					sum+=*ip1->un.src;
				else
					sum-=*ip1->un.src;
				ip1->dst.dst=NULL;
				r=1;
			}
		}
		rip=expr_findconst(ep,ip);
		if(rip){
			rip->un.value+=sum;
			ip->dst.dst=NULL;
			r=1;
			expr_writeconsts(ep);
		}else {
			/*if(sum<0.0){
				*ip->un.src=-sum;
				ip->op=EXPR_SUB;
			}
			else */{
				*ip->un.src=sum;
				ip->op=EXPR_ADD;
			}
		}
		continue;
	}
	optimize_end;
}
static int expr_optimize_contsh(struct expr *restrict ep){
	double sum;
	struct expr_inst *rip;
	int r=0;
	for(struct expr_inst *ip=ep->data;ip->op!=EXPR_END;++ip){
		if(!expr_modified(ep,ip->dst.dst)
			||(ip->op!=EXPR_SHL&&ip->op!=EXPR_SHR)
			||expr_modified(ep,ip->un.src)
			)
			continue;
		sum=ip->op==EXPR_SHL?*ip->un.src:-*ip->un.src;
		for(struct expr_inst *ip1=ip+1;ip1->op!=EXPR_END;++ip1){
			if(ip1->dst.dst!=ip->dst.dst)
				continue;
			if(ip1->op!=EXPR_SHL
				&&ip1->op!=EXPR_SHR
				)
				break;
			if(!expr_modified(ep,ip1->un.src)){
				if(ip1->op==EXPR_SHL)
					sum+=*ip1->un.src;
				else
					sum-=*ip1->un.src;
				ip1->dst.dst=NULL;
				r=1;
			}
		}
		rip=expr_findconst(ep,ip);
		if(rip){
			EXPR_EDEXP(&rip->un.value)+=(int64_t)sum;
			ip->dst.dst=NULL;
			r=1;
			expr_writeconsts(ep);
		}else {
			/*if(sum<0.0){
				*ip->un.src=-sum;
				ip->op=EXPR_SHR;
			}
			else */{
				*ip->un.src=sum;
				ip->op=EXPR_SHL;
			}
		}
		continue;
	}
	optimize_end;
}
static int expr_optimize_contmul(struct expr *restrict ep,enum expr_op op){
	double sum;
	struct expr_inst *rip;
	int r=0;
	for(struct expr_inst *ip=ep->data;ip->op!=EXPR_END;++ip){
		if(!ip->dst.dst
			||!expr_modified(ep,ip->dst.dst)
			||ip->op!=op
			||expr_modified(ep,ip->un.src)
			)continue;
		rip=expr_findconst(ep,ip);
		if(rip){
			sum=rip->un.value;
		}else {
			switch(op){
				case EXPR_POW:
				case EXPR_MOD:
				case EXPR_LT:
				case EXPR_LE:
				case EXPR_SLE:
				case EXPR_SGE:
				case EXPR_GT:
				case EXPR_GE:
				case EXPR_EQ:
				case EXPR_SEQ:
				case EXPR_SNE:
				case EXPR_NE:
				case EXPR_NEXT:
				case EXPR_DIFF:
				case EXPR_OFF:
					continue;
				default:
					break;
			}
			sum=*ip->un.src;
		}
		for(struct expr_inst *ip1=ip+!rip;ip1->op!=EXPR_END;++ip1){
			if(ip1->dst.dst!=ip->dst.dst)
				continue;
			if(ip1->op!=op)
				break;
			if(!expr_modified(ep,(double *)ip1->un.src)){
				switch(op){
					case EXPR_ADD:
						sum+=*ip1->un.src;
						break;
					case EXPR_SUB:
						if(rip)
							sum-=*ip1->un.src;
						else
							sum+=*ip1->un.src;
						break;
					case EXPR_MUL:
						sum*=*ip1->un.src;
						break;
					case EXPR_DIV:
						if(rip)
							sum/=*ip1->un.src;
						else
							sum*=*ip1->un.src;
						break;
					case EXPR_MOD:
						sum=fmod(sum,*ip1->un.src);
						break;
					case EXPR_AND:
						sum=and2(sum,*ip1->un.src);
						break;
					case EXPR_OR:
						sum=or2(sum,*ip1->un.src);
						break;
					case EXPR_XOR:
						sum=xor2(sum,*ip1->un.src);
						break;
					case EXPR_POW:
						sum=pow(sum,*ip1->un.src);
						break;
					case EXPR_COPY:
						sum=*ip1->un.src;
						break;
					case EXPR_GT:
						sum=sum>*ip1->un.src?
							1.0:
							0.0;
						break;
					case EXPR_LT:
						sum=sum<*ip1->un.src?
							1.0:
							0.0;
						break;
					case EXPR_SGE:
						sum=sum>=*ip1->un.src?
							1.0:
							0.0;
						break;
					case EXPR_SLE:
						sum=sum<=*ip1->un.src?
							1.0:
							0.0;
						break;
					case EXPR_GE:
						sum=sum>=*ip1->un.src
						||expr_equal(sum,*ip1->un.src)?
							1.0:
							0.0;
						break;
					case EXPR_LE:
						sum=sum<=*ip1->un.src
						||expr_equal(sum,*ip1->un.src)?
							1.0:
							0.0;
						break;
					case EXPR_SEQ:
						sum=sum==*ip1->un.src?
							1.0:
							0.0;
						break;
					case EXPR_SNE:
						sum=sum!=*ip1->un.src?
							1.0:
							0.0;
						break;
					case EXPR_EQ:
						sum=expr_equal(sum,*ip1->un.src)?
							1.0:
							0.0;
						break;
					case EXPR_NE:
						sum=!expr_equal(sum,*ip1->un.src)?
							1.0:
							0.0;
						break;
					case EXPR_ANDL:
						sum=LOGIC(sum,*ip1->un.src,&&)?
							1.0:
							0.0;
						break;
					case EXPR_ORL:
						sum=LOGIC(sum,*ip1->un.src,||)?
							1.0:
							0.0;
						break;
					case EXPR_XORL:
						sum=LOGIC(sum,*ip1->un.src,^)?
							1.0:
							0.0;
						break;
					case EXPR_NEXT:
						EXPR_EDIVAL(&sum)+=(int64_t)*ip1->un.src;
						break;
					case EXPR_DIFF:
						sum=(double)(EXPR_EDIVAL(&sum)-*ip->un.isrc);
						break;
					default:
						__builtin_unreachable();
				}
				ip1->dst.dst=NULL;
				r=1;
			}
		}
		if(rip){
			rip->un.value=sum;
			ip->dst.dst=NULL;
			r=1;
			expr_writeconsts(ep);
		}else {
			*ip->un.src=sum;
		}
		continue;
	}
	optimize_end;
}
static double expr_zero_element(enum expr_op op){
	switch(op){
		case EXPR_ADD:
		case EXPR_SUB:
		case EXPR_OR:
		case EXPR_XOR:
		case EXPR_SHL:
		case EXPR_SHR:
		case EXPR_NEXT:
		case EXPR_OFF:
			return 0.0;
		case EXPR_MUL:
		case EXPR_DIV:
		case EXPR_MOD:
		case EXPR_POW:
			return 1.0;
		default:
			return -1.0;
	}
}
static int expr_optimize_zero(struct expr *restrict ep){
	double ze;
	int r=0;
	for(struct expr_inst *ip=ep->data;ip->op!=EXPR_END;++ip){
		switch(ip->op){
			case EXPR_ANDL:
				if(!expr_modified(ep,ip->dst.dst)&&*ip->un.src==0.0){
					ip->op=EXPR_CONST;
					ip->un.value=0.0;
					expr_writeconsts(ep);
					r=1;
				}
				break;
			case EXPR_ORL:
				if(!expr_modified(ep,ip->dst.dst)&&*ip->un.src!=0.0){
					ip->op=EXPR_CONST;
					ip->un.value=1.0;
					expr_writeconsts(ep);
					r=1;
				}
				break;
			default:
				ze=expr_zero_element(ip->op);
				if(ze<0.0
					||expr_modified(ep,ip->un.src)
					)continue;
				if(ze==*ip->un.src){
					ip->dst.dst=NULL;
					r=1;
				}
				break;
		}
	}
	optimize_end;
}
//oconst
static int expr_optimize_const(struct expr *restrict ep){
	int r=0;
	for(struct expr_inst *ip=ep->data;ip->op!=EXPR_END;++ip){
		if(ip->op==EXPR_CONST&&!expr_modified(ep,ip->dst.dst)){
			if(!(ip->flag&EXPR_SF_INJECTION)){
				ip->dst.dst=NULL;
				r=1;
			}
		}
	}
	optimize_end;
}
static int expr_side(enum expr_op op){
	switch(op){
		case SRCCASES:
		case EXPR_INPUT:
		case EXPR_IP:
		case EXPR_CONST:
		case EXPR_NEG:
		case EXPR_NOT:
		case EXPR_NOTL:
		case EXPR_TSTL:
		case EXPR_NEX0:
		case EXPR_DIF0:
		case EXPR_ALO:
			return 0;
		default:
			return 1;
	}
}
static int expr_override(enum expr_op op){
	switch(op){
		case EXPR_COPY:
		case EXPR_INPUT:
		case EXPR_IP:
		case EXPR_CONST:
		case BRANCHCASES:
		case SUMCASES:
		case EXPR_ZA:
		case EXPR_PZA:
		case EXPR_EP:
		case MDCASES:
		case EXPR_HMD:
		case EXPR_VMD:
		case EXPR_READ:
			return 1;
		default:
			return 0;
	}
}
static int expr_usesrc(enum expr_op op){
	switch(op){
		case SRCCASES:
		case EXPR_READ:
		case EXPR_WRITE:
		case EXPR_LJ:
		case EXPR_PBL:
		case EXPR_PZA:
		case EXPR_EVAL:
		case EXPR_RET:
		case EXPR_SVCP:
		case EXPR_SVCP0:
			return 1;
		default:
			return 0;
	}
}

static int expr_vused(struct expr_inst *ip1,double *v){
	int ov;
	for(;;++ip1){
		if(ip1->op==EXPR_CONST&&(ip1->flag&EXPR_SF_INJECTION)&&cast(ip1->un.value,double *)==v)
			return 1;
		ov=expr_override(ip1->op);
		if((expr_usesrc(ip1->op)&&ip1->un.src==v)
			||(ip1->dst.dst==v&&!ov)
		){
			return 1;
		}
		if(ip1->dst.dst==v&&ov){
			return 0;
		}
		if(ip1->op==EXPR_END){
			return 0;
		}
	}
	return 0;
}
static int expr_constexpr(const struct expr *restrict ep,double *except){
	for(struct expr_inst *ip=ep->data;;++ip){
		if(!expr_varofep(ep,ip->dst.dst)&&ip->dst.dst!=except)
			return 0;
		switch(ip->op){
			case EXPR_BL:
			case EXPR_ZA:
			case EXPR_HOT:
				if(ip->flag&EXPR_SF_INJECTION)
					break;
			case EXPR_INPUT:
			case EXPR_IP:
			case EXPR_TO:
			case EXPR_TO1:
			case MDCASES_WITHP:
			case EXPR_VMD:
			case EXPR_HMD:
			case SUMCASES:
			case BRANCHCASES:
			case EXPR_DO:
			case EXPR_DO1:
			case EXPR_EP:
			case EXPR_WIF:
			case EXPR_EVAL:
			case EXPR_RET:
			case SVCCASES:
			case EXPR_READ:
			case EXPR_WRITE:
			case EXPR_ALO:
			case EXPR_SJ:
			case EXPR_LJ:
			case EXPR_PBL:
			case EXPR_PZA:
				return 0;
			case SRCCASES:
				if(!expr_varofep(ep,ip->un.src)&&
					ip->un.src!=except)
					return 0;
				break;
			case EXPR_END:
				return 1;
			case EXPR_CONST:
				if(ip->flag&EXPR_SF_INJECTION){
					return 0;
				}
			default:
				break;
		}
	}
}
#define CALSUM(_op,_do,_init,_neg,dest)\
			case _op :\
				neg=0;\
				from=eval(ip->un.es->fromep,input);\
				to=eval(ip->un.es->toep,input);\
				if(unlikely(from>to)){\
					step=from;\
					from=to;\
					to=step;\
					neg=1;\
				}\
				step=eval(ip->un.es->stepep,input);\
				if(unlikely(step<0)){\
					step=-step;\
					neg^=1;\
				}\
				_init ;\
				for(ip->un.es->index=from;\
					likely(ip->un.es->index<=to);\
					ip->un.es->index+=step){\
					y=eval(ip->un.es->ep,input) ;\
					_do ;\
				}\
				if(unlikely(neg))\
					sum= _neg ;\
				*dest=sum;\
				break
#define CALSUM_INSWITCH(dest) CALSUM(EXPR_SUM,sum+=y,sum=0.0,-sum,dest);\
			CALSUM(EXPR_INT,sum+=step*y,sum=0.0;from+=step/2.0,-sum,dest);\
			CALSUM(EXPR_PROD,sum*=y,sum=1.0,1.0/sum,dest);\
			CALSUM(EXPR_SUP,if(unlikely(!inited)){inited=1;sum=y;}else if(y>sum)sum=y,inited=0,sum,dest);\
			CALSUM(EXPR_INF,if(unlikely(!inited)){inited=1;sum=y;}else if(y<sum)sum=y,inited=0,sum,dest);\
			CALSUM(EXPR_ANDN,sum=likely(inited)?and2(sum,y):(inited=1,y),inited=0,sum,dest);\
			CALSUM(EXPR_ORN,sum=likely(inited)?or2(sum,y):(inited=1,y),inited=0,sum,dest);\
			CALSUM(EXPR_XORN,sum=likely(inited)?xor2(sum,y):(inited=1,y),inited=0,sum,dest);\
			CALSUM(EXPR_GCDN,sum=likely(inited)?gcd2(sum,y):(inited=1,y),inited=0,sum,dest);\
			CALSUM(EXPR_LCMN,sum=likely(inited)?lcm2(sum,y):(inited=1,y),inited=0,sum,dest);\
			CALSUM(EXPR_FOR,,,sum,dest);\
			case EXPR_LOOP:\
				ip->un.es->index=\
				eval(ip->un.es->fromep,input);\
				to=eval(ip->un.es->toep,input);\
				if(unlikely(to<0))\
					to=-to;\
				for(;likely(to>0.0);to-=1.0){\
					eval(ip->un.es->stepep,input);\
				}\
				*dest=eval(ip->un.es->ep,input);\
				break
#define CALMD_INSWITCH(dest) case EXPR_MD:\
				ap=ip->un.em->args;\
				endp=ap+ip->un.em->dim;\
				epp=ip->un.em->eps;\
				for(;likely(ap<endp);++ap)\
					*ap=eval(epp++,input);\
				*dest=ip->un.em->un.func(ip->un.em->args,ip->un.em->dim);\
				break;\
			case EXPR_MEP:\
				((volatile struct expr *)ep)->ipp=&ip;\
			case EXPR_ME:\
				*dest=ip->un.em->un.funcep(ip->un.em->eps,ip->un.em->dim,input);\
				break
#define CALHMD_INSWITCH(dest) case EXPR_HMD:\
				ap=ip->un.eh->args;\
				endp=ap+ip->un.eh->dim;\
				epp=ip->un.eh->eps;\
				for(;likely(ap<endp);++ap)\
					*ap=eval(epp++,input);\
				*dest=eval(ip->un.eh->hotfunc,input);\
				break

#define vmdeval_def(___ev,input) (\
{\
	_ev=(___ev);\
	_max=_ev->max;\
	from=eval(_ev->fromep,input);\
	to=eval(_ev->toep,input);\
	step=fabs(eval(_ev->stepep,input));\
	if(unlikely(!_max)){\
		if(unlikely(step==0.0))\
			return NAN;\
		_max=1;\
		for(double from1=from,from2;;++_max){\
			if(likely(from<to)){\
				from2=from1;\
				from1+=step;\
				if(from1>to)\
					break;\
			}else if(from>to){\
				from2=from1;\
				from1-=step;\
				if(from1<to)\
					break;\
			}else {\
				break;\
			}\
			if(from2==from1)\
				return NAN;\
		}\
	}\
	_args=_ev->args?_ev->args:alloca(_max*sizeof(double));\
	ap=_args;\
	_ev->index=from;\
	for(;likely(_max);--_max){\
		*(ap++)=eval(_ev->ep,input);\
		if(likely(from<to)){\
			_ev->index+=step;\
			if(_ev->index>to)\
				break;\
		}else if(from>to){\
			_ev->index-=step;\
			if(_ev->index<to)\
				break;\
		}else {\
			break;\
		}\
	}\
	_ev->func(_args,ap-_args);\
}\
)

#define EXPR_EVALVARS \
	union {\
		struct {\
			double _step,_from,_to,_y,_sum;\
			int _neg,_inited;\
		} s0;\
		struct {\
			double _step,_from,_to;\
			double *_ap,*_endp;\
			struct expr *_epp;\
		} s1;\
		struct {\
			double _step,_from,_to;\
			double *_ap,*__args;\
			size_t __max;\
			struct expr_vmdinfo *restrict __ev;\
		} sv;\
		struct {\
			struct expr *ep;\
			struct expr_inst *end;\
		} r;\
		struct {\
			intptr_t num;\
			const intptr_t *arg;\
			int count;\
		} svc;\
		union {\
			double d;\
			struct expr_internal_jmpbuf *p;\
		} j;\
		void *up;\
		double d;\
		size_t index;\
	} un

#define sum (un.s0._sum)
#define from (un.s0._from)
#define to (un.s0._to)
#define step (un.s0._step)
#define y (un.s0._y)
#define neg (un.s0._neg)
#define ap (un.s1._ap)
#define endp (un.s1._endp)
#define epp (un.s1._epp)
#define _max (un.sv.__max)
#define _ev (un.sv.__ev)
#define _args (un.sv.__args)
#define inited (un.s0._inited)

__attribute__((noinline))
static double vmdeval(struct expr_vmdinfo *restrict ev,double input){
	EXPR_EVALVARS;
	return vmdeval_def(ev,input);
}
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#pragma GCC diagnostic ignored "-Wdangling-pointer"
static int expr_optimize_constexpr(struct expr *restrict ep){
	EXPR_EVALVARS;
	double result;
	static const double input=0.0;
	struct expr *hep,*endp1;
	int r=0;
	for(struct expr_inst *ip=ep->data;ip->op!=EXPR_END;++ip){
		switch(ip->op){
			case SUMCASES:
				if(!(expr_constexpr(ip->un.es->fromep,NULL)&&
				expr_constexpr(ip->un.es->toep,(double *)&ip->un.es->index)&&
				expr_constexpr(ip->un.es->stepep,(double *)&ip->un.es->index)&&
				expr_constexpr(ip->un.es->ep,(double *)&ip->un.es->index)))
					continue;
				switch(ip->op){
					CALSUM_INSWITCH((&result));
					default:
						__builtin_unreachable();
				}
				freesuminfo(ip->un.es);
				ip->op=EXPR_CONST;
				ip->un.value=result;
				expr_writeconsts(ep);
				r=1;
				break;
			case MDCASES:
				if(!(ip->flag&EXPR_SF_INJECTION))
					continue;
				epp=ip->un.em->eps;
				endp1=epp+ip->un.em->dim;
				for(;epp<endp1;++epp){
					if(!expr_constexpr(epp,NULL))
						goto force_continue;
				}
				switch(ip->op){
					CALMD_INSWITCH((&result));
					default:
						__builtin_unreachable();
				}
				freemdinfo(ip->un.em);
				ip->op=EXPR_CONST;
				ip->flag=0;
				ip->un.value=result;
				expr_writeconsts(ep);
				r=1;
				break;
			case EXPR_VMD:
				if(!(ip->flag&EXPR_SF_INJECTION))
					continue;
				if(!(expr_constexpr(ip->un.ev->fromep,NULL)&&
				expr_constexpr(ip->un.ev->toep,(double *)&ip->un.ev->index)&&
				expr_constexpr(ip->un.ev->stepep,(double *)&ip->un.ev->index)&&
				expr_constexpr(ip->un.ev->ep,(double *)&ip->un.ev->index)))
					continue;
				result=vmdeval(ip->un.ev,input);
				freevmdinfo(ip->un.ev);
				ip->op=EXPR_CONST;
				ip->flag=0;
				ip->un.value=result;
				expr_writeconsts(ep);
				r=1;
				break;
			case EXPR_HMD:
				if(!(ip->flag&EXPR_SF_INJECTION))
					continue;
				epp=ip->un.eh->eps;
				endp1=epp+ip->un.eh->dim;
				for(;epp<endp1;++epp){
					if(!expr_constexpr(epp,NULL))
						goto force_continue;
				}
				switch(ip->op){
					CALHMD_INSWITCH((&result));
					default:
						__builtin_unreachable();
				}
				freehmdinfo(ip->un.eh);
				ip->op=EXPR_CONST;
				ip->flag=0;
				ip->un.value=result;
				expr_writeconsts(ep);
				r=1;
				break;
			case EXPR_WHILE:
				if(!expr_constexpr(ip->un.eb->cond,NULL)){
					if(expr_constexpr(ip->un.eb->body,NULL)){
wif:
						hep=ip->un.eb->cond;
						expr_free(ip->un.eb->body);
						expr_free(ip->un.eb->value);
						xfree(ip->un.eb);
						ip->op=EXPR_WIF;
						ip->un.hotfunc=hep;
						ip->flag=0;
						r=1;
						break;
					}
					continue;
				}
				if(eval(ip->un.eb->cond,input)!=0.0){
endless:
					hep=ip->un.eb->body;
					expr_free(ip->un.eb->cond);
					expr_free(ip->un.eb->value);
					xfree(ip->un.eb);
					ip->op=EXPR_DO;
					ip->un.hotfunc=hep;
					ip->flag=0;
					r=1;
					break;
				}
				hep=ip->un.eb->value;
				expr_free(ip->un.eb->cond);
				expr_free(ip->un.eb->body);
				goto free_eb;
			case EXPR_DON:
				if(expr_constexpr(ip->un.eb->body,NULL))
					goto delete_eb;
				if(!expr_constexpr(ip->un.eb->cond,NULL))
					continue;
				switch((size_t)eval(ip->un.eb->cond,input)){
					case 0:
						/*hep=ip->un.eb->value;
						expr_free(ip->un.eb->cond);
						expr_free(ip->un.eb->body);
						goto free_eb1;*/
delete_eb:
						expr_free(ip->un.eb->cond);
						expr_free(ip->un.eb->body);
						expr_free(ip->un.eb->value);
						xfree(ip->un.eb);
						ip->dst.uaddr=NULL;
						continue;
					case 1:
						hep=ip->un.eb->body;
						expr_free(ip->un.eb->cond);
						expr_free(ip->un.eb->value);
						goto free_eb1;
					default:
						continue;
				}
				break;
			case EXPR_DOW:
				if(!expr_constexpr(ip->un.eb->cond,NULL)){
					if(expr_constexpr(ip->un.eb->body,NULL))
						goto wif;
					continue;
				}
				if(eval(ip->un.eb->cond,input)!=0.0)
					goto endless;
				else {
					hep=ip->un.eb->body;
					expr_free(ip->un.eb->cond);
					expr_free(ip->un.eb->value);
free_eb1:
					xfree(ip->un.eb);
					ip->op=EXPR_DO1;
					ip->un.hotfunc=hep;
					ip->flag=0;
					r=1;
				}
				break;
			case EXPR_IF:
				if(!expr_constexpr(ip->un.eb->cond,NULL))
					continue;
				result=eval(ip->un.eb->cond,input);
				expr_free(ip->un.eb->cond);
				if(result!=0.0){
					hep=ip->un.eb->body;
					expr_free(ip->un.eb->value);
				}else {
					hep=ip->un.eb->value;
					expr_free(ip->un.eb->body);
				}
free_eb:
				xfree(ip->un.eb);
				ip->op=EXPR_EP;
				ip->un.hotfunc=hep;
				ip->flag=0;
				r=1;
			case EXPR_HOT:
			case EXPR_EP:
			case EXPR_WIF:
				if(!expr_constexpr(ip->un.hotfunc,NULL))
					continue;
				result=eval(ip->un.hotfunc,input);
				expr_free(ip->un.hotfunc);
				ip->op=EXPR_CONST;
				ip->un.value=result;
				expr_writeconsts(ep);
				ip->flag=0;
				r=1;
				break;
			case EXPR_DO1:
				if(!expr_constexpr(ip->un.hotfunc,NULL))
					continue;
				expr_free(ip->un.hotfunc);
				ip->dst.uaddr=NULL;
			default:
				break;
		}
force_continue:
		continue;
	}
	return r;
#undef sum
#undef from
#undef to
#undef step
#undef y
#undef neg
#undef ap
#undef endp
#undef epp
#undef _max
#undef _ev
#undef _args
#undef inited
}
#pragma GCC diagnostic pop
static int expr_vcheck_ep(struct expr *restrict ep,struct expr_inst *ip,double *v){
	if(expr_vused(ip,v))
		return 1;
	for(;;++ip){
		switch(ip->op){
			case EXPR_LJ:
			case EXPR_TO:
			case EXPR_TO1:
				return 1;
			case EXPR_END:
				if(ip->dst.dst==v)
					return 1;
				else
					goto break2;
			case SUMCASES:
				if(
					expr_vcheck_ep(ip->un.es->fromep,ip->un.es->fromep->data,v)||
					expr_vcheck_ep(ip->un.es->toep,ip->un.es->toep->data,v)||
					expr_vcheck_ep(ip->un.es->stepep,ip->un.es->stepep->data,v)||
					expr_vcheck_ep(ip->un.es->ep,ip->un.es->ep->data,v))
					return 1;
				break;
			case EXPR_VMD:
				if(
					expr_vcheck_ep(ip->un.ev->fromep,ip->un.ev->fromep->data,v)||
					expr_vcheck_ep(ip->un.ev->toep,ip->un.ev->toep->data,v)||
					expr_vcheck_ep(ip->un.ev->stepep,ip->un.ev->stepep->data,v)||
					expr_vcheck_ep(ip->un.ev->ep,ip->un.ev->ep->data,v))
					return 1;
				break;
			case BRANCHCASES:
				if(
					expr_vcheck_ep(ip->un.eb->cond,ip->un.eb->cond->data,v)||
					expr_vcheck_ep(ip->un.eb->body,ip->un.eb->body->data,v)||
					expr_vcheck_ep(ip->un.eb->value,ip->un.eb->value->data,v))
					return 1;
				break;
			case MDCASES_WITHP:
				for(size_t i=0;i<ip->un.em->dim;++i){
					if(expr_vcheck_ep(ip->un.em->eps+i,ip->un.em->eps[i].data,v))
						return 1;
				}
				break;
			case HOTCASES:
				if(
					expr_vcheck_ep(ip->un.hotfunc,ip->un.hotfunc->data,v)
					)
					return 1;
				break;
			case EXPR_HMD:
				for(size_t i=0;i<ip->un.eh->dim;++i){
					if(expr_vcheck_ep(ip->un.eh->eps+i,ip->un.eh->eps[i].data,v))
						return 1;
				}
				if(expr_vcheck_ep(ip->un.eh->hotfunc,ip->un.eh->hotfunc->data,v))
					return 1;
				break;
			default:
				break;
		}
	}
break2:
	for(struct expr_resource *rp=ep->res;rp;rp=rp->next){
		if(!rp->un.uaddr)
			continue;
		switch(rp->type){
			case EXPR_HOTFUNCTION:
				if(expr_vcheck_ep(rp->un.ep,rp->un.ep->data,v))
					return 1;
			default:
				break;
		}
	}
	return 0;
}
/*static int expr_optimize_mulpow2n(struct expr *restrict ep){
	int r=0;
	for(struct expr_inst *ip=ep->data;ip->op!=EXPR_END;++ip){
		switch(ip->op){
			case EXPR_MUL:
			case EXPR_DIV:
				break;
			default:
				continue;
		}
		if(expr_modified(ep,ip->un.src)||
			EXPR_EDSIGN(ip->un.src)||
			EXPR_EDBASE(ip->un.src))
			continue;
		switch(ip->op){
			case EXPR_MUL:
				*ip->un.src=(double)(EXPR_EDEXP(ip->un.src)-1023);
				break;
			case EXPR_DIV:
				*ip->un.src=-(double)(EXPR_EDEXP(ip->un.src)-1023);
				break;
			default:
				__builtin_unreachable();
		}
		ip->op=EXPR_SHL;
		r=1;
	}
	return r;
}*/
//2 bugs
static int expr_optimize_unused(struct expr *restrict ep){
	int r=0;
	for(struct expr_inst *ip=ep->data;ip->op!=EXPR_END;++ip){

		if(!expr_varofep(ep,ip->dst.dst)
			||expr_side(ip->op))continue;
		if(!expr_vcheck_ep(ep,ip+1,ip->dst.dst)){
			ip->dst.dst=NULL;
			r=1;
		}
	}
	optimize_end;
}
static int expr_optimize_constneg(struct expr *restrict ep){
	int r=0;
	for(struct expr_inst *ip=ep->data;ip->op!=EXPR_END;++ip){
		switch(ip->op){
			case EXPR_NEG:
			case EXPR_NOT:
			case EXPR_NOTL:
			case EXPR_TSTL:
			case EXPR_NEX0:
			case EXPR_DIF0:
				break;
			default:
				continue;
		}
		for(struct expr_inst *ip1=ip-1;ip1>=ep->data;--ip1){
			if(ip1->dst.dst!=ip->dst.dst)
				continue;
			if(ip1->op!=EXPR_CONST)
				break;
			switch(ip->op){
				case EXPR_NEG:
					ip1->un.value=-ip1->un.value;
					break;
				case EXPR_NOT:
					ip1->un.value=not(ip1->un.value);
					break;
				case EXPR_NOTL:
					ip1->un.value=(ip1->un.value==0.0)?
						1.0:0.0;
					break;
				case EXPR_TSTL:
					ip1->un.value=(ip1->un.value!=0.0)?
						1.0:0.0;
					break;
				case EXPR_NEX0:
					ip1->un.value=nex0(ip1->un.value);
					break;
				case EXPR_DIF0:
					ip1->un.value=dif0(ip1->un.value);
					break;
				default:
					continue;
			}
			expr_writeconsts(ep);
			ip->dst.dst=NULL;
			r=1;
			break;
		}
	}
	optimize_end;
}

static int expr_injection_optype(enum expr_op op){
	switch(op){
		case EXPR_BL:
		case EXPR_ZA:
		case EXPR_HOT:
			return 1;
		default:
			return 0;
	}
}
static int expr_isinjection(struct expr *restrict ep,struct expr_inst *ip){
	return expr_injection_optype(ip->op)
		&&(ip->flag&EXPR_SF_INJECTION);
}
static int expr_injective_hotfunction_check(struct expr *restrict ep){
	for(struct expr_inst *ip=ep->data;;++ip){
		if(!expr_varofep(ep,ip->dst.dst))
			return 0;
		switch(ip->op){
			case EXPR_END:
				return 1;
			case EXPR_CONST:
			case EXPR_INPUT:
				break;
			case EXPR_BL:
			case EXPR_ZA:
			case MDCASES:
			case EXPR_HMD:
				if(!(ip->flag&EXPR_SF_INJECTION))
					return 0;
				break;
			case SRCCASES:
				if(!expr_varofep(ep,ip->un.src))
					return 0;
				break;
			case EXPR_HOT:
				if(!expr_injective_hotfunction_check(ip->un.hotfunc))
					return 0;
				break;
			default:
				return 0;
		}
	}
}
#define checkeihc(V) (!expr_varofep(ep,V)&&(V<eh->args||V>=eh->args+eh->dim))
static int expr_injective_hotmdfunction_check(struct expr_hmdinfo *eh){
	struct expr *restrict ep=eh->hotfunc;
	for(struct expr_inst *ip=ep->data;;++ip){
		if(checkeihc(ip->dst.dst))
			return 0;
		switch(ip->op){
			case EXPR_END:
				return 1;
			case EXPR_CONST:
				break;
			case EXPR_BL:
			case EXPR_ZA:
			case MDCASES:
			case EXPR_HOT:
			case EXPR_HMD:
				if(!(ip->flag&EXPR_SF_INJECTION))
					return 0;
				break;
			case SRCCASES:
			if(checkeihc(ip->un.src))
					return 0;
				break;
			default:
			case EXPR_INPUT:
				return 0;
		}
	}
}
static int expr_optimize_injective_hotfunction(struct expr *restrict ep){
	int r=0;
	for(struct expr_inst *ip=ep->data;ip->op!=EXPR_END;++ip){
		switch(ip->op){
			case EXPR_HOT:
				if(ip->flag&EXPR_SF_INJECTION)
					break;
				if(!expr_injective_hotfunction_check(ip->un.hotfunc))
					break;
				ip->flag|=EXPR_SF_INJECTION;
				r=1;
				break;
			case EXPR_HMD:
				if(ip->flag&EXPR_SF_INJECTION)
					break;
				if(!expr_injective_hotmdfunction_check(ip->un.eh))
					break;
				ip->flag|=EXPR_SF_INJECTION;
				r=1;
				break;
			default:
				break;
		}
		continue;
	}
	optimize_end;
}
static int expr_optimize_injection(struct expr *restrict ep){
	int r=0;
	for(struct expr_inst *ip=ep->data;ip->op!=EXPR_END;++ip){
			if(!expr_isinjection(ep,ip))
				continue;
			if(ip->op==EXPR_ZA){
				ip->un.value=ip->un.zafunc();
				ip->op=EXPR_CONST;
				ip->flag=0;
				expr_writeconsts(ep);
				r=1;
				continue;
			}
		for(struct expr_inst *ip1=ip-1;ip1>=ep->data;--ip1){
			if(ip1->dst.dst!=ip->dst.dst)
				continue;
			switch(ip1->op){
				case EXPR_INPUT:
				case EXPR_IP:
					if(!expr_vcheck_ep(ep,ip+1,ip->dst.dst))
						goto delete;
				default:
					break;
			}
			if(ip1->op!=EXPR_CONST)
				break;
			switch(ip->op){
				case EXPR_BL:
					ip1->un.value=ip->un.func(ip1->un.value);
					break;
				case EXPR_HOT:
					ip1->un.value=eval(ip->un.hotfunc,ip1->un.value);
					expr_free(ip->un.hotfunc);
					break;
				default:
					__builtin_unreachable();
			}
			expr_writeconsts(ep);
delete:
			r=1;
			ip->dst.dst=NULL;
			break;
		}
	}
	optimize_end;
}
static int expr_optimize_copyadd(struct expr *restrict ep){
	int r=0;
	for(struct expr_inst *ip=ep->data;ip->op!=EXPR_END;++ip){
		if(ip->op!=EXPR_COPY)
			continue;
		switch(ip[1].op){
			case SRCCASES_NOCOPY:
				break;
			default:
				continue;
		}
		if(!expr_varofep(ep,ip->dst.dst))
			continue;
		if(ip[1].un.src!=ip->dst.dst)
			continue;
		if(expr_vcheck_ep(ep,ip+2,ip->dst.dst))
			continue;
		ip->dst.dst=NULL;
		ip[1].un.src=ip->un.src;
		++ip;
		r=1;
	}
	optimize_end;
}
/*static int expr_optimize_copy2const(struct expr *restrict ep){
	int r=0;
	for(struct expr_inst *ip=ep->data;ip->op!=EXPR_END;++ip){
		if(ip->op!=EXPR_COPY
			//||
			//!expr_varofep(ep,ip->dst.dst)||
			//!expr_varofep(ep,ip->un.src)
			)continue;
		if(!expr_modified(ep,ip->un.src)){
			ip->op=EXPR_CONST;
			ip->un.value=*ip->un.src;
			expr_writeconsts(ep);
			r=1;
		}
	}
	expr_optimize_completed(ep);
	return r;
}*/
//a bug cannot fix
static int expr_optimize_copyend(struct expr *restrict ep){
	int r=0;
	struct expr_inst *ip=ep->data+ep->size-1;
	if(ip<=ep->data||ip[-1].op!=EXPR_COPY/*||!varofep(ep,ip->dst.dst)*/)
		goto end;
	if(ip->dst.dst==ip[-1].dst.dst){
		if(expr_varofep(ep,ip->dst.dst)){
			ip->dst.dst=ip[-1].un.src;
			ip[-1].dst.dst=NULL;
			r=1;
		}
	}else if(ip->dst.dst==ip[-1].un.src){
			ip->dst.dst=ip[-1].dst.dst;
			r=1;
	}
end:
	optimize_end;
}
static int expr_optimize_strongorder_and_notl(struct expr *restrict ep){
	int r=0;
	for(struct expr_inst *ip=ep->data;ip->op!=EXPR_END;++ip){
		if(ip[1].dst.dst!=ip->dst.dst)
			continue;
		switch(ip[1].op){
			case EXPR_NOTL:
			case EXPR_TSTL:
				break;
			default:
				continue;
		}
		switch(ip->op){
			case EXPR_LT:
			case EXPR_GT:
			case EXPR_SLE:
			case EXPR_SGE:
			case EXPR_SEQ:
			case EXPR_SNE:
			case EXPR_EQ:
			case EXPR_NE:
				break;
			default:
				++ip;
				continue;
		}
		if(ip[1].op==EXPR_NOTL)switch(ip->op){
			case EXPR_LT:
				ip->op=EXPR_SGE;
				break;
			case EXPR_GT:
				ip->op=EXPR_SLE;
				break;
			case EXPR_SLE:
				ip->op=EXPR_GT;
				break;
			case EXPR_SGE:
				ip->op=EXPR_LT;
				break;
			case EXPR_SEQ:
				ip->op=EXPR_SNE;
				break;
			case EXPR_SNE:
				ip->op=EXPR_SEQ;
				break;
			case EXPR_EQ:
				ip->op=EXPR_NE;
				break;
			case EXPR_NE:
				ip->op=EXPR_EQ;
				break;
			default:
				__builtin_unreachable();
		}
		(++ip)->dst.dst=NULL;
		r=1;
	}
	optimize_end;
}
static int expr_optimize_contneg(struct expr *restrict ep){
	int r=0;
	for(struct expr_inst *ip=ep->data;ip->op!=EXPR_END;++ip){
		if(ip[1].dst.dst!=ip->dst.dst)
			continue;
		switch(ip->op){
			case EXPR_NOT:
			case EXPR_NEG:
				if(ip[1].op==ip->op){
					ip->dst.dst=NULL;
					(++ip)->dst.dst=NULL;
					r=1;
				}
				break;
			case EXPR_TSTL:
				switch(ip[1].op){
					case EXPR_NOT:
						goto from_tstl_to_not;
					case EXPR_NEG:
						goto from_tstl_to_neg;
					case EXPR_NOTL:
					case EXPR_TSTL:
						(ip++)->dst.dst=NULL;
						r=1;
						break;
					default:
						break;
				}
				break;
			case EXPR_NOTL:
				switch(ip[1].op){
					case EXPR_NOT:
from_tstl_to_not:
						if(ip[2].dst.dst==ip->dst.dst)
						switch(ip[2].op){
							case EXPR_NOTL:
								ip->un.value=0;
								goto notl_tstl;
							case EXPR_TSTL:
								ip->un.value=1;
notl_tstl:
								ip->op=EXPR_CONST;
								(++ip)->dst.dst=NULL;
								(++ip)->dst.dst=NULL;
								r=1;
								expr_writeconsts(ep);
							default:
								break;
						}
						continue;
					case EXPR_NEG:

from_tstl_to_neg:
						if(ip[2].dst.dst==ip->dst.dst)
						switch(ip[2].op){
							case EXPR_NOTL:
							case EXPR_TSTL:
								(++ip)->dst.dst=NULL;
								r=1;
							default:
								break;
						}
						continue;
					case EXPR_NOTL:
						ip->op=EXPR_TSTL;
					case EXPR_TSTL:
						(++ip)->dst.dst=NULL;
						r=1;
						break;
					default:
						break;
				}
				break;
			default:
				break;
		}
	}
	optimize_end;
}
static int expr_optimize_contcopy(struct expr *restrict ep){
	int r=0;
	for(struct expr_inst *ip=ep->data;ip->op!=EXPR_END;++ip){
		if(ip->op!=EXPR_COPY||ip[1].op!=EXPR_COPY)
			continue;
		if(ip[1].un.src!=ip->dst.dst)
			continue;
		if(expr_vcheck_ep(ep,ip+2,ip->dst.dst))
			continue;
		ip->dst.dst=ip[1].dst.dst;
		(++ip)->dst.dst=NULL;
		r=1;
	}
	optimize_end;
}
static int expr_optimize_copysrcunused(struct expr *restrict ep){
	struct expr_inst *ip1;
	int r=0;
	for(struct expr_inst *ip=ep->data;ip->op!=EXPR_END;++ip){
		if(ip->op!=EXPR_COPY)
			continue;
		if(expr_vcheck_ep(ep,ip+1,ip->un.src))
			continue;
		if(!expr_varofep(ep,ip->dst.dst)||!expr_varofep(ep,ip->un.src))
			continue;
		for(ip1=ip+1;;++ip1){
			switch(ip1->op){
				case UNARYCASES:
				case SRCCASES:
					continue;
				case EXPR_END:
					++ip1;
					goto atend;
				default:
					goto break2;
			}
		}
break2:
		if(expr_vcheck_ep(ep,ip1,ip->dst.dst))
			continue;
atend:
		debug("setting %zd ips",ip1-ip-1);
		for(struct expr_inst *ip2=ip+1;ip2<ip1;++ip2){
			if(ip2->dst.dst==ip->dst.dst)
				ip2->dst.dst=ip->un.src;
			switch(ip2->op){
				case SRCCASES:
					if(ip2->un.src==ip->dst.dst)
						ip2->un.src=ip->un.src;
				default:
					break;
			}
		}
		ip->dst.dst=NULL;
		r=1;
	}
	optimize_end;
}
static int expr_optimize_coc(struct expr *restrict ep){
	int r=0;
	size_t k;
	for(struct expr_inst *ip=ep->data;ip->op!=EXPR_END;++ip){
		if(ip->op!=EXPR_COPY)
			continue;
		for(k=1;;++k){
			switch(ip[k].op){
				case SRCCASES_NOCOPY:
					if(ip[k].un.src==ip->dst.dst)
						goto continue2;
				case UNARYCASES:
					if(ip[k].dst.dst!=ip->dst.dst)
						goto continue2;
					break;
				case EXPR_COPY:
					if(ip->dst.dst!=ip[k].un.src)
						goto continue2;
					if(ip[k].dst.dst!=ip->un.src)
						goto continue2;
					goto break2;
				default:
					goto continue2;
			}
		}
break2:
		if(k>1){
			for(size_t j=1;;){
				ip[j].dst.dst=ip->un.src;
				++j;
				if(j==k)
					break;
			}
		}else {
			ip[1].dst.dst=NULL;
			++ip;
			r=1;
			continue;
		}
		if(expr_vcheck_ep(ep,ip+k+1,ip->dst.dst)){
			ip[k].dst.dst=ip->dst.dst;
			ip[k].un.src=ip->un.src;
		}else {
			ip[k].dst.dst=NULL;
		}
		ip->dst.dst=NULL;
		ip+=k;
		r=1;
continue2:
		continue;
	}
	optimize_end;
}
static int expr_optimize_cac(struct expr *restrict ep){
	int r=0;
	size_t k;
	for(struct expr_inst *ip=ep->data;ip->op!=EXPR_END;++ip){
		if(ip->op!=EXPR_CONST)
			continue;
		switch(ip[1].op){
			case SRCCASES_NOCOPY_SWAPABLE:
				if(ip[1].dst.dst!=ip->dst.dst)
					goto continue2;
				break;
			default:
				goto continue2;
		}
		for(k=2;;++k){
			switch(ip[k].op){
				case SRCCASES_NOCOPY_SWAPABLE:
					if(!expr_varofep(ep,ip[k].un.src))
						goto continue2;
				case UNARYCASES:
					if(ip[k].dst.dst!=ip->dst.dst)
						goto continue2;
					break;
				case EXPR_COPY:
					if(ip[k].dst.dst!=ip[1].un.src)
						goto continue2;
					if(ip[k].un.src!=ip->dst.dst)
						goto continue2;
					goto break2;
				default:
					goto continue2;
			}
		}
break2:
		if(expr_vcheck_ep(ep,ip+k+1,ip->dst.dst))
			continue;
		*ip->dst.dst=ip->un.value;
		for(size_t j=2;j<k;++j){
			ip[j].dst.dst=ip[1].un.src;
		}
		ip[1].dst.dst=ip[1].un.src;
		ip[1].un.src=ip->dst.dst;
		ip->dst.dst=NULL;
		ip[k].dst.dst=NULL;
		ip+=k;
		r=1;
continue2:
		continue;
	}
	optimize_end;
}
static int expr_optimize_svc0(struct expr *restrict ep){
	int r=0;
	for(struct expr_inst *ip=ep->data;ip->op!=EXPR_END;++ip){
		if(ip[1].dst.dst!=ip->dst.dst)
			continue;
		switch(ip->op){
			case EXPR_SVC:
				switch(ip[1].op){
					case EXPR_DIF0:
						ip->op=EXPR_SVC0;
						(++ip)->dst.dst=NULL;
						r=1;
						break;
					default:
						break;
				}
				break;
			case EXPR_SVCP:
				switch(ip[1].op){
					case EXPR_DIF0:
						ip->op=EXPR_SVCP0;
						(++ip)->dst.dst=NULL;
						r=1;
						break;
					default:
						break;
				}
				break;
			default:
				break;
		}
	}
	optimize_end;
}
static int expr_optimize_once(struct expr *restrict ep){
	int r=0;
	r+=expr_optimize_const(ep);
	r+=expr_optimize_constneg(ep);
	r+=expr_optimize_contneg(ep);
	r+=expr_optimize_strongorder_and_notl(ep);
	r+=expr_optimize_injection(ep);
	r+=expr_optimize_injective_hotfunction(ep);
	r+=expr_optimize_contmul(ep,EXPR_POW);
	r+=expr_optimize_contmul(ep,EXPR_MUL);
	r+=expr_optimize_contmul(ep,EXPR_DIV);
	r+=expr_optimize_contmul(ep,EXPR_MOD);
	r+=expr_optimize_contadd(ep);
	r+=expr_optimize_contsh(ep);
	r+=expr_optimize_contmul(ep,EXPR_NEXT);
	r+=expr_optimize_contmul(ep,EXPR_DIFF);
	r+=expr_optimize_contmul(ep,EXPR_LT);
	r+=expr_optimize_contmul(ep,EXPR_LE);
	r+=expr_optimize_contmul(ep,EXPR_GT);
	r+=expr_optimize_contmul(ep,EXPR_GE);
	r+=expr_optimize_contmul(ep,EXPR_SLE);
	r+=expr_optimize_contmul(ep,EXPR_SGE);
	r+=expr_optimize_contmul(ep,EXPR_SEQ);
	r+=expr_optimize_contmul(ep,EXPR_SNE);
	r+=expr_optimize_contmul(ep,EXPR_EQ);
	r+=expr_optimize_contmul(ep,EXPR_NE);
	r+=expr_optimize_contmul(ep,EXPR_AND);
	r+=expr_optimize_contmul(ep,EXPR_XOR);
	r+=expr_optimize_contmul(ep,EXPR_OR);
	r+=expr_optimize_contmul(ep,EXPR_ANDL);
	r+=expr_optimize_contmul(ep,EXPR_XORL);
	r+=expr_optimize_contmul(ep,EXPR_ORL);

	r+=expr_optimize_zero(ep);
	r+=expr_optimize_copyadd(ep);
	r+=expr_optimize_unused(ep);
	r+=expr_optimize_constexpr(ep);
	r+=expr_optimize_copysrcunused(ep);
	r+=expr_optimize_coc(ep);
	r+=expr_optimize_contcopy(ep);//after coc
	r+=expr_optimize_cac(ep);
	r+=expr_optimize_svc0(ep);
	r+=expr_optimize_copyend(ep);
	debug("optimized %d times",r);
	return r;
}
static int expr_optimize0(struct expr *restrict ep){
	int r,ret=0;
	expr_writeconsts(ep);
again:
	r=expr_optimize_once(ep);
	if(likely(r)){
		addo(ret,r);
		goto again;
	}
	return ret;
}
int expr_optimize(struct expr *restrict ep){
	struct expr_resource *rp;
	double v;
	uint8_t sf;
	int r=expr_optimize0(ep);
	if(expr_isconst(ep)){
		sf=ep->freeable;
		rp=ep->res;
		v=eval(ep,0.0);
		expr_free_keepres(ep);
		init_expr_const(ep,v);
		ep->res=rp;
		ep->freeable=sf;
		addo(r,1);
	}
	return r;
}
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#pragma GCC diagnostic ignored "-Wdangling-pointer"

#define sum (un.s0._sum)
#define from (un.s0._from)
#define to (un.s0._to)
#define step (un.s0._step)
#define y (un.s0._y)
#define neg (un.s0._neg)
#define ap (un.s1._ap)
#define endp (un.s1._endp)
#define epp (un.s1._epp)
#define _max (un.sv.__max)
#define _ev (un.sv.__ev)
#define _args (un.sv.__args)
#define inited (un.s0._inited)

#define EXPR_EVALSTEP(_out) \
		switch(ip->op){\
			case EXPR_COPY:\
				*ip->dst.dst=*ip->un.src;\
				break;\
			case EXPR_INPUT:\
				*ip->dst.dst=input;\
				break;\
			case EXPR_BL:\
				*ip->dst.dst=ip->un.func(*ip->dst.dst);\
				break;\
			case EXPR_CONST:\
				*ip->dst.dst=ip->un.value;\
				break;\
			case EXPR_ADD:\
				*ip->dst.dst+=*ip->un.src;\
				break;\
			case EXPR_SUB:\
				*ip->dst.dst-=*ip->un.src;\
				break;\
			case EXPR_MUL:\
				*ip->dst.dst*=*ip->un.src;\
				break;\
			case EXPR_DIV:\
				*ip->dst.dst/=*ip->un.src;\
				break;\
			case EXPR_MOD:\
				*ip->dst.dst=fmod(*ip->dst.dst,*ip->un.src);\
				break;\
			case EXPR_POW:\
				*ip->dst.dst=pow(*ip->dst.dst,*ip->un.src);\
				break;\
			case EXPR_AND:\
				*ip->dst.dst=and2(*ip->dst.dst,*ip->un.src);\
				break;\
			case EXPR_OR:\
				*ip->dst.dst=or2(*ip->dst.dst,*ip->un.src);\
				break;\
			case EXPR_XOR:\
				*ip->dst.dst=xor2(*ip->dst.dst,*ip->un.src);\
				break;\
			case EXPR_SHL:\
				ip->dst.rdst->exp+=(int64_t)*ip->un.src;\
				break;\
			case EXPR_SHR:\
				ip->dst.rdst->exp-=(int64_t)*ip->un.src;\
				break;\
			case EXPR_NEXT:\
				*ip->dst.idst+=(int64_t)*ip->un.src;\
				break;\
			case EXPR_DIFF:\
				*ip->dst.dst=(double)(*ip->dst.idst-*ip->un.isrc);\
				break;\
			case EXPR_OFF:\
				*ip->dst.idst+=(int64_t)*ip->un.src*(int64_t)sizeof(double);\
				break;\
			case EXPR_NEG:\
				*ip->dst.dst=-*ip->dst.dst;\
				break;\
			case EXPR_NOT:\
				*ip->dst.dst=not(*ip->dst.dst);\
				break;\
			case EXPR_NEX0:\
				*ip->dst.idst=(int64_t)*ip->dst.dst;\
				break;\
			case EXPR_DIF0:\
				*ip->dst.dst=(double)*ip->dst.idst;\
				break;\
			case EXPR_NOTL:\
				*ip->dst.dst=(*ip->dst.dst==0.0)?\
					1.0:\
					0.0;\
				break;\
			case EXPR_TSTL:\
				*ip->dst.dst=(*ip->dst.dst!=0.0)?\
					1.0:\
					0.0;\
				break;\
			case EXPR_GT:\
				*ip->dst.dst=*ip->dst.dst>*ip->un.src?\
					1.0:\
					0.0;\
				break;\
			case EXPR_LT:\
				*ip->dst.dst=*ip->dst.dst<*ip->un.src?\
					1.0:\
					0.0;\
				break;\
			case EXPR_SGE:\
				*ip->dst.dst=*ip->dst.dst>=*ip->un.src?\
					1.0:\
					0.0;\
				break;\
			case EXPR_SLE:\
				*ip->dst.dst=*ip->dst.dst<=*ip->un.src?\
					1.0:\
					0.0;\
				break;\
			case EXPR_GE:\
				*ip->dst.dst=*ip->dst.dst>=*ip->un.src\
				||expr_equal(*ip->dst.dst,*ip->un.src)?\
					1.0:\
					0.0;\
				break;\
			case EXPR_LE:\
				*ip->dst.dst=*ip->dst.dst<=*ip->un.src\
				||expr_equal(*ip->dst.dst,*ip->un.src)?\
					1.0:\
					0.0;\
				break;\
\
			case EXPR_SEQ:\
				*ip->dst.dst=*ip->dst.dst==*ip->un.src?\
					1.0:\
					0.0;\
				break;\
			case EXPR_SNE:\
				*ip->dst.dst=*ip->dst.dst!=*ip->un.src?\
					1.0:\
					0.0;\
				break;\
			case EXPR_EQ:\
				*ip->dst.dst=expr_equal(*ip->dst.dst,*ip->un.src)?\
					1.0:\
					0.0;\
				break;\
			case EXPR_NE:\
				*ip->dst.dst=!expr_equal(*ip->dst.dst,*ip->un.src)?\
					1.0:\
					0.0;\
				break;\
			case EXPR_ANDL:\
				*ip->dst.dst=LOGIC(*ip->dst.dst,*ip->un.src,&&)?\
					1.0:\
					0.0;\
				break;\
			case EXPR_ORL:\
				*ip->dst.dst=LOGIC(*ip->dst.dst,*ip->un.src,||)?\
					1.0:\
					0.0;\
				break;\
			case EXPR_XORL:\
				*ip->dst.dst=LOGIC(*ip->dst.dst,*ip->un.src,^)?\
					1.0:\
					0.0;\
				break;\
			CALSUM_INSWITCH(ip->dst.dst);\
			case EXPR_IF:\
				*ip->dst.dst=\
				eval(ip->un.eb->cond,input)!=0.0?\
				eval(ip->un.eb->body,input):\
				eval(ip->un.eb->value,input);\
				break;\
			case EXPR_WHILE:\
				while(likely(eval(ip->un.eb->cond,input)!=0.0))\
					eval(ip->un.eb->body,input);\
				break;\
			case EXPR_WIF:\
				while(likely(eval(ip->un.hotfunc,input)!=0.0));\
				break;\
			case EXPR_DOW:\
				do\
					eval(ip->un.eb->body,input);\
				while(likely(eval(ip->un.eb->cond,input)!=0.0));\
				break;\
			case EXPR_DO:\
				for(;;)\
					eval(ip->un.hotfunc,input);\
			case EXPR_DON:\
				un.index=(size_t)eval(ip->un.eb->cond,input);\
				while(likely(un.index)){\
					eval(ip->un.eb->body,input);\
					--un.index;\
				}\
				break;\
			case EXPR_ZA:\
				*ip->dst.dst=ip->un.zafunc();\
				break;\
			CALMD_INSWITCH(ip->dst.dst);\
			case EXPR_VMD:\
				*ip->dst.dst=vmdeval_def(ip->un.ev,input);\
				break;\
			case EXPR_DO1:\
				eval(ip->un.hotfunc,input);\
				break;\
			case EXPR_EP:\
				*ip->dst.dst=eval(ip->un.hotfunc,input);\
				break;\
			case EXPR_EVAL:\
				*ip->dst.dst=eval(*ip->un.hotfunc2,*ip->dst.dst);\
				break;\
			case EXPR_HOT:\
				*ip->dst.dst=eval(ip->un.hotfunc,*ip->dst.dst);\
				break;\
			case EXPR_PBL:\
				*ip->dst.dst=(*ip->un.func2)(*ip->dst.dst);\
				break;\
			case EXPR_PZA:\
				*ip->dst.dst=(*ip->un.zafunc2)();\
				break;\
			case EXPR_PMD:\
				ap=ip->un.em->args;\
				endp=ap+ip->un.em->dim;\
				epp=ip->un.em->eps;\
				for(;likely(ap<endp);++ap)\
					*ap=eval(epp++,input);\
				*ip->dst.dst=(*ip->dst.md2)(ip->un.em->args,ip->un.em->dim);\
				break;\
			case EXPR_PMEP:\
				((volatile struct expr *)ep)->ipp=&ip;\
			case EXPR_PME:\
				*ip->dst.dst=(*ip->dst.me2)(ip->un.em->eps,ip->un.em->dim,input);\
				break;\
			case EXPR_READ:\
				*ip->dst.dst=**ip->un.src2;\
				break;\
			case EXPR_WRITE:\
				**ip->un.src2=*ip->dst.dst;\
				break;\
			case EXPR_ALO:\
				un.up=alloca((ssize_t)*ip->dst.dst*ip->un.zu);\
				*ip->dst.dst=un.d;\
				break;\
			case EXPR_SJ:\
				un.j.d=*ip->dst.dst;\
				un.j.p->ipp=&ip;\
				un.j.p->ip=ip;\
				*ip->dst.dst=(double)setjmp(un.j.p->jb);\
				break;\
			case EXPR_LJ:\
				un.j.d=*ip->dst.dst;\
				un.j.p->un.val=*ip->un.src;\
				*un.j.p->ipp=un.j.p->ip;\
				longjmp(un.j.p->jb,(int)un.j.p->un.val);\
			case EXPR_IP:\
				switch(ip->un.zu){\
					case 0:\
						*ip->dst.instaddr2=ip;\
						goto break1;\
					case 1:\
						*ip->dst.instaddr2=ep->data+(ep->size-1);\
						goto break1;\
					case 2:\
						*ip->dst.uaddr2=&ip;\
						goto break1;\
					case 3:\
					       ((volatile struct expr *)ep)->ipp=&ip;\
						*ip->dst.uaddr2=(void *)ep;\
						goto break1;\
					default:\
						__builtin_unreachable();\
				}\
			case EXPR_TO:\
				ip=*ip->dst.instaddr2;\
				goto break2;\
			case EXPR_TO1:\
				ip=*ip->dst.instaddr2;\
				break;\
			CALHMD_INSWITCH(ip->dst.dst);\
			case EXPR_RET:\
				un.r.ep=*ip->un.uaddr2;\
				un.r.end=un.r.ep->data+un.r.ep->size-1;\
				*un.r.end->dst.dst=*ip->dst.dst;\
				*un.r.ep->ipp=un.r.end-1;\
				break;\
			case EXPR_SVC:\
				un.svc.num=(intptr_t)*ip->dst.dst;\
				un.svc.arg=(const intptr_t *)ep->un.args;\
				expr_internal_syscall_eval(*ip->dst.pdst,intptr_t,ip->flag,un.svc.num,un.svc.arg[0],un.svc.arg[1],un.svc.arg[2],un.svc.arg[3],un.svc.arg[4],un.svc.arg[5],un.svc.arg[6]);\
				break;\
			case EXPR_SVC0:\
				un.svc.num=(intptr_t)*ip->dst.dst;\
				un.svc.arg=(const intptr_t *)ep->un.args;\
				expr_internal_syscall_eval(*ip->dst.dst,double,ip->flag,un.svc.num,un.svc.arg[0],un.svc.arg[1],un.svc.arg[2],un.svc.arg[3],un.svc.arg[4],un.svc.arg[5],un.svc.arg[6]);\
				break;\
			case EXPR_SVCP:\
				un.svc.num=(intptr_t)*ip->dst.dst;\
				un.svc.arg=(const intptr_t *)*ip->un.uaddr2;\
				expr_internal_syscall_eval(*ip->dst.pdst,intptr_t,ip->flag,un.svc.num,un.svc.arg[0],un.svc.arg[1],un.svc.arg[2],un.svc.arg[3],un.svc.arg[4],un.svc.arg[5],un.svc.arg[6]);\
				break;\
			case EXPR_SVCP0:\
				un.svc.num=(intptr_t)*ip->dst.dst;\
				un.svc.arg=(const intptr_t *)*ip->un.uaddr2;\
				expr_internal_syscall_eval(*ip->dst.dst,double,ip->flag,un.svc.num,un.svc.arg[0],un.svc.arg[1],un.svc.arg[2],un.svc.arg[3],un.svc.arg[4],un.svc.arg[5],un.svc.arg[6]);\
				break;\
			case EXPR_END:\
				_out;\
			default:\
				__builtin_unreachable();\
		}\
break1:\
		__asm__("":::"memory");\
		++ip;\
break2:
__attribute__((noinline))
double expr_eval(const struct expr *restrict ep,double input){
	EXPR_EVALVARS;
	for(struct expr_inst *ip=ep->data;;){
		EXPR_EVALSTEP(return *ip->dst.dst);
	}
}

__attribute__((noinline))
int expr_step(const struct expr *restrict ep,double input,double *restrict output,struct expr_inst **restrict saveip){
	EXPR_EVALVARS;
	struct expr_inst *ip=*saveip;
	EXPR_EVALSTEP(if(output)*output=*ip->dst.dst;return 1);
	*saveip=ip;
	return 0;
}

__attribute__((noinline))
double expr_callback(const struct expr *restrict ep,double input,const struct expr_callback *ec){
	EXPR_EVALVARS;
	struct expr_inst *old_ip;
	double out;
	for(struct expr_inst *ip=ep->data;;){
		old_ip=ip;
		if(ec->before)
			ec->before(ep,ip,ec->arg);
#undef eval
#define eval(_ep,_input) expr_callback(_ep,_input,ec)
		EXPR_EVALSTEP(out=*ip->dst.dst;goto end);
#undef eval
		if(ec->after)
			ec->after(ep,old_ip,ec->arg);
	}
end:
	if(ec->after)
		ec->after(ep,old_ip,ec->arg);
	return out;
}

#undef sum
#undef from
#undef to
#undef step
#undef y
#undef neg
#undef ap
#undef endp
#undef epp
#undef _max
#undef _ev
#undef _args
#undef inited

#pragma GCC diagnostic pop
