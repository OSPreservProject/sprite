/* 
 * pipe.c --
 *
 *	UNIX pipe() for the Sprite server.
 *
 * Copyright 1992 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that this copyright
 * notice appears in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/lib/c/emulator/RCS/pipe.c,v 1.1 92/03/13 20:41:25 kupfer Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <mach.h>
#include <sprite.h>
#include <compatInt.h>
#include <spriteEmuInt.h>
#include <spriteSrv.h>


/*
 *----------------------------------------------------------------------
 *
 * pipe --
 *
 *	Procedure to map from Unix pipe system call to Sprite Fs_CreatePipe
 *	call.
 *
 * Results:
 *	Error returned if error returned from Fs_CreatePipe. Otherwise
 *	UNIX_SUCCESS.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
pipe(filedes)
    int	filedes[2];			/* array of stream identifiers */	
{
    ReturnStatus status;
    kern_return_t kernStatus;
    Boolean sigPending;

    kernStatus = Fs_CreatePipeStub(SpriteEmu_ServerPort(), &status, 
				   &(filedes[0]), &(filedes[1]), &sigPending);
    if (kernStatus != KERN_SUCCESS) {
	status = Utils_MapMachStatus(kernStatus);
    }
    if (sigPending) {
	SpriteEmu_TakeSignals();
    }
    if (status != SUCCESS) {
	errno = Compat_MapCode(status);
	return(UNIX_ERROR);
    } else {
	return(UNIX_SUCCESS);
    }
}

