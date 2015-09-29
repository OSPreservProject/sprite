/* 
 * procTable.c --
 *
 *	Routines to manage the process table.  This maintains a monitor
 *	that synchronizes access to PCB's.
 *
 * Copyright 1985, 1988, 1991 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/sprited/proc/RCS/procTable.c,v 1.11 92/03/12 17:37:15 kupfer Exp $ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include <user/sync.h>

#include <fs.h>
#include <main.h>
#include <proc.h>
#include <procInt.h>
#include <rpc.h>
#include <sig.h>
#include <sync.h>
#include <utils.h>
#include <vm.h>

static Sync_Lock	tableLock;
#define	LOCKPTR &tableLock

Proc_ControlBlock **proc_PCBTable;

#define PROC_MAX_PROCESSES 256
#define PROC_PCB_NUM_ALLOC 16
int proc_MaxNumProcesses;

int procLastSlot = 0;	/* Circular index into proctable for choosing slots */
static unsigned int realMaxProcesses;	/* The absolute number of process table
					 * entries, not necessarily
					 * allocated yet. */ 
static int entriesInUse = 0;	/* Number of PCB's in use. */

/* 
 * State transition table for debugging.  The first index is the old state, 
 * the second index is the new state.  Sorry about using numbers instead of 
 * TRUE/FALSE, but this makes the table easier to read (or at least that's 
 * the theory).
 */
static Boolean legalState[PROC_NUM_STATES][PROC_NUM_STATES] = {
				/* to */			   /* from */
/* unusd, rning, ready, wait,  exit,  dead,  migrt, new,  susp */
  {  0,     0,     0,     0,     0,     0,     0,    1,     0},/* unused */
  {  0,     0,     0,     0,     0,     0,     0,    0,     0},/* running */
  {  0,     0,     1,     1,     1,     1,     1,    0,     1},/* ready */
  {  0,     0,     1,     0,     1,     1,     0,    0,     1},/* waiting*/
  {  0,     0,     0,     0,     0,     1,     0,    0,     0},/* exiting*/
  {  1,     0,     0,     0,     0,     1,     0,    0,     0},/* dead */
  {  0,     0,     0,     0,     1,     1,     0,    1,     0},/* migrated */
  {  1,     0,     1,     0,     0,     0,     1,    0,     0},/* new */
  {  0,     0,     1,     0,     1,     1,     0,    0,     1},/* suspended */
};

/* 
 * This is the key that we give to the thread local data package when 
 * we want to get or set the PCB that is associated with a thread. 
 */
static cthread_key_t procPcbKey;

static void 	InitPCB _ARGS_((Proc_ControlBlock *pcbPtr, int pid));
static void	AddPCBs _ARGS_((Proc_ControlBlock **procPtrPtr));


/*
 * ----------------------------------------------------------------------------
 *
 * ProcInitTable --
 *
 *	Initializes the PCB table.  Initializes an array of
 *	PROC_MAX_PROCESSES pointers to PCB's but only allocates
 *	PROC_PCB_NUM_ALLOC entries at first.  The rest are done
 *	dynamically.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The PCB table is initialized.  Also initializes procPcbKey.
 *
 * ----------------------------------------------------------------------------
 */
    
