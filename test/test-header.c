#undef _GNU_SOURCE
#define EXPR_NOCHECK_GNU_SOURCE 1
#define EXPR_NOSYSCALL 1
#define restrict
#include "expr.h"
#include <stdio.h>
#include <stdlib.h>
expr_globals_define();
int main(void){
	struct expr_list l[1];
	struct expr_list l1[1];
	expr_list_poison(l);
	expr_list_zero(l);
	expr_list_ring(l);
	expr_list_rinsert_next(l,l1);
	printf("header test in non-GNU mode ok.\n");
}
