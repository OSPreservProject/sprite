/* 
 * schedQueue.c --
 *
 *	Routines for ordering processes in the run queue based on weighted
 *	usage.
 *
 * Copyright 1985 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"
#include "proc.h"
#include "list.h"
#include "sync.h"
#include "sched.h"
#include "schedInt.h"

List_Links schedReadyQueueHeader;
List_Links *schedReadyQueueHdrPtr = &schedReadyQueueHeader;


/*
 * ----------------------------------------------------------------------------
 *
 * Sched_SetClearUsageFlag --
 *
 *	Set flag in process table entry for the current process that says to
 *	clear usage info whenever is scheduled.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	schedFlags field modified for calling process.
 *
 * ----------------------------------------------------------------------------
 */
void
Sched_SetClearUsageFlag()
{
    Proc_ControlBlock	*procPtr;

    procPtr = Proc_GetCurrentProc(Sys_GetProcessorNumber());
    MASTER_LOCK(sched_Mutex);
    procPtr->schedFlags |= SCHED_CLEAR_USAGE;
    MASTER_UNLOCK(sched_Mutex);
}


/*
 * ----------------------------------------------------------------------------
 *
 * Sched_MoveInQueue --
 *
 *	Given a pointer to a process, move it in the run queue (or insert
 *	it if it is not already there) based on its current weighted usage.
 *	If the process is of higher priority than the current process,
 *	flag the current process as having a pending context switch.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Process is moved within or added to the run queue.  If the process
 *	is added, the global count of the number of ready processes is
 *	incremented.
 *
 * ----------------------------------------------------------------------------
 */

INTERNAL void
Sched_MoveInQueue(procPtr)
    register	Proc_ControlBlock *procPtr;	/* pointer to process to 
						   move/insert */
{
    register Proc_ControlBlock 	*curProcPtr;		/* Pointer to currently
							 * running process */
    register List_Links		*followingItemPtr;	/* Pointer to item that
							 * will follow one being
						         * moved/inserted */
    register Proc_ControlBlock 	*itemProcPtr;		/* Pointer to 
							 * corresponding process
						   	 * table entry */
    Boolean 			insert = TRUE;		/* TRUE if process is 
							 * to be inserted */
    Boolean 			foundInsertPoint = FALSE;/* TRUE once we know 
							  * where to insert */
    ReturnStatus 		status;

    if (!sched_Mutex) {
	Sys_Panic(SYS_FATAL, "Sched_MoveInQueue: master lock not held.\n");
    }

    /*
     * Compare the process's weighted usage to the usage of the currently
     * running process.  If the new process has higher priority (lower
     * usage), then force the current process to context switch.  There
     * are two cases:
     *	(1) We are being called at interrupt level, in which case if the
     *      current process was in user mode it is safe (and necessary)
     *	    to set sched_DoContextSwitch directly.  This is because
     *	    the interrupt handler doesn't deal with the schedFlags.
     *  (2) Otherwise, the best thing we can do is to make sure the
     *      current process switches the next time it comes into the
     *      trap handler.
     */

    if (procPtr->schedFlags & SCHED_CLEAR_USAGE) {
	procPtr->recentUsage = 0;
	procPtr->weightedUsage = 0;
	procPtr->unweightedUsage = 0;
    }

    curProcPtr = Proc_GetCurrentProc(Sys_GetProcessorNumber());
    if ((curProcPtr != (Proc_ControlBlock *) NIL) &&
	    (procPtr->weightedUsage < curProcPtr->weightedUsage)) {
	if (sys_AtInterruptLevel && !sys_KernelMode) {
	    sched_DoContextSwitch = TRUE;
	} 
	curProcPtr->schedFlags |= SCHED_CONTEXT_SWITCH_PENDING;
    }

    /*
     * Loop through all elements in the run queue.  Search for the first
     * process with a weighted usage greater than the process being inserted.
     *
     * While looping through the list, search for the process to see whether
     * it already is in the queue.  If so, set insert = FALSE to indicate that
     * the process should be removed from its current position before being
     * placed in its correct position.
     *
     * If a process is found that belongs after procPtr, set foundInsertPoint
     * and just look for procPtr to see whether the process is being moved
     * or inserted.  If procPtr has already been found and the insert point
     * has been determined, exit the loop.
     */

    LIST_FORALL(schedReadyQueueHdrPtr, (List_Links *) itemProcPtr) {
	if (itemProcPtr == procPtr) {
	    insert = FALSE;
	}
	if (foundInsertPoint && !insert) {
	    break;
	}
	if (foundInsertPoint) {
	    continue;
	}
	if (procPtr->weightedUsage < itemProcPtr->weightedUsage) {
	    /*
	     * If no change in position, return.
	     */
	    if (itemProcPtr == procPtr) {
		return;
	    }
	    followingItemPtr = (List_Links *) itemProcPtr;
	    foundInsertPoint = TRUE;
	}
    }
    if (!insert) {
	List_Remove((List_Links *) procPtr);
    } else {
	sched_Instrument.numReadyProcesses += 1;
    }
    if (foundInsertPoint) {
	List_Insert((List_Links *) procPtr,
			     LIST_BEFORE(followingItemPtr));
    } else {
	/*
	 * Process goes at end of queue.
	 */
	List_Insert((List_Links *) procPtr,
			     LIST_ATREAR(schedReadyQueueHdrPtr));
    }
}


/*
 * ----------------------------------------------------------------------------
 *
 * Sched_GetRunnableProcess --
 *
 *	Return a pointer to the process table entry for the READY process with
 *	the least weighted usage.
 *
 *	The queue is already sorted by weighted usage, so take the first
 *	ready process in the queue.
 *
 * Results:
 *	Pointer to runnable process.
 *
 * Side effects:
 *	Process is removed from the ready queue, and the global count of the
 *	number of ready processes is decremented.
 *
 * ----------------------------------------------------------------------------
 */

INTERNAL Proc_ControlBlock *
Sched_GetRunnableProcess()
{
    register Proc_ControlBlock	*procPtr; 	/* Pointer to corresponding 
						 * process table entry */
    register List_Links		*queuePtr;	/* Pointer to sched ready 
						 * queue. */

    queuePtr = schedReadyQueueHdrPtr;
    if (List_IsEmpty(queuePtr)) {
	return((Proc_ControlBlock *) NIL);
    }
    procPtr = (Proc_ControlBlock *) List_First(queuePtr);
    if (procPtr->state != PROC_READY) {
	Sys_Panic(SYS_FATAL, "Non-ready process found in ready queue.\n");
    }
    List_Remove((List_Links *) procPtr);
    sched_Instrument.numReadyProcesses -= 1;
    return(procPtr);
}
