/* machAsm.s --
 *
 *     Contains misc. assembler routines for the sun4.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 *
 * rcs = $Header$ SPRITE (Berkeley)
 */

#include "machConst.h"
#include "machAsmDefs.h"
.seg	"text"

/*
 * ----------------------------------------------------------------------
 *
 * __MachGetPc --
 *
 *	Jump back to caller without over-writing pc that was put into
 *	%RETURN_VAL_REG as a side effect of the jmp to this routine.
 *
 * Results:
 *	Old pc is returned in %RETURN_VAL_REG.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------
 */
.globl	__MachGetPc
__MachGetPc:
	jmpl	%RETURN_VAL_REG, %g0
	nop

/*
 * ----------------------------------------------------------------------
 *
 * Mach_DisableIntr --
 *
 *	Disable interrupts.  This leaves nonmaskable interrupts enabled.
 *	Note that this uses out registers.  Since it's a leaf
 *	routine, callable from C, it cannot use %VOL_TEMP1 and %VOL_TEMP2.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Maskable interrupts are disabled.
 *
 * ----------------------------------------------------------------------
 */
.globl	_Mach_DisableIntr
_Mach_DisableIntr:
	mov %psr, %OUT_TEMP1
	set MACH_DISABLE_INTR, %OUT_TEMP2
	or %OUT_TEMP1, %OUT_TEMP2, %OUT_TEMP1
	mov %OUT_TEMP1, %psr
	nop					/* time for valid state reg */
	retl
	nop

/*
 * ----------------------------------------------------------------------
 *
 * Mach_EnableIntr --
 *
 *      Enable interrupts.
 *	Note that this uses out registers.  Since it's a leaf
 *	routine, callable from C, it cannot use %VOL_TEMP1 and %VOL_TEMP2.
 *	This enables all interrupts, so if before disabling them
 *	we had only certain priority interrupts enabled, this will lose
 *	that information.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *      Interrupts are enabled.
 *
 * ----------------------------------------------------------------------
 */
.globl	_Mach_EnableIntr
_Mach_EnableIntr:
	mov %psr, %OUT_TEMP1
	set MACH_ENABLE_INTR, %OUT_TEMP2
	and %OUT_TEMP1, %OUT_TEMP2, %OUT_TEMP1
	mov %OUT_TEMP1, %psr
	nop					/* time for valid state reg */
	retl
	nop

/*
 * Jmpl writes current pc into RETURN_VAL_REG_CHILD here, which is where values
 * are returned from calls.  The return from __MachGetPc must not overwrite 
 * this.  Since this uses a non-pc-relative jump, we CANNOT use this routine 
 * while executing before we've copied the kernel to where it was linked for. 
 */
.globl	_Mach_GetPC
_Mach_GetPC:
	set	__MachGetPc, %RETURN_VAL_REG
	jmpl	%RETURN_VAL_REG, %RETURN_VAL_REG
	nop
	retl
	nop

/*
 *---------------------------------------------------------------------
 *
 * Mach_TestAndSet --
 *
 *     int Mach_TestAndSet(intPtr)
 *          int *intPtr;
 *
 *     Test and set an operand.
 *
 * Results:
 *     Returns 0 if *intPtr was zero and 1 if *intPtr was non-zero.  Also
 *     in all cases *intPtr is set to a non-zero value.
 *
 * Side effects:
 *     None.
 *
 *---------------------------------------------------------------------
 */
.globl	_Mach_TestAndSet
_Mach_TestAndSet:
	/*
	 * We're a leaf routine, so our return value goes in the save register
	 * that our operand does.  Move the operand before overwriting.
	 * The ISP description of the swap instruction indicates that it is
	 * okay to use the same address and destination registers.  This will
	 * put the address into the addressed location to set it.  That means
	 * we can't use address 0, but we shouldn't be doing that anyway.
	 */
