/*******************************************************************************
 *License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>*
 *This is free software: you are free to change and redistribute it.           *
 *******************************************************************************/
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "sbmp.h"
struct sbmp *sbmp_compress(const struct sbmp *sp){
	struct sbmp *nsp,*nsp1;
	unsigned char *p;
	uint64_t index,total,last;
	unsigned char currentval;
	if(sp->compressed||!(nsp=malloc(
		(sp->size<<5)+sizeof(uint64_t)+sizeof(struct sbmp)
	)))return NULL;
	nsp->width=sp->width;
	nsp->height=sp->height;
	nsp->c=sp->c;
	total=sp->width*sp->height;
	currentval=(*sp->data)&1;
	nsp->startval=currentval;
	memset(nsp->unused,0xcc,6);
	//fprintf(stderr,"%hhu\n",currentval);
	last=0;
	p=nsp->data;
	for(index=1;index<=total;++index){//fprintf(stderr,"%u\n",SBMP_TSTPIXEL(sp,index));
		if(((!!SBMP_TSTPIXEL(sp,index))==currentval)
			&&index<total)
			continue;
		
		currentval^=1;
		if(index-last<=(UINT8_MAX>>2)){
			*(uint8_t *)p=((index-last)<<2);
			p+=sizeof(uint8_t);
		}else if(index-last<=(UINT16_MAX>>2)){
			*(uint16_t *)p=((index-last)<<2)|UINT64_C(1);
			p+=sizeof(uint16_t);
		}else if(index-last<=(UINT32_MAX>>2)){
			*(uint32_t *)p=((index-last)<<2)|UINT64_C(2);
			p+=sizeof(uint32_t);
		}else {
			*(uint64_t *)p=((index-last)<<2)|UINT64_C(3);
			p+=sizeof(uint64_t);
		}
		last=index;

	}
	*(uint64_t *)p=0xccccccccccccccccul;
	nsp->size=(p-nsp->data+sizeof(uint64_t));
	nsp->compressed=1;
	nsp1=realloc(nsp,nsp->size+sizeof(struct sbmp));
	return nsp1?nsp1:nsp;
}
int sbmp_decompress(const struct sbmp *sp,struct sbmp *out){
	const unsigned char *p=sp->data,*end=sp->data+sp->size-sizeof(uint64_t);
	uint64_t index,last,v,va3;
	int currentval=!!sp->startval;
	if(!sp->compressed)return -1;
	last=0;
	out->width=sp->width;
	out->height=sp->height;
	out->size=(sp->width*sp->height+7)/8;
	out->c=sp->c;
	out->compressed=0;
	while(p<end){
		v=*(uint64_t *)p;
		va3=v&UINT64_C(3);
		p+=UINT64_C(1)<<va3;
		if(va3<UINT64_C(3))v&=((UINT64_C(1)<<(UINT64_C(8)<<va3))-UINT64_C(1));
		index=last+(v>>UINT64_C(2));
		for(;last<index;++last){
			if(currentval){
				SBMP_SETPIXEL(out,last);
			}else {
				SBMP_CLRPIXEL(out,last);
			}
		}
		currentval^=1;
	}
	return 0;
}
int sbmp_tstpixeln(const struct sbmp *sp,uint64_t n){
	const unsigned char *p,*end;
	uint64_t last,v,va3;
	int currentval;
	if(!sp->compressed)
		return !!SBMP_TSTPIXEL(sp,n);
	currentval=sp->startval;
	p=sp->data;
	end=sp->data+sp->size-sizeof(uint64_t);
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
	int r=sbmp_tstpixeln(sp,y*sp->width+x);
	//if(r)++ok;
	return r;
}
