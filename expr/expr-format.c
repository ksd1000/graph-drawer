/*******************************************************************************
 *License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>*
 *This is free software: you are free to change and redistribute it.           *
 *******************************************************************************/

/*
 * History: 2026-02-03 separated from expr.c
 * String formatter
 * Motivation: For making the printf() available in expression.
 * 	The multi-dimension(multi-argument) function that libexpr supports is like the form:
 * 	f(args[],n);
 * 	But the libc printf supports only the va_list for passing arguments, like:
 * 	f(arg0,arg1,...); or f(arg0,ap);
 * 	The printf for expression was implemented like so:
 * 		double d_printf(double *args,size_t n){
 * 			const char *fmt=cast(*args,const char *);
 * 			switch(n){
 * 				case 1:return (double)printf(fmt);
 * 				case 2:return (double)printf(fmt,args[1]);
 * 				case 3:return (double)printf(fmt,args[1],args[2]);
 * 				case 4:return (double)printf(fmt,args[1],args[2],args[3]);
 * 				case 5:return (double)printf(fmt,args[1],args[2],args[3],args[4]);
 * 				case 6:return (double)printf(fmt,args[1],args[2],args[3],args[4],args[5]);
 * 				case 7:return (double)printf(fmt,args[1],args[2],args[3],args[4],args[5],args[6]);
 * 				case 8:return (double)printf(fmt,args[1],args[2],args[3],args[4],args[5],args[6],args[7]);
 * 				default:return (double)printf("Too many args!");
 * 			}
 * 		}
 * 	It only worked for double type, and at most 7 arguments.
 * Dependencies: expr-internal.c
 */

#define _GNU_SOURCE
#ifndef _EXPR_DEFS_H_
#include <math.h>
#endif

#if (defined(EXPR_LIBC_STRING_IMPL)&&(EXPR_LIBC_STRING_IMPL))
#include <string.h>
#endif

#define EXPR_INLIB 1
#include "expr.h"

/*
	SSIZE_MIN+1: allocate fail
	SSIZE_MIN+2: no enough bufsize
	SSIZE_MIN+3: arg fail
	SSIZE_MIN+4: digit not set
	SSIZE_MIN+128: truncate
*/
#define TRUNC_CODE EXPR_TRUNC_CODE

#define c_trywrite(buf,sz) if(likely((r=(ssize_t)(sz))>0)){\
	r=writer(fd,(buf),(size_t)r);\
	if(unlikely(r<0))\
		return r;\
	sum+=r;\
	if(unlikely(r<(ssize_t)(sz)))\
		return TRUNC_CODE+sum;\
}
#define c_trywrite3(buf,sz,onerr) if(likely((r=(ssize_t)(sz))>0)){\
	r=writer(fd,(buf),(size_t)r);\
	if(unlikely(r<0)){\
		onerr;\
	}\
	if(unlikely(r<(ssize_t)(sz))){\
		r+=sum+TRUNC_CODE;\
		onerr;\
	}\
	sum+=r;\
}
#define c_trywriteext(c,sz) {\
	r=writeext(writer,fd,(size_t)(sz),(c));\
	if(unlikely(r<0))\
		return r;\
	sum+=r;\
}
#define xmtl mtl
#define EXPR_WCBUFSIZE 1024
ssize_t expr_writec_r(expr_writer writer,intptr_t fd,size_t count,int c,const struct expr_memtool *restrict mtl){
	char *buf;
	ssize_t r,sum;
	if(unlikely(!count))
		return 0;
	if(unlikely(count>(size_t)SSIZE_MAX))
		return SSIZE_MIN+2;
	if(count<=EXPR_WCBUFSIZE){
		buf=xmalloc(count);
		if(unlikely(!buf))
			return SSIZE_MIN+1;
		memset(buf,c,count);
		r=writer(fd,buf,count);
		xfree(buf);
		return r;
	}
	buf=xmalloc(EXPR_WCBUFSIZE);
	if(unlikely(!buf))
		return SSIZE_MIN+1;
	memset(buf,c,EXPR_WCBUFSIZE);
	sum=0;
	do {
		c_trywrite3(buf,EXPR_WCBUFSIZE,goto err);
		count-=EXPR_WCBUFSIZE;
	}while(count>EXPR_WCBUFSIZE);
	if(count){
		c_trywrite3(buf,count,goto err);
	}
	xfree(buf);
	return sum;
err:
	xfree(buf);
	return r;
}
#undef xmtl
#define xmtl flag->mtl
#define writeext(writer,fd,count,c) expr_writec_r(writer,fd,count,c,xmtl)

//#define flag_plusorspace(_f) ((_f)->bit&(UINT64_C(3)<<62))
#define flag_plus(_f) ((_f)->plus)
#define flag_space(_f) ((_f)->space)
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
	return (likely(sz>0)?writer(fd,(_s),(size_t)(sz)):0)

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
	size_t val=(size_t)arg->tint;
	char *endp=nbuf+32;
	char *p=endp;
	ssize_t ext,sum,r,sz;
	int minus;
	if(!val)
		minus=2;
	else if(val>(size_t)SSIZE_MAX){
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
			if(flag_plus(flag))
				*(--p)='+';
			else if(flag_space(flag))
				*(--p)=' ';
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
	cwrite_common(p,(endp-p));
}
static ssize_t converter_size(expr_writer writer,intptr_t fd,const union expr_argf *arg,struct expr_writeflag *flag){
	char nbuf[64];
	char dbuf[32];
	int64_t val=(int64_t)arg->tint;
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
			*(--p)=(flag->cap?"BKMGTPE":"bkmgtpe")[dim];
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
					dig=(size_t)(dp-dbuf);
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
			if(flag_plus(flag))
				*(--p)='+';
			else if(flag_space(flag))
				*(--p)=' ';
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
	cwrite_common(p,(endp-p));
}
static const char conv_btox[]={"0123456789abcdefx"};
static const char conv_btoX[]={"0123456789ABCDEFX"};
static ssize_t converter_xu(expr_writer writer,intptr_t fd,const union expr_argf *arg,struct expr_writeflag *flag,uint32_t base){
	char nbuf[72];
	size_t val=arg->zint,mod;
	char *endp=nbuf+72;
	char *p=endp;
	ssize_t ext,sum,r,sz;
	int positive;
	const char *conv_str=flag->cap?conv_btoX:conv_btox;
	if(val){
		positive=1;
		do {
			mod=val%base;
			val/=base;
			*(--p)=(char)conv_str[mod];
		}while(val);
	}else {
		positive=0;
		*(--p)='0';
	}
	if(positive&&flag->sharp){
		switch(base){
			case 8:
				*(--p)='0';
				break;
			case 16:
				*(--p)=flag->cap?'X':'x';
				*(--p)='0';
				break;
			default:
				break;
		}
	}
	switch(positive){
		case 1:
			if(flag_plus(flag))
				*(--p)='+';
			else if(flag_space(flag))
				*(--p)=' ';
			break;
		case 0:
			if(flag->plus)
				*(--p)='+';
			break;
		default:
			__builtin_unreachable();
	}
	cwrite_common(p,(endp-p));
}
#define conv_x(name,base) \
static ssize_t converter_##name(expr_writer writer,intptr_t fd,const union expr_argf *arg,struct expr_writeflag *flag){\
	return converter_xu(writer,fd,arg,flag,base);\
}
conv_x(x81,16);
conv_x(x82,2);
conv_x(x83,3);
conv_x(x84,4);
conv_x(x85,5);
conv_x(x86,6);
conv_x(x87,7);
conv_x(x88,8);
conv_x(x89,9);
conv_x(x8a,10);
conv_x(x8b,11);
conv_x(x8c,12);
conv_x(x8d,13);
conv_x(x8e,14);
conv_x(x8f,15);

