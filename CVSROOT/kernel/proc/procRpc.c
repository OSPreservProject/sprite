/* 
 * procRpc.c --
 *
 *	Procedures to handle remote procedure calls on behalf of migrated
 *	processes.
 *
 * Copyright (C) 1986, 1988 Regents of the University of California
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
#include "fs.h"
#include "proc.h"
#include "procInt.h"
#include "procMigrate.h"
#include "migrate.h"
#include "byte.h"
#include "mem.h"
#include "sig.h"
#include "vm.h"
#include "sys.h"
#include "rpc.h"
#include "timer.h"
#include "sched.h"
#include "sync.h"
#include "sysSysCall.h"
#include "sysSysCallParam.h"
#include "devVid.h"
#include "prof.h"
#include "string.h"

static ReturnStatus RpcProcExit();
static ReturnStatus RpcProcFork();
static ReturnStatus RpcFsOpenStream();

typedef ReturnStatus ((*PRS) ());

/*
 * When a migrated system call is handled, a local procedure is called
 * to perform the system call.  If the procedure has side effects,
 * then before the results are returned, the process state on the
 * remote node needs to be updated.  (This will be the case for
 * routines such as Proc_SetIDs or Fs_Open.)  The "CallBack" type
 * defines a structure containing pointers to two procedures.
 *
 * For each system call, there are several possibilities for procedures to
 * be invoked:
 *
 *	1) The call should never be migrated (or has not been implemented).
 *	   This is a fatal error and is identified by localPtr being "RSNIL".
 *	2) The call is a typical call that transfers data but has no side
 *	   effects.  localPtr is set, but sideEffectsPtr is RSNIL.
 *	3) The call has side effects.  localPtr and sideEffectsPtr are non-NIL.
 */

typedef struct {
    PRS localPtr;		/* procedure to process call locally */
    PRS sideEffectsPtr;		/* procedure to update info on remote node */
} CallBack;

#define RSNIL ((PRS) NIL)

