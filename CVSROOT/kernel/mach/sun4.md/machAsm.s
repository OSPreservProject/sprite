/* machAsm.s --
 *
 *     Contains misc. assembler routines for the sun4.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 *
 * rcs = $Header$ SPRITE (Berkeley)
 */

#include "user/proc.h"
#include "machConst.h"
#include "machAsmDefs.h"
.seg	"text"

/*
 * ----------------------------------------------------------------------
 *
 * Mach_GetPC --
 *
 *	Jump back to caller, moving the return address that was put into
 *	%RETURN_ADDR_REG as a side effect of the call into the %RETURN_VAL_REG.
 *
 * Results:
 *	Old pc is returned in %RETURN_VAL_REG.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------
 */
.globl	_Mach_GetPC
_Mach_GetPC:
	retl
	mov	%RETURN_ADDR_REG, %RETURN_VAL_REG


/*
 * ----------------------------------------------------------------------
 *
 * Mach_GetCallerPC --
 *
 *	Return the PC of the caller of the routine who called us.
 *
 * Results:
 *	Our caller's caller's pc.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------
 */
.globl	_Mach_GetCallerPC
_Mach_GetCallerPC:
	retl
	mov	%RETURN_ADDR_REG_CHILD, %RETURN_VAL_REG


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
#ifdef SWAP_INSTR_WORKS
	swap	[%RETURN_VAL_REG], %RETURN_VAL_REG	/* set addr with addr */
#else
	ldstub	[%RETURN_VAL_REG], %RETURN_VAL_REG	/* value into reg */
#endif /* SWAP_INSTR_WORKS */
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
#ifdef sun4c
	ldub	[%o0], %o0
#else
	lduba	[%o0] VMMACH_CONTROL_SPACE, %o0
#endif
	retl
	nop


/*
 *---------------------------------------------------------------------
 *
 * Mach_GetEtherAddress -
 *
 *	Returns the type of machine that is stored in the id prom.
 *
 *	int *	Mach_GetEtherAddress(ether)
 *
 * Results:
 *	The argument struct gets the prom's ethernet address and is
 *	returned.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------
 */
.globl	_Mach_GetEtherAddress
_Mach_GetEtherAddress:
	set	VMMACH_ETHER_ADDR, %OUT_TEMP1
	/* first byte */
#ifdef sun4c
	ldub	[%OUT_TEMP1], %OUT_TEMP2
#else
	lduba	[%OUT_TEMP1] VMMACH_CONTROL_SPACE, %OUT_TEMP2
#endif
	stb	%OUT_TEMP2, [%o0]
	add	%OUT_TEMP1, VMMACH_IDPROM_INC, %OUT_TEMP1
	add	%o0, 1, %o0

	/* second byte */
#ifdef sun4c
	ldub	[%OUT_TEMP1], %OUT_TEMP2
#else
	lduba	[%OUT_TEMP1] VMMACH_CONTROL_SPACE, %OUT_TEMP2
#endif
	stb	%OUT_TEMP2, [%o0]
	add	%OUT_TEMP1, VMMACH_IDPROM_INC, %OUT_TEMP1
	add	%o0, 1, %o0

	/* third byte */
#ifdef sun4c
	ldub	[%OUT_TEMP1], %OUT_TEMP2
#else
	lduba	[%OUT_TEMP1] VMMACH_CONTROL_SPACE, %OUT_TEMP2
#endif
	stb	%OUT_TEMP2, [%o0]
	add	%OUT_TEMP1, VMMACH_IDPROM_INC, %OUT_TEMP1
	add	%o0, 1, %o0

	/* fourth byte */
#ifdef sun4c
	ldub	[%OUT_TEMP1], %OUT_TEMP2
#else
	lduba	[%OUT_TEMP1] VMMACH_CONTROL_SPACE, %OUT_TEMP2
#endif
	stb	%OUT_TEMP2, [%o0]
	add	%OUT_TEMP1, VMMACH_IDPROM_INC, %OUT_TEMP1
	add	%o0, 1, %o0

	/* fifth byte */
#ifdef sun4c
	ldub	[%OUT_TEMP1], %OUT_TEMP2
#else
	lduba	[%OUT_TEMP1] VMMACH_CONTROL_SPACE, %OUT_TEMP2
#endif
	stb	%OUT_TEMP2, [%o0]
	add	%OUT_TEMP1, VMMACH_IDPROM_INC, %OUT_TEMP1
	add	%o0, 1, %o0

	/* sixth byte */
#ifdef sun4c
	ldub	[%OUT_TEMP1], %OUT_TEMP2
#else
	lduba	[%OUT_TEMP1] VMMACH_CONTROL_SPACE, %OUT_TEMP2
