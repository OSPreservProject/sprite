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
 * MachTrap --
 *
 *	Jump to system trap table or my traps.
 *	NOTE:  This will change when I handle all the traps.  Then window
 *	overflow and underflow will go directly from trap vector to their
 *	routines, and this won't need the first few lines for overflow and
 *	underflow. Also, currently this saves only the psr for window overflow
 *	and underflow traps.  If they end up doing multiplies, I'll need to
 *	save the y register.  I've coded them to avoid globals and in registers,
 *	so I don't need to save that.  This should make overflow and underflow
 *	as fast as possible.  Window overflow and underflow are constrained
 *	to keep traps off the whole time, then.
 *
 *	The old system %tbr has been stored in %TBR_REG, a global register,
 *	temporarily for borrowing prom trap routines I haven't written yet..
 *
 *	The scheme of things (except for overflow, underflow, and other "fast"
 *	stuff):
 *
 *	1) Check to see if we're in an invalid window.  If so, deal first with
 *	window overflow.
 *	2) Now that it's safe to overwrite our out registers, update the stack
 *	pointer.  If coming from user mode, this means pulling the kernel
 *	stack pointer out of the state structure and adding the appropriate
 *	amount.  Otherwise, it means just means adding the appropriate amount to
 *	our %fp.
 *	3) Save the rest of the trap state (globals) to the stack.  Note that
 *	although locals and ins contain trap state (ins are our caller's outs
 *	which we shouldn't mess up, and the psr and other state registers are
 *	in our locals) we need not save them explicitly, since they will be
 *	saved as a part of the normal window overflow and underflow.  If this
 *	is a debugger trap or context switch, then we must make sure they
 *	are saved, however, since they will be needed.  This is done as a
 *	a set of saves and restores across the windows.
 *	4) Re-enable traps, so that if we get another overflow or underflow,
 *	we'll be able to deal with it.  MAYBE THIS IS A BAD IDEA?  Maybe this
 *	should be up to the handlers?  I'll find out soon enough.
 *	5) Figure out what handler to call and call it.
 *
 *	The handler must call our return from trap post-amble.
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
	/*
	 * Save the state registers.  This is safe, since we're saving them
	 * to local registers.
	 * The psr is saved here for now, but maybe it should be saved in
	 * delay slot of the trap table??
	 * The pc and npc were saved into their regs as part of the trap
	 * instruction.
	 */
	mov	%psr, %CUR_PSR_REG
	mov	%tbr, %CUR_TBR_REG
	mov	%y, %CUR_Y_REG
    /* for debugging */
	set	_debugCounter, %VOL_TEMP1
	ld	[%VOL_TEMP1], %VOL_TEMP1
	set	500, %VOL_TEMP2
	subcc	%VOL_TEMP2, %VOL_TEMP1, %g0
	ble	FinishedDebugging
	nop
	sll	%VOL_TEMP1, 2, %VOL_TEMP1
	set	_debugSpace, %VOL_TEMP2
	add	%VOL_TEMP2, %VOL_TEMP1, %VOL_TEMP2
	st	%g0, [%VOL_TEMP2]
	st	%CUR_TBR_REG, [%VOL_TEMP2 + 4]
	st	%CUR_PC_REG, [%VOL_TEMP2 + 8]
	st	%fp, [%VOL_TEMP2 + 12]
	set	_debugCounter, %VOL_TEMP1
	ld	[%VOL_TEMP1], %VOL_TEMP2
	add	%VOL_TEMP2, 4, %VOL_TEMP2
	st	%VOL_TEMP2, [%VOL_TEMP1]
FinishedDebugging:
	/*
	 * Are we in an invalid window? If so, deal with it.
	 */
	MACH_INVALID_WINDOW_TEST()
	be	WindowOkay
	nop
	/* deal with window overflow - put return addr in SAFE_TEMP */
	set	MachWindowOverflow, %VOL_TEMP1
	jmpl	%VOL_TEMP1, %SAFE_TEMP
	nop
