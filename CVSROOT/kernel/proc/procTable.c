/* 
 * procTable.c --
 *
 *	Routines to manage the process table.  This maintains a monitor
 *	that synchronizes access to PCB's.
 *
 * Copyright 1985 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "proc.h"
#include "procInt.h"
#include "sync.h"
#include "sched.h"
#include "timer.h"
#include "list.h"
#include "vm.h"
#include "sys.h"
#include "machine.h"
#include "mem.h"
#include "rpc.h"

Sync_Lock	tableLock = {0, 0};
#define	LOCKPTR &tableLock

Proc_ControlBlock **proc_PCBTable;
Proc_ControlBlock  **proc_RunningProcesses;
#define PROC_MAX_PROCESSES 128
int proc_MaxNumProcesses;
int proc_MaxRunningProcesses;

int procLastSlot = 0;	/* Circular index into proctable for choosing slots */


/*
 * ----------------------------------------------------------------------------
 *
 * Proc_InitTable --
 *
 *	Initializes the PCB table and running process table.  Must be called
 *	at initialization time with interrupts off.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The PCB table is initialized.
 *
 * ----------------------------------------------------------------------------
 */

void
Proc_InitTable()
{
    register	int 		  i;
    register	Proc_ControlBlock *pcbPtr;

    proc_MaxRunningProcesses = sys_NumProcessors;
    proc_MaxNumProcesses     = PROC_MAX_PROCESSES;

    proc_PCBTable = (Proc_ControlBlock **)
        Vm_BootAlloc(proc_MaxNumProcesses * sizeof(pcbPtr));

    for (i = 0; i < proc_MaxNumProcesses; i++) {
	pcbPtr = (Proc_ControlBlock *) Vm_BootAlloc(sizeof(Proc_ControlBlock));
	proc_PCBTable[i] = pcbPtr; 

	List_InitElement((List_Links *)pcbPtr);
        pcbPtr->state		= PROC_UNUSED;
	pcbPtr->processID	= i;
	pcbPtr->genFlags	= 0;

	/*
	 *  Initialize the pointers to the list headers and the
	 *  PCB entry. These values do not change when the PCB
	 *  entry is re-used.
	 */
	pcbPtr->childList	= &(pcbPtr->childListHdr);
        pcbPtr->siblingElement.procPtr	= pcbPtr;
        pcbPtr->familyElement.procPtr	= pcbPtr;

	/*
	 *  Set the links to NIL to catch any invalid uses of
	 *  the lists before they are properly initialized.
	 *  These pointers change whenever the PCB entry is re-used.
	 */
        pcbPtr->childListHdr.nextPtr	= (List_Links *) NIL;
        pcbPtr->childListHdr.prevPtr	= (List_Links *) NIL;

	List_InitElement((List_Links *)&pcbPtr->siblingElement);
	List_InitElement((List_Links *)&pcbPtr->familyElement);

	pcbPtr->numGroupIDs	= 0;
	pcbPtr->groupIDs	= (int *) NIL;
	pcbPtr->eventHashChain.procPtr = pcbPtr;
	List_InitElement((List_Links *)&pcbPtr->eventHashChain);

	pcbPtr->peerHostID = NIL;
	pcbPtr->peerProcessID = (Proc_PID) NIL;
	pcbPtr->codeFileName[0] = '\0';
	pcbPtr->vmPtr = (Vm_ProcInfo *)NIL;
	pcbPtr->trapStackPtr = (Exc_TrapStack *) NIL;
	pcbPtr->rpcClientProcess = (Proc_ControlBlock *) NIL;

	pcbPtr->waitToken = 0;
	pcbPtr->timerArray = (struct ProcIntTimerInfo *) NIL;
    }
    
    proc_RunningProcesses = (Proc_ControlBlock **)
        Vm_BootAlloc(proc_MaxRunningProcesses * sizeof(pcbPtr));

    for (i = 0; i < proc_MaxRunningProcesses; i++) {
        proc_RunningProcesses[i] = (Proc_ControlBlock *) NIL;
    }

    ProcDebugInit();
}


/*
 * ----------------------------------------------------------------------------
 *
 * Proc_InitMainProc --
 *
 *	Finish initializing the process table by making a proc table entry
 *	for the main process.  Called with interrupts disabled.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The first element of the proc table is modified.
 *
 * ----------------------------------------------------------------------------
 */

