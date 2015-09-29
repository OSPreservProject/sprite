/* 
 * getppid.c --
 *
 *	UNIX getppid() for the Sprite server.
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
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/lib/c/emulator/RCS/getppid.c,v 1.1 92/03/13 20:39:52 kupfer Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <mach.h>
#include <sprite.h>
#include <compatInt.h>
#include <spriteEmuInt.h>
#include <spriteSrv.h>


/*
 *----------------------------------------------------------------------
 *
 * getppid --
 *
 *	Procedure to map from Unix getppid system call to Sprite Proc_GetIDs.
 *
 * Results:
 *	UNIX_ERROR is returned upon error, with the actual error code
 *	stored in errno.  Upon success, the ID of the parent of the current
 *	process is returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
getppid()
{
    ReturnStatus status;	/* result returned by Proc_GetIDs */
    Proc_PID parentPid;		/* ID of parent of current process */
    kern_return_t kernStatus;
    Proc_PID dummyPid;
    int dummyUid;
    Boolean sigPending;

    kernStatus = Proc_GetIDsStub(SpriteEmu_ServerPort(), &dummyPid,
				 &parentPid, &dummyUid, &dummyUid,
				 &sigPending);
    status = (kernStatus == KERN_SUCCESS) ? 
	SUCCESS : Utils_MapMachStatus(kernStatus);
    if (sigPending) {
	SpriteEmu_TakeSignals();
    }
    if (status != SUCCESS) {
	errno = Compat_MapCode(status);
	return(UNIX_ERROR);
    } else {
	return(parentPid);
    }
}

