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
#include "vmSunConst.h"
#include "devAddrs.h"

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

	and	%CUR_PSR_REG, MACH_PS_BIT, %VOL_TEMP1
	set	_mach_KernelMode, %VOL_TEMP2
	st	%VOL_TEMP1, [%VOL_TEMP2]	/* 0 = user, 1 = kernel */

	/* Call into vector table using tbr */
	and	%CUR_TBR_REG, MACH_TRAP_TYPE_MASK, %o0
	sub	%o0, MACH_LEVEL0_INT, %o0	/* get interrupt level */
	srl	%o0, 2, %VOL_TEMP1		/* convert to index */
	set	_machInterruptArgs, %VOL_TEMP2
	add	%VOL_TEMP2, %VOL_TEMP1, %VOL_TEMP2
	ld	[%VOL_TEMP2], %o0		/* arg, if any */
	/*
	 * For now, this is the only way to get the interrupt pc to the
	 * profiler via the Timer_TimerService callback.  It's an implicit
	 * parameter.
	 */
	mov	%CUR_PC_REG, %o1	/* pc as next arg. */
	set	_machVectorTable, %VOL_TEMP2
	add	%VOL_TEMP2, %VOL_TEMP1, %VOL_TEMP1
	ld	[%VOL_TEMP1], %VOL_TEMP1
	call	%VOL_TEMP1
	nop

	set	_mach_AtInterruptLevel, %VOL_TEMP1
#ifdef NOTDEF
	tst	%SAFE_TEMP
	bne	LeaveInterruptLevel
#endif NOTDEF
	nop
	st	%g0, [%VOL_TEMP1]
LeaveInterruptLevel:
	/*
	 * Put a good return value into the return value register so that
	 * MachReturnFromTrap will be happy if we're returning to user mode.
	 */
	mov	MACH_OK, %RETURN_VAL_REG

	set	_MachReturnFromTrap, %VOL_TEMP1
	jmp	%VOL_TEMP1
	nop



/*
 * ----------------------------------------------------------------------
 *
 * MachVectoredInterrupt --
 *
 *	Handle an interrupt that requires getting an interrupt vector in
 *	an interrupt acknowledge cycle.  The single argument to the routine is
 *	the vme trap vector address to read.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Handle the interrupt.
 *
 * ----------------------------------------------------------------------
 */
.globl	_MachVectoredInterrupt
_MachVectoredInterrupt:
	/* We need to return to a leaf routine, so we need to save a frame */
	save	%sp, -MACH_FULL_STACK_FRAME, %sp
	lduba	[%i0] VMMACH_CONTROL_SPACE, %VOL_TEMP1	/* got vector */
	sll	%VOL_TEMP1, 2, %VOL_TEMP1		/* convert to index */
	set	_machInterruptArgs, %VOL_TEMP2
	add	%VOL_TEMP2, %VOL_TEMP1, %VOL_TEMP2
	ld	[%VOL_TEMP2], %o0			/* clientData arg */
	set	_machVectorTable, %VOL_TEMP2
	add	%VOL_TEMP2, %VOL_TEMP1, %VOL_TEMP2
	ld	[%VOL_TEMP2], %VOL_TEMP2
	call	%VOL_TEMP2				/* %o0 is arg */
	nop

	ret
	restore

/*
 * ----------------------------------------------------------------------
 *
 * MachHandleLevel15Intr --
 *
 *	Handle a level 15 interrrupt.  This is a non-maskable memory error
 *	interrupt, and we want to clear the condition so we report only
 *	the first CE.  Then we'll jump to MachTrap and go into the debugger.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	We should go into the debugger.
 *
 * ----------------------------------------------------------------------
 */
.globl	MachHandleLevel15Intr
MachHandleLevel15Intr:
	/*
	 * Put the register values into global registers so we can see
	 * what they were easily when entering the debugger, since we'll
	 * mess up the locals before we get there.  We won't survive this
	 * error anyway, so we won't be needing the globals again.
	 */
#ifdef sun4c
	set	VMMACH_ASYNC_ERROR_REG, %VOL_TEMP1
	lda	[%VOL_TEMP1] VMMACH_CONTROL_SPACE, %g5

	set	VMMACH_ASYNC_ERROR_ADDR_REG, %VOL_TEMP1
	lda	[%VOL_TEMP1] VMMACH_CONTROL_SPACE, %g6

	/* I must clear these too to clear async error.  Very silly. */
	set	VMMACH_SYNC_ERROR_REG, %VOL_TEMP1
	lda	[%VOL_TEMP1] VMMACH_CONTROL_SPACE, %g0

	set	VMMACH_SYNC_ERROR_ADDR_REG, %VOL_TEMP1
	lda	[%VOL_TEMP1] VMMACH_CONTROL_SPACE, %g0

	/*
	 * To clear the interrupt condition, first write a 0 to the enable
	 * all interrupts bit, and then write a one again.
	 */
	set	DEV_INTERRUPT_REG_ADDR, %VOL_TEMP1
	ld	[%VOL_TEMP1], %VOL_TEMP2
	andn	%VOL_TEMP2, MACH_ENABLE_ALL_INTERRUPTS, %VOL_TEMP2
	st	%VOL_TEMP2, [%VOL_TEMP1]
	or	%VOL_TEMP2, MACH_ENABLE_ALL_INTERRUPTS, %VOL_TEMP2
	st	%VOL_TEMP2, [%VOL_TEMP1]
#else
	set	VMMACH_ADDR_CONTROL_REG, %VOL_TEMP1
	ld	[%VOL_TEMP1], %g5

	and	%VOL_TEMP2, ~VMMACH_ENABLE_MEM_ERROR_BIT, %VOL_TEMP2
	st	%VOL_TEMP2, [%VOL_TEMP1]
	set	VMMACH_ADDR_ERROR_REG, %VOL_TEMP1
	ld	[%VOL_TEMP1], %g6
#endif
	set	_MachTrap, %VOL_TEMP1
	jmp	%VOL_TEMP1
	nop
