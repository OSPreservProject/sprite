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

#include "user/proc.h"
#include "machConst.h"
#include "machAsmDefs.h"

.align	8
.seg	"text"

/*
 * ----------------------------------------------------------------------
 *
 * MachTrap --
 *
 *	All traps except window overflow and window underflow go through
 *	this trap handler.  This handler is used to save the appropriate
 *	state, set up the kernel stack, and then jump to the correct
 *	handler depending on the type of trap.  The handler assumes that
 *	we have saved the trap %psr into %CUR_PSR_REG before we get here (as
 *	an instruction in the vector table that made us branch here).
 *
 *	Window overflow and underflow jump directly from the vector table
 *	to their handlers, since in the most common cases they do not need
 *	to save state and they should be as fast as possible.
 *
 *	The scheme of things:
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
 *	saved as a part of the normal window overflow and underflow.  There
 *	are exceptions to this, such as context switching or debugger traps,
 *	where we must explicitly save the window to the stack, but the
 *	exceptions must take care of this for themselves.
 *	4) Re-enable traps and disable interrupts.  Traps must be disabled
 *	so that if we get another window overflow or underflow,
 *	we'll be able to deal with it.
 *	5) Figure out what handler to call and call it.
 *
 *	The handler must call our return-from-trap post-amble, rather than
 *	return here.
 *
 *	If a window underflow or overflow trap discovers that it must do
 *	something tricky, such as call Vm_PageIn, that requires turning
 *	on traps and interrupts, then it will call MachTrap to save state
 *	for it.  This is why there are entries in MachTrap for window
 *	overflow and underflow.  These are the entries to take care of the
 *	"slow trap" overflows and underflows.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	We make space on our kernel stack to save this trap window.
 *	If it is a user process entering the kernel, then we make its trapRegs
 *	field point to this save-window area on the kernel stack.
 *
 * ----------------------------------------------------------------------
 */
.global	_MachTrap
_MachTrap:
	/*
	 * Save the state registers.  This is safe, since we're saving them
	 * to local registers and we can do this even if we've entered
	 * into an invalid window.
	 * In the vector table that jumps here, we have already saved
	 * the %psr into %CUR_PSR_REG.  The trap instruction itself saved
	 * the trap pc and next pc into local registers %CUR_PC_REG and
	 * %NEXT_PC_REG.  This means we must save only the %tbr and the %y
	 * registers.
	 */
	mov	%tbr, %CUR_TBR_REG
	mov	%y, %CUR_Y_REG
	/*
	 * Are we in an invalid window?
	 */
	MACH_INVALID_WINDOW_TEST()
	be	WindowOkay
	nop
	/*
	 * Deal with window overflow - put return addr in SAFE_TEMP since
	 * the overflow handler will look for the return address there..
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
	 * Now that we know our out registers are safe to use, since we're
	 * in a valid window, update our stack  pointer.
	 * If coming from user mode, I need to get kernel sp from
	 * state structure.  If we came from kernel
	 * mode, just subtract a stack frame from the current frame pointer and
	 * continue. To see if we came from user mode, we look at the
	 * previous state bit (PS) in the processor state register.
	 * I set up the stack pointer in the delay slot of the branch if I came
	 * from kernel mode, so it's as fast as possible.  I annul the
	 * instruction if it turns out to be a user stack.
	 * We must give the stack a full state frame so that C routines
	 * we call will have space to store their arguments.  (System calls,
	 * for example!)
	 *
	 * NOTE: The amount we bump up the stack by here must
	 * agree with the debugger stack test in the window underflow routine,
	 * so that we don't trash our stack when returning from the
	 * debugger!!!!!
	 */
        andcc   %CUR_PSR_REG, MACH_PS_BIT, %g0 		/* previous state? */
        bne,a   DoneWithUserStuff 			/* was kernel mode */
        add     %fp, -MACH_SAVED_STATE_FRAME, %sp 	/* set kernel sp */
	MACH_GET_CUR_STATE_PTR(%VOL_TEMP2, %VOL_TEMP1)	/* into %VOL_TEMP2 */
#ifdef NOTDEF
        set     _machCurStatePtr, %VOL_TEMP1
        ld      [%VOL_TEMP1], %VOL_TEMP2 		/* get machStatePtr */
#endif NOTDEF
        add     %VOL_TEMP2, MACH_KSP_OFFSET, %VOL_TEMP1 /* &(machPtr->ksp) */
        ld      [%VOL_TEMP1], %sp 			/* machPtr->ksp */
	set	(MACH_KERN_STACK_SIZE - MACH_SAVED_STATE_FRAME), %VOL_TEMP1
        add     %sp, %VOL_TEMP1, %sp 
	/*
	 * Since we came from user mode, set the trapRegs field of the
	 * current state structure to the value of our new kernel sp,
	 * since the trap regs are saved on top of the stack.
	 */
	add	%VOL_TEMP2, MACH_TRAP_REGS_OFFSET, %VOL_TEMP2
	st	%sp, [%VOL_TEMP2]

