/*******************************************************************************
 *License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>*
 *This is free software: you are free to change and redistribute it.           *
 *******************************************************************************/
#define _GNU_SOURCE
#include <math.h>
#include <stdio.h>
#include <string.h>

#define _EXPR_LIB 1
#include "expr.h"

#define c_trywrite(buf,sz) if(likely((r=(sz))>0)){\
	r=writer(fd,(buf),r);\
	if(unlikely(r<0))\
		return r;\
	sum+=r;\
}
#define c_trywriteext(c,sz) {\
	r=writeext(writer,fd,(sz),(c));\
	if(unlikely(r<0))\
		return r;\
	sum+=r;\
}
#define EXPR_WCBUFSIZE 512
ssize_t expr_writec(expr_writer writer,intptr_t fd,size_t count,int c){
	char buf[EXPR_WCBUFSIZE];
	ssize_t r,sum;
	if(unlikely(count>SSIZE_MAX))
		return 0;
	if(count<=EXPR_WCBUFSIZE){
		memset(buf,c,count);
		return writer(fd,buf,count);
	}
	memset(buf,c,EXPR_WCBUFSIZE);
	sum=0;
	do {
		c_trywrite(buf,EXPR_WCBUFSIZE);
		count-=EXPR_WCBUFSIZE;
	}while(count>EXPR_WCBUFSIZE);
	if(count){
		c_trywrite(buf,count);
	}
	return sum;
}
#define writeext expr_writec

#define flag_plusorspace(_f) (*(_f)->bit&(UINT64_C(3)<<62))
#define flag_width(_f,_dflt) ((_f)->width_set?(_f)->width:(_dflt))
#define flag_digit(_f,_dflt) ((_f)->digit_set?(size_t)(_f)->digit:(_dflt))

#define cwrite_common0(_s) \
	ext=(ssize_t)flag_width(flag,0)-sz;\
	if(ext>0){\
		sum=0;\
		if(flag->minus){\
			c_trywrite((_s),sz);\
			c_trywriteext((flag->zero)?'0':' ',ext);\
		}else {\
			c_trywriteext((flag->zero)?'0':' ',ext);\
			c_trywrite((_s),sz);\
		}\
		return sum;\
	}\
	return (likely(sz>0)?writer(fd,(_s),sz):0)

#define cwrite_common(_s,_sz) \
	sz=(_sz);\
	cwrite_common0(_s)

ssize_t expr_converter_common(expr_writer writer,intptr_t fd,const void *buf,size_t size,const struct expr_writeflag *flag){
	ssize_t ext,r,sum;
#define sz ((ssize_t)size)
	cwrite_common0(buf);
#undef sz
}
#define write_val10_to_p(val) \
	do {\
		*(--p)=(char)('0'+val%10);\
		val/=10;\
	}while(val)
static ssize_t converter_d(expr_writer writer,intptr_t fd,const union expr_argf *arg,struct expr_writeflag *flag){
	char nbuf[32];
	intptr_t val=arg->sint;
	char *endp=nbuf+32;
	char *p=endp;
	ssize_t ext,sum,r,sz;
	int minus;
	if(!val)
		minus=2;
	else if(val<0){
		val=-val;
		minus=1;
	}else
		minus=0;
	if(val){
		write_val10_to_p(val);
	}else
		*(--p)='0';
	switch(minus){
		case 0:
			if(flag_plusorspace(flag))
				*(--p)='+';
			break;
		case 1:
			*(--p)='-';
			break;
		case 2:
			if(flag->plus)
				*(--p)='+';
			break;
		default:
			__builtin_unreachable();
	}
	cwrite_common(p,endp-p);
}
static ssize_t converter_size(expr_writer writer,intptr_t fd,const union expr_argf *arg,struct expr_writeflag *flag){
	static const char dimc[]={"bkmgtpe"};
	static const char dimC[]={"BKMGTPE"};
	char nbuf[64];
	char dbuf[32];
	int64_t val=(int64_t)arg->sint;
	char *endp=nbuf+32;
	char *p=endp;
	ssize_t ext,sum,r,sz;
	int minus,dim;
	if(!val)
		minus=2;
	else if(val<0){
		val=-val;
		minus=1;
	}else
		minus=0;
	if(val){
		int64_t sdimm1;
		size_t dig;
		dim=(63-clz64((uint64_t)val))/10;
		if(dim||flag->sharp)
			*(--p)=(flag->cap?dimC:dimc)[dim];
		dim*=10;
		dig=(size_t)flag_digit(flag,0);
		if(unlikely(dig>32))
			dig=32;
		if(dig){
			char *dp,*dp_end;
			int64_t r;
			int feq;
			feq=!!flag->eq;
			dp=dbuf;
			dp_end=dbuf+dig;
			sdimm1=(INT64_C(1)<<dim)-1;
			r=val&sdimm1;
			if(!r&&feq)
				goto no_digit;
			do {
				r*=10;
				*(dp++)=(char)('0'+(r>>dim));
				r&=sdimm1;
				if(!r&&feq){
					dig=dp-dbuf;
					break;
				}
			}while(dp<dp_end);
			p-=dig;
			memcpy(p,dbuf,dig);
			*(--p)='.';
		}
no_digit:
		val>>=dim;
		write_val10_to_p(val);
	}else
		*(--p)='0';
	switch(minus){
		case 0:
			if(flag_plusorspace(flag))
				*(--p)='+';
			break;
		case 1:
			*(--p)='-';
			break;
		case 2:
			if(flag->plus)
				*(--p)='+';
			break;
		default:
			__builtin_unreachable();
	}
	cwrite_common(p,endp-p);
}
#define conv_x(name,base,bufsz) \
static ssize_t converter_##name(expr_writer writer,intptr_t fd,const union expr_argf *arg,struct expr_writeflag *flag){\
	char nbuf[bufsz];\
	uintptr_t val=arg->uint;\
	char *endp=nbuf+bufsz;\
	char *p=endp;\
	ssize_t ext,sum,r,sz;\
	int positive;\
	const char *conv_str=flag->cap?conv_btoX:conv_btox;\
	if(val){\
		positive=1;\
		do {\
			*(--p)=(char)conv_str[val%base];\
			val/=base;\
		}while(val);\
	}else {\
		positive=0;\
		*(--p)='0';\
	}\
	if(base==16){\
		if(flag->sharp){\
			*(--p)=conv_str[base];\
			*(--p)='0';\
		}\
	}\
	switch(positive){\
		case 1:\
			if(flag_plusorspace(flag))\
				*(--p)='+';\
			break;\
		case 0:\
			if(flag->plus)\
				*(--p)='+';\
			break;\
		default:\
			__builtin_unreachable();\
	}\
	cwrite_common(p,endp-p);\
}
static const char conv_btox[]={"0123456789abcdefx"};
static const char conv_btoX[]={"0123456789ABCDEFX"};
conv_x(x81,16,32);
conv_x(x82,2,72);
conv_x(x83,3,48);
conv_x(x84,4,48);
conv_x(x85,5,32);
conv_x(x86,6,32);
conv_x(x87,7,32);
conv_x(x88,8,32);
conv_x(x89,9,32);
conv_x(x8a,10,32);
conv_x(x8b,11,32);
conv_x(x8c,12,32);
conv_x(x8d,13,32);
conv_x(x8e,14,32);
conv_x(x8f,15,32);

