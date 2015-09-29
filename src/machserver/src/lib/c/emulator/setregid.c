/* 
 * setregid.c --
 *
 *	UNIX setregid() for the Sprite server.
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
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/lib/c/emulator/RCS/setregid.c,v 1.1 92/03/13 20:41:47 kupfer Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <mach.h>
#include <sprite.h>
#include <compatInt.h>
#include <spriteEmuInt.h>
#include <spriteSrv.h>


/*
 *----------------------------------------------------------------------
 *
 * setregid --
 *
 *	Procedure to map from Unix setregid system call to Sprite Proc_SetIDs.
 *	Sprite doesn't have the notion of real and effective groud IDs;
 *	instead, both gid arguments become the set of Sprite group IDs for
 *	current process.
 *
 * Results:
 *      UNIX_SUCCESS    - the call was successful.
 *      UNIX_ERROR      - the call was not successful.
 *                        The actual error code stored in errno.
 *
 * Side effects:
 *	The previous group IDs are deleted.
 *
 *----------------------------------------------------------------------
 */

int
setregid(rgid, egid)
    int	rgid, egid;
{
    ReturnStatus status = SUCCESS;
    int array[2];
    int num = 0;
    kern_return_t kernStatus;
    Boolean sigPending;

    /*
     * Make the rgid and egid the group IDs for the process. If a gid is
     * -1, it is ignored.
     */

    if (rgid != -1) {
	array[0] = rgid;
	num = 1;
	if (egid != rgid && egid != -1) {
	    array[1] = egid;
	    num++;
	}
    } else if (egid != -1) {
	array[0] = egid;
	num++;
    }
    if (num > 0) {
	kernStatus = Proc_SetGroupIDsStub(SpriteEmu_ServerPort(), num,
					  (vm_address_t)array, &status,
					  &sigPending);
	if (kernStatus != KERN_SUCCESS) {
	    status = Utils_MapMachStatus(kernStatus);
	}
	if (sigPending) {
	    SpriteEmu_TakeSignals();
	}
    }

    if (status != SUCCESS) {
	errno = Compat_MapCode(status);
	return(UNIX_ERROR);
    } else {
	return(UNIX_SUCCESS);
    }
}

