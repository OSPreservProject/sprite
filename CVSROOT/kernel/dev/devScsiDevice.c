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

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif /* not lint */

#include "sprite.h"
#include "scsiDevice.h"
#include "scsiHBA.h"
#include "dev/scsi.h"
#include "scsi.h"
#include "devQueue.h"
#include "fs.h"
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
    int	 hbaType;	
    ScsiDevice   *handle;
    ScsiCmd	inquiryCmdBlock;

    /*
     * Call the Attach procedure for the HBA type specified in the Fs_Device.
     */
    hbaType = SCSI_HBA_TYPE(devicePtr);

    if (hbaType >= devScsiNumHBATypes) {
	handle = (ScsiDevice *) NIL;
    } else {
	handle = (devScsiAttachProcs[hbaType])(devicePtr,insertProc);
    }
    /*
     * If the inquiry data doesn't exists for this device yet send the
     * device a INQUIRY command.
     */
    if ((handle != (ScsiDevice *) NIL) && (handle->inquiryLength == 0)) {
	unsigned char statusByte;
	if (handle->inquiryDataPtr == (char *) 0) {
	    handle->inquiryDataPtr = (char *) malloc(DEV_MAX_INQUIRY_SIZE);
	}
	(void) DevScsiGroup0Cmd(handle,SCSI_INQUIRY,0,DEV_MAX_INQUIRY_SIZE,
				&inquiryCmdBlock);
	inquiryCmdBlock.buffer = handle->inquiryDataPtr;
	inquiryCmdBlock.bufferLen = DEV_MAX_INQUIRY_SIZE;
	inquiryCmdBlock.dataToDevice = FALSE;
	(void) DevScsiSendCmdSync(handle, &inquiryCmdBlock, &statusByte, 
		    &(handle->inquiryLength),  (int *) NIL, (char *) NIL);
    }
    if (handle != (ScsiDevice *) NIL) {
	handle->referenceCount++;
    }
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
    DevScsiSendCmd(scsiDevicePtr, scsiCmdPtr);
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
    scsiDevicePtr->referenceCount--;
    if (scsiDevicePtr->referenceCount == 0) { 
	free(scsiDevicePtr->inquiryDataPtr);
	scsiDevicePtr->inquiryDataPtr = (char *) 0;
	scsiDevicePtr->inquiryLength = 0;
    }
    return ((scsiDevicePtr->releaseProc)(scsiDevicePtr));
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

/*
 *----------------------------------------------------------------------
 *
 *  DevScsiTestReady --
 *
 * 	Test to see if a SCSI device is ready.
 *
 * Results:
 *	
 *
 * Side effects: 
 *	A TEST_UNIT_READY command is set to the device.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
DevScsiTestReady(scsiDevicePtr)
    ScsiDevice *scsiDevicePtr;  /* Handle for device to be released. */
{
    ScsiCmd	unitReadyCmd;	/* Scsi command buffer to fill in. */
    ReturnStatus status;
    unsigned char	statusByte;
    int		len;
    char	senseData[SCSI_MAX_SENSE_LEN];
    int		senseLen;
    char	errorString[MAX_SCSI_ERROR_STRING];
    Boolean	class7;

    DevScsiGroup0Cmd(scsiDevicePtr,SCSI_TEST_UNIT_READY, 0, 0, &unitReadyCmd);
    unitReadyCmd.bufferLen = 0;
    len = 0;
    senseLen = SCSI_MAX_SENSE_LEN;
    status = DevScsiSendCmdSync(scsiDevicePtr,&unitReadyCmd, &statusByte,
				&len, &senseLen, senseData);
    class7 = DevScsiMapClass7Sense(senseLen, senseData, &status, errorString);
    if (class7) {
	return status;
    }
    return status;
}


