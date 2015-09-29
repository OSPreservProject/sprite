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

#ifdef notdef
static char rcsid[] = "$Header: /sprite/src/kernel/dev/RCS/devBlockDevice.c,v 1.1 89/05/01 15:32:32 mendel Exp Locker: brent $ SPRITE (Berkeley)";
#endif /* not lint */


#include "sprite.h"
#include "user/fs.h"
#include "devBlockDevice.h"
#include "devFsOpTable.h"
#include "sync.h"
#include "boot.h"
#include "machMon.h"

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

    /*
     * If type is out of range or the device type as no block IO 
     * capabilities then give up. Otherwise we let the Device Attach
     * procedure file the device.
     */
    return ((devFsOpTable[0].blockDevAttach)(devicePtr));
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

}

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
	Mach_MonPrintf("Read from 0x%x offset %d size %d to 0x%x\n", blockDevHandlePtr, requestPtr->startAddress,
			requestPtr->bufferLen, requestPtr->buffer);
    requestPtr->clientData = (ClientData) &syncCmdData;
    requestPtr->doneProc = syncDoneProc;
    Sync_SemInitDynamic((&syncCmdData.mutex),"BlockSyncCmdMutex");
    syncCmdData.done = FALSE;
    syncCmdData.amountTransferred = 0;
    status = Dev_BlockDeviceIO(blockDevHandlePtr, requestPtr);
    if (status == SUCCESS) {
	MASTER_LOCK((&syncCmdData.mutex));
	while (syncCmdData.done == FALSE) { 
	    boot_Poll();
	}
	MASTER_UNLOCK((&syncCmdData.mutex));
	status = syncCmdData.status;
    } 
    if (amountTransferredPtr != (int *) NIL) { 
	*amountTransferredPtr = syncCmdData.amountTransferred;
    }
	Mach_MonPrintf("Done %x\n", status);
    return (status);
}
