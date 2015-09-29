/* 
 * getpid.c --
 *
 *	Source code for the getpid library procedure.
 *
 * Copyright 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/lib/c/emulator/RCS/getpid.c,v 1.3 92/03/12 19:22:33 kupfer Exp $ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include <mach.h>
#include <proc.h>
#include <compatInt.h>
#include <spriteEmuInt.h>
#include <status.h>


/*
 *----------------------------------------------------------------------
 *
 * getpid --
 *
 *	Procedure to map from Unix getpid system call to Sprite 
 *	Proc_GetIDsStub.
 *
 * Results:
 *	UNIX_ERROR is returned upon error, with the actual error code
 *	stored in errno.  Upon success, the process ID of the current
 *	process is returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
    
int
getpid()
{
    kern_return_t kernStatus;	/* result returned by Proc_GetIDsStub */
    Proc_PID pid;		/* ID of current process */
    int dummy[3];		/* throw-away values for Proc_GetIDsStub */
    Boolean sigPending;

    kernStatus = Proc_GetIDsStub(SpriteEmu_ServerPort(), &pid,
				 (Proc_PID *)&dummy[0], &dummy[1],
				 &dummy[2], &sigPending);
    if (sigPending) {
	SpriteEmu_TakeSignals();
    }
    if (kernStatus != KERN_SUCCESS) {
	errno = Compat_MapCode(Utils_MapMachStatus(kernStatus));
	return UNIX_ERROR;
    } else {
	return (int)pid;
    }
}
