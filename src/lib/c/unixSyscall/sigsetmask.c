/* 
 * sigsetmask.c --
 *
 *	Procedure to map from Unix sigsetmask system call to Sprite.
 *	Note: many Unix signals are not supported under Sprite.
 *
 * Copyright (C) 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: sigsetmask.c,v 1.1 88/06/19 14:32:01 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "sig.h"

#include "compatInt.h"
#include <signal.h>
#include <errno.h>


/*
 *----------------------------------------------------------------------
 *
 * sigsetmask --
 *
 *	Procedure to map from Unix sigsetmask system call to Sprite 
 *	Sig_SetHoldMask.
 *
 * Results:
 *	UNIX_ERROR is returned upon error, with the actual error code
 *	stored in errno.  Upon success, the previous signal mask is returned.
 *
 * Side effects:
 *	The current signal mask is modified.
 *
 *----------------------------------------------------------------------
 */

int
sigsetmask(mask)
    int mask;			/* new mask */
{
    int spriteMask = 0;		/* equivalent mask for Sprite */
    int oldSpriteMask;		/* old mask, in Sprite terms */
    int oldMask = 0;		/* old mask, in Unix terms */
    ReturnStatus status;	/* generic result code */

    status = Compat_UnixSigMaskToSprite(mask,&spriteMask);
    if (status == FAILURE) {
	errno = EINVAL;
	return( UNIX_ERROR);
    }
    status = Sig_SetHoldMask(spriteMask, &oldSpriteMask);
    if (status != SUCCESS) {
	errno = Compat_MapCode(status);
	/*
	 * Does anyone check for -1 upon return or does it look like a mask?
	 */
	return(UNIX_ERROR);	
    } else {
	status = Compat_SpriteSigMaskToUnix(oldSpriteMask,&oldMask);
	if (status == FAILURE) {
	    errno = EINVAL;
	    return( UNIX_ERROR);
	}
	return(oldMask);
    }
}
