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
const char *varname;
#define TEMPLATE_IMM \
"\t{\n"\
"\t\t.op=EXPR_FIMM,\n"\
"\t\t.imm_str=%#.*E,\n"\
"\t\t.imm_len=%~=:zu\n"\
"\t},\n"
#define TEMPLATE_CON \
"\t{\n"\
"\t\t.op=%s,\n"\
"\t\t.flag={{\n"\
"\t\t\t.bit=%#~=:8x,\n"\
"\t\t\t.width=%~=:zu,\n"\
"\t\t\t.digit=%~=:zu,\n"\
"\t\t}},\n"\
"\t\t.converter=%s,\n"\
"\t\t.read_width=%:iu,\n"\
"\t\t.read_digit=%:iu,\n"\
"\t\t.read_argsize=%:iu,\n"\
"\t},\n"
#define ARG_CON(OP) \
(#OP)\
,fip->flag->bit\
,fip->flag->width\
,fip->flag->digit\
,strconv(fip->converter)\
,(unsigned int)fip->read_width\
,(unsigned int)fip->read_digit\
,(unsigned int)fip->read_argsize
#define TEMPLATE_CTL(...) \
"\t{\n"\
"\t\t.op=%s,\n"\
"\t\t.flag={{\n"\
"\t\t\t.bit=%#~=:8x,\n"\
"\t\t\t.width=%~=:zu,\n"\
"\t\t\t.digit=%~=:zu,\n"\
"\t\t}},\n"\
"\t\t.fmtc_code=%s,\n"\
__VA_ARGS__\
"\t\t.read_width=%:iu,\n"\
"\t\t.read_digit=%:iu,\n"\
"\t\t.read_argsize=%:iu\n"\
"\t},\n"
#define TEMPLATE_DT \
"\t\t.imm_str=%#.*E,\n"
#define TEMPLATE_FPH \
"\t\t.fph=\'%.1E\',\n"
#define ARG_CTL(...) \
"EXPR_FCTL"\
,fip->flag->bit\
,fip->flag->width\
,fip->flag->digit\
,strccode(fip->fmtc_code)\
,##__VA_ARGS__\
,(unsigned int)fip->read_width\
,(unsigned int)fip->read_digit\
,(unsigned int)fip->read_argsize
void dump_c(const struct expr_finst *restrict fip){
	if(varname)
		expr_printf("const struct expr_finst %s[]={\n",varname);
	else
		expr_printf("%s","{\n");
	for(;;++fip){
		switch(fip->op){
			case EXPR_FIMM:
				expr_printf(TEMPLATE_IMM,fip->imm_len,fip->imm_str,fip->imm_len);
				break;
			case EXPR_FCON:
				expr_printf(TEMPLATE_CON,ARG_CON(EXPR_FCON));
				break;
			case EXPR_FCOL:
				expr_printf(TEMPLATE_CON,ARG_CON(EXPR_FCOL));
				break;
			case EXPR_FCOA:
				expr_printf(TEMPLATE_CON,ARG_CON(EXPR_FCOA));
				break;
			case EXPR_FCOZ:
				expr_printf(TEMPLATE_CON,ARG_CON(EXPR_FCOZ));
				break;
			case EXPR_FCTL:
				switch(fip->fmtc_code){
					case EXPR_FMTC_DELIM:
					case EXPR_FMTC_TAIL:
						expr_printf(TEMPLATE_CTL(TEMPLATE_DT),ARG_CTL(fip->flag->width,fip->imm_str));
						break;
					case EXPR_FMTC_FPH:
						expr_printf(TEMPLATE_CTL(TEMPLATE_FPH),ARG_CTL(&fip->fph));
						break;
					default:
						expr_printf(TEMPLATE_CTL(),ARG_CTL());
						break;
				}
				break;
			case EXPR_FEND:
				expr_printf("%s",
					"\t{\n"
					"\t\t.op=EXPR_FEND,\n"
					"\t}\n");
				expr_printf("%s",varname?"};\n":"}\n");
				return;
			default:
				abort();
				// not necessary to use __builtin_unreachable()
				// for here is not in libexpr.
		}
	}
}
const struct option ops[]={
	{"no-optimize",0,NULL,'n'},
	{"style-c",0,NULL,'c'},
	{"varname",0,NULL,'v'},
	{"help",0,NULL,'h'},
	{NULL,0,NULL,0},
};
void show_help(const char *a0){
	fprintf(stdout,"usage: %s [options] format\n"
			"\t--no-optimize, -n\tdo not optimize\n"
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
		switch(getopt_long(argc,argv,"nchv:",ops,NULL)){
			case 'n':
				flag|=EXPR_INOOPTIMIZE;
				break;
			case 'c':
				style=dump_c;
				break;
			case 'v':
				varname=optarg;
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
