/* 
 * dup.c --
 *
 *	UNIX dup() and dup2() for the Sprite server.
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
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/lib/c/emulator/RCS/dup.c,v 1.1 92/03/13 20:38:08 kupfer Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <mach.h>
#include <sprite.h>
#include <compatInt.h>
#include <fs.h>
#include <spriteEmuInt.h>
#include <spriteSrv.h>


/*
 *----------------------------------------------------------------------
 *
 * dup --
 *
 *	Procedure to map from Unix dup system call to Sprite Fs_GetNewID.
 *
 * Results:
 *	UNIX_ERROR is returned upon error, with the actual error code
 *	stored in errno.  Upon success, the new file descriptor is returned.
 *
 * Side effects:
 *	A new open file descriptor is allocated for the process.
 *
 *----------------------------------------------------------------------
 */

int
dup(oldStreamID)
    int oldStreamID;		/* original stream identifier */
{
    ReturnStatus status;
    kern_return_t kernStatus;
    int newStreamID = FS_ANYID;	/* new stream identifier */
    Boolean sigPending;

    kernStatus = Fs_GetNewIDStub(SpriteEmu_ServerPort(), oldStreamID,
				 &newStreamID, &status, &sigPending);
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
	return(newStreamID);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * dup2 --
 *
 *	Procedure to map from Unix dup2 system call to Sprite Fs_GetNewID.
 *
 * Results:
 *	UNIX_ERROR is returned upon error, with the actual error code
 *	stored in errno.  Upon success, the new file descriptor is returned.
 *
 * Side effects:
 *	A new open file descriptor is allocated for the process.
 *
 *----------------------------------------------------------------------
 */

int
dup2(oldStreamID, newStreamID)
    int oldStreamID;		/* original stream identifier */
    int newStreamID;		/* new stream identifier */
{
    ReturnStatus status;
    kern_return_t kernStatus;
    Boolean sigPending;

    kernStatus = Fs_GetNewIDStub(SpriteEmu_ServerPort(), oldStreamID, 
				 &newStreamID, &status, &sigPending);
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

