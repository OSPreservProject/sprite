/* 
 * setpgrp.c --
 *
 *	UNIX setpgrp() for the Sprite server.
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
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/lib/c/emulator/RCS/setpgrp.c,v 1.1 92/03/13 20:41:43 kupfer Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <mach.h>
#include <sprite.h>
#include <compatInt.h>
#include <spriteEmuInt.h>
#include <spriteSrv.h>


/*
 *----------------------------------------------------------------------
 *
 * setpgrp --
 *
 *	Procedure to map from Unix setpgrp system call to Sprite 
 *	Proc_SetFamilyID. 
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
setpgrp(pid, pgrp)
    pid_t pid;			/* Process to set the pid for. */
    pid_t pgrp;			/* Name of group. */
{
    ReturnStatus status;	/* result returned by Proc_SetFamilyID */
    kern_return_t kernStatus;
    Boolean sigPending;

    if (pid == 0) {
	pid = PROC_MY_PID;
    }

    kernStatus = Proc_SetFamilyIDStub(SpriteEmu_ServerPort(),
				      (Proc_PID)pid, (Proc_PID)pgrp,
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

