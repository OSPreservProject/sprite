/* 
 * procMigrate.c --
 *
 *	Routines for process migration.
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
#endif not lint


#include "sprite.h"
#include "proc.h"
#include "procInt.h"
#include "procMigrate.h"
#include "migrate.h"
#include "fs.h"
#include "mem.h"
#include "sig.h"
#include "time.h"
#include "trace.h"
#include "list.h"
#include "byte.h"
#include "vm.h"
#include "sys.h"
#include "dbg.h"
#include "rpc.h"
#include "sched.h"
#include "sync.h"
#include "sysSysCall.h"

static	Sync_Condition	migrateCondition;
static	Sync_Lock	migrateLock = {0, 0};
#define	LOCKPTR &migrateLock

int proc_MigDebugLevel = 0;

Trace_Header proc_TraceHeader;
Trace_Header *proc_TraceHdrPtr = (Trace_Header *)NIL;
Boolean proc_DoTrace = FALSE;
Boolean proc_DoCallTrace = FALSE;

/*
 * Procedures internal to this file
 */

static ReturnStatus SendProcessState();
static ReturnStatus SendSegment();
static ReturnStatus SendFileState();
static ReturnStatus ResumeExecution();
static ReturnStatus InitiateMigration();
static void	    LockAndSwitch();
static ENTRY void   WakeupCallers();
static ENTRY ReturnStatus   WaitForMigration();


/*
 *----------------------------------------------------------------------
 *
 * Proc_Migrate --
 *
 *	Migrates a process to another workstation.  The process may be
 *    	PROC_MY_PID, PROC_ALL_PROCESSES, or a process ID.
 *    	PROC_ALL_PROCESSES implies evicting all foreign processes, in
 *    	which case the nodeID is ignored.  The workstation may be
 *    	PROC_MIG_ANY or a particular workstation.  (For now, the
 *    	workstation argument must be a specific workstation.)
 *
 * 	This procedure implements the system call by the same name.
 *
 * Results:
 *	PROC_INVALID_PID -	the pid argument was illegal.
 *	PROC_INVALID_NODE -	the node argument was illegal.
 *	Other results may be returned from the VM and RPC modules.
 *
 * Side effects:
 *	The specified process is migrated to another workstation.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Proc_Migrate(pid, nodeID)
    Proc_PID pid;
    int	     nodeID;
{
    register Proc_ControlBlock *procPtr;
    ReturnStatus status;
    Proc_TraceRecord record;

    if (Proc_ComparePIDs(pid, PROC_ALL_PROCESSES)) {
	status = Proc_EvictForeignProcs();
	return(status);
    }
    
    /*
     * Currently not allowed to migrate yourself.
     */
    
    if (Proc_ComparePIDs(pid, PROC_MY_PID)) {
#ifdef DOESNT_WORK_YET	
	procPtr = Proc_GetActualProc(Sys_GetProcessorNumber());
	if (procPtr == (Proc_ControlBlock *) NIL) {
	    Sys_Panic(SYS_FATAL, "Proc_Migrate: procPtr == NIL\n");
	}
	Proc_Lock(procPtr);
#else  DOESNT_WORK_YET	
	return(PROC_INVALID_PID);
#endif DOESNT_WORK_YET
    } else {
	procPtr = Proc_LockPID(pid);
	if (procPtr == (Proc_ControlBlock *) NIL) {
	    return (PROC_INVALID_PID);
	}
    }

    if (proc_MigDebugLevel > 3) {
	Sys_Printf("Proc_Migrate: migrate process %x to node %d.\n",
		   procPtr->processID, nodeID);
    }

    if (proc_DoTrace && proc_MigDebugLevel > 0) {
	record.processID = procPtr->processID;
	record.flags = PROC_MIGTRACE_START | PROC_MIGTRACE_HOME;
	record.info.filler = NIL;
	Trace_Insert(proc_TraceHdrPtr, PROC_MIGTRACE_BEGIN_MIG,
		     (ClientData *) &record);
    }
   
    /*
     * If no node is specified, pick a node somehow.  Can't do this yet.
     * Should also check validity of node ID here.  For example, you
     * are not allowed to migrate to yourself.
     *
     * Contact the remote workstation to establish communication and
     * verify that migration is permissible.
     */
    
    if ((nodeID == PROC_MIG_ANY) || (nodeID == rpc_SpriteID)) {
	return(PROC_INVALID_NODE_ID);
    }

    status = InitiateMigration(procPtr, nodeID);


    if (status != SUCCESS) {
	Proc_Unlock(procPtr);
	return(status);
    }

    Proc_FlagMigration(procPtr, nodeID);

    Proc_Unlock(procPtr);

    status = WaitForMigration(procPtr);
    
    if (proc_DoTrace && proc_MigDebugLevel > 0) {
	record.flags = PROC_MIGTRACE_HOME;
	Trace_Insert(proc_TraceHdrPtr, PROC_MIGTRACE_END_MIG,
		     (ClientData *) &record);
    }
   
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_MigrateTrap --
 *
 *	Transfer the state of a process once it has reached a state with no
 *	relevant information on the kernel stack.  This is done following a
 *	kernel call, when the only information on the kernel stack is an
 *	Exc_TrapStack.  
 *
 * Results:
 *	No value is returned.
 *
 * Side effects:
 *	A remote procedure call is performed and the process state
 *	is transferred.
 *
 *----------------------------------------------------------------------
 */

