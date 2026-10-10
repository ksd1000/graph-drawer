
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
/*
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
*/
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
						v1=expr_ntable((uint8_t)*p);\
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
static uint32_t oldpow32(uint32_t x,uint32_t y){
	uint32_t r;
	if(y){
		r=1;
		for(;;){
			if(y&1){
				r*=x;
			}
			y>>=1;
			if(!y)
				return r;
			x=x*x;
		}
	}
	return 1;
}
#define extint_zero(buf,size) memset(buf,0,(size)*sizeof(uint64_t))
#define extint_copy(buf,src,size) memcpy(buf,src,(size)*sizeof(uint64_t))
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
	while(p>buf&&!p[-1])
		--p;
	return p==buf?1:(size_t)(p-buf);
}
size_t expr_extint_addz(uint64_t *buf,size_t size,uint64_t addend){
	uint64_t *restrict p=buf;
	size_t tmp_size;
	buf[size]=0;
	do {
		addend=((*(p++)+=addend)<addend);
	}while(addend);
	while(p>buf&&!p[-1])
		--p;
	tmp_size=(p==buf?1:(size_t)(p-buf));
	return tmp_size>size?tmp_size:size;
}
size_t expr_extint_add2(uint64_t *buf,const uint64_t *addend_buf,size_t size){
	uint64_t *restrict p=buf;
	size_t s;
	while(size--){
		s=expr_extint_add(p,*(addend_buf++))+(p-buf);
		++p;
	}
	return s;
}
size_t expr_extint_orindex(uint64_t *buf,size_t size,uint64_t index){
	size_t off=index/64;
	if(off>=size){
		buf[off]=UINT64_C(1)<<(index%64);
		if(off>size)
			extint_zero(buf+size,off-size);
		return off+1;
	}
	buf[off]|=UINT64_C(1)<<(index%64);
	return size;
}
size_t expr_extint_sub(uint64_t *buf,uint64_t subtractor){
	uint64_t *restrict p=buf;
	int borrow;
	//the caller should ensure subtractor<=buf
	borrow=(*p<subtractor);
	*(p++)-=subtractor;
	if(borrow)for(;;){
		if(*p){
			--(*p);
			++p;
			break;
		}
		*p=UINT64_MAX;
		++p;
	}
	while(p>buf&&!p[-1])
		--p;
	return p==buf?1:(size_t)(p-buf);
}
void expr_extint_subnrv(uint64_t *buf,uint64_t subtractor){
	uint64_t *restrict p=buf;
	int borrow;
	//the caller should ensure subtractor<=buf
	borrow=(*p<subtractor);
	*(p++)-=subtractor;
	if(borrow)for(;;){
		if(*p){
			--(*p);
			++p;
			break;
		}
		*p=UINT64_MAX;
		++p;
	}
}
size_t expr_extint_subz(uint64_t *buf,size_t size,uint64_t subtractor){
	uint64_t *restrict p=buf;
	int borrow;
	//the caller should ensure subtractor<=buf
	borrow=(*p<subtractor);
	*(p++)-=subtractor;
	if(borrow)while(--size){
		if(*p){
			--(*p);
			++p;
			break;
		}
		*p=UINT64_MAX;
		++p;
	}
	while(p>buf&&!p[-1])
		--p;
	return p==buf?1:(size_t)(p-buf);
}
size_t expr_extint_sub2(uint64_t *buf,size_t size,const uint64_t *subtractor_buf,size_t subsize){
	uint64_t *restrict p=buf;
	//the caller should ensure subtractor_buf<=buf
	do {
		expr_extint_subnrv(p,*(subtractor_buf++));
		++p;
	}while(--subsize);
	while(size>1&&!buf[size-1])
		--size;
	return size;
}
int expr_extint_cmp(const uint64_t *restrict buf,const uint64_t *restrict buf1,size_t size,size_t *index){
	for(--size;;){
		if(buf[size]!=buf1[size]){
			if(index)
				*index=size;
			return buf[size]<buf1[size]?-1:1;
		}
		if(!size)
			return 0;
		--size;
	}
}
int expr_extint_cmpz(const uint64_t *restrict buf,size_t size,const uint64_t *restrict buf1,size_t buf1size){
	if(size!=buf1size){
		return size<buf1size?-1:1;
	}
	for(--size;;){
		if(buf[size]!=buf1[size]){
			return buf[size]<buf1[size]?-1:1;
		}
		if(!size)
			return 0;
		--size;
	}
}
int expr_extint_cmpsub(uint64_t *restrict buf,size_t size,const uint64_t *restrict buf1,size_t buf1size,size_t *outsize){
	if(expr_extint_cmpz(buf,size,buf1,buf1size)<0)
		return 0;
	*outsize=expr_extint_sub2(buf,size,buf1,buf1size);
	return 1;
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
		if(!*wp)
			continue;
		un.size=p-(uint32_t *)buf;
		if(((uintptr_t)p)&7)
			un.size=(un.size>>1)+1+
				expr_extint_add((uint64_t *)(p+1),*wp);
		else {
			*wp=((p[1]+=*wp)<*wp);
			if(*wp){
			un.size=(un.size>>1)+1+
				expr_extint_add((uint64_t *)(p+2),*wp);
			}else
				continue;
		}
		if(un.size>size){
			size=un.size;
			size32=size<<1;
		}
	}
	return buf[size-1]?size:(size>2?size-1:1);
}
// WARNING: too SLOOOOOOOOOWER than the mul2 of GMP
size_t expr_extint_mul2(uint64_t *buf,size_t bufsize,const uint64_t *factor_buf,size_t factor_size,uint64_t *workspace){
	uint32_t *restrict p=(uint32_t *)buf;
	uint32_t *restrict fbuf=(uint32_t *)factor_buf;
	uint64_t *pa=workspace+bufsize,
		 *wsp=workspace+bufsize*2+1,
		 *endp=buf+bufsize+factor_size;
	size_t i=(factor_size<<1);
	extint_copy(workspace,buf,bufsize);
	extint_zero(buf,bufsize+factor_size);
	while(i--){
		extint_zero(pa,wsp-pa);
		extint_copy(pa,workspace,bufsize);
		if(expr_extint_mul(pa,bufsize,*(fbuf++),wsp)>bufsize)
			expr_extint_add2((uint64_t *)p,pa,bufsize+1);
		else
			expr_extint_add2((uint64_t *)p,pa,bufsize);
		++p;
	}
	while(--endp>buf){
		if(*endp)
			break;
	}
	return endp-buf+1;
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
#define bitindexof_ze2(buf,size) ((ssize_t)(63-clz64((buf)[(size)-1]))+(ssize_t)((size)-1)*64)
#define bitindexof(buf,size) ({\
	size_t _size=(size);\
	bitindexof_ze2(buf,_size);\
})
size_t expr_extint_div2(uint64_t *buf,size_t size,uint64_t *divisor_buf,size_t divisor_size,uint64_t *out,size_t *outsize){
	size_t qz;
	size_t lf=(size_t)(bitindexof_ze2(buf,size)-bitindexof_ze2(divisor_buf,divisor_size));
	if((ssize_t)lf<0){
		*out=0;
		*outsize=1;
		return size;
	}
	qz=1;
	*out=0;
	divisor_size=expr_extint_left(divisor_buf,divisor_size,lf);
	for(;;){
		if(expr_extint_cmpsub(buf,size,divisor_buf,divisor_size,&size))
			qz=expr_extint_orindex(out,qz,lf);
		if(unlikely(!lf))
			break;
		--lf;
		divisor_size=expr_extint_right(divisor_buf,divisor_size,1);
	}
	*outsize=qz;
	return size;
}
static uint32_t oldsuppow32(uint32_t base,uint32_t expo2,uint32_t *powed){
	uint32_t ds,n,i,tmp;
	uint32_t cache[32];
	uint32_t *cp;
	if(base<2){
		*powed=1;
		return 0;
	}
	*cache=base;
	cp=cache;
	n=1;
	for(;;){
		if(unlikely(mulo3(*cp,*cp,&tmp))){
			//if(unlikely(expo2&&clz64((uint64_t)*cp)<expo2*n)){
			//	--cp;
			//}
			break;
		}
		n<<=1;
		if(unlikely(expo2&&clz64((uint64_t)tmp)<expo2*n)){
			break;
		}
		*(++cp)=tmp;
	}
	ds=*cp;
	i=(uint32_t)(cp-cache);
	n=UINT32_C(1)<<i;
	--i;
	for(;(int32_t)i>=0;--i){
		--cp;
		if(unlikely(mulo3(ds,*cp,&tmp)))
			continue;
		n|=UINT32_C(1)<<i;
		if(unlikely(expo2&&clz64((uint64_t)tmp)<expo2*n)){
			n-=UINT32_C(1)<<i;
			continue;
		}
		ds=tmp;
	}
	*powed=ds;
	return n;
}
size_t expr_extint_mulnp(uint64_t *buf,size_t size,uint32_t factor,uint32_t power,uint64_t *workspace){
	uint32_t ds,expo2,id,im,n;
	expo2=(uint32_t)ctz32(factor);
	factor>>=expo2;
	n=oldsuppow32(factor,0,&ds);
	if(n){
		id=power/n;
		im=power%n;
		for(;id;--id)
			size=expr_extint_mul(buf,size,ds,workspace);
		size=expr_extint_mul(buf,size,oldpow32(factor,im),workspace);
	}
	return expr_extint_left(buf,size,(uint64_t)expo2*power);
}
#define write_ascii(_op,_sz) \
	uint32_t mod,ds=base,dsn,n;\
	char *out=outbuf;\
	n=oldsuppow32(base,0,&ds);\
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
	return (_sz)