#define extint_left(i,l,b) expr_extint_left(i,(size_t)(l),(uint64_t)(b))
#define extint_right(i,l,b) expr_extint_right(i,(size_t)(l),(uint64_t)(b))
#define extint_mul(i,l,m,w) expr_extint_mul(i,(size_t)(l),m,w)
#define extint_ascii_rev expr_extint_ascii_rev
#define extint_mirror(buf,size) ({\
	volatile char *_buf=(buf);\
	volatile char *_out=_buf+(size)-1;\
	char _swapbuf;\
	while(_out>_buf){\
		_swapbuf=*_out;\
		*_out=*_buf;\
		*_buf=_swapbuf;\
		--_out;\
		++_buf;\
	}\
})
// warning on gcc 13.3.0 if no volatile
struct doubleconverter_workspace {
	uint64_t w[69];
	uint64_t i[69-16];
	uint64_t ii[16];
	char v[1096];
	char n[];
};

#ifndef EXPR_FORMAT_NOFLOAT
#define EXPR_FORMAT_NOFLOAT 0
#endif

#if !(EXPR_FORMAT_NOFLOAT)
// the libc printf for double rounds to the closer one. but the code is too difficult to write, so this keeps truncate, round by the caller.
#define conv_fu(_start,_name,_how,_before_how) \
static ssize_t converteru_##_name(expr_writer writer,intptr_t fd,const union expr_argf *arg,struct expr_writeflag *flag,uint32_t base,int64_t shouldleft,struct doubleconverter_workspace *bufst,uint32_t nsize){\
	double val=arg->dbl;\
	;\
	char *endp;\
	char *p,*np;\
	int positive,f61,f58;\
	ssize_t ext,sum,r,sz,fsz;\
	size_t digit,width,ds;\
	const char *_conv_str=flag->cap?conv_btoX:conv_btox;\
	uint32_t baseodd;\
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
	f61=!!(flag->minus);\
	f58=!!(flag->eq);\
	digit=(size_t)flag_digit(flag,6);\
	if(digit>(size_t)SSIZE_MAX-1){\
		digit=(size_t)SSIZE_MAX-1;\
	}\
	width=flag_width(flag,0);\
	endp=nbuf+(size_t)nsize;\
	if(EXPR_EDSIGN(&val)){\
		EXPR_EDSIGN(&val)=0;\
		positive=0;\
	}else {\
		positive=1;\
	}\
	np=endp;\
	sum=0;\
	sz=(ssize_t)EXPR_EDEXP(&val)-1023;\
	if(sz>=0){\
		double fval;\
		fval=expr_internal_trunc(val);\
		val-=fval;\
		*iival=EXPR_EDBASE(&fval)|(UINT64_C(1)<<52);\
		sz-=52;\
		if(sz){\
			if(sz>0){\
				r=(ssize_t)extint_left(iival,1,(size_t)sz);\
			}else {\
				*iival>>=-sz;\
				r=1;\
			}\
		}else\
			r=1;\
		np-=extint_ascii_rev(iival,(size_t)r,conv_btox,np,base);\
	}else {\
		*(--np)='0';\
	}\
	if(base==16){\
		if(flag->sharp){\
			*(--np)=_conv_str[base];\
			*(--np)='0';\
		}\
	}\
	write_sign_to_np(!positive);\
	ds=(size_t)(endp-np);\
	p=vbuf+1096;\
	if(digit){\
		fsz=(ssize_t)EXPR_EDEXP(&val);\
		if(fsz)\
			*ival=EXPR_EDBASE(&val)|(UINT64_C(1)<<52);\
		else\
			*ival=EXPR_EDBASE(&val)<<1;\
		ext=1075-fsz;\
		r=1;\
		baseodd=base>>(uint32_t)ctz32(base);\
		if(baseodd>1&&ext){\
			r=(ssize_t)expr_extint_mulnp(ival,(size_t)r,baseodd,(uint32_t)ext,workspace);\
		}\
		if(shouldleft>0){\
			r=(ssize_t)expr_extint_left(ival,(size_t)r,(uint64_t)shouldleft);\
		}else if(shouldleft<0){\
			r=(ssize_t)expr_extint_right(ival,(size_t)r,(uint64_t)-shouldleft);\
		}\
		fsz=(ssize_t)extint_ascii_rev(ival,(size_t)r,_conv_str,p,base);\
		p-=fsz;\
		ext-=fsz;\
		if(ext>0){\
			p-=ext;\
			fsz+=ext;\
			memset(p,'0',(size_t)ext);\
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




#define conv_fu0(_name) conv_fu(,_name,{\
	c_trywrite(np,ds);\
	if(fsz){\
		c_trywrite(p,fsz);\
	}\
	if(!f58&&(size_t)fsz<=digit){\
		if(!fsz){\
			c_trywrite(".",1);\
			c_trywriteext('0',digit-(size_t)fsz);\
		}else\
			c_trywriteext('0',digit-(size_t)fsz+(size_t)1);\
	}\
},\
	ext=(ssize_t)(digit+1);\
	if(fsz>ext){\
		fsz=ext;\
	}\
	for(endp=p+fsz-1;endp>=p&&*endp=='0';){\
		--fsz;\
		--endp;\
	}\
	if(fsz==1)\
		fsz=0;\
	sz=(ssize_t)ds+(f58?fsz:ext);\
	debug("set sz=%zd",sz);\
)

