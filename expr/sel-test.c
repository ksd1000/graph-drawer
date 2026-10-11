
#define expr_internal_syscall_sel(_n,_0,_1,_2,_3,_4,_5,_6,_name,...) _name
#define expr_internal_syscall(...) expr_internal_syscall_sel(__VA_ARGS__,expr_internal_syscall7,expr_internal_syscall6,expr_internal_syscall5,expr_internal_syscall4,expr_internal_syscall3,expr_internal_syscall2,expr_internal_syscall1,expr_internal_syscall0)(__VA_ARGS__)
expr_internal_syscall();
expr_internal_syscall(SYS_write);
expr_internal_syscall(SYS_write,fd);
expr_internal_syscall(SYS_write,fd,buf);
expr_internal_syscall(SYS_write,fd,buf,size);
expr_internal_syscall(SYS_write,fd,buf,size,a4);
expr_internal_syscall(SYS_write,fd,buf,size,a4,a5);
expr_internal_syscall(SYS_write,fd,buf,size,a4,a5,a6);
expr_internal_syscall(SYS_write,fd,buf,size,a4,a5,a6,a7);
expr_internal_syscall(SYS_write,fd,buf,size,a4,a5,a6,a8);

