/*******************************************************************************
 *License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>*
 *This is free software: you are free to change and redistribute it.           *
 *******************************************************************************/
// for the -stdinc option of expr-core.c expr-internal.c expr-format.c expr-buffered.c expr-alloc.c expr-symext.c

#ifndef _EXPR_DEFS_H_
#define _EXPR_DEFS_H_
// if this file is not be included, _EXPR_DEFS_H_ is not defined, the above code will try to include the standard C headers, then fail under -nostdinc.

#undef size_t
#define size_t __SIZE_TYPE__
#undef ptrdiff_t
#define ptrdiff_t __PTRDIFF_TYPE__
#undef intptr_t
#define intptr_t __INTPTR_TYPE__
#undef uintptr_t
#define uintptr_t __UINTPTR_TYPE__
#undef int64_t
#define int64_t __INT64_TYPE__
#undef int32_t
#define int32_t __INT32_TYPE__
#undef int16_t
#define int16_t __INT16_TYPE__
#undef int8_t
#define int8_t __INT8_TYPE__
#undef intmax_t
#define intmax_t __INTMAX_TYPE__
#undef uint64_t
#define uint64_t __UINT64_TYPE__
#undef uint32_t
#define uint32_t __UINT32_TYPE__
#undef uint16_t
#define uint16_t __UINT16_TYPE__
#undef uint8_t
#define uint8_t __UINT8_TYPE__
#undef uintmax_t
#define uintmax_t __UINTMAX_TYPE__
#define INTPTR_MAX __INTPTR_MAX__
#define INT64_MAX __INT64_MAX__
#define INT32_MAX __INT32_MAX__
#define INT16_MAX __INT16_MAX__
#define INT8_MAX __INT8_MAX__
#define UINTPTR_MAX __UINTPTR_MAX__
#define UINT64_MAX __UINT64_MAX__
#define UINT32_MAX __UINT32_MAX__
#define UINT16_MAX __UINT16_MAX__
#define UINT8_MAX __UINT8_MAX__

#define EXPR_DEFS_CAT0(a,b) a ## b
#define EXPR_DEFS_CAT(a,b) EXPR_DEFS_CAT0(a,b)

#if defined(__INT64_C)
#define INT64_C __INT64_C
#elif defined(__INT64_C_SUFFIX__)
#define INT64_C(c) EXPR_DEFS_CAT(c,__INT64_C_SUFFIX__)
#else
#define INT64_C(c) EXPR_DEFS_CAT(c,L)
#endif

#if defined(__UINT64_C)
#define UINT64_C __UINT64_C
#elif defined(__UINT64_C_SUFFIX__)
#define UINT64_C(c) EXPR_DEFS_CAT(c,__UINT64_C_SUFFIX__)
#else
#define UINT64_C(c) EXPR_DEFS_CAT(c,UL)
#endif

#if defined(__INT32_C)
#define INT32_C __INT32_C
#elif defined(__INT32_C_SUFFIX__)
#define INT32_C(c) EXPR_DEFS_CAT(c,__INT32_C_SUFFIX__)
#else
#define INT32_C(c) c
#endif

#if defined(__UINT32_C)
#define UINT32_C __UINT32_C
#elif defined(__UINT32_C_SUFFIX__)
#define UINT32_C(c) EXPR_DEFS_CAT(c,__UINT32_C_SUFFIX__)
#else
#define UINT32_C(c) EXPR_DEFS_CAT(c,U)
#endif

#if defined(__INT16_C)
#define INT16_C __INT16_C
#elif defined(__INT16_C_SUFFIX__)
#define INT16_C(c) EXPR_DEFS_CAT(c,__INT16_C_SUFFIX__)
#else
#define INT16_C(c) c
#endif

#if defined(__UINT16_C)
#define UINT16_C __UINT16_C
#elif defined(__UINT16_C_SUFFIX__)
#define UINT16_C(c) EXPR_DEFS_CAT(c,__UINT16_C_SUFFIX__)
#else
#define UINT16_C(c) c
#endif

#if defined(__INT8_C)
#define INT8_C __INT8_C
#elif defined(__INT8_C_SUFFIX__)
#define INT8_C(c) EXPR_DEFS_CAT(c,__INT8_C_SUFFIX__)
#else
#define INT8_C(c) c
#endif

#if defined(__UINT8_C)
#define UINT8_C __UINT8_C
#elif defined(__UINT8_C_SUFFIX__)
#define UINT8_C(c) EXPR_DEFS_CAT(c,__UINT8_C_SUFFIX__)
#else
#define UINT8_C(c) c
#endif

#undef SIZE_MAX
#define SIZE_MAX __SIZE_MAX__
#undef PTRDIFF_MAX
#define PTRDIFF_MAX __PTRDIFF_MAX__
#undef PTRDIFF_MIN
#define PTRDIFF_MIN (-PTRDIFF_MAX-1)
#define INT_MAX __INT_MAX__
#define UINT_MAX ((unsigned int)((INT_MAX<<1)|(1)))
#define NULL ((void *)0)
#define DBL_EPSILON __DBL_EPSILON__
#define NAN (__builtin_nanf(""))
#define INFINITY (__builtin_inff())
#define SCHAR_MAX __SCHAR_MAX__
#define UCHAR_MAX (((unsigned char)SCHAR_MAX<<1)|(unsigned char)1)
#define va_list __builtin_va_list
#define va_start __builtin_va_start
#define va_arg __builtin_va_arg
#define va_end __builtin_va_end
#define va_copy __builtin_va_copy
#define offsetof __builtin_offsetof
// assuming these __builtin_* are probided by gcc/clang.

#endif