#endif
	stb	%OUT_TEMP2, [%o0]
	sub	%o0, 5, %o0		/* restore pointer parameter */

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
	mov	%psr, %CUR_PSR_REG

	/*
	 * Enable the floating point processor so we can save and
	 * restore FPU state if necessary.
	 */
	set	MACH_ENABLE_FPP, %VOL_TEMP1
	or	%CUR_PSR_REG,%VOL_TEMP1,%VOL_TEMP1
	mov	%VOL_TEMP1, %psr
	/*
	 * No delay instructions needed here as long as the following
	 * three instructions don't touch the FPU.
	 *
	 * Save stack pointer into state struct.  This is also the pointer
	 * in the mach state struct of the saved context switch state.  It
	 * just so happens it's stored on the stack...
	 */
	set	_machStatePtrOffset, %VOL_TEMP1
	ld	[%VOL_TEMP1], %VOL_TEMP1		/* get offset */
		/* FPU is now active. */
	add	%i0, %VOL_TEMP1 , %VOL_TEMP1		/* add to base */
	ld	[%VOL_TEMP1], %SAFE_TEMP		/* get machStatePtr */
	add	%SAFE_TEMP, MACH_SWITCH_REGS_OFFSET, %VOL_TEMP1
	st	%sp, [%VOL_TEMP1]

	/*
	 * If FPU is active for this process, save FPU state.
	 */
	ld	[%SAFE_TEMP+MACH_FPU_STATUS_OFFSET], %VOL_TEMP1
	tst	%VOL_TEMP1
	bz	noFPUactive
	/*
	 *  Note: Part of following set instruction executed in "bz" delay
	 *  slot.  This trashes %VOL_TEMP1 when the branch is taken.
	 *  
	 *  We are saving the user's FPU state into the trap regs during
	 *  the context switch because we don't want the latency of saving
	 *  all the floating point state on every trap.  This works because
	 *  the kernel doesn't user floating point instructions.  
	 *
	 *  Tricky code - We save the fromProcPtr in _machFPUSaveProcPtr and
	 *  execute a "st %fsr". The "st fsr" will pause until all FPU ops
	 *  are compete.  If any FPU execptions are pending we will get a
	 *  FP_EXCEPTION trap on the instruction marked by the label 
	 *  _machFPUSyncInst. The routine MachHandleTrap in
	 *  machCode.c will catch this fault and note it in the processes
	 *  PCB.  
	 */
	set	_machFPUSaveProcPtr, %VOL_TEMP1
	st	%i0, [%VOL_TEMP1];
	/*
	 * Set %SAFE_TEMP to be trapRegsPtr.
	 */
	ld	[%SAFE_TEMP + MACH_TRAP_REGS_OFFSET], %SAFE_TEMP
.global _machFPUSyncInst
_machFPUSyncInst: st	%fsr, [%SAFE_TEMP + MACH_FPU_FSR_OFFSET]
        /*
	 * The following symbol is include to make the following code 
	 * be attributed to the routine Mach_ContextSwitch2 and not
	 * _machFPUSyncInst.
	 */
.globl	Mach_ContextSwitch2
Mach_ContextSwitch2:
	std %f0, [%SAFE_TEMP + MACH_FPU_REGS_OFFSET + 4*0]
	std %f2, [%SAFE_TEMP + MACH_FPU_REGS_OFFSET + 4*2]
	std %f4, [%SAFE_TEMP + MACH_FPU_REGS_OFFSET + 4*4]
	std %f6, [%SAFE_TEMP + MACH_FPU_REGS_OFFSET + 4*6]
	std %f8, [%SAFE_TEMP + MACH_FPU_REGS_OFFSET + 4*8]
	std %f10, [%SAFE_TEMP + MACH_FPU_REGS_OFFSET + 4*10]
	std %f12, [%SAFE_TEMP + MACH_FPU_REGS_OFFSET + 4*12]
	std %f14, [%SAFE_TEMP + MACH_FPU_REGS_OFFSET + 4*14]
	std %f16, [%SAFE_TEMP + MACH_FPU_REGS_OFFSET + 4*16]
	std %f18, [%SAFE_TEMP + MACH_FPU_REGS_OFFSET + 4*18]
	std %f20, [%SAFE_TEMP + MACH_FPU_REGS_OFFSET + 4*20]
	std %f22, [%SAFE_TEMP + MACH_FPU_REGS_OFFSET + 4*22]
	std %f24, [%SAFE_TEMP + MACH_FPU_REGS_OFFSET + 4*24]
	std %f26, [%SAFE_TEMP + MACH_FPU_REGS_OFFSET + 4*26]
	std %f28, [%SAFE_TEMP + MACH_FPU_REGS_OFFSET + 4*28]
	std %f30, [%SAFE_TEMP + MACH_FPU_REGS_OFFSET + 4*30]
noFPUactive:
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

	/* Update machCurStatePtr */
	set	_machCurStatePtr, %VOL_TEMP1
	st	%SAFE_TEMP, [%VOL_TEMP1]
	/*
	 * If FPU is active for this process, restore the FPU state.
	 */
	ld	[%SAFE_TEMP+MACH_FPU_STATUS_OFFSET], %VOL_TEMP1
	tst	%VOL_TEMP1
	bz	dontRestoreFPU
	nop
	/*
	 * Set %SAFE_TEMP to be machStatePtr.
	 */
	ld  [%SAFE_TEMP + MACH_TRAP_REGS_OFFSET], %SAFE_TEMP
	ld  [%SAFE_TEMP + MACH_FPU_FSR_OFFSET], %fsr
	ldd [%SAFE_TEMP + MACH_FPU_REGS_OFFSET + 4*0],%f0
	ldd [%SAFE_TEMP + MACH_FPU_REGS_OFFSET + 4*2],%f2 
	ldd [%SAFE_TEMP + MACH_FPU_REGS_OFFSET + 4*4],%f4 
	ldd [%SAFE_TEMP + MACH_FPU_REGS_OFFSET + 4*6],%f6 
	ldd [%SAFE_TEMP + MACH_FPU_REGS_OFFSET + 4*8],%f8 
	ldd [%SAFE_TEMP + MACH_FPU_REGS_OFFSET + 4*10],%f10
	ldd [%SAFE_TEMP + MACH_FPU_REGS_OFFSET + 4*12],%f12
	ldd [%SAFE_TEMP + MACH_FPU_REGS_OFFSET + 4*14],%f14 
	ldd [%SAFE_TEMP + MACH_FPU_REGS_OFFSET + 4*16],%f16 
	ldd [%SAFE_TEMP + MACH_FPU_REGS_OFFSET + 4*18],%f18 
	ldd [%SAFE_TEMP + MACH_FPU_REGS_OFFSET + 4*20],%f20 
	ldd [%SAFE_TEMP + MACH_FPU_REGS_OFFSET + 4*22],%f22
	ldd [%SAFE_TEMP + MACH_FPU_REGS_OFFSET + 4*24],%f24 
	ldd [%SAFE_TEMP + MACH_FPU_REGS_OFFSET + 4*26],%f26
	ldd [%SAFE_TEMP + MACH_FPU_REGS_OFFSET + 4*28],%f28
	ldd [%SAFE_TEMP + MACH_FPU_REGS_OFFSET + 4*30],%f30
