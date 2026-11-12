
/*******************************************************************************
 *License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>*
 *This is free software: you are free to change and redistribute it.           *
 *******************************************************************************/
#define _GNU_SOURCE

#define EXPR_VERSION "1.0.18-43"

#define EXPR_LIBC_STRING_IMPL 1
#include <string.h>
// asan only replaces the strlen() of libc

#ifndef EXPR_SETUP_ENABLED
#define EXPR_SETUP_ENABLED 1
#endif
#include <stdio.h>
#include <errno.h>

#include <stdlib.h>
#define EXPR_INLIB 1
#include "expr.h"

#if (EXPR_INTERNAL_SYSCALL_IMPLEMENTED)&&(__linux__)
#include <stdatomic.h>
#include <sys/syscall.h>
#include <linux/futex.h>

#ifndef _MUTEX_H_
#define _MUTEX_H_
typedef _Atomic(uint32_t) mutex_t;
#define mutex_lock(lock) ({\
	mutex_t *_lock=(lock);\
	uint32_t _r;\
	while(expr_unlikely(_r=atomic_fetch_add(_lock,1))){\
		mutex_wait(_lock,_r+1);\
	}\
})

#define mutex_spinlock(lock) ({\
	mutex_t *__lock=(lock);\
	while(mutex_trylock(__lock));\
})

#define mutex_trylock(lock) ({\
	uint32_t __e=0;\
	!atomic_compare_exchange_strong((lock),&__e,1);\
})

#define mutex_unlock(lock) ({\
	mutex_t *_lock=(lock);\
	if(expr_unlikely(atomic_exchange(_lock,0)>=2)){\
		mutex_wake(_lock,INT32_MAX);\
	}\
})

#define mutex_spinunlock(lock) ({\
	atomic_store((lock),0);\
})

#define mutex_atomicl(lock,_label) for(mutex_lock(lock);;({mutex_unlock(lock);goto expr_combine(__atomic_label_,_label);}))if(0){expr_combine(__atomic_label_,_label):break;}else
#define mutex_atomic(lock) mutex_atomicl(lock,__LINE__)
#define mutex_spinatomicl(lock,_label) for(mutex_spinlock(lock);;({mutex_spinunlock(lock);goto expr_combine(__atomic_label_,_label);}))if(0){expr_combine(__atomic_label_,_label):break;}else
#define mutex_spinatomic(lock) mutex_spinatomicl(lock,__LINE__)

#define mutex_wait(lock,val) expr_internal_syscall6(SYS_futex,(intptr_t)(lock),FUTEX_WAIT,(val),0,0,0)
#define mutex_wake(lock,val) expr_internal_syscall6(SYS_futex,(intptr_t)(lock),FUTEX_WAKE,(val),0,0,0)
#endif

#endif

