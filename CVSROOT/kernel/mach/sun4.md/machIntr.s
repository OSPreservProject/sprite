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
 *	For now, this only takes us to clock interrupts.
 *	We have to indirect this way so that we come back to the return from
 *	trap routine.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------
 */
.globl	MachHandleTimerInterrupt
MachHandleTimerInterrupt:
	/* set us at interrupt level - do this in trap handler?? */
	set	_mach_AtInterruptLevel, %VOL_TEMP1
	ld	[%VOL_TEMP1], %SAFE_TEMP
	set	1, %VOL_TEMP2
	st	%VOL_TEMP2, [%VOL_TEMP1]
	set	_Timer_TimerServiceInterrupt, %VOL_TEMP1
	call	%VOL_TEMP1
	nop

	set	_mach_AtInterruptLevel, %VOL_TEMP1
	tst	%SAFE_TEMP
	bne	TimerLeaveIntrLevel
	nop
	st	%g0, [%VOL_TEMP1]
TimerLeaveIntrLevel:
	set	_MachReturnFromTrap, %VOL_TEMP1
	jmp	%VOL_TEMP1
	nop

.globl	MachHandleEtherInterrupt
MachHandleEtherInterrupt:
	set	_mach_AtInterruptLevel, %VOL_TEMP1
	ld	[%VOL_TEMP1], %SAFE_TEMP
	set	1, %VOL_TEMP2
	st	%VOL_TEMP2, [%VOL_TEMP1]
	set	_NetIEIntr, %VOL_TEMP1
	set	0, %o0				/* argument "FALSE" */
	call	%VOL_TEMP1, 1
	nop

	set	_mach_AtInterruptLevel, %VOL_TEMP1
	tst	%SAFE_TEMP
	bne	EtherLeaveIntrLevel
	nop
	st	%g0, [%VOL_TEMP1]
EtherLeaveIntrLevel:
	set	_MachReturnFromTrap, %VOL_TEMP1
	jmp	%VOL_TEMP1
	nop


.globl	MachHandleLevel0Interrupt
MachHandleLevel0Interrupt:
	set	~0xFFF, %VOL_TEMP1
	and	%CUR_TBR_REG, %VOL_TEMP1, %CUR_TBR_REG
	ba	MachHandleLevel0Interrupt
	nop
