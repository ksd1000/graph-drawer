/*******************************************************************************
 *License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>*
 *This is free software: you are free to change and redistribute it.           *
 *******************************************************************************/
#define _GNU_SOURCE

#define EXPR_INLIB 1
#include "expr.h"
#include <string.h>

//NOT COMPLETED
#define HBITZ ((size_t)PTRDIFF_MIN)
#define UNIT_SIZE (sizeof(struct expr_areaunit))
#define INIT_COMMON \
	if(unlikely(size<UNIT_SIZE))\
		return -1;\
	area->data=zone;\
	area->data->prev=size;\
	area->data->unsize=size-UNIT_SIZE;\
	area->data->tail=1;\
	area->tail=area->data
int expr_area_init(struct expr_area *restrict area,void *zone,size_t size){
	INIT_COMMON;
	area->flag=0;
	return 0;
}
int expr_area_init4(struct expr_area *restrict area,void *zone,size_t size,int flag){
	INIT_COMMON;
	area->flag=flag;
	return 0;
}
#define zalign(size) (((size)+(UNIT_SIZE-1))/UNIT_SIZE)*UNIT_SIZE
#define zp p.z
#define ip p.i
#define zp1 p1.z
#define ip1 p1.i
#define _zp _p.z
#define _ip _p.i
#define _zp1 _p1.z
#define _ip1 _p1.i
static int findbest(struct expr_area *restrict area,size_t size,uintptr_t *dest,int flag){
	union {
		struct expr_areaunit *z;
		uintptr_t i;
	} p;
	uintptr_t prev=0;
	size_t prev_extra=0;
	zp=area->data;
	if(flag&EXPR_ALAZY){
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
			if(unlikely((flag&EXPR_ALAZY)||size+UNIT_SIZE>zp->size))
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
						if((extra>=UNIT_SIZE&&extra<prev_extra)){
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
#define alloc_internal(_flag) ({\
	union {\
		struct expr_areaunit *z;\
		uintptr_t i;\
	} _p,_p1;\
	uintptr_t _ret;\
	size_t _old;\
	switch(findbest(area,size,&_ip,_flag)){\
		case 0:\
			_old=_zp->unsize;\
			_zp->unsize=size;\
			_zp->tail=0;\
			_ret=_ip+UNIT_SIZE;\
			_ip+=_zp->unsize+UNIT_SIZE;\
			_zp->prev=size;\
			_zp->unsize=_old-(size+UNIT_SIZE);\
			_zp->tail=1;\
			area->tail=_zp;\
			break;\
		case 1:\
			_zp->deallocated=0;\
			_ret=_ip+UNIT_SIZE;\
			size=_zp->unsize;\
			break;\
		case 2:\
			_old=_zp->size;\
			_zp->unsize=size;\
			_ip+=UNIT_SIZE;\
			_ret=_ip;\
			_ip+=size;\
			_zp->prev=size;\
			_old=_old-(size+UNIT_SIZE);\
			_ip1=_ip+_old+UNIT_SIZE;\
			if(_zp1->tail){\
				_old+=_zp1->size+UNIT_SIZE;\
				_zp->tail=1;\
				area->tail=_zp;\
			}else if(_zp1->deallocated){\
				_old+=_zp1->size+UNIT_SIZE;\
				_ip1+=_zp1->size+UNIT_SIZE;\
				_zp1->prev=_old;\
				_zp->deallocated=1;\
				_zp->tail=0;\
			}else {\
				_zp1->prev=_old;\
				_zp->deallocated=1;\
				_zp->tail=0;\
			}\
			_zp->size=_old;\
			break;\
		case 3:\
			return NULL;\
		default:\
			__builtin_unreachable();\
	}\
	(void *)_ret;\
})
#define ALLOC_BODY(_flag) \
	void *ret;\
	if(unlikely(size>PTRDIFF_MAX)){\
		return NULL;\
	}\
	if(!(_flag&EXPR_ANOALIGN))\
		size=zalign(size);\
	ret=alloc_internal(_flag);\
	if(_flag&EXPR_AZERO)\
		memset(ret,0,size);\
	return ret
void *expr_area_alloc(struct expr_area *restrict area,size_t size){
	ALLOC_BODY(area->flag);
}
void *expr_area_alloc3(struct expr_area *restrict area,size_t size,int flag){
	ALLOC_BODY(flag);
}
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
		area->tail=zp;
	}else {
		if(zp1->deallocated){
			zp->size+=zp1->size+UNIT_SIZE;
		}
		ip1=ip+zp->size+UNIT_SIZE;
		zp1->prev=zp->size;
	}
}
#define REALLOC_BODY(_flag) \
	union {\
		struct expr_areaunit *z;\
		uintptr_t i;\
	} p,p1;\
	size_t extra,temp_size,temp_prev;\
	if(unlikely(size>PTRDIFF_MAX)){\
		return NULL;\
	}\
	ip=((uintptr_t)old-UNIT_SIZE);\
	if(!(_flag&EXPR_ANOALIGN))\
		size=zalign(size);\
	if(size<=zp->unsize){\
		extra=zp->unsize-size;\
		if(extra<UNIT_SIZE)\
			return old;\
		extra-=UNIT_SIZE;\
		ip1=ip+zp->unsize+UNIT_SIZE;\
		zp->unsize=size;\
		ip+=size+UNIT_SIZE;\
		zp->prev=size;\
		if(zp1->tail){\
			extra+=zp1->size+UNIT_SIZE;\
			zp->tail=1;\
			area->tail=zp;\
		}else if(zp1->deallocated){\
			extra+=zp1->size+UNIT_SIZE;\
			ip1+=zp1->size+UNIT_SIZE;\
			zp1->prev=extra;\
			zp->deallocated=1;\
			zp->tail=0;\
		}else {\
			zp1->prev=extra;\
			zp->deallocated=1;\
			zp->tail=0;\
		}\
		zp->size=extra;\
		return old;\
	}\
	ip1=ip+zp->unsize+UNIT_SIZE;\
	extra=size-zp->unsize;\
	if(zp1->size>=extra){\
		if(zp1->tail){\
			temp_size=zp1->size-extra;\
			temp_prev=zp1->prev+extra;\
			ip1+=extra;\
			zp1->unsize=temp_size;\
			zp1->unprev=temp_prev|HBITZ;\
			area->tail=zp1;\
			goto old_extend;\
		}else if(zp1->deallocated){\
			temp_size=zp1->size-extra;\
			temp_prev=zp1->prev+extra;\
			ip1+=extra;\
			zp1->unsize=temp_size|HBITZ;\
			zp1->unprev=temp_prev;\
			ip1+=temp_size+UNIT_SIZE;\
			zp1->prev=temp_size;\
			goto old_extend;\
		}\
	}else {\
		temp_prev=zp1->size+UNIT_SIZE;\
		if(temp_prev>=extra&&!zp1->tail&&zp1->deallocated){\
			ip1+=temp_prev;\
			size=zp->unsize+temp_prev;\
			zp1->prev=size;\
			goto old_extend;\
		}\
	}\
	if(_flag&EXPR_ANAIL){\
		return NULL;\
	}\
	ip1=(uintptr_t)alloc_internal(_flag);\
	memcpy((void *)ip1,old,zp->unsize);\
	if(_flag&EXPR_AZERO)\
		memset((void *)(ip1+zp->unsize),0,size-zp->unsize);\
	expr_area_dealloc(area,old);\
	return (void *)ip1;\
old_extend:\
	if(_flag&EXPR_AZERO)\
		memset((void *)((uintptr_t)old+zp->unsize),0,size-zp->unsize);\
	zp->unsize=size;\
	return old
void *expr_area_realloc(struct expr_area *restrict area,void *old,size_t size){
	REALLOC_BODY(area->flag);
}
void *expr_area_realloc4(struct expr_area *restrict area,void *old,size_t size,int flag){
	REALLOC_BODY(flag);
}