const struct expr_libinfo expr_libinfo_current[1]={{
	.version=EXPR_VERSION,
	.compiler_version=
#ifdef __VERSION__
		__VERSION__,
#else
		"Unknown",
#endif
	.platform=EXPR_ARCHN
#ifdef __linux__
		" Linux",
#else
		" Unknown system",
#endif
	.date=__DATE__,
	.time=__TIME__,
	.license="GPL v3+",
}};
static int expr_setuped=0;
static struct expr_global_file *expr_stdin_var=NULL;
static struct expr_global_file *expr_stdout_var=NULL;
static struct expr_global_file *expr_stderr_var=NULL;
static uint32_t global_mutex[1]={EXPR_MUTEX_INITIALIZER};
// mutex lock/unlock will be void operation on non-unix platform, the man page keeps MT-UnSafe.
#define GLOBAL_MUTEX_START ({debug("GLOBAL_MUTEX_START");expr_mutex_lock(global_mutex);})
#define GLOBAL_MUTEX_END ({debug("GLOBAL_MUTEX_END");expr_mutex_unlock(global_mutex);})
static ssize_t global_write(intptr_t fd,const void *buf,size_t size){
	ssize_t r;
#if (EXPR_INTERNAL_SYSCALL_IMPLEMENTED)&&(__linux__)
	fd=fileno((FILE *)fd);
	r=(ssize_t)expr_internal_syscall(SYS_write,fd,buf,size);
	if(unlikely(r<0)){
		errno=-r;
	}
	return r;
#else
	FILE *fp=(FILE *)fd;
	r=(ssize_t)fwrite(buf,1,size,fp);
	fflush(fp);
	if(unlikely((size_t)r<size&&ferror(fp))){
		return (ssize_t)-errno;
	}
	return r;
#endif
}
static ssize_t global_read(intptr_t fd,void *buf,size_t size){
	ssize_t r;
#if (EXPR_INTERNAL_SYSCALL_IMPLEMENTED)&&(__linux__)
	fd=fileno((FILE *)fd);
	r=(ssize_t)expr_internal_syscall(SYS_read,fd,buf,size);
	if(unlikely(r<0)){
		errno=-r;
	}
	return r;
#else
	errno=0;
	r=(ssize_t)fread(buf,1,size,(FILE *)fd);
	if(unlikely(errno))
		return (ssize_t)-errno;
	return r;
#endif
}
static int global_close(intptr_t fd){
	errno=0;
	fclose((FILE *)fd);
	return -errno;
}
static struct expr_list global_head[1]={EXPR_LIST_INITIALIZER};
#ifndef EBADF
#define EBADF 9
#endif
#define xmtl expr_globalmtl
int expr_close(struct expr_global_file *fp){
	int r=0;
	if(unlikely(!fp))
		return -EBADF;
	expr_mutex_lock(fp->mutex);
	expr_list_foreach_ro(v,global_head,next){
		if(v==fp->node)
			goto found;
	}
	return -EBADF;
found:
	if(fp->write){
		expr_buffered_flush(fp->buf);
		if(!fp->external&&fp->buf->buf)
			xfree(fp->buf->buf);
	}
	if(fp->close){
		if(fp->read)
			r=fp->close(fp->rbuf->fd);
		else if(fp->write)
			r=fp->close(fp->buf->fd);
	}
	if(!expr_list_isempty(fp->node)){
		GLOBAL_MUTEX_START;
		expr_list_detach(fp->node);
		GLOBAL_MUTEX_END;
	}
	if(fp->iface)
		*fp->iface=NULL;
	expr_mutex_unlock(fp->mutex);
	xfree(fp);
	return r;
}
static void expr_global_atexit(void){
	//GLOBAL_MUTEX_START;
	expr_list_foreach_rw(v,global_head,next){
		expr_close(expr_host(v,struct expr_global_file,node));
	}
	//GLOBAL_MUTEX_END;
	// dead lock
}
__attribute__((constructor))
static void expr_global_setup(void){
	GLOBAL_MUTEX_START;
	if(unlikely(expr_setuped)){
		GLOBAL_MUTEX_END;
		return;
	}
	if(unlikely(!(expr_stdin_var=expr_bind(stdin,"rD"))||!(expr_stdout_var=expr_bind(stdout,"w"))||!(expr_stderr_var=expr_bind(stderr,"w"))))
		warn_abort("expr-lc/setup","setup failed: cannot allocate memory of size=%zu",(size_t)BUFSIZ);
	expr_stdin_var->iface=&expr_stdin_var;
	expr_stdout_var->iface=&expr_stdout_var;
	expr_stderr_var->iface=&expr_stderr_var;
	if(unlikely(atexit(expr_global_atexit)))
		warn_abort("expr-lc/setup","atexit() failed");
	expr_setuped=1;
	GLOBAL_MUTEX_END;
}
#define EXPR_GLOBAL_SETUP_CHECK() ({\
	if(unlikely(!expr_setuped))\
		expr_global_setup();\
})
struct expr_global_file *const *expr_stdin_get(void){
	EXPR_GLOBAL_SETUP_CHECK();
	return &expr_stdin_var;
}
struct expr_global_file *const *expr_stdout_get(void){
	EXPR_GLOBAL_SETUP_CHECK();
	return &expr_stdout_var;
}
struct expr_global_file *const *expr_stderr_get(void){
	EXPR_GLOBAL_SETUP_CHECK();
	return &expr_stderr_var;
}
#define VBUF_DYNAMIC ((size_t)EXPR_IMAXOU)

