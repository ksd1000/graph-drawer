#define EXPR_NOCHECK_GNU_SOURCE 1
#define restrict
#include "expr.h"
#include <stdio.h>

int main(void){
	printf("header test in non-GNU mode ok.\n");
}