void
Proc_InitMainProc()
{
    register	Proc_ControlBlock *procPtr;

#define MAIN_PID 0

    procPtr = proc_PCBTable[MAIN_PID];

    /*
     *  Initialize the main process.
     */
    procPtr->state		= PROC_RUNNING;
    procPtr->genFlags		= PROC_KERNEL;
    procPtr->syncFlags		= 0;
    procPtr->schedFlags		= 0;
    procPtr->processID	 	= MAIN_PID | (1 << PROC_GEN_NUM_SHIFT) | 
					(rpc_SpriteID << PROC_ID_NUM_SHIFT);
    procPtr->parentID		= procPtr->processID;
    procPtr->billingRate 	= PROC_NORMAL_PRIORITY;
    procPtr->recentUsage 	= 0;
    procPtr->weightedUsage 	= 0;
    procPtr->unweightedUsage 	= 0;
    procPtr->kernelCpuUsage     = timer_TicksZeroSeconds;
    procPtr->userCpuUsage       = timer_TicksZeroSeconds;
    procPtr->childKernelCpuUsage = timer_TicksZeroSeconds;
    procPtr->childUserCpuUsage  = timer_TicksZeroSeconds;
    procPtr->numQuantumEnds 	= 0;
    procPtr->numWaitEvents 	= 0;
    procPtr->cwdPtr		= (Fs_Stream *) NIL;
    procPtr->setJumpStatePtr	= (Sys_SetJumpState *) NIL;

    procPtr->stackStart 	= MACH_STACK_BOTTOM;

    procPtr->familyID 		= PROC_NO_FAMILY;	/* not in a family */
    
    List_Init(procPtr->childList);

    procPtr->userID		= 0;
    procPtr->effectiveUserID	= 0;
    procPtr->numGroupIDs	= 1;
    procPtr->groupIDs 		= (int *) Mem_Alloc(1 * sizeof(int));
    procPtr->groupIDs[0]	= 0;

    Vm_ProcInit(procPtr);
    Sig_ProcInit(procPtr);

    Proc_SetCurrentProc(Sys_GetProcessorNumber(), procPtr);

    ProcInitMainEnviron(procPtr);

    ProcFamilyHashInit();
}


/*
 * ----------------------------------------------------------------------------
 *
 * Proc_LockPID --
 *
 *	Determine the validity of the given pid and if valid return a pointer
 *	to the proc table entry.  The proc table entry is returned locked.
 *
 * Results:
 *	Pointer to proc table entry.
 *
 * Side effects:
 *	Proc table entry is locked.
 *
 * ----------------------------------------------------------------------------
 */

ENTRY Proc_ControlBlock *
Proc_LockPID(pid)
    Proc_PID	pid;
{
    register	Proc_ControlBlock *procPtr;

    LOCK_MONITOR;

    if (Proc_PIDToIndex(pid) >= proc_MaxNumProcesses) {
	UNLOCK_MONITOR;
	return((Proc_ControlBlock *) NIL);
    }
    procPtr = proc_PCBTable[Proc_PIDToIndex(pid)];

    while (TRUE) {
	if (procPtr->state == PROC_UNUSED || procPtr->state == PROC_DEAD) {
	    procPtr = (Proc_ControlBlock *) NIL;
	    break;
	}

	if (procPtr->genFlags & PROC_LOCKED) {
	    do {
		(void) Sync_Wait(&procPtr->lockedCondition, FALSE);
	    } while (procPtr->genFlags & PROC_LOCKED);
	} else {
	    if (!Proc_ComparePIDs(procPtr->processID, pid)) {
		procPtr = (Proc_ControlBlock *) NIL;
	    } else {
		procPtr->genFlags |= PROC_LOCKED;
	    }
	    break;
	}
    }

    UNLOCK_MONITOR;
    return(procPtr);
}


/*
 * ----------------------------------------------------------------------------
 *
 * Proc_Lock --
 *
 *	Lock the proc table entry.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Proc table entry is locked.
 *
 * ----------------------------------------------------------------------------
 */

ENTRY void
Proc_Lock(procPtr)
    register	Proc_ControlBlock *procPtr;
{
    LOCK_MONITOR;

    while (procPtr->genFlags & PROC_LOCKED) {
	(void) Sync_Wait(&procPtr->lockedCondition, FALSE);
    }
    procPtr->genFlags |= PROC_LOCKED;

    UNLOCK_MONITOR;
}


/*
 * ----------------------------------------------------------------------------
 *
 * Proc_Unlock --
 *
 *	Unlock the proc table entry.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Proc table entry is unlocked.
 *
 * ----------------------------------------------------------------------------
 */

