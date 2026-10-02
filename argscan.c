
/*******************************************************************************
 *License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>*
 *This is free software: you are free to change and redistribute it.           *
 *******************************************************************************/
//#define _GNU_SOURCE
#include <stdlib.h>
#include <stdint.h>
struct argtype {
	const char *name,*alias,*desc;
	union {
		void *uval;
		char **sval;
		int64_t *lval;
		int32_t *ival;
		double *dval;
		int (*callback)(char *);
		int (*callbackz)(void);
	} un;
	int type,period;
};
enum :int{
AT_INT,
AT_LONG,
AT_DOUBLE,
AT_BOOL,
AT_BOOLX,
AT_BOOLA,
AT_STR,
AT_CALL,
AT_CALLZ,
};
static int atolodx(const char *p,long *out);
static int atod2(const char *str,double *dst);
static int ashandle(const struct argtype *at,char ***argv,char **endv,int period){
	union {
		long r;
		double d;
	} un;
	switch(at->type){
		case AT_INT:
			if(at->period==period){
				if(*argv+1>=endv){
					warnx("no argument \"%s\"",**argv);
					return -1;
				}
				if(atolodx((*argv)[1],&un.r)<0){
					warnx("invaild number \"%s\"",(*argv)[1]);
					return -1;
				}
				*at->un.ival=(int)un.r;
			}
			*argv+=2;
			return 0;
		case AT_LONG:
			if(at->period==period){
				if(*argv+1>=endv){
					warnx("no argument \"%s\"",**argv);
					return -1;
				}
				if(atolodx((*argv)[1],&un.r)<0){
					warnx("invaild number \"%s\"",(*argv)[1]);
					return -1;
				}
				*at->un.ival=un.r;
			}
			*argv+=2;
			return 0;
		case AT_DOUBLE:
			if(at->period==period){
				if(*argv+1>=endv){
					warnx("no argument \"%s\"",**argv);
					return -1;
				}
				if(atod2((*argv)[1],&un.d)<0){
					warnx("invaild float \"%s\"",(*argv)[1]);
					return -1;
				}
				*at->un.dval=un.d;
			}
			*argv+=2;
			return 0;
		case AT_BOOL:
			if(at->period==period)
				*at->un.ival=1;
			++(*argv);
			return 0;
		case AT_BOOLX:
			if(at->period==period)
				*at->un.ival^=1;
			++(*argv);
			return 0;
		case AT_BOOLA:
			if(at->period==period)
				*at->un.ival+=1;
			++(*argv);
			return 0;
		case AT_STR:
			if(at->period==period){
				if(*argv+1>=endv){
					warnx("no argument \"%s\"",**argv);
					return -1;
				}
				*at->un.sval=(*argv)[1];
			}
			*argv+=2;
			return 0;
		case AT_CALL:
			if(at->period==period){
				if(*argv+1>=endv){
					warnx("no argument \"%s\"",**argv);
					return -1;
				}
				if(at->un.callback((*argv)[1])<0)
					return -1;
			}
			*argv+=2;
			return 0;
		case AT_CALLZ:
			if(at->period==period){
				if(at->un.callbackz()<0)
					return -1;
			}
			++(*argv);
			return 0;
		default:
			return -1;
	}
}
static int atolodx(const char *p,long *out){
	char *end;
	int base;
	switch(*p){
		case '0':
			base=(p[1]=='x'?16:8);
			break;
		default:
			base=10;
			break;
	}
	*out=strtol(p,&end,base);
	if(*end)
		return -1;
	return 0;
}
static int atod2(const char *str,double *dst){
	char *c;
	*dst=strtod(str,&c);
	if(c==str)
		return -1;
	else if(*c)
		return -2;
	else return 0;
}
static const struct argtype *asfind(const char *a,const struct argtype *at){
	size_t len=strlen(a);
	for(;at->name||at->alias;++at){
		if(!at->name)
			continue;
		if(strlen(at->name)!=len)
			continue;
		if(memcmp(a,at->name,len))
			continue;
		return at;
	}
	return NULL;
}
static const struct argtype *asfind_a(const char *a,const struct argtype *at){
	size_t len=strlen(a);
	for(;at->name||at->alias;++at){
		if(!at->alias)
			continue;
		if(strlen(at->alias)!=len)
			continue;
		if(memcmp(a,at->alias,len))
			continue;
		return at;
	}
	return NULL;
}
int argscan(int argc,char **argv,int period,const struct argtype *at,int (*common)(char *)){
	char **endv=argv+argc;
	const struct argtype *atp;
	for(;argv<endv;){
		if(**argv!='-'){
ascommon:
			if(common){
				if(common(*argv)<0)
					return -1;
			}
			++argv;
			continue;
		}
		if((*argv)[1]!='-'){
			atp=asfind_a((*argv)+1,at);
			goto get;
		}else if(!(*argv)[2]&&argv+1<endv){
			++argv;
			goto ascommon;
		}else {
			atp=asfind((*argv)+2,at);
			goto get;
		}
get:
		if(!atp){
			warnx("unknown option \"%s\"",*argv);
			return -1;
		}
		if(ashandle(atp,&argv,endv,period)<0)
			return -1;
	}
	return 0;
}