static CallBack callBackVector[] = {
/*
 *     callPtr	  (why)		sideEffectsPtr	call number    
 */
    { RpcProcFork,			RSNIL }, /* SYS_PROC_FORK	   0 */
    { RSNIL /* Proc_Exec */,		RSNIL }, /* SYS_PROC_EXEC	   1 */
    { RpcProcExit,			RSNIL }, /* SYS_PROC_EXIT	   2 */
    { RSNIL /* Not migrated */,		RSNIL }, /* SYS_SYNC_WAITTIME	   3 */
    { RSNIL /* Not migrated */,		RSNIL }, /* SYS_TEST_PRINTOUT	   4 */
    { RSNIL /* Not migrated */,		RSNIL }, /* SYS_TEST_GETLINE	   5 */
    { RSNIL /* Not migrated */,		RSNIL }, /* SYS_TEST_GETCHAR	   6 */
    { RSNIL /* Not migrated */,		RSNIL }, /* SYS_FS_OPEN		   7 */
    { RSNIL /* Not migrated */,		RSNIL }, /* SYS_FS_READ		   8 */
    { RSNIL /* Not migrated */,		RSNIL }, /* SYS_FS_WRITE	   9 */
    { RSNIL /* Not migrated */,		RSNIL }, /* SYS_FS_CLOSE	  10 */
    { RSNIL /* Not migrated */,		RSNIL }, /* SYS_FS_REMOVE	  11 */
    { RSNIL /* Not migrated */,		RSNIL }, /* SYS_FS_REMOVE_DIR	  12 */
    { RSNIL /* Not migrated */,		RSNIL }, /* SYS_FS_MAKE_DIR	  13 */
    { RSNIL /* Not migrated */,		RSNIL }, /* SYS_FS_CHANGE_DIR	  14 */
    { RSNIL /* Proc_Wait */,		RSNIL }, /* SYS_PROC_WAIT	  15 */
    { Proc_Detach,			RSNIL }, /* SYS_PROC_DETACH	  16 */
    { Proc_GetIDs,			RSNIL }, /* SYS_PROC_GETIDS	  17 */
    { Proc_SetIDs,			RSNIL }, /* SYS_PROC_SETIDS	  18 */
    { RSNIL /* Not migrated */,		RSNIL }, /* SYS_PROC_GETGROUPIDS  19 */
    { RSNIL /* Proc_SetGroupIDs */,	RSNIL }, /* SYS_PROC_SETGROUPIDS  20 */
    { Proc_GetFamilyID,			RSNIL }, /* SYS_PROC_GETFAMILYID  21 */
    { Proc_SetFamilyID,			RSNIL }, /* SYS_PROC_SETFAMILYID  22 */
    { RSNIL /* Not migrated */,		RSNIL }, /* SYS_TEST_RPC	  23 */
    { RSNIL /* Not migrated */,		RSNIL }, /* SYS_TEST_STATS	  24 */
    { RSNIL /* Not migrated */,		RSNIL }, /* SYS_VM_CREATEVA	  25 */
    { RSNIL /* Not migrated */,		RSNIL }, /* SYS_VM_DESTROYVA      26 */
    { Sig_UserSend,			RSNIL }, /* SYS_SIG_SEND	  27 */
    { RSNIL /* Sig_Pause */,		RSNIL }, /* SYS_SIG_PAUSE	  28 */
    { RSNIL /* Sig_SetHoldMask */,	RSNIL }, /* SYS_SIG_SETHOLDMASK   29 */
    { RSNIL /* Sig_SetAction */,	RSNIL }, /* SYS_SIG_SETACTION     30 */
    { Prof_Start,			RSNIL }, /* SYS_PROF_START	  31 */
    { Prof_End,				RSNIL }, /* SYS_PROF_END	  32 */
    { Prof_DumpStub,			RSNIL }, /* SYS_PROF_DUMP	  33 */
    { Vm_Cmd,				RSNIL }, /* SYS_VM_CMD	          34 */
    { Sys_GetTimeOfDay,			RSNIL }, /* SYS_SYS_GETTIMEOFDAY  35 */
    { Sys_SetTimeOfDay,			RSNIL }, /* SYS_SYS_SETTIMEOFDAY  36 */
    { RSNIL /* Not migrated */,		RSNIL }, /* SYS_SYS_DONOTHING     37 */
    { Proc_GetPCBInfo,			RSNIL }, /* SYS_PROC_GETPCBINFO   38 */
    { RSNIL /* Vm_GetSegInfo */,	RSNIL }, /* SYS_VM_GETSEGINFO     39 */
    { Proc_GetResUsage,			RSNIL }, /* SYS_PROC_GETRESUSAGE  40 */
    { Proc_GetPriority,			RSNIL }, /* SYS_PROC_GETPRIORITY  41 */
    { Proc_SetPriority,			RSNIL }, /* SYS_PROC_SETPRIORITY  42 */
    { Proc_Debug,			RSNIL }, /* SYS_PROC_DEBUG	  43 */
    { RSNIL /* Not implemented */,	RSNIL }, /* SYS_PROC_PROFILE      44 */
    { RSNIL /* obsolete */,		RSNIL }, /* SYS_FS_TRUNC	  45 */
    { RSNIL /* obsolete */,		RSNIL }, /* SYS_FS_TRUNC_ID	  46 */
    { RSNIL /* Not migrated */,		RSNIL }, /* SYS_FS_GET_NEW_ID	  47 */
    { RSNIL /* Not migrated */,		RSNIL }, /* SYS_FS_GET_ATTRIBUTES 48 */
    { RSNIL /* Not migrated */,		RSNIL }, /* SYS_FS_GET_ATTR_ID	  49 */
    { RSNIL /* Not migrated */,		RSNIL }, /* SYS_FS_SET_ATTRIBUTES 50 */
    { RSNIL /* Not migrated */,		RSNIL }, /* SYS_FS_SET_ATTR_ID	  51 */
    { RSNIL /* Not migrated */,		RSNIL }, /* SYS_FS_SET_DEF_PERM	  52 */
    { RSNIL /* Not migrated */,		RSNIL }, /* SYS_FS_IO_CONTROL	  53 */
    { Dev_VidEnable,			RSNIL }, /* SYS_SYS_ENABLEDISPLAY 54 */
    { RSNIL /* obsolete */,		RSNIL }, /* SYS_PROC_SET_ENVIRON  55 */
    { RSNIL /* obsolete */,		RSNIL }, /* SYS_PROC_UNSET_ENVIRON 56 */
    { RSNIL /* obsolete */,		RSNIL }, /* SYS_PROC_GET_ENVIRON_VAR 57 */
    { RSNIL /* obsolete */, 		RSNIL }, /* SYS_PROC_GET_ENVIRON_RANGE 58 */
    { RSNIL /* obsolete */,  		RSNIL }, /* SYS_PROC_INSTALL_ENVIRON 59 */
    { RSNIL /* obsolete */,		RSNIL }, /* SYS_PROC_COPY_ENVIRON 60 */
    { RSNIL /* Not migrated */,		RSNIL }, /* SYS_SYNC_SLOWLOCK 	  61 */
    { RSNIL /* Not migrated */,		RSNIL }, /* SYS_SYNC_SLOWWAIT     62 */
    { RSNIL /* Not migrated */,		RSNIL }, /* SYS_SYNC_SLOWBROADCAST 63 */
    { RSNIL /* Not migrated */,		RSNIL }, /* SYS_VM_PAGESIZE       64 */
    { RSNIL /* Not migrated */,		RSNIL }, /* SYS_FS_HARDLINK       65 */
    { RSNIL /* Not migrated */,		RSNIL }, /* SYS_FS_RENAME         66 */
    { RSNIL /* Not migrated */,		RSNIL }, /* SYS_FS_SYMLINK        67 */
    { RSNIL /* Not migrated */,		RSNIL }, /* SYS_FS_READLINK 	  68 */
    { RSNIL /* Not migrated */,		RSNIL }, /* SYS_FS_CREATEPIPE     69 */
    { RSNIL /* Not migrated */,		RSNIL }, /* SYS_VM_MAPKERNELINTOUSER 70 */
    { Fs_AttachDiskStub ,		RSNIL }, /* SYS_FS_ATTACH_DISK    71 */
    { RSNIL /* Not migrated */,		RSNIL }, /* SYS_FS_SELECT         72 */
    { Sys_Shutdown,			RSNIL }, /* SYS_SYS_SHUTDOWN	  73 */
    { Proc_Migrate,			RSNIL }, /* SYS_PROC_MIGRATE	  74 */
    { Fs_MakeDeviceStub,		RSNIL }, /* SYS_FS_MAKE_DEVICE    75 */
    { Fs_CommandStub,			RSNIL }, /* SYS_FS_COMMAND        76 */
    { RSNIL /* obsolete */,		RSNIL }, /* SYS_FS_LOCK		  77 */
    { Sys_GetMachineInfo,		RSNIL }, /* SYS_GETMACHINEINFO    78 */
    { RSNIL /* Not migrated */,		RSNIL }, /* SYS_NET_INSTALL_ROUTE 79 */
    { RSNIL /* Not migrated */,		RSNIL }, /* SYS_FS_READVECTOR	  80 */
    { RSNIL /* Not migrated */,		RSNIL }, /* SYS_FS_WRITEVECTOR	  81 */
    { RSNIL /* Not migrated */,		RSNIL }, /* SYS_FS_CHECKACCESS    82 */
    { RSNIL /* Not migrated */,		RSNIL }, /* SYS_PROC_GETINTERVALTIMER  83 */
    { RSNIL /* Not migrated */,		RSNIL }, /* SYS_PROC_SETINTERVALTIMER  84 */
    { RSNIL /* Not migrated */,		RSNIL }, /* SYS_FS_WRITEBACKID    85 */
    { RSNIL /* Not migrated */,		RSNIL }, /* SYS_PROC_EXEC_ENV     86 */

};


