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
 * $Header: /user5/kupfer/spriteserver/src/sprited/sig/RCS/sig.h,v 1.8 92/05/08 15:11:40 kupfer Exp $ SPRITE (Berkeley)
 */

#ifndef _SIG
#define _SIG

#ifdef SPRITED
#include <sigTypes.h>
#include <rpc.h>
#include <procTypes.h>
#include <procMigrate.h>
#else /* SPRITED */
#include <sprited/sigTypes.h>
#include <sprited/rpc.h>
#include <sprited/procTypes.h>
#include <sprited/procMigrate.h>
#endif /* SPRITED */

/*
 *----------------------------------------------------------------------
 *
 * Sig_Pending --
 *
 *	Return TRUE if a signal is pending and FALSE if not.  This routine
 *	does not impose any synchronization.
 *
 * Results:
 *	Returns a bit mask with bits enabled for pending signals.  If no 
 *	signals are pending, returns zero.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
#define Sig_Pending(procPtr) \
    ((procPtr)->sigPendingMask & ~((procPtr)->sigHoldMask))

#ifdef SPRITED

extern ReturnStatus Sig_Send _ARGS_((int sigNum, int code, Proc_PID id, 
		Boolean familyID, Address addr));
extern ReturnStatus Sig_SendProc _ARGS_((Proc_LockedPCB *procPtr, 
		int sigNum, Boolean isException, int code, Address addr));
extern ReturnStatus Sig_SetHoldMask _ARGS_((int newMask, int *oldMaskPtr));
extern ReturnStatus Sig_SetAction _ARGS_((int sigNum, Sig_Action *newActionPtr,
		Sig_Action *oldActionPtr));
extern ReturnStatus Sig_Pause _ARGS_((int sigHoldMask));

extern void Sig_Init _ARGS_((void));
extern void Sig_ProcInit _ARGS_((Proc_ControlBlock *procPtr));
extern void Sig_Fork _ARGS_((Proc_ControlBlock *parProcPtr, 
		Proc_LockedPCB *childProcPtr));
extern void Sig_Exec _ARGS_((Proc_LockedPCB *procPtr));
extern void Sig_ChangeState _ARGS_((Proc_LockedPCB *procPtr, 
		int actions[], int sigMasks[], int pendingMask, int sigCodes[],
		int holdMask));
extern Boolean Sig_Handle _ARGS_((Proc_LockedPCB *procPtr, 
		Boolean doNow, Boolean *suspendedPtr, Sig_Stack *sigStackPtr, 
		Address *pcPtr));
extern void Sig_CheckForKill _ARGS_((Proc_ControlBlock *procPtr));
extern void Sig_ExcToSig _ARGS_((int exceptionType, int exceptionCode, 
		int exceptionSubcode, int *sigNumPtr, int *codePtr,
		Address *sigAddrPtr));
extern void Sig_RestoreAfterSignal _ARGS_((Proc_LockedPCB *procPtr,
		Sig_Context *sigContextPtr));
extern void Sig_SetUpHandler _ARGS_((Proc_LockedPCB *procPtr, 
		Boolean suspended, Sig_Stack *sigStackPtr, Address pc));

extern ReturnStatus Sig_RpcSend _ARGS_((ClientData srvToken, int clientID,
			int command, Rpc_Storage *storagePtr));

/*
 * Procedures to support process migration.
 */

#ifdef SPRITED_MIGRATION
extern ReturnStatus Sig_GetEncapSize _ARGS_((Proc_ControlBlock *procPtr, 
			int hostID, Proc_EncapInfo *infoPtr));
extern ReturnStatus Sig_EncapState _ARGS_((Proc_ControlBlock *procPtr, 
			int hostID, Proc_EncapInfo *infoPtr, Address bufPtr));
extern ReturnStatus Sig_DeencapState _ARGS_((Proc_ControlBlock *procPtr, 
			Proc_EncapInfo *infoPtr, Address bufPtr));
extern void Sig_AllowMigration _ARGS_((Proc_ControlBlock *procPtr));
#endif /* SPRITED_MIGRATION */

#endif /* SPRITED */

#endif /* _SIG */
