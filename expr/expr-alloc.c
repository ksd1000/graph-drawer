/*******************************************************************************
 *License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>*
 *This is free software: you are free to change and redistribute it.           *
 *******************************************************************************/
#define _GNU_SOURCE

#define EXPR_INLIB 1
#include "expr.h"
#include <string.h>

#define HBITZ ((size_t)PTRDIFF_MIN)
#define UNIT_SIZE (sizeof(struct expr_areaunit))
#define zero_from_field(buf,type,field) memset((void *)((uintptr_t)(buf)+offsetof(type,field)),0,sizeof(type)-offsetof(type,field))
#define zero_fromto_field(buf,type,field,endfield) memset((void *)((uintptr_t)(buf)+offsetof(type,field)),0,offsetof(type,endfield)-offsetof(type,field))
#define align_size(val,const_size) (((val)+((const_size)-1))/(const_size))*(const_size)
#define zalign(size,_flag) ((!((_flag)&EXPR_ADYNAMICALIGN))?(((size)+(UNIT_SIZE-1))/UNIT_SIZE)*UNIT_SIZE:({\
	unsigned int _r=(unsigned int)(_flag)>>EXPR_AALIGN_SHIFT;\
	(((size)+(_r-1))/_r)*_r;\
}))
#define INIT_COMMON(_flag) \
	size_t sizemu;\
	if((_flag)&EXPR_ASTARTADDRALIGN){\
		uintptr_t alp;\
		size_t dif;\
		if(unlikely(size>(size_t)PTRDIFF_MAX)){\
			return -1;\
		}\
		alp=zalign((uintptr_t)zone,_flag);\
		dif=alp-(uintptr_t)zone;\
		if(dif){\
			size-=dif;\
			zone=(void *)alp;\
		}\
	}\
	sizemu=size-UNIT_SIZE;\
	if(unlikely(sizemu>((size_t)PTRDIFF_MAX-UNIT_SIZE)))\
		return -1;\
	area->data=zone;\
	area->data->unprev=size|HBITZ;\
	area->data->unsize=sizemu;\
	area->tail=area->data;\
	zero_fromto_field(area,struct expr_area,monotonic_allocate,flag);\
	area->flag=(_flag)
int expr_area_init(struct expr_area *restrict area,void *zone,size_t size){
	INIT_COMMON(EXPR_ALAZY_ALL|EXPR_ASTARTADDRALIGN);
	return 0;
}
int expr_area_init4(struct expr_area *restrict area,void *zone,size_t size,int flag){
	INIT_COMMON(flag);
	return 0;
}
#define WIPE_COMMON \
	area->data->unsize=area->data->prev-UNIT_SIZE;\
	area->data->tail=1;\
	area->tail=area->data
