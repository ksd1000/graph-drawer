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
#define __zp __p.z
#define __ip __p.i
#define findbest(dest,_flag) {\
	union {\
		struct expr_areaunit *z;\
		uintptr_t i;\
	} __p;\
	uintptr_t __prev=0;\
	size_t __prev_extra=0,__extra;\
	__zp=area->data;\
	if(_flag&EXPR_ALAZY){\
		if(likely(size+UNIT_SIZE<=area->tail->size)){\
			dest=(uintptr_t)area->tail;\
			goto case_0;\
		}\
	}\
	for(;;){\
		if(__zp->tail){\
			if(__prev){\
				dest=__prev;\
				if(__prev_extra<UNIT_SIZE)\
					goto case_1;\
				else\
					goto case_2;\
			}\
			if(unlikely((_flag&EXPR_ALAZY)||size+UNIT_SIZE>__zp->size))\
				return NULL;\
			dest=__ip;\
			goto case_0;\
		}\
		if(__zp->deallocated){\
			_old=__zp->size;\
			if(size<=_old){\
				if(__prev){\
					if(__prev_extra>=UNIT_SIZE){\
						__extra=_old-size;\
						if((__extra>=UNIT_SIZE&&__extra<__prev_extra)){\
							__prev=__ip;\
							__prev_extra=__extra;\
						}\
					}\
				}else {\
					__prev=__ip;\
					__prev_extra=_old-size;\
				}\
			}\
		}\
		__ip+=__zp->size+UNIT_SIZE;\
	}\
}
#define alloc_internal(_flag) ({\
	union {\
		struct expr_areaunit *z;\
		uintptr_t i;\
	} _p,_p1;\
	uintptr_t _ret;\
	size_t _old;\
	findbest(_ip,_flag){\
case_0:\
			_old=_zp->unsize;\
			_zp->unsize=size;\
			_zp->tail=0;\
			_ret=_ip+UNIT_SIZE;\
			_ip+=_zp->unsize+UNIT_SIZE;\
			_zp->prev=size;\
			_zp->unsize=_old-(size+UNIT_SIZE);\
			_zp->tail=1;\
			area->tail=_zp;\
			goto end;\
case_1:\
			_zp->deallocated=0;\
			_ret=_ip+UNIT_SIZE;\
			size=_zp->unsize;\
			goto end;\
case_2:\
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
			goto end;\
	}\
end:\
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
int expr_area_summary(const struct expr_area *restrict area,struct expr_areainfo *restrict info){
	union {
		struct expr_areaunit *z;
		uintptr_t i;
	} p;
	struct expr_areaunit *prev=NULL;
	zp=area->data;
	info->size=zp->prev;
	memset((void *)((uintptr_t)info+offsetof(struct expr_areainfo,unit_count)),0,sizeof(struct expr_areainfo)-offsetof(struct expr_areainfo,unit_count));
	for(;;){
		if(prev){
			if(unlikely(prev->size!=zp->prev))
				return -1;
			if(unlikely(prev->deallocated&&!zp->tail&&zp->deallocated))
				return -2;
		}
		if(unlikely(zp->tail)){
			break;
		}
		++info->unit_count;
		if(zp->deallocated){
			info->free+=zp->size;
			if(zp->size>info->free_max)
				info->free_max=zp->size;
			++info->free_count;
		}else {
			info->leak+=zp->size;
			if(zp->size>info->leak_max)
				info->leak_max=zp->size;
			++info->leak_count;
		}
		prev=zp;
		ip+=zp->size+UNIT_SIZE;
	}
	info->max=(info->leak_max>info->free_max?info->leak_max:info->free_max);
	info->tail_index=ip-(uintptr_t)area->data;
	info->tail_size=zp->size;
	if(unlikely(info->tail_size+info->tail_index+UNIT_SIZE!=info->size))
		return -3;
	if(unlikely(zp!=area->tail))
		return -4;
	++info->unit_count;
	return 0;
}
