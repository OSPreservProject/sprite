/* 
 * getgroups.c --
 *
 *	UNIX getgroups() for the Sprite server.
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
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/lib/c/emulator/RCS/getgroups.c,v 1.1 92/03/13 20:39:31 kupfer Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <mach.h>
#include <sprite.h>
#include <compatInt.h>
#include <spriteEmuInt.h>
#include <spriteSrv.h>


/*
 *----------------------------------------------------------------------
 *
 * getgroups --
 *
 *	Procedure to map from Unix getgroups system call to 
 *	Sprite Proc_GetGroupIDs.
 *
 * Results:
 *      UNIX_SUCCESS    - the call was successful.
 *      UNIX_ERROR      - the call was not successful.
 *                        The actual error code stored in errno.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
getgroups(gidsetlen, gidset)
    int gidsetlen;
    int *gidset;
{
    ReturnStatus status;	/* result returned by Proc_GetGroupIDs */
    int	numGids;
    kern_return_t kernStatus;
    Boolean sigPending;

    numGids = gidsetlen;
    kernStatus = Proc_GetGroupIDsStub(SpriteEmu_ServerPort(), &numGids,
				      (vm_address_t)gidset, &status,
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
	if (numGids > gidsetlen) {
	    numGids = gidsetlen;
	}
	return(numGids);
    }
}

