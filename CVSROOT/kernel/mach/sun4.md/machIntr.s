/*
 * machIntr.s --
 *
 *	Interrupts for sun4.
 *
 * Copyright 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

.seg	"data"
.asciz	"$Header$ SPRITE (Berkeley)"
.align	8
.seg	"text"

#include "machConst.h"
#include "machAsmDefs.h"

.align	8
.seg	"text"

/*
 * ----------------------------------------------------------------------
 *
 * MachHandleInterrupt --
 *
 *	Handle an interrupt by calling the specific interrupt handler whose
 *	address is given as our first (and only) argument.
 *
 *	MachHandleInterrupt(SpecificInterruptHandlerAddress)
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------
 */
.globl	MachHandleInterrupt
MachHandleInterrupt:
	/* set us at interrupt level - do this in trap handler?? */
	set	_mach_AtInterruptLevel, %VOL_TEMP1
	ld	[%VOL_TEMP1], %SAFE_TEMP
	set	1, %VOL_TEMP2
	st	%VOL_TEMP2, [%VOL_TEMP1]
	mov	%psr, %OUT_TEMP1
	MACH_DEBUG_BUF(%VOL_TEMP1, %VOL_TEMP2, InterruptDebugLabel1, %OUT_TEMP1)
	mov	%o0, %OUT_TEMP1
	MACH_DEBUG_BUF(%VOL_TEMP1, %VOL_TEMP2, InterruptDebugLabel2, %OUT_TEMP1)
	call	%o0				/* call specific handler */
	nop

	set	_mach_AtInterruptLevel, %VOL_TEMP1
#ifdef NOTDEF
	tst	%SAFE_TEMP
	bne	LeaveInterruptLevel
#endif NOTDEF
	nop
	st	%g0, [%VOL_TEMP1]
LeaveInterruptLevel:
	set	_MachReturnFromTrap, %VOL_TEMP1
	jmp	%VOL_TEMP1
	nop
