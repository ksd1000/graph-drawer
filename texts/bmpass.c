#include <stdio.h>	
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include "text.h"
#define BUFSIZE 1024
#define printval(x) fprintf(stderr,#x ":%lu\n",x)
#define printvall(x) fprintf(stderr,#x ":%ld\n",x)
#define printvald(x) fprintf(stderr,#x ":%lf\n",x)
static size_t sbsize_max=0;
ssize_t readall(int fd,void **pbuf){
	char *buf,*p;
	size_t bufsiz,r1;
	ssize_t r,ret=0;
	int i;
	bufsiz=BUFSIZE;
	if((buf=malloc(BUFSIZE))==NULL)return -errno;
	//memset(buf,0,BUFSIZE);
	//lseek(fd,0,SEEK_SET);
	r1=0;
	while((r=read(fd,buf+ret,BUFSIZE-r1))>0){
		r1+=r;
		ret+=r;
		if(ret==bufsiz){
			bufsiz+=BUFSIZE;
			if((p=realloc(buf,bufsiz))==NULL){
				i=errno;
				free(buf);
				return -i;
			}
			buf=p;
		//	memset(buf+bufsiz-BUFSIZE,0,BUFSIZE);
			r1=0;
		}
	}
	if(ret==bufsiz){
	if((p=realloc(buf,bufsiz+1))==NULL){
		i=errno;
		free(buf);
		return -i;
	}
	buf=p;
	}
	buf[ret]=0;
	*pbuf=buf;
	return ret;
}
static int scanv(const char *data,int32_t x){
	const char *text=data+*(int32_t *)(data+10);
	int32_t ow=(((*(int32_t *)(data+18))*8+31)>>5)<<2,
	h=*(int32_t *)(data+22);
	for(int32_t y=0;y<h;++y){
		if(text[y*ow+x]==0x00){
			//printf("found at x=%d\n",x);
			return 1;
		}
	}
	return 0;
}
static int scanh(const char *data,int32_t y){
	const char *text=data+*(int32_t *)(data+10);
	//int32_t h=*(int32_t *)(data+22),
	int32_t w=*(int32_t *)(data+18);
	int32_t ow=((w*8+31)>>5)<<2;
	for(int32_t x=0;x<w;++x){
		if(text[y*ow+x]==0x00){
			//printf("found at y=%d\n",y);
			return 1;
		}
	}
	return 0;
}
static void correct(const char *data,int32_t *x1,int32_t *y1,int32_t *x2,int32_t *y2){
	int32_t v,w=*(int32_t *)(data+18),h=*(int32_t *)(data+22);
	//printf("from %d,%d %d,%d\n",*x1,*y1,*x2,*y2);
	for(v=0;v<w;++v){
		if(!scanv(data,v))continue;
		if(v>*x1)*x1=v;
		break;
	}
	for(v=0;v<h;++v){
		if(!scanh(data,v))continue;
		if(v<*y1)*y1=v;
		break;
	}
	for(v=w-1;v>=0;--v){
		if(!scanv(data,v))continue;
		if(v<*x2)*x2=v;

		break;
	}
	for(v=h-1;v>=0;--v){
		if(!scanh(data,v))continue;
		if(v>*y2)*y2=v;
		break;
	}
	//printf("to %d,%d %d,%d\n",*x1,*y1,*x2,*y2);
	//*x1=0;*y1=0;*x2=w-1;*y2=h-1;
}
static void cut(const char *name,const char *data,int32_t x1,int32_t y1,int32_t x2,int32_t y2){
	const char *text=data+*(int32_t *)(data+10);
	char nbuf[16],c;
	sscanf(name,"0x%hhx",&c);
	if(c==' ')x2>>=2;
	int32_t w=x2-x1,h=y2-y1;
	int32_t ow=(((*(int32_t *)(data+18))*8+31)>>5)<<2;
	size_t sbsize=(w*h+7)/8+sizeof(struct sbmp);
	struct sbmp *buf=malloc(sbsize);
	int fd;
	memset(buf,0,sbsize);
	//printf("%s cut to %dx%d\n",name,w,h);
	buf->width=w;buf->height=h;
	buf->c=c;
	for(int32_t x=0;x<w;++x)
	for(int32_t y=0;y<h;++y){
		if(!text[(y+y1)*ow+(x+x1)])
			TEXT_SETPIXEL(buf,x,y);
	}
	sprintf(nbuf,"0x%hhx.sbmp",buf->c);
	assert(fd=open(nbuf,O_WRONLY|O_CREAT,S_IRUSR|S_IWUSR));
	pwrite(fd,buf,sbsize,0);
	if(sbsize>sbsize_max)sbsize_max=sbsize;
	ftruncate(fd,sbsize);
	close(fd);
	free(buf);
}
int main(int argc,char **argv){
	int *fds=malloc((argc-1)*sizeof(int));
	int32_t *x1s=malloc((argc-1)*sizeof(int32_t));
	int32_t *x2s=malloc((argc-1)*sizeof(int32_t));
	char **datas=malloc((argc-1)*sizeof(char *));
	int32_t y1=INT32_MAX,y2=INT32_MIN;

	for(int i=1;i<argc;++i){
		x1s[i]=0;
		fds[i]=open(argv[i],O_RDONLY);
		if(fds[i]<0)continue;
		assert(readall(fds[i],(void **)datas+i)>0);
		x1s[i]=0;
		x2s[i]=*(int32_t *)(datas[i]+18);
		//printf("reading %s\n",argv[i]);
		correct(datas[i],x1s+i,&y1,x2s+i,&y2);
	}
	for(int i=1;i<argc;++i){
		if(fds[i]<0)continue;
		cut(argv[i],datas[i],x1s[i],y1,x2s[i],y2);
		close(fds[i]);
		free(datas[i]);
	}
	free(fds);
	free(x1s);
	free(x2s);
	free(datas);
	printf("%zu\n",sbsize_max);
	return 0;
}
