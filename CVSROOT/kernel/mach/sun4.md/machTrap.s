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
	 * The pc and npc were saved into their regs as part of the trap
	 * instruction.  NOTE that we cannot trash %g1, even after saving
	 * the global state, since it contains the system call number if this
	 * trap is due to a syscall.  Or, we could trash it, but we'd have
	 * to retrieve it from the saved global state in the machState
	 * structure in that case...
	 */
	/*
	 * The psr was saved into %CUR_PSR_REG in delay slot in trap table. */
	 */
	mov	%tbr, %CUR_TBR_REG
	mov	%y, %CUR_Y_REG
	/*
	 * Are we in an invalid window? If so, deal with it.
	 */
	MACH_INVALID_WINDOW_TEST()
	be	WindowOkay
	nop
	/*
	 * Deal with window overflow - put return addr in SAFE_TEMP.
	 * It is okay to do this, even if we got here after due to a slow
	 * overflow trap due to special action needed, because if we did that,
	 * then we've already taken care of the overflow problem and so we
	 * won't get it here and so we won't be overwriting the return address
	 * in SAFE_TEMP.
	 */
	set	MachWindowOverflow, %VOL_TEMP1
	jmpl	%VOL_TEMP1, %SAFE_TEMP
	nop
WindowOkay:
	/*
	 * Now that our out registers are safe to use, update our stack
	 * pointer.  If coming from user mode, I need to get kernel sp from
	 * state structure instead.  If we came from kernel
	 * mode, just subtract a stack frame from the current frame pointer and
	 * continue. To see if we came from user mode, we look at the
	 * previous state bit (PS) in the processor state register.
	 * I set up the stack pointer in the delay slot of the branch if I came
	 * from kernel mode, so it's as fast as possible.  I annul the
	 * instruction if it turns out to be a user stack.
	 * We must give the stack a whole full state frame so that routines
	 * we call will have space to store their arguments.  System calls,
	 * for example!  NOTE: The amount we bump up the stack by here must
	 * agree with the test in the window underflow routine, so that bad
	 * things don't happen as we return from the debugger!!!!!
	 */
        andcc   %CUR_PSR_REG, MACH_PS_BIT, %g0 		/* previous state? */
        bne,a   DoneWithUserStuff 			/* was kernel mode */
        add     %fp, -MACH_SAVED_STATE_FRAME, %sp 	/* set kernel sp */
        set     _machCurStatePtr, %VOL_TEMP1 		/* was user mode */
        ld      [%VOL_TEMP1], %VOL_TEMP2 		/* get machStatePtr */
        add     %VOL_TEMP2, MACH_KSP_OFFSET, %VOL_TEMP1 /* &(machPtr->ksp) */
        ld      [%VOL_TEMP1], %sp 			/* machPtr->ksp */
        add     %sp, (MACH_KERN_STACK_SIZE - MACH_SAVED_STATE_FRAME), %sp 
	/*
	 * Since we came from user mode, set the trapRegs field of the
	 * current state structure to the value of our new kernel sp,
	 * since the trap regs are saved on top of the stack.
	 */
	add	%VOL_TEMP2, MACH_TRAP_REGS_OFFSET, %VOL_TEMP2
	st	%sp, [%VOL_TEMP2]