#define expr_stdout (*expr_stdout_get())
#define expr_stderr (*expr_stderr_get())
#define FP_START(dyn) \
	if(fp->external){\
		fp->buf->buf=NULL;\
		fp->buf->index=0;\
		fp->buf->length=(dyn);\
		fp->buf->dynamic=0;\
		fp->external=0;\
	}
static int expr_setrvbuf(struct expr_global_file *fp,void *buf,int mode,size_t size){
	if(buf){
		switch(mode){
			case _IONBF:
				fp->rbuf->length=0;
				break;
			case _IOLBF:
				fp->rbuf->length=size;
				break;
			case _IOFBF:
				fp->rbuf->length=size;
				break;
			default:
				goto err;
		}
		if(!fp->external&&fp->rbuf->buf)
			xfree(fp->rbuf->buf);
		fp->rbuf->buf=buf;
		fp->rbuf->dynamic=0;
		fp->rbuf->index=0;
		fp->rbuf->written=0;
		fp->external=1;
		goto end;
	}
	switch(mode){
		case _IONBF:
			FP_START(0);
			if(expr_buffered_truncate(fp->rbuf,0)<0){
				goto err;
			}
			break;
		case _IOLBF:
			if(size>=VBUF_DYNAMIC){
				FP_START(SIZE_MAX);
			}else {
				FP_START(0);
				if(expr_buffered_truncate(fp->rbuf,size)<0){
					goto err;
				}
			}
			break;
		case _IOFBF:
			if(size>=VBUF_DYNAMIC){
				FP_START(SIZE_MAX);
			}else {
				FP_START(0);
				if(expr_buffered_truncate(fp->rbuf,size)<0){
					goto err;
				}
			}
			break;
		default:
			goto err;
	}
end:
	return 0;
err:
	return -1;
}
static int expr_setwvbuf(struct expr_global_file *fp,void *buf,int mode,size_t size){
	expr_buffered_flush(fp->buf);
	if(buf){
		switch(mode){
			case _IONBF:
				fp->mode=expr_buffered_write_warped;
				fp->buf->length=0;
				break;
			case _IOLBF:
				fp->mode=expr_buffered_write_iolbf_warped;
				fp->buf->length=size;
				break;
			case _IOFBF:
				fp->mode=expr_buffered_write_warped;
				fp->buf->length=size;
				break;
			default:
				goto err;
		}
		if(!fp->external&&fp->buf->buf)
			xfree(fp->buf->buf);
		fp->buf->buf=buf;
		fp->buf->dynamic=0;
		fp->buf->index=0;
		fp->external=1;
		goto end;
	}
	switch(mode){
		case _IONBF:
			FP_START(0);
			if(expr_buffered_truncate(fp->buf,0)<0){
				goto err;
			}
			fp->mode=expr_buffered_write_warped;
			break;
		case _IOLBF:
			if(size>=VBUF_DYNAMIC){
				FP_START(SIZE_MAX);
			}else {
				FP_START(0);
				if(expr_buffered_truncate(fp->buf,size)<0){
					goto err;
				}
			}
			fp->mode=expr_buffered_write_iolbf_warped;
			break;
		case _IOFBF:
			if(size>=VBUF_DYNAMIC){
				FP_START(SIZE_MAX);
			}else {
				FP_START(0);
				if(expr_buffered_truncate(fp->buf,size)<0){
					goto err;
				}
			}
			fp->mode=expr_buffered_write_warped;
			break;
		default:
			goto err;
	}
end:
	return 0;
err:
	return -1;
}
int expr_setvbuf(struct expr_global_file *fp,void *buf,int mode,size_t size){
	int r0,r1;
	if(unlikely(!fp))
		return -1;
	expr_mutex_lock(fp->mutex);
	if(fp->read)
		r0=expr_setrvbuf(fp,buf,mode,size);
	else
		r0=0;
	if(fp->write)
		r1=expr_setwvbuf(fp,buf,mode,size);
	else
		r1=0;
	// same buf should not be used by a read-write file
	// when the other has not completed.
	expr_mutex_unlock(fp->mutex);
	return r0|r1;
}
ssize_t expr_vfprintf(struct expr_global_file *fp,const char *format,va_list ap){
	ssize_t r;
	if(unlikely(!fp||!fp->write))
		return (ssize_t)-EBADF;
	expr_mutex_lock(fp->mutex);
	r=expr_vapwriteft_r(fp->mode,(intptr_t)fp->buf,format,strlen(format),expr_ftable_default,xmtl,ap);
	expr_mutex_unlock(fp->mutex);
	return r;
}
ssize_t expr_fprintf(struct expr_global_file *fp,const char *format,...){
	va_list ap;
	ssize_t r;
	if(unlikely(!fp||!fp->write))
		return (ssize_t)-EBADF;
	va_start(ap,format);
	expr_mutex_lock(fp->mutex);
	r=expr_vapwriteft_r(fp->mode,(intptr_t)fp->buf,format,strlen(format),expr_ftable_default,xmtl,ap);
	expr_mutex_unlock(fp->mutex);
	va_end(ap);
	return r;
}
ssize_t expr_farrprintf(struct expr_global_file *fp,const char *format,const union expr_argf *restrict args,size_t arglen){
	ssize_t r;
	if(unlikely(!fp||!fp->write))
		return (ssize_t)-EBADF;
	expr_mutex_lock(fp->mutex);
	r=expr_writeft_r(fp->mode,(intptr_t)fp->buf,format,strlen(format),args,arglen,expr_ftable_default,xmtl);
	expr_mutex_unlock(fp->mutex);
	return r;
}
ssize_t expr_printf(const char *format,...){
	va_list ap;
	ssize_t r;
	va_start(ap,format);
	r=expr_vfprintf(expr_stdout,format,ap);
	va_end(ap);
	return r;
}
ssize_t expr_arrprintf(const char *format,const union expr_argf *restrict args,size_t arglen){
	ssize_t r;
	r=expr_farrprintf(expr_stdout,format,args,arglen);
	return r;
}
ssize_t expr_fwrite(struct expr_global_file *fp,const void *buf,size_t size){
	ssize_t r;
	if(unlikely(!fp||!fp->write))
		return (ssize_t)-EBADF;
	expr_mutex_lock(fp->mutex);
	r=expr_buffered_write(fp->buf,buf,size);
	expr_mutex_unlock(fp->mutex);
	return r;
}
ssize_t expr_fread(struct expr_global_file *fp,void *buf,size_t size){
	ssize_t r;
	if(unlikely(!fp||!fp->read))
		return (ssize_t)-EBADF;
	expr_mutex_lock(fp->mutex);
	r=expr_buffered_read(fp->rbuf,buf,size,NULL);
	expr_mutex_unlock(fp->mutex);
	return r;
}
static void *memchr_warped(const void *buf,intptr_t c,size_t size){
	return (void *)(uintptr_t)memchr(buf,(int)c,size);
}
const char *expr_freadline(struct expr_global_file *fp,size_t *outsize){
	ssize_t r;
	char *p;
	if(unlikely(!fp||!fp->read||!fp->rbuf->dynamic)){
		errno=EBADF;
		return NULL;
	}
	expr_mutex_lock(fp->mutex);
	r=expr_buffered_readline(fp->rbuf,memchr_warped,'\n',&p);
	expr_mutex_unlock(fp->mutex);
	if(r<0){
		return NULL;
	}
	if(outsize)
		*outsize=(size_t)r;
	return p;
}
struct expr_global_file *expr_bind(void *stream,const char *mode){
	struct expr_global_file *fp;
	unsigned int r=0,w=0,b=0,d=0,dr=0;
	for(;*mode;++mode)switch(*mode){
		case 'r':
			r=1;
			break;
		case 'w':
			w=1;
			break;
		case 'a':
			w=1;
			break;
		case 'b':
			b=1;
			break;
		case 'd':
			d=1;
			break;
		case 'D':
			dr=1;
			break;
		case '+':
			r=1;
			w=1;
			break;
		default:
			return NULL;
	}
	fp=xmalloc(sizeof(struct expr_global_file));
	if(unlikely(!fp))
		goto err;
	memset(fp,0,sizeof(struct expr_global_file));
	if(r){
		expr_buffered_init_r(fp->rbuf,global_read,(intptr_t)stream,NULL,dr?SIZE_MAX:0,expr_globalmtl);
		fp->read=1;
	}
	if(w){
		expr_buffered_init_r(fp->buf,global_write,(intptr_t)stream,NULL,d?SIZE_MAX:0,expr_globalmtl);
		if(unlikely(expr_setvbuf(fp,NULL,b?_IOFBF:_IOLBF,BUFSIZ)<0))
			goto err1;
		fp->write=1;
		fp->mode=(b?expr_buffered_write_warped:expr_buffered_write_iolbf_warped);
	}
	setvbuf((FILE *)stream,NULL,_IONBF,0);
	if(likely(expr_setuped)){
		GLOBAL_MUTEX_START;
		expr_list_insert_next(global_head,fp->node);
		GLOBAL_MUTEX_END;
	}else
		expr_list_insert_next(global_head,fp->node);
	return fp;
err1:
	expr_close(fp);
err:
	return NULL;
}