/*
 *----------------------------------------------------------------------
 *
 * DevScsiIOControl --
 *
 *	Process a generic SCSI device IOControl.
 *
 * Results:
 *	The return status of the IOControl.
 *
 * Side effects:
 *	Many.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
DevScsiIOControl(devPtr, ioctlPtr, replyPtr)
    ScsiDevice	*devPtr;	/* SCSI Handle for device. */
    Fs_IOCParam *ioctlPtr;	/* Standard I/O Control parameter block */
    Fs_IOReply *replyPtr;	/* Size of outBuffer and returned signal */
{
    ReturnStatus	status;
    int			senseDataLen;
    char		senseData[DEV_MAX_SENSE_BYTES];

    if (ioctlPtr->command == IOC_SCSI_COMMAND) {
	Dev_ScsiCommand	 *cmdPtr;
	Dev_ScsiStatus	 *statusPtr;
	ScsiCmd		 scsiCmd;
	unsigned char    statusByte;
	Boolean		dataToDevice;
	int		senseBufLen;
	/*
	 * The user wants to send a SCSI command. First validate
	 * the input buffer is large enough to have the parameter
	 * block and the output buffer can hold the return status.
	 */
	if ((ioctlPtr->inBufSize < sizeof(Dev_ScsiCommand)) ||
	    (ioctlPtr->outBufSize < sizeof(Dev_ScsiStatus))) {
	    return(GEN_INVALID_ARG);
	}
	cmdPtr = (Dev_ScsiCommand *) ioctlPtr->inBuffer;
	/*
	 * Validate the SCSI command block.
	 */
	if ((cmdPtr->commandLen < 0) || (cmdPtr->commandLen > 16) ||
	    (cmdPtr->commandLen > 
		(ioctlPtr->inBufSize-sizeof(Dev_ScsiCommand))))	{
	    return(GEN_INVALID_ARG);
	}
	/*
	 * Validate the input or output data buffer.
	 */
	dataToDevice = (cmdPtr->dataOffset < ioctlPtr->inBufSize);
	if ((cmdPtr->bufferLen < 0) ||
	    (cmdPtr->bufferLen > devPtr->maxTransferSize)) {
	    return FS_BUFFER_TOO_BIG;
	}
	if (dataToDevice && 
	    (cmdPtr->bufferLen > (ioctlPtr->inBufSize - cmdPtr->dataOffset))) {
		return (GEN_INVALID_ARG);
	}
	if (!dataToDevice && 
	   (cmdPtr->bufferLen > (ioctlPtr->outBufSize-sizeof(Dev_ScsiStatus)))){
		return (GEN_INVALID_ARG);
	}
	/*
	 * Things look ok. Fill in the ScsiCmd for the device.
	 */
	scsiCmd.dataToDevice = dataToDevice;
	scsiCmd.bufferLen = cmdPtr->bufferLen;
	scsiCmd.buffer = dataToDevice ? 
				(ioctlPtr->inBuffer+cmdPtr->dataOffset) :
				(ioctlPtr->outBuffer + sizeof(Dev_ScsiStatus));

	scsiCmd.commandBlockLen = cmdPtr->commandLen;
	bcopy(ioctlPtr->inBuffer+sizeof(Dev_ScsiCommand), scsiCmd.commandBlock,
	      scsiCmd.commandBlockLen);
	statusPtr = (Dev_ScsiStatus *) ioctlPtr->outBuffer;
	status = DevScsiSendCmdSync(devPtr, &scsiCmd, &statusByte,
		&(statusPtr->amountTransferred), &senseDataLen, senseData);
	statusPtr->statusByte = statusByte;
        statusPtr->senseDataLen = senseDataLen;
	senseBufLen = (ioctlPtr->outBufSize - sizeof(Dev_ScsiStatus) - 
			statusPtr->amountTransferred);
        if (senseBufLen > senseDataLen) {
	    senseBufLen = senseDataLen;
	}
	bcopy(senseData, ioctlPtr->outBufSize + sizeof(Dev_ScsiStatus) + 
			statusPtr->amountTransferred,
	     senseBufLen);
	return status;
    } else {
	return GEN_INVALID_ARG;
    }

}

/*
 *----------------------------------------------------------------------
 *
 * DevNoHBAAttachDevice --
 *
 *	A SCSI HBA attach procedure that always returns no device. This
 *	routine should be inserted into attach procedure for HBA types
 *	that aren't support on the machine.
 *
 * Results:
 *	NIL 
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ScsiDevice *
DevNoHBAAttachDevice(devicePtr, insertProc)
    Fs_Device	*devicePtr;	/* Device to attach. */
    void 	(*insertProc)(); /* Insert procedure to use. */
{
    return (ScsiDevice *) NIL;
}
