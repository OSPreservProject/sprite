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
 *  $Header$ SPRITE (Berkeley)
 *
 * Copyright 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.

 */

#include "machConst.h"

	.globl	_Prof_ThisFP
	.globl	_Prof_CallerFP
	.globl	_Prof_ThisPC

_Prof_ThisFP:
	add_nt	r11,r0,$0	/* just return a zero. */
	invalidate_ib
	return	r10,$8
	Nop

_Prof_CallerFP:
	add_nt	r11,r0,$1	/* just return a one. */
	invalidate_ib
	return	r10,$8
	Nop

_Prof_ThisPC:
	add_nt		r9, r10, $0	/* Save were to return to in r12. */
			
	invalidate_ib
        rd_special      r20, pc		/* Go back into our caller's window. */
        return          r20, $12
        Nop
	cmp_br_delayed	ne, r27, r0, 1f	/* Do we not want this window? */
	Nop
	/* 
	 * Return the return PC to our caller, we're in his window now so
	 * we just use jump_reg
	 */
	add_nt		r27,r10,$0
	jump_reg	r28,$8		/* r28==(our old r12) */
	Nop
1:
	/* Must go back to our caller's caller. This requires interrupts to 
	 * be disabled. It is safe to use our output registers because they
	 * are our original input registers.
	 */
        rd_kpsw         r27
        and             r29, r27, $(~MACH_KPSW_INTR_TRAP_ENA)
        wr_kpsw         r29, $0

	/*
	 * Save the current r10 which gets overwritten by the call we
	 * use to return to this window. 
	 */
	add_nt		r29, r10, $0

	invalidate_ib
        rd_special      r30, pc		/* Go back a window. */
        return          r30, $12
        Nop

	add_nt		r9, r10, $0	/* Save the value we want in r9. */

	/*
	 * Move forward a window.
	 */
	invalidate_ib
        call           2f                  
        Nop
2:
	add_nt		r10, r29, $0	/* Restore r10 */
	wr_kpsw         r27, $0		/* Restore the kpsw. */
	add_nt		r27,r9,$0
	jump_reg	r28,$8
	Nop