struct expr_global_file *expr_open(const char *path,const char *mode){
	FILE *stream;
	struct expr_global_file *fp;
	unsigned int r=0,w=0,b=0,d=0,dr=0;
	char a[4];
	a[0]=*mode;
	a[1]='b';
	for(;*mode;++mode)switch(*mode){
		case 'r':
			r=1;
			break;
		case 'w':
			w=1;
			break;
		case 'a':
			w=1;
			break;
		case 'b':
			b=1;
			break;
		case 'd':
			d=1;
			break;
		case 'D':
			dr=1;
			break;
		case '+':
			r=1;
			w=1;
			break;
		default:
			return NULL;
	}
	if(r&&w){
		a[2]='+';
		a[3]=0;
	}else
		a[2]=0;
	stream=fopen(path,a);
	if(unlikely(!stream))
		return NULL;
	fp=xmalloc(sizeof(struct expr_global_file));
	if(unlikely(!fp))
		goto err;
	memset(fp,0,sizeof(struct expr_global_file));
	setvbuf((FILE *)stream,NULL,_IONBF,0);
	if(r){
		expr_buffered_init_r(fp->rbuf,global_read,(intptr_t)stream,NULL,dr?SIZE_MAX:0,expr_globalmtl);
		fp->read=1;
	}
	if(w){
		expr_buffered_init_r(fp->buf,global_write,(intptr_t)stream,NULL,d?SIZE_MAX:0,expr_globalmtl);
		if(unlikely(expr_setvbuf(fp,NULL,b?_IOFBF:_IOLBF,BUFSIZ)<0))
			goto err1;
		fp->write=1;
		fp->mode=(b?expr_buffered_write_warped:expr_buffered_write_iolbf_warped);
	}
	fp->close=global_close;
	if(likely(expr_setuped)){
		GLOBAL_MUTEX_START;
		expr_list_insert_next(global_head,fp->node);
		GLOBAL_MUTEX_END;
	}else
		expr_list_insert_next(global_head,fp->node);
	return fp;
err1:
	expr_close(fp);
err:
	fclose(stream);
	return NULL;
}

