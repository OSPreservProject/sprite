/* 
 * sigStubs.c --
 *
 *	Stubs for Unix compatible system calls.
 *
 * Copyright 1990 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header$";
#endif /* not lint */

#define MACH_UNIX_COMPAT

#include <sprite.h>
#include <stdio.h>
#include <stdlib.h>
#include <status.h>
#include <errno.h>
#include <user/sys/types.h>
#include <user/sys/wait.h>
#include <user/sys/time.h>
#include <user/sys/resource.h>
#include <user/sys/signal.h>
#include <mach.h>
#include <proc.h>
#include <procUnixStubs.h>
#include <vm.h>
#include <fsutil.h>
#include <assert.h>
#include <sig.h>
#include <sigInt.h>
#include <compatInt.h>

extern unsigned int 	sigBitMasks[SIG_NUM_SIGNALS];
extern int		sigDefActions[SIG_NUM_SIGNALS];
extern int		sigCanHoldMask;

int debugSigStubs;


/*
 *----------------------------------------------------------------------
 *
 * Sig_KillStub --
 *
 *	Procedure to map from Unix kill system call to Sprite.
 *
 * Results:
 *	Error code is returned upon error.  Otherwise SUCCESS is returned.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *
 *----------------------------------------------------------------------
 */
int
Sig_KillStub(pid, sig)
    int pid;
    int sig;
{
    ReturnStatus status;
    int		 spriteSignal;

    if (debugSigStubs) {
	printf("Sig_KillStub(0x%x, 0x%x)\n", pid, sig);
    }
    status = Compat_UnixSignalToSprite(sig, &spriteSignal);
    if (status == FAILURE || (spriteSignal == NULL && sig != 0)) {
	status = SYS_INVALID_ARG;
    } else {
	if (pid == 0) {
	    pid = PROC_MY_PID;
	}
	status = Sig_UserSend(spriteSignal, pid, FALSE);
    }
    if (status != SUCCESS) {
	Mach_SetErrno(Compat_MapCode(status));
	return -1;
    }
    return 0;
}



/*
 *----------------------------------------------------------------------
 *
 * Sig_KillpgStub --
 *
 *	Procedure to map from Unix killpg system call to Sprite.
 *
 * Results:
 *	Error code is returned upon error.  Otherwise SUCCESS is returned.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *
 *----------------------------------------------------------------------
 */
int
Sig_KillpgStub(pgrp, sig)
    int pgrp;
    int sig;
{
    ReturnStatus status;
    int		 spriteSignal;

    if (debugSigStubs) {
	printf("Sig_KillpgStub(0x%x, 0x%x)\n", pgrp, sig);
    }
    status = Compat_UnixSignalToSprite(sig, &spriteSignal);
    if (status == FAILURE || (spriteSignal == NULL && sig != 0)) {
	Mach_SetErrno(EINVAL);
	return -1;
    }
    status = Sig_UserSend(spriteSignal, pgrp, TRUE);
    if (status != SUCCESS) {
	Mach_SetErrno(Compat_MapCode(status));
	return -1;
    }
    return 0;
}



/*
 *----------------------------------------------------------------------
 *
 * Sig_SigvecStub --
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
int
Sig_SigvecStub(sig, newVectorPtr, oldVectorPtr)
    int 		sig;		/* Signal to set the vector for. */
    struct sigvec	*newVectorPtr;	/* New vector. */
    struct sigvec	*oldVectorPtr;	/* Old vector. */
{
    int 		spriteSignal;	/* Equivalent signal for Sprite */
    Sig_Action 		newAction;	/* Action to take */
    Sig_Action 		oldAction;	/* Former action */
    ReturnStatus 	status;		/* Generic result code */
    struct sigvec newVector;
    struct sigvec oldVector;
    Proc_ControlBlock	*procPtr = Proc_GetActualProc();
    Address		dummy;

    if (debugSigStubs) {
	printf("Sig_SigvecStub(%d, %x, %x)\n", sig, newVectorPtr,
		oldVectorPtr);
    }

    /*
     * Set magic flag to indicate we're in Unix signal mode.
     */
    procPtr->unixProgress = PROC_PROGRESS_UNIX;

