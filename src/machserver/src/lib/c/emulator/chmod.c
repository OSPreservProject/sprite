/* 
 * chmod.c --
 *
 *	UNIX chmod() and fchmod() for the Sprite server.
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
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/lib/c/emulator/RCS/chmod.c,v 1.1 92/03/13 20:38:01 kupfer Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <mach.h>
#include <mach/message.h>
#include <sprite.h>
#include <compatInt.h>
#include <fs.h>
#include <spriteEmuInt.h>
#include <spriteSrv.h>
#include <string.h>


/*
 *----------------------------------------------------------------------
 *
 * chmod --
 *
 *	Procedure to map from Unix chmod system call to Sprite 
 *	Fs_SetAttr call.
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
chmod(path, mode)
    char *path;
    int mode;
{
    ReturnStatus status;
    Fs_Attributes attributes;	/* struct containing all file attributes,
				 * only mode is looked at. */
    kern_return_t kernStatus;
    mach_msg_type_number_t pathNameLength = strlen(path) + 1;
    Boolean sigPending;

    attributes.permissions = mode;
    kernStatus = Fs_SetAttrStub(SpriteEmu_ServerPort(), path,
				pathNameLength, FS_ATTRIB_FILE, attributes,
				FS_SET_MODE, &status, &sigPending);
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


/*
 *----------------------------------------------------------------------
 *
 * fchmod --
 *
 *	Procedure to map from Unix fchmod system call to Sprite 
 *	Fs_SetAttributesID call.
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
fchmod(fd, mode)
    int fd;
    int mode;
{
    ReturnStatus status;
    Fs_Attributes attributes;      /* struct containing all file attributes,
				    * only mode is looked at. */
    kern_return_t kernStatus;
    Boolean sigPending;

    attributes.permissions = mode;
    kernStatus = Fs_SetAttrIDStub(SpriteEmu_ServerPort(), fd, attributes,
				  FS_SET_MODE, &status, &sigPending);
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

