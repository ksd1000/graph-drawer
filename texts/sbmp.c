/*******************************************************************************
 *License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>*
 *This is free software: you are free to change and redistribute it.           *
 *******************************************************************************/
#define _GNU_SOURCE
#include <stdlib.h>
#include <assert.h>
#include "sbmp.h"
struct sbmp *sbmp_compress(const struct sbmp *sp){
	struct sbmp *nsp;
	char *p;
	uint64_t index,total,last;
	int currentval;
	if(sp->compressed||!(nsp=malloc(
		(sp->size<<5)+sizeof(uint64_t)+sizeof(struct sbmp)
	)))return NULL;
	nsp->width=sp->width;
	nsp->height=sp->height;
	nsp->c=sp->c;
	total=sp->width*sp->height;
	currentval=nsp->startval=*sp->data&1;
	last=0;
	p=nsp->data;
	for(index=1;index<=total;++index){
		if(!!SBMP_TSTPIXEL(sp,index)==currentval
			&&index<total)
			continue;
		currentval^=1;
		if(index-last<=(UINT8_MAX>>2)){
			*(uint8_t *)p=((index-last)<<2);
			p+=sizeof(uint8_t);
		}else if(index-last<=(UINT16_MAX>>2)){
			*(uint16_t *)p=((index-last)<<2)|1U;
			p+=sizeof(uint16_t);
		}else if(index-last<=(UINT32_MAX>>2)){
			*(uint32_t *)p=((index-last)<<2)|2U;
			p+=sizeof(uint32_t);
		}else {
			*(uint64_t *)p=((index-last)<<2)|3UL;
			p+=sizeof(uint64_t);
		}
		last=index;

	}
	nsp->size=p-nsp->data+sizeof(struct sbmp)+sizeof(uint64_t);
	nsp->compressed=1;
	assert(nsp=realloc(nsp,nsp->size));
	return nsp;
}
int sbmp_decompress(const struct sbmp *sp,struct sbmp *out){
	const char *p=sp->data,*end=sp->data+sp->size-sizeof(uint64_t);
	uint64_t index,last,v,va3;
	int currentval=sp->startval;
	if(!sp->compressed)return -1;
	last=0;
	out->width=sp->width;
	out->height=sp->height;
	out->size=(sp->width*sp->height+7)/8;
	out->c=sp->c;
	out->compressed=0;
	while(p<end){
		v=*(uint64_t *)p;
		va3=v&3UL;
		p+=1UL<<va3;
		if(va3<3UL)v&=((1UL<<(8UL<<va3))-1UL);
		index=last+(v>>2UL);
		for(;last<index;++last){
			if(currentval)
				SBMP_SETPIXEL(out,last);
			else
				SBMP_CLRPIXEL(out,last);
		}
		currentval^=1;
	}
	return 0;
}
int sbmp_tstpixeln(const struct sbmp *sp,uint64_t n){
	if(!sp->compressed)return !!SBMP_TSTPIXEL(sp,n);
	const char *p=sp->data,*end=sp->data+sp->size-sizeof(uint64_t);
	uint64_t last,v,va3;
	int currentval=sp->startval;
	last=0;
	while(p<end){
		v=*(uint64_t *)p;
		va3=v&3UL;
		p+=1UL<<va3;
		if(va3<3UL)v&=((1UL<<(8UL<<va3))-1UL);
		last+=(v>>2UL);
		if(n<last)break;
		currentval^=1;
	}
	return currentval;
}
int sbmp_tstpixel(const struct sbmp *sp,int32_t x,int32_t y){
	return sbmp_tstpixeln(sp,y*sp->width+x);
}
