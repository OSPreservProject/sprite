/* 
 * devBlockDevice.c --
 *
 *	Routines supporting the interface to Sprite block devices.
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
#include "fs.h"
#include "devBlockDevice.h"
#include "devFsOpTable.h"
#include "sync.h"


/*
 *----------------------------------------------------------------------
 *
 * Dev_BlockDeviceAttach --
 *
 *	Build data structures necessary for accessing a block device.
 *	This routine interprets the type field of the Fs_Device structure
 *	and calls the appropriate attach routine. 
 *
 * Results:
 *	A pointer to a DevBlockDeviceHandle structure for the device. 
 *	NIL if the device can not be attached.
 *
 * Side effects:
 *	Device dependent.
 *
 *----------------------------------------------------------------------
 */
DevBlockDeviceHandle *
Dev_BlockDeviceAttach(devicePtr)
    Fs_Device	*devicePtr;	/* The device to attach. */
{

    int	type = DEV_TYPE_INDEX(devicePtr->type);
    /*
     * If type is out of range or the device type as no block IO 
     * capabilities then give up. Otherwise we let the Device Attach
     * procedure file the device.
     */
    if ((type >= devNumDevices) ||
	(devFsOpTable[type].blockDevAttach == DEV_NO_ATTACH_PROC)) {
	return ((DevBlockDeviceHandle *) NIL);
    }
    return ((devFsOpTable[type].blockDevAttach)(devicePtr));
}


/*
 *----------------------------------------------------------------------
 *
 * Dev_BlockDeviceRelease --
 *
 *	Release an attached block device. Resources held by the device 
 *	should be freed.
 *
 * Results:
 *	A Sprite return Status specifying it the device could be releases.
 *
 * Side effects:
 *	Device dependent.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Dev_BlockDeviceRelease(blockDevHandlePtr)
     DevBlockDeviceHandle
		*blockDevHandlePtr;  /* Handle of the device to releaes */
{
    return (blockDevHandlePtr->releaseProc)(blockDevHandlePtr);

}


/*
 *----------------------------------------------------------------------
 *
 * Dev_BlockDeviceIO --
 *
 *	Enqueue a block IO request for the specified device. Upon operation
 *	completion doneProc will be called. 
 *
 * 	NOTE: The DevBlockDeviceRequest structure pointed to by the requestPtr
 *	      argument is assumed to remain valid and unchanged until the
 *	      doneProc specified in DevBlockDeviceRequest is called.
 *
 * Results:
 *	SUCCESS if operation is sucessfully enqueued. 
 *	A Sprite error code otherwise.
 *
 * Side effects:
 *	The specified doneProc is called with the following arguments:
 *
 *	(*doneProc)(requestPtr, returnStatus, amountTransferred)
 *		DevBlockDeviceRequest	*requestPtr;   
 *					       / * The requestPtr passed to
 *						 * Dev_BlockDeviceIO. * /
 *		ReturnStatus	returnStatus;  / * The error status of the
 *						 * command. SUCCESS if no
 *						 * error occured. * /
 *		int	amountTransferred;     / * The number of blocks
 *						 * transfered by the 
 *						 * operation. * /
 *----------------------------------------------------------------------
 * Because of its simplicity and in an attempt to reduce procedure calling
 * depth, Dev_BlockDeviceIO may be coded as an macro in devBlockDevice.h.
 */
#ifndef Dev_BlockDeviceIO
ReturnStatus 
Dev_BlockDeviceIO(blockDevHandlePtr, requestPtr)
    DevBlockDeviceHandle
		*blockDevHandlePtr;  /* Handle of the device to operate on. */
    DevBlockDeviceRequest
		*requestPtr;	     /* Request to be performed. */
{
    return (blockDevHandlePtr->blockIOProc)(blockDevHandlePtr, requestPtr);
}
#endif /* Dev_BlockDeviceIO */

/*
 *----------------------------------------------------------------------
 *
 * Dev_BlockDeviceIOControl --
 *
 *	Execute an IO control operation on the specified device.
 *
 * Results:
 *	SUCCESS if operation is successful. 
 *	An error status otherwise.
 *
 * Side effects:
 *	Device dependent.
 *
 *----------------------------------------------------------------------
 * Because of its simplicity and in an attempt to reduce procedure calling
 * depth, Dev_BlockDeviceIOControl may be coded as an macro in devBlockDevice.h.
 */