    status = Compat_UnixSignalToSprite(sig, &spriteSignal);
    if (status == FAILURE || spriteSignal == NULL) {
	Mach_SetErrno(EINVAL);
	return -1;
    }
    if (newVectorPtr != (struct sigvec *)NULL) {
	status = Vm_CopyIn(sizeof(struct sigvec), (Address)newVectorPtr,
			   (Address)&newVector);
	if (status != SUCCESS) {
	    Mach_SetErrno(Compat_MapCode(status));
	    return -1;
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
		newAction.handler = (int (*)())newVector.sv_handler;
	}
	status = Compat_UnixSigMaskToSprite(newVector.sv_mask,
					    &newAction.sigHoldMask);
	if (status == FAILURE) {
	    printf("Sig_SigvecStub: compat failure\n");
	    Mach_SetErrno(EINVAL);
	    return -1;
	}
	/*
	 * Make sure that the signal is in range.
	 */
	if (spriteSignal < SIG_MIN_SIGNAL || spriteSignal >= SIG_NUM_SIGNALS || 
	spriteSignal == SIG_KILL || spriteSignal == SIG_SUSPEND) {
	    printf("Sig_SigvecStub: bad signal %d\n", spriteSignal);
	    Mach_SetErrno(EINVAL);
	    return -1;
	}
	/* 
         * There are two cases:
         *
	 *    1) The current action really contains a handler to call.  Thus
	 *	the current action is SIG_HANDLE_ACTION.
	 *    2) The current action is one of the other four actions.
	 */
	if (procPtr->sigActions[spriteSignal] > SIG_NUM_ACTIONS) {
	    oldAction.action = SIG_HANDLE_ACTION;
	    oldAction.handler = (int (*)())procPtr->sigActions[spriteSignal];
	    oldAction.sigHoldMask = procPtr->sigMasks[spriteSignal];
	} else {
	    if (procPtr->sigActions[spriteSignal]
	        == sigDefActions[spriteSignal]) {
		oldAction.action = SIG_DEFAULT_ACTION;
	    } else {
		oldAction.action = procPtr->sigActions[spriteSignal];
	    }
	}

	/*
	 * Make sure that the action is valid.
	 */

	if (newAction.action < 0 || newAction.action > SIG_NUM_ACTIONS) {
	  printf("Sig_SigvecStub: invalid action %d\n", newAction.action);
	  Mach_SetErrno(EINVAL);
	  return -1;
	}

	if (newAction.action == SIG_DEFAULT_ACTION) {
	    newAction.action = sigDefActions[spriteSignal];
	}

      /*
       * Store the action.  If it is SIG_HANDLE_ACTION then the handler
       * is stored in place of the action.
       */
      if (newAction.action == SIG_HANDLE_ACTION) {
	  if (Vm_CopyIn(4, (Address) ((unsigned int) (newAction.handler)), 
	      (Address) &dummy) != SUCCESS) {
	      printf("Sig_SigvecStub: copy in fault\n");
	      Mach_SetErrno(EFAULT);
	      return -1;
	  }
	  procPtr->sigMasks[spriteSignal] =  (sigBitMasks[spriteSignal]
	      | newAction.sigHoldMask) & sigCanHoldMask;
	  procPtr->sigActions[spriteSignal] = (unsigned int) newAction.handler;
      } else if (newAction.action == SIG_IGNORE_ACTION) {

	  /*
	   * Only actions that can be blocked can be ignored.  This prevents a
	   * user from ignoring a signal such as a bus error which would cause
	   * the process to take a bus error repeatedly.
	   */
	  printf("Sig_SigvecStub: ignore\n");
	  if (sigBitMasks[spriteSignal] & sigCanHoldMask) {
	      procPtr->sigActions[spriteSignal] = SIG_IGNORE_ACTION;
	      SigClearPendingMask(procPtr, spriteSignal);
	  } else {
	      Mach_SetErrno(EINVAL);
	      return -1;
	  }
	  procPtr->sigMasks[spriteSignal] = 0;
      } else {
	  printf("Sig_SigvecStub: default\n");
	  procPtr->sigActions[spriteSignal] = newAction.action;
	  procPtr->sigMasks[spriteSignal] = 0;
      }
    }
    if (oldVectorPtr != NULL) {
	switch (oldAction.action) {

	  case SIG_DEFAULT_ACTION:
	      oldVector.sv_handler = SIG_DFL;
	      break;

	  case SIG_IGNORE_ACTION:
	      oldVector.sv_handler = SIG_IGN;
	      break;

	  default:
	      oldVector.sv_handler = (void (*)())oldAction.handler;
	      break;
	  }
	  (void) Compat_SpriteSigMaskToUnix(oldAction.sigHoldMask, 
	                                    &oldVector.sv_mask);
          oldVector.sv_flags = 0;
	  status = Vm_CopyOut(sizeof(oldVector), (Address)&oldVector,
			    (Address)oldVectorPtr);
	  if (status != SUCCESS) {
	      Mach_SetErrno(EFAULT);
	      return -1;
	  }
      }
      return 0;
}


