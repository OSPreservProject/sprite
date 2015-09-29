/* 
 * setreuid.c --
 *
 *	UNIX setreuid() for the Sprite server.
 *
 * Copyright 1988, 1992 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that this copyright
 * notice appears in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/lib/c/emulator/RCS/setreuid.c,v 1.1 92/03/13 20:41:51 kupfer Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <mach.h>
#include <sprite.h>
#include <compatInt.h>
#include <spriteEmuInt.h>
#include <spriteSrv.h>


/*
 *----------------------------------------------------------------------
 *
 * setreuid --
 *
 *	Procedure to map from Unix setreuid system call to Sprite Proc_SetIDs.
 *
 * Results:
 *	UNIX_ERROR is returned upon error, with the actual error code
 *	stored in errno.  Upon success, UNIX_SUCCESS is returned.
 *
 * Side effects:
 *	The real and effective user ID's of the process are modified as
 *	specified, if the user is privileged to do so.
 *
 *----------------------------------------------------------------------
 */

int
setreuid(ruid, euid)
    int	ruid, euid;
{
    ReturnStatus status;	/* result returned by Proc_SetIDs */
    kern_return_t kernStatus;
    Boolean sigPending;

    if (ruid == -1) {
	ruid = PROC_NO_ID;
    }
    if (euid == -1) {
	euid = PROC_NO_ID;
    }
    kernStatus = Proc_SetIDsStub(SpriteEmu_ServerPort(), ruid, euid,
				 &status, &sigPending);
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