DoneWithUserStuff:
	andn	%sp, 0x7, %sp	/* double-word aligned - should already be ok */

	/*
	 * This only saves the globals to the stack.  The locals
	 * and ins make it there through normal overflow and underflow.
	 */
	MACH_SAVE_GLOBAL_STATE()

	/* traps on, maskable interrupts off */
	MACH_SR_HIGHPRIO()

	/*
	 * It is tedious to do all these serial comparisons against the
	 * trap type, so this should be changed to a jump table.  It's this
	 * way right now because I'm working on other things and this is
	 * easy for debugging.
	 */
	and	%CUR_TBR_REG, MACH_TRAP_TYPE_MASK, %VOL_TEMP1 /* get trap */

	cmp	%VOL_TEMP1, MACH_TRAP_SYSCALL		/* system call */
	be	MachSysCallTrap
	nop

	cmp	%VOL_TEMP1, MACH_LEVEL10_INT		/* clock interrupt */
	set	_Timer_TimerServiceInterrupt, %o0
	be	MachHandleInterrupt
	nop

	cmp	%VOL_TEMP1, MACH_LEVEL14_INT		/* unused clock level */
	set	_Timer_TimerServiceInterrupt, %o0
	be	MachHandleInterrupt
	nop

	cmp	%VOL_TEMP1, MACH_LEVEL12_INT		/* keyboard interrupt */
	set	_Dev_KbdServiceInterrupt, %o0
	be	MachHandleInterrupt
	nop

	cmp	%VOL_TEMP1, MACH_LEVEL6_INT		/* ether interrupt */
	set	_Net_Intr, %o0
	be	MachHandleInterrupt
	nop

	/* level 8 autovector is video - later */

	/* levels 1, 4 and 6 are software, 6 is also ether */

	/* levels 2, 3, 5, 7, 9, 11 and 13 are vme bus */

	cmp	%VOL_TEMP1, 0x100			/* level 0 int - none */
	set	_MachNoLevel0Interrupt, %o0
	be	MachHandleInterrupt
	nop

	cmp	%VOL_TEMP1, MACH_LEVEL1_INT		/* not yet */
	set	_MachIntrNotHandledYet, %o0
	be	MachHandleInterrupt
	nop

	cmp	%VOL_TEMP1, MACH_LEVEL2_INT		/* not yet */
	set	_MachIntrNotHandledYet, %o0
	be	MachHandleInterrupt
	nop

	cmp	%VOL_TEMP1, MACH_LEVEL3_INT		/* not yet */
	set	_MachIntrNotHandledYet, %o0
	be	MachHandleInterrupt
	nop

	cmp	%VOL_TEMP1, MACH_LEVEL4_INT		/* not yet */
	set	_MachIntrNotHandledYet, %o0
	be	MachHandleInterrupt
	nop

	cmp	%VOL_TEMP1, MACH_LEVEL5_INT		/* not yet */
	set	_MachIntrNotHandledYet, %o0
	be	MachHandleInterrupt
	nop

	cmp	%VOL_TEMP1, MACH_LEVEL7_INT		/* not yet */
	set	_MachIntrNotHandledYet, %o0
	be	MachHandleInterrupt
	nop

	cmp	%VOL_TEMP1, MACH_LEVEL9_INT		/* not yet */
	set	_MachIntrNotHandledYet, %o0
	be	MachHandleInterrupt
	nop

	cmp	%VOL_TEMP1, MACH_LEVEL11_INT		/* not yet */
	set	_MachIntrNotHandledYet, %o0
	be	MachHandleInterrupt
	nop

	cmp	%VOL_TEMP1, MACH_LEVEL13_INT		/* not yet */
	set	_MachIntrNotHandledYet, %o0
	be	MachHandleInterrupt
	nop

	cmp	%VOL_TEMP1, MACH_INSTR_ACCESS		/* instruction fault */
	be	MachHandlePageFault
	nop

	cmp	%VOL_TEMP1, MACH_DATA_ACCESS		/* data fault */
	be	MachHandlePageFault
	nop

	/*
	 * We never get here directly from the window overflow trap.
	 * Instead, what this means is that after handling a window
	 * overflow trap, on the way out we had to deal with a special
	 * user action.  Because we don't save state, etc, on a regular
	 * window overflow, we've executed the above code to save the
	 * state for us.  Now we just want to go back and deal with the
	 * special
	cmp	%VOL_TEMP1, MACH_WINDOW_OVERFLOW	/* weird overflow */
	be	MachReturnFromOverflowWithSavedState
	nop

	cmp	%VOL_TEMP1, MACH_WINDOW_UNDERFLOW	/* weird underflow */
	be	MachReturnFromUnderflowWithSavedState
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
	/* Returning to user mode?  If not, goto NormalReturn. */
	/*
	 * If returning to user mode, check special handling flags here.
	 * These may indicate a need to (in some order)
	 * 1) take a context switch
	 * 2) copy user window overflow values from where they were saved
	 * in the process state buffer out to the user stack, because the
	 * user stack wasn't resident.
	 * 3) deal with a signal.
	 * 4) deal with a bad return value from a user trap that involves
	 * doing something to the user process.
	 */
	/* Look at return values */
	/* Look at context switches */
	/* Look at signal stuff */
	/* Look at window overflow stuff */

	/* Are we a user or kernel process? */
	andcc	%CUR_PSR_REG, MACH_PS_BIT, %g0
	bne	NormalReturn
	nop
	/* Do we need to take a special action? Check special handling flag. */
	set	_proc_RunningProcesses, %VOL_TEMP1
	ld	[%VOL_TEMP1], %VOL_TEMP1		/* ptr to PCB */
	set	_machSpecialHandlingOffset, %VOL_TEMP2
	ld	[%VOL_TEMP2], %VOL_TEMP2
	add	%VOL_TEMP1, %VOL_TEMP2, %VOL_TEMP1
	ld	[%VOL_TEMP1], %VOL_TEMP1
	tst	%VOL_TEMP1
	be	NormalReturn
	call	_MachUserAction
	nop
	/* Must we handle a signal? */
	tst	%RET_VAL_REG
	bne	NormalReturn

