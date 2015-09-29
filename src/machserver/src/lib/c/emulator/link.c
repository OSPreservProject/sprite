/* 
 * link.c --
 *
 *	UNIX link() for the Sprite server.
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
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/lib/c/emulator/RCS/link.c,v 1.1 92/03/13 20:40:43 kupfer Exp $ SPRITE (Berkeley)";
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
 * link --
 *
 *	Procedure to map from Unix link system call to Sprite Fs_HardLink.
 *
 * Results:
 *      UNIX_SUCCESS    - the call was successful.
 *      UNIX_ERROR      - the call was not successful.
 *                        The actual error code stored in errno.
 *
 * Side effects:
 *	Cause the two pathnames to refer to the same file.
 *
 *----------------------------------------------------------------------
 */

int
link(name1, name2)
    char *name1;
    char *name2;
{
    ReturnStatus status;
    kern_return_t kernStatus;
    mach_msg_type_number_t nameOneLength = strlen(name1) + 1;
    mach_msg_type_number_t nameTwoLength = strlen(name2) + 1;
    Boolean sigPending;

    kernStatus = Fs_HardLinkStub(SpriteEmu_ServerPort(), name1,
				 nameOneLength, name2, nameTwoLength,
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