#ifndef	Dev_BlockDeviceIOControl
ReturnStatus 
Dev_BlockDeviceIOControl(blockDevHandlePtr, ioctlPtr, replyPtr) 
    DevBlockDeviceHandle
		*blockDevHandlePtr;  /* Handle of the device to operate on. */
    Fs_IOCParam *ioctlPtr;	/* Standard I/O Control parameter block */
    Fs_IOReply *replyPtr;	/* outBuffer length and returned signal */
{

    return (blockDevHandlePtr->IOControlProc)(blockDevHandlePtr, ioctlPtr, replyPtr);


}
#endif /* Dev_BlockDeviceIOControl */
/*
 * The following structure and routines are used to implement the routine
 * Dev_BlockDeviceIOSync.
 * The arguments to Dev_BlockDeviceIOSync are stored in a SyncCmdBuf on 
 * the caller's stack and  Dev_BlockDeviceIO is called. The call back function
 * syncDoneProc fills in the OUT arguments are wakes the caller.
 *
 */
typedef struct SyncCmdBuf {
    Sync_Semaphore mutex;	  /* Lock for synronizing updates of 
				   * this structure with the call back 
				   * function. */
    Sync_Condition wait;	  /* Condition valued used to wait for
				   * callback. */
    Boolean	  done;		  /* Is the operation finished or not? */
    int	  amountTransferred; 	  /* Number of bytes transferred according
				   * to the call back. */
    ReturnStatus status; 	  /* Operation return status according to 
				   * call back. */
} SyncCmdBuf;

/*
 *----------------------------------------------------------------------
 *
 * syncDoneProc --
 *
 *	This procedure is called when a sync block command started by 
 *	Dev_BlockDeviceIO finished. It's calling sequence is 
 *	defined by the call back caused by the Dev_BlockDeviceIO routine.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	SyncCmdBuf is filled in and a wakeup is broadcast.
 *
 *----------------------------------------------------------------------
 */

static void
syncDoneProc(requestPtr, status, amountTransferred)
    DevBlockDeviceRequest	*requestPtr;
    ReturnStatus status;
    int	amountTransferred;
{
    SyncCmdBuf	*syncCmdDataPtr = (SyncCmdBuf *) (requestPtr->clientData);
    /*
     * A pointer to a SyncCmdBuf is passed as the clientData to this call.
     * Lock the structure, fill in the return values and wake up the
     * initiator.
     */
    MASTER_LOCK(&syncCmdDataPtr->mutex);
    syncCmdDataPtr->status = status;
    syncCmdDataPtr->amountTransferred = amountTransferred;
    syncCmdDataPtr->done = TRUE;
    Sync_MasterBroadcast(&syncCmdDataPtr->wait);
    MASTER_UNLOCK(&syncCmdDataPtr->mutex);
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * Dev_BlockDeviceIOSync --
 *
 *	Perform a block IO request for the specified device and wait 
 *	for completion.
 *
 * Results:
 *	SUCCESS if operation is successful completed. 
 *	A Sprite error code otherwise.
 *
 * Side effects:
 *	Device specific block IO done.
 *----------------------------------------------------------------------
 */
ReturnStatus 
Dev_BlockDeviceIOSync(blockDevHandlePtr, requestPtr,amountTransferredPtr)
    DevBlockDeviceHandle
		*blockDevHandlePtr;  /* Handle of the device to operate on. */
    DevBlockDeviceRequest 
		*requestPtr;	     /* Request to block IO device. */
    int	*amountTransferredPtr; 	     /* Area to store number of bytes
				      * transferred. May be NIL. */
{
    ReturnStatus status;
    SyncCmdBuf	 syncCmdData;

    requestPtr->clientData = (ClientData) &syncCmdData;
    requestPtr->doneProc = syncDoneProc;
    Sync_SemInitDynamic((&syncCmdData.mutex),"BlockSyncCmdMutex");
    syncCmdData.done = FALSE;
    syncCmdData.amountTransferred = 0;
    status = Dev_BlockDeviceIO(blockDevHandlePtr, requestPtr);
    if (status == SUCCESS) {
	MASTER_LOCK((&syncCmdData.mutex));
	while (syncCmdData.done == FALSE) { 
	    Sync_MasterWait((&syncCmdData.wait),(&syncCmdData.mutex),FALSE);
	}
	MASTER_UNLOCK((&syncCmdData.mutex));
	status = syncCmdData.status;
    } 
    if (amountTransferredPtr != (int *) NIL) { 
	*amountTransferredPtr = syncCmdData.amountTransferred;
    }
    return (status);
}
