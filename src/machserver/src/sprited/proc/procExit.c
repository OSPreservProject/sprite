/*
 * procExit.c --
 *
 *	Routines to terminate and detach processes, and to cause a 
 *	process to wait for the termination of other processes.  This file
 *	maintains a monitor to synchronize between exiting, detaching, and
 *	waiting processes.  The monitor also synchronizes access to the
 *	dead list.
 *
 * Copyright 1986, 1988, 1991 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 *	Proc table fields managed by this monitor:
 *
 *	    1) exitFlags is managed solely by this monitor.
 *	    2) When a process is about to exit the PROC_DYING flag is set
 *	       in the genFlags field.
 *	    3) The only way a process can go to the exiting and dead states 
 *	       is through using routines in this file.
 *	
 *	To avoid deadlock, obtain the monitor lock before locking a pcb.
 *
 *	A process can be in one of several states:
 *
 *	PROC_READY:	It is ready to execute, at least as far as 
 *			Sprite is concerned.  Whether it is actually 
 *			running is up to the kernel.
 *	PROC_WAITING:	It is waiting for an event to occur. 
 *	PROC_EXITING:	It has finished executing but the PCB is kept around
 *			until the parent waits for it via Proc_Wait or dies
 *			via Proc_Exit.
 *	PROC_DEAD:	It has been waited on and the PCB is attached to 
 *			the Dead list.
 *	PROC_SUSPENDED  It is suspended from executing.
 *	PROC_UNUSED:	There is no process using this PCB entry.
 *
 *	In addition, the process may have the following attributes:
 *
 *	PROC_DETACHED:	It is detached from its parent.
 *	PROC_WAITED_ON:	It is detached from its parent and its parent
 *			knows about it. This attribute implies PROC_DETACHED
 *			has been set.
 *	PROC_SUSPEND_STATUS:
 *			It is suspended and its parent has not waited on it
 *			yet.  In this case it isn't detached.
 *	PROC_RESUME_STATUS:
 *			It has been resumed and its parent has not waited on
 *			it yet.  In this case it isn't detached.
 *
 *	These attributes are set independently of the process states.
 *
 *	The routines in this file deal with transitions from the
 *	RUNNING state to the EXITING and DEAD states and methods to 
 *	detach a process.
 *
 *  State and Attribute Transitions:
 *	The following tables show how a process changes attributes and states.
 *
 *
 *   Legend:
 *	ATTRIBUTE1 -> ATTRIBUTE2:
 *	---------------------------------------------------------------------
 *	  who		"routine it called to change attribute"	comments
 *
 *
 *
 *	Attached  -> Detached
 *	---------------------------------------------------------------------
 *	 current process	Proc_Detach
 *
 *	Detached -> Detached and Waited on
 *	---------------------------------------------------------------------
 *	 parent 		Proc_Wait	WAITED_ON attribute set when
 *						parent finds child is detached.
 *
 *	Attached or Detached -> Detached and Waited on
 *	---------------------------------------------------------------------
 *	 parent			Proc_ExitInt	parent exiting, child detached.
 *
 *
 *
 *
 *   Legend:
 *	STATE1 -> STATE2:	(attributes before transition)
 *	---------------------------------------------------------------------
 *	  who		"routine it called to change state" 	comments
 *
 *
 *	RUNNING -> EXITING:	(attached or detached but not waited on)
 *	RUNNING -> DEAD:	(detached and waited on)
 *	---------------------------------------------------------------------
 *	 current process	Proc_Exit	normal termination.
 *	 kernel			Proc_ExitInt	invalid process state found or
 *						process got a signal
 *
 *	EXITING -> DEAD:	(attached or detached or detached and waited on)
 *	---------------------------------------------------------------------
 *	 last family member to exit	Proc_ExitInt	
 *	 parent			Proc_Wait	parent waiting for child to exit
 *	 parent			Proc_ExitInt	parent exiting but did not 
 *						wait on child.
 *
 *	DEAD -> UNUSED:
 *	---------------------------------------------------------------------
 *	  The Reaper		Proc_Reaper 	kernel process to clean the
 *						dead list. 
 */

#ifndef lint
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/sprited/proc/RCS/procExit.c,v 1.19 92/07/16 18:06:50 kupfer Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <sprite.h>
#include <ckalloc.h>
#include <mach.h>
#include <mach_error.h>
#include <spriteTime.h>
#include <status.h>
#include <user/sig.h>

#include <proc.h>
#include <procInt.h>
#include <sig.h>
#include <spriteSrvServer.h>
#include <sync.h>
#include <sys.h>
#include <timerTick.h>
#include <utils.h>
#include <vm.h>
#include <user/vmStat.h>

#define min(a,b) ((a) < (b) ? (a) : (b))

static	Sync_Lock	exitLock = Sync_LockInitStatic("Proc:exitLock"); 
#define	LOCKPTR &exitLock

/* 
 * When a process is put into the PROC_DEAD state, it is put on a linked 
 * list, so that the pcb can be reused.  This reclamation can only happen 
 * at certain times to avoid races between process destruction and process 
 * requests. 
 */
static List_Links deadProcessListHdr;
#define deadProcessList	(&deadProcessListHdr)

/*
 * Shared memory.
 */
extern int vmShmDebug;
#ifndef lint
#define dprintf if (vmShmDebug) printf
#else /* lint */
#define dprintf printf
#endif /* lint */

/* Forward references: */

static ReturnStatus 	CheckPidArray _ARGS_((Proc_ControlBlock *curProcPtr,
				Boolean returnSuspend, int numPids,
				Proc_PID *pidArray,
				Proc_LockedPCB **procPtrPtr));
static void		UpdateChildCpuUsage _ARGS_((
				Proc_ControlBlock *parentProcPtr,
				Proc_ControlBlock *exitProcPtr));
extern ReturnStatus 	DoWait _ARGS_((Proc_ControlBlock *curProcPtr,
			    int	flags, int numPids, Proc_PID *newPidArray,
			    ProcChildInfo *childInfoPtr));
static Proc_State	ExitProcessInt _ARGS_((
				Proc_ControlBlock *exitProcPtr,
				Boolean	migrated, Boolean contextSwitch));
static ReturnStatus	FindExitingChild _ARGS_((
				    Proc_ControlBlock *parentProcPtr,
				    Boolean returnSuspend, int numPids,
				    Proc_PID *pidArray, 
				    ProcChildInfo *infoPtr));
static ReturnStatus 	LookForAnyChild _ARGS_((Proc_ControlBlock *curProcPtr,
				Boolean returnSuspend, 
				Proc_LockedPCB **procPtrPtr));
static void		ProcPutOnDeadList _ARGS_((Proc_ControlBlock *procPtr));
static void 		SendSigChild _ARGS_((ClientData data, 
				Proc_CallInfo *callInfoPtr));
