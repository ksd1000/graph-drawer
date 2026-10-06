/*******************************************************************************
 *License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>*
 *This is free software: you are free to change and redistribute it.           *
 *******************************************************************************/
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#define EXPR_BLOCKWARNING 1
#include "expr.h"
#include <time.h>
#include <math.h>
#include <float.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#ifdef __unix__
#define REAL_UNIX
#include <unistd.h>
#include <sys/socket.h>
#include <sys/random.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <syscall.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <fcntl.h>
static const char *sysnames[]={
#define register_syscall(sysid) [__NR_##sysid]=#sysid,
#include "systable.c"
#undef register_syscall
};
#endif
#include "fake_unix.h"
#define casting(x,T) ((T)(x))
#define cast(x,T) expr_cast(x,T)
#define castingd(x) ((double)(x))

#define warp1(rtype,sym,atype) static double d_##sym(double x){\
	rtype r=(rtype)sym(casting(x,atype));\
	return castingd(r);\
}
#define warpip(rtype,sym,atype) static double d_##sym(double x){\
	rtype r=(rtype)sym(cast(x,atype));\
	return castingd(r);\
}
#define warppi(rtype,sym,atype) static double d_##sym(double x){\
	rtype r=(rtype)sym(casting(x,atype));\
	return cast(r,double);\
}
#define warp2(rtype,sym,at0,at1) static double d_##sym(double *v,size_t n){\
	rtype r=(rtype)sym(casting(v[0],at0),casting(v[1],at1));\
	return castingd(r);\
}
#define warppip(rtype,sym,at0,at1) static double d_##sym(double *v,size_t n){\
	rtype r=(rtype)sym(casting(v[0],at0),cast(v[1],at1));\
	return expr_cast(r,double);\
}
#define warpppi(rtype,sym,at0,at1) static double d_##sym(double *v,size_t n){\
	rtype r=(rtype)sym(cast(v[0],at0),casting(v[1],at1));\
	return expr_cast(r,double);\
}
#define warp3(rtype,sym,at0,at1,at2) static double d_##sym(double *v,size_t n){\
	rtype r=(rtype)sym(casting(v[0],at0),casting(v[1],at1),casting(v[2],at2));\
	return castingd(r);\
}
#define warpiipi(rtype,sym,at0,at1,at2) static double d_##sym(double *v,size_t n){\
	rtype r=(rtype)sym(casting(v[0],at0),cast(v[1],at1),casting(v[2],at2));\
	return castingd(r);\
}
#define warpiipp(rtype,sym,at0,at1,at2) static double d_##sym(double *v,size_t n){\
	rtype r=(rtype)sym(casting(v[0],at0),cast(v[1],at1),cast(v[2],at2));\
	return castingd(r);\
}
#define warpipii(rtype,sym,at0,at1,at2) static double d_##sym(double *v,size_t n){\
	rtype r=(rtype)sym(cast(v[0],at0),n>1?casting(v[1],at1):0,n>2?casting(v[2],at2):0);\
	return castingd(r);\
}
#define warpz(rtype,sym) static double d_##sym(void){\
	rtype r=(rtype)sym();\
	return castingd(r);\
}
#ifdef REAL_UNIX
warpz(pid_t,fork)
warpz(pid_t,vfork)
warpz(pid_t,getpid)
warpz(pid_t,getppid)
warpz(pid_t,gettid)
warpz(pid_t,getuid)
warpz(pid_t,getgid)
warpz(pid_t,geteuid)
warpz(pid_t,getegid)
warp1(int,setuid,uid_t)
warp1(int,setgid,gid_t)
warp1(int,seteuid,uid_t)
warp1(int,setegid,gid_t)
warp1(int,raise,int)
warpip(in_addr_t,inet_addr,void *)
warpip(pid_t,wait,void *)
warp1(uint32_t,htonl,uint32_t)
warp1(uint16_t,htons,uint16_t)
double last_sig;
struct expr *volatile sigep[64+1];
const struct addrinfo ai_req_icmp[1]={{
	.ai_family=AF_INET,
	.ai_socktype=SOCK_DGRAM,
	//.ai_protocol=IPPROTO_ICMP,
}};
static in_addr_t inet_addr2(const char *cp){
	struct addrinfo *ai0,*ai;
	in_addr_t ret=INADDR_NONE;
	if(getaddrinfo(cp,NULL,ai_req_icmp,&ai0)<0)
		return INADDR_NONE;
	for(ai=ai0;ai;ai=ai->ai_next){
		//if(ai->ai_addrlen!=16)
		//	continue;
		ret=((struct sockaddr_in *)ai->ai_addr)->sin_addr.s_addr;
		//if(ret!=INADDR_NONE)
		//	break;
	}
	freeaddrinfo(ai0);
	return ret;
}
in_addr_t inet_addr(const char *cp) __attribute__((alias("inet_addr2")));
void d_setsig(int sig){
 	last_sig=(double)sig;
}
void d_sigep(int sig){
	expr_eval(sigep[sig],(double)sig);
}
double d_signalep(double *v,size_t n){
	int s=(int)v[0];
	sigep[s]=cast(v[1],struct expr *);
	return cast(signal(s,d_sigep),double);
}
double dtime(void){
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME,&ts);
	return (double)ts.tv_sec+ts.tv_nsec/1000000000.0;
}
double d_sleep(double x){
	struct timespec rts,ts;
	double fx;
	x=fabs(x);
	fx=floor(x);
	ts.tv_sec=(time_t)fx;
	ts.tv_nsec=(time_t)((x-fx)*1000000000.0);
	memset(&rts,0,sizeof(struct timespec));
	nanosleep(&ts,&rts);
	return (double)rts.tv_sec+rts.tv_nsec/1000000000.0;
}

