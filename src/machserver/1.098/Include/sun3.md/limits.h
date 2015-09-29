/*
 * limits.h --
 *
 *	Declares machine-dependent limits on the values that can
 *	be stored in integer-like objects.
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
 * $Header: /sprite/src/lib/include/sun3.md/RCS/limits.h,v 1.3 89/02/10 11:15:55 rab Exp $ SPRITE (Berkeley)
 */

#ifndef _LIMITS
#define _LIMITS

/*
 * Number of bits in a "char":
 */
#define CHAR_BIT		8

/*
 * Minimum and maximum values for a "signed char":
 */
#define SCHAR_MIN		(-128)
#define SCHAR_MAX		127

/*
 * Maximum value for an "unsigned char":
 */
#define UCHAR_MAX		255

/*
 * Minimum and maximum values for a "char":
 */
#define CHAR_MIN		(-128)
#define CHAR_MAX		+127

/*
 * Minimum and maximum values for a "short int":
 */
#define SHRT_MIN		0x8000
#define SHRT_MAX		0x7fff

/*
 * Maximum value for an "unsigned short int":
 */
#define USHRT_MAX		0xffff

/*
 * Minimum and maximum values for an "int":
 */
#define INT_MIN			0x80000000
#define INT_MAX			0x7fffffff

/*
 * Maximum value for an "unsigned int":
 */
#define UINT_MAX		0xffffffff

/*
 * Minimum and maximum values for a "long int":
 */
#define LONG_MIN		0x80000000
#define LONG_MAX		0x7fffffff

/*
 * Maximum value for an "unsigned long int":
 */
#define ULONG_MAX		0xffffffff

#endif /* _LIMITS */
