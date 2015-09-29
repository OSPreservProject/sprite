/*
 * Copyright (c) 1987 University of Maryland Department of Computer Science.
 * All rights reserved.  Permission to copy for any purpose is hereby granted
 * so long as this copyright notice remains intact.
 */

/*
 * C-TeX types (system dependent).
 */

#ifndef _CTEX_TYPES_
#define _CTEX_TYPES_

/* a 16 bit integer (signed) */
typedef short i16;

/* a 32 bit integer (signed) */
typedef long i32;

/* macros to sign extend quantities that are less than 32 bits long */
#if defined(u3b) || defined(u3b2) || defined(u3b5) || defined(ibm03)
#define Sign8(n)	((n) & (1 << 7) ? ((n) | 0xffffff00) : (n))
#define Sign16(n)	((i32) (short) (n))
#define Sign24(n)	((n) & (1 << 23) ? ((n) | 0xff000000) : (n))
#else
#ifdef sun
/* Sun mishandles (int)(char)(constant), but this subterfuge works: */
#define Sign8(n)	((i32) (char) (int) (n))
#else
#define Sign8(n)	((i32) (char) (n))
#endif /* sun */
#define Sign16(n)	((i32) (short) (n))
#define Sign24(n)	(((n) << 8) >> 8)
#endif /* u3b || u3b2 || u3b5 */

/* macros to truncate quantites that are signed but shouldn't be */
#define UnSign8(n)	((n) & 0xff)
#define UnSign16(n)	((n) & 0xffff)
#define UnSign24(n)	((n) & 0xffffff)

/* note that we never have unsigned 32 bit integers */

#endif /* _CTEX_TYPES_ */
