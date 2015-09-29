/* 
 * devRawBlockDev.c --
 *
 *	Routines for providing the Raw open/close/read/write/ioctl on 
 *	Block Devices. The routines in this file are called though the
 * 	file system device switch table.
 *
 * Copyright 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifdef notdef
static char rcsid[] = "$Header: /sprite/src/kernel/dev/RCS/devRawBlockDev.c,v 1.1 89/05/01 15:51:17 mendel Exp Locker: brent $ SPRITE (Berkeley)";
#endif /* not lint */

#include "sprite.h"
#include "status.h"
#include "rawBlockDev.h"
#include "devBlockDevice.h"
#include "dev.h"
#include "devInt.h"
#include "user/fs.h"
#include "fs.h"


/*
 *----------------------------------------------------------------------
 *
 * DevRawBlockDevOpen --
 *
 *	Open a Block Device as a file for reads and writes.  Modify the
 *	Fs_Device structure to point at the attached device's handle.
 *
 * Results:
 *	SUCCESS if the device is successfully attached.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
/* ARGSUSED */
ReturnStatus
DevRawBlockDevOpen(devicePtr, useFlags, token)
    Fs_Device *devicePtr;	/* Device info, unit number etc. */
    int useFlags;		/* Flags from the stream being opened */
    ClientData token;		/* Call-back token for input, unused here */
{


    /*
     * See if the device was already open by someone else. NOTE: The 
     * file system passes us the same Fs_Device for each open and initalizes
     * the clientData field to NIL on the first call. 
     */
    /*
     * If the device is not already attach, attach it. This will fail if the 
     * device doesn't exists or is not functioning correctly.
     */
    devicePtr->data = (ClientData) Dev_BlockDeviceAttach(devicePtr);
    if (devicePtr->data == (ClientData) NIL) {
	return(DEV_NO_DEVICE);
    }
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * DevRawBlockDevRead --
 *
 *	Read from a raw Block Device. 
 *
 * Results:
 *	The Sprite return status of the read.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
DevRawBlockDevRead(devicePtr, offset, bufSize, buffer, lenPtr)
    Fs_Device *devicePtr;	/* Handle for raw Block device */
    int offset;			/* Indicates starting point for read.  */
    int bufSize;		/* Number of bytes to read. */
    char *buffer;		/* Buffer for the read */
    int *lenPtr;		/* How many bytes actually read */
{
    ReturnStatus error;
    DevBlockDeviceRequest	request;
    DevBlockDeviceHandle	*handlePtr;
    int				transferLen;

    /*
     * Extract the BlockDeviceHandle from the Fs_Device and sent a 
     * BlockDeviceRequest READ request using the synchronous
     * Block IO interface. If the request is too large break it into
     * smaller pieces. Insure the request to a multiple of the min
     * blocksize.
     */
    handlePtr = (DevBlockDeviceHandle *) (devicePtr->data);
    request.operation = FS_READ;
    request.startAddress = offset;
    request.startAddrHigh = 0;
    request.bufferLen = bufSize;
    request.buffer = buffer;
    error = Dev_BlockDeviceIOSync(handlePtr, &request, lenPtr);
    *lenPtr += transferLen;
    return(error);
}

/*
 *----------------------------------------------------------------------
 *
 * DevRawBlockDevWrite --
 *
 *	Write to a raw Block Device.
 *
 * Results:
 *	The return status of the IO.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
DevRawBlockDevWrite(devicePtr, offset, bufSize, buffer, lenPtr)
    Fs_Device *devicePtr;	/* Handle of raw disk device */
    int offset;			/* Indicates the starting point of the write.
				 */
    int bufSize;		/* Number of bytes to write.  */
    char *buffer;		/* Write buffer */
    int *lenPtr;		/* How much was actually written */
{
}

/*
 *----------------------------------------------------------------------
 *
 * DevRawBlockDevIOControl --
 *
 *	Do a special operation on a raw Block Device.
 *
 * Results:
 *	Return status of the IOControl.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
ReturnStatus
DevRawBlockDevIOControl(devicePtr, command, inBufSize, inBuffer,
				 outBufSize, outBuffer)
    Fs_Device *devicePtr;	/* Device pointer to operate of. */
    int command;		/* IO Control to be performed. */
    int inBufSize;		/* Size of inBuffer in bytes. */
    char *inBuffer;		/* Input data to command. */
    int outBufSize;		/* Size of inBuffer in bytes. */
    char *outBuffer;		/* Output data from command. */
{
}

/*
 *----------------------------------------------------------------------
 *
 * DevRawBlockDevClose --
 *
 *	Close a raw Block Device file. If this is last open of the device
 *	to be close then detach the device.
 *
 * Results:
 *	The return status of the close operation.
 *
 * Side effects:
 *	Block device may be detached.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
DevRawBlockDevClose(devicePtr, useFlags, openCount, writerCount)
    Fs_Device	*devicePtr;	/* Device to close. */
    int 	useFlags;	/* File system useFlags. */
    int		openCount;	/* Count of reference to device. */
    int		writerCount;	/* Count of open writers. */
{
}

