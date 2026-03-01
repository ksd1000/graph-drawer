/*******************************************************************************
 *License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>*
 *This is free software: you are free to change and redistribute it.           *
 *******************************************************************************/
#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>

#define _EXPR_LIB 1
#include "expr.h"

#if defined(EXPR_ISOLATED)&&(EXPR_ISOLATED)
expr_globals;
#endif

#define EXTEND_FRAC(x) (3*(x)/8)

#define fbuf ((char *)fp->buf)
#define reterr(V) {r=(V);goto err;}
#define rcheckadd(V) r=(V);\
	if(unlikely(r<0))\
		return r;\
	ret+=r
ssize_t expr_buffered_write(struct expr_buffered_file *restrict fp,const void *buf,size_t size){
	size_t i,c;
	ssize_t r,ret=0;
	if(unlikely(size>SSIZE_MAX))
		reterr(PTRDIFF_MIN);
	if(unlikely(!size)){
		return expr_buffered_flush(fp);
	}
	c=fp->length-fp->index;
	if(unlikely(!c&&fp->length)){
		rcheckadd(fp->un.writer(fp->fd,fp->buf,fp->length));
		fp->index=0;
		c=fp->length;
	}
	if(size<=c){
size_le_c:
		memcpy(fbuf+fp->index,buf,size);
		if(size==c){
			r=fp->un.writer(fp->fd,fp->buf,fp->length);
			if(unlikely(r<0))
				goto err;
			fp->index=0;
			return ret+r;
		}else {
			fp->index+=size;
			return ret;
		}
	}
	if(fp->length<fp->dynamic){
		void *p;
		i=align(fp->index+size+bufsize_initial+EXTEND_FRAC(fp->length));
		if(i>fp->dynamic)
			i=fp->dynamic;
		p=xrealloc(fp->buf,i);
		if(unlikely(!p)){
			reterr(PTRDIFF_MIN);
		}
		debug("buffer_size %zu -> %zu",fp->length,i);
		fp->buf=p;
		fp->length=i;
		c=i-fp->index;
		if(size<=c)
			goto size_le_c;
	}
	memcpy(fbuf+fp->index,buf,c);
	r=fp->un.writer(fp->fd,fp->buf,fp->length);
	if(unlikely(r<0)){
		fp->index=fp->length;
		goto err;
	}
	ret+=r;
	size-=c;
	buf+=c;
	if(size>=fp->length){
		fp->index=0;
		rcheckadd(fp->un.writer(fp->fd,buf,size));
		debug("%zd bytes written",ret);
		return ret;
	}
	fp->index=size;
	memcpy(fp->buf,buf,size);
	debug("%zd bytes written",ret);
	return ret;
err:
	fp->written=ret;
	debug("%zu bytes written,error code:%zd",ret,r);
	return r;
}
ssize_t expr_buffered_read(struct expr_buffered_file *restrict fp,void *buf,size_t size){
	size_t i;
	ssize_t r;
	if(unlikely(size>SSIZE_MAX))
		reterr(PTRDIFF_MIN);
	while(fp->length<fp->dynamic){
		void *p;
		i=align(fp->length+bufsize_initial+EXTEND_FRAC(fp->length));
		if(i>fp->dynamic)
			i=fp->dynamic;
		p=xrealloc(fp->buf,i);
		if(unlikely(!p))
			reterr(PTRDIFF_MIN);
		fp->buf=p;
		fp->length=i;
try_read_again:
		r=fp->un.reader(fp->fd,fbuf+fp->index,fp->length-fp->index);
		if(unlikely(r<0))
			goto err;
		if(!r){
			if(!size)
				return 0;
			break;
		}
		fp->index+=r;
		if(fp->index<fp->length)
			goto try_read_again;
	}
	if(unlikely(!size))
		return 0;
	i=fp->index-fp->written;
	if(i){
		if(i>size){
			memcpy(buf,fbuf+fp->written,size);
			fp->written+=size;
			return size;
		}
		fp->written=0;
		fp->index=0;
		memcpy(buf,fbuf+fp->written,i);
		if(i==size)
			return size;
		buf+=i;
		size-=i;
	}
	if(unlikely(fp->length<=size)){
		return fp->un.reader(fp->fd,buf,size);
	}
	r=fp->un.reader(fp->fd,fp->buf,fp->length);
	if(unlikely(r<0))
		goto err;
	if(!r){
		return 0;
	}
	if(r<=size){
		memcpy(buf,fp->buf,r);
		return r;
	}
	memcpy(buf,fp->buf,size);
	fp->index=r;
	fp->written=size;
	return size;
err:
	debug("error code:%zd",r);
	return r;
}
#undef fbuf
#undef reterr

