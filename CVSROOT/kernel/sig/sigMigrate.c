/* 
 * sigMigrate.c --
 *
 *	Routines to handle signals for migrated procedures.
 *
 * Copyright (C) 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"
#include "sig.h"
#include "sync.h"
#include "exc.h"
#include "list.h"
#include "proc.h"
#include "procMigrate.h"
#include "status.h"
#include "byte.h"
#include "machine.h"
#include "sync.h"
#include "sched.h"
#include "sigInt.h"
#include "rpc.h"

/* 
 * Information sent when sending a signal
 */

typedef struct {
    Proc_PID			remotePID;
    int				sigNum;
    int				code;
} SigMigInfo;
    


/*
 *----------------------------------------------------------------------
 *
 * SigMigSend --
 *
 *	Send a signal to a migrated process.  The current host is found
 *	in the process control block for the process.  
 *
 * RPC: Input parameters:
 *		process ID
 *		signal number
 *		code
 *	Return parameters:
 *		ReturnStatus
 *
 * Results:
 *	SUCCESS or an error condition from the RPC or remote node.
 *
 * Side effects:
 *	A remote procedure call is performed and the process is signalled
 *	on its currently executing host.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
SigMigSend(procPtr, sigNum, code)
    register Proc_ControlBlock 	*procPtr; /* The migrated process */
    int				  sigNum;
    int				  code;
{
    ReturnStatus status;
    Rpc_Storage storage;
    SigMigInfo info;

    if (proc_MigDebugLevel > 4) {
	Sys_Printf("SigMigSend(%x, %d, %d) entered.\n", procPtr->processID,
		   sigNum, code);
    }
    /*
     * Set up for the RPC.
     */
    info.remotePID = procPtr->peerProcessID;
    info.sigNum = sigNum;
    info.code = code;
    storage.requestParamPtr = (Address) &info;
    storage.requestParamSize = sizeof(SigMigInfo);

    storage.requestDataPtr = (Address) NIL;
    storage.requestDataSize = 0;

    storage.replyParamPtr = (Address) NIL;
    storage.replyParamSize = 0;
    storage.replyDataPtr = (Address) NIL;
    storage.replyDataSize = 0;

    status = Rpc_Call(procPtr->peerHostID, RPC_SIG_MIG_SEND, &storage);

    if (proc_MigDebugLevel > 4) {
	Sys_Printf("SigMigSend returning %x.\n", status);
    }

    if (status != SUCCESS) {
	Sys_Panic(SYS_WARNING,
		  "SigMigSend:Error %x returned by Rpc_Call.\n", status);
	if (status == RPC_TIMEOUT) {
	    Sys_Printf("Killing local copy of process %x.\n",
		       procPtr->processID);
	    Proc_DestroyMigratedProc(PROC_TERM_DESTROYED, 
				     status, 0);
	}

    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Sig_RpcMigSend --
 *
 *	Send a signal to a migrated process executing on this host.
 *
 * Results:
 *	PROC_INVALID_PID  - the process is not a valid, migrated process from
 *			    the specified host.
 *	SUCCESS.
 *
 * Side effects:
 *	Signal pending mask and code modified.  The sched_Mutex master lock
 *	is grabbed.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
Sig_RpcMigSend(dataPtr, remoteHostID)
    ClientData dataPtr;
    int remoteHostID;
{
#ifdef notdef
    Proc_ControlBlock *procPtr;
    ReturnStatus status = SUCCESS;

    infoPtr = (SigMigInfo *) dataPtr;

    if (proc_MigDebugLevel > 4) {
	Sys_Printf("Sig_RpcMigSend(%x, %d) entered.\n", infoPtr->remotePID,
		   infoPtr->sigNum);
    }

    PROC_GET_MIG_PCB(infoPtr->remotePID, procPtr, remoteHostID);
    if (procPtr == (Proc_ControlBlock *) NIL) {
	if (proc_MigDebugLevel > 1) {
	    Sys_Panic(SYS_WARNING, "Sig_RpcMigSend: Invalid pid: %x.\n",
		      infoPtr->remotePID);
	}
	status = PROC_INVALID_PID;
    } else {
	Proc_Unlock(procPtr);
	status = Sig_SendProc(procPtr, infoPtr->sigNum, infoPtr->code);
    }
    if (proc_MigDebugLevel > 4) {
	Sys_Printf("Sig_RpcMigSend returning %x.\n", status);
    }
    return(status);
#endif
