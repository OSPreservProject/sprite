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

#include <sprite.h>
#include <scsiDevice.h>
#include <scsiHBA.h>
#include <dev/scsi.h>
#include <sys/scsi.h>
#include <devQueue.h>
#include <fs.h>
#include <sync.h>
#include <stdlib.h>
#include <bstring.h>

static int scsiDoneProc _ARGS_((struct ScsiCmd *scsiCmdPtr, 
		    ReturnStatus status, int statusByte, 
		    int byteCount, int senseLength, Address senseDataPtr));


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
	/*
	 * The attach routines bzero the handle and only fill in some
	 * of the fields.  The rest of the kernel assumes that errorProc
	 * is either points to a routine or it is NIL.  Take care of it
	 * here.
	 */
	if ((handle != (ScsiDevice *) NIL) &&
	    (handle->errorProc == (ReturnStatus (*)()) 0)) {
	    handle->errorProc = (ReturnStatus (*)()) NIL;
	}
    }
    /*
     * If the inquiry data doesn't exists for this device yet send the
     * device a INQUIRY command. We try it twice because the device might
     * abort the INQUIRY with a UNIT_ATTENTION.
     */
    if ((handle != (ScsiDevice *) NIL) && (handle->inquiryLength == 0)) {
	int	tryNumber = 1;
	ReturnStatus	status = SUCCESS;

	while((tryNumber <= 2) && (handle->inquiryLength == 0) && 
	      (status != DEV_TIMEOUT)) { 
	    if (handle->inquiryDataPtr == (char *) 0) {
		handle->inquiryDataPtr = (char *) malloc(DEV_MAX_INQUIRY_SIZE);
	    }
	    (void) DevScsiGroup0Cmd(handle,SCSI_INQUIRY,0,DEV_MAX_INQUIRY_SIZE,
				    &inquiryCmdBlock);
	    inquiryCmdBlock.buffer = handle->inquiryDataPtr;
	    inquiryCmdBlock.bufferLen = DEV_MAX_INQUIRY_SIZE;
	    inquiryCmdBlock.dataToDevice = FALSE;
	    status =  DevScsiSendCmdSync(handle, &inquiryCmdBlock, 
			    &(handle->inquiryLength));
	    if (status != SUCCESS) {
		handle->inquiryLength = 0;
	    }
	    tryNumber++;
	}
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
    int		 *statusBytePtr; /* Area to store SCSI status byte. */
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

    *(syncCmdDataPtr->statusBytePtr) = (int) statusByte;
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
DevScsiSendCmdSync(scsiDevicePtr, scsiCmdPtr, amountTransferredPtr)
    ScsiDevice *scsiDevicePtr;  /* Handle for target device. */
    ScsiCmd	*scsiCmdPtr;		    /* SCSI command to be sent. */
    int		*amountTransferredPtr; /* OUT - Nuber of bytes transferred. */
{
    ReturnStatus status;
    SyncCmdBuf	 syncCmdData;

    scsiCmdPtr->clientData = (ClientData) &syncCmdData;
    scsiCmdPtr->doneProc = scsiDoneProc;
    scsiCmdPtr->senseLen = sizeof(scsiCmdPtr->senseBuffer);
    Sync_SemInitDynamic((&syncCmdData.mutex),"ScsiSyncCmdMutex");
    syncCmdData.done = FALSE;
    syncCmdData.statusBytePtr = &scsiCmdPtr->statusByte;
    syncCmdData.senseBufferPtr = (Address) scsiCmdPtr->senseBuffer;
    syncCmdData.senseBufferLenPtr = &scsiCmdPtr->senseLen;
    syncCmdData.countPtr = amountTransferredPtr;
    DevScsiSendCmd(scsiDevicePtr, scsiCmdPtr);
    MASTER_LOCK((&syncCmdData.mutex));
    while (syncCmdData.done == FALSE) { 
	Sync_MasterWait((&syncCmdData.wait),(&syncCmdData.mutex),FALSE);
    }
    MASTER_UNLOCK((&syncCmdData.mutex));
    status = syncCmdData.status;
    Sync_SemClear(&syncCmdData.mutex);
    if ((status == SUCCESS) && (scsiCmdPtr->statusByte != 0) &&
	(scsiDevicePtr->errorProc != (ReturnStatus (*)()) NIL)) {
	status = (scsiDevicePtr->errorProc)(scsiDevicePtr, scsiCmdPtr);
    }
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
    int		len;

    DevScsiGroup0Cmd(scsiDevicePtr,SCSI_TEST_UNIT_READY, 0, 0, &unitReadyCmd);
    unitReadyCmd.bufferLen = 0;
    len = 0;
    status = DevScsiSendCmdSync(scsiDevicePtr,&unitReadyCmd, &len);
    return status;
}
/*
 *----------------------------------------------------------------------
 *
 *  DevScsiStartStopUnit --
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
DevScsiStartStopUnit(scsiDevicePtr, start)
    ScsiDevice *scsiDevicePtr;  /* Handle for device to be released. */
    Boolean start;
{
    ScsiCmd		scsiCmd;	/* Scsi command buffer to fill in. */
    ScsiStartStopCmd 	*cmdPtr;
    ReturnStatus 	status;
    int			len;

    bzero((char *) &scsiCmd, sizeof(ScsiCmd));
    scsiCmd.commandBlockLen = sizeof(ScsiStartStopCmd);
    scsiCmd.bufferLen = 0;
    cmdPtr = (ScsiStartStopCmd *) scsiCmd.commandBlock;
    cmdPtr->command = SCSI_START_STOP;
    cmdPtr->unitNumber = scsiDevicePtr->LUN;
    cmdPtr->immed = 0;
    cmdPtr->loadEject = 0;
    cmdPtr->start = (start == TRUE) ? 1 : 0;
    len = 0;
    status = DevScsiSendCmdSync(scsiDevicePtr,&scsiCmd, &len);
    return status;
}

/*
 *----------------------------------------------------------------------
 *
 *  DevScsiReadBlockLimits --
 *
 * 	Send a Read Block Limits command to the device.
 *
 * Results:
 *	
 *
 * Side effects: 
 *	A SCSI_READ_BLOCK_LIMITS command is set to the device.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
DevScsiReadBlockLimits(scsiDevicePtr, minPtr, maxPtr)
    ScsiDevice 	*scsiDevicePtr; /* Handle for device to be released. */
    int		*minPtr;	/* Minimum block size. */
    int		*maxPtr;	/* Max block size. */
{
    ScsiCmd		cmd;	/* Scsi command buffer to fill in. */
    ReturnStatus 	status = SUCCESS;
    int			len;
    ScsiBlockLimits	limits;

    DevScsiGroup0Cmd(scsiDevicePtr,SCSI_READ_BLOCK_LIMITS, 0, 0, &cmd);
    cmd.dataToDevice = FALSE;
    cmd.bufferLen = sizeof(ScsiBlockLimits);
    cmd.buffer = (char *) &limits;
    len = 0;
    status = DevScsiSendCmdSync(scsiDevicePtr,&cmd, &len);
    *maxPtr = (limits.max2 << 16) | (limits.max1 << 8) | limits.max0;
    *minPtr = (limits.min1 << 8) | limits.min0;
    return status;
}


/*
 *----------------------------------------------------------------------
 *
 *  DevScsiModeSense --
 *
 * 	Send a Mode Sense command to the device.
 *
 * Results:
 *	
 *
 * Side effects: 
 *	A SCSI_READ_BLOCK_LIMITS command is set to the device.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
DevScsiModeSense(scsiDevicePtr, disableBlockDesc, pageControl, pageCode, 
	    vendor, sizePtr, bufferPtr)
    ScsiDevice 	*scsiDevicePtr; 	/* Handle for device to be released. */
    int		disableBlockDesc;	/* Disable block descriptor field */
    int		pageControl;		/* Page control field. */
    int		pageCode;		/* Page code field. */
    int		vendor;			/* Vendor unique field. */
    int		*sizePtr;		/* Size of buffer/data returned. */
    char	*bufferPtr;		/* Buffer for mode sense data. */
{
    ReturnStatus 	status = SUCCESS;
    int			len;
    ScsiCmd		scsiCmd;	/* Scsi command buffer to fill in. */
    ScsiModeSenseCmd 	*cmdPtr;

    bzero((char *) &scsiCmd, sizeof(ScsiCmd));
    scsiCmd.commandBlockLen = sizeof(ScsiModeSenseCmd);
    scsiCmd.dataToDevice = FALSE;
    scsiCmd.bufferLen = *sizePtr;
    scsiCmd.buffer = bufferPtr;
    cmdPtr = (ScsiModeSenseCmd *) scsiCmd.commandBlock;
    cmdPtr->command = SCSI_MODE_SENSE;
    cmdPtr->unitNumber = scsiDevicePtr->LUN;
    cmdPtr->disableBlockDesc = disableBlockDesc;
    cmdPtr->pageControl = pageControl;
    cmdPtr->pageCode = pageCode;
    cmdPtr->allocLen = *sizePtr;
    cmdPtr->vendor = vendor;
    len = 0;
    status = DevScsiSendCmdSync(scsiDevicePtr,&scsiCmd, &len);
    *sizePtr = len;
    return status;
}


/*
 *----------------------------------------------------------------------
 *
 *  DevScsiRequestSense --
 *
 * 	Send a Request Sense command to the device.
 *
 * Results:
 *	
 *
 * Side effects: 
 *	A SCSI_REQUEST_SENSE command is set to the device.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
DevScsiRequestSense(scsiDevicePtr, clearCount, vendor, sizePtr, 
	bufferPtr)
    ScsiDevice 	*scsiDevicePtr; 	/* Handle for device to be released. */
    int		clearCount;		/* Clear counters field. */
    int		vendor;			/* Vendor unique field. */
    int		*sizePtr;		/* Size of buffer/data returned. */
    char	*bufferPtr;		/* Buffer for mode sense data. */
{
    ReturnStatus 	status = SUCCESS;
    int			len;
    ScsiCmd		scsiCmd;	/* Scsi command buffer to fill in. */
    ScsiRequestSenseCmd 	*cmdPtr;

    bzero((char *) &scsiCmd, sizeof(ScsiCmd));
    scsiCmd.commandBlockLen = sizeof(ScsiRequestSenseCmd);
    scsiCmd.dataToDevice = FALSE;
    scsiCmd.bufferLen = *sizePtr;
    scsiCmd.buffer = bufferPtr;
    cmdPtr = (ScsiRequestSenseCmd *) scsiCmd.commandBlock;
    cmdPtr->command = SCSI_REQUEST_SENSE;
    cmdPtr->unitNumber = scsiDevicePtr->LUN;
    cmdPtr->allocLen = *sizePtr;
    cmdPtr->clearCount = clearCount;
    cmdPtr->vendor = vendor;
    len = 0;
    status = DevScsiSendCmdSync(scsiDevicePtr,&scsiCmd, &len);
    *sizePtr = len;
    return status;
}


/*
 *----------------------------------------------------------------------
 *
 *  DevScsiReadPosition --
 *
 * 	Send a Read Position command to the device.
 *
 * Results:
 *	
 *
 * Side effects: 
 *	A SCSI_READ_POSITION command is set to the device.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
DevScsiReadPosition(scsiDevicePtr, blockType, positionPtr)
    ScsiDevice 	*scsiDevicePtr; 	/* Handle for device to be released. */
    int		blockType;		/* Block type field. */
    ScsiReadPositionResult *positionPtr; /* Position information. */
{
    ReturnStatus 	status = SUCCESS;
    int			len;
    ScsiCmd		scsiCmd;	/* Scsi command buffer to fill in. */
    ScsiReadPositionCmd 	*cmdPtr;

    bzero((char *) &scsiCmd, sizeof(ScsiCmd));
    scsiCmd.commandBlockLen = sizeof(ScsiReadPositionCmd);
    scsiCmd.dataToDevice = FALSE;
    scsiCmd.bufferLen = sizeof(ScsiReadPositionResult);
    scsiCmd.buffer = (char *) positionPtr;
    cmdPtr = (ScsiReadPositionCmd *) scsiCmd.commandBlock;
    cmdPtr->command = SCSI_READ_POSITION;
    cmdPtr->unitNumber = scsiDevicePtr->LUN;
    cmdPtr->blockType = blockType;
    len = 0;
    status = DevScsiSendCmdSync(scsiDevicePtr,&scsiCmd, &len);
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
/*ARGSUSED*/
ReturnStatus
DevScsiIOControl(devPtr, ioctlPtr, replyPtr)
    ScsiDevice	*devPtr;	/* SCSI Handle for device. */
    Fs_IOCParam *ioctlPtr;	/* Standard I/O Control parameter block */
    Fs_IOReply *replyPtr;	/* Size of outBuffer and returned signal */
{
    ReturnStatus	status;

    if (ioctlPtr->command == IOC_SCSI_COMMAND) {
	Dev_ScsiCommand	 *cmdPtr;
	Dev_ScsiStatus	 *statusPtr;
	ScsiCmd		 scsiCmd;
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
	status = DevScsiSendCmdSync(devPtr, &scsiCmd, 
			&(statusPtr->amountTransferred));
	statusPtr->statusByte = scsiCmd.statusByte;
        statusPtr->senseDataLen = scsiCmd.senseLen;
	senseBufLen = (ioctlPtr->outBufSize - sizeof(Dev_ScsiStatus) - 
			statusPtr->amountTransferred);
        if (senseBufLen > statusPtr->senseDataLen) {
	    senseBufLen = statusPtr->senseDataLen;
	}
	if (senseBufLen >= 0) {
	    bcopy(scsiCmd.senseBuffer, 
			(char *)(ioctlPtr->outBuffer + sizeof(Dev_ScsiStatus)
					 + statusPtr->amountTransferred),
	                 senseBufLen);
	}
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
