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
#include "machineConst.h"
#include "dbg.h"
#include "proc.h"
#include "sunSR.h"

/*
 * The format that the kernel stack has to be in to start a process off.
 */

typedef struct {
    int		magicNumber;		/* Magic number used to determine if
					   the stack has been corrupted. */
    Address	userStackPtr;		/* The user's stack pointer. */
    short	statusReg;		/* The status register. */
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
 * Machine dependent variables.
 */
int	mach_SP;
int	mach_FP;
Address	mach_KernStart;
Address	mach_CodeStart;
Address	mach_StackBottom;
int	mach_KernStackSize;
Address	mach_KernEnd;
int	mach_DummySPOffset;
int	mach_DummyFPOffset;
int	mach_ExecStackOffset;
Address	mach_FirstUserAddr;
Address	mach_LastUserAddr;
Address	mach_MaxUserStackAddr;
int	mach_LastUserStackPage;


/*
 * ----------------------------------------------------------------------------
 *
 * Mach_Init --
 *
 *     Machine dependent boot time initialization.
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
Mach_Init()
{
    KernelStack	stack;

    mach_SP = MACH_STACK_PTR;
    mach_FP = MACH_FRAME_PTR;
    mach_KernStart = (Address)MACH_KERN_START;
    mach_KernEnd = (Address)MACH_KERN_END;
    mach_CodeStart = (Address)MACH_CODE_START;
    mach_StackBottom = (Address)MACH_STACK_BOTTOM;
    mach_KernStackSize = MACH_KERN_STACK_SIZE;
    mach_DummySPOffset = MACH_KERN_STACK_SIZE - sizeof(KernelStack);
    mach_DummyFPOffset = MACH_KERN_STACK_SIZE - sizeof(KernelStack) +
		(unsigned int)&stack.statusReg - (unsigned int)&stack;
    mach_ExecStackOffset = MACH_EXEC_STACK_OFFSET;
    mach_FirstUserAddr = (Address)MACH_FIRST_USER_ADDR;
    mach_LastUserAddr = (Address)MACH_LAST_USER_ADDR;
    mach_MaxUserStackAddr = (Address)MACH_MAX_USER_STACK_ADDR;
    mach_LastUserStackPage = (MACH_MAX_USER_STACK_ADDR - 1) / VMMACH_PAGE_SIZE;
    Exc_Init();
}


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
    stackPtr = (KernelStack *) (stackStart + mach_DummySPOffset);
    stackPtr->magicNumber = MAGIC;
    stackPtr->userStackPtr = mach_MaxUserStackAddr;
    stackPtr->statusReg = SUN_SR_HIGHPRIO;
    stackPtr->startFunc =  startFunc;
    stackPtr->progCounter = progCounter;
    stackPtr->exitProc = Proc_Exit;
    stackPtr->fill1 = 0;
    stackPtr->fill2 = 0;
}
