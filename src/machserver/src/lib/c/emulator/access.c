/* 
 * access.c --
 *
 *	UNIX access() for the Sprite server.
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
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/lib/c/emulator/RCS/access.c,v 1.1 92/03/13 20:37:51 kupfer Exp $ SPRITE (Berkeley)";
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
 * access --
 *
 *	Procedure for Unix access call. 
 *
 * Results:
 *	UNIX_SUCCESS if the access mode was valid.
 *	UNIX_FAILURE if the access mode was not valid.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
access(pathName, mode)
    char *pathName;		/* name of file to check */
    int mode;			/* access mode to check for */
{
    ReturnStatus status;
    kern_return_t kernStatus;
    mach_msg_type_number_t pathNameLength = strlen(pathName) + 1;
    Boolean sigPending;

    kernStatus = Fs_AccessStub(SpriteEmu_ServerPort(), pathName,
			       pathNameLength, mode, &status, &sigPending);
    if (kernStatus != KERN_SUCCESS) {
	status = Utils_MapMachStatus(kernStatus);
    }
    if (sigPending) {
	SpriteEmu_TakeSignals();
    }
    if (status != SUCCESS) {
	errno = Compat_MapCode(status);
	return UNIX_ERROR;
    } else {
	return UNIX_SUCCESS;
    }
}