void
ProcInitTable()
{
    register	int 		  i;
    register	Proc_ControlBlock *pcbPtr;

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

    Sync_LockInitDynamic(&tableLock, "Proc:tableLock");

    if (cthread_keycreate(&procPcbKey) < 0) {
	panic("ProcInitTable: can't create thread local data key.\n");
    }
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
InitPCB(pcbPtr, pid)
    Proc_ControlBlock *pcbPtr;
    int pid;			/* the process ID to assign to the slot */
{
    List_InitElement((List_Links *)pcbPtr);
    pcbPtr->magic	= PROC_PCB_MAGIC_NUMBER;
    pcbPtr->backPtr	= pcbPtr;
    pcbPtr->state	= PROC_UNUSED;
    pcbPtr->processID	= pid;
    pcbPtr->genFlags	= 0;

    pcbPtr->syscallPort = MACH_PORT_NULL;
    pcbPtr->exceptionPort = MACH_PORT_NULL;
    pcbPtr->thread = MACH_PORT_NULL;
    pcbPtr->taskInfoPtr = NULL;

    pcbPtr->currCondPtr = NULL;
    Sync_ConditionInit(&pcbPtr->waitCondition, "proc:waitCondition", TRUE);
    Sync_ConditionInit(&pcbPtr->lockedCondition, "proc:lockedCondition",
		       TRUE);
    Sync_ConditionInit(&pcbPtr->sleepCondition, "proc:sleepCondition",
		       TRUE);
    Sync_ConditionInit(&pcbPtr->remoteCondition, "proc:remoteCondition",
		       TRUE);
    Sync_ConditionInit(&pcbPtr->resumeCondition, "proc:resumeCondition",
		       TRUE);

    /*
     *  Initialize the pointers to the list headers and the
     *  PCB entry. These values do not change when the PCB
     *  entry is re-used.
     */
    pcbPtr->childList	= &(pcbPtr->childListHdr);
    pcbPtr->siblingElement.procPtr	= pcbPtr;
    pcbPtr->familyElement.procPtr	= pcbPtr;
    pcbPtr->deadElement.procPtr		= pcbPtr;

    /*
     *  Set the links to NIL to catch any invalid uses of
     *  the lists before they are properly initialized.
     *  These pointers change whenever the PCB entry is re-used.
     */
    pcbPtr->childListHdr.nextPtr	= (List_Links *) NIL;
    pcbPtr->childListHdr.prevPtr	= (List_Links *) NIL;

    List_InitElement((List_Links *)&pcbPtr->siblingElement);
    List_InitElement((List_Links *)&pcbPtr->familyElement);
    List_InitElement((List_Links *)&pcbPtr->deadElement);

    pcbPtr->peerHostID = NIL;
    pcbPtr->peerProcessID = (Proc_PID) NIL;
    pcbPtr->remoteExecBuffer = (Address) NIL;
    pcbPtr->migCmdBuffer = (Address) NIL;
    pcbPtr->migCmdBufSize = 0;
    pcbPtr->migFlags = 0;
    pcbPtr->argString = (char *) NIL;

#ifdef LOCKDEP
    pcbPtr->lockStackSize = 0;
#endif
    pcbPtr->locksHeld = 0;
    pcbPtr->fsPtr = (Fs_ProcessState *)NIL;
    pcbPtr->rpcClientProcess = (Proc_ControlBlock *) NIL;

    pcbPtr->waitToken = 0;
    pcbPtr->timerArray = (struct ProcIntTimerInfo *) NIL;

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
 *	Add new proc_ControlBlocks.
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

static INTERNAL void
AddPCBs(procPtrPtr)
    Proc_ControlBlock **procPtrPtr;
{
    register int i;
    
    for (i = 0; i < PROC_PCB_NUM_ALLOC; i++) {
	proc_PCBTable[proc_MaxNumProcesses] = *procPtrPtr;
	procPtrPtr++;
	proc_MaxNumProcesses++;
    }
}
    

/*
 * ----------------------------------------------------------------------------
 *
 * Proc_InitMainProc --
 *
 *	Finish initializing the process table by making a proc table entry
 *	for the main server process.  The first entry in the table 
 *	should already be initialized as an unused entry.
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

    entriesInUse = 1;
    
    procPtr = proc_PCBTable[PROC_MAIN_PROC_SLOT];
    cthread_set_name(cthread_self(), "main");

    /*
     *  Fix up any fields that InitPCB didn't set or set to the wrong 
     *  value. 
     *  XXX This business of calling Proc_SetState twice is ugly.  
     *  Maybe we should just set the process's state directly (it is a 
     *  special case, after all).
     */
    
    Proc_SetState(procPtr, PROC_NEW);
    Proc_SetState(procPtr, PROC_READY);
    procPtr->genFlags		= PROC_KERNEL;
    procPtr->syncFlags		= 0;
    procPtr->exitFlags		= 0;
    procPtr->processID	 	= (PROC_MAIN_PROC_SLOT |
				   (1 << PROC_GEN_NUM_SHIFT) | 
				   (rpc_SpriteID << PROC_ID_NUM_SHIFT));
    procPtr->parentID		= procPtr->processID;
    procPtr->familyID 		= PROC_NO_FAMILY;	/* not in a family */
    
#ifdef SPRITED_PROFILING
    procPtr->Prof_Buffer        = (short *) NIL;
    procPtr->Prof_BufferSize    = 0;
    procPtr->Prof_Offset        = 0;
    procPtr->Prof_Scale         = 0;
    procPtr->Prof_PC            = 0;
#endif

    List_Init(procPtr->childList);

    procPtr->userID		= 0;
    procPtr->effectiveUserID	= 0;

    Sig_ProcInit(procPtr);

    Proc_SetCurrentProc(procPtr);

    ProcInitMainEnviron(procPtr);

    ProcFamilyHashInit();

    procPtr->peerProcessID = (Proc_PID) NIL;
    procPtr->peerHostID = (int) NIL;
    procPtr->remoteExecBuffer = (Address) NIL;

    procPtr->termReason = 0;
    procPtr->termStatus = 0;
    procPtr->termCode = 0;

    procPtr->unixErrno = 0;
    procPtr->unixProgress = -1;	/* XXX avoid magic numbers */
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

ENTRY Proc_LockedPCB *
Proc_LockPID(pid)
    Proc_PID	pid;
{
    register	Proc_ControlBlock *procPtr;
#ifndef CLEAN_LOCK
    register	Sync_Semaphore	  *lockPtr;
#endif

    LOCK_MONITOR;

    if (Proc_PIDToIndex(pid) >= proc_MaxNumProcesses) {
	UNLOCK_MONITOR;
	return NULL;
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
		Sync_RecordMiss(lockPtr);
		(void) Sync_Wait(&procPtr->lockedCondition, FALSE);
	    } while (procPtr->genFlags & PROC_LOCKED);
	} else {
	    if (!Proc_ComparePIDs(procPtr->processID, pid)) {
		procPtr = (Proc_ControlBlock *) NIL;
	    } else {
		procPtr->genFlags |= PROC_LOCKED;
		Sync_RecordHit(lockPtr);
		Sync_StoreDbgInfo(lockPtr, FALSE);
		Sync_AddPrior(lockPtr);
	    }
	    break;
	}
    }

    UNLOCK_MONITOR;
    return Proc_AssertLocked(procPtr);
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
#ifndef CLEAN_LOCK
    register	Sync_Semaphore	  *lockPtr;
#endif

    LOCK_MONITOR;

#ifndef CLEAN_LOCK
    lockPtr = &(procPtr->lockInfo);
#endif

    while (procPtr->genFlags & PROC_LOCKED) {
	Sync_RecordMiss(lockPtr);
	(void) Sync_Wait(&procPtr->lockedCondition, FALSE);
    }
    procPtr->genFlags |= PROC_LOCKED;

    Sync_RecordHit(lockPtr);
    Sync_StoreDbgInfo(lockPtr, FALSE);
    Sync_AddPrior(lockPtr);

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
    register	Proc_LockedPCB *procPtr;
{
    LOCK_MONITOR;

#ifndef CLEAN_LOCK
    if (!(procPtr->pcb.genFlags & PROC_LOCKED)) {
	panic("Proc_Unlock: PCB not locked.\n");
    }
#endif
    procPtr->pcb.genFlags &= ~PROC_LOCKED;
    Sync_Broadcast(&procPtr->pcb.lockedCondition);

    UNLOCK_MONITOR;
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_UnlockAndSwitch --
 *
 *	Unlock a PCB and perform a context switch to the given state.  
 *	This is done atomically: no other process can lock the PCB before 
 *	this process context switches.  
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The process is unlocked when this routine returns.
 *
 *----------------------------------------------------------------------
 */

void
Proc_UnlockAndSwitch(procPtr, state)
    Proc_LockedPCB *procPtr;	/* the PCB to unlock */
    Proc_State state;		/* the state to context switch to */
{
    LOCK_MONITOR;

    if (!(procPtr->pcb.genFlags & PROC_LOCKED)) {
	panic("Proc_Unlock: PCB not locked.\n");
    }
    procPtr->pcb.genFlags &= ~PROC_LOCKED;
    Sync_Broadcast(&procPtr->pcb.lockedCondition);

    switch (state) {
    case PROC_DEAD:
    case PROC_EXITING:
	Sync_UnlockAndSwitch(LOCKPTR, state);
	panic("Proc_UnlockAndSwitch: Sync_UnlockAndSwitch returned.\n");
	break;
    case PROC_SUSPENDED:
	/* 
	 * Hang out until somebody else resumes this process.  Note that 
	 * the PCB's lock is used for synchronization, rather than a simple 
	 * monitor lock.  This complicates the wait loop below.
	 */
	Proc_SetState((Proc_ControlBlock *)procPtr, state);
	while (procPtr->pcb.state == PROC_SUSPENDED) {
	    Sync_Wait(&procPtr->pcb.resumeCondition, FALSE);
	    /* Make sure nobody else has the process locked. */
	    while (procPtr->pcb.genFlags & PROC_LOCKED) {
		Sync_RecordMiss(&procPtr->pcb.lockInfo);
		Sync_Wait(&procPtr->pcb.lockedCondition, FALSE);
	    }
	}
	break;
    default:
	panic("Proc_UnlockAndSwitch: unexpected state.\n");
	break;
    };

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
 *	Proc table entry is locked and marked as PROC_NEW.
 *
 * ----------------------------------------------------------------------------
 */

ENTRY Proc_LockedPCB *
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
    procPtr->migFlags = 0;
    Proc_SetState(procPtr, PROC_NEW);
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

    return Proc_AssertLocked(procPtr);
}


/*
 * ----------------------------------------------------------------------------
 *
 * ProcFreePCB --
 *
 *	Mark the given locked PCB as unused.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Proc table entry marked as unused and unlocked.
 *
 * ----------------------------------------------------------------------------
 */

ENTRY void
ProcFreePCB(procPtr)
    register	Proc_LockedPCB 	*procPtr;
{
    LOCK_MONITOR;
    Proc_SetState((Proc_ControlBlock *)procPtr, PROC_UNUSED);
    procPtr->pcb.genFlags = 0;
    entriesInUse--;
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
    unsigned int maxPids;		/* size of pidArray */
    Boolean (*booleanFuncPtr) _ARGS_((Proc_ControlBlock *pcbPtr));
					/* function to match */
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


/*
 *----------------------------------------------------------------------
 *
 * Proc_SetState --
 *
 *	Set process state, verifying that the state transition is 
 *	expected.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sets the state of the given process to the given state.
 *
 *----------------------------------------------------------------------
 */

#ifndef CLEAN
void
Proc_SetState(procPtr, newState)
    Proc_ControlBlock *procPtr;
    Proc_State newState;
{
    if (!legalState[(int)(procPtr->state)][(int)newState]) {
	panic("Proc_SetState: pid %x old state %s, new state %s\n",
	       procPtr->processID, Proc_StateName(procPtr->state),
	       Proc_StateName(newState));
    }

    procPtr->state = newState;
}
#endif /* CLEAN */


/*
 *----------------------------------------------------------------------
 *
 * Proc_GetCurrentProc --
 *
 *	Get the PCB for the currently running process.
 *
 * Results:
 *	Returns a pointer to the PCB associated with the current 
 *	thread.  Returns NULL if no PCB is associated with the thread 
 *	(e.g., during system initialization).
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Proc_ControlBlock *
Proc_GetCurrentProc()
{
    Proc_ControlBlock *procPtr;

    if (!main_MultiThreaded) {
	return NULL;
    }

    if (cthread_getspecific(procPcbKey, (any_t *)&procPtr) < 0) {
	panic("Proc_GetCurrentProc: key not recognized.\n");
    }

    return procPtr;
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_SetCurrentProc --
 *
 *	Record the given process as the currently running one.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Binds the given PCB to the thread local data for the currently 
 *	running thread.
 *	Note: if the PCB contains a back pointer to the thread, this is 
 *	not the place to set it, because this routine must be callable with 
 *	the process unlocked.
 *
 *----------------------------------------------------------------------
 */

void
Proc_SetCurrentProc(procPtr)
    Proc_ControlBlock *procPtr;
{
    if (cthread_setspecific(procPcbKey, (any_t)procPtr) < 0) {
	panic("Proc_SetCurrentProc: cthread_setspecific failed.\n");
    }
}
