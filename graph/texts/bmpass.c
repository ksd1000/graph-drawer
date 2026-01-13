/*******************************************************************************
 *License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>*
 *This is free software: you are free to change and redistribute it.           *
 *******************************************************************************/
#define _GNU_SOURCE
#include <stdio.h>	
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <err.h>
#include <limits.h>
#include <string.h>
#include "sbmp.h"
#define BUFSIZE (1024*1024)
static size_t sbsize_max=0;
static size_t sbosize_max=0;
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
static int scanv(const unsigned char *data,int32_t x){
	const unsigned char *text=data+*(int32_t *)(data+10);
	int32_t ow=(((*(int32_t *)(data+18))*8+31)>>5)<<2,
	h=*(int32_t *)(data+22);
	for(int32_t y=0;y<h;++y){
		if(text[y*ow+x]==0x00){
			//fprintf(stdout,"found at x=%d\n",x);
			return 1;
		}
	}
	return 0;
}
static int scanh(const unsigned char *data,int32_t y){
	const unsigned char *text=data+*(int32_t *)(data+10);
	//int32_t h=*(int32_t *)(data+22),
	int32_t w=*(int32_t *)(data+18);
	int32_t ow=((w*8+31)>>5)<<2;
	for(int32_t x=0;x<w;++x){
		if(text[y*ow+x]==0x00){
			//fprintf(stdout,"found at y=%d\n",y);
			return 1;
		}
	}
	return 0;
}
static void correct(const unsigned char *data,int32_t *x1,int32_t *y1,int32_t *x2,int32_t *y2){
	int32_t v,w=*(int32_t *)(data+18),h=*(int32_t *)(data+22);
	//fprintf(stdout,"from %d,%d %d,%d\n",*x1,*y1,*x2,*y2);
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
	//fprintf(stdout,"to %d,%d %d,%d\n",*x1,*y1,*x2,*y2);
	//*x1=0;*y1=0;*x2=w-1;*y2=h-1;
}
static void cut(const char *name,const unsigned char *data,int32_t x1,int32_t y1,int32_t x2,int32_t y2){
	const unsigned char *text=data+*(int32_t *)(data+10);
	char nbuf[16],c;
	if(y1==INT_MAX)y1=*(int32_t *)(data+18);
	assert(sscanf(name,"0x%hhx.bmp",&c)==1);
	if(c==' ')x2>>=2;
	int32_t w=x2-x1,h=y2-y1;
	int32_t ow=(((*(int32_t *)(data+18))*8+31)>>5)<<2;
	int64_t index;
	size_t sbsize=(w*h+7)/8+sizeof(struct sbmp);
	struct sbmp *buf,*cbuf;
	int fd;
	
	buf=malloc(sbsize);//fputs("\nsb\n",stderr);
	assert(buf);
	memset(buf,0,sbsize);
	//errx(-1,"buf:%p",buf->data);
	//fprintf(stdout,"%s cut to %dx%d\n",name,w,h);
	buf->width=w;buf->height=h;
	buf->c=c;
	buf->compressed=0;
	buf->size=sbsize-sizeof(struct sbmp);
	//fprintf(stdout,"%d\n",*(int32_t *)(data+10));
	for(int32_t x=0;x<w;++x){
	for(int32_t y=0;y<h;++y){
			//fprintf(stderr,"index %d %d %d %d %d\n",y,y1,ow,x,x1);
			//fprintf(stderr,"%u",(text[(y+y1)*ow+(x+x1)]<128u));
		if(text[(y+y1)*ow+(x+x1)]<128){
			index=SBMP_INDEX(buf,x,y);
			//char *dp=&buf;
			//fprintf(stderr,"index in %ld,%p\n",index,dp);
			//getchar();
			SBMP_SETPIXEL(buf,index);
		}
	}
	//fprintf(stderr,"\n");
	}
	//fprintf(stderr,"\n\n");
	sprintf(nbuf,"0x%hhx.sbmp",c);
	{
		cbuf=sbmp_compress(buf);
		free(buf);
	}//else cbuf=buf;
	//cbuf=buf;
	assert(cbuf);
	fd=open(nbuf,O_WRONLY|O_CREAT,S_IRUSR|S_IWUSR);
	assert(fd>=0);
	//fprintf(stdout,"size from %zu\n",sbsize);
	if(sbsize>sbosize_max)sbosize_max=sbsize;
	pwrite(fd,cbuf,sbsize=cbuf->size+sizeof(struct sbmp),0);
	//fprintf(stdout,"size to %zu\n",sbsize);
	if(sbsize>sbsize_max)sbsize_max=sbsize;
	ftruncate(fd,sbsize);
	close(fd);
	free(cbuf);
}
int main(int argc,char **argv){
	int32_t *x2s=malloc((argc-1)*sizeof(int32_t));
	int32_t *x1s=malloc((argc-1)*sizeof(int32_t));
	int *fds=malloc((argc-1)*sizeof(int));
	//int32_t *x3s=malloc((argc-1)*sizeof(int32_t));
	
	unsigned char **datas=malloc((argc-1)*sizeof(char *));
	int32_t y1=INT32_MAX,y2=0;
	++argv;--argc;
	for(int i=0;i<argc;++i){
		fds[i]=open(argv[i],O_RDONLY);
		if(fds[i]<0)continue;
		assert(readall(fds[i],(void **)datas+i)>0);
		x1s[i]=0;
		x2s[i]=*(int32_t *)(datas[i]+18);
		//fprintf(stdout,"reading %s\n",argv[i]);
		fprintf(stderr," assimilating %s ",argv[i]);
		correct(datas[i],x1s+i,&y1,x2s+i,&y2);
		fprintf(stderr,"to W:%d H:%d\n",x2s[i]-x1s[i],y2-y1);
		//if(i==argc-1){free(x1s);return 0;}
	}
	//free(x3s);
	fprintf(stderr,"\n");
	for(int i=0;i<argc;++i){
		if(fds[i]<0)continue;
		
		fprintf(stderr," cuting %s...",argv[i]);
		cut(argv[i],datas[i],x1s[i],y1,x2s[i],y2);
		fprintf(stderr,"ok\n");
		close(fds[i]);
		free(datas[i]);
		
	}
	fprintf(stderr,"\n");
	free(fds);
	free(x1s);
	free(x2s);
	free(datas);
	fprintf(stdout,"#define TEXT_MAXSIZE %zu\n"
		"#define TEXT_MAXOSIZE %zu\n"
		"#define TEXT_COUNT %d\n"
		"#define TEXT_HEIGHT %d\n",sbsize_max,sbosize_max,argc-1,y2-y1);
	return 0;
}
