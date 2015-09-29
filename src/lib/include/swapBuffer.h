/*
 * swapBuffer.h --
 *
 *	Declarations of public routines for byte-swapping data and
 *	calculating byte-swapped data size.
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
 * $Header: /sprite/src/lib/include/RCS/swapBuffer.h,v 1.3 90/03/29 12:45:03 mendel Exp $ SPRITE (Berkeley)
 */

#ifndef _SWAP
#define _SWAP

/* constants */

/* data structures */

/* procedures */

#define SWAP_SUN_TYPE	0
#define	SWAP_VAX_TYPE	1
#define	SWAP_SPUR_TYPE	2
#define	SWAP_SPARC_TYPE	SWAP_SUN_TYPE

extern	void	Swap_Buffer();
extern	void	Swap_BufSize();

#endif /* _SWAP */