#define conv_fue(_name) conv_fu(,_name,\
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
		size_t wsz;\
		++firp;\
		wsz=(size_t)(firp-np);\
		c_trywrite(np,wsz);\
		if(digit)\
			c_trywrite(".",1);\
		r1=(np+ds)-firp-iz;\
		if((size_t)r1>=digit){\
			ext=0;\
			r1=(ssize_t)digit;\
		}else {\
			ext=(ssize_t)digit-r1;\
		}\
		c_trywrite(firp,r1);\
		if(ext){\
			if(fsz){\
				ext=fsz>ext?ext:fsz;\
				c_trywrite(p,ext);\
			}else {\
				ext=1;\
				c_trywrite("0",1);\
			}\
		}\
		fsz=r1+ext;\
	}\
	if(!f58&&(size_t)fsz<digit){\
		c_trywriteext('0',(ssize_t)digit-fsz);\
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
	r1=(ssize_t)ds-1;\
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
	if((size_t)fsz>digit){\
		fsz=(ssize_t)digit;\
	}\
	for(endp=p+fsz-1;endp>=p&&*endp=='0';){\
		--fsz;\
		--endp;\
	}\
	*ebuf=(flag->cap?'E':'e');\
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
	if(ebuf+2<endp){\
		r2=endp-(ebuf+1);\
		assume(r2<=6);\
		extint_mirror(ebuf+2,r2);\
	};\
	esz=sz;\
	debug("set sz=%zd",sz);\
	sz+=(ssize_t)ds+(f58?fsz+!!digit:(ssize_t)digit+!!digit);\
	if(!fsz)\
		++sz;\
	debug("set sz=%zd",sz);\
	for(iz=0,endp=np+ds;--endp>np;){\
		if(*endp=='0')\
			++iz;\
		else\
			break;\
	}\
	debug("set sz=%zd",sz);\
	sz-=iz;\
	if(!digit)\
		--sz;\
	if(sz<0)\
		sz=SSIZE_MAX;\
	debug("set sz=%zd",sz);\
})
#define conv_f(_name,_base,_shift,_nsize) \
static ssize_t converter_##_name(expr_writer writer,intptr_t fd,const union expr_argf *arg,struct expr_writeflag *flag){\
	struct doubleconverter_workspace *bufst;\
	ssize_t r;\
	bufst=xmalloc(offsetof(struct doubleconverter_workspace,n)+(_nsize));\
	if(unlikely(!bufst))\
		return SSIZE_MIN+1;\
	r=converteru_plain(writer,fd,arg,flag,_base,_shift,bufst,_nsize);\
	xfree(bufst);\
	return r;\
}
#define conv_fe(_name,_base,_shift,_nsize) \
static ssize_t converter_##_name(expr_writer writer,intptr_t fd,const union expr_argf *arg,struct expr_writeflag *flag){\
	struct doubleconverter_workspace *bufst;\
	ssize_t r;\
	bufst=xmalloc(offsetof(struct doubleconverter_workspace,n)+(_nsize));\
	if(unlikely(!bufst))\
		return SSIZE_MIN+1;\
	r=converteru_expo(writer,fd,arg,flag,_base,_shift,bufst,_nsize);\
	xfree(bufst);\
	return r;\
}
#define writeext_f \
	debug("width=%zu,sz=%zd",width,sz);\
	ext=(ssize_t)width-sz;\
	if(ext>0){\
		c_trywriteext((flag->zero)?'0':' ',ext);\
	}
#define write_sign_to_p(_neg) \
		if(_neg){\
			*(p++)='-';\
		}else {\
			if(flag_plus(flag))\
				*(p++)='+';\
			else if(flag_space(flag))\
				*(p++)=' ';\
		}
#define write_sign_to_np(_neg) \
		if(_neg){\
			*(--np)='-';\
		}else {\
			if(flag_plus(flag))\
				*(--np)='+';\
			else if(flag_space(flag))\
				*(--np)=' ';\
		}
#define vbuf (bufst->v)
#define nbuf (bufst->n)
#define ival (bufst->i)
#define iival (bufst->ii)
#define workspace (bufst->w)
#define nbuf_size(x) (((x)+23u)&~7u)
conv_fu0(plain);
conv_fue(expo);
conv_f(xa1,16,+3,nbuf_size(256));
conv_f(xa2,2,0,nbuf_size(1024));
conv_f(xa3,3,-1,nbuf_size(646));
conv_f(xa4,4,+1,nbuf_size(512));
conv_f(xa5,5,-1,nbuf_size(441));
conv_f(xa6,6,0,nbuf_size(396));
conv_f(xa7,7,-1,nbuf_size(364));
conv_f(xa8,8,+2,nbuf_size(342));
conv_f(xa9,9,-1,nbuf_size(323));
conv_f(xaa,10,0,nbuf_size(308));
conv_f(xab,11,-1,nbuf_size(296));
conv_f(xac,12,+2,nbuf_size(285));
conv_f(xad,13,-1,nbuf_size(276));
conv_f(xae,14,0,nbuf_size(268));
conv_f(xaf,15,-1,nbuf_size(262));

conv_fe(fe,10,0,nbuf_size(308+8));
//256,1024,646,512,441,396,364,342,323,308,296,285,276,268,262;
#undef nbuf
#undef vbuf
#undef ival
#undef iival
#undef workspace

static ssize_t log10_floor_inaccurate(double x){
	uint64_t ix;
	ssize_t r;
	if(x<1.0/10000)
		return -5;
	if(x<1.0)
		return -1;
	ix=(uint64_t)x;
	r=0;
#define check_div(v,e) if(ix>=UINT64_C(v)){ix/=UINT64_C(v);r+=(e);}
	check_div(10000000000,10);
	check_div(100000,5);
	check_div(1000,3);
	check_div(100,2);
	return ix>=10?r+1:r;
}
static ssize_t faction_g(expr_writer writer,intptr_t fd,const union expr_argf *arg,struct expr_writeflag *flag){
	double val=fabs(arg->dbl);
	ssize_t vl;
	return ((val==0.0||((vl=log10_floor_inaccurate(val))>=-4&&vl<(ssize_t)flag_digit(flag,6)))?
	converter_xaa:converter_fe)(writer,fd,arg,(flag->eq=1,flag));
}
static ssize_t faction_G(expr_writer writer,intptr_t fd,const union expr_argf *arg,struct expr_writeflag *flag){
	double val=fabs(arg->dbl);
	ssize_t vl;
	return ((val==0.0||((vl=log10_floor_inaccurate(val))>=-4&&vl<(ssize_t)flag_digit(flag,6)))?
	converter_xaa:converter_fe)(writer,fd,arg,(flag->eq=1,flag));
}

