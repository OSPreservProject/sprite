/*
 * machCPC.c --
 *
 *	This file contains the routines for cross processor proceedure calls
 *	in multiprocessor SPUR.  CPC provides a mechanism upon which upon
 *	one processor can execute a subroutine on another processor in the
 *	same SPUR workstation.  
 *
 * Copyright 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 */

#include "sprite.h"
#include "mach.h"
#include "machInt.h"
#include "sync.h"

/*
 * Declaration of the cross processor call communication block. One such
 * block exists for each processor.
 */

typedef struct  {
    Sync_Semaphore lock;	/* Lock protecting entry. */
    ClientData	(*routine)();	/* Routine to call. */
    ClientData	argument;	/* Argument to the routine. */
    ClientData	*returnValue;	/* Location to place return value. */
    ReturnStatus status;	/* Return status of call. */
    Boolean	sync;		/* TRUE if the call synchronous. */
    Boolean	done;		/* TRUE when a synchronous completes. */
} MachCpcEntry;

/*
 * Communication blocks, one for each processor.
 */
static MachCpcEntry machCPCData[MACH_MAX_NUM_PROCESSORS];

/*
 * Interrupt number for cross processor signal. 
 */
unsigned int	mach_CpcInterruptNumber;


#define	TIMEOUT		10000000

/* 
 *----------------------------------------------------------------------
 *
 * Mach_CPC_Init --
 *
 *	Initialized the SPUR cross processor call facility.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 * 
 */
void
Mach_CPC_Init()
{
    int		i;
    /*
     * Ensure that locks and boolean variables of every communication block
     * are zero.
     */
    bzero(machCPCData, sizeof(machCPCData));
    for (i = 0; i  < MACH_MAX_NUM_PROCESSORS; i++) {
	Sync_SemInitDynamic(&(machCPCData[i].lock),"CPCMutex");
    }
    /*
     * Allocate an external interrupt for CPCs. 
     */
    mach_CpcInterruptNumber = MACH_EXT_INTERRUPT_ANY;
    Mach_AllocExtIntrNumber(machExecuteCall,&mach_CpcInterruptNumber);
    Mach_SetNonmaskableIntr(1 << mach_CpcInterruptNumber);
}


/* 
 *----------------------------------------------------------------------
 *
 * Mach_CallProcessor --
 *
 *	Execute a routine on a different SPUR processor.
 *
 * Results:
 *	SUCESS if call is executed, FAILURE otherwise.
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 * 
 */


ReturnStatus
Mach_CallProcessor(processorNumber, routine, argument, wait, returnValue)
    int		processorNumber; /* Processor to perform call. */
    ClientData	(*routine)();	 /* Procedure to call. */
    ClientData	argument;	 /* Argument to procedure. */
    Boolean	wait;		 /* TRUE if procedure should wait for call to
				  * complete. */
    ClientData	*returnValue;	/* Location to place return value of 
				 * function. */
{
    ReturnStatus	status = SUCCESS;

    if (mach_ProcessorStatus[processorNumber] != MACH_ACTIVE_STATUS) {
	return (FAILURE);
    }

    /*
     * First grap execlusive access to the CPC communication area for the
     * processor we wish to talk with. 
     */
    MASTER_LOCK(&(machCPCData[processorNumber].lock));
    /* 
     * Once we grap the lock, fill in the entry and interrupt the
     * specified procesor. 
     */
    machCPCData[processorNumber].routine = routine;
    machCPCData[processorNumber].argument = argument;
    machCPCData[processorNumber].returnValue = returnValue;
    machCPCData[processorNumber].sync = wait;
    machCPCData[processorNumber].done = FALSE;
    Mach_SignalProcessor(processorNumber,mach_CpcInterruptNumber);
    /*
     * If the call is synchronous we spin waiting for it to complete. 
     * Completion is signaled when the calling processor sets the done
     * field to TRUE.
     * If the call is not synchronous, we just return and the remote 
     * processor releases the lock when done. 
     */
    if (wait) {
	register int	i;
	register Boolean * volatile done = &(machCPCData[processorNumber].done);
	for(i = 0; !(*done) && ( i < TIMEOUT); i++) {
		continue;
	}
	if (machCPCData[processorNumber].done) {
	    status = machCPCData[processorNumber].status;
	} else { 
	   Mach_MonPrintf("Warning: Processor %d appears hung.\n", processorNumber);
	   status = FAILURE;
	}
	MASTER_UNLOCK(&(machCPCData[processorNumber].lock));

    }
    return (status);
}

/* 
 *----------------------------------------------------------------------
 *
 * machExecuteCall --
 *
 *	Execute a routine called from a different SPUR processor. ExecuteCall
 *	is called from the interrupt handler upon the reception of a
 *	CPC interrupt.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 * 
 */

void
machExecuteCall(intrStatusPtr)
    unsigned int	intrStatusPtr;	/* Not used. */

{
    int		processorNumber; 
    ClientData	val;
    /*
     * Compute a store the current processor number.
     */
    processorNumber = Mach_GetProcessorNumber();
    if (machCPCData[processorNumber].lock.value == 0) {
	printf("Warning: Processor %d received bogus call interrupt\n",
			processorNumber);
	return;
    }

    /*
     * Call the specified routine with the specified argument.
     */
    val = (machCPCData[processorNumber].routine)
				(machCPCData[processorNumber].argument);

    /*
     * If it is a synchronous call, store the return value and signal the
     * end of the call by senting done to TRUE.
     * If it not a synchronous call then set the returnValue value to
     * true and release the lock for the caller.
     */
    if (machCPCData[processorNumber].sync) {
	if (machCPCData[processorNumber].returnValue != (ClientData *) NIL) {
	    (*machCPCData[processorNumber].returnValue) = val;
	}
	machCPCData[processorNumber].status = SUCCESS;
	machCPCData[processorNumber].done = TRUE;
    } else {
	if (machCPCData[processorNumber].returnValue != (ClientData *) NIL) {
	    (*machCPCData[processorNumber].returnValue) = (ClientData) TRUE;
	}
	MASTER_UNLOCK(&(machCPCData[processorNumber].lock));
    }
    return;
}


/* 
 *----------------------------------------------------------------------
 *
 * Mach_SignalProcessor --
 *
 * 	Send a interrupt to the specified remote processor.
 *
 * Results:
 *	SUCCESS if arguments are ok.
 *
 * Side effects:
 *	The remote processor is interrupted.
 *----------------------------------------------------------------------
 * 
 */

ReturnStatus
Mach_SignalProcessor(processorNumber, interruptNum)
    int			processorNumber; 
    unsigned int	interruptNum; /* External interrupt to generate. */ 
{
    unsigned int	interruptAddress;

    if (interruptNum > MACH_MAX_EXT_INTERRUPT) {
	return (FAILURE);
    }
    interruptAddress = 0xf0000000 | 
			(Mach_MapPnumToSlotId(processorNumber) << 24) | 
			(interruptNum << 2);
    Mach_WritePhysicalWord(interruptAddress, 0);
    return (SUCCESS);
}

