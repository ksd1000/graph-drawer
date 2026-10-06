/*******************************************************************************
 *License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>*
 *This is free software: you are free to change and redistribute it.           *
 *******************************************************************************/
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include "expr.h"
#include <math.h>
#include <string.h>
#include <assert.h>
#ifdef __unix__
#define REAL_UNIX
#endif
#include "fake_unix.h"
#define psize(s) printf("sizeof(" #s ")=%zu\n",sizeof(s))
const char *t2s[]={
	[EXPR_CONSTANT]="Constant",
	[EXPR_VARIABLE]="Variable",
	[EXPR_FUNCTION]="Function",
	[EXPR_MDFUNCTION]="Multi-dimension function",
	[EXPR_MDEPFUNCTION]="Multi-dimension function*",
	[EXPR_HOTFUNCTION]="Hot function",
	[EXPR_ZAFUNCTION]="Zero-argument function"
};
const char *aflag(int type,int flag,size_t dim){
	static char abuf[64];
	static char abuf1[64+16+20];
	*abuf=0;
		switch(type){
			case EXPR_CONSTANT:
			case EXPR_VARIABLE:
				return abuf;
			case EXPR_FUNCTION:
			case EXPR_ZAFUNCTION:
			case EXPR_HOTFUNCTION:
				if(flag&EXPR_SPURE)
					strcat(abuf,"I");
				if(flag&EXPR_SUNSAFE)
					strcat(abuf,"U");
				return abuf;
			case EXPR_MDFUNCTION:
			case EXPR_MDEPFUNCTION:
				if(flag&EXPR_SPURE)
					strcat(abuf,"I");
				if(flag&EXPR_SUNSAFE)
					strcat(abuf,"U");
				if(dim)
					sprintf(abuf1,"%-2s %zu",abuf,dim);
				else
					sprintf(abuf1,"%-2s no limit",abuf);
				return abuf1;
			default:
				abort();

		}
}
void psymbol(int type,int flag,size_t dim,const char *str,const union expr_symvalue *un){
		printf("%-28s",str);
		switch(type){
			case EXPR_CONSTANT:
				printf("%-16lg",un->value);
				break;
			case EXPR_VARIABLE:
				printf("%-16lg",*(double *)un->addr);
				break;
			case EXPR_FUNCTION:
			case EXPR_MDFUNCTION:
			case EXPR_MDEPFUNCTION:
			case EXPR_ZAFUNCTION:
			case EXPR_HOTFUNCTION:
				printf("%-16s",aflag(type,flag,dim));
				break;
			default:
				abort();

		}
		printf("%s\n",t2s[type]);
}
void list(void){
	psize(struct expr);
	psize(struct expr_inst);
	psize(struct expr_suminfo);
	psize(struct expr_mdinfo);
	psize(struct expr_vmdinfo);
	psize(struct expr_branchinfo);
	psize(struct expr_symset);
	psize(struct expr_symbol);
	psize(struct expr_builtin_symbol);
	psize(struct expr_builtin_keyword);
	psize(struct expr_resource);
	for(const struct expr_builtin_keyword *p=expr_keywords;;++p){
		if(!p->str){
			printf("%zu keywords\n",p-expr_keywords);
			break;
		}
		printf("%-12s\tKeyword %s%s%s\t%s\n",p->str,p->flag&EXPR_KSUBEXPR?"S":" ",p->flag&EXPR_KSEPCOMMA?"C":" ",p->flag&EXPR_KNOPROTECT?"N":" ",p->desc);
	}
	printf("\n");
	return;
}
void list_symbol(struct expr_symbol *p){
	size_t dim;
	switch(p->type){
		case EXPR_CONSTANT:
		case EXPR_VARIABLE:
		case EXPR_FUNCTION:
		case EXPR_ZAFUNCTION:
			psymbol(p->type,p->flag,0,p->str,expr_symbol_un(p));
			break;
		case EXPR_MDFUNCTION:
		case EXPR_MDEPFUNCTION:
			dim=*expr_symbol_dim(p);
			psymbol(p->type,p->flag,dim,p->str,expr_symbol_un(p));
			break;
		default:
			abort();

	}
}
void add_all_common_symbols(struct expr_symset *);
int adbt=0;
void list_common(void){
	size_t n=0;
	struct expr_symset es[1]={EXPR_SYMSET_INITIALIZER(expr_defmtl)};
	add_all_common_symbols(es);
	if(adbt)
		expr_builtin_symbol_addalls(es,expr_symbols_all);
//	expr_symset_callback(es,list_symbol,NULL);
//	expr_symset_callbacks(es,list_symbol1);
	expr_symset_foreach4(sp,es,__builtin_alloca(es->depth*EXPR_SYMSET_DEPTHUNIT),EXPR_SYMNEXT){
		list_symbol(sp);
		++n;
	}
	assert(n==es->size);
	assert(expr_symset_depth(es)==es->depth);
	assert(expr_symset_size(es)==es->size);
	assert(expr_symset_length(es)==es->length);
	printf("%zu common symbols,depth=%zu\n",n,es->depth);
	//printf("size:%zu,size_m:%zu,length:%zu,length_m:%zu,removed:%zu,removed_m:%zu,depth:%zu,depth_n:%zu,depth_nm:%zu,\n",es->size,es->size_m,es->length,es->length_m,es->removed,es->removed_m,es->depth,es->depth_n,es->depth_nm);
	expr_symset_remove(es,"x0",2);
	//printf("size:%zu,size_m:%zu,length:%zu,length_m:%zu,removed:%zu,removed_m:%zu,depth:%zu,depth_n:%zu,depth_nm:%zu,\n",es->size,es->size_m,es->length,es->length_m,es->removed,es->removed_m,es->depth,es->depth_n,es->depth_nm);
	expr_symset_correct(es);
	//printf("size:%zu,size_m:%zu,length:%zu,length_m:%zu,removed:%zu,removed_m:%zu,depth:%zu,depth_n:%zu,depth_nm:%zu,\n",es->size,es->size_m,es->length,es->length_m,es->removed,es->removed_m,es->depth,es->depth_n,es->depth_nm);
	expr_symset_free(es);
}
int main(int argc,char **argv){
//	expr_symset_allow_heap_stack=1;
	if(argc<2){
		list();
	}else {
		expr_setup_mtl(2);
		if(!strcmp(argv[1],"b"))
			adbt=1;
		list_common();
		expr_setup_mtl(0);
	}
	//printf("allocated:%d,freed:%d\n",allocated,freed);
	return EXIT_SUCCESS;
}