DoneWithUserStuff:
	/* Test stack alignment here?? */

	/*
	 * This only saves the globals to the stack.  The locals
	 * and ins make it there through normal overflow and underflow.
	 *
	 * NOTE that we cannot trash %g1 even after saving the global
	 * registers, since that is the register that contains the system
	 * call number for system call traps!
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

	cmp	%VOL_TEMP1, MACH_LEVEL0_INT
	bl	NotAnInterrupt
	nop
	cmp	%VOL_TEMP1, MACH_LEVEL15_INT
	bgu	NotAnInterrupt
	nop
	/*
	 * It's an interrupt.
	 */
	b	MachHandleInterrupt
	nop

NotAnInterrupt:
	cmp	%VOL_TEMP1, MACH_TRAP_SYSCALL		/* system call */
	be	MachSyscallTrap
	nop

	cmp	%VOL_TEMP1, MACH_INSTR_ACCESS		/* instruction fault */
	be	MachHandlePageFault
	nop

	cmp	%VOL_TEMP1, MACH_DATA_ACCESS		/* data fault */
	be	MachHandlePageFault
	nop

	cmp	%VOL_TEMP1, MACH_TRAP_SIG_RETURN	/* ret from handler */
	be	_MachReturnFromSignal
	nop

	cmp	%VOL_TEMP1, MACH_TRAP_FLUSH_WINDOWS	/* flush window trap */
	be	MachFlushWindowsToStackTrap
	nop

	/*
	 * These next few are handled by C routines, and we want them to
	 * return to MachReturnFromTrap, so set that address as the return pc.
	 */
	cmp	%VOL_TEMP1, MACH_ILLEGAL_INSTR		/* illegal instr */
	set	_MachReturnFromTrap, %RETURN_ADDR_REG	/* set return pc */
	mov	%VOL_TEMP1, %o0
	mov	%CUR_PC_REG, %o1
	mov	%CUR_PSR_REG, %o2
	be	_MachHandleWeirdoInstruction		/* C routine */
	nop

	cmp	%VOL_TEMP1, MACH_PRIV_INSTR		/* privileged instr */
	set	_MachReturnFromTrap, %RETURN_ADDR_REG	/* set return pc */
	mov	%VOL_TEMP1, %o0
	mov	%CUR_PC_REG, %o1
	mov	%CUR_PSR_REG, %o2
	be	_MachHandleWeirdoInstruction		/* C routine */
	nop

	cmp	%VOL_TEMP1, MACH_MEM_ADDR_ALIGN		/* addr not aligned */
	set	_MachReturnFromTrap, %RETURN_ADDR_REG	/* set return pc */
	mov	%VOL_TEMP1, %o0
	mov	%CUR_PC_REG, %o1
	mov	%CUR_PSR_REG, %o2
	be	_MachHandleWeirdoInstruction		/* C routine */
	nop

	cmp	%VOL_TEMP1, MACH_TAG_OVERFLOW		/* tagged instr ovfl */
	set	_MachReturnFromTrap, %RETURN_ADDR_REG	/* set return pc */
	mov	%VOL_TEMP1, %o0
	mov	%CUR_PC_REG, %o1
	mov	%CUR_PSR_REG, %o2
	be	_MachHandleWeirdoInstruction		/* C routine */
	nop

	cmp	%VOL_TEMP1, MACH_FP_EXCEP		/* fp unit badness */
	set	_MachReturnFromTrap, %RETURN_ADDR_REG	/* set return pc */
	mov	%VOL_TEMP1, %o0
	mov	%CUR_PC_REG, %o1
	mov	%CUR_PSR_REG, %o2
	be	_MachHandleWeirdoInstruction		/* C routine */
	nop

	cmp	%VOL_TEMP1, MACH_FP_DISABLED		/* fp unit disabled */
	set	_MachReturnFromTrap, %RETURN_ADDR_REG	/* set return pc */
	mov	%VOL_TEMP1, %o0
	mov	%CUR_PC_REG, %o1
	mov	%CUR_PSR_REG, %o2
	be	_MachHandleWeirdoInstruction		/* C routine */
	nop

	/*
	 * We never get here directly from the window overflow trap.
	 * Instead, what this means is that after handling a window
	 * overflow trap, on the way out we had to deal with a special
	 * user action.  Because we don't save state, etc, on a regular
	 * window overflow, we've executed the above code to save the
	 * state for us.  Now we just want to go back and deal with the
	 * special overflow.
	 */
	cmp	%VOL_TEMP1, MACH_WINDOW_OVERFLOW	/* weird overflow */
	be	MachReturnToOverflowWithSavedState
	nop

	cmp	%VOL_TEMP1, MACH_WINDOW_UNDERFLOW	/* weird underflow */
	be	MachReturnToUnderflowWithSavedState
	nop

	cmp	%VOL_TEMP1, MACH_TRAP_DEBUGGER		/* enter debugger */
	be	_MachHandleDebugTrap
	nop

	b	_MachHandleDebugTrap		/* all  others to debugger */
	nop


