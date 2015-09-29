/* 
 * devScsiDevice.c --
 *
 *	Routines for attaching/releasing/sending commands to SCSI device 
 *	attached to SCSI HBAs.
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
static char rcsid[] = "$Header: /sprite/src/kernel/dev/RCS/devScsiDevice.c,v 1.2 89/05/23 10:23:26 mendel Exp Locker: brent $ SPRITE (Berkeley)";
#endif /* not lint */

#include "sprite.h"
#include "scsiDevice.h"
#include "scsiHBA.h"
#include "dev/scsi.h"
#include "scsi.h"
#include "devQueue.h"
#include "user/fs.h"
#include "sync.h"	
#include "stdlib.h"



/*
 *----------------------------------------------------------------------
 *
 * DevScsiAttachDevice --
 *
 *	Return a handle that allows access to the specified SCSI device.
 *
 * Results:
 *	A pointer to the ScsiDevice structure for the device. 
 *	NIL if the device could not be attached.
 *
 * Side effects:
 *	If the device is attached, an INQUIRY command is sent to the 
 *	device.
 *
 *----------------------------------------------------------------------
 */

ScsiDevice   *
DevScsiAttachDevice(devicePtr, insertProc)
    Fs_Device	*devicePtr;	/* Device to attach. */
    void 	(*insertProc)(); /* Insert procedure to use. */
{
    ScsiDevice   *handle;
    ScsiCmd	inquiryCmdBlock;

    /*
     * Call the Attach procedure for the HBA type specified in the Fs_Device.
     */

	handle = (devScsiAttachProcs[0])(devicePtr,insertProc);
    return handle;
}

/*
 * The following structure and routine are used to implement DevScsiSendCmdSync.
 * The arguments to DevScsiSendCmdSync are stored in a SyncCmdBuf on 
 * the caller's stack and DevScsiSendCmd is called. The call back function
 * scsiDoneProc fills in the OUT arguments are wakes the caller.
 *
 */
typedef struct SyncCmdBuf {
    Sync_Semaphore mutex;	  /* Lock for synronizing updates of 
				   * this structure with the call back 
				   * function. */
    Sync_Condition wait;	  /* Condition valued used to wait for
				   * callback. */
    Boolean	  done;		  /* Is the operation finished or not? */
    unsigned char *statusBytePtr; /* Area to store SCSI status byte. */
    int	      *senseBufferLenPtr; /* Sense buffer length pointer. */
    Address    senseBufferPtr;	  /* Sense buffer. */
    ReturnStatus status;   	  /* HBA error for command. */
    int	       *countPtr;	  /* Btes transferred pointer. */
} SyncCmdBuf;

/*
 *----------------------------------------------------------------------
 *
 * scsiDoneProc --
 *
 *	This procedure is called when a scsi command started by 
 *	DevScsiSendCmdSync finished. It's calling sequence is 
 *	defined by the call back done by the DevScsiSendCmd routine.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	A scsi command is executed.
 *
 *----------------------------------------------------------------------
 */

static int
scsiDoneProc(scsiCmdPtr, errorCode, statusByte, byteCount, 
	     senseDataLen, senseDataPtr)
    ScsiCmd	*scsiCmdPtr;
    ReturnStatus errorCode;
    unsigned char statusByte;
    int		byteCount;
    int		senseDataLen;
    Address	senseDataPtr;
{
    SyncCmdBuf	*syncCmdDataPtr = (SyncCmdBuf *) (scsiCmdPtr->clientData);
    /*
     * A pointer to a SyncCmdBuf is passed as the clientData to this call.
     * Lock the structure, fill in the return values and wake up the
     * initiator.
     */
    MASTER_LOCK(&syncCmdDataPtr->mutex);

    *(syncCmdDataPtr->statusBytePtr) = statusByte;
    syncCmdDataPtr->status = errorCode;
    *(syncCmdDataPtr->countPtr) = byteCount;
    if (syncCmdDataPtr->senseBufferLenPtr != (int *) NIL) {
	int	len;
	len = *(syncCmdDataPtr->senseBufferLenPtr);
	if (senseDataLen < len) {
	    len = senseDataLen;
	}
	bcopy(senseDataPtr, syncCmdDataPtr->senseBufferPtr, len);			*(syncCmdDataPtr->senseBufferLenPtr) = len;
    }
    syncCmdDataPtr->done = TRUE;
    Sync_MasterBroadcast(&syncCmdDataPtr->wait);
    MASTER_UNLOCK(&syncCmdDataPtr->mutex);
    return (0);

}