#endif
static ssize_t faction_s(expr_writer writer,intptr_t fd,const union expr_argf *arg,struct expr_writeflag *flag){
	size_t len;
	ssize_t r,sum,ext,sz;
	if(unlikely(!arg->str)){
		cwrite_common("(null)",6);
	}
	len=flag->digit_set?strnlen(arg->str,(size_t)flag->digit):strlen(arg->str);
	cwrite_common(arg->str,(ssize_t)len);
}
static ssize_t faction_S(expr_writer writer,intptr_t fd,const union expr_argf *arg,struct expr_writeflag *flag){
	size_t len=(size_t)flag->digit;
	ssize_t r,sum,ext,sz;
	cwrite_common(arg->str,(ssize_t)len);
}
static ssize_t faction_hb(expr_writer writer,intptr_t fd,const union expr_argf *arg,struct expr_writeflag *flag,uint8_t buf[960]){
	size_t len=(size_t)flag->digit;
	ssize_t r,sum,ext;
	uint8_t *p,*endp,*datap;
	uint8_t data;
	int f61=!!(flag->minus),f60=!!(flag->sharp),f63=!!(flag->plus);
	const char *conv_str=flag->cap?conv_btoX:conv_btox;
	ext=(ssize_t)flag_width(flag,0)-(f60?4:2)*(ssize_t)len;
	if(f63&&len>1)
		ext-=(ssize_t)(len-1);
	sum=0;
	if(!f61&&ext>0){
		c_trywriteext((flag->zero)?'0':' ',ext);
	}
	if(len){
		p=buf;
		endp=buf+960;
		for(datap=arg->addr;;){
			data=*datap;
			if(f60){
				*(p++)='0';
				*(p++)=(uint8_t)conv_str[16];
			}
			*(p++)=(uint8_t)conv_str[data>>4];
			*(p++)=(uint8_t)conv_str[data&15];
			if(f63&&likely(len>1))
				*(p++)=',';
			if(unlikely(!--len))
				break;
			if(unlikely(p>=endp)){
				c_trywrite(buf,960);
				p=buf;
			}
			++datap;
		}
		if(likely(p>buf)){
			len=(size_t)(p-buf);
			c_trywrite(buf,len);
		}
	}
	if(f61&&ext>0){
		c_trywriteext((flag->zero)?'0':' ',ext);
	}
	return sum;
}
static ssize_t faction_h(expr_writer writer,intptr_t fd,const union expr_argf *arg,struct expr_writeflag *flag){
	uint8_t *buf;
	ssize_t r;
	buf=xmalloc(960);
	if(unlikely(!buf))
		return SSIZE_MIN+1;
	r=faction_hb(writer,fd,arg,flag,buf);
	xfree(buf);
	return r;
}
static ssize_t faction_c(expr_writer writer,intptr_t fd,const union expr_argf *arg,struct expr_writeflag *flag){
	ssize_t r,sum,ext,sz;
	cwrite_common(&arg->ch,(ssize_t)1);
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
			((_op)>=(uint8_t)'\x91'&&(_op)<=(uint8_t)'\x9f')||\
			((_op)>=(uint8_t)'\xb1'&&(_op)<=(uint8_t)'\xbf')||\
			((_op)>='A'&&(_op)<='Z')\
			?1:0),\
	.op={_op,##__VA_ARGS__},\
}
const struct expr_writefmt expr_writefmts_default[]={
	fmtc_register(faction_percent,0,0,'%'),

	fmtc_register(converter_x81,1,EXPR_FLAGTYPE_UNSIGNED,(uint8_t)'\x81','x'),
	fmtc_register(converter_x82,1,EXPR_FLAGTYPE_UNSIGNED,(uint8_t)'\x82',(uint8_t)'\x92','b'),
	fmtc_register(converter_x83,1,EXPR_FLAGTYPE_UNSIGNED,(uint8_t)'\x83',(uint8_t)'\x93'),
	fmtc_register(converter_x84,1,EXPR_FLAGTYPE_UNSIGNED,(uint8_t)'\x84',(uint8_t)'\x94'),
	fmtc_register(converter_x85,1,EXPR_FLAGTYPE_UNSIGNED,(uint8_t)'\x85',(uint8_t)'\x95'),
	fmtc_register(converter_x86,1,EXPR_FLAGTYPE_UNSIGNED,(uint8_t)'\x86',(uint8_t)'\x96'),
	fmtc_register(converter_x87,1,EXPR_FLAGTYPE_UNSIGNED,(uint8_t)'\x87',(uint8_t)'\x97'),
	fmtc_register(converter_x88,1,EXPR_FLAGTYPE_UNSIGNED,(uint8_t)'\x88',(uint8_t)'\x98','o'),
	fmtc_register(converter_x89,1,EXPR_FLAGTYPE_UNSIGNED,(uint8_t)'\x89',(uint8_t)'\x99'),
	fmtc_register(converter_x8a,1,EXPR_FLAGTYPE_UNSIGNED,(uint8_t)'\x8a',(uint8_t)'\x9a','u'),
	fmtc_register(converter_x8b,1,EXPR_FLAGTYPE_UNSIGNED,(uint8_t)'\x8b'),
	fmtc_register(converter_x8c,1,EXPR_FLAGTYPE_UNSIGNED,(uint8_t)'\x8c'),
	fmtc_register(converter_x8d,1,EXPR_FLAGTYPE_UNSIGNED,(uint8_t)'\x8d'),
	fmtc_register(converter_x8e,1,EXPR_FLAGTYPE_UNSIGNED,(uint8_t)'\x8e'),
	fmtc_register(converter_x8f,1,EXPR_FLAGTYPE_UNSIGNED,(uint8_t)'\x8f'),

	fmtc_register(converter_x81,1,EXPR_FLAGTYPE_UNSIGNED,(uint8_t)'\x91','X'),
	fmtc_register(converter_x8b,1,EXPR_FLAGTYPE_UNSIGNED,(uint8_t)'\x9b'),
	fmtc_register(converter_x8c,1,EXPR_FLAGTYPE_UNSIGNED,(uint8_t)'\x9c'),
	fmtc_register(converter_x8d,1,EXPR_FLAGTYPE_UNSIGNED,(uint8_t)'\x9d'),
	fmtc_register(converter_x8e,1,EXPR_FLAGTYPE_UNSIGNED,(uint8_t)'\x9e'),
	fmtc_register(converter_x8f,1,EXPR_FLAGTYPE_UNSIGNED,(uint8_t)'\x9f'),

	fmtc_register(faction_s,1,EXPR_FLAGTYPE_ADDR,'s'),
	fmtc_register(faction_S,2,EXPR_FLAGTYPE_ADDR,'S'),
	fmtc_register(faction_c,1,EXPR_FLAGTYPE_UNSIGNED,'c'),
	fmtc_register(converter_d,1,EXPR_FLAGTYPE_SIGNED,'d'),
	fmtc_register(faction_p,1,EXPR_FLAGTYPE_ADDR,'p'),
	fmtc_register(faction_P,1,EXPR_FLAGTYPE_ADDR,'P'),
	fmtc_register(faction_h,2,EXPR_FLAGTYPE_ADDR,'h'),
	fmtc_register(faction_h,2,EXPR_FLAGTYPE_ADDR,'H'),
	fmtc_register(converter_size,1,EXPR_FLAGTYPE_SIGNED,'m'),
	fmtc_register(converter_size,1,EXPR_FLAGTYPE_SIGNED,'M'),

#if !(EXPR_FORMAT_NOFLOAT)
	fmtc_register(converter_xa1,1,EXPR_FLAGTYPE_DOUBLE,(uint8_t)'\xa1','a'),
	fmtc_register(converter_xa2,1,EXPR_FLAGTYPE_DOUBLE,(uint8_t)'\xa2'),
	fmtc_register(converter_xa3,1,EXPR_FLAGTYPE_DOUBLE,(uint8_t)'\xa3'),
	fmtc_register(converter_xa4,1,EXPR_FLAGTYPE_DOUBLE,(uint8_t)'\xa4'),
	fmtc_register(converter_xa5,1,EXPR_FLAGTYPE_DOUBLE,(uint8_t)'\xa5'),
	fmtc_register(converter_xa6,1,EXPR_FLAGTYPE_DOUBLE,(uint8_t)'\xa6'),
	fmtc_register(converter_xa7,1,EXPR_FLAGTYPE_DOUBLE,(uint8_t)'\xa7'),
	fmtc_register(converter_xa8,1,EXPR_FLAGTYPE_DOUBLE,(uint8_t)'\xa8'),
	fmtc_register(converter_xa9,1,EXPR_FLAGTYPE_DOUBLE,(uint8_t)'\xa9'),
	fmtc_register(converter_xaa,1,EXPR_FLAGTYPE_DOUBLE,(uint8_t)'\xaa','f'),
	fmtc_register(converter_xab,1,EXPR_FLAGTYPE_DOUBLE,(uint8_t)'\xab'),
	fmtc_register(converter_xac,1,EXPR_FLAGTYPE_DOUBLE,(uint8_t)'\xac'),
	fmtc_register(converter_xad,1,EXPR_FLAGTYPE_DOUBLE,(uint8_t)'\xad'),
	fmtc_register(converter_xae,1,EXPR_FLAGTYPE_DOUBLE,(uint8_t)'\xae'),
	fmtc_register(converter_xaf,1,EXPR_FLAGTYPE_DOUBLE,(uint8_t)'\xaf'),

	fmtc_register(converter_xa1,1,EXPR_FLAGTYPE_DOUBLE,(uint8_t)'\xb1','A'),
	fmtc_register(converter_xa2,1,EXPR_FLAGTYPE_DOUBLE,(uint8_t)'\xb2','B'),
	fmtc_register(converter_xa3,1,EXPR_FLAGTYPE_DOUBLE,(uint8_t)'\xb3'),
	fmtc_register(converter_xa4,1,EXPR_FLAGTYPE_DOUBLE,(uint8_t)'\xb4'),
	fmtc_register(converter_xa5,1,EXPR_FLAGTYPE_DOUBLE,(uint8_t)'\xb5'),
	fmtc_register(converter_xa6,1,EXPR_FLAGTYPE_DOUBLE,(uint8_t)'\xb6'),
	fmtc_register(converter_xa7,1,EXPR_FLAGTYPE_DOUBLE,(uint8_t)'\xb7'),
	fmtc_register(converter_xa8,1,EXPR_FLAGTYPE_DOUBLE,(uint8_t)'\xb8','O'),
	fmtc_register(converter_xa9,1,EXPR_FLAGTYPE_DOUBLE,(uint8_t)'\xb9'),
	fmtc_register(converter_xaa,1,EXPR_FLAGTYPE_DOUBLE,(uint8_t)'\xba','F'),
	fmtc_register(converter_xab,1,EXPR_FLAGTYPE_DOUBLE,(uint8_t)'\xbb'),
	fmtc_register(converter_xac,1,EXPR_FLAGTYPE_DOUBLE,(uint8_t)'\xbc'),
	fmtc_register(converter_xad,1,EXPR_FLAGTYPE_DOUBLE,(uint8_t)'\xbd'),
	fmtc_register(converter_xae,1,EXPR_FLAGTYPE_DOUBLE,(uint8_t)'\xbe'),
	fmtc_register(converter_xaf,1,EXPR_FLAGTYPE_DOUBLE,(uint8_t)'\xbf'),

	fmtc_register(converter_fe,1,EXPR_FLAGTYPE_DOUBLE,'e'),
	fmtc_register(converter_fe,1,EXPR_FLAGTYPE_DOUBLE,'E'),
	fmtc_register(faction_g,1,EXPR_FLAGTYPE_DOUBLE,'g'),
	fmtc_register(faction_G,1,EXPR_FLAGTYPE_DOUBLE,'G'),
#define FLTIDX(n) n
#else
#define FLTIDX(n) 0
#endif
};
const uint8_t expr_writefmts_default_size=arrsize(expr_writefmts_default);
const struct expr_ftable expr_ftable_default[1]={{
	.fmts=expr_writefmts_default,
	.c={
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

['+']=EXPR_FPLUS,
[' ']=EXPR_FSPACE,
['-']=EXPR_FMINUS,
['#']=EXPR_FSHARP,
['0']=EXPR_FZERO,
['=']=EXPR_FEQ,
['?']=EXPR_FSAVED,
['&']=EXPR_FADDR,
['!']=EXPR_FCAP,

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
['s']=23,
['S']=24,
['c']=25,
['d']=26,
['p']=27,
['P']=28,
['h']=29,
['H']=30,
['m']=31,
['M']=32,
[(uint8_t)'\xa1']=FLTIDX(33),['a']=FLTIDX(33),
[(uint8_t)'\xa2']=FLTIDX(34),
[(uint8_t)'\xa3']=FLTIDX(35),
[(uint8_t)'\xa4']=FLTIDX(36),
[(uint8_t)'\xa5']=FLTIDX(37),
[(uint8_t)'\xa6']=FLTIDX(38),
[(uint8_t)'\xa7']=FLTIDX(39),
[(uint8_t)'\xa8']=FLTIDX(40),
[(uint8_t)'\xa9']=FLTIDX(41),
[(uint8_t)'\xaa']=FLTIDX(42),['f']=FLTIDX(42),
[(uint8_t)'\xab']=FLTIDX(43),
[(uint8_t)'\xac']=FLTIDX(44),
[(uint8_t)'\xad']=FLTIDX(45),
[(uint8_t)'\xae']=FLTIDX(46),
[(uint8_t)'\xaf']=FLTIDX(47),
[(uint8_t)'\xb1']=FLTIDX(48),['A']=FLTIDX(48),
[(uint8_t)'\xb2']=FLTIDX(49),['B']=FLTIDX(49),
[(uint8_t)'\xb3']=FLTIDX(50),
[(uint8_t)'\xb4']=FLTIDX(51),
[(uint8_t)'\xb5']=FLTIDX(52),
[(uint8_t)'\xb6']=FLTIDX(53),
[(uint8_t)'\xb7']=FLTIDX(54),
[(uint8_t)'\xb8']=FLTIDX(55),['O']=FLTIDX(55),
[(uint8_t)'\xb9']=FLTIDX(56),
[(uint8_t)'\xba']=FLTIDX(57),['F']=FLTIDX(57),
[(uint8_t)'\xbb']=FLTIDX(58),
[(uint8_t)'\xbc']=FLTIDX(59),
[(uint8_t)'\xbd']=FLTIDX(60),
[(uint8_t)'\xbe']=FLTIDX(61),
[(uint8_t)'\xbf']=FLTIDX(62),
['e']=FLTIDX(63),
['E']=FLTIDX(64),
['g']=FLTIDX(65),
['G']=FLTIDX(66),
	},
	.fph='%',.digit='.',.bitwidth=':',.readarg='*',.readarg2='*',
	.fph_set=1,.digit_set=1,.bitwidth_set=1,.readarg_set=1,.readarg2_set=1,
	.unused_bf=0,
	.unused={0},
}};
#define scannum(base) \
	while(nptr<endp){\
		if(base<=10)\
			get=(uint8_t)*nptr-(uint8_t)'0';\
		else\
			get=expr_ntable(*nptr);\
		if(unlikely(get>=base))\
			goto out;\
		if(unlikely(mulo(r,base)))\
			goto out;\
		if(unlikely(addo(r,get)))\
			goto out;\
		++nptr;\
	}\
	goto out
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
	scannum(10);
