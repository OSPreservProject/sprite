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
 *	Handle an interrupt.  This means
 *		1) Check to see if we're in an invalid window.  If so, deal
 *		first with window overflow.
 *		2) Now that it's safe to overwrite our out registers, update
 *		the stack pointer.
 *		3) Save the trap state - return from trap pc, psr, etc.
 *		4) Re-enable traps so that if we get another window overflow,
 *		we can handle it.
 *		5) Deal with interrupt.
 *		6) Re-disable traps.
 *		7) Restore the trap state - return from trap pc, psr, etc.
 *		8) Call the usual return from trap routine.
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
	set	_saveCounter, %VOL_TEMP1
	ld	[%VOL_TEMP1], %VOL_TEMP2
	mov	1000, %VOL_TEMP1
	cmp	%VOL_TEMP1, %VOL_TEMP2
	be	finishedHere
	nop
	sll	%VOL_TEMP2, 0x2, %VOL_TEMP2	/* get array offset */
	set	_saveBuffer, %VOL_TEMP1
	add	%VOL_TEMP2, %VOL_TEMP1, %VOL_TEMP2	/* add to array */
	mov	0x0, %VOL_TEMP1
	st	%VOL_TEMP1, [%VOL_TEMP2]
	add	%VOL_TEMP2, 4, %VOL_TEMP2
	mov	%psr, %VOL_TEMP1
	st	%VOL_TEMP1, [%VOL_TEMP2]
	add	%VOL_TEMP2, 4, %VOL_TEMP2
	st	%l0, [%VOL_TEMP2]
	add	%VOL_TEMP2, 4, %VOL_TEMP2
	mov	%wim, %VOL_TEMP1
	st	%VOL_TEMP1, [%VOL_TEMP2]
	set	_saveCounter, %VOL_TEMP1
	ld	[%VOL_TEMP1], %VOL_TEMP2
	add	%VOL_TEMP2, 4, %VOL_TEMP2
	st	%VOL_TEMP2, [%VOL_TEMP1]
finishedHere:
	MACH_INVALID_WINDOW_TEST()
	be	WindowOkay
	nop
	/* deal with window overflow */
	set	MachWindowOverflow, %VOL_TEMP1
	jmpl	%VOL_TEMP1, %SAFE_TEMP
	nop
WindowOkay:
	/* add stack frame to stack pointer */
	mov	%fp, %sp
	set	MACH_SAVED_WINDOW_SIZE, %VOL_TEMP1
	sub	%sp, %VOL_TEMP1, %sp
	MACH_SAVE_TRAP_STATE()
	MACH_SR_HIGHPRIO()		/* turn on traps, off interrupts */
	/*
	 * Deal with interrupt - for now we don't look at trap type, since
	 * we know it can only be a clock interrupt.
	 */
	set	_Timer_TimerServiceInterrupt, %VOL_TEMP1
	call	%VOL_TEMP1
	nop
	/*
	 * The read psr, write psr sequence is interruptible, but this should
	 * be okay here since we have all but non-maskable interrupts disabled
	 * here...
	 */
	MACH_DISABLE_TRAPS()
	/*
	 * There's some redundancy here.  MACH_RESTORE_TRAP_STATE also writes
	 * the psr.  It puts into it the value it had above during
	 * MACH_SAVE_TRAP_STATE, except that it doesn't change the current
	 * CWP bits in case we're in a different window.  In MachReturnFromTrap,
	 * we move the %CUR_PSR_REG back into the psr, also taking care not
	 * to change the CWP bits.  This seems like extra work someplace.
	 */
	MACH_RESTORE_TRAP_STATE()
	set	_saveCounter, %VOL_TEMP1
	ld	[%VOL_TEMP1], %VOL_TEMP2
	mov	1000, %VOL_TEMP1
	cmp	%VOL_TEMP1, %VOL_TEMP2
	be	finished2
	nop
	sll	%VOL_TEMP2, 0x2, %VOL_TEMP2	/* get array offset */
	set	_saveBuffer, %VOL_TEMP1
	add	%VOL_TEMP2, %VOL_TEMP1, %VOL_TEMP2	/* add to array */
	mov	0x1, %VOL_TEMP1
	st	%VOL_TEMP1, [%VOL_TEMP2]
	add	%VOL_TEMP2, 4, %VOL_TEMP2
	mov	%psr, %VOL_TEMP1
	st	%VOL_TEMP1, [%VOL_TEMP2]
	add	%VOL_TEMP2, 4, %VOL_TEMP2
	st	%l0, [%VOL_TEMP2]
	add	%VOL_TEMP2, 4, %VOL_TEMP2
	mov	%wim, %VOL_TEMP1
	st	%VOL_TEMP1, [%VOL_TEMP2]
	set	_saveCounter, %VOL_TEMP1
	ld	[%VOL_TEMP1], %VOL_TEMP2
	add	%VOL_TEMP2, 4, %VOL_TEMP2
	st	%VOL_TEMP2, [%VOL_TEMP1]
finished2:
	set	_MachReturnFromTrap, %VOL_TEMP1
	jmp	%VOL_TEMP1
	nop
