/* 
 * procRemote.c --
 *
 *	Routines for a remote workstation to handle process migration.  This
 *	involves the following:
 *
 *		- Grant permission to another workstation to migrate to this
 *		  one.
 *		- Accept the process from the other workstation, creating a
 *		  local copy of it.
 *		- Execute the process, sending system calls back to the home
 *		  node for processing.
 *		- Transfer the process back to the home node upon termination.
 *
 * Copyright 1986, 1988 Regents of the University of California
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
#include "mach.h"
#include "proc.h"
#include "procMigrate.h"
#include "procInt.h"
#include "migrate.h"
#include "procAOUT.h"
#include "fs.h"
#include "mem.h"
#include "sig.h"
#include "spriteTime.h"
#include "list.h"
#include "byte.h"
#include "vm.h"
#include "sys.h"
#include "rpc.h"
#include "sched.h"
#include "sync.h"
#include "sysSysCall.h"
#include "sysSysCallParam.h"

#include "dbg.h"

/*
 * Procedures internal to this file.
 */

static Proc_PID GetProcessState();
static ReturnStatus GetStream();
ENTRY static ReturnStatus GetUserInfo();
static ReturnStatus ContinueMigratedProc();

/*
 * An address for copy-out or make accessible is reasonable if it is not NIL.
 */

#define ValidAddress(addr) (((Address) addr != (Address) NIL) && \
			    ((Address) addr != (Address) USER_NIL))


/*
 *----------------------------------------------------------------------
 *
 * 
 * Proc_MigReceiveInfo --
 *
 *	Get information about a process migrating to this machine.  The
 *	information may take the form of a process, VM, or file state.
 *	For each type of encapsulation, call the appropriate routine
 *	to decompose the information.  The process state is the first
 *	information that should be migrated, and upon receiving the
 *	Proc_ControlBlock information, a local process must be created.
 *
 * Results:
 *	A ReturnStatus is returned.
 *
 * Side effects:
 *	The effects depend on the type of information transferred.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
ReturnStatus
Proc_MigReceiveInfo(hostID, commandPtr, bufferSize, buffer, returnInfoPtr)
    int hostID;
    Proc_MigrateCommand *commandPtr;
    int bufferSize;
    Address buffer;
    Proc_MigrateReply *returnInfoPtr;
{
    Proc_PID pid;
    Proc_ControlBlock *procPtr;

    if (commandPtr->command == PROC_MIGRATE_PROC) {
	if (proc_MigDebugLevel > 5) {
	    printf("Calling GetProcessState.\n");
	}
	pid = GetProcessState(buffer, hostID);
	if (proc_MigDebugLevel > 5) {
	    printf("GetProcessState returned pid %x.\n", pid);
	}
	if (pid != (Proc_PID) NIL) {
	    returnInfoPtr->remotePID = pid;
	    returnInfoPtr->status = SUCCESS;
	} else {
	    returnInfoPtr->status = FAILURE;
	}
    } else {
	pid = commandPtr->remotePID;
	PROC_GET_VALID_PCB(pid, procPtr);
	if (procPtr == (Proc_ControlBlock *) NIL) {
	    if (proc_MigDebugLevel > 3) {
		panic("Invalid pid: %x.\n", pid);
	    }
	    returnInfoPtr->status = PROC_INVALID_PID;
	} else {
	    switch(commandPtr->command) {
		case PROC_MIGRATE_VM:
		    /*
		     * Get segment information and create local segment.
		     */
		     if (proc_MigDebugLevel > 7) {
			 printf("Proc_MigReceiveInfo: receiving segment.\n");
		     } 
		    returnInfoPtr->status =
			    Vm_ReceiveSegmentInfo(procPtr, buffer);
		     if (proc_MigDebugLevel > 7) {
			 printf("Proc_MigReceiveInfo: received segment.\n");
		     } 
		    break;
		case PROC_MIGRATE_FILES:
		    /*
		     * Get stream information and set up file pointers.
		     */
		    if (proc_MigDebugLevel > 7) {
	printf("Proc_MigReceiveInfo: calling Fs_DeencapFileState.\n");
		    }
		    returnInfoPtr->status =
			    Fs_DeencapFileState(procPtr, buffer);
		    if (proc_MigDebugLevel > 7) {
			printf("Fs_DeencapFileState returned status %x.\n",
				   returnInfoPtr->status);
		    }
		    break;
		case PROC_MIGRATE_USER_INFO:
		    /*
		     * Change information in a process's control block.
		     */
		    returnInfoPtr->status = GetUserInfo(procPtr, buffer);
		    if (proc_MigDebugLevel > 6) {
			printf("GetUserInfo returned status %x.\n",
				   returnInfoPtr->status);
		    }
		    break;
	        case PROC_MIGRATE_RESUME:
		    /*
		     * Get segment information and create local segment.
		     */
		    returnInfoPtr->status = ContinueMigratedProc(procPtr);
		    if (proc_MigDebugLevel > 6) {
			printf("ContinueMigratedProc returned status %x.\n",
				   returnInfoPtr->status);
		    }
		    break;
	        default:
	            returnInfoPtr->status = FAILURE;
	            return(FAILURE);
		    break;
		}
	}
    }
    return(SUCCESS);
}
    

