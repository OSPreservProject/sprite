/*
 * machTrap.s --
 *
 *	Traps for sun4.
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
 * MachWindowOverflow --
 *
 *	Window overflow handler.  It's set up so that it can be called as a
 *	result of a window overflow trap or as a result of moving into an
 *	invalid window as a result of some other trap or interrupt.
 *	The address of the calling instruction is stored in %SAFE_TEMP.
 *
 *	The window we've trapped into is currently invalid.  We want to
 *	make it valid.  We do this by moving one window further, saving that
 *	window to the stack, marking it as the new invalid window, and then
 *	moving back to the window that we trapped into.  It is then valid
 *	and usable.  Note that we move first to the window to save and then
 *	mark it invalid.  In the other order we would get another overflow
 *	trap, but with traps turned off...
 *	Note that %sp should point to highest (on stack, lowest in memory)
 *	usable word in the current stack frame.  %fp is the %sp of the
 *	caller's stack frame, so the first (highest in memory) usable word
 *	of a current stack frame is (%fp - 4).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The stack grows.
 *
 * ----------------------------------------------------------------------
 */
.globl	MachWindowOverflow
MachWindowOverflow:
	/*
	 * We enter inside of an invalid window, so we can't use in registers,
	 * out registers or global registers to begin with.
	 * We temporarily clear out some globals since we won't be able to
	 * use locals in window we move to before saving it.  Using them would
	 * mess them up for window's owner.
	 */
	mov	%g3, %VOL_TEMP1
	mov	%g4, %VOL_TEMP2
	save				/* move to the window to save */
	MACH_ADVANCE_WIM(%g3, %g4)	/* reset %wim to past current window */
	/*
	 * save this window to stack - save locals and ins to top 16 words
	 * on the stack. (Since our stack grows down, the top word is %sp
	 * and the bottom will be (%sp + offset).
	 */
	MACH_SAVE_WINDOW_TO_STACK()
	restore				/* move back to trap window */
	mov	%VOL_TEMP1, %g3		/* restore global registers */
	mov	%VOL_TEMP2, %g4

	/*
	 * jump to calling routine - this may be a trap-handler or not.
	 */
	jmp	%SAFE_TEMP + 8
	nop


/*
 * ----------------------------------------------------------------------
 *
 * MachWindowUnderflow --
 *
 *	Window underflow trap handler.
 *
 *	Unlike MachWindowOverflow, this routine can only be entered as a
 *	result of taking an underflow trap.  This is because we can check for
 *	an overflow condition on traps and deal with it in the same way we
 *	do when we get a real overflow trap, but the underflow situation is not
 *	symmetrical.  On an underflow trap we enter 2 windows away from
 *	the window that is invalid.  When returning from traps we must check
 *	for an underflow condition, but we'll only be one window away from
 *	the invalid window.  For that situation, we have another routine.
 *	Maybe I should combine them and flag the difference inside the routine,
 *	but I didn't.
 *
 *	For an underflow, we tried to retreat to an invalid window and couldn't,
 *	so we trapped.  Trapping advances the current window, so we are 2
 *	windows away from the invalid window.  We want to restore that invalid
 *	window.  First we mark the window behind the invalid window as the
 *	new invalid window, and then we move to the invalid window.  Then we
 *	restore data from the stack into the invalid window.  Then we return
 *	to our trap window, 2 windows away again.  Note that we first mark
 *	the new invalid window and then move to the old invalid window.  If
 *	we did this in the other order, we'd get another window underflow trap.
 *	(This is the opposite order from window overflow.)
 *
 *	For the window to restore, the %sp should be good since it's the %fp
 *	of the window it called.  I hope I can count on this...  (This won't
 *	be true with system and user windows mixed, but this is just for system
 *	code right now.)
 *
 *	The %sp should point to highest (on stack, lowest in memory)
 *	usable word in the current stack frame.  %fp is the %sp of the
 *	caller's stack frame, so the first (highest in memory) usable word
 *	of a current stack frame is (%fp - 4).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The stack shrinks.
 *
 * ----------------------------------------------------------------------
 */
.globl	MachWindowUnderflow
MachWindowUnderflow:
	/*
	 * It should be ok to use locals here - it's a dead window.
	 * Note that this means one cannot do a restore and then a save
	 * and expect that the old locals in the dead window are still the
	 * same!
	 */
	/* mark new invalid window */
	MACH_RETREAT_WIM(%VOL_TEMP1, %VOL_TEMP2, UnderflowLabel)
	restore				/* move to window to restore */
	restore
	/* restore data from stack to window */
	MACH_RESTORE_WINDOW_FROM_STACK()
	/* Move back to trap window with 2 saves.  Clear registers too??? */
	save
	save
	/* jump to return from trap routine */
	set	_MachReturnFromTrap, %VOL_TEMP1
	jmp	%VOL_TEMP1
	nop


