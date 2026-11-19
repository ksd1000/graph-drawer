/******************************************************************************
*License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>*
*This is free software: you are free to change and redistribute it.           *
******************************************************************************/
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "expr.h"

// compile command: gcc -I ../graph-drawer/expr ../graph-drawer/bugs/2.c ../graph-drawer/expr/expr-{internal,core,global,buffered,alloc,format,pool}.c -o main -Wall -Wextra -fsanitize=address,undefined -DEXPR_NOSYSCALL=1 -U__aarch64__ -U__x86_64__

#define TEST_STRING "str1234567890"

int main(int argc,char **argv){
	size_t len;
	char *p;
	(void)argc;
	(void)argv;
	p=malloc(sizeof(TEST_STRING));
	assert(p);
	memcpy(p,TEST_STRING,sizeof(TEST_STRING));
	printf("p=%p\n",p);
	printf("rem=%zu\n",(size_t)4096-(size_t)p%4096);

	len=expr_internal_strlen(p);
	// an asan-error will occur here.

	printf("len=%zu\n",len);
	free(p);
	return EXIT_SUCCESS;
};
