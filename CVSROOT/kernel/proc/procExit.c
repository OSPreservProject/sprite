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
 *	    2) When a process is about to exit the PROC_DIEING flag is set
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
#endif not lint

#include "sprite.h"
#include "status.h"
#include "proc.h"
#include "procInt.h"
#include "procMigrate.h"
#include "migrate.h"

#include "sync.h"
#include "sched.h"
#include "list.h"
#include "vm.h"
#include "sys.h"
#include "dbg.h"
#include "machine.h"
#include "mem.h"
#include "rpc.h"
#include "sig.h"

static	Sync_Lock	exitLock = {0, 0};
#define	LOCKPTR &exitLock

static INTERNAL void AddWaiter();

/*
 * SIGNAL_PARENT
 *
 * Macro to send a SIG_CHILD message to the parent.
 */
#define SIGNAL_PARENT(parentProcPtr, funcName) \
    if ((parentProcPtr)->genFlags & PROC_USER) { \
	ReturnStatus	status; \
	Proc_Lock(parentProcPtr); \
	status = Sig_SendProc(parentProcPtr, SIG_CHILD, 0); \
	Proc_Unlock(parentProcPtr); \
	if (status != SUCCESS) { \
	    Sys_Panic(SYS_WARNING, "%s: Could not signal parent, status<%x>", \
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

    curProcPtr = Proc_GetActualProc(Sys_GetProcessorNumber());
    if (curProcPtr == (Proc_ControlBlock *) NIL) {
	Sys_Panic(SYS_FATAL, "Proc_ExitInt: bad procPtr.\n");
    }

    if (curProcPtr->genFlags & PROC_FOREIGN) {
	ProcRemoteExit(curProcPtr, reason, status, code);
    }
    if (curProcPtr->genFlags & PROC_DEBUGGED) {
	/*
	 * If a process is being debugged then force it onto the debug
	 * list before allowing it to exit.  NOTE: Need to get at the 
	 * machine state (registers and PC).
	 */
	Proc_SuspendProcess(curProcPtr, TRUE, reason, status, code, 0);
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

    Sys_Panic(SYS_FATAL, "Proc_ExitInt: Exiting process still alive!!!\n");
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
    Proc_State				newState;
    register Proc_PCBLink 		*procLinkPtr;

    LOCK_MONITOR;

    Proc_Lock(exitProcPtr);

    exitProcPtr->genFlags |= PROC_DIEING;

    /*
     *  If the parent is still around, add the user and kernel cpu usage
     *  of this process to the parent's summary of children.
     *  If we're detached, don't give this stuff to the parent
     *  (it might not exist.)
     */

    if (!(exitProcPtr->genFlags & PROC_FOREIGN) &&
        !(exitProcPtr->exitFlags & PROC_DETACHED)) {
	register Proc_ControlBlock 	*parentProcPtr;

	parentProcPtr = Proc_GetPCB(exitProcPtr->parentID);
	Timer_AddTicks(exitProcPtr->kernelCpuUsage, 
		parentProcPtr->childKernelCpuUsage, 
		&(parentProcPtr->childKernelCpuUsage));
	Timer_AddTicks(exitProcPtr->childKernelCpuUsage, 
		parentProcPtr->childKernelCpuUsage, 
		&(parentProcPtr->childKernelCpuUsage));
	Timer_AddTicks(exitProcPtr->userCpuUsage, 
		parentProcPtr->childUserCpuUsage, 
		&(parentProcPtr->childUserCpuUsage));
	Timer_AddTicks(exitProcPtr->childUserCpuUsage, 
		parentProcPtr->childUserCpuUsage, 
		&(parentProcPtr->childUserCpuUsage));
    }
    /*
     *  Go through the list of children of the current process to 
     *  make them orphans. When the children exit, this will typically
     *  cause them to go on the dead list directly.
     */

    if (!migrated) {
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
     * If the current process is detached and waited on (i.e. an orphan) then
     * one of two things happen.  If the process is a family head and its list
     * of family members is not empty then the process is put onto the exiting
     * list.  Otherwise the process is put onto the dead list since its 
     * parent has already waited for it.
     */

    if (((exitProcPtr->exitFlags & PROC_DETACHED) &&
        (exitProcPtr->exitFlags & PROC_WAITED_ON)) || migrated) {
	Proc_CallFunc(Proc_Reaper,  (ClientData) exitProcPtr, 0);
	newState = PROC_DEAD;
    } else {
	Proc_ControlBlock 	*parentProcPtr;

	parentProcPtr = Proc_GetPCB(exitProcPtr->parentID);
	if (parentProcPtr->state != PROC_MIGRATED) {
	    Sync_Broadcast(&parentProcPtr->waitCondition);
	} else {
	    AddWaiter(parentProcPtr->processID);
	}

	SIGNAL_PARENT(parentProcPtr, "ExitProcessInt");

	newState = PROC_EXITING;
    }
    if (contextSwitch) {
	Proc_Unlock(exitProcPtr);
	UNLOCK_MONITOR_AND_SWITCH(newState);
	Sys_Panic(SYS_FATAL, "ExitProcessInt: Exiting process still alive\n");
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
 *  	If contextSwitch is TRUE, a context switch is performed.
 *
 *----------------------------------------------------------------------
 */

void
ProcExitProcess(exitProcPtr, reason, status, code, contextSwitch) 
    register Proc_ControlBlock 	*exitProcPtr;	/* Exiting process. */
    int 			reason;		/* Why the process is dieing: 
						 * EXITED, SIGNALED, 
						 * DESTROYED  */
    int				status;		/* Exit status, signal # or 
						 * destroy status. */
    int 			code;		/* Signal sub-status */
    Boolean 			contextSwitch;	/* TRUE => context switch */
{
    int				i;
    register Boolean 		migrated;
    Boolean			noVm;

    migrated = (exitProcPtr->genFlags & PROC_FOREIGN);

    /*
     * Clean up the filesystem state of the exiting process.
     */

    Fs_CloseState(exitProcPtr);

    /*
     * Decrement the reference count on the environmenet.
     */

    if (!migrated && exitProcPtr->environPtr != (Proc_EnvironInfo *) NIL) {
	ProcDecEnvironRefCount(exitProcPtr->environPtr);
    }

    Proc_Lock(exitProcPtr);
    if (exitProcPtr->genFlags & PROC_NO_VM) {
	noVm = TRUE;
    } else {
	noVm = FALSE;
	exitProcPtr->genFlags |= PROC_NO_VM;
    }
    Proc_Unlock(exitProcPtr);

    /*
     * Free up virtual memory resources, unless they were already freed.
     */

    if ((exitProcPtr->genFlags & PROC_USER) && !noVm) {
	for (i = VM_CODE; i <= VM_STACK; i++) {
	    Vm_SegmentDelete(exitProcPtr->vmPtr->segPtrArray[i], exitProcPtr);
	}
    }

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

    exitProcPtr->state = ExitProcessInt(exitProcPtr, migrated, contextSwitch);

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
Proc_Reaper(procPtr, callInfoPtr)
    register	Proc_ControlBlock 	*procPtr;
    Proc_CallInfo			*callInfoPtr;
{
    LOCK_MONITOR;

    if (procPtr->state != PROC_DEAD) {
	Sys_Panic(SYS_FATAL, 
		"Proc_Reaper: non-DEAD proc on dead list.\n");
    }

    if (procPtr->stackStart != NIL) {
	Vm_FreeKernelStack(procPtr->stackStart);
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
    Sync_Broadcast(&parentProcPtr->waitCondition);

    SIGNAL_PARENT(parentProcPtr, "Proc_DetachInt");

    if (parentProcPtr->state == PROC_MIGRATED) {
	AddWaiter(parentProcPtr->processID);
    }

    UNLOCK_MONITOR;
}

void SendSigChild();


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
Proc_InformParent(procPtr, childStatus, backGroundSig)
    register Proc_ControlBlock	*procPtr;	/* Process whose parent to
						 * inform of state change. */
    int				childStatus;	/* PROC_SUSPEND_STATUS |
						 * PROC_RESUME_STATUS */
    Boolean			backGroundSig;	/* Send the SIG_CHILD to
						 * the parent in background.*/
{
    Proc_ControlBlock 	*parentProcPtr;

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
     */
    parentProcPtr = Proc_GetPCB(procPtr->parentID);
    Sync_Broadcast(&parentProcPtr->waitCondition);
    if (backGroundSig) {
	Proc_CallFunc(SendSigChild, (ClientData)procPtr->parentID, 0);
    } else {
	SIGNAL_PARENT(parentProcPtr, "ProcInformParent");
    }
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
void
SendSigChild(data, callInfoPtr)
    ClientData		data;
    Proc_CallInfo	*callInfoPtr;
{
    (void)Sig_Send(SIG_CHILD, SIG_NO_CODE, (Proc_PID)data, FALSE);
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

    procPtr = Proc_GetEffectiveProc(Sys_GetProcessorNumber());
    if (procPtr == (Proc_ControlBlock *) NIL) {
	Sys_Panic(SYS_FATAL, "Proc_Detach: procPtr == NIL\n");
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

static ReturnStatus 	CheckPidArray();
static ReturnStatus 	LookForAnyChild();
static ReturnStatus 	DoWait();


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

    curProcPtr = Proc_GetCurrentProc(Sys_GetProcessorNumber());
    if (curProcPtr == (Proc_ControlBlock *) NIL) {
	Sys_Panic(SYS_FATAL, "Proc_Wait: curProcPtr == NIL.\n");
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
	    newPidArray = (Proc_PID *) Mem_Alloc(newPidSize);
	    status = Vm_CopyIn(newPidSize, (Address) pidArray,
			       (Address) newPidArray);
	    if (status != SUCCESS) {
		Mem_Free((Address) newPidArray);
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
	Mem_Free((Address) newPidArray);
    }

    if (status == SUCCESS) {
	if (procIDPtr != USER_NIL) {
	    if (Vm_CopyOut(sizeof(Proc_PID), (Address) &childInfo.processID, 
		(Address) procIDPtr) != SUCCESS) {
		status = SYS_ARG_NOACCESS;
	    }
	}
	if (status == SUCCESS && reasonPtr != USER_NIL) {
	    if (Vm_CopyOut(sizeof(int), (Address) &childInfo.termReason, 
		(Address) reasonPtr) != SUCCESS) {
		status = SYS_ARG_NOACCESS;
	    }
	}
	if (status == SUCCESS && statusPtr != USER_NIL) {
	    if (Vm_CopyOut(sizeof(int), (Address) &childInfo.termStatus, 
		(Address) statusPtr) != SUCCESS) {
		status = SYS_ARG_NOACCESS;
	    }
	}
	if (status == SUCCESS && subStatusPtr != USER_NIL) {
	    if (Vm_CopyOut(sizeof(int), (Address) &childInfo.termCode, 
		(Address) subStatusPtr) != SUCCESS) {
		status = SYS_ARG_NOACCESS;
	    }
	}
	if (status == SUCCESS && usagePtr != USER_NIL) {
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

ENTRY static ReturnStatus 
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
	     */
	    if (Sync_Wait(&curProcPtr->waitCondition, TRUE)) {
		status = GEN_ABORTED_BY_SIGNAL;
		break;
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

    if (proc_MigDebugLevel > 3) {
	Sys_Printf("ProcRemoteWait(%x, ...) called.\n", procPtr->processID);
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
    
	status = Rpc_Call(procPtr->peerHostID, RPC_PROC_REMOTE_WAIT,
			  &storage);

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

    if (proc_MigDebugLevel > 3) {
	Sys_Printf("ProcRemoteWait returning status %x.\n", status);
	if (status == SUCCESS && proc_MigDebugLevel > 6) {
	    Sys_Printf("Child's id is %x, status %x.\n",
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
	Sys_Printf("pid %x got status %x from FindExitingChild\n",
		   curProcPtr->processID, status);
	if (status == SUCCESS && proc_MigDebugLevel > 6) {
	    Sys_Printf("Child's id is %x, status %x.\n",
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
	infoPtr->kernelCpuUsage		= procPtr->kernelCpuUsage;
	infoPtr->userCpuUsage		= procPtr->userCpuUsage;
	infoPtr->childKernelCpuUsage	= procPtr->childKernelCpuUsage;
	infoPtr->childUserCpuUsage 	= procPtr->childUserCpuUsage;
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
ENTRY void
Proc_NotifyMigratedWaiters(pid, callInfoPtr)
    Proc_PID 		pid;
    Proc_CallInfo	*callInfoPtr;		/* not used */
{
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
	Sys_Printf("Proc_NotifyMigratedWaiters: notifying process %x.\n",
		   waiter.pid);
    }
    status = Sync_RemoteNotify(&waiter);
    if (status != SUCCESS) {
	Sys_Panic(SYS_WARNING,
		  "Warning: received status %x notifying process.\n",
		  status);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * AddWaiter --
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
AddWaiter(pid)
    Proc_PID pid;
{

    if (proc_MigDebugLevel > 3) {
	Sys_Printf("AddWaiter: inserting process %x.\n", pid);
    }
    Proc_CallFunc(Proc_NotifyMigratedWaiters, (ClientData) pid, 0);
}

