
/*******************************************************************************
 *License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>*
 *This is free software: you are free to change and redistribute it.           *
 *******************************************************************************/
#define _GNU_SOURCE
#if (defined(EXPR_LIBC_STRING_IMPL)&&(EXPR_LIBC_STRING_IMPL))
#include <string.h>
#endif
#include <float.h>
#include <math.h>

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
								*(buf++)=(char)v;\
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
					*(buf++)=(char)v;\
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
	return (size_t)(buf-buf0);
}

void expr_memswap(void *restrict s1,void *restrict s2,size_t size){
	register union {
		int64_t swapbuf;
		int32_t swapbuf32;
		int16_t swapbuf16;
		int8_t swapbuf8;
	} un;
	while(size>=8){
		un.swapbuf=*(int64_t *)(uintptr_t)s1;
		*(int64_t *)(uintptr_t)s1=*(int64_t *)(uintptr_t)s2;
		*(int64_t *)(uintptr_t)s2=un.swapbuf;
		size-=8;
		++(*(int64_t **)(uintptr_t)&s1);
		++(*(int64_t **)(uintptr_t)&s2);
	}
	if(size>=4){
		un.swapbuf32=*(int32_t *)(uintptr_t)s1;
		*(int32_t *)(uintptr_t)s1=*(int32_t *)(uintptr_t)s2;
		*(int32_t *)(uintptr_t)s2=un.swapbuf32;
		size-=4;
		++(*(int32_t **)(uintptr_t)&s1);
		++(*(int32_t **)(uintptr_t)&s2);
	}
	if(size>=2){
		un.swapbuf16=*(int16_t *)(uintptr_t)s1;
		*(int16_t *)(uintptr_t)s1=*(int16_t *)(uintptr_t)s2;
		*(int16_t *)(uintptr_t)s2=un.swapbuf16;
		size-=2;
		++(*(int16_t **)(uintptr_t)&s1);
		++(*(int16_t **)(uintptr_t)&s2);
	}
	if(size){
		un.swapbuf8=*(int8_t *)(uintptr_t)s1;
		*(int8_t *)(uintptr_t)s1=*(int8_t *)(uintptr_t)s2;
		*(int8_t *)(uintptr_t)s2=un.swapbuf8;
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
#define extint_zero(buf,size) memset(buf,0,(size_t)(size)*sizeof(uint64_t))
#define extint_copy(buf,src,size) memcpy(buf,src,(size_t)(size)*sizeof(uint64_t))
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
		memset(buf,0,(uintptr_t)p-(uintptr_t)buf+8);
		/*do{
			*p=0;
			--p;
		}while(p>=buf);*/
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
	do {
		s=expr_extint_add(p,*(addend_buf++))+(size_t)(p-buf);
		++p;
	}while(--size);
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
			(size_t)(wp-(uint32_t *)workspace)<csize32;
			++p,++wp){
		if(!*wp)
			continue;
		un.size=(size_t)(p-(uint32_t *)buf);
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
	const uint32_t *restrict fbuf=(const uint32_t *)factor_buf;
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
	return (size_t)(endp-buf+1);
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
		backup=(uint32_t)v;
		if(!v)
			continue;
		if(!rsize)
			rsize=(size_t)((p-(uint32_t *)buf)+1);
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
		if(unlikely(expo2&&(uint32_t)clz64((uint64_t)tmp)<expo2*n)){
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
		if(unlikely(expo2&&(uint32_t)clz64((uint64_t)tmp)<expo2*n)){
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
	write_ascii(out++,(size_t)(out-outbuf));
}
size_t expr_extint_ascii_rev(uint64_t *buf,size_t size,const char *chars,char *outbuf,uint32_t base){
	write_ascii(--out,(size_t)(outbuf-out));
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
			if(nptr+1<endp&&(*nptr=='x'||*nptr=='b')){
				++nptr;
				base=(*nptr=='x'?16:2);
				get=expr_ntable(*nptr);
				if(unlikely(get>=(unsigned int)base)){
					--nptr;
					goto out;
				}
				goto got;
			}else {
				base=8;
				get=expr_ntable(*nptr);
				if(unlikely(get>=8)){
					goto out;
				}
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
	if(unlikely(get>=(unsigned int)base)){
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
		if(unlikely(get>=(unsigned int)base))
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
		if(unlikely(get>=(unsigned int)base))
			break;
		++nptr;
	}
	*end_index=(size_t)(nptr-startp);
	debug("overflow result:%zd",(ssize_t)(neg?PTRDIFF_MIN:PTRDIFF_MAX));
	return (ssize_t)(neg?PTRDIFF_MIN:PTRDIFF_MAX);
}

#define set_index(idx) {debug("set index: %zu",(size_t)(idx));if(end_index)*end_index=(idx);}

#define special_case(c0,c1,c2,c3,c4,c5,c6,c7,val) \
		case c0:\
			if(nptr+2>=endp)\
				goto fail0;\
			if(likely((nptr[1]|32)==c1&&(nptr[2]|32)==c2)){\
				if(nptr+7<endp&&(nptr[3]|32)==c3&&(nptr[4]|32)==c4&&(nptr[5]|32)==c5&&(nptr[6]|32)==c6&&(nptr[7]|32)==c7){\
					set_index((size_t)(nptr-nptr0)+8);\
				}else {\
					set_index((size_t)(nptr-nptr0)+3);\
				}\
				return negative?-(val):(val);\
			}\
			goto fail0

#define special_case_payload(c1,c2,defv,constructor) \
			if(unlikely(nptr+2>=endp||(nptr[1]|32)!=(c1)||(nptr[2]|32)!=(c2)))\
				goto fail0;\
			nptr+=3;\
			if(nptr==endp){\
				set_index((size_t)(nptr-nptr0));\
				return negative?-(defv):(defv);\
			}\
			if(*nptr!='('){\
				set_index((size_t)(nptr-nptr0));\
				return negative?-(defv):(defv);\
			}\
			++nptr;\
			np=memchr(nptr,')',(size_t)(endp-nptr));\
			if(unlikely(!np)){\
				set_index((size_t)(nptr-nptr0)-1);\
				return negative?-(defv):(defv);\
			}\
			trunc=(ssize_t)(np-nptr);\
			expo=expr_internal_strtoz(nptr,(size_t)trunc,&npsize,0);\
			val.uval=(constructor);\
			set_index((size_t)(np-nptr0)+1);\
			return val.val
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wunused-variable"
#define DDMAXBIT 1076
#define nbuf ws->iabuf
#define buf ws->dbuf
#define wspace ws->workspace
#define frac ws->dfrac
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
		set_index(0);
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
		special_case('i','n','f','i','n','i','t','y',INFINITY);
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
					set_index(umbrella?(size_t)(nptr-nptr0):0);
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
		set_index(umbrella?nsize:0);
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
	debug("*nptr=%d,index=%zd",(int)*nptr,(size_t)(nptr-nptr0));
	if(unlikely(!umbrella&&np==nbuf)){
		set_index(0);
		RZSU();
	}
	if(nptr+1<endp&&(*nptr|32)==(base==10?'e':'p')){
		++nptr;
		expo=expr_internal_strtoz(nptr,(size_t)(endp-nptr),&npsize,0);
		if(unlikely(!npsize)){
			--nptr;
			goto no_expo;
		}
		set_index((size_t)((size_t)(nptr-nptr0)+npsize));
		--nptr;
	}else {
		expo=0;
		extra2=0;
no_expo:
		//strtoz returns 0 on fail. need not set again.
		set_index((size_t)(nptr-nptr0));
	}
	npsize=(size_t)(np-nbuf);
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
			e2=-1078+1-(ssize_t)npsize;
			kd=1024+1-(ssize_t)npsize;
			break;
		case 10:
			e2=-324+1-(ssize_t)npsize;
			kd=308+1-(ssize_t)npsize;
			break;
		case 16:
			e2=-1078+4-(ssize_t)npsize*4;
			kd=1024+4-(ssize_t)npsize*4;
			break;
			//+1/+4 from -(npsize-1)
		default:
			__builtin_unreachable();
	}
	debug("expo=%zd,npsize-1=%zd,e2=%zd,kd=%zd",expo,(ssize_t)(npsize-1),e2,kd);
	if(unlikely(expo<e2)){
		return negative?-0.0:0.0;
	}
	if(unlikely(expo>kd)){
e2inf:
		return negative?-INFINITY:INFINITY;
	}
	npsize=expr_extint_ascii_convert(buf,nbuf,npsize,base,wspace);
	//npsize use for buf after here.
	if(expo>=0){
		if(expo)
			npsize=expr_extint_mulnp(buf,npsize,base==16?2:(uint32_t)base,(uint32_t)expo,wspace);
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
			wsize=expr_extint_left(wspace,frsize,(size_t)kd);
			cmp_correct(buf,npsize,wspace,wsize,expr_extint_left);
		}else if(kd<0){
			memcpy(wspace,buf,npsize*sizeof(uint64_t));
			wsize=expr_extint_left(wspace,npsize,(size_t)-kd);
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
	if((val.uval>>52)==2){
		val.uval>>=1;
		++e2;
	}
	if(e2>1023)
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
#define expr_allocator(size) (xmtl->allocate((size),xmtl->arg))
#define expr_reallocator(old,size) (xmtl->reallocate((old),(size),xmtl->arg))
#define expr_deallocator(old) (xmtl->deallocate((old),xmtl->arg))
#define xmtl mtl
double expr_internal_strtod_mtl_auto(const char *restrict nptr,size_t nsize,size_t *restrict end_index,const struct expr_memtool *restrict mtl){
	struct expr_strtod_workspace *ws;
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
double expr_internal_strtod_mtl_force(const char *restrict nptr,size_t nsize,size_t *restrict end_index,const struct expr_memtool *restrict mtl){
	struct expr_strtod_workspace *ws;
	ws=xmalloc(sizeof(struct expr_strtod_workspace));
	if(ws){
		double r;
		r=expr_internal_strtod4(nptr,nsize,end_index,ws);
		xfree(ws);
		return r;
	}else {
		set_index(SIZE_MAX);
		return 0.0;
	}
}
double expr_internal_trunc(double x){
	union expr_double val;
	ssize_t index;
	val.val=x;
	index=(ssize_t)(1023+52)-(ssize_t)val.rd.exp;
	if(unlikely(index<=0))
		return val.val;
	if(unlikely(index>52))
		return val.rd.sign?-0.0:0.0;
	val.uval&=(UINT64_MAX<<(size_t)index);
	return val.val;
}
#define fetch_base_exp(v,b,e) ({\
	if(v.rd.exp){\
		b=(uint64_t)v.rd.base|(UINT64_C(1)<<52);\
		e=(int)v.rd.exp-(1023+52);\
	}else {\
		off=clz64(v.rd.base)-11;\
		b=(uint64_t)v.rd.base<<off;\
		e=-(1023+52-1)-off;\
		debug("off=%d",off);\
	}\
})
#define build_double(ed,mx,ex,sign) ({\
	off=clz64(mx)-11;\
	debug("mx=%lu,clz64(mx)=%d",mx,off+11);\
	if(off){\
		mx<<=off;\
		ex-=off;\
	}\
	ex+=52;\
	if(ex>-1023){\
		ed.uval=(((uint64_t)!!(sign))<<63)\
			|((uint64_t)(ex+1023)<<52)\
			|(mx&(UINT64_MAX>>12));\
	}else {\
		off=-1022-ex;\
		ed.uval=(((uint64_t)!!(sign))<<63)\
			|((mx>>off)&(UINT64_MAX>>12));\
	}\
})
#define uzero(u) (!((u)&(UINT64_MAX>>1)))
double expr_internal_fmod(double x,double y){
	uint64_t mx,my;
	int ex,ey,off;
	union expr_double vx,vy;
	vx.val=x;
	vy.val=y;
	if(unlikely(vx.rd.exp==2047)){
		return NAN;
	}
	if(unlikely(uzero(vy.uval))){
		return NAN;
	}
	if(unlikely(vy.rd.exp==2047)){
		return vy.rd.base?NAN:vx.val;
	}
	if(unlikely(uzero(vx.uval))){
		return vx.val;
	}
	fetch_base_exp(vx,mx,ex);
	fetch_base_exp(vy,my,ey);
	debug("mx=%lu,ex=%d,my=%lu,ey=%d",mx,ex,my,ey);
	if(ex<ey)
		return vx.val;
	ex-=ey;
	if(ex)do {
		if(mx>=my)
			mx-=my;
		mx<<=1;
	}while(--ex);
	if(mx>=my)
		mx-=my;
	if(!mx){
		vx.uval&=(UINT64_C(1)<<63);
		return vx.val;
	}
	build_double(vx,mx,ey,vx.rd.sign);
	return vx.val;
}
#define FBIT_COMMON(xz,yz,xl,yl) \
	uint64_t mx,my;\
	int ex,ey,off;\
	union expr_double vx,vy;\
	vx.val=x;\
	vy.val=y;\
	if(unlikely(uzero(vx.uval))){\
		return (xz);\
	}\
	if(unlikely(uzero(vy.uval))){\
		return (yz);\
	}\
	if(unlikely(vx.rd.exp==2047)){\
		return (xl);\
	}\
	if(unlikely(vy.rd.exp==2047)){\
		return (yl);\
	}\
	fetch_base_exp(vx,mx,ex);\
	fetch_base_exp(vy,my,ey);\
	debug("mx=%lu,ex=%d,my=%lu,ey=%d",mx,ex,my,ey);\
	if(ex<ey){\
		off=ey-ex;\
		if(off>=64)\
			mx=0;\
		else\
			mx>>=off;\
		ex=ey;\
	}else {\
		off=ex-ey;\
		if(off>=64)\
			my=0;\
		else\
			my>>=off;\
	}
#define FAND_SPEC(val) ((vx.rd.sign&&vy.rd.sign)?-(val):(val))
#define FOR_SPEC(val) ((vx.rd.sign||vy.rd.sign)?-(val):(val))
#define FXOR_SPEC(val) ((vx.rd.sign!=vy.rd.sign)?-(val):(val))
double expr_internal_fand(double x,double y){
	FBIT_COMMON(
		FAND_SPEC(vy.rd.exp==2047&&vy.rd.base?NAN:0.0),
		FAND_SPEC(vx.rd.exp==2047&&vx.rd.base?NAN:0.0),
		FXOR_SPEC(vx.rd.base?NAN:(vy.rd.exp==2047?(vy.rd.base?NAN:INFINITY):0.0)),
		FXOR_SPEC(vy.rd.base?NAN:(vx.rd.exp==2047?(vx.rd.base?NAN:INFINITY):0.0))
	);
	mx&=my;
	if(!mx){
		return FAND_SPEC(0.0);
	}
	build_double(vx,mx,ex,vx.rd.sign&&vy.rd.sign);
	return vx.val;
}
#define FOR_NEXT(vx,vy) ({if(vx.rd.sign)vy.rd.sign=1;vy.val;})
#define FXOR_NEXT(vx,vy) ({if(vx.rd.sign)vy.rd.sign^=1;vy.val;})
double expr_internal_for(double x,double y){
	FBIT_COMMON(
		FOR_NEXT(vx,vy),
		FOR_NEXT(vy,vx),
		FOR_SPEC(vx.rd.base?NAN:(vy.rd.exp==2047&&vy.rd.base?NAN:INFINITY)),
		FOR_SPEC(vy.rd.base?NAN:(vx.rd.exp==2047&&vx.rd.base?NAN:INFINITY))
	);
	mx|=my;
	build_double(vx,mx,ex,vx.rd.sign||vy.rd.sign);
	return vx.val;
}
double expr_internal_fxor(double x,double y){
	FBIT_COMMON(
		FXOR_NEXT(vx,vy),
		FXOR_NEXT(vy,vx),
		FXOR_SPEC(vx.rd.base?NAN:(vy.rd.exp==2047?(vy.rd.base?NAN:0.0):INFINITY)),
		FXOR_SPEC(vy.rd.base?NAN:(vx.rd.exp==2047?(vx.rd.base?NAN:0.0):INFINITY))
	);
	mx^=my;
	if(!mx){
		return FXOR_SPEC(0.0);
	}
	build_double(vx,mx,ex,vx.rd.sign!=vy.rd.sign);
	return vx.val;
}
double expr_internal_sqrt(double x){
	uint64_t mx;	
	int ex,off;
	union expr_double vx;
	vx.val=x;
	if(unlikely(uzero(vx.uval))){
		return vx.val;
	}
	if(unlikely(vx.rd.sign)){
		return -NAN;
	}
	if(unlikely(vx.rd.exp==2047)){
		return vx.val;
	}
	fetch_base_exp(vx,mx,ex);
	if(ex&1){
		mx<<=1;
		--ex;
	}
	ex=(ex-52)/2;
	mx=expr_internal_sqrt128(mx<<52,mx>>12);
	if(mx>(UINT64_MAX>>11)){
		mx>>=1;
		++ex;
	}
	build_double(vx,mx,ex,0);
	return vx.val;
}
uint64_t expr_internal_sqrt64(uint64_t x){
	uint64_t bit,res,t;
	if(unlikely(!x))
		return 0;
	bit=UINT64_C(1)<<((63-clz64(x))&~1);
	res=0;
	do {
		t=res+bit;
		if(t<=x){
			x-=t;
			res=(res>>1)|bit;
		}else {
			res>>=1;
		}
		bit>>=2;
	}while(bit);
	return res;
}
uint64_t expr_internal_sqrt128(uint64_t low,uint64_t high){
	uint64_t bh,bl,rh,rl,th,tl;
	if(unlikely(!high)){
		return expr_internal_sqrt64(low);
	}
	bh=UINT64_C(1)<<((63-clz64(high))&~1);
	bl=0;
	rh=0;
	rl=0;
	do {
		th=rh+bh;
		tl=rl+bl;
		if(tl<bl)
			++th;
		if(th<high||(th==high&&tl<=low)){
			high-=th;
			if(low<tl)
				--high;
			low-=tl;
			rl=((rh&1)<<63)|(rl>>1);
			rh>>=1;
			rh|=bh;
			rl|=bl;
		}else {
			rl=((rh&1)<<63)|(rl>>1);
			rh>>=1;
		}
		bl=((bh&3)<<62)|(bl>>2);
		bh>>=2;
	}while(bl||bh);
	return rl;
}
//WARNING: the pow is worse than libc pow. for no-libc mode only
double expr_internal_pow(double x,double y){
	double r;
	union expr_double vx,vy;
	uint64_t y64;
	int neg,off;
	vx.val=x;
	vy.val=y;
	if(unlikely(vx.rd.exp==2047&&vx.rd.base)){
		return NAN;
	}
	if(unlikely(vy.rd.exp==2047)){
		if(vy.rd.base)
			return NAN;
yinf:
		if(vx.rd.exp<1023)
			return vy.rd.sign?INFINITY:0.0;
		if(vx.rd.exp==1023&&!vx.rd.base)
			return 1.0;
		return vy.rd.sign?0.0:INFINITY;
	}
	if(unlikely(!(vy.uval&(UINT64_MAX>>1)))){
		return 1.0;
	}
	if(unlikely(vx.rd.exp==2047)){
		return vy.rd.base?NAN:INFINITY;
	}
	if(unlikely(vy.val>9007199254740991.0)){
		goto yinf;
	}
	neg=vy.rd.sign;
	vy.rd.sign=0;
	y64=(uint64_t)vy.val;
	vy.val-=(double)y64;
	r=1.0;
	if(y64){
		for(;;){
			if(y64&1){
				r*=x;
			}
			y64>>=1;
			if(!y64)
				break;
			x=x*x;
		}
	}
	if(!uzero(vx.uval)){
		uint64_t my;
		int ey;
		fetch_base_exp(vy,my,ey);
		for(ey=-52-ey;ey>0;--ey)
			vx.val=expr_internal_sqrt(vx.val);
		debug("r=%lf,vx.val=%lf,my=%lx",r,vx.val,my);
		r*=vx.val;
		my<<=12;
		while(my){
			vx.val=expr_internal_sqrt(vx.val);
			if(my&(UINT64_C(1)<<63))
				r*=vx.val;
			debug("r=%lf,vx.val=%lf,my=%lx",r,vx.val,my);
			my<<=1;
		}
	}
	return neg?1.0/r:r;
}
double expr_internal_fabs(double x){
	union expr_double val;
	val.val=x;
	val.uval&=UINT64_MAX>>1;
	return val.val;
}

#define PALIGN __SIZEOF_POINTER__

#if defined(__aarch64__)
#define BYTE_COPY_ALIGN8P(d,s,n) __asm__ volatile(\
		".L0%=:\n\t"\
		"ldp q0,q1,[%1],#32\n\t"\
		"stp q0,q1,[%0],#32\n\t"\
		"ldp q0,q1,[%1],#32\n\t"\
		"stp q0,q1,[%0],#32\n\t"\
		"subs %2,%2,#64\n\t"\
		"b.ne .L0%="\
		:"+r"(d),"+r"(s),"+r"(n)\
		:\
		:"v0","v1","memory","cc"\
)
#define BYTE_COPY_ALIGN8PK(d,s) __asm__ volatile(\
		"ldp q0,q1,[%1]\n\t"\
		"stp q0,q1,[%0]\n\t"\
		"ldp q0,q1,[%1,#32]\n\t"\
		"stp q0,q1,[%0,#32]\n\t"\
		:\
		:"r"((void *)d),"r"(s)\
		:"v0","v1","memory","cc"\
)
#define BYTE_COPY_ALIGN4P(d,s) __asm__ volatile(\
		"ldp q0,q1,[%1],#32\n\t"\
		"stp q0,q1,[%0],#32\n\t"\
		:"+r"(d),"+r"(s)\
		:\
		:"v0","v1","memory","cc"\
)
#define BYTE_COPY_ALIGN4PK(d,s) __asm__ volatile(\
		"ldp q0,q1,[%1]\n\t"\
		"stp q0,q1,[%0]\n\t"\
		:\
		:"r"(d),"r"(s)\
		:"v0","v1","memory","cc"\
)
#define BYTE_SET_ALIGN4P(d,s,n) __asm__ volatile(\
		"ldp q0,q1,[%2]\n\t"\
		".L0%=:\n\t"\
		"stp q0,q1,[%0],#32\n\t"\
		"subs %1,%1,#32\n\t"\
		"b.ne .L0%="\
		:"+r"(d),"+r"(n)\
		:"r"(s)\
		:"v0","v1","memory","cc"\
)
#define BYTE_COPY_ALIGN2P(d,s) __asm__ volatile(\
		"ldr q0,[%1],#16\n\t"\
		"str q0,[%0],#16\n\t"\
		:"+r"(d),"+r"(s)\
		:\
		:"v0","memory","cc"\
)
#define BYTE_COPY_ALIGN2PK(d,s) __asm__ volatile(\
		"ldr q0,[%1]\n\t"\
		"str q0,[%0]\n\t"\
		:\
		:"r"(d),"r"(s)\
		:"v0","memory","cc"\
)
#define BYTE_SET_ALIGN2P(d,s) __asm__ volatile(\
		"ldr q0,[%1]\n\t"\
		"str q0,[%0],#16\n\t"\
		:"+r"(d)\
		:"r"(s)\
		:"v0","memory","cc"\
)
#define BYTE_COPY_ALIGN1P(d,s) __asm__ volatile(\
		"ldr x0,[%1],#8\n\t"\
		"str x0,[%0],#8\n\t"\
		:"+r"(d),"+r"(s)\
		:\
		:"x0","memory","cc"\
)
#define BYTE_COPY_ALIGN1PK(d,s) __asm__ volatile(\
		"ldr x0,[%1]\n\t"\
		"str x0,[%0]\n\t"\
		:\
		:"r"(d),"r"(s)\
		:"x0","memory","cc"\
)
#define BYTE_SET(d,s,n) __asm__ volatile(\
		"ldrb w0,[%2]\n\t"\
		".L0%=:\n\t"\
		"strb w0,[%0],#1\n\t"\
		"subs %1,%1,#1\n\t"\
		"b.ne .L0%="\
		:"+r"(d),"+r"(n)\
		:"r"(s)\
		:"x0","x1","memory","cc"\
)
#define BYTE_COPY_ALIGN32K BYTE_COPY_ALIGN4PK
#define BYTE_COPY_ALIGN8K BYTE_COPY_ALIGN1PK
#elif defined(__x86_64__)
#define BYTE_COPY_ALIGN8P(d,s,n) __asm__ volatile(\
		".L0%=:\n\t"\
		"movdqu (%1),%%xmm0\n\t"\
		"movdqu 16(%1),%%xmm1\n\t"\
		"movups %%xmm0,(%0)\n\t"\
		"movups %%xmm1,16(%0)\n\t"\
		"movdqu 32(%1),%%xmm0\n\t"\
		"movdqu 48(%1),%%xmm1\n\t"\
		"movups %%xmm0,32(%0)\n\t"\
		"movups %%xmm1,48(%0)\n\t"\
		"add $64,%0\n\t"\
		"add $64,%1\n\t"\
		"sub $64,%2\n\t"\
		"jnz .L0%="\
		:"+r"(d),"+r"(s),"+r"(n)\
		:\
		:"xmm0","xmm1","memory","cc"\
)
#define BYTE_COPY_ALIGN8PK(d,s) __asm__ volatile(\
		"movdqu (%1),%%xmm0\n\t"\
		"movdqu 16(%1),%%xmm1\n\t"\
		"movups %%xmm0,(%0)\n\t"\
		"movups %%xmm1,16(%0)\n\t"\
		"movdqu 32(%1),%%xmm0\n\t"\
		"movdqu 48(%1),%%xmm1\n\t"\
		"movups %%xmm0,32(%0)\n\t"\
		"movups %%xmm1,48(%0)\n\t"\
		:\
		:"r"(d),"r"(s)\
		:"xmm0","xmm1","memory","cc"\
)
#define BYTE_SET_ALIGN4P(d,s,n) __asm__ volatile(\
		"movdqu (%2),%%xmm0\n\t"\
		".L0%=:\n\t"\
		"movups %%xmm0,(%0)\n\t"\
		"movups %%xmm0,16(%0)\n\t"\
		"add $32,%0\n\t"\
		"sub $32,%1\n\t"\
		"jnz .L0%="\
		:"+r"(d),"+r"(n)\
		:"r"(s)\
		:"xmm0","memory","cc"\
)
#define BYTE_COPY_ALIGN4P(d,s) __asm__ volatile(\
		"movdqu (%1),%%xmm0\n\t"\
		"movdqu 16(%1),%%xmm1\n\t"\
		"movups %%xmm0,(%0)\n\t"\
		"movups %%xmm1,16(%0)\n\t"\
		"add $32,%0\n\t"\
		"add $32,%1\n\t"\
		:"+r"(d),"+r"(s)\
		:\
		:"xmm0","xmm1","memory","cc"\
)
#define BYTE_COPY_ALIGN4PK(d,s) __asm__ volatile(\
		"movdqu (%1),%%xmm0\n\t"\
		"movdqu 16(%1),%%xmm1\n\t"\
		"movups %%xmm0,(%0)\n\t"\
		"movups %%xmm1,16(%0)\n\t"\
		:\
		:"r"(d),"r"(s)\
		:"xmm0","xmm1","memory","cc"\
)
#define BYTE_COPY_ALIGN2P(d,s) __asm__ volatile(\
		"movdqu (%1),%%xmm0\n\t"\
		"movups %%xmm0,(%0)\n\t"\
		"add $16,%0\n\t"\
		"add $16,%1\n\t"\
		:"+r"(d),"+r"(s)\
		:\
		:"xmm0","memory","cc"\
)
#define BYTE_COPY_ALIGN2PK(d,s) __asm__ volatile(\
		"movdqu (%1),%%xmm0\n\t"\
		"movups %%xmm0,(%0)"\
		:\
		:"r"(d),"r"(s)\
		:"xmm0","memory","cc"\
)
#define BYTE_SET_ALIGN2P(d,s) __asm__ volatile(\
		"movdqu (%1),%%xmm0\n\t"\
		"movups %%xmm0,(%0)\n\t"\
		"add $16,%0\n\t"\
		:"+r"(d)\
		:"r"(s)\
		:"xmm0","memory","cc"\
)
#define BYTE_COPY_ALIGN1P(d,s) __asm__ volatile(\
		"movq (%1),%%xmm0\n\t"\
		"movq %%xmm0,(%0)\n\t"\
		"add $8,%0\n\t"\
		"add $8,%1\n\t"\
		:"+r"(d),"+r"(s)\
		:\
		:"xmm0","memory","cc"\
)
#define BYTE_COPY_ALIGN1PK(d,s) __asm__ volatile(\
		"movq (%1),%%xmm0\n\t"\
		"movq %%xmm0,(%0)"\
		:\
		:"r"(d),"r"(s)\
		:"xmm0","memory","cc"\
)
#define BYTE_SET(d,s,n) __asm__ volatile(\
		"movb (%2),%%al\n\t"\
		".L0%=:\n\t"\
		"movb %%al,(%0)\n\t"\
		"add $1,%0\n\t"\
		"sub $1,%1\n\t"\
		"jnz .L0%="\
		:"+r"(d),"+r"(n)\
		:"r"(s)\
		:"rax","memory","cc"\
)
#define BYTE_COPY_ALIGN32K BYTE_COPY_ALIGN4PK
#define BYTE_COPY_ALIGN8K BYTE_COPY_ALIGN1PK
#else
#define BYTE_COPY_ALIGN8P(d,s,n) do {\
	(*(uintptr_t *)(d))=(*(uintptr_t *)(s));\
	((uintptr_t *)(d))[1]=((uintptr_t *)(s))[1];\
	((uintptr_t *)(d))[2]=((uintptr_t *)(s))[2];\
	((uintptr_t *)(d))[3]=((uintptr_t *)(s))[3];\
	((uintptr_t *)(d))[4]=((uintptr_t *)(s))[4];\
	((uintptr_t *)(d))[5]=((uintptr_t *)(s))[5];\
	((uintptr_t *)(d))[6]=((uintptr_t *)(s))[6];\
	((uintptr_t *)(d))[7]=((uintptr_t *)(s))[7];\
	d=(void *)((uintptr_t)(d)+8*PALIGN);\
	s=(void *)((uintptr_t)(s)+8*PALIGN);\
	n-=8*PALIGN;\
}while(n);
#define BYTE_COPY_ALIGN8PK(d,s) ({\
	(*(uintptr_t *)(d))=(*(uintptr_t *)(s));\
	((uintptr_t *)(d))[1]=((uintptr_t *)(s))[1];\
	((uintptr_t *)(d))[2]=((uintptr_t *)(s))[2];\
	((uintptr_t *)(d))[3]=((uintptr_t *)(s))[3];\
	((uintptr_t *)(d))[4]=((uintptr_t *)(s))[4];\
	((uintptr_t *)(d))[5]=((uintptr_t *)(s))[5];\
	((uintptr_t *)(d))[6]=((uintptr_t *)(s))[6];\
	((uintptr_t *)(d))[7]=((uintptr_t *)(s))[7];\
})
#define BYTE_COPY_ALIGN4P(d,s) ({\
	(*(uintptr_t *)(d))=(*(uintptr_t *)(s));\
	((uintptr_t *)(d))[1]=((uintptr_t *)(s))[1];\
	((uintptr_t *)(d))[2]=((uintptr_t *)(s))[2];\
	((uintptr_t *)(d))[3]=((uintptr_t *)(s))[3];\
	d=(void *)((uintptr_t)(d)+4*PALIGN);\
	s=(void *)((uintptr_t)(s)+4*PALIGN);\
})
#define BYTE_COPY_ALIGN4PK(d,s) ({\
	(*(uintptr_t *)(d))=(*(uintptr_t *)(s));\
	((uintptr_t *)(d))[1]=((uintptr_t *)(s))[1];\
	((uintptr_t *)(d))[2]=((uintptr_t *)(s))[2];\
	((uintptr_t *)(d))[3]=((uintptr_t *)(s))[3];\
})
#define BYTE_SET_ALIGN4P(d,s,n) do {\
	(*(uintptr_t *)(d))=(*(uintptr_t *)(s));\
	((uintptr_t *)(d))[1]=(*(uintptr_t *)(s));\
	((uintptr_t *)(d))[2]=(*(uintptr_t *)(s));\
	((uintptr_t *)(d))[3]=(*(uintptr_t *)(s));\
	d=(void *)((uintptr_t)(d)+4*PALIGN);\
	n-=4*PALIGN;\
}while(n);
#define BYTE_COPY_ALIGN2P(d,s) ({\
	(*(uintptr_t *)(d))=(*(uintptr_t *)(s));\
	((uintptr_t *)(d))[1]=((uintptr_t *)(s))[1];\
	d=(void *)((uintptr_t)(d)+2*PALIGN);\
	s=(void *)((uintptr_t)(s)+2*PALIGN);\
})
#define BYTE_COPY_ALIGN2PK(d,s) ({\
	(*(uintptr_t *)(d))=(*(uintptr_t *)(s));\
	((uintptr_t *)(d))[1]=((uintptr_t *)(s))[1];\
})
#define BYTE_SET_ALIGN2P(d,s) ({\
	(*(uintptr_t *)(d))=(*(uintptr_t *)(s));\
	((uintptr_t *)(d))[1]=(*(uintptr_t *)(s));\
	d=(void *)((uintptr_t)(d)+2*PALIGN);\
})
#define BYTE_COPY_ALIGN1P(d,s) ({\
	(*(uintptr_t *)(d))=(*(uintptr_t *)(s));\
	d=(void *)((uintptr_t)(d)+PALIGN);\
	s=(void *)((uintptr_t)(s)+PALIGN);\
})
#define BYTE_COPY_ALIGN1PK(d,s) ({\
	(*(uintptr_t *)(d))=(*(uintptr_t *)(s));\
})
#define BYTE_COPY_ALIGN32K(d,s) ({\
	(*(uint64_t *)(d))=(*(uint64_t *)(s));\
	((uint64_t *)(d))[1]=((uint64_t *)(s))[1];\
	((uint64_t *)(d))[2]=((uint64_t *)(s))[2];\
	((uint64_t *)(d))[3]=((uint64_t *)(s))[3];\
})
#define BYTE_COPY_ALIGN8K(d,s) ({\
	(*(uint64_t *)(d))=(*(uint64_t *)(s));\
})
#define BYTE_SET(d,s,n) do {\
	(*(uint8_t *)(d))=(*(uint8_t *)(s));\
	d=(void *)((uintptr_t)(d)+1);\
}while(--n);
#endif