/*
 * ----------------------------------------------------------------------
 *
 * MachReturnFromTrap --
 *
 *	Go through the inverse of MachTrap.  The trap handlers that MachTrap
 *	called return to here rather than MachTrap.
 *
 *	The scheme of things:
 *
 *	1) Determine if we are returning to user mode.  If so, then we must
 *	check the specialHandling flag.  If it is set, then we must call
 *	MachUserAction.
 *	2) If we called MachUserAction, then we must check its return value
 *	to see if we need to deal with any signals.  If we do, then
 *	we call MachHandleSignal().  It returns to user mode itself via
 *	a rett instruction, so we don't come back here until the
 *	return-from-signal trap that the user ends up executing to return to
 *	the kernel from a signal.
 *	3) For both returns to user mode and returns to kernel mode, we
 *	must next check if we would return to an invalid window.  If so,
 *	we must make it valid or we will get a watchdog reset.  We may
 *	call the window underflow routine to do this.  But, if we are
 *	returning to user mode, we must check to make sure the user stack
 *	is resident, since the underflow routine can't get any page faults.
 *	If the user stack isn't resident, we page it in.
 *	4) Finally, we disable traps again (or the rett instruction will
 *	give us a watchdog reset), restore the global registers, restore
 *	the %tbr, %y registers, restore the %psr to the trap psr, and rett
 *	(return from trap) to the saved trap pc and next pc.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	We jump back to where we came from.  If we came from a system call
 *	or certain other traps, we'll actually return to where we came from
 *	plus 8, or else we would re-execute the system call instruction.
 *	The addition of 8 was done by the individual handlers that know
 *	whether or not they would need to do this.
 *
 *	If a user process has a bad stack pointer, we will kill it here and
 *	print out a message.
 *
 * ----------------------------------------------------------------------
 */
.global	_MachReturnFromTrap
_MachReturnFromTrap:
	/* Are we a user or kernel process? */
	andcc	%CUR_PSR_REG, MACH_PS_BIT, %g0
	bne	NormalReturn
	nop
	/* Do we need to take a special action? Check special handling flag. */
	MACH_GET_CUR_PROC_PTR(%VOL_TEMP1)
	set	_machSpecialHandlingOffset, %VOL_TEMP2
	ld	[%VOL_TEMP2], %VOL_TEMP2
	add	%VOL_TEMP1, %VOL_TEMP2, %VOL_TEMP1
	ld	[%VOL_TEMP1], %VOL_TEMP1
	tst	%VOL_TEMP1
	be	NormalReturn
	nop
	call	_MachUserAction
	nop
	/* Check if we must handle a signal */
	tst	%RETURN_VAL_REG
	be	NormalReturn
	nop
	/*
	 * We must handle a signal.  Call the leaf routine MachSetupSignal.
	 * It does its own return from trap, so we don't come back here when
	 * it's done!
	 */
	call	_MachHandleSignal
	nop
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
	MACH_CHECK_FOR_FAULT(%fp, %VOL_TEMP1)
	be	CheckNextFault
	nop
	/*
	 * Call VM Stuff with the %fp which will be stack pointer in
	 * the window we restore.
	 */
	QUICK_ENABLE_INTR(%VOL_TEMP1)
	mov	%fp, %o0
	clr	%o1		/* also check for protection????? */
	call	_Vm_PageIn, 2
	nop
	QUICK_DISABLE_INTR(%VOL_TEMP1)
	tst	%RETURN_VAL_REG
	bne	KillUserProc
	nop
CheckNextFault:
	/* Check other extreme of area we'd touch */
	add	%fp, (MACH_SAVED_WINDOW_SIZE - 4), %o0
	MACH_CHECK_FOR_FAULT(%o0, %VOL_TEMP1)
	be	CallUnderflow
	nop
	QUICK_ENABLE_INTR(%VOL_TEMP1)
	clr	%o1
	call	_Vm_PageIn, 2
	nop
	QUICK_DISABLE_INTR(%VOL_TEMP1)
	tst	%RETURN_VAL_REG
	be	CallUnderflow
	nop
KillUserProc:
	/*
	 * Kill user process!  Its stack is bad.
	 */
	MACH_SR_HIGHPRIO()	/* traps back on for overflow from printf */
	set	_MachReturnFromTrapDeathString, %o0
	call	_printf, 1
	nop
	MACH_GET_CUR_PROC_PTR(%o0)		/* procPtr in %o0 */
        set     _machGenFlagsOffset, %o1 
        ld      [%o1], %o1
        add     %o0, %o1, %o1
        ld      [%o1], %o1
        set     _machForeignFlag, %o2
        ld      [%o2], %o2
        andcc   %o1, %o2, %o1                   /* Is this a migrated proc? */
        be      DebugIt
        nop
	set	PROC_TERM_DESTROYED, %o0	/* If so, kill it. */
	set	PROC_BAD_STACK, %o1
	clr	%o2
	call	_Proc_ExitInt, 3
	nop
