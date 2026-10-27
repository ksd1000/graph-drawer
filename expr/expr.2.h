
#define expr_calc4_r(e,error,errinfo,esp,mtl) expr_calc5_r(e,error,errinfo,esp,EXPR_INOOPTIMIZE|EXPR_IDEFAULT,mtl)
#define expr_calc3_r(e,error,errinfo,mtl) expr_calc5_r(e,error,errinfo,NULL,EXPR_INOOPTIMIZE|EXPR_IDEFAULT,mtl)
#define expr_calc2_r(e,flag,mtl) expr_calc5_r(e,NULL,NULL,NULL,flag,mtl)
#define expr_calc_r(e,mtl) expr_calc5_r(e,NULL,NULL,NULL,EXPR_INOOPTIMIZE|EXPR_IDEFAULT,mtl)

#define expr_calc5(e,error,errinfo,esp,flag) expr_calc5_r(e,error,errinfo,esp,flag,expr_defmtl)
#define expr_calc4(e,error,errinfo,esp) expr_calc4_r(e,error,errinfo,esp,expr_defmtl)
#define expr_calc3(e,error,errinfo) expr_calc3_r(e,error,errinfo,expr_defmtl)
#define expr_calc2(e,flag) expr_calc2_r(e,flag,expr_defmtl)
#define expr_calc(e) expr_calc_r(e,expr_defmtl)

#define expr_init7_r(ep,e,len,asym,asymlen,esp,flag,mtl) expr_init8_r(ep,e,len,asym,asymlen,esp,flag,expr_extarg_default,mtl)
#define expr_init6_r(ep,e,len,asym,asymlen,flag,mtl) expr_init8_r(ep,e,len,asym,asymlen,NULL,flag,expr_extarg_default,mtl)
#define expr_init5_r(ep,e,len,asym,asymlen,mtl) expr_init8_r(ep,e,len,asym,asymlen,NULL,0,expr_extarg_default,mtl)
#define expr_init_r(ep,e,asym,esp,flag,mtl) ({\
	const char *_e=(e),*_asym=(asym);\
	expr_init8_r(ep,_e,_e?expr_internal_strlen(_e):0,_asym,_asym?expr_internal_strlen(_asym):0,esp,flag,expr_extarg_default,mtl);\
})
#define expr_init4_r(ep,e,asym,flag,mtl) expr_init_r(ep,e,asym,NULL,flag,mtl)
#define expr_init3_r(ep,e,asym,mtl) expr_init_r(ep,e,asym,NULL,EXPR_IDEFAULT,mtl)
#define expr_init2_r(ep,e,mtl) expr_init_r(ep,e,NULL,NULL,EXPR_IDEFAULT,mtl)

#define expr_init7(ep,e,len,asym,asymlen,esp,flag) expr_init7_r(ep,e,len,asym,asymlen,esp,flag,expr_defmtl)
#define expr_init6(ep,e,len,asym,asymlen,flag) expr_init6_r(ep,e,len,asym,asymlen,flag,expr_defmtl)
#define expr_init5(ep,e,len,asym,asymlen) expr_init5_r(ep,e,len,asym,asymlen,expr_defmtl)
#define expr_init(ep,e,asym,esp,flag) expr_init_r(ep,e,asym,esp,flag,expr_defmtl)
#define expr_init4(ep,e,asym,flag) expr_init4_r(ep,e,asym,flag,expr_defmtl)
#define expr_init3(ep,e,asym) expr_init3_r(ep,e,asym,expr_defmtl)
#define expr_init2(ep,e) expr_init2_r(ep,e,expr_defmtl)