void
Proc_MigrateTrap(procPtr, machStatePtr)
    register Proc_ControlBlock 	*procPtr; /* The process being migrated */
    Mach_State	*machStatePtr;		  /* Machine state at time of call. */
{
#ifdef notdef
    int nodeID;				  /* node to which it migrates */
    Proc_PCBLink *procLinkPtr;
    register Proc_ControlBlock *procItemPtr;
    List_Links *sharersPtr;
    int numSharers;
    List_Links *itemPtr;
    ReturnStatus status;
    Proc_TraceRecord record;
    Boolean foreign = FALSE;

    Proc_Lock(procPtr);

    if (procPtr->genFlags & PROC_FOREIGN) {
	foreign = TRUE;
    }

    if (proc_DoTrace && proc_MigDebugLevel > 1) {
	record.processID = procPtr->processID;
	record.flags = PROC_MIGTRACE_START;
	if (!foreign) {
	    record.flags |= PROC_MIGTRACE_HOME;
	}
	record.info.filler = NIL;
	Trace_Insert(proc_TraceHdrPtr, PROC_MIGTRACE_MIGTRAP,
		     (ClientData *) &record);
    }
   
    procPtr->genFlags = (procPtr->genFlags & ~PROC_MIG_PENDING) |
	    PROC_MIGRATING;
    
    nodeID = procPtr->peerHostID;
    (void) Vm_FreezeSegments(procPtr, nodeID, &sharersPtr, &numSharers);

    /*
     * Go through the list of processes sharing memory.  This routine was
     * originally coded to migrate everything immediately but needs
     * to be changed to flag them for migration and WAIT for them to hit
     * Proc_MigrateTrap.  For now, work under the assumption that only
     * one process will be migrated.
     */
    if (numSharers > 1) {
	Sys_Panic(SYS_FATAL, "Proc_MigrateTrap: cannot handle shared heaps.\n");
	return;
    }
    LIST_FORALL(sharersPtr, itemPtr) {
	procLinkPtr = (Proc_PCBLink *) itemPtr;
	procItemPtr = procLinkPtr->procPtr;
	if (proc_MigDebugLevel > 7) {
	    Sys_Printf("Proc_Migrate: Sending process state.\n");
	}
	status = SendProcessState(procItemPtr, nodeID, trapStackPtr, foreign);
	if (status != SUCCESS) {
	    Sys_Panic(SYS_WARNING, "Error %x returned by SendProcessState.\n",
		      status);
	    goto failure;
	}

	/*
	 * Send the virtual memory, and set up the process so the kernel
	 * knows the process has no VM on this machine.
	 */
	
	procItemPtr->genFlags |= PROC_NO_VM;
	if (proc_MigDebugLevel > 7) {
	    Sys_Printf("Proc_Migrate: Sending code.\n");
	} 
	status = SendSegment(procItemPtr, VM_CODE, nodeID, foreign);
	if (status != SUCCESS) {
	    Sys_Panic(SYS_WARNING, "Error %x returned by SendSegment on code.\n",
		      status);
	    goto failure;
	}
    }
    if (proc_MigDebugLevel > 7) {
	Sys_Printf("Proc_Migrate: Sending stack.\n");
    }
    status = SendSegment(procItemPtr, VM_STACK, nodeID, foreign);
    if (status != SUCCESS) {
	Sys_Panic(SYS_WARNING, "Error %x returned by SendSegment on stack.\n",
		  status);
	goto failure;
    }
    if (proc_MigDebugLevel > 7) {
	Sys_Printf("Proc_Migrate: Sending heap.\n");
    }
#ifdef SHARED
    status = SendSharedSegment(sharersPtr, numSharers, VM_HEAP, nodeID,
			       foreign);
#else SHARED
    status = SendSegment(procPtr, VM_HEAP, nodeID, foreign);
#endif SHARED
    if (status != SUCCESS) {
	Sys_Panic(SYS_WARNING, "Error %x returned by SendSegment on heap.\n",
		  status);
	goto failure;
    }

    status = SendFileState(procPtr, nodeID, foreign);
    if (status != SUCCESS) {
	Sys_Panic(SYS_WARNING, "Error in SendFileState.\n");
	goto failure;
    }

    if (proc_MigDebugLevel > 4) {
	Sys_Printf("Proc_MigrateTrap: calling ResumeExecution.\n");
    }
    status = ResumeExecution(procPtr, nodeID, foreign);
    if (status != SUCCESS) {
	Sys_Panic(SYS_WARNING, "Error %x returned by ResumeExecution.\n",
		  status);
#ifdef KILL_IT
	goto failure;
#else
	Sys_Printf("Warning: not killing migrating process.\n");
#endif
    }
    procPtr->genFlags = (procPtr->genFlags & ~PROC_MIGRATING) |
	    PROC_MIGRATION_DONE;
    Proc_Unlock(procPtr);
    WakeupCallers();
    if (proc_DoTrace && proc_MigDebugLevel > 1) {
	record.flags = (foreign ? 0 : PROC_MIGTRACE_HOME);
	Trace_Insert(proc_TraceHdrPtr, PROC_MIGTRACE_MIGTRAP,
		     (ClientData *) &record);
    }

    if (foreign) {
	ProcExitProcess(procPtr, -1, -1, -1, TRUE);
    } else {
	Sched_ContextSwitch(PROC_MIGRATED);
    }
    Sys_Panic(SYS_FATAL, "Proc_MigrateTrap: returned from context switch.\n");
    return;

failure:
    procPtr->genFlags &= ~PROC_MIGRATING;
    Sig_SendProc(procPtr, SIG_KILL, status);
    Proc_Unlock(procPtr);
    WakeupCallers();

#endif
}



