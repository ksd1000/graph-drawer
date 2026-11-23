#undef _GNU_SOURCE
#undef __GNUC__
#define EXPR_NOSYSCALL 1
#define restrict
#include "expr.h"
#include <stdio.h>
#include <stdlib.h>
expr_globals_define();
struct test_list {
	void *data;
	expr_list_t n;
};
int main(void){
	struct test_list l[1];
	struct test_list l1[1];
	struct test_list *p;
	expr_list_poison(l->n);
	expr_list_zero(l->n);
	expr_list_ring(l->n);
	expr_list_rinsert_next(l->n,l1->n);
	p=expr_host(l1->n,struct test_list,n);
	expr_list_detach(p->n);
	printf("header test in non-GNU mode ok.\n");
}