#ifndef __unix__
#define memrchr expr_fake_memrchr
#endif
#define memrmem expr_fake_memrmem
ssize_t expr_buffered_write_flushatc(struct expr_buffered_file *restrict fp,const void *buf,size_t size,int c){
	uintptr_t rc=(uintptr_t)memrchr(buf,size,c);
	ssize_t r,ret;
	if(!rc)
		return expr_buffered_write(fp,buf,size);
	++rc;
	ret=0;
	rcheckadd(expr_buffered_write(fp,buf,rc-(uintptr_t)buf));
	rcheckadd(expr_buffered_flush(fp));
	rcheckadd(expr_buffered_write(fp,(const void *)rc,(uintptr_t)buf+size-rc));
	return ret;
}
ssize_t expr_buffered_write_flushat(struct expr_buffered_file *restrict fp,const void *buf,size_t size,void *c,size_t c_size){
	uintptr_t rc=(uintptr_t)memrmem(buf,size,c,c_size);
	ssize_t r,ret;
	if(!rc)
		return expr_buffered_write(fp,buf,size);
	rc+=c_size;
	ret=0;
	rcheckadd(expr_buffered_write(fp,buf,rc-(uintptr_t)buf));
	rcheckadd(expr_buffered_flush(fp));
	rcheckadd(expr_buffered_write(fp,(const void *)rc,(uintptr_t)buf+size-rc));
	return ret;
}
#undef rcheckadd
ssize_t expr_buffered_flush(struct expr_buffered_file *restrict fp){
	ssize_t r;
	if(fp->index){
		r=fp->un.writer(fp->fd,fp->buf,fp->index);
		fp->index=0;
	}else
		r=0;
	debug("%zd bytes written",r);
	return r;
}
ssize_t expr_buffered_drop(struct expr_buffered_file *restrict fp){
	ssize_t r;
	r=fp->index;
	debug("%zd - %zd bytes dropped",r,fp->written);
	fp->index=0;
	return r;
}
ssize_t expr_buffered_rdrop(struct expr_buffered_file *restrict fp){
	ssize_t r;
	r=fp->index;
	debug("%zd - %zd bytes dropped",r,fp->written);
	fp->index=0;
	fp->written=0;
	return r;
}
ssize_t expr_buffered_rdropall(struct expr_buffered_file *restrict fp){
	ssize_t r,ret=fp->index-fp->written;
	char *trash;
	size_t trashlen;
	if(fp->length){
		trash=fp->buf;
		trashlen=fp->length;
	}else {
		trash=alloca(1024);
		trashlen=1024;
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
ssize_t expr_file_readfd(expr_reader reader,intptr_t fd,size_t tail,void *savep){
	struct expr_buffered_file vf[1];
	ssize_t r;
	ssize_t ret;
	vf->fd=fd;
	vf->un.reader=reader;
	vf->buf=NULL;
	vf->index=0;
	vf->length=0;
	vf->dynamic=SIZE_MAX;
	vf->written=0;
	r=expr_buffered_read(vf,NULL,0);
	if(unlikely(r<0)){
		expr_buffered_rclose(vf);
		return -r;
	}
	vf->un.reader=zero_reader;
	vf->dynamic=vf->index+tail;
	r=expr_buffered_read(vf,NULL,0);
	ret=(ssize_t)vf->index;
	if(unlikely(r<0)){
		expr_buffered_rclose(vf);
		return -r;
	}
	debug("savep=%p,r=%zu",vf->buf,vf->index);
	*(void **)savep=vf->buf;
	return ret;
}
