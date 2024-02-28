#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "xgraph/header/expr.h"
#include <time.h>
const char *t2s[]={
	[EXPR_CONSTANT]="Constant",
	[EXPR_VARIABLE]="Variable",
	[EXPR_FUNCTION]="Function",
	[EXPR_MDFUNCTION]="Multi-dimension function",
	[EXPR_MDEPFUNCTION]="Multi-dimension function*"
};
int main(int c,char **argv){
	for(const struct expr_builtin_keyword *p=expr_keywords;p->str;++p){
		printf("%-12s\tKeyword \t%s(%s)\n",p->str,p->str,p->desc);
	}
	printf("\n");
	for(const struct expr_builtin_symbol *p=expr_bsyms;p->str;++p){
		printf("%-12s\t%-30s",p->str,t2s[p->type]);
		switch(p->type){
			case EXPR_CONSTANT:
				printf("value: %e",p->un.value);
				break;
			case EXPR_VARIABLE:
				printf("value: %e",*(double *)p->un.addr);
				break;
			case EXPR_FUNCTION:
				printf("f(1.0): %e",p->un.func(1));
				break;
			case EXPR_MDFUNCTION:
			case EXPR_MDEPFUNCTION:
				if(p->dim)
				printf("dimension: %u",p->dim);
				else
				printf("dimension: no limit");
				break;

		}
		printf("\n");
	}
	return 0;
}