static void		WakeupMigratedParent _ARGS_((Proc_PID pid));


/*
 *----------------------------------------------------------------------
 *
 * ProcExitInit --
 *
 *	Initialization for this monitor.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Initializes the list of dead processes.
 *
 *----------------------------------------------------------------------
 */

void
ProcExitInit()
{
    List_Init(deadProcessList);
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_RawExitStub --
 *
 *	MIG stub for when a user process has decided it wants to be an 
 *	ex-process. 
 *
 * Results:
 *	Returns MIG_NO_REPLY, so that no reply message is sent to the 
 *	client process.
 *
 * Side effects:
 *	Sets the PROC_NO_MORE_REQUESTS flag for the process, so that the 
 *	process will exit when all the RPC bookkeeping is done.
 *
 *----------------------------------------------------------------------
 */

kern_return_t
Proc_RawExitStub(serverPort, status)
    mach_port_t serverPort;	/* system request port */
    int status;			/* UNIX exit status code */
{
    Proc_ControlBlock *procPtr = Proc_GetCurrentProc();
    kern_return_t kernStatus;

#ifdef lint
    serverPort = serverPort;
#endif
    Proc_Lock(procPtr);

    /* 
     * Freeze the thread, so that it can't run after its reply message is 
     * destroyed. 
     */
    kernStatus = thread_suspend(procPtr->thread);
    if (kernStatus != KERN_SUCCESS) {
	printf("Proc_RawExitStub: can't suspend thread for pid %x: %s.\n",
	       procPtr->processID, mach_error_string(kernStatus));
    }

    procPtr->genFlags |= PROC_NO_MORE_REQUESTS;
    procPtr->termReason = PROC_TERM_EXITED;
    procPtr->termStatus = status;
    procPtr->termCode = 0;
    Proc_Unlock(Proc_AssertLocked(procPtr));

    return MIG_NO_REPLY;
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_Exit --
 *
 *	The current process has decided to end it all voluntarily.
 *	Call an internal procedure to do the work.
 *
 * Results:
 *	None.  This routine should NOT return!
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
Proc_Exit(status)
    int	status;		/* Exit status from caller. */
{
    Proc_ExitInt(PROC_TERM_EXITED, status, 0);

    /*
     *  Proc_ExitInt should never return.
     */
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_ExitInt --
 *
 *	Internal routine to handle the termination of a process.  It
 *	determines the process that is exiting and then calls
 *	ProcExitProcess to do the work. This routine does not return.
 *	If the process is foreign, ProcRemoteExit is called to handle
 *	cleanup on the home node of the process, then ProcExitProcess
 *	is called.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
Proc_ExitInt(reason, status, code)
    int reason;	/* Why the process is dying: EXITED, SIGNALED, DESTROYED  */
    int	status;	/* Exit status or signal # or destroy status */
    int code;	/* Signal sub-status */
{
    register Proc_ControlBlock 	*curProcPtr;

    curProcPtr = Proc_GetActualProc();
    if (curProcPtr == (Proc_ControlBlock *) NIL) {
	panic("Proc_ExitInt: bad procPtr.\n");
    }

#ifdef SPRITED_MIGRATION
    if (curProcPtr->genFlags & PROC_FOREIGN) {
	ProcRemoteExit(curProcPtr, reason, status, code);
    }
#endif
#ifdef SPRITED_PROFILING
    if (curProcPtr->Prof_Scale != 0) {
	Prof_Disable(curProcPtr);
    }
#endif
    if (curProcPtr->genFlags & PROC_DEBUGGED) {
	/*
	 * If a process is being debugged then force it onto the debug
	 * list before allowing it to exit.  Note that Proc_SuspendProcess
	 * will unlock the PCB.
	 */
	Proc_Lock(curProcPtr);
	Proc_SuspendProcess(Proc_AssertLocked(curProcPtr), TRUE, reason, 
			    status, code);
    }

    if (sys_ErrorShutdown) {
	/*
	 * Are shutting down the system because of an error.  In this case
	 * don't close anything down because we want to leave as much state
	 * around as possible.
	 */
	Proc_ContextSwitch(PROC_DEAD);
    }
    ProcExitProcess(curProcPtr, reason, status, code, TRUE);

    panic("Proc_ExitInt: Exiting process still alive!!!\n");
}


/*
 *----------------------------------------------------------------------
 *
 * ExitProcessInt --
 *
 *	Do monitor level exit stuff for the process.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The process is locked.  If it doesn't context switch, it is left
 *	locked.
 *
 *----------------------------------------------------------------------
 */

ENTRY static Proc_State
ExitProcessInt(exitProcPtr, foreign, contextSwitch) 
    register Proc_ControlBlock	*exitProcPtr;	/* The exiting process. */
    Boolean			foreign;	/* TRUE => foreign process. */
    Boolean			contextSwitch;	/* TRUE => context switch. */
{
    register	Proc_ControlBlock	*procPtr;
    Proc_State				newState = PROC_UNUSED;
    register Proc_PCBLink 		*procLinkPtr;
#if !defined(CLEAN) && defined(SPRITED_MIGRATION)
    Timer_Ticks 		        ticks;
#endif

    LOCK_MONITOR;

    Proc_Lock(exitProcPtr);

    exitProcPtr->genFlags |= PROC_DYING;
  
#ifdef SPRITED_MIGRATION
    if (exitProcPtr->genFlags & PROC_MIG_PENDING) {
	/*
	 * Someone is waiting for this guy to migrate.  Let them know that
	 * the process is dying.
	 */
	exitProcPtr->genFlags &= ~PROC_MIG_PENDING;
	ProcMigWakeupWaiters();
    }
#endif

    /*
     *  If the parent is still around, add the user and kernel cpu usage
     *  of this process to the parent's summary of children.
     *  If we're detached, don't give this stuff to the parent
     *  (it might not exist.)  Keep track of global usage if we're local.
     */

    if (!(exitProcPtr->genFlags & PROC_FOREIGN)) {
	if (!(exitProcPtr->exitFlags & PROC_DETACHED)) {
	    register Proc_ControlBlock 	*parentProcPtr;

	    parentProcPtr = Proc_GetPCB(exitProcPtr->parentID);
	    if (parentProcPtr != (Proc_ControlBlock *) NIL) {
		UpdateChildCpuUsage(parentProcPtr, exitProcPtr);
	    }
	}
#if !defined(CLEAN) && defined(SPRITED_MIGRATION)
	Timer_AddTicks(exitProcPtr->kernelCpuUsage.ticks,
			exitProcPtr->userCpuUsage.ticks, &ticks);
	ProcRecordUsage(ticks, PROC_MIG_USAGE_TOTAL_CPU);
	/*
	 * Record usage for just the amount of work performed after the
	 * first eviction.
	 */
	if (exitProcPtr->migFlags & PROC_WAS_EVICTED) {
	    if (proc_MigDebugLevel > 4) {
		printf("ExitProcessInt: process %x was evicted.  Used %d ticks before eviction, %d total.\n",
		       exitProcPtr->processID,
		       exitProcPtr->preEvictionUsage.ticks,
		       ticks);
	    }
	    Timer_SubtractTicks(ticks, exitProcPtr->preEvictionUsage.ticks,
				&ticks);
	    ProcRecordUsage(ticks, PROC_MIG_USAGE_POST_EVICTION);
	    exitProcPtr->migFlags &= ~PROC_WAS_EVICTED;
	}
#endif /* !CLEAN && SPRITED_MIGRATION */
    }

    
    /*
     * Make sure there are no lingering interval timer callbacks associated
     * with this process.
     *
     *  Go through the list of children of the current process to 
     *  make them orphans. When the children exit, this will typically
     *  cause them to go on the dead list directly.
     */

    if (!foreign) {
	ProcDeleteTimers(Proc_AssertLocked(exitProcPtr));
	while (!List_IsEmpty(exitProcPtr->childList)) {
	    procLinkPtr = (Proc_PCBLink *)List_First(exitProcPtr->childList);
	    procPtr = procLinkPtr->procPtr;
	    List_Remove((List_Links *) procLinkPtr);
	    if (procPtr->state == PROC_EXITING) {
		/*
		 * The child is exiting waiting for us to wait for it.
		 */
		ProcPutOnDeadList(procPtr);
	    } else {
		/*
		 * Detach the child so when it exits, it will be
		 * put on the dead list automatically.
		 */
		procPtr->exitFlags = PROC_DETACHED | PROC_WAITED_ON;
	    }
	}
    }

    /*
     * If the debugger is waiting for this process to return to the debug
     * state wake it up so that it will realize that the process is dead.
     */

    if (exitProcPtr->genFlags & PROC_DEBUG_WAIT) {
	ProcDebugWakeup();
    }

#ifdef SPRITED_MIGRATION
    /*
     * If the process is still waiting on an event, this is an error.
     * [For now, flag this error only for foreign processes in case this
     * isn't really an error after all.]
     */
    if (foreign && (exitProcPtr->currCondPtr != NULL)) {
	if (proc_MigDebugLevel > 0) {
	    panic(
		"ExitProcessInt: process still waiting on condition.\n");
	} else {
	    printf(
	     "Warning: ExitProcessInt: process still waiting on condition.\n");
	}
    }
#endif /* SPRITED_MIGRATION */

    /*
     * If the current process is detached and waited on (i.e. an orphan) then
     * one of two things happen.  If the process is a family head and its list
     * of family members is not empty then the process is put onto the exiting
     * list.  Otherwise the process is put onto the dead list since its 
     * parent has already waited for it.
     */
    
    if (((exitProcPtr->exitFlags & PROC_DETACHED) &&
	 (exitProcPtr->exitFlags & PROC_WAITED_ON))
	|| foreign) {
	ProcPutOnDeadList(exitProcPtr);
	newState = PROC_DEAD;
    } else {
	Proc_ControlBlock 	*parentProcPtr;

#ifdef DEBUG_PARENT_PID
	int hostID;
	
	hostID = Proc_GetHostID(exitProcPtr->parentID);
	if (hostID != rpc_SpriteID && hostID != 0) {
	    panic("ExitProcessInt: parent process (%x) is on wrong host.\n",
		  exitProcPtr->parentID);
	    goto done;
	}
#endif DEBUG_PARENT_PID
	parentProcPtr = Proc_GetPCB(exitProcPtr->parentID);
	if (parentProcPtr == (Proc_ControlBlock *) NIL) {
	    panic("ExitProcessInt: no parent process (pid == %x)\n",
		  exitProcPtr->parentID);
	    goto done;
	}
	if (parentProcPtr->state != PROC_MIGRATED) {
	    Sync_Broadcast(&parentProcPtr->waitCondition);
	} else {
	    WakeupMigratedParent(parentProcPtr->processID);
	}
	/*
	 * Signal the parent later on, when not holding the exit monitor
	 * lock.
	 */
	Proc_CallFunc(SendSigChild, (ClientData)exitProcPtr->parentID,
		      time_ZeroSeconds);

	newState = PROC_EXITING;
    }
done:
    if (contextSwitch) {
	Proc_Unlock(Proc_AssertLocked(exitProcPtr));
	UNLOCK_MONITOR_AND_SWITCH(newState);
	panic("ExitProcessInt: Exiting process still alive\n");
    } else {
	UNLOCK_MONITOR;
    }
    return(newState);
}


/*
 *----------------------------------------------------------------------
 *
 * ProcExitProcess --
 *
 *	Internal routine to handle the termination of a process, due
 *	to a normal exit, a signal or because the process state was
 *	inconsistent.  The file system state associated with the process 
 *	is invalidated. Any children of the process will become detatched.
 *	
 *	Warning: this function should be called only once for a given 
 *	process.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The PCB entry for the process is modified to clean-up FS
 *	state. The specified process may be placed on the dead list.
 *  	If thisProcess is TRUE, a context switch is performed.  If not,
 *	then the exit is being performed on behalf of another process.
 *
 *----------------------------------------------------------------------
 */
void
ProcExitProcess(exitProcPtr, reason, status, code, thisProcess) 
    register Proc_ControlBlock 	*exitProcPtr;	/* Exiting process; should 
						 * be locked if not the
						 * current process */
    int 			reason;		/* Why the process is dieing: 
						 * EXITED, SIGNALED, 
						 * DESTROYED  */
    int				status;		/* Exit status, signal # or 
						 * destroy status. */
    int 			code;		/* Signal sub-status */
    Boolean 			thisProcess;	/* TRUE => context switch */
{
    register Boolean 		foreign;
    int				currHeapPages; /* num. of mapped heap pages */
    int				currStackPages;

    foreign = (exitProcPtr->genFlags & PROC_FOREIGN);

    /*
     * Decrement the reference count on the environment.
     */

    if (!foreign && exitProcPtr->environPtr != (Proc_EnvironInfo *) NIL) {
	ProcDecEnvironRefCount(exitProcPtr->environPtr);
    }

    /*
     * If the process is being killed off by another process, it should 
     * already be locked.  Also, it shouldn't be in the middle of a 
     * request, since we can't synchronize with the thread handling the 
     * request inside the server.  
     */
    if (thisProcess) {
	Proc_Lock(exitProcPtr);
    } else {
	if (!(exitProcPtr->genFlags & PROC_LOCKED)) {
	    panic("ProcExitProcess: process isn't locked.\n");
	}
	if (exitProcPtr->genFlags & PROC_BEING_SERVED) {
	    panic("ProcExitProcess: active process is being killed.\n");
	}
    }

    /* 
     * If the process was already marked with PROC_NO_MORE_REQUESTS, then 
     * the code that set the flag should have also filled in the 
     * termination information.  (XXX Ugly.)
     * This is so that a user process can be marked as "about to exit" 
     * (e.g., Proc_Kill, Proc_RawExitStub), clean up from a current request 
     * (e.g., free request buffers), and then exit while getting the exit 
     * status codes right.
     */
    if (exitProcPtr->genFlags & PROC_NO_MORE_REQUESTS) {
	/* sanity check */
	if (exitProcPtr->termReason < PROC_TERM_EXITED ||
	        exitProcPtr->termReason > PROC_TERM_DESTROYED) {
	    panic("ProcExitProcess: bogus termination reason.\n");
	}
    } else {
	exitProcPtr->genFlags |= PROC_NO_MORE_REQUESTS;
	exitProcPtr->termReason	= reason;
	exitProcPtr->termStatus	= status;
	exitProcPtr->termCode	= code;
    }

    /* 
     * Some instrumentation.  Figure out how many pages had to get faulted 
     * back in again because the heap and stack segments were destroyed and 
     * then recreated when the process last did an exec.  For each segment, 
     * this is the smaller of {the current number of pages, the number of 
     * pages at exec time}.
     */
    if (exitProcPtr->taskInfoPtr != NULL) {
	currHeapPages = (exitProcPtr->taskInfoPtr->vmInfo.heapInfoPtr == NULL
			 ? 0
			 : Vm_ByteToPage(exitProcPtr->taskInfoPtr->
					 vmInfo.heapInfoPtr->length));
	currStackPages = (exitProcPtr->taskInfoPtr->vmInfo.stackInfoPtr == NULL
			  ? 0
			  : Vm_ByteToPage(exitProcPtr->taskInfoPtr->
					  vmInfo.stackInfoPtr->length));
	vmStat.swapPagesWasted += min(currHeapPages,
				      exitProcPtr->taskInfoPtr->
				      vmInfo.execHeapPages);
	vmStat.swapPagesWasted += min(currStackPages,
				      exitProcPtr->taskInfoPtr->
				      vmInfo.execStackPages);
    }

    if (exitProcPtr->genFlags & PROC_USER) {
	ProcFreeTaskThread(Proc_AssertLocked(exitProcPtr));
    }

    /*
     * Native Sprite had to do the Fs_CloseState in two phases.  
     * Because in the Sprite server the VM code is divorced from 
     * specific processes, we can do it in one shot.
     */
    
    if (exitProcPtr->fsPtr != (struct Fs_ProcessState *) NIL) {
	Fs_CloseState(Proc_AssertLocked(exitProcPtr),0);
	Fs_CloseState(Proc_AssertLocked(exitProcPtr),1);
    }

    Proc_Unlock(Proc_AssertLocked(exitProcPtr));

    /*
     * Remove the process from its process family.  (Note that migrated
     * processes have family information on the home node.)  
     */
    if (!foreign) {
	ProcFamilyRemove(exitProcPtr);
    }
    
    Proc_SetState(exitProcPtr,
		  ExitProcessInt(exitProcPtr, foreign, thisProcess));
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_Reaper --
 *
 *	Cleans up the state information kept in the PCB for dead processes.
 *	Processes get put on the dead list after they exit and someone has 
 *	called Proc_Wait to wait for them. Detached processes
 *	are put on the dead list when they call Proc_Exit or Proc_ExitInt.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Virtual memory and Mach state for processes on the list is 
 *	deallocated.  The PCB is marked as unused.
 *
 *----------------------------------------------------------------------
 */

ENTRY void
Proc_Reaper()
{
    Proc_ControlBlock *procPtr;	/* a dead process */
    List_Links *itemPtr;

    LOCK_MONITOR;

    LIST_FORALL(deadProcessList, itemPtr) {
	List_Remove(itemPtr);
	procPtr = ((Proc_PCBLink *)itemPtr)->procPtr;
	Proc_Lock(procPtr);
	if (procPtr->genFlags & PROC_USER && procPtr->taskInfoPtr != NULL) {
	    panic("Proc_Reaper: process wasn't cleaned up properly.\n");
	}
	ProcFreePCB(Proc_AssertLocked(procPtr));
    }
    UNLOCK_MONITOR;
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_DetachInt --
 *
 *	The given process is detached from its parent.
 *
 * Results:
 *	SUCCESS			- always returned.
 *
 * Side effects:
 *	PROC_DETACHED flags set in the exitFlags field for the process.
 *
 *----------------------------------------------------------------------
 */

ENTRY void
Proc_DetachInt(procPtr)
    register	Proc_ControlBlock	*procPtr;
{
    Proc_ControlBlock 	*parentProcPtr;

    LOCK_MONITOR;

    /*
     * If the process is already detached, there's no point to do it again.
     * The process became detached by calling this routine or its parent
     * has died.
     */

    if (procPtr->exitFlags & PROC_DETACHED) {
	UNLOCK_MONITOR;
	return;
    }

    procPtr->exitFlags |= PROC_DETACHED;

    /*
     *  Wake up the parent in case it has called Proc_Wait to
     *  wait for this child (or any other children) to terminate.
     */

    parentProcPtr = Proc_GetPCB(procPtr->parentID);

    if (parentProcPtr->state == PROC_MIGRATED) {
	WakeupMigratedParent(parentProcPtr->processID);
    } else {
	Sync_Broadcast(&parentProcPtr->waitCondition);
    }
    /*
     * Signal the parent later on, when not holding the exit monitor
     * lock.
     */
    Proc_CallFunc(SendSigChild, (ClientData)parentProcPtr->processID,
		  time_ZeroSeconds);

    UNLOCK_MONITOR;
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_InformParent --
 *
 *	Tell the parent of the given process that the process has changed
 *	state.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Status bit set in the exit flags.
 *
 *----------------------------------------------------------------------
 */
ENTRY void
Proc_InformParent(procPtr, childStatus)
    register Proc_LockedPCB	*procPtr;	/* Process whose parent to
						 * inform of state change. */
    int				childStatus;	/* PROC_SUSPEND_STATUS |
						 * PROC_RESUME_STATUS */
{
    Proc_ControlBlock 	*parentProcPtr;
    Boolean foreign = FALSE;

    LOCK_MONITOR;

    /*
     * If the process is already detached, then there is no parent to tell.
     */
    if (procPtr->pcb.exitFlags & PROC_DETACHED) {
	UNLOCK_MONITOR;
	return;
    }

    /*
     * Wake up the parent in case it has called Proc_Wait to
     * wait for this child (or any other children) to terminate.  Also
     * clear the suspended and waited on flag.
     *
     * For a foreign process, just send a signal no matter what, since it
     * can go to an arbitrary node.  Also, do RPC's using a callback so
     * the monitor lock isn't held during the RPC.  
     */

    if (procPtr->pcb.genFlags & PROC_FOREIGN) {
	foreign = TRUE;
    }
    if (!foreign) {
	parentProcPtr = Proc_GetPCB(procPtr->pcb.parentID);
	Sync_Broadcast(&parentProcPtr->waitCondition);
    }
    Proc_CallFunc(SendSigChild, (ClientData)procPtr->pcb.parentID,
		  time_ZeroSeconds);
    procPtr->pcb.exitFlags &= ~PROC_STATUSES;
    procPtr->pcb.exitFlags |= childStatus;

    UNLOCK_MONITOR;
}


/*
 *----------------------------------------------------------------------
 *
 * SendSigChild --
 *
 *	Send a SIG_CHILD signal to the given process.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
static void
SendSigChild(data, callInfoPtr)
    ClientData		data;
    Proc_CallInfo	*callInfoPtr;	/* passed in by callback routine */
{
    (void)Sig_Send(SIG_CHILD, SIG_NO_CODE, (Proc_PID)data, FALSE, (Address)0);
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_Detach --
 *
 *	The current process is detached from its parent. Proc_DetachInt called
 *	to do most work.
 *
 * Results:
 *	SUCCESS			- always returned.
 *
 * Side effects:
 *	Statuses set in the proc table for the process.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Proc_Detach(status)
    int	status;		/* Detach status from caller. */
{
    register	Proc_ControlBlock 	*procPtr;

    procPtr = Proc_GetEffectiveProc();
    if (procPtr == (Proc_ControlBlock *) NIL) {
	panic("Proc_Detach: procPtr == NIL\n");
    }

    /*
     *  The following information is kept in case the parent does a
     *	Proc_Wait on this process.
     */
    
    Proc_Lock(procPtr);
    procPtr->termReason	= PROC_TERM_DETACHED;
    procPtr->termStatus	= status;
    procPtr->termCode	= 0;
    Proc_Unlock(Proc_AssertLocked(procPtr));

    Proc_DetachInt(procPtr);

    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_WaitStub --
 *
 *	MIG interface to Proc_Wait.
 *
 * Results:
 *	Returns KERN_SUCCESS.  Fills in the Sprite status code 
 *	from Proc_Wait and the "pending signals" flag.
 *
 * Side effects:
 *	See Proc_Wait.
 *
 *----------------------------------------------------------------------
 */

kern_return_t
Proc_WaitStub(serverPort, numPids, pidArray, flags, procIDPtr, reasonPtr, 
	      procStatusPtr, subStatusPtr, usageAddr, statusPtr,
	      sigPendingPtr)
    mach_port_t serverPort;	/* server request port */
    int numPids;		/* number of pids in pidArray */
    vm_address_t pidArray;	/* array of pids to look at (maybe nil) */
    int flags;
    Proc_PID *procIDPtr;	/* OUT: pid for the child that changed state */
    int *reasonPtr;		/* OUT: reason for state change */
    int *procStatusPtr;		/* OUT: status code for state change (e.g., 
				 *   Unix exit status) */
    int *subStatusPtr;		/* OUT: substatus code for state change */
    vm_address_t usageAddr;	/* OUT: resource usage info for child 
				 * (maybe nil, if info isn't desired) */
    ReturnStatus *statusPtr;	/* OUT: Sprite status code for the call 
				 *   (SUCCESS, etc.) */ 
    boolean_t *sigPendingPtr;	/* OUT: is there a signal pending */
{
#ifdef lint
    serverPort = serverPort;
#endif
    *statusPtr = Proc_Wait(numPids, (Proc_PID *)pidArray, flags, procIDPtr,
			   reasonPtr, procStatusPtr, subStatusPtr,
			   (Proc_ResUsage *)usageAddr);
    *sigPendingPtr = Sig_Pending(Proc_GetCurrentProc());
    return KERN_SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_Wait --
 *
 *	Returns information about a child process that has changed state to
 *	one of terminated, detached, suspended or running.  If the 
 *	PROC_WAIT_FOR_SUSPEND flag is not set then info is only returned about
 *	terminated and detached processes.  If the PROC_WAIT_BLOCK flag is
 *	set then this function will wait for a child process to change state.
 *
 *	A terminated process is a process that has ceased execution because
 *	it voluntarily called Proc_Exit or was involuntarily killed by 
 *	a signal or was destroyed by the kernel for some reason.
 *	A detached process is process that has called Proc_Detach to detach 
 *	itself from its parent. It continues to execute until it terminates.
 *
 * Results:
 *	PROC_INVALID_PID -	a process ID in the pidArray was invalid.
 *	SYS_INVALID_ARG -	the numPids argument specified a negative
 *				number of pids in pidArray, or numPids
 *				valid but pidArray was USER_NIL
 *	SYS_ARG_NOACCESS -	an out parameter was inaccessible or
 *				pidArray was inaccessible.
 *
 * Side effects:
 *	Processes may be put onto the dead list after they have been waited on.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Proc_Wait(numPids, pidArray, flags, procIDPtr, reasonPtr, 
	  statusPtr, subStatusPtr, usagePtr)
    int 		numPids;	/* Number of entries in pidArray.  
			 		 * 0 means wait for any child. */
    Proc_PID 		pidArray[]; 	/* Array of IDs of children to wait 
					 * for. (user address, possibly nil) */
    int 		flags;		/* PROC_WAIT_BLOCK => wait if no 
					 * children have exited, detached or
					 * suspended.  
					 * PROC_WAIT_FOR_SUSPEND => return 
					 * status of suspended children. */
    Proc_PID 		*procIDPtr; 	/* ID of the process that terminated. */
    int 		*reasonPtr;	/* Reason why the process exited. */
    int 		*statusPtr;	/* Exit status or termination signal 
					 * number.  */
    int 		*subStatusPtr;	/* Additional signal status if the 
					 * process died because of a signal. */
    Proc_ResUsage	*usagePtr;	/* Resource usage summary for the 
					 * process and its descendents.
					 * (user address, possibly nil) */

{
    register Proc_ControlBlock 	*curProcPtr;
    ReturnStatus		status;
    Proc_PID 			*newPidArray = (Proc_PID *) NIL;
    int 			newPidSize;
    ProcChildInfo		childInfo;
    Proc_ResUsage 		resUsage;
    Boolean			foreign = FALSE;

    curProcPtr = Proc_GetCurrentProc();
    if (curProcPtr == (Proc_ControlBlock *) NIL) {
	panic("Proc_Wait: curProcPtr == NIL.\n");
    }

    if (curProcPtr->genFlags & PROC_FOREIGN) {
	foreign = TRUE;
    }
    
    /*
     *  If a list of pids to check was given, use it, otherwise
     *  look for any child that has changed state.
     */
    if (numPids < 0) {
	return(SYS_INVALID_ARG);
    } else if (numPids > 0) {
	if (pidArray == USER_NIL) {
	    return(SYS_INVALID_ARG);
	} else {
	    /*
	     *  If pidArray is used, make it accessible. Also make sure that
	     *  the pids are in the proper range.
	     */
	    newPidSize = numPids * sizeof(Proc_PID);
	    newPidArray = (Proc_PID *) ckalloc((unsigned)newPidSize);
	    status = Vm_CopyIn(newPidSize, (Address) pidArray,
			       (Address) newPidArray);
	    if (status != SUCCESS) {
		ckfree((Address) newPidArray);
		return(SYS_ARG_NOACCESS);
	    }
	}
    }

    if (!foreign) {
	status = DoWait(curProcPtr, flags, numPids, newPidArray, &childInfo);
    } else {
	status = ProcRemoteWait(curProcPtr, flags, numPids, newPidArray,
				&childInfo);
    }

    if (numPids > 0) {
	ckfree((Address) newPidArray);
    }

    if (status == SUCCESS) {
	*procIDPtr = childInfo.processID;
	*reasonPtr = childInfo.termReason;
	*statusPtr = childInfo.termStatus;
	*subStatusPtr  = childInfo.termCode;
	if (usagePtr != USER_NIL) {
	    /*
	     * Convert the usages from the internal Timer_Ticks format
	     * into the external Time format.
	     */
	    Timer_TicksToTime(childInfo.kernelCpuUsage,
			      &resUsage.kernelCpuUsage);
	    Timer_TicksToTime(childInfo.userCpuUsage,
			      &resUsage.userCpuUsage);
	    Timer_TicksToTime(childInfo.childKernelCpuUsage, 
			      &resUsage.childKernelCpuUsage);
	    Timer_TicksToTime(childInfo.childUserCpuUsage,
			      &resUsage.childUserCpuUsage);
	    resUsage.numQuantumEnds = childInfo.numQuantumEnds;
	    resUsage.numWaitEvents = childInfo.numWaitEvents;
	    if (Vm_CopyOut(sizeof(Proc_ResUsage), (Address) &resUsage, 
			   (Address) usagePtr) != SUCCESS) {
		status = SYS_ARG_NOACCESS;
	    }
	}
    }

    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * DoWait --
 *
 *	Execute monitor level code for a Proc_Wait.  Return the information
 *	about the child that was found if any. 
 *
 * Results:
 *	PROC_INVALID_PID -	a process ID in the pidArray was invalid.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ENTRY ReturnStatus 
DoWait(curProcPtr, flags, numPids, newPidArray, childInfoPtr)
    register	Proc_ControlBlock	*curProcPtr;	/* Parent process. */
			/* PROC_WAIT_BLOCK => wait if no children have changed
			 * 		      state.
			 * PROC_WAIT_FOR_SUSPEND => return status of suspended
			 *			    children. */
    int 				flags;	
    int					numPids;	/* Number of pids in
							 * newPidArray. */
    Proc_PID 				*newPidArray;	/* Array of pids. */
    ProcChildInfo			*childInfoPtr;	/* Place to store child
							 * information. */
{
    ReturnStatus	status;

    LOCK_MONITOR;

    while (TRUE) {
	/*
	 * If a pid array was given, check the array to see if someone on it
	 * has changed state. Otherwise, see if any child has done so.
	 */
	status = FindExitingChild(curProcPtr, flags & PROC_WAIT_FOR_SUSPEND,
				numPids, newPidArray, childInfoPtr);

	/* 
	 * If someone was found or there was an error, break out of the loop
	 * because we are done. FAILURE means no one was found.
	 */
	if (status != FAILURE) {
	    break;
	}

	/*
	 *  If the search doesn't yields a child, we go to sleep waiting 
	 *  for a process to wake us up if it exits or detaches itself.
	 */
	if (!(flags & PROC_WAIT_BLOCK)) {
	    /*
	     * Didn't find anyone to report on.
	     */
	    status = PROC_NO_EXITS;
	    break;

	} else {	
	    /*
	     *  Since the current process is local, it will go to sleep
	     *  on its waitCondition.  A child will wakeup the process by
	     *  doing a wakeup on its parent's waitCondition.
	     *
	     *  This technique reduces the number of woken processes
	     *  compared to having every process wait on the same
	     *  event (e.g. exiting list address) when it goes to
	     *  sleep.  When we wake up, the search will start again.
	     *
	     * Check for the signal being SIG_CHILD, in which case
	     * don't abort the Proc_Wait.  This can happen if the parent and
	     * the child are on different hosts so the Sync_Wait is aborted
	     * by the signal rather than a wakeup.  (The parent should handle
	     * SIGCHLD better, but it might not, thereby missing the child's
	     * change in state.)
	     */
	    if (Sync_Wait(&curProcPtr->waitCondition, TRUE)) {
		if (Sig_Pending(curProcPtr) != (1 << (SIG_CHILD - 1))) {
		    status = GEN_ABORTED_BY_SIGNAL;
		    break;
		}
	    }
	}
    }

    UNLOCK_MONITOR;
    return(status);
}


/*
 * ----------------------------------------------------------------------------
 *
 * ProcRemoteWait --
 *
 *	Perform an RPC to do a Proc_Wait on the home node of the given
 *	process.  Transfer the information for a child that has changed state,
 *	if one exists, and set up the information for a remote
 *	wait if the process wishes to block waiting for a child.  Note
 *	that this routine is unsynchronized, since monitor locking is
 *	performed on the home machine of the process.
 *
 * Results:
 *	The status from the remote Proc_Wait (or RPC status) is returned.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

ReturnStatus
ProcRemoteWait(procPtr, flags, numPids, pidArray, childInfoPtr)
    Proc_ControlBlock	*procPtr;
    int			flags;
    int			numPids;
    Proc_PID		pidArray[];
    ProcChildInfo	*childInfoPtr;
{
#ifdef lint
    procPtr = procPtr;
    flags = flags;
    numPids = numPids;
    pidArray[0] = 42;
    childInfoPtr = childInfoPtr;
#endif
    panic("ProcRemoteWait called.\n");
    return FAILURE;		/* lint */
}


/*
 *----------------------------------------------------------------------
 *
 *  FindExitingChild --
 *
 *	Find a child of the specified process who has changed state,
 *	subject to possible constraints (a list of process
 *	IDs to check).  If a process is found, send that process to the
 *	reaper if appropriate.
 *
 *	If numPids is 0, look for any child, else look for specific
 *	processes.
 *
 * Results:
 *	PROC_NO_CHILDREN -	There are no children of this process left
 *				to be waited on.
 *	FAILURE -		didn't find any child of interest.
 *	SUCCESS -		got one.
 *
 * Side effects:
 *	If a process is found, *childInfoPtr is set to contain the relevant
 *	information from the child.
 *
 *----------------------------------------------------------------------
 */

INTERNAL static ReturnStatus 
FindExitingChild(parentProcPtr, returnSuspend, numPids, pidArray, infoPtr)
    Proc_ControlBlock 		*parentProcPtr;	/* Parent's PCB */
    Boolean			returnSuspend;	/* Return information about
						 * suspended or resumed
						 * children. */ 
    int 			numPids;	/* Number of Pids in pidArray */
    Proc_PID 			*pidArray;	/* Array of Pids to check */
    register ProcChildInfo	*infoPtr;	/* Place to return info */
{
    ReturnStatus status;
    Proc_LockedPCB *paramProcPtr;
    register Proc_ControlBlock *procPtr;
    
    if (numPids > 0) {
	status = CheckPidArray(parentProcPtr, returnSuspend, numPids, pidArray,
			       &paramProcPtr);
    } else {
	status = LookForAnyChild(parentProcPtr, returnSuspend, &paramProcPtr);
    }
    if (status == SUCCESS) {
	procPtr = (Proc_ControlBlock *)paramProcPtr;
	if (procPtr->state == PROC_EXITING ||
	    (procPtr->exitFlags & PROC_DETACHED)) {
	    List_Remove((List_Links *) &(procPtr->siblingElement));
	    infoPtr->termReason		= procPtr->termReason;
	    if (procPtr->state == PROC_EXITING) {
		/*
		 * Once an exiting process has been waited on it is moved
		 * from the exiting state to the dead state.
		 */
		ProcPutOnDeadList(procPtr);
	    } else {
		/*
		 * The child is detached and running.  Set a flag to make sure
		 * we don't find this process again in a future call to
		 * Proc_Wait.
		 */
		procPtr->exitFlags |= PROC_WAITED_ON;
	    }
	} else {
	    /*
	     * The child was suspended or resumed.
	     */
	    if (procPtr->exitFlags & PROC_SUSPEND_STATUS) {
		procPtr->exitFlags &= ~PROC_SUSPEND_STATUS;
		infoPtr->termReason = PROC_TERM_SUSPENDED;
	    } else if (procPtr->exitFlags & PROC_RESUME_STATUS) {
		procPtr->exitFlags &= ~PROC_RESUME_STATUS;
		infoPtr->termReason = PROC_TERM_RESUMED;
	    }
	}

	infoPtr->processID		= procPtr->processID;
	infoPtr->termStatus		= procPtr->termStatus;
	infoPtr->termCode		= procPtr->termCode;
#ifdef SPRITED_ACCOUNTING
	infoPtr->kernelCpuUsage		= procPtr->kernelCpuUsage.ticks;
	infoPtr->userCpuUsage		= procPtr->userCpuUsage.ticks;
	infoPtr->childKernelCpuUsage	= procPtr->childKernelCpuUsage.ticks;
	infoPtr->childUserCpuUsage 	= procPtr->childUserCpuUsage.ticks;
	infoPtr->numQuantumEnds		= procPtr->numQuantumEnds;
	infoPtr->numWaitEvents		= procPtr->numWaitEvents;
#else
	infoPtr->kernelCpuUsage		= time_ZeroSeconds;
	infoPtr->userCpuUsage		= time_ZeroSeconds;
	infoPtr->childKernelCpuUsage	= time_ZeroSeconds;
	infoPtr->childUserCpuUsage 	= time_ZeroSeconds;
	infoPtr->numQuantumEnds		= 0;
	infoPtr->numWaitEvents		= 0;
#endif /* SPRITED_ACCOUNTING */

	Proc_Unlock(Proc_AssertLocked(procPtr));
    }

    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 *  LookForAnyChild --
 *
 *	Search the process's list of children to see if any of 
 *	them have exited, become detached or been suspended or resumed.
 *	If no child is found, make sure there is a child who can wake us up.
 *
 * Results:
 *	PROC_NO_CHILDREN -	There are no children of this process left
 *				to be waited on.
 *	FAILURE -		didn't find any child of interest.
 *	SUCCESS -		got one.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

INTERNAL static ReturnStatus
LookForAnyChild(curProcPtr, returnSuspend, procPtrPtr)
    register Proc_ControlBlock	*curProcPtr;   /* Parent proc.*/
    Boolean			returnSuspend; /* Return info about
						* suspended children.*/
    Proc_LockedPCB 		**procPtrPtr;  /* Child proc. */
{
    register Proc_ControlBlock *procPtr;
    register Proc_PCBLink *procLinkPtr;
    Boolean foundValidChild = FALSE;

    /*
     *  Loop through the list of children, looking for the first child
     *  to have changed state. Ignore children that are detached
     *  and waited-on.
     */

    LIST_FORALL((List_Links *) curProcPtr->childList,
		(List_Links *) procLinkPtr) {
        procPtr = procLinkPtr->procPtr;
	/*
	 *  It may be that one of our children is in the process of exiting.
	 *  If it is marked as 'dying' but not 'exiting', then it has
	 *  left the monitor (obvious because we're in the monitor now)
	 *  but hasn't completed the context switch to the 'exiting' state.
	 *  We'll wait for the child to become exiting since it will
	 *  take at most the length of a context switch to finish.  If
	 *  we don't wait for this child we will miss the transition
	 *  and potentially wait forever.
	 */
	if (procPtr->genFlags & PROC_DYING) {
	    while (procPtr->state != PROC_EXITING) {
		/*
		 * Wait for the other processor to set the state to exiting.
		 */
	    }
	}
	if ((procPtr->state == PROC_EXITING) ||
	    (procPtr->exitFlags & PROC_DETACHED) ||
	    (returnSuspend && (procPtr->exitFlags & PROC_STATUSES))) {
	    if (!(procPtr->exitFlags & PROC_WAITED_ON)) {
		Proc_Lock(procPtr);
	        *procPtrPtr = Proc_AssertLocked(procPtr);
	        return(SUCCESS);
	    }
	} else {
	    foundValidChild = TRUE;
	}
    }

    if (foundValidChild) {
	return(FAILURE);
    }
    return(PROC_NO_CHILDREN);
}


/*
 *----------------------------------------------------------------------
 *
 *  CheckPidArray --
 *
 *	Search the process's array of children to see if any of them 
 *	have exited, become detached or been suspended or resumed.
 *
 * Results:
 *	FAILURE -		didn't find any child of interest.
 *	PROC_INVALID_PID -	a pid in the array was invalid.
 *	SUCCESS -		got one.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */


INTERNAL static ReturnStatus
CheckPidArray(curProcPtr, returnSuspend, numPids,  pidArray, procPtrPtr)
    register	Proc_ControlBlock	*curProcPtr;	/* Parent proc. */
    Boolean				returnSuspend;	/* Return information
							 * about suspended or
							 * resumed children. */
    int					numPids;	/* Number of pids in 
							 * pidArray. */
    Proc_PID				*pidArray;	/* Array of pids to 
							 * check. */
    Proc_LockedPCB 			**procPtrPtr;	/* Child proc. */
{
    register Proc_LockedPCB	*procPtr;
    int				i;

    /*
     * The user has specified a list of processes to wait for.
     * If a specified process is non-existent or is not a child of the
     * calling process return an error status.
     */
    for (i=0; i < numPids; i++) {
	procPtr = Proc_LockPID(pidArray[i]);
	if (procPtr == NULL) {
	    return(PROC_INVALID_PID);
	}
	if (!Proc_ComparePIDs(procPtr->pcb.parentID,
			      curProcPtr->processID)) {
	    Proc_Unlock(procPtr);
	    return(PROC_INVALID_PID);
	}
	if ((procPtr->pcb.state == PROC_EXITING) ||
	    (procPtr->pcb.exitFlags & PROC_DETACHED) ||
	    (returnSuspend && (procPtr->pcb.exitFlags & PROC_STATUSES))) {
	    if (!(procPtr->pcb.exitFlags & PROC_WAITED_ON)) {
		*procPtrPtr = procPtr;
		return(SUCCESS);
	    }
	}
	Proc_Unlock(procPtr);
    } 
    return(FAILURE);
}


/*
 *----------------------------------------------------------------------
 *
 * WakeupMigratedParent --
 *
 *	Call the function Proc_NotifyMigratedWaiters by starting a process
 *	on it.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static INTERNAL void
WakeupMigratedParent(pid)
    Proc_PID pid;
{
#ifdef lint
    pid = pid;
#endif
    panic("WakeupMigratedParent called.\n");
}


/*
 *----------------------------------------------------------------------
 *
 * ProcPutOnDeadList --
 *
 *	Put the given process on the dead list.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The process state is set to "dead", and we arrange for the 
 *	process to get reaped.
 *
 *----------------------------------------------------------------------
 */

static INTERNAL void
ProcPutOnDeadList(procPtr)
    Proc_ControlBlock *procPtr;
{
    Proc_SetState(procPtr, PROC_DEAD);
    List_Insert((List_Links *)&procPtr->deadElement,
		LIST_ATREAR(deadProcessList));
}


/*
 *----------------------------------------------------------------------
 *
 * UpdateChildCpuUsage --
 *
 *	Update the "child" CPU usage information for a waiting parent.  
 *	This is an internal proc, because the monitor lock protects updates
 *	to the parent's accounting information.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The CPU usage and child CPU usage for the exiting process are 
 *	added to the child CPU usage for the parent process.
 *
 *----------------------------------------------------------------------
 */

static INTERNAL void
UpdateChildCpuUsage(parentProcPtr, exitProcPtr)
    Proc_ControlBlock *parentProcPtr;
    Proc_ControlBlock *exitProcPtr;
{
    /* XXX not implemented */
#ifdef lint
    parentProcPtr = parentProcPtr;
    exitProcPtr = exitProcPtr;
#endif
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_Kill --
 *
 *	Routine to destroy a process, usually asynchronously.
 *	
 *	User processes are stopped, so that if they're in an infinite
 *	loop they stop consuming cycles.  The process is marked so that we 
 *	will not accept any more requests from it.  System processes and
 *	user processes that have pending requests are woken up and allowed
 *	to continue running until they reach a convenient place to quit.
 *	User processes that don't have a pending request die now.
 *	
 *	Note: it might be useful to call this routine directly in some 
 *	places, so that a specific termination reason (i.e., something more 
 *	meaningful than "the process got a SIG_KILL") is recorded in the 
 *	PCB.
 *	
 *	XXX Are there any other things that Proc_ExitInt does that we 
 *	should do here?
 *	
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
Proc_Kill(procPtr, reason, status)
    Proc_LockedPCB *procPtr;	/* the process to kill */
    int reason;			/* signaled or destroyed */
    int status;			/* signal number or destroy status */
{
    /*
     * If the process is already near death, leave it alone.
     */
    if (procPtr->pcb.genFlags & PROC_DYING) {
	return;
    }

    /* 
     * Clean up after migrated or profiled processes.
     * XXX Need to think some more about whether this will all do the right 
     * thing.  For example, is it okay to call ProcRemoteExit with a locked
     * PCB other than the current PCB?
     */
#ifdef SPRITED_MIGRATION
    if (procPtr->genFlags & PROC_FOREIGN) {
	ProcRemoteExit(procPtr, reason, status, 0);
    }
#endif
#ifdef SPRITED_PROFILING
    if (procPtr->Prof_Scale != 0) {
	Prof_Disable(procPtr);
    }
#endif

    /*
     * In general, we can only nuke the thread.  The task information
     * should be left around until the process exits, in case the process
     * has a request in the server.  This is because various routines
     * (e.g., Vm_Copy{In,Out}, ProcMakeTaskThread) access the task
     * information without locking the pcb.
     */
    if (procPtr->pcb.genFlags & PROC_USER) {
	ProcKillThread(procPtr);
    }

    /* 
     * If the process is already on its way out, this is all we need to do. 
     * ProcExitProcess isn't idempotent, so avoid calling it multiple times 
     * for the same process.
     */
    if (procPtr->pcb.genFlags & PROC_NO_MORE_REQUESTS) {
	return;
    }

    procPtr->pcb.genFlags	|= PROC_NO_MORE_REQUESTS;
    procPtr->pcb.termReason	= reason;
    procPtr->pcb.termStatus	= status;
    procPtr->pcb.termCode	= 0;

    /* 
     * If it's a user process that has no pending request, force it to exit 
     * now.  Otherwise, we just marked it dead, so let the cleanup happen 
     * later.
     */
    if ((procPtr->pcb.genFlags & (PROC_KERNEL | PROC_BEING_SERVED)) == 0) {
	if ((Proc_ControlBlock *)procPtr == Proc_GetCurrentProc()) {
	    panic("Proc_Kill: process in server but not marked.\n");
	}
	ProcExitProcess((Proc_ControlBlock *)procPtr, reason, status, 0,
			FALSE);
    } else {
	Sync_WakeWaitingProcess(procPtr);
    }
}