dontRestoreFPU:

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
 *	MachRunUserProc();
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
	 * We set up %VOL_TEMP2 to point to trapRegs structure and restore
	 * from there.
	 */
	MACH_GET_CUR_STATE_PTR(%VOL_TEMP1, %VOL_TEMP2)	/* into %VOL_TEMP1 */
	set	_machCurStatePtr, %VOL_TEMP2
	st	%VOL_TEMP1, [%VOL_TEMP2]
	add	%VOL_TEMP1, MACH_TRAP_REGS_OFFSET, %VOL_TEMP1
	ld	[%VOL_TEMP1], %VOL_TEMP2	/* machStatePtr->trapRegs */
	mov	%VOL_TEMP2, %sp			/* set sp to trapRegs */
	/*
	 * Restore %fp.  This will be the user's %sp when we return from
	 * the trap window.
	 */
	add	%VOL_TEMP2, MACH_FP_OFFSET, %VOL_TEMP1
	ld	[%VOL_TEMP1], %fp		/* set %fp - user sp */
	andcc	%fp, 0x7, %g0
	be	UserStackOkay
	nop
	/*
	 * User stack wasn't aligned.  It should have been when it got here!
	 */
	set	_MachRunUserDeathString, %o0
	call	_printf, 1
	nop
	set	PROC_TERM_DESTROYED, %o0
	set	PROC_BAD_STACK, %o1
	clr	%o2
	set	_Proc_ExitInt, %VOL_TEMP1
	call	%VOL_TEMP1, 3
	nop
UserStackOkay:
	/*
	 * Set return from trap pc and next pc.
	 */
	add	%VOL_TEMP2, MACH_TRAP_PC_OFFSET, %VOL_TEMP1
	ld	[%VOL_TEMP1], %CUR_PC_REG
	add	%VOL_TEMP1, 4, %VOL_TEMP1
	ld	[%VOL_TEMP1], %NEXT_PC_REG
	add	%VOL_TEMP1, 4, %VOL_TEMP1
	ld	[%VOL_TEMP1], %CUR_TBR_REG

	/*
	 * Put a return value into the return value register.
	 */
	add	%VOL_TEMP2, MACH_ARG0_OFFSET, %VOL_TEMP1
	ld	[%VOL_TEMP1], %i0
	/*
	 * Restore the other in registers, which were trapper's out regs.
	 * We've already restored fp = i6, so skip it.
	 */
	add	%VOL_TEMP1, 4, %VOL_TEMP1
	ld	[%VOL_TEMP1], %i1
	add	%VOL_TEMP1, 4, %VOL_TEMP1
	ld	[%VOL_TEMP1], %i2
	add	%VOL_TEMP1, 4, %VOL_TEMP1
	ld	[%VOL_TEMP1], %i3
	add	%VOL_TEMP1, 4, %VOL_TEMP1
	ld	[%VOL_TEMP1], %i4
	add	%VOL_TEMP1, 4, %VOL_TEMP1
	ld	[%VOL_TEMP1], %i5

	add	%VOL_TEMP1, 8, %VOL_TEMP1
	ld	[%VOL_TEMP1], %i7
	/*
	 * Get new psr value.
	 */
	add	%VOL_TEMP2, MACH_PSR_OFFSET, %VOL_TEMP1
	ld	[%VOL_TEMP1], %CUR_PSR_REG

	/*
	 * Now go through regular return from trap code.
	 * We must make sure that the window we will return to from the trap
	 * is invalid when we get into MachReturnFromTrap so that it will
	 * be restored from the user stack.  Otherwise the in registers (such
	 * as frame pointer) in that window we return to will be bad and
	 * won't be restored correctly.
	 */
	MACH_SET_WIM_TO_CWP();
	MACH_RETREAT_WIM(%VOL_TEMP1, %VOL_TEMP2, SetWimForChild);

	set	_MachReturnFromTrap, %VOL_TEMP1
	jmp	%VOL_TEMP1
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


/*
 *---------------------------------------------------------------------
 *
 * MachHandleSignal -
 *
 *	void MachHandleSignal()
 *
 *	Setup the necessary stack to handle a signal.  Interrupts are off when
 *	we enter.  They must be off so we don't overwrite the signal stuff
 *	in the process state with more signal stuff.
 *
 * Results:
 *	We return via a rett to user mode to the pc of the signal handler.
 *
 * Side effects:
 *	Depends on signal handler, etc.
 *
 *---------------------------------------------------------------------
 */
.globl	_MachHandleSignal
_MachHandleSignal:
	/*
	 * Save window to stack so that user trap values will be saved
	 * to kernel stack so that when we copy them to the user stack, we'll
	 * get the correct values.
	 */
	MACH_SAVE_WINDOW_TO_STACK()

	/* Get new user stack pointer value into %SAFE_TEMP */
	set	_machSignalStackSizeOnStack, %VOL_TEMP1
	ld	[%VOL_TEMP1], %VOL_TEMP1	/* size MachSignalStack */
	sub	%fp, %VOL_TEMP1, %SAFE_TEMP		/* new sp in safetemp */

	/* Copy out sig stack from state structure to user stack */
	set	_machSigStackOffsetOnStack, %VOL_TEMP1
	ld	[%VOL_TEMP1], %VOL_TEMP1		/* offset Sig_Stack */
	add	%SAFE_TEMP, %VOL_TEMP1, %o2	/* dest addr of Sig_Stack */
	set	_machSigStackSize, %o0
	ld	[%o0], %o0				/* size of Sig_Stack */
	MACH_GET_CUR_STATE_PTR(%VOL_TEMP1, %VOL_TEMP2)	/* into %VOL_TEMP1 */
	set	_machSigStackOffsetInMach, %VOL_TEMP2
	ld	[%VOL_TEMP2], %VOL_TEMP2
	add	%VOL_TEMP1, %VOL_TEMP2, %o1	/* src addr of sig stack */
	QUICK_ENABLE_INTR(%VOL_TEMP1)
	call	_Vm_CopyOut, 3				/* copy Sig_Stack */
	nop
	QUICK_DISABLE_INTR(%VOL_TEMP1)
	be	CopiedOutSigStack
	nop
