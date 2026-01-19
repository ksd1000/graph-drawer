
/*******************************************************************************
 *License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>*
 *This is free software: you are free to change and redistribute it.           *
 *******************************************************************************/
#ifndef _MUTEX_H_
#define _MUTEX_H_
#include <stdatomic.h>
#include <stdint.h>
#include <sys/syscall.h>
#include <linux/futex.h>
#include "expr.h"
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

#define mutex_wait(lock,val) expr_syscall((long)(lock),FUTEX_WAIT,(val),0,0,0,SYS_futex)
#define mutex_wake(lock,val) expr_syscall((long)(lock),FUTEX_WAKE,(val),0,0,0,SYS_futex)

#endif