DebugIt:					/* Else, make it debuggable. */
	set	TRUE, %o1			/* debug TRUE */
	set	PROC_TERM_DESTROYED, %o2
	set	PROC_BAD_STACK, %o3
	clr	%o4
KeepSuspended:
	call	_Proc_SuspendProcess, 5
	nop
	ba	KeepSuspended
	nop

CallUnderflow:
	set	MachWindowUnderflow, %VOL_TEMP1
	jmpl	%VOL_TEMP1, %RETURN_ADDR_REG
	nop
UnderflowOkay:
	MACH_DISABLE_TRAPS(%VOL_TEMP1, %VOL_TEMP2)
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
 *	Trap entrance to the window overflow handler.  We try to get in and
 *	out of here as quickly as possible in the normal case.  Unlike
 *	most traps, we jump here directly from the trap table, and do not
 *	go through the usual MachTrap preamble.  This means no
 *	state has been saved and we have no stack pointer.  This in turn means
 *	that we cannot turn traps on, since we have no saved state and no place
 *	to save the trap registers. This routine sets up a return
 *	address, calls the overflow handler, restores the psr, and
 *	returns from the trap.  It assumes that the trap psr was stored in
 *	%CUR_PSR_REG before jumping here (in the trap vector table).
 *
 *	When we return to this routine from the actual overflow handling
 *	routine, we check to see if we're returning to user mode.  If we aren't
 *	we can just leave this routine normally.  If we're returning to user
 *	mode, however, we must check for some special cases.  It may be that
 *	the user stack wasn't resident and that we had to save the overflow
 *	window to a special internal buffer rather than the user stack.  If this
 *	is so, we require extra processing.  So we call MachTrap to save state
 *	for us and then call MachReturnFromTrap, which will handle the fact
 *	that the window was saved to an internal buffer.  The effect of all of
 *	this is to turn the overflow trap into a "slow overflow trap" that goes
 *	through the usual trap preamble and postamble.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The mach state structure may be modified if this is a user window
 *	and its stack isn't resident, causing us to save the window to
 *	the buffers in the mach state structure.
 *
 * ----------------------------------------------------------------------
 */
.global	MachHandleWindowOverflowTrap
MachHandleWindowOverflowTrap:
	/*
	 * Call actual overflow handler.
	 */
	set	MachWindowOverflow, %VOL_TEMP1
	jmpl	%VOL_TEMP1, %SAFE_TEMP
	nop
	/*
	 * If returning to user mode, check special handling flags here to
	 * see if we need to do any fancy processing (due to the user's
	 * stack not being resident so we had to save the window to an internal
	 * buffer instead).
	 */
        andcc   %CUR_PSR_REG, MACH_PS_BIT, %g0		/* user or kernel? */
        bne     NormalOverflowReturn
        nop
        /* Do we need to take a special action? Check special handling flag. */
	MACH_GET_CUR_PROC_PTR(%VOL_TEMP1)
        set     _machSpecialHandlingOffset, %VOL_TEMP2
        ld      [%VOL_TEMP2], %VOL_TEMP2
        add     %VOL_TEMP1, %VOL_TEMP2, %VOL_TEMP1
        ld      [%VOL_TEMP1], %VOL_TEMP1
        tst     %VOL_TEMP1
        be      NormalOverflowReturn
	nop
	/*
	 * We need to save state and allocate space on the stack, etc.
	 * Run through regular trap preamble to do this.  The trap preamble
	 * will then return us to here.  This is all in preparation for
	 * calling MachReturnFromTrap, since that will call MachUserAction
	 * to deal with the window saved to the internal buffer.
	 */
	call	_MachTrap
	nop
MachReturnToOverflowWithSavedState:
	/*
	 * Now call trap postamble to restore state and push saved window to
	 * the user stack.  MachReturnFromTrap will return to user mode, so
	 * we will not come back here after this call!
	 */
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
 *	Window overflow handler.  It's set up so that it can be called from the
 *	fast window overflow trap handler, or it can be called directly from
 *	MachTrap for any trap that causes us to land inside an invalid window.
 *
 *	The window we've trapped into is currently invalid.  We want to
 *	make it valid.  We do this by moving one window further, saving that
 *	window to the stack, marking it as the new invalid window, and then
 *	moving back to the window that we trapped into.  It is then valid
 *	and usable.  Note that we move first to the window to save and then
 *	mark it invalid.  In the other order we would get another overflow
 *	trap, but with traps turned off, which causes a watchdog reset...
 *	Note that %sp should point to the lowest address usable
 *	word in the current stack frame.  %fp is the %sp of the
 *	caller's stack frame, so the first (highest in memory) usable word
 *	of a current stack frame is (%fp - 4).
 *
 *	This code will be called with traps DISABLED, so nothing in this
 *	code is allowed to cause a trap.  This means no procedure calls are
 *	allowed.
 *
 * Results:
 *	Returns to the address %SAFE_TEMP + 8.
 *
 * Side effects:
 *	The window invalid mask changes and a register window is saved.
 *	The mach state structure may change if we need to save this window
 *	into its internal buffers (if it's a user window and the stack isn't
 *	resident).
 *
 * ----------------------------------------------------------------------
 */
