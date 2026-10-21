#include "expr-defs.h"
#include "expr.h"
int eval_expr_write_result(const char *e,char *buf,size_t len,void *heap,size_t size);
int eval_expr_write_result(const char *e,char *buf,size_t len,void *heap,size_t size){
	struct expr_memtool mtl[1];
	struct expr *ep;
	double result;
	ssize_t r;
	if(expr_setup_heapmtl(mtl,heap,size)<0)
		return -1;
	ep=expr_new1_r(e,mtl);
	if(!ep)
		return -2;
	result=expr_eval(ep,0.0);
	expr_free(ep);
	//must be free here, for use below.
	r=expr_snprintf_r(buf,len,"%.6f",4,mtl,result);
	if(r<0)
		return -3;
	if(r>=len)
		return -1;
	return 0;
	
	
}