/*
 *----------------------------------------------------------------------
 *
 * DevScsiSendCmdSync --
 *
 *	Send a SCSI command block to the device specified in the 
 *	ScsiDevice. This is the synchronous version that waits
 *	for the status byte and sense data before returning.
 *
 * Results:
 *	A ReturnStatus
 *
 * Side effects: 
 *	A SCSI command block is sent to the device.  
 *
 *----------------------------------------------------------------------
 *
 */
ReturnStatus
DevScsiSendCmdSync(scsiDevicePtr, scsiCmdPtr, statusBytePtr, 
		    amountTransferredPtr,  senseBufferLenPtr, senseBufferPtr)
    ScsiDevice *scsiDevicePtr;  /* Handle for target device. */
    ScsiCmd	*scsiCmdPtr;		    /* SCSI command to be sent. */
    unsigned char *statusBytePtr; /* Area to store SCSI status byte. */
    Address 	senseBufferPtr;	  /* Buffer to put sense data upon error. */
    int		*senseBufferLenPtr; /*  IN - Length of senseBuffer available.
				     * OUT - Length of senseData returned. */
    int		*amountTransferredPtr; /* OUT - Nuber of bytes transferred. */
{
    ReturnStatus status;
    SyncCmdBuf	 syncCmdData;

    scsiCmdPtr->clientData = (ClientData) &syncCmdData;
    scsiCmdPtr->doneProc = scsiDoneProc;
    Sync_SemInitDynamic((&syncCmdData.mutex),"ScsiSyncCmdMutex");
    syncCmdData.done = FALSE;
    syncCmdData.statusBytePtr = statusBytePtr;
    syncCmdData.senseBufferPtr = senseBufferPtr;
    syncCmdData.senseBufferLenPtr = senseBufferLenPtr;
    syncCmdData.countPtr = amountTransferredPtr;
    boot_SendSCSICommand(scsiDevicePtr, scsiCmdPtr);
    MASTER_LOCK((&syncCmdData.mutex));
    while (syncCmdData.done == FALSE) { 
	Sync_MasterWait((&syncCmdData.wait),(&syncCmdData.mutex),FALSE);
    }
    MASTER_UNLOCK((&syncCmdData.mutex));
    status = syncCmdData.status;
    return status;
}



/*
 *----------------------------------------------------------------------
 *
 * DevScsiSendCmd --
 *
 *	Send a SCSI command block to the device specified in the 
 *	ScsiDevice.
 *
 * Results:
 *	Nothing
 *
 * Side effects: 
 *	A SCSI command block enqueue for the device.  The doneProc procedure
 *	is call upon SCSI command completion.
 *
 *----------------------------------------------------------------------
 *
 * Due to the simplity of this routine and as an attempt to reduce procedure
 * calling depth, this routine is coded as a macro and can be found in
 * scsiHBAInt.h. 
 */
#ifndef DevScsiSendCmd
void 
DevScsiSendCmd(scsiDevicePtr, scsiCmdPtr)
    ScsiDevice 	  *scsiDevicePtr; 	 /* Handle for target device. */
    ScsiCmd	     *scsiCmdPtr;        /* Command to be executed. */
{
     Dev_QueueInsert(scsiDevicePtr->devQueue, (List_Links *) scsiCmdPtr);
}
#endif /* DevScsiSendCmd */


/*
 *----------------------------------------------------------------------
 *
 * DevScsiReleaseDevice --
 *
 * 	Release a device previously attached with ScsiAttachDevice().
 *
 * Results:
 *	A ReturnStatus.
 *
 * Side effects: 
 *	Unknown.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus 
DevScsiReleaseDevice(scsiDevicePtr)
    ScsiDevice *scsiDevicePtr;  /* Handle for device to be released. */
{
}


/*
 *----------------------------------------------------------------------
 *
 *  DevScsiGetSenseCmd --
 *
 * 	Procedure for formatting REQUEST SENSE.
 *
 * Results:
 *	void
 *
 * Side effects: 
 *	Unknown.
 *
 *----------------------------------------------------------------------
 */
void
DevScsiSenseCmd(scsiDevicePtr, bufferSize, buffer, scsiCmdPtr)
    ScsiDevice *scsiDevicePtr;  /* Handle for device to be released. */
    int		bufferSize;	/* Size of request sense data buffer. */
    char	*buffer;	/* Data buffer to put sense data. */
    ScsiCmd	*scsiCmdPtr;	/* Scsi command buffer to fill in. */
{
    DevScsiGroup0Cmd(scsiDevicePtr, SCSI_REQUEST_SENSE, 0, 
			(unsigned) bufferSize, scsiCmdPtr);
    scsiCmdPtr->dataToDevice = FALSE;
    scsiCmdPtr->bufferLen = bufferSize;
    scsiCmdPtr->buffer = buffer;
}
