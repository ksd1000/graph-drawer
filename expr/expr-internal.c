
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
size_t expr_extint_left(uint64_t *buf,size_t size,uint64_t bits){
	uint64_t b64=bits/64;
	size_t rsize=size;
	uint64_t *p,*p0;
	bits%=64;
	if(b64){
		rsize+=b64;
		p0=buf+size-1;
		p=p0+b64;
		do{
			*p=*p0;
			--p0;
			--p;
		}while(p0>=buf);
		do{
			*p=0;
			--p;
		}while(p>=buf);
	}
	if(bits){
		p=buf+rsize;
		p0=p;
		*p=0;
		do {
			*p=(p[-1]>>(64-bits))|(*p<<bits);
			--p;
		}while(p>buf);
		*buf<<=bits;
		if(*p0)
			++rsize;
	}
	return rsize;
}
size_t expr_extint_right(uint64_t *buf,size_t size,uint64_t bits){
	uint64_t b64=bits/64;
	size_t rsize=size;
	uint64_t *p,*p0,*end;
	if(b64){
		if(rsize<=b64){
			*buf=0;
			return 1;
		}
		rsize-=b64;
		p=buf;
		p0=p+b64;
		end=buf+rsize;
		do{
			*p=*p0;
			++p;
			if(p0>=end)
				break;
			++p0;
		}while(p0<end);
	}
	bits%=64;
	if(bits){
		p=buf;
		end=buf+rsize-1;
		while(p<end){
			*p=(p[1]<<(64-bits))|(*p>>bits);
			++p;
		}
		*end>>=bits;
		if(rsize>1&&!*end)
			--rsize;
	}
	return rsize;
}
size_t expr_extint_add(uint64_t *buf,uint64_t addend){
	uint64_t *restrict p=buf;
	do {
		addend=((*(p++)+=addend)<addend);
	}while(addend);
	return p-buf;
}
// WARNING: the expr_extint_mul is old
size_t expr_extint_mul(uint64_t *buf,size_t size,uint32_t factor,uint64_t *workspace){
	uint32_t *restrict p;
	uint32_t *restrict wp;
	union {
		uint64_t v;
		size_t size;
	} un;
	buf[size]=0;
	size_t size32=size<<1,csize32;
	for(p=(uint32_t *)buf,wp=(uint32_t *)workspace;
			(size_t)(p-(uint32_t *)buf)<size32;
			++p,++wp){
		un.v=((uint64_t)*p)*factor;
		*p=un.v&0xfffffffful;
		*wp=(un.v>>32);
	}
	csize32=size32;
	for(p=(uint32_t *)buf,wp=(uint32_t *)workspace;
			(wp-(uint32_t *)workspace)<csize32;
			++p,++wp){
		if(!*wp)continue;
		un.size=p-(uint32_t *)buf;
		if(((uintptr_t)p)&7)
			un.size=(un.size>>1)+1+
				expr_extint_add((uint64_t *)(p+1),*wp);
		else {
			*wp=((p[1]+=*wp)<*wp);
			if(*wp){
			un.size=(un.size>>1)+1+
				expr_extint_add((uint64_t *)(p+2),*wp);
			}else continue;
		}
		if(un.size>size){
			size=un.size;
			size32=size<<1;
		}
	}
	return size;
}
size_t expr_extint_div(uint64_t *buf,size_t size,uint32_t divisor,uint32_t *mod){
	uint32_t *p=(uint32_t *)(buf+size);
	uint64_t v;
	size_t rsize=0;
	uint32_t backup=0;
	*(uint32_t *)p=0;
	do {
		--p;
		v=*(uint64_t *)p/divisor;
		*(uint64_t *)p%=divisor;
		((uint32_t *)p)[1]=backup;
		backup=v;
		if(!v)
			continue;
		if(!rsize)
			rsize=(p-(uint32_t *)buf)+1;
	}while(p>(uint32_t *)buf);
	if(mod)
		*mod=*(uint32_t *)buf;
	*(uint32_t *)buf=backup;
	if(rsize)
		return (rsize+1)>>1;
	else
		return 0;
}
#define write_ascii(_op,_sz) \
	uint32_t mod,ds=base,dsn,n;\
	char *out=outbuf;\
	for(n=1;;){\
		dsn=ds*base;\
		if(dsn>ds&&!(dsn%ds)){\
			ds=dsn;\
			++n;\
		}else\
			break;\
	}\
	for(;;){\
		size=expr_extint_div(buf,size,ds,&mod);\
		if(size){\
			for(dsn=n;dsn;--dsn){\
				*(_op)=chars[mod%base];\
				mod/=base;\
			}\
		}else {\
			while(mod){\
				*(_op)=chars[mod%base];\
				mod/=base;\
			}\
			break;\
		}\
	}\
	if(out==outbuf)\
		*(_op)=chars[0];\
	size=(_sz);\
	return size
