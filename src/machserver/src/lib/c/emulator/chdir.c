/* 
 * chdir.c --
 *
 *	UNIX chdir() for the Sprite server.
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
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/lib/c/emulator/RCS/chdir.c,v 1.1 92/03/13 20:37:57 kupfer Exp $ SPRITE (Berkeley)";
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
 * chdir --
 *
 *	Procedure to map from Unix chdir system call to Sprite Fs_ChangeDir.
 *
 * Results:
 *	UNIX_ERROR is returned upon error, with the actual error code
 *	stored in errno.  Upon success, UNIX_SUCCESS is returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
chdir(pathName)
    char *pathName;		/* directory to be made current directory */
{
    ReturnStatus status;	/* result returned by Fs_ChangeDir */
    kern_return_t kernStatus;
    mach_msg_type_number_t pathNameLength = strlen(pathName) + 1;
    Boolean sigPending;

    kernStatus = Fs_ChangeDirStub(SpriteEmu_ServerPort(), pathName,
				  pathNameLength, &status, &sigPending);
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

