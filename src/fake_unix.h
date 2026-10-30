
/*******************************************************************************
 *License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>*
 *This is free software: you are free to change and redistribute it.           *
 *******************************************************************************/
#ifndef _FAKE_UNIX_H_
#define _FAKE_UNIX_H_

#ifdef __unix__
#include <unistd.h>
#include <fcntl.h>
#include <unistd.h>
#include <err.h>
#include <pthread.h>
#include <syscall.h>
#define fderr(fd) ((fd)<0)
#define ERRFD (-1)
#define O_WRONLY_CREAT O_WRONLY|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR
static __attribute__((unused)) ssize_t real_read(intptr_t fd,void *buf,size_t size){
	return (ssize_t)expr_internal_syscall(SYS_read,fd,buf,size);
}
static __attribute__((unused)) ssize_t real_write(intptr_t fd,const void *buf,size_t size){
	return (ssize_t)expr_internal_syscall(SYS_write,fd,buf,size);
}
#define read real_read
#define write real_write

#else

#include <stdio.h>
#include <errno.h>

#undef STDIN_FILENO
#undef STDOUT_FILENO
#undef STDERR_FILENO
#define STDIN_FILENO ((intptr_t)0)
#define STDOUT_FILENO ((intptr_t)1)
#define STDERR_FILENO ((intptr_t)2)

#ifdef O_RDONLY
#undef O_RDONLY
#endif
#ifdef O_WRONLY
#undef O_WRONLY
#endif

#define O_RDONLY "rb"
#define O_WRONLY "wb"
#define O_WRONLY_CREAT "wb"
#define fderr(fd) (!(fd))
#define ERRFD ((intptr_t)NULL)
static FILE *fake_d2p(intptr_t fd){
	FILE *fp;
	switch(fd){
		case STDIN_FILENO:
			fp=stdin;
			break;
		case STDOUT_FILENO:
			fp=stdout;
			break;
		case STDERR_FILENO:
			fp=stderr;
			break;
		default:
			fp=(FILE *)fd;
			break;
	}
	return fp;
}
static __attribute__((unused)) intptr_t fake_open(const char *path,const char *flags){
	FILE *fp;
	errno=0;
	fp=fopen(path,flags);
	return (intptr_t)fp;
}
#define open(fd,flag,...) fake_open((fd),(flag))
static __attribute__((unused)) int fake_close(intptr_t fd){
	errno=0;
	fclose(fake_d2p(fd));
	if(errno)
		return -errno;
	return 0;
}
#define close fake_close
static __attribute__((unused)) ssize_t fake_read(intptr_t fd,void *buf,size_t size){
	ssize_t r;
	errno=0;
	r=fread(buf,1,size,fake_d2p(fd));
	if(errno)
		return -errno;
	return r;
}
#define read fake_read
static __attribute__((unused)) ssize_t fake_write(intptr_t fd,const void *buf,size_t size){
	ssize_t r;
	FILE *fp=fake_d2p(fd);
	errno=0;
	r=fwrite(buf,1,size,fp);
	if(errno)
		return -errno;
	fflush(fp);
	return r;
}
#define write fake_write

#define err(v,fmt,...) ({fprintf(stderr,fmt ":%s\n",##__VA_ARGS__,strerror(errno));exit(v);})
#define errx(v,fmt,...) ({fprintf(stderr,fmt "\n",##__VA_ARGS__);exit(v);})
#define warn(fmt,...) ({fprintf(stderr,fmt ":%s\n",##__VA_ARGS__,strerror(errno));})
#define warnx(fmt,...) ({fprintf(stderr,fmt "\n",##__VA_ARGS__);})

#ifdef pthread_t
#undef pthread_t
#endif
typedef void *fake_pthread_t;
#define pthread_t fake_pthread_t
#define pthread_create(thread,attr,routine,arg) (*(thread)=(routine)(arg),0)
#define pthread_join(thread,retval) ((thread)?-1:0)

#endif

#endif
