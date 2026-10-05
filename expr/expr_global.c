
/*******************************************************************************
 *License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>*
 *This is free software: you are free to change and redistribute it.           *
 *******************************************************************************/
#define _GNU_SOURCE

#define EXPR_INLIB 1
#include "expr.h"

expr_globals_define();

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif
void expr_contract(void *buf,size_t size){
	volatile char *p=(volatile char *)buf,*endp=(volatile char *)buf+size;
	while(p<endp){
		*p=0;
		p+=PAGE_SIZE;
	}
	*endp=0;
	//treat the case like [0,1,...,endp->4094,p->4095] 
}
__attribute__((noreturn)) void expr_explode_r(void (*contractor)(void *,size_t),const struct expr_memtool *restrict mtl,size_t max){
	void *r;
	do {
		while((r=mtl->allocate(max,mtl->arg))){
			contractor(r,max);
			//if do not contract,
			//the virtual memory
			//has not physical
			//memory and the OOM
			//will not be called.
		}
		max>>=1;
	}while(max);
	abort();
//the abort() is usually unreachable,should
//be killed by kernel before sz reaches 0
}
__attribute__((noreturn)) void expr_explode(void){
	expr_explode_r(expr_contract,expr_defmtl,expr_allocate_max);
}
__attribute__((noreturn)) void expr_trap(void){
	__builtin_trap();
}
__attribute__((noreturn)) void expr_ubehavior(void){
	__builtin_unreachable();
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
	return;
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
	return;
}
void expr_mutex_spinlock(uint32_t *lock){
#ifdef _MUTEX_H_
	mutex_spinlock((mutex_t *)lock);
#endif
	return;
}
void expr_mutex_spinunlock(uint32_t *lock){
#ifdef _MUTEX_H_
	mutex_spinunlock((mutex_t *)lock);
#endif
	return;
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

#ifndef EXPR_SETUP_ENABLED
#define EXPR_SETUP_ENABLED 1
#endif

#if EXPR_SETUP_ENABLED
#include <stdio.h>
#include <string.h>
#define r_fail \
	{\
		fprintf(stderr,"cannot allocate memory,size=%zu\n",size);\
		fprintf(stderr,"ABORTING\n");\
		abort();\
	}
static int nonnull=0,ckleak=0;
int expr_mtl_setup=0;
static uint32_t mutex[1]={0};
ssize_t count=0;
static void *xmalloc_setup(size_t size,void *arg){
	void *r;
	r=size>expr_allocate_max?NULL:malloc(size);
	if(unlikely(!r&&nonnull))
		r_fail;
	if(likely(r)){
		expr_mutex_lock(mutex);
		++count;
		expr_mutex_unlock(mutex);
	}
	return r;
}
static void *xrealloc_setup(void *old,size_t size,void *arg){
	void *r;
	r=size>expr_allocate_max?NULL:realloc(old,size);
	if(unlikely(!r&&nonnull))
		r_fail;
	if(unlikely(r&&!old)){
		expr_mutex_lock(mutex);
		++count;
		expr_mutex_unlock(mutex);
	}
	return r;
}
static void xfree_setup(void *old,void *arg){
	free(old);
	expr_mutex_lock(mutex);
	--count;
	expr_mutex_unlock(mutex);
}
static const struct expr_memtool expr_setupmtl[1]={{
	.allocate=xmalloc_setup,
	.reallocate=xrealloc_setup,
	.deallocate=xfree_setup,
	.arg=NULL,
}};
static struct expr_memtool expr_oldmtl[1];
int expr_setup_mtl(int flag){
	if(expr_mtl_setup){
		memcpy(expr_defmtl,expr_oldmtl,sizeof(struct expr_memtool));
		expr_mtl_setup=0;
		if(unlikely(ckleak&&count)){
			fprintf(stderr,"DETECTED MEMORY LEAK IN %zd OBJECTS\n",count);
			fprintf(stderr,"ABORTING\n");
			abort();
		}
		return 1;
	}else {
		nonnull=!!(flag&1);
		ckleak=!!(flag&2);
		memcpy(expr_oldmtl,expr_defmtl,sizeof(struct expr_memtool));
		memcpy(expr_defmtl,expr_setupmtl,sizeof(struct expr_memtool));
		expr_mtl_setup=1;
		return 0;
	}
}

#endif
