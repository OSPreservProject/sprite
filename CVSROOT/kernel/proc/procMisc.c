/*
 *  procMisc.c --
 *
 *	Misc. routines to get and set process state.
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
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif /* not lint */

#include "sprite.h"
#include "proc.h"
#include "status.h"
#include "sync.h"
#include "sched.h"
#include "sig.h"
#include "stdlib.h"
#include "list.h"
#include "string.h"
#include "procInt.h"
#include "rpc.h"
#include "dbg.h"
#include "vm.h"

#define min(a,b) ((a) < (b) ? (a) : (b))
/*
 * Procedures internal to this file
 */

static Boolean 		CheckIfUsed();
static ReturnStatus 	GetRemotePCB();
static void		FillPCBInfo();	

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
    ProcInitTable();
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

/*
 * Macro to fix up ticks for a process control block.
 */
#define TICKS_TO_TIME(pcbEntry) \
	Timer_TicksToTime(pcbEntry.kernelCpuUsage.ticks,  \
			  &pcbEntry.kernelCpuUsage.time); \
	Timer_TicksToTime(pcbEntry.userCpuUsage.ticks,  \
			  &pcbEntry.userCpuUsage.time); \
	Timer_TicksToTime(pcbEntry.childKernelCpuUsage.ticks,  \
			  &pcbEntry.childKernelCpuUsage.time); \
	Timer_TicksToTime(pcbEntry.childUserCpuUsage.ticks,  \
			  &pcbEntry.childUserCpuUsage.time);


