/* 
 * rename.c --
 *
 *	UNIX rename() for the Sprite server.
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
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/lib/c/emulator/RCS/rename.c,v 1.1 92/03/13 20:41:36 kupfer Exp $ SPRITE (Berkeley)";
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
 * rename --
 *
 *	Procedure to map from Unix rename system call to Sprite Fs_Rename.
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
rename(from, to)
    char *from;
    char *to;
{
    ReturnStatus status;
    kern_return_t kernStatus;
    mach_msg_type_number_t fromNameLength = strlen(from) + 1;
    mach_msg_type_number_t toNameLength = strlen(to) + 1;
    Boolean sigPending;

    kernStatus = Fs_RenameStub(SpriteEmu_ServerPort(), from,
			       fromNameLength, to, toNameLength, &status,
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

