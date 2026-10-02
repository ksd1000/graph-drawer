#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stdint.h>
#include <float.h>
#include <math.h>
#define SIGN(p) (*(uint64_t *)(p)&(1ul<<63))
#define c_trywrite(buf,sz) {\
	r=writer(fd,(buf),(sz));\
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
#define unlikely(x) (!!(x))
#define likely(x) (!!(x))
static ssize_t writeext(ssize_t (*writer)(intptr_t fd,const void *buf,size_t size),intptr_t fd,size_t count,int c){
	char buf[128];
	ssize_t r,sum;
	if(count<=128){
		memset(buf,c,count);
		return writer(fd,buf,count);
	}
	memset(buf,c,128);
	sum=0;
	do {
		c_trywrite(buf,128);
		count-=128;
	}while(count>128);
	if(count){
		c_trywrite(buf,count);
	}
	return sum;
}
//WARNING:the converter is not accurated as libc printf.I have not find a way to fix it now.
static ssize_t converter_f(ssize_t (*writer)(intptr_t fd,const void *buf,size_t size),intptr_t fd,void *const *arg,uint64_t flag){
	double val=*(const double *)arg;
	char nbuf[320];
	char *endp;
	char *p;
	int positive,f61;
	ssize_t ext,sum,r,sz;
	uint32_t digit;
	intmax_t ds;
	if(unlikely(!isfinite(val))){
		abort();
#define write_sign_to_p(_neg) \
		if(_neg){\
			*(p++)='-';\
		}else {\
			if(flag&(3ul<<62ul))\
				*(p++)='+';\
		}
	}
	endp=nbuf+320;
	p=endp;
	if(SIGN(&val)){
		val=-val;
		positive=0;
	}else {
		positive=1;
	}
	sum=0;
	p=nbuf;
	write_sign_to_p(!positive);
	if(likely(val>=1.0)){
		double p10,p1;
		char *p0;
		ds=(intmax_t)(log(val)/M_LN10)+1;
		p+=ds;
		p0=p;
		p1=1.0;
		for(;;){
			p10=p1*10;
			*(--p0)=(char)((unsigned char)(fmod(val,p10)/p1)+'0');
			--ds;
			if(unlikely(!ds))
				break;
			p1=p10;
		}
	}else {
		*(p++)='0';
	}
	f61=!!(flag&(1ul<<61ul));
	sz=(p-nbuf);
	digit=(flag>>29ul)&0x1ffffffful;
	if(digit)
		sz+=1+digit;
#define writeext_f \
	ext=(ssize_t)(flag&0x1ffffffful)-sz;\
	if(ext>0){\
		c_trywriteext((flag&(1ul<<59ul))?'0':' ',ext);\
	}
	if(!f61){
		writeext_f;
	}
	c_trywrite(nbuf,p-nbuf);
	if(digit){
		p=nbuf;
		*(p++)='.';
		do {
			val=fmod(val*10,10);
			*(p++)=(char)((unsigned char)val+'0');
			if(unlikely(p>=endp)){
				c_trywrite(nbuf,320);
				p=nbuf;
			}
		}while(--digit);
		if(likely(p>nbuf)){
			c_trywrite(nbuf,p-nbuf);
		}
	}
	if(f61){
		writeext_f;
	}
	return sum;
}
ssize_t fwriter(intptr_t fd,const void *buf,size_t size){
	return fwrite(buf,size,1,(FILE *)fd);
}
int main(void){
	double v[1]={DBL_MAX};
	printf("converter_f:");
	converter_f(fwriter,(intptr_t)stdout,(void **)v,0);
	printf("\nlibc printf:%.lf\n",*v);
	return -1;
}