/*
 *----------------------------------------------------------------------
 *
 * GetProcessState --
 *
 *	Get the state of a process from another node.  The information to
 *	be received is listed in the comments before SendProcessState.
 *	The information is contained in the parameter ``buffer''.
 *	A new local process is created to act as a surrogate for the
 *	foreign process.
 *
 * Results:
 *	The local process ID of the newly-created process is returned.
 *
 * Side effects:
 *	A dummy local process is created and is initialized to reflect the
 *	state of the foreign process.
 *
 *----------------------------------------------------------------------
 */

static Proc_PID
GetProcessState(buffer, hostID)
    Address buffer;
    int hostID;
{
    Proc_ControlBlock 	*procPtr;	/* The process being migrated */
    Proc_PID		pid;
    int			argStringLength;
    Boolean 		home = FALSE;
    ReturnStatus	status;

    Byte_EmptyBuffer(buffer, Proc_PID, pid);
    if (pid == (Proc_PID) NIL) {
	procPtr = ProcGetUnusedPCB();
	Byte_EmptyBuffer(buffer, Proc_PID, procPtr->peerProcessID);
	procPtr->peerHostID = hostID;
    } else {
	Proc_PID peerProcessID;

	home = TRUE;
	procPtr = Proc_LockPID(pid);
	if (procPtr == (Proc_ControlBlock *) NIL ||
	        (procPtr->state != PROC_MIGRATED)) {
	    return((Proc_PID) NIL);
	}
	Byte_EmptyBuffer(buffer, Proc_PID, peerProcessID);
	if (peerProcessID != procPtr->peerProcessID) {
	    Proc_Unlock(procPtr);
	    return((Proc_PID) NIL);
	}
    }

    /*
     * Retrieve IDs, flags, scheduling information, and machine-dependent
     * state.
     */

    bcopy(buffer, (Address) &procPtr->parentID,
	  PROC_NUM_ID_FIELDS * sizeof (int));
    buffer += PROC_NUM_ID_FIELDS * sizeof(int);
    bcopy(buffer, (Address) &procPtr->genFlags, PROC_NUM_FLAGS * sizeof (int));
    buffer += PROC_NUM_FLAGS * sizeof(int);

    procPtr->genFlags |= PROC_NO_VM;
    if (home) {
	procPtr->genFlags &= (~PROC_FOREIGN);
	procPtr->kcallTable = mach_NormalHandlers;
    } else {
	procPtr->genFlags |= PROC_FOREIGN;
	procPtr->kcallTable = mach_MigratedHandlers;
    }
    procPtr->genFlags &= ~(PROC_MIG_PENDING | PROC_MIGRATING);

    bcopy(buffer, (Address) &procPtr->billingRate,
	    PROC_NUM_SCHED_FIELDS * sizeof (int));
    buffer += PROC_NUM_SCHED_FIELDS * sizeof(int);

    bcopy(buffer, (Address) &procPtr->sigHoldMask, SIG_INFO_SIZE);
    buffer += SIG_INFO_SIZE;
    procPtr->sigPendingMask &=
	    ~((1 << SIG_MIGRATE_TRAP) | (1 << SIG_MIGRATE_HOME));

    /*
     * Set up machine-dependent state.
     */
    status = Mach_DeencapState(procPtr, buffer);
    if (status != SUCCESS) {
	if (proc_MigDebugLevel > 0) {
	    panic("GetProcessState: error %x returned by Mach_DeencapState");
	}
	return((Proc_PID) NIL);
    }
    buffer += Mach_GetEncapSize();

    /*
     * Set up the code segment.
     */

    Byte_EmptyBuffer(buffer, int, argStringLength);

    if (procPtr->argString != (char *) NIL) {
	free(procPtr->argString);
    }
    procPtr->argString = (char *) malloc(argStringLength);
    bcopy(buffer, (Address) procPtr->argString, argStringLength);
    buffer += argStringLength;

    procPtr->state 		= PROC_NEW     ;

    Vm_ProcInit(procPtr);

    /*
     * Initialize some of the fields as if for a new process.  If migrating
     * home, these are already set up.  FIXME: inheritance of fields has
     * changed a lot, and some of this may become the same for both
     * migrating away and migrating home.
     */
    if (!home) {
	procPtr->event			= NIL;
	/*
	 *  Initialize our child list to remove any old links.
	 */
	List_Init((List_Links *) procPtr->childList);


    }


    pid = procPtr->processID;
    Proc_Unlock(procPtr);

    if (proc_MigDebugLevel > 4) {
	printf("Received process state for process %x.\n", procPtr->processID);
    }

    return(pid);
}


/*
 *----------------------------------------------------------------------
 *
 * GetUserInfo --
 *
 *	Get an updated Proc_ControlBlock for a migrated process.
 *	(This could be changed into a macro and used in GetProcessState
 *	as well, but leave it this way for tracing for a while.)
 *
 * Results:
 *	SUCCESS is always returned.
 *
 * Side effects:
 *	The Proc_ControlBlock for the specified process is updated to
 *	contain possibly changed information regarding IDs, priority, etc.
 *
 *----------------------------------------------------------------------
 */