WindowOkay:
	/*
	 * Now that our out registers are safe to use, update our stack
	 * pointer.  If coming from user mode, I need to get sp from state
	 * instead.  I don't do that yet.
	 */
	mov	%fp, %sp
	set	MACH_SAVED_STATE_FRAME, %VOL_TEMP1
	sub	%sp, %VOL_TEMP1, %sp
	andn	%sp, 0x7, %sp			/* double-word aligned */


	/*
	 * For now, this only saves the globals to the stack.  The locals
	 * and ins make it there through normal overflow and underflow.  If
	 * that turns out nasty, then make this macro save them explicitly.
	 */
	MACH_SAVE_TRAP_STATE()
	/*
	 * If we came from user mode, put a pointer to our saved stack from
	 * into the state structure.  I don't do this yet.
	 */
	MACH_SR_HIGHPRIO()	/* traps on, maskable interrupts off */
	and	%CUR_TBR_REG, MACH_TRAP_TYPE_MASK, %VOL_TEMP1 /* get trap */
	cmp	%VOL_TEMP1, MACH_LEVEL10_INT		/* clock interrupt */
	be	MachHandleTimerInterrupt
	nop
	cmp	%VOL_TEMP1, MACH_LEVEL14_INT
	be	MachHandleTimerInterrupt
	nop
	cmp	%VOL_TEMP1, MACH_LEVEL6_INT		/* ether interrupt */
	be	MachHandleEtherInterrupt
	nop
	cmp	%VOL_TEMP1, 0x100			/* level 0 int */
	be	MachHandleLevel0Interrupt
	nop
	cmp	%VOL_TEMP1, MACH_TRAP_DEBUGGER		/* enter debugger */
	be	MachHandleDebugTrap
	nop
	be	MachHandleDebugTrap		/* all  others to debugger */
	nop


/*
 * ----------------------------------------------------------------------
 *
 * MachReturnFromTrap --
 *
 *	Go through the inverse of MachTrap.  Handlers cannot return to where
 *	they were called.  They must return here to get trap postamble.
 *
 *	1) Disable traps.  This means the handler we've return from mustn't
 *	have them disabled already.
 *	2) Check for window underflow.  Deal with it if necessary.
 *	3) Restore the trap state.
 *	4) Then jump to where we were when we got a trap, re-enabling traps.
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
	MACH_DISABLE_TRAPS()
	MACH_UNDERFLOW_TEST(testModuloLabel)
	be	UnderflowOkay
	nop
	set	MachWindowUnderflow, %VOL_TEMP1
	jmpl	%VOL_TEMP1, %SAFE_TEMP
	nop
UnderflowOkay:
	MACH_RESTORE_TRAP_STATE()
	/* restore y reg */
	mov	%CUR_Y_REG, %y
	/* restore tbr reg */
	mov	%CUR_TBR_REG, %tbr
	/* restore psr */
	MACH_RESTORE_PSR()
	jmp	%CUR_PC_REG
	rett	%NEXT_PC_REG
	nop

/*
 * ----------------------------------------------------------------------
 *
 * MachHandleWindowOverflowTrap --
 *
 *	Trap entrance to the window overflow handler.  This sets up a return
 *	address, calls the overflow handler, restores the psr, and
 *	returns from the trap.
 *
 *	This is set up so that we can just call the
 *	overflow handler even if it's not from a trap, and the right thing
 *	should happen.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------
 */
.globl	MachHandleWindowOverflowTrap
MachHandleWindowOverflowTrap:
    /* for debugging */
	set	_debugCounter, %VOL_TEMP1
	ld	[%VOL_TEMP1], %VOL_TEMP1
	set	500, %VOL_TEMP2
	subcc	%VOL_TEMP2, %VOL_TEMP1, %g0
	ble	FinishedDebugging1
	nop
	sll	%VOL_TEMP1, 2, %VOL_TEMP1
	set	_debugSpace, %VOL_TEMP2
	add	%VOL_TEMP2, %VOL_TEMP1, %VOL_TEMP2
	st	%CUR_PC_REG, [%VOL_TEMP2]
	st	%fp, [%VOL_TEMP2 + 4]
	set	_debugCounter, %VOL_TEMP1
	ld	[%VOL_TEMP1], %VOL_TEMP2
	add	%VOL_TEMP2, 2, %VOL_TEMP2
	st	%VOL_TEMP2, [%VOL_TEMP1]
FinishedDebugging1:
	set	MachWindowOverflow, %VOL_TEMP1
	jmpl	%VOL_TEMP1, %SAFE_TEMP
	nop
	MACH_RESTORE_PSR()
	jmp	%CUR_PC_REG
	rett	%NEXT_PC_REG
	nop

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
 *	The window invalid mask changes and a register window is saved.
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
	MACH_ADVANCE_WIM(%g3, %g4)	/* reset %wim to current window */
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
 * MachHandleWindowUnderflowTrap --
 *
 *	Trap entrance to the window underflow handler.  This sets up a
 *	return address, retreats a window, calls the underflow handler,
 *	advances back to window in which we entered the routine, restores
 *	the psr, and returns from the trap.  We must retreat a window since
 *	on a trap we are 2 windows away from the window to restore, but
 *	on returning from a trap and checking if we need to restore a window,
 *	we are only one window away.  See the comments in MachWindowUnderflow()
 *	for more details.
 *	Because we must do some work in the window we were executing in when
 *	we took a trap, we must make sure to save and restore the registers
 *	we use there.  This means clearing two globals out for this purpose.
 *	We save and restore the globals in the actual trap window, since we
 *	know we can use the local registers there freely.  The underflow
 *	routine expects its return address in %SAFE_TEMP, so this is the
 *	register we must save and restore in the inbetween window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------
 */