static size_t extint_left(uint64_t *buf,size_t size,uint64_t bits){
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
static size_t extint_right(uint64_t *buf,size_t size,uint64_t bits){
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
static size_t extint_add(uint64_t *buf,uint64_t addend){
	uint64_t *restrict p=buf;
	do {
		addend=((*(p++)+=addend)<addend);
	}while(addend);
	return p-buf;
}
static size_t extint_mul(uint64_t *buf,size_t size,uint32_t factor,uint64_t *workspace){
	uint32_t *restrict p;
	uint32_t *restrict wp;
	union {
		uint64_t v;
		size_t size;
	} un;
	buf[size]=0;
	size_t size32=size<<1,csize32;
	for(p=(uint32_t *)buf,wp=(uint32_t *)workspace;
			(p-(uint32_t *)buf)<size32;
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
				extint_add((uint64_t *)(p+1),*wp);
		else {
			*wp=((p[1]+=*wp)<*wp);
			if(*wp){
			un.size=(un.size>>1)+1+
				extint_add((uint64_t *)(p+2),*wp);
			}else continue;
		}
		if(un.size>size){
			size=un.size;
			size32=size<<1;
		}
	}
	return size;
}
static size_t extint_div(uint64_t *buf,size_t size,uint32_t divisor,uint32_t *mod){
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
static void extint_mirror(char *buf,size_t size){
	char *out=buf+size-1;
	register char swapbuf;
	while(out>buf){
		//printf("swap %c,%c\n",*buf,*out);
		swapbuf=*out;
		*out=*buf;
		*buf=swapbuf;
		--out;
		++buf;
	}
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
		size=extint_div(buf,size,ds,&mod);\
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
static size_t extint_ascii_rev(uint64_t *buf,size_t size,const char *chars,uint32_t base,char *outbuf){
	write_ascii(--out,outbuf-out);
}
#define conv_f0s(_start,_name,_base,_shift,_nsize,_how,_before_how) \
static ssize_t converter_##_name(expr_writer writer,intptr_t fd,const union expr_argf *arg,struct expr_writeflag *flag){\
	expr_static_assert(__builtin_constant_p((_nsize)));\
	double val=arg->dbl;\
	struct {\
		uint64_t w[69];\
		char v[1096];\
		uint64_t i[69-16];\
		uint64_t ii[16];\
		char n[_nsize];\
	} bufst;\
	char *endp;\
	char *p,*np;\
	int positive,f61,f58;\
	ssize_t ext,sum,r,sz,fsz,ds;\
	size_t digit,width;\
	const char *_conv_str=flag->cap?conv_btoX:conv_btox;\
	_start;\
	if(unlikely(!expr_isfinite(val))){\
		p=nbuf;\
		write_sign_to_p(EXPR_EDSIGN(&val));\
		if(flag->cap)\
			*(uint32_t *)p=*(const uint32_t *)(expr_isinf(val)?"INF":"NAN");\
		else\
			*(uint32_t *)p=*(const uint32_t *)(expr_isinf(val)?"inf":"nan");\
		cwrite_common(nbuf,(ssize_t)(p==nbuf?3:4));\
	}\
	endp=nbuf+_nsize;\
	np=endp;\
	if(EXPR_EDSIGN(&val)){\
		EXPR_EDSIGN(&val)=0;\
		positive=0;\
	}else {\
		positive=1;\
	}\
	sum=0;\
	sz=(ssize_t)EXPR_EDEXP(&val)-1023;\
	if(sz>=0){\
		double fval;\
		fval=floor(val);\
		val-=fval;\
		*iival=EXPR_EDBASE(&fval)|(UINT64_C(1)<<52);\
		sz-=52;\
		if(sz){\
			if(sz>0){\
				r=extint_left(iival,1,sz);\
			}else {\
				*iival>>=-sz;\
				r=1;\
			}\
		}else\
			r=1;\
		np-=extint_ascii_rev(iival,r,conv_btox,_base,np);\
	}else {\
		*(--np)='0';\
	}\
	if(_base==16){\
		if(flag->sharp){\
			*(--np)=_conv_str[_base];\
			*(--np)='0';\
		}\
	}\
	write_sign_to_np(!positive);\
	ds=endp-np;\
	f61=!!(flag->minus);\
	f58=!!(flag->eq);\
	digit=flag_digit(flag,6);\
	if(digit>SSIZE_MAX-1)\
		digit=SSIZE_MAX-1;\
	width=flag_width(flag,0);\
	p=vbuf+1096;\
	if(digit){\
		fsz=(ssize_t)EXPR_EDEXP(&val);\
		if(fsz)\
			*ival=EXPR_EDBASE(&val)|(UINT64_C(1)<<52);\
		else\
			*ival=EXPR_EDBASE(&val)<<1;\
		ext=1075-fsz;\
		r=1;\
		_shift;\
		fsz=extint_ascii_rev(ival,r,_conv_str,_base,p);\
		p-=fsz;\
		ext-=fsz;\
		if(ext>0){\
			p-=ext;\
			fsz+=ext;\
			memset(p,'0',ext);\
		}\
		if(fsz){\
			*(--p)='.';\
			++fsz;\
		}\
	}else\
		fsz=0;\
	{\
		_before_how;\
		if(!f61){\
			writeext_f;\
		}\
		_how;\
		if(f61){\
			writeext_f;\
		}\
	}\
	return sum;\
}
#define conv_f0(_name,_base,_shift,_nsize,_how,_before_how) conv_f0s(,_name,_base,_shift,_nsize,_how,_before_how)
#define conv_f(_name,_base,_shift,_nsize) conv_f0(_name,_base,_shift,_nsize,{\
	c_trywrite(np,ds);\
	if(fsz){\
		c_trywrite(p,fsz);\
	}\
	if(!f58&&fsz<digit){\
		if(!fsz){\
			c_trywrite(".",1);\
			c_trywriteext('0',digit-fsz);\
		}else\
			c_trywriteext('0',digit-fsz+1);\
	}\
},\
	ext=digit+1;\
	if(fsz>ext){\
		fsz=ext;\
	}\
	for(endp=p+fsz-1;endp>=p&&*endp=='0';){\
		--fsz;\
		--endp;\
	}\
	if(fsz==1)\
		fsz=0;\
	sz=ds+(f58?fsz:ext);\
)
#define conv_fe(_name,_base,_shift,_nsize,_e) conv_f0(_name,_base,_shift,_nsize,\
	if(r1<0){\
		if(fsz){\
			c_trywrite(p,1);\
			c_trywrite(".",1);\
			if(fsz>2){\
				c_trywrite(p+1,fsz-1);\
			}else {\
				c_trywrite("0",1);\
			}\
		}\
	}else {\
		++firp;\
		c_trywrite(np,firp-np);\
		if(digit)\
			c_trywrite(".",1);\
		r1=(np+ds)-firp-iz;\
		if(r1>=digit){\
			ext=0;\
			r1=digit;\
		}else {\
			ext=digit-r1;\
		}\
		c_trywrite(firp,r1);\
		if(ext){\
			if(fsz){\
				ext=fsz>ext?ext:fsz;\
				c_trywrite(p,ext);\
			}else \
				c_trywrite("0",ext=1);\
		}\
		fsz=r1+ext;\
	}\
	if(!f58&&fsz<digit){\
		c_trywriteext('0',digit-fsz);\
	}\
	c_trywrite(ebuf,esz);\
,\
	char ebuf[8];\
	char *firp;\
	ssize_t r1;\
	ssize_t r2;\
	ssize_t esz;\
	ssize_t iz;\
	{\
	r1=ds-1;\
	sz=0;\
	if(!fsz){\
		goto nofsz;\
	}\
	++p;\
	--fsz;\
	switch(ds){\
		case 1:\
			firp=np;\
			if(*np=='0')\
				goto onzero;\
			break;\
		case 2:\
			switch(*np){\
				case '+':\
				case '-':\
					--r1;\
					firp=np+1;\
					if(np[1]=='0')\
						goto onzero;\
					break;\
				default:\
					firp=np;\
					break;\
			}\
			break;\
		default:\
nofsz:\
			switch(*np){\
				case '+':\
				case '-':\
					firp=np+1;\
					break;\
				default:\
					firp=np;\
					break;\
			}\
			break;\
onzero:\
		endp=p+fsz;\
		for(--r1;p<endp;){\
			if(*p=='0'){\
				--r1;\
				++p;\
				--fsz;\
				continue;\
			}\
			break;\
		}\
	}\
	if(fsz>digit){\
		fsz=digit;\
	}\
	for(endp=p+fsz-1;endp>=p&&*endp=='0';){\
		--fsz;\
		--endp;\
	}\
	*ebuf=(_e);\
	r2=r1;\
	if(r2<0){\
		ebuf[1]='-';\
		r2=-r2;\
	}else {\
		ebuf[1]='+';\
	}\
	sz+=2;\
	endp=ebuf+1;\
	do {\
		*(++endp)=_conv_str[r2%10];\
		++sz;\
	}while((r2/=10));\
	if(endp>ebuf+2)\
		extint_mirror(ebuf+2,endp-(ebuf+1));\
	esz=sz;\
	sz+=ds+(f58?fsz+!!digit:digit+!!digit);\
	if(!fsz)\
		++sz;\
	for(iz=0,endp=np+ds;--endp>np;){\
		if(*endp=='0')\
			++iz;\
		else\
			break;\
	}\
	sz-=iz;\
	if(!digit)\
		--sz;\
})
#define writeext_f \
	ext=(ssize_t)width-sz;\
	if(ext>0){\
		c_trywriteext((flag->zero)?'0':' ',ext);\
	}
