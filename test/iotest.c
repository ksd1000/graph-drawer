#define _GNU_SOURCE
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include "expr.h"
char buf[12341234];
int main(void){
	struct expr_global_file *fp=expr_open("out.txt","w");
	assert(fp);
	expr_fprintf(fp,"hello\n");
	expr_fprintf(fp,"six=%:id\n",6);
	expr_fprintf(fp,"e=%f\n",M_E);
	expr_fprintf(fp,"City1:%s City2:%s\n","Kawasaki","Osaka");
	expr_fprintf(fp,"buf has %.2:zM size\n",sizeof(buf));
	expr_close(fp);
	// buffer is flushed by iolbf here.
	return EXIT_SUCCESS;
}
