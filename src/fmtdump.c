/*******************************************************************************
 *License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>*
 *This is free software: you are free to change and redistribute it.           *
 *******************************************************************************/
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#define EXPR_BLOCKWARNING 1
#include "expr.h"
void wdaprint(const struct expr_finst *restrict fip){
#define wdashow(field,fmt,cast) \
	switch(fip->read_##field){\
		case 0:\
			printf(fmt,(cast)fip->flag->field);\
			break;\
		case 1:\
			printf("*");\
			break;\
		case 2:\
			printf("**");\
			break;\
		case 3:\
			printf("?");\
			break;\
		default:\
			abort();\
	}
#define flshow(field,ch) if(fip->flag->field)printf("%c",(int)(ch))
	flshow(plus,'+');
	flshow(space,' ');
	flshow(zero,'0');
	flshow(minus,'-');
	flshow(sharp,'#');
	flshow(eq,'=');
	flshow(cap,'!');
	flshow(saved,'?');
	flshow(addr,'&');
	printf("|");
	// make the space visible easily
	wdashow(width,"%zu",size_t);
	printf(".");
	wdashow(digit,"%zu",size_t);
	printf(":");
	wdashow(argsize,"%zu",size_t);
}
void dump(const struct expr_finst *restrict fip){
	for(;;++fip){
		switch(fip->op){
			case EXPR_FIMM:
				fflush(stdout);
				expr_printf("immstr%.1s: \"%.*S\"\n",fip->shouldfree?"*":"",fip->imm_len,fip->imm_str);
				//printf("immstr%s: \"",fip->shouldfree?"*":"");
				//fwrite(fip->imm_str,1,fip->imm_len,stdout);
				// "%.*s" does not work properly in libc printf on
				// this mobile device(but properly on computer),
				// for the imm_str may be not terminated by 0
				// see bugs/libc-printf.c
				// ---
				// CLOSED
				// it is not a bug. in fact,it was caused by the author attempted to pass the length of the string with "%*s". but the behavior of passing a string with no terminating zero to "%.*s" is still undefined in C standard, so here will keep using fwrite.
				//printf("\"\n");
				break;
			case EXPR_FCON:
				printf("conv: %%");
				wdaprint(fip);
				printf(" %c\n",(int)fip->flag->op);
				break;
			case EXPR_FCOL:
				printf("conv(l): %%");
				wdaprint(fip);
				printf(" %c\n",(int)fip->flag->op);
				break;
			case EXPR_FCOA:
				printf("conv(a): %%");
				wdaprint(fip);
				printf(" %c\n",(int)fip->flag->op);
				break;
			case EXPR_FCOZ:
				printf("conv(z): %%");
				wdaprint(fip);
				printf(" %c\n",(int)fip->flag->op);
				break;
			case EXPR_FCTL:
				printf("control: %%");
				wdaprint(fip);
				printf(" %c\n",(int)fip->flag->op);
				break;
			case EXPR_FEND:
				printf("end\n");
				fflush(stdout);
				return;
			default:
				abort();
				// not necessary to use __builtin_unreachable()
				// for here is not in libexpr.
		}
	}
}
#define CCODE(code) case code:return #code
const char *strccode(int fmtccode){
	switch(fmtccode){
		CCODE(EXPR_FMTC_EXIT);
		CCODE(EXPR_FMTC_WRITESIZE);
		CCODE(EXPR_FMTC_NEXTSAVED);
		CCODE(EXPR_FMTC_JUMP_INDEX);
		CCODE(EXPR_FMTC_LOOP);
		CCODE(EXPR_FMTC_LOOP_FORWARD);
		CCODE(EXPR_FMTC_DELIM);
		CCODE(EXPR_FMTC_TAIL);
		CCODE(EXPR_FMTC_SETARR);
		CCODE(EXPR_FMTC_EXITIF);
		CCODE(EXPR_FMTC_READ_FMTC);
		CCODE(EXPR_FMTC_JUMP);
		CCODE(EXPR_FMTC_READ_CONVERTER);
		CCODE(EXPR_FMTC_INC);
		CCODE(EXPR_FMTC_DEC);
		CCODE(EXPR_FMTC_PUSH);
		CCODE(EXPR_FMTC_CLEAR);
		CCODE(EXPR_FMTC_SAVE);
		CCODE(EXPR_FMTC_EVAL);
		CCODE(EXPR_FMTC_FLUSH);
		CCODE(EXPR_FMTC_FPH);
		default:
			abort();
	}
}
const char *strtype(int type){
	switch(type){
		CCODE(EXPR_FLAGTYPE_ADDR);
		CCODE(EXPR_FLAGTYPE_EXPR_T);
		CCODE(EXPR_FLAGTYPE_SIGNED);
		CCODE(EXPR_FLAGTYPE_UNSIGNED);
		CCODE(EXPR_FLAGTYPE_SSIZE);
		CCODE(EXPR_FLAGTYPE_SIZE);
		CCODE(EXPR_FLAGTYPE_DOUBLE);
		default:
			abort();
	}
}
#define REGCONV(C) if(conv==C)return #C
const char *strconv(expr_converter_t conv){
	REGCONV(expr_converter_d);
	REGCONV(expr_converter_size);
	REGCONV(expr_converter_x81);
	REGCONV(expr_converter_x82);
	REGCONV(expr_converter_x83);
	REGCONV(expr_converter_x84);
	REGCONV(expr_converter_x85);
	REGCONV(expr_converter_x86);
	REGCONV(expr_converter_x87);
	REGCONV(expr_converter_x88);
	REGCONV(expr_converter_x89);
	REGCONV(expr_converter_x8a);
	REGCONV(expr_converter_x8b);
	REGCONV(expr_converter_x8c);
	REGCONV(expr_converter_x8d);
	REGCONV(expr_converter_x8e);
	REGCONV(expr_converter_x8f);
	REGCONV(expr_converter_xa1);
	REGCONV(expr_converter_xa2);
	REGCONV(expr_converter_xa3);
	REGCONV(expr_converter_xa4);
	REGCONV(expr_converter_xa5);
	REGCONV(expr_converter_xa6);
	REGCONV(expr_converter_xa7);
	REGCONV(expr_converter_xa8);
	REGCONV(expr_converter_xa9);
	REGCONV(expr_converter_xaa);
	REGCONV(expr_converter_xab);
	REGCONV(expr_converter_xac);
	REGCONV(expr_converter_xad);
	REGCONV(expr_converter_xae);
	REGCONV(expr_converter_xaf);
	REGCONV(expr_converter_fe);
	REGCONV(expr_converter_g);
	REGCONV(expr_converter_s);
	REGCONV(expr_converter_immbinary);
	REGCONV(expr_converter_h);
	REGCONV(expr_converter_escape);
	REGCONV(expr_converter_c);
	REGCONV(expr_converter_p);
	abort();
}
int plain=0,add_index=0;
#define TEMPLATE_IMM \
(!plain?\
"\t\t.op=EXPR_FIMM,\n"\
"\t\t.imm_str=%#.*E,\n"\
"\t\t.imm_len=%=:zu,\n":\
".op=EXPR_FIMM,"\
".imm_str=%#.*E,"\
".imm_len=%=:zu,"\
)
#define plain0(s) (plain?s:"" s "\n")
#define plain1(s) (plain?s:"\t" s "\n")
#define plain2(s) (plain?s:"\t\t" s "\n")
#define plain3(s) (plain?s:"\t\t\t" s "\n")
#define DUMPF(fl) if(flag->fl)expr_printf(plain3("." #fl "=%:zu,"),(size_t)flag->fl)
#define type_check(type,T) \
	if(expr_ftable_default->bitwidth_##type##_set&&flag->asop==expr_ftable_default->bitwidth_##type){\
		expr_printf(plain3(".argsize=sizeof(" #T "),"));\
	}
#define DUMPM(m) if(fip->m)expr_printf(plain2("." #m "=%:iu,"),(unsigned int)fip->m)
const char *varname;
void dumpflag(const struct expr_writeflag_nomtl *flagp){
	struct expr_writeflag_nomtl flag[1];
	memcpy(flag,flagp,sizeof(struct expr_writeflag_nomtl));
	flag->op=0;
	if(!expr_internal_memtst(flag,sizeof(struct expr_writeflag_nomtl)))
		return;
	expr_printf(plain2(".flag={{"));
	if(flag->type)
		expr_printf(plain3(".type=%s,"),strtype((int)flag->type));
	DUMPF(plus);
	DUMPF(space);
	DUMPF(zero);
	DUMPF(minus);
	DUMPF(sharp);
	DUMPF(eq);
	DUMPF(cap);
	DUMPF(saved);
	DUMPF(digit_set);
	DUMPF(width_set);
	DUMPF(addr);
	DUMPF(suffix);
	DUMPF(width);
	DUMPF(digit);
	type_check(addr,void *)
	else type_check(size,size_t)
	else type_check(char,char)
	else type_check(short,short)
	else type_check(int,int)
	else type_check(long,long)
	else type_check(llong,long long)
	else {
		DUMPF(argsize);
	}
	expr_printf(plain2("}},"));
}
void dumpcon(const struct expr_finst *restrict fip,const char *op){
	expr_printf(plain2(".op=%s,"),op);
	expr_printf(plain2(".converter=%s,"),strconv(fip->converter));
	dumpflag(fip->flag);
	DUMPM(read_width);
	DUMPM(read_digit);
	DUMPM(read_argsize);
}
void dumpctl(const struct expr_finst *restrict fip){
	expr_printf(plain2(".op=EXPR_FCTL,"));
	expr_printf(plain2(".fmtc_code=%s,"),strccode(fip->fmtc_code));
	switch(fip->fmtc_code){
		case EXPR_FMTC_DELIM:
		case EXPR_FMTC_TAIL:
			expr_printf(plain2(".imm_str=%#.*E,"),fip->flag->width,fip->imm_str);
			break;
		case EXPR_FMTC_FPH:
			expr_printf(plain2(".fph=\'%.1E\',"),&fip->fph);
			break;
		default:
			break;
	}
	dumpflag(fip->flag);
	DUMPM(read_width);
	DUMPM(read_digit);
	DUMPM(read_argsize);
}
void dump_c(const struct expr_finst *restrict fip){
	const struct expr_finst *fip0=fip;
	if(varname)
		expr_printf(plain0("const struct expr_finst %s[]={"),varname);
	else
		expr_printf("%s",plain0("{"));
	for(;;++fip){
		if(add_index)
			expr_printf(plain1("[%=:zd]={"),(ssize_t)(fip-fip0));
		else
			expr_printf(plain1("{"));
		switch(fip->op){
			case EXPR_FIMM:
				expr_printf(TEMPLATE_IMM,fip->imm_len,fip->imm_str,fip->imm_len);
				break;
			case EXPR_FCON:
				dumpcon(fip,"EXPR_FCON");
				break;
			case EXPR_FCOL:
				dumpcon(fip,"EXPR_FCOL");
				break;
			case EXPR_FCOA:
				dumpcon(fip,"EXPR_FCOA");
				break;
			case EXPR_FCOZ:
				dumpcon(fip,"EXPR_FCOZ");
				break;
			case EXPR_FCTL:
				dumpctl(fip);
				break;
			case EXPR_FEND:
				expr_printf("%s",!plain?
					"\t\t.op=EXPR_FEND,\n"
					"\t}\n":
					".op=EXPR_FEND,"
					"}");
				expr_printf("%s",varname?"};\n":"}\n");
				return;
			default:
				abort();
		}
		expr_printf(plain1("},"));
	}
}
const struct option ops[]={
	{"index",0,NULL,'i'},
	{"no-optimize",0,NULL,'n'},
	{"plain",0,NULL,'p'},
	{"style-c",0,NULL,'c'},
	{"varname",0,NULL,'v'},
	{"help",0,NULL,'h'},
	{NULL,0,NULL,0},
};
void show_help(const char *a0){
	fprintf(stdout,"usage: %s [options] format\n"
			"\t--index, -i\tadd index to each instruments in --style-c\n"
			"\t--no-optimize, -n\tdo not optimize\n"
			"\t--plain, -p\tremove the tab and nextline in --style-c\n"
			"\t--style-c, -c\tdump in C style\n"
			"\t--help, -h\tshow this help\n"
			"version: %s\n"
			,a0,expr_libinfo_current->version);
	exit(EXIT_SUCCESS);
}
int main(int argc,char **argv){
	char *e=NULL;
	struct expr_finst *fip;
	void (*style)(const struct expr_finst *restrict)=dump;
	ssize_t r;
	int flag=0;
	if(argc<2)
		show_help(*argv);
	opterr=1;
	for(;;){
		switch(getopt_long(argc,argv,"nchv:pi",ops,NULL)){
			case 'n':
				flag|=EXPR_INOOPTIMIZE;
				break;
			case 'c':
				style=dump_c;
				break;
			case 'v':
				varname=optarg;
				break;
			case 'p':
				plain=1;
				break;
			case 'i':
				add_index=1;
				break;
			case 'h':
				show_help(*argv);
				break;
			case -1:
				goto break2;
			case '?':
				exit(EXIT_FAILURE);
				break;
		}
	}
break2:
	if(optind>=argc){
		fprintf(stderr,"format not found\n");
		return EXIT_FAILURE;
	}
	e=argv[optind];
	r=expr_mkinstf(&fip,e,strlen(e),flag);
	if(r<0){
		fprintf(stderr,"mkinst(): %zd\n",r);
		return EXIT_FAILURE;
	}
	fflush(stdout);
	style(fip);
	expr_ifree(fip);
	return EXIT_SUCCESS;
}
