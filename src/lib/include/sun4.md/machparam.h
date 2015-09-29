/*
 * machparam.h --
 *
 *	This file contains various machine-dependent parameters needed
 *	by UNIX programs running under Sprite.  This file includes parts
 *	of the UNIX header files "machine/machparm.h" and
 *	"machine/endian.h".  Many of things in the UNIX file are only
 *	useful for the kernel;  stuff gets added to this file only
 *	when it's clear that it is needed for user programs.
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
 * $Header: /sprite/src/lib/include/sun4.md/RCS/machparam.h,v 1.3 90/05/02 16:52:42 rab Exp $ SPRITE (Berkeley)
 */

#ifndef _MACHPARAM
#define _MACHPARAM

#ifndef _LIMITS
#include <limits.h>
#endif

/*
 *----------------------
 * Taken from endian.h:
 *----------------------
 */

/*
 * Definitions for byte order,
 * according to byte significance from low address to high.
 */
#define LITTLE_ENDIAN   1234    /* least-significant byte first (vax) */
#define BIG_ENDIAN      4321    /* most-significant byte first (IBM, net) */
#define PDP_ENDIAN      3412    /* LSB first in word, MSW first in long (pdp) */

#define BYTE_ORDER      BIG_ENDIAN   /* byte order on sun3 */

/*
 *----------------------
 * Miscellaneous:
 *----------------------
 */

/*
 * The bits of a address that should not be set if word loads and stores
 * are done on the address. This mask intended for fast byte manipulation
 * routines.
 */

#define	WORD_ALIGN_MASK	0x3

/*
 * Sizes of pages and segments.  SEGSIZ is valid only on Suns, and used
 * only in EMACS, I think.  I'm not sure why it should get used anywhere.
 */

#define PAGSIZ 0x2000
#define SEGSIZ 0x40000

#endif /* _MACHPARAM */
