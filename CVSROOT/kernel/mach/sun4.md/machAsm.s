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
#ifdef NOTDEF
	/* save old addresses for debugging */
	set	_theAddrOfVmPtr, %VOL_TEMP1
	ld	[%VOL_TEMP1], %VOL_TEMP1
	set	_oldAddrOfVmPtr, %VOL_TEMP2
	st	%VOL_TEMP1, [%VOL_TEMP2]
	tst	%VOL_TEMP1
	be	FirstTimeDontPrint
	nop
	ld	[%VOL_TEMP1], %VOL_TEMP1	/* value of vmPtr */
	MACH_DEBUG_BUF(%OUT_TEMP1, %OUT_TEMP2, DebugContextSwitch5, %VOL_TEMP1)
	set	_theAddrOfMachPtr, %VOL_TEMP1
	ld	[%VOL_TEMP1], %VOL_TEMP1
	set	_oldAddrOfMachPtr, %VOL_TEMP2
	st	%VOL_TEMP1, [%VOL_TEMP2]
	ld	[%VOL_TEMP1], %VOL_TEMP1	/* value of vmPtr */
	MACH_DEBUG_BUF(%OUT_TEMP1, %OUT_TEMP2, DebugContextSwitch6, %VOL_TEMP1)
FirstTimeDontPrint:
#endif NOTDEF


	/*
	 * Switch contexts to that of toProcPtr.  It's the second arg, so
	 * move it to be first arg of routine we call.
	 */
	mov	%i1, %o0
	call	_VmMach_SetupContext, 1	/* does the work, no? */
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
	set	_machMachProcOffset, %VOL_TEMP1
	ld	[%VOL_TEMP1], %VOL_TEMP1
	add	%i0, %VOL_TEMP1 , %VOL_TEMP1
	/* AAAACCCK! */
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

	/* restore stack pointer of new process - WE SWITCH STACKS HERE!!! */
	set	_machMachProcOffset, %VOL_TEMP1
	ld	[%VOL_TEMP1], %VOL_TEMP1
	add	%i1, %VOL_TEMP1, %VOL_TEMP1	/* &(procPtr->machStatePtr) */
	ld	[%VOL_TEMP1], %VOL_TEMP1	/* procPtr->machStatePtr */
	add	%VOL_TEMP1, MACH_SWITCH_REGS_OFFSET, %VOL_TEMP1
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

#ifdef NOTDEF
	set	_theAddrOfVmPtr, %OUT_TEMP1	/* get addr of vmPtr */
	ld	[%OUT_TEMP1], %OUT_TEMP1
	ld	[%OUT_TEMP1], %OUT_TEMP1	/* get value of vmPtr */
	MACH_DEBUG_BUF(%VOL_TEMP1, %VOL_TEMP2, DebugContextSwitch11, %OUT_TEMP1)

	set	_theAddrOfMachPtr, %OUT_TEMP1
	ld	[%OUT_TEMP1], %OUT_TEMP1
	ld	[%OUT_TEMP1], %OUT_TEMP1	/* value of machPtr */
	MACH_DEBUG_BUF(%VOL_TEMP1, %VOL_TEMP2, DebugContextSwitch12, %OUT_TEMP1)

	set	_oldAddrOfVmPtr, %OUT_TEMP1	/* get addr of old vmPtr */
	ld	[%OUT_TEMP1], %OUT_TEMP1
	tst	%OUT_TEMP1
	be	AgainFirstTimeDontPrint
	nop
	ld	[%OUT_TEMP1], %OUT_TEMP1	/* get value of old vmPtr */
	MACH_DEBUG_BUF(%VOL_TEMP1, %VOL_TEMP2, DebugContextSwitch13, %OUT_TEMP1)

	set	_oldAddrOfMachPtr, %OUT_TEMP1
	ld	[%OUT_TEMP1], %OUT_TEMP1
	ld	[%OUT_TEMP1], %OUT_TEMP1	/* value of old machPtr */
	MACH_DEBUG_BUF(%VOL_TEMP1, %VOL_TEMP2, DebugContextSwitch14, %OUT_TEMP1)
AgainFirstTimeDontPrint:

	MACH_DEBUG_BUF(%VOL_TEMP1, %VOL_TEMP2, DebugContextSwitch15, %CUR_PSR_REG)
#endif NOTDEF

	/*
	 * Restore status register in such a way that it doesn't make
	 * us change windows.
	 */
	MACH_RESTORE_PSR()

	ret
	restore
