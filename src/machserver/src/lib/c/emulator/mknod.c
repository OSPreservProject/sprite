/* 
 * mknod.c --
 *
 *	Procedure to map from Unix mknod system call to Sprite.
 *
 * Copyright (C) 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/lib/c/emulator/RCS/mknod.c,v 1.4 92/03/13 20:40:53 kupfer Exp $ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include <mach.h>
#include <mach/message.h>
#include <fs.h>
#include "compatInt.h"
#include <errno.h>
#include <spriteEmuInt.h>
#include <string.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>


/*
 *----------------------------------------------------------------------
 *
 * mknod --
 *
 *	Procedure to map from Unix mkdir system call to Sprite Fs_MakeDevice.
 *	Unfortunately, this doesn't map from Unix land device types to
 *	Sprite device types.  This means a tar of /dev on a UNIX system
 *	will not be recreated correctly on a Sprite system, unless the
 *	tar program itself is fixed.
 *
 * Results:
 *	UNIX_ERROR is returned upon error, with the actual error code
 *	stored in errno.  Otherwise UNIX_SUCCESS is returned.
 *
 * Side effects:
 *	Creates a special file used to refer to a device.
 *
 *----------------------------------------------------------------------
 */

int
mknod(pathName, mode, dev)
    char *pathName;		/* The name of the directory to create */
    int mode;			/* Permission mask plus type */
    int dev;			/* Specifies minor and major dev numbers */
{
    ReturnStatus status;
    kern_return_t kernStatus;
    mach_msg_type_number_t pathNameLength = strlen(pathName) + 1;
    int streamID;
    Boolean sigPending = FALSE;

    switch (mode & S_IFMT) {
	case S_IFREG:
	    status = Fs_Open(pathName, FS_CREATE, mode & 0777, &streamID);
	    if (status == SUCCESS) {
		(void)close(streamID);
	    }
	    break;
	case S_IFBLK:
	case S_IFCHR: {
	    Fs_Device device;

	    device.serverID = FS_LOCALHOST_ID;
	    device.type = major(dev);
	    device.unit = minor(dev);

	    kernStatus = Fs_MakeDeviceStub(SpriteEmu_ServerPort(),
					   pathName, pathNameLength,
					   device, mode & 0777, &status,
					   &sigPending);
	    if (kernStatus != KERN_SUCCESS) {
		status = Utils_MapMachStatus(kernStatus);
	    }
	    break;
	}
	default:
	    status = GEN_INVALID_ARG;
	    break;
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
