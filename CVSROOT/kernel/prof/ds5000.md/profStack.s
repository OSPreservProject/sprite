/* 
 * profStack.s --
 *
 * Procedures for managing stack traces.
 *
 * Prof_ThisFP --
 *	Return the frame pointer of our caller. Since the frame pointer is
 *	not interesting to us on the SPUR, we return 0.
 *
 * Prof_CallerFP --
 *	Return the frame pointer of our caller's caller. Since the frame 
 *	pointer is not interesting to us on the SPUR, we return 1. This 
 *	
 *
 * Prof_ThisPC --
 *	Given a frame pointer, return the PC from which this
 *	frame was called. This must be called with interrupts off!
 *
 * Copyright (C) 1989 Digital Equipment Corporation.
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appears in all copies.  
 * Digital Equipment Corporation makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 *  $Header$ SPRITE (DECWRL)
 */

#include "machConst.h"
#include <regdef.h>

	.globl	_Prof_ThisFP
	.globl	_Prof_CallerFP
	.globl	_Prof_ThisPC

_Prof_ThisFP:
	j	ra

_Prof_CallerFP:
	j	ra

_Prof_ThisPC:
	j	ra