/*
 *----------------------------------------------------------------------
 *
 * Proc_RpcRemoteCall --
 *
 *	Service a system call for a migrated process.  Call the rpc
 *	module to return the results, if any.
 * 
 * Results:
 *	Any data retured by the system call is passed back in a buffer,
 *	as is its length.  The result returned by the system call is
 *	returned as the ReturnStatus for this procedure.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
ReturnStatus 
Proc_RpcRemoteCall(callPtr, dataPtr, dataLength, replyDataPtr,
		   replyDataLengthPtr)
    Proc_RemoteCall *callPtr;
    Address dataPtr;
    int dataLength;
    Address *replyDataPtr;
    int *replyDataLengthPtr;
{
    register Proc_ControlBlock *procPtr;
    ReturnStatus status;
    Sys_ArgArray args;
    int disp;
    int size;
    int paddedSize;
    Address inputPtr;
    Address outputPtr;
    int i;
    Proc_TraceRecord record;

    if (proc_MigDebugLevel > 4) {
	Sys_Printf("Proc_RpcRemoteCall(%d) called.\n", callPtr->callNumber);
    }

    *replyDataLengthPtr = 0;
    *replyDataPtr = (Address) NIL;

    procPtr = Proc_LockPID(callPtr->processID);
    if (procPtr == (Proc_ControlBlock *) NIL
	|| ((procPtr->state != PROC_MIGRATED) &&
	    !(procPtr->genFlags & PROC_MIGRATION_DONE)))   {
 	Sys_Panic(SYS_FATAL,
		  "Proc_RpcRemoteCall: invalid pid: %x.\n", callPtr->processID);
	if (procPtr != (Proc_ControlBlock *) NIL) {
	    Proc_Unlock(procPtr);
	}
	return (PROC_INVALID_PID);
    }
    Proc_Unlock(procPtr);
    if (callBackVector[callPtr->callNumber].localPtr ==
	    (ReturnStatus (*) ()) NIL) {
 	Sys_Panic(SYS_FATAL,
		  "Proc_RpcRemoteCall: can't handle call %d.\n",
		  callPtr->callNumber);
	return(SYS_INVALID_SYSTEM_CALL);
    }

    /*
     * Set the effective process for this processor.
     */

    Proc_SetEffectiveProc(procPtr);

    if (proc_DoTrace && proc_DoCallTrace) {
	record.processID = callPtr->processID;
	record.flags = PROC_MIGTRACE_START | PROC_MIGTRACE_HOME;
	record.info.call.callNumber = callPtr->callNumber;
	Trace_Insert(proc_TraceHdrPtr, PROC_MIGTRACE_CALL,
		     (ClientData *) &record);
    }
    
    if (!callPtr->parseArgs) {
	status = (*(callBackVector[callPtr->callNumber].localPtr))
		(procPtr, dataPtr, dataLength, replyDataPtr,
		 replyDataLengthPtr);
	if (proc_MigDebugLevel > 4) {
	    Sys_Printf("Proc_RpcRemoteCall: unparsed call %d returned %x.\n",
		       callPtr->callNumber, status);
	}

	if (proc_DoTrace && proc_DoCallTrace) {
	    record.info.call.status = status;
	    record.flags &= ~PROC_MIGTRACE_START;
	    Trace_Insert(proc_TraceHdrPtr, PROC_MIGTRACE_CALL,
			 (ClientData *) &record);
	}
	return(status);
    }

    *replyDataPtr = Mem_Alloc(callPtr->replySize);
    *replyDataLengthPtr = callPtr->replySize;
    outputPtr = *replyDataPtr;
    inputPtr = dataPtr;

    for (i = 0; i < callPtr->numArgs; i++) {
	disp = callPtr->info[i].disposition;
	size = callPtr->info[i].size;
	paddedSize = Byte_AlignAddr(size);
	if (disp & (SYS_PARAM_ACC | SYS_PARAM_COPY)) {
	    if (disp & SYS_PARAM_NIL) {
		args.argArray[i] = USER_NIL;
	    } else if (disp & SYS_PARAM_OUT) {
		if (disp & SYS_PARAM_IN) {
		    Byte_Copy(size, inputPtr, outputPtr);
		    inputPtr += paddedSize;
		}
		args.argArray[i] = (int) outputPtr;
		outputPtr += paddedSize;
	    } else {
		args.argArray[i] = (int) inputPtr;
		inputPtr += paddedSize;
	    }
	} else {
	    if (size != sizeof(int)) {
		Sys_Panic(SYS_FATAL, "Proc_RpcRemoteCall: size mismatch.\n");
	    }
	    args.argArray[i] = * ((int *)inputPtr);
	    inputPtr += sizeof(int);
	}
    }

    status = (*(callBackVector[callPtr->callNumber].localPtr)) (args);
    if (proc_MigDebugLevel > 4) {
 	Sys_Printf("Proc_RpcRemoteCall: parsed call %d returned %x.\n",
		   callPtr->callNumber, status);
    }
    Proc_SetEffectiveProc((Proc_ControlBlock *) NIL);

    if (proc_DoTrace && proc_DoCallTrace) {
	record.info.call.status = status;
	record.flags &= ~PROC_MIGTRACE_START;
	Trace_Insert(proc_TraceHdrPtr, PROC_MIGTRACE_CALL,
		     (ClientData *) &record);
    }

    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * RpcProcExit --
 *
 *	Perform an exit on the home node of a process. 
 * 
 * Results:
 *	SUCCESS 		- the process exited successfully.
 *	PROC_INVALID_PID 	- the process does not exist.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