#define write_sign_to_p(_neg) \
		if(_neg){\
			*(p++)='-';\
		}else {\
			if(flag_plusorspace(flag))\
				*(p++)='+';\
		}
#define write_sign_to_np(_neg) \
		if(_neg){\
			*(--np)='-';\
		}else {\
			if(flag_plusorspace(flag))\
				*(--np)='+';\
		}
#define vbuf (bufst.v)
#define nbuf (bufst.n)
#define ival (bufst.i)
#define iival (bufst.ii)
#define workspace (bufst.w)
#define ival_mul_pow(n,M) \
		for(int i=fsz/(n);i;--i)\
			r=extint_mul(ival,r,M,workspace);\
		fsz%=(n)
#define ival_mul_pow_nl(n,M) \
		if(fsz>=(n)){\
			r=extint_mul(ival,r,M,workspace);\
			fsz-=(n);\
		}
#define ival_mul_pow_nlm(n,M) \
		if(fsz>=(n)){\
			r=extint_mul(ival,r,M,workspace);\
		}
#define ival_mul5p \
		fsz=ext;\
		ival_mul_pow(13,UINT32_C(1220703125));\
		ival_mul_pow_nl(8,UINT32_C(390625));\
		ival_mul_pow_nl(4,625);\
		ival_mul_pow_nl(2,25);\
		ival_mul_pow_nlm(1,5)
#define ival_mul3p \
		fsz=ext;\
		ival_mul_pow(20,UINT32_C(3486784401));\
		ival_mul_pow_nl(16,UINT32_C(43046721));\
		ival_mul_pow_nl(8,6561);\
		ival_mul_pow_nl(4,81);\
		ival_mul_pow_nl(2,9);\
		ival_mul_pow_nlm(1,3)
#define ival_mul7p \
		fsz=ext;\
		ival_mul_pow(11,UINT32_C(1977326743));\
		ival_mul_pow_nl(8,UINT32_C(5764801));\
		ival_mul_pow_nl(4,2401);\
		ival_mul_pow_nl(2,49);\
		ival_mul_pow_nlm(1,7)
#define ival_mul9p \
		fsz=ext;\
		ival_mul_pow(10,UINT32_C(3486784401));\
		ival_mul_pow_nl(8,UINT32_C(43046721));\
		ival_mul_pow_nl(4,6561);\
		ival_mul_pow_nl(2,81);\
		ival_mul_pow_nlm(1,9)
#define ival_mul11p \
		fsz=ext;\
		ival_mul_pow(9,UINT32_C(2357947691));\
		ival_mul_pow_nl(8,UINT32_C(214358881));\
		ival_mul_pow_nl(4,14641);\
		ival_mul_pow_nl(2,121);\
		ival_mul_pow_nlm(1,11)
#define ival_mul13p \
		fsz=ext;\
		ival_mul_pow(8,UINT32_C(815730721));\
		ival_mul_pow_nl(4,28561);\
		ival_mul_pow_nl(2,169);\
		ival_mul_pow_nlm(1,13)
#define ival_mul15p \
		fsz=ext;\
		ival_mul_pow(8,UINT32_C(2562890625));\
		ival_mul_pow_nl(4,UINT32_C(50625));\
		ival_mul_pow_nl(2,225);\
		ival_mul_pow_nlm(1,15)
