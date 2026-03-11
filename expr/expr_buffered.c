/*******************************************************************************
 *License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>*
 *This is free software: you are free to change and redistribute it.           *
 *******************************************************************************/
#define _GNU_SOURCE
#include <string.h>

#define _EXPR_LIB 1
#include "expr.h"

#if defined(EXPR_ISOLATED)&&(EXPR_ISOLATED)
expr_globals;
#endif

#define EXTEND_FRAC(x) (((x)/8)*3)

#define reterr(V) {r=(V);goto err;}
#define FLUSH(size,trunc,onerr) \
	r=fp->un.writer(fp->fd,fp->buf,size);\
	if(unlikely(r<0)){\
		onerr;\
		return r;\
	}\
	if(unlikely(!r))\
		fp->flag|=EXPR_BF_ZERO;\
	else\
		fp->written+=r;\
	if(unlikely((ssize_t)(r1=size-r)>0)){\
		fp->flag|=EXPR_BF_TRUNC;\
		memmove(fp->buf,fp->buf+r,r1);\
		fp->index=r1;\
		return trunc;\
	}else\
		fp->index=0
ssize_t expr_buffered_write(struct expr_buffered_file *restrict fp,const void *buf,size_t size){
	size_t i,c;
	ssize_t r,s0,r1;
	if(unlikely(!size)){
		r=expr_buffered_flush(fp);
		if(unlikely(r<0)){
			return PTRDIFF_MIN;
		}
		fp->written+=r;
		return 0;
	}
	if(unlikely(size>SSIZE_MAX)){
		expr_buffered_drop(fp);
		return 0;
	}
	c=fp->length-fp->index;
	if(unlikely(!c&&fp->length)){
		FLUSH(fp->length,0,);
		c=fp->length;
	}
	if(size<=c){
size_le_c:
		memcpy(fp->buf+fp->index,buf,size);
		if(size==c){
			FLUSH(fp->length,size,);
			return size;
		}else {
			fp->index+=size;
			return size;
		}
	}
	if(fp->length<fp->dynamic){
		void *p;
		i=align(fp->index+size+BUFSIZE_INITIAL+EXTEND_FRAC(fp->length));
		if(unlikely(i>fp->dynamic||i<=fp->length))
			i=fp->dynamic;
		p=xrealloc(fp->buf,i);
		if(unlikely(!p)){
			fp->flag|=EXPR_BF_EMEM;
			return PTRDIFF_MIN;
		}
		debug("buffer_size %zu -> %zu",fp->length,i);
		fp->buf=p;
		fp->length=i;
		c=i-fp->index;
		if(size<=c)
			goto size_le_c;
	}
	s0=size;
	if(fp->index){
		memcpy(fp->buf+fp->index,buf,c);
		size-=c;
		FLUSH(fp->length,s0-size,fp->index=fp->length);
		buf+=c;
	}
	if(size>=fp->length){
		fp->index=0;
		r=fp->un.writer(fp->fd,buf,size);
		if(unlikely(r<0)){
			fp->flag|=EXPR_BF_EMPTY;
			return PTRDIFF_MIN;
		}
		if(unlikely(!r))
			fp->flag|=EXPR_BF_ZERO;
		else {
			size-=r;
			fp->written+=r;
		}
		return s0-size;
	}
	fp->index=size;
	memcpy(fp->buf,buf,size);
	debug("%zd bytes written",s0);
	return s0;
}
ssize_t expr_buffered_read(struct expr_buffered_file *restrict fp,void *buf,size_t size){
	return expr_buffered_read5(fp,buf,size,NULL,0);
}
ssize_t expr_buffered_read5(struct expr_buffered_file *restrict fp,void *buf,size_t size,expr_buffered_test test,intptr_t arg){
	size_t i;
	ssize_t r;
	if(unlikely(size>SSIZE_MAX)){
		if(size==SIZE_MAX){
			i=fp->index-fp->written;
			expr_buffered_rdrop(fp);
			return i;
		}else {
			return expr_buffered_rdropall(fp);
		}
	}
	if(unlikely(fp->flag&EXPR_BF_ZERO)){
		debug("end index=%zu",fp->index);
		return 0;
	}
	debug("start index=%zu",fp->index);
	while(fp->length<fp->dynamic){
		void *p;
		i=align(fp->length+BUFSIZE_INITIAL+EXTEND_FRAC(fp->length));
		if(unlikely(i>fp->dynamic||i<=fp->length))
			i=fp->dynamic;
		p=xrealloc(fp->buf,i);
		if(unlikely(!p)){
			fp->flag|=EXPR_BF_EMEM;
			reterr(PTRDIFF_MIN);
		}
		fp->buf=p;
		fp->length=i;
try_read_again:
		r=fp->length-fp->index;
		debug("index=%zu length=%zu",fp->index,fp->length);
		if(likely(r)){
			p=fp->buf+fp->index;
			r=fp->un.reader(fp->fd,p,r);
			if(unlikely(r<0))
				goto err;
			if(test&&test(p,arg,r)){
				fp->index+=r;
				debug("end index=%zu",fp->index);
				return 0;
			}
		}
		if(!r){
			fp->flag|=EXPR_BF_ZERO;
			if(!size){
				debug("end index=%zu",fp->index);
				return 0;
			}
			goto size_ok;
		}else {
			fp->index+=r;
			if(fp->index<fp->length)
				goto try_read_again;
		}
	}
	if(!size){
		i=fp->length-fp->index;
		if(i){
			r=fp->un.reader(fp->fd,fp->buf+fp->index,i);
			if(unlikely(r<0))
				goto err;
			fp->index+=r;
		}
		debug("end index=%zu",fp->index);
		return 0;
	}
size_ok:
	i=fp->index-fp->written;
	if(i){
		if(i>size){
			memcpy(buf,fp->buf+fp->written,size);
			fp->written+=size;
			debug("end index=%zu",fp->index);
			return size;
		}
		memcpy(buf,fp->buf+fp->written,i);
		fp->written=0;
		fp->index=0;
		if(i==size){
			debug("end index=%zu",fp->index);
			return size;
		}
		buf+=i;
		size-=i;
	}
	if(unlikely(fp->flag&EXPR_BF_ZERO)){
		debug("end index=%zu",fp->index);
		return 0;
	}
	if(unlikely(fp->length<=size)){
		debug("end index=%zu",fp->index);
		return fp->un.reader(fp->fd,buf,size);
	}
	r=fp->un.reader(fp->fd,fp->buf,fp->length);
	if(unlikely(r<0))
		goto err;
	if(!r){
		debug("end index=%zu",fp->index);
		return 0;
	}
	if(r<=size){
		memcpy(buf,fp->buf,r);
		debug("end index=%zu",fp->index);
		return r;
	}
	memcpy(buf,fp->buf,size);
	fp->index=r;
	fp->written=size;
	debug("end index=%zu",fp->index);
	return size;
err:
	debug("error code:%zd",r);
	return r;
}
#undef reterr

