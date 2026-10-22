/*******************************************************************************
 *License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>*
 *This is free software: you are free to change and redistribute it.           *
 *******************************************************************************/
// for the -stdinc option of expr-core.c expr-internal.c expr-format.c expr-buffered.c expr-alloc.c expr-symext.c

#ifndef _EXPR_DEFS_H_
#define _EXPR_DEFS_H_
// if this file is not be included, _EXPR_DEFS_H_ is not defined, the above code will try to include the standard C headers, then fail under -nostdinc.

#ifndef size_t
#define size_t __SIZE_TYPE__
#endif
#ifndef ptrdiff_t
#define ptrdiff_t __PTRDIFF_TYPE__
#endif
#ifndef intptr_t
#define intptr_t __INTPTR_TYPE__
#endif
#ifndef uintptr_t
#define uintptr_t __UINTPTR_TYPE__
#endif
#ifndef int64_t
#define int64_t __INT64_TYPE__
#endif
#ifndef int32_t
#define int32_t __INT32_TYPE__
#endif
#ifndef int16_t
#define int16_t __INT16_TYPE__
#endif
#ifndef int8_t
#define int8_t __INT8_TYPE__
#endif
#ifndef intmax_t
#define intmax_t __INTMAX_TYPE__
#endif
#ifndef uint64_t
#define uint64_t __UINT64_TYPE__
#endif
#ifndef uint32_t
#define uint32_t __UINT32_TYPE__
#endif
#ifndef uint16_t
#define uint16_t __UINT16_TYPE__
#endif
#ifndef uint8_t
#define uint8_t __UINT8_TYPE__
#endif
#ifndef uintmax_t
#define uintmax_t __UINTMAX_TYPE__
#endif

#ifndef INTPTR_MAX
#define INTPTR_MAX __INTPTR_MAX__
#endif
#ifndef INT64_MAX
#define INT64_MAX __INT64_MAX__
#endif
#ifndef INT32_MAX
#define INT32_MAX __INT32_MAX__
#endif
#ifndef INT16_MAX
#define INT16_MAX __INT16_MAX__
#endif
#ifndef INT8_MAX
#define INT8_MAX __INT8_MAX__
#endif
#ifndef UINTPTR_MAX
#define UINTPTR_MAX __UINTPTR_MAX__
#endif
#ifndef UINT64_MAX
#define UINT64_MAX __UINT64_MAX__
#endif
#ifndef UINT32_MAX
#define UINT32_MAX __UINT32_MAX__
#endif
#ifndef UINT16_MAX
#define UINT16_MAX __UINT16_MAX__
#endif
#ifndef UINT8_MAX
#define UINT8_MAX __UINT8_MAX__
#endif

#define EXPR_DEFS_CAT0(a,b) a ## b
#define EXPR_DEFS_CAT(a,b) EXPR_DEFS_CAT0(a,b)

#ifndef INT64_C
#if defined(__INT64_C)
#define INT64_C __INT64_C
#elif defined(__INT64_C_SUFFIX__)
#define INT64_C(c) EXPR_DEFS_CAT(c,__INT64_C_SUFFIX__)
#else
#define INT64_C(c) EXPR_DEFS_CAT(c,L)
#endif
#endif

#ifndef UINT64_C
#if defined(__UINT64_C)
#define UINT64_C __UINT64_C
#elif defined(__UINT64_C_SUFFIX__)
#define UINT64_C(c) EXPR_DEFS_CAT(c,__UINT64_C_SUFFIX__)
#else
#define UINT64_C(c) EXPR_DEFS_CAT(c,UL)
#endif
#endif

#ifndef INT32_C
#if defined(__INT32_C)
#define INT32_C __INT32_C
#elif defined(__INT32_C_SUFFIX__)
#define INT32_C(c) EXPR_DEFS_CAT(c,__INT32_C_SUFFIX__)
#else
#define INT32_C(c) c
#endif
#endif

#ifndef UINT32_C
#if defined(__UINT32_C)
#define UINT32_C __UINT32_C
#elif defined(__UINT32_C_SUFFIX__)
#define UINT32_C(c) EXPR_DEFS_CAT(c,__UINT32_C_SUFFIX__)
#else
#define UINT32_C(c) EXPR_DEFS_CAT(c,U)
#endif
#endif

#ifndef INT16_C
#if defined(__INT16_C)
#define INT16_C __INT16_C
#elif defined(__INT16_C_SUFFIX__)
#define INT16_C(c) EXPR_DEFS_CAT(c,__INT16_C_SUFFIX__)
#else
#define INT16_C(c) c
#endif
#endif

#ifndef UINT16_C
#if defined(__UINT16_C)
#define UINT16_C __UINT16_C
#elif defined(__UINT16_C_SUFFIX__)
#define UINT16_C(c) EXPR_DEFS_CAT(c,__UINT16_C_SUFFIX__)
#else
#define UINT16_C(c) c
#endif
#endif

#ifndef INT8_C
#if defined(__INT8_C)
#define INT8_C __INT8_C
#elif defined(__INT8_C_SUFFIX__)
#define INT8_C(c) EXPR_DEFS_CAT(c,__INT8_C_SUFFIX__)
#else
#define INT8_C(c) c
#endif
#endif

#ifndef UINT8_C
#if defined(__UINT8_C)
#define UINT8_C __UINT8_C
#elif defined(__UINT8_C_SUFFIX__)
#define UINT8_C(c) EXPR_DEFS_CAT(c,__UINT8_C_SUFFIX__)
#else
#define UINT8_C(c) c
#endif
#endif

#ifndef SIZE_MAX
#define SIZE_MAX __SIZE_MAX__
#endif
#ifndef PTRDIFF_MAX
#define PTRDIFF_MAX __PTRDIFF_MAX__
#endif
#ifndef PTRDIFF_MIN
#define PTRDIFF_MIN (-PTRDIFF_MAX-1)
#endif



#ifndef INT_MAX
#define INT_MAX __INT_MAX__
#endif
#ifndef INT_MIN
#define INT_MIN (-INT_MAX-1)
#endif
#ifndef UINT_MAX
#define UINT_MAX (((unsigned int)INT_MAX<<1)|(unsigned int)1)
#endif
#ifndef NULL
#define NULL ((void *)0)
#endif
#ifndef DBL_EPSILON
#define DBL_EPSILON __DBL_EPSILON__
#endif
#ifndef NAN
#define NAN (__builtin_nanf(""))
#endif
#ifndef INFINITY
#define INFINITY (__builtin_inff())
#endif
#ifndef SCHAR_MAX
#define SCHAR_MAX __SCHAR_MAX__
#endif
#ifndef UCHAR_MAX
#define UCHAR_MAX (((unsigned char)SCHAR_MAX<<1)|(unsigned char)1)
#endif
#ifndef va_list
#define va_list __builtin_va_list
#endif
#ifndef va_start
#define va_start __builtin_va_start
#endif
#ifndef va_arg
#define va_arg __builtin_va_arg
#endif
#ifndef va_end
#define va_end __builtin_va_end
#endif
#ifndef va_copy
#define va_copy __builtin_va_copy
#endif
#ifndef offsetof
#define offsetof __builtin_offsetof
#endif

// assuming these __builtin_* are probided by gcc/clang.

#endif
