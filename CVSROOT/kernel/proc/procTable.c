/* 
 * procTable.c --
 *
 *	Routines to manage the process table.  This maintains a monitor
 *	that synchronizes access to PCB's.
 *
 * Copyright 1985, 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "mach.h"
#include "proc.h"
#include "procInt.h"
#include "sync.h"
#include "sched.h"
#include "timer.h"
#include "list.h"
#include "vm.h"
#include "sys.h"
#include "stdlib.h"
#include "rpc.h"

Sync_Lock	tableLock;
#define	LOCKPTR &tableLock

static Proc_ControlBlock  *RunningProcesses[MACH_MAX_NUM_PROCESSORS];
Proc_ControlBlock  **proc_RunningProcesses = RunningProcesses;
Proc_ControlBlock **proc_PCBTable;

#define PROC_MAX_PROCESSES 256
#define PROC_PCB_NUM_ALLOC 16
int proc_MaxNumProcesses;

int procLastSlot = 0;	/* Circular index into proctable for choosing slots */
static int realMaxProcesses;	/* The absolute number of process table
				 * entries, not necessarily allocated yet. */
static int entriesInUse = 0;	/* Number of PCB's in use. */

static void InitPCB();


/*
 * ----------------------------------------------------------------------------
 *
 * ProcInitTable --
 *
 *	Initializes the PCB table and running process table.  Must be called
 *	at initialization time with interrupts off.  Initializes an array
 *	of PROC_MAX_PROCESSES pointers to PCB's but only allocates
 *	PROC_PCB_NUM_ALLOC entries at first.  The rest are done dynamically.
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
ProcInitTable()
{
    register	int 		  i;
    register	Proc_ControlBlock *pcbPtr;
    int 	maxRunningProcesses;

    maxRunningProcesses = MACH_MAX_NUM_PROCESSORS;
    proc_MaxNumProcesses     = PROC_PCB_NUM_ALLOC;
    realMaxProcesses         = PROC_MAX_PROCESSES;

    proc_PCBTable = (Proc_ControlBlock **)
        Vm_BootAlloc(realMaxProcesses * sizeof(pcbPtr));

    for (i = 0; i < proc_MaxNumProcesses; i++) {
	pcbPtr = (Proc_ControlBlock *) Vm_BootAlloc(sizeof(Proc_ControlBlock));
	proc_PCBTable[i] = pcbPtr;
	InitPCB(pcbPtr, i);
    }

    /*
     * Set the rest of the proc table to catch any misuse of nonexistent
     * entries.
     */

    for (i = proc_MaxNumProcesses; i < realMaxProcesses; i++) {
	proc_PCBTable[i] = (Proc_ControlBlock *) NIL;
    }

    for (i = 0; i < maxRunningProcesses; i++) {
        proc_RunningProcesses[i] = (Proc_ControlBlock *) NIL;
    }
    Sync_LockInitDynamic(&tableLock, "Proc:tableLock");
}


/*
 * ----------------------------------------------------------------------------
 *
 * InitPCB --
 *
 *	Initializes a process control block.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */
static void
InitPCB(pcbPtr, i)
    Proc_ControlBlock *pcbPtr;
    int i;
{
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

    pcbPtr->eventHashChain.procPtr = pcbPtr;
    List_InitElement((List_Links *)&pcbPtr->eventHashChain);

    pcbPtr->peerHostID = NIL;
    pcbPtr->peerProcessID = (Proc_PID) NIL;
    pcbPtr->remoteExecBuffer = (Address) NIL;
    pcbPtr->argString = (char *) NIL;
#ifdef LOCKDEP
    pcbPtr->lockStackSize = 0;
#endif
    pcbPtr->vmPtr = (Vm_ProcInfo *)NIL;
    pcbPtr->fsPtr = (Fs_ProcessState *)NIL;
    pcbPtr->rpcClientProcess = (Proc_ControlBlock *) NIL;

    pcbPtr->waitToken = 0;
    pcbPtr->timerArray = (struct ProcIntTimerInfo *) NIL;

    pcbPtr->kcallTable = mach_NormalHandlers;
    pcbPtr->specialHandling = 0;
    pcbPtr->machStatePtr = (struct Mach_State *)NIL;
#ifndef CLEAN_LOCK
    Sync_SemInitDynamic(&pcbPtr->lockInfo, "Proc:perPCBlock");
#endif
#ifdef LOCKREG
    Sync_LockRegister(&pcbPtr->lockInfo);
#endif
}


/*
 * ----------------------------------------------------------------------------
 *
 *  AddPCBs --
 *
 *	Add new proc_ControlBlocks with sched_Mutex locked.  This avoids
 *	conflicts accessing the proc_MaxNumProcesses variable, such as in
 *	the Sched_ForgetUsage routine.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The global array of process control blocks is updated to point
 *	to the PCB's pointed to by procPtrPtr, and the count of useable entries
 *	is updated.
 *
 * ----------------------------------------------------------------------------
 */

