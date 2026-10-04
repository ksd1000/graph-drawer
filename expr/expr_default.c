
/*******************************************************************************
 *License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>*
 *This is free software: you are free to change and redistribute it.           *
 *******************************************************************************/
#define _GNU_SOURCE

#define _EXPR_LIB 1
#include "expr.h"

#if defined(EXPR_ISOLATED)&&(EXPR_ISOLATED)
expr_globals;
#endif

ssize_t expr_buffered_write(struct expr_buffered_file *restrict fp,const void *buf,size_t size){
	return expr_buffered_write_r(fp,buf,size,expr_defmtl);
}
ssize_t expr_buffered_read(struct expr_buffered_file *restrict fp,void *buf,size_t size){
	return expr_buffered_read_r(fp,buf,size,expr_defmtl);
}
ssize_t expr_buffered_read5(struct expr_buffered_file *restrict fp,void *buf,size_t size,expr_buffered_test test,intptr_t arg){
	return expr_buffered_read5_r(fp,buf,size,test,arg,expr_defmtl);
}
ssize_t expr_buffered_write_flushatc(struct expr_buffered_file *restrict fp,const void *buf,size_t size,int c){
	return expr_buffered_write_flushatc_r(fp,buf,size,c,expr_defmtl);
}
ssize_t expr_buffered_write_flushatt(struct expr_buffered_file *restrict fp,const void *buf,size_t size,expr_buffered_test test,intptr_t arg){
	return expr_buffered_write_flushatt_r(fp,buf,size,test,arg,expr_defmtl);
}
ssize_t expr_buffered_write_flushat(struct expr_buffered_file *restrict fp,const void *buf,size_t size,const void *c,size_t c_size){
	return expr_buffered_write_flushat_r(fp,buf,size,c,c_size,expr_defmtl);
}
ssize_t expr_buffered_write_sflushatc(struct expr_buffered_file *restrict fp,const void *buf,size_t size,int c){
	return expr_buffered_write_sflushatc_r(fp,buf,size,c,expr_defmtl);
}
ssize_t expr_buffered_write_sflushatt(struct expr_buffered_file *restrict fp,const void *buf,size_t size,expr_buffered_test test,intptr_t arg){
	return expr_buffered_write_sflushatt_r(fp,buf,size,test,arg,expr_defmtl);
}
ssize_t expr_buffered_write_sflushat(struct expr_buffered_file *restrict fp,const void *buf,size_t size,const void *c,size_t c_size){
	return expr_buffered_write_sflushat_r(fp,buf,size,c,c_size,expr_defmtl);
}
ssize_t expr_buffered_write_sync(struct expr_buffered_file *restrict fp,const void *buf,size_t size){
	return expr_buffered_write_sync_r(fp,buf,size,expr_defmtl);
}
ssize_t expr_buffered_close(struct expr_buffered_file *restrict fp){
	return expr_buffered_close_r(fp,expr_defmtl);
}
void expr_buffered_rclose(struct expr_buffered_file *restrict fp){
	expr_buffered_rclose_r(fp,expr_defmtl);
}
ssize_t expr_buffered_readline(struct expr_buffered_file *restrict fp,int c,void *savep){
	return expr_buffered_readline_r(fp,c,savep,expr_defmtl);
}
ssize_t expr_file_readfd(expr_reader reader,intptr_t fd,size_t tail,void *savep){
	return expr_file_readfd_r(reader,fd,tail,savep,expr_defmtl);
}
int expr_sort4(double *restrict v,size_t n){
	return expr_sort4_r(v,n,expr_defmtl);
}
struct expr_symset *expr_builtin_symbol_convert(const struct expr_builtin_symbol *syms){
	return expr_builtin_symbol_convert_r(syms,expr_defmtl);
}
void expr_free2(struct expr *restrict ep,int flag){
	expr_free2_r(ep,flag,expr_defmtl);
}
void expr_free(struct expr *restrict ep){
	expr_free_r(ep,expr_defmtl);
}
struct expr_symbol *expr_symbol_create(const char *sym,int type,int flag,...){
	struct expr_symbol *r;
	va_list ap;
	va_start(ap,flag);
	r=expr_symbol_vcreate_r(sym,type,flag,expr_defmtl,ap);
	va_end(ap);
	return r;
}
struct expr_symbol *expr_symbol_createl(const char *sym,size_t symlen,int type,int flag,...){
	struct expr_symbol *r;
	va_list ap;
	va_start(ap,flag);
	r=expr_symbol_createl_r(sym,symlen,type,flag,expr_defmtl,ap);
	va_end(ap);
	return r;
}
struct expr_symbol *expr_symbol_vcreate(const char *sym,int type,int flag,va_list ap){
	return expr_symbol_vcreate_r(sym,type,flag,expr_defmtl,ap);
}
struct expr_symbol *expr_symbol_vcreatel(const char *sym,size_t symlen,int type,int flag,va_list ap){
	return expr_symbol_vcreatel_r(sym,symlen,type,flag,expr_defmtl,ap);
}
void expr_symset_init(struct expr_symset *restrict esp){
	expr_symset_init_r(esp,expr_defmtl);
}
struct expr_symset *expr_symset_new(void){
	return expr_symset_new_r(expr_defmtl);
}
void expr_init_const(struct expr *restrict ep,double val){
	expr_init_const_r(ep,val,expr_defmtl);
}
struct expr *expr_new_const(double val){
	return expr_new_const_r(val,expr_defmtl);
}
int expr_init7(struct expr *restrict ep,const char *e,size_t len,const char *asym,size_t asymlen,struct expr_symset *esp,int flag){
	return expr_init7_r(ep,e,len,asym,asymlen,esp,flag,expr_defmtl);
}
int expr_init(struct expr *restrict ep,const char *e,const char *asym,struct expr_symset *esp,int flag){
	return expr_init_r(ep,e,asym,esp,flag,expr_defmtl);
}
int expr_init4(struct expr *restrict ep,const char *e,const char *asym,int flag){
	return expr_init4_r(ep,e,asym,flag,expr_defmtl);
}
int expr_init3(struct expr *restrict ep,const char *e,const char *asym){
	return expr_init3_r(ep,e,asym,expr_defmtl);
}
struct expr *expr_new9(const char *e,size_t len,const char *asym,size_t asymlen,struct expr_symset *esp,int flag,int n,int *error,char errinfo[EXPR_SYMLEN]){
	return expr_new9_r(e,len,asym,asymlen,esp,flag,n,error,errinfo,expr_defmtl);
}
struct expr *expr_new8(const char *e,size_t len,const char *asym,size_t asymlen,struct expr_symset *esp,int flag,int *error,char errinfo[EXPR_SYMLEN]){
	return expr_new8_r(e,len,asym,asymlen,esp,flag,error,errinfo,expr_defmtl);
}
struct expr *expr_new7(const char *e,const char *asym,struct expr_symset *esp,int flag,int n,int *error,char errinfo[EXPR_SYMLEN]){
	return expr_new7_r(e,asym,esp,flag,n,error,errinfo,expr_defmtl);
}
struct expr *expr_new(const char *e,const char *asym,struct expr_symset *esp,int flag,int *error,char errinfo[EXPR_SYMLEN]){
	return expr_new_r(e,asym,esp,flag,error,errinfo,expr_defmtl);
}
struct expr *expr_new4(const char *e,const char *asym,struct expr_symset *esp,int flag){
	return expr_new4_r(e,asym,esp,flag,expr_defmtl);
}
struct expr *expr_new3(const char *e,const char *asym,int flag){
	return expr_new3_r(e,asym,flag,expr_defmtl);
}
struct expr *expr_new2(const char *e,const char *asym){
	return expr_new2_r(e,asym,expr_defmtl);
}
double expr_calc5(const char *e,int *error,char errinfo[EXPR_SYMLEN],struct expr_symset *esp,int flag){
	return expr_calc5_r(e,error,errinfo,esp,flag,expr_defmtl);
}
double expr_calc4(const char *e,int *error,char errinfo[EXPR_SYMLEN],struct expr_symset *esp){
	return expr_calc4_r(e,error,errinfo,esp,expr_defmtl);
}
double expr_calc3(const char *e,int *error,char errinfo[EXPR_SYMLEN]){
	return expr_calc3_r(e,error,errinfo,expr_defmtl);
}
double expr_calc2(const char *e,int flag){
	return expr_calc2_r(e,flag,expr_defmtl);
}
double expr_calc(const char *e){
	return expr_calc_r(e,expr_defmtl);
}
