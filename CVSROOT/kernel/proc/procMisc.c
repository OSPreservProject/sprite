/*
 *  procMisc.c --
 *
 *	Misc. routines to get and set process state.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "proc.h"
#include "status.h"
#include "sync.h"
#include "sched.h"
#include "sig.h"
#include "mem.h"
#include "list.h"


/*
 *----------------------------------------------------------------------
 *
 * Proc_Suspend --
 *
 *	Suspend execution of the calling process.  The parent is notified
 *	that this child stopped but the child remains attached to the
 *	parent.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Process context switches to the suspended state.
 *
 *----------------------------------------------------------------------
 */
void
Proc_Suspend(sigNum)
    int	sigNum;
{
    register	Proc_ControlBlock	*procPtr;

    procPtr = Proc_GetEffectiveProc(Sys_GetProcessorNumber());
    procPtr->termReason = PROC_TERM_SIGNALED;
    procPtr->termStatus = sigNum;
    procPtr->termCode = SIG_NO_CODE;
    ProcInformParent();
    Sched_ContextSwitch(PROC_SUSPENDED);
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_Resume --
 *
 *	Resume execution of the given process if it is currently suspended.
 *	
 *	SYNCHRONIZATION NOTE:
 *
 *	This routine is only called from the signal module with its monitor
 *	lock down which ensures that we are the only ones who can change the 
 *	state of the process to or from the suspended state.  Thus there
 *	is no race condition on the state of the destination process.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Process is made runnable
 *
 *----------------------------------------------------------------------
 */
void
Proc_Resume(procPtr)
    Proc_ControlBlock	*procPtr;
{
    if (procPtr->state == PROC_SUSPENDED) {
	Sched_MakeReady(procPtr);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_GetPCBInfo --
 *
 *	Returns the process control block.  If firstPid is equal to  
 *	PROC_MY_PID then the PCB for the current process is returned.
 *	Otherwise PCBs for all processes in the range firstPid to lastPid
 *	are returned.  trueNumBuffers is set to be the actual number of
 *	PCBs returned which can be less than the number requested if firstPid
 *	of lastPid are greater than the maximum PCB available.
 *
 * Results:
 *	SYS_INVALID_ARG - 	firstPid was < 0, firstPid > lastPid
 *	SYS_ARG_NOACCESS - 	The buffers to store the pcbs in were not
 *				accessible.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Proc_GetPCBInfo(firstPid, lastPid, bufferPtr, trueNumBuffersPtr)
    Proc_PID 		firstPid;	     /* First pid to get info for. */
    Proc_PID		lastPid;	     /* Last pid to get info for. */
    Proc_ControlBlock 	*bufferPtr;	     /* Pointer to buffers. */
    int 		*trueNumBuffersPtr;  /* The actual number of buffers 
						used.*/
{
    register Proc_ControlBlock 	*procPtr;
    int				i, j;

    if ((firstPid != PROC_MY_PID) && (firstPid > lastPid)) {
	return(SYS_INVALID_ARG);
    } else if (bufferPtr == USER_NIL) {
	return (SYS_ARG_NOACCESS);
    }

    if (firstPid == PROC_MY_PID) {

	/*
	 *  Return PCB for the current process.
	 */

	if (Proc_ByteCopy(FALSE, sizeof(Proc_ControlBlock), 
		(Address) Proc_GetEffectiveProc(Sys_GetProcessorNumber()),
		(Address) bufferPtr) != SUCCESS) {
	    return(SYS_ARG_NOACCESS);
	}
    } else {

	/*
	 * Return PCB for all processes or enough to fill all of
	 * the buffers, whichever comes first.
	 */

	for (i = firstPid, j = 0; 
	     i < proc_MaxNumProcesses && i <= lastPid; 
	     i++, j++) {
	    procPtr = Proc_GetPCB(i);
	    if (procPtr == (Proc_ControlBlock *) NIL) {
		Sys_Panic(SYS_FATAL, "Proc_GetInfo: procPtr == NIL!");
	    }
	    if (Proc_ByteCopy(FALSE, sizeof(Proc_ControlBlock), 
		(Address) procPtr, (Address) &(bufferPtr[j])) != SUCCESS) {
		return(SYS_ARG_NOACCESS);
	    }
	}

	if (trueNumBuffersPtr != USER_NIL) {
	    if (Proc_ByteCopy(FALSE, sizeof(j), (Address) &j, 
			    (Address) trueNumBuffersPtr) != SUCCESS) {
		return(SYS_ARG_NOACCESS);
	    }
	}
    }

    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_GetResUsage --
 *
 *	Returns the resource usage for a process.
 *
 * Results:
 *	SYS_INVALID_ARG - 	buffer address was invalid.
 *	PROC_INVALID_PID - 	The pid was out-of-range or specified a
 *				non-existent process.
 *	SYS_ARG_NOACCESS - 	The buffers to store the pcbs in were not
 *				accessible.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Proc_GetResUsage(pid, bufferPtr)
    Proc_PID 		pid;
    Proc_ResUsage 	*bufferPtr;	     
{
    register Proc_ControlBlock 	*procPtr;
    Proc_ResUsage 		resUsage;	     
    ReturnStatus		status = SUCCESS;

    if (pid == PROC_MY_PID) {
	procPtr = Proc_GetEffectiveProc(Sys_GetProcessorNumber());
	if (procPtr == (Proc_ControlBlock *) NIL) {
	    Sys_Panic(SYS_FATAL, "Proc_GetIDs: procPtr == NIL\n");
	} 
	Proc_Lock(procPtr);
    } else {
	procPtr = Proc_LockPID(pid);
        if (procPtr == (Proc_ControlBlock *) NIL) {
            return (PROC_INVALID_PID);
        }
    }

    /*
     *  Copy the information to the out parameters.
     */

    if (bufferPtr == USER_NIL) {
	status = SYS_INVALID_ARG;
    } else {
	Timer_TicksToTime(procPtr->kernelCpuUsage, &resUsage.kernelCpuUsage);
	Timer_TicksToTime(procPtr->userCpuUsage, &resUsage.userCpuUsage);
	Timer_TicksToTime(procPtr->childKernelCpuUsage, 
				&resUsage.childKernelCpuUsage);
	Timer_TicksToTime(procPtr->childUserCpuUsage,
				&resUsage.childUserCpuUsage);
	resUsage.numQuantumEnds = procPtr->numQuantumEnds;
	resUsage.numWaitEvents 	= procPtr->numWaitEvents;

	if (Proc_ByteCopy(FALSE, sizeof(Proc_ResUsage), 
	    (Address) &resUsage, (Address) bufferPtr) != SUCCESS){
	    status = SYS_ARG_NOACCESS;
	}
    }
    Proc_Unlock(procPtr);
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_GetPriority --
 *
 *	Returns the priority of a process.
 *
 * Results:
 *	SYS_INVALID_ARG - 	priorityPtr address was invalid.
 *	PROC_INVALID_PID - 	The pid was out-of-range or specified a
 *				non-existent process.
 *	SYS_ARG_NOACCESS - 	The buffer to store the priority was not
 *				accessible.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Proc_GetPriority(pid, priorityPtr)
    Proc_PID pid;	/* ID of process whose priority is to be returned. */
    int *priorityPtr;	/* Priority returned by Proc_GetPriority. */
{
    register Proc_ControlBlock 	*procPtr;
    ReturnStatus		status = SUCCESS;

    if (pid == PROC_MY_PID) {
	procPtr = Proc_GetEffectiveProc(Sys_GetProcessorNumber());
	if (procPtr == (Proc_ControlBlock *) NIL) {
	    Sys_Panic(SYS_FATAL, "Proc_GetIDs: procPtr == NIL\n");
	} 
	Proc_Lock(procPtr);
    } else {
	procPtr = Proc_LockPID(pid);
        if (procPtr == (Proc_ControlBlock *) NIL) {
            return (PROC_INVALID_PID);
        }
    }

    /*
     *  Copy the information to the out parameter.
     */

    if (priorityPtr == USER_NIL) {
	status = SYS_INVALID_ARG;
    } else {
	if (Proc_ByteCopy(FALSE, sizeof(int),
 	        (Address) &(procPtr->billingRate),
	        (Address) priorityPtr) != SUCCESS) {
	    status = SYS_ARG_NOACCESS;
	}
    }
    Proc_Unlock(procPtr);
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_SetPriority --
 *
 *	Sets the priority for a process.
 *
 * Results:
 *	PROC_INVALID_PID - 	The pid was out-of-range or specified a
 *				non-existent process.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Proc_SetPriority(pid, priority, useFamily)
    Proc_PID 	pid;		/* ID of process whose priority is to be set. */
    int 	priority;	/* New scheduling priority for pid. */
    Boolean 	useFamily;	/* If TRUE, use pid as the head of a process 
				 * family, and set the priority of every 
				 * process in the family. */
{
    register Proc_ControlBlock 	*procPtr;
    register Proc_PCBLink	*procLinkPtr;
    List_Links			*familyList;
    int				userID;
    ReturnStatus		status;

    if (priority > PROC_NO_INTR_PRIORITY) {
	priority = PROC_NO_INTR_PRIORITY;
    } else if (priority < PROC_VERY_LOW_PRIORITY) {
	priority = PROC_VERY_LOW_PRIORITY;
    }

    if (useFamily) {
	/*
	 * Set priorities of processes in family.
	 */
	status = Proc_LockFamily((int) pid, &familyList, &userID);
	if (status != SUCCESS) {
	    return(status);
	}
	if (!Proc_HasPermission(userID)) {
	    Proc_UnlockFamily((int) pid);
	    return(PROC_UID_MISMATCH);
	}
	LIST_FORALL(familyList, (List_Links *) procLinkPtr) {
	    procPtr = procLinkPtr->procPtr;
	    Proc_Lock(procPtr);
	    procPtr->billingRate = priority;
	    Proc_Unlock(procPtr);
	}
	Proc_UnlockFamily((int) pid);
    } else {
	/*
	 * Set the individual process's priority.
	 */
	if (pid == PROC_MY_PID) {
	    procPtr = Proc_GetEffectiveProc(Sys_GetProcessorNumber());
	    Proc_Lock(procPtr);
	} else {
	    procPtr = Proc_LockPID(pid);
	    if (procPtr == (Proc_ControlBlock *) NIL) {
		return (PROC_INVALID_PID);
	    }
	    if (!Proc_HasPermission(procPtr->effectiveUserID)) {
		Proc_Unlock(procPtr);
		return(PROC_UID_MISMATCH);
	    }
	}
	procPtr->billingRate = priority;
	Proc_Unlock(procPtr);
    }

    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_Profile --
 *
 *	Starts profiling the memory accesses of the current process.
 *
 * Results:
 *	SUCCESS		-	always returned for now.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
ReturnStatus
Proc_Profile(shiftSize, lowPC, highPC, interval, counterArray)
    int shiftSize;	/* # of bits to shift the PC to the right. */
    int lowPC;		/* The lowest PC to profile. */
    int highPC;		/* The highest PC to profile. */
    Time interval;	/* The time interval at which the PC is sampled. */
    int counterArray[];	/* Counters used to count instruction executions. */
{
    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_Dump --
 *
 *	Prints out an abbreviated proc table for debugging purposes.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Proc_Dump()
{
    register int i;
    register Proc_ControlBlock *pcbPtr;

    Sys_Printf("%8s: %11s %8s %8s %s\n", "PID:", "state ", "event ", "pc ",
		     "program");
    for (i = 0; i < proc_MaxNumProcesses; i++) {
	pcbPtr = proc_PCBTable[i];
	if (pcbPtr->state == PROC_UNUSED) {
	    continue;
	}
	Sys_Printf("%8x:", pcbPtr->processID);
	switch(pcbPtr->state) {
	    case PROC_RUNNING:
		Sys_Printf(" running   ");
		break;
	    case PROC_READY:
		Sys_Printf(" ready     ");
		break;
	    case PROC_WAITING:
		Sys_Printf(" waiting   ");
		break;
	    case PROC_EXITING:
		Sys_Printf(" exiting   ");
		break;
	    case PROC_DEAD:
		Sys_Printf(" dead      ");
		break;
	    case PROC_DEBUGABLE:
		Sys_Printf(" debug     ");
		break;
	    case PROC_SUSPENDED:
		Sys_Printf(" suspended ");
		break;
	}
	Sys_Printf(" %8x", pcbPtr->event);
	Sys_Printf(" %8x", pcbPtr->progCounter);
	Sys_Printf(" %s", pcbPtr->codeFileName);
	Sys_Printf("\n");
    }
    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_KillAllProcesses --
 *
 *	Send the kill signal to all processes in the proc table except for
 *	the caller.  If userProcsOnly is TRUE only send signals to user 
 *	processes.
 *
 * Results:
 *	The number of runnable and waiting processes.
 *
 * Side effects:
 *	The kill signal bit is set for all processes.
 *
 *----------------------------------------------------------------------
 */

int
Proc_KillAllProcesses(userProcsOnly)
    Boolean	userProcsOnly;	/* TRUE if only kill user processes. */
{
    register int i;
    register Proc_ControlBlock	*pcbPtr;
    Proc_ControlBlock		*curProcPtr;
    int				alive = 0;

    curProcPtr = Proc_GetActualProc(Sys_GetProcessorNumber());

    for (i = 0; i < proc_MaxNumProcesses; i++) {
	pcbPtr = proc_PCBTable[i];
	if (pcbPtr == curProcPtr || pcbPtr->state == PROC_UNUSED ||
	    (userProcsOnly && !(pcbPtr->genFlags & PROC_USER))) {
	    continue;
	}
	Proc_Lock(pcbPtr);
	if (pcbPtr->state == PROC_RUNNING ||
		pcbPtr->state == PROC_READY ||
		pcbPtr->state == PROC_WAITING ||
		pcbPtr->state == PROC_MIGRATED) {
	    alive++;
	    (void) Sig_SendProc(pcbPtr, SIG_KILL, 0);
	}
	Proc_Unlock(pcbPtr);
    }

    return(alive);
}



/*
 *----------------------------------------------------------------------
 *
 * Proc_HasPermission --
 *
 *      See if the current process has permission to perform an operation on
 *      a process with the given user id.
 *
 * Results:
 *      TRUE if the current process has the same effective user id
 *      as the given user id or the current process is super user.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */

Boolean
Proc_HasPermission(userID)
    int         userID;
{
    Proc_ControlBlock   *procPtr;

    procPtr = Proc_GetEffectiveProc(Sys_GetProcessorNumber());
    return(procPtr->effectiveUserID == userID ||
           procPtr->effectiveUserID == PROC_SUPER_USER_ID);
}



/*
 *----------------------------------------------------------------------
 *
 * Proc_DoForEveryProc --
 *
 *	For every process in the process table, apply *booleanFuncPtr to it.
 *	If that returns TRUE, apply *actionFuncPtr to it.  This is done by
 *	passing booleanFuncPtr to a monitored routine and having it
 *	return an array of qualifying processes.  There is a bit of a race
 *	condition if something happens to any of those processes after the
 *	list is returned, but in that case the process is ignored and the next
 *	one is processed.
 *
 *	IgnoreStatus indicates whether the routine should abort if
 *	a non-SUCCESS status is returned by *actionFuncPtr.
 *
 * Results:
 *	If anything "unexpected" happens, FAILURE will be returned, but in
 *	general SUCCESS is returned.
 *
 * Side effects:
 *	The process table is locked temporarily.  Otherwise, dependent on the
 *	call-back procedures.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Proc_DoForEveryProc(booleanFuncPtr, actionFuncPtr, ignoreStatus)
    Boolean (*booleanFuncPtr)();	/* function to match */
    ReturnStatus (*actionFuncPtr)();	/* function to invoke on matches */
    Boolean ignoreStatus;		/* do not abort if bad ReturnStatus  */
{
    ReturnStatus status;
    Proc_PID *pidArray;
    int max;
    int i;
    int numMatched;

    max = proc_MaxNumProcesses;

    pidArray = (Proc_PID *) Mem_Alloc(sizeof(Proc_PID) * max);
    numMatched = ProcTableMatch(max, booleanFuncPtr, pidArray);
    for (i = 0; i < numMatched; i++) {
	status = (*actionFuncPtr)(pidArray[i]);
	if ((!ignoreStatus) && (status != SUCCESS)) {
	    break;
	}
    }
    Mem_Free((Address) pidArray);
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_SetServerPriority --
 *
 *	Changes the priority of a server process to the non-interruptable
 *	value. The pid is assumed to be valid.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The process's priority is changed.
 *
 *----------------------------------------------------------------------
 */

void
Proc_SetServerPriority(pid)
    Proc_PID	pid;
{
    Proc_GetPCB(pid)->billingRate = PROC_NO_INTR_PRIORITY;
}