.global	MachWindowOverflow
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
	 * save to buffers.  It's fairly dreadful that we must do all this
	 * checking here, since this will slow down all the overflow traps.
	 */
	set	MACH_MAX_USER_STACK_ADDR, %g3	/* %sp in user space? */
	subcc	%g3, %sp, %g0			/* need sp < highest addr */
	bleu	NotUserStack
	nop
	set	MACH_FIRST_USER_ADDR, %g3	/* need sp >= lowest addr */
	subcc	%sp, %g3, %g0
	bgeu	UserStack
	nop
NotUserStack:
	set	MACH_STACK_BOTTOM, %g3
	subcc	%sp, %g3, %g0			/* need sp >= lowest addr */
	blu	BadStack
	nop
	set	VMMACH_DEV_START_ADDR, %g3	/* need sp < highest addr */
	subcc	%g3, %sp, %g0
	bgu	NormalOverflow
	nop
BadStack:
	/*
	 * Assume it was a user process's bad stack pointer.  We can't kill
	 * the process here, so just take over the window and the user process
	 * will die a terrible death later.  To take over the window, we just
	 * return, since we've already advanced the %wim.
	 */
	ba	ReturnFromOverflow
	nop
UserStack:
	/*
	 * If the stack alignment is bad, we just take over the window and
	 * assume the user process will die a horrible death later on.
	 */
	andcc	%sp, 0x7, %g0
	bne	ReturnFromOverflow
	nop

	/* Would saving window cause a page fault? */
	MACH_CHECK_FOR_FAULT(%sp, %g3)
	bne	SaveToInternalBuffer
	nop

	/* check other address extreme */
	add	%sp, (MACH_SAVED_WINDOW_SIZE - 4), %g4
	MACH_CHECK_FOR_FAULT(%g4, %g3)
	be	NormalOverflow
	nop
SaveToInternalBuffer:
	/*
	 * The stack wasn't resident, so we must save to an internal buffer
	 * and set the special handling flag.
	 * We update the saved mask to show that this window had to be saved
	 * to the internal buffers.  We do this by or'ing in the value of
	 * the current window invalid mask, since it's been set to point to
	 * this window we must save.
	 */
#ifdef NOTDEF
	set	_machCurStatePtr, %g3
	ld	[%g3], %g3
#endif NOTDEF
	MACH_GET_CUR_STATE_PTR(%g3, %g4)	/* puts machStatePtr in %g3 */
	set	_machCurStatePtr, %g4
	st	%g3, [%g4]			/* update it */
	add	%g3, MACH_SAVED_MASK_OFFSET, %g3
	ld	[%g3], %g3
	mov	%wim, %g4
	or	%g3, %g4, %g4
	set	_machCurStatePtr, %g3
	ld	[%g3], %g3
	add	%g3, MACH_SAVED_MASK_OFFSET, %g3
	st	%g4, [%g3]

	/*
	 * Get and set the special handling flag in the current process state.
	 */
	MACH_GET_CUR_PROC_PTR(%g3)
	set	_machSpecialHandlingOffset, %g4
	ld	[%g4], %g4
	add	%g3, %g4, %g3
	set	0x1, %g4
	st	%g4, [%g3]

	/*
	 * Save the current user stack pointer for this window.
	 */
	set	_machCurStatePtr, %g3
	ld	[%g3], %g3
	add	%g3, MACH_SAVED_SPS_OFFSET, %g3
	mov	%psr, %g4
	and	%g4, MACH_CWP_BITS, %g4
	/* Multiply by 4 bytes per int */
	sll	%g4, 2, %g4
	add	%g3, %g4, %g3
	st	%sp, [%g3]

	/*
	 * Now save to internal buffer.
	 */
	set	_machCurStatePtr, %g3
	ld	[%g3], %g3
	add	%g3, MACH_SAVED_REGS_OFFSET, %g3
	/*
	 * Current window * 4 bytes per reg is still in %g4.  Now we just
	 * need to multiply it by the number of registers per window.
	 */
	sll	%g4, MACH_NUM_REG_SHIFT, %g4
	add	%g3, %g4, %g3		/* offset to start saving at */
	MACH_SAVE_WINDOW_TO_BUFFER(%g3)
	set	ReturnFromOverflow, %g3
	jmp	%g3
	nop