#ifndef __unix__
#define memrchr expr_fake_memrchr
#define memmem expr_fake_memmem
#endif
#define memrmem expr_fake_memrmem

#define rcheckadd(V) r=(V);\
	if(unlikely(r<0))\
		return r;\
	ret+=r
#define flushat_common(rcfetch,rcinc) \
	ssize_t r,ret;\
	ssize_t n;\
	uintptr_t rc=(uintptr_t)(rcfetch);\
	if(!rc)\
		return expr_buffered_write(fp,buf,size);\
	rcinc;\
	n=rc-(uintptr_t)buf;\
	ret=0;\
	rcheckadd(expr_buffered_write(fp,buf,n));\
	r=expr_buffered_flush(fp);\
	if(unlikely(r<0))\
		return r;\
	n=size-n;\
	if(n){\
		rcheckadd(expr_buffered_write(fp,(const void *)rc,n));\
	}\
	return ret
ssize_t expr_buffered_write_flushatc(struct expr_buffered_file *restrict fp,const void *buf,size_t size,int c){
	flushat_common(memrchr(buf,size,c),++rc);
}
ssize_t expr_buffered_write_flushatt(struct expr_buffered_file *restrict fp,const void *buf,size_t size,expr_buffered_test test,intptr_t arg){
	flushat_common(test(buf,arg,size),rc+=(uintptr_t)buf);
}
ssize_t expr_buffered_write_flushat(struct expr_buffered_file *restrict fp,const void *buf,size_t size,const void *c,size_t c_size){
	flushat_common(memrmem(buf,size,c,c_size),rc+=c_size);
}
#define sflushat_common(rcfetch,rcinc) \
	ssize_t r,ret=0;\
	ssize_t n;\
	uintptr_t rc;\
	do {\
		rc=(uintptr_t)(rcfetch);\
		if(!rc){\
			rcheckadd(expr_buffered_write(fp,buf,size));\
			return ret;\
		}\
		rcinc;\
		n=rc-(uintptr_t)buf;\
		rcheckadd(expr_buffered_write(fp,buf,n));\
		debug("flash point found at %zd",n);\
		r=expr_buffered_flush(fp);\
		if(unlikely(r<0))\
			return r;\
		debug("flash ok %zd",r);\
		buf=(const void *)rc;\
		size-=n;\
	}while(size);\
	return ret
