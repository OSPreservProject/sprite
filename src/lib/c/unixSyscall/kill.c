/* 
 * kill.c --
 *
 *	Procedure to map from Unix kill system call to Sprite Sig_Send call.
 *	Note: many Unix signals are not supported under Sprite.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/lib/c/unixSyscall/RCS/kill.c,v 1.2 89/04/19 17:07:12 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "sig.h"

#include "compatInt.h"
#include "proc.h"
#include <signal.h>
#include <errno.h>


/*
 *----------------------------------------------------------------------
 *
 * kill --
 *
 *	Procedure to map from Unix kill system call to Sprite 
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
kill(pid, sig)
    int pid;
    int sig;
{
    ReturnStatus status;
    int		 spriteSignal;

    if (pid == 0) {
	return killpg(getpgrp(0), sig);
    }
    status = Compat_UnixSignalToSprite(sig, &spriteSignal);
    if (status == FAILURE || (spriteSignal == NULL && sig != 0)) {
	errno = EINVAL;
	return(UNIX_ERROR);
    }
    status = Sig_Send(spriteSignal, pid, FALSE);
    if (status != SUCCESS) {
	errno = Compat_MapCode(status);
	return(UNIX_ERROR);	
    } else {
	return(UNIX_SUCCESS);
    }
}
