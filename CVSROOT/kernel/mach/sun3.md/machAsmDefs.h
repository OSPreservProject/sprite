/*
 * machAsmDefs.h --
 *
 *	Macros used when writing assembler programs.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _MACHASMDEFS
#define _MACHASMDEFS

#include "machConst.h"

/*
 * ----------------------------------------------------------------------------
 *
 * ENTRY --
 *
 *      Define an entry point for an assembler function.  This should be
 *      the first line in any assembler function.  It provides the same
 *      prolog that all C functions use.  name is the name of the
 *      function,  numRegsToSave is the number of general purpose
 *      registers (d0-d7, a0-a7) to save and regSaveMask is the mask which is
 *      provided to the moveml instruction to save registers on the
 *      stack.  See the 68000 manual for a description of how to formulate
 *      the mask.
 *
 * ----------------------------------------------------------------------------
 */

#define ENTRY(name, numRegsToSave, regSaveMask) \
	.text ; .globl _/**/name ; _/**/name: ; \
	NUMREGS  = numRegsToSave ;\
	SAVEMASK = regSaveMask ;\
	link a6,#-(NUMREGS*4); moveml #SAVEMASK, sp@;

/*
 * ----------------------------------------------------------------------------
 *
 * RETURN --
 *
 *      Return from an assembly language function.  To be used in conjunction
 *	with ENTRY.  This should be the last statement to appear in any 
 *	assembly language function.
 *
 * ----------------------------------------------------------------------------
 */

#define RETURN \
	moveml  a6@(-(NUMREGS*4)),#SAVEMASK ; unlk a6 ; rts ;

/*
 * ----------------------------------------------------------------------------
 *
 * Call Interrupt Handler --
 *
 *      Call an interrupt handler.  The temporary registers d0, d1, a0 and a1
 *	are saved first, then interrupts are disabled and an "At Interrupt 
 *	Level" flag is set so the handler can determine that it is running a
 *	interrupt level.  The registers are restored at the end.
 *	This code assumes that if the interrupt occured in user mode, and if
 *	the specialHandling flag is set on the way back to user mode, then
 *	a context switch is desired. Note that schedFlags is not checked.
 *
 *  Algorithm:
 *	Save temporary registers
 *	Determine if interrupt occured while in kernel mode or user mode
 *	Call routine
 *	if interrupt occured while in user mode.
 *	    if specialHandling is set for the current process 
 *    	        Set the old status register trace mode bit on.
 *	        Clear the specialHandling flag.
 *	    endif
 *	endif
 *	restore registers
 *		
 *
 * ----------------------------------------------------------------------------
 */

#define	INTR_SR_OFFSET	16

#define CallInterruptHandler(routine) \
	moveml	#0xC0C0, sp@-; \
	movw	#MACH_SR_HIGHPRIO, sr ; \
	movw	sp@(INTR_SR_OFFSET), d0; \
	andl	#MACH_SR_SUPSTATE, d0; \
	movl	d0, _mach_KernelMode; \
	movl	#1, _mach_AtInterruptLevel ; \
	\
	jsr	routine; \
	\
	clrl	_mach_AtInterruptLevel ; \
	tstl	_mach_KernelMode; \
	bne	1f; \
	\
	movl	_proc_RunningProcesses, a0; \
	movl	a0@, a1; \
	movl	_machSpecialHandlingOffset, d1;\
	tstl	a1@(0,d1:l); \
	beq	1f; \
	\
	clrl	a1@(0,d1:l); \
	movw	sp@(INTR_SR_OFFSET), d0; \
	orw	#MACH_SR_TRACEMODE, d0; \
	movw	d0, sp@(INTR_SR_OFFSET); \
	\
1:	moveml	sp@+, #0x0303; \
    	rte ;


#ifdef sun3
#define BUS_ERROR_MOVS movsb
#else 
#define BUS_ERROR_MOVS movsw
#endif

/*
 * ----------------------------------------------------------------------------
 *
 * RestoreUserFpuState --
 *
 *      Restore the floating point registers from the process state.
 *
 * ----------------------------------------------------------------------------
 */
#ifdef sun3
#define RestoreUserFpuState() \
        tstl        _mach68881Present; \
	beq         2f; \
	movl        _machCurStatePtr, a0; \
	tstb        a0@(MACH_TRAP_FP_STATE_OFFSET); \
	beq         1f; \
	fmovem      a0@(MACH_TRAP_FP_CTRL_REGS_OFFSET), fpc/fps/fpi; \
	fmovem      a0@(MACH_TRAP_FP_REGS_OFFSET), #0xff; \
1: \
	frestore    a0@(MACH_TRAP_FP_STATE_OFFSET); \
2:
#else
#define RestoreUserFpuState()
#endif