CopyOutForSigFailed:
	/*
	 * Copying to user stack failed.  We have no choice but to kill the
	 * thing.  This causes us to exit this routine and process.
	 */
	set	_MachHandleSignalDeathString, %o0
	call	_printf, 1
	nop
	set	PROC_TERM_DESTROYED, %o0
	set	PROC_BAD_STACK, %o1
	clr	%o2
	call	_Proc_ExitInt, 3
	nop

CopiedOutSigStack:
	/* Copy out sig context from state structure to user stack */
	/* put addr of mach state structure in %VOL_TEMP1 again */
	MACH_GET_CUR_STATE_PTR(%VOL_TEMP1, %VOL_TEMP2)	/* into %VOL_TEMP1 */
	set	_machSigContextOffsetOnStack, %VOL_TEMP2
	ld	[%VOL_TEMP2], %VOL_TEMP2		/* offset Sig_Context */
	add	%SAFE_TEMP, %VOL_TEMP2, %o2	/* dest addr of Sig_Context */
	set	_machSigContextSize, %o0
	ld	[%o0], %o0			/* size of Sig_Context */
	set	_machSigContextOffsetInMach, %VOL_TEMP2
	ld	[%VOL_TEMP2], %VOL_TEMP2		/* offset Sig_Context */
	add	%VOL_TEMP1, %VOL_TEMP2, %o1	/* src addr of sig context */
	QUICK_ENABLE_INTR(%VOL_TEMP1)
	call	_Vm_CopyOut, 3				/* copy Sig_Context */
	nop
	QUICK_DISABLE_INTR(%VOL_TEMP1)
	bne	CopyOutForSigFailed
	nop

	/* Copy out user trap state from state structure to user stack */
	/* put addr of mach state structure in %VOL_TEMP1 again */
	MACH_GET_CUR_STATE_PTR(%VOL_TEMP1, %VOL_TEMP2)	/* into %VOL_TEMP1 */
	set	_machSigUserStateOffsetOnStack, %VOL_TEMP2
	ld	[%VOL_TEMP2], %VOL_TEMP2
	add	%SAFE_TEMP, %VOL_TEMP2, %o2	/* dest addr of user state */
	set	MACH_SAVED_STATE_FRAME, %o0	/* size of copy */
	add	%VOL_TEMP1, MACH_TRAP_REGS_OFFSET, %VOL_TEMP1
	ld	[%VOL_TEMP1], %o1		/* address of trap regs */
	QUICK_ENABLE_INTR(%VOL_TEMP1)
	call	_Vm_CopyOut, 3
	nop
	QUICK_DISABLE_INTR(%VOL_TEMP1)
	bne	CopyOutForSigFailed
	nop

	/*
	 * Get address of trapInst field in machContext field of sig context
	 * and put it in ret addr of next window so when we return from handler
	 * in next window, we'll return to the trap instruction.
	 */
	set	_machSigTrapInstOffsetOnStack, %VOL_TEMP2
	ld	[%VOL_TEMP2], %VOL_TEMP2	/* offset to trap instr */
	add	%SAFE_TEMP, %VOL_TEMP2, %RETURN_ADDR_REG	/* addr */
	/* ret from proc instr jumps to (ret addr + 8), so subtract 8 here */
	sub	%RETURN_ADDR_REG, 0x8, %RETURN_ADDR_REG

	/*
	 * Set return from trap pc and next pc in the next window to the
	 * address of the handler so that when we do a rett back to this
	 * window from the next window, we'll start executing at the signal
	 * handler.  This requires a global register to get
	 * it across the window boundary, so we must do this before restoring
	 * our global registers.  This should be done after the Vm_CopyOut's
	 * so that there's no overwriting our confusion with the registers
	 * in the next window.
	 */
	set	_machSigPCOffsetOnStack, %VOL_TEMP1
	ld	[%VOL_TEMP1], %VOL_TEMP1		/* offset to pc field */
	add	%SAFE_TEMP, %VOL_TEMP1, %VOL_TEMP1	/* addr of sig pc */
	ld	[%VOL_TEMP1], %g3
	
	save
	mov	%g3, %CUR_PC_REG		/* trap pc addr into rett pc */
	add	%g3, 0x4, %NEXT_PC_REG
	restore

	/*
	 * restore global regs for user after calling Vm_CopyOut and after
	 * using %g3 (above) and before messing up our kernel stack pointer.
	 */
	MACH_RESTORE_GLOBAL_STATE()

	/*
	 * Set up our out registers to be correct arguments to sig handler,
	 * since this is the window it will start off in, but the C code will
	 * do a save into the next window so our out's will be the handler's
	 * in regs.
	 *
	 *	Handler(sigNum, sigCode, contextPtr, sigAddr)
	 */
	MACH_GET_CUR_STATE_PTR(%VOL_TEMP1, %VOL_TEMP2)	/* into %VOL_TEMP1 */
	set	_machSigStackOffsetInMach, %VOL_TEMP2
	ld	[%VOL_TEMP2], %VOL_TEMP2	/* offset to sig stack */
	add	%VOL_TEMP1, %VOL_TEMP2, %VOL_TEMP2	/* addr of sig stack */
	set	_machSigNumOffsetInSig, %o0
	ld	[%o0], %o0			/* offset to sigNum */
	add	%o0, %VOL_TEMP2, %o0		/* addr of sig num */
	ld	[%o0], %o0			/* sig num == arg 1 */
	set	_machSigCodeOffsetInSig, %o1
	ld	[%o1], %o1			/* offset to sigCode */
	add	%o1, %VOL_TEMP2, %o1		/* addr of sig code */
	ld	[%o1], %o1			/* sig code == arg2 */
	/* Stack address of Sig_Context is third arg */
	set	_machSigContextOffsetOnStack, %VOL_TEMP2
	ld	[%VOL_TEMP2], %VOL_TEMP2		/* offset Sig_Context */
	add	%SAFE_TEMP, %VOL_TEMP2, %o2	/* stack addr context = arg3  */
	set	_machSigStackOffsetInMach, %VOL_TEMP2
	ld	[%VOL_TEMP2], %VOL_TEMP2	/* offset to sig stack */
	add	%VOL_TEMP1, %VOL_TEMP2, %VOL_TEMP2	/* addr of sig stack */
	set	_machSigAddrOffsetInSig, %o3
	ld	[%o3], %o3			/* offset to sigAddr */
	add	%o3, %VOL_TEMP2, %o3		/* addr of sig addr */
	ld	[%o3], %o3			/* sig addr == arg 4 */

	/*
	 * NOTE: the sigStack.sigContextPtr field has not been filled in.
	 * Will that matter?
	 */

	/* set stack pointer in this window to new user stack pointer */
	mov	%SAFE_TEMP, %sp

	/*
	 * Move the user psr to restore to next window, since we'll have
	 * to restore it from there since it disables traps and we must still
	 * have traps enabled when executing the save instruction, in case
	 * of window overflow.
	 */
	mov	%CUR_PSR_REG, %o4

	/*
	 * Move to window to do rett from.  We must do this while traps are
	 * still enabled, in case we get overflow.
	 */
	save

	/*
	 * Set up psr for return to user mode.  It must be the user mode
	 * psr with traps disabled.
	 */
	mov	%i4, %CUR_PSR_REG		/* so restore psr will work */
	MACH_RESTORE_PSR()

	jmp	%CUR_PC_REG
	rett	%NEXT_PC_REG