#undef buf
#define BYTE_SET_4PM1(v_len,v_bp,v_sbp) ({\
	switch(v_len){\
		case 1 ... PALIGN-1:\
			BYTE_SET(v_bp,v_sbp,v_len);\
			break;\
		case PALIGN:\
			BYTE_COPY_ALIGN1PK(v_bp,v_sbp);\
			v_bp+=PALIGN;\
			break;\
		case PALIGN+1 ... 2*PALIGN-1:\
			BYTE_COPY_ALIGN1PK(v_bp,v_sbp);\
			v_bp+=v_len;\
			BYTE_COPY_ALIGN1PK(v_bp-PALIGN,v_sbp);\
			break;\
		case 2*PALIGN:\
			BYTE_SET_ALIGN2P(v_bp,v_sbp);\
			break;\
		case 2*PALIGN+1 ... 4*PALIGN-1:\
			BYTE_COPY_ALIGN2PK(v_bp,v_sbp);\
			v_bp+=v_len;\
			BYTE_COPY_ALIGN2PK(v_bp-2*PALIGN,v_sbp);\
			break;\
		default:\
			break;\
	}\
})
void expr_internal_memset64(void *buf,uintptr_t val,size_t size){
	uint8_t *bp;
	uintptr_t end;
	size_t tmp_size;
	uint64_t val4[4];
	if(unlikely(!size))
		return;
	bp=buf;
	*val4=val;
	val4[1]=val;
	val4[2]=val;
	val4[3]=val;
	end=(uintptr_t)(4*PALIGN)-((uintptr_t)bp&(4*PALIGN-1));
	if(end!=4*PALIGN){
		if(size<end){
			BYTE_SET_4PM1(size,bp,((uint8_t *)val4));
			return;
		}
		size-=end;
		BYTE_SET_4PM1(end,bp,((uint8_t *)val4));
	}
	// dp is aligned here
	tmp_size=size&~(size_t)(PALIGN*4-1);
	if(tmp_size){
		size-=tmp_size;
		BYTE_SET_ALIGN4P(bp,val4,tmp_size);
	}
	if(size>=PALIGN*2){
		size-=PALIGN*2;
		BYTE_SET_ALIGN2P(bp,val4);
	}
	if(size>=PALIGN){
		size-=PALIGN;
		BYTE_COPY_ALIGN1PK(bp,val4);
		bp+=PALIGN;
	}
	if(size){
		*((uintptr_t *)bp)=(val&(UINTPTR_MAX>>((PALIGN-size)*8)))|(*((uintptr_t *)bp)&(UINTPTR_MAX<<((size)*8)));
	}
}
#define BYTE_COPY_4PM1(v_len,v_bp,v_sbp) ({\
	switch(v_len){\
		case 1 ... PALIGN-1:\
			do {\
				*(v_bp++)=*(v_sbp++);\
			}while(--v_len);\
			break;\
		case PALIGN:\
			BYTE_COPY_ALIGN1PK(v_bp,v_sbp);\
			v_bp+=PALIGN;\
			v_sbp+=PALIGN;\
			break;\
		case PALIGN+1 ... 2*PALIGN-1:\
			BYTE_COPY_ALIGN1PK(v_bp,v_sbp);\
			v_bp+=v_len;\
			v_sbp+=v_len;\
			BYTE_COPY_ALIGN1PK(v_bp-PALIGN,v_sbp-PALIGN);\
			break;\
		case 2*PALIGN:\
			BYTE_COPY_ALIGN2P(v_bp,v_sbp);\
			break;\
		case 2*PALIGN+1 ... 4*PALIGN-1:\
			BYTE_COPY_ALIGN2PK(v_bp,v_sbp);\
			v_bp+=v_len;\
			v_sbp+=v_len;\
			BYTE_COPY_ALIGN2PK(v_bp-2*PALIGN,v_sbp-2*PALIGN);\
			break;\
		default:\
			__builtin_unreachable();\
	}\
})
void expr_internal_memcpy(void *dst,const void *src,size_t size){
	uint8_t *bp;
	const uint8_t *sbp;
	uintptr_t end;
	size_t tmp_size;
	bp=dst;
	sbp=src;
	switch(size){
		case 0:
			return;
		case 1 ... PALIGN-1:
			for(;;){
				*bp=*sbp;
				if(unlikely(!--size))
					return;
				++bp;
				++sbp;
			}
			return;
		case PALIGN:
			BYTE_COPY_ALIGN1PK(bp,sbp);
			return;
		case PALIGN+1 ... 2*PALIGN-1:
			size-=PALIGN;
			BYTE_COPY_ALIGN1PK(bp,sbp);
			BYTE_COPY_ALIGN1PK(bp+size,sbp+size);
			return;
		case 2*PALIGN:
			BYTE_COPY_ALIGN2PK(bp,sbp);
			return;
		case 2*PALIGN+1 ... 4*PALIGN-1:
			size-=2*PALIGN;
			BYTE_COPY_ALIGN2PK(bp,sbp);
			BYTE_COPY_ALIGN2PK(bp+size,sbp+size);
			return;
		case 4*PALIGN:
			BYTE_COPY_ALIGN4PK(bp,sbp);
			return;
		case 4*PALIGN+1 ... 8*PALIGN-1:
			size-=4*PALIGN;
			BYTE_COPY_ALIGN4PK(bp,sbp);
			BYTE_COPY_ALIGN4PK(bp+size,sbp+size);
			return;
		case 8*PALIGN:
			BYTE_COPY_ALIGN8PK(bp,sbp);
			return;
		case 8*PALIGN+1 ... 16*PALIGN:
			size-=8*PALIGN;
			BYTE_COPY_ALIGN8PK(bp,sbp);
			BYTE_COPY_ALIGN8PK(bp+size,sbp+size);
			return;
		default:
			break;
	}
	end=(uintptr_t)(4*PALIGN)-((uintptr_t)bp&(4*PALIGN-1));
	if(end!=4*PALIGN){
		size-=end;
		BYTE_COPY_4PM1(end,bp,sbp);
	}
	// dp is aligned here
	tmp_size=size&~(size_t)(PALIGN*8-1);
	if(tmp_size){
		size-=tmp_size;
		BYTE_COPY_ALIGN8P(bp,sbp,tmp_size);
	}
	if(size>=PALIGN*4){
		size-=PALIGN*4;
		BYTE_COPY_ALIGN4P(bp,sbp);
	}
	if(size>=PALIGN*2){
		size-=PALIGN*2;
		BYTE_COPY_ALIGN2P(bp,sbp);
	}
	if(size>=PALIGN){
		size-=PALIGN;
		BYTE_COPY_ALIGN1P(bp,sbp);
	}
	if(size){
		end=(uintptr_t)bp+size;
		for(;;){
			*bp=*sbp;
			++bp;
			if(bp==(uint8_t *)end)
				break;
			++sbp;
		}
	}
}
void expr_internal_memmove(void *dst,const void *src,size_t size){
	uintptr_t d=(uintptr_t)dst
		,s=(uintptr_t)src
		,de,se;
	size_t v;
	if(unlikely(!size||d==s||(d|s|size)>=(size_t)PTRDIFF_MAX))
		return;
	de=d+size;
	se=s+size;
	if(s>=de||d>=se){
		expr_internal_memcpy((void *)d,(void *)s,size);
		return;
	}
	if(d<s){
		while(size>4*PALIGN){
			BYTE_COPY_ALIGN4PK(d,s);
			d+=4*PALIGN;
			s+=4*PALIGN;
			size-=4*PALIGN;
		}
		if(size==4*PALIGN){
			BYTE_COPY_ALIGN4PK(d,s);
			return;
		}
		v=(size_t)(s-d);
		while(size>v){
			expr_internal_memcpy((void *)d,(void *)s,v);
			d+=v;
			s+=v;
			size-=v;
		}
		if(size)
			expr_internal_memcpy((void *)d,(void *)s,size);
		return;
	}
	while(size>4*PALIGN){
		de-=4*PALIGN;
		se-=4*PALIGN;
		BYTE_COPY_ALIGN4PK(de,se);
		size-=4*PALIGN;
	}
	if(size==4*PALIGN){
		BYTE_COPY_ALIGN4PK(d,s);
		return;
	}
	v=(size_t)(d-s);
	while(size>v){
		de-=v;
		se-=v;
		expr_internal_memcpy((void *)de,(void *)se,v);
		size-=v;
	}
	if(size){
		expr_internal_memcpy((void *)d,(void *)s,size);
	}
	return;
}
#define SWAR(w) ({uint64_t _w=(w);~(((_w&UINT64_C(0x7f7f7f7f7f7f7f7f))+UINT64_C(0x7f7f7f7f7f7f7f7f))|_w)&UINT64_C(0x8080808080808080);})
#if (!defined(__BIG_ENDIAN__)||!(__BIG_ENDIAN__))
#define cz64 ctz64
#define crz64 clz64
#else
#define cz64 clz64
#define crz64 ctz64
#endif
#define w0 w._w0
#define w1 w._w1
#define w2 w._w2
#define w3 w._w3
void *expr_internal_memchr(const void *buf,int c,size_t size){
	const uint8_t *p=buf;
	uint64_t val=expr_mkval64(c);
	struct {
		uint64_t _w0,_w1,_w2,_w3;
	} w;
	while(size>=32){
		BYTE_COPY_ALIGN32K(&w,p);
		w0=SWAR(w0^val);
		w1=SWAR(w1^val);
		w2=SWAR(w2^val);
		w3=SWAR(w3^val);
		if(w0|w1|w2|w3){
			if(w0)
				return (void *)(uintptr_t)(p+cz64(w0)/8);
			if(w1)
				return (void *)(uintptr_t)(p+8+cz64(w1)/8);
			if(w2)
				return (void *)(uintptr_t)(p+16+cz64(w2)/8);
			return (void *)(uintptr_t)(p+24+cz64(w3)/8);
		}
		p+=32;
		size-=32;
	}
	while(size>=8){
		BYTE_COPY_ALIGN8K(&w0,p);
		w0=SWAR(w0^val);
		if(w0)
			return (void *)(uintptr_t)(p+cz64(w0)/8);
		p+=8;
		size-=8;
	}
	while(size--){
		if(*p==(uint8_t)c)
			return (void *)(uintptr_t)p;
		++p;
	}
	return NULL;
}
void *expr_internal_memrchr(const void *buf,int c,size_t size){
	const uint8_t *p=buf+size;
	uint64_t val=expr_mkval64(c);
	struct {
		uint64_t _w0,_w1,_w2,_w3;
	} w;
	while(size>=32){
		p-=32;
		BYTE_COPY_ALIGN32K(&w,p);
		w0=SWAR(w0^val);
		w1=SWAR(w1^val);
		w2=SWAR(w2^val);
		w3=SWAR(w3^val);
		if(w0|w1|w2|w3){
			if(w3)
				return (void *)(uintptr_t)(p+(24+7)-crz64(w3)/8);
			if(w2)
				return (void *)(uintptr_t)(p+(16+7)-crz64(w2)/8);
			if(w1)
				return (void *)(uintptr_t)(p+(8+7)-crz64(w1)/8);
			return (void *)(uintptr_t)(p+7-crz64(w0)/8);
		}
		size-=32;
	}
	while(size>=8){
		p-=8;
		BYTE_COPY_ALIGN8K(&w0,p);
		w0=SWAR(w0^val);
		if(w0)
			return (void *)(uintptr_t)(p+7-crz64(w0)/8);
		size-=8;
	}
	while(size--){
		--p;
		if(*p==(uint8_t)c)
			return (void *)(uintptr_t)p;
	}
	return NULL;
}
#define w01 w_1._w0
#define w11 w_1._w1
#define w21 w_1._w2
#define w31 w_1._w3
int expr_internal_memcmp(const void *s0,const void *s1,size_t size){
	const uint8_t *p=s0;
	const uint8_t *p1=s1;
	uint64_t d;
	int r;
	struct {
		uint64_t _w0,_w1,_w2,_w3;
	} w,w_1;
	while(size>=32){
		BYTE_COPY_ALIGN32K(&w,p);
		BYTE_COPY_ALIGN32K(&w_1,p1);
#define TEST(x,y,off) \
		d=x^y;\
		if(d){\
			r=cz64(d)/8;\
			return (int)p[r off ]-(int)p1[r off ];\
		}
		TEST(w0,w01,);
		TEST(w1,w11,+8);
		TEST(w2,w21,+16);
		TEST(w3,w31,+24);
		p+=32;
		p1+=32;
		size-=32;
	}
	while(size>=8){
		BYTE_COPY_ALIGN8K(&w0,p);
		BYTE_COPY_ALIGN8K(&w01,p1);
		TEST(w0,w01,);
		p+=8;
		p1+=8;
		size-=8;
	}
	while(size--){
		r=(int)*p-(int)*p1;
		if(r)
			return r;
		++p;
		++p1;
	}
	return 0;
}
#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif
void *expr_internal_memchr2(const void *buf,int c){
	const uint8_t *p=buf,*s=buf,*q;
	size_t ava;
	ava=PAGE_SIZE-((uintptr_t)p&(PAGE_SIZE-1));
	if(ava){
		q=expr_internal_memchr(p,c,ava);
		if(q)
			return (void *)(uintptr_t)q;
		p+=ava;
	}
	for(;;){
		q=expr_internal_memchr(p,c,PAGE_SIZE);
		if(q)
			return (void *)(uintptr_t)q;
		p+=PAGE_SIZE;
	}
}
