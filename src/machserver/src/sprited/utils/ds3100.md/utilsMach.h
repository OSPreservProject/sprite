/*
 * utilsMach.h --
 *
 *	Declarations for random DECstation-dependent stuff.
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
 * $Header: /r3/kupfer/spriteserver/src/sprited/utils/ds3100.md/RCS/utilsMach.h,v 1.1 91/11/14 10:17:46 kupfer Exp $ SPRITE (Berkeley)
 */

#ifndef _UTILSMACH
#define _UTILSMACH

#include <sprite.h>

extern Address UtilsMach_GetPC();


/*
 *----------------------------------------------------------------------
 *
 * UtilsMach_GetCallerPC --
 *
 *	Supposed to return the PC of the caller of the current routine.
 *	The MIPS machines don't have a frame pointer, so punt.
 *
 * Results:
 *	Returns 0.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

#define UtilsMach_GetCallerPC() 	0


/*
 *----------------------------------------------------------------------
 *
 * UtilsMach_Delay --
 *
 *	Delay for N microseconds.  This is currently the same for both the 
 *	ds3100 and ds5000.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
    
#define	UtilsMach_Delay(n) \
	{ register int N = (n) * 6; while (N > 0) {N--;} }

/* 
 * The cc 1.31 optimizer for the DECstation optimizes away static functions 
 * that are passed to Proc_NewProc, unless the function is preceded by the 
 * following cast, which protects it.
 */

#define UTILSMACH_MAGIC_CAST	(unsigned)(void (*)())

#endif /* _UTILSMACH */
