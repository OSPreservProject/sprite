/* 
 * devFsOpTable.c --
 *
 *	The operation tables for the file system devices.  
 *
 * Copyright 1987, 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifdef notdef
static char rcsid[] = "$Header: /sprite/src/boot/sunprom/sun4.md/RCS/devFsOpTable.c,v 1.1 90/09/17 11:24:28 rab Exp Locker: rab $ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"
#include "dev.h"
#include "devFsOpTable.h"

extern ReturnStatus SunPromDevOpen _ARGS_ ((Fs_Device *devicePtr, int flags,
	                        Fs_NotifyToken notifyToken, int *flagsPtr));
extern ReturnStatus SunPromDevRead _ARGS_ ((Fs_Device *devicePtr,
                                Fs_IOParam *readPtr, Fs_IOReply *replyPtr));
#if 0
static ReturnStatus NullWriteProc _ARGS_ ((Fs_Device *devicePtr,
                                Fs_IOParam *writePtr, Fs_IOReply *replyPtr));
static ReturnStatus NullIoctlProc _ARGS_ ((Fs_Device *devicePtr,
                                Fs_IOCParam *ioctlPtr, Fs_IOReply *replyPtr));
static ReturnStatus NullCloseProc _ARGS_ ((Fs_Device *devicePtr, int flags,
	                        int numUsers, int numWriters));
static ReturnStatus NullSelectProc _ARGS_ ((Fs_Device *devicePtr, int *readPtr,
	                            int *writePtr, int *exceptPtr));
static DevBlockDeviceHandle NullBlockDevAttachProc _ARGS_ ((Fs_Device *ptr)));
static ReturnStatus NullReopenProc _ARGS_ ((Fs_Device *devicePtr, int numUsers,
	                            int numWriters,
				    Fs_NotifyToken notifyToken,
				    int *flagsPtr));
static ReturnStatus NullMmapProc _ARGS_ ((Fs_Device *devicePtr,
                                Address startAddr, int length,
				int offset, Address *newAddrPtr));
#endif


/*
 * Device type specific routine table:
 *	This is for the file-like operations as they apply to devices.
 *	DeviceOpen
 *	DeviceRead
 *	DeviceWrite
 *	DeviceIOControl
 *	DeviceClose
 *	DeviceSelect
 *	BlockDeviceAttach
 */


DevFsTypeOps devFsOpTable[] = {
    /*
     * Simple interface to the routines in the Sun PROM.
     */
    {0, SunPromDevOpen, SunPromDevRead, 0, 0, 0, 0, 0, 0, 0 }
};

int devNumDevices = sizeof(devFsOpTable) / sizeof(DevFsTypeOps);

#if 0
static ReturnStatus
NullProc()
{
    return(SUCCESS);
}
#endif

