/* 
 * utimes.c --
 *
 *	Procedure to map from Unix utimes system call to Sprite system call.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/lib/c/emulator/RCS/utimes.c,v 1.4 92/03/13 20:43:04 kupfer Exp $ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include <mach.h>
#include <mach/message.h>
#include <fs.h>

#include "compatInt.h"
#include <errno.h>
#include <sys/time.h>
#include <spriteEmuInt.h>
#include <spriteSrv.h>
#include <string.h>


/*
 *----------------------------------------------------------------------
 *
 * utimes --
 *
 *	Procedure to map from Unix utimes system call to Sprite 
 *	Fs_SetAttributes system call.
 *
 * Results:
 *      UNIX_SUCCESS    - the call was successful.
 *      UNIX_ERROR      - the call was not successful.
 *                        The actual error code stored in errno.
 *
 * Side effects:
 *	The protection of the specified file is modified.
 *
 *----------------------------------------------------------------------
 */

int
utimes(path, tvp)
    char *path;
    struct timeval tvp[2];
{
    ReturnStatus status;
    kern_return_t kernStatus;
    Fs_Attributes attributes;	/* struct containing all file attributes,
				 * only access/modify times looked at. */
    mach_msg_type_number_t pathNameLength = strlen(path) + 1;
    Boolean sigPending;

    attributes.accessTime.seconds = tvp[0].tv_sec;
    attributes.accessTime.microseconds = tvp[0].tv_usec;
    attributes.dataModifyTime.seconds = tvp[1].tv_sec;
    attributes.dataModifyTime.microseconds = tvp[1].tv_usec;
    kernStatus = Fs_SetAttrStub(SpriteEmu_ServerPort(), path,
				pathNameLength, FS_ATTRIB_FILE, attributes,
				FS_SET_TIMES, &status, &sigPending);
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
