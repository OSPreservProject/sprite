/* 
 * mkdir.c --
 *
 *	UNIX mkdir() for the Sprite server.
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
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/lib/c/emulator/RCS/mkdir.c,v 1.1 92/03/13 20:40:49 kupfer Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <mach.h>
#include <mach/message.h>
#include <sprite.h>
#include <compatInt.h>
#include <spriteEmuInt.h>
#include <spriteSrv.h>
#include <string.h>


/*
 *----------------------------------------------------------------------
 *
 * mkdir --
 *
 *	Procedure to map from Unix mkdir system call to Sprite Fs_MakeDir.
 *
 * Results:
 *	UNIX_ERROR is returned upon error, with the actual error code
 *	stored in errno.  Otherwise UNIX_SUCCESS is returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
mkdir(pathName, permissions)
    char *pathName;		/* The name of the directory to create */
    int permissions;		/* Permission mask to use on creation */
{
    ReturnStatus status;
    kern_return_t kernStatus;
    mach_msg_type_number_t pathNameLength = strlen(pathName) + 1;
    Boolean sigPending;

    kernStatus = Fs_MakeDirStub(SpriteEmu_ServerPort(), pathName,
				pathNameLength, permissions, &status,
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