NormalReturn:
	MACH_UNDERFLOW_TEST(testModuloLabel)
	be	UnderflowOkay
	nop
	/*
	 * Make sure stack is resident.  We don't want to get a page
	 * fault in the underflow routine, because it moves into the window
	 * to restore and so its calls to Vm code would move into this
	 * window and overwrite it!  First check if it's a user process, since
	 * that should be the only case where the stack isn't resident.
	 */
	andcc	%CUR_PSR_REG, MACH_PS_BIT, %g0
	bne	CallUnderflow
	nop
	/*
	 * It's a user process, so check residence and protection fields of pte.
	 * Before anything we check stack alignment.
	 */
	andcc	%fp, 0x7, %g0
	bne	KillUserProc
	nop
	mov	%fp, %o0
	call	_VmMachGetPageMap
	nop
	srl	%o0, VMMACH_PAGE_PROT_SHIFT, %o0
	cmp	%o0, VMMACH_PTE_OKAY_VALUE
	bne	CallPageIn
	nop
	/*
	 * Check the other extreme of the area we'd be touching on the stack.
	 */
	mov	%fp, %o0
	add	%o0, MACH_SAVED_WINDOW_SIZE, %o0
	call	_VmMachGetPageMap
	nop
	srl	%o0, VMMACH_PAGE_PROT_SHIFT, %o0
	cmp	%o0, VMMACH_PTE_OKAY_VALUE
	be	CallUnderflow			/* both addrs okay */
	nop
	
CallPageIn:
	/*
	 * Call VM Stuff with the %fp which will be stack pointer in
	 * the window we restore..
	 */
	mov	%fp, %o0
	set	0x1, %o1		/* also check for protection????? */
	call	_Vm_PageIn, 2
	nop
	tst	%RETURN_VAL_REG
	be	CallUnderflow
	nop
KillUserProc:
	/*
	 * Kill user process!  Its stack is bad.
	 */
	set	PROC_TERM_DESTROYED, %o0
	set	PROC_BAD_STACK, %o1
	clr	%o2
	call	_Proc_ExitInt, 3
	nop
CallUnderflow:
	set	MachWindowUnderflow, %VOL_TEMP1
	jmpl	%VOL_TEMP1, %RETURN_ADDR_REG
	nop
