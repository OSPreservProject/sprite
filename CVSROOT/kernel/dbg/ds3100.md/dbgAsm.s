/*
 * dbgAsm.s --
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
 * $Header$ SPRITE (DECWRL)
 */

#include <regdef.h>

/*----------------------------------------------------------------------------
 *
 * Dbg_Call --
 *
 *	Trap to the debugger.
 *
 * Results:
 *     	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------------
 */
    .globl Dbg_Call
    .ent Dbg_Call, 0
Dbg_Call:
    .frame	sp, 0, ra
    break	0
    j		ra
    .end	Dbg_Call