/*
 *---------------------------------------------------------------------
 *
 * MachReturnFromSignal -
 *
 *	void MachReturnFromSignal()
 *
 *	The routine executed after a return from signal trap.  This must
 *	restore state from the user stack so we can return to user mode
 *	via the regular return from trap method.
 *
 *	Interrupts must be off when we're called.
 *
 * Results:
 *	We execute the return-from-trap code, so it depends what happens there.
 *
 * Side effects:
 *	Depends.
 *
 *---------------------------------------------------------------------
 */
.globl	_MachReturnFromSignal
_MachReturnFromSignal:
	/*
	 * We've trapped into this window so our %fp is the user's sp that
	 * we set up before handling the signal.  We must copy stuff back off
	 * the user's stack to the trap regs to restore state.  But we're one
	 * window away from where we want to be, so we have to back up also.
	 */
	/* get our kernel stack pointer into global regs (which get restored
	 * in MachReturnFromTrap) so we can keep it across windows.
	 */
	mov	%sp, %g3
	restore		/* no underflow since we just came from here */
	/*
	 * I could at this point just move %sp to %SAFE_TEMP if I really
	 * trusted that %fp - sizeof (MachSignalStack) really equals %sp which
	 * was user sp we set up before calling signal handler, but I
	 * check this for now.
	 */
	set	_machSignalStackSizeOnStack, %VOL_TEMP1
	ld	[%VOL_TEMP1], %VOL_TEMP1	/* size MachSignalStack */
	sub	%fp, %VOL_TEMP1, %SAFE_TEMP	/* sig user sp in safetemp */
	cmp	%SAFE_TEMP, %sp
	bne	CopyInForSigFailed
	nop

	mov	%g3, %sp			/* kernel sp now here too */

	/* Copy in sig stack from user stack to mach state structure */
	set	_machSigStackOffsetOnStack, %VOL_TEMP1
	ld	[%VOL_TEMP1], %VOL_TEMP1	/* offset of Sig_Stack */
	add	%SAFE_TEMP, %VOL_TEMP1, %o1	/* src addr of Sig_Stack */
	set	_machSigStackSize, %o0
	ld	[%o0], %o0			/* size of Sig_Stack */
	MACH_GET_CUR_STATE_PTR(%VOL_TEMP1, %VOL_TEMP2)	/* into %VOL_TEMP1 */
	set	_machSigStackOffsetInMach, %VOL_TEMP2
	ld	[%VOL_TEMP2], %VOL_TEMP2
	add	%VOL_TEMP1, %VOL_TEMP2, %o2	/* dest addr of sig stack */
	QUICK_ENABLE_INTR(%VOL_TEMP1)
	call	_Vm_CopyIn, 3			/* copy Sig_Stack */
	nop
	QUICK_DISABLE_INTR(%VOL_TEMP1)
	be	CopiedInSigStack
	nop
CopyInForSigFailed:
 	/* Copy failed from user space - kill the process */ 
	set	_MachReturnFromSignalDeathString, %o0
	call	_printf, 1
	nop
	set	PROC_TERM_DESTROYED, %o0
	set	PROC_BAD_STACK, %o1
	clr	%o2
	call	_Proc_ExitInt, 3
	nop