NormalOverflow:
	/*
	 * Save this window to stack - save locals and ins to top 16 words
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
 *	In the case where the window we're restoring is for a user process,
 *	we must also check for whether the stack is resident and writable.
 *	If it isn't, we must page it in.  To do this is gross.  Since the
 *	trap window we've entered into is 2 windows away from the window to
 *	restore, we must move back one window and check the %fp there.  (This
 *	will be the %sp in the actual window to restore.)  If it isn't
 *	resident, then we must move forward again to the trap window and call
 *	MachTrap to save state for us. Then we can call the Vm_PageIn code
 *	from the trap window to page in the nasty stuff.  If it fails, we
 *	must kill the user process.  All of this is complicated a bit further
 *	by the fact that we have 2 addresses to test: the %fp and the
 *	%fp + the size of the saved window stuff. So we check both for
 *	residence.  If both are okay, we just do normal underflow.  If one or
 *	the other fails, we save state.  We then test the first address,
 *	if it fails, we page it in.  Then if the second one fails, we also
 *	page it in.  It most likely will have been paged in by the first one,
 *	though.  Then, after all this, we call the normal underflow which
 *	should then be protected from page faults.  It has to be, since
 *	it operates in one window back from the trap window where we saved
 *	state, and we can't have any calls to Vm code there overwriting our
 *	saved state.  Then we return.  If we marked that we had to save
 *	state, then we must restore state by calling MachReturnFromTrap and
 *	leaving as if this had all been a slow trap instead.  Unfortunately,
 *	we must test some stuff twice, since there are no registers to save
 *	anything into (%VOL_TEMP registers are blasted by the MachTrap code,
 *	and the SAFE_TEMP register is already used to mark whether we saved
 *	state).
 *
 *	NOTE:  In the case where we must save state by calling MachTrap,
 *	a lot of the underflow code is duplicated in MachReturnFromTrap,
 *	which we also end up calling.  The reason I cannot simply call
 *	MachReturnFrom trap then to do all that work, is that I must do the
 *	work in one window away in the case of a real underflow trap.  There
 *	may be a way to deal with this, but I haven't done it yet.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The window in question is restored.
 *
 * ----------------------------------------------------------------------
 */
.global	MachHandleWindowUnderflowTrap
MachHandleWindowUnderflowTrap:
	/*
	 * We need a global to mark further actions, and a global to save
	 * the fp in if we find it needs to be page faulted.  We can save one
	 * global in SAFE_TEMP since it's only whacked by MachTrap, which is
	 * the only thing that might whack it, in case MachTrap gets an
	 * overflow.  But we won't get one since this is an underflow trap.
	 * %o5 also appears to be safe here.
	 */
	mov	%g3, %SAFE_TEMP
	mov	%g4, %o5

	/* Test if we came from user mode. */
	andcc	%CUR_PSR_REG, MACH_PS_BIT, %g0
	bne	NormalUnderflow
	nop
	/*
	 * Test if stack is resident for window we need to restore.
	 * This means move back a window and test its frame pointer since
	 * we've trapped into a window 2 away from the one whose stack is
	 * in question.
	 */
	restore
	/*
	 * If the alignment of the user stack pointer is bad, kill the process!
	 * We must be back in the trap window to do that.
	 */
	andcc	%fp, 0x7, %g0
	be,a	CheckForFaults
	clr	%g3			/* g3 clear for no problem yet */
	set	0x1, %g3		/* Mark why we will save state */
	bne,a	MustSaveState
	save		/* back to trap window, in annulled delay slot */

CheckForFaults:
	/*
	 * %g3 will have have a record of what faults would occur.
	 */
	MACH_CHECK_STACK_FAULT(%fp, %o0, %g3, %o1, UndflFault1, UndflFault2)
	mov	%fp, %g4		/* in case we need to fault it */
	be	NormalUnderflow
	save				/* back to trap window */

MustSaveState:
	/*
	 * We need to save state.   We must do this in actual trap window.
	 * This state-saving enables traps.  We will return to
	 * MachReturnToUnderflowWithSavedState
	 */
	call	_MachTrap
	nop
MachReturnToUnderflowWithSavedState:
	/*
	 * We had to save state for some reason.  %g3 contains the reason
	 * why.
	 */
	cmp	%g3, 0x1
	be	KillTheProc
	nop
	andcc	%g3, 0x2, %g0
	be	CheckNextUnderflow	/* It wasn't first possible fault */
	nop

	QUICK_ENABLE_INTR(%VOL_TEMP1)
	/* Address that would fault is in %g4. */
	mov	%g4, %o0
	clr	%o1			/* also check protection???? */
	call	_Vm_PageIn, 2
	nop
	QUICK_DISABLE_INTR(%VOL_TEMP1)
	tst	%RETURN_VAL_REG
	be	CheckNextUnderflow		/* succeeded, try next */
	nop
	/* Otherwise, bad return, fall through to kill process. */
KillTheProc:
	mov	%SAFE_TEMP, %g3		/* Need I restore these really? */
	mov	%o5, %g4
	/* KILL IT - must be in trap window */
	MACH_SR_HIGHPRIO()	/* traps back on for overflow from printf */
	set	_MachHandleWindowUnderflowDeathString, %o0
	call	_printf, 1
	nop
	MACH_GET_CUR_PROC_PTR(%o0)		/* procPtr in %o0 */
	set	_machGenFlagsOffset, %o1
	ld	[%o1], %o1
	add	%o0, %o1, %o1
	ld	[%o1], %o1
	set	_machForeignFlag, %o2
	ld	[%o2], %o2
	andcc	%o1, %o2, %o1			/* Is this a migrated proc? */
	be	SuspendIt
	nop
	set	PROC_TERM_DESTROYED, %o0	/* If so, kill it. */
	set	PROC_BAD_STACK, %o1
	clr	%o2
	call	_Proc_ExitInt, 3
	nop
