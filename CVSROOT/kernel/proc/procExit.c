/*
 * procExit.c --
 *
 *	Routines to terminate and detach processes, and to cause a 
 *	process to wait for the termination of other processes.  This file
 *	maintains a monitor to synchronize between exiting, detaching, and
 *	waiting processes.  The monitor also synchronizes access to the
 *	dead list.
 *
 * Copyright 1986, 1988 Regents of the University of California
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
 *	       in the genFlags field.  Also PROC_NO_VM flag set before
 *	       freeing VM segments.
 *	    3) The only way a process can go to the exiting and dead states 
 *	       is through using routines in this file.
 *
 *	A process can be in one of several states:
 *
 *	PROC_READY:	It is ready to execute when a CPU becomes available.
 *			The PCB is attached to the Ready list.
 *	PROC_RUNNING:	It is currently executing on a CPU.
 *			The PCB is in the list of running processes.
 *	PROC_WAITING:	It is waiting for an event to occur. 
 *			The PCB is attached to an event-specific list of 
 *			waiting processes.
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
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif /* not lint */

#include <sprite.h>
#include <mach.h>
#include <status.h>
#include <proc.h>
#include <procInt.h>
#include <procMigrate.h>
#include <migrate.h>
#include <sync.h>
#include <sched.h>
#include <list.h>
#include <sys.h>
#include <vm.h>
#include <prof.h>
#include <dbg.h>
#include <stdlib.h>
#include <rpc.h>
#include <sig.h>
#include <stdio.h>
#include <vmMach.h>
#include <recov.h>

static	Sync_Lock	exitLock = Sync_LockInitStatic("Proc:exitLock"); 
#define	LOCKPTR &exitLock

static 	INTERNAL ReturnStatus FindExitingChild _ARGS_((
				    Proc_ControlBlock *parentProcPtr,
				    Boolean returnSuspend, int numPids,
				    Proc_PID *pidArray, 
				    ProcChildInfo *infoPtr));
static 	INTERNAL void WakeupMigratedParent _ARGS_((Proc_PID pid));
static 	void 	SendSigChild _ARGS_((ClientData data, 
			Proc_CallInfo *callInfoPtr));
static 	Proc_State	ExitProcessInt _ARGS_((
				Proc_ControlBlock *exitProcPtr,
				Boolean	migrated, Boolean contextSwitch));

/*
 * Shared memory.
 */
extern int vmShmDebug;
#ifndef lint
#define dprintf if (vmShmDebug) printf
#else /* lint */
#define dprintf printf
#endif /* lint */

/*
 * SIGNAL_PARENT
 *
 * Macro to send a SIG_CHILD message to the parent.
 */