void
AddPCBs(procPtrPtr)
    Proc_ControlBlock **procPtrPtr;
{
    register int i;
    
    /*
     *  Gain exclusive access to the process table.
     */
    MASTER_LOCK(sched_MutexPtr);

    for (i = 0; i < PROC_PCB_NUM_ALLOC; i++) {
	proc_PCBTable[proc_MaxNumProcesses] = *procPtrPtr;
	procPtrPtr++;
	proc_MaxNumProcesses++;
    }

    MASTER_UNLOCK(sched_MutexPtr);
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
 *	The first element of the proc table is modified, and the count of
 *	used entries is set to 1.
 *
 * ----------------------------------------------------------------------------
 */

void
Proc_InitMainProc()
{
    register	Proc_ControlBlock *procPtr;

#define MAIN_PID 0

    entriesInUse = 1;
    
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
    procPtr->kernelCpuUsage.ticks     = timer_TicksZeroSeconds;
    procPtr->userCpuUsage.ticks       = timer_TicksZeroSeconds;
    procPtr->childKernelCpuUsage.ticks = timer_TicksZeroSeconds;
    procPtr->childUserCpuUsage.ticks  = timer_TicksZeroSeconds;
    procPtr->numQuantumEnds 	= 0;
    procPtr->numWaitEvents 	= 0;

    procPtr->Prof_Buffer        = (short *) NIL;
    procPtr->Prof_BufferSize    = 0;
    procPtr->Prof_Offset        = 0;
    procPtr->Prof_Scale         = 0;
    procPtr->Prof_PC            = 0;

    Mach_InitFirstProc(procPtr);
    Vm_ProcInit(procPtr);
    VmMach_SetupContext(procPtr);

    procPtr->familyID 		= PROC_NO_FAMILY;	/* not in a family */
    
    List_Init(procPtr->childList);

    procPtr->userID		= 0;
    procPtr->effectiveUserID	= 0;

    Sig_ProcInit(procPtr);

    procPtr->processor = Mach_GetProcessorNumber();
    Proc_SetCurrentProc(procPtr);

    ProcInitMainEnviron(procPtr);

    ProcFamilyHashInit();

    procPtr->peerProcessID = (Proc_PID) NIL;
    procPtr->peerHostID = (int) NIL;
    procPtr->remoteExecBuffer = (Address) NIL;
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
    register	Sync_Semaphore	  *lockPtr;

    LOCK_MONITOR;

    if (Proc_PIDToIndex(pid) >= proc_MaxNumProcesses) {
	UNLOCK_MONITOR;
	return((Proc_ControlBlock *) NIL);
    }
    procPtr = proc_PCBTable[Proc_PIDToIndex(pid)];
#ifndef CLEAN_LOCK
    lockPtr = &(procPtr->lockInfo);
#endif

    while (TRUE) {
	if (procPtr->state == PROC_UNUSED || procPtr->state == PROC_DEAD) {
	    procPtr = (Proc_ControlBlock *) NIL;
	    break;
	}

	if (procPtr->genFlags & PROC_LOCKED) {
	    do {
		SyncRecordMiss(lockPtr);
		(void) Sync_Wait(&procPtr->lockedCondition, FALSE);
	    } while (procPtr->genFlags & PROC_LOCKED);
	} else {
	    if (!Proc_ComparePIDs(procPtr->processID, pid)) {
		procPtr = (Proc_ControlBlock *) NIL;
	    } else {
		procPtr->genFlags |= PROC_LOCKED;
		SyncRecordHit(lockPtr);
		SyncStoreDbgInfo(lockPtr);
		SyncAddPrior(lockPtr);
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
    register	Sync_Semaphore	  *lockPtr;

    LOCK_MONITOR;

#ifndef CLEAN_LOCK
    lockPtr = &(procPtr->lockInfo);
#endif

    while (procPtr->genFlags & PROC_LOCKED) {

	SyncRecordMiss(lockPtr);
	(void) Sync_Wait(&procPtr->lockedCondition, FALSE);
    }
    procPtr->genFlags |= PROC_LOCKED;

    SyncRecordHit(lockPtr);
    SyncStoreDbgInfo(lockPtr);
    SyncAddPrior(lockPtr);
    SyncAddPrior(lockPtr);

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
	panic("Proc_Unlock: PCB not locked.\n");
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
    Proc_ControlBlock 			*pcbArray[PROC_PCB_NUM_ALLOC];
    register	int 			i;
    int					generation;

    LOCK_MONITOR;



    /* 
     * See if we need to allocate more process table entries.
     */
    if (entriesInUse == proc_MaxNumProcesses) {
	if (proc_MaxNumProcesses > realMaxProcesses - PROC_PCB_NUM_ALLOC) {
	    panic("ProcGetUnusedPCB: PCB table full!!\n");
	}
	for (i = 0; i < PROC_PCB_NUM_ALLOC; i++) {
	    pcbArray[i] = (Proc_ControlBlock *)
		    Vm_RawAlloc(sizeof(Proc_ControlBlock));
	    InitPCB(pcbArray[i], proc_MaxNumProcesses + i);
	}
	AddPCBs(pcbArray);
    }

	
	
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
	/*
	 * Shouldn't hit this, but check to avoid infinite loop.
	 */
	if (i == procLastSlot) {
	    panic("ProcGetUnusedPCB: PCB table full!!\n");
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

    entriesInUse++;

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
    register	Sync_Semaphore	  *lockPtr;

    LOCK_MONITOR;

#ifndef CLEAN_LOCK
    lockPtr = &(procPtr->lockInfo);
#endif

    while (procPtr->genFlags & PROC_LOCKED) {
	SyncRecordMiss(lockPtr);
	(void) Sync_Wait(&procPtr->lockedCondition, FALSE);
    }
    procPtr->state = PROC_UNUSED;
    procPtr->genFlags = 0;
    entriesInUse--;

    SyncRecordHit(lockPtr);

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