.globl	MachHandleWindowUnderflowTrap
MachHandleWindowUnderflowTrap:
    /* for debugging */
	set	_debugCounter, %VOL_TEMP1
	ld	[%VOL_TEMP1], %VOL_TEMP1
	set	500, %VOL_TEMP2
	subcc	%VOL_TEMP2, %VOL_TEMP1, %g0
	ble	FinishedDebugging2
	nop
	sll	%VOL_TEMP1, 2, %VOL_TEMP1
	set	_debugSpace, %VOL_TEMP2
	add	%VOL_TEMP2, %VOL_TEMP1, %VOL_TEMP2
	st	%CUR_PC_REG, [%VOL_TEMP2]
	st	%fp, [%VOL_TEMP2 + 4]
	set	_debugCounter, %VOL_TEMP1
	ld	[%VOL_TEMP1], %VOL_TEMP2
	add	%VOL_TEMP2, 2, %VOL_TEMP2
	st	%VOL_TEMP2, [%VOL_TEMP1]
FinishedDebugging2:
	mov	%g3, %VOL_TEMP1			/* clear out globals */
	mov	%g4, %VOL_TEMP2
	restore					/* retreat a window */
	mov	%SAFE_TEMP, %g3			/* put safe_temp in global */
	set	MachWindowUnderflow, %g4	/* put address in global */
	jmpl	%g4, %SAFE_TEMP
	nop
	mov	%g3, %SAFE_TEMP			/* restore safe_temp */
	save					/* back to trap window */
	mov	%VOL_TEMP1, %g3			/* restore globals */
	mov	%VOL_TEMP2, %g4
	MACH_RESTORE_PSR()			/* restore psr */
	jmp	%CUR_PC_REG			/* return from trap */
	rett	%NEXT_PC_REG
	nop


/*
 * ----------------------------------------------------------------------
 *
 * MachWindowUnderflow --
 *
 *	Window underflow handler.  It's set up so that it can be called as a
 *	result of a window underflow trap or as a result of needing to restore
 *	an invalid window before returning from a trap or interrupt.
 *	The address of the calling instruction is stored in %SAFE_TEMP.
 *
 *	The window we are in when we enter is one beyond the invalid window we
 *	need to restore. On an underflow trap we enter the kernel 2 windows
 *	away from the window that is invalid.  (This is because we tried to
 *	retreat to an invalid window and couldn't, so we trapped.  Trapping
 *	advances the current window, so we are 2 windows away from the invalid
 *	window.)  From the trap handler, then, we must retreat one window
 *	before calling this routine so that we're only one window away from
 *	the invalid window.  Then after returning from here, we must do a
 *	save to get back to our old trap window.
 *
 *	First we mark the window behind the invalid window as the
 *	new invalid window, and then we move to the invalid window.  Then we
 *	restore data from the stack into the invalid window.  Then we return
 *	to our previous window.  Note that we first mark
 *	the new invalid window and then move to the old invalid window.  If
 *	we did this in the other order, we'd get another window underflow trap.
 *	(This is the opposite order from window overflow.)
 *
 *	For the window to restore, the %sp should be good since it doesn't
 *	get changed by these routines, even if it was a user window and this
 *	is a kernel window.
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
 *	The window invalid mask changes and a register window is restored.
 *
 * ----------------------------------------------------------------------
 */
.globl	MachWindowUnderflow
MachWindowUnderflow:
	/*
	 * Check to see if we're about to return to the trap window from
	 * the debugger.  If so, then the frame pointer should be equal
	 * to the base of the debugger stack.  If it is, change it to be
	 * the top of the regular stack, so that when we return to the previous
	 * window, our sp is at the top of the regular stack.
	 */
	set	MACH_DEBUG_STACK_START, %VOL_TEMP1
	cmp	%VOL_TEMP1, %fp
	bne	RegularStack
	nop

