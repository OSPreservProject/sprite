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

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif /* not lint */

#include "sprite.h"
#include "status.h"
#include "rawBlockDev.h"
#include "devBlockDevice.h"
#include "dev.h"
#include "devInt.h"
#include "fs.h"
#include "user/fs.h"


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
DevRawBlockDevOpen(devicePtr, useFlags, token, flagsPtr)
    Fs_Device *devicePtr;	/* Device info, unit number etc. */
    int useFlags;		/* Flags from the stream being opened */
    Fs_NotifyToken token;	/* Call-back token for input, unused here */
    int		*flagsPtr;	/* OUT: Device IO flags. */
{
    DevBlockDeviceHandle *handlePtr;

    /*
     * See if the device was already open by someone else. NOTE: The 
     * file system passes us the same Fs_Device for each open and initalizes
     * the clientData field to NIL on the first call. 
     */
    handlePtr =  (DevBlockDeviceHandle *) devicePtr->data;
    if (handlePtr != (DevBlockDeviceHandle *) NIL) {
	/*
	 * Block devices handle their own locking.
	 */
	*flagsPtr |= FS_DEV_DONT_LOCK;
	/*
	 * Already attached. 
	 */
	return (SUCCESS);
    }
    /*
     * If the device is not already attach, attach it. This will fail if the 
     * device doesn't exists or is not functioning correctly.
     */
    handlePtr = Dev_BlockDeviceAttach(devicePtr);
    if (handlePtr == (DevBlockDeviceHandle *) NIL) {
	return(DEV_NO_DEVICE);
    }
    devicePtr->data = (ClientData) handlePtr;
    /*
     * Block devices handle their own locking.
     */
    *flagsPtr |= FS_DEV_DONT_LOCK;
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * DevRawBlockDevReopen --
 *
 *	Reopen a Block Device as a file for reads and writes.
 *	This calls the regular open procedure to do all the work.
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
DevRawBlockDevReopen(devicePtr, refs, writers, token, flagsPtr)
    Fs_Device *devicePtr;	/* Device info, unit number etc. */
    int refs;			/* Number of open streams */
    int writers;		/* Number of writing streams */
    Fs_NotifyToken token;	/* Call-back token for input, unused here */
    int	*flagsPtr;		/* OUT: Device IO flags. */
{
    int useFlags = FS_READ;

    if (writers) {
	useFlags |= FS_WRITE;
    }
    return( DevRawBlockDevOpen(devicePtr, useFlags, token, flagsPtr) );
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
DevRawBlockDevRead(devicePtr, readPtr, replyPtr)
    Fs_Device *devicePtr;	/* Handle for raw Block device */
    Fs_IOParam	*readPtr;	/* Read parameter block */
    Fs_IOReply	*replyPtr;	/* Return length and signal */ 
{
    ReturnStatus error;
    DevBlockDeviceRequest	request;
    DevBlockDeviceHandle	*handlePtr;
    int				amountRead;
    int				toRead;

    /*
     * Extract the BlockDeviceHandle from the Fs_Device and sent a 
     * BlockDeviceRequest READ request using the synchronous
     * Block IO interface. If the request is too large break it into
     * smaller pieces. Insure the request to a multiple of the min
     * blocksize.
     */
    handlePtr = (DevBlockDeviceHandle *) (devicePtr->data);
    if ((readPtr->offset % (handlePtr->minTransferUnit)) || 
	(readPtr->length % (handlePtr->minTransferUnit))) {
	replyPtr->length = 0;
	printf("DevRawBlockDevRead: Non-aligned read, %d bytes at %d\n",
		readPtr->length, readPtr->offset);
	return DEV_INVALID_ARG;
    }

    amountRead = 0;
    while (readPtr->length > 0) {
	/*
	 * Reinitialize everything each loop because lower-levels
	 * might trash operation or startAddrHigh for their own reasons.
	 */
	request.operation = FS_READ;
	request.startAddrHigh = 0;
	request.startAddress = readPtr->offset + amountRead;
	request.buffer = readPtr->buffer + amountRead;
	toRead = (readPtr->length > handlePtr->maxTransferSize) ?
		  handlePtr->maxTransferSize : readPtr->length;
	request.bufferLen = toRead;
	error = Dev_BlockDeviceIOSync(handlePtr, &request, &replyPtr->length);
	amountRead += replyPtr->length;
	readPtr->length -= replyPtr->length;
	if ((error != SUCCESS) || (replyPtr->length != toRead)) {
	    printf("DevRawBlockDevRead: error 0x%x inLength %d at offset 0x%x outLength %d\n",
		    error, request.bufferLen, request.startAddress,
		    replyPtr->length);
	    return error;
	}
    }
    replyPtr->length = amountRead;
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
DevRawBlockDevWrite(devicePtr, writePtr, replyPtr)
    Fs_Device *devicePtr;	/* Handle of raw disk device */
    Fs_IOParam	*writePtr;	/* Standard write parameter block */
    Fs_IOReply	*replyPtr;	/* Return length and signal */
{
    ReturnStatus 		error;	
    DevBlockDeviceRequest	request;
    DevBlockDeviceHandle	*handlePtr;
    int				amountWritten;
    int				toWrite;
    /*
     * Extract the BlockDeviceHandle from the Fs_Device and sent a 
     * BlockDeviceRequest WRITE request using the synchronous
     * Block IO interface.
     */

    handlePtr = (DevBlockDeviceHandle *) (devicePtr->data);
    if ((writePtr->offset % (handlePtr->minTransferUnit)) || 
	(writePtr->length % (handlePtr->minTransferUnit))) {
	replyPtr->length = 0;
	return DEV_INVALID_ARG;
    }
    amountWritten = 0;
    while (writePtr->length > 0) {
	/*
	 * Reinitialize everything each loop because lower-levels
	 * might trash operation or startAddrHigh for their own reasons.
	 */
	request.operation = FS_WRITE;
	request.startAddrHigh = 0;
	request.startAddress = writePtr->offset + amountWritten;
	request.buffer = writePtr->buffer + amountWritten;
	toWrite = (writePtr->length > handlePtr->maxTransferSize) ?
		   handlePtr->maxTransferSize : writePtr->length;
	request.bufferLen = toWrite;
	error = Dev_BlockDeviceIOSync(handlePtr, &request, &replyPtr->length);
	amountWritten += replyPtr->length;
	writePtr->length -= replyPtr->length;
	if ((error != SUCCESS) || (replyPtr->length != toWrite)) {
	    return error;
	}
    }
    replyPtr->length = amountWritten;
    return(error);
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
DevRawBlockDevIOControl(devicePtr, ioctlPtr, replyPtr)
    Fs_Device *devicePtr;	/* Device pointer to operate of. */
    Fs_IOCParam *ioctlPtr;	/* Standard I/O Control parameter block */
    Fs_IOReply *replyPtr;	/* reply length and signal */
{
    return Dev_BlockDeviceIOControl((DevBlockDeviceHandle *) (devicePtr->data),
		ioctlPtr, replyPtr);
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
    ReturnStatus 		error;	
    DevBlockDeviceHandle	*handlePtr;

    if (openCount == 0) { 
	handlePtr = (DevBlockDeviceHandle *) (devicePtr->data);
	error = Dev_BlockDeviceRelease(handlePtr);
	devicePtr->data = (ClientData) NIL;
    } else {
	error = SUCCESS;
    }
    return(error);
}

