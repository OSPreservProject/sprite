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
#include "byte.h"
#include "sched.h"
#include "sigInt.h"
#include "rpc.h"

/* 
 * Information sent when sending a signal
 */

#ifdef notdef
typedef struct {
    Proc_PID			remotePID;
    int				sigNum;
    int				code;
} SigMigInfo;
    
#endif 


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

    if (proc_MigDebugLevel > 4) {
	Sys_Printf("SigMigSend(%x, %d, %d) entered.\n", procPtr->processID,
		   sigNum, code);
    }

    remoteProcessID = procPtr->peerProcessID;
    remoteHostID = procPtr->peerHostID;

    /*
     * It is necessary to unlock the process while sending the remote
     * signal, since the signal could cause the remote node to come back
     * and lock the process again.
     */
    Proc_Unlock(procPtr);
    status = SendRemoteSignal(remoteHostID, sigNum, code,
			      remoteProcessID, FALSE);

    if (proc_MigDebugLevel > 4) {
	Sys_Printf("SigMigSend returning %x.\n", status);
    }

    if (status != SUCCESS) {
	if (proc_MigDebugLevel > 0) {
	    Sys_Panic(SYS_WARNING,
		      "SigMigSend:Error %x returned by Rpc_Call.\n", status);
	}
	if (status == RPC_TIMEOUT && sigNum == SIG_KILL) {
	    if (proc_MigDebugLevel > 0) {
		Sys_Printf("SigMigSend: killing local copy of process %x.\n",
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
