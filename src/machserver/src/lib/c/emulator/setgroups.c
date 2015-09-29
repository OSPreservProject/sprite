/* 
 * setgroups.c --
 *
 *	Procedure to map from Unix setgroups system call to Sprite.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/lib/c/emulator/RCS/setgroups.c,v 1.2 92/04/02 21:43:00 kupfer Exp $ SPRITE (Berkeley)";
#endif not lint

#include <mach.h>
#include <sprite.h>
#include <proc.h>
#include <compatInt.h>
#include <spriteEmuInt.h>
#include <spriteSrv.h>


/*
 *----------------------------------------------------------------------
 *
 * setgroups --
 *
 *	Procedure to map from Unix setgroups system call to 
 *	Sprite Proc_SetGroupIDs.
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
setgroups(ngroups, gidset)
    int ngroups;
    int *gidset;
{
    ReturnStatus status;	/* Sprite status code from Proc_SetGroupIDs */
    kern_return_t kernStatus;	/* Mach status code */
    Boolean sigPending;

    kernStatus = Proc_SetGroupIDsStub(SpriteEmu_ServerPort(), ngroups,
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
	return(UNIX_SUCCESS);
    }
}
