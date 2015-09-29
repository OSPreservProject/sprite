/* 
 * mknod.c --
 *
 *	Procedure to map from Unix mknod system call to Sprite.
 *
 * Copyright (C) 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: mknod.c,v 1.3 88/06/29 15:41:05 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "fs.h"
#include "compatInt.h"
#include <errno.h>
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
    ReturnStatus status;	/* result returned by Fs_Open */
    int streamID;

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

	    status = Fs_MakeDevice(pathName, &device, mode & 0777);
	    break;
	}
	default:
	    errno = EINVAL;
	    return(UNIX_ERROR);
    }
     if (status != SUCCESS) {
	errno = Compat_MapCode(status);
	return(UNIX_ERROR);
    } else {
	return(UNIX_SUCCESS);
    }
}
