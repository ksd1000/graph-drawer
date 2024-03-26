
#define expr_addcopy(e,t,f) expr_addop(e,t,f,EXPR_COPY)
#define expr_addcall(e,t,f) expr_addop(e,t,f,EXPR_CALL)
#define expr_addadd(e,t,f) expr_addop(e,t,f,EXPR_ADD)
#define expr_addsub(e,t,f) expr_addop(e,t,f,EXPR_SUB)
#define expr_addmul(e,t,f) expr_addop(e,t,f,EXPR_MUL)
#define expr_adddiv(e,t,f) expr_addop(e,t,f,EXPR_DIV)
#define expr_addpow(e,t,f) expr_addop(e,t,f,EXPR_POW)
#define expr_addneg(e,t) expr_addop(e,t,NULL,EXPR_NEG)
#define expr_addnot(e,t) expr_addop(e,t,NULL,EXPR_NOT)
#define expr_addnotl(e,t) expr_addop(e,t,NULL,EXPR_NOTL)
#define expr_addinput(e,t) expr_addop(e,t,NULL,EXPR_INPUT)
//#define expr_addsum(e,t,es) expr_addop(e,t,es,EXPR_SUM)
#define expr_addend(e,t) expr_addop(e,t,NULL,EXPR_END)
#define expr_addcallza(e,t,em) expr_addop(e,t,em,EXPR_CALLZA)
#define expr_addcallmd(e,t,em) expr_addop(e,t,em,EXPR_CALLMD)
#define expr_addcallmdep(e,t,em) expr_addop(e,t,em,EXPR_CALLMDEP)
#define expr_addcallhot(e,t,em) expr_addop(e,t,em,EXPR_CALLHOT)
//#define expr_addconst(e,t,v) expr_addop(e,t,v,EXPR_CONST)
//#define expr_addconst(e,t,v) expr_addop(e,t,*(void **)(v),EXPR_CONST)
#define expr_addconst(e,t,v) (expr_addop(e,t,NULL,EXPR_CONST)->un.value=(v))
#define expr_compute expr_eval
#define expr_evaluate expr_eval
#define expr_calculate expr_eval
#endif