UnderflowOkay:
	MACH_DISABLE_TRAPS()
	MACH_RESTORE_GLOBAL_STATE()
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
 *	The mach state structure may be modified if this is a user window
 *	and its stack isn't resident so that we need to save windows to
 *	the buffers in the mach state structure.
 *
 * ----------------------------------------------------------------------
 */
.globl	MachHandleWindowOverflowTrap
MachHandleWindowOverflowTrap:
	set	MachWindowOverflow, %VOL_TEMP1
	jmpl	%VOL_TEMP1, %SAFE_TEMP
	nop
	/*
	 * If returning to user mode, check special handling flags here.
	 * We only need to check for the condition that we need to copy user
	 * window overflow values from where they were saved
	 * in the process state buffer out to the user stack, because the
	 * user stack wasn't resident.
	 */
	/* Are we a user or kernel process? */
        andcc   %CUR_PSR_REG, MACH_PS_BIT, %g0
        bne     NormalOverflowReturn
        nop
        /* Do we need to take a special action? Check special handling flag. */
        set     _proc_RunningProcesses, %VOL_TEMP1
        ld      [%VOL_TEMP1], %VOL_TEMP1                /* ptr to PCB */
        set     _machSpecialHandlingOffset, %VOL_TEMP2
        ld      [%VOL_TEMP2], %VOL_TEMP2
        add     %VOL_TEMP1, %VOL_TEMP2, %VOL_TEMP1
        ld      [%VOL_TEMP1], %VOL_TEMP1
        tst     %VOL_TEMP1
        be      NormalOverflowReturn
	/*
	 * We need to save state and put space on the stack pointer, etc.
	 * Run through regular trap preamble to do this.  The trap preamble
	 * will then return us to here, where we'll call MachUserAction
	 * to take care of the special action.
	 */
	call	_MachTrap
	nop
	/*
	 * Now we must restore state, take stuff off the stack, etc.
	 * The easy way to do this is go through the regular return from
	 * trap sequence, and this will do the handling of the special
	 * user action stuff for us.  I could just call MachReturnFromTrap
	 * directly from MachTrap for this trap case, but this looks better
	 * here.  Maybe?
	 */
MachReturnFromOverflowWithSavedState:
	call	_MachReturnFromTrap
	nop

	/*
	 * This was a normal fast window overflow trap, so just return.
	 */
NormalOverflowReturn:
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
 *	Returns to the address in %SAFE_TEMP + 8.
 *
 * Side effects:
 *	The window invalid mask changes and a register window is saved.
 *	The mach state structure may change if we need to save this window
 *	into it if it's a user window and the stack isn't resident.
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
	 * If this is a user window, then see if stack space is resident.
	 * If it is, go ahead, but if not, then set special handling and
	 * save to buffers.
	 */
	set	MACH_KERN_START, %g3		/* %sp lower than kernel? */
	subcc	%g3, %sp, %g0
	bgu	UserStack
	nop
	set	MACH_KERN_END, %g3		/* %sp <= top kernel addr? */
	subcc	%sp, %g3, %g0
	bleu	NormalOverflow			/* is kernel sp */
	nop
UserStack:
	/*
	 * If alignment is bad, what should we do?
	 * I just clear the last bits if it's a user stack pointer!
	 */
	cmp	%sp, 0x7, %g0
	be	ContinueTesting
	nop
	and	%sp, ~(0x7), %sp

	/*
	 * I can't just call the Vm routine to test residence as I do
	 * in the underflow stuff, 'cause it would user output registers
	 * that aren't available here, so I have to do the same work in-line.
	 * GROSS.
	 */
	set	VMMACH_PAGE_MAP_MASK, %g3
	and	%sp, %g3, %g3
	lda	[%g3] VMMACH_PAGE_MAP_SPACE, %g3
	srl	%g3, VMMACH_PAGE_PROT_SHIFT, %g3
	cmp	%g3, VMMACH_PTE_OKAY_VALUE
	be	NormalOverflow
	nop
