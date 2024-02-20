
#define expr_addcopy(e,t,f) expr_addop(e,t,f,EXPR_COPY)
#define expr_addadd(e,t,f) expr_addop(e,t,f,EXPR_ADD)
#define expr_addsub(e,t,f) expr_addop(e,t,f,EXPR_SUB)
#define expr_addmul(e,t,f) expr_addop(e,t,f,EXPR_MUL)
#define expr_adddiv(e,t,f) expr_addop(e,t,f,EXPR_DIV)
#define expr_addpow(e,t,f) expr_addop(e,t,f,EXPR_POW)
#define expr_addneg(e,t) expr_addop(e,t,NULL,EXPR_NEG)
#define expr_addsum(e,t,es) expr_addsumop(e,t,es,EXPR_SUM)
#define expr_addint(e,t,es) expr_addsumop(e,t,es,EXPR_INT)
#define expr_addend(e,t) expr_addop(e,t,NULL,EXPR_END)
#endif