/*
 * ----------------------------------------------------------------------
 *
 * MachDealWithWindowUnderflow --
 *
 *	Deal with a window underflow condition on returning from traps.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The window invalid mask changes.
 *
 * ----------------------------------------------------------------------
 */
.globl	MachDealWithWindowUnderflow
MachDealWithWindowUnderflow:

	/* mark new invalid window */
	MACH_RETREAT_WIM(%VOL_TEMP1, %VOL_TEMP2, DealUnderflowLabel)
	restore
	/* restore data from stack to window */
	MACH_RESTORE_WINDOW_FROM_STACK()
	save
	/* return to where we were. */
	retl
	nop

/*
 * ----------------------------------------------------------------------
 *
 * MachTrap --
 *
 *	Jump to system trap table.
 *
 *	Use of registers:  %VOL_TEMP1 is trap type and then the place to jump
 *	to.  %VOL_TEMP2 is the address of my trap table to reset the %tbr with.
 *
 *	The old system %tbr has been stored in %TBR_REG.
 *	
 *	Currently this does a bunch of compares to see where to go.  When I
 *	fill out the real trap table, that won't be necessary.  Instead, I
 *	can jump directly to the handlers, or I can make this more of a
 *	generic preamble.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------
 */
.globl	_MachTrap
_MachTrap:
	rd	%tbr, %VOL_TEMP1
	and	%VOL_TEMP1, MACH_TRAP_TYPE_MASK, %VOL_TEMP1 /* get trap type */

	cmp	%VOL_TEMP1, MACH_WINDOW_OVERFLOW	/* my overflow? */
	be	MachHandleWindowOverflowTrap
	rd	%psr, %CUR_PSR_REG
	cmp	%VOL_TEMP1, MACH_WINDOW_UNDERFLOW	/* my underflow? */
	be	MachWindowUnderflow
	rd	%psr, %CUR_PSR_REG
#ifdef NOTDEF
	cmp	%VOL_TEMP1, MACH_LEVEL14_INT		/* clock interrupt */
	be	MachHandleInterrupt
	rd	%psr, %CUR_PSR_REG
#endif NOTDEF
							/* no - their stuff */
	add	%VOL_TEMP1, %TBR_REG, %VOL_TEMP1 /* add t.t. to real tbr */
	jmp	%VOL_TEMP1		/* jmp (non-pc-rel) to real tbr */
	nop




/*
 * ----------------------------------------------------------------------
 *
 * MachReturnFromTrap --
 *
 *	Restore old %psr from %CUR_PSR_REG.   Then jump
 *	to where we were when we got a trap, re-enabling traps.
 *	NOTE: this restores old psr to what it was, except for its current
 *	window pointer bits.  These we take from the current psr, in case
 *	we're in a different window now (which can happen after context
 *	switches).
 *
 * Results:
 *	None.
 * * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------
 */
.globl	_MachReturnFromTrap
_MachReturnFromTrap:
	/* restore psr */
	mov	%CUR_PSR_REG, %VOL_TEMP2;	/* get old psr */
	set	(~MACH_CWP_BITS), %VOL_TEMP1;	/* clear only its cwp bits */
	and	%VOL_TEMP2, %VOL_TEMP1, %VOL_TEMP2;
	mov	%psr, %VOL_TEMP1;		/* get current psr */
	and	%VOL_TEMP1, MACH_CWP_BITS, %VOL_TEMP1;	/* take only its cwp */
	or	%VOL_TEMP2, %VOL_TEMP1, %VOL_TEMP2;	/* put cwp on old psr */
	mov	%VOL_TEMP2, %psr
	MACH_UNDERFLOW_TEST()
	be	UnderflowOkay
	nop
	call	MachDealWithWindowUnderflow
	nop
UnderflowOkay:
	jmp	%CUR_PC_REG
	rett	%NEXT_PC_REG
	nop

/*
 * ----------------------------------------------------------------------
 *
 * MachHandleWindowOverflowTrap --
 *
 *	Trap entrance to the window overflow handler.  This sets up a return
 *	address, calls the overflow handler, and then goes to
 *	MachReturnFromTrap.  This is set up so that we can just call the
 *	overflow handler even if it's not from a trap, and the right thing
 *	should happen.
 *
 * Results:
 *	None.
 * * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------
 */
.globl	MachHandleWindowOverflowTrap
MachHandleWindowOverflowTrap:
	set	MachWindowOverflow, %VOL_TEMP1
	jmpl	%VOL_TEMP1, %SAFE_TEMP
	nop
	set	_MachReturnFromTrap, %VOL_TEMP1
	jmp	%VOL_TEMP1
	nop

