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
    Proc_PID 			processID;
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
    Proc_PID processID;
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
	 * When calling Proc_WaitForMigration, make sure the process isn't
	 * locked.
	 */
	callerProcPtr = Proc_GetActualProc();
	if (callerProcPtr->genFlags & PROC_KERNEL) {
	    DeferInfo *infoPtr;

	    infoPtr = (DeferInfo *) malloc(sizeof(*infoPtr));
	    infoPtr->processID = procPtr->processID;
	    infoPtr->sigNum = sigNum;
	    infoPtr->code = code;
	    Proc_CallFunc(DeferSignal, (ClientData) infoPtr, 0);
	    return(SUCCESS);
	}
	processID = procPtr->processID;
        Proc_Unlock(procPtr);
	status = Proc_WaitForMigration(processID);
	Proc_Lock(procPtr);
	if (status != SUCCESS) {
	    return(status);
	}
	if (status == SUCCESS && procPtr->state != PROC_MIGRATED) {
	    /*
	     * Process was migrating back home.  Return PROC_INVALID_PID,
	     * which will make Sig_SendProc do a local send.
	     */
	    return(PROC_INVALID_PID);
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
    Proc_ControlBlock *procPtr;

    status = Proc_WaitForMigration(infoPtr->processID);
    if (status != SUCCESS) {
	goto failure;
    }
    procPtr = Proc_LockPID(infoPtr->processID);
    if (procPtr == (Proc_ControlBlock *) NIL) {
	goto failure;
    }
    (void) SigMigSend(procPtr, infoPtr->sigNum, infoPtr->code);
    Proc_Unlock(procPtr);
    return;

    failure:
    if (proc_MigDebugLevel > 2) {
	printf("DeferSignal: unable to send delayed signal to migrated process %x\n",
	       infoPtr->processID);
    }
}
    

typedef struct {
    int		sigHoldMask;
    int		sigPendingMask;
    int		sigActions[SIG_NUM_SIGNALS];
    int		sigMasks[SIG_NUM_SIGNALS];
    int		sigCodes[SIG_NUM_SIGNALS];
    int		sigFlags;
} EncapState;

#define COPY_STATE(from, to, field) to->field = from->field

/*
 *----------------------------------------------------------------------
 *
 * Sig_GetEncapSize --
 *
 *	Determine the size of the encapsulated signal state.
 *
 * Results:
 *	SUCCESS is returned directly; the size of the encapsulated state
 *	is returned in infoPtr->size.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
ReturnStatus
Sig_GetEncapSize(procPtr, hostID, infoPtr)
    Proc_ControlBlock *procPtr;			/* process being migrated */
    int hostID;					/* host to which it migrates */
    Proc_EncapInfo *infoPtr;			/* area w/ information about
						 * encapsulated state */
{
    infoPtr->size = sizeof(EncapState);
    return(SUCCESS);	
}


/*
 *----------------------------------------------------------------------
 *
 * Sig_EncapState --
 *
 *	Encapsulate the signal state of a process from the Proc_ControlBlock
 *	and return it in the buffer provided.
 *
 * Results:
 *	SUCCESS.  The buffer is filled.
 *
 * Side effects:
 *	None.
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
ReturnStatus
Sig_EncapState(procPtr, hostID, infoPtr, bufPtr)
    register Proc_ControlBlock 	*procPtr;  /* The process being migrated */
    int hostID;				   /* host to which it migrates */
    Proc_EncapInfo *infoPtr;		   /* area w/ information about
					    * encapsulated state */
    Address bufPtr;			   /* Pointer to allocated buffer */
{
    EncapState *encapPtr = (EncapState *) bufPtr;

    COPY_STATE(procPtr, encapPtr, sigHoldMask);
    COPY_STATE(procPtr, encapPtr, sigPendingMask);
    COPY_STATE(procPtr, encapPtr, sigFlags);
    bcopy((Address) procPtr->sigActions, (Address) encapPtr->sigActions,
	  SIG_NUM_SIGNALS * sizeof(int));
    bcopy((Address) procPtr->sigMasks, (Address) encapPtr->sigMasks,
	  SIG_NUM_SIGNALS * sizeof(int));
    bcopy((Address) procPtr->sigCodes, (Address) encapPtr->sigCodes,
	  SIG_NUM_SIGNALS * sizeof(int));
    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * Sig_DeencapState --
 *
 *	Get signal information from a Proc_ControlBlock from another host.
 *	The information is contained in the parameter ``buffer''.
 *
 * Results:
 *	SUCCESS.
 *
 * Side effects:
 *	None.
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
ReturnStatus
Sig_DeencapState(procPtr, infoPtr, bufPtr)
    register Proc_ControlBlock 	*procPtr; /* The process being migrated */
    Proc_EncapInfo *infoPtr;		  /* information about the buffer */
    Address bufPtr;			  /* buffer containing data */
{
    EncapState *encapPtr = (EncapState *) bufPtr;

    if (infoPtr->size != sizeof(EncapState)) {
	if (proc_MigDebugLevel > 0) {
	    printf("Sig_DeencapState: warning: host %d tried to migrate onto this host with wrong structure size.  Ours is %d, theirs is %d.\n",
		   procPtr->peerHostID, sizeof(EncapState),
		   infoPtr->size);
	}
	return(PROC_MIGRATION_REFUSED);
    }
    COPY_STATE(encapPtr, procPtr, sigHoldMask);
    COPY_STATE(encapPtr, procPtr, sigPendingMask);
    procPtr->sigPendingMask &=
	    ~((1 << SIG_MIGRATE_TRAP) | (1 << SIG_MIGRATE_HOME));
    COPY_STATE(encapPtr, procPtr, sigFlags);
    bcopy((Address) encapPtr->sigActions, (Address) procPtr->sigActions,
	  SIG_NUM_SIGNALS * sizeof(int));
    bcopy((Address) encapPtr->sigMasks, (Address) procPtr->sigMasks,
	  SIG_NUM_SIGNALS * sizeof(int));
    bcopy((Address) encapPtr->sigCodes, (Address) procPtr->sigCodes,
	  SIG_NUM_SIGNALS * sizeof(int));
    return(SUCCESS);
}