static ReturnStatus 
RpcProcExit(procPtr, dataPtr, dataLength, replyDataPtr,
		   replyDataLengthPtr)
    register Proc_ControlBlock *procPtr;
    Address dataPtr;
    int dataLength;
    Address *replyDataPtr;
    int *replyDataLengthPtr;
{
    Timer_Ticks ticks;
    int count;
    int reason;	/* Why the process is dying: EXITED, SIGNALED, DESTROYED  */
    int	status;	/* Exit status or signal # or destroy status */
    int code;	/* Signal sub-status */

    if (proc_MigDebugLevel > 4) {
	Sys_Printf("RpcProcExit called.\n");
    }

    Proc_Lock(procPtr);
    Byte_EmptyBuffer(dataPtr, Timer_Ticks,  ticks);
    Timer_AddTicks(procPtr->kernelCpuUsage, ticks, &procPtr->kernelCpuUsage); 
    Byte_EmptyBuffer(dataPtr, Timer_Ticks,  ticks);
    Timer_AddTicks(procPtr->kernelCpuUsage, ticks, &procPtr->userCpuUsage); 
    Byte_EmptyBuffer(dataPtr, int,  count);
    procPtr->numQuantumEnds += count;
    Byte_EmptyBuffer(dataPtr, int, count);
    procPtr->numWaitEvents += count;

    Byte_EmptyBuffer(dataPtr, int, reason);
    Byte_EmptyBuffer(dataPtr, int, status);
    Byte_EmptyBuffer(dataPtr, int, code);

    Proc_Unlock(procPtr);
    ProcExitProcess(procPtr, reason, status, code, FALSE);
    if (proc_MigDebugLevel > 4) {
	Sys_Printf("RpcProcExit returning SUCCESS.\n");
    }
    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * RpcProcFork --
 *
 *	Perform a fork on the home node of a process.  This involves allocating
 *	a PCB and process ID for the process, and copying certain local
 *	information such as the environment.
 *
 * Results:
 *	SUCCESS 		- the process forked successfully.
 *	PROC_INVALID_PID 	- the process does not exist.
 *
 * Side effects:
 *	A new process is created.  The identifier of the newly-created
 *	process is returned to the remote node in the data buffer.  
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
static ReturnStatus 
RpcProcFork(parentProcPtr, dataPtr, dataLength, replyDataPtr,
	    replyDataLengthPtr)
    register Proc_ControlBlock *parentProcPtr;
    Address dataPtr;
    int dataLength;
    Address *replyDataPtr;
    int *replyDataLengthPtr;
{
    Proc_ControlBlock 	*childProcPtr;	/* The new process being created */

    if (proc_MigDebugLevel > 3) {
	Sys_Printf("RpcProcFork called.\n");
    }
    childProcPtr = ProcGetUnusedPCB();
    childProcPtr->state 		= PROC_MIGRATED;
    childProcPtr->genFlags 		= PROC_USER | PROC_NO_VM;
    childProcPtr->syncFlags		= 0;
    childProcPtr->schedFlags		= 0;
    childProcPtr->exitFlags		= 0;

    childProcPtr->billingRate 	= parentProcPtr->billingRate;
    childProcPtr->recentUsage 	= 0;
    childProcPtr->weightedUsage 	= 0;
    childProcPtr->unweightedUsage 	= 0;

    childProcPtr->kernelCpuUsage 	= timer_TicksZeroSeconds;
    childProcPtr->userCpuUsage 		= timer_TicksZeroSeconds;
    childProcPtr->childKernelCpuUsage 	= timer_TicksZeroSeconds;
    childProcPtr->childUserCpuUsage 	= timer_TicksZeroSeconds;
    childProcPtr->numQuantumEnds	= 0;
    childProcPtr->numWaitEvents	= 0;

    childProcPtr->parentID 		= parentProcPtr->processID;
    childProcPtr->userID 		= parentProcPtr->userID;
    childProcPtr->effectiveUserID 	= parentProcPtr->effectiveUserID;
    childProcPtr->familyID 		= parentProcPtr->familyID;

    Vm_ProcInit(childProcPtr);

    if (ProcFamilyInsert(childProcPtr, childProcPtr->familyID) != SUCCESS) {
	Sys_Panic(SYS_FATAL, "RpcProcFork: ProcFamilyInsert failed\n");
    }

    /*
     *  Initialize our child list to remove any old links.
     *  Insert this PCB entry into the list of children of our parent.
     */
    List_Init((List_Links *) childProcPtr->childList);
    List_Insert((List_Links *) &(childProcPtr->siblingElement), 
		LIST_ATREAR(parentProcPtr->childList));

    Sig_Fork(parentProcPtr, childProcPtr);

    /*
     * Initialize information for migration.
     */

    Byte_EmptyBuffer(dataPtr, Proc_PID, childProcPtr->peerProcessID);
    childProcPtr->peerHostID = parentProcPtr->peerHostID;
    
    /*
     * Have the new process inherit filesystem state.
     */
    Fs_InheritState(parentProcPtr, childProcPtr);

    /*
     * Send back the child's process ID in the replyData buffer.
     */
    
    *replyDataLengthPtr = sizeof(Proc_PID);
    *replyDataPtr = Mem_Alloc(sizeof(Proc_PID));
    * ((Proc_PID *) *replyDataPtr) = childProcPtr->processID;
    
    if (proc_MigDebugLevel > 3) {
	Sys_Printf("RpcProcFork returning SUCCESS.\n");
    }
    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_ByteCopy --
 *
 *	Perform a copy into or out of the user's address space.  If
 *	the user process is migrated, copy the arguments directly and
 *	let the rpc server send the results to the remote node.
 * 
 * Results:
 *	SUCCESS 		- the copy was performed successfully.
 *	SYS_ARG_NOACCESS 	- the argument was not accessible.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Proc_ByteCopy(copyIn, numBytes, sourcePtr, destPtr)
    Boolean copyIn;		/* copy into or out of kernel address space */
    int numBytes;		/* The number of bytes to copy */
    Address sourcePtr;		/* Where to copy from */
    Address destPtr;		/* Where to copy to */
{
    ReturnStatus status = SUCCESS;
    
    if (Proc_IsMigratedProcess()) {
	Byte_Copy(numBytes, sourcePtr, destPtr);
    } else if (copyIn) {
	status = Vm_CopyIn(numBytes, sourcePtr, destPtr);
    } else {
	status = Vm_CopyOut(numBytes, sourcePtr, destPtr);
    }
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_StringNCopy --
 *
 *	Combine the operations of String_NCopy and String_NLength.  Used
 *	by file system stubs when the file system operation is being done
 *	by an rpc server.  This routine is a parallel routine to 
 *	Vm_StringNCopy which is used for file system operations for 
 *	non-migrated processes.
 * 
 * Results:
 *	SUCCESS 
 *
 * Side effects:
 *	*strNameLength is set to the actual length of the string copied.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Proc_StringNCopy(numBytes, srcStr, destStr, strLengthPtr)
    int		numBytes;		/* Maximum number of bytes to copy. */
    char	*srcStr;		/* String to copy. */
    char	*destStr;		/* Where to copy string to. */
    int		*strLengthPtr;		/* Where to return actual length of 
					 * string copied. */
{
    *strLengthPtr = String_NLength(numBytes, srcStr);
    (void) String_NCopy(numBytes, srcStr, destStr);
    return(SUCCESS);
}



/*
 *----------------------------------------------------------------------
 *
 * Proc_MakeStringAccessible --
 *
 *	If the string is in user space, convert the user space path pointer
 *	into a kernel valid pointer.  If the string is in kernel space
 *	already (i.e., a system call is being performed on behalf of a
 *	migrated process), do nothing.
 *
 *
 * Results:
 *	SYS_ARG_NOACCESS    - the Vm_MakeAccessible call failed.
 *	PROC_INVALID_STRING - the length of the string exceeds the maximum
 *            		      permissible length or is not null-terminated.
 *
 *	The new pointer is returned in *stringPtrPtr.  The length of the
 *	string is returned in *newLengthPtr if newLengthPtr is non-NIL.
 *
 * Side effects:
 *	Make an area of the user code accessible.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Proc_MakeStringAccessible(maxLength, stringPtrPtr, accessLengthPtr, 
			  newLengthPtr)
    int maxLength;	 /* Maximum allowable string length */
    char **stringPtrPtr; /* On input *stringPtrPtr is a user space pointer,
			  * On output *stringPtrPtr is a kernel space pointer */
    int *accessLengthPtr;/* Number of bytes actually made accessible. */
    int *newLengthPtr;	 /* length of the string that is made accessible */
{
    int accessLength = maxLength;
    int realLength;
    Boolean madeAccessible = FALSE;

    if (!Proc_IsMigratedProcess()) {
	Vm_MakeAccessible(VM_READONLY_ACCESS, 
			  maxLength, (Address) *stringPtrPtr,
			  &accessLength, (Address *) stringPtrPtr);
	if (*stringPtrPtr == (Address)NIL) {
	    return(SYS_ARG_NOACCESS);
	}
	madeAccessible = TRUE;
	*accessLengthPtr = accessLength;
    }
    /*
     * Do a length check.  If the file name takes up all the addressable
     * space then it isn't null terminated and will cause problems later.
     */
    realLength = String_NLength(accessLength, *stringPtrPtr);
    if (realLength == accessLength) {
	if (madeAccessible) {
	    Vm_MakeUnaccessible((Address) *stringPtrPtr, accessLength);
	}
	*stringPtrPtr = (Address)NIL;
	return(PROC_INVALID_STRING);
    }
    if (newLengthPtr != (int *) NIL) {
	*newLengthPtr = realLength;
    }
    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_MakeUnaccessible
 *
 *	If the pointer is to something that was made accessible as a
 *	result of a local system call, call Vm_MakeUnaccessible.  If
 *	the system call was migrated, do nothing since the address was
 *	never made accessible in the first place.  This routine is
 *	used by generic system call stubs that may be called either locally
 *	or on behalf of a migrated process.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Kernel page table may be modified.
 *
 *----------------------------------------------------------------------
 */
void
Proc_MakeUnaccessible(addr, numBytes)
    Address		addr;
    int			numBytes;
{

    if (!Proc_IsMigratedProcess()) {
	Vm_MakeUnaccessible(addr, numBytes);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_RpcRemoteWait --
 *
 *	Perform a Proc_Wait on behalf of a migrated process.
 *
 * Results:
 *	SUCCESS - an exiting/detached child was found and the information
 *		  about it is being returned.
 *	PROC_NO_EXITS - No children have performed an exit or detach
 *		        (except possibly children about whom the parent has
 *			already been notified).
 *
 * Side effects:
 *	If the process has no exiting children and it has specified that
 *	it should block until a child exits, the process is established
 *	as performing a remote wait.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
ReturnStatus
Proc_RpcRemoteWait(srvToken, clientID, command, storagePtr)
    ClientData srvToken;	/* Handle on server process passed to
				 * Rpc_Reply */
    int clientID;		/* Sprite ID of client host */
    int command;		/* Command identifier */
    Rpc_Storage *storagePtr;	/* The request fields refer to the request
				 * buffers and also indicate the exact amount
				 * of data in the request buffers.  The reply
				 * fields are initialized to NIL for the
				 * pointers and 0 for the lengths.  This can
				 * be passed to Rpc_Reply */
{
    ReturnStatus status;
    ProcChildInfo *childInfoPtr;
    register ProcRemoteWaitCmd *cmdPtr;
    Proc_PID *pidArray;
    Rpc_ReplyMem *replyMemPtr;
    Proc_ControlBlock *procPtr;

    if (proc_MigDebugLevel > 3) {
	Sys_Printf("Proc_RpcRemoteWait entered.\n");

    }

    cmdPtr = (ProcRemoteWaitCmd *) storagePtr->requestParamPtr;
    procPtr = Proc_LockPID(cmdPtr->pid);
    if (procPtr == (Proc_ControlBlock *) NIL) {
	return (PROC_INVALID_PID);
    } else if (procPtr->state != PROC_MIGRATED ||
	    procPtr->peerHostID != clientID) {
	Proc_Unlock(procPtr);
	return (PROC_INVALID_PID);
    } 
    pidArray = (Proc_PID *) storagePtr->requestDataPtr;
    childInfoPtr = (ProcChildInfo *) Mem_Alloc(sizeof(ProcChildInfo));

    status = ProcServiceRemoteWait(procPtr, cmdPtr->flags, cmdPtr->numPids,
				    pidArray, cmdPtr->token,
				   childInfoPtr);

    Proc_Unlock(procPtr);
    if (status == SUCCESS) {
	storagePtr->replyDataPtr = (Address) childInfoPtr;
	storagePtr->replyDataSize = sizeof(ProcChildInfo);
	replyMemPtr = (Rpc_ReplyMem *) Mem_Alloc(sizeof(Rpc_ReplyMem));
	replyMemPtr->paramPtr = (Address) NIL;
	replyMemPtr->dataPtr = storagePtr->replyDataPtr;

	Rpc_Reply(srvToken, status, storagePtr, Rpc_FreeMem,
		(ClientData) replyMemPtr);
    } else {
	Mem_Free((Address) childInfoPtr);
    }

    if (proc_MigDebugLevel > 3) {
	Sys_Printf("Proc_RpcRemoteWait: returning %x.\n", status);
    }
    return(status);
}
