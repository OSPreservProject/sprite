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
 *
 *  Algorithm:
 *	Save temporary registers
 *	Determine if interrupt occured while in kernel mode or user mode
 *	Call routine
 *	If a context switch is wanted then
 *	    Set the old status register trace mode bit on.
 *	    Clear the "context switch is wanted" flag.
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
	tstl	_sched_DoContextSwitch; \
	beq 	1$; \
	\
	clrl	_sched_DoContextSwitch; \
	movw	sp@(INTR_SR_OFFSET), d0; \
	orw	#MACH_SR_TRACEMODE, d0; \
	movw	d0, sp@(INTR_SR_OFFSET); \
	\
1$:	moveml	sp@+, #0x0303; \
    	rte ;

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

#ifdef sun3
#define BUS_ERROR_MOVS movsb
#else 
#define BUS_ERROR_MOVS movsw
#endif

#define CallTrapHandler(type) \
	.globl	_proc_RunningProcesses, _machStatePtrOffset; \
	movl	d0, sp@-; \
	movw	sp@(4), d0; \
	andl	#MACH_SR_SUPSTATE, d0; \
	beq	101$; \
	movl	sp@+, d0; \
	moveml	#0xC0C0, sp@-; \
	BUS_ERROR_MOVS VMMACH_BUS_ERROR_REG, d0; \
        movl    d0, sp@-; \
	movl	#type, sp@-; \
        jsr 	_MachTrap; \
	jra	MachReturnFromKernTrap; \
	\
101$:   movl	sp@+, d0; \
	cmpl	#0xffffffff, _machCurStatePtr; \
	bne	100$; \
	\
	subl	#16, sp; \
	BUS_ERROR_MOVS VMMACH_BUS_ERROR_REG,d0; \
        movl	d0, sp@-; \
	movl	#type, sp@-; \
	jra	_Dbg_Trap; \
	\
100$:	movl	a0, sp@-; \
	movl	_machCurStatePtr, a0; \
	moveml	#0x7fff, a0@(MACH_TRAP_REGS_OFFSET); \
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

#endif _MACHASMDEFS