DealWithDebugStack:
	/* Set stack pointer of next window to regular frame pointer */
	set	_machSavedRegisterState, %VOL_TEMP1
	ld	[%VOL_TEMP1], %fp
	nop
	
RegularStack:
    /* for debugging */
	set	_debugCounter, %VOL_TEMP1
	ld	[%VOL_TEMP1], %VOL_TEMP1
	set	500, %VOL_TEMP2
	subcc	%VOL_TEMP2, %VOL_TEMP1, %g0
	ble	FinishedDebugging4
	nop
	sll	%VOL_TEMP1, 2, %VOL_TEMP1
	set	_debugSpace, %VOL_TEMP2
	add	%VOL_TEMP2, %VOL_TEMP1, %VOL_TEMP2
	st	%g0, [%VOL_TEMP2]
	st	%fp, [%VOL_TEMP2 + 4]
	set	_debugCounter, %VOL_TEMP1
	ld	[%VOL_TEMP1], %VOL_TEMP2
	add	%VOL_TEMP2, 2, %VOL_TEMP2
	st	%VOL_TEMP2, [%VOL_TEMP1]
FinishedDebugging4:
	/*
	 * It should be ok to use locals here - it's a dead window.
	 * Note that this means one cannot do a restore and then a save
	 * and expect that the old locals in the dead window are still the
	 * same!  That would be dumb anyway.
	 */
	/* mark new invalid window */
	MACH_RETREAT_WIM(%VOL_TEMP1, %VOL_TEMP2, underflowLabel)
	restore				/* move to window to restore */
	/* restore data from stack to window */
	MACH_RESTORE_WINDOW_FROM_STACK()
	/* Move back to previous window.   Clear registers too??? */
	save
	/*
	 * jump to calling routine - this may be a trap-handler or not.
	 */
	jmp	%SAFE_TEMP + 8
	nop



/*
 * ----------------------------------------------------------------------
 *
 * MachHandleDebugTrap --
 *
 *	Enter the debugger.  This means make sure all the windows are
 *	saved to the stack, save my stack pointer, switch the stack pointer
 *	to the debugger stack, and go.
 *
 *	When we return, we change stack pointers and go to the usual
 *	return from trap routine.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------
 */
.globl	MachHandleDebugTrap
MachHandleDebugTrap:
	/*
	 * This points to the top stack frame, which consists of a
	 * Mach_RegState structure.  This is handed to the debugger.  We
	 * also use this to restore our stack pointer upon returning from the
	 * debugger.
	 */
	set	_machSavedRegisterState, %VOL_TEMP1
	st	%sp, [%VOL_TEMP1]
	/*
	 * Now make sure all windows are flushed to the stack.  We do this
	 * with MACH_NUM_WINDOWS - 1 saves and restores.  The window overflow
	 * and underflow traps will then handle this for us.  We put the counter
	 * in a global, since they have all been saved already and are free
	 * for our use and we need the value across windows.
	 */
	set	(MACH_NUM_WINDOWS - 1), %g1
SaveSomeMore:
	save
	subcc	%g1, 1, %g1
	bne	SaveSomeMore
	nop
	set	(MACH_NUM_WINDOWS - 1), %g1
RestoreSomeMore:
	restore
	subcc	%g1, 1, %g1
	bne	RestoreSomeMore
	nop
	/* Set stack base for debugger */
	set	MACH_DEBUG_STACK_START, %sp
	/* get trap type into o0 from local saved value */
	and	%CUR_TBR_REG, MACH_TRAP_TYPE_MASK, %o0
	srl	%o0, 4, %o0
	/* put saved reg ptr into o1 */
	set	_machSavedRegisterState, %VOL_TEMP1
	ld	[%VOL_TEMP1], %o1
	/*
	 * Set wim to this window, so that when we return from debugger,
	 * we'll restore the registers for this window from the stack state
	 * handled by the debugger.  This is safe to do before calling the
	 * debugger, since we just saved all the windows to the stack.
	 */
	MACH_SET_WIM_TO_CWP()
	/*
	 * Is there a window of vulnerability here when the sp doesn't match
	 * what's saved on the stack and I might restore incorrectly from
	 * stack if I took another trap?
	 */

	/* call debugger */
	call	_Dbg_Main,2
	nop

	/* put saved stack pointer into %sp. */
	set	_machSavedRegisterState, %VOL_TEMP1
	ld	[%VOL_TEMP1], %sp

	/* finish as for regular trap */
	set	_MachReturnFromTrap, %VOL_TEMP1
	jmp	%VOL_TEMP1
	nop