#if !(defined(EXPR_NOSTACKWORKSPACE)&&(EXPR_NOSTACKWORKSPACE))
double expr_internal_strtod(const char *restrict nptr,size_t nsize,size_t *restrict end_index){
	struct expr_strtod_workspace ws[1];
	return expr_internal_strtod4(nptr,nsize,end_index,ws);
}
#endif
expr_globals_define();

__attribute__((noreturn)) void expr_explode(void){
	expr_explode_r(expr_memcontract,expr_allocate_max,expr_defmtl);
	warn_abort("expr-lc/memory-bomb","malloc(1) fails, cannot allocate more memory.");
}
__attribute__((noreturn)) void expr_trap(void){
	__builtin_trap();
}
__attribute__((noreturn)) void expr_ubehavior(void){
	__builtin_unreachable();
}
static ssize_t zero_reader(intptr_t fd,void *buf,size_t size){
	(void)fd;
	memset(buf,0,size);
	debug("write %zu zeros on %p",size,buf);
	return (ssize_t)size;
}
#define checkr(_fp) ({\
	if(unlikely(r<0)){\
		expr_buffered_rclose(_fp);\
		return r;\
	}\
})
ssize_t expr_file_readfd_r(expr_reader_t reader,intptr_t fd,size_t tail,void *savep,const struct expr_memtool *restrict mtl){
	struct expr_buffered_file vf[1];
	ssize_t r;
	ssize_t ret;
	expr_buffered_init_r(vf,reader,fd,NULL,SIZE_MAX,mtl);
	r=expr_buffered_read(vf,NULL,0,NULL);
	checkr(vf);
	ret=(ssize_t)vf->index;
	if(tail){
		size_t total;
		total=vf->index+tail;
		if(vf->length<total){
			vf->reader=zero_reader;
			vf->dynamic=total;
			debug("before add %zu zeros",tail);
			r=expr_buffered_read(vf,NULL,0,NULL);
			debug("add %zu zeros,r=%zd",tail,r);
			checkr(vf);
		}else {
			memset(vf->buf+vf->index,0,tail);
		}
	}
	debug("savep=%p,r=%zu",vf->buf,vf->index);
	*(void **)savep=vf->buf;
	return ret;
}

