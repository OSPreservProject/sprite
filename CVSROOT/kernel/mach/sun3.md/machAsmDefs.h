/*
 * asmDefs.h --
 *
 *	Macros used when writing assembler programs.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _ASMDEFS
#define _ASMDEFS

#include "vmSunConst.h"
#include "sunSR.h"

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
 * SaveRegs --
 *
 *      Save all general purpose registers and the user stack pointer on the
 *	stack.
 *
 * ----------------------------------------------------------------------------
 */

#define SaveRegs() \
	moveml 	#0xFFFF, sp@- ; \
	movc	usp, d0; \
	movl	d0, sp@- ;

/*
 * ----------------------------------------------------------------------------
 *
 * RestoreRegs --
 *
 *      Restore the user stack pointer and the general purpose registers from
 *	of the stack.  To be used in conjuction with SaveRegs.
 *
 * ----------------------------------------------------------------------------
 */

#define RestoreRegs() \
	movl	sp@+, d0 ; \
	movc	d0, usp ; \
	moveml 	sp@+, #0xFFFF ;

/*
 * ----------------------------------------------------------------------------
 *
 * RestoreTrapRegs --
 *
 *      Restore the user stack pointer and the general purpose registers from
 *	the stack after a call to the C trap routine.  6 is added to the stack
 *      pointer first to get past the trap code and bus error register which
 *	were pushed onto the stack by CallTrapHandler.
 *
 * ----------------------------------------------------------------------------
 */

#define RestoreTrapRegs() \
	addl	#6, sp ; \
	RestoreRegs();

/*
 * ----------------------------------------------------------------------------
 *
 * Call Interrupt Handler --
 *
 *      Call an interrupt handler.  The registers are saved first, 
 *	interrupts are disabled and a "At Interrupt Level" flag is set so 
 *	the handler can determine that it is running at interrupt level. 
 *	The registers are restored at the end.
 *
 *  Algorithm:
 *	Save registers
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

#define	INTR_SR_OFFSET	68

#define CallInterruptHandler(routine) \
	SaveRegs(); \
	\
	movw	#SUN_SR_HIGHPRIO, sr ; \
	movw	sp@(INTR_SR_OFFSET), d0; \
	andl	#SUN_SR_SUPSTATE, d0; \
	movl	d0, _sys_KernelMode; \
	movl	#1, _sys_AtInterruptLevel ; \
	\
	jsr	routine; \
	\
	clrl	_sys_AtInterruptLevel ; \
	tstl	_sched_DoContextSwitch; \
	beq 	1$; \
	\
	clrl	_sched_DoContextSwitch; \
	movw	sp@(INTR_SR_OFFSET), d0; \
	orw	#SUN_SR_TRACEMODE, d0; \
	movw	d0, sp@(INTR_SR_OFFSET); \
	\
1$:	RestoreRegs(); \
    	rte ;

/*
 *----------------------------------------------------------------------
 *
 * Call Trap Handler --
 *
 * Go through the following steps:
 * 
 *   1) Save the normal registers (a0-a7,d0-d7 and usp) on the stack.
 *   2) Save the bus error register on the stack.
 *   3) Push the trap type on the stack.
 *   4) Call the trap handler.
 *   5) Return from the trap handler.
 *   6) Call routine to handle return from trap.
 *
 *----------------------------------------------------------------------
 */

#ifdef SUN3
#define BUS_ERROR_MOVS movsb
#else 
#define BUS_ERROR_MOVS movsw
#endif

#define CallTrapHandler(type) \
        SaveRegs(); \
        movc    sfc, a0; \
        BUS_ERROR_MOVS VMMACH_BUS_ERROR_REG,d0; \
        movw    d0, sp@-; \
	movl	#type, sp@-; \
        jsr 	_Exc_Trap; \
	jra	ExcReturnFromTrap;
 
#define SysCallHandler() \
        SaveRegs(); \
	clrw	sp@-; \
	movl	#EXC_SYSCALL_TRAP, sp@-; \
        jsr 	_Sys_SysCall; \
	RestoreTrapRegs(); \
        rte
 
#endif _ASMDEFS