ENTRY void
Proc_Unlock(procPtr)
    register	Proc_ControlBlock *procPtr;
{
    LOCK_MONITOR;

    if (!(procPtr->genFlags & PROC_LOCKED)) {
	Sys_Panic(SYS_FATAL, "Proc_Unlock: PCB not locked.\n");
    }
    procPtr->genFlags &= ~PROC_LOCKED;
    Sync_Broadcast(&procPtr->lockedCondition);

    UNLOCK_MONITOR;
}


/*
 * ----------------------------------------------------------------------------
 *
 * ProcGetUnusedPCB --
 *
 *	Return the first unused PCB.
 *
 * Results:
 *	Pointer to PCB.
 *
 * Side effects:
 *	Proc table entry marked as PROC_NEW.
 *
 * ----------------------------------------------------------------------------
 */

ENTRY Proc_ControlBlock *
ProcGetUnusedPCB()
{
    register	Proc_ControlBlock 	**procPtrPtr;
    register	Proc_ControlBlock 	*procPtr;
    register	int 			i;
    int					generation;

    LOCK_MONITOR;

    /*
     * Scan the proc table looking for an unused slot.  The search is
     * circular, starting just after the last slot chosen.  This is done
     * so that slots are not re-used often so the generation number of
     * each slot can just be a few bits wide.
     */
    for (i = procLastSlot, procPtrPtr = &proc_PCBTable[procLastSlot]; ; ) {
	if ((*procPtrPtr)->state == PROC_UNUSED) {
	    break;
	}
	i++;
	procPtrPtr++;
	if (i >= proc_MaxNumProcesses) {
	    i = 0;
	    procPtrPtr = &proc_PCBTable[0];
	}
	if (i == procLastSlot) {
	    Sys_Panic(SYS_FATAL, "ProcGetUnusedPCB: PCB table full!!\n");
	}
    }

    procLastSlot = i+1;
    if (procLastSlot >= proc_MaxNumProcesses) {
	procLastSlot = 0;
    }
    procPtr = *procPtrPtr;
    procPtr->genFlags = PROC_LOCKED;
    procPtr->state = PROC_NEW;
    /*
     *  The PCB entry has a generation number that is incremented each time
     *  the entry is re-used. The low-order bits are in index into
     *  the PCB table.
     */
    generation = (procPtr->processID & PROC_GEN_NUM_MASK) >> PROC_GEN_NUM_SHIFT;
    generation += 1;
    generation = (generation << PROC_GEN_NUM_SHIFT) & PROC_GEN_NUM_MASK;
    procPtr->processID = i | generation | (rpc_SpriteID << PROC_ID_NUM_SHIFT);

    UNLOCK_MONITOR;

    return(procPtr);
}


/*
 * ----------------------------------------------------------------------------
 *
 * ProcFreePCB --
 *
 *	Mark the given PCB as unused.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Proc table entry marked as PROC_UNUSED.
 *
 * ----------------------------------------------------------------------------
 */

ENTRY void
ProcFreePCB(procPtr)
    register	Proc_ControlBlock 	*procPtr;
{
    LOCK_MONITOR;

    while (procPtr->genFlags & PROC_LOCKED) {
	(void) Sync_Wait(&procPtr->lockedCondition, FALSE);
    }
    procPtr->state = PROC_UNUSED;
    procPtr->genFlags = 0;

    UNLOCK_MONITOR;
}


/*
 * ----------------------------------------------------------------------------
 *
 * ProcTableMatch --
 *
 *	Go through the process table and return an array of process
 *	IDs for which the specified function returns TRUE.
 *
 * Results:
 *	The array of PIDs and the number of matches are returned.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

ENTRY int
ProcTableMatch(maxPids, booleanFuncPtr, pidArray)
    int maxPids;			/* size of pidArray */
    Boolean (*booleanFuncPtr)();	/* function to match */
    Proc_PID *pidArray;			/* array to store results */
{
    Proc_ControlBlock *pcbPtr;
    int i;
    int matched = 0;
    
    LOCK_MONITOR;

    for (i = 0; i < proc_MaxNumProcesses && matched < maxPids; i++) {
	pcbPtr = proc_PCBTable[i];
	if (pcbPtr->state == PROC_UNUSED) {
	    continue;
	}
	if ((*booleanFuncPtr)(pcbPtr)) {
	    pidArray[matched] = pcbPtr->processID;
	    matched++;
	}
    }
    UNLOCK_MONITOR;
    return(matched);
}
