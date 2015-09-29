/* 
 * umask.c --
 *
 *	Procedure to map from Unix umask system call to Sprite.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/lib/c/emulator/RCS/umask.c,v 1.3 92/03/12 19:22:30 kupfer Exp $ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include <mach.h>
#include <fs.h>

#include "compatInt.h"
#include <spriteEmuInt.h>

/*
 * These are the bits to invert and pass to the Sprite system call.
 */

#define PERMISSION_MASK 0777


/*
 *----------------------------------------------------------------------
 *
 * umask --
 *
 *	Procedure to map from Unix umask system call to Sprite
 *	Fs_SetDefPerm.
 *
 * Results:
 *	On success, the former value of umask is returned.  If
 *	Fs_SetDefPerm returns an error,	UNIX_ERROR is returned and the
 *	actual error code is stored in errno.  
 *
 * Side effects:
 *	The default protection of files created by the current process
 *	is changed.
 *
 *----------------------------------------------------------------------
 */

int
umask(newMask)
    int newMask;
{
    ReturnStatus status;
    kern_return_t kernStatus;
    int oldMask;
    Boolean sigPending;

    /*
     * Sprite default permissions are the logical NOT of Unix permissions.
     */

    newMask = (~newMask) & PERMISSION_MASK;
    
    kernStatus = Fs_SetDefPermStub(SpriteEmu_ServerPort(), newMask,
				   &oldMask, &sigPending);
    status = (kernStatus == KERN_SUCCESS) ? 
	SUCCESS : Utils_MapMachStatus(kernStatus);
    if (sigPending) {
	SpriteEmu_TakeSignals();
    }
    if (status != SUCCESS) {
	errno = Compat_MapCode(status);
	return(UNIX_ERROR);
    } else {
	oldMask = (~oldMask) & PERMISSION_MASK;
	return(oldMask);
    }
}