#define expr_new8_r(e,len,asym,asymlen,esp,flag,error,errinfo,mtl) expr_new9_r(e,len,asym,asymlen,esp,flag,error,errinfo,expr_extarg_default,mtl)
#define expr_new6_r(e,len,asym,asymlen,esp,flag,mtl) expr_new9_r(e,len,asym,asymlen,esp,flag,NULL,NULL,expr_extarg_default,mtl)
#define expr_new_r(e,asym,esp,flag,error,errinfo,mtl) ({\
	const char *_e=(e),*_asym=(asym);\
	expr_new9_r(_e,_e?expr_internal_strlen(_e):0,_asym,_asym?expr_internal_strlen(_asym):0,esp,flag,error,errinfo,expr_extarg_default,mtl);\
})
#define expr_new4_r(e,asym,esp,flag,mtl) expr_new_r(e,asym,esp,flag,NULL,NULL,mtl)
#define expr_new3_r(e,asym,flag,mtl) expr_new_r(e,asym,NULL,flag,NULL,NULL,mtl)
#define expr_new2_r(e,asym,mtl) expr_new_r(e,asym,NULL,EXPR_IDEFAULT,NULL,NULL,mtl)
#define expr_new1_r(e,mtl) expr_new_r(e,NULL,NULL,EXPR_IDEFAULT,NULL,NULL,mtl)

#define expr_new8(e,len,asym,asymlen,esp,flag,error,errinfo) expr_new8_r(e,len,asym,asymlen,esp,flag,error,errinfo,expr_defmtl)
#define expr_new6(e,len,asym,asymlen,esp,flag,error,errinfo) expr_new6_r(e,len,asym,asymlen,esp,flag,error,errinfo,expr_defmtl)
#define expr_new(e,asym,esp,flag,error,errinfo) expr_new_r(e,asym,esp,flag,error,errinfo,expr_defmtl)
#define expr_new4(e,asym,esp,flag) expr_new4_r(e,asym,esp,flag,expr_defmtl)
#define expr_new3(e,asym,flag) expr_new3_r(e,asym,flag,expr_defmtl)
#define expr_new2(e,asym) expr_new2_r(e,asym,expr_defmtl)
#define expr_new1(e) expr_new1_r(e,expr_defmtl)

#define expr_vwritef_r(writer,fd,fmt,fmtlen,arg,addr,mtl) expr_vwriteft_r(writer,fd,fmt,fmtlen,arg,addr,expr_ftable_default,mtl)

#define expr_vwritef(writer,fd,fmt,fmtlen,arg,addr) expr_vwritef_r(writer,fd,fmt,fmtlen,arg,addr,expr_defmtl)

#define expr_writef_r(writer,fd,fmt,fmtlen,args,arglen,mtl) expr_writeft_r(writer,fd,fmt,fmtlen,args,arglen,expr_ftable_default,mtl)

#define expr_writef(writer,fd,fmt,fmtlen,args,arglen) expr_writef_r(writer,fd,fmt,fmtlen,args,arglen,expr_defmtl)

#define expr_vapwritef_r(writer,fd,fmt,fmtlen,mtl,ap) expr_vapwriteft_r(writer,fd,fmt,fmtlen,expr_ftable_default,mtl,ap)
#define expr_apwritef_r(writer,fd,fmt,fmtlen,mtl,...) expr_apwriteft_r(writer,fd,fmt,fmtlen,expr_ftable_default,mtl,##__VA_ARGS__)

#define expr_vapwritef(writer,fd,fmt,fmtlen,ap) expr_vapwritef_r(writer,fd,fmt,fmtlen,expr_defmtl,ap)
#define expr_apwritef(writer,fd,fmt,fmtlen,...) expr_apwritef_r(writer,fd,fmt,fmtlen,expr_defmtl,##__VA_ARGS__)

#define expr_swritef_r(buf,buflen,fmt,fmtlen,args,arglen,mtl) expr_swriteft_r(buf,buflen,fmt,fmtlen,args,arglen,expr_ftable_default,mtl)

#define expr_swritef(buf,buflen,fmt,fmtlen,args,arglen) expr_swritef_r(buf,buflen,fmt,fmtlen,args,arglen,expr_defmtl)

#define expr_vsnprintf_r(buf,buflen,fmt,fmtlen,mtl,ap) expr_vsnprintft_r(buf,buflen,fmt,fmtlen,expr_ftable_default,mtl,ap)
#define expr_snprintf_r(buf,buflen,fmt,fmtlen,mtl,...) expr_snprintft_r(buf,buflen,fmt,fmtlen,expr_ftable_default,mtl,##__VA_ARGS__)