#define nbuf_size(x) (((x)+23u)&~7u)
#define fconvs(pref) \
conv_f(expr_combine(pref,1),16,r=extint_left(ival,r,3*ext),nbuf_size(256));\
conv_f(expr_combine(pref,2),2,,nbuf_size(1024));\
conv_f(expr_combine(pref,3),3,ival_mul3p;r=extint_right(ival,r,ext),nbuf_size(646));\
conv_f(expr_combine(pref,4),4,r=extint_left(ival,r,ext),nbuf_size(512));\
conv_f(expr_combine(pref,5),5,ival_mul5p;r=extint_right(ival,r,ext),nbuf_size(441));\
conv_f(expr_combine(pref,6),6,ival_mul3p,nbuf_size(396));\
conv_f(expr_combine(pref,7),7,ival_mul7p;r=extint_right(ival,r,ext),nbuf_size(364));\
conv_f(expr_combine(pref,8),8,r=extint_left(ival,r,2*ext),nbuf_size(342));\
conv_f(expr_combine(pref,9),9,ival_mul9p;r=extint_right(ival,r,ext),nbuf_size(323));\
conv_f(expr_combine(pref,a),10,ival_mul5p,nbuf_size(308));\
conv_f(expr_combine(pref,b),11,ival_mul11p;r=extint_right(ival,r,ext),nbuf_size(296));\
conv_f(expr_combine(pref,c),12,ival_mul3p;r=extint_left(ival,r,2*ext),nbuf_size(285));\
conv_f(expr_combine(pref,d),13,ival_mul13p;r=extint_right(ival,r,ext),nbuf_size(276));\
conv_f(expr_combine(pref,e),14,ival_mul7p;r=extint_left(ival,r,ext),nbuf_size(268));\
conv_f(expr_combine(pref,f),15,ival_mul15p;r=extint_right(ival,r,ext),nbuf_size(262))
fconvs(xa);
conv_fe(fe,10,ival_mul5p,nbuf_size(308),flag->cap?'E':'e');
//5-441,396,364,8,323,10,296,285,276,268,262;
#undef nbuf
#undef vbuf
#undef ival
#undef iival
#undef workspace
static ssize_t faction_s(expr_writer writer,intptr_t fd,const union expr_argf *arg,struct expr_writeflag *flag){
	size_t len;
	ssize_t r,sum,ext,sz;
	if(unlikely(!arg->str)){
		cwrite_common("(null)",6);
	}
	len=flag->digit_set?strnlen(arg->str,flag->digit):strlen(arg->str);
	cwrite_common(arg->str,(ssize_t)len);
}
static ssize_t faction_S(expr_writer writer,intptr_t fd,const union expr_argf *arg,struct expr_writeflag *flag){
	size_t len=(size_t)flag->digit;
	ssize_t r,sum,ext,sz;
	cwrite_common(arg->str,(ssize_t)len);
}
#define faction_hexdump(name) \
static ssize_t faction_##name(expr_writer writer,intptr_t fd,const union expr_argf *arg,struct expr_writeflag *flag){\
	size_t len=(size_t)flag->digit;\
	ssize_t r,sum,ext;\
	uint8_t buf[960];\
	uint8_t *p,*endp,*datap;\
	uint8_t data;\
	int f61=!!(flag->minus),f60=!!(flag->sharp),f63=!!(flag->plus);\
	const char *conv_str=flag->cap?conv_btoX:conv_btox;\
	ext=(ssize_t)flag_width(flag,0)-(f60?4:2)*(ssize_t)len;\
	if(f63&&len>1)\
		ext-=(ssize_t)(len-1);\
	sum=0;\
	if(!f61&&ext>0){\
		c_trywriteext((flag->zero)?'0':' ',ext);\
	}\
	if(len){\
		p=buf;\
		endp=buf+sizeof(buf);\
		for(datap=arg->addr;;){\
			data=*datap;\
			if(f60){\
				*(p++)='0';\
				*(p++)=conv_str[16];\
			}\
			*(p++)=conv_str[data>>4];\
			*(p++)=conv_str[data&15];\
			if(f63&&likely(len>1))\
				*(p++)=',';\
			if(unlikely(!--len))\
				break;\
			if(unlikely(p>=endp)){\
				c_trywrite(buf,sizeof(buf));\
				p=buf;\
			}\
			++datap;\
		}\
		if(likely(p>buf)){\
			c_trywrite(buf,p-buf);\
		}\
	}\
	if(f61&&ext>0){\
		c_trywriteext((flag->zero)?'0':' ',ext);\
	}\
	return sum;\
}
faction_hexdump(h);
static ssize_t faction_c(expr_writer writer,intptr_t fd,const union expr_argf *arg,struct expr_writeflag *flag){
#if (!defined(__BIG_ENDIAN__)||!(__BIG_ENDIAN__))
	return writer(fd,arg,1);
#else
	return writer(fd,(const uint8_t *)arg+(sizeof(void *)-1),1);
#endif
}
static ssize_t faction_g(expr_writer writer,intptr_t fd,const union expr_argf *arg,struct expr_writeflag *flag){
	double val=fabs(arg->dbl),vl;
	return ((val==0.0||((vl=log(val)/log(10))>=-4&&vl<flag_digit(flag,6)))?
	converter_xaa:converter_fe)(writer,fd,arg,(flag->eq=1,flag));
}
static ssize_t faction_G(expr_writer writer,intptr_t fd,const union expr_argf *arg,struct expr_writeflag *flag){
	double val=fabs(arg->dbl),vl;
	return ((val==0.0||((vl=log(val)/log(10))>=-4&&vl<flag_digit(flag,6)))?
	converter_xaa:converter_fe)(writer,fd,arg,(flag->eq=1,flag));
}
static ssize_t faction_p(expr_writer writer,intptr_t fd,const union expr_argf *arg,struct expr_writeflag *flag){
	if(!arg->addr){
		ssize_t r,sum,ext,sz;
		cwrite_common("null",(ssize_t)4);
	}
	return converter_x81(writer,fd,arg,(flag->sharp=1,flag));
}
static ssize_t faction_P(expr_writer writer,intptr_t fd,const union expr_argf *arg,struct expr_writeflag *flag){
	if(!arg->addr){
		ssize_t r,sum,ext,sz;
		cwrite_common("NULL",(ssize_t)4);
	}
	return converter_x81(writer,fd,arg,(flag->sharp=1,flag));
}
static ssize_t faction_percent(expr_writer writer,intptr_t fd,const union expr_argf *arg,struct expr_writeflag *flag){
	return writer(fd,"%",1);
}
#define fmtc_register(_act,_argc,_type,_op,...) {\
	.converter=(_act),\
	.type=(_type),\
	.no_arg=(_argc?0:1),\
	.digit_check=(_argc==2?1:0),\
	.setcap=(\
			((_op)>='\x91'&&(_op)<='\x9f')||\
			((_op)>='\xb1'&&(_op)<='\xbf')||\
			((_op)>='A'&&(_op)<='Z')\
			?1:0),\
	.op={_op,##__VA_ARGS__},\
}
const struct expr_writefmt expr_writefmts_default[]={
	fmtc_register(faction_percent,0,0,'%'),

	fmtc_register(converter_x81,1,EXPR_FLAGTYPE_UNSIGNED,'\x81','x'),
	fmtc_register(converter_x82,1,EXPR_FLAGTYPE_UNSIGNED,'\x82','\x92','b'),
	fmtc_register(converter_x83,1,EXPR_FLAGTYPE_UNSIGNED,'\x83','\x93'),
	fmtc_register(converter_x84,1,EXPR_FLAGTYPE_UNSIGNED,'\x84','\x94'),
	fmtc_register(converter_x85,1,EXPR_FLAGTYPE_UNSIGNED,'\x85','\x95'),
	fmtc_register(converter_x86,1,EXPR_FLAGTYPE_UNSIGNED,'\x86','\x96'),
	fmtc_register(converter_x87,1,EXPR_FLAGTYPE_UNSIGNED,'\x87','\x97'),
	fmtc_register(converter_x88,1,EXPR_FLAGTYPE_UNSIGNED,'\x88','\x98','o'),
	fmtc_register(converter_x89,1,EXPR_FLAGTYPE_UNSIGNED,'\x89','\x99'),
	fmtc_register(converter_x8a,1,EXPR_FLAGTYPE_UNSIGNED,'\x8a','\x9a','u'),
	fmtc_register(converter_x8b,1,EXPR_FLAGTYPE_UNSIGNED,'\x8b'),
	fmtc_register(converter_x8c,1,EXPR_FLAGTYPE_UNSIGNED,'\x8c'),
	fmtc_register(converter_x8d,1,EXPR_FLAGTYPE_UNSIGNED,'\x8d'),
	fmtc_register(converter_x8e,1,EXPR_FLAGTYPE_UNSIGNED,'\x8e'),
	fmtc_register(converter_x8f,1,EXPR_FLAGTYPE_UNSIGNED,'\x8f'),

	fmtc_register(converter_x81,1,EXPR_FLAGTYPE_UNSIGNED,'\x91','X'),
	fmtc_register(converter_x8b,1,EXPR_FLAGTYPE_UNSIGNED,'\x9b'),
	fmtc_register(converter_x8c,1,EXPR_FLAGTYPE_UNSIGNED,'\x9c'),
	fmtc_register(converter_x8d,1,EXPR_FLAGTYPE_UNSIGNED,'\x9d'),
	fmtc_register(converter_x8e,1,EXPR_FLAGTYPE_UNSIGNED,'\x9e'),
	fmtc_register(converter_x8f,1,EXPR_FLAGTYPE_UNSIGNED,'\x9f'),

	fmtc_register(converter_xa1,1,EXPR_FLAGTYPE_DOUBLE,'\xa1','a'),
	fmtc_register(converter_xa2,1,EXPR_FLAGTYPE_DOUBLE,'\xa2'),
	fmtc_register(converter_xa3,1,EXPR_FLAGTYPE_DOUBLE,'\xa3'),
	fmtc_register(converter_xa4,1,EXPR_FLAGTYPE_DOUBLE,'\xa4'),
	fmtc_register(converter_xa5,1,EXPR_FLAGTYPE_DOUBLE,'\xa5'),
	fmtc_register(converter_xa6,1,EXPR_FLAGTYPE_DOUBLE,'\xa6'),
	fmtc_register(converter_xa7,1,EXPR_FLAGTYPE_DOUBLE,'\xa7'),
	fmtc_register(converter_xa8,1,EXPR_FLAGTYPE_DOUBLE,'\xa8'),
	fmtc_register(converter_xa9,1,EXPR_FLAGTYPE_DOUBLE,'\xa9'),
	fmtc_register(converter_xaa,1,EXPR_FLAGTYPE_DOUBLE,'\xaa','f'),
	fmtc_register(converter_xab,1,EXPR_FLAGTYPE_DOUBLE,'\xab'),
	fmtc_register(converter_xac,1,EXPR_FLAGTYPE_DOUBLE,'\xac'),
	fmtc_register(converter_xad,1,EXPR_FLAGTYPE_DOUBLE,'\xad'),
	fmtc_register(converter_xae,1,EXPR_FLAGTYPE_DOUBLE,'\xae'),
	fmtc_register(converter_xaf,1,EXPR_FLAGTYPE_DOUBLE,'\xaf'),

	fmtc_register(converter_xa1,1,EXPR_FLAGTYPE_DOUBLE,'\xb1','A'),
	fmtc_register(converter_xa2,1,EXPR_FLAGTYPE_DOUBLE,'\xb2','B'),
	fmtc_register(converter_xa3,1,EXPR_FLAGTYPE_DOUBLE,'\xb3'),
	fmtc_register(converter_xa4,1,EXPR_FLAGTYPE_DOUBLE,'\xb4'),
	fmtc_register(converter_xa5,1,EXPR_FLAGTYPE_DOUBLE,'\xb5'),
	fmtc_register(converter_xa6,1,EXPR_FLAGTYPE_DOUBLE,'\xb6'),
	fmtc_register(converter_xa7,1,EXPR_FLAGTYPE_DOUBLE,'\xb7'),
	fmtc_register(converter_xa8,1,EXPR_FLAGTYPE_DOUBLE,'\xb8','O'),
	fmtc_register(converter_xa9,1,EXPR_FLAGTYPE_DOUBLE,'\xb9'),
	fmtc_register(converter_xaa,1,EXPR_FLAGTYPE_DOUBLE,'\xba','F'),
	fmtc_register(converter_xab,1,EXPR_FLAGTYPE_DOUBLE,'\xbb'),
	fmtc_register(converter_xac,1,EXPR_FLAGTYPE_DOUBLE,'\xbc'),
	fmtc_register(converter_xad,1,EXPR_FLAGTYPE_DOUBLE,'\xbd'),
	fmtc_register(converter_xae,1,EXPR_FLAGTYPE_DOUBLE,'\xbe'),
	fmtc_register(converter_xaf,1,EXPR_FLAGTYPE_DOUBLE,'\xbf'),

	fmtc_register(faction_s,1,EXPR_FLAGTYPE_ADDR,'s'),
	fmtc_register(faction_S,2,EXPR_FLAGTYPE_ADDR,'S'),
	fmtc_register(faction_c,1,EXPR_FLAGTYPE_UNSIGNED,'c'),
	fmtc_register(converter_d,1,EXPR_FLAGTYPE_SIGNED,'d'),
	fmtc_register(converter_fe,1,EXPR_FLAGTYPE_DOUBLE,'e'),
	fmtc_register(converter_fe,1,EXPR_FLAGTYPE_DOUBLE,'E'),
	fmtc_register(faction_g,1,EXPR_FLAGTYPE_DOUBLE,'g'),
	fmtc_register(faction_G,1,EXPR_FLAGTYPE_DOUBLE,'G'),
	fmtc_register(faction_p,1,EXPR_FLAGTYPE_ADDR,'p'),
	fmtc_register(faction_P,1,EXPR_FLAGTYPE_ADDR,'P'),
	fmtc_register(faction_h,2,EXPR_FLAGTYPE_ADDR,'h'),
	fmtc_register(faction_h,2,EXPR_FLAGTYPE_ADDR,'H'),
	fmtc_register(converter_size,1,EXPR_FLAGTYPE_SIGNED,'m'),
	fmtc_register(converter_size,1,EXPR_FLAGTYPE_SIGNED,'M'),

};
const uint8_t expr_writefmts_default_size=arrsize(expr_writefmts_default);
const uint8_t expr_writefmts_table_default[256]={
['q']=0,
['n']=EXPR_FMTC_WRITESIZE,
['N']=EXPR_FMTC_WRITESIZEDF,
['r']=EXPR_FMTC_JUMP_INDEX,
['l']=EXPR_FMTC_LOOP,
['L']=EXPR_FMTC_LOOP_FORWARD,
['T']=EXPR_FMTC_DELIM,
['t']=EXPR_FMTC_TAIL,
['k']=EXPR_FMTC_SETARR,
['Q']=EXPR_FMTC_EXITIF,
['@']=EXPR_FMTC_READ_FMTC,
['j']=EXPR_FMTC_JUMP,
['C']=EXPR_FMTC_READ_CONVERTER,
['I']=EXPR_FMTC_INC,
['D']=EXPR_FMTC_DEC,
['w']=EXPR_FMTC_PUSH,
['W']=EXPR_FMTC_CLEAR,
['V']=EXPR_FMTC_SAVE,
['v']=EXPR_FMTC_EVAL,
['z']=EXPR_FMTC_FLUSH,

['%']=1,
[(uint8_t)'\x81']=2,['x']=2,
[(uint8_t)'\x82']=3,[(uint8_t)'\x92']=3,['b']=3,
[(uint8_t)'\x83']=4,[(uint8_t)'\x93']=4,
[(uint8_t)'\x84']=5,[(uint8_t)'\x94']=5,
[(uint8_t)'\x85']=6,[(uint8_t)'\x95']=6,
[(uint8_t)'\x86']=7,[(uint8_t)'\x96']=7,
[(uint8_t)'\x87']=8,[(uint8_t)'\x97']=8,
[(uint8_t)'\x88']=9,[(uint8_t)'\x98']=9,['o']=9,
[(uint8_t)'\x89']=10,[(uint8_t)'\x99']=10,
[(uint8_t)'\x8a']=11,[(uint8_t)'\x9a']=11,['u']=11,
[(uint8_t)'\x8b']=12,
[(uint8_t)'\x8c']=13,
[(uint8_t)'\x8d']=14,
[(uint8_t)'\x8e']=15,
[(uint8_t)'\x8f']=16,
[(uint8_t)'\x91']=17,['X']=17,
[(uint8_t)'\x9b']=18,
[(uint8_t)'\x9c']=19,
[(uint8_t)'\x9d']=20,
[(uint8_t)'\x9e']=21,
[(uint8_t)'\x9f']=22,
[(uint8_t)'\xa1']=23,['a']=23,
[(uint8_t)'\xa2']=24,
[(uint8_t)'\xa3']=25,
[(uint8_t)'\xa4']=26,
[(uint8_t)'\xa5']=27,
[(uint8_t)'\xa6']=28,
[(uint8_t)'\xa7']=29,
[(uint8_t)'\xa8']=30,
[(uint8_t)'\xa9']=31,
[(uint8_t)'\xaa']=32,['f']=32,
[(uint8_t)'\xab']=33,
[(uint8_t)'\xac']=34,
[(uint8_t)'\xad']=35,
[(uint8_t)'\xae']=36,
[(uint8_t)'\xaf']=37,
[(uint8_t)'\xb1']=38,['A']=38,
[(uint8_t)'\xb2']=39,['B']=39,
[(uint8_t)'\xb3']=40,
[(uint8_t)'\xb4']=41,
[(uint8_t)'\xb5']=42,
[(uint8_t)'\xb6']=43,
[(uint8_t)'\xb7']=44,
[(uint8_t)'\xb8']=45,['O']=45,
[(uint8_t)'\xb9']=46,
[(uint8_t)'\xba']=47,['F']=47,
[(uint8_t)'\xbb']=48,
[(uint8_t)'\xbc']=49,
[(uint8_t)'\xbd']=50,
[(uint8_t)'\xbe']=51,
[(uint8_t)'\xbf']=52,
['s']=53,
['S']=54,
['c']=55,
['d']=56,
['e']=57,
['E']=58,
['g']=59,
['G']=60,
['p']=61,
['P']=62,
['h']=63,
['H']=64,
['m']=65,
['M']=66,
};
#define wf_trywrite(buf,sz) {\
	v=writer(fd,(buf),(sz));\
	if(unlikely(v<0))\
		return v;\
	ret+=v;\
}
static inline const char *internal_strtoz_10(const char *restrict nptr,const char *endp,ssize_t *restrict outval){
	size_t r=0;
	int neg=0;
	unsigned int get;
	if(likely(nptr<endp)){
		if(*nptr=='-'){
			++nptr;
			neg=1;
		}
	}
#define scannum(base) \
	while(nptr<endp){\
		get=(uint8_t)*nptr-(uint8_t)'0';\
		if(unlikely(get>=base))\
			goto out;\
		if(unlikely(mulo(r,base)))\
			goto out;\
		if(unlikely(addo(r,get)))\
			goto out;\
		++nptr;\
	}\
	goto out
	scannum(10);
out:
	debug("result:%zd",(ssize_t)(neg?-r:r));
	*outval=(ssize_t)(neg?-r:r);
	return nptr;
}
#define range_check(val,closed_min,open_max,onfalse) ({\
	uintptr_t register __val=(uintptr_t)(val);\
	if(unlikely(__val<(uintptr_t)(closed_min)||__val>=(uintptr_t)(open_max))){\
		onfalse;\
	}\
})
#define range_checkn(val,closed_min,open_n,onfalse) ({\
	if(unlikely((uintptr_t)((val)-(closed_min))>=(uintptr_t)(open_n))){\
		onfalse;\
	}\
})
#define range_checkcn(val,closed_min,closed_n,onfalse) ({\
	if(unlikely((uintptr_t)((val)-(closed_min))>(uintptr_t)(closed_n))){\
		onfalse;\
	}\
})
ssize_t expr_vwritef(const char *restrict fmt,size_t fmtlen,expr_writer writer,intptr_t fd,expr_argffetch arg,void *addr){
	return expr_vwritef_r(fmt,fmtlen,writer,fd,arg,addr,expr_writefmts_default,expr_writefmts_table_default);
}
ssize_t expr_vwritef_r(const char *restrict fmt,size_t fmtlen,expr_writer writer,intptr_t fd,expr_argffetch arg,void *addr,const struct expr_writefmt *restrict fmts,const uint8_t *restrict table){
	const char *endp=fmt+fmtlen,*fmt_old=fmt,*fmt0=fmt;
	ssize_t ret=0;
	ssize_t v;
	const struct expr_writefmt *wfp;
	struct expr_writeflag flag[1];
	const char *delim=NULL,*tail=NULL,*d1;
	size_t delimsz=0,tailsz=0,arrlen=0;
	size_t loop=0;
	ptrdiff_t index=0;
	uint8_t forward=0,arrwid=0,current,r1;
	const char *fmt_save[8];
	const char **fmt_save_current;
	union expr_argf save;
	if(unlikely(endp<=fmt))
		return 0;
	fmt_save_current=fmt_save;
next:
	fmt=memchr(fmt,'%',endp-fmt);
	if(unlikely(!fmt)){
		wf_trywrite(fmt_old,endp-fmt_old);
		goto end;
	}
	if(fmt>fmt_old){
		wf_trywrite(fmt_old,fmt-fmt_old);
		fmt_old=fmt;
	}
#define fmt_inc_check ++fmt;\
	if(unlikely(fmt>=endp))\
		goto end
	fmt_inc_check;
	*flag->bit=0;
#define fmt_setflag(_field) \
	flag->_field=1;\
	fmt_inc_check;\
	goto reflag
reflag:
	switch(*fmt){
		case '+':
			fmt_setflag(plus);
		case ' ':
			fmt_setflag(space);
		case '-':
			fmt_setflag(minus);
		case '#':
			fmt_setflag(sharp);
		case '0':
			fmt_setflag(zero);
		case '=':
			fmt_setflag(eq);
		case '?':
			fmt_setflag(saved);
		case '&':
			fmt_setflag(addr);
		default:
			break;
	}
#define t_copy(_d,_s,T) *(T *)(_d)=*(T *)(_s)
#define t_ocopy(_d,_s,T,_off) *(T *)((uintptr_t)(_d)+(_off))=*(T *)((uintptr_t)(_s)+(_off))
#define bit_copy(_d,_s,_bit) ({\
	switch(_bit){\
		case 1:\
			t_copy((_d),(_s),uint8_t);\
			break;\
		case 2:\
			t_copy((_d),(_s),uint16_t);\
			break;\
		case 3:\
			t_copy((_d),(_s),uint16_t);\
			t_ocopy((_d),(_s),uint8_t,2);\
			break;\
		case 4:\
			t_copy((_d),(_s),uint32_t);\
			break;\
		case 5:\
			t_copy((_d),(_s),uint32_t);\
			t_ocopy((_d),(_s),uint8_t,4);\
			break;\
		case 6:\
			t_copy((_d),(_s),uint32_t);\
			t_ocopy((_d),(_s),uint16_t,4);\
			break;\
		case 7:\
			t_copy((_d),(_s),uint32_t);\
			t_ocopy((_d),(_s),uint16_t,4);\
			t_ocopy((_d),(_s),uint8_t,6);\
			break;\
		case 0:\
		case 8:\
			t_copy((_d),(_s),uint64_t);\
			break;\
		default:\
			__builtin_unreachable();\
	}\
})
#define argnext1 ++index;debug("arg index to %zd",index)
#define argnext(N) index+=(N);debug("arg index to %zd",index)
#define argback(N) index-=(N);debug("arg index back to %zd",index)
#define goto_argfail {debug("arg failed");return PTRDIFF_MIN;}
#define argt(_type) ({register const union expr_argf *__arg;flag->type=(_type);__arg=arg(index,flag,addr);if(unlikely(!__arg)){debug("cannot get arg[%zd]",index);goto_argfail;}__arg;})
#define arg1 argt(EXPR_FLAGTYPE_ADDR)
#define get_next_arg64(dest,_after) \
	if(*fmt=='*'){\
		fmt_inc_check;\
		if(*fmt=='*'){\
			dest=*argt(EXPR_FLAGTYPE_SIGNED)->siaddr;\
			fmt_inc_check;\
		}else\
			dest=argt(EXPR_FLAGTYPE_SIGNED)->sint;\
		_after;\
		argnext1;\
	}else {\
		fmt_old=fmt;\
		fmt=internal_strtoz_10(fmt,endp,&v);\
		if(fmt>fmt_old){\
			dest=v;\
			_after;\
			if(unlikely(fmt>=endp))\
				goto end;\
		}\
	}
	get_next_arg64(flag->width,flag->width_set=1);
	if(*fmt=='.'){
		fmt_inc_check;
		get_next_arg64(flag->digit,flag->digit_set=1);
	}
	if(*fmt==':'){
		fmt_inc_check;
		get_next_arg64(flag->argsize,);
	}
	current=*(uint8_t *)fmt;
