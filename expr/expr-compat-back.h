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
#define expr_buffered_write_warped expr_buffered_write_wrapped
#define expr_buffered_read_warped expr_buffered_read_wrapped
#define expr_buffered_write_sync_warped expr_buffered_write_sync_wrapped
#define expr_buffered_write_iolbf_warped expr_buffered_write_iolbf_wrapped
#define expr_memfd_write_warped expr_memfd_write_wrapped
#define expr_dmemfd_write_warped expr_dmemfd_write_wrapped

#define expr_warped_syscall0 expr_wrapped_syscall0
#define expr_warped_syscall1 expr_wrapped_syscall1
#define expr_warped_syscall2 expr_wrapped_syscall2
#define expr_warped_syscall3 expr_wrapped_syscall3
#define expr_warped_syscall4 expr_wrapped_syscall4
#define expr_warped_syscall5 expr_wrapped_syscall5
#define expr_warped_syscall6 expr_wrapped_syscall6
#define expr_warped_syscall7 expr_wrapped_syscall7
// :20! ../calc/calc -q 'for(i,0,7,1,printf("\#define expr_warped_syscall\%.0f expr_wrapped_syscall\%.0f\n",i,i))'

#endif