#define expr_vsnprintf(buf,buflen,fmt,fmtlen,ap) expr_vsnprintf_r(buf,buflen,fmt,fmtlen,expr_defmtl,ap)
#define expr_snprintf(buf,buflen,fmt,fmtlen,...) expr_snprintf_r(buf,buflen,fmt,fmtlen,expr_defmtl,##__VA_ARGS__)

#define expr_common_asprintft_r(strp,fmt,fmtlen,table,mtl,called,...) ({\
	struct expr_buffered_file _fp[1];\
	const struct expr_memtool *restrict _mtl=(mtl);\
	ssize_t _r;\
	if(expr_likely(!expr_buffered_ainit_r(_fp,SIZE_MAX,_mtl))){\
		_r=called(expr_buffered_write_warp,(intptr_t)_fp,fmt,fmtlen,table,_mtl,##__VA_ARGS__);\
		if(expr_likely(_r>=0)){\
			((char *)_fp->buf)[_r]=0;\
		/* buffer expands if the writer touches its limit*/\
		*(strp)=_fp->buf;\
		}else {\
			expr_buffered_close(_fp);\
			(void)(strp);\
		}\
	}else {\
		_r=SSIZE_MIN;\
		(void)(strp);\
	}\
	_r;\
})

#define expr_vasprintft_r(strp,fmt,fmtlen,table,mtl,ap) expr_common_asprintft_r(strp,fmt,fmtlen,table,mtl,expr_vapwriteft_r,ap)
#define expr_asprintft_r(strp,fmt,fmtlen,table,mtl,...) expr_common_asprintft_r(strp,fmt,fmtlen,table,mtl,expr_apwriteft_r,##__VA_ARGS__)

#define expr_vasprintf_r(strp,fmt,fmtlen,mtl,ap) expr_vasprintft_r(strp,fmt,fmtlen,expr_ftable_default,mtl,ap)
#define expr_asprintf_r(strp,fmt,fmtlen,mtl,...) expr_asprintft_r(strp,fmt,fmtlen,expr_ftable_default,mtl,##__VA_ARGS__)

#define expr_vasprintft(strp,fmt,fmtlen,table,ap) expr_vasprintft_r(strp,fmt,fmtlen,table,expr_defmtl,ap)
#define expr_asprintft(strp,fmt,fmtlen,table,...) expr_asprintft_r(strp,fmt,fmtlen,table,expr_defmtl,##__VA_ARGS__)

#define expr_vasprintf(strp,fmt,fmtlen,ap) expr_vasprintft_r(strp,fmt,fmtlen,expr_ftable_default,expr_defmtl,ap)
#define expr_asprintf(strp,fmt,fmtlen,...) expr_asprintft_r(strp,fmt,fmtlen,expr_ftable_default,expr_defmtl,##__VA_ARGS__)

#define expr_builtin_symbol_xaddall(esp,symsp,syms) expr_builtin_symbol_xaddalls(esp,symsp,syms,NULL)
#define expr_builtin_symbol_addall(esp,syms) expr_builtin_symbol_addalls(esp,syms,NULL)

#define expr_area_init(area,zone,size) expr_area_init4(area,zone,size,EXPR_ADEFAULT)
#define expr_setup_heapmtl(mtl,heap,size) expr_setup_heapmtl4(mtl,heap,size,EXPR_ADEFAULT)

#endif

// visible also with EXPR_INLIB set, for expr-alloc.c use.
#define expr_area_malloc3(area,size,flag) expr_area_realloc4(area,NULL,size,flag)
#define expr_area_malloc(area,size) ({\
	struct expr_area *restrict __areap=(area);\
	expr_area_malloc3(__areap,size,__areap->flag);\
})
#define expr_area_realloc(area,old,size) ({\
	struct expr_area *restrict __areap=(area);\
	expr_area_realloc4(__areap,old,size,__areap->flag);\
})
#define expr_area_calloc(area,size) ({\
	struct expr_area *restrict __areap=(area);\
	expr_area_malloc3(__areap,size,__areap->flag|EXPR_AZERO);\
})

#endif