#include <sys/time.h>
double d_alarm(double x){
	struct itimerval rtv,tv;
	double fx;
	x=fabs(x);
	fx=floor(x);
	tv.it_value.tv_sec=(time_t)fx;
	tv.it_value.tv_usec=(time_t)((x-fx)*1000000.0);
	memset(&tv.it_interval,0,sizeof(struct timeval));
	memset(&rtv,0,sizeof(struct itimerval));
	if(setitimer(ITIMER_REAL,&tv,&rtv)<0)
		return -1.0;
	return (double)rtv.it_value.tv_sec+rtv.it_value.tv_usec/1000000000.0;
}
#include <sys/poll.h>
#include <fcntl.h>
ssize_t nonblock_read(int fd,void *buf,size_t len){
	struct pollfd pf;
	pf.fd=fd;
	pf.events=POLLIN;
	switch(poll(&pf,1,0)){
		case 1:
			return read(fd,buf,len);
		case 0:
			return 0;
		default:
			return -1;
	}
}
#endif
warppi(void *,strerror,int)
#define a2s(buf,args,size) buf=alloca(size+1);\
	for(size_t i=0;i<size;++i)\
		buf[i]=(char)*(args++)
double d_write(double *args,size_t n){
	size_t size;
	union {
		void *buf;
		double dr;
	} un;
	int fd;
	fd=(int)*args;
	un.dr=*(++args);
	size=(size_t)*(++args);
	return write(fd,un.buf,size);
}
double d_read(double *args,size_t n){
	size_t size;
	union {
		void *buf;
		double dr;
	} un;
	int fd;
	fd=(int)*args;
	un.dr=*(++args);
	size=(size_t)*(++args);
	return read(fd,un.buf,size);
}
static ssize_t linebuf(intptr_t fd,const void *buf,size_t size){
	return expr_buffered_write_flushat((struct expr_buffered_file *)fd,buf,size,"\n",1);
}
double d_printk(double *args,size_t n){
	const char *fmt=cast(*args,const char *);
	size_t flen=strlen(fmt);
	struct expr_buffered_file f;
	ssize_t r,r1;
	int kfd=open("/dev/kmsg",O_WRONLY);
	if(kfd<0)
		return kfd;
	f.writer=(expr_writer)write;
	f.fd=kfd;
	f.buf=NULL;
	f.index=0;
	f.dynamic=BUFSIZ;
	f.length=0;
	r=expr_writef(fmt,flen,linebuf,(intptr_t)&f,(const union expr_argf *)args+1,n-1);
	r1=expr_buffered_close(&f);
	if(r>=0){
		if(r1<0)
			r=r1;
		else
			r+=r1;
	}
	close(kfd);
	return (double)r;
}
char getchbuf[6];
struct expr_buffered_file getchf=EXPR_BUFFERED_INITIALIZER(read,0,getchbuf,sizeof(getchf));
double d_getchar(void){
	unsigned char c;
	ssize_t r;
	getchf.fd=STDIN_FILENO;
	r=expr_buffered_read(&getchf,&c,1);
	if(r<1)
		return -1;
	return (double)c;
}
int isprime(uint64_t n){
	if(n==2)return 1;
	if(!(n&1))return 0;
	uint64_t end=(uint64_t)(sqrt(n)+1.0);
	for(uint64_t i=3;i<end;i+=2)
		if(!(n%i))return 0;
	return 1;
}
uint64_t prime(uint64_t x){
	static uint64_t *p=NULL;
	static size_t n=0;
	static size_t size=0;
	static int im65=0;
	uint64_t *lp;
	uint64_t endn;
	size_t sizem1;
	if(!n){
		lp=malloc((size=5)*sizeof(uint64_t));
		if(!lp)return 0;
		p=lp;
		p[0]=1;
		p[1]=2;
		p[2]=3;
		p[3]=5;
		p[4]=7;
		n=4;
	}
	if(x<=n)return p[x];
	if(x+1>=size){
		sizem1=(x+1025)&~1023;
		lp=realloc(p,sizem1*sizeof(uint64_t));
		if(!lp)return 0;
		p=lp;
		size=sizem1;
	}
	sizem1=size-1;
	for(uint64_t i=p[n]+(im65?2:4);n<sizem1;){
		endn=(uint64_t)(sqrt((double)i)+1.0);
		for(uint64_t i1=1;p[i1]<=endn&&i1<=n;++i1){
			if(!(i%p[i1]))goto fail;
		}
		p[++n]=i;
fail:
		if(im65^=1)i+=2;
		else i+=4;
	}
	return p[x];
}
uint64_t prime_mt(uint64_t x){
	static uint32_t mutex=0;
	uint64_t r;
	expr_mutex_lock(&mutex);
	r=prime(x);
	expr_mutex_unlock(&mutex);
	return r;
}
uint64_t prime_old(uint64_t x){
	uint64_t im65,im652;
	uint64_t i,endn;
	switch(x){
		case 0:return 1;
		case 1:return 2;
		case 2:return 3;
		case 3:return 5;
		case 4:return 7;
		default:break;
	}
	im65=0;
	x-=4;
	for(i=11;;){
		endn=(uint64_t)(sqrt((double)i)+1.0);
		if(!(i&1))goto fail;
		if(!(i%3))goto fail;
		im652=0;
		for(uint64_t i1=5;i1<=endn;){
			if(!(i%i1))goto fail;
			if(im652^=1)i1+=2;
			else i1+=4;
		}
		if(!--x)break;
fail:
		if(im65^=1)i+=2;
		else i+=4;
	}
	return i;
}
double cal_prime(double x,uint64_t (*f)(uint64_t)){
	double fx,xmfx;
	if(x<=0.0)return 1.0;
	fx=floor(x);
	xmfx=x-fx;
	if(xmfx<=DBL_EPSILON)
		return (double)f((uint64_t)(x));
	return (1.0-xmfx)*(double)f((uint64_t)(fx))
		+xmfx*(double)f(1ul+(uint64_t)fx);
}
double d_prime(double x){
	return cal_prime(x,prime);
}
double d_prime_mt(double x){
	return cal_prime(x,prime_mt);
}
double d_prime_old(double x){
	return cal_prime(x,prime_old);
}
double d_isprime(double x){
	return (double)isprime((uint64_t)(fabs(x)));
}
double geterrno(void){
	return cast(&errno,double);
}
void *readall(intptr_t fd,size_t *len){
	char *save;
	ssize_t r=expr_file_readfd((void *)read,fd,1,&save);
	if(r<0)
		return NULL;
	if(len)
		*len=r;
	return save;
}
double d_readline(double x){
	size_t n;
	char *buf=readall(STDIN_FILENO,NULL),*p;
	if(!buf)
		return 0.0;
	p=buf;
	n=1;
	while((p=strchr(p,'\n'))){
		*p=0;
		++p;
		++n;
	}
	*expr_cast(x,double *)=(double)n;
	return expr_cast(buf,double);
}
struct expr_buffered_file r1f=EXPR_BUFFERED_INITIALIZER(read,0,NULL,SIZE_MAX);
double d_readline1(double x){
	ssize_t r;
	char *p;
	r1f.fd=STDIN_FILENO;
	r=expr_buffered_readline(&r1f,'\n',&p);
	*expr_cast(x,void **)=p;
	return (double)r;
}
double d_endline1(void){
	expr_buffered_rclose(&r1f);
	return 0;
}
volatile double vx[8];
/*
struct expr_symbol *symset_add(struct expr_symset *restrict esp,const char *sym,int type,int flag,...){
	va_list ap;
	struct expr_symbol *r;
	va_start(ap,flag);
	r=expr_symset_vadd(esp,sym,type,flag,ap);
	warnx("vadd:%p -- %s",r,sym);
	va_end(ap);
	return r;
}
*/
#define symset_add(esp,sym,type,flag,...) ({\
	if(!(expr_symset_add(esp,sym,type,flag,##__VA_ARGS__)))\
		errx(EXIT_FAILURE,"cannot add symbol %s",sym);\
})
#define setza(c) symset_add(es,#c,EXPR_ZAFUNCTION,EXPR_SUNSAFE,d_##c)
#define setzau(c) symset_add(es,#c,EXPR_ZAFUNCTION,EXPR_SUNSAFE,d_##c)
#define setfunc(c) symset_add(es,#c,EXPR_FUNCTION,EXPR_SUNSAFE,d_##c,EXPR_SUNSAFE)
#define setfunci(c) symset_add(es,#c,EXPR_FUNCTION,EXPR_SUNSAFE,d_##c,EXPR_SPURE)
#define setmd(c,dim) symset_add(es,#c,EXPR_MDFUNCTION,EXPR_SUNSAFE,d_##c,(size_t)dim)
#define setconst(c) symset_add(es,#c,EXPR_CONSTANT,0,(double)(c))
const struct expr_builtin_symbol systable[];
void add_common_symbols(struct expr_symset *es){
	char buf[32];
	for(size_t i=0;i<(sizeof(vx)/sizeof(*vx));++i){
		sprintf(buf,"x%zu",i);
		symset_add(es,buf,EXPR_VARIABLE,0,vx+i);
	}
	symset_add(es,"errno",EXPR_VARIABLE,0,&errno);
	symset_add(es,"geterrno",EXPR_ZAFUNCTION,EXPR_SUNSAFE,geterrno);
	setza(getchar);
	setfunc(readline);
	setfunc(readline1);
	setza(endline1);
	setfunci(isprime);
	setfunci(prime);
	setfunci(prime_mt);
	setfunci(prime_old);
	setfunc(strerror);
	setconst(EXIT_FAILURE);
	setconst(EXIT_SUCCESS);
	symset_add(es,"systable",EXPR_CONSTANT,EXPR_SPACKAGE,systable);
#ifdef REAL_UNIX
	symset_add(es,"time",EXPR_ZAFUNCTION,EXPR_SUNSAFE,dtime);
	symset_add(es,"sig",EXPR_VARIABLE,0,&last_sig);
	symset_add(es,"sigepv",EXPR_VARIABLE,0,&sigep);
	setza(getpid);
	setza(getppid);
	setza(gettid);
	setza(getuid);
	setza(geteuid);
	setza(getgid);
	setza(getegid);
	setzau(fork);
	setzau(vfork);
	setfunc(alarm);
	setfunci(htonl);
	setfunci(htons);
	setfunc(sigep);
	setfunc(raise);
	setfunc(setsig);
	setfunc(setuid);
	setfunc(seteuid);
	setfunc(setgid);
	setfunc(setegid);
	setfunc(sleep);
	setfunc(wait);
	setmd(inet_addr,0);
	setmd(printk,0);
	setmd(read,3);
	setmd(signalep,2);
	setmd(write,3);
	setconst(AF_UNIX);
	setconst(AF_INET);
	setconst(AF_INET6);
	setconst(AF_PACKET);
	setconst(PF_UNIX);
	setconst(PF_INET);
	setconst(PF_INET6);
	setconst(PF_PACKET);
	setconst(SOCK_DGRAM);
	setconst(SOCK_RAW);
	setconst(SOCK_RDM);
	setconst(SOCK_SEQPACKET);
	setconst(SOCK_STREAM);
	setconst(IPPROTO_ICMP);
	setconst(IPPROTO_IP);
	setconst(IPPROTO_IGMP);
	setconst(IPPROTO_TCP);
	setconst(IPPROTO_UDP);
	setconst(O_APPEND);
	setconst(O_ASYNC);
	setconst(O_CLOEXEC);
	setconst(O_CREAT);
	//setconst(O_DIRECT);
	setconst(O_DIRECTORY);
	setconst(O_DSYNC);
	setconst(O_EXCL);
	//setconst(O_LARGEFILE);
	//setconst(O_NOATIME);
	setconst(O_NOCTTY);
	setconst(O_NDELAY);
	setconst(O_NOFOLLOW);
	setconst(O_NONBLOCK);
	//setconst(O_PATH);
	setconst(O_RDONLY);
	setconst(O_RDWR);
	setconst(O_SYNC);
	//setconst(O_TMPFILE);
	setconst(O_TRUNC);
	setconst(O_WRONLY);
	symset_add(es,"SIG_DFL",EXPR_CONSTANT,expr_cast(SIG_DFL,double));
	symset_add(es,"SIG_ERR",EXPR_CONSTANT,expr_cast(SIG_ERR,double));
	symset_add(es,"SIG_IGN",EXPR_CONSTANT,expr_cast(SIG_IGN,double));
	setconst(SIGHUP);
	setconst(SIGINT);
	setconst(SIGQUIT);
	setconst(SIGILL);
	setconst(SIGTRAP);
	setconst(SIGABRT);
	setconst(SIGBUS);
	setconst(SIGFPE);
	setconst(SIGKILL);
	setconst(SIGUSR1);
	setconst(SIGSEGV);
	setconst(SIGUSR2);
	setconst(SIGPIPE);
	setconst(SIGALRM);
	setconst(SIGTERM);
	setconst(SIGSTKFLT);
	setconst(SIGCHLD);
	setconst(SIGCONT);
	setconst(SIGSTOP);
	setconst(SIGTSTP);
	setconst(SIGTTIN);
	setconst(SIGTTOU);
	setconst(SIGURG);
	setconst(SIGXCPU);
	setconst(SIGXFSZ);
	setconst(SIGVTALRM);
	setconst(SIGPROF);
	setconst(SIGWINCH);
	setconst(SIGIO);
	setconst(SIGPWR);
	setconst(SIGSYS);
	setconst(SIGRTMIN);
	setconst(SIGRTMAX);
	setconst(STDIN_FILENO);
	setconst(STDOUT_FILENO);
	setconst(STDERR_FILENO);
	setconst(AT_FDCWD);
	setconst(GRND_NONBLOCK);
	setconst(GRND_RANDOM);
	setconst(GRND_INSECURE);
	setconst(INADDR_NONE);
	symset_add(es,"pid",EXPR_CONSTANT,0,(double)getpid());
	symset_add(es,"ppid",EXPR_CONSTANT,0,(double)getppid());
	symset_add(es,"uid",EXPR_CONSTANT,0,(double)getuid());
	symset_add(es,"gid",EXPR_CONSTANT,0,(double)getgid());
#endif
}
#ifdef REAL_UNIX
const char *sysname(unsigned int id){
	const char *r;
	if(id>=sizeof(sysnames)/sizeof(*sysnames))
		return "unknown";
	r=sysnames[id];
	if(!r)
		return "unknown";
	return r;
}
const struct expr_builtin_symbol systable[]={
#define register_syscall(id) {.strlen=sizeof(#id)-1+4,.str="sys_" #id,.type=EXPR_CONSTANT,.un={.value=(double)(__NR_##id)}},
#include "systable.c"
#undef register_syscall
	{.str=NULL}
};
#else
const struct expr_builtin_symbol systable[]={
	{.str=NULL}
};
#endif
void add_all_common_symbols(struct expr_symset *es){
	add_common_symbols(es);
	expr_builtin_symbol_addall(es,systable);
}
