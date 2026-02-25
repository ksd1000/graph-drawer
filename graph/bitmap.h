/*******************************************************************************
 *License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>*
 *This is free software: you are free to change and redistribute it.           *
 *******************************************************************************/
#ifndef _BITMAP_H_
#define _BITMAP_H_
#include <stdint.h>
struct bitmap {
	uint16_t magic;//0x4d42
	uint32_t size;
	uint32_t reserve;//0
	uint32_t pindex;//54
	uint32_t dhsize;//40
	uint32_t width;
	uint32_t height;
	uint16_t ncp;//1
	uint16_t bpp;
	uint32_t zip_type;//0
	uint32_t dsize;//0
	uint32_t wratio;//0
	uint32_t hratio;//0
	uint32_t palette;//0
	uint32_t nic;//0
	char data[];
} __attribute__((packed));
_Static_assert(sizeof(struct bitmap)==54,"error on size of bitmap header");
#if (!defined(__BIG_ENDIAN__)||!(__BIG_ENDIAN__))
#define BM_MAGIC ((uint16_t)('B')|((uint16_t)('M'<<8)))
#else
#define BM_MAGIC ((uint16_t)('M')|((uint16_t)('B'<<8)))
#endif
#define bm_byte_width(_w,_bpp) ((((_w)*(_bpp)+31)>>5)<<2)
#define bm_byte_widthof(__bm) ({const struct bitmap *restrict _bm=(__bm);bm_byte_width(_bm->width,_bm->bpp);})
#define bm_dsize(_w,_h,_bpp) (bm_byte_width((_w),(_bpp))*(_h))
#define bm_size(_w,_h,_bpp) (bm_dsize((_w),(_h),(_bpp))+54)
#define bm_init(__bm,__w,__h,__bpp) (\
{\
	struct bitmap *restrict _bm=(__bm);\
	uint32_t _w=(__w);\
	uint32_t _h=(__h);\
	uint16_t _bpp=(__bpp);\
	uint32_t _sz=bm_size(_w,_h,_bpp);\
	_bm->magic=BM_MAGIC;\
	_bm->size=_sz;\
	_bm->reserve=0;\
	_bm->pindex=54;\
	_bm->dhsize=40;\
	_bm->width=_w;\
	_bm->height=_h;\
	_bm->ncp=1;\
	_bm->bpp=_bpp;\
	_bm->zip_type=0;\
	_bm->dsize=_sz-54;\
	_bm->wratio=0;\
	_bm->hratio=0;\
	_bm->palette=0;\
	_bm->nic=0;\
	(void)0;\
}\
)
#define bm_check(__bm,__sz) (\
{\
	const struct bitmap *restrict _bm=(__bm);\
	size_t _sz=(__sz);\
	int _r=0;\
	if(_sz<54)\
		_r=-1;\
	else if(_bm->magic!=BM_MAGIC)\
		_r=-2;\
	else if(_bm->size!=_sz)\
		_r=-3;\
	else if(_bm->pindex<54||_bm->pindex>=_sz)\
		_r=-4;\
	else {\
		switch(_bm->bpp){\
			case 8:\
			case 16:\
			case 24:\
			case 32:\
				_sz-=_bm->pindex;\
				if(_bm->dsize!=_sz)\
					_r=-6;\
				if(_sz!=bm_byte_width(_bm->width,_bm->bpp)*_bm->height)\
					_r=-7;\
				break;\
			default:\
				_r=-5;\
				break;\
		}\
	}\
	_r;\
}\
)
#define bm_read24(__p) ({const char *_p=(__p);(*(uint16_t *)_p)|((uint32_t)(*(uint8_t *)(_p+2))<<16);})
#define bm_write24(__p,__v) ({char *_p=(__p);uint32_t _v=(__v);*(uint16_t *)_p=(uint16_t)_v;*(uint8_t *)(_p+2)=(uint8_t)(_v>>16);(void)0;})
#define bm_setpixel6(_data,_bypp,_bw,_x,_y,_color) (\
{\
	switch((_bypp)){\
		case 1:\
			*(uint8_t *)((_data)+(_y)*(_bw)+(_x)*(_bypp))=(uint8_t)(_color);\
			break;\
		case 2:\
			*(uint16_t *)((_data)+(_y)*(_bw)+(_x)*(_bypp))=(uint16_t)(_color);\
			break;\
		case 3:\
			bm_write24((_data)+(_y)*(_bw)+(_x)*(_bypp),(_color));\
			break;\
		case 4:\
			*(uint32_t *)((_data)+(_y)*(_bw)+(_x)*(_bypp))=(uint32_t)(_color);\
			break;\
		default:\
			__builtin_unreachable();\
	}\
}\
)
#define bm_setpixel(__bm,_x,_y,_color) (\
{\
	struct bitmap *restrict _bm=(__bm);\
	uint16_t _bypp=_bm->bpp/8;\
	bm_setpixel6((char *)_bm+_bm->pindex,_bypp,bm_byte_width(_bm->width,_bm->bpp),(_x),(_y),(_color));\
}\
)
#define bm_getpixel5(_data,_bypp,_bw,_x,_y) (\
{\
	uint32_t _r0;\
	switch((_bypp)){\
		case 1:\
			_r0=*(uint8_t *)((_data)+(_y)*(_bw)+(_x)*(_bypp));\
			break;\
		case 2:\
			_r0=*(uint16_t *)((_data)+(_y)*(_bw)+(_x)*(_bypp));\
			break;\
		case 3:\
			_r0=bm_read24((_data)+(_y)*(_bw)+(_x)*(_bypp));\
			break;\
		case 4:\
			_r0=*(uint32_t *)((_data)+(_y)*(_bw)+(_x)*(_bypp));\
			break;\
		default:\
			__builtin_unreachable();\
	}\
	_r0;\
}\
)
#define bm_getpixel(__bm,_x,_y) (\
{\
	const struct bitmap *restrict _bm=(__bm);\
	uint16_t _bypp=_bm->bpp/8;\
	bm_getpixel5((char *)_bm+_bm->pindex,_bypp,bm_byte_width(_bm->width,_bm->bpp),(_x),(_y));\
}\
)
#define bm_locatepixel5(_data,_bypp,_bw,_x,_y) ((_data)+(_y)*(_bw)+(_x)*(_bypp))
#define bm_locatepixel(__bm,_x,_y) (\
{\
	const struct bitmap *restrict _bm=(__bm);\
	uint16_t _bypp=_bm->bpp/8;\
	bm_locatepixel5((char *)_bm+_bm->pindex,_bypp,bm_byte_width(_bm->width,_bm->bpp),(_x),(_y));\
}\
)
#endif