#define SIGNAL_PARENT(parentProcPtr, funcName) \
    if ((parentProcPtr)->genFlags & PROC_USER) { \
	ReturnStatus	status; \
	Proc_Lock(parentProcPtr); \
	status = Sig_SendProc(parentProcPtr, SIG_CHILD, 0, (Address)0); \
	Proc_Unlock(parentProcPtr); \
	if (status != SUCCESS) { \
	    printf("Warning: %s: Could not signal parent, status<%x>", \
				funcName, status); \
	} \
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
 *	ProcExitProcess to do the work. This routine does NOT return because
 *	a context switch is performed.  If the process is foreign,
 *	ProcRemoteExit is called to handle cleanup on the home node
 *	of the process, then ProcExitProcess is called.
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

    if (curProcPtr->genFlags & PROC_FOREIGN) {
	ProcRemoteExit(curProcPtr, reason, status, code);
    }
    if (curProcPtr->Prof_Scale != 0) {
	Prof_Disable(curProcPtr);
    }
    if (curProcPtr->genFlags & PROC_DEBUGGED) {
	/*
	 * If a process is being debugged then force it onto the debug
	 * list before allowing it to exit.
	 */
	Proc_SuspendProcess(curProcPtr, TRUE, reason, status, code);
    }

    if (sys_ErrorShutdown) {
	/*
	 * Are shutting down the system because of an error.  In this case
	 * don't close anything down because we want to leave as much state
	 * around as possible.
	 */
	Sched_ContextSwitch(PROC_DEAD);
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
 *	None.
 *
 *----------------------------------------------------------------------
 */

ENTRY static Proc_State
ExitProcessInt(exitProcPtr, migrated, contextSwitch) 
    register Proc_ControlBlock	*exitProcPtr;	/* The exiting process. */
    Boolean			migrated;	/* TRUE => foreign process. */
    Boolean			contextSwitch;	/* TRUE => context switch. */
{
    register	Proc_ControlBlock	*procPtr;
    Proc_State				newState = PROC_UNUSED;
    register Proc_PCBLink 		*procLinkPtr;
    Timer_Ticks 		        ticks;

    LOCK_MONITOR;


    Proc_Lock(exitProcPtr);

    exitProcPtr->genFlags |= PROC_DYING;

    if (exitProcPtr->genFlags & PROC_MIG_PENDING) {
	/*
	 * Someone is waiting for this guy to migrate.  Let them know that
	 * the process is dying.
	 */
	exitProcPtr->genFlags &= ~PROC_MIG_PENDING;
	ProcMigWakeupWaiters();
    }

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
		Timer_AddTicks(exitProcPtr->kernelCpuUsage.ticks, 
			       parentProcPtr->childKernelCpuUsage.ticks, 
			       &(parentProcPtr->childKernelCpuUsage.ticks));
		Timer_AddTicks(exitProcPtr->childKernelCpuUsage.ticks, 
			       parentProcPtr->childKernelCpuUsage.ticks, 
			       &(parentProcPtr->childKernelCpuUsage.ticks));
		Timer_AddTicks(exitProcPtr->userCpuUsage.ticks, 
			       parentProcPtr->childUserCpuUsage.ticks, 
			       &(parentProcPtr->childUserCpuUsage.ticks));
		Timer_AddTicks(exitProcPtr->childUserCpuUsage.ticks, 
			       parentProcPtr->childUserCpuUsage.ticks, 
			       &(parentProcPtr->childUserCpuUsage.ticks));
	    }
	}
#ifndef CLEAN
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
#endif /* CLEAN */
    }

    
    /*
     * Make sure there are no lingering interval timer callbacks associated
     * with this process.
     *
     *  Go through the list of children of the current process to 
     *  make them orphans. When the children exit, this will typically
     *  cause them to go on the dead list directly.
     */

    if (!migrated) {
	ProcDeleteTimers(exitProcPtr);
	while (!List_IsEmpty(exitProcPtr->childList)) {
	    procLinkPtr = (Proc_PCBLink *) List_First(exitProcPtr->childList);
	    procPtr = procLinkPtr->procPtr;
	    List_Remove((List_Links *) procLinkPtr);
	    if (procPtr->state == PROC_EXITING) {
		/*
		 * The child is exiting waiting for us to wait for it.
		 */
		procPtr->state = PROC_DEAD;
		Proc_CallFunc(Proc_Reaper, (ClientData) procPtr, 0);
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

    /*
     * If the process is still waiting on an event, this is an error.
     * [For now, flag this error only for foreign processes in case this
     * isn't really an error after all.]
     */
    if (migrated && (exitProcPtr->event != NIL)) {
	if (proc_MigDebugLevel > 0) {
	    panic(
		"ExitProcessInt: exiting process still waiting on event %x.\n",
		exitProcPtr->event);
	} else {
	    printf(
	      "%s ExitProcessInt: exiting process still waiting on event %x.\n",
	      "Warning:", exitProcPtr->event);
	}
    }

    /*
     * If the current process is detached and waited on (i.e. an orphan) then
     * one of two things happen.  If the process is a family head and its list
     * of family members is not empty then the process is put onto the exiting
     * list.  Otherwise the process is put onto the dead list since its 
     * parent has already waited for it.
     */

    if (((exitProcPtr->exitFlags & PROC_DETACHED) &&
        (exitProcPtr->exitFlags & PROC_WAITED_ON)) || migrated) {
	newState = PROC_DEAD;
	Proc_CallFunc(Proc_Reaper,  (ClientData) exitProcPtr, 0);
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
#ifdef notdef
	    SIGNAL_PARENT(parentProcPtr, "ExitProcessInt");
#endif
	} else {
	    WakeupMigratedParent(parentProcPtr->processID);
	}
	/*
	 * Signal the parent later on, when not holding the exit monitor
	 * lock.
	 */
	Proc_CallFunc(SendSigChild, (ClientData)exitProcPtr->parentID, 0);


	newState = PROC_EXITING;
    }
done:
    if (contextSwitch) {
	Proc_Unlock(exitProcPtr);
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
 * Results:
 *	None.
 *
 * Side effects:
 *	The PCB entry for the process is modified to clean-up FS
 *	state. The specified process may be placed on the dead list.
 *  	If thisProcess is TRUE, a context switch is performed.  If not,
 *	then the exit is being performed on behalf of another process;
 * 	the procPtr comes in locked and is unlocked as a side effect.
 *
 *----------------------------------------------------------------------
 */
void
ProcExitProcess(exitProcPtr, reason, status, code, thisProcess) 
    register Proc_ControlBlock 	*exitProcPtr;	/* Exiting process. */
    int 			reason;		/* Why the process is dieing: 
						 * EXITED, SIGNALED, 
						 * DESTROYED  */
    int				status;		/* Exit status, signal # or 
						 * destroy status. */
    int 			code;		/* Signal sub-status */
    Boolean 			thisProcess;	/* TRUE => context switch */
{
    register Boolean 		migrated;
    Boolean			noVm;

    migrated = (exitProcPtr->genFlags & PROC_FOREIGN);

    /*
     * Decrement the reference count on the environment.
     */

    if (!migrated && exitProcPtr->environPtr != (Proc_EnvironInfo *) NIL) {
	ProcDecEnvironRefCount(exitProcPtr->environPtr);
    }

    /*
     * The process is already locked if it comes in on behalf of someone
     * else.
     */
    if (thisProcess) {
	Proc_Lock(exitProcPtr);
    }
    if (exitProcPtr->genFlags & PROC_NO_VM) {
	noVm = TRUE;
    } else {
	noVm = FALSE;
	exitProcPtr->genFlags |= PROC_NO_VM;
    }
    Proc_Unlock(exitProcPtr);

    /*
     * We have to do the Fs_CloseState in two phases:
     *
     * We must close pseudo-devices before destroying vm.
     * Otherwise there is a race (hit by migd) when a pseudo-device
     * is destroyed because data may be sent to the pseudo-device after
     * it loses its vm but before it is removed from the file system.
     * We will then crash because it has no vm for the data.
     *
     * We need to close swap files after deleting all the VM segments.
     * Otherwise we die if a COW segment has swapped-out data.  The
     * problem is we get the data from swap when we delete the segment.
     * Thus we must delete the segment before we close down the swap files.
     * --Ken Shirriff 10/90
     */

    if (exitProcPtr->fsPtr != (struct Fs_ProcessState *) NIL) {
	Fs_CloseState(exitProcPtr,0);
    }

    /*
     * Free up virtual memory resources, unless they were already freed.
     */

#ifdef sun4
    Mach_FlushWindowsToStack();
    VmMach_FlushCurrentContext();
#endif
    if ((exitProcPtr->genFlags & PROC_USER) && !noVm) {
	int i=0;
	while (exitProcPtr->vmPtr->sharedSegs != (List_Links *)NIL) {

	    if (exitProcPtr->vmPtr->sharedSegs == (List_Links *)NULL) {
		dprintf("ProcExitProcess: warning: sharedSegs == NULL\n");
		break;
	    }
  	    i++;
  	    if (i>20) {
  		dprintf("ProcExitProcess: procExit: segment loop!\n");
  		break;
  	    }
  	    if (exitProcPtr->vmPtr->sharedSegs==(List_Links *)NULL) {
  		printf("ProcExitProcess: Danger: null sharedSegs list\n");
  		break;
  	    }
  	    if (List_IsEmpty(exitProcPtr->vmPtr->sharedSegs)) {
  		printf("ProcExitProcess: Danger: empty sharedSegs list\n");
  		break;
  	    }
  	    if (List_First(exitProcPtr->vmPtr->sharedSegs)==
  		    (List_Links *)NULL) {
  		break;
  	    }
  	    Vm_DeleteSharedSegment(exitProcPtr, (Vm_SegProcList *)
		    List_First(exitProcPtr->vmPtr->sharedSegs));
  	}
	for (i = VM_CODE; i <= VM_STACK; i++) {
	    Vm_SegmentDelete(exitProcPtr->vmPtr->segPtrArray[i], exitProcPtr);
	    exitProcPtr->vmPtr->segPtrArray[i] = (Vm_Segment *)NIL;
	}
    }

    if (exitProcPtr->fsPtr != (struct Fs_ProcessState *) NIL) {
	Fs_CloseState(exitProcPtr,1);
    }

    /*
     * Remove the process from its process family.  (Note, 
     * migrated processes have family information on the home node.)
     */
    if (!migrated) {
	ProcFamilyRemove(exitProcPtr);
    }

    /*
     *  The following information is kept in case the parent
     *	calls Proc_Wait to wait for this process to terminate.
     */

    exitProcPtr->termReason	= reason;
    exitProcPtr->termStatus	= status;
    exitProcPtr->termCode	= code;

    exitProcPtr->state = ExitProcessInt(exitProcPtr, migrated, thisProcess);

    Proc_Unlock(exitProcPtr);
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_Reaper --
 *
 *	Cleans up the state information kept in the PCB for a dead process.
 *	Processes get put on the dead list after they exit and someone has 
 *	called Proc_Wait to wait for them. Detached processes
 *	are put on the dead list when they call Proc_Exit or Proc_ExitInt.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Virtual memory state for processes on the list is deallocated.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
ENTRY void
Proc_Reaper(data, callInfoPtr)
    ClientData				data;		/* procPtr */
    Proc_CallInfo			*callInfoPtr;
{
    register	Proc_ControlBlock 	*procPtr = (Proc_ControlBlock *) data;
    LOCK_MONITOR;
    /*
     * On a multiprocess there are two cases where we can't reap the process
     * right away.  1 - the dying process may not have context switched
     * into the DEAD state.  2 - the dying process's kernel stack may
     * be used by a processor in the IdleLoop().  In either of these
     * cases we reschedule ourselves for a second later.
     */
    if ((procPtr->state != PROC_DEAD) ||
	(procPtr->schedFlags & SCHED_STACK_IN_USE)) {
	callInfoPtr->interval = timer_IntOneSecond;
	UNLOCK_MONITOR;
	return;
    } else {
	callInfoPtr->interval = 0;
    }
#ifdef notdef
    /*
     * Next wait for the process's stack to become free.  On a multiprocessor
     * a DEAD processes stack may be used to field interrupt by a processor
     * until another process becomes ready.
     */
    while (procPtr->schedFlags & SCHED_STACK_IN_USE) {
	UNLOCK_MONITOR;
	Sync_WaitTime(time_OneSecond);
	LOCK_MONITOR;
    }
    /*
     * Since the SCHED_STACK_IN_USE is removed from a process before the
     * context switch from that processor occurs we need to syncronize 
     * with the scheduler.  Context switching on to the READY queue will
     * cause such a syncronization.
     */
    Sched_ContextSwitch(PROC_READY);
#endif
    /*
     * At this point a migrated process is not in the PROC_MIGRATED
     * state since it's been moved to the PROC_DEAD state.  
     * Migrated processes don't have a local machine-dependent state
     * hanging off them.  They also don't have a current context, but
     * VmMach_FreeContext can handle that.
     */
    if (procPtr->machStatePtr != (Mach_State *) NIL) {
	Mach_FreeState(procPtr);
    }
    VmMach_FreeContext(procPtr);

    ProcFreePCB(procPtr);

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
#ifdef notdef
	SIGNAL_PARENT(parentProcPtr, "Proc_DetachInt");
#endif
    }
    /*
     * Signal the parent later on, when not holding the exit monitor
     * lock.
     */
    Proc_CallFunc(SendSigChild, (ClientData)parentProcPtr->processID, 0);

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
    register Proc_ControlBlock	*procPtr;	/* Process whose parent to
						 * inform of state change. */
    int				childStatus;	/* PROC_SUSPEND_STATUS |
						 * PROC_RESUME_STATUS */
{
    Proc_ControlBlock 	*parentProcPtr;
    Boolean migrated = FALSE;

    LOCK_MONITOR;

    /*
     * If the process is already detached, then there is no parent to tell.
     */
    if (procPtr->exitFlags & PROC_DETACHED) {
	UNLOCK_MONITOR;
	return;
    }

    /*
     * Wake up the parent in case it has called Proc_Wait to
     * wait for this child (or any other children) to terminate.  Also
     * clear the suspended and waited on flag.
     *
     * For a migrated process, just send a signal no matter what, since it
     * can go to an arbitrary node.  Also, do RPC's using a callback so
     * the monitor lock isn't held during the RPC.  
     */

    if (procPtr->genFlags & PROC_FOREIGN) {
	migrated = TRUE;
    }
    if (!migrated) {
	parentProcPtr = Proc_GetPCB(procPtr->parentID);
	Sync_Broadcast(&parentProcPtr->waitCondition);
    }
    Proc_CallFunc(SendSigChild, (ClientData)procPtr->parentID, 0);
    procPtr->exitFlags &= ~PROC_STATUSES;
    procPtr->exitFlags |= childStatus;

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

    procPtr->termReason	= PROC_TERM_DETACHED;
    procPtr->termStatus	= status;
    procPtr->termCode	= 0;

    Proc_DetachInt(procPtr);

    return(SUCCESS);
}

static ReturnStatus 	CheckPidArray _ARGS_((Proc_ControlBlock *curProcPtr,
				Boolean returnSuspend, int numPids,
				Proc_PID *pidArray, 
				Proc_ControlBlock **procPtrPtr));
static ReturnStatus 	LookForAnyChild _ARGS_((Proc_ControlBlock *curProcPtr,
				Boolean returnSuspend, 
				Proc_ControlBlock **procPtrPtr));
extern ReturnStatus 	DoWait _ARGS_((Proc_ControlBlock *curProcPtr,
			    int	flags, int numPids, Proc_PID *newPidArray,
			    ProcChildInfo *childInfoPtr));


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
 *	a signal or was destroyed by the kernel due to an invalid stack. 
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
					 * for. */
    int 		flags;		/* PROC_WAIT_BLOCK => wait if no 
					 * children have exited, detached or
					 * suspended.  
					 * PROC_WAIT_FOR_SUSPEND => return 
					 * status of suspended children. */

				/* The following parameters may be USER_NIL. */
    Proc_PID 		*procIDPtr; 	/* ID of the process that terminated. */
    int 		*reasonPtr;	/* Reason why the process exited. */
    int 		*statusPtr;	/* Exit status or termination signal 
					 * number.  */
    int 		*subStatusPtr;	/* Additional signal status if the 
					 * process died because of a signal. */
    Proc_ResUsage	*usagePtr;	/* Resource usage summary for the 
					 * process and its descendents. */

{
    register Proc_ControlBlock 	*curProcPtr;
    ReturnStatus		status;
    Proc_PID 			*newPidArray = (Proc_PID *) NIL;
    int 			newPidSize;
    ProcChildInfo		childInfo;
    Proc_ResUsage 		resUsage;
    Boolean			migrated = FALSE;

    curProcPtr = Proc_GetCurrentProc();
    if (curProcPtr == (Proc_ControlBlock *) NIL) {
	panic("Proc_Wait: curProcPtr == NIL.\n");
    }

    if (curProcPtr->genFlags & PROC_FOREIGN) {
	migrated = TRUE;
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
	    newPidArray = (Proc_PID *) malloc(newPidSize);
	    status = Vm_CopyIn(newPidSize, (Address) pidArray,
			       (Address) newPidArray);
	    if (status != SUCCESS) {
		free((Address) newPidArray);
		return(SYS_ARG_NOACCESS);
	    }
	}
    }

    if (!migrated) {
	status = DoWait(curProcPtr, flags, numPids, newPidArray, &childInfo);
    } else {
	status = ProcRemoteWait(curProcPtr, flags, numPids, newPidArray,
				&childInfo);
    }

    if (numPids > 0) {
	free((Address) newPidArray);
    }

    if (status == SUCCESS) {
	if (procIDPtr != USER_NIL) {
	    if (Vm_CopyOut(sizeof(Proc_PID), (Address) &childInfo.processID, 
		(Address) procIDPtr) != SUCCESS) {
		status = SYS_ARG_NOACCESS;
	    }
	}
	if (reasonPtr != USER_NIL) {
	    if (Vm_CopyOut(sizeof(int), (Address) &childInfo.termReason, 
		(Address) reasonPtr) != SUCCESS) {
		status = SYS_ARG_NOACCESS;
	    }
	}
	if (statusPtr != USER_NIL) {
	    if (Vm_CopyOut(sizeof(int), (Address) &childInfo.termStatus, 
		(Address) statusPtr) != SUCCESS) {
		status = SYS_ARG_NOACCESS;
	    }
	}
	if (subStatusPtr != USER_NIL) {
	    if (Vm_CopyOut(sizeof(int), (Address) &childInfo.termCode, 
		(Address) subStatusPtr) != SUCCESS) {
		status = SYS_ARG_NOACCESS;
	    }
	}
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
    ProcRemoteWaitCmd cmd;
    Rpc_Storage storage;
    ReturnStatus status;
    int numTries;

    if (proc_MigDebugLevel > 3) {
	printf("ProcRemoteWait(%x, ...) called.\n", procPtr->processID);
    }

    /*
     * Check to make sure the home node is up, and kill the process if
     * it isn't.  The call to exit never returns.
     */
    status = Recov_IsHostDown(procPtr->peerHostID);
    if (status != SUCCESS) {
	if (proc_MigDebugLevel > 0) {
	    printf("Proc_DoRemoteCall: host %d is down; killing process %x.\n",
		       procPtr->peerHostID, procPtr->processID);
	}
	Proc_ExitInt(PROC_TERM_DESTROYED, (int) PROC_NO_PEER, 0);
	/*
	 * This point should not be reached, but the N-O-T-R-E-A-C-H-E-D
	 * directive causes a complaint when there's code after it.
	 */
	panic("ProcRemoteWait: Proc_ExitInt returned.\n");
	return(PROC_NO_PEER);
    }
    /*
     * Set up the invariant fields of the rpc call, since we may make
     * multiple calls after waiting.
     */
    
    cmd.pid = procPtr->peerProcessID;
    cmd.numPids = numPids;
    cmd.flags = flags;
    cmd.token = NIL;

    storage.requestParamPtr = (Address) &cmd;
    storage.requestParamSize = sizeof(ProcRemoteWaitCmd);
    storage.requestDataPtr = (Address) pidArray;
    storage.requestDataSize = numPids * sizeof(Proc_PID);

    while (TRUE) {

	storage.replyParamPtr = (Address) NIL;
	storage.replyParamSize = 0;
	storage.replyDataPtr = (Address) childInfoPtr;
	storage.replyDataSize = sizeof(ProcChildInfo);

	if (flags & PROC_WAIT_BLOCK) {
	    Sync_GetWaitToken((Proc_PID *) NIL, &cmd.token);
	}

	/*
	 * Set up for the RPC.
	 */
    
	for (numTries = 0; numTries < PROC_MAX_RPC_RETRIES; numTries++) {
	    status = Rpc_Call(procPtr->peerHostID, RPC_PROC_REMOTE_WAIT,
			      &storage);
	    if (status != RPC_TIMEOUT) {
		break;
	    }
	    status = Proc_WaitForHost(procPtr->peerHostID);
	    if (status != SUCCESS) {
		break;
	    }
	}

	/*
	 * If the status is FAILURE, no children have exited so far.  In
	 * this case, we may want to sleep.
	 */
	if (status != FAILURE) {
	    break;
	}

	/*
	 * If the search doesn't yield a child and we are supposed to block,
	 * we go to sleep waiting for a process to wake us up if it
	 * exits or detaches itself.
	 */

	if (!(flags & PROC_WAIT_BLOCK)) {
	    /*
	     * Didn't find anyone to report on.
	     */
	    status = PROC_NO_EXITS;
	    break;

	} else if (Sync_ProcWait((Sync_Lock *) NIL, TRUE)) {
	    status = GEN_ABORTED_BY_SIGNAL;
	    break;
	}  
    }

    if (status == PROC_NO_PEER) {
	(void) Sig_Send(SIG_KILL, (int) PROC_NO_PEER, procPtr->processID,
			FALSE, (Address)0); 
	if (proc_MigDebugLevel > 1) {
	    printf("ProcRemoteWait killing process %x: home node's copy died.\n",
		   procPtr->processID);
	}
    } else if (proc_MigDebugLevel > 3) {
	printf("ProcRemoteWait returning status %x.\n", status);
	if (status == SUCCESS && proc_MigDebugLevel > 6) {
	    printf("Child's id is %x, status %x.\n",
		       childInfoPtr->processID, childInfoPtr->termStatus);
	}
    }
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * ProcServiceRemoteWait --
 *
 *	Services the Proc_Wait command for a migrated process.  If
 *	there is an appropriate child, returns information about it (refer
 *	to Proc_Wait).  If not, returns	PROC_NO_EXITS.  If the migrated
 *	process specified that it should block, set up a remote wakeup
 *	for when a child changes state.
 *
 * Results:
 *	PROC_INVALID_PID -	a process ID in the pidArray is invalid.
 *	PROC_NO_EXITS    -	no children have changed state which have
 *				not already been waited upon.
 *
 * Side effects:
 *	Processes on the exiting list may be put on the dead list after 
 *	they have been waited on.  A remote wakeup may be established.
 *
 *----------------------------------------------------------------------
 */

ENTRY ReturnStatus
ProcServiceRemoteWait(curProcPtr, flags, numPids, pidArray, waitToken,
		      childInfoPtr)
    register Proc_ControlBlock 	*curProcPtr;
    int				flags;
    int numPids;	/* Number of entries in pidArray.  
			 *  0 means wait for any child. */
    Proc_PID pidArray[]; /* Array of IDs of children to wait for. */
    int waitToken;      /* Token to use if blocking. */
    ProcChildInfo *childInfoPtr; /* Information to return about child. */

{
    ReturnStatus		status;

    LOCK_MONITOR;

    status = FindExitingChild(curProcPtr, flags & PROC_WAIT_FOR_SUSPEND,
			    numPids, pidArray, childInfoPtr);
    if (proc_MigDebugLevel > 3) {
	printf("pid %x got status %x from FindExitingChild\n",
		   curProcPtr->processID, status);
	if (status == SUCCESS && proc_MigDebugLevel > 6) {
	    printf("Child's id is %x, status %x.\n",
		       childInfoPtr->processID, childInfoPtr->termStatus);
	}
    }

    /* 
     * FAILURE means no one was found, so set up a remote wakeup if needed.
     * Otherwise, just return the childInfo as it was set by FindExitingChild
     * and get out.
     */
    if (status == FAILURE) {
	if (flags & PROC_WAIT_BLOCK) {
	    Sync_SetWaitToken(curProcPtr, waitToken);
	}
    } 

    UNLOCK_MONITOR;
    return(status);
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
    Proc_ControlBlock *paramProcPtr;
    register Proc_ControlBlock *procPtr;
    
    if (numPids > 0) {
	status = CheckPidArray(parentProcPtr, returnSuspend, numPids, pidArray,
			       &paramProcPtr);
    } else {
	status = LookForAnyChild(parentProcPtr, returnSuspend, &paramProcPtr);
    }
    if (status == SUCCESS) {
	procPtr = paramProcPtr;
	if (procPtr->state == PROC_EXITING ||
	    (procPtr->exitFlags & PROC_DETACHED)) {
	    List_Remove((List_Links *) &(procPtr->siblingElement));
	    infoPtr->termReason		= procPtr->termReason;
	    if (procPtr->state == PROC_EXITING) {
		/*
		 * Once an exiting process has been waited on it is moved
		 * from the exiting state to the dead state.
		 */
		procPtr->state = PROC_DEAD;
		Proc_CallFunc(Proc_Reaper,  (ClientData) procPtr, 0);
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
	infoPtr->kernelCpuUsage		= procPtr->kernelCpuUsage.ticks;
	infoPtr->userCpuUsage		= procPtr->userCpuUsage.ticks;
	infoPtr->childKernelCpuUsage	= procPtr->childKernelCpuUsage.ticks;
	infoPtr->childUserCpuUsage 	= procPtr->childUserCpuUsage.ticks;
	infoPtr->numQuantumEnds		= procPtr->numQuantumEnds;
	infoPtr->numWaitEvents		= procPtr->numWaitEvents;

	Proc_Unlock(procPtr);
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
    register	Proc_ControlBlock	*curProcPtr;	/* Parent proc.*/
    Boolean				returnSuspend;	/* Return info about
							 * suspended children.*/    Proc_ControlBlock 			**procPtrPtr;	/* Child proc. */
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
	 *  This can only happen if the child is on a different processor
	 *  from ourself.  We'll wait for the child to become exiting since
	 *  it will take at most the length of a context switch to finish.
	 *  If we don't wait for this child we will miss the transition
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
	        *procPtrPtr = procPtr;
		Proc_Lock(procPtr);
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
    Proc_ControlBlock 			**procPtrPtr;	/* Child proc. */
{
    register Proc_ControlBlock	*procPtr;
    int				i;

    /*
     * The user has specified a list of processes to wait for.
     * If a specified process is non-existent or is not a child of the
     * calling process return an error status.
     */
    for (i=0; i < numPids; i++) {
	procPtr = Proc_LockPID(pidArray[i]);
	if (procPtr == (Proc_ControlBlock *) NIL) {
	    return(PROC_INVALID_PID);
	}
	if (!Proc_ComparePIDs(procPtr->parentID, curProcPtr->processID)) {
	    Proc_Unlock(procPtr);
	    return(PROC_INVALID_PID);
	}
	if ((procPtr->state == PROC_EXITING) ||
	    (procPtr->exitFlags & PROC_DETACHED) ||
	    (returnSuspend && (procPtr->exitFlags & PROC_STATUSES))) {
	    if (!(procPtr->exitFlags & PROC_WAITED_ON)) {
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
 * Proc_NotifyMigratedWaiters --
 *
 *	Waits to find out about migrated processes that have performed
 *	Proc_Waits and then issues Sync_RemoteNotify calls to wake them
 *	up.  Uses a monitored procedure to access the shared list containing
 *	process IDs.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	RPCs are issued as children of migrated processes detach or die.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
void
Proc_NotifyMigratedWaiters(data, callInfoPtr)
    ClientData		data;			/* pid */
    Proc_CallInfo	*callInfoPtr;		/* not used */
{
    Proc_PID 			pid = (Proc_PID) data;
    register Proc_ControlBlock 	*procPtr;
    Sync_RemoteWaiter 		waiter;
    ReturnStatus 		status;

    procPtr = Proc_LockPID(pid);
    if (procPtr == (Proc_ControlBlock *) NIL) {
	return;
    }

    waiter.hostID = procPtr->peerHostID;
    waiter.pid = procPtr->peerProcessID;
    waiter.waitToken =  procPtr->waitToken;
    Proc_Unlock(procPtr);

    if (proc_MigDebugLevel > 3) {
	printf("Proc_NotifyMigratedWaiters: notifying process %x.\n",
		   waiter.pid);
    }
    status = Sync_RemoteNotify(&waiter);
    if (status != SUCCESS) {
	printf("Warning: received status %x notifying process.\n", status);
    }
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

    if (proc_MigDebugLevel > 3) {
	printf("WakeupMigratedParent: inserting process %x.\n", pid);
    }
    Proc_CallFunc(Proc_NotifyMigratedWaiters, (ClientData) pid, 0);
}

