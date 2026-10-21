obj-m += kexpr.o
kexpr-y := kmain.o eval.o expr-core.o expr-internal.o expr-format.o expr-alloc.o expr-symext.o
KPATH = /usr/src/kernels/$(shell uname -r)
EXPR_CFLAGS := -include expr-defs.h -Dexpr_noinline="" -DEXPR_NOSYSCALL=1 -DEXPR_NODEBUGEVAL=1 -DEXPR_NOSTACKWORKSPACE=1
CFLAGS_eval.o += $(CC_FLAGS_FPU)
CFLAGS_REMOVE_eval.o += $(CC_FLAGS_NO_FPU)
CFLAGS_expr-core.o += $(CC_FLAGS_FPU) $(EXPR_CFLAGS)
CFLAGS_REMOVE_expr-core.o += $(CC_FLAGS_NO_FPU)
CFLAGS_expr-internal.o += $(CC_FLAGS_FPU) $(EXPR_CFLAGS)
CFLAGS_REMOVE_expr-internal.o += $(CC_FLAGS_NO_FPU)
CFLAGS_expr-format.o += $(CC_FLAGS_FPU) $(EXPR_CFLAGS)
CFLAGS_REMOVE_expr-format.o += $(CC_FLAGS_NO_FPU)
#CFLAGS_expr-alloc.o += $(CC_FLAGS_FPU) $(EXPR_CFLAGS)
#CFLAGS_REMOVE_expr-alloc.o += $(CC_FLAGS_NO_FPU)
#CFLAGS_expr-symext.o += $(CC_FLAGS_FPU) $(EXPR_CFLAGS)
#CFLAGS_REMOVE_expr-symext.o += $(CC_FLAGS_NO_FPU)
EXPR_FILES := ../../expr/expr-core.c ../../expr/expr-internal.c ../../expr/expr-format.c ../../expr/expr-alloc.c ../../expr/expr.h  ../../expr/expr-defs.h
all:
	cp $(EXPR_FILES) .
	make -C "$(KPATH)" M=$(PWD) modules
.PHONY:
ustest:
	cp $(EXPR_FILES) .
	gcc -DUSERSPACE_TEST -fsanitize=address,undefined -c -o kmain-user.o kmain.c
	gcc $(EXPR_CFLAGS) -fsanitize=address,undefined -o userspace-test ../../expr/expr-core.c ../../expr/expr-internal.c ../../expr/expr-format.c ../../expr/expr-alloc.c kmain-user.o eval.c
.PHONY:
clean:
	rm -f expr-core.c
	rm -f expr-internal.c
	rm -f expr-format.c
	rm -f expr-alloc.c
	rm -f expr.h
	rm -f expr-defs.h
	rm -f userspace-test
	rm -f kmain-user.o
	make -C "$(KPATH)" M=$(PWD) clean