ReturnStatus
Proc_GetPCBInfo(firstPid, lastPid, hostID, infoSize, bufferPtr, 
		argsPtr, trueNumBuffersPtr)
    Proc_PID 		firstPid;	     /* First pid to get info for. */
    Proc_PID		lastPid;	     /* Last pid to get info for. */
    int			hostID;		     /* Host ID to get info for. */
    int			infoSize;   	     /* Size of structure */
    Address	 	bufferPtr;	     /* Pointer to buffers. */
    Proc_PCBArgString	*argsPtr;	     /* Pointer to argument strings. */
    int 		*trueNumBuffersPtr;  /* The actual number of buffers 
						used.*/
{
    register Proc_ControlBlock 	*procPtr;
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
	    TICKS_TO_TIME(pcbEntry);
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

		TICKS_TO_TIME(pcbEntry);
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

	if (trueNumBuffersPtr != USER_NIL) {
	    if (Proc_ByteCopy(FALSE, sizeof(j), (Address) &j, 
			    (Address) trueNumBuffersPtr) != SUCCESS) {
		return(SYS_ARG_NOACCESS);
	    }
	}
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

    statusInfoPtr->processor = pcbPtr->processor;
    statusInfoPtr->state = pcbPtr->state;
    statusInfoPtr->genFlags = pcbPtr->genFlags;
    statusInfoPtr->processID = pcbPtr->processID;
    statusInfoPtr->parentID = pcbPtr->parentID;
    statusInfoPtr->familyID = pcbPtr->familyID;
    statusInfoPtr->userID = pcbPtr->userID;
    statusInfoPtr->effectiveUserID = pcbPtr->effectiveUserID;
    statusInfoPtr->event = pcbPtr->event;
    statusInfoPtr->billingRate = pcbPtr->billingRate;
    statusInfoPtr->recentUsage = pcbPtr->recentUsage;
    statusInfoPtr->weightedUsage = pcbPtr->weightedUsage;
    statusInfoPtr->unweightedUsage = pcbPtr->unweightedUsage;
    statusInfoPtr->kernelCpuUsage = pcbPtr->kernelCpuUsage.time;
    statusInfoPtr->userCpuUsage = pcbPtr->userCpuUsage.time;
    statusInfoPtr->childKernelCpuUsage = pcbPtr->childKernelCpuUsage.time;
    statusInfoPtr->childUserCpuUsage = pcbPtr->childUserCpuUsage.time;
    statusInfoPtr->numQuantumEnds = pcbPtr->numQuantumEnds;
    statusInfoPtr->numWaitEvents = pcbPtr->numWaitEvents;
    statusInfoPtr->schedQuantumTicks = pcbPtr->schedQuantumTicks;
    for(i = 0; i < VM_NUM_SEGMENTS; i++) {
	if (pcbPtr->vmPtr != (Vm_ProcInfo *) NIL && 
	    pcbPtr->vmPtr->segPtrArray[i] != (Vm_Segment *) NIL) {
	    statusInfoPtr->vmSegments[i] = 
		(Vm_SegmentID) pcbPtr->vmPtr->segPtrArray[i]->segNum;
	} else {
	    statusInfoPtr->vmSegments[i] = (Vm_SegmentID) -1;
	}
    }
    statusInfoPtr->sigHoldMask = pcbPtr->sigHoldMask;
    statusInfoPtr->sigPendingMask = pcbPtr->sigPendingMask;
    for(i = 0; i < SIG_NUM_SIGNALS; i++) {
	statusInfoPtr->sigActions[i] = pcbPtr->sigActions[i];
    }
    statusInfoPtr->peerHostID = pcbPtr->peerHostID;
    statusInfoPtr->peerProcessID = pcbPtr->peerProcessID;
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_RpcGetPCB --
 *
 *	Stub to handle a remote request for a PCB or Vm_Segment.
 *
 * Results:
 *	Status of reply:
 *	GEN_INVALID_ARG - index into table of process control blocks is
 *			  invalid, or segment is invalid.
 *	SUCCESS 	- information is returned.
 *
 *	SUCCESS is passed to the caller on this machine.
 *
 * Side effects:
 *	Reply is sent.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus	
Proc_RpcGetPCB(srvToken, clientID, command, storagePtr)
    ClientData 		 srvToken;	/* Handle on server process passed to
				 	 * Rpc_Reply */
    int 		 clientID;	/* Sprite ID of client host */
    int 		 command;	/* Command identifier */
    register Rpc_Storage *storagePtr;	/* The request fields refer to the 
					 * request buffers and also indicate 
					 * the exact amount of data in the 
					 * request buffers.  The reply fields 
					 * are initialized to NIL for the
				 	 * pointers and 0 for the lengths.  
					 * This can be passed to Rpc_Reply */
{
    ReturnStatus	status = SUCCESS;
    Proc_PID		*pidPtr;
    Rpc_ReplyMem	*replyMemPtr;
    Proc_PCBInfo	*pcbPtr;
    Proc_ControlBlock   *procPtr;
    Proc_ControlBlock   pcb;
    int			*segNumPtr;
    Vm_SegmentInfo	*segInfoPtr;
    int			*requestPtr;

    requestPtr = (int *) storagePtr->requestParamPtr;
    if (*requestPtr == GET_SEG_INFO) {
	segNumPtr = (int *) storagePtr->requestDataPtr;
	segInfoPtr = (Vm_SegmentInfo *) malloc(sizeof (Vm_SegmentInfo));
	status = Vm_EncapSegInfo(*segNumPtr, segInfoPtr);
	storagePtr->replyParamPtr = (Address) segInfoPtr;
	storagePtr->replyParamSize = sizeof(Vm_SegmentInfo);
	goto done;
    } else if (*requestPtr == GET_PCB) {
	pidPtr = (Proc_PID *) storagePtr->requestDataPtr;
	if (*pidPtr >= proc_MaxNumProcesses) {
	    status = GEN_INVALID_ARG;
	} else {
	    procPtr = Proc_GetPCB(*pidPtr);
	    if (procPtr == (Proc_ControlBlock *) NIL) {
		panic("Proc_RpcGetPCB: found nil PCB!");
		status = FAILURE;
	    }
	}
	if (status != SUCCESS) {
	    Rpc_Reply(srvToken, status, storagePtr,
		      (int(*)())NIL, (ClientData)NIL);
	    return(SUCCESS);
	}

	bcopy((Address) procPtr, (Address) &pcb, sizeof (Proc_ControlBlock));
	TICKS_TO_TIME(pcb);
	pcbPtr = (Proc_PCBInfo *) malloc(sizeof (Proc_PCBInfo));
	storagePtr->replyParamPtr = (Address) pcbPtr;
	storagePtr->replyParamSize = sizeof(Proc_PCBInfo);
	FillPCBInfo(&pcb, pcbPtr);

	if (procPtr->argString != (Address) NIL) {
	    storagePtr->replyDataSize = strlen(procPtr->argString) + 1;
	    storagePtr->replyDataPtr = (Address) malloc(storagePtr->replyDataSize);
	    (void) strcpy(storagePtr->replyDataPtr, procPtr->argString);
	} else {
	    storagePtr->replyDataSize = 0;
	    storagePtr->replyDataPtr = (Address) NIL;
	}
    }
    done:
    replyMemPtr = (Rpc_ReplyMem *) malloc(sizeof(Rpc_ReplyMem));
    replyMemPtr->paramPtr = storagePtr->replyParamPtr;
    replyMemPtr->dataPtr = storagePtr->replyDataPtr;

    Rpc_Reply(srvToken, status, storagePtr, Rpc_FreeMem,
	    (ClientData) replyMemPtr);
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
	procPtr = Proc_GetEffectiveProc();
	if (procPtr == (Proc_ControlBlock *) NIL) {
	    panic("Proc_GetResUsage: procPtr == NIL\n");
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
	Timer_TicksToTime(procPtr->kernelCpuUsage.ticks,
			  &resUsage.kernelCpuUsage);
	Timer_TicksToTime(procPtr->userCpuUsage.ticks, &resUsage.userCpuUsage);
	Timer_TicksToTime(procPtr->childKernelCpuUsage.ticks, 
				&resUsage.childKernelCpuUsage);
	Timer_TicksToTime(procPtr->childUserCpuUsage.ticks,
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
	procPtr = Proc_GetEffectiveProc();
	if (procPtr == (Proc_ControlBlock *) NIL) {
	    panic("Proc_GetPriority: procPtr == NIL\n");
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
	    procPtr = Proc_GetEffectiveProc();
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
 *	SUCCESS.
 *
 * Side effects:
 *	Prints stuff to screen.
 *
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Proc_Dump()
{
    int i;
    Proc_ControlBlock *pcbPtr;

    printf("\n%8s %5s %10s %10s %8s %8s   %s\n",
	"ID", "wtd", "user", "kernel", "event", "state", "name");
    
    for (i = 0; i < proc_MaxNumProcesses; i++) {
	pcbPtr = proc_PCBTable[i];
	if (pcbPtr->state != PROC_UNUSED) {
	    Proc_DumpPCB(pcbPtr);
	}
    }
    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_DumpPCB --
 *
 *	Prints out the contents of a PCB for debugging purposes.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Prints stuff to the screen.
 *
 *----------------------------------------------------------------------
 */

void
Proc_DumpPCB(procPtr)
    Proc_ControlBlock *procPtr;
{

    Time	kernelTime, userTime;

#define DEBUG_INDEX	0x9

    static char *states[] = {
	"unused",
	"running",
	"ready",
	"waiting",
	"exiting",
	"dead",
	"migrated",
	"new",
	"suspended",
	"debug",
    };
    Proc_State state;

    state = procPtr->state;
    switch (state) {
	case PROC_UNUSED:
	case PROC_RUNNING:
	case PROC_READY:
	case PROC_WAITING:
	case PROC_EXITING:
	case PROC_DEAD:
	case PROC_MIGRATED:
	case PROC_NEW:
	    break;
	case PROC_SUSPENDED:
	    /* If process is suspended for debugging print "debug" for its
	     * state.
	     */
	    if (procPtr->genFlags & (PROC_DEBUGGED | PROC_ON_DEBUG_LIST)) {
		state = (Proc_State)DEBUG_INDEX;
	    }
	    break;
	default:
	    printf("Warning: Proc_DumpPCB: process %x has invalid process state: %x.\n",
		   procPtr->processID, state);
	    return;
    }
    /*
     * A header describing the fields has already been printed.
     */
    Timer_TicksToTime(procPtr->userCpuUsage.ticks, &userTime);
    Timer_TicksToTime(procPtr->kernelCpuUsage.ticks, &kernelTime);
    printf("%8x %5d [%1d,%6d] [%1d,%6d] %8x %8s",
	       procPtr->processID, 
	       procPtr->weightedUsage, 
	       userTime.seconds,
	       userTime.microseconds,
	       kernelTime.seconds, 
	       kernelTime.microseconds,
	       procPtr->event,
	       states[(int) state]);
    if (procPtr->argString != (Address) NIL) {
	char cmd[30];
	char *space;

	(void) strncpy(cmd, procPtr->argString, 30);
	space = strchr(cmd, ' ');
	if (space != (char *) NULL) {
	    *space = '\0';
	}
	printf(" %s\n", cmd);
    } else {
	printf("\n");
    }
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

    curProcPtr = Proc_GetActualProc();

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
	    Sync_WakeWaitingProcess(pcbPtr);
	}
	Proc_Unlock(pcbPtr);
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
    Boolean (*booleanFuncPtr)();	/* function to match */
    ReturnStatus (*actionFuncPtr)();	/* function to invoke on matches */
    Boolean ignoreStatus;		/* do not abort if bad ReturnStatus  */
    int *numMatchedPtr;			/* number of matches in table, or NIL */
{
    ReturnStatus status = SUCCESS;
    Proc_PID *pidArray;
    int max;
    int i;
    int numMatched;

    max = proc_MaxNumProcesses;

    pidArray = (Proc_PID *) malloc(sizeof(Proc_PID) * max);
    numMatched = ProcTableMatch(max, booleanFuncPtr, pidArray);
    for (i = 0; i < numMatched; i++) {
	status = (*actionFuncPtr)(pidArray[i]);
	if ((!ignoreStatus) && (status != SUCCESS)) {
	    break;
	}
    }
    free((Address) pidArray);
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



/*
 *----------------------------------------------------------------------
 *
 * Proc_GetHostIDs --
 *
 *	Returns the sprite IDs corresponding to the machines on which
 *	the current process is effectively executing and on which
 *	it is physically executing.  These hosts are called the virtualHost
 *	and physicalHost, respectively.  For an unmigrated process, these
 *	two are identical.
 *
 * Results:
 *	SUCCESS 		The call was successful.
 *	SYS_ARG_NOACCESS	The user arguments were not accessible.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Proc_GetHostIDs(virtualHostPtr, physicalHostPtr)
    int	*virtualHostPtr;   	/* Buffer to hold virtual host ID. */
    int	*physicalHostPtr;   	/* Buffer to hold physical host ID. */
{

    Proc_ControlBlock *procPtr;
    int host;

    if (physicalHostPtr != (int *) USER_NIL) {

	if (Vm_CopyOut(sizeof(int), (Address) &rpc_SpriteID, 
				(Address) physicalHostPtr) != SUCCESS) {
	    return(SYS_ARG_NOACCESS);
	}
    }


    if (virtualHostPtr != (int *) USER_NIL) {
	procPtr = Proc_GetCurrentProc();
	Proc_Lock(procPtr);
	host = procPtr->peerHostID;
	Proc_Unlock(procPtr);
	if (host == NIL) {
	    host = rpc_SpriteID;
	}
	if (Vm_CopyOut(sizeof(int), (Address) &host, 
				(Address) virtualHostPtr) != SUCCESS) {
	    return(SYS_ARG_NOACCESS);
	}
    }

    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * Proc_PushLockStack --
 *
 *	Pushes the given lock type on the lock stack for the process.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Stuff is printed if the stack overflows.
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
void
Proc_PushLockStack(pcbPtr, type, lockPtr)
    Proc_ControlBlock		*pcbPtr;	/* ptr to pcb to modify */
    int				type;		/* type of lock */
    Address			lockPtr;	/* ptr to lock */

{
#ifdef LOCKDEP
    static Boolean	firstOverflow = TRUE;

    /*
     *  Modifying the lock stack of a process has to be an atomic operation,
     *  but we don't want to use a lock to do this, since this is part of
     *  the code used when locking or unlocking. Using a lock would lead
     *  to a circularity and probably a deadlock. All we really need to
     *  prevent is an interrupt handler from grabbing a lock while we're
     *  modifying the lock stack. A process only modifies its own
     *  lock stack, so turning off interrupts should be good enough.
     */
    DISABLE_INTR();
    if (pcbPtr->lockStackSize >= PROC_LOCKSTACK_SIZE) {
	if (firstOverflow) {
	    printf("Proc_PushLockStack: stack overflow in pcb 0x%x.\n",pcbPtr);
	    firstOverflow = FALSE;
	}
	goto exit;
    }
    if (pcbPtr->lockStackSize < 0 ) {
	printf("Proc_PushLockStack: stack underflow (%d) in pcb 0x%x.\n",
	       pcbPtr->lockStackSize, pcbPtr);
        goto exit;
    }
    pcbPtr->lockStack[pcbPtr->lockStackSize].type = type;
    pcbPtr->lockStack[pcbPtr->lockStackSize].lockPtr = lockPtr;
    pcbPtr->lockStackSize++;
exit:
    ENABLE_INTR();
#endif
}

/*
 *----------------------------------------------------------------------
 *
 * Proc_RemoveFromLockStack --
 *
 *	Removes the given lock from the stack if it is there.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
void
Proc_RemoveFromLockStack(pcbPtr, lockPtr)
    Proc_ControlBlock		*pcbPtr;	/* ptr to pcb to modify */
    Address			lockPtr;	/* ptr to lock */
{
#ifdef LOCKDEP
    int 	i;
    int		stackTop;
    Boolean 	found = FALSE;

    DISABLE_INTR();
    if (pcbPtr->lockStackSize < 0) {
	ENABLE_INTR();
	panic("Lock stack underflow (1).\n");
	goto exit;
    }
    if (pcbPtr->lockStackSize == 0) {
	goto exit;
    }
    stackTop = pcbPtr->lockStackSize - 1;
    for (i = pcbPtr->lockStackSize - 1; i >= 0; i--) {
	if (pcbPtr->lockStack[i].lockPtr == lockPtr) {
	    pcbPtr->lockStack[i].lockPtr = (Address) NIL;
	    pcbPtr->lockStack[i].type = -1;
	    found = TRUE;
	    break;
	}
    }
    if (!found) {
	goto exit;
    }
    for (i = stackTop; i >= 0; i--) {
	if (pcbPtr->lockStack[i].lockPtr != (Address) NIL) {
	    break;
	}
    }
    pcbPtr->lockStackSize = i + 1;
    if (pcbPtr->lockStackSize < 0) {
	printf("lockStackSize %d\n",pcbPtr->lockStackSize);
    }
exit:
    ENABLE_INTR();
#endif
}

