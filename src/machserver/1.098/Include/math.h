/*
 * math.h --
 *
 *	
 *
 * Copyright 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /sprite/src/lib/m/RCS/math.h,v 1.6 90/09/18 17:41:39 kupfer Exp Locker: rab $
 */

#ifndef _MATH_H
#define _MATH_H

#ifdef __GNUC__                 /* Use const function attribute */
    /* As a non-standard extension, Gnu C will treat any function
       that has a `const' attribute as a pure function.  A pure function
       has no side effects, and the return value is function of the
       arguments only.  If a pure function is invoked several times
       with the same arguments, the compiler can optimize out all but
       the first call.
     */
#define _CONST_FUNC  const
#else
#define _CONST_FUNC
#endif

#define M_LN2   0.69314718055994530942
#define M_PI    3.14159265358979323846
#define M_SQRT2 1.41421356237309504880

#define M_E             2.7182818284590452354
#define M_LOG2E         1.4426950408889634074
#define M_LOG10E        0.43429448190325182765
#define M_LN10          2.30258509299404568402
#define M_PI_2          1.57079632679489661923
#define M_PI_4          0.78539816339744830962
#define M_1_PI          0.31830988618379067154
#define M_2_PI          0.63661977236758134308
#define M_2_SQRTPI      1.12837916709551257390
#define M_SQRT1_2       0.70710678118654752440
#define _POLY1(x, c)    ((c)[0] * (x) + (c)[1])
#define _POLY2(x, c)    (_POLY1((x), (c)) * (x) + (c)[2])
#define _POLY3(x, c)    (_POLY2((x), (c)) * (x) + (c)[3])
#define _POLY4(x, c)    (_POLY3((x), (c)) * (x) + (c)[4])
#define _POLY5(x, c)    (_POLY4((x), (c)) * (x) + (c)[5])
#define _POLY6(x, c)    (_POLY5((x), (c)) * (x) + (c)[6])
#define _POLY7(x, c)    (_POLY6((x), (c)) * (x) + (c)[7])
#define _POLY8(x, c)    (_POLY7((x), (c)) * (x) + (c)[8])
#define _POLY9(x, c)    (_POLY8((x), (c)) * (x) + (c)[9])

#ifdef __STDC__

#if defined(sun3) && !defined(__STRICT_ANSI__) && !defined(__SOFT_FLOAT__)

#include <math-68881.h>

#else

extern _CONST_FUNC double    sin(double x);
extern _CONST_FUNC double    cos(double x);
extern _CONST_FUNC double    tan(double x);
extern _CONST_FUNC double    asin(double x);
extern _CONST_FUNC double    acos(double x);
extern _CONST_FUNC double    atan(double x);
extern _CONST_FUNC double    atan2(double y, double x);
extern _CONST_FUNC double    sinh(double x);
extern _CONST_FUNC double    cosh(double x);
extern _CONST_FUNC double    tanh(double x);
extern _CONST_FUNC double    exp(double x);
extern _CONST_FUNC double    log(double x);
extern _CONST_FUNC double    log10(double x);
extern _CONST_FUNC double    pow(double x, double y);
extern _CONST_FUNC double    sqrt(double x);
extern _CONST_FUNC double    ceil(double x);
extern _CONST_FUNC double    floor(double x);
extern _CONST_FUNC double    fabs(double x);
extern _CONST_FUNC double    ldexp(double x, int n);
extern _CONST_FUNC double    fmod(double x , double y);
extern double           frexp(double x, int *exp);
extern double           modf(double x, double *ip);

extern _CONST_FUNC double    atanh(double x);
extern _CONST_FUNC double    asinh(double x);
extern _CONST_FUNC double    acosh(double x);
extern _CONST_FUNC double    expm1(double x);

#endif

extern _CONST_FUNC double    erf(double x);
extern _CONST_FUNC double    erfc(double x);
extern _CONST_FUNC double    log1p(double x);
extern _CONST_FUNC double    rint(double x);
extern _CONST_FUNC double    lgamma(double x);
extern _CONST_FUNC double    hypot(double x, double y);
extern _CONST_FUNC double    cabs();
extern _CONST_FUNC double    copysign(double x, double y);
extern _CONST_FUNC double    drem(double x, double y);
extern _CONST_FUNC double    logb(double x);
extern _CONST_FUNC double    scalb(double x, int n);
extern _CONST_FUNC double    j0(double x);
extern _CONST_FUNC double    j1(double x);
extern _CONST_FUNC double    jn(int n, double x);
extern _CONST_FUNC double    y0(double x);
extern _CONST_FUNC double    y1(double x);
extern _CONST_FUNC double    yn(int n, double x);
extern _CONST_FUNC double    cbrt(double x);

extern int finite(double x);

extern int isinf(double x);
extern int isnan(double x);

#else /* __STDC__ */

extern double   sin();
extern double   cos();
extern double   tan();
extern double   asin();
extern double   acos();
extern double   atan();
extern double   atan2();
extern double   sinh();
extern double   cosh();
extern double   tanh();
extern double   exp();
extern double   log();
extern double   log10();
extern double   pow();
extern double   sqrt();
extern double   ceil();
extern double   floor();
extern double   fabs();
extern double   ldexp();
extern double   fmod();
extern double   frexp();
extern double   modf();
extern double   asinh();
extern double   acosh();
extern double   atanh();
extern double   erf();
extern double   erfc();
extern double   expm1();
extern double   log1p();
extern double   rint();
extern double   lgamma();
extern double   hypot();
extern double   cabs();
extern double   copysign();
extern double   drem();
extern double   logb();
extern double   scalb();
extern double   j0();
extern double   j1();
extern double   jn();
extern double   y0();
extern double   y1();
extern double   yn();
extern double   cbrt();
extern int finite();
extern int isinf();
extern int isnan();

#endif /* __STDC__ */

#ifndef HUGE_VAL
#define HUGE_VAL    1.701411733192644270e38
#endif

#define HUGE	    HUGE_VAL

#undef _CONST_FUNC

#endif /* _MATH_H */