current_get:
	flag->op=current;
	switch((r1=table[current])){
		case EXPR_FMTC_EXIT:
			goto end;
		case EXPR_FMTC_WRITESIZE:
			debug("n=%zu",ret);
			bit_copy(arg1->zaddr,&ret,flag->argsize);
			argnext1;
			break;
		case EXPR_FMTC_WRITESIZEDF:
			*arg1->daddr=(double)ret;
			argnext1;
			break;
		case EXPR_FMTC_JUMP_INDEX:
#define fmt_op_cond(onexit) \
			if(flag->eq){\
				intptr_t *restrict ar;\
				int c;\
				if(flag->digit_set){\
					ar=arg1->addr;\
					argnext1;\
					*ar+=flag->digit;\
					c=flag->zero?*ar<0:(flag->space?*ar>0:!*ar);\
					if(flag->sharp)\
						c=!c;\
					if(c){\
						onexit;\
						break;\
					}\
				}else {\
					ar=arg1->addr;\
					argnext1;\
					if(!ar){\
						onexit;\
						break;\
					}\
				}\
			}
			fmt_op_cond();
			v=flag_width(flag,1);
			if(flag->minus){
				index-=v;
			}else {
				index+=v;
			}
			debug("jumpto args[%zd]",index);
			break;
		case EXPR_FMTC_LOOP:
			loop=flag_width(flag,0);
			break;
		case EXPR_FMTC_LOOP_FORWARD:
			loop=flag_width(flag,0);
			forward=1;
			break;
		case EXPR_FMTC_DELIM:
#define setdlm(dlm,dsz) \
			dsz=flag_width(flag,0);\
			if(dsz){\
				dlm=fmt+1;\
				d1=dlm+dsz;\
				if(unlikely(d1>=endp||d1<dlm))\
					goto_argfail;\
				fmt+=dsz;\
			}
			setdlm(delim,delimsz);
			break;
		case EXPR_FMTC_TAIL:
			setdlm(tail,tailsz);
			break;
		case EXPR_FMTC_SETARR:
			arrwid=flag_digit(flag,8);
			if(unlikely(arrwid>8||!arrwid))
				goto_argfail;
			arrlen=flag_width(flag,0);
			break;
		case EXPR_FMTC_EXITIF:
			if(flag->plus){
				if(save.umax)
					goto end;
			}else {
				if(arg1->uint)
					goto end;
			}
			argnext1;
			break;
		case EXPR_FMTC_READ_FMTC:
			current=(uint8_t)arg1->uint;
			argnext1;
			goto current_get;
#define fmt_jumpto(target) range_checkn(fmt=(target),fmt0,fmtlen,goto_argfail)
		case EXPR_FMTC_JUMP:
			fmt_op_cond(
				if(flag->plus){
					range_checkn(--fmt_save_current,fmt_save,8,goto_argfail);
				}
				arrlen=0);
			v=flag_width(flag,0);
			if(flag->plus){
				range_checkn(fmt_save_current-1,fmt_save,8,goto_argfail);
				d1=fmt_save_current[-1];
			}else if(flag->minus)
				d1=fmt-v;
			else
				d1=fmt+v;
			++d1;
			debug("jumpto fmt[%zd]",d1-fmt0);
			fmt_jumpto(d1);
			argback(arrlen);
			arrlen=0;
			goto continue_keepfmt;
		case EXPR_FMTC_READ_CONVERTER:
			wfp=(const struct expr_writefmt *)arg1->addr;
			argnext1;
			goto wfp_get;
		case EXPR_FMTC_INC:
			if(flag->width_set)
				*arg1->uiaddr*=flag->width;
			if(flag->digit_set)
				*arg1->uiaddr+=flag->digit;
			else
				++(*arg1->uiaddr);
			argnext1;
			break;
		case EXPR_FMTC_DEC:
			if(flag->width_set)
				*arg1->uiaddr/=flag->width;
			if(flag->digit_set)
				*arg1->uiaddr-=flag->digit;
			else
				--(*arg1->uiaddr);
			argnext1;
			break;
		case EXPR_FMTC_PUSH:
			debug("push fmt[%zd]",fmt-fmt0);
			range_checkn(fmt_save_current,fmt_save,8,goto_argfail);
			EXPR_RPUSH(fmt_save_current)=fmt;
			break;
		case EXPR_FMTC_CLEAR:
			fmt_save_current=fmt_save;
			break;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
		case EXPR_FMTC_SAVE:
			if(flag->zero){
				if(flag->digit_set){
					if(flag->plus)
						save.smax+=flag->digit;
					else
						save.smax=flag->digit;
				}else
					save.umax=0;
			}else {
				save.umax=*arg1->umaddr;
				argnext1;
			}
			break;
		case EXPR_FMTC_EVAL:
			{
				union expr_argf old;
				old.umax=*arg1->umaddr;
				switch(flag_width(flag,0)){
					case 0:
						old.umax=save.umax;
						break;
					case 1:
						old.umax+=save.umax;
						break;
					case 2:
						old.umax-=save.umax;
						break;
					case 3:
						old.umax*=save.umax;
						break;
					case 4:
						old.umax/=save.umax;
						break;
					case 5:
						old.umax%=save.umax;
						break;
					case 6:
						old.umax&=save.umax;
						break;
					case 7:
						old.umax^=save.umax;
						break;
					case 8:
						old.umax|=save.umax;
						break;
					case 9:
						old.umax<<=save.umax;
						break;
					case 10:
						old.umax>>=save.umax;
						break;
					case 11:
						old.umax=((uintmax_t (*)(const union expr_argf *,intptr_t))save.addr)(&old,fd);
						break;
					case 12:
						old.umax=~old.umax;
						break;
					case 13:
						old.umax=-old.umax;
						break;
					case 14:
						save.umax=~save.umax;
						break;
					case 15:
						save.umax=-save.umax;
						break;
					case 16:
						save.umax=!save.umax;
						break;
					case 17:
						save.umax=!!save.umax;
						break;
					default:
						goto_argfail;
				}
				*arg1->umaddr=old.umax;
				if(flag->eq)
					save.umax=old.umax;
			}
			argnext1;
			break;
		case EXPR_FMTC_FLUSH:
			v=writer(fd,"",0);
			if(unlikely(v<0))
				return v;
			ret+=v;
			break;
#pragma GCC diagnostic pop

#define fmt_repeat if(!loop)loop=1;for(;loop;--loop)
#define fmt_once(_arg) \
	v=wfp->converter(writer,fd,(_arg),flag);\
	if(unlikely(v<0))\
		return v;\
	ret+=v
#define fmt_dorepeat(_arg) \
	fmt_repeat {\
		fmt_once(_arg);\
		if(tailsz){\
			wf_trywrite(tail,tailsz);\
		}\
		if(delimsz&&loop>1){\
			wf_trywrite(delim,delimsz);\
		}\
		if(!__builtin_constant_p(_arg)){\
			if(forward){\
				argnext1;\
			}\
		}\
	}

		default:
			wfp=fmts+(r1-1);//r
wfp_get:
			if(unlikely(wfp->digit_check&&!flag->digit_set))
				goto_argfail;
			if(wfp->setcap)
				flag->cap=1;
			if(wfp->no_arg){
				fmt_dorepeat(NULL);
				forward=0;
			}else if(flag->saved){
				fmt_dorepeat((const union expr_argf *)&save);
				forward=0;
			}else if(arrlen){
				uintptr_t aps;
				expr_umaxf_t val,mask;
				flag->argsize=sizeof(void *);
				aps=arg1->uint;
				mask=(UINT64_C(1)<<(arrwid*8))-1;
				argnext1;
				for(;;){
					if(arrwid<8){
						bit_copy(&val,aps,arrwid);
						if(wfp->type==EXPR_FLAGTYPE_SIGNED&&
						(val&(UINT64_C(0x80)<<((arrwid-1)*8))))
							val=(~UINT64_C(0)&~mask)|(val&mask);
						else
							val&=mask;
					}else
						val=*(uint64_t *)aps;
					fmt_once((const union expr_argf *)&val);
					if(tailsz){
						wf_trywrite(tail,tailsz);
					}
					if(delimsz&&arrlen>1){
						wf_trywrite(delim,delimsz);
					}
					if(unlikely(!--arrlen))
						break;
					aps+=arrwid;
				}
			}else {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
				const union expr_argf *arg_cur;
				if(!forward&&unlikely(!(arg_cur=argt(wfp->type))))
					goto_argfail;
				fmt_dorepeat(arg_cur);
				if(!forward){
					argnext1;
				}else
					forward=0;
#pragma GCC diagnostic pop
			}
			break;
	}
	fmt_inc_check;