CopiedInSigStack:
	/* Copy in sig context from user stack to state structure */
	/* put addr of mach state structure in %VOL_TEMP1 again */
	MACH_GET_CUR_STATE_PTR(%VOL_TEMP1, %VOL_TEMP2)	/* into %VOL_TEMP1 */
	set	_machSigContextOffsetOnStack, %VOL_TEMP2
	ld	[%VOL_TEMP2], %VOL_TEMP2		/* offset Sig_Context */
	add	%SAFE_TEMP, %VOL_TEMP2, %o1	/* src addr of Sig_Context */
	set	_machSigContextSize, %o0
	ld	[%o0], %o0			/* size of Sig_Context */
	set	_machSigContextOffsetInMach, %VOL_TEMP2
	ld	[%VOL_TEMP2], %VOL_TEMP2	/* offset of Sig_Context */
	add	%VOL_TEMP1, %VOL_TEMP2, %o2	/* dest addr of sig context */
	QUICK_ENABLE_INTR(%VOL_TEMP1)
	call	_Vm_CopyIn, 3			/* copy Sig_Context */
	nop
	QUICK_DISABLE_INTR(%VOL_TEMP1)
	bne	CopyInForSigFailed
	nop

	/*
	 * Call a routine that calls Sig_Return with appropriate stuff.
	 * This occurs after copying in the Sig stuff, above, because Sig_Return
	 * needs the Sig stuff.
	 */
	call	_MachCallSigReturn
	nop

	/* Copy in user trap state from user stack to kernel trap regs */
	set	_machSigUserStateOffsetOnStack, %VOL_TEMP2
	ld	[%VOL_TEMP2], %VOL_TEMP2
	add	%SAFE_TEMP, %VOL_TEMP2, %o1		/* src addr of copy */
	set	MACH_SAVED_STATE_FRAME, %o0		/* size of copy */
	/* destination of copy is trapRegs, but our sp points to that already */
	/* SHOULD I VERIFY THIS? */
	mov	%sp, %o2				/* dest addr of copy */
	QUICK_ENABLE_INTR(%VOL_TEMP1)
	call	_Vm_CopyIn, 3
	nop
	QUICK_DISABLE_INTR(%VOL_TEMP1)
	bne	CopyInForSigFailed
	nop

	/* Restore user trap state */
	mov	%sp, %g3			/* save stack pointer */
	MACH_RESTORE_WINDOW_FROM_STACK()
	/* test if stack pointer is the same */
	cmp	%g3, %sp
	mov	%g3, %sp		/* restore, in case it was bad */
	bne	CopyInForSigFailed
	nop

	/* Make sure the psr is okay since we got it from the user */
	set	~(MACH_PS_BIT), %VOL_TEMP1
	and	%CUR_PSR_REG, %VOL_TEMP1, %CUR_PSR_REG	/* clear ps bit */
	set	MACH_DISABLE_TRAP_BIT, %VOL_TEMP1
	and	%CUR_PSR_REG, %VOL_TEMP1, %CUR_PSR_REG	/* clear trap en. bit */

	/*
	 * Now go through the regular return from trap code.
	 */
	call	_MachReturnFromTrap
	nop


#ifdef NOTDEF
/*
 *---------------------------------------------------------------------
 *
 * MachFlushWindowsToStack -
 *
 *	void MachFlushWindowsToStack()
 *
 *	Flush all the register windows to the stack.
 * 	Interrupts must be off when we're called.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------
 */
.globl	_MachFlushWindowsToStack
_MachFlushWindowsToStack:
	mov	%g3, %o0
	set	(MACH_NUM_WINDOWS - 1), %g3
SaveTheWindow:
	save
	subcc	%g3, 1, %g3
	bne	SaveTheWindow
	nop
	set	(MACH_NUM_WINDOWS - 1), %g3
RestoreTheWindow:
	restore
	subcc	%g3, 1, %g3
	bne	RestoreTheWindow
	nop

	mov	%o0, %g3

	retl
	nop
#endif NOTDEF


/*
 *---------------------------------------------------------------------
 *
 * Mach_ReadPsr -
 *
 *	Capture %psr for C routines.
 *
 * Results:
 *	%psr.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------
 */
.globl	_Mach_ReadPsr
_Mach_ReadPsr:
	mov	%psr, %o0
	retl
	nop

#ifdef sun4c
.globl	_MachGetSyncErrorReg
_MachGetSyncErrorReg:
	set	VMMACH_SYNC_ERROR_REG, %o0
	lda	[%o0] VMMACH_CONTROL_SPACE, %o0
	retl
	nop

.globl	_MachGetASyncErrorReg
_MachGetASyncErrorReg:
	set	VMMACH_ASYNC_ERROR_REG, %o0
	lda	[%o0] VMMACH_CONTROL_SPACE, %o0
	retl
	nop

.globl	_MachGetSyncErrorAddrReg
_MachGetSyncErrorAddrReg:
	set	VMMACH_SYNC_ERROR_ADDR_REG, %o0
	lda	[%o0] VMMACH_CONTROL_SPACE, %o0
	retl
	nop

.globl	_MachGetASyncErrorAddrReg
_MachGetASyncErrorAddrReg:
	set	VMMACH_ASYNC_ERROR_ADDR_REG, %o0
	lda	[%o0] VMMACH_CONTROL_SPACE, %o0
	retl
	nop

.globl	_MachGetSystemEnableReg
_MachGetSystemEnableReg:
	set	VMMACH_SYSTEM_ENABLE_REG, %o0
	lduba	[%o0] VMMACH_CONTROL_SPACE, %o0
	retl
	nop
#endif

/*
 *---------------------------------------------------------------------
 *
 * MachHandleBadArgs -
 *
 *	A page fault occured in a copy-in or copy-out from a user-mode process,
 *	and the page couldn't be paged in.  So we want to return
 *	SYS_ARG_NOACCESS to the process from its system call.
 *	This means we must return this value from the routine doing the copying.
 *
 * Results:
 *	SYS_ARG_NOACCESS returned to process as if returned from its system
 *	call.
 *
 * Side effects:
 *	We cause the routine that was copying args to return SYS_ARG_NO_ACCESS.
 *
 *---------------------------------------------------------------------
 */
.globl	_MachHandleBadArgs
_MachHandleBadArgs:
	restore			/* back to page fault trap window */
	mov	%CUR_PSR_REG, %i1
	restore			/* back to copy routine that got trap */
	/* Copy routine was leaf routine and so return failure in %o0 */
	set	SYS_ARG_NOACCESS, %o0
	/* Set interrupts to what they were when trap occured. */
	SET_INTRS_TO(%o1, %OUT_TEMP1, %OUT_TEMP2)
	retl			/* return from leaf routine */
	nop

