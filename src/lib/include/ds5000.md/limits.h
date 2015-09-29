/*
 * limits.h --
 *
 *	Declares machine-dependent limits on the values that can
 *	be stored in integer-like objects.
 *
 * Copyright (C) 1989 by Digital Equipment Corporation, Maynard MA
 *
 *			All Rights Reserved
 *
 * Permission to use, copy, modify, and distribute this software and its 
 * documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in 
 * supporting documentation, and that the name of Digital not be
 * used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.  
 *
 * Digitial disclaims all warranties with regard to this software, including
 * all implied warranties of merchantability and fitness.  In no event shall
 * Digital be liable for any special, indirect or consequential damages or
 * any damages whatsoever resulting from loss of use, data or profits,
 * whether in an action of contract, negligence or other tortious action,
 * arising out of or in connection with the use or performance of this
 * software.
 *
 * $Header: /sprite/src/lib/include/ds3100.md/RCS/limits.h,v 1.1 89/07/08 14:55:49 nelson Exp Locker: jhh $ SPRITE (Berkeley)
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
#define CHAR_MAX		127

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
