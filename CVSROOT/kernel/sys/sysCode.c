/* 
 * sysCode.c --
 *
 *	Miscellaneous routines for the system.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "dbg.h"
#include "sys.h"
#include "rpc.h"
#include "sync.h"
#include "sched.h"
#include "proc.h"
#include "vm.h"

/*
 *  Number of processors in the system.
 *
 */

#ifndef NUM_PROCESSORS
#define NUM_PROCESSORS 1
#endif NUM_PROCESSORS

/*
 * TRUE if cpu was in kernel mode before the interrupt, FALSE if was in 
 * user mode.
 */

Boolean	sys_KernelMode;

int sys_NumProcessors = NUM_PROCESSORS;

/*
 *  Flag used by routines to determine if they are running at
 *  interrupt level.
 */

Boolean sys_AtInterruptLevel = FALSE;

/*
 *  Count of number of ``calls'' to enable interrupts minus number of calls
 *  to disable interrupts.  Kept on a per-processor basis.
 */

int sys_NumDisableInterrupts[NUM_PROCESSORS];
int *sys_NumDisableIntrsPtr = sys_NumDisableInterrupts;


/*
 * ----------------------------------------------------------------------------
 *
 * Sys_Init --
 *
 *	Initializes system-dependent data structures.
 *
 *	The number of calls to disable interrupts is set to 1 for 
 *	each processor, since Sys_Init is assumed to be called with 
 *	interrupts off and to be followed with an explicit call to 
 *	enable interrupts.
 *
 *	Until ENABLE_INTR() is called without a prior DISABLE_INTR() (i.e.,
 *	when it is called outside the context of a MASTER_UNLOCK), interrupts
 *	will remain disabled.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	For each processor, the number of disable interrupt calls outstanding
 *	is initialized.  
 *
 * ----------------------------------------------------------------------------
 */

void 
Sys_Init()
{
    int processorNumber;
    extern void SysInitSysCall();

    for (processorNumber = 0; processorNumber < NUM_PROCESSORS;
	    processorNumber++) {
	sys_NumDisableInterrupts[processorNumber] = 1;
    }
    SysInitSysCall();
}

/*
 *----------------------------------------------------------------------
 *
 * Sys_GetHostId --
 *
 *	This returns the Sprite Host Id for the system.  This Id is
 *	guaranteed to be unique accross all Sprite Hosts participating
 *	in the system.  This is plucked from the RPC system now,
 *	but perhaps should be determined from the filesystem.
 *
 * Results:
 *	The Sprite Host Id.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
int
Sys_GetHostId()
{
    return(rpc_SpriteID);
}


/*
 *----------------------------------------------------------------------
 *
 * Sys_GetProcessorNumber --
 *
 *	CURRENTLY replaced by a macro in sys.h.
 *
 *	Return the processor number of the processor making the call.
 *	This will probably be an assembly routine once we have multiple
 *	processors with a register containing the processor number.
 *
 * Results:
 *	The processor number of the processor making the call is returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

#ifdef not_defined
int
Sys_GetProcessorNumber()
{
    return(0);
}
#endif



/*
 * ----------------------------------------------------------------------------
 *
 * Sys_ProcessorState --
 *
 *	Determines what state the processor is in.
 *
 * Results:
 *	SYS_USER	if was at user level
 *	SYS_KERNEL	if was at kernel level
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */


/*ARGSUSED*/
Sys_ProcessorStates 
Sys_ProcessorState(processor)
    int processor;	/* processor number for which info is requested */
{
    if (sys_KernelMode) {
	return(SYS_KERNEL);
    } else {
	return(SYS_USER);
    }
}


/*
 * ----------------------------------------------------------------------------
 *
 * Sys_InterruptProcessor --
 *
 *	Interrupts processor i with a command to execute.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

/*ARGSUSED*/
void
Sys_InterruptProcessor(processorNum, command)
    int	processorNum;
    Sys_InterruptCodes	command;
{
    return;
}


/*
 * ----------------------------------------------------------------------------
 *
 * Sys_UnsetJump --
 *
 *	Clear out the pointer to the saved state from a set jump.
 *
 * Results:
 *	None. of SysSetJump.
 *
 * Side effects:
 *	setJumpStatePtr field of proc table for current process is nil'd out.
 *
 * ----------------------------------------------------------------------------
 */

void
Sys_UnsetJump()
{
    Proc_ControlBlock	*procPtr;

    procPtr = Proc_GetCurrentProc(Sys_GetProcessorNumber());
    procPtr->setJumpStatePtr = (Sys_SetJumpState *) NIL;
}
