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
#include "list.h"
#include "proc.h"
#include "procMigrate.h"
#include "status.h"
#include "sched.h"
#include "sigInt.h"
#include "rpc.h"

/* 
 * Information sent when sending a signal.  Needed when doing a callback.
 */

typedef struct {
    Proc_ControlBlock 		*procPtr;
    int				sigNum;
    int				code;
} DeferInfo;

static void DeferSignal();


/*
 *----------------------------------------------------------------------
 *
 * SigMigSend --
 *
 *	Send a signal to a migrated process.  The current host is found
 *	in the process control block for the process.  
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
    Proc_PID remoteProcessID;
    int remoteHostID;
    Proc_ControlBlock 	*callerProcPtr; /* The calling process */

    if (proc_MigDebugLevel > 4) {
	printf("SigMigSend(%x, %d, %d) entered.\n", procPtr->processID,
		   sigNum, code);
    }

    if (procPtr->genFlags & (PROC_MIG_PENDING | PROC_MIGRATING)) {
	/*
	 * If the current process is a user process, wait for the
	 * process to finish migrating before signalling it. If
	 * it's a kernel process, start a background process to
	 * wait for migration and deliver the signal asynchronously.
	 */
	callerProcPtr = Proc_GetActualProc();
	if (callerProcPtr->genFlags & PROC_KERNEL) {
	    DeferInfo *infoPtr;

	    infoPtr = (DeferInfo *) malloc(sizeof(*infoPtr));
	    infoPtr->procPtr = procPtr;
	    infoPtr->sigNum = sigNum;
	    infoPtr->code = code;
	    Proc_CallFunc(DeferSignal, (ClientData) infoPtr, 0);
	    return(SUCCESS);
	}
	status = Proc_WaitForMigration(procPtr);
	if (status != SUCCESS) {
	    return(status);
	}
    }
    remoteProcessID = procPtr->peerProcessID;
    remoteHostID = procPtr->peerHostID;

    /*
     * It is necessary to unlock the process while sending the remote
     * signal, since the signal could cause the remote node to come back
     * and lock the process again.
     */
    Proc_Unlock(procPtr);
    status = SigSendRemoteSignal(remoteHostID, sigNum, code,
			      remoteProcessID, FALSE);

    if (proc_MigDebugLevel > 4) {
	printf("SigMigSend returning %x.\n", status);
    }

    if (status != SUCCESS) {
	if (proc_MigDebugLevel > 0) {
	    printf("Warning: SigMigSend:Error trying to signal %d to process %x (%x on host %d):\n\t%s\n",
		   sigNum, procPtr->processID, remoteProcessID, remoteHostID,
		   Stat_GetMsg(status));
	}
	if (sigNum == SIG_KILL) {
	    if (proc_MigDebugLevel > 0) {
		printf("SigMigSend: killing local copy of process %x.\n",
			   procPtr->processID);
	    }
	    Proc_CallFunc(Proc_DestroyMigratedProc,
			  (ClientData) procPtr->processID, 0);
	}
    }

    /*
     * Give back the procPtr in the same state we found it (locked).
     */
    Proc_Lock(procPtr);
	
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * DeferSignal --
 *
 *	Wait for a process to migrate, then send it a signal. This
 *	is done using the callback queue so the sender of the signal,
 *	if a kernel process, doesn't block.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The process doing the callback goes to sleep until the process
 *	being signalled has migrated or been killed.
 *
 *----------------------------------------------------------------------
 */

static void 
DeferSignal(data)
    ClientData data;
{
    DeferInfo *infoPtr = (DeferInfo *) data;
    ReturnStatus status;

    status = Proc_WaitForMigration(infoPtr->procPtr);
    if (status != SUCCESS) {
	return;
    }
    (void) SigMigSend(infoPtr->procPtr, infoPtr->sigNum, infoPtr->code);
}
    
