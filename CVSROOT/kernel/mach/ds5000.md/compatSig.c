/*
 * compatSig.c --
 *
 * 	Returns the Sprite signal number corresponding to a Unix signal.
 *
 * Copyright (C) 1986 Regents of the University of California
 * All rights reserved.
 *
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"

#ifndef NULL
#define NULL 0
#endif

#include "compatInt.h"
#include "user/sig.h"
#include "user/sys/signal.h"
#include "machInt.h"
#include "mach.h"

#include "compatSig.h"


/*
 *----------------------------------------------------------------------
 *
 * Compat_UnixSignalToSprite --
 *
 *	Given a Unix signal, return the closest corresponding Sprite signal
 *	number. Signal 0 is special-cased to map to sprite signal 0 (NULL).
 *	Some programs use kill(pid, 0) to see if pid exists....
 *
 * Results:
 *	A Sprite signal number is returned, assuming the Unix signal is
 *	in a valid range.  Note that the Sprite "signal number" may be
 *	NULL (0) if there's no matching signal.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Compat_UnixSignalToSprite(signal, spriteSigPtr)
    int signal;
    int *spriteSigPtr;
{
    if (signal >= 0 && signal <= numSignals) {
	*spriteSigPtr = compat_UnixSigToSprite[signal];
	return(SUCCESS);
    } else {
	return(FAILURE);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * Compat_SpriteSignalToUnix --
 *
 *	Given a Sprite signal, return the closest corresponding Unix signal
 *	number.
 *
 * Results:
 *	A Unix signal number is returned, assuming the Sprite signal is
 *	in a valid range. 
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Compat_SpriteSignalToUnix(signal, unixSigPtr)
    int signal;
    int *unixSigPtr;
{
    if (signal >= 0 && signal <= numSignals) {
	*unixSigPtr = spriteToUnix[signal];
	return(SUCCESS);
    } else {
	return(FAILURE);
    }
}


/*
 *----------------------------------------------------------------------
 *
 *  Compat_UnixSigMaskToSprite --
 *
 *	Given a Unix signal mask, return the corresponding Sprite signal
 *	mask.
 *
 * Results:
 *	A Sprite signal mask is returned, assuming the Unix signal mask is
 *	valid.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Compat_UnixSigMaskToSprite(unixMask, spriteMaskPtr)
    int unixMask;
    int *spriteMaskPtr;
{
    int i;
    int signal;
    ReturnStatus status;

    *spriteMaskPtr = 0;
    for (i = 1; i <= NSIG; i++) {
	if (unixMask & (1 << (i - 1))) {
	    status = Compat_UnixSignalToSprite(i, &signal);
	    if (status == FAILURE) {
		return(FAILURE);
	    }
	    if (signal != NULL) {
		*spriteMaskPtr |= 1 << (signal - 1);
	    }
	}
    }
#ifdef COMPAT_DEBUG
    printf("Unix mask = <%x> Sprite mask = <%x>\n", 
		unixMask, *spriteMaskPtr);
#endif
    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * Compat_SpriteSigMaskToUnix --
 *
 *	Given a Sprite signal mask, return the corresponding Unix signal
 *	mask.
 *
 * Results:
 *	A Unix signal mask is returned, assuming the Sprite signal mask is
 *	valid.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Compat_SpriteSigMaskToUnix(SpriteMask, UnixMaskPtr)
    int SpriteMask;
    int *UnixMaskPtr;
{
    int i;
    int signal;
    ReturnStatus status;

    *UnixMaskPtr = 0;
    for (i = 1; i <= SIG_NUM_SIGNALS; i++) {
	if (SpriteMask & (1 << (i - 1))) {
	    status = Compat_SpriteSignalToUnix(i, &signal);
	    if (status == FAILURE) {
		return(FAILURE);
	    }
	    if (signal != NULL) {
		*UnixMaskPtr |= 1 << (signal - 1);
	    }
	}
    }
    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * Compat_GetSigHoldMask --
 *
 *	Return the current signal mask.
 *
 * Results:
 *	The current signal mask (in Sprite terms) is returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

#define MASK_ALL_SIGNALS 0xFFFFFFFF

ReturnStatus
Compat_GetSigHoldMask(maskPtr)
    int *maskPtr;
{
    ReturnStatus status;
    Address	usp;
    extern Mach_State	*machCurStatePtr;

    /*
     * To modify the hold mask we need to get the old one by
     * calling Sig_SetHoldMask to get the current mask.  Since we
     * don't know what to set the mask to, set the mask to mask ALL 
     * signals and then reset it again to the proper value.
     */

    usp = (Address) (machCurStatePtr->userState.regState.regs[SP] - 4);

    status = Sig_SetHoldMask((int) MASK_ALL_SIGNALS, (int *) usp);
    if (status != SUCCESS) {
	return(status);
    }
    (void)Vm_CopyIn(sizeof(int), usp, (Address) maskPtr);
    status = Sig_SetHoldMask(*maskPtr, (int *) NULL);
    return(status);
}
