/* 
 * close.c --
 *
 *	UNIX close() for the Sprite server.
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
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/lib/c/emulator/RCS/close.c,v 1.1 92/03/13 20:38:59 kupfer Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <mach.h>
#include <sprite.h>
#include <compatInt.h>
#include <spriteEmuInt.h>
#include <spriteSrv.h>


/*
 *----------------------------------------------------------------------
 *
 * close --
 *
 *	Procedure to map from Unix close system call to Sprite Fs_Close.
 *
 * Results:
 *	UNIX_ERROR is returned upon error, with the actual error code
 *	stored in errno.  UNIX_SUCCESS is returned upon success.
 *
 * Side effects:
 *	The steamId passed to close is no longer associated with an open 
 *	file.
 *
 *----------------------------------------------------------------------
 */

int
close(streamId)
    int streamId;		/* identifier for stream to close */
{
    ReturnStatus status;	/* result returned by Fs_Close */
    kern_return_t kernStatus;
    Boolean sigPending;

    kernStatus = Fs_CloseStub(SpriteEmu_ServerPort(), streamId, &status,
			      &sigPending);
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