/*
 *----------------------------------------------------------------------
 *
 * SendProcessState --
 *
 *	Send the state of a process to another node.  This requires
 *	packaging the relevant information from the Proc_ControlBlock
 *	and sending it via an RPC.
 *
 *
 *	The relevant fields are as follows, with contiguous entries
 * 	listed with vertical bars:
 *
 *	Proc_PID	processID
 *   |	int		genFlags
 *   |	int		syncFlags
 *   |	int		schedFlags
 *   |	int		exitFlags
 *    | Proc_PID	parentID
 *    | int		familyID
 *    | int		userID
 *    |	int		effectiveUserID
 *    |	int		numGroupIDs
 *	int		groupIDs[]   (depends on numGroupIDs)
 * |	int 		billingRate
 * |	unsigned int 	recentUsage
 * |	unsigned int 	weightedUsage
 * |	unsigned int 	unweightedUsage
 * |    Timer_Ticks 	kernelCpuUsage
 * |    Timer_Ticks 	userCpuUsage
 * | 	Timer_Ticks 	childKernelCpuUsage
 * |    Timer_Ticks 	childUserCpuUsage
 * |    int 		numQuantumEnds
 * |    int		numWaitEvents
 * |    unsigned int 	schedQuantumTicks
 *  |	int		sigHoldMask
 *  |	int		sigPendingMask
 *  |	int		sigActions[SIG_NUM_SIGNALS]
 *  |	int		sigMasks[SIG_NUM_SIGNALS]
 *  |	int		sigCodes[SIG_NUM_SIGNALS]
 *  |	int		sigFlags
 *
 *	Note that if the Proc_ControlBlock structure is changed, it may
 * 	be necessary to change the logic of this procedure to copy
 *	fields separately.
 *
 *	In addition, the trap stack is transferred.
 *
 * RPC: Input parameters:
 *		process ID
 *		process control block information
 *	Return parameters:
 *		ReturnStatus
 *		process ID of process on remote node
 *
 * Results:
 *	A ReturnStatus is returned, along with the remote process ID.
 *
 * Side effects:
 *	A remote procedure call is performed and the process state
 *	is transferred.
 *
 *----------------------------------------------------------------------
 */

