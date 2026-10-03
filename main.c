/*******************************************************************************
 *License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>*
 *This is free software: you are free to change and redistribute it.           *
 *******************************************************************************/
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#include <float.h>
#include "graph.h"
#define EXPR_BLOCKWARNING 1
#include "expr.h"
#ifdef __unix__
#define REAL_UNIX
#endif
#include "fake_unix.h"
//argscan
#include "argscan.c"
//argscan end
#define SIZE 16
#define RATIO 4096
#define BUFSIZE (1024*1024)
#pragma GCC diagnostic ignored "-Wunused-result"
#define outstring(str) write(STDERR_FILENO,str,sizeof(str)-1)
void add_common_symbols(struct expr_symset *es);
//#define free(v)
void *readall(intptr_t fd,size_t *len);
double evalxy(double input,void *arg){
	return expr_eval((const struct expr *)arg,input);
}
void graph_drawep_mt(struct graph *restrict gp,uint32_t color,int32_t bold,const struct expr *xeps,const struct expr *yeps,double from,double to,double step,volatile double *currents,int thread){
	graph_draw_mt(gp,color,bold,evalxy,evalxy,(void *)xeps,sizeof(struct expr),(void *)yeps,sizeof(struct expr),from,to,step,currents,thread);
}
volatile double *currents;
volatile double n1=1.6;
int32_t bold=2,fbold=1;
int thread=1,ioret=0;
int32_t width=RATIO,height=RATIO;
unsigned int barlen,textline=0;
uint32_t color=0xff0000;
struct graph g;
double step=0.0;
struct expr *xeps,*yeps;
struct fake_winsize {
	int ws_row,ws_col;
} wsize={128,64};
struct expr_symset es[1];
double maxy=SIZE,miny=-SIZE,maxx=SIZE,minx=-SIZE,from=-SIZE,to=SIZE,gapx=1.0,gapy=1.0;
char *bar,*wbuf;
const char *ex="t",*para="t";
double draw_connect(double *args,size_t n){
	graph_connect(&g,color,0,args[0],args[1],args[2],args[3]);
	return NAN;
}
void *drawing(void *args){
	graph_drawep_mt(&g,color,fbold,xeps,yeps,from,to,step,currents,thread);
	//graph_drawep(&g,color,fbold,xep,yep,from,to,step,currents);
	return NULL;
}
int protect=0;
int drawat(char *ey){
	int srate,lrate,mrate,pc,mpc;
	pthread_t pt;
	double gap=(to-from)/thread;
	char *wbuf0,ei[EXPR_SYMLEN];
	int flag=protect?EXPR_IF_PROTECT:0;
	xeps=expr_new7(ex,para,es,flag,thread,&pc,ei);
	if(!xeps)
		errx(EXIT_FAILURE,"x expression error:%s (%s)",expr_error(pc),ei);
	yeps=expr_new7(ey,para,es,flag,thread,&pc,ei);
	if(!yeps)
		errx(EXIT_FAILURE,"y expression error:%s (%s)",expr_error(pc),ei);
	for(int i=0;i<thread;++i){
		currents[i]=DBL_MIN;
	}
	//g.arrow=1;
	//g.draw_value=1;
	mpc=0;
	bar[0]='[';
	bar[barlen+1]=']';
	bar[barlen+2]=0;
	write(STDERR_FILENO,wbuf,sprintf(wbuf,"drawing x=%s y=%s\n",ex,ey));
	srate=g.height/64;
	for(;;){
		lrate=graph_textlen(&g,wbuf+7,4,srate)*2;
		if(lrate>g.width)srate=srate*g.width/lrate;
		else
			break;
	}
	graph_draw_text_pixel(&g,color,0,wbuf+7,4,srate,0,textline);
	textline+=srate;
	wbuf0=wbuf+sprintf(wbuf,"\033[%dA",wsize.ws_row<=thread+3?3:thread+3);
	lrate=-1;
	mrate=thread*10000;
	if(pthread_create(&pt,NULL,drawing,NULL)<0)
			err(EXIT_FAILURE,"cannot create thread");
	for(;;){
		char *p=wbuf0;
		int rate;
		if(mpc==10000)break;
		else mpc=10000;
		srate=0;
		for(int i=0;i<thread;++i){
			double s;
			s=from+i*gap;
			if(currents[i]==DBL_MAX)
				pc=10000;
			else 
				if(currents[i]==DBL_MIN)pc=0;
			else
				pc=(int)((currents[i]-s)*10000/gap);
			rate=barlen*pc/10000;
			srate+=pc;
			if(wsize.ws_row>thread+3){
			for(int j=0;j<barlen;++j){
				if(j>rate)bar[j+1]='-';
				else if(j<rate)bar[j+1]='=';
				else bar[j+1]='>';
			}
			p+=sprintf(p,"thread %d\t%s[%3d.%2d%%]\n",i,bar,pc/100,pc%100);
			}

			if(pc<mpc)
				mpc=pc;
		}
		if(srate>lrate){
			rate=barlen*srate/mrate;
			for(int j=0;j<barlen;++j){
				if(j>rate)
					bar[j+1]='-';
				else if(j<rate)bar[j+1]='=';
				else
					bar[j+1]='>';
			}
			pc=srate/thread;
			p+=sprintf(p,"\nsummary\t%s[%3d.%2d%%]\n\n",bar,pc/100,pc%100);
			if(lrate!=-1){
				write(STDERR_FILENO,wbuf,p-wbuf);
			}else {
				write(STDERR_FILENO,wbuf0,p-wbuf0);
			}
			lrate=srate;
		}
		//if(srate==lrate)usleep(sleep_gap);
	}
	//write(STDERR_FILENO,"\033[?25h",6);
	if(pthread_join(pt,NULL)<0)
		err(EXIT_FAILURE,"pthread_join");
	expr_free(xeps);
	expr_free(yeps);
	return 0;
}
void writefile(const char *file,const void *buf,size_t sz){
	intptr_t tofd;
	if(!strcmp(file,"-")){
		tofd=STDOUT_FILENO;
	}else {
		char *p=strrchr(file,'.');
		if(!p||strcmp(p,".bmp"))
			warnx("unsupposed format \"%s\". writing .bmp format as default",p?p:file);
		tofd=open(file,O_WRONLY_CREAT);
		if(fderr(tofd))
			err(EXIT_FAILURE,"cannot open %s",file);
	}
	write(tofd,buf,sz);
	close(tofd);
}
int a_nv(void){
	wsize.ws_row=3;
	return 0;
}
int a_ratio(char *cnt){
	if(sscanf(cnt,"%dx%d",&width,&height)<2)
		errx(EXIT_FAILURE,"invaild ratio");
	return 0;
}
int no_connect=0;
char *frombmp=NULL,*frombuf=NULL,*file=NULL;
int printhelp(void);
#define ARG(a1,a2,dst,t,p,d) {.name=a1,.alias=a2,.desc=d,.un={.uval=dst},.type=t,.period=p,}
const struct argtype ats[]={
	ARG("thread","T",&thread,AT_INT,0,"count -- how many threads used"),
	ARG("ratio",NULL,a_ratio,AT_CALL,0,"WxH -- ratio of bitmap"),
	ARG("frombmp",NULL,&frombmp,AT_STR,0,"file -- use a bitmap as background"),
	ARG(NULL,"x",&ex,AT_STR,1,"expression -- set the expression for x,default:x(t)=t"),
	ARG("no-connect",NULL,&no_connect,AT_BOOLX,0,"do not connect discontinuities"),
	ARG("bold","b",&bold,AT_INT,0,"bold -- bold of the axis"),
	ARG("fbold","B",&fbold,AT_INT,1,"fbold -- bold of the function"),
	ARG("minx",NULL,&minx,AT_DOUBLE,0,"x -- minimal x of the graph"),
	ARG("maxx",NULL,&maxx,AT_DOUBLE,0,"x -- maximal x of the graph"),
	ARG("miny",NULL,&miny,AT_DOUBLE,0,"y -- minimal y of the graph"),
	ARG("maxy",NULL,&maxy,AT_DOUBLE,0,"y -- maximal y of the graph"),
	ARG("gapx",NULL,&gapx,AT_DOUBLE,0,"gap -- gap of x asix of the graph"),
	ARG("gapy",NULL,&gapy,AT_DOUBLE,0,"gap -- gap of y asix of the graph"),
	ARG(NULL,"nv",a_nv,AT_CALLZ,0,"hide the drawing progress of each threads"),
	ARG("output","o",&file,AT_STR,0,"file -- output file, \"-\" for stdout"),
	ARG("from",NULL,&from,AT_DOUBLE,1,"t -- minimal t of the graph"),
	ARG("to",NULL,&to,AT_DOUBLE,1,"t -- minimal t of the graph"),
	ARG("step",NULL,&step,AT_DOUBLE,1,"step -- step of t"),
	ARG("color",NULL,&color,AT_INT,1,"color -- color of the functions"),
	ARG("safe","p",&protect,AT_BOOL,1,"work in protected mode"),
	ARG("help","h",printhelp,AT_CALLZ,0,"show this help"),
	ARG(NULL,NULL,NULL,0,0,NULL)
//	ARG("",,&,AT_,0),
};
int printhelp(void){
	char name[16],alias[16];
	for(const struct argtype *atp=ats;atp->name||atp->alias;++atp){
		if(atp->name){
			sprintf(name,"--%s",atp->name);
		}else {
			*name=0;
		}
		if(atp->alias){
			sprintf(alias,"-%s",atp->alias);
		}else {
			*alias=0;
		}
		fprintf(stderr,"%-12s%-8s%s\n",name,alias,atp->desc);
	}
	exit(EXIT_SUCCESS);
}
int main(int argc,char **argv){
	intptr_t fromfd;
	size_t fromsize=0;
	barlen=wsize.ws_col>28?wsize.ws_col-28:0;
	expr_symset_init(es);
	if(argc<2)
		printhelp();
	if(argscan(argc-1,argv+1,0,ats,NULL)<0)
		exit(EXIT_FAILURE);
	if(!file)
		errx(EXIT_FAILURE,"no output file (-o)");
	if(thread<1)
		thread=1;
	if(thread<2)
		wsize.ws_row=3;
	if(frombmp){
		fromfd=open(frombmp,O_RDONLY);
		if(fderr(fromfd)){
			err(EXIT_FAILURE,"cannot open %s",frombmp);
		}
		frombuf=readall(fromfd,&fromsize);
		if(!frombuf){
			err(EXIT_FAILURE,"cannot read %s",frombmp);
		}
		close(fromfd);
	}
	if(frombuf){
		fromfd=init_graph_frombmp(&g,frombuf,fromsize,minx,maxx,miny,maxy);
	}
	else {
		fromfd=init_graph(&g,width,height,24,minx,maxx,miny,maxy);
	}
	if(fromfd<0)errx(EXIT_FAILURE,"cannot create a graph (%zd)",fromfd);
	if(step<DBL_EPSILON){
		step=graph_pixelstep(&g);
	}
	g.connect=!no_connect;
	if(!frombuf)
		graph_fill(&g,0xffffff);
	outstring("drawing axis...");
	//graph_fill(&g,0xffffff);
	graph_draw_grid(&g,0xbfbfbf,0,gapx/4.0,gapy/4.0);
	graph_draw_grid(&g,0x7f7f7f,bold,gapx,gapy);
	//g.draw_value=0;
	graph_draw_axis(&g,0x000000,bold,gapx,gapy,32*(width+height)/8192);
	outstring("ok\n");
	currents=malloc(thread*sizeof(double));

	wbuf=malloc((thread+1)*(18+barlen)+14);
	bar=malloc(3+barlen);

	expr_builtin_symbol_addalls(es,expr_symbols_all);
	expr_symset_add(es,"draw_connect",EXPR_MDFUNCTION,0,draw_connect,4ul);
	add_common_symbols(es);
	//draw start
	if(argscan(argc-1,argv+1,1,ats,drawat)<0)
		exit(EXIT_FAILURE);
	//draw end
	expr_symset_free(es);

	free((void *)currents);
	free(wbuf);
	free(bar);
	if(frombuf)
		free(frombuf);
	writefile(file,g.bm,g.bm->size);
	warnx("Done");
	return EXIT_SUCCESS;
}