#ifdef	NO_SUN4_CACHING
	DISABLE_INTR_ASM(%OUT_TEMP1, %OUT_TEMP2, TestAndSetDisableIntrs)
	set	0x1, %OUT_TEMP1
	ld	[%RETURN_VAL_REG], %OUT_TEMP2
	st	%OUT_TEMP1, [%RETURN_VAL_REG]
	mov	%OUT_TEMP2, %RETURN_VAL_REG
	ENABLE_INTR_ASM(%OUT_TEMP1, %OUT_TEMP2, TestAndSetEnableIntrs)
#else
	swap	[%RETURN_VAL_REG], %RETURN_VAL_REG	/* set addr with addr */
#endif /* NO_SUN4_CACHING */
	tst	%RETURN_VAL_REG				/* was it set? */
	be,a	ReturnZero				/* if not, return 0 */
	clr	%RETURN_VAL_REG				/* silly, delay slot */
	mov	0x1, %RETURN_VAL_REG			/* yes set, return 1 */
ReturnZero:
	retl
	nop


/*
 *---------------------------------------------------------------------
 *
 * Mach_GetMachineType -
 *
 *	Returns the type of machine that is stored in the id prom.
 *
 *	int	Mach_GetMachineType()
 *
 * Results:
 *	The type of machine.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------
 */
.globl	_Mach_GetMachineType
_Mach_GetMachineType:
	set	VMMACH_MACH_TYPE_ADDR, %o0
	lduba	[%o0] VMMACH_CONTROL_SPACE, %o0
	retl
	nop

/*
 *---------------------------------------------------------------------
 *
 * Mach_MonTrap -
 *
 *      Trap to the monitor.  This involves dummying up a trap stack for the
 *      monitor, allowing non-maskable interrupts and then jumping to the
 *      monitor trap routine.  When it returns, non-maskable interrupts are
 *      disabled and we return.
 *
 *	void	Mach_MonTrap(address_to_trap_to)
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      None.
 *
 * --------------------------------------------------------------------
 */
.global	_Mach_MonTrap
_Mach_MonTrap:
	save	%sp, -MACH_SAVED_STATE_FRAME, %sp
	/* enable non-maskable interrupts? */
	/* get address */
	call	%i0
	nop
	/* disable non-maskable interrupts? */
	ret
	restore


/*
 *---------------------------------------------------------------------
 *
 * Mach_ContextSwitch -
 *
 *	Mach_ContextSwitch(fromProcPtr, toProcPtr)
 *
 *	Switch the thread of execution to a new process.  This routine
 *	is passed a pointer to the process to switch from and a pointer to
 *	the process to switch to.  It goes through the following steps:
 *
 *	1) Change to the new context.
 *	2) save stuff
 *	3) restore stuff
 *	4) Return in the new process
 *
 *	The kernel stack is changed implicitly when the registers are restored.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The kernel stack, all general registers, and some special-purpose
 *	registers are changed.
 *
 *---------------------------------------------------------------------
 */
