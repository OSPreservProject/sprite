/* 
 * signals.c --
 *
 *	Procedure to map from Unix signal system calls to Sprite.
 *
 *
 * Copyright (C) 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "sig.h"
#include "status.h"

#include "compatInt.h"
#include "user/sys/signal.h"
#include "machInt.h"
#include "mach.h"
#include "machConst.h"

extern Mach_State	*machCurStatePtr;


/*
 *----------------------------------------------------------------------
 *
 * sigvec --
 *
 *	Procedure to map from Unix sigvec system call to Sprite Sig_SetAction.
 *
 * Results:
 *	Error code is returned upon error.  Otherwise SUCCESS is returned.
 *
 * Side effects:
 *	The signal handler associated with the specified signal is modified.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
MachUNIXSigvec(sig, newVectorPtr, oldVectorPtr)
    int 		sig;		/* Signal to set the vector for. */
    struct sigvec	*newVectorPtr;	/* New vector. */
    struct sigvec	*oldVectorPtr;	/* Old vector. */
{
    int 		spriteSignal;	/* Equivalent signal for Sprite */
    Sig_Action 		newAction;	/* Action to take */
    Sig_Action		*newActionPtr;
    Sig_Action 		oldAction;	/* Former action */
    Sig_Action		*oldActionPtr;
    ReturnStatus 	status;		/* Generic result code */
    Address		usp;
    struct sigvec newVector;
    struct sigvec oldVector;

    usp = (Address)machCurStatePtr->userState.regState.regs[SP];
    newActionPtr = (Sig_Action *)(usp - sizeof(Sig_Action));
    oldActionPtr = (Sig_Action *)(usp - 2 * sizeof(Sig_Action));

    status = Compat_UnixSignalToSprite(sig, &spriteSignal);
    if (status == FAILURE || spriteSignal == NULL) {
	return(SYS_INVALID_ARG);
    }

    if (newVectorPtr != (struct sigvec *)NULL) {
	status = Vm_CopyIn(sizeof(struct sigvec), (Address)newVectorPtr,
			   (Address)&newVector);
	if (status != SUCCESS) {
	    return(status);
	}
	switch ((int)newVector.sv_handler) {
	    case SIG_DFL:
		newAction.action = SIG_DEFAULT_ACTION;
		break;
	    case SIG_IGN:
		newAction.action = SIG_IGNORE_ACTION;
		break;
	    default:
		newAction.action = SIG_HANDLE_ACTION;
		newAction.handler = newVector.sv_handler;
	}
	status = Compat_UnixSigMaskToSprite(newVector.sv_mask,
					    &newAction.sigHoldMask);
	if (status == FAILURE) {
	    return(SYS_INVALID_ARG);
	}

	status = Vm_CopyOut(sizeof(Sig_Action), (Address)&newAction,
			    (Address)newActionPtr);
	if (status != SUCCESS) {
	    return(status);
	}
	status = Sig_SetAction(spriteSignal, newActionPtr, oldActionPtr);
	if (status != SUCCESS) {
	    return(status);
	} 
    }