void expr_area_wipe(struct expr_area *restrict area){
	WIPE_COMMON;
}
void expr_area_wipe_monotonic(struct expr_area *restrict area){
	WIPE_COMMON;
	zero_fromto_field(area,struct expr_area,monotonic_allocate,flag);
}
int expr_area_resize(struct expr_area *restrict area,size_t size){
	size_t old_size;
	if(unlikely(size>(size_t)PTRDIFF_MAX)){
		return -1;
	}
	old_size=area->data->prev;
	if(size>=old_size){
		area->data->prev=size;
		area->tail->unsize+=size-old_size;
		return 0;
	}
	old_size=area->tail->size-(old_size-size);
	if(unlikely((ssize_t)old_size<0)){
		return -1;
	}
	area->data->prev=size;
	area->tail->unsize=old_size;
	return 0;
}
#define monoinc(__type) {if(area->monotonic_##__type!=UINT_MAX)++area->monotonic_##__type;}

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
#define findbest(dest,_flag,_type) {\
	union {\
		struct expr_areaunit *z;\
		uintptr_t i;\
	} __p;\
	uintptr_t __prev=0;\
	size_t __prev_extra=0,__extra;\
	__zp=area->data;\
	if((_flag)&EXPR_ALAZY){\
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
			if(unlikely(((_flag)&EXPR_ALAZY)||size+UNIT_SIZE>__zp->size)){\
				monoinc(_type##_fail);\
				return NULL;\
			}\
			dest=__ip;\
			goto case_0;\
		}\
		if(__zp->deallocated){\
			_old=__zp->size;\
			if(size<=_old){\
				if((_flag)&EXPR_ALAZYEX){\
					__extra=_old-size;\
					dest=__ip;\
					if(__extra<UNIT_SIZE)\
						goto case_1;\
					else\
						goto case_2;\
				}\
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
#define alloc_internal(_flag,_type) ({\
	union {\
		struct expr_areaunit *z;\
		uintptr_t i;\
	} _p,_p1;\
	uintptr_t _ret;\
	size_t _old;\
	findbest(_ip,_flag,_type){\
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
			monoinc(retail_up);\
			monoinc(create);\
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
			_zp1->prev=_old;\
			_zp->deallocated=1;\
			_zp->tail=0;\
			_zp->size=_old;\
			monoinc(create);\
			goto end;\
	}\
end:\
	(void *)_ret;\
})
#define ALLOC_BODY(_flag) \
	void *ret;\
	if(unlikely(size>(size_t)PTRDIFF_MAX)){\
		monoinc(allocate_fail);\
		return NULL;\
	}\
	if(!((_flag)&EXPR_ANOALIGN))\
		size=zalign(size,_flag);\
	ret=alloc_internal(_flag,allocate);\
	monoinc(allocate);\
	if((_flag)&EXPR_AZERO)\
		memset(ret,0,size);\
	return ret
void *expr_area_malloc(struct expr_area *restrict area,size_t size){
	ALLOC_BODY(area->flag);
}
void *expr_area_malloc3(struct expr_area *restrict area,size_t size,int flag){
	ALLOC_BODY(flag);
}
#define DEALLOC_BODY(_old) \
	ip=((uintptr_t)(_old)-UNIT_SIZE);\
	if(zp!=area->data){\
		ip1=ip-zp->prev-UNIT_SIZE;\
		if(zp1->deallocated){\
			zp1->size+=zp->size+UNIT_SIZE;\
			zp=zp1;\
			monoinc(combine);\
		}else\
			zp->deallocated=1;\
	}else\
			zp->deallocated=1;\
	ip1=ip+zp->size+UNIT_SIZE;\
	if(zp1->tail){\
		zp->size+=zp1->size+UNIT_SIZE;\
		zp->tail=1;\
		area->tail=zp;\
		monoinc(retail_down);\
		monoinc(combine);\
	}else {\
		if(zp1->deallocated){\
			zp->size+=zp1->size+UNIT_SIZE;\
			monoinc(combine);\
		}\
		ip1=ip+zp->size+UNIT_SIZE;\
		zp1->prev=zp->size;\
	}
void expr_area_dealloc(struct expr_area *restrict area,void *old){
	union {
		struct expr_areaunit *z;
		uintptr_t i;
	} p,p1;
	DEALLOC_BODY(old);
	monoinc(deallocate);
}
#define extra un._extra
#define new un._new
#define REALLOC_BODY(_flag) \
	union {\
		struct expr_areaunit *z;\
		uintptr_t i;\
	} p,p1;\
	union {\
		void *_new;\
		size_t _extra;\
	} un;\
	size_t temp_size,temp_prev;\
	if(unlikely(size>(size_t)PTRDIFF_MAX)){\
		monoinc(expand_fail);\
		return NULL;\
	}\
	ip=((uintptr_t)old-UNIT_SIZE);\
	if(!((_flag)&EXPR_ANOALIGN))\
		size=zalign(size,_flag);\
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
			monoinc(retail_down);\
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
			monoinc(create);\
		}\
		zp->size=extra;\
		monoinc(shrink);\
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
			monoinc(retail_up);\
			goto old_expand;\
		}else if(zp1->deallocated){\
			temp_size=zp1->size-extra;\
			temp_prev=zp1->prev+extra;\
			ip1+=extra;\
			zp1->unsize=temp_size|HBITZ;\
			zp1->unprev=temp_prev;\
			ip1+=temp_size+UNIT_SIZE;\
			zp1->prev=temp_size;\
			goto old_expand;\
		}\
	}else {\
		temp_prev=zp1->size+UNIT_SIZE;\
		if(temp_prev>=extra&&!zp1->tail&&zp1->deallocated){\
			ip1+=temp_prev;\
			size=zp->unsize+temp_prev;\
			zp1->prev=size;\
			monoinc(combine);\
			goto old_expand;\
		}\
	}\
	if((_flag)&EXPR_ANAIL){\
		monoinc(expand_fail);\
		return NULL;\
	}\
	new=alloc_internal((_flag),expand);\
	memcpy(new,old,zp->unsize);\
	if((_flag)&EXPR_AZERO)\
		memset((void *)((uintptr_t)new+zp->unsize),0,size-zp->unsize);\
	{\
		DEALLOC_BODY(old)\
	}\
	monoinc(expand_move);\
	return new;\