.globl	_Mach_ContextSwitch
_Mach_ContextSwitch:
	/*
	 * We call stuff here, so we need a new window.  An overflow trap
	 * is okay here.  Save enough space for the context switch state on
	 * the stack.
	 */
	save 	%sp, -MACH_SAVED_STATE_FRAME, %sp
	andn	%sp, 0x7, %sp		/* should already be okay */
	mov	%psr, %CUR_PSR_REG
	MACH_SR_HIGHPRIO()		/*
					 * MAY BE EXTRANEOUS - interrupts should
					 * already be off!
					 */

	/*
	 * Switch contexts to that of toProcPtr.  It's the second arg, so
	 * move it to be first arg of routine we call.
	 */
	mov	%i1, %o0
	call	_VmMach_SetupContext, 1
	nop
	/*
	 * Context to use was returned in %RETURN_VAL_REG.  Set the context
	 * to that value.  VmMachSetContextReg is a leaf routine, so we
	 * leave the value for its argument in %RETURN_VAL_REG.
	 */
	call	_VmMachSetContextReg, 1
	nop
	/*
	 * Save stack pointer into state struct.  This is also the pointer
	 * in the mach state struct of the saved context switch state.  It
	 * just so happens it's stored on the stack...
	 */
	set	_machStatePtrOffset, %VOL_TEMP1
	ld	[%VOL_TEMP1], %VOL_TEMP1		/* get offset */
	add	%i0, %VOL_TEMP1 , %VOL_TEMP1		/* add to base */
	ld	[%VOL_TEMP1], %VOL_TEMP1		/* get machStatePtr */
	add	%VOL_TEMP1, MACH_SWITCH_REGS_OFFSET, %VOL_TEMP1
	st	%sp, [%VOL_TEMP1]
	
	/*
	 * Where was trapRegs pointer saved into mach state?
	 * if this were user process, save user stack pointer?
	 */

	/*
	 * Save context switch state, globals only for the time being...
	 * This gets saved in our stack frame.
	 */
	MACH_SAVE_GLOBAL_STATE()

	/*
	 * Now make sure all windows are flushed to the stack.  We do this
	 * with MACH_NUM_WINDOWS - 1 saves and restores.  The window overflow
	 * and underflow traps will then handle this for us.  We put the counter
	 * in a global, since they have all been saved already and are free
	 * for our use and we need the value across windows.
	 */
	set	(MACH_NUM_WINDOWS - 1), %g1
ContextSaveSomeMore:
	save
	subcc	%g1, 1, %g1
	bne	ContextSaveSomeMore
	nop
	set	(MACH_NUM_WINDOWS - 1), %g1
ContextRestoreSomeMore:
	restore
	subcc	%g1, 1, %g1
	bne	ContextRestoreSomeMore
	nop

	/*
	 * Restore stack pointer of new process - WE SWITCH STACKS HERE!!!
	 * the pointer to the new processes' state structure goes into
	 * %SAFE_TEMP.
	 */
	set	_machStatePtrOffset, %VOL_TEMP1
	ld	[%VOL_TEMP1], %VOL_TEMP1	/* get offset */
	add	%i1, %VOL_TEMP1, %VOL_TEMP1	/* &(procPtr->machStatePtr) */
	ld	[%VOL_TEMP1], %SAFE_TEMP	/* procPtr->machStatePtr */
	add	%SAFE_TEMP, MACH_SWITCH_REGS_OFFSET, %VOL_TEMP1
	ld	[%VOL_TEMP1], %sp

	/* restore global registers of new process */
	MACH_RESTORE_GLOBAL_STATE()

	/*
	 * Restore the local and in registers for this process.  (We need
	 * at least the psr in %CUR_PSR_REG and the fp and return pc.)
	 * To do this we set the wim to this window and do a save and restore.
	 * That way the registers are automatically saved from the stack.
	 * We could instead save all these explicitly in the state structure
	 * and not do a save and restore, but that wouldn't save that many
	 * loads and it also would be different than how we save and restore
	 * with the debugger.  It's nice to have them the same.
	 */
	MACH_SET_WIM_TO_CWP()
	save
	restore
	/* restore user stack pointer if a user process? */

	/*
	 * Save the pointer to the current mach state structure that's in
	 * a local register into a global variable.
	 */
	set	_machCurStatePtr, %VOL_TEMP1
	st	%SAFE_TEMP, [%VOL_TEMP1]

	/*
	 * Restore status register in such a way that it doesn't make
	 * us change windows.
	 */
	MACH_RESTORE_PSR()

	ret
	restore

/*
 *---------------------------------------------------------------------
 *
 * MachRunUserProc -
 *
 *	void	MachRunUserProc()
 *
 *	Make the first user process start off by returning it from a kernel
 *	trap.
 *	Interrupts must be disabled coming into this.
 *
 * Results:
 *	Restore registers and return to user space.
 *
 * Side effects:
 *	Registers restored.
 *
 *---------------------------------------------------------------------
 */
.globl	_MachRunUserProc
_MachRunUserProc:
	/*
	 * Get values to restore registers to from the state structure.
	 */
