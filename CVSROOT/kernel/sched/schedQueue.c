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
#endif /* not lint */


#include "sprite.h"
#include "mach.h"
#include "proc.h"
#include "list.h"
#include "sync.h"
#include "sched.h"
#include "schedInt.h"

List_Links schedReadyQueueHeader;
List_Links *schedReadyQueueHdrPtr = &schedReadyQueueHeader;


Sched_OnDeck	sched_OnDeck[MACH_MAX_NUM_PROCESSORS];

int	sched_Insert;
int	sched_QueueEmpty;
int	sched_Stage;
int	sched_Preempt;


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

    procPtr = Proc_GetCurrentProc();
    MASTER_LOCK(sched_MutexPtr);
    procPtr->schedFlags |= SCHED_CLEAR_USAGE;
    MASTER_UNLOCK(sched_MutexPtr);
}

/*
 * ----------------------------------------------------------------------------
 *
 * Sched_InsertInQueue --
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
Sched_InsertInQueue(procPtr, runPtrPtr)
    register Proc_ControlBlock	*procPtr;	/* pointer to process to 
						   move/insert */
    Proc_ControlBlock		**runPtrPtr;	/* returns process from 
						 * front of queue.
						 */
{
    register Proc_ControlBlock 	*itemProcPtr;
    register List_Links		*queuePtr;
    Boolean			insert;
    Boolean			delete;
    Boolean			foundInsertPoint;
    List_Links			*followingItemPtr;
    int				i;
    Proc_ControlBlock		*lowestProcPtr;

    sched_Insert++;
    if (procPtr->schedFlags & SCHED_CLEAR_USAGE) {
	procPtr->recentUsage = 0;
	procPtr->weightedUsage = 0;
	procPtr->unweightedUsage = 0;
    }

    queuePtr = schedReadyQueueHdrPtr;
    /*
     * Special case an empty queue. If the queue is empty there may be
     * idle processors. We optimize things by bypassing the queue and
     * giving processes to the processors through the staging areas.
     */
    if (mach_NumProcessors > 1 && List_IsEmpty(queuePtr)) {
	int		processor;
	int		cpu;

	sched_QueueEmpty++;
	processor = procPtr->processor;
	cpu = Mach_GetProcessorNumber();
	/*
	 * If we are supposed to return a runnable process and the process
	 * we were given last ran on the current processor, then just return
	 * the process.
	if ((runPtrPtr != (Proc_ControlBlock **) NIL) && 
	    (processor == Mach_GetProcessorNumber()) {
	    *runPtrPtr = procPtr;
	    return;
	}
	/*
	 * If the last processor to run the process is idle then just give
	 * the process to that processor.
	 */
	if ((proc_RunningProcesses[processor] == (Proc_ControlBlock *) NIL) &&
	    (sched_ProcessorStatus[processor] != SCHED_PROCESSOR_IDLE)) {
	    if (sched_OnDeck[processor].procPtr == 
		(Proc_ControlBlock *) NIL) {
		sched_OnDeck[processor].procPtr = procPtr;
		procPtr =  (Proc_ControlBlock *) NIL;
	    /*
	     * There is already a process on deck, but the one new one
	     * can't be run by anyone else because it's stack is in
	     * use by this processor. Switch the two processes.
	     */
	    } else if (procPtr->schedFlags & SCHED_STACK_IN_USE) {
		Proc_ControlBlock *tempPtr;

		tempPtr = procPtr;
		procPtr = sched_OnDeck[processor].procPtr;
		sched_OnDeck[processor].procPtr = tempPtr;
	    }
	}
	if (procPtr != (Proc_ControlBlock *) NIL && 
	    !(procPtr->schedFlags & SCHED_STACK_IN_USE)) {
	    /*
	     * Give the process to any idle processor. It's stack cannot
	     * be in use (it can't be run anyway so don't bother trying).
	     */
	    for (i = 0; i < mach_NumProcessors; i++) {
		if ((sched_ProcessorStatus[i] != SCHED_PROCESSOR_IDLE) &&
		    (proc_RunningProcesses[i] ==  (Proc_ControlBlock *) NIL) &&
		    (sched_OnDeck[i].procPtr == 
		    (Proc_ControlBlock *) NIL)) {
		    sched_OnDeck[i].procPtr = procPtr;
		    procPtr =  (Proc_ControlBlock *) NIL;
		    break;
		}
	    }
	}
	/* 
	 * If we gave the process away then we're done.
	 */
	if (procPtr == (Proc_ControlBlock *) NIL) {
	    sched_Stage++;
	    if (runPtrPtr != (Proc_ControlBlock **) NIL) {
		*runPtrPtr = (Proc_ControlBlock *) NIL;
	    }
	    return;
	}
    }
    /*
     * Compare the process's weighted usage to the usage of the currently
     * running processes.  If the new process has higher priority (lower
     * usage) than one of the current processes, then force the current 
     * process to context switch.  
     * The context switch will occur in one of two places. 
     * 1) If we are being called at interrupt level, and the interrupt occurred
     *    in user mode, then the context switch will occur prior to returning 
     *	  to user mode. (the CallInterruptHandler macro checks the 
     *    specialHandling flag)
     * 2) The next time the current process enters a trap handler.
     *
     */
    lowestProcPtr = (Proc_ControlBlock *) NIL;
    for (i = 0; i < mach_NumProcessors; i++) {
	itemProcPtr = proc_RunningProcesses[i];
	if (itemProcPtr == (Proc_ControlBlock *) NIL) {
	    continue;
	}
	if ((lowestProcPtr == (Proc_ControlBlock *) NIL) ||
	    (itemProcPtr->weightedUsage > lowestProcPtr->weightedUsage)) {
	    lowestProcPtr = itemProcPtr;
	}
    }
    if ((lowestProcPtr != (Proc_ControlBlock *) NIL) &&
	    (procPtr->weightedUsage < lowestProcPtr->weightedUsage)) {
	sched_Preempt++;
	lowestProcPtr->schedFlags |= SCHED_CONTEXT_SWITCH_PENDING;
	lowestProcPtr->specialHandling = 1;
    }
    if (List_IsEmpty(queuePtr)) {
	/*
	 * This is easy if the queue is empty.
	 */
	if (runPtrPtr != (Proc_ControlBlock **) NIL) {
	    *runPtrPtr = procPtr;
	    return;
	} else {
	    List_Insert((List_Links *) procPtr, LIST_ATREAR(queuePtr));
	    sched_Instrument.numReadyProcesses = 1;
	    return;
	}
    }
    /*
     * Loop through all elements in the run queue.  Search for the first
     * process with a weighted usage greater than the process being inserted.
     *
     * If a process is found that belongs after procPtr, set foundInsertPoint
     * and just look for procPtr to see whether the process is being moved
     * or inserted.  If procPtr has already been found and the insert point
     * has been determined, exit the loop.
     */
    insert = TRUE;
    delete = FALSE;
    foundInsertPoint = FALSE;
    itemProcPtr = (Proc_ControlBlock *) schedReadyQueueHdrPtr;
    LIST_FORALL(queuePtr, (List_Links *) itemProcPtr) {
	if (itemProcPtr == procPtr) {
	    delete = TRUE;
	}
	if (foundInsertPoint && delete) {
	    break;
	}
	if (foundInsertPoint) {
	    continue;
	}
	if (procPtr->weightedUsage < itemProcPtr->weightedUsage) {
	    followingItemPtr = (List_Links *) itemProcPtr;
	    if (((List_Links *)(procPtr))->nextPtr == 
		(List_Links *) followingItemPtr && delete) {
		insert = FALSE;
		delete = FALSE;
		break;
	    }
	    foundInsertPoint = TRUE;
	}
    }
    if (foundInsertPoint) {
	itemProcPtr = (Proc_ControlBlock *)LIST_BEFORE(followingItemPtr);
    }
    /*
     * Process was already on queue in a different position so delete it.
     */
    if (delete) {
	List_Remove((List_Links *) procPtr);
	sched_Instrument.numReadyProcesses -= 1;
    }
    if (runPtrPtr != (Proc_ControlBlock **) NIL) {
	if (foundInsertPoint && (List_Links *)itemProcPtr == queuePtr) {
	    /*
	     * We are being inserted at the front
	     * of the queue so return ourselves.
	     */
	    *runPtrPtr = procPtr;
	    return;
	}
    }
    /*
     * Insert the process into the queue.
     */
    if (insert) {
	if (foundInsertPoint) {
	    List_Insert((List_Links *) procPtr, itemProcPtr);
	} else {
	    List_Insert((List_Links *) procPtr, LIST_ATREAR(queuePtr));
	}
	sched_Instrument.numReadyProcesses += 1;
    }
    /*
     * If we are to return a process then delete the first process from
     * the queue.
     */
    if (runPtrPtr != (Proc_ControlBlock **) NIL) {
	Proc_ControlBlock 	*tempPtr;
	tempPtr = (Proc_ControlBlock *)List_First(queuePtr);
	List_Remove((List_Links *)tempPtr);
	*runPtrPtr = tempPtr;
	sched_Instrument.numReadyProcesses -= 1;
    }
}