SuspendIt:					/* Else, make it debuggable. */
	set	TRUE, %o1			/* debug TRUE */
	set	PROC_TERM_DESTROYED, %o2
	set	PROC_BAD_STACK, %o3
	clr	%o4
KeepFromContinuing:
	call	_Proc_SuspendProcess, 5
	nop
	ba	KeepFromContinuing
	nop

CheckNextUnderflow:
	andcc	%g3, 0x4, %g0		/* See if second address would fault */
	be	BackAgain
	nop
	QUICK_ENABLE_INTR(%VOL_TEMP1)
	/* old %fp is in %g4 */
	add	%g4, (MACH_SAVED_WINDOW_SIZE - 4), %o0
	clr	%o1			/* also check protection???? */
	call	_Vm_PageIn, 2
	nop
	QUICK_DISABLE_INTR(%VOL_TEMP1)
	tst	%RETURN_VAL_REG
	bne	KillTheProc
	nop
BackAgain:
	/*
	 * Don't deal with underflow itself.  Just pretend this was only
	 * a stack page fault.  User's restore instruction will get
	 * re-executed and will cause a further underflow trap.  This time,
	 * the page should be there.
	 */
	mov	%SAFE_TEMP, %g3			/* restore globals */
	mov	%o5, %g4
	call	_MachReturnFromTrap
	nop

NormalUnderflow:
	restore					/* retreat a window */
	mov	%RETURN_ADDR_REG, %g3		/* save reg in global */
	set	MachWindowUnderflow, %g4	/* put address in global */
	jmpl	%g4, %RETURN_ADDR_REG		/* our return addr to reg */
	nop
	mov	%g3, %RETURN_ADDR_REG		/* restore ret_addr */
	save					/* back to trap window */
	mov	%SAFE_TEMP, %g3			/* restore globals */
	mov	%o5, %g4

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
.global	MachWindowUnderflow
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
	set	_machDebugStackStart, %VOL_TEMP1
	ld	[%VOL_TEMP1], %VOL_TEMP1		/* base of stack */
	set	MACH_FULL_STACK_FRAME, %VOL_TEMP2
	sub	%VOL_TEMP1, %VOL_TEMP2, %VOL_TEMP1	/* offset from base */
	cmp	%VOL_TEMP1, %fp
	bne	RegularStack
	nop

DealWithDebugStack:
	/* Set stack pointer of next window to regular frame pointer */
	set	_machSavedRegisterState, %VOL_TEMP1
	ld	[%VOL_TEMP1], %fp
	
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
.global	_MachHandleDebugTrap
_MachHandleDebugTrap:
	/*
	 * If we came from user mode, we do different stuff.
	 */
        andcc   %CUR_PSR_REG, MACH_PS_BIT, %g0 		/* previous state? */
        bne	KernelDebug 				/* was kernel mode */
	nop
	call	_MachUserDebug
	nop
	set	_MachReturnFromTrap, %VOL_TEMP1
	jmp	%VOL_TEMP1
	nop
KernelDebug:
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
	set	_machDebugStackStart, %VOL_TEMP1
	ld	[%VOL_TEMP1], %VOL_TEMP1		/* stack base */
	set	MACH_FULL_STACK_FRAME, %VOL_TEMP2
	sub	%VOL_TEMP1, %VOL_TEMP2, %sp		/* offset from base */
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
.global	MachSyscallTrap
MachSyscallTrap:
	/*
	 * So that we don't re-execute the trap instruction when we
	 * return from the system call trap via the return trap procedure,
	 * we increment the return pc and npc here.
	 */
	mov	%NEXT_PC_REG, %CUR_PC_REG
	add	%NEXT_PC_REG, 0x4, %NEXT_PC_REG
	/*
	 * Make sure user stack pointer is written into state structure so that
	 * it can be used while processing the system call.  (Who uses it??)
	 * This means saving the frame pointer (previous user stack pointer).
	 */
	MACH_GET_CUR_STATE_PTR(%VOL_TEMP1, %VOL_TEMP2)	/* into %VOL_TEMP1 */
#ifdef NOTDEF
	set	_machCurStatePtr, %VOL_TEMP1
	ld	[%VOL_TEMP1], %VOL_TEMP1
#endif NOTDEF
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
	bgeu	GoodSysCall
	nop
	/*
	 * If bad, (take user error? - on spur) then do normal return from trap.
	 * Is this magic number a return value?  It's in the sun3 code.
	 */
	set	SYS_INVALID_SYSTEM_CALL, %RETURN_VAL_REG
	set	ReturnFromSyscall, %VOL_TEMP1
	jmp	%VOL_TEMP1
	nop