void expr_mutex_lock(uint32_t *lock){
	//debug("locking");
#ifdef _MUTEX_H_
	mutex_lock((mutex_t *)lock);
#endif
	return;
}
int expr_mutex_trylock(uint32_t *lock){
	//debug("trylocking");
#ifdef _MUTEX_H_
	return mutex_trylock((mutex_t *)lock);
#else
	return 0;
#endif
}
void expr_mutex_unlock(uint32_t *lock){
	//debug("unlocking");
#ifdef _MUTEX_H_
	mutex_unlock((mutex_t *)lock);
#endif
	return;
}
void expr_mutex_spinlock(uint32_t *lock){
#ifdef _MUTEX_H_
	mutex_spinlock((mutex_t *)lock);
#endif
	return;
}
void expr_mutex_spinunlock(uint32_t *lock){
#ifdef _MUTEX_H_
	mutex_spinunlock((mutex_t *)lock);
#endif
	return;
}

intptr_t expr_warped_syscall0(int num){
	return expr_internal_syscall0(num);
}
#ifdef EXPR_SYSA0
intptr_t expr_warped_syscall1(int num,intptr_t a0){
	return expr_internal_syscall1(num,a0);
}
#ifdef EXPR_SYSA1
intptr_t expr_warped_syscall2(int num,intptr_t a0,intptr_t a1){
	return expr_internal_syscall2(num,a0,a1);
}
#ifdef EXPR_SYSA2
intptr_t expr_warped_syscall3(int num,intptr_t a0,intptr_t a1,intptr_t a2){
	return expr_internal_syscall3(num,a0,a1,a2);
}
#ifdef EXPR_SYSA3
intptr_t expr_warped_syscall4(int num,intptr_t a0,intptr_t a1,intptr_t a2,intptr_t a3){
	return expr_internal_syscall4(num,a0,a1,a2,a3);
}
#ifdef EXPR_SYSA4
intptr_t expr_warped_syscall5(int num,intptr_t a0,intptr_t a1,intptr_t a2,intptr_t a3,intptr_t a4){
	return expr_internal_syscall5(num,a0,a1,a2,a3,a4);
}
#ifdef EXPR_SYSA5
intptr_t expr_warped_syscall6(int num,intptr_t a0,intptr_t a1,intptr_t a2,intptr_t a3,intptr_t a4,intptr_t a5){
	return expr_internal_syscall6(num,a0,a1,a2,a3,a4,a5);
}
#ifdef EXPR_SYSA6
intptr_t expr_warped_syscall7(int num,intptr_t a0,intptr_t a1,intptr_t a2,intptr_t a3,intptr_t a4,intptr_t a5,intptr_t a6){
	return expr_internal_syscall7(num,a0,a1,a2,a3,a4,a5,a6);
}
#endif
#endif
#endif
#endif
#endif
#endif
#endif