    if (oldVectorPtr != NULL) {
	(void)Vm_CopyIn(sizeof(Sig_Action), (Address)oldActionPtr, 
			(Address)&oldAction);
	switch (oldAction.action) {
	    case SIG_DEFAULT_ACTION:
		oldVector.sv_handler = SIG_DFL;
		break;
	    case SIG_IGNORE_ACTION:
		oldVector.sv_handler = SIG_IGN;
		break;
	    default:
		oldVector.sv_handler = oldActionPtr->handler;
		break;
	}
	(void) Compat_SpriteSigMaskToUnix(oldAction.sigHoldMask, 
					  &oldVector.sv_mask);
	oldVector.sv_flags = 0;
	status = Vm_CopyOut(sizeof(oldVector), (Address)&oldVector,
			    (Address)oldVectorPtr);
	if (status != SUCCESS) {
	    return(status);
	}
    }

    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * sigblock --
 *
 *	Procedure to map from Unix sigblock system call to Sprite 
 *	Sig_SetHoldMask.
 *
 * Results:
 *	Error code is returned upon error.  Otherwise SUCCESS is returned.
 *
 * Side effects:
 *	The current signal mask is modified.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
MachUNIXBlock(mask)
    int mask;			/* additional bits to mask */
{
    int spriteMask = 0;		/* equivalent mask for Sprite */
    int oldSpriteMask;		/* old mask, in Sprite terms */
    ReturnStatus status;	/* generic result code */

    status = Compat_UnixSigMaskToSprite(mask,&spriteMask);
    if (status == FAILURE) {
	return(SYS_INVALID_ARG);
    }
    status = Compat_GetSigHoldMask(&oldSpriteMask);
    if (status == FAILURE) {
	return(SYS_INVALID_ARG);
    }
    status = Sig_SetHoldMask(spriteMask | oldSpriteMask, (int *) NULL);
    if (status != SUCCESS) {
	return(status);
    } else {
	status = Compat_SpriteSigMaskToUnix(oldSpriteMask, 
					&machCurStatePtr->userState.unixRetVal);
	if (status == FAILURE) {
	    return(SYS_INVALID_ARG);
	}
	return(SUCCESS);
    }
}



/*
 *----------------------------------------------------------------------
 *
 * sigsetmask --
 *
 *	Procedure to map from Unix sigsetmask system call to Sprite 
 *	Sig_SetHoldMask.
 *
 * Results:
 *	Error code is returned upon error.  Otherwise SUCCESS is returned.
 *
 * Side effects:
 *	The current signal mask is modified.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
MachUNIXSigSetmask(mask)
    int mask;			/* new mask */
{
    int spriteMask = 0;		/* equivalent mask for Sprite */
    int oldSpriteMask;		/* old mask, in Sprite terms */
    ReturnStatus status;	/* generic result code */
    Address usp;

    status = Compat_UnixSigMaskToSprite(mask,&spriteMask);
    if (status == FAILURE) {
	return(SYS_INVALID_ARG);
    }
    usp = (Address)(machCurStatePtr->userState.regState.regs[SP] - 4);
    status = Sig_SetHoldMask(spriteMask, (int *)usp);
    if (status != SUCCESS) {
	return(status);
    } else {
	(void)Vm_CopyIn(sizeof(oldSpriteMask), usp, (Address)&oldSpriteMask);
	status = Compat_SpriteSigMaskToUnix(oldSpriteMask,
				    &machCurStatePtr->userState.unixRetVal);
	if (status == FAILURE) {
	    return(SYS_INVALID_ARG);
	}
	return(SUCCESS);
    }
}



/*
 *----------------------------------------------------------------------
 *
 * sigpause --
 *
 *	Procedure to map from Unix sigsetmask system call to Sprite 
 *	Sig_SetHoldMask.
 *
 * Results:
 *	Error code is returned upon error.  Otherwise SUCCESS is returned.
 *
 * Side effects:
 *	The current signal mask is modified.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
MachUNIXSigPause(mask)
    int mask;			/* new mask */
{
    int spriteMask = 0;		/* equivalent mask for Sprite */
    ReturnStatus status;	/* generic result code */

    status = Compat_UnixSigMaskToSprite(mask,&spriteMask);
    if (status == FAILURE) {
	return(SYS_INVALID_ARG);
    }
    status = Sig_Pause(spriteMask);
    if (status != SUCCESS) {
	return(status);
    } else {
	return(GEN_ABORTED_BY_SIGNAL);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * sigstack --
 *
 *	Procedure to fake the Unix sigstack system call.
 *
 * Results:
 *	Error code is returned upon error.  Otherwise SUCCESS is returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
MachUNIXSigStack(ss, oss)
    struct sigstack *ss, *oss;
{
    struct sigstack oldStack;
    if (oss != NULL) {
	oldStack.ss_sp = 0;
	oldStack.ss_onstack = 0;
	return(Vm_CopyOut(sizeof(struct sigstack), (Address)&oldStack,
			  (Address)oss));
    }
    return(SUCCESS);
}

