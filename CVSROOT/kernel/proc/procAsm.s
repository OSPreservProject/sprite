|* procAsm.s --
|*
|*     Contains assembly language routines required by the proc module.
|*
|* Copyright (C) 1985 Regents of the University of California
|* All rights reserved.
|*
|* rcs = $Header$ SPRITE (Berkeley)
|*

#include "asmDefs.h"

|*----------------------------------------------------------------------------
|*
|* Proc_RunUserProc -
|*
|*	void	Proc_RunUserProc(regPtr, progCounter, exitFunc, stackPointer)
|*	    int		*regPtr;
|*	    int		progCounter;
|*	    int		(*exitFunc)();
|*	    int		stackPointer;
|*
|* Results:
|*     	Restore registers and set up the stack to allow start a user process
|*	running.  Then do an rte to actually start off the process. 
|*
|* Side effects:
|*	None.
|*
|*----------------------------------------------------------------------------

    .text
    .globl	_Proc_RunUserProc
_Proc_RunUserProc:
    movl	sp@(4), a0		| Move the address of the saved 
					|     registers into a register.
    movl	a0@(60), a1		| Restore the user stack pointer
    movl	a1, usp			|     (registers are stored d0-d7, 	
					|      a0-a6, a7 == usp).
    movl	sp@(8), d1		| Get the program counter into
					|     a register.
    movl	sp@(12), a1		| Get the address of the procedure to
					|     call upon exit into a register.
    movl	sp@(16), sp		| Reset the stack pointer.
    movl	a1, sp@-		| The address of the exiting procedure
					|     is put on top of the stack.
    clrw	sp@-			| Set up the vector offset register to
					|     indicate a short stack.
    movl	d1, sp@-		| Push the pc onto the stack.
    movl	#SUN_SR_USERPRIO, d0
    movw	d0, sp@-		| Push the proper status register value
					|     onto the stack.
    moveml	a0@, #0x7FFF		| Restore all registers except sp (a7).
    rte

|*----------------------------------------------------------------------------
|*
|* ProcRunMigProcSysCall -
|*
|*	void	ProcRunMigProcSysCall(trapStack)
|*	    Exc_TrapStack	trapStack
|*
|* Results:
|*	Restart a system call and then restore registers and set up the stack 
|*	to allow start a user process running.  Then do an rte to actually 
|*	start off the process. 
|*
|* Side effects:
|*	None.
|*
|*----------------------------------------------------------------------------

    .text
    .globl	_ProcRunMigProcSysCall
_ProcRunMigProcSysCall:
    addql	#4, sp			| Get rid of the callers PC.
    jsr		_Exc_Trap		| Restart the system call.
    RestoreMigTrapRegs()		| Restore the registers.
    rte					| Run it.

|*----------------------------------------------------------------------------
|*
|* ProcRunMigProc -
|*
|*	void	ProcRunMigProc(trapStack)
|*	    Exc_TrapStack	trapStack
|*
|* Results:
|*     	Restore registers and set up the stack to allow start a user process
|*	running.  Then do an rte to actually start off the process. 
|*
|* Side effects:
|*	None.
|*
|*----------------------------------------------------------------------------

    .text
    .globl	_ProcRunMigProc
_ProcRunMigProc:
    addql	#4, sp			| Get rid of the callers PC.
    RestoreMigTrapRegs()		| Restore the registers.
    rte					| Run it.
