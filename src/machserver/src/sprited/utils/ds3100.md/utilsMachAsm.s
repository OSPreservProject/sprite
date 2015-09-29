/*
 * utilsMachAsm.s --
 *
 *	Contains misc. assembler routines for the PMAX.
 *
 *	Copyright (C) 1989 Digital Equipment Corporation.
 *	Permission to use, copy, modify, and distribute this software and
 *	its documentation for any purpose and without fee is hereby granted,
 *	provided that the above copyright notice appears in all copies.  
 *	Digital Equipment Corporation makes no representations about the
 *	suitability of this software for any purpose.  It is provided "as is"
 *	without express or implied warranty.
 *
 * $Header: /r3/kupfer/spriteserver/src/sprited/utils/ds3100.md/RCS/utilsMachAsm.s,v 1.2 91/11/14 10:18:04 kupfer Exp $ SPRITE (DECWRL)
 */

#include <regdef.h>


/*----------------------------------------------------------------------------
 *
 * Mach_GetPC --
 *
 *	Return the caller's PC.
 *
 * Results:
 *     	The PC of the caller.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------------
 */
    .globl UtilsMach_GetPC
UtilsMach_GetPC:
    add		v0, ra, zero
    j		ra