static ReturnStatus
SendProcessState(procPtr, nodeID, machStatePtr, foreign)
    register Proc_ControlBlock 	*procPtr; /* The process being migrated */
    int nodeID;				  /* node to which it migrates */
    Mach_State	*machStatePtr;	  /* trap stack at time of migration */
    Boolean foreign;			  /* Is it migrating back home? */
{
#ifdef notdef
    Address procBuffer;
    Address ptr;
    int procBufferSize;
    ReturnStatus error;
    Rpc_Storage storage;
    Proc_MigrateCommand migrateCommand;
    Proc_MigrateReply returnInfo;
    int argStringLength;
    int trapStackSize;
    Proc_TraceRecord record;

    if (proc_DoTrace && proc_MigDebugLevel > 2) {
	record.processID = procPtr->processID;
	record.flags = PROC_MIGTRACE_START;
	if (!foreign) {
	    record.flags |= PROC_MIGTRACE_HOME;
	}
	record.info.command.type = PROC_MIGRATE_PROC;
	record.info.command.data = (ClientData) NIL;
	Trace_Insert(proc_TraceHdrPtr, PROC_MIGTRACE_TRANSFER,
		     (ClientData *) &record);
    }
   
    argStringLength = Byte_AlignAddr(String_Length(procPtr->argString) + 1);
    trapStackSize = Exc_GetTrapStackSize(machStatePtr->trapStackPtr);
    procBufferSize = (3 + PROC_NUM_FLAGS + PROC_NUM_BILLING_FIELDS +
		      PROC_NUM_ID_FIELDS + procPtr->numGroupIDs) *
	    sizeof(int) +
            sizeof(Proc_PID) + sizeof(Proc_State) + SIG_INFO_SIZE +
	    trapStackSize + argStringLength;
    procBuffer = Mem_Alloc(procBufferSize);

    ptr = procBuffer;

    if (foreign) {
	Byte_FillBuffer(ptr, Proc_PID, procPtr->peerProcessID);
    } else {
	Byte_FillBuffer(ptr, Proc_PID, NIL);
    }
    Byte_FillBuffer(ptr, Proc_PID, procPtr->processID);

    /*
     * Copy in flags, IDs, scheduling information, registers, and the PC.
     */
    Byte_Copy(PROC_NUM_FLAGS * sizeof(int),
	      (Address) &procPtr->genFlags, ptr);
    ptr += PROC_NUM_FLAGS * sizeof(int);

    Byte_Copy(PROC_NUM_ID_FIELDS * sizeof(int),
	      (Address) &procPtr->parentID, ptr);
    ptr += PROC_NUM_ID_FIELDS * sizeof(int);
    if (procPtr->numGroupIDs > 0) {
	Byte_Copy(procPtr->numGroupIDs * sizeof(int),
		  (Address) procPtr->groupIDs, ptr);
	ptr += procPtr->numGroupIDs * sizeof(int);
    }
    
    Byte_Copy(PROC_NUM_BILLING_FIELDS * sizeof(int),
	      (Address) &procPtr->billingRate, ptr);
    ptr += PROC_NUM_BILLING_FIELDS * sizeof(int);

    Byte_Copy(SIG_INFO_SIZE, (Address) &procPtr->sigHoldMask, ptr);
    ptr += SIG_INFO_SIZE;
    Byte_FillBuffer(ptr, int, trapStackSize);
    Byte_Copy(trapStackSize, (Address) trapStackPtr, ptr);
    ptr += trapStackSize;

    Byte_FillBuffer(ptr, int, argStringLength);
    Byte_Copy(argStringLength, (Address) procPtr->argString, ptr);
    ptr += argStringLength;

    /*
     * Set up for the RPC.
     */
    migrateCommand.command = PROC_MIGRATE_PROC;
    migrateCommand.remotePID = NIL;
    storage.requestParamPtr = (Address) &migrateCommand;
    storage.requestParamSize = sizeof(Proc_MigrateCommand);

    storage.requestDataPtr = procBuffer;
    storage.requestDataSize = procBufferSize;

    storage.replyParamPtr = (Address) &returnInfo;
    storage.replyParamSize = sizeof(Proc_MigrateReply);
    storage.replyDataPtr = (Address) NIL;
    storage.replyDataSize = 0;


    error = Rpc_Call(nodeID, RPC_PROC_MIG_INFO, &storage);

    Mem_Free(procBuffer);

    if (proc_DoTrace && proc_MigDebugLevel > 2) {
	record.flags = (foreign ? 0 : PROC_MIGTRACE_HOME);
	Trace_Insert(proc_TraceHdrPtr, PROC_MIGTRACE_TRANSFER,
		     (ClientData *) &record);
    }
   
    if (error != SUCCESS) {
	Sys_Panic(SYS_WARNING,
		  "SendProcessState:Error %x returned by Rpc_Call.\n", error);
	return(error);
    } else {
	if (!foreign) {
	    procPtr->peerProcessID = returnInfo.remotePID;
	}
	return(returnInfo.status);
    }
#endif
}


/*
 *----------------------------------------------------------------------
 *
 * SendSegment --
 *
 *	Send the information for a segment to another node.  The
 *	information includes the process ID, segment type, and segment
 * 	info as encapsulated by the vm module.
 *
 * RPC: Input parameters:
 *		process ID
 *		segment type
 *		VM segment information
 *	Return parameters:
 *		ReturnStatus
 *
 * Results:
 *	A ReturnStatus is returned.
 *
 * Side effects:
 *	A remote procedure call is performed and the segment information
 *	is transferred.
 *
 *----------------------------------------------------------------------
 */