SaveToInternalBuffer:
	/*
	 * It wasn't resident, so we must save to internal buffer and set
	 * special handling.
	 */
	XXXXX
	jmp	ReturnFromOverflow
	nop
NormalOverflow:
	/*
	 * save this window to stack - save locals and ins to top 16 words
	 * on the stack. (Since our stack grows down, the top word is %sp
	 * and the bottom will be (%sp + offset).
	 */
	MACH_SAVE_WINDOW_TO_STACK()
ReturnFromOverflow:
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
 *	routine expects its return address in %RETURN_ADDR_REG, so this is the
 *	register we must save and restore in the inbetween window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The window in question is restored.
 *
 * ----------------------------------------------------------------------
 */
.globl	MachHandleWindowUnderflowTrap
MachHandleWindowUnderflowTrap:
	clr	%SAFE_TEMP	/* used to mark whether we saved state */
	/* Test if we came from user mode. */
	andcc	%CUR_SER_REG, MACH_PS_BIT, %g0
	bne	NormalUnderflow
	/*
	 * Test if stack is resident for window we need to restore.
	 * This means move back a window and test its frame pointer since
	 * we've trapped into a window 2 away from the one whose stack is
	 * in question.
	 */
	restore
	MACH_NEED_TO_PAGE_IN_USER_STACK()
	be	NormalUnderflow
	/* back again - in delay slot of branch */
	save
	/*
	 * We need to save state.  
	 */
	call	_MachTrap
	nop
MachReturnToUnderflowWithSavedState:
	/*
	 * We came back from saving state.  Mark the fact we had to so we can
	 * restore state and then handle underflow.
	 */
	set	0x11111111, %SAFE_TEMP
	/*
	 * Call	Vm_PageIn stuff and deal with faulting starting at this window.
	 * We can't do it from actual underflow routine, since that's in the
	 * wrong window and calls to the vm stuff would overwrite this window
	 * where we had to save stuff so that MachReturnFromTrap could restore
	 * stuff correctly and return from the correct window.
	 */
	/* Call vm stuff */
	XXX Call vm stuff XXX

NormalUnderflow:
	mov	%g3, %VOL_TEMP1			/* clear out globals */
	mov	%g4, %VOL_TEMP2
	mov	%CUR_PSR_REG, %g4
	restore					/* retreat a window */
	mov	%RETURN_ADDR_REG, %g3		/* save reg in global */
	set	MachWindowUnderflow, %g4	/* put address in global */
	jmpl	%g4, %RETURN_ADDR_REG		/* our return addr to reg */
	nop
	mov	%g3, %RETURN_ADDR_REG		/* restore ret_addr */
	save					/* back to trap window */
	mov	%VOL_TEMP1, %g3			/* restore globals */
	mov	%VOL_TEMP2, %g4

	cmp	%SAFE_TEMP, 0x11111111		/* did we have to save state? */
	bne	NormalUnderflowReturn		/* we were okay */
	nop
	/*
	 * We had to save state, so now we have to restore it.
	 */
	call	_MachReturnFromTrap
	nop
NormalUnderflowReturn:
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
 *	The address of the calling instruction is stored in %RETURN_ADDR_REG,
 *	which is the normal place.
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
	 * NOTE: The test below must agree with the amount we bump up the stack
	 * by in MachTrap.
	 */
	set	_mach_DebugStack - MACH_SAVED_STATE_FRAME, %VOL_TEMP1
	cmp	%VOL_TEMP1, %fp
	bne	RegularStack
	nop

DealWithDebugStack:
	/* Set stack pointer of next window to regular frame pointer */
	set	_machSavedRegisterState, %VOL_TEMP1
	ld	[%VOL_TEMP1], %fp
	nop
	
