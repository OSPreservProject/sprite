/* 
 * machDep.c --
 *
 *     Machine dependent routines for Sun-2 and Sun-3.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "sys.h"
#include "machine.h"
#include "dbg.h"
#include "proc.h"
#include "sunSR.h"

/*
 * The format that the kernel stack has to be in to start a process off.
 */

typedef struct {
    int		magicNumber;		/* Magic number used to determine if
					   the stack has been corrupted. */
    int		destFuncCode;		/* The value of the destination 
					   function code register. */
    int		srcFuncCode;		/* The value of the source 
					   function code register. */
    int		userStackPtr;		/* The user's stack pointer. */
    short	statusReg;		/* The status register. */
    int		framePtr;		/* The value of a6, the framePtr. */
    void	(*startFunc)();		/* Function to call when process
					   first starts executing. */
    Address	progCounter;		/* Value of program counter where
					   startFunc should jump to when
					   it is ready to actually start the
					   process. */
    void	(*exitProc)();		/* Function to call if the process
					   dies.  This is left on top of the
					   stack so that if the process 
					   returns it will jump to this
					   function. */
    int		fill1;			/* Filler for the debugger. */
    int		fill2;			/* Filler for the debugger. */
} KernelStack;


/*
 * ----------------------------------------------------------------------------
 *
 * Mach_InitStack --
 *
 *     Initialize a new process's stack.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     The stack is initialized.
 *
 * ----------------------------------------------------------------------------
 */

void
Mach_InitStack(stackStart, startFunc, progCounter)
    int		stackStart;
    void	(*startFunc)();
    Address	progCounter;
{
    register	KernelStack	*stackPtr;

    /*
     * initialize the stack so that it looks like it is the middle of
     * Sun_ContextSwitch.  The old frame pointer is set to the highest 
     * possible stack address to make kdbx happy.
     */
    stackPtr = (KernelStack *) (stackStart + MACH_DUMMY_SP_OFFSET);
    stackPtr->magicNumber = MAGIC;
    stackPtr->destFuncCode = VM_USER_DATA_SPACE;
    stackPtr->srcFuncCode = VM_USER_DATA_SPACE;
    stackPtr->userStackPtr = MACH_MAX_USER_STACK_ADDR;
    stackPtr->statusReg = SUN_SR_HIGHPRIO;
    stackPtr->framePtr = stackStart + MACH_NUM_STACK_PAGES * VM_PAGE_SIZE;
    stackPtr->startFunc =  startFunc;
    stackPtr->progCounter = progCounter;
    stackPtr->exitProc = Proc_Exit;
    stackPtr->fill1 = 0;
    stackPtr->fill2 = 0;
}
