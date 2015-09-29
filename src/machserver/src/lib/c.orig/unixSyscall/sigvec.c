/* 
 * sigvec.c --
 *
 *	Procedure to map from Unix sigvec system call to Sprite.
 *
 *	Note: None of the special flags in the sigvec structure is supported
 *	as yet. Eventually we could support the signal stack, if we're kludgey,
 *	the interruption, if we use the raw Sprite system calls for the
 *	emulation, and the reset handler (Sun UNIX only) bits if desired.
 *
 *	Note further: many Unix signals are not supported under Sprite.
 *
 * Copyright (C) 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/lib/c/unixSyscall/RCS/sigvec.c,v 1.4 90/12/13 17:07:42 shirriff Exp $ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "sig.h"

#include "compatInt.h"
#include <signal.h>
#include <errno.h>

/*
 * Array of signal handlers for UNIX signals. Need the extra indirection
 * to provide the proper UNIX signal number to the handler, since it may
 * decide what to do based on that number...We'll worry about codes later.
 */
void	(*unixHandlers[NSIG])();

/*
 *----------------------------------------------------------------------
 *
 * unixHandleSig --
 *
 *	Procedure to handle the receipt of a Sprite signal. Maps the
 *	Sprite signal to a UNIX signal and calls the right handler.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The signal handler for the UNIX signal is called. Note that since
 *	several UNIX signals may map to the same Sprite signal, there
 *	will be problems if different functions are given for the 
 *	overlapping signals.
 *
 *----------------------------------------------------------------------
 */
	/* ARGSUSED */

static
unixHandleSig (spriteSig, spriteCode, sigContext, sigAddr)
    int		spriteSig;	/* Sprite signal being delivered */
    int		spriteCode;	/* Sub code of signal */

    struct sigcontext *sigContext;	/* Context of signal. */
    char	*sigAddr;	/* Address of fault. */
{
    int		unixSig;	/* "Equivalent" UNIX signal */
    
    if ((Compat_SpriteSignalToUnix (spriteSig, &unixSig) == SUCCESS) &&
	(unixHandlers[unixSig] != (void (*)()) NULL)) {
	    /*
	     * XXX: Should decode spriteCode and pass sigcontext *
	     */
	    (* unixHandlers[unixSig]) (unixSig, spriteCode, sigContext,
		    sigAddr);
    }
    /*
     * XXX: Should warn about bogus signal handler, yes?
     */
}


/*
 *----------------------------------------------------------------------
 *
 * sigvec --
 *
 *	Procedure to map from Unix sigvec system call to Sprite Sig_SetAction.
 *
 * Results:
 *	UNIX_ERROR is returned upon error, with the actual error code
 *	stored in errno.  Upon success 0 is returned.
 *
 * Side effects:
 *	The signal handler associated with the specified signal is modified.
 *
 *----------------------------------------------------------------------
 */
int
sigvec(sig, newVectorPtr, oldVectorPtr)
    int 		sig;		/* Signal to set the vector for. */
    struct sigvec	*newVectorPtr;	/* New vector. */
    struct sigvec	*oldVectorPtr;	/* Old vector. */
{
    int 		spriteSignal;	/* Equivalent signal for Sprite */
    Sig_Action 		newAction;	/* Action to take */
    Sig_Action 		oldAction;	/* Former action */
    ReturnStatus 	status;		/* Generic result code */
    void    	  	(*handler)();	/* New handler for UNIX signal */

    status = Compat_UnixSignalToSprite(sig, &spriteSignal);
    if (status == FAILURE || spriteSignal == NULL) {
	errno = EINVAL;
	return(UNIX_ERROR);
    }
    handler = (void (*)()) NULL;

    if (newVectorPtr != (struct sigvec *)NULL) {
	switch ((int)newVectorPtr->sv_handler) {
	    case SIG_DFL:
		newAction.action = SIG_DEFAULT_ACTION;
		break;
	    case SIG_IGN:
		newAction.action = SIG_IGNORE_ACTION;
		break;
	    default:
		newAction.action = SIG_HANDLE_ACTION;
		newAction.handler = unixHandleSig;
		handler = newVectorPtr->sv_handler;
	}
	status = Compat_UnixSigMaskToSprite(newVectorPtr->sv_mask,
					    &newAction.sigHoldMask);
	if (status == FAILURE) {
	    errno = EINVAL;
	    return(UNIX_ERROR);
	}

	status = Sig_SetAction(spriteSignal, &newAction, &oldAction);
	if (status != SUCCESS) {
	    errno = Compat_MapCode(status);
	    return(UNIX_ERROR);
	} 
    }

    if (oldVectorPtr != NULL) {
	switch (oldAction.action) {
	    case SIG_DEFAULT_ACTION:
		oldVectorPtr->sv_handler = SIG_DFL;
		break;
	    case SIG_IGNORE_ACTION:
		oldVectorPtr->sv_handler = SIG_IGN;
		break;
	    default:
		oldVectorPtr->sv_handler = unixHandlers[sig];
		break;
	}
	(void) Compat_SpriteSigMaskToUnix(oldAction.sigHoldMask, 
					  &oldVectorPtr->sv_mask);
	oldVectorPtr->sv_flags = 0;
    }

    if (newVectorPtr != NULL) {
	unixHandlers[sig] = handler;
    }

    return(UNIX_SUCCESS);
}
