/*
 * sig.h --
 *
 *     Data structures and procedure headers exported by the
 *     the signal module.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _SIG
#define _SIG

#ifdef KERNEL
#include "sigTypes.h"
#include "rpc.h"
#include "procTypes.h"
#include "procMigrate.h"
#else
#include <kernel/sigTypes.h>
#include <kernel/rpc.h>
#include <kernel/procTypes.h>
#include <kernel/procMigrate.h>
#endif

/*
 *----------------------------------------------------------------------
 *
 * Sig_Pending --
 *
 *	Return TRUE if a signal is pending and FALSE if not.  This routine
 *	does not impose any synchronization.
 *
 * Results:
 *	TRUE if a signal is pending and FALSE if not.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
#define Sig_Pending(procPtr) \
    ((Boolean) (procPtr->sigPendingMask & ~procPtr->sigHoldMask))

#ifdef KERNEL

extern ReturnStatus Sig_Send _ARGS_((int sigNum, int code, Proc_PID id, 
		Boolean familyID, Address addr));
extern ReturnStatus Sig_SendProc _ARGS_((Proc_ControlBlock *procPtr, 
		int sigNum, int code, Address addr));
extern ReturnStatus Sig_UserSend _ARGS_((int sigNum, Proc_PID pid, 
		Boolean familyID));
extern ReturnStatus Sig_SetHoldMask _ARGS_((int newMask, int *oldMaskPtr));
extern ReturnStatus Sig_SetAction _ARGS_((int sigNum, Sig_Action *newActionPtr,
		Sig_Action *oldActionPtr));
extern ReturnStatus Sig_Pause _ARGS_((int sigHoldMask));

extern void Sig_Init _ARGS_((void));
extern void Sig_ProcInit _ARGS_((Proc_ControlBlock *procPtr));
extern void Sig_Fork _ARGS_((Proc_ControlBlock *parProcPtr, 
		Proc_ControlBlock *childProcPtr));
extern void Sig_Exec _ARGS_((Proc_ControlBlock *procPtr));
extern void Sig_ChangeState _ARGS_((Proc_ControlBlock *procPtr, 
		int actions[], int sigMasks[], int pendingMask, int sigCodes[],
		int holdMask));
extern Boolean Sig_Handle _ARGS_((Proc_ControlBlock *procPtr, 
		Sig_Stack *sigStackPtr, Address *pcPtr));
extern void Sig_CheckForKill _ARGS_((Proc_ControlBlock *procPtr));
extern void Sig_Return _ARGS_((Proc_ControlBlock *procPtr, 
		Sig_Stack *sigStackPtr));

extern ReturnStatus Sig_RpcSend _ARGS_((ClientData srvToken, int clientID,
			int command, Rpc_Storage *storagePtr));

/*
 * Procedures to support process migration.
 */

extern ReturnStatus Sig_GetEncapSize _ARGS_((Proc_ControlBlock *procPtr, 
			int hostID, Proc_EncapInfo *infoPtr));
extern ReturnStatus Sig_EncapState _ARGS_((Proc_ControlBlock *procPtr, 
			int hostID, Proc_EncapInfo *infoPtr, Address bufPtr));
extern ReturnStatus Sig_DeencapState _ARGS_((Proc_ControlBlock *procPtr, 
			Proc_EncapInfo *infoPtr, Address bufPtr));
extern void Sig_AllowMigration _ARGS_((Proc_ControlBlock *procPtr));

#endif /* KERNEL */

#endif /* _SIG */