ssize_t expr_buffered_write_sflushatc(struct expr_buffered_file *restrict fp,const void *buf,size_t size,int c){
	sflushat_common(memchr(buf,size,c),++rc);
}
ssize_t expr_buffered_write_sflushatt(struct expr_buffered_file *restrict fp,const void *buf,size_t size,expr_buffered_test test,intptr_t arg){
	sflushat_common(test(buf,arg,size),rc+=(uintptr_t)buf);
}
ssize_t expr_buffered_write_sflushat(struct expr_buffered_file *restrict fp,const void *buf,size_t size,const void *c,size_t c_size){
	sflushat_common(memmem(buf,size,c,c_size),rc+=c_size);
}
ssize_t expr_buffered_write_sync(struct expr_buffered_file *restrict fp,const void *buf,size_t size){
	ssize_t r1,r=expr_buffered_write(fp,buf,size);
	if(unlikely(r<0))
		return r;
	r1=expr_buffered_flush(fp);
	if(unlikely(r1<0))
		return r1;
	return r;
}
#undef rcheckadd
ssize_t expr_buffered_flush(struct expr_buffered_file *restrict fp){
	ssize_t r,r1;
	if(fp->index){
		FLUSH(fp->index,r,);
	}else
		r=0;
	debug("%zd bytes written",r);
	return r;
}
#define TRASHLEN 1024
ssize_t expr_buffered_rdropall(struct expr_buffered_file *restrict fp){
	ssize_t r,ret=fp->index-fp->written;
	char *trash;
	size_t trashlen;
	if(fp->length>=TRASHLEN){
		trash=fp->buf;
		trashlen=fp->length;
	}else {
		trash=alloca(TRASHLEN);
		trashlen=TRASHLEN;
	}
	for(;;){
		r=fp->un.reader(fp->fd,trash,trashlen);
		if(unlikely(r<0))
			goto err;
		if(!r)
			break;
		ret+=r;
	}
	debug("%zd bytes dropped",ret);
	fp->index=0;
	fp->written=0;
	return ret;
err:
	debug("%zu bytes dropped,error code:%zd",ret,r);
	return r;
}
ssize_t expr_buffered_close(struct expr_buffered_file *restrict fp){
	ssize_t r;
	if(fp->index&&fp->un.writer)
		r=fp->un.writer(fp->fd,fp->buf,fp->index);
	else
		r=0;
	debug("%zd bytes written",r);
	if(fp->dynamic&&fp->buf)
		xfree(fp->buf);
	return r;
}
void expr_buffered_rclose(struct expr_buffered_file *restrict fp){
	debug("close");
	if(fp->dynamic&&fp->buf)
		xfree(fp->buf);
}
static ssize_t zero_reader(intptr_t fd,void *buf,size_t size){
	memset(buf,0,size);
	return size;
}
#define checkr(_fp) \
	if(unlikely(r<0)){\
		expr_buffered_rclose(_fp);\
		return r;\
	}
ssize_t expr_buffered_readline(struct expr_buffered_file *restrict fp,int c,void *savep){
	ssize_t r;
	size_t in;
	char *p,*end,*cp;
	p=fp->buf+fp->written;
	end=fp->buf+fp->index;
	r=end-p;
	if(r){
		cp=memchr(p,c,r);
		if(cp){
			*cp=0;
			fp->written=cp+1-(char *)fp->buf;
			*(void **)savep=p;
			debug("return %zd",cp-p);
			return cp-p;
		}
		memmove(fp->buf,p,r);
		fp->index-=fp->written;
		fp->written=0;
	}else {
		fp->index=0;
		fp->written=0;
	}
	in=fp->index;
	r=expr_buffered_read5(fp,NULL,0,(expr_buffered_test)memchr,c);
	if(unlikely(r<0)){
		debug("read fail %zd",r);
		return r;
	}
	if(fp->index==in){
		if(!in){
			*(void **)savep=NULL;
			debug("end");
			return 0;
		}
		if(unlikely(in==fp->length)){
#define leninc1 \
			if(unlikely(fp->dynamic==in||!(p=xrealloc(fp->buf,in+1)))){\
				return PTRDIFF_MIN;\
			}\
			fp->buf=p;\
			++fp->length
			leninc1;
		}
		*(char *)(fp->buf+in)=0;
		fp->written=in;
		*(void **)savep=fp->buf;
		debug("return %zd",in);
		return in;
	}
	cp=memchr(fp->buf,c,fp->index);
	if(cp){
		*cp=0;
		fp->written=cp+1-(char *)fp->buf;
		*(void **)savep=fp->buf;
		debug("return %zd",fp->written-1);
		return fp->written-1;
	}
	in=fp->index;
	if(unlikely(in==fp->length)){
		leninc1;
	}
	fp->written=in;
	*(char *)(fp->buf+in)=0;
	*(void **)savep=fp->buf;
	return in;
}
ssize_t expr_file_readfd(expr_reader reader,intptr_t fd,size_t tail,void *savep){
	struct expr_buffered_file vf[1];
	ssize_t r;
	ssize_t ret;
	expr_buffered_rinit(vf,fd,reader,NULL,SIZE_MAX);
	r=expr_buffered_read(vf,NULL,0);
	checkr(vf);
	vf->un.reader=zero_reader;
	vf->dynamic=vf->index+tail;
	r=expr_buffered_read(vf,NULL,0);
	checkr(vf);
	ret=(ssize_t)vf->index;
	debug("savep=%p,r=%zu",vf->buf,vf->index);
	*(void **)savep=vf->buf;
	return ret;
}

