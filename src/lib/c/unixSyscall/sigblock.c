/* 
 * sigblock.c --
 *
 *	Procedure to map from Unix sigblock system call to Sprite.
 *	Note: many Unix signals are not supported under Sprite.
 *
 * Copyright (C) 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: sigblock.c,v 1.2 88/07/29 18:39:27 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "sig.h"

#include "compatInt.h"
#include <signal.h>
#include <errno.h>


/*
 *----------------------------------------------------------------------
 *
 * sigblock --
 *
 *	Procedure to map from Unix sigblock system call to Sprite 
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
sigblock(mask)
    int mask;			/* additional bits to mask */
{
    int spriteMask = 0;		/* equivalent mask for Sprite */
    int oldSpriteMask;		/* old mask, in Sprite terms */
    int oldMask = 0;		/* old mask, in Unix terms */
    ReturnStatus status;	/* generic result code */

    status = Compat_UnixSigMaskToSprite(mask,&spriteMask);
    if (status == FAILURE) {
	errno = EINVAL;
	return(UNIX_ERROR);
    }
    status = Compat_GetSigHoldMask(&oldSpriteMask);
    if (status == FAILURE) {
	errno = EINVAL;
	return(UNIX_ERROR);
    }
    status = Sig_SetHoldMask(spriteMask | oldSpriteMask, (int *) NULL);
    if (status != SUCCESS) {
	errno = Compat_MapCode(status);
	return(UNIX_ERROR);	
    } else {
	status = Compat_SpriteSigMaskToUnix(oldSpriteMask,&oldMask);
	if (status == FAILURE) {
	    errno = EINVAL;
	    return(UNIX_ERROR);
	}
	return(oldMask);
    }
}