#if EXPR_SETUP_ENABLED
#define r_fail \
	warn_abort("expr-lc/nonnull-allocator","cannot allocate memory,size=%zu",size)
static int nonnull=0,ckleak=0;
static int expr_mtl_setup=0;
static uint32_t mutex[1]={0};
static ssize_t count=0;
static void *malloc_setup(size_t size,intptr_t arg){
	void *r;
	r=size>expr_allocate_max?NULL:malloc(size);
	if(unlikely(!r&&nonnull))
		r_fail;
	if(likely(r)){
		expr_mutex_lock(mutex);
		++count;
		expr_mutex_unlock(mutex);
	}
	return r;
}
static void *realloc_setup(void *old,size_t size,intptr_t arg){
	void *r;
	r=size>expr_allocate_max?NULL:realloc(old,size);
	if(unlikely(!r&&nonnull))
		r_fail;
	if(unlikely(r&&!old)){
		expr_mutex_lock(mutex);
		++count;
		expr_mutex_unlock(mutex);
	}
	return r;
}
static void free_setup(void *old,intptr_t arg){
	free(old);
	expr_mutex_lock(mutex);
	--count;
	expr_mutex_unlock(mutex);
}
static const struct expr_memtool expr_setupmtl[1]={{
	.allocate=malloc_setup,
	.reallocate=realloc_setup,
	.deallocate=free_setup,
	.arg=0,
}};
static struct expr_memtool expr_oldmtl[1];
int expr_setup_mtl(int flag){
	if(flag&4){
		nonnull=1;
		memcpy(expr_defmtl,expr_setupmtl,sizeof(struct expr_memtool));
		return 0;
	}
	if(expr_mtl_setup){
		memcpy(expr_defmtl,expr_oldmtl,sizeof(struct expr_memtool));
		expr_mtl_setup=0;
		if(unlikely(ckleak&&count)){
			warn_abort("expr-lc/leak-detector","DETECTED MEMORY LEAK IN %zd OBJECTS",count);
		}
		return 1;
	}else {
		nonnull=!!(flag&1);
		ckleak=!!(flag&2);
		memcpy(expr_oldmtl,expr_defmtl,sizeof(struct expr_memtool));
		memcpy(expr_defmtl,expr_setupmtl,sizeof(struct expr_memtool));
		expr_mtl_setup=1;
		return 0;
	}
}

void *expr_heapmtl_malloc_nonnull(size_t size,intptr_t arg){
	void *r=expr_area_malloc((struct expr_area *)arg,size);
	if(unlikely(!r))
		r_fail;
	return r;
}
void *expr_heapmtl_realloc_nonnull(void *old,size_t size,intptr_t arg){
	void *r=expr_area_realloc((struct expr_area *)arg,old,size);
	if(unlikely(!r))
		r_fail;
	return r;
}
#define align_size(val,const_size) (((val)+((const_size)-1))/(const_size))*(const_size)
int expr_setup_heapmtl4_nonnull(struct expr_memtool *restrict mtl,void *heap,size_t size,int flag){
	if(flag&EXPR_ASTARTADDRALIGN){
		uintptr_t alp;
		size_t dif;
		alp=align_size((uintptr_t)heap,sizeof(struct expr_zoneunit));
		dif=alp-(uintptr_t)heap;
		if(dif){
			size-=dif;
			heap=(void *)alp;
		}
	}
	if(unlikely((ssize_t)size<(ssize_t)sizeof(struct expr_area)))
		return -1;
	if(unlikely(expr_area_init4((struct expr_area *)heap,expr_zoneof(heap),size-sizeof(struct expr_area),flag)<0))
		return -1;
	mtl->allocate=expr_heapmtl_malloc_nonnull;
	mtl->reallocate=expr_heapmtl_realloc_nonnull;
	mtl->deallocate=expr_heapmtl_dealloc;
	mtl->arg=(intptr_t)heap;
	return 0;
}

#endif
