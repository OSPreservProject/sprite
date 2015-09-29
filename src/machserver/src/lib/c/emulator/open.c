/* 
 * open.c --
 *
 *	Procedure to map from Unix open system call to Sprite.
 *
 * Copyright (C) 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/lib/c/emulator/RCS/open.c,v 1.4 92/03/13 20:41:21 kupfer Exp $ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include <mach.h>
#include <mach/message.h>
#include <stdio.h>
#include <fs.h>
#include "compatInt.h"
#include <sys/file.h>
#include <errno.h>
#include <status.h>
#include <spriteEmuInt.h>
#include <spriteSrv.h>
#include <string.h>


/*
 *----------------------------------------------------------------------
 *
 * open --
 *
 *	Procedure to map from Unix open system call to Sprite Fs_Open.
 *	Mostly this has to map the usageFlags argument
 *
 * Results:
 *	UNIX_ERROR is returned upon error, with the actual error code
 *	stored in errno.  A file descriptor is returned upon success.
 *
 * Side effects:
 *	Opening a file sets up state in the filesystem until the file is
 *	closed.  
 *
 *----------------------------------------------------------------------
 */

	/* VARARGS2 */
int
open(pathName, unixFlags, permissions)
    char *pathName;		/* The name of the file to open */
    register int unixFlags;	/* O_RDONLY O_WRONLY O_RDWR O_NDELAY
				 * O_APPEND O_CREAT O_TRUNC O_EXCL */
    int permissions;		/* Permission mask to use on creation */
{
    int streamId;		/* place to hold stream id allocated by
				 * Fs_Open */
    ReturnStatus status;	/* result returned by Fs_Open */
    kern_return_t kernStatus;
    register int useFlags = 0;	/* Sprite version of flags */
    mach_msg_type_number_t pathNameLength = strlen(pathName) + 1;
    Boolean sigPending;

    /*
     * Convert unixFlags to FS_READ, etc.
     */
     
    if (unixFlags & FASYNC) {
	fprintf(stderr, "open - FASYNC not supported\n");
	errno = EINVAL;
	return(UNIX_ERROR);
    }
    if (unixFlags & O_RDWR) {
	useFlags |= FS_READ|FS_WRITE;
    } else if (unixFlags & O_WRONLY) {
	useFlags |= FS_WRITE;
    } else {
	useFlags |= FS_READ;
    }
    if (unixFlags & FNDELAY) {
	useFlags |= FS_NON_BLOCKING;
    }
    if (unixFlags & FAPPEND) {
	useFlags |= FS_APPEND;
    }
    if (unixFlags & FTRUNC) {
	useFlags |= FS_TRUNC;
    }
    if (unixFlags & FEXCL) {
	useFlags |= FS_EXCLUSIVE;
    }
    if (unixFlags & O_MASTER) {
	useFlags |= FS_PDEV_MASTER;
    }
    if (unixFlags & O_PFS_MASTER) {
	useFlags |= FS_PFS_MASTER;
    }
    if (unixFlags & FCREAT) {
	useFlags |= FS_CREATE;
    }

    kernStatus = Fs_OpenStub(SpriteEmu_ServerPort(), pathName,
			     pathNameLength, useFlags, permissions,
			     &status, &streamId, &sigPending);
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
	return(streamId);
    }
}
