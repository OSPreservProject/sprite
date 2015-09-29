/* 
 * creat.c --
 *
 *	UNIX creat() for the Sprite server.
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
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/lib/c/emulator/RCS/creat.c,v 1.1 92/03/13 20:39:06 kupfer Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <mach.h>
#include <mach/message.h>
#include <sprite.h>
#include <compatInt.h>
#include <fs.h>
#include <spriteEmuInt.h>
#include <spriteSrv.h>
#include <string.h>


/*
 *----------------------------------------------------------------------
 *
 * creat --
 *
 *	Procedure to map from Unix creat system call to Sprite Fs_Open,
 *	with appropriate parameters.
 *
 * Results:
 *	UNIX_ERROR is returned upon error, with the actual error code
 *	stored in errno.  A file descriptor is returned upon success.
 *
 * Side effects:
 *	Creating a file sets up state in the filesystem until the file is
 *	closed.  
 *
 *----------------------------------------------------------------------
 */

int
creat(pathName, permissions)
    char *pathName;		/* The name of the file to create */
    int permissions;		/* Permission mask to use on creation */
{
    int streamId;		/* place to hold stream id allocated by
				 * Fs_Open */
    ReturnStatus status;
    kern_return_t kernStatus;
    mach_msg_type_number_t pathNameLength = strlen(pathName) + 1;
    boolean_t sigPending;

    kernStatus = Fs_OpenStub(SpriteEmu_ServerPort(), pathName,
			     pathNameLength, FS_CREATE|FS_TRUNC|FS_WRITE, 
			     permissions, &status, &streamId, &sigPending);
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
	return(streamId);
    }
}