#ifdef NOTDEF
	set	_proc_RunningProcesses, %VOL_TEMP1
	ld	[%VOL_TEMP1], %VOL_TEMP1		/* procPtr */
	set	_machStatePtrOffset, %VOL_TEMP2
	ld	[%VOL_TEMP2], %VOL_TEMP2		/* get offset */
	add	%VOL_TEMP1, %VOL_TEMP2, %VOL_TEMP1
	ld	[%VOL_TEMP1], %VOL_TEMP1		/* machStatePtr */
	add	%VOL_TEMP1, MACH_TRAP_REGS_OFFSET, %VOL_TEMP1
	ld	[%VOL_TEMP1], %VOL_TEMP2	/* machStatePtr->trapRegs */
	/*
	 * Restore %fp.  This will be the user's %sp when we return from
	 * the trap window.
	 */
	add	%VOL_TEMP2, MACH_FP_OFFSET, %VOL_TEMP1
	ld	[%VOL_TEMP1], %fp		/* set %fp - user sp */
#else
	mov	%o0, %CUR_PC_REG
	add	%o0, 4, %NEXT_PC_REG
	mov	%o1, %fp
#endif NOTDEF
	andcc	%fp, 0x7, %fp
	be	UserStackOkay
	nop
	/*
	 * User stack wasn't aligned.  It should have been when it got here!
	 */
	set	PROC_TERM_DESTROYED, %o0
	set	PROC_BAD_STACK, %o1
	clr	%o2
	call	_Proc_ExitInt, 3
	nop
UserStackOkay:
	/*
	 * So that user stack has space for saved window area and storage
	 * of callee's input registers.
	 */
	add	%fp, -MACH_FULL_STACK_FRAME, %fp

#ifdef NOTDEF
	/*
	 * Set return from trap pc and next pc.
	 */
	add	%VOL_TEMP2, MACH_RETPC_OFFSET, %VOL_TEMP1
	ld	[%VOL_TEMP1], %CUR_PC_REG
	add	%VOL_TEMP1, 4, %VOL_TEMP1
	ld	[%VOL_TEMP1], %NEXT_PC_REG
#endif NOTDEF
	/*
	 * Make sure traps are disabled before setting up next psr value.
	 * Next psr value will have all interrupts enabled, so we make sure
	 * traps are disabled here so we don't get one of those interrupts
	 * before returning to user space.
	 */
	set	MACH_DISABLE_TRAP_BIT, %VOL_TEMP1
	mov	%psr, %VOL_TEMP2
	and	%VOL_TEMP2, %VOL_TEMP1, %VOL_TEMP2
	mov	%VOL_TEMP2, %psr
	MACH_WAIT_FOR_STATE_REGISTER()
	set	MACH_FIRST_USER_PSR, %CUR_PSR_REG

	/*
	 * Put a happy return value into the return value register.  This
	 * probably doesn't matter at all.
	 */
	clr	%i0

	/*
	 * Make sure invalid window is 1 in front of the window we'll return to.
	 * This makes sure we don't get a watchdog reset returning from the
	 * trap window, because we'll be sure the window we return to is valid.
	 * Making the window before that be the invalid window means that we'll
	 * be able to start out life for a while without a window overflow
	 * trap.
	 */
	MACH_SET_WIM_TO_CWP()
	MACH_RETREAT_WIM(%VOL_TEMP1, %VOL_TEMP2, FirstRetreat)
	MACH_RETREAT_WIM(%VOL_TEMP1, %VOL_TEMP2, SecondRetreat)

	/*
	 * Restore psr
	 */
	MACH_RESTORE_PSR()
	jmp	%CUR_PC_REG
	rett	%NEXT_PC_REG
	nop

/*
 *---------------------------------------------------------------------
 *
 * MachGetCurrentSp -
 *
 *	Address	MachGetCurrentSp()
 *
 *	Return the value of the current stack pointer.
 *
 * Results:
 *	The current stack pointer.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------
 */
.globl	_MachGetCurrentSp
_MachGetCurrentSp:
	mov	%sp, %o0
	retl
	nop