GoodSysCall:
	/*
	 * Save sys call number into lastSysCall field in process state so
	 * that migration can figure out what it was supposed to be.
	 * %g1 must still contain syscall number.
	 */
	MACH_GET_CUR_STATE_PTR(%VOL_TEMP1, %VOL_TEMP2)	/* into %VOL_TEMP1 */
	set	_machLastSysCallOffset, %VOL_TEMP2
	ld	[%VOL_TEMP2], %VOL_TEMP2
	add	%VOL_TEMP1, %VOL_TEMP2, %VOL_TEMP1
	st	%g1, [%VOL_TEMP1]
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
	MACH_GET_CUR_PROC_PTR(%VOL_TEMP1)		/* into %VOL_TEMP1 */
	set	_machKcallTableOffset, %VOL_TEMP2
	ld	[%VOL_TEMP2], %VOL_TEMP2		/* offset to kcalls */
	add	%VOL_TEMP1, %VOL_TEMP2, %VOL_TEMP1
	ld	[%VOL_TEMP1], %VOL_TEMP1		/* addr of array */
	add	%g1, %VOL_TEMP1, %VOL_TEMP1		/* index to kcall */
	ld	[%VOL_TEMP1], %VOL_TEMP1		/* got addr */
	/* enable interrupts */
	QUICK_ENABLE_INTR(%VOL_TEMP2)
	/* go do it */
	call	%VOL_TEMP1
	nop
ReturnFromSyscall:
	/* Disable interrupts. */
	QUICK_DISABLE_INTR(%VOL_TEMP1)
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
#ifdef sun4c
	/* synchronous error register value as first arg - clears it too. */
	set	VMMACH_SYNC_ERROR_REG, %o0
	lda	[%o0] VMMACH_CONTROL_SPACE, %o0

	/* synch error address register value as second arg - clears it. */
	set	VMMACH_SYNC_ERROR_ADDR_REG, %o1
	lda	[%o1] VMMACH_CONTROL_SPACE, %o1

	/* clear async regs as well since they latch on sync errors as well. */
	set	VMMACH_ASYNC_ERROR_REG, %VOL_TEMP1
	lda	[%VOL_TEMP1] VMMACH_CONTROL_SPACE, %g0

	/* async error addr reg as well, so clear it.  Very silly. */
	set	VMMACH_ASYNC_ERROR_ADDR_REG, %VOL_TEMP1
	lda	[%VOL_TEMP1] VMMACH_CONTROL_SPACE, %g0
#else
	/* bus error register value as first arg. */
	set	VMMACH_BUS_ERROR_REG, %o0
	lduba	[%o0] VMMACH_CONTROL_SPACE, %o0

	/* memory address causing the error as second arg */
	set	VMMACH_ADDR_ERROR_REG, %o1
	ld	[%o1], %o1

	/* Write the address register to clear it */
	set	VMMACH_ADDR_ERROR_REG, %VOL_TEMP1
	st	%o1, [%VOL_TEMP1]
#endif
	/* trap value of the psr as third arg */
	mov	%CUR_PSR_REG, %o2	

	/* trap value of pc as fourth argument */
	mov	%CUR_PC_REG, %o3
#ifndef sun4c
	/*
	 * If the trap was on a pc access rather than a data access, the
	 * memoory address register won't have frozen the correct address.
	 * Just move the pc into the second argument.
	 */
	and	%CUR_TBR_REG, MACH_TRAP_TYPE_MASK, %VOL_TEMP1
	cmp	%VOL_TEMP1, MACH_INSTR_ACCESS
	bne	AddressValueOkay
	nop
	mov	%CUR_PC_REG, %o1
#endif
AddressValueOkay:
	/* enable interrupts */
	QUICK_ENABLE_INTR(%VOL_TEMP1)
	call	_MachPageFault, 4
	nop
	/* Disable interrupts. */
	QUICK_DISABLE_INTR(%VOL_TEMP1)

	set	_MachReturnFromTrap, %VOL_TEMP1
	jmp	%VOL_TEMP1
	nop


/*
 * ----------------------------------------------------------------------
 *
 * MachFlushWindowsToStackTrap --
 *
 *	A trap requesting that all our windows be flushed to the stack has
 *	occured.  Bump up our return pc, since we don't want to re-execute
 *	the trap, and then call the appropriate routine.
 *
 * Results:
 *	Returns to MachReturnFromTrap, rather than our caller.
 *
 * Side effects:
 *	Register windows will be flushed to the stack and the invalid
 *	window mask will be set to point to the window before us.
 *
 * ----------------------------------------------------------------------
 */
.global	MachFlushWindowsToStackTrap
MachFlushWindowsToStackTrap:
	mov	%NEXT_PC_REG, %CUR_PC_REG
	add	%NEXT_PC_REG, 4, %NEXT_PC_REG
	call	_MachFlushWindowsToStack
	nop
	set	_MachReturnFromTrap, %VOL_TEMP1
	jmp	%VOL_TEMP1
	nop
