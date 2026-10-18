
/*******************************************************************************
 *License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>*
 *This is free software: you are free to change and redistribute it.           *
 *******************************************************************************/
#define _GNU_SOURCE

#if (defined(EXPR_LIBC_STRING_IMPL)&&(EXPR_LIBC_STRING_IMPL))
#include <string.h>
#endif
#define EXPR_INLIB 1
#include "expr.h"

// WARNING: this file is not completed, and may not be completed for quite some time, because of the huge difficulty of it to me.

#if defined(__aarch64__)
ssize_t asmwrite_copy(uintptr_t buf,uintptr_t endptr,void *dst,const void *src){
	return -1;
}
#elif defined(__x86_64__)
ssize_t asmwrite_copy(uintptr_t buf,uintptr_t endptr,void *dst,const void *src){
	return -1;
}
#else
#define ASMWRITE_INVAILDARCH 1
#pragma warning "current arch is not supported by expr_asmwrite"
ssize_t expr_asmwrite(const struct expr *restrict ep,void *outbuf,size_t size){
	(void)ep;
	(void)outbuf;
	(void)size;
	return SSIZE_MIN:
}
#endif
#if !defined(ASMWRITE_INVAILDARCH)||!(ASMWRITE_INVAILDARCH)
ssize_t expr_asmwrite(const struct expr *restrict ep,void *outbuf,size_t size){
	uintptr_t buf=(uintptr_t)outbuf;
	uintptr_t endptr=buf+size;
	ssize_t r;
	for(struct expr_inst *ip=ep->data;;){
		switch(ip->op){
			case EXPR_COPY:
				r=asmwrite_copy(buf,endptr,ip->dst.dst,ip->un.src);
				break;
				/*
			case EXPR_INPUT:
				r=asmwrite_input(buf,endptr,ip->dst.dst);
				break;
			case EXPR_BL:
				r=asmwrite_bl(buf,endptr,ip->dst.dst,ip->un.uaddr);
				break;
			case EXPR_CONST:
				r=asmwrite_const(buf,endptr,ip->dst.dst,ip->un.value);
				break;
			case EXPR_ADD:
				r=asmwrite_add(buf,endptr,ip->dst.dst,ip->un.src);
				break;
			case EXPR_SUB:
				r=asmwrite_sub(buf,endptr,ip->dst.dst,ip->un.src);
				break;
			case EXPR_MUL:
				r=asmwrite_mul(buf,endptr,ip->dst.dst,ip->un.src);
				break;
			case EXPR_DIV:
				r=asmwrite_div(buf,endptr,ip->dst.dst,ip->un.src);
				break;
				*/
			default:
				return SSIZE_MIN+1+(ssize_t)ip->op;
				//instrument is not supported.
		}
		if(unlikely(r<0))
			return r;
		buf+=(size_t)r;
		++ip;
	}
}
#endif
