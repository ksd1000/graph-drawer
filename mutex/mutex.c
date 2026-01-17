/*******************************************************************************
 *License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>*
 *This is free software: you are free to change and redistribute it.           *
 *******************************************************************************/
#define _GNU_SOURCE
#include "../expr/expr.h"
#include <sys/syscall.h>
#include <linux/futex.h>
#include "mutex.h"
#define warped_syscall(a0,a1,a2,a3,a4,a5,a6) expr_syscall((long)(a1),(long)(a2),(long)(a3),(long)(a4),(long)(a5),(long)(a6),(a0))
#define fwait(uaddr,val) warped_syscall(SYS_futex,uaddr,FUTEX_WAIT,(val),NULL,NULL,0)
#define fwake(uaddr,val) warped_syscall(SYS_futex,uaddr,FUTEX_WAKE,(val),NULL,NULL,0)
void mutex_wait(mutex_t *lock,uint32_t val){
	fwait(lock,val);
}
void mutex_wake(mutex_t *lock){
	fwake(lock,2);
}
void mutex_wakeall(mutex_t *lock){
	fwake(lock,INT32_MAX);
}