/*
 * Beginning of area where the kernel should be able to handle a bus error
 * (which includes size errors) while in kernel mode.
 */
.globl	_Mach_ProbeStart
_Mach_ProbeStart:

/*
 * ----------------------------------------------------------------------
 *
 * Mach_Probe --
 *
 *	Read or write a virtual address handling bus errors that may occur.
 *	This routine is intended to be used to probe for memory-mapped devices.
 *
 * Results:
 *	SUCCESS if the read or write worked and FAILURE otherwise.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------
 */
.globl	_Mach_Probe
_Mach_Probe:
				/* %o0 = size in bytes of the read to do.
				 * Must be 1, 2, 4, or 8 bytes. */
				/* %o1 = address to read. */
				/* %o2 = Location to store the read value. */

				/* %o0 = address to peek in. */
				/* %o1 = size in bytes of the read to do.
				 * Must be 1, 2, 4, or 8 bytes. */
				/* %o2 = Location to store the correctly read
				 * value. It should be a pointer to an
				 * object of size size. */
	cmp	%o0, 1
	bne	Read2Bytes
	nop
	ldub	[%o1], %OUT_TEMP1
	stb	%OUT_TEMP1, [%o2]
	set	SUCCESS, %RETURN_VAL_REG
	retl
	nop
Read2Bytes:
	cmp	%o0, 2
	bne	Read4Bytes
	nop
	andcc	%o1, 0x1, %g0
	bne	BadRead
	nop
	andcc	%o2, 0x1, %g0
	bne	BadRead
	nop
	lduh	[%o1], %OUT_TEMP1
	sth	%OUT_TEMP1, [%o2]
	set	SUCCESS, %RETURN_VAL_REG
	retl
	nop
Read4Bytes:
	cmp	%o0, 4
	bne	Read8Bytes
	nop
	andcc	%o1, 0x3, %g0
	bne	BadRead
	nop
	andcc	%o2, 0x3, %g0
	bne	BadRead
	nop
	ld	[%o1], %OUT_TEMP1
	st	%OUT_TEMP1, [%o2]
	set	SUCCESS, %RETURN_VAL_REG
	retl
	nop
Read8Bytes:
	cmp	%o0, 8
	bne	BadRead
	nop
	andcc	%o1, 0x7, %g0
	bne	BadRead
	nop
	andcc	%o2, 0x7, %g0
	bne	BadRead
	nop
	ldd	[%o1], %OUT_TEMP1
	std	%OUT_TEMP1, [%o2]
	set	SUCCESS, %RETURN_VAL_REG
	retl
	nop
BadRead:
	set	FAILURE, %RETURN_VAL_REG
	retl
	nop

/*
 * End of the area where the kernel should be able to handle a bus error
 * while in kernel mode.
 */
.globl	_Mach_ProbeEnd
_Mach_ProbeEnd:

/*
 *---------------------------------------------------------------------
 *
 * MachHandleBadProbe -
 *
 *	A page fault occured during a probe operation, which means
 *	the page couldn't be paged in or there was a size error.  So we want to
 *	return FAILURE to the caller of the probe operation.  This means we must
 *	return this value from the routine doing the probe.
 *
 * Results:
 *	FAILURE returned to caller of probe operation.
 *
 * Side effects:
 *	We cause the routine that was doing the probe to return FAILURE.
 *
 *---------------------------------------------------------------------
 */
.globl	_MachHandleBadProbe
_MachHandleBadProbe:
	restore		/* back to page fault trap window */
	mov	%CUR_PSR_REG, %i1	/* get trap psr */
	restore		/* back to probe routine window */
	/* Mach_Probe is a leaf routine - return stuff in %o0 */
	set	FAILURE, %o0	/* return FAILURE */
	/* Set interrupts to what they were when trap occured. */
	SET_INTRS_TO(%o1, %OUT_TEMP1, %OUT_TEMP2)
	retl		/* return from leaf routine */
	nop

/*
 *---------------------------------------------------------------------
 *
 * MachHandleBadQuickCopy -
 *
 *	A page fault occured during a quick copy operation, which isn't
 *	allowed.  So we want to return FAILURE to the caller of the copy
 *	operation.  This means we must return this value from the routine
 *	doing the copy.
 *
 * Results:
 *	FAILURE returned to caller of copy operation.
 *
 * Side effects:
 *	We cause the routine that was doing the copy to return FAILURE.
 *
 *---------------------------------------------------------------------
 */
.globl	_MachHandleBadQuickCopy
_MachHandleBadQuickCopy:
	restore		/* back to page fault trap window */
	mov	%CUR_PSR_REG, %i1	/* get trap psr */
	restore		/* back to copy routine window */
	/* VmMachQuickNDirtyCopy is NOT a leaf routine - return stuff in %i0 */
	set	FAILURE, %i0	/* return FAILURE */
	/* Set interrupts to what they were when trap occured. */
	SET_INTRS_TO(%o1, %OUT_TEMP1, %OUT_TEMP2)
	ret		/* return from non-leaf routine */
	restore
/*
 *---------------------------------------------------------------------
 *
 * _MachFPULoadState -
 *
 *	Load the current state into the FPU.
 *
 *	void MachFPULoadState(regStatePtr)
 * 	     Mach_RegState *regStatePtr;
 * Results:
 *	None
 *
 * Side effects:
 *	FPU registers changed.
 *
 *---------------------------------------------------------------------
 */
