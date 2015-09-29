/* 
 * killpg.c --
 *
 *	Procedure to map from Unix kill system call to Sprite Sig_Send call.
 *	Note: many Unix signals are not supported under Sprite.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: killpg.c,v 1.1 88/06/19 14:31:35 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "sig.h"

#include "compatInt.h"
#include <signal.h>
#include <errno.h>


/*
 *----------------------------------------------------------------------
 *
 * killpg --
 *
 *	Procedure to map from Unix killpg system call to Sprite 
 *	Sig_Send.
 *
 * Results:
 *	UNIX_ERROR is returned upon error, with the actual error code
 *	stored in errno.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
killpg(pgrp, sig)
    int pgrp;
    int sig;
{
    ReturnStatus status;
    int		 spriteSignal;

    status = Compat_UnixSignalToSprite(sig, &spriteSignal);
    if (status == FAILURE || (spriteSignal == NULL && sig != 0)) {
	errno = EINVAL;
	return(UNIX_ERROR);
    }
    status = Sig_Send(spriteSignal, pgrp, TRUE);
    if (status != SUCCESS) {
	errno = Compat_MapCode(status);
	return(UNIX_ERROR);	
    } else {
	return(UNIX_SUCCESS);
    }
}