out:
	debug("result:%zd",(ssize_t)(neg?-r:r));
	*outval=(ssize_t)(neg?-r:r);
	return nptr;
}
static inline const char *internal_strtoz_x(const char *restrict nptr,const char *endp,ssize_t *restrict outval){
	size_t r=0;
	unsigned int get;
	scannum(16);
out:
	debug("result:%zd",(ssize_t)r);
	*outval=(ssize_t)r;
	return nptr;
}
static inline const char *internal_strtoz_o(const char *restrict nptr,const char *endp,ssize_t *restrict outval){
	size_t r=0;
	unsigned int get;
	scannum(8);
out:
	debug("result:%zd",(ssize_t)r);
	*outval=(ssize_t)r;
	return nptr;
}
static inline const char *internal_strtoz_oux(const char *restrict nptr,const char *endp,ssize_t *restrict outval,int allowneg){
	const char *n0=nptr;
	size_t r=0;
	int neg=0,base=0;
	unsigned int get;
	if(allowneg&&nptr<endp&&*nptr=='-'){
		++nptr;
		neg=1;
	}
	if(nptr<endp&&*nptr=='0'){
		++nptr;
		if(nptr<endp&&*nptr=='x'){
			++nptr;
			base=2;
		}else
			base=1;
	}
	if(unlikely(nptr>=endp)){
		debug("result:?");
		return n0;
	}
	switch(base){
		case 0:
			if(unlikely((uint8_t)*nptr-(uint8_t)'0'>=(uint8_t)10))
				return n0;
			break;
		case 1:
			if(unlikely((uint8_t)*nptr-(uint8_t)'0'>=(uint8_t)8))
				return n0;
			break;
		case 2:
			if(unlikely(expr_ntable(*nptr)>=(uint8_t)16))
				return n0;
			break;
		default:
			__builtin_unreachable();
	}
	switch(base){
		case 0:
			scannum(10);
		case 1:
			scannum(8);
		case 2:
			scannum(16);
		default:
			__builtin_unreachable();
	}
out:
	debug("result:%zd,nptr:%p",(ssize_t)(neg?-r:r),(void *)(uintptr_t)nptr);
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

struct expr_vwritef_workspace {
	const char *endp,*fmt_old,*fmt0;
	ssize_t ret;
	ssize_t v;
	const struct expr_writefmt *wfp;
	const char *delim,*tail,*d1;
	size_t delimsz,tailsz,arrlen;
	size_t loop;
	ptrdiff_t index;
	struct expr_writeflag flag[1];
	const char *fmt_save[8];
	const char **fmt_save_current;
	union {
		const union expr_argf *arg_cur;
		uintptr_t aps;
		union expr_argf old;
	};
	union expr_argf save;
	uint8_t forward,arrwid,current,r1;
};
#undef xmtl
#define xmtl mtl
ssize_t expr_vwriteft_r(expr_writer writer,intptr_t fd,const char *restrict fmt,size_t fmtlen,expr_argfetch arg,intptr_t addr,const struct expr_ftable *restrict table,const struct expr_memtool *restrict mtl){
	struct expr_vwritef_workspace *restrict wsp;
	if(unlikely(!fmtlen))
		return 0;
#define endp wsp->endp
#define fmt_old wsp->fmt_old
#define fmt0 wsp->fmt0
#define ret wsp->ret
#define v wsp->v
#define wfp wsp->wfp
#define flag wsp->flag
#define delim wsp->delim
#define tail wsp->tail
#define d1 wsp->d1
#define delimsz wsp->delimsz
#define tailsz wsp->tailsz
#define arrlen wsp->arrlen
#define loop wsp->loop
#define index wsp->index
#define forward wsp->forward
#define arrwid wsp->arrwid
#define current wsp->current
#define r1 wsp->r1
#define fmt_save wsp->fmt_save
#define fmt_save_current wsp->fmt_save_current
#define save wsp->save
#define arg_cur wsp->arg_cur
#define aps wsp->aps
#define old wsp->old
	wsp=xmalloc(sizeof(struct expr_vwritef_workspace));
	if(unlikely(!wsp)){
		return SSIZE_MIN+1;
	}
	endp=fmt+fmtlen;
	fmt_old=fmt;
	fmt0=fmt;
	ret=0;
	delim=NULL;
	tail=NULL;
	delimsz=0;
	tailsz=0;
	arrlen=0;
	loop=0;
	index=0;
	forward=0;
	arrwid=0;
#define wf_return(retval) {xfree(wsp);return (retval);}
#define wf_trywrite(buf,sz) {\
	v=writer(fd,(buf),(size_t)(sz));\
	if(unlikely(v<0))\
		wf_return(v);\
	if(unlikely(v<(ssize_t)(sz))){\
		v+=TRUNC_CODE+ret;\
		wf_return(v);\
	}\
	ret+=v;\
}
	if(unlikely(!table->fph_set)){
		wf_trywrite(fmt,fmtlen);
		goto end;
	}
	fmt_save_current=fmt_save;
	flag->mtl=mtl;
next:
	fmt=memchr(fmt,table->fph,(size_t)(endp-fmt));
	if(unlikely(!fmt)){
		loop=(size_t)(endp-fmt_old);
		wf_trywrite(fmt_old,loop);
		goto end;
	}
	v=fmt-fmt_old;
	if(v>0){
		wf_trywrite(fmt_old,v);
		fmt_old=fmt;
	}
#define fmt_inc_check ++fmt;\
	if(unlikely(fmt>=endp))\
		goto end
	fmt_inc_check;
	flag->bit=0;
#define fmt_setflag(_field) \
	flag->_field=1;\
	fmt_inc_check;\
	goto reflag
reflag:
	switch(table->c[(uint8_t)*fmt]){
		case EXPR_FPLUS:
			fmt_setflag(plus);
		case EXPR_FSPACE:
			fmt_setflag(space);
		case EXPR_FMINUS:
			fmt_setflag(minus);
		case EXPR_FSHARP:
			fmt_setflag(sharp);
		case EXPR_FZERO:
			fmt_setflag(zero);
		case EXPR_FEQ:
			fmt_setflag(eq);
		case EXPR_FSAVED:
			fmt_setflag(saved);
		case EXPR_FADDR:
			fmt_setflag(addr);
		case EXPR_FCAP:
			fmt_setflag(cap);
		default:
			break;
	}
#define t_copy(_d,_s,T) *(T *)(_d)=*(T *)(_s)
#define t_ocopy(_d,_s,T,_off) *(T *)((uintptr_t)(_d)+(_off))=*(T *)((uintptr_t)(_s)+(_off))
#define bit_copy(_d,_s,_bit) memcpy(_d,_s,_bit)
#define argnext1 ++index;debug("arg index to %zd",index)
#define argnext(N) index+=(ptrdiff_t)(N);debug("arg index to %zd",index)
#define argback(N) index-=(ptrdiff_t)(N);debug("arg index back to %zd",index)
#define goto_argfail {debug("arg failed");wf_return(SSIZE_MIN+3);}
#define goto_argfail1(e) {debug("arg failed");wf_return(e);}
#define argt(_type) ({register const union expr_argf *__arg;flag->type=(_type);__arg=arg(index,flag,addr);if(unlikely(!__arg)){debug("cannot get arg[%zd]",index);goto_argfail;}__arg;})
#define arg1 argt(EXPR_FLAGTYPE_ADDR)
#define get_next_arg64(dest,dtype,_after,_type,_field,_afield) \
	if(table->readarg_set&&*fmt==table->readarg){\
		fmt_inc_check;\
		if(table->readarg2_set&&*fmt==table->readarg2){\
			dest=*argt(EXPR_FLAGTYPE_ADDR)->_afield;\
			fmt_inc_check;\
		}else\
			dest=argt(_type)->_field;\
		_after;\
		argnext1;\
	}else {\
		fmt_old=fmt;\
		fmt=internal_strtoz_10(fmt,endp,&v);\
		if(fmt>fmt_old){\
			dest=(dtype)v;\
			_after;\
			if(unlikely(fmt>=endp))\
				goto end;\
		}\
	}
	get_next_arg64(flag->width,size_t,flag->width_set=1,EXPR_FLAGTYPE_UNSIGNED,zint,zaddr);
	if(table->digit_set&&*fmt==table->digit){
		fmt_inc_check;
		get_next_arg64(flag->digit,ssize_t,flag->digit_set=1,EXPR_FLAGTYPE_SIGNED,tint,taddr);
	}
	if(table->bitwidth_set&&*fmt==table->bitwidth){
		fmt_inc_check;
		get_next_arg64(flag->argsize,uint64_t,,EXPR_FLAGTYPE_UNSIGNED,zint,zaddr);
	}
	current=*(const uint8_t *)fmt;
current_get:
	flag->op=current;
	switch((r1=table->c[current])){
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
				ssize_t *restrict ar;\
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
			v=(ssize_t)flag_width(flag,1);
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
			arrwid=(uint8_t)flag_digit(flag,sizeof(size_t));
			if(unlikely(arrwid>sizeof(union expr_argf)||!arrwid))
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
			v=(ssize_t)flag_width(flag,0);
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
				*arg1->uiaddr+=(size_t)flag->digit;
			else
				++(*arg1->uiaddr);
			argnext1;
			break;
		case EXPR_FMTC_DEC:
			if(flag->width_set)
				*arg1->uiaddr/=flag->width;
			if(flag->digit_set)
				*arg1->uiaddr-=(size_t)flag->digit;
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
			v=writer(fd,NULL,flag->sharp?SIZE_MAX:0);
			if(unlikely(v<0))
				wf_return(v);
			ret+=v;
			break;
#pragma GCC diagnostic pop

#define fmt_repeat if(!loop)loop=1;for(;loop;--loop)
#define fmt_once(_arg) \
	v=wfp->converter(writer,fd,(_arg),flag);\
	if(unlikely(v<0))\
		wf_return(v);\
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
	}
#define fmt_dorepeat_fwd(_arg) \
	fmt_repeat {\
		arg_cur=(_arg);\
		if(unlikely(!arg_cur))\
			goto_argfail;\
		fmt_once(arg_cur);\
		if(tailsz){\
			wf_trywrite(tail,tailsz);\
		}\
		if(delimsz&&loop>1){\
			wf_trywrite(delim,delimsz);\
		}\
		argnext1;\
	}

		default:
			wfp=table->fmts+(r1-1);//r
wfp_get:
			if(unlikely(wfp->digit_check&&!flag->digit_set))
				goto_argfail1(SSIZE_MIN+4);
			if(wfp->setcap)
				flag->cap=1;
			if(wfp->no_arg){
				fmt_dorepeat(NULL);
				forward=0;
			}else if(flag->saved){
				fmt_dorepeat((const union expr_argf *)&save);
				forward=0;
			}else if(arrlen){
				expr_umaxf_t val,mask;
				flag->argsize=sizeof(void *);
				aps=arg1->uint;
				mask=(UINT64_C(1)<<(arrwid*8))-1;
				argnext1;
				for(;;){
					if(arrwid<8){
						bit_copy(&val,(const void *)aps,arrwid);
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
				if(forward){
					fmt_dorepeat_fwd(argt(wfp->type));
				}else {
					arg_cur=argt(wfp->type);
					if(unlikely(!arg_cur))
						goto_argfail;
					fmt_dorepeat(arg_cur);
					argnext1;
				}
#pragma GCC diagnostic pop
			}
			break;
	}
	fmt_inc_check;
continue_keepfmt:
	fmt_old=fmt;
	goto next;
end:
	wf_return(ret);
#undef endp
#undef fmt_old
#undef fmt0
#undef ret
#undef v
#undef wfp
#undef flag
#undef delim
#undef tail
#undef d1
#undef delimsz
#undef tailsz
#undef arrlen
#undef loop
#undef index
#undef forward
#undef arrwid
#undef current
#undef r1
#undef fmt_save
#undef fmt_save_current
#undef save
#undef arg_cur
#undef aps
#undef old
}
#undef arg1
#undef argt
#undef goto_argfail
#undef goto_argfail1
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
#undef wf_return
#undef wf_trywrite
struct writef_args {
	const union expr_argf *base;
	size_t arglen;
};
static const union expr_argf *arr_arg(ptrdiff_t index,const struct expr_writeflag *flag,intptr_t addr){
	struct writef_args *wa=(struct writef_args *)addr;
	const union expr_argf *r=wa->base+index;
	range_checkn(r,wa->base,wa->arglen,goto err);
	debug("getting arg[%zd] (base=%p,len=%zu)=%p",index,(void *)(uintptr_t)wa->base,wa->arglen,(void *)(uintptr_t)r);
	return flag->addr?r->aaddr:r;
err:
	debug("getting arg[%zd] (base=%p,len=%zu)=NULL",index,(void *)(uintptr_t)wa->base,wa->arglen);
	return NULL;
}
ssize_t expr_writeft_r(expr_writer writer,intptr_t fd,const char *restrict fmt,size_t fmtlen,const union expr_argf *restrict args,size_t arglen,const struct expr_ftable *restrict table,const struct expr_memtool *restrict mtl){
	struct writef_args wa[1];
	wa->base=args;
	wa->arglen=arglen;
	return expr_vwriteft_r(writer,fd,fmt,fmtlen,arr_arg,(intptr_t)wa,table,mtl);
}
struct fmtarg {
	va_list ap;
	ptrdiff_t index_old;
	union expr_argf save[1];
};
static const union expr_argf *ap_getarg(ptrdiff_t index,const struct expr_writeflag *flag,intptr_t addr){
	struct fmtarg *restrict a=(struct fmtarg *)addr;
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
					a->save->sint=va_arg(a->ap,int);
					break;
				default:
					a->save->smax=va_arg(a->ap,ssize_t);
					break;
			}
			break;
		case EXPR_FLAGTYPE_UNSIGNED:
			switch(flag->argsize){
				case 1 ... sizeof(int):
					a->save->uint=va_arg(a->ap,unsigned int);
					break;
				default:
					a->save->umax=va_arg(a->ap,size_t);
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
#define ap_common(_start,_end,_table) \
	ssize_t r;\
	struct fmtarg a[1];\
	a->index_old=-1;\
	_start;\
	r=expr_vwriteft_r(writer,fd,fmt,fmtlen,ap_getarg,(intptr_t)a,_table,mtl);\
	_end
ssize_t expr_vapwriteft_r(expr_writer writer,intptr_t fd,const char *restrict fmt,size_t fmtlen,const struct expr_ftable *restrict table,const struct expr_memtool *restrict mtl,va_list ap){
	ap_common(va_copy(a->ap,ap),va_end(a->ap),table);
	return r;
}
ssize_t expr_apwriteft_r(expr_writer writer,intptr_t fd,const char *restrict fmt,size_t fmtlen,const struct expr_ftable *restrict table,const struct expr_memtool *restrict mtl,...){
	ap_common(va_start(a->ap,mtl),va_end(a->ap),table);
	return r;
}

#define memfd_write_common \
	size_t old_size;\
	if(unlikely(size>(ssize_t)SSIZE_MAX)){\
		return SSIZE_MIN+2;\
	}\
	old_size=size;\
	if(unlikely(mfd->size<size)){\
		size=mfd->size;\
	}\
	if(likely(size)){\
		memcpy(mfd->buf,buf,size);\
		mfd->buf+=size;\
		mfd->size-=size;\
	}\
	return (ssize_t)old_size
static ssize_t expr_memfd_write_warped(intptr_t fd,const void *buf,size_t size){
	struct expr_memfd *restrict mfd=(struct expr_memfd *)fd;
	memfd_write_common;
}
ssize_t expr_snwriteft_r(char *buf,size_t buflen,const char *restrict fmt,size_t fmtlen,const union expr_argf *restrict args,size_t arglen,const struct expr_ftable *restrict table,const struct expr_memtool *restrict mtl){
	struct writef_args wa[1];
	struct expr_memfd mfd[1];
	ssize_t r;
	mfd->buf=buf;
	mfd->size=buflen;
	wa->base=args;
	wa->arglen=arglen;
	r=expr_vwriteft_r(expr_memfd_write_warped,((intptr_t)mfd),fmt,fmtlen,arr_arg,(intptr_t)wa,table,mtl);
	if(unlikely(r<0))
		return r;
	if(unlikely((size_t)r>=buflen)){
		if(buflen)
			buf[buflen-1]=0;
	}else
		buf[r]=0;
	return r;
}
#define sp_common(_start,_end,_table) \
	struct expr_memfd mfd[1];\
	mfd->buf=buf;\
	mfd->size=buflen;\
	{\
		ap_common(_start,_end,_table);\
		if(unlikely(r<0))\
			return r;\
		if(unlikely((size_t)r>=buflen)){\
			if(buflen)\
				buf[buflen-1]=0;\
		}else\
			buf[r]=0;\
		return r;\
	}
#define writer expr_memfd_write_warped
#define fd ((intptr_t)mfd)
ssize_t expr_vsnprintft_r(char *buf,size_t buflen,const char *restrict fmt,size_t fmtlen,const struct expr_ftable *restrict table,const struct expr_memtool *restrict mtl,va_list ap){
	sp_common(va_copy(a->ap,ap),va_end(a->ap),table);
}
ssize_t expr_snprintft_r(char *buf,size_t buflen,const char *restrict fmt,size_t fmtlen,const struct expr_ftable *restrict table,const struct expr_memtool *restrict mtl,...){
	sp_common(va_start(a->ap,mtl),va_end(a->ap),table);
}
#undef writer
#undef fd
// libexpr will never provide sprintf()
static inline const char *internal_strtom_10(const char *restrict nptr,const char *endp,ssize_t *restrict outval){
	size_t r=0;
	int neg=0;
	unsigned int get;
	if(likely(nptr<endp)){
		if(*nptr=='-'){
			++nptr;
			neg=1;
		}
		switch(*nptr){
			case '0' ... '9':
				break;
			default:
				return neg?nptr-1:nptr;
		}
	}
	scannum(10);
out:
	debug("result:%zd",(ssize_t)(neg?-r:r));
	if(nptr<endp){
		switch(*nptr){
			case 'B':
			case 'b':
				++nptr;
				break;
#define setdim(d,n) \
			case (d):\
			case (d)^32:\
				++nptr;\
				if(likely(r<=(SIZE_MAX>>(n))))\
					r<<=(n);\
				else{\
					*outval=SSIZE_MAX;\
					return nptr;\
				}\
				break
			setdim('K',10);
			setdim('M',20);
			setdim('G',30);
			setdim('T',40);
			setdim('P',50);
			setdim('E',60);
		}
	}
	*outval=(ssize_t)(neg?-r:r);
	return nptr;
}
static inline const char *internal_strtos(const char *nptr,const char *endp,const char *c,char *restrict out,size_t *restrict outval){
	size_t outlen=(size_t)(endp-nptr);
	if(c){
		const char *r;
		r=memchr(nptr,*c,outlen);
		if(r)
			outlen=(size_t)(r-nptr);

	}
	if(!out){
		if(outval)
			*outval=outlen;
		return nptr+outlen;
	}
	debug("outlen=%zu",outlen);
	if(outval){
		if(outlen>*outval)
			outlen=*outval;
		memcpy(out,nptr,outlen);
		if(outlen<*outval){
			out[outlen]=0;
			*outval=outlen;
		}
	}else {
		memcpy(out,nptr,outlen);
	}
	return nptr+outlen;
}
static const char *warped_strtof(const char *nptr,const char *endp,double *restrict outval,const struct expr_memtool *restrict mtl){
	size_t len;
	double r;
	r=expr_internal_strtod_mtl_force(nptr,(size_t)(endp-nptr),&len,mtl);
	if(unlikely(!len))
		return nptr;
	*outval=r;
	return nptr+len;
}
size_t expr_sscanf_r(const char *str,size_t len,const char *fmt,size_t fmtlen,void *const *addr,size_t addrlen,const struct expr_memtool *restrict mtl){
	const char *p,*str0=str,*fend=fmt+fmtlen,*end=str+len;
	void *const *aend=addr+addrlen;
	void *a;
	size_t r,n=0;
	union {
		ssize_t _zi;
		double _dbl;
		char _ch;
	} un;
#define zi un._zi
#define dbl un._dbl
#define ch un._ch
next:
	p=memchr(fmt,'%',fmtlen);
	debug("memchr=%p",(void *)(uintptr_t)p);
	if(!p){
		return n;
	}
	r=(size_t)(p-fmt);
	debug("r=%zu,len=%zu,*fmt=%c,*str=%c",r,len,*fmt,*str);
	if(r){
		if(unlikely(len<r||memcmp(fmt,str,r)||p+1==fend)){
			return n;
		}
		debug("memcmp ok");
		str+=r;
		len-=r;
		fmt=p;
		fmtlen-=r;
	}
	++p;
#define do_scan(scanner,write) \
			if(unlikely(addr>=aend))\
				return n;\
			fmt=p+1;\
			fmtlen=(size_t)(fend-fmt);\
			scanner;\
			if(unlikely(p==str))\
				return n;\
			str=p;\
			len=(size_t)(end-str);\
			a=*addr;\
			++addr;\
			if(a){\
				write;\
			}\
			++n;\
			goto next
	switch(*p){
		case '%':
			if(unlikely(*str!='%'))
				return n;
			fmt=p+1;
			fmtlen=(size_t)(fend-fmt);
			++str;
			--len;
			goto next;
		case 'i':
			fmt=p+1;
			fmtlen=(size_t)(fend-fmt);
			goto next;
		case 'n':
			if(unlikely(addr>=aend))
				return n;
			fmt=p+1;
			fmtlen=(size_t)(fend-fmt);
			a=*addr;
			++addr;
			if(a)
				*(ssize_t *)a=str-str0;
			++n;
			goto next;
		case 's':
			if(unlikely(addr+1>=aend))
				return n;
			fmt=p+1;
			fmtlen=(size_t)(fend-fmt);
			p=(fmtlen?fmt:NULL);
			str=internal_strtos(str,end,p,*addr,addr[1]);
			addr+=2;
			++n;
			goto next;
		case 'C':
			do_scan(ch=*str;p=str+1,*(char *)a=ch);
		case 'c':
			do_scan(zi=(ssize_t)(unsigned char)*str;p=str+1,*(ssize_t *)a=zi);
		case 'f':
			do_scan(p=warped_strtof(str,end,&dbl,mtl),*(double *)a=dbl);
		case 'd':
			do_scan(p=internal_strtoz_oux(str,end,&zi,1),*(ssize_t *)a=zi);
		case 'u':
			do_scan(p=internal_strtoz_oux(str,end,&zi,0),*(ssize_t *)a=zi);
		case 'x':
			do_scan(p=internal_strtoz_x(str,end,&zi),*(ssize_t *)a=zi);
		case 'o':
			do_scan(p=internal_strtoz_o(str,end,&zi),*(ssize_t *)a=zi);
		case 'm':
			do_scan(p=internal_strtom_10(str,end,&zi),*(ssize_t *)a=zi);
		default:
			return n;
	}
}
#undef zi
#undef dbl
#undef ch

