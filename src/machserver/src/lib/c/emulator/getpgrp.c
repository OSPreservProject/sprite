/* 
 * getpgrp.c --
 *
 *	UNIX getpgrp() for the Sprite server.
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
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/lib/c/emulator/RCS/getpgrp.c,v 1.1 92/03/13 20:39:47 kupfer Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <mach.h>
#include <sprite.h>
#include <compatInt.h>
#include <spriteEmuInt.h>
#include <spriteSrv.h>


/*
 *----------------------------------------------------------------------
 *
 * getpgrp --
 *
 *	Procedure to map from Unix getpgrp system call to Sprite 
 *	Proc_GetFamilyID. 
 *
 * Results:
 *	UNIX_ERROR is returned upon error, with the actual error code
 *	stored in errno.  Otherwise the family id of the given process is
 *	returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

pid_t
getpgrp(pid)
    pid_t pid;			/* Process to get the process group for. */
{
    ReturnStatus status;
    Proc_PID familyID;		/* Family ID of process. */
    kern_return_t kernStatus;
    Boolean sigPending;

    if (pid == 0) {
	pid = PROC_MY_PID;
    }
    kernStatus = Proc_GetFamilyIDStub(SpriteEmu_ServerPort(),
				      (Proc_PID)pid, &status, &familyID,
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
	return((pid_t)familyID);
    }
}