static ReturnStatus
SendSegment(procPtr, type, nodeID, foreign)
    register Proc_ControlBlock 	*procPtr;    /* The process being migrated */
    int type;				     /* The type of segment */
    int nodeID;				     /* Node to which it migrates */
    Boolean foreign;			     /* Is it migrating back home? */
{
    Address segBuffer;
    int segBufferSize;
    Proc_MigrateReply returnInfo;
    ReturnStatus error;
    ReturnStatus status;
    Rpc_Storage storage;
    Proc_MigrateCommand migrateCommand;
    Proc_TraceRecord record;
    int numPages;			     /* Number of pages flushed */

    if (proc_DoTrace && proc_MigDebugLevel > 2) {
	record.processID = procPtr->processID;
	record.flags = PROC_MIGTRACE_START;
	if (!foreign) {
	    record.flags |= PROC_MIGTRACE_HOME;
	}
	record.info.command.type = PROC_MIGRATE_VM;
	record.info.command.data = (ClientData) NIL;
	Trace_Insert(proc_TraceHdrPtr, PROC_MIGTRACE_TRANSFER,
		     (ClientData *) &record);
    }
   
    status = Vm_MigrateSegment(procPtr->vmPtr->segPtrArray[type], &segBuffer,
            &segBufferSize, &numPages);

    if (proc_MigDebugLevel > 5) {
	Sys_Printf("SendSegment: Vm_MigrateSegment(%d) wrote %d pages, returned %x.\n",
		   type, numPages, status);
    }
    if (status != SUCCESS) {
	return(status);
    }
    /*
     * Set up for the RPC.
     */
    migrateCommand.command = PROC_MIGRATE_VM;
    migrateCommand.remotePID = procPtr->peerProcessID;
    storage.requestParamPtr = (Address) &migrateCommand;
    storage.requestParamSize = sizeof(Proc_MigrateCommand);

    storage.requestDataPtr = segBuffer;
    storage.requestDataSize = segBufferSize;

    storage.replyParamPtr = (Address) &returnInfo;
    storage.replyParamSize = sizeof(Proc_MigrateReply);
    storage.replyDataPtr = (Address) NIL;
    storage.replyDataSize = 0;


    error = Rpc_Call(nodeID, RPC_PROC_MIG_INFO, &storage);

    Mem_Free(segBuffer);

    /*
     * Free up the segment on the home node.
     */
    Vm_SegmentDelete(procPtr->vmPtr->segPtrArray[type], procPtr);
    if (proc_DoTrace && proc_MigDebugLevel > 2) {
	record.flags = (foreign ? 0 : PROC_MIGTRACE_HOME);
	record.info.command.data = (ClientData) numPages;
	Trace_Insert(proc_TraceHdrPtr, PROC_MIGTRACE_TRANSFER,
		     (ClientData *) &record);
    }
   
    if (error != SUCCESS) {
	Sys_Panic(SYS_WARNING,
		  "SendSegment:Error %x returned by Rpc_Call.\n", error);
	return(error);
    } else {
	return(returnInfo.status);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * SendFileState --
 *
 *	Encapsulate all open file streams and send them to another node.  
 *	The open streams come from procPtr->openFileList[].
 *		
 * RPC: Input parameters:
 *		process ID
 *		for each stream (other than CWD): file ID, encapsulated data
 *	Return parameters:
 *		ReturnStatus
 *
 * Results:
 *	SUCCESS, or a ReturnStatus from rpc or fs routines.
 *
 * Side effects:
 *	A remote procedure call is performed and the file information
 *	is transferred.
 *
 *----------------------------------------------------------------------
 */

static ReturnStatus
SendFileState(procPtr, nodeID, foreign)
    register Proc_ControlBlock 	*procPtr;    /* The process being migrated */
    int nodeID;				     /* Node to which it migrates */
    Boolean foreign;			     /* Is it migrating back home? */
{
    Address fsInfoPtr;
    register Address ptr;
    int fsInfoSize;
    int totalSize;
    Proc_MigrateReply returnInfo;
    ReturnStatus status;
    Rpc_Storage storage;
    Proc_MigrateCommand migrateCommand;
    Fs_Stream *streamPtr;
    int i;
    int numFiles;
    int numEncap = 0;
    Proc_TraceRecord record;

    if (proc_DoTrace && proc_MigDebugLevel > 2) {
	record.processID = procPtr->processID;
	record.flags = PROC_MIGTRACE_START;
	if (!foreign) {
	    record.flags |= PROC_MIGTRACE_HOME;
	}
	record.info.command.type = PROC_MIGRATE_FILES;
	record.info.command.data = (ClientData) NIL;
	Trace_Insert(proc_TraceHdrPtr, PROC_MIGTRACE_TRANSFER,
		     (ClientData *) &record);
    }
   
    numFiles = procPtr->numStreams;

    fsInfoSize = Fs_GetEncapSize();
    totalSize = numFiles * (fsInfoSize + sizeof(int)) + sizeof (int) 
	    + fsInfoSize + sizeof(int)
		    ;
    fsInfoPtr = Mem_Alloc(totalSize);
    ptr = fsInfoPtr;

    /*
     * Send filePermissions, numStreams, the cwd, and each file.
     */
    
    Byte_FillBuffer(ptr, int, procPtr->filePermissions);
    Byte_FillBuffer(ptr, int, procPtr->numStreams);
    
    status = Fs_EncapStream(procPtr->cwdPtr, ptr);
    if (status != SUCCESS) {
	Sys_Panic(SYS_WARNING,
		  "SendFileState: Error %x from Fs_EncapStream on cwd.\n",
		  status);
	return(status);
    }
    ptr += fsInfoSize;
    numEncap += 1;

    for (i = 0; i < procPtr->numStreams; i++) {
	streamPtr = procPtr->streamList[i];
	if (streamPtr != (Fs_Stream *) NIL) {
	    numEncap += 1;
	    Byte_FillBuffer(ptr, int, i);
	    status = Fs_EncapStream(streamPtr, ptr);
	    if (status != SUCCESS) {
		Sys_Panic(SYS_WARNING,
			  "SendFileState: Error %x from Fs_EncapStream.\n",
			  status);
		return(status);
	    }
	} else {
	    Byte_FillBuffer(ptr, int, NIL);
	    Byte_Zero(fsInfoSize, ptr);
	}	
	ptr += fsInfoSize;
    }
    /*
     * Set up for the RPC.
     */
    migrateCommand.command = PROC_MIGRATE_FILES;
    migrateCommand.remotePID = procPtr->peerProcessID;
    storage.requestParamPtr = (Address) &migrateCommand;
    storage.requestParamSize = sizeof(Proc_MigrateCommand);

    storage.requestDataPtr = fsInfoPtr;
    storage.requestDataSize = totalSize;

    storage.replyParamPtr = (Address) &returnInfo;
    storage.replyParamSize = sizeof(Proc_MigrateReply);
    storage.replyDataPtr = (Address) NIL;
    storage.replyDataSize = 0;


    status = Rpc_Call(nodeID, RPC_PROC_MIG_INFO, &storage);

    Mem_Free(fsInfoPtr);

    if (proc_DoTrace && proc_MigDebugLevel > 2) {
	record.flags = (foreign ? 0 : PROC_MIGTRACE_HOME);
	record.info.command.data = (ClientData) numEncap;
	Trace_Insert(proc_TraceHdrPtr, PROC_MIGTRACE_TRANSFER,
		     (ClientData *) &record);
    }
   
    if (status != SUCCESS) {
	Sys_Panic(SYS_WARNING,
		  "SendFileState:Error %x returned by Rpc_Call.\n", status);
	return(status);
    } else {
	for (i = 0; i < procPtr->numStreams; i++) {
	    procPtr->streamList[i] = (Fs_Stream *) NIL;
	}
	procPtr->cwdPtr = (Fs_Stream *) NIL;
	return(returnInfo.status);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * ResumeExecution --
 *
 *	Inform the remote node that the state has been transferred and the
 *	process is ready to resume execution.
 *
 * RPC: Input parameters:
 *		remote process ID
 *	Return parameters:
 *		ReturnStatus
 *
 * Results:
 *	A ReturnStatus is returned.
 *
 * Side effects:
 *	A remote procedure call is performed and the migrated process
 *	returns to its state prior to migration (READY, WAITING, etc.)
 *
 *----------------------------------------------------------------------
 */

static ReturnStatus
ResumeExecution(procPtr, nodeID, foreign)
    register Proc_ControlBlock 	*procPtr;    /* The process being migrated */
    int nodeID;				     /* Node to which it migrates */
    Boolean foreign;			     /* Is it migrating back home? */
{
    ReturnStatus status;
    Proc_MigrateReply returnInfo;
    Rpc_Storage storage;
    Proc_MigrateCommand migrateCommand;
    Proc_TraceRecord record;

    if (proc_DoTrace && proc_MigDebugLevel > 2) {
	record.processID = procPtr->processID;
	record.flags = PROC_MIGTRACE_START;
	if (!foreign) {
	    record.flags |= PROC_MIGTRACE_HOME;
	}
	record.info.command.type = PROC_MIGRATE_RESUME;
	record.info.command.data = (ClientData) NIL;
	Trace_Insert(proc_TraceHdrPtr, PROC_MIGTRACE_TRANSFER,
		     (ClientData *) &record);
    }
   

    /*
     * Set up for the RPC.
     */
    migrateCommand.command = PROC_MIGRATE_RESUME;
    migrateCommand.remotePID = procPtr->peerProcessID;
    storage.requestParamPtr = (Address) &migrateCommand;
    storage.requestParamSize = sizeof(Proc_MigrateCommand);

    storage.requestDataPtr = (Address) NIL;
    storage.requestDataSize = 0;

    storage.replyParamPtr = (Address) &returnInfo;
    storage.replyParamSize = sizeof(Proc_MigrateReply);
    storage.replyDataPtr = (Address) NIL;
    storage.replyDataSize = 0;


    status = Rpc_Call(nodeID, RPC_PROC_MIG_INFO, &storage);

    if (proc_DoTrace && proc_MigDebugLevel > 2) {
	record.flags = (foreign ? 0 : PROC_MIGTRACE_HOME);
	Trace_Insert(proc_TraceHdrPtr, PROC_MIGTRACE_TRANSFER,
		     (ClientData *) &record);
    }
   
    if (status != SUCCESS) {
	Sys_Panic(SYS_WARNING,
		  "ResumeExecution:Error %x returned by Rpc_Call.\n", status);
	return(status);
    } else {
	return(returnInfo.status);
    }
}



/*
 *----------------------------------------------------------------------
 *
 * Proc_FlagMigration --
 *
 *	Mark a process as waiting to migrate.  This will cause the process
 *	to trap to the Proc_MigrateTrap routine after the next time it
 *	traps into the kernel. 
 *
 *	The calling routine is assumed to hold the lock for the process.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The process is flagged for migration.
 *
 *----------------------------------------------------------------------
 */

void
Proc_FlagMigration(procPtr, nodeID)
    Proc_ControlBlock 	*procPtr;	/* The process being migrated */
    int nodeID;				/* Node to which it migrates */
{

    procPtr->genFlags |= PROC_MIG_PENDING;
    procPtr->peerHostID = nodeID;
    Sig_SendProc(procPtr, SIG_MIGRATE_TRAP, 0);

}


/*
 *----------------------------------------------------------------------
 *
 * InitiateMigration --
 *	
 *	Send a message to a specific workstation requesting permission to
 *	migrate a process.
 *
 * Results:
 *	SUCCESS is returned if permission is granted.
 *	PROC_MIGRATION_REFUSED is returned if the node is not accepting
 *		migrated processes.
 *	Other errors may be returned by the rpc module.
 *
 * Side effects:
 *	A message is sent to the remote workstation.
 *
 *----------------------------------------------------------------------
 */

static ReturnStatus
InitiateMigration(procPtr, nodeID)
    register Proc_ControlBlock *procPtr;    	/* process to migrate */
    int nodeID;			      		/* node to which to migrate */
{
    Rpc_Storage storage;
    ReturnStatus status;

    storage.requestParamPtr = (Address) &procPtr->processID;
    storage.requestParamSize = sizeof(Proc_PID);

    storage.requestDataPtr = (Address) NIL;
    storage.requestDataSize = 0;

    storage.replyParamPtr = (Address) NIL;
    storage.replyParamSize = 0;

    storage.replyDataPtr = (Address) NIL;
    storage.replyDataSize = 0;

    status = Rpc_Call(nodeID, RPC_PROC_MIG_INIT, &storage);

    if (status != SUCCESS) {
	Sys_Panic(SYS_WARNING,
		  "InitiateMigration: Error %x returned by Rpc_Call.\n",
		  status);
    }
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_GetEffectiveProc --
 *
 *	Get a pointer to the Proc_ControlBlock for the process that is
 *	*effectively* running on the specified processor.  Thus, for an
 *	RPC server performing a system call on behalf of a migrated process,
 *	the "effective" process will be the process that invoked the system
 *	call.  In all other cases, the "effective" process will be the
 *	same as the "actual" process.
 *
 * Results:
 *	A pointer to the process is returned.  If no process is active,
 *	NIL is returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Proc_ControlBlock *
Proc_GetEffectiveProc(processor)
    int processor;
{
    Proc_ControlBlock *procPtr;

    procPtr = proc_RunningProcesses[processor];
    if (procPtr == (Proc_ControlBlock *) NIL ||
	    procPtr->rpcClientProcess ==  (Proc_ControlBlock *) NIL) {
	return(procPtr);
    }
    return(procPtr->rpcClientProcess);
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_SetEffectiveProc --
 *
 *	Set the "effective" process on the specified processor.  If the
 *	process is (Proc_ControlBlock) NIL, the effective process is
 *	the same as the real process.
 *
 * Results:
 *	None.  
 *
 * Side effects:
 *	The "rpcClientProcess" field of the current process's
 *	Proc_ControlBlock is set to hold the effective process.
 *
 *----------------------------------------------------------------------
 */

void
Proc_SetEffectiveProc(processor, procPtr)
    int processor;
    Proc_ControlBlock *procPtr;
{
    Proc_ControlBlock *actualProcPtr;

    actualProcPtr = Proc_GetActualProc(processor);
    if (actualProcPtr == (Proc_ControlBlock *) NIL) {
	Sys_Panic(SYS_FATAL,
		  "Proc_SetEffectiveProcess: current process is NIL.\n");
    } else {
	actualProcPtr->rpcClientProcess = procPtr;
    }
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_MigSendUserInfo --
 *
 *	Send the updateable portions of the state of a process to the
 *	node on which it is currently executing.  This requires
 *	packaging the relevant information from the Proc_ControlBlock
 *	and sending it via an RPC.
 *
 *
 *	The relevant fields are as follows:
 *
 *	int		userID
 *	int		effectiveUserID
 * 	int 		billingRate
 *
 * RPC: Input parameters:
 *		process ID
 *		process control block information
 *	Return parameters:
 *		ReturnStatus
 *
 * Results:
 *	A ReturnStatus is returned to indicate the status of the RPC.
 *
 * Side effects:
 *	A remote procedure call is performed and the process state
 *	is transferred.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Proc_MigSendUserInfo(procPtr)
    register Proc_ControlBlock 	*procPtr; /* The migrated process */
{
    Address procBuffer;
    Address ptr;
    int procBufferSize;
    ReturnStatus error;
    Rpc_Storage storage;
    Proc_MigrateCommand migrateCommand;
    Proc_MigrateReply returnInfo;
    Proc_TraceRecord record;

    if (proc_DoTrace && proc_MigDebugLevel > 2) {
	record.processID = procPtr->processID;
	record.flags = PROC_MIGTRACE_START | PROC_MIGTRACE_HOME;
	record.info.command.type = PROC_MIGRATE_USER_INFO;
	record.info.command.data = (ClientData) NIL;
	Trace_Insert(proc_TraceHdrPtr, PROC_MIGTRACE_TRANSFER,
		     (ClientData *) &record);
    }
   
    procBufferSize = PROC_NUM_USER_INFO_FIELDS * sizeof(int);
    procBuffer = Mem_Alloc(procBufferSize);

    ptr = procBuffer;

    /*
     * Copy in user IDs and scheduling information.
     */
    Byte_FillBuffer(ptr, int, procPtr->userID);
    Byte_FillBuffer(ptr, int, procPtr->effectiveUserID);
    Byte_FillBuffer(ptr, int, procPtr->billingRate);

    /*
     * Set up for the RPC.
     */
    migrateCommand.command = PROC_MIGRATE_USER_INFO;
    migrateCommand.remotePID = procPtr->peerProcessID;
    storage.requestParamPtr = (Address) &migrateCommand;
    storage.requestParamSize = sizeof(Proc_MigrateCommand);

    storage.requestDataPtr = procBuffer;
    storage.requestDataSize = procBufferSize;

    storage.replyParamPtr = (Address) &returnInfo;
    storage.replyParamSize = sizeof(Proc_MigrateReply);
    storage.replyDataPtr = (Address) NIL;
    storage.replyDataSize = 0;


    error = Rpc_Call(procPtr->peerHostID, RPC_PROC_MIG_INFO, &storage);

    Mem_Free(procBuffer);

    if (proc_DoTrace && proc_MigDebugLevel > 2) {
	record.flags = PROC_MIGTRACE_HOME;
	Trace_Insert(proc_TraceHdrPtr, PROC_MIGTRACE_TRANSFER,
		     (ClientData *) &record);
    }
   
    if (error != SUCCESS) {
	Sys_Panic(SYS_WARNING,
		  "Proc_MigSendUserInfo:Error %x returned by Rpc_Call.\n", error);
	return(error);
    } else {
	return(returnInfo.status);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * WaitForMigration --
 *
 *	Monitored procedure to wait for a process to migrate.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static ENTRY ReturnStatus
WaitForMigration(procPtr)
    Proc_ControlBlock *procPtr;
{
    ReturnStatus status;

    LOCK_MONITOR;

    while (procPtr->genFlags & (PROC_MIG_PENDING | PROC_MIGRATING)) {
	if (Sync_Wait(&migrateCondition, TRUE)) {
	    UNLOCK_MONITOR;
	    return(GEN_ABORTED_BY_SIGNAL);
	}
    }

    if (procPtr->genFlags & PROC_MIGRATION_DONE) {
	status = SUCCESS;
    } else {
	status = FAILURE;
    }
    
    UNLOCK_MONITOR;
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * WakeupCallers --
 *
 *	Monitored procedure to signal any processes that may have waited for
 *	a process to migrate.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static ENTRY void
WakeupCallers()
{

    LOCK_MONITOR;

    Sync_Broadcast(&migrateCondition);

    UNLOCK_MONITOR;
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_MigrateStartTracing --
 *
 *	Initialize the tracing variables for process migration.
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
Proc_MigrateStartTracing()
{
    static Boolean init = FALSE;

    if (!init) {
	init = TRUE;
	proc_TraceHdrPtr = &proc_TraceHeader;
	Trace_Init(proc_TraceHdrPtr, PROC_NUM_TRACE_RECS,
		   sizeof(Proc_TraceRecord), 0);
    }
    proc_DoTrace = TRUE;
}



/*
 *----------------------------------------------------------------------
 *
 * Proc_DestroyMigratedProc --
 *
 *	Kill a local copy of a migrated process (this is an asynchronous
 *	Proc_Exit performed on behalf of a migrated process that has
 *	been signaled, and the signal has timed out).
 *
 * 	Additional cleanup will probably be necessary.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The process is destroyed.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
void 
Proc_DestroyMigratedProc(procPtr, reason, status, code) 
    Proc_ControlBlock 		*procPtr;	/* Exiting process. */
    int 			reason;		/* Why the process is dying: 
						 * EXITED, SIGNALED, 
						 * DESTROYED  */
    int				status;		/* Exit status, signal # or 
						 * destroy status. */
    int 			code;		/* Signal sub-status */
{
#ifdef DOESNT_WORK    
    ProcExitProcess(procPtr, reason, status, code, FALSE);
#endif DOESNT_WORK
}



/*
 *----------------------------------------------------------------------
 *
 * Proc_EvictForeignProcs --
 *
 *	Evict all foreign processes from this machine.  To do this, send
 *	each foreign process the SIG_MIGRATE_HOME signal.
 *
 * Results:
 *	If sending any signals returns a non-SUCCESS status, that status
 *	is returned.  Otherwise, SUCCESS is returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Proc_EvictForeignProcs()
{
    ReturnStatus status;
    
    status = Proc_DoForEveryProc(Proc_IsMigratedProc, Proc_EvictProc, TRUE);
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_IsMigratedProc --
 *
 *	Return whether the specified process is foreign.  (This routine is
 * 	a callback procedure that may be passed as a parameter to routines
 *	requiring an arbitrary Boolean procedure operating on a PCB.)
 *
 * Results:
 *	Boolean result: TRUE if foreign, FALSE if not.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Boolean
Proc_IsMigratedProc(procPtr)
    Proc_ControlBlock *procPtr;
{
    if (procPtr->genFlags & PROC_FOREIGN) {
	return(TRUE);
    } else {
	return(FALSE);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_EvictProc --
 *
 *	Send a process the SIG_MIGRATE_HOME signal.  Note that if
 *	the process is not foreign, then the signal will be ignored.
 *	(This routine is a callback procedure that may be passed as a
 *    	parameter to routines requiring an arbitrary procedure
 *    	operating on a Proc_PID and returning a ReturnStatus.)
 *
 * Results:
 *	The value from Sig_Send is returned.
 *
 * Side effects:
 *	The specified process is signalled.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Proc_EvictProc(pid)
    Proc_PID pid;
{
    ReturnStatus status;
    
    status = Sig_Send(SIG_MIGRATE_HOME, 0, pid, FALSE);
    return(status); 
}