old_expand:\
	if((_flag)&EXPR_AZERO)\
		memset((void *)((uintptr_t)old+zp->unsize),0,size-zp->unsize);\
	zp->unsize=size;\
	monoinc(expand_nail);\
	return old
void *expr_area_realloc(struct expr_area *restrict area,void *old,size_t size){
	REALLOC_BODY(area->flag);
}
void *expr_area_realloc4(struct expr_area *restrict area,void *old,size_t size,int flag){
	REALLOC_BODY(flag);
}
void *expr_area_calloc(struct expr_area *restrict area,size_t size){
	return expr_area_malloc3(area,size,area->flag|EXPR_AZERO);
}
int expr_area_summary(const struct expr_area *restrict area,struct expr_areainfo *restrict info){
	union {
		struct expr_areaunit *z;
		uintptr_t i;
	} p;
	struct expr_areaunit *prev=NULL;
	zp=area->data;
	info->size=zp->prev;
	zero_from_field(info,struct expr_areainfo,unit_count);
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
static void *static_area_malloc(size_t size,intptr_t arg){
	return expr_area_malloc((struct expr_area *)arg,size);
}
static void *static_area_realloc(void *old,size_t size,intptr_t arg){
	return expr_area_realloc((struct expr_area *)arg,old,size);
}
static void static_area_dealloc(void *old,intptr_t arg){
	expr_area_dealloc((struct expr_area *)arg,old);

}
int expr_setup_heapmtl(struct expr_memtool *restrict mtl,void *heap,size_t size,int flag){
	if(flag&EXPR_ASTARTADDRALIGN){
		uintptr_t alp;
		size_t dif;
		alp=align_size((uintptr_t)heap,sizeof(struct expr_area));
		dif=alp-(uintptr_t)heap;
		if(dif){
			size-=dif;
			heap=(void *)alp;
		}
	}
	if(unlikely((ssize_t)size<(ssize_t)sizeof(struct expr_area)))
		return -1;
	if(unlikely(expr_area_init4((struct expr_area *)heap,expr_zoneof(heap),size-sizeof(struct expr_area),flag))<0)
		return -1;
	mtl->allocate=static_area_malloc;
	mtl->reallocate=static_area_realloc;
	mtl->deallocate=static_area_dealloc;
	mtl->test=NULL;
	mtl->arg=(intptr_t)heap;
	return 0;
}
int expr_setup_heapmtl3(struct expr_memtool *restrict mtl,void *heap,size_t size){
	return expr_setup_heapmtl(mtl,heap,size,EXPR_ALAZY_ALL|EXPR_ASTARTADDRALIGN);
}