size_t expr_extint_ascii(uint64_t *buf,size_t size,const char *chars,uint32_t base,char *outbuf){
	write_ascii(out++,out-outbuf);
}
size_t expr_extint_ascii_rev(uint64_t *buf,size_t size,const char *chars,uint32_t base,char *outbuf){
	write_ascii(--out,outbuf-out);
}
ssize_t expr_internal_strtoz(const char *restrict nptr,size_t nsize,size_t *restrict end_index,int base){
	ssize_t r=0;
	int neg=0;
	unsigned int get;
	const char *startp,*endp;
	startp=(const char *)nptr;
	endp=startp+nsize;
	if(nsize>=2){
		switch(*nptr){
			case '-':
				neg=1;
			case '+':
				++nptr;
				break;
		}
	}
	if(!base){
		if(nptr+1<endp&&*nptr=='0'){
			++nptr;
			if(nptr+1<endp&&*nptr=='x'){
				++nptr;
				get=expr_ntable(*nptr);
				if(unlikely(get>=16)){
					--nptr;
					goto out;
				}
				base=16;
				goto got;
			}else {
				get=expr_ntable(*nptr);
				if(unlikely(get>=8)){
					goto out;
				}
				base=8;
				goto got;
			}
		}else {
			base=10;
		}
	}
	if(unlikely(nptr>=endp)){
		goto atend;
	}
	// base cannot be >36
	get=expr_ntable(*nptr);
	if(unlikely(get>=base)){
atend:
		if(nptr==startp+1){
			--nptr;
		}
		goto out;
	}
got:
	if(unlikely(mulo(r,base)))
		goto overflow;
	if(unlikely(addo(r,get)))
		goto overflow;
	++nptr;
	while(nptr<endp){
		get=expr_ntable(*nptr);
		if(unlikely(get>=base))
			goto out;
		if(unlikely(mulo(r,base)))
			goto overflow;
		if(unlikely(addo(r,get)))
			goto overflow;
		++nptr;
	}
out:
	*end_index=(size_t)(nptr-startp);
	debug("result:%zd",(ssize_t)(neg?-r:r));
	return (ssize_t)(neg?-r:r);
overflow:
	++nptr;
	while(nptr<endp){
		get=expr_ntable(*nptr);
		if(unlikely(get>=base))
			break;
		++nptr;
	}
	*end_index=(size_t)(nptr-startp);
	debug("overflow result:%zd",(ssize_t)(neg?PTRDIFF_MIN:PTRDIFF_MAX));
	return (ssize_t)(neg?PTRDIFF_MIN:PTRDIFF_MAX);
}
#define special_case(c0,c1,c2,val) \
		case c0:\
			if(nptr+2>=endp)\
				goto fail0;\
			if(likely((nptr[1]|32)==c1&&(nptr[2]|32)==c2)){\
				*end_index=(size_t)(nptr-nptr0)+3;\
				return negative?-(val):(val);\
			}\
			goto fail0