RegularStack:
	/*
	 * We assume that by the time we've gotten here, the stack has been
	 * made resident, if it was a user stack, and we can just go blasting
	 * ahead.
	 *
	 * It should be ok to use locals here - it's a dead window.
	 * Note that this means one cannot do a restore and then a save
	 * and expect that the old locals in the dead window are still the
	 * same.  That would be a really dumb thing to think anyway.
	 */
	/* mark new invalid window */
	MACH_RETREAT_WIM(%VOL_TEMP1, %VOL_TEMP2, underflowLabel)

	/* move to window to restore */
	restore
	/* restore data from stack to window - stack had better be resident! */
	MACH_RESTORE_WINDOW_FROM_STACK()
	/* Move back to previous window.   Clear registers too??? */
	save
	/*
	 * jump to calling routine - this may be a trap-handler or not.
	 */
	jmp	%RETURN_ADDR_REG + 8
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
	MACH_DEBUG_BUF(%g2, %g3, g1Debug1, %g1)
RestoreSomeMore:
	restore
	MACH_DEBUG_BUF(%g2, %g3, g1Debug2, %g1)
	subcc	%g1, 1, %g1
	bne	RestoreSomeMore
	nop
	/* Set stack base for debugger */
	set	_mach_DebugStack - MACH_FULL_STACK_FRAME, %sp
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
	 * stack if I took another trap?  But interrupts should be off and
	 * I shouldn't get a trap.
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


/*
 * ----------------------------------------------------------------------
 *
 * MachSyscallTrap --
 *
 *	This is the code to handle system call traps.  The number of the
 *	system call is in %g1.
 *
 * Results:
 *	Returns a status to the caller in the caller's %o0 (or %i0).
 *
 * Side effects:
 *	Depends on the kernel call.
 *
 * ----------------------------------------------------------------------
 */
.globl	MachSyscallTrap
MachSyscallTrap:

	/*
	 * Make sure user stack pointer is written into state structure so that
	 * it can be used while processing the system call.  (Who uses it??)
	 * This means saving the frame pointer (previous user stack pointer).
	 */
	set	_machCurStatePtr, %VOL_TEMP1
	ld	[%VOL_TEMP1], %VOL_TEMP1
	add	%VOL_TEMP1, MACH_TRAP_REGS_OFFSET, %VOL_TEMP1
	ld	[%VOL_TEMP1], %VOL_TEMP1
	add	%VOL_TEMP1, MACH_FP_OFFSET, %VOL_TEMP1
	st	%fp, [%VOL_TEMP1]
	/* If fork call, save registers and invalidate trapRegs stuff????? */
	/*
	 * Check number of kernel call for validity.  This was stored in %g1.
	 * We must be careful not to have trashed it.  But if we end up
	 * trashing it, we could instead pull it out of the saved globals state
	 * in the mach state structure since it got saved there.
	 */
	set	_machMaxSysCall, %VOL_TEMP1
	ld	[%VOL_TEMP1], %VOL_TEMP1
	cmp	%VOL_TEMP1, %g1
	bge	GoodSysCall
	nop
	/*
	 * If bad, (take user error? - on spur) then do normal return from trap.
	 * Is this magic number a return value?  It's in the sun3 code.
	 */
	set	SYS_INVALID_SYSTEM_CALL, %RETURN_VAL_REG
	jmp	ReturnFromSyscall
	nop
