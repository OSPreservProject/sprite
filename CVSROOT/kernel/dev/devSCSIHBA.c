/* 
 * devSCSIHBA.c --
 *
 *      The standard Open, Read, Write, IOControl, and Close operations
 *      for  SCSI Host Bus Adaptor (HBA) device drivers. This driver permits
 *	user level programs to send arbitrary SCSI commands to SCSI
 *	devices.
 *
 * Copyright 1986 Regents of the University of California
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"
#include "fs.h"
#include "dev.h"
#include "devInt.h"
#include "scsi.h"
#include "scsiDevice.h"
#include "dev/scsi.h"
#include "scsiHBADevice.h"
#include "stdlib.h"

#include "dbg.h"



/*
 *----------------------------------------------------------------------
 *
 * DevSCSIDeviceOpen --
 *
 *	Open a SCSI device as a file.  This routine attaches the 
 *	device.
 *
 * Results:
 *	SUCCESS if the device is attached.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
/* ARGSUSED */
ReturnStatus
DevSCSIDeviceOpen(devicePtr, useFlags, token, flagsPtr)
    Fs_Device *devicePtr;	/* Device info, unit number etc. */
    int useFlags;		/* Flags from the stream being opened */
    Fs_NotifyToken token;	/* Call-back token for input, unused here */
    int	 *flagsPtr;		/* OUT: Device open flags. */
{
    ScsiDevice *devPtr;

    /*
     * Ask the HBA to set up the path to the device with FIFO ordering
     * of requests.
     */
    devPtr = DevScsiAttachDevice(devicePtr, DEV_QUEUE_FIFO_INSERT);
    if (devPtr == (ScsiDevice *) NIL) {
	return DEV_NO_DEVICE;
    }
    devicePtr->data = (ClientData) devPtr;
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * DevSCSIDeviceIOControl --
 *
 *	Do a special operation on a raw SCSI Device.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
DevSCSIDeviceIOControl(devicePtr, ioctlPtr, replyPtr)
    Fs_Device *devicePtr;
    Fs_IOCParam *ioctlPtr;	/* Standard I/O Control parameter block */
    Fs_IOReply *replyPtr;	/* Size of outBuffer and returned signal */
{
    ScsiDevice *devPtr;
    devPtr = (ScsiDevice *)(devicePtr->data);
    return DevScsiIOControl(devPtr, ioctlPtr, replyPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Dev_SCSITapeClose --
 *
 *	Close a raw SCSI device. 
 *
 * Results
 *	SUCCESS
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
DevSCSIDeviceClose(devicePtr, useFlags, openCount, writerCount)
    Fs_Device	*devicePtr;
    int		useFlags;	/* FS_READ | FS_WRITE */
    int		openCount;	/* Number of times device open. */
    int		writerCount;	/* Number of times device open for writing. */
{
    ScsiDevice *devPtr;
    ReturnStatus status = SUCCESS;

    devPtr = (ScsiDevice *)(devicePtr->data);
    (void) DevScsiReleaseDevice(devPtr);
    devicePtr->data = (ClientData) NIL;
    return(status);
}