size_t expr_extint_ascii(uint64_t *buf,size_t size,const char *chars,char *outbuf,uint32_t base){
	write_ascii(out++,out-outbuf);
}
size_t expr_extint_ascii_rev(uint64_t *buf,size_t size,const char *chars,char *outbuf,uint32_t base){
	write_ascii(--out,outbuf-out);
}
size_t expr_extint_ascii_convert(uint64_t *buf,const char *inbuf,size_t inbuf_size,uint32_t base,uint64_t *workspace){
	uint32_t ds=base,n;
	uint32_t expo2,base_odd;
	uint64_t v,expo2_mn;
	size_t size;
	const char *in,*endp;
	//the caller should ensure only '0' ... '0'+min(base,36) in inbuf and base !=0
	expo2=(uint32_t)ctz32(base);
	base_odd=base>>expo2;
	n=oldsuppow32(base_odd,expo2,&ds);
	//id=inbuf_size/n;
	in=inbuf+inbuf_size;
	*buf=0;
	size=1;
	v=0;
	endp=inbuf+inbuf_size;
	if(n){
		expo2_mn=expo2*n;
		in=inbuf+(inbuf_size%n);
		for(const char *p=inbuf;p<in;++p){
			v=v*base+expr_ntable(*p);
		}
		size=expr_extint_mul(buf,size,ds,workspace);
		size=expr_extint_left(buf,size,expo2_mn);
		size=expr_extint_addz(buf,size,v);
		while(in<endp){
			v=0;
			for(const char *p=in+n;in<p;++in){
				v=v*base+expr_ntable(*in);
			}
			size=expr_extint_mul(buf,size,ds,workspace);
			size=expr_extint_left(buf,size,expo2_mn);
			size=expr_extint_addz(buf,size,v);
		}
	}else {
		n=UINT32_C(32)/expo2;
		expo2_mn=expo2*n;
		in=inbuf+(inbuf_size%n);
		for(const char *p=inbuf;p<in;++p){
			v=(v<<expo2)+expr_ntable(*p);
		}
		size=expr_extint_left(buf,size,expo2_mn);
		size=expr_extint_addz(buf,size,v);
		while(in<endp){
			v=0;
			for(const char *p=in+n;in<p;++in){
				v=(v<<expo2)+expr_ntable(*in);
			}
			size=expr_extint_left(buf,size,expo2_mn);
			size=expr_extint_addz(buf,size,v);
		}
	}
	return size;
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
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wunused-variable"
#define DDMAXBIT 1076
#define nbuf ws->iabuf
#define buf ws->dbuf
#define wspace ws->workspace
#define frac ws->dfrac
#define set_index if(end_index)*end_index
double expr_internal_strtod4(const char *restrict nptr,size_t nsize,size_t *restrict end_index,struct expr_strtod_workspace *restrict ws){
	const char *nptr0;
	union {
		ssize_t _trunc;
		size_t _frsize;
	} un;
	union {
		char *_np1;
		ssize_t _u;
	} un1;
	union {
		char *_np;
		ssize_t _kd;
	} un2;
	union {
		const char *_endp;
		size_t _wsize;
		ssize_t _extra2;
	} un3;
	union {
		const char *_point;
		ssize_t _e2;
	} un4;
#define trunc un._trunc
#define frsize un._frsize
#define np1 un1._np1
#define u un1._u
#define np un2._np
#define kd un2._kd
#define endp un3._endp
#define wsize un3._wsize
#define extra2 un3._extra2
#define point un4._point
#define e2 un4._e2
	ssize_t expo;
	size_t npsize;
	union expr_double val;
	uint32_t base;
	uint8_t trunc_nonzero,negative=0,umbrella;
	//treat the special cases.
	if(unlikely(!nsize)){
fail0:
		set_index=0;
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
	switch(*nptr|32){
		special_case('I','n','f',INFINITY);
		special_case('i','n','f',INFINITY);
#define special_case_payload(c1,c2,defv,constructor) \
			if(unlikely(nptr+2>=endp||(nptr[1]|32)!=(c1)||(nptr[2]|32)!=(c2)))\
				goto fail0;\
			nptr+=3;\
			if(nptr==endp){\
				set_index=(size_t)(nptr-nptr0);\
				return negative?-(defv):(defv);\
			}\
			if(*nptr!='('){\
				set_index=(size_t)(nptr-nptr0);\
				return negative?-(defv):(defv);\
			}\
			++nptr;\
			np=memchr(nptr,')',endp-nptr);\
			if(unlikely(!np)){\
				set_index=(size_t)(nptr-nptr0)-1;\
				return negative?-(defv):(defv);\
			}\
			trunc=(ssize_t)(np-nptr);\
			expo=expr_internal_strtoz(nptr,(size_t)trunc,&npsize,0);\
			val.uval=(constructor);\
			set_index=(size_t)(np-nptr0)+1;\
			return val.val
		case 'n':
			special_case_payload('a','n',NAN,((uint64_t)negative<<63)|(UINT64_C(4095)<<51)|((uint64_t)expo&((UINT64_C(1)<<52)-1)));
		case 'c':
			special_case_payload('t','b',0.0,(uint64_t)expo);
			//construct bits like nan() but do not fill 0x7ff8000000000000
	//npsize not used here, avoid SIGSEGV.
		default:
			break;
	}
	if(nptr+2<endp&&*nptr=='0'){
#define checkbase(base) (expr_ntable(nptr[2])<base||(nptr+3<endp&&nptr[2]=='.'&&expr_ntable(nptr[3])<base))
		if((nptr[1]|32)=='x'&&checkbase(16)){
			base=16;
			nptr+=2;
		}else if((nptr[1]|32)=='b'&&checkbase(2)){
			base=2;
			nptr+=2;
		}else 
			base=10;
	}else
		base=10;
	//jump all zeros on the head.
	point=NULL;
	umbrella=0;
#define RZSU() if(umbrella)goto zero;else return 0.0
	//return zero with sign when umbrella is set.
	while(nptr<endp){
		switch(*nptr){
			case '.':
				if(unlikely(point)){
					set_index=umbrella?(size_t)(nptr-nptr0):0;
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
		set_index=umbrella?nsize:0;
		RZSU();
	}
	np=nbuf;
	switch(base){
		case 2:
			np1=nbuf+1130;
			break;
		case 10:
			np1=nbuf+DDMAXBIT;
			break;
		case 16:
			np1=nbuf+283;
			break;
		default:
			__builtin_unreachable();
	}
	trunc=0;
	trunc_nonzero=0;
	debug("*nptr=%d,index=%zd",(int)*nptr,nptr-nptr0);
	for(;nptr<endp;++nptr){
		if(expr_ntable(*nptr)<base){
			if(likely(np<np1)){
				*(np++)=*nptr;
				debug("table(*nptr)=%d,index=%zd,base=%u",(int)expr_ntable(*nptr),nptr-nptr0,base);
			}else {
				++trunc;
				if(*nptr!='0')
					trunc_nonzero=1;
			}
			continue;
		}else if(*nptr=='.'){
			if(unlikely(point)){
				expo=0;
				goto no_expo;
			}
			point=(const char *)nptr;
			continue;
		}
		break;
	}
	debug("*nptr=%d,index=%zd",(int)*nptr,nptr-nptr0);
	if(unlikely(!umbrella&&np==nbuf)){
		set_index=0;
		RZSU();
	}
	if(nptr+1<endp&&(*nptr|32)==(base==10?'e':'p')){
		++nptr;
		expo=expr_internal_strtoz(nptr,endp-nptr,&npsize,0);
		if(unlikely(!npsize)){
			--nptr;
			goto no_expo;
		}
		set_index=(size_t)((nptr-nptr0)+npsize);
		--nptr;
	}else {
		expo=0;
		extra2=0;
no_expo:
		//strtoz returns 0 on fail. need not set again.
		set_index=(size_t)(nptr-nptr0);
	}
	npsize=np-nbuf;
	if(base==16)
		expo+=(point?trunc-((ssize_t)(nptr-point)-1):trunc)*4;
	else
		expo+=point?trunc-((ssize_t)(nptr-point)-1):trunc;
	if(!npsize){
zero:
		return negative?-0.0:0.0;
	}
	//D=nbuf with npsize,E=expo
	switch(base){
		case 2:
		case 16:
			e2=-1176+1;
			kd=1024+1;
			break;
		case 10:
			e2=-324+1;
			kd=308+1;
			break;
		/*
		case 16:
			e2=-269+1;
			kd=256+1;
			break;
		*/
			//+1 from -(npsize-1)
		default:
			__builtin_unreachable();
	}
	debug("expo=%zd,expo+npsize-1=%zd,e2-npsize=%zd",expo,(ssize_t)(expo-npsize-1),e2-(ssize_t)npsize);
	if(unlikely((ssize_t)expo<e2-(ssize_t)npsize)){
		return negative?-0.0:0.0;
	}
	if(unlikely((ssize_t)expo>kd-(ssize_t)npsize)){
e2inf:
		return negative?-INFINITY:INFINITY;
	}
	npsize=expr_extint_ascii_convert(buf,nbuf,npsize,base,wspace);
	//npsize use for buf after here.
	if(expo>=0){
		if(expo)
			npsize=expr_extint_mulnp(buf,npsize,base==16?2:base,expo,wspace);
		*frac=1;
		frsize=1;
		e2=bitindexof_ze2(buf,npsize);
		umbrella=0;
		u=e2-52;
	}else {
		*frac=1;
		frsize=expr_extint_mulnp(frac,1,base==16?2:base,(uint32_t)-expo,wspace);
#define cmp_correct(s,sz,d,dz,left_or_right)\
		if(expr_extint_cmpz(s,sz,d,dz)<0){\
			--kd;\
		}else {\
			wsize=left_or_right(wspace,wsize,1);\
			if(expr_extint_cmpz(s,sz,d,dz)>=0){\
				++kd;\
			}\
		}
		kd=bitindexof_ze2(buf,npsize)-bitindexof_ze2(frac,frsize);
		if(kd>0){
			memcpy(wspace,frac,frsize*sizeof(uint64_t));
			wsize=expr_extint_left(wspace,frsize,kd);
			cmp_correct(buf,npsize,wspace,wsize,expr_extint_left);
		}else if(kd<0){
			memcpy(wspace,buf,npsize*sizeof(uint64_t));
			wsize=expr_extint_left(wspace,npsize,-kd);
			cmp_correct(wspace,wsize,frac,frsize,expr_extint_right);
		}else {
			if(npsize<frsize||(npsize==frsize&&expr_extint_cmp(buf,frac,npsize,NULL)<0)){
				--kd;
			}
		}
		if(kd<-1022){
			umbrella=1;
			e2=-1022;
		}else {
			umbrella=0;
			e2=kd;
		}
		u=e2-52;
	}
	if(u>0)
		frsize=expr_extint_left(frac,frsize,(uint64_t)u);
	else if(u<0)
		npsize=expr_extint_left(buf,npsize,(uint64_t)-u);
	npsize=expr_extint_div2(buf,npsize,frac,frsize,&val.uval,&wsize);
	assume(wsize==1);
	npsize=expr_extint_left(buf,npsize,1);
	switch(expr_extint_cmpz(buf,npsize,frac,frsize)){
		case -1:
			break;
		case 0:
			if(trunc_nonzero||(val.uval&1))
				++val.uval;
			break;
		case 1:
			++val.uval;
			break;
		default:
			__builtin_unreachable();
	}
	if(e2>1023||(val.rd.exp&~(UINT64_C(1))))
		goto e2inf;
	val.rd.sign=negative;
	if(!umbrella){
		val.rd.exp=(uint64_t)(e2+1023);
	}
	return val.val;//completed ?
}
double expr_internal_strtod(const char *restrict nptr,size_t nsize,size_t *restrict end_index){
	struct expr_strtod_workspace ws[1];
	return expr_internal_strtod4(nptr,nsize,end_index,ws);
}
double expr_internal_strtod_mtl(const char *restrict nptr,size_t nsize,size_t *restrict end_index,const struct expr_memtool *restrict mtl){
	struct expr_strtod_workspace *ws;
#define expr_allocator(size) (xmtl->allocate((size),xmtl->arg))
#define expr_reallocator(old,size) (xmtl->reallocate((old),(size),xmtl->arg))
#define expr_deallocator(old) (xmtl->deallocate((old),xmtl->arg))
#define xmtl mtl
	ws=xmalloc(sizeof(struct expr_strtod_workspace));
	if(ws){
		double r;
		r=expr_internal_strtod4(nptr,nsize,end_index,ws);
		xfree(ws);
		return r;
	}else {
		ws=alloca(sizeof(struct expr_strtod_workspace));
		return expr_internal_strtod4(nptr,nsize,end_index,ws);
	}
}