GoodSysCall:
	/*
	 * Sun3 stuff saves sys call # in a lastSysCall field in state
	 * struct. Why?
	 */
	/*
	 * Fetch args.  Copy them from user space if they aren't all in
	 * the input registers.  For now I copy all the input registers,
	 * since there isn't a table giving the actual number of args?
	 * For args beyond the number of words that can fit in the input
	 * registers, I get the offset to copy to and the pc to jump to inside
	 * the copying from tables set up in Mach_SyscallInit.  If there
	 * are no args to copy, then the pc gets set to jump to
	 * MachFetchArgsEnd.
	 */
	mov	%i5, %o5
	mov	%i4, %o4
	mov	%i3, %o3
	mov	%i2, %o2
	mov	%i1, %o1
	mov	%i0, %o0
	/*
	 * Fetch offsets to copy from and to out of table.
	 */
	sll	%g1, 2, %g1				/* system call index */
	set	_machArgOffsets, %VOL_TEMP2		/* offset in table */
	add	%g1, %VOL_TEMP2, %VOL_TEMP2		/* addr in table */
	ld	[%VOL_TEMP2], %VOL_TEMP2		/* offset value */
	add	%fp, %VOL_TEMP2, %VOL_TEMP1		/* need global here? */
	add	%sp, %VOL_TEMP2, %VOL_TEMP2		/* need global here? */

	/*
	 * Fetch pc branch address out of table and put it into %SAFE_TEMP
	 */
	set	_machArgDispatch, %SAFE_TEMP
	add	%g1, %SAFE_TEMP, %SAFE_TEMP
	ld	[%SAFE_TEMP], %SAFE_TEMP
	jmp	%SAFE_TEMP
	nop

.global	_MachFetchArgs
_MachFetchArgs:
	ld	[%VOL_TEMP1], %SAFE_TEMP	/* first word - pc offset 0 */
	st	%SAFE_TEMP, [%VOL_TEMP2]
	add	%VOL_TEMP1, 4, %VOL_TEMP1
	add	%VOL_TEMP2, 4, %VOL_TEMP2
	ld	[%VOL_TEMP1], %SAFE_TEMP	/* second word - pc offset 16 */
	st	%SAFE_TEMP, [%VOL_TEMP2]
	add	%VOL_TEMP1, 4, %VOL_TEMP1
	add	%VOL_TEMP2, 4, %VOL_TEMP2
	ld	[%VOL_TEMP1], %SAFE_TEMP	/* third word - pc offset 32 */
	st	%SAFE_TEMP, [%VOL_TEMP2]
	add	%VOL_TEMP1, 4, %VOL_TEMP1
	add	%VOL_TEMP2, 4, %VOL_TEMP2
	ld	[%VOL_TEMP1], %SAFE_TEMP	/* fourth word */
	st	%SAFE_TEMP, [%VOL_TEMP2]
	add	%VOL_TEMP1, 4, %VOL_TEMP1
	add	%VOL_TEMP2, 4, %VOL_TEMP2
	ld	[%VOL_TEMP1], %SAFE_TEMP	/* fifth word */
	st	%SAFE_TEMP, [%VOL_TEMP2]
	add	%VOL_TEMP1, 4, %VOL_TEMP1
	add	%VOL_TEMP2, 4, %VOL_TEMP2
	ld	[%VOL_TEMP1], %SAFE_TEMP	/* sixth word */
	st	%SAFE_TEMP, [%VOL_TEMP2]
	add	%VOL_TEMP1, 4, %VOL_TEMP1
	add	%VOL_TEMP2, 4, %VOL_TEMP2
	ld	[%VOL_TEMP1], %SAFE_TEMP	/* seventh word */
	st	%SAFE_TEMP, [%VOL_TEMP2]
	add	%VOL_TEMP1, 4, %VOL_TEMP1
	add	%VOL_TEMP2, 4, %VOL_TEMP2
	ld	[%VOL_TEMP1], %SAFE_TEMP	/* eighth word */
	st	%SAFE_TEMP, [%VOL_TEMP2]
	add	%VOL_TEMP1, 4, %VOL_TEMP1
	add	%VOL_TEMP2, 4, %VOL_TEMP2
	ld	[%VOL_TEMP1], %SAFE_TEMP	/* ninth word */
	st	%SAFE_TEMP, [%VOL_TEMP2]
	add	%VOL_TEMP1, 4, %VOL_TEMP1
	add	%VOL_TEMP2, 4, %VOL_TEMP2
	ld	[%VOL_TEMP1], %SAFE_TEMP	/* tenth word */
	st	%SAFE_TEMP, [%VOL_TEMP2]
	add	%VOL_TEMP1, 4, %VOL_TEMP1
	add	%VOL_TEMP2, 4, %VOL_TEMP2
	/*
	 * Marks last place where PC could be when a page fault occurs while
	 * fetching arguments.  Needed to distinguish between a page fault
	 * during arg fetch (which is okay) from other page faults in the
	 * kernel, which are fatal errors.
	 */
