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

// the file provides some function for struct expr_symset and struct expr_builtin_symbol not required by expr-core.c

#define STACK_SIZEOFSSET(esp) ({size_t depth=(esp)->depth;depth<2?0:(depth-1)*EXPR_SYMSET_DEPTHUNIT;})
#define STACK_DEFAULT(_stack,esp) \
	void *_stack;\
	size_t _stack##_size;\
	_stack##_size=STACK_SIZEOFSSET(esp);\
	_stack=alloca(_stack##_size)

#define free (use xfree() instead!)
#define malloc (use xmalloc() instead!)
#define realloc (use xrealloc() instead!)

#define expr_allocator(size) (xmtl->allocate((size),xmtl->arg))
#define expr_reallocator(old,size) (xmtl->reallocate((old),(size),xmtl->arg))
#define expr_deallocator(old) (xmtl->deallocate((old),xmtl->arg))
#define xmtl esp->mtl
const struct expr_builtin_symbol *expr_builtin_symbol_search(const struct expr_builtin_symbol *syms,const char *sym,size_t sz){
	for(;syms->str;++syms){
		if(unlikely(sz==syms->strlen&&!memcmp(syms->str,sym,sz))){
			return syms;
		}
	}
	return NULL;
}
const struct expr_builtin_symbol *expr_builtin_symbol_rsearch(const struct expr_builtin_symbol *syms,void *addr){
	for(;syms->str;++syms){
		if(unlikely(syms->un.uaddr==addr)){
			return syms;
		}
	}
	return NULL;
}
struct expr_symset *expr_builtin_symbol_converts_r(const struct expr_memtool *restrict mtl,const struct expr_builtin_symbol *syms,...){
	struct expr_symset *esp=expr_symset_new_r(mtl);
	va_list ap;
	if(unlikely(!esp))
		return NULL;
	va_start(ap,syms);
	do {
		for(;syms->str;++syms){
			//printf("add %s\n",syms->str);
			if(unlikely(!expr_builtin_symbol_add(esp,syms)))
				goto err;
		}
		syms=va_arg(ap,const struct expr_builtin_symbol *);
	}while(syms);
	va_end(ap);
	return esp;
err:
	expr_symset_free(esp);
	va_end(ap);
	return NULL;
}
struct expr_symset *expr_builtin_symbol_convert_r(const struct expr_builtin_symbol *syms,const struct expr_memtool *restrict mtl){
	return expr_builtin_symbol_converts_r(mtl,syms,NULL);
}

int expr_symset_recombine(struct expr_symset *restrict esp,long seed){
	//the function tries to recombine the symset randomly,which sometimes
	//can reduce the depth of it. but it is not ensured,even probably make
	//a greater depth,for I have not found an algorithm to find the best
	//arrange to get the minimal depth.
	STACK_DEFAULT(stack,esp);
	return expr_symset_recombine_s(esp,seed,stack);
}
int expr_symset_recombine_s(struct expr_symset *restrict esp,int64_t seed,void *stack){
	struct expr_symbol **ss,**ss_cur,**ss_end,**p1,**p0;
	struct expr_symbol *swapbuf;
	if(unlikely(!esp->size))
		return 0;
	ss=xmalloc(esp->size*sizeof(struct expr_symbol *));
	if(unlikely(!ss))
		return -1;
	ss_cur=ss;
	expr_symset_foreach4(sp,esp,stack,EXPR_SYMNEXT){
		*ss_cur=sp;
		++ss_cur;
	}
	for(size_t i=0,endi=esp->size,i1;i<endi;++i){
		i1=(size_t)(expr_ltol48(expr_next48(&seed))%(int64_t)endi);
		if(likely(i1!=i)){
			p0=ss+i;
			p1=ss+i1;
		}else if(likely(i<endi-1)){
			p0=ss+i;
			p1=ss+i+1;
		}else
			continue;
		swapbuf=*p0;
		*p0=*p1;
		*p1=swapbuf;
	}
	//expr_memfry48(ss,sizeof(struct expr_symbol *),esp->size,expr_seed_default);
	esp->syms=NULL;
	esp->depth=0;
	esp->depth_n=0;
	ss_end=ss_cur;
	ss_cur=ss;
	do {
		expr_symset_reinsert(esp,*ss_cur);
		++ss_cur;
	}while(ss_cur<ss_end);
	xfree(ss);
	return 0;
}
int expr_symset_tryrecombine(struct expr_symset *restrict esp,long seed){
	struct expr_symset *temp;
	struct expr_symbol *b;
	size_t d;
	temp=expr_symset_clone(esp);
	if(unlikely(!temp))
		return -1;
	if(unlikely(expr_symset_recombine(temp,seed)<0)){
		expr_symset_free(temp);
		return -1;
	}
	if(temp->depth>esp->depth){
		expr_symset_free(temp);
		return 1;
	}
	d=esp->depth;
	esp->depth=temp->depth;
	esp->depth_n=temp->depth_n;
	esp->depth_nm+=temp->depth_nm;
	b=esp->syms;
	esp->syms=temp->syms;
	temp->syms=b;
	temp->depth=d;
	expr_symset_free(temp);
	return 0;

}
struct expr_symbol *expr_symset_rsearch(const struct expr_symset *restrict esp,void *addr){
	STACK_DEFAULT(stack,esp);
	return expr_symset_rsearch_s(esp,addr,stack);
}
struct expr_symbol *expr_symset_rsearch_s(const struct expr_symset *restrict esp,void *addr,void *stack){
	expr_symset_foreach(sp,esp,stack){
		if(unlikely(expr_symbol_un(sp)->uaddr==addr))
			return sp;
	}
	return NULL;
}
size_t expr_symset_depth(const struct expr_symset *restrict esp){
	STACK_DEFAULT(stack,esp);
	return expr_symset_depth_s(esp,stack);
}
size_t expr_symset_depth_s(const struct expr_symset *restrict esp,void *stack){
	uintptr_t depth=0;
	expr_symset_foreach(sp,esp,stack)
		if(unlikely((uintptr_t)__inloop_sp>depth))
			depth=(uintptr_t)__inloop_sp;
	return depth?(depth-(uintptr_t)stack)/EXPR_SYMSET_DEPTHUNIT+1:0;
}
void expr_symset_correct(struct expr_symset *restrict esp){
	STACK_DEFAULT(stack,esp);
	expr_symset_correct_s(esp,stack);
}
void expr_symset_correct_s(struct expr_symset *restrict esp,void *stack){
	size_t depth=0,cdepth,n=0;
	expr_symset_foreach(sp,esp,stack){
		cdepth=((uintptr_t)__inloop_sp-(uintptr_t)stack)/EXPR_SYMSET_DEPTHUNIT+1;
		if(likely(cdepth<depth)){
			continue;
		}else if(cdepth==depth){
			++n;
		}else {
			depth=cdepth;
			n=1;
		}
	}
	esp->depth=depth;
	esp->depth_n=n;
	esp->removed=0;
}
size_t expr_symset_length(const struct expr_symset *restrict esp){
	STACK_DEFAULT(stack,esp);
	return expr_symset_length_s(esp,stack);
}
size_t expr_symset_length_s(const struct expr_symset *restrict esp,void *stack){
	size_t length=0;
	expr_symset_foreach(sp,esp,stack)
		length+=sp->length;
	return length;
}
size_t expr_symset_size(const struct expr_symset *restrict esp){
	STACK_DEFAULT(stack,esp);
	return expr_symset_size_s(esp,stack);
}
size_t expr_symset_size_s(const struct expr_symset *restrict esp,void *stack){
	size_t sz=0;
	expr_symset_foreach(sp,esp,stack)
		++sz;
	return sz;
}
void expr_symset_save(struct expr_symset *restrict esp,void *buf){
	STACK_DEFAULT(stack,esp);
	expr_symset_save_s(esp,buf,stack);
}
void expr_symset_save_s(struct expr_symset *restrict esp,void *buf,void *stack){
	size_t off;
	struct expr_symbol *prev;
	size_t index;
	expr_symset_foreach4(sp,(volatile struct expr_symset *)esp,stack,EXPR_SYMNEXT){
		*sp->tail=(struct expr_symbol *)buf;
		memcpy(buf,sp,sp->length);
		off=align(sp->length);
		if(!sp->saved){
			xfree(sp);
			((struct expr_symbol *)buf)->saved=1;
		}
		buf=(void *)((uintptr_t)buf+off);
	}
	expr_symset_foreach4(sp,(volatile struct expr_symset *)esp,stack,EXPR_SYMNEXT){
		if(likely(__inloop_sp!=__inloop_stack)){
			prev=*(__inloop_sp-1);
			index=*((size_t *)__inloop_sp-2)-1;
			sp->tail=prev->next+index;
		}
	}
}
void expr_symset_move(struct expr_symset *restrict esp,ptrdiff_t off){
	STACK_DEFAULT(stack,esp);
	expr_symset_move_s(esp,off,stack);
}
void expr_symset_move_s(struct expr_symset *restrict esp,ptrdiff_t off,void *stack){
	expr_symset_foreach4(sp,esp,stack,EXPR_SYMNEXT){
		if(!sp->saved)
			continue;
		*sp->tail=(struct expr_symbol *)((intptr_t)*sp->tail+off);
	}
}
#define off_sin offsetof(struct expr_symbol_infile,str)
#define off_str offsetof(struct expr_symbol,str)
ssize_t expr_symset_write(const struct expr_symset *restrict esp,expr_writer writer,intptr_t fd){
	STACK_DEFAULT(stack,esp);
	return expr_symset_write_s(esp,writer,fd,stack);
}
#define try_write(buf,size) \
	r=writer(fd,(buf),(size_t)(size));\
	if(unlikely(r<0))\
		goto err2;\
	else if((size_t)r<(size_t)(size))\
		goto err3;\
	((void)0)
ssize_t expr_symset_write_s(const struct expr_symset *restrict esp,expr_writer writer,intptr_t fd,void *stack){
	size_t maxlen=0,count;
	ssize_t r;
	size_t off;
	struct expr_symset_infile tempesp[1];
	struct expr_symbol_infile *temp;
	expr_symset_foreach4(sp,esp,stack,0){
		off=sp->length;
		if(unlikely(off>maxlen))
			maxlen=off;
	}
	temp=xmalloc(align(maxlen)-EXPR_SYMBOL_EXTRA);
	if(unlikely(!temp))
		return SSIZE_MIN+1;
	tempesp->size=le64(esp->size);
	tempesp->maxlen=le32(maxlen);
	try_write(tempesp,sizeof(struct expr_symset_infile));
	count=esp->size;
	if(!count)
		goto end;
	++count;
	expr_symset_foreach4(sp,esp,stack,EXPR_SYMNEXT/2){
		if(unlikely(sp->strlen-1>=63))
			return SSIZE_MIN+3;
		off=sp->length;
		maxlen=off-EXPR_SYMBOL_EXTRA;
		if(unlikely(maxlen>UINT32_MAX))
			return SSIZE_MIN+3;
		temp->length=le32(maxlen);
		temp->strlen=(uint16_t)(sp->strlen-1);
		temp->type=sp->type;
		temp->flag=sp->flag;
		temp->unused=0;
		memcpy(temp->str,sp->str,off-off_str);
		try_write(temp,maxlen);
	}
end:
	xfree(temp);
	return 0;
err2:
	xfree(temp);
	return r;
err3:
	xfree(temp);
	return SSIZE_MIN+2;
}
ssize_t expr_symset_read(struct expr_symset *restrict esp,const void *buf,size_t size){
	const struct expr_symset_infile *esi;
	const struct expr_symbol_infile *ebi;
	struct expr_symbol *sp;
	size_t count,len;
	ssize_t added;
	if(size<sizeof(struct expr_symset_infile))
		return SSIZE_MIN+2;
	esi=(const struct expr_symset_infile *)buf;
	count=le64(esi->size);
	ebi=(const struct expr_symbol_infile *)((uintptr_t)buf+sizeof(struct expr_symset_infile));
	size-=sizeof(struct expr_symset_infile);
	added=0;
	while(count){
		if(unlikely(size<=off_sin))
			break;
		if(unlikely(ebi->length<off_sin))
			break;
		if(unlikely(ebi->length>=size))
			return SSIZE_MIN+2;
		len=ebi->length+EXPR_SYMBOL_EXTRA;
		sp=xmalloc(len);
		if(unlikely(!sp))
			return SSIZE_MIN+1;
		sp->length=len;
		sp->strlen=(size_t)ebi->strlen+1;
		sp->type=ebi->type;
		sp->flag=ebi->flag;
		sp->saved=0;
		memcpy(sp->str,ebi->str,ebi->length-off_sin);
		if(unlikely(sp->str[sp->strlen]))
			return SSIZE_MIN+5;
		if(unlikely(expr_symset_insert(esp,sp)<0)){
			xfree(sp);
		}else {
			++added;
		}
		size-=ebi->length;
		ebi=(const struct expr_symbol_infile *)((uintptr_t)ebi+ebi->length);
		--count;
	}
	return unlikely((size_t)added<esi->size)?SSIZE_MIN+3:added;
}
ssize_t expr_symset_readfd(struct expr_symset *restrict esp,expr_reader reader,intptr_t fd){
	struct expr_symset_infile esi[1];
	struct expr_symbol_infile *ebi;
	struct expr_symbol *sp;
	size_t count,len,maxlen;
	ssize_t added,r;
	r=reader(fd,esi,sizeof(struct expr_symset_infile));
	if(unlikely(r<0))
		return r;
	if(unlikely((size_t)r<sizeof(struct expr_symset_infile)))
		return SSIZE_MIN+2;
	count=le64(esi->size);
	maxlen=le32(esi->maxlen);
	if(unlikely(maxlen<=off_sin))
		return SSIZE_MIN+2;
	ebi=xmalloc(maxlen);
	if(unlikely(!ebi))
		return SSIZE_MIN+1;
	added=0;
	while(count){
		r=reader(fd,ebi,off_sin);
		if(unlikely(r<0))
			goto errr;
		if(unlikely((size_t)r<off_sin))
			goto err2;
		len=le32(ebi->length+1);
		if(unlikely(len<=off_sin))
			goto err2;
		if(unlikely(len>maxlen))
			goto err2;
		len+=EXPR_SYMBOL_EXTRA;
		if(unlikely(len<=sizeof(struct expr_symbol)))
			goto err2;
		r=reader(fd,(void *)((uintptr_t)ebi+off_sin),ebi->length+1-off_sin);
		if(unlikely(r<0))
			goto errr;
		if(unlikely((size_t)r<=ebi->length-off_sin))
			goto err2;
		sp=xmalloc(len);
		if(unlikely(!sp))
			goto err;
		sp->length=len;
		sp->strlen=(size_t)ebi->strlen+1;
		sp->type=ebi->type;
		sp->flag=ebi->flag;
		sp->saved=0;
		memcpy(sp->str,ebi->str,ebi->length+1-off_sin);
		if(unlikely(sp->str[sp->strlen]))
			return SSIZE_MIN+5;
		if(unlikely(expr_symset_insert(esp,sp)<0)){
			xfree(sp);
		}else {
			++added;
		}
		--count;
	}
	xfree(ebi);
	return unlikely(added<(ssize_t)esi->size)?SSIZE_MIN+4:added;
err:
	xfree(ebi);
	return SSIZE_MIN+1;
err2:
	xfree(ebi);
	return SSIZE_MIN+2;
errr:
	xfree(ebi);
	return r;
//WARNING: the symbol value of variables or functions
//cannot be reused after a PIE-program restarted.
}
