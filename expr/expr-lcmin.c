/******************************************************************************
*License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>*
*This is free software: you are free to change and redistribute it.           *
******************************************************************************/
#define _GNU_SOURCE
#include <math.h>
#include <float.h>

#define EXPR_LIBC_FABS_IMPL 1
#define EXPR_INLIB 1
#include "expr.h"

const struct expr_builtin_symbol expr_symbols_default[]={
	REGCSYM(DBL_MAX),
	REGCSYM(DBL_MIN),
#ifdef DBL_TRUE_MIN
	REGCSYM(DBL_TRUE_MIN),
#else
	REGCSYM2("DBL_TRUE_MIN",4.9406564584124654e-324),
#endif
	REGCSYM(DBL_EPSILON),


	REGCSYM(FLT_MAX),
	REGCSYM(FLT_MIN),
	REGCSYM(FLT_EPSILON),
	REGCSYM(HUGE_VAL),
	REGCSYM(HUGE_VALF),
	REGCSYM(INFINITY),
	REGCSYM(NAN),
	REGCSYM2("NULL",(double)(uintptr_t)NULL),
	REGCSYM2("1_pi",M_1_PI),
	REGCSYM2("2_pi",M_2_PI),
	REGCSYM2("2_sqrtpi",M_2_SQRTPI),
	REGCSYM2("C",0.577215664901532860606512090082402431042),
	REGCSYM2("e",M_E),
	REGCSYM2("log2e",M_LOG2E),
	REGCSYM2("log10e",M_LOG10E),
	REGCSYM2("ln2",M_LN2),
	REGCSYM2("ln10",M_LN10),
	REGCSYM2("pi",M_PI),
	REGCSYM2("pi_2",M_PI_2),
	REGCSYM2("pi_4",M_PI_4),
	REGCSYM2("sqrt2",M_SQRT2),
	REGCSYM2("sqrt1_2",M_SQRT1_2),
	REGCSYM2("DBL_SHIFT",(double)ctz64(sizeof(double))),
	REGCSYM2("DBL_SIZE",(double)sizeof(double)),
	REGFSYM2("abs",fabs),
	REGFSYM(acos),
	REGFSYM(acosh),
	REGFSYM2("arccos",acos),
	REGFSYM2("arcosh",acosh),
	REGFSYM(asin),
	REGFSYM(asinh),
	REGFSYM2("arcsin",asin),
	REGFSYM2("arsinh",asinh),
	REGFSYM(atan),
	REGFSYM(atanh),
	REGFSYM2("arctan",atan),
	REGFSYM2("artanh",atanh),
	REGFSYM(cbrt),
	REGFSYM(ceil),
	REGFSYM(cos),
	REGFSYM(cosh),
	REGFSYM(erf),
	REGFSYM(exp),
	REGFSYM(exp2),
	REGFSYM(expm1),
	REGFSYM(fabs),
	REGFSYM(floor),
	REGFSYM(j0),
	REGFSYM(j1),
	REGFSYM(lgamma),
	REGFSYM(log),
	REGFSYM2("ln",log),
	REGFSYM(log10),
	REGFSYM(log1p),
	REGFSYM(log2),
	REGFSYM(logb),
	REGFSYM(nearbyint),
	REGFSYM(rint),
	REGFSYM(round),
	REGFSYM(sin),
	REGFSYM(sinh),
	REGFSYM(sqrt),
	REGFSYM(tan),
	REGFSYM(tanh),
	REGFSYM(tgamma),
	REGFSYM(trunc),
	REGFSYM(y0),
	REGFSYM(y1),
	{.str=NULL}
};
EXPR_LICENSE("GPL-3.0-or-later");
