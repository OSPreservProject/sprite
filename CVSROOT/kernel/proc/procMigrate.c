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
#endif /* not lint */


#include "sprite.h"
#include "mach.h"
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
 * Set to true to refuse ALL migrations onto this machine.
 */
Boolean proc_RefuseMigrations = FALSE;

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
    Boolean migrateSelf = FALSE;

    /*
     * It is possible for a process to try to migrate onto the machine
     * on which it is currently executing.
     */

    if (nodeID == rpc_SpriteID) {
	return(SUCCESS);
    }
    
    if (Proc_ComparePIDs(pid, PROC_ALL_PROCESSES)) {
	status = Proc_EvictForeignProcs();
	return(status);
    }
    
    if (Proc_ComparePIDs(pid, PROC_MY_PID)) {
	migrateSelf = TRUE;
	procPtr = Proc_GetActualProc();
	if (procPtr == (Proc_ControlBlock *) NIL) {
	    Sys_Panic(SYS_FATAL, "Proc_Migrate: procPtr == NIL\n");
	}
	Proc_Lock(procPtr);
#ifdef notdef
	return(PROC_INVALID_PID);
#endif /* notdef */
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

    /*
     * Do some sanity checking.  
     */
    if (procPtr->state == PROC_DEAD || procPtr->state == PROC_EXITING) {
	if (proc_MigDebugLevel > 3) {
	    Sys_Printf("Proc_Migrate: process %x has exited.\n",
		       procPtr->processID);
	}
	return(PROC_INVALID_PID);
    }
    if (procPtr->state == PROC_MIGRATED) {
	if (proc_MigDebugLevel > 1) {
	    Sys_Printf("Proc_Migrate: process %x has already migrated.\n",
		       procPtr->processID);
	}
	return(PROC_INVALID_PID);
    }
	
    if (proc_DoTrace && proc_MigDebugLevel > 0) {
	record.processID = procPtr->processID;
	record.flags = PROC_MIGTRACE_START | PROC_MIGTRACE_HOME;
	record.info.filler = NIL;
	Trace_Insert(proc_TraceHdrPtr, PROC_MIGTRACE_BEGIN_MIG,
		     (ClientData) &record);
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

    /*
     * If migrating another process, wait for it to migrate.
     */
    if (!migrateSelf) {
	status = WaitForMigration(procPtr);
    } else {
	status = SUCCESS;
    }

    /*
     * Note: the "end migration" trace record is inserted by the MigrateTrap
     * routine to get around the issue of waiting for oneself to migrate.
     * (Otherwise, since we can't wait here for the current process to
     * migrate -- only a different process -- we wouldn't be able to insert
     * the trace record at the right time.)
     */
     
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_MigrateTrap --
 *
 *	Transfer the state of a process once it has reached a state with no
 *	relevant information on the kernel stack.  This is done following a
 *	kernel call, when the process can migrate and immediately return
 *	to user mode and there is no relevant information on the kernel
 *	stack.  
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
Proc_MigrateTrap(procPtr)
    register Proc_ControlBlock 	*procPtr; /* The process being migrated */
{
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
		     (ClientData) &record);
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
	Sys_Panic(SYS_WARNING,
		  "Proc_MigrateTrap: cannot handle shared heaps.\n");
	return;
    }
    LIST_FORALL(sharersPtr, itemPtr) {
	procLinkPtr = (Proc_PCBLink *) itemPtr;
	procItemPtr = procLinkPtr->procPtr;
	if (proc_MigDebugLevel > 7) {
	    Sys_Printf("Proc_Migrate: Sending process state.\n");
	}
	status = SendProcessState(procItemPtr, nodeID, foreign);
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
#endif /* SHARED */
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
#endif /* */
    }
    procPtr->genFlags = (procPtr->genFlags & ~PROC_MIGRATING) |
	    PROC_MIGRATION_DONE;
    Proc_Unlock(procPtr);

    /*
     * If not migrating back home, note the dependency on the other host.
     */
    if (!foreign) {
	Proc_AddMigDependency(procPtr->processID, nodeID);
    }
    
    WakeupCallers();
    if (proc_DoTrace && proc_MigDebugLevel > 1) {
	record.flags = (foreign ? 0 : PROC_MIGTRACE_HOME);
	Trace_Insert(proc_TraceHdrPtr, PROC_MIGTRACE_MIGTRAP,
		     (ClientData) &record);
    }

    if (proc_DoTrace && proc_MigDebugLevel > 0 && !foreign) {
	record.processID = procPtr->processID;
	record.flags = PROC_MIGTRACE_HOME;
	record.info.filler = NIL;
	Trace_Insert(proc_TraceHdrPtr, PROC_MIGTRACE_END_MIG,
		     (ClientData) &record);
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
    if (proc_DoTrace && proc_MigDebugLevel > 0 && !foreign) {
	record.processID = procPtr->processID;
	record.flags = PROC_MIGTRACE_HOME;
	record.info.filler = NIL;
	Trace_Insert(proc_TraceHdrPtr, PROC_MIGTRACE_END_MIG,
		     (ClientData) &record);
    }

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
 *    	Proc_PID	processID    [not stored contiguously on remote]
 *    | Proc_PID	parentID
 *    | int		familyID
 *    | int		userID
 *    |	int		effectiveUserID
 *   |	int		genFlags
 *   |	int		syncFlags
 *   |	int		schedFlags
 *   |	int		exitFlags
 * |	int 		billingRate
 * |	unsigned int 	recentUsage
 * |	unsigned int 	weightedUsage
 * |	unsigned int 	unweightedUsage
 * |    Timer_Ticks 	kernelCpuUsage.ticks
 * |    Timer_Ticks 	userCpuUsage.ticks
 * | 	Timer_Ticks 	childKernelCpuUsage.ticks
 * |    Timer_Ticks 	childUserCpuUsage.ticks
 * |    int 		numQuantumEnds
 * |    int		numWaitEvents
 * |    unsigned int 	schedQuantumTicks
 *  |	int		sigHoldMask
 *  |	int		sigPendingMask
 *  |	int		sigActions[SIG_NUM_SIGNALS]
 *  |	int		sigMasks[SIG_NUM_SIGNALS]
 *  |	int		sigCodes[SIG_NUM_SIGNALS]
 *  |	int		sigFlags
 *	variable: encapsulated machine state
 *
 *	Note that if the Proc_ControlBlock structure is changed, it may
 * 	be necessary to change the logic of this procedure to copy
 *	fields separately.
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
SendProcessState(procPtr, nodeID, foreign)
    register Proc_ControlBlock 	*procPtr; /* The process being migrated */
    int nodeID;				  /* node to which it migrates */
    Boolean foreign;			  /* Is it migrating back home? */
{
    Address procBuffer;
    Address ptr;
    int procBufferSize;
    int machStateSize;
    ReturnStatus error;
    Rpc_Storage storage;
    Proc_MigrateCommand migrateCommand;
    Proc_MigrateReply returnInfo;
    int argStringLength;
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
		     (ClientData) &record);
    }
   
    machStateSize = Mach_GetEncapSize();
    
    argStringLength = Byte_AlignAddr(String_Length(procPtr->argString) + 1);
    procBufferSize = 2 * sizeof(Proc_PID) +
	    (PROC_NUM_ID_FIELDS + PROC_NUM_FLAGS +
	     PROC_NUM_SCHED_FIELDS + 1) * sizeof(int) +
	    SIG_INFO_SIZE + machStateSize + argStringLength;
    procBuffer = Mem_Alloc(procBufferSize);

    ptr = procBuffer;

    if (foreign) {
	Byte_FillBuffer(ptr, Proc_PID, procPtr->peerProcessID);
    } else {
	Byte_FillBuffer(ptr, Proc_PID, NIL);
    }

    /*
     * Copy in IDs, flags, scheduling information, and machine-dependent
     * state.
     */
    Byte_FillBuffer(ptr, Proc_PID, procPtr->processID);
    Byte_Copy(PROC_NUM_ID_FIELDS * sizeof(int),
	      (Address) &procPtr->parentID, ptr);
    ptr += PROC_NUM_ID_FIELDS * sizeof(int);
    Byte_Copy(PROC_NUM_FLAGS * sizeof(int),
	      (Address) &procPtr->genFlags, ptr);
    ptr += PROC_NUM_FLAGS * sizeof(int);

    Byte_Copy(PROC_NUM_SCHED_FIELDS * sizeof(int),
	      (Address) &procPtr->billingRate, ptr);
    ptr += PROC_NUM_SCHED_FIELDS * sizeof(int);

    Byte_Copy(SIG_INFO_SIZE, (Address) &procPtr->sigHoldMask, ptr);
    ptr += SIG_INFO_SIZE;

    Mach_EncapState(procPtr, ptr);
    ptr += machStateSize;

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
		     (ClientData) &record);
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
		     (ClientData) &record);
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
		     (ClientData) &record);
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
 *	Transfer the file state of a process to another node.  
 *
 * RPC: Input parameters:
 *		encapsulated file state
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
    Address buffer;
    int totalSize;
    Proc_MigrateReply returnInfo;
    ReturnStatus status;
    Rpc_Storage storage;
    Proc_MigrateCommand migrateCommand;
    int numEncap;
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
		     (ClientData) &record);
    }

    if (proc_MigDebugLevel > 6) {
	Sys_Printf("SendFileState: calling Fs_EncapFileState.\n");
    }
    status = Fs_EncapFileState(procPtr, &buffer, &totalSize, &numEncap);
    if (status != SUCCESS) {
	if (proc_MigDebugLevel > 6) {
	    Sys_Panic(SYS_WARNING,
		      "SendFileState: error %x returned by Fs_EncapFileState",
		      status);
	}
	return (status);
    }
	
    /*
     * Set up for the RPC.
     */
    migrateCommand.command = PROC_MIGRATE_FILES;
    migrateCommand.remotePID = procPtr->peerProcessID;
    storage.requestParamPtr = (Address) &migrateCommand;
    storage.requestParamSize = sizeof(Proc_MigrateCommand);

    storage.requestDataPtr = buffer;
    storage.requestDataSize = totalSize;

    storage.replyParamPtr = (Address) &returnInfo;
    storage.replyParamSize = sizeof(Proc_MigrateReply);
    storage.replyDataPtr = (Address) NIL;
    storage.replyDataSize = 0;


    status = Rpc_Call(nodeID, RPC_PROC_MIG_INFO, &storage);

    Mem_Free(buffer);

    if (proc_DoTrace && proc_MigDebugLevel > 2) {
	record.flags = (foreign ? 0 : PROC_MIGTRACE_HOME);
	record.info.command.data = (ClientData) numEncap;
	Trace_Insert(proc_TraceHdrPtr, PROC_MIGTRACE_TRANSFER,
		     (ClientData) &record);
    }
   
    if (status != SUCCESS) {
	Sys_Panic(SYS_WARNING,
		  "SendFileState:Error %x returned by Rpc_Call.\n", status);
	return(status);
    } else {
#ifndef TO_BE_REMOVED
	Fs_ClearFileState(procPtr);
#endif
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
		     (ClientData) &record);
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
		     (ClientData) &record);
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
 *	The process is flagged for migration.  If the process is suspended,
 *	it is resumed.
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
    if (procPtr->state == PROC_SUSPENDED) {
	Sig_SendProc(procPtr, SIG_RESUME, 0);
    }
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
 *	*effectively* running on the current processor.  Thus, for an
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
Proc_GetEffectiveProc()
{
    Proc_ControlBlock *procPtr;

    procPtr = proc_RunningProcesses[Mach_GetProcessorNumber()];
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
 *	Set the "effective" process on the current processor.  If the
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
Proc_SetEffectiveProc(procPtr)
    Proc_ControlBlock *procPtr;
{
    Proc_ControlBlock *actualProcPtr;

    actualProcPtr = Proc_GetActualProc();
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
		     (ClientData) &record);
    }

    /*
     * Note: this depends on the size of a Proc_PID being the size of
     * an int.
     */
    procBufferSize = PROC_NUM_USER_INFO_FIELDS * sizeof(int);
    procBuffer = Mem_Alloc(procBufferSize);

    ptr = procBuffer;

    /*
     * Copy in user IDs and scheduling information.
     */
    Byte_FillBuffer(ptr, int, procPtr->userID);
    Byte_FillBuffer(ptr, int, procPtr->effectiveUserID);
    Byte_FillBuffer(ptr, int, procPtr->familyID);
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
		     (ClientData) &record);
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
 *	Kill a process, presumably when its peer host (the home node
 *	of a foreign process, or the remote host of a migrated process)
 *	is down.  It may also be done if the process is
 *	unsuccessfully killed with a signal, even if the remote node
 *	hasn't been down long enough to be sure it has crashed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The process is killed.
 *
 *----------------------------------------------------------------------
 */