/*
 * ----------------------------------------------------------------------------
 *
 * RestoreUserRegs --
 *
 *      Restore the user stack pointer and the general purpose registers from
 *	the process state.
 *
 * ----------------------------------------------------------------------------
 */
#define RestoreUserRegs() \
	movl	_machCurStatePtr, a0; \
	movl	a0@(MACH_USER_SP_OFFSET), a1; \
	movc	a1, usp; \
	moveml	a0@(MACH_TRAP_REGS_OFFSET), #0xffff

/*
 * ----------------------------------------------------------------------------
 *
 * SaveUserFpuState --
 *
 *      Restore the floating point registers from the process state.
 *      The address of machCurStatePtr must already be in register a0.
 *
 * ----------------------------------------------------------------------------
 */
#ifdef sun3
#define SaveUserFpuState() \
        tstl        _mach68881Present; \
	beq         1f; \
	fsave       a0@(MACH_TRAP_FP_STATE_OFFSET); \
	tstb        a0@(MACH_TRAP_FP_STATE_OFFSET); \
	beq         1f; \
	fmovem      fpc/fps/fpi, a0@(MACH_TRAP_FP_CTRL_REGS_OFFSET); \
	fmovem      #0xff, a0@(MACH_TRAP_FP_REGS_OFFSET); \
	frestore    _mach68881NullState; \
1:
#else
#define SaveUserFpuState()
#endif

/*
 * ----------------------------------------------------------------------------
 *
 * SaveUserRegs --
 *
 *      Restore the user stack pointer and the general purpose registers from
 *	the process state.
 *
 * ----------------------------------------------------------------------------
 */
#define SaveUserRegs() \
	movl	_machCurStatePtr, a0; \
	movc    usp, a1; \
	movl	a1, a0@(MACH_USER_SP_OFFSET); \
	moveml	#0xffff, a0@(MACH_TRAP_REGS_OFFSET);


/*
 *----------------------------------------------------------------------
 *
 * Call Trap Handler --
 *
 * Go through the following steps:
 * 
 *   1) Determine if are in kernel or user mode.  If kernel mode then just
 *	put d0, d1, a0, a1, bus error reg and trap type onto stack and
 *	call trap handler.
 *
 *   Otherwise:
 *
 *   1) Grab a pointer to the current processes state structure.
 *	If the state structure does not exist then call the debugger directly.
 *      Since it requires a temporary register to get a pointer to the
 *	state structure a0 is saved on the stack.
 *   2) Save the normal registers (a0-a7,d0-d6) into the state struct.
 *   3) Copy the saved value of a0 from the stack into the state struct.
 *   4) Copy the true value of the stack pointer into the state struct.
 *   5) Make room on the stack for the registers that would have been saved
 *	(a0, a1, d0, and d1) if we had been in kernel mode.
 *   6) Save the bus error register on the stack.
 *   7) Push the trap type on the stack.
 *   8) Call the trap handler.
 *
 *----------------------------------------------------------------------
 */

#define CallTrapHandler(type) \
	.globl	_proc_RunningProcesses, _machStatePtrOffset; \
	movl	d0, sp@-; \
	movw	sp@(4), d0; \
	andl	#MACH_SR_SUPSTATE, d0; \
	beq	9f; \
	movl	sp@+, d0; \
	moveml	#0xC0C0, sp@-; \
	BUS_ERROR_MOVS VMMACH_BUS_ERROR_REG, d0; \
        movl    d0, sp@-; \
	movl	#type, sp@-; \
        jsr 	_MachTrap; \
	jra	MachReturnFromKernTrap; \
	\
9:      movl	sp@+, d0; \
	cmpl	#0xffffffff, _machCurStatePtr; \
	bne	8f; \
	\
	subl	#16, sp; \
	BUS_ERROR_MOVS VMMACH_BUS_ERROR_REG,d0; \
        movl	d0, sp@-; \
	movl	#type, sp@-; \
	jra	_Dbg_Trap; \
	\
8:	movl	a0, sp@-; \
	movl	_machCurStatePtr, a0; \
	moveml	#0x7fff, a0@(MACH_TRAP_REGS_OFFSET); \
	SaveUserFpuState(); \
	movl	sp@+, a0@(MACH_TRAP_REGS_OFFSET + 32); \
	movl	sp, a0@(MACH_TRAP_REGS_OFFSET + 60); \
	movc	usp, a1; \
	movl	a1, a0@(MACH_USER_SP_OFFSET); \
	movl	sp, a0@(MACH_EXC_STACK_PTR_OFFSET); \
	subl	#16, sp; \
	BUS_ERROR_MOVS VMMACH_BUS_ERROR_REG,d0; \
        movl    d0, sp@-; \
	movl	#type, sp@-; \
        jsr 	_MachTrap; \
	jra	MachReturnFromUserTrap;

#endif /* _MACHASMDEFS */