/*
 *----------------------------------------------------------------------
 *
 * Sig_SigblockStub --
 *
 *	Procedure to map from Unix sigblock system call to Sprite.
 *
 * Results:
 *	Error code is returned upon error.  Otherwise SUCCESS is returned.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *
 *----------------------------------------------------------------------
 */
int
Sig_SigblockStub(mask)
    int mask;			/* additional bits to mask */
{
    int spriteMask = 0;		/* equivalent mask for Sprite */
    int oldSpriteMask;		/* old mask, in Sprite terms */
    ReturnStatus status;	/* generic result code */
    register	Proc_ControlBlock	*procPtr = Proc_GetActualProc();
    int oldMask;

    if (debugSigStubs) {
	printf("Sig_SigblockStub(%x)\n", mask);
    }
    status = Compat_UnixSigMaskToSprite(mask, &spriteMask);
    if (status == FAILURE) {
	Mach_SetErrno(EINVAL);
	return -1;
    }
    oldSpriteMask = procPtr->sigHoldMask;
    if (status == FAILURE) {
	Mach_SetErrno(EINVAL);
	return -1;
    }

    procPtr->sigHoldMask = (spriteMask | oldSpriteMask) & sigCanHoldMask;
    procPtr->specialHandling = 1;
    status = Compat_SpriteSigMaskToUnix(oldSpriteMask, &oldMask);
    if (status == FAILURE) {
	Mach_SetErrno(EINVAL);
	return -1;
    }
    return oldMask;
}


/*
 *----------------------------------------------------------------------
 *
 * Sig_SigsetmaskStub --
 *
 *	Procedure to map from Unix sigsetmask system call to Sprite.
 *
 * Results:
 *	Error code is returned upon error.  Otherwise SUCCESS is returned.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *
 *----------------------------------------------------------------------
 */
int
Sig_SigsetmaskStub(mask)
    int mask;			/* new mask */
{
    int spriteMask = 0;		/* equivalent mask for Sprite */
    int oldSpriteMask;		/* old mask, in Sprite terms */
    ReturnStatus status;	/* generic result code */
    int oldMask;
    register	Proc_ControlBlock	*procPtr;

    if (debugSigStubs) {
	printf("Sig_SigsetmaskStub(%x)\n");
    }

    procPtr = Proc_GetActualProc();
    status = Compat_UnixSigMaskToSprite(mask,&spriteMask);
    if (status == FAILURE) {
	Mach_SetErrno(EINVAL);
	return -1;
    }
    procPtr->sigHoldMask = spriteMask & sigCanHoldMask;
    oldSpriteMask = procPtr->sigHoldMask;
    procPtr->sigHoldMask = spriteMask & sigCanHoldMask;
    procPtr->specialHandling = 1;
    status = Compat_SpriteSigMaskToUnix(oldSpriteMask, &oldMask);
    if (status == FAILURE) {
	Mach_SetErrno(EINVAL);
	return -1;
    }
    return oldMask;
}


/*
 *----------------------------------------------------------------------
 *
 * Sig_SigpauseStub --
 *
 *	Procedure to map from Unix sigpause system call to Sprite.
 *
 * Results:
 *	Error code is returned upon error.  Otherwise SUCCESS is returned.
 *
 * Side effects:
 *	Side effects associated with the system call.
 *
 *----------------------------------------------------------------------
 */
int
Sig_SigpauseStub(mask)
    int mask;			/* new mask */
{
    int spriteMask = 0;		/* equivalent mask for Sprite */
    ReturnStatus status;	/* generic result code */

    if (debugSigStubs) {
	printf("Sig_Sigpause\n");
    }
    status = Compat_UnixSigMaskToSprite(mask,&spriteMask);
    if (status == FAILURE) {
	Mach_SetErrno(EINVAL);
	return -1;
    }
    status = Sig_Pause(spriteMask);
    if (debugSigStubs) {
	printf("Sig_Sigpause done\n");
    }
    Mach_SetErrno(EINTR);
    if (status == GEN_ABORTED_BY_SIGNAL) {
	Proc_GetCurrentProc()->unixProgress = PROC_PROGRESS_RESTART;
	printf("Sigpause: setting RESTART\n");
	
    }
    return -1;
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
/*ARGSUSED*/
int
Sig_SigstackStub(ss, oss)
    struct sigstack *ss, *oss;
{
    struct sigstack oldStack;

    if (debugSigStubs) {
	printf("Sig_SigstackStub\n");
    }
    if (oss != NULL) {
	oldStack.ss_sp = 0;
	oldStack.ss_onstack = 0;
	Vm_CopyOut(sizeof(struct sigstack), (Address)&oldStack,
	                  (Address)oss);
    }
    return 0;
}
