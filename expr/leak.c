
/*******************************************************************************
 *License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>*
 *This is free software: you are free to change and redistribute it.           *
 *******************************************************************************/
#define _GNU_SOURCE
#include "expr.h"
#include <stdlib.h>
#include <err.h>
ssize_t allocated=0;
ssize_t freed=0;
int allow_allocator_return_null=0;
uint32_t mutex[1]={0};
int lactive=1,lreport=0;
#define lwarn if(lreport)warnx
static void *lmalloc(size_t size){
	void *r;
	r=malloc(size);
	if(!lactive)
		return r;
	lwarn("malloc(%zu)=%p\n",size,r);
	if(!allow_allocator_return_null&&!r){
		warn("IN malloc(size=%zu)\n"
			"CANNOT ALLOCATE MEMORY",size);
		warnx("ABORTING");
		abort();
	}
	if(r){
		expr_mutex_lock(mutex);
		++allocated;
		expr_mutex_unlock(mutex);
	}
	return r;
}
static void *lrealloc(void *old,size_t size){
	void *r;
	r=realloc(old,size);
	if(!lactive)
		return r;
	lwarn("realloc(%p,%zu)=%p\n",old,size,r);
	if(!allow_allocator_return_null&&!r){
		warn("IN realloc(old=%p,size=%zu)\n"
			"CANNOT REALLOCATE MEMORY",old,size);
		warnx("ABORTING");
		abort();
	}
	if(r&&!old){
		expr_mutex_lock(mutex);
		++allocated;
		expr_mutex_unlock(mutex);
	}
	return r;
}
void lfree(void *p){
	if(!lactive)
		return;
	lwarn("free(%p)\n",p);
	free(p);
	expr_mutex_lock(mutex);
	++freed;
	expr_mutex_unlock(mutex);
}
static void __attribute__((constructor)) lstart(void){
	expr_allocator=lmalloc;
	expr_reallocator=lrealloc;
	expr_deallocator=lfree;
}
static void __attribute__((destructor)) lend(void){
	expr_mutex_lock(mutex);
	if(freed!=allocated){
		warnx("DETECTED MEMORY LEAK IN %zd OBJECTS",allocated-freed);
		warnx("ABORTING");
		abort();
	}
	expr_mutex_unlock(mutex);
}
const char *__asan_default_options(void){
	return "allocator_may_return_null=1";
}
