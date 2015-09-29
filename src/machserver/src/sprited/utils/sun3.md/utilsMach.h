/*
 * utilsMach.h --
 *
 *	Declarations for random sun3-dependent stuff.
 *
 * Copyright 1991 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that this copyright
 * notice appears in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /r3/kupfer/spriteserver/src/sprited/utils/sun3.md/RCS/utilsMach.h,v 1.5 91/11/14 10:20:23 kupfer Exp $ SPRITE (Berkeley)
 */

#ifndef _UTILSMACH
#define _UTILSMACH

#include <sprite.h>


/*
 *----------------------------------------------------------------------
 *
 * UtilsMach_GetPC --
 *
 *	Returns the PC of the current instruction.
 *
 * Results:
 *	Current PC
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

#ifdef lint
#define UtilsMach_GetPC() 	0
#else
#define UtilsMach_GetPC() \
    ({\
	register Address __pc; \
	asm volatile ("1:\n\tlea\t1b,%0\n":"=a" (__pc));\
	(__pc);\
    })
#endif


/*
 *----------------------------------------------------------------------
 *
 * UtilsMach_GetCallerPC --
 *
 *	Returns the PC of the caller of the current routine.
 *
 * Results:
 *	Our caller's PC.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

#ifdef lint
#define UtilsMach_GetCallerPC() 	0
#else
#define UtilsMach_GetCallerPC() \
    ({\
	register Address __pc; \
	asm volatile ("\tmovl a6@(4),%0\n":"=a" (__pc));\
	__pc;\
    })
#endif


/*
 *----------------------------------------------------------------------
 *
 * UtilsMach_Delay --
 *
 *	Delay for N microseconds.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

#ifdef sun3
#define	UtilsMach_Delay(n) \
	{ register int N = (n)<<1; N--; while (N > 0) {N--;} }
#else
#define	UtilsMach_Delay(n) \
	{ register int N = (n)>>1; N--; while (N > 0) {N--;} }
#endif

/* Workaround for DECstation compiler bug. */
#define UTILSMACH_MAGIC_CAST

#endif /* _UTILSMACH */
