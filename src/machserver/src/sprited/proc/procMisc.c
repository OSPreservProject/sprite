/*
 *  procMisc.c --
 *
 *	Misc. routines related to processes.
 *
 * Copyright 1986 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/sprited/proc/RCS/procMisc.c,v 1.13 92/04/16 11:36:08 kupfer Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <sprite.h>
#include <mach.h>
#include <mach/mach_host.h>
#include <ckalloc.h>
#include <status.h>
#include <string.h>

#include <proc.h>
#include <procInt.h>
#include <procMachInt.h>
#include <rpc.h>
#include <sig.h>
#include <sync.h>
#include <timer.h>
#include <utils.h>
#include <user/vmTypes.h>

/* 
 * XXX - this is in mach_traps.h, but there's a cpp problem including both 
 * mach.h and mach_traps.h. 2-Oct-91.
 */
extern mach_port_t mach_thread_self _ARGS_((void));

#define min(a,b) ((a) < (b) ? (a) : (b))

#define MACH_HIGHEST_PRIORITY	 0 /* highest Mach priority */
#define MACH_LOWEST_PRIORITY	31 /* lowest Mach priority */

/*
 * Procedures internal to this file
 */

static ReturnStatus 	GetRemotePCB _ARGS_((int hostID, Proc_PID pid,
				Proc_PCBInfo *pcbPtr, char *argString));
static void		FillPCBInfo _ARGS_((Proc_ControlBlock *pcbPtr,
				Proc_PCBInfo *statusInfoPtr));	
static int		SpritePriorityToMach _ARGS_((int spritePriority));


/*
 *----------------------------------------------------------------------
 *
 * Proc_Init --
 *
 *	Called during startup to initialize data structures.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Process table initialized, debug list initialized, locks initialized.
 *
 *----------------------------------------------------------------------
 */

