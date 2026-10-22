
#define expr_calc4_r(e,error,errinfo,esp,mtl) expr_calc5_r(e,error,errinfo,esp,EXPR_INOOPTIMIZE|EXPR_IPROTECT,mtl)
#define expr_calc3_r(e,error,errinfo,mtl) expr_calc5_r(e,error,errinfo,NULL,EXPR_INOOPTIMIZE|EXPR_IPROTECT,mtl)
#define expr_calc2_r(e,flag,mtl) expr_calc5_r(e,NULL,NULL,NULL,flag,mtl)
#define expr_calc_r(e,mtl) expr_calc5_r(e,NULL,NULL,NULL,EXPR_INOOPTIMIZE|EXPR_IPROTECT,mtl)

#define expr_calc5(e,error,errinfo,esp,flag) expr_calc5_r(e,error,errinfo,esp,flag,expr_defmtl)
#define expr_calc4(e,error,errinfo,esp) expr_calc4_r(e,error,errinfo,esp,expr_defmtl)
#define expr_calc3(e,error,errinfo) expr_calc3_r(e,error,errinfo,expr_defmtl)
#define expr_calc2(e,flag) expr_calc2_r(e,flag,expr_defmtl)
#define expr_calc(e) expr_calc_r(e,expr_defmtl)

#define expr_init7_r(ep,e,len,asym,asymlen,esp,flag,mtl) expr_init8_r(ep,e,len,asym,asymlen,esp,flag,NULL,mtl)
#define expr_init6_r(ep,e,len,asym,asymlen,flag,mtl) expr_init8_r(ep,e,len,asym,asymlen,NULL,flag,NULL,mtl)
#define expr_init5_r(ep,e,len,asym,asymlen,mtl) expr_init8_r(ep,e,len,asym,asymlen,NULL,0,NULL,mtl)
#define expr_init_r(ep,e,asym,esp,flag,mtl) ({\
	const char *_e=(e),*_asym=(asym);\
	expr_init8_r(ep,_e,_e?expr_internal_strlen(_e):0,_asym,_asym?expr_internal_strlen(_asym):0,esp,flag,NULL,mtl);\
})
#define expr_init4_r(ep,e,asym,flag,mtl) expr_init_r(ep,e,asym,NULL,flag,mtl)
#define expr_init3_r(ep,e,asym,mtl) expr_init_r(ep,e,asym,NULL,EXPR_IPROTECT,mtl)
#define expr_init2_r(ep,e,mtl) expr_init_r(ep,e,NULL,NULL,EXPR_IPROTECT,mtl)

#define expr_init8(ep,e,len,asym,asymlen,esp,flag,parent) expr_init8_r(ep,e,len,asym,asymlen,esp,flag,parent,expr_defmtl)
#define expr_init7(ep,e,len,asym,asymlen,esp,flag) expr_init7_r(ep,e,len,asym,asymlen,esp,flag,expr_defmtl)
#define expr_init6(ep,e,len,asym,asymlen,flag) expr_init6_r(ep,e,len,asym,asymlen,flag,expr_defmtl)
#define expr_init5(ep,e,len,asym,asymlen) expr_init5_r(ep,e,len,asym,asymlen,expr_defmtl)
#define expr_init(ep,e,asym,esp,flag) expr_init_r(ep,e,asym,esp,flag,expr_defmtl)
#define expr_init4(ep,e,asym,flag) expr_init4_r(ep,e,asym,flag,expr_defmtl)
#define expr_init3(ep,e,asym) expr_init3_r(ep,e,asym,expr_defmtl)
#define expr_init2(ep,e) expr_init2_r(ep,e,expr_defmtl)

#define expr_new8_r(e,len,asym,asymlen,esp,flag,error,errinfo,mtl) expr_new9_r(e,len,asym,asymlen,esp,flag,error,errinfo,NULL,mtl)
#define expr_new6_r(e,len,asym,asymlen,esp,flag,mtl) expr_new9_r(e,len,asym,asymlen,esp,flag,NULL,NULL,NULL,mtl)
#define expr_new_r(e,asym,esp,flag,error,errinfo,mtl) ({\
	const char *_e=(e),*_asym=(asym);\
	expr_new9_r(_e,_e?expr_internal_strlen(_e):0,_asym,_asym?expr_internal_strlen(_asym):0,esp,flag,error,errinfo,NULL,mtl);\
})
#define expr_new4_r(e,asym,esp,flag,mtl) expr_new_r(e,asym,esp,flag,NULL,NULL,mtl)
#define expr_new3_r(e,asym,flag,mtl) expr_new_r(e,asym,NULL,flag,NULL,NULL,mtl)
#define expr_new2_r(e,asym,mtl) expr_new_r(e,asym,NULL,EXPR_IPROTECT,NULL,NULL,mtl)
#define expr_new1_r(e,mtl) expr_new_r(e,NULL,NULL,EXPR_IPROTECT,NULL,NULL,mtl)

#define expr_new9(e,len,asym,asymlen,esp,flag,error,errinfo,parent) expr_new9_r(e,len,asym,asymlen,esp,flag,error,errinfo,parent,expr_defmtl)
#define expr_new8(e,len,asym,asymlen,esp,flag,error,errinfo) expr_new8_r(e,len,asym,asymlen,esp,flag,error,errinfo,expr_defmtl)
#define expr_new6(e,len,asym,asymlen,esp,flag,error,errinfo) expr_new6_r(e,len,asym,asymlen,esp,flag,error,errinfo,expr_defmtl)
#define expr_new(e,asym,esp,flag,error,errinfo) expr_new_r(e,asym,esp,flag,error,errinfo,expr_defmtl)
#define expr_new4(e,asym,esp,flag) expr_new4_r(e,asym,esp,flag,expr_defmtl)
#define expr_new3(e,asym,flag) expr_new3_r(e,asym,flag,expr_defmtl)
#define expr_new2(e,asym) expr_new2_r(e,asym,expr_defmtl)
#define expr_new1(e) expr_new1_r(e,expr_defmtl)

#define expr_vapwritef_r(fmt,fmtlen,writer,fd,mtl,ap) expr_vapwriteft_r(fmt,fmtlen,writer,fd,expr_ftable_default,mtl,ap)
#define expr_apwritef_r(fmt,fmtlen,writer,fd,mtl,...) expr_apwriteft_r(fmt,fmtlen,writer,fd,expr_ftable_default,mtl,##__VA_ARGS__)

#define expr_vapwritef(fmt,fmtlen,writer,fd,ap) expr_vapwritef_r(fmt,fmtlen,writer,fd,expr_defmtl,ap)
#define expr_apwritef(fmt,fmtlen,writer,fd,...) expr_apwritef_r(fmt,fmtlen,writer,fd,expr_defmtl,##__VA_ARGS__)

#define expr_vsnprintf_r(buf,buflen,fmt,fmtlen,mtl,ap) expr_vsnprintft_r(buf,buflen,fmt,fmtlen,expr_ftable_default,mtl,ap)
#define expr_snprintf_r(buf,buflen,fmt,fmtlen,mtl,...) expr_snprintft_r(buf,buflen,fmt,fmtlen,expr_ftable_default,mtl,##__VA_ARGS__)

#define expr_vsnprintf(buf,buflen,fmt,fmtlen,ap) expr_vsnprintf_r(buf,buflen,fmt,fmtlen,expr_defmtl,ap)
#define expr_snprintf(buf,buflen,fmt,fmtlen,...) expr_snprintf_r(buf,buflen,fmt,fmtlen,expr_defmtl,##__VA_ARGS__)

#endif

#endif

