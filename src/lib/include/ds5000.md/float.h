/*
 * floats.h --
 *
 *	Declares machine-dependent floating point characteristics.
 *
 * Copyright 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header$
 */

#ifndef _FLOAT_H
#define _FLOAT_H

#define FLT_RADIX       2
#define FLT_ROUNDS      1

#define FLT_DIG         6
#define DBL_DIG         15

#define FLT_EPSILON     1.19209290e-07
#define DBL_EPSILON     2.2204460492503131e-16

#define FLT_MANT_DIG    24
#define DBL_MANT_DIG    53

#define FLT_MAX         3.40282347e+38
#define DBL_MAX         1.797693134862316e+308


#define FLT_MAX_EXP     128
#define DBL_MAX_EXP     1024

#define FLT_MIN         1.17549435e-38
#define DBL_MIN         2.225073858507201e-308

#define FLT_MIN_EXP     (-125)
#define DBL_MIN_EXP     (-1021)

#endif /* _FLOAT_H */

