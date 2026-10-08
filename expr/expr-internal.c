
/*******************************************************************************
 *License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>*
 *This is free software: you are free to change and redistribute it.           *
 *******************************************************************************/
#define _GNU_SOURCE
#include <math.h>
#include <string.h>
#include <float.h>
#include <setjmp.h>

#define EXPR_INLIB 1
#include "expr.h"

const uint8_t expr_number_table[256]={
[0 ... '0'-1]=127,
['0']=0,
['1']=1,
['2']=2,
['3']=3,
['4']=4,
['5']=5,
['6']=6,
['7']=7,
['8']=8,
['9']=9,
['9'+1 ... 'A'-1]=127,
['A']=10,
['B']=11,
['C']=12,
['D']=13,
['E']=14,
['F']=15,
['F'+1 ... 'a'-1]=127,
['a']=10,
['b']=11,
['c']=12,
['d']=13,
['e']=14,
['f']=15,
['f'+1 ... 255]=127,
};
size_t expr_strscan(const char *restrict s,size_t sz,char *restrict buf,size_t outsz){
	const char *p,*endp=s+sz;
	char *buf0=(char *)buf;
	uint8_t v,v1;
	char *oend;
	oend=buf0+outsz;
	while(s<endp&&buf<oend)switch(*s){
		case '\\':
			if(unlikely(s+1>=endp))
				goto dflt;
			switch(s[1]){
				case '\\':
					*(buf++)='\\';
					s+=2;
					break;
				case 'a':
					*(buf++)='\a';
					s+=2;
					break;
				case 'b':
					*(buf++)='\b';
					s+=2;
					break;
				case 'c':
					*(buf++)='\0';
					s+=2;
					break;
				case 'e':
					*(buf++)='\033';
					s+=2;
					break;
				case 'f':
					*(buf++)='\f';
					s+=2;
					break;
				case 'n':
					*(buf++)='\n';
					s+=2;
					break;
				case 'r':
					*(buf++)='\r';
					s+=2;
					break;
				case 't':
					*(buf++)='\t';
					s+=2;
					break;
				case 'v':
					*(buf++)='\v';
					s+=2;
					break;
#define scanoux(base,maxlen) \
					v=0;\
					while(p<endp){\
						v1=expr_number_table[(uint8_t)*p];\
						if(unlikely(v1>=base))\
							break;\
						v=v*base+v1;\
						++p;\
						if(p-s>=maxlen){\
							if(base==16){\
								*(buf++)=v;\
								if(unlikely(buf>=oend))\
									goto no_enough_size;\
								s=p;\
								goto x_start;\
							}else\
								break;\
						}\
					}\
					if(unlikely(p==s))\
						goto fail;\
					*(buf++)=v;\
					s=p
				case 'B':
					s+=2;
					p=s;
					scanoux(2,8);
					break;
				case 'u':
					s+=2;
					p=s;
					scanoux(10,3);
					break;
				case 'x':
					s+=2;
					p=s;
x_start:
					scanoux(16,2);
					break;
				default:
					++s;
					p=s;
					scanoux(8,3);
					break;
fail:
					break;
			}
			break;
		default:
dflt:
			*(buf++)=*(s++);
			break;
	}
no_enough_size:
	return buf-buf0;
}

void expr_memswap(void *restrict s1,void *restrict s2,size_t size){
	register union {
		int64_t swapbuf;
		int32_t swapbuf32;
		int16_t swapbuf16;
		int8_t swapbuf8;
	} un;
	while(size>=8){
		un.swapbuf=*(int64_t *)s1;
		*(int64_t *)s1=*(int64_t *)s2;
		*(int64_t *)s2=un.swapbuf;
		size-=8;
		++(*(int64_t **)&s1);
		++(*(int64_t **)&s2);
	}
	if(size>=4){
		un.swapbuf32=*(int32_t *)s1;
		*(int32_t *)s1=*(int32_t *)s2;
		*(int32_t *)s2=un.swapbuf32;
		size-=4;
		++(*(int32_t **)&s1);
		++(*(int32_t **)&s2);
	}
	if(size>=2){
		un.swapbuf16=*(int16_t *)s1;
		*(int16_t *)s1=*(int16_t *)s2;
		*(int16_t *)s2=un.swapbuf16;
		size-=2;
		++(*(int16_t **)&s1);
		++(*(int16_t **)&s2);
	}
	if(size){
		un.swapbuf8=*(int8_t *)s1;
		*(int8_t *)s1=*(int8_t *)s2;
		*(int8_t *)s2=un.swapbuf8;
	}
}
