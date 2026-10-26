/*******************************************************************************
 *License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>*
 *This is free software: you are free to change and redistribute it.           *
 *******************************************************************************/

/*
 * History: 2026-10-26
 * Memory requester for libexpr
 * Dependencies: expr-internal.c expr-alloc.c
 */

#define _GNU_SOURCE

#if (defined(EXPR_LIBC_STRING_IMPL)&&(EXPR_LIBC_STRING_IMPL))
#include <string.h>
#endif
#define EXPR_INLIB 1
#include "expr.h"

#define UNIT_SIZE (sizeof(struct expr_zoneunit))
#define align_size(val,const_size) ((((val)+((const_size)-1))/(const_size))*(const_size))

#if !(EXPR_AREA_FORCEALIGN)
#define zalign(size,_flag) ((!((_flag)&EXPR_ADYNAMICALIGN))?align_size(size,UNIT_SIZE):({\
	unsigned int _r=(unsigned int)(_flag)>>EXPR_AALIGN_SHIFT;\
	(((size)+(_r-1))/_r)*_r;\
}))
#else
#define zalign(size,_flag) align_size(size,UNIT_SIZE)
#endif


static void pa_delete(struct expr_pool *restrict pool,struct expr_poolarea *restrict pa){
	if(pa==pool->head){
		pool->head=pa->next;
	}
	if(pa==pool->tail){
		pool->tail=pa->prev;
	}
	if(pa->prev){
		pa->prev->next=pa->next;
	}
	if(pa->next){
		pa->next->prev=pa->prev;
	}
	pool->sys_deallocate(pa,pool->sys_arg);

}
static void *pa_create(struct expr_pool *restrict pool,size_t size){
	size_t req;
	struct expr_poolarea *pa;
	void *r;
	req=offsetof(struct expr_poolarea,zone)+zalign(size,pool->flag)+UNIT_SIZE*3-1;
	// 2*U+(U-1)
	req=(size_t)1<<(sizeof(size_t)*8-(size_t)clzz(req));
	if(req<pool->allocate_size)
		req=pool->allocate_size;
	pa=pool->sys_allocate(req,pool->sys_arg);
	if(unlikely(!pa))
		return NULL;
	if(unlikely(expr_area_init4(pa->area,pa->zone,req-offsetof(struct expr_poolarea,zone),pool->flag)<0))
		goto err;
	r=expr_area_malloc3(pa->area,size,pool->flag);
	if(unlikely(!r))
		goto err;
	pa->next=pool->head;
	pa->prev=NULL;
	if(pool->head)
		pool->head->prev=pa;
	else
		pool->tail=pa;
	pool->head=pa;
	return r;
err:
	pool->sys_deallocate(pa,pool->sys_arg);
	return NULL;
}
static void *pool_tryalloc(struct expr_pool *restrict pool,size_t size){
	struct expr_poolarea *pa=pool->head;
	void *r;
	while(pa){
		r=expr_area_malloc3(pa->area,size,pool->flag);
		if(r)
			return r;
		pa=pa->next;
	}
	return NULL;

}
static struct expr_poolarea *pool_areaof(struct expr_pool *restrict pool,void *p){
	struct expr_poolarea *pa=pool->head;
	for(;;){
		if(((uintptr_t)p-(uintptr_t)pa->area->data)<pa->area->data->prev)
			return pa;
		pa=pa->next;
	}
	__builtin_unreachable();

}
void *expr_pool_malloc(struct expr_pool *restrict pool,size_t size){
	void *r;
	r=pool_tryalloc(pool,size);
	if(r)
		return r;
	return pa_create(pool,size);
}
void expr_pool_dealloc(struct expr_pool *restrict pool,void *old){
	// the caller must ensure old is from pool.
	struct expr_poolarea *pa=pool_areaof(pool,old);
	expr_area_dealloc(pa->area,old);
	if(pa->area->tail==pa->area->data)
		pa_delete(pool,pa);
}
void *expr_pool_realloc(struct expr_pool *restrict pool,void *old,size_t size){
	// the caller must ensure old is from pool.
	struct expr_poolarea *pa=pool_areaof(pool,old);
	void *r;
	size_t oldsize;
	r=expr_area_realloc(pa->area,old,size);
	if(r)
		return r;
	r=expr_pool_malloc(pool,size);
	if(!unlikely(r))
		return NULL;
	oldsize=expr_memlen(old);
	memcpy(r,old,oldsize);
#if !(EXPR_AREA_IGNORE_AZERO)
	if((pool->flag)&EXPR_AZERO)
		memset((void *)((uintptr_t)r+oldsize),0,expr_memlen(r)-oldsize);
#endif
	expr_area_dealloc(pa->area,old);
	if(pa->area->tail==pa->area->data)
		pa_delete(pool,pa);
	return r;
}
void expr_pool_wipe(struct expr_pool *restrict pool){
	struct expr_poolarea *pa=pool->head,*pa1;
	while(pa){
		pa1=pa->next;
		pool->sys_deallocate(pa,pool->sys_arg);
		pa=pa1;
	}
	pool->head=NULL;
	pool->tail=NULL;
	// pool is provided by the caller, libexpr will not free it or modify its other fields.
}
void *expr_poolmtl_malloc(size_t size,intptr_t arg){
	return expr_pool_malloc((struct expr_pool *)arg,size);
}
void *expr_poolmtl_realloc(void *old,size_t size,intptr_t arg){
	return expr_pool_realloc((struct expr_pool *)arg,old,size);
}
void expr_poolmtl_dealloc(void *old,intptr_t arg){
	expr_pool_dealloc((struct expr_pool *)arg,old);

}
void expr_setup_poolmtl(struct expr_memtool *restrict mtl,struct expr_pool *pool){
	mtl->allocate=expr_poolmtl_malloc;
	mtl->reallocate=expr_poolmtl_realloc;
	mtl->deallocate=expr_poolmtl_dealloc;
	mtl->test=NULL;
	mtl->arg=(intptr_t)pool;
}