void 
Proc_DestroyMigratedProc(pidData) 
    ClientData pidData;		/* the process ID, as a ClientData */
{
    Proc_ControlBlock 		*procPtr; /* Process to kill. */
    Proc_PID pid = (Proc_PID) pidData;

    procPtr = Proc_LockPID(pid);
    if (procPtr == (Proc_ControlBlock *) NIL) {
	if (proc_MigDebugLevel > 0) {
	    Sys_Panic(SYS_WARNING,
		      "Proc_DestroyMigratedProc: process %x not found.\n",
		      (int) pid);
	}
	return;
    }
    if ((procPtr->state != PROC_MIGRATED) &&
	!(procPtr->genFlags & PROC_FOREIGN)) {
	if (proc_MigDebugLevel > 0) {
	    Sys_Panic(SYS_WARNING,
		      "Proc_DestroyMigratedProc: process %x not migrated.\n",
		      (int) pid);
	}
	Proc_Unlock(procPtr);
	return;
    }
    /*
     * Unlock the process again, since ProcExitProcess locks it.  [Is
     * there any race condition?  ProcExitProcess must be careful about
     * the process it's passed.]
     */
    Proc_Unlock(procPtr);
    
    ProcExitProcess(procPtr, PROC_TERM_DESTROYED, (int) PROC_NO_PEER, 0,
		    FALSE);

    Proc_RemoveMigDependency(pid);
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