.global	_MachFetchArgsEnd
_MachFetchArgsEnd:
	/* get address from table */
	set	_proc_RunningProcesses, %VOL_TEMP1
	ld	[%VOL_TEMP1], %VOL_TEMP1		/* ptr to PCB */
	set	_machKcallTableOffset, %VOL_TEMP2
	ld	[%VOL_TEMP2], %VOL_TEMP2		/* offset to kcalls */
	add	%VOL_TEMP1, %VOL_TEMP2, %VOL_TEMP1
	ld	[%VOL_TEMP1], %VOL_TEMP1		/* addr of array */
	add	%g1, %VOL_TEMP1, %VOL_TEMP1		/* index to kcall */
	ld	[%VOL_TEMP1], %VOL_TEMP1		/* got addr */
	/* go do it */
	call	%VOL_TEMP1
	nop
	/* Disable interrupts.  (Is this necessary?  Sun3 does it.) */
	mov	%psr, %VOL_TEMP1
	or	%VOL_TEMP1, MACH_DISABLE_INTERRUPTS, %VOL_TEMP1
	mov	%VOL_TEMP1, %psr
	WAIT_FOR_STATE_REGISTER
	/*
	 * So that we don't re-execute the trap instruction when we
	 * return from the system call trap via the return trap procedure,
	 * we increment the return pc and npc here.
	 */
	add	%CUR_PC_REG, 0x4, %CUR_PC_REG
	add	%NEXT_PC_REG, 0x4, %NEXT_PC_REG
	/*
	 * Move return value to caller's return val register.
	 */
	mov	%RETURN_VAL_REG, %RETURN_VAL_REG_CHILD
	/*
	 * Sun3 checks special handling flag here.  I do it in return from
	 * trap, if we came from a user process.
	 */

	/* restore the stack pointers? */

	/* do normal return from trap */
	set	_MachReturnFromTrap, %VOL_TEMP1
	jmp	%VOL_TEMP1
	nop


/*
 * ----------------------------------------------------------------------
 *
 * MachHandlePageFault --
 *
 *	An instruction or data fault has occurred.  This routine will retrieve
 *	the fault error from the bus error register and the faulting address
 *	from the address error register and will call
 *	Vm routines to handle faulting in the page.
 *
 * Results:
 *	Returns to MachReturnFromTrap rather than its caller.
 *
 * Side effects:
 *	The page that caused the fault will become valid.
 *
 * ----------------------------------------------------------------------
 */
.global MachHandlePageFault
MachHandlePageFault:
	/* bus error register value as first arg. */
	set	VMMACH_BUS_ERROR_REG, %o0
	lduba	[%o0] VMMACH_CONTROL_SPACE, %o0
	/* memory address causing the error as second arg */
	set	VMMACH_ADDR_ERROR_REG, %VOL_TEMP1
	ld	[%VOL_TEMP1], %o1
	/* have to write to memory error address reg to clear the error */
	st	%g0, [%VOL_TEMP1]
	/* trap value of the psr as third arg */
	mov	%CUR_PSR_REG, %o2	
	/* trap value of pc as fourth argument */
	mov	%CUR_PC_REG, %o3
	call	_MachPageFault, 4
	nop
	set	_MachReturnFromTrap, %VOL_TEMP1
	jmp	%VOL_TEMP1
	nop
