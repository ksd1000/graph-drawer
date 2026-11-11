#ifndef _EXPR_COMPAT_BACK_H_
#define _EXPR_COMPAT_BACK_H_
// the header is used for some programs that uses old APIs of libexpr and libexpr-lc that were removed or changed name on newer versions.

#define expr_writer expr_writer_t
#define expr_reader expr_reader_t
#define expr_argffetch expr_argfetch_t
#define expr_argfetch expr_argfetch_t
#define expr_builtin_symbol_addall expr_symset_addall_builtin
#define expr_builtin_symbol_addalls expr_symset_addalls_builtin
#define expr_builtin_symbol_xaddall expr_symset_xaddall_builtin
#define expr_builtin_symbol_xaddalls expr_symset_xaddalls_builtin

#endif