ENTRY static ReturnStatus
GetUserInfo(procPtr, buffer)
    Proc_ControlBlock *procPtr;
    Address buffer;
{
    Proc_Lock(procPtr);
    /*
     * Retrieve user IDs and scheduling information.
     */
    Byte_EmptyBuffer(buffer, int, procPtr->userID);
    Byte_EmptyBuffer(buffer, int, procPtr->effectiveUserID);
    Byte_EmptyBuffer(buffer, int, procPtr->familyID);
    Byte_EmptyBuffer(buffer, int, procPtr->billingRate);

    Proc_Unlock(procPtr);

    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * ContinueMigratedProc --
 *
 *	Restore the state of a migrated process to its state prior to
 *	migration.  If it is migrating home, remove the link between
 *	the foreign copy of the process and this copy.  Add it to the
 *	ready queue.
 *
 *
 * Results:
 *      A ReturnStatus.  (SUCCESS for now.)
 *
 * Side effects:
 *	The process may be made runnable.
 *
 *----------------------------------------------------------------------
 */

static ReturnStatus
ContinueMigratedProc(procPtr)
    Proc_ControlBlock 	*procPtr;	/* The process being migrated */
{
    if (proc_MigDebugLevel > 10) {
	printf(">> Entering debugger before continuing process %x.\n", procPtr->processID);
	DBG_CALL;
    }

    Proc_Lock(procPtr);
    if (!(procPtr->genFlags & PROC_FOREIGN)) {
	procPtr->peerProcessID = (Proc_PID) NIL;
	procPtr->peerHostID = NIL;
    } else {
	Proc_AddMigDependency(procPtr->processID, procPtr->peerHostID);
    }
    Proc_Unlock(procPtr);
    Sched_MakeReady(procPtr);

    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_ResumeMigProc --
 *
 *	Resume a migrated user process.  This is the first thing that is
 * 	called when a migrated process continues execution.
 *
 * Results:
 *	Does not return.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
Proc_ResumeMigProc(pc)
    int pc;		/* program counter to resume execution */
{
    register     	Proc_ControlBlock *procPtr;


    MASTER_UNLOCK(sched_Mutex);

    procPtr = Proc_GetCurrentProc();
    Proc_Lock(procPtr);
    procPtr->genFlags &= ~PROC_NO_VM;
    VmMach_ReinitContext(procPtr);
    Proc_Unlock(procPtr);

    /*
     * Start the process running.  This does not return.  
     */

    Mach_StartUserProc(procPtr, (Address) pc);
    panic("ProcResumeMigProc: Mach_StartUserProc returned.\n");
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_DoRemoteCall --
 *
 *	Generic system call handler for migrated processes.  This routine
 *	takes a specification for the system call to invoke and the usage
 *	of its parameters and sends them to the home node for processing.
 *      Each parameter has a type and a disposition.  The type indicates
 *	whether the parameter is an int, string, or other specialized
 * 	structure.  The disposition is "in", "out", or "in/out", as well
 *	as whether the parameter should be accessed using Vm_Copy{In,Out}
 *	or Vm_MakeAccessible.  The specific defined constants are documented
 *	in sysSysCallParam.h.
 *
 *	All "in" or "in/out" parameters are sent in the RPC in the data
 *	buffer.  All "out" or "in/out" parameters have space allocated for
 *	them in the reply data buffer.  Strings and characters are padded
 *	so that each field is aligned on an even address, so the actual
 *	addresses within the buffers may be cast as integers and other types
 *	without additional copying.
 *
 * Results:
 *	A ReturnStatus is returned.  Values may be returned in the areas
 *	referenced by pointers that are passed into the routine, but they
 *	are dependent upon the system call invoked.
 *
 * Side effects:
 *	A remote procedure call is performed.  
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Proc_DoRemoteCall(callNumber, numWords, argsPtr, specsPtr)
    int callNumber;			/* number of system call to invoke */
    int numWords;			/* number of words passed in */
    ClientData *argsPtr;		/* pointer to data passed in */
    Sys_CallParam *specsPtr;		/* array of information about the
					 * parameters to this call */
{
    Proc_ControlBlock 	*procPtr;	/* The migrated process */
    int dataSize = 0;			/* size of the data buffer */
    int replyDataSize = 0;		/* size of the reply buffer */
    Address ptr;			/* place holder within buffer */
    int *intArgsArray;			/* to treat argsPtr as array of ints */
    int disp;				/* "disposition" of a parameter */
    int type;				/* type of param (int, string, ...) */
    int size;				/* number of bytes of a param */
    int paddedSize;			/* likewise, padded to be aligned */
    Proc_RemoteCall call;		/* parameter information for RPC */
    Address pointerArray[SYS_MAX_ARGS]; /* array of made accessible pointers */
    int numBytesAccessible[SYS_MAX_ARGS];/* number of bytes made accessible,
					  * or zero if this variable has not
					  * been made accessible */
    Rpc_Storage storage;		
    Address dataBuffer = (Address) NIL;
    Address replyDataBuffer = (Address) NIL;
    int i;
    register ReturnStatus status;	/* returned by assorted procedures */
    ReturnStatus remoteCallStatus;	/* status returned by system call */
    Proc_TraceRecord record;		/* used to store trace data */
    int lastArraySize = -1;			/* size of last array found */
    int numTries;			/* number of times trying RPC */

    /*
     * Create a synonym for argsPtr so that integer arguments can be referred
     * to without casting them all the time.
     */
    
    intArgsArray = (int *) argsPtr;
    
    
    procPtr = Proc_GetActualProc();

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
	panic("Proc_DoRemoteCall: Proc_ExitInt returned.\n");
	return(PROC_NO_PEER);
    }

    /*
     * Set up the RPC parameters: pass the home processID, the system call
     * number, and parameter information.  Additional parameter information
     * (type and disposition) is copied below.
     */

    call.processID = procPtr->peerProcessID;
    call.callNumber = callNumber;
    call.numArgs = numWords;
    call.parseArgs = TRUE;

    if (proc_DoTrace && proc_DoCallTrace) {
	record.processID = call.processID;
	record.flags = PROC_MIGTRACE_START;
	record.info.call.callNumber = callNumber;
	Trace_Insert(proc_TraceHdrPtr, PROC_MIGTRACE_CALL,
		     (ClientData) &record);
    }

    for (i = 0; i < numWords; i++) {
	numBytesAccessible[i] = 0;
    }

    /*
     * Determine the type and size of each argument.  If the argument is
     * a string, make it accessible now in order to find its size.
     */

    for (i = 0; i < numWords; i++) {
	disp = specsPtr[i].disposition;
	type = specsPtr[i].type;
	if ((disp & SYS_PARAM_OUT) &&
	    !(disp & (SYS_PARAM_ACC | SYS_PARAM_COPY))) {
	    panic("Proc_DoRemoteCall: Illegal parameter information for call %d for output parameter %d",
		      callNumber, i);
	    return(GEN_INVALID_ARG);
	}
	switch(type) {
	    case SYS_PARAM_INT:
	    case SYS_PARAM_CHAR: 
	    case SYS_PARAM_PROC_PID:
	    case SYS_PARAM_PROC_RES:
	    case SYS_PARAM_SYNC_LOCK:
	    case SYS_PARAM_FS_ATT:
	    case SYS_PARAM_TIMEPTR: 
	    case SYS_PARAM_TIME1:
	    case SYS_PARAM_TIME2:
	    case SYS_PARAM_RANGE1:
	    case SYS_PARAM_RANGE2:
	    case SYS_PARAM_PCB:
	    case SYS_PARAM_PCBARG:
	    case SYS_PARAM_VM_CMD:
	    case SYS_PARAM_DUMMY:
	        /*
		 * For typical arguments, the size may be found in a global
		 * array that is subscripted by the type.  If the argument
		 * is really an array, multiply by the number of arguments.
		 *
		 * Special case Proc_GetPCBInfo because it can allow
		 * a range of values or a PROC_MY_PID to specify one
		 * value.
		 */
	        if ((! (disp & SYS_PARAM_ARRAY)) ||
		        (callNumber == SYS_PROC_GETPCBINFO &&
		        intArgsArray[0] == PROC_MY_PID)) {
		    size = sys_ParamSizes[type];
		} else {
		    /*
		     * The argument is actually a pointer to an array of
		     * the given type.  The size of the array is either
		     * the parameter just before the array pointer (an INT),
		     * or the difference between the two preceding arguments
		     * (RANGE1 and RANGE2).  Remember the size of the array,
		     * since a more arrays of the same size may follow.
		     *
		     */
		    if (i > 0 && specsPtr[i-1].type == SYS_PARAM_INT) {
			size = (intArgsArray[i-1]) *
				sys_ParamSizes[type];
		    } else if (i > 1 && specsPtr[i-2].type ==
			       SYS_PARAM_RANGE1 &&
			       specsPtr[i-1].type == SYS_PARAM_RANGE2) {
			 if (type == SYS_PARAM_PCB) {
			    intArgsArray[i-1] = intArgsArray[i-1] &
				    PROC_INDEX_MASK;
			    intArgsArray[i-2] = intArgsArray[i-2]  &
				    PROC_INDEX_MASK;
			}
			size = (intArgsArray[i-1] - intArgsArray[i-2] + 1) *
				sys_ParamSizes[type];
		    } else if (i > 1 && (specsPtr[i-1].disposition
					 & (SYS_PARAM_ARRAY))) {
			size = lastArraySize;
		    } else {
			panic("Proc_DoRemoteCall: bad parameter list.\n");
			status = FAILURE;
			goto failure;
		    }
		    lastArraySize = size;
		}
		break;
 	    case SYS_PARAM_FS_NAME: 
 	    case SYS_PARAM_PROC_ENV_NAME: 
    	    case SYS_PARAM_PROC_ENV_VALUE:
                /*
		 * The argument is a string.  If copying a string in, make
		 * it accessible and figure out its size dynamically.  If
		 * copying it out, just allocate space for the maximum number 
		 * of bytes for that type.
		 */
		if ((disp & SYS_PARAM_ACC) && (disp & SYS_PARAM_IN)) {
		    status = Proc_MakeStringAccessible(sys_ParamSizes[type],
 		        (char **) &argsPtr[i], &numBytesAccessible[i], &size);
		    if (status != SUCCESS) {
			if (proc_MigDebugLevel > 6) {
			    panic("Proc_DoRemoteCall: status %x returned by Proc_MakeStringAccessible.\n", status);
			}
			goto failure;
		    }
		    /*
 		     * Send the null byte as well.
		     */
		    size += 1;
		} else if ((disp & SYS_PARAM_COPY) && (disp & SYS_PARAM_OUT)) {
		    size = sys_ParamSizes[type];
		} else {
		    panic("Proc_DoRemoteCall: can't handle string parameter combination.\n");
		    status = FAILURE;
		    goto failure;
		}
		pointerArray[i] = (Address) argsPtr[i];
		break;				    
	    case SYS_PARAM_STRING:
	    default:
		panic("Proc_DoRemoteCall: can't handle argument type.\n");
		status = FAILURE;
		goto failure;
	}

	/*
	 * Store the information about this parameter.  If the argument is
	 * a pointer and it is USER_NIL, then store the fact that the
	 * parameter is NIL so that we won't try to copy out to that address
	 * later on, or to make it accessible.  Keep track of the sizes of the
	 * input and output buffers.  Make accessible anything that isn't
	 * already accessible.
	 */
	call.info[i].size = size;
	if ((disp & (SYS_PARAM_COPY | SYS_PARAM_ACC))
	        && !ValidAddress(argsPtr[i])) {
	    disp |= SYS_PARAM_NIL;
	} else {
	    paddedSize = Byte_AlignAddr(size);
	    if (disp & SYS_PARAM_IN) {
		dataSize += paddedSize;
	    }
	    if (disp & SYS_PARAM_OUT) {
		replyDataSize += paddedSize;
	    }
	}
	call.info[i].disposition = disp;
	if ((disp & SYS_PARAM_ACC) && numBytesAccessible[i] == 0) {
	    if (! (disp & SYS_PARAM_NIL)) {
		int accessType; 

		if ((disp & SYS_PARAM_IN) && !(disp & SYS_PARAM_OUT)) {
		    accessType = VM_READONLY_ACCESS;
		} else if (!(disp & SYS_PARAM_IN) && (disp & SYS_PARAM_OUT)) {
		    accessType = VM_OVERWRITE_ACCESS;
		} else {
		    accessType = VM_READWRITE_ACCESS;
		}
		Vm_MakeAccessible(accessType, size, (Address) argsPtr[i],
				  &numBytesAccessible[i], &pointerArray[i]);
		if (numBytesAccessible[i] != size ||
		        pointerArray[i] == (Address) NIL) {
		    status = SYS_ARG_NOACCESS;
		    goto failure;
		}
	    } else {
		pointerArray[i] = (Address) NIL;
	    }
	}
    }

    /*
     * Now that the total sizes are known, allocate space and save the
     * arguments in the data buffer.  While the RPC system and network
     * driver are confused and screw up on buffers that are too small,
     * pad the sizes accordingly.
     */

#define RPC_MIN_BUFFER_SIZE 12
#ifdef RPC_MIN_BUFFER_SIZE
    if (dataSize < RPC_MIN_BUFFER_SIZE) {
	dataSize = RPC_MIN_BUFFER_SIZE;
    }
    if (replyDataSize < RPC_MIN_BUFFER_SIZE) {
	replyDataSize = RPC_MIN_BUFFER_SIZE;
    }
#endif /* RPC_MIN_BUFFER_SIZE */
	
    dataBuffer = (Address) malloc(dataSize);
    ptr = dataBuffer;
    replyDataBuffer = (Address) malloc(replyDataSize);
    call.replySize = replyDataSize;

    for (i = 0; i < numWords; i++) {
	disp = call.info[i].disposition;
	if ((disp & SYS_PARAM_IN) && ! (disp & SYS_PARAM_NIL)) {
	    if (disp & SYS_PARAM_ACC) {
		bcopy(pointerArray[i], ptr, call.info[i].size);
	    } else if (disp & SYS_PARAM_COPY) {
		status = Vm_CopyIn(call.info[i].size, (Address) argsPtr[i],
				   ptr);
		if (status != SUCCESS) {
		    goto failure;
		}
	    } else {
		bcopy((Address) &argsPtr[i], ptr, call.info[i].size);
	    }
	ptr += Byte_AlignAddr(call.info[i].size);
	}
    }

    /*
     * Set up for the RPC.
     */
    storage.requestParamPtr = (Address) &call;
    storage.requestParamSize = sizeof(Proc_RemoteCall);

    storage.requestDataPtr = dataBuffer;
    storage.requestDataSize = dataSize;

    storage.replyParamPtr = (Address) NIL;
    storage.replyParamSize = 0;
    storage.replyDataPtr = replyDataBuffer;
    storage.replyDataSize = replyDataSize;

    if (proc_MigDebugLevel > 4) {
	printf("Proc_DoRemoteCall: sending call %d home.\n", callNumber); 
    }

    for (numTries = 0; numTries < PROC_MAX_RPC_RETRIES; numTries++) {
	remoteCallStatus = Rpc_Call(procPtr->peerHostID, RPC_PROC_REMOTE_CALL,
				    &storage);
	if (remoteCallStatus != RPC_TIMEOUT) {
	    break;
	}
	status = Proc_WaitForHost(procPtr->peerHostID);
	if (status != SUCCESS) {
	    break;
	}
    }
    if (proc_MigDebugLevel > 4) {
	printf("Proc_DoRemoteCall: status %x returned by Rpc_Call.\n",
		   remoteCallStatus);
    }
    if (proc_DoTrace && proc_DoCallTrace) {
	record.flags &= ~PROC_MIGTRACE_START;
	record.info.call.status = remoteCallStatus;
	Trace_Insert(proc_TraceHdrPtr, PROC_MIGTRACE_CALL,
		     (ClientData) &record);
    }

    /*
     * If we were told the process no longer exists on the home node,
     * then blow it away.
     */
    if (remoteCallStatus == PROC_NO_PEER) {
	status = PROC_NO_PEER;
	(void) Sig_Send(SIG_KILL, (int) PROC_NO_PEER, procPtr->processID,
			FALSE); 
	goto failure;
    }
    /*
     * Step through the reply data buffer and copy out any arguments that
     * were output parameters.
     */

    ptr = replyDataBuffer;
    for (i = 0; i < numWords; i++) {
	disp = call.info[i].disposition;
	if (disp & SYS_PARAM_ACC) {
	    if ((disp & SYS_PARAM_OUT) && !(disp & SYS_PARAM_NIL)) {
		bcopy(ptr, pointerArray[i], call.info[i].size);
		ptr += Byte_AlignAddr(call.info[i].size);
	    }
	    if (numBytesAccessible[i] > 0) {
		Vm_MakeUnaccessible(pointerArray[i], numBytesAccessible[i]);
		numBytesAccessible[i] = 0;
	    }
	} else if ((disp & SYS_PARAM_COPY) && (disp & SYS_PARAM_OUT)
		   && !(disp & SYS_PARAM_NIL)) {
	    if (specsPtr[i].type != SYS_PARAM_PROC_ENV_VALUE) {
		size = call.info[i].size;
	    } else {
		/*
		 * Compute true size of the string, up to the call info limit.
		 */
		register char *charPtr;
		for (charPtr = (char *)ptr, size = 0;
		     (size < call.info[i].size) && (*charPtr != '\0');
		     charPtr++, size++) {
		}
		*charPtr = '\0';	/* null terminate */
		size++;			/* at most call.info[i].size */
	    }
	    status = Vm_CopyOut(size, ptr, (Address) argsPtr[i]);
	    if (status != SUCCESS) {
		if (proc_MigDebugLevel > 6) {
		    panic("Proc_DoRemoteCall: status %x returned by Vm_CopyOut.\n",
			       status);
		}

		status = SYS_ARG_NOACCESS;
		goto failure;
	    }
	    ptr += Byte_AlignAddr(call.info[i].size);
	}
    }

    free(dataBuffer);
    free(replyDataBuffer);
    
    return(remoteCallStatus);


    /*
     * Try to unwind after an error by making everything unaccessible
     * again and freeing memory before returning the error condition.
     * Note: if the procedure has progressed far enough for dataBuffer 
     * to be non_NIL, then replyDataBuffer should be non-NIL then as well.
     */

failure:

    for (i = 0; i < numWords; i++) {
	if (numBytesAccessible[i] > 0) {
	    Vm_MakeUnaccessible(pointerArray[i], numBytesAccessible[i]);
	}
    }
    if (dataBuffer != (Address) NIL) {
	free(dataBuffer);
	free(replyDataBuffer);
    }
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_RemoteDummy --
 *
 *	Dummy routine to return FAILURE for any system call not yet
 *	implemented.
 *
 * Results:
 *	FAILURE.
 *
 * Side effects:
 *	None.  
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
ReturnStatus
Proc_RemoteDummy(callNumber, numWords, argsPtr, specsPtr)
    int callNumber;
    int numWords;
    Sys_ArgArray *argsPtr;
    Sys_CallParam *specsPtr;
{
    printf("Warning: Call %d not yet implemented.\n", callNumber);
    return(FAILURE);
}


/*
 * ----------------------------------------------------------------------------
 *
 * ProcRemoteFork --
 *
 *	Perform an RPC to execute a fork on behalf of a migrated process.
 *
 * Results:
 *	The status returned by the RPC is returned.
 *
 * Side effects:
 *	An RPC is performed.  A skeletal process is created on the home node
 *	to be the "real" process corresponding to the migrated forked process
 *	that is in the process of being created.  The ID of the child on
 *	the home node is stored in the PCB of the remote process.
 *
 * ----------------------------------------------------------------------------
 */

ReturnStatus
ProcRemoteFork(parentProcPtr, childProcPtr)
    Proc_ControlBlock *parentProcPtr;	/* PCB for parent */
    Proc_ControlBlock *childProcPtr;		/* PCB for child */
{
    Rpc_Storage storage;
    Proc_RemoteCall call;
    ReturnStatus status;
    Proc_TraceRecord record;		/* used to store trace data */
    int numTries;			/* number of times trying RPC */

    if (proc_MigDebugLevel > 3) {
	printf("ProcRemoteFork called.\n");
    }

    status = Recov_IsHostDown(parentProcPtr->peerHostID);
    if (status != SUCCESS) {
	if (proc_MigDebugLevel > 0) {
	    printf("ProcRemoteFork: host %d is down; killing process %x.\n",
		       parentProcPtr->peerHostID, parentProcPtr->processID);
	}
	Proc_ExitInt(PROC_TERM_DESTROYED, (int) PROC_NO_PEER, 0);
	/*
	 * This point should not be reached, but the N-O-T-R-E-A-C-H-E-D
	 * directive causes a complaint when there's code after it.
	 */
	panic("ProcRemoteFork: Proc_ExitInt returned.\n");
	return(PROC_NO_PEER);
    }

    if (proc_DoTrace && proc_DoCallTrace) {
	record.processID = parentProcPtr->peerProcessID;
	record.flags = PROC_MIGTRACE_START;
	record.info.call.callNumber = SYS_PROC_FORK;
	Trace_Insert(proc_TraceHdrPtr, PROC_MIGTRACE_CALL,
		     (ClientData) &record);
    }

    /*
     * Set up for the RPC.
     */
    call.processID = parentProcPtr->peerProcessID;
    call.callNumber = SYS_PROC_FORK;
    call.parseArgs = FALSE;
    storage.requestParamPtr = (Address) &call;
    storage.requestParamSize = sizeof(Proc_RemoteCall);

    storage.requestDataPtr = (Address) &childProcPtr->processID;
    storage.requestDataSize = sizeof(int);

    storage.replyParamPtr = (Address) NIL;
    storage.replyParamSize = 0;
    storage.replyDataPtr = (Address) &childProcPtr->peerProcessID;
    storage.replyDataSize = sizeof(Proc_PID);

    for (numTries = 0; numTries < PROC_MAX_RPC_RETRIES; numTries++) {
	status = Rpc_Call(parentProcPtr->peerHostID, RPC_PROC_REMOTE_CALL,
			   &storage);
	if (status != RPC_TIMEOUT) {
	    break;
	}
	status = Proc_WaitForHost(parentProcPtr->peerHostID);
	if (status != SUCCESS) {
	    break;
	}
    }

    if (proc_DoTrace && proc_DoCallTrace) {
	record.flags &= ~PROC_MIGTRACE_START;
	record.info.call.status = status;
	Trace_Insert(proc_TraceHdrPtr, PROC_MIGTRACE_CALL,
		     (ClientData) &record);
    }

    /*
     * If we were told the process no longer exists on the home node,
     * then blow it away.  Any other return status just means we didn't
     * initialize the child properly.
     */
    if (status != SUCCESS) {
	if (status == PROC_NO_PEER) {
	    (void) Sig_Send(SIG_KILL, (int) PROC_NO_PEER,
			    parentProcPtr->processID, FALSE); 
	}
	if (proc_MigDebugLevel > 0) {
	    printf("Warning: ProcRemoteFork returning status %x.\n",
		       status);
	}
	return(status);
    }
    childProcPtr->peerHostID = parentProcPtr->peerHostID;
    childProcPtr->genFlags |= PROC_FOREIGN;
    childProcPtr->kcallTable = mach_MigratedHandlers;

    /*
     * Note the dependency of the new process on the other host.
     */
    Proc_AddMigDependency(childProcPtr->processID, childProcPtr->peerHostID);

    if (proc_MigDebugLevel > 3) {
	printf("ProcRemoteFork returning status %x.\n", status);
    }
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * ProcRemoteExit --
 *
 *	Cause a migrated process to exit.  Throw away its address space, but
 * 	migrate runtime information back to the home node.  
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A remote procedure call is performed and the part of the
 * 	process state that survives past Proc_Exit is transferred.
 *
 *----------------------------------------------------------------------
 */

void
ProcRemoteExit(procPtr, reason, exitStatus, code)
    register Proc_ControlBlock 	*procPtr;  /* process that is exiting */
    int reason;	/* Why the process is dying: EXITED, SIGNALED, DESTROYED  */
    int	exitStatus;	/* Exit status or signal # or destroy status */
    int code;	/* Signal sub-status */
{
    Address buffer;
    Address ptr;
    int bufferSize;
    Rpc_Storage storage;
    Proc_RemoteCall call;
    ReturnStatus status;
    Proc_TraceRecord record;		/* used to store trace data */
    int numTries;			/* number of times trying RPC */

    if (proc_MigDebugLevel > 4) {
	printf("ProcRemoteExit(%x) called.\n", exitStatus);
    }

    /*
     * Remove the dependency on the other host.
     */
    Proc_RemoveMigDependency(procPtr->processID);

    status = Recov_IsHostDown(procPtr->peerHostID);
    if (status != SUCCESS) {
	if (proc_MigDebugLevel > 0) {
	    printf("ProcRemoteExit: host %d is down; ignoring exit for process %x.\n",
		       procPtr->peerHostID, procPtr->processID);
	}
	return;
    }

    if (proc_DoTrace && proc_DoCallTrace) {
	record.processID = procPtr->processID;
	record.flags = PROC_MIGTRACE_START;
	record.info.call.callNumber = SYS_PROC_EXIT;
	Trace_Insert(proc_TraceHdrPtr, PROC_MIGTRACE_CALL,
		     (ClientData) &record);
    }


    bufferSize = 2 * sizeof(Timer_Ticks) + 5 * sizeof(int);
    buffer = (Address) malloc(bufferSize);

    ptr = buffer;
    Byte_FillBuffer(ptr, Timer_Ticks,  procPtr->kernelCpuUsage.ticks);
    Byte_FillBuffer(ptr, Timer_Ticks,  procPtr->userCpuUsage.ticks);
    Byte_FillBuffer(ptr, int,  procPtr->numQuantumEnds);
    Byte_FillBuffer(ptr, int,  procPtr->numWaitEvents);
    Byte_FillBuffer(ptr, int, reason);
    Byte_FillBuffer(ptr, int, exitStatus);
    Byte_FillBuffer(ptr, int, code);


    /*
     * Set up for the RPC.
     */
    call.processID = procPtr->peerProcessID;
    call.callNumber = SYS_PROC_EXIT;
    call.parseArgs = FALSE;
    storage.requestParamPtr = (Address) &call;
    storage.requestParamSize = sizeof(Proc_RemoteCall);

    storage.requestDataPtr = buffer;
    storage.requestDataSize = bufferSize;

    storage.replyParamPtr = (Address) NIL;
    storage.replyParamSize = 0;
    storage.replyDataPtr = (Address) NIL;
    storage.replyDataSize = 0;


    for (numTries = 0; numTries < PROC_MAX_RPC_RETRIES; numTries++) {
	status = Rpc_Call(procPtr->peerHostID, RPC_PROC_REMOTE_CALL, &storage);
	if (status != RPC_TIMEOUT) {
	    break;
	}
	status = Proc_WaitForHost(procPtr->peerHostID);
	if (status != SUCCESS) {
	    break;
	}
    }

    if (proc_DoTrace && proc_DoCallTrace) {
	record.flags &= ~PROC_MIGTRACE_START;
	record.info.call.status = status;
	Trace_Insert(proc_TraceHdrPtr, PROC_MIGTRACE_CALL,
		     (ClientData) &record);
    }

    free(buffer);

    if ((status != SUCCESS) && (proc_MigDebugLevel > 0)) {
	printf("Warning: ProcRemoteExit received status %x.\n", status);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * ProcRemoteExec --
 *
 *	Tell the home node of a process that it has done an exec, and to
 * 	change any information it might have about effective IDs.  
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A remote procedure call is performed.
 *
 *----------------------------------------------------------------------
 */

void
ProcRemoteExec(procPtr, uid)
    register Proc_ControlBlock 	*procPtr;  /* process that is doing exec */
    int uid;				   /* new effective user ID, or -1 */
{
    Address buffer;
    Address ptr;
    int bufferSize;
    Rpc_Storage storage;
    Proc_RemoteCall call;
    ReturnStatus status;
    Proc_TraceRecord record;		/* used to store trace data */
    int numTries;			/* number of times trying RPC */

    if (proc_MigDebugLevel > 4) {
	printf("ProcRemoteExec(%s) called.\n", procPtr->argString);
    }

    status = Recov_IsHostDown(procPtr->peerHostID);
    if (status != SUCCESS) {
	if (proc_MigDebugLevel > 0) {
	    printf("ProcRemoteExec: host %d is down; killing process %x.\n",
		       procPtr->peerHostID, procPtr->processID);
	}
	Proc_Unlock(procPtr);
	Proc_ExitInt(PROC_TERM_DESTROYED, (int) PROC_NO_PEER, 0);
	/*
	 * This point should not be reached, but the N-O-T-R-E-A-C-H-E-D
	 * directive causes a complaint when there's code after it.
	 */
	panic("ProcRemoteExec: Proc_ExitInt returned.\n");
	return;
    }


    if (proc_DoTrace && proc_DoCallTrace) {
	record.processID = procPtr->processID;
	record.flags = PROC_MIGTRACE_START;
	record.info.call.callNumber = SYS_PROC_EXEC;
	Trace_Insert(proc_TraceHdrPtr, PROC_MIGTRACE_CALL,
		     (ClientData) &record);
    }


    bufferSize = sizeof(int) + strlen(procPtr->argString) + 1;
    buffer = (Address) malloc(bufferSize);

    ptr = buffer;
    Byte_FillBuffer(ptr, int,  uid);
    strcpy(ptr,  procPtr->argString);


    /*
     * Set up for the RPC.
     */
    call.processID = procPtr->peerProcessID;
    call.callNumber = SYS_PROC_EXEC;
    call.parseArgs = FALSE;
    storage.requestParamPtr = (Address) &call;
    storage.requestParamSize = sizeof(Proc_RemoteCall);

    storage.requestDataPtr = buffer;
    storage.requestDataSize = bufferSize;

    storage.replyParamPtr = (Address) NIL;
    storage.replyParamSize = 0;
    storage.replyDataPtr = (Address) NIL;
    storage.replyDataSize = 0;


    /*
     * Unlock the process while we're doing the RPC.
     */
    Proc_Unlock(procPtr);
    
    for (numTries = 0; numTries < PROC_MAX_RPC_RETRIES; numTries++) {
	status = Rpc_Call(procPtr->peerHostID, RPC_PROC_REMOTE_CALL, &storage);
	if (status != RPC_TIMEOUT) {
	    break;
	}
	status = Proc_WaitForHost(procPtr->peerHostID);
	if (status != SUCCESS) {
	    break;
	}
    }

    /*
     * Give the process back the way it was handed to us (locked).
     */
    Proc_Lock(procPtr);

    if (proc_DoTrace && proc_DoCallTrace) {
	record.flags &= ~PROC_MIGTRACE_START;
	record.info.call.status = status;
	Trace_Insert(proc_TraceHdrPtr, PROC_MIGTRACE_CALL,
		     (ClientData) &record);
    }

    free(buffer);

    if ((status != SUCCESS) && (proc_MigDebugLevel > 0)) {
	printf("Warning: ProcRemoteExec received status %x.\n", status);
    }
}

