/* 
 * machCode.c --
 *
 *     C code for the mach module.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "machConst.h"
#include "machMon.h"
#include "mach.h"

/*
 *  Number of processors in the system.
 */
#ifndef NUM_PROCESSORS
#define NUM_PROCESSORS 1
#endif NUM_PROCESSORS

int mach_NumProcessors = NUM_PROCESSORS;

/*
 * TRUE if cpu was in kernel mode before the interrupt, FALSE if was in 
 * user mode.
 */
Boolean	mach_KernelMode;

/*
 *  Flag used by routines to determine if they are running at
 *  interrupt level.
 */
Boolean mach_AtInterruptLevel = FALSE;

/*
 * The machine type string is imported by the file system and
 * used when expanding $MACHINE in file names.
 */

char *mach_MachineType = "sun4";

/*
 *  Count of number of ``calls'' to enable interrupts minus number of calls
 *  to disable interrupts.  Kept on a per-processor basis.
 */
int mach_NumDisableInterrupts[NUM_PROCESSORS];
int *mach_NumDisableIntrsPtr = mach_NumDisableInterrupts;

/*
 * Machine dependent variables.
 */
Address	mach_KernStart;
Address	mach_CodeStart;
Address	mach_StackBottom;
int	mach_KernStackSize;
Address	mach_KernEnd;
Address	mach_FirstUserAddr;
Address	mach_LastUserAddr;
Address	mach_MaxUserStackAddr;
int	mach_LastUserStackPage;

/*
 * Temporarily, for implementing and testing interrupts, I'm saving trap
 * state into the following buffer.
 */
Mach_RegState	stateHolder;
Mach_RegState	*temporaryTrapState = &stateHolder;
/*
 * Temporarily, for counting clock interrupts, I'm saving a counter in
 * this integer.
 */
int	counterHolder;
int	*temporaryClockCounter = &counterHolder;


/*
 * ----------------------------------------------------------------------------
 *
 * Mach_Init --
 *
 *	Initialize some stuff.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */
void
Mach_Init()
{
    /*
     * Set exported machine dependent variables.
     */
    mach_KernStart = (Address)MACH_KERN_START;
    mach_KernEnd = (Address)MACH_KERN_END;
    mach_CodeStart = (Address)MACH_CODE_START;
    mach_StackBottom = (Address)MACH_STACK_BOTTOM;
    mach_KernStackSize = MACH_KERN_STACK_SIZE;
    mach_FirstUserAddr = (Address)MACH_FIRST_USER_ADDR;
    mach_LastUserAddr = (Address)MACH_LAST_USER_ADDR;
    mach_MaxUserStackAddr = (Address)MACH_MAX_USER_STACK_ADDR;
    mach_LastUserStackPage = (MACH_MAX_USER_STACK_ADDR - 1) / VMMACH_PAGE_SIZE;

#define	CHECK_OFFSETS1(s, o)		\
    if ((int)(&(stateHolder.s)) - (int)(&stateHolder) != o) {\
	panic("Bad offset for trap registers.  Redo machConst.h!\n");\
    }
#define	CHECK_OFFSETS2(s, o)		\
    if ((int)(&(stateHolder.s)) - (int)(&stateHolder) != o) {\
	panic("Bad offset for psr register.  Redo machConst.h!\n");\
    }
#define	CHECK_OFFSETS3(s, o)		\
    if ((int)(&(stateHolder.s)) - (int)(&stateHolder) != o) {\
	panic("Bad offset for y register.  Redo machConst.h!\n");\
    }
#define	CHECK_OFFSETS4(s, o)		\
    if ((int)(&(stateHolder.s)) - (int)(&stateHolder) != o) {\
	panic("Bad offset for tbr register.  Redo machConst.h!\n");\
    }
#define	CHECK_OFFSETS5(s, o)		\
    if ((int)(&(stateHolder.s)) - (int)(&stateHolder) != o) {\
	panic("Bad offset for wim register.  Redo machConst.h!\n");\
    }

    CHECK_OFFSETS1(regs, MACH_TRAP_REGS_OFFSET);
    CHECK_OFFSETS2(psr, MACH_PSR_OFFSET);
    CHECK_OFFSETS3(y, MACH_Y_OFFSET);
    CHECK_OFFSETS4(tbr, MACH_TBR_OFFSET);
    CHECK_OFFSETS5(wim, MACH_WIM_OFFSET);
#ifdef NOTDEF
    CHECK_OFFSETS(curPC, MACH_PC_OFFSET);
    CHECK_OFFSETS(nextPC, MACH_NEXT_PC_OFFSET);
#endif /* NOTDEF */
#undef CHECK_OFFSETS

    return;
}

int
panic(s)
char	*s;
{
    Mach_MonPrintf(s);
    for (; ;) {
	;
    }
}

void
MachHandleTestCounter()
{
#ifdef NOTDEF
    Mach_MonPrintf("timer reached 1000\n");
#endif NOTDEF
    return;
}