.globl	_MachFPULoadState
_MachFPULoadState:
	/*
	 * Insure that the FPU is enabled.
	 */
	mov	%psr, %o1
	set	MACH_ENABLE_FPP, %o2
	or	%o2,%o1,%o2
	mov	%o2,%psr
	nop
	nop
	nop
	ld	[%o0 + MACH_FPU_FSR_OFFSET], %fsr
	ldd	[%o0 + MACH_FPU_REGS_OFFSET+4*0],%f0
	ldd	[%o0 + MACH_FPU_REGS_OFFSET+4*2],%f2 
	ldd	[%o0 + MACH_FPU_REGS_OFFSET+4*4],%f4 
	ldd	[%o0 + MACH_FPU_REGS_OFFSET+4*6],%f6 
	ldd	[%o0 + MACH_FPU_REGS_OFFSET+4*8],%f8 
	ldd	[%o0 + MACH_FPU_REGS_OFFSET+4*10],%f10
	ldd	[%o0 + MACH_FPU_REGS_OFFSET+4*12],%f12
	ldd	[%o0 + MACH_FPU_REGS_OFFSET+4*14],%f14 
	ldd	[%o0 + MACH_FPU_REGS_OFFSET+4*16],%f16 
	ldd	[%o0 + MACH_FPU_REGS_OFFSET+4*18],%f18 
	ldd	[%o0 + MACH_FPU_REGS_OFFSET+4*20],%f20 
	ldd	[%o0 + MACH_FPU_REGS_OFFSET+4*22],%f22
	ldd	[%o0 + MACH_FPU_REGS_OFFSET+4*24],%f24 
	ldd	[%o0 + MACH_FPU_REGS_OFFSET+4*26],%f26
	ldd	[%o0 + MACH_FPU_REGS_OFFSET+4*28],%f28
	ldd	[%o0 + MACH_FPU_REGS_OFFSET+4*30],%f30
	retl
	mov	%o1,%psr
/*
 *---------------------------------------------------------------------
 *
 * MachFPUDumpState -
 *
 *	Save the FPU state of a process into the Mach_RegState structure.
 *
 *	void MachFPUDumpState(regStatePtr)
 *		Mach_RegState *regStatePtr;
 *
 * Results:
 *	None
 *
 * Side effects:
 *	Pending exception state cleared from FPU.
 *
 *---------------------------------------------------------------------
 */
.globl	_MachFPUDumpState
_MachFPUDumpState:
	/*
	 * Insure that the FPU is enabled.
	 */
	mov	%psr, %o1
	set	MACH_ENABLE_FPP, %o2
	or	%o2,%o1,%o2
	mov	%o2,%psr
	/*
	 * Next three instructions should not be FPU instructions. 
	 */
	set     MACH_FSR_QUEUE_NOT_EMPTY, %o3
	add	%o0, MACH_FPU_QUEUE_OFFSET, %o4
	mov	0, %o5

	/*
	 * We have to do the same trick here as in Mach_ContextSwitch,
	 * flagging the instruction that waits for the FPU to finish
	 * and checking for exceptions.  We have a different label this
	 * time, of course.  The trap handler knows that if the trap
	 * occurred at this label then we were already dumping the state
	 * and it shouldn't call us recursively.
	 * Note that the fsr is read again in the loop, and the purpose
	 * of this extra read is to have a separate label for the first
	 * read -- traps on subsequent reads of fsr would be an error.
	 */
.global _machFPUDumpSyncInst
_machFPUDumpSyncInst: 
	st	%fsr, [%o0 + MACH_FPU_FSR_OFFSET]
        /*
	 * The following symbol is include to make the following code 
	 * be attributed to the routine MachFPUDumpState2 and not
	 * _machFPUDumpSyncInst.
	 */
.globl	MachFPUDumpState2
MachFPUDumpState2:
	/*
	 * While MACH_FSR_QUEUE_NOT_EMPTY set in the FSR, save
	 * the entry in the %fq register.
	 */
clearQueueLoop:
	st	%fsr, [%o0 + MACH_FPU_FSR_OFFSET]
	ld      [%o0 + MACH_FPU_FSR_OFFSET], %o2
	andcc	%o2,%o3,%g0
	be	queueEmpty
	nop
	std	%fq, [%o4+%o5]
	ba	clearQueueLoop
	add	%o5,8,%o5
queueEmpty:
	/*
	 * Compute numQueueEntries.
	 */
	srl	%o5,3,%o5
	st	%o5, [%o0 + MACH_FPU_QUEUE_COUNT]
	/*
	 * Save registers two at a time.
	 */
	std	%f0, [%o0 + MACH_FPU_REGS_OFFSET+4*0]
	std	%f2, [%o0 + MACH_FPU_REGS_OFFSET+4*2]
	std	%f4, [%o0 + MACH_FPU_REGS_OFFSET+4*4]
	std	%f6, [%o0 + MACH_FPU_REGS_OFFSET+4*6]
	std	%f8, [%o0 + MACH_FPU_REGS_OFFSET+4*8]
	std	%f10, [%o0 + MACH_FPU_REGS_OFFSET+4*10]
	std	%f12, [%o0 + MACH_FPU_REGS_OFFSET+4*12]
	std	%f14, [%o0 + MACH_FPU_REGS_OFFSET+4*14]
	std	%f16, [%o0 + MACH_FPU_REGS_OFFSET+4*16]
	std	%f18, [%o0 + MACH_FPU_REGS_OFFSET+4*18]
	std	%f20, [%o0 + MACH_FPU_REGS_OFFSET+4*20]
	std	%f22, [%o0 + MACH_FPU_REGS_OFFSET+4*22]
	std	%f24, [%o0 + MACH_FPU_REGS_OFFSET+4*24]
	std	%f26, [%o0 + MACH_FPU_REGS_OFFSET+4*26]
	std	%f28, [%o0 + MACH_FPU_REGS_OFFSET+4*28]
	std	%f30, [%o0 + MACH_FPU_REGS_OFFSET+4*30]
	/*
	 * Restore the FPU to previous state and return.
	 */
	retl
	mov	%o1,%psr


