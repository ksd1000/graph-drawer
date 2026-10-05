/*******************************************************************************
 *License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>*
 *This is free software: you are free to change and redistribute it.           *
 *******************************************************************************/
#define _GNU_SOURCE

#define EXPR_INLIB 1
#include "expr.h"

//NOT COMPLETED

struct expr_areaunit {
	size_t prev:sizeof(size_t)*8-1,tail:1;
	union {
		struct {
			size_t size:sizeof(size_t)*8-1,deallocated:1;
		};
		size_t unsize;
	};
};
struct expr_area {
	struct expr_areaunit *data;
	struct expr_areaunit *tail;
	int flag,unused;
};
#define UNIT_SIZE (sizeof(struct expr_areaunit))
#define EXPR_AZERO 1
#define EXPR_ANOALIGN 2
#define EXPR_ALAZY 4
int expr_area_setup(struct expr_area *restrict area,void *zone,size_t size,int flag){
	if(unlikely(size<UNIT_SIZE))
		return -1;
	area->data=zone;
	area->data->prev=size;
	area->data->size=size-UNIT_SIZE;
	area->data->tail=1;
	area->tail=area->data;
	area->flag=flag;
	return 0;
}
#define zalign(size) (((size)+(UNIT_SIZE-1))/UNIT_SIZE)*UNIT_SIZE
#define zp p.z
#define ip p.i
static int findbest(struct expr_area *restrict area,size_t size,uintptr_t *dest){
	union {
		struct expr_areaunit *z;
		uintptr_t i;
	} p;
	uintptr_t prev=0;
	size_t prev_extra=0;
	zp=area->data;
	if(area->flag&EXPR_ALAZY){
		if(likely(size+UNIT_SIZE<=area->tail->size)){
			*dest=(uintptr_t)area->tail;
			return 0;
		}
	}
	for(;;){
		if(zp->tail){
			if(prev){
				*dest=prev;
				return prev_extra<UNIT_SIZE?1:2;
			}
			if(unlikely((area->flag&EXPR_ALAZY)||size+UNIT_SIZE>zp->size))
				return 3;
			*dest=ip;
			return 0;
		}
		if(zp->deallocated){
			size_t old,extra;
			old=zp->size;
			if(size<=old){
				if(prev){
					if(prev_extra>=UNIT_SIZE){
						extra=old-size;
						if((extra<UNIT_SIZE||extra>prev_extra)){
							prev=ip;
							prev_extra=extra;
						}
					}
				}else {
					prev=ip;
					prev_extra=old-size;
				}
			}
		}
		ip+=zp->size+UNIT_SIZE;
	}
}
void *expr_area_alloc(struct expr_area *restrict area,size_t size){
	union {
		struct expr_areaunit *z;
		uintptr_t i;
	} p;
	uintptr_t ret;
	size_t old;
	if(!(area->flag&EXPR_ANOALIGN))
		size=zalign(size);
	switch(findbest(area,size,&ip)){
		case 0:
			old=zp->unsize;
			zp->unsize=size;
			zp->tail=0;
			ret=ip+UNIT_SIZE;
			ip+=zp->unsize+UNIT_SIZE;
			zp->prev=size;
			zp->unsize=old-(size+UNIT_SIZE);
			zp->tail=1;
			area->tail=zp;
			break;
		case 1:
			zp->deallocated=0;
			ret=ip+UNIT_SIZE;
			break;
		case 2:
			old=zp->size;
			zp->unsize=size;
			zp->deallocated=1;
			ip+=UNIT_SIZE;
			ret=ip;
			ip+=size;
			zp->prev=size;
			old=old-(size+UNIT_SIZE);
			zp->unsize=old;
			ip+=old+UNIT_SIZE;
			zp->prev=old;
			break;
		case 3:
			return NULL;
	}
	if(area->flag&EXPR_AZERO)
		memset((void *)ret,0,size);
	return (void *)ret;
}
#define zp1 p1.z
#define ip1 p1.i
void expr_area_dealloc(struct expr_area *restrict area,void *old){
	union {
		struct expr_areaunit *z;
		uintptr_t i;
	} p,p1;
	ip=((uintptr_t)old-UNIT_SIZE);
	if(zp!=area->data){
		ip1=ip-zp->prev-UNIT_SIZE;
		if(zp1->deallocated){
			zp1->size+=zp->size+UNIT_SIZE;
			zp=zp1;
		}else
			zp->deallocated=1;
	}else
			zp->deallocated=1;
	ip1=ip+zp->size+UNIT_SIZE;
	if(zp1->tail){
		zp->size+=zp1->size+UNIT_SIZE;
		zp->tail=1;
	}else {
		if(zp1->deallocated){
			zp->size+=zp1->size+UNIT_SIZE;
		}
		ip1=ip+zp->size+UNIT_SIZE;
		zp1->prev=zp->size;
	}
}
void *expr_area_realloc(struct expr_area *restrict area,void *old,size_t size){
	union {
		struct expr_areaunit *z;
		uintptr_t i;
	} p,p1;
	ip=((uintptr_t)old-UNIT_SIZE);
	size_t extra;
	if(!(area->flag&EXPR_ANOALIGN))
		size=zalign(size);
	if(size<=zp->size){
		extra=zp->size-size;
		if(extra<UNIT_SIZE)
			return old;
		ip1=ip+size+UNIT_SIZE;
		zp1->deallocated=1;
		zp1->prev=size;
		extra-=UNIT_SIZE;
		zp1->size=extra;
		ip1=ip+zp->size+UNIT_SIZE;
		zp1->prev=extra;
		zp->size=size;
		return old;
	}
	return NULL;//sleepy,keep this temp-ly
}
char buf[100000+200];
struct expr_area ea[1];
void *try(size_t n){
	void *r;
	printf("alloc(%zu)=%p\n",n,r=expr_area_alloc(ea,n));
	return r;
}
void *tryr(void *s,size_t n){
	void *r;
	printf("realloc(%zu)=%p\n",n,r=expr_area_realloc(ea,s,n));
	return r;
}
int main(void){
	void *p;
	printf("%d\n",expr_area_setup(ea,buf,sizeof(buf),EXPR_ALAZY));
	try(10000);
	try(10000);
	try(10000);
	try(10000);
	try(10000);
	try(10000);
	try(10000);
	p=try(10000);
	tryr(p,5000);
	try(10000);
	try(10000);
	try(10000);
	expr_area_dealloc(ea,p);
}