void
Proc_Init()
{
    ProcMachInit();
    ProcInitTable();
    ProcExitInit();
    ProcTaskThreadInit();
    ProcDebugInit();
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_GetPCBInfo --
 *
 *	Returns the process control blocks for the specified processes
 *	on the specified host.  If firstPid is equal to PROC_MY_PID
 *	and the hostID is PROC_MY_HOSTID, then the PCB for the current
 *	process is returned.  Otherwise PCBs for all processes in the
 *	range firstPid to lastPid on host hostID are returned.  Only
 *	the index portions of the processIDs for firstPid and lastPid
 *	are relevant. 
 *
 * Results:
 *	SYS_INVALID_ARG - 	firstPid was < 0, firstPid > lastPid
 *	SYS_ARG_NOACCESS - 	The buffers to store the pcbs in were not
 *				accessible.
 *      *trueNumBuffers is set to be the actual number of
 *	PCBs returned which can be less than the number requested if
 *	lastPid - firstPid is greater than the maximum PCBs available.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Proc_GetPCBInfo(firstPid, lastPid, hostID, infoSize, bufferPtr, 
		argsPtr, trueNumBuffersPtr)
    Proc_PID 		firstPid;	     /* First pid to get info for. */
    Proc_PID		lastPid;	     /* Last pid to get info for. */
    int			hostID;		     /* Host ID to get info for. */
    int			infoSize;   	     /* Size of structure */
    Address	 	bufferPtr;	     /* Pointer to buffers (user 
					      * address) */
    Proc_PCBArgString	*argsPtr;	     /* Pointer to argument strings 
					      * (user address) */
    int 		*trueNumBuffersPtr;  /* OUT: The actual number of 
					      * buffers  used.*/
{
    register Proc_ControlBlock 	*procPtr = (Proc_ControlBlock *) NIL;
    int				i, j;
    char 			argString[PROC_PCB_ARG_LENGTH];
    Proc_ControlBlock		pcbEntry;
    Boolean			remote = FALSE;
    Proc_PID			processID = firstPid;
    ReturnStatus		status = SUCCESS;
    Proc_PCBInfo		statusInfo;
    int				bytesToCopy;


    if (firstPid != PROC_MY_PID) {
	firstPid &= PROC_INDEX_MASK;
	lastPid &= PROC_INDEX_MASK;
	if ((firstPid > lastPid) ||
	    ((firstPid == PROC_MY_PID) && hostID != PROC_MY_HOSTID)) {
	    return(GEN_INVALID_ARG);
	}
    }
    if (bufferPtr == USER_NIL) {
	return (SYS_ARG_NOACCESS);
    }
    if (hostID != PROC_MY_HOSTID &&
	(hostID <= 0 || hostID > NET_NUM_SPRITE_HOSTS)) {
	return(GEN_INVALID_ARG);
    }

    bytesToCopy = min(sizeof(Proc_PCBInfo), infoSize);
    /*
     * Determine whether to get process table entries for this machine.
     * Currently, the information for this machine is returned unless
     * another machine is explicitly specified; i.e., migrated processes
     * get information for their current machine rather than their home.
     */

    if (hostID == PROC_MY_HOSTID) {
#ifdef FORWARD_MIGRATED_GET_PCBS
	procPtr = Proc_GetCurrentProc();
	Proc_Lock(procPtr);
	if (procPtr->genFlags & PROC_FOREIGN) {
	    hostID = procPtr->peerHostID;
	    processID = procPtr->peerHostID;
	    remote = TRUE;
	} 
	Proc_Unlock(procPtr);
#endif /* FORWARD_MIGRATED_GET_PCBS */
    } else if (hostID != rpc_SpriteID) {
	remote = TRUE;
    }

    if (firstPid == PROC_MY_PID) {
	/*
	 *  Return PCB for the current process.
	 */
	procPtr = Proc_GetCurrentProc();
	if (!remote) {
	    bcopy((Address)procPtr, (Address)&pcbEntry,
		    sizeof (Proc_ControlBlock));
	    FillPCBInfo(&pcbEntry, &statusInfo);
	} else {
	    status = GetRemotePCB(hostID, processID, &statusInfo,
				       argString);
	    if (status != SUCCESS) {
		return(status);
	    }
	}
	if (Proc_ByteCopy(FALSE, bytesToCopy,
		(Address)&statusInfo, (Address) bufferPtr) != SUCCESS) {
	    return(SYS_ARG_NOACCESS);
	}
	if (argsPtr != (Proc_PCBArgString *) USER_NIL) {
	    if (!remote) {
		if (procPtr->argString != (Address) NIL) {
		    (void) strncpy(argString, procPtr->argString,
			    PROC_PCB_ARG_LENGTH - 1);
		    argString[PROC_PCB_ARG_LENGTH - 1] = '\0';
		} else {
		    argString[0] = '\0';
		}
	    }
	    if (Proc_ByteCopy(FALSE, PROC_PCB_ARG_LENGTH, argString,
			      (Address) argsPtr) != SUCCESS) {
		return(SYS_ARG_NOACCESS);
	    }
	}
    } else {
	
	/*
	 * Return PCB for all processes or enough to fill all of
	 * the buffers, whichever comes first.
	 */

	
	for (i = firstPid, j = 0; 
	     i <= lastPid; 
	     i++, j++, (Address) bufferPtr += infoSize) {
	    if (!remote) {
		if (i >= proc_MaxNumProcesses) {
		    break;
		}
		procPtr = Proc_GetPCB(i);
		if (procPtr == (Proc_ControlBlock *) NIL) {
		    panic("Proc_GetInfo: procPtr == NIL!\n");
		    status = FAILURE;
		    break;
		}
		bcopy((Address)procPtr, (Address)&pcbEntry,
			sizeof (Proc_ControlBlock));

		FillPCBInfo(&pcbEntry, &statusInfo);
	    } else {
		status = GetRemotePCB(hostID, (Proc_PID) i, &statusInfo,
					   argString);
		if (status != SUCCESS) {
		    /*
		     * Break if we hit an error.  The typical error condition
		     * is to hit an invalid process ID, which happens since
		     * we don't know proc_MaxNumProcesses on the other
		     * machine.  Instead, we convert GEN_INVALID_ARG to
		     * SUCCESS and return what we found so far.
		     */
		    if (status == GEN_INVALID_ARG) {
			status = SUCCESS;
		    }
		    break;
		}
	    }
	    if (Proc_ByteCopy(FALSE, bytesToCopy,
		    (Address)&statusInfo, (Address) bufferPtr) != SUCCESS) {
		return(SYS_ARG_NOACCESS);
	    }
	    if (argsPtr != (Proc_PCBArgString *) USER_NIL) {
		if (!remote) {
		    if (procPtr->argString != (Address) NIL) {
			(void) strncpy(argString, procPtr->argString,
				       PROC_PCB_ARG_LENGTH - 1);
			argString[PROC_PCB_ARG_LENGTH - 1] = '\0';
		    } else {
			argString[0] = '\0';
		    }
		}
		if (Proc_ByteCopy(FALSE, PROC_PCB_ARG_LENGTH, argString,
				  (Address) &(argsPtr[j])) != SUCCESS) {
		    return(SYS_ARG_NOACCESS);
		}
	    }
	}

	*trueNumBuffersPtr = j;
    }

    return(status);
}



/*
 * Define some constants used to distinguish RPC sub-commands.
 */
#define GET_PCB 1
#define GET_SEG_INFO 2

/*
 *----------------------------------------------------------------------
 *
 * GetRemotePCB --
 *
 *	Perform an RPC to get a process control block from another host.
 *
 * Results:
 *	The return status from the RPC is returned.  
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static ReturnStatus	
GetRemotePCB(hostID, pid, pcbPtr, argString)
    int		hostID;		/* Host to send RPC to. */
    Proc_PID	pid;		/* index of PCB to obtain. */
    Proc_PCBInfo *pcbPtr;	/* Place to return PCB data. */
    char	*argString;	/* Place to return argument string. */
{
    Rpc_Storage		storage;
    ReturnStatus 	status;
    int			request;

    request = GET_PCB;
    storage.requestParamPtr = (Address)&request;
    storage.requestParamSize = sizeof(request);
    storage.requestDataPtr = (Address)&pid;
    storage.requestDataSize = sizeof(Proc_PID);
    storage.replyParamPtr = (Address)pcbPtr;
    storage.replyParamSize = sizeof(Proc_PCBInfo);
    storage.replyDataPtr = (Address)argString;
    storage.replyDataSize = PROC_PCB_ARG_LENGTH;

    status = Rpc_Call(hostID, RPC_PROC_GETPCB, &storage);
    if (status == SUCCESS && storage.replyDataSize == 0) {
	argString[0] = '\0';
    }
    return(status);

}
#if 0
/*
 *----------------------------------------------------------------------
 *
 * Proc_GetRemoteSegInfo --
 *
 *	Perform an RPC to get info for a VM segment control from another host.
 *
 * Results:
 *	The return status from the RPC is returned.  
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus	
Proc_GetRemoteSegInfo(hostID, segNum, segInfoPtr)
    int		hostID;		/* Host to send RPC to. */
    int		segNum;		/* index of segment to obtain. */
    Vm_SegmentInfo *segInfoPtr;	/* Place to return segment data. */
{
    Rpc_Storage		storage;
    ReturnStatus 	status;
    int			request;

    request = GET_SEG_INFO;
    storage.requestParamPtr = (Address)&request;
    storage.requestParamSize = sizeof(request);
    storage.requestDataPtr = (Address)&segNum;
    storage.requestDataSize = sizeof(int);
    storage.replyParamPtr = (Address)segInfoPtr;
    storage.replyParamSize = sizeof(Vm_SegmentInfo);
    storage.replyDataPtr = (Address)NIL;
    storage.replyDataSize = 0;

    status = Rpc_Call(hostID, RPC_PROC_GETPCB, &storage);
    return(status);

}
#endif /* 0 */


/*
 *----------------------------------------------------------------------
 *
 * FillPCBInfo --
 *
 *	Fills in a Proc_PCBInfo structure from the contents of a
 *	control block.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
FillPCBInfo(pcbPtr, statusInfoPtr)
    Proc_ControlBlock		*pcbPtr; 	/* Ptr to pcb to convert */
    Proc_PCBInfo		*statusInfoPtr; /* Structure to fill in */
{
    int 	i;

    /* 
     * For the time being, any fields we can't immediately fill in are 
     * zeroed out.  Some of them could probably get better information 
     * either by simply calling Mach or by having sprited keep better 
     * records. 
     */

    statusInfoPtr->processor = 0; /* XXX */
    statusInfoPtr->state = pcbPtr->state;
    statusInfoPtr->genFlags = pcbPtr->genFlags;
    statusInfoPtr->processID = pcbPtr->processID;
    statusInfoPtr->parentID = pcbPtr->parentID;
    statusInfoPtr->familyID = pcbPtr->familyID;
    statusInfoPtr->userID = pcbPtr->userID;
    statusInfoPtr->effectiveUserID = pcbPtr->effectiveUserID;
    statusInfoPtr->event = (int)(pcbPtr->currCondPtr);
    statusInfoPtr->billingRate = 0; /* XXX */
    statusInfoPtr->recentUsage = 0; /* XXX */
    statusInfoPtr->weightedUsage = 0; /* XXX */
    statusInfoPtr->unweightedUsage = 0;	/* XXX */
    statusInfoPtr->kernelCpuUsage = time_ZeroSeconds; /* XXX */
    statusInfoPtr->userCpuUsage = time_ZeroSeconds; /* XXX */
    statusInfoPtr->childKernelCpuUsage = time_ZeroSeconds; /* XXX */
    statusInfoPtr->childUserCpuUsage = time_ZeroSeconds; /* XXX */
    statusInfoPtr->numQuantumEnds = 0; /* XXX */
    statusInfoPtr->numWaitEvents = 0; /* XXX */
    statusInfoPtr->schedQuantumTicks = 0; /* XXX */

    /* 
     * For the time being just fill in the addresses of the segments.  It 
     * would probably make more sense to fill in the MachID number for the 
     * segment.
     */
    /* 
     * All right, so we won't even fill in the addresses of the segments.  
     * We'll get to it eventually...
     */
    bzero((_VoidPtr)statusInfoPtr->vmSegments,
	  VM_NUM_SEGMENTS * sizeof(Vm_SegmentID));

    statusInfoPtr->sigHoldMask = pcbPtr->sigHoldMask;
    statusInfoPtr->sigPendingMask = pcbPtr->sigPendingMask;
    for(i = 0; i < SIG_NUM_SIGNALS; i++) {
	statusInfoPtr->sigActions[i] = pcbPtr->sigActions[i];
    }

    statusInfoPtr->peerHostID = pcbPtr->peerHostID;
    statusInfoPtr->peerProcessID = pcbPtr->peerProcessID;

    statusInfoPtr->machID = ((pcbPtr->taskInfoPtr == NULL)
			     ? 0 : pcbPtr->taskInfoPtr->machId);

}


/*
 *----------------------------------------------------------------------
 *
 * Proc_StateName --
 *
 *	Convert a process state to a printable string.
 *	XXX this should probably go into libc.
 *
 * Results:
 *	Returns the (read-only) string name for the given process 
 *	state. 
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

char *
Proc_StateName(state)
    Proc_State state;
{
    char *result;

    /* 
     * A switch statement is used instead of an array because it's 
     * less likely to screw up if somebody changes Proc_State.
     */
    switch (state) {
    case PROC_UNUSED:
	result = "unused";
	break;
    case PROC_RUNNING:
	result = "running";
	break;
    case PROC_READY:
	result = "ready";
	break;
    case PROC_WAITING:
	result = "waiting";
	break;
    case PROC_EXITING:
	result = "exiting";
	break;
    case PROC_DEAD:
	result = "dead";
	break;
    case PROC_MIGRATED:
	result = "migrated";
	break;
    case PROC_NEW:
	result = "new";
	break;
    case PROC_SUSPENDED:
	result = "suspended";
	break;
    default:
	result = "<bogus state value>";
	break;
    }

    return result;
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_SetPriority --
 *
 *	Set the priority for a process.  Currently can only be used by a 
 *	server process to set its own priority.
 *
 * Results:
 *	Returns a status code.
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
    Proc_ControlBlock *procPtr;
    kern_return_t kernStatus;
    int machPriority;		/* Mach priority number */
    thread_t thread;		/* thread corresponding to given pid */

    if (useFamily) {
	return GEN_NOT_IMPLEMENTED;
    }

    if (pid != PROC_MY_PID) {
	return GEN_NOT_IMPLEMENTED;
    } else {
	procPtr = Proc_GetEffectiveProc();
	if (!(procPtr->genFlags & PROC_KERNEL)) {
	    return GEN_NOT_IMPLEMENTED;
	}
    } 

    machPriority = SpritePriorityToMach(priority);
    thread = mach_thread_self();

    kernStatus = thread_max_priority(thread, sys_PrivProcSetPort,
				     machPriority);
    if (kernStatus != KERN_SUCCESS) {
	return Utils_MapMachStatus(kernStatus);
    }

    kernStatus = thread_priority(thread, machPriority, FALSE);
    if (kernStatus != KERN_SUCCESS) {
	return Utils_MapMachStatus(kernStatus);
    }

    return SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * SpritePriorityToMach --
 *
 *	Convert a Sprite priority number to a Mach priority number.  Note 
 *	that the highest Sprite priority number gives the highest priority, 
 *	whereas the highest Mach priority number gives the lowest priority.
 *
 * Results:
 *	Returns the approximate Mach equivalent to the given Sprite number.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
SpritePriorityToMach(spritePriority)
    int spritePriority;
{
    int machPriority = 0;

    /* 
     * Could do this arithmetically, but what the heck, there are only 5
     * possibilities.
     */
    
    switch (spritePriority) {
    case PROC_NO_INTR_PRIORITY:
	machPriority = MACH_HIGHEST_PRIORITY + 1;
	break;
    case PROC_HIGH_PRIORITY:
	machPriority = MACH_LOWEST_PRIORITY / 4;
	break;
    case PROC_NORMAL_PRIORITY:
	machPriority = MACH_LOWEST_PRIORITY / 2;
	break;
    case PROC_LOW_PRIORITY:
	machPriority = MACH_LOWEST_PRIORITY * 3 / 4;
	break;
    case PROC_VERY_LOW_PRIORITY:
	machPriority = MACH_LOWEST_PRIORITY - 1;
	break;
    default:
	panic("SpritePriorityToMach: bogus priority (%d)\n",
	      spritePriority);
    }

    return machPriority;
}



/*
 *----------------------------------------------------------------------
 *
 * Proc_KillAllProcesses --
 *
 *	Kill all processes in the proc table except for the main 
 *	process.  If userProcsOnly is TRUE only kill user processes.
 *
 * Results:
 *	The number of runnable and waiting processes.
 *
 * Side effects:
 *	As many processes as possible are killed.
 *
 *----------------------------------------------------------------------
 */
int
Proc_KillAllProcesses(userProcsOnly)
    Boolean	userProcsOnly;	/* TRUE if only kill user processes. */
{
    register int index;		/* index into process table */
    register Proc_ControlBlock	*pcbPtr;
    Proc_ControlBlock		*curProcPtr;
    int				alive = 0;

    curProcPtr = Proc_GetActualProc();

    for (index = 0; index < proc_MaxNumProcesses; index++) {
	if (index == PROC_MAIN_PROC_SLOT) {
	    continue;
	}
	pcbPtr = proc_PCBTable[index];
	if (pcbPtr->state == PROC_UNUSED ||
	    (userProcsOnly && !(pcbPtr->genFlags & PROC_USER))) {
	    continue;
	}
	Proc_Lock(pcbPtr);
	/* 
	 * The current process should be a user process making the shutdown 
	 * system call.  If that process has already been marked as killed, 
	 * don't count it as alive, and don't bother trying to kill it 
	 * again. 
	 */
	if (pcbPtr != curProcPtr ||
	    	!(pcbPtr->genFlags & PROC_NO_MORE_REQUESTS)) {
	    if (pcbPtr->state == PROC_READY ||
		    pcbPtr->state == PROC_WAITING ||
		    pcbPtr->state == PROC_MIGRATED ||
		    pcbPtr->state == PROC_SUSPENDED) {
		alive++;
		(void) Sig_SendProc(Proc_AssertLocked(pcbPtr), SIG_KILL,
				    FALSE, 0, (Address)0);
		/* 
		 * When the current process kills itself, it should 
		 * free up Mach and VM resources that it holds.  This is so 
		 * that its VM resources won't get stuck during system 
		 * shutdown. 
		 */
		if (pcbPtr == curProcPtr) {
		    ProcFreeTaskThread(Proc_AssertLocked(pcbPtr));
		}
	    }
	}
	Proc_Unlock(Proc_AssertLocked(pcbPtr));
    }

    return(alive);
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_WakeupAllProcesses --
 *
 *	Wakup all waiting processes.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	All waiting processes are awakened.
 *
 *----------------------------------------------------------------------
 */
void
Proc_WakeupAllProcesses()
{
    register int		i;
    register Proc_ControlBlock	*pcbPtr;
    Proc_ControlBlock		*curProcPtr;

    curProcPtr = Proc_GetActualProc();

    for (i = 0; i < proc_MaxNumProcesses; i++) {
	pcbPtr = proc_PCBTable[i];
	if (pcbPtr == curProcPtr) {
	    continue;
	}
	Proc_Lock(pcbPtr);
	if (pcbPtr->state != PROC_UNUSED) {
	    Sync_WakeWaitingProcess(Proc_AssertLocked(pcbPtr));
	}
	Proc_Unlock(Proc_AssertLocked(pcbPtr));
    }
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

    procPtr = Proc_GetEffectiveProc();
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
 *	general SUCCESS is returned.  If numMatchedPtr is non-NIL, then
 *	the number of processes matched is returned in *numMatchedPtr.
 *
 * Side effects:
 *	The process table is locked temporarily.  Otherwise, dependent on the
 *	call-back procedures.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Proc_DoForEveryProc(booleanFuncPtr, actionFuncPtr, ignoreStatus, numMatchedPtr)
    Boolean (*booleanFuncPtr) _ARGS_((Proc_ControlBlock *pcbPtr));
					/* function to match */
    ReturnStatus (*actionFuncPtr)_ARGS_((Proc_PID pid));	
					/* function to invoke on matches */
    Boolean ignoreStatus;		/* do not abort if bad ReturnStatus  */
    int *numMatchedPtr;			/* number of matches in table, or NIL */
{
    ReturnStatus status = SUCCESS;
    Proc_PID *pidArray;
    unsigned int max;
    int i;
    int numMatched;

    max = proc_MaxNumProcesses;

    pidArray = (Proc_PID *) ckalloc(sizeof(Proc_PID) * max);
    numMatched = ProcTableMatch(max, booleanFuncPtr, pidArray);
    for (i = 0; i < numMatched; i++) {
	status = (*actionFuncPtr)(pidArray[i]);
	if ((!ignoreStatus) && (status != SUCCESS)) {
	    break;
	}
    }
    ckfree((Address) pidArray);
    if (numMatchedPtr != (int *) NIL) {
	*numMatchedPtr = numMatched;
    }
    if (ignoreStatus) {
	return(SUCCESS);
    }
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_GetHostIDsStub --
 *
 *	Returns the sprite IDs corresponding to the machines on which
 *	the current process is effectively executing and on which
 *	it is physically executing.  These hosts are called the virtualHost
 *	and physicalHost, respectively.  For an unmigrated process, these
 *	two are identical.
 *
 * Results:
 * 	Returns KERN_SUCCESS.  Fills in the two host ID numbers and the 
 * 	"pending signals" flag.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

kern_return_t
Proc_GetHostIDsStub(serverPort, virtualHostPtr, physicalHostPtr,
		    sigPendingPtr)
    mach_port_t serverPort;
    int	*virtualHostPtr;   	/* OUT: virtual host ID. */
    int	*physicalHostPtr;   	/* OUT: physical host ID. */
    boolean_t *sigPendingPtr;	/* OUT: pending signals flag */
{
    Proc_ControlBlock *procPtr;
    int host;

#ifdef lint
    serverPort = serverPort;
#endif

    *physicalHostPtr = rpc_SpriteID;
    
    procPtr = Proc_GetCurrentProc();
    Proc_Lock(procPtr);
    host = procPtr->peerHostID;
    Proc_Unlock(Proc_AssertLocked(procPtr));
    if (host == NIL) {
	host = rpc_SpriteID;
    }
    *virtualHostPtr = host;

    *sigPendingPtr = Sig_Pending(Proc_GetCurrentProc());
    return KERN_SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_ContextSwitch --
 *
 *	Compatibility routine for native Sprite code to simulate a context 
 *	switch. XXX is this routine really necessary anymore?
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The current thread either context switches or exits.
 *
 *----------------------------------------------------------------------
 */

void
Proc_ContextSwitch(newState)
    Proc_State newState;
{
    Proc_ControlBlock *procPtr = Proc_GetCurrentProc();

    switch (newState) {
    case PROC_UNUSED:
    case PROC_WAITING:
    case PROC_NEW:
	panic("Proc_ContextSwitch: unexpected new state.\n");
	break;
    case PROC_READY:
	cthread_yield();
	break;
    case PROC_EXITING:
    case PROC_DEAD:
    case PROC_MIGRATED:
	/* 
	 * XXX Maybe it would be simpler to just allow a transition from 
	 * DEAD to DEAD, rather than have this "if" here.
	 */
	if (procPtr->state != newState) {
	    Proc_SetState(procPtr, newState);
	}
	cthread_set_name(cthread_self(), (char *)0);
	cthread_exit(0);
	break;
    case PROC_SUSPENDED:
	/* 
	 * We would like to do something like what Sync_UnlockAndSwitch 
	 * does, except that the process is unlocked when it calls this 
	 * routine, which makes it difficult to wait on the 
	 * suspendedCondition in the PCB. Oh, well.  The caller probably 
	 * should be using Proc_UnlockAndSwitch anyway.
	 */ 
	panic("Proc_ContextSwitch: PROC_SUSPENDED not supported yet.\n");
	break;
    default:
	panic("Proc_ContextSwitch: bogus state.\n");
    }
}