#define DDMAXBIT 1076
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
double expr_internal_strtod(const char *restrict nptr,size_t nsize,size_t *restrict end_index){
	char nbuf[DDMAXBIT];
	char *np,*np1;
	const char *endp,*point,*nptr0;
	ssize_t expo,trunc;
	size_t npsize;
	union expr_double val;
	int trunc_nonzero,negative=0,umbrella;
	//treat the special cases.
	if(unlikely(!nsize)){
fail0:
		*end_index=0;
		return 0.0;
	}
	nptr0=(const char *)nptr;
	endp=(const char *)nptr+nsize;
	//the caller should ensure no blank at the start of nptr.
	if(nsize>=2){
		switch(*nptr){
			case '-':
				negative=1;
			case '+':
				++nptr;
				break;
		}
	}
	switch(*nptr){
		special_case('I','n','f',INFINITY);
		special_case('i','n','f',INFINITY);
		case 'N':
		case 'n':
			if(unlikely(nptr+2>=endp||(nptr[1]|32)!='a'||(nptr[2]|32)!='n'))
				goto fail0;
			nptr+=3;
			if(nptr==endp){
no_payload:
				*end_index=(size_t)(nptr-nptr0);
				return negative?-NAN:NAN;
			}
			if(*nptr!='(')
				goto no_payload;
			++nptr;
			np=memchr(nptr,')',endp-nptr);
			if(unlikely(!np)){
				--nptr;
				goto no_payload;
			}
			trunc=(ssize_t)(np-nptr);
			expo=expr_internal_strtoz(nptr,(size_t)trunc,&npsize,0);
			//npsize not used here, avoid SIGSEGV.
			val.uval=((uint64_t)negative<<63)|(UINT64_C(4095)<<51)|(expo&((UINT64_C(1)<<52)-1));
			*end_index=(size_t)(np-nptr0)+1;
			return val.val;
		default:
			break;
	}
	//jump all zeros on the head.
	point=NULL;
	umbrella=0;
#define RZSU() if(umbrella)goto zero;else return 0.0
	//return zero with sign when umbrella is set.
	while(nptr<endp){
		switch(*nptr){
			case '.':
				if(unlikely(point)){
					*end_index=umbrella?(size_t)(nptr-nptr0):0;
					RZSU();
				}
				point=nptr;
				++nptr;
				continue;
			case '0':
				umbrella=1;
				++nptr;
				continue;
			default:
				break;
		}
		break;
	}
	if(unlikely(nptr==endp)){
		*end_index=umbrella?nsize:0;
		RZSU();
	}
	np=nbuf;
	np1=nbuf+DDMAXBIT;
	trunc=0;
	trunc_nonzero=0;
	for(;nptr<endp;++nptr){
		switch(*nptr){
			case '0' ... '9':
				if(likely(np<np1))
					*(np++)=*nptr-(char)'0';
				else {
					++trunc;
					if(*nptr!='0')
						trunc_nonzero=1;
				}
				continue;
			case '.':
				if(unlikely(point)){
					expo=0;
					goto no_expo;
				}
				point=(const char *)nptr;
				continue;
			default:
				break;
		}
		break;
	}
	if(unlikely(!umbrella&&np==nbuf)){
		*end_index=0;
		RZSU();
	}
	if(nptr+1<endp&&(*nptr|32)=='e'){
		++nptr;
		expo=expr_internal_strtoz(nptr,endp-nptr,&npsize,10);
		if(unlikely(!npsize)){
			--nptr;
			goto no_expo;
		}
		*end_index=(size_t)((nptr-nptr0)+npsize);
		--nptr;
	}else {
		expo=0;
no_expo:
		//strtoz returns 0 on fail. need not set again.
		*end_index=(size_t)(nptr-nptr0);
	}
	npsize=np-nbuf;
	if(!point)
		expo+=trunc;
	else
		expo+=trunc-((ssize_t)(nptr-point)-1);
	if(np==nbuf){
zero:
		val.uval=((uint64_t)negative<<63);
		return val.val;
	}
	//do D=nbuf with npsize,E=expo?
	return 0;//incompleted
}