continue_keepfmt:
	fmt_old=fmt;
	goto next;
end:
	return ret;
}
#undef arg1
#undef argt
#undef goto_argfail
#undef fmt_setflag
#undef fmt_inc_check
#undef get_next_arg64
#undef argback
#undef argnext
#undef argnext1
#undef t_copy
#undef t_ocopy
#undef bit_copy
#undef fmt_jumpto
#undef fmt_setdlm
#undef fmt_once
#undef fmt_repeat
#undef fmt_dorepeat
#undef fmt_op_cond
struct writef_args {
	const union expr_argf *base;
	size_t arglen;
};
static const union expr_argf *arr_arg(ptrdiff_t index,const struct expr_writeflag *flag,void *addr){
	struct writef_args *wa=addr;
	const union expr_argf *r=wa->base+index;
	range_checkn(r,wa->base,wa->arglen,goto err);
	debug("getting arg[%zd] (base=%p,len=%zu)=%p",index,wa->base,wa->arglen,r);
	return flag->addr?r->aaddr:r;
err:
	debug("getting arg[%zd] (base=%p,len=%zu)=NULL",index,wa->base,wa->arglen);
	return NULL;
}
ssize_t expr_writef(const char *restrict fmt,size_t fmtlen,expr_writer writer,intptr_t fd,const union expr_argf *restrict args,size_t arglen){
	struct writef_args wa[1];
	wa->base=args;
	wa->arglen=arglen;
	return expr_vwritef_r(fmt,fmtlen,writer,fd,arr_arg,wa,expr_writefmts_default,expr_writefmts_table_default);
}
ssize_t expr_writef_r(const char *restrict fmt,size_t fmtlen,expr_writer writer,intptr_t fd,const union expr_argf *restrict args,size_t arglen,const struct expr_writefmt *restrict fmts,const uint8_t *restrict table){
	struct writef_args wa[1];
	wa->base=args;
	wa->arglen=arglen;
	return expr_vwritef_r(fmt,fmtlen,writer,fd,arr_arg,wa,fmts,table);
}
struct fmtarg {
	va_list ap;
	ptrdiff_t index_old;
	union expr_argf save[1];
};
static const union expr_argf *ap_getarg(ptrdiff_t index,const struct expr_writeflag *flag,void *addr){
	struct fmtarg *restrict a=addr;
	int type=(int)flag->type,useaddr=!!flag->addr;
	if(index==a->index_old){
		goto end;
	}
	if(unlikely(index!=a->index_old+1)){
		return NULL;
	}
	if(useaddr){
		type=EXPR_FLAGTYPE_ADDR;
	}
	switch(type){
		case EXPR_FLAGTYPE_DOUBLE:
			a->save->dbl=va_arg(a->ap,double);
			break;
		case EXPR_FLAGTYPE_ADDR:
			a->save->addr=va_arg(a->ap,void *);
			break;
		case EXPR_FLAGTYPE_SIGNED:
			switch(flag->argsize){
				case 1 ... sizeof(int):
					a->save->smax=va_arg(a->ap,int);
					break;
				default:
					a->save->smax=va_arg(a->ap,intptr_t);
					break;
			}
			break;
		case EXPR_FLAGTYPE_UNSIGNED:
			switch(flag->argsize){
				case 1 ... sizeof(int):
					a->save->smax=va_arg(a->ap,unsigned int);
					break;
				default:
					a->save->smax=va_arg(a->ap,uintptr_t);
					break;
			}
			break;
		default:
			__builtin_unreachable();
	}
	++a->index_old;
end:
	return useaddr?a->save->aaddr:a->save;
}
#define ap_common(_start,_end,_fmts,_table) \
	ssize_t r;\
	struct fmtarg a[1];\
	a->index_old=-1;\
	_start;\
	r=expr_vwritef_r(fmt,fmtlen,writer,fd,ap_getarg,a,_fmts,_table);\
	_end;\
	return r
ssize_t expr_vapwritef_r(const char *restrict fmt,size_t fmtlen,expr_writer writer,intptr_t fd,const struct expr_writefmt *restrict fmts,const uint8_t *restrict table,va_list ap){
	ap_common(va_copy(a->ap,ap),va_end(a->ap),fmts,table);
}
ssize_t expr_apwritef_r(const char *restrict fmt,size_t fmtlen,expr_writer writer,intptr_t fd,const struct expr_writefmt *restrict fmts,const uint8_t *restrict table,...){
	ap_common(va_start(a->ap,table),va_end(a->ap),fmts,table);
}
ssize_t expr_vapwritef(const char *restrict fmt,size_t fmtlen,expr_writer writer,intptr_t fd,va_list ap){
	ap_common(va_copy(a->ap,ap),va_end(a->ap),expr_writefmts_default,expr_writefmts_table_default);
}
ssize_t expr_apwritef(const char *restrict fmt,size_t fmtlen,expr_writer writer,intptr_t fd,...){
	ap_common(va_start(a->ap,fd),va_end(a->ap),expr_writefmts_default,expr_writefmts_table_default);
}
