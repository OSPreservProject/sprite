/* 
 * devSCSIRobot.c --
 *
 *      The standard Open, IOControl, and Close operations
 *      are defined here for a SCSI Tape Robot.
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * Copyright 1992 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: /cdrom/src/kernel/Cvsroot/kernel/dev/devSCSIRobot.c,v 9.4 92/05/13 13:23:53 mottsmth Exp $ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include <stdio.h>
#include <bstring.h>
#include <string.h>
#include <fs.h>
#include <dev.h>
#include <devInt.h>
#include <sys/scsi.h>
#include <scsiDevice.h>
#include <devSCSIRobot.h>
#include <dev/scsi.h>
#include <stdlib.h>
#include <dbg.h>
#include <mach.h>
     
static ReturnStatus InitExbRobot _ARGS_((Fs_Device *devicePtr,
     ScsiDevice *devPtr));
static ReturnStatus InitError _ARGS_((ScsiDevice *devPtr, 
     ScsiCmd *scsiCmdPtr));
     

/*
 *----------------------------------------------------------------------
 *
 * InitError --
 *
 *	Initial error proc used by InitExbRobot when it is initializing
 *	things for the real error handlers.  
 *
 * Results:
 *	DEV_OFFLINE if the device is offline, SUCCESS otherwise.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static ReturnStatus
InitError(devPtr, scsiCmdPtr)
    ScsiDevice	 *devPtr;	/* SCSI device that's complaining. */
    ScsiCmd	*scsiCmdPtr;	/* SCSI command that had the problem. */
{
    ScsiStatus 		*scsiStatusPtr;
    unsigned char	statusByte;
    ScsiClass7Sense	*sensePtr;

    statusByte = (unsigned char) scsiCmdPtr->statusByte;
    scsiStatusPtr = (ScsiStatus *) &statusByte;
    if (!scsiStatusPtr->check) {
	if (scsiStatusPtr->busy) {
	    return DEV_OFFLINE;
	}
	return SUCCESS;
    }
    sensePtr = (ScsiClass7Sense *) scsiCmdPtr->senseBuffer;
    if (sensePtr->key == SCSI_CLASS7_NOT_READY) {
	return DEV_OFFLINE;
    }
    if (sensePtr->key == SCSI_CLASS7_NO_SENSE) {
	return SUCCESS;
    }
    return FAILURE;
}



/* 
 *----------------------------------------------------------------------
 *
 * InitExbRobot --
 *
 *	Initialize the device driver state for a EXB-120 tape robot.
 *
 * Results:
 *	SUCCESS, if the robot driver is successfully initialized. A
 *	Sprite error code otherwise.
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */	
/*ARGSUSED*/
static ReturnStatus
InitExbRobot(devicePtr, devPtr)
     Fs_Device	*devicePtr;	/* Device info, unit number, etc. */
     ScsiDevice	*devPtr;	/* Attached EXB-120. */
{
    ScsiExbRobot	robotData;
    ScsiExbRobot 	*robotPtr;
    ReturnStatus 	status;
    ExbRobotInquiryData *inquiryDataPtr;

    /*
     * Determine the type of device from the inquiry return
     * by the attach.  Reject device if not a jukebox.
     */

    inquiryDataPtr = (ExbRobotInquiryData *) (devPtr->inquiryDataPtr);
    if (devPtr->inquiryLength > 0) {
	if (inquiryDataPtr->type != SCSI_MEDIUM_CHANGER_TYPE ||
	    devPtr->inquiryLength != sizeof(ExbRobotInquiryData)) {
	    return DEV_NO_DEVICE;
	}
    }
    devPtr->errorProc = InitError; 

    /*
     * Do a quick test to see if the device is ready.
     */ 

    status = DevScsiTestReady(devPtr);
    if (status != SUCCESS) {
	/*
	 * Give up if it still isn't ready.
	 */
	status = DevScsiTestReady(devPtr);
	if (status != SUCCESS) {
	    return status;
	}
    }
    if (devicePtr->data == (ClientData) NIL) {
	robotPtr = &robotData;
	bzero((char *) robotPtr, sizeof(ScsiExbRobot));
	robotPtr->devPtr = devPtr;
	robotPtr->state = SCSI_ROBOT_CLOSED;
	robotPtr->name = "SCSI Robot";
	robotPtr->chmAddr = (unsigned) 0x79; /* Default. */
    }
    else {
	robotPtr = (ScsiExbRobot *) (devicePtr->data);
    }

    /*
     * Allocate and return the ScsiExbRobot structure in the data field
     * of the Fs_Device.
     */
    
    if (status == SUCCESS  &&  devicePtr->data == (ClientData) NIL) {
	robotPtr = (ScsiExbRobot *) malloc(sizeof(ScsiExbRobot));
	*robotPtr = robotData;
	devicePtr->data = (ClientData) robotPtr;
	devPtr->clientData = (ClientData) robotPtr;
	if (devPtr->errorProc == InitError) {
	    devPtr->errorProc = DevSCSIExbRobotError;
	}
    }
    return status;
}



/*
 *----------------------------------------------------------------------
 *
 * DevSCSIExbRobotError --
 *
 *	Map SCSI errors indicated by the sense data into Sprite ReturnStatus
 *	and error message. This proceedure handles two types of 
 *	sense data Class 0 and class 7.
 *
 * Results:
 *	A sprite error code.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
/* ARGSUSED */
ReturnStatus
DevSCSIExbRobotError(devPtr, scsiCmdPtr)
     ScsiDevice 	*devPtr;	/* SCSI Device that's complaining. */
     ScsiCmd		*scsiCmdPtr;	/* SCSI command that had the problem. */
{
    char errorString[MAX_SCSI_ERROR_STRING];
    char *name;
    ScsiExbRobot *robotPtr;
    int senseLength;
    ScsiClass0Sense *sensePtr;
    unsigned char statusByte;
    ScsiStatus *statusPtr;
    ReturnStatus status;
    
    statusByte = scsiCmdPtr->statusByte;
    statusPtr = (ScsiStatus *) &statusByte;
    sensePtr = (ScsiClass0Sense *) scsiCmdPtr->senseBuffer;
    senseLength = scsiCmdPtr->senseLen;
    robotPtr = (ScsiExbRobot *) devPtr->clientData;
    name = devPtr->locationName;

    if (!statusPtr->check) {
	if (SCSI_RESERVED_STATUS(statusByte)  ||  statusPtr->intStatus) {
	    printf("Warning: %s at %s unknown status byte 0x%x\n",
		   robotPtr->name, name, statusByte);
	    return SUCCESS;
	}
	if (statusPtr->busy) {
	    return DEV_OFFLINE;
	}
	return SUCCESS;
    }
    if (senseLength == 0) {
	printf("Warning: %s at %s error: no sense data\n", robotPtr->name, name);
	return DEV_NO_SENSE;
    }

    /*
     * The EXB-120 only returns class 7 (extended sense) data.
     */
    
    DevScsiMapClass7Sense(senseLength, (char *)sensePtr,
			  &status, errorString);
    if (errorString[0]) {
	printf("Warning: %s at %s error: %s\n", robotPtr->name, name,
	       errorString);
    }
    return status;
}
    


/*
 *---------------------------------------------------------------------
 *
 * DevSCSIExbRobotOpen --
 *
 *	Open an EXB-120 jukebox robot as a file. This routine verifies
 * 	the device's existence and sets any special mode flags.
 *
 * Results:
 *	SUCCESS if the device is online.
 *
 * Side effects:
 * 	None.
 *
 *---------------------------------------------------------------------
 */
/* ARGSUSED */
ReturnStatus
DevSCSIExbRobotOpen(devicePtr, useFlags, token, flagsPtr)
     Fs_Device *devicePtr;	/* Device info, lun, etc. */
     int useFlags;		/* Flags from the stream being opened (unused) */
     Fs_NotifyToken token;	/* Call-back token for input (unused) */
     int *flagsPtr;		/* OUT: Device flags (unused) */
{
    ReturnStatus status;
    ScsiDevice *devPtr;
    ScsiExbRobot *robotPtr;
    
    robotPtr = (ScsiExbRobot *) (devicePtr->data);
    if (robotPtr == (ScsiExbRobot *) NIL) {
	/*
	 * Ask the HBA to set up the path to the device with FIFO ordering
	 * of requests.
	 */
	devPtr = DevScsiAttachDevice(devicePtr, DEV_QUEUE_FIFO_INSERT);
	if (devPtr == (ScsiDevice *) NIL) {
	    return DEV_NO_DEVICE;
	}
    }
    else {
	/*
	 * If the robotPtr is already attached to the device, it must
	 * be busy.
	 */
	return FS_FILE_BUSY;
    }
    status = InitExbRobot(devicePtr, devPtr);
    return status;
}



/*---------------------------------------------------------------------
 *
 * DevSCSIExbRobotClose --
 *
 *	Close an EXB-120 jukebox. Free all data structures associated
 * 	with the jukebox.
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
DevSCSIExbRobotClose(devicePtr, useFlags, openCount, writerCount)
     Fs_Device 	*devicePtr;
     int useFlags;
     int openCount;
     int writerCount;
{
    ScsiExbRobot *robotPtr;
    ReturnStatus status;

    status = SUCCESS;		/* There's no way we can fail. */

    robotPtr = (ScsiExbRobot *) (devicePtr->data);
    if (openCount > 0) {
	return status;
    }

    (void) DevScsiReleaseDevice(robotPtr->devPtr);
    free((char *) robotPtr);
    devicePtr->data = (ClientData) NIL;
    return status;
}



/*
 *------------------------------------------------------------------------
 *
 * DevSCSIExbRobotIOControl --
 *
 * 	Do operations on the Exabyte EXB-120 Robot.
 *
 * Results:
 * 	The Sprite status.
 *
 * Side Effects:
 *      None.
 *
 *-----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
DevSCSIExbRobotIOControl(devicePtr, ioctlPtr, replyPtr)
     Fs_Device 		*devicePtr;
     Fs_IOCParam	*ioctlPtr;	/* Standard I/O Control parameter block */
     Fs_IOReply		*replyPtr;	/* Size of outBuffer and returned signal */
{
    ScsiExbRobot 	*robotPtr;
    ScsiCmd		scsiRobotCmd;
    ReturnStatus 	status;
    Dev_RobotCommand 	*cmdPtr;
    int 		scsiCmd;
    int 		amountTransferred;
    ExbRobotModeSelectData	modeSelData;
    
    bzero((char *) &scsiRobotCmd, sizeof(ScsiCmd));

    robotPtr = (ScsiExbRobot *) (devicePtr->data);
    if ((ioctlPtr->command & ~0xffff) == IOC_SCSI) {
	status = DevScsiIOControl(robotPtr->devPtr, ioctlPtr, replyPtr);
	return status;
    }

    cmdPtr = (Dev_RobotCommand *) ioctlPtr->inBuffer;
    amountTransferred = 0;

    switch (ioctlPtr->command) {
    case IOC_ROBOT_INIT_ELEM_STATUS:
	scsiCmd = SCSI_INIT_ELEM_STATUS;
	ExbRobotInitElemStatus(&scsiRobotCmd, cmdPtr);
	break;
    case IOC_ROBOT_INQUIRY: 
	scsiCmd = SCSI_INQUIRY;
	if (ioctlPtr->outBuffer == NULL) {
	    return DEV_INVALID_ARG;
	}
	ExbRobotInquiry(&scsiRobotCmd,
			(ExbRobotInquiryData *)ioctlPtr->outBuffer);
	break;
    case IOC_ROBOT_MODE_SEL: 
	scsiCmd = SCSI_MODE_SELECT;
	ExbModeSelect(&scsiRobotCmd, cmdPtr, NULL, 0);
	break;
    case IOC_ROBOT_DISPLAY: 
	scsiCmd = SCSI_MODE_SELECT;
	bzero((char *) &modeSelData, sizeof(ExbRobotModeSelectData));
	ExbModeSelect(&scsiRobotCmd, cmdPtr,
		      (Address)&modeSelData, sizeof(modeSelData));
	break;
    case IOC_ROBOT_MOVE_MEDIUM:
	scsiCmd = SCSI_MOVE_MEDIUM;
	ExbMoveMedium(&scsiRobotCmd, robotPtr, cmdPtr);
	break;
    case IOC_ROBOT_POS_ELEM:
	scsiCmd = SCSI_POSITION_ELEMENT;
	ExbPosElem(&scsiRobotCmd, robotPtr, cmdPtr);
	break;
    case IOC_ROBOT_PREV_REMOVAL:
	scsiCmd = SCSI_PREVENT_ALLOW;
	ExbPrevRemoval(&scsiRobotCmd, cmdPtr);
	break;
    case IOC_ROBOT_NO_OP:
	scsiCmd = SCSI_TEST_UNIT_READY;
	scsiRobotCmd.commandBlockLen = 6;
	break;
    case IOC_ROBOT_REQ_SENSE:
	scsiCmd = SCSI_REQUEST_SENSE;
	if (ioctlPtr->outBuffer == NULL) {
	    return DEV_INVALID_ARG;
	}
	ExbReqSense(&scsiRobotCmd,
		    (ExbRobotSenseData *)ioctlPtr->outBuffer);
	break;
    default:
	/* Invalid or Command not supported. */
	scsiCmd = 0;
	panic("Fs_IOControl -- Tape Robot: Unknown command %d\n", 
	      ioctlPtr->command);
    }

    /* Since we've set up the SCSI CDB, send the block out. */

    status = DevScsiSendCmdSync(robotPtr->devPtr, &scsiRobotCmd, 
				&amountTransferred);
    return status;
}



/*
 *---------------------------------------------------------------------
 *
 * ExbRobotInitElemStatus --
 *
 *	Fill in the SCSI CDB for an InitElemStatus command to an Exabyte
 *      EXB-120 robot.
 * 	
 * Results:
 *	None.
 *
 * Side effects:
 * 	None.
 *
 *---------------------------------------------------------------------
 */
void
ExbRobotInitElemStatus(scsiRobotCmdPtr, cmdPtr)
     ScsiCmd		*scsiRobotCmdPtr;
     Dev_RobotCommand 	*cmdPtr;
{
    InitElemStatCommand *cdbPtr;
    
    cdbPtr = (InitElemStatCommand *) scsiRobotCmdPtr->commandBlock;
    bzero((char *) cdbPtr, sizeof(InitElemStatCommand));
    
    scsiRobotCmdPtr->dataToDevice = FALSE;
    scsiRobotCmdPtr->commandBlockLen = sizeof(InitElemStatCommand);

    /*
     * Fill in Command Descriptor Block. All fields are initially assumed
     * to contain 0.
     */
    
    cdbPtr->command = SCSI_INIT_ELEM_STATUS;
    cdbPtr->unitNumber = 0; 
    cdbPtr->range = cmdPtr->range;
    cdbPtr->elemAddr[0] = (cmdPtr->elemAddr) >> 8;
    cdbPtr->elemAddr[1] = ((cmdPtr->elemAddr) << 24) >> 24;
    cdbPtr->numElements[0] = (cmdPtr->numElements) >> 8;
    cdbPtr->numElements[1] = ((cmdPtr->numElements) << 24) >> 24;
}



/*
 *---------------------------------------------------------------------
 *
 * ExbRobotInquiry --
 *
 *	Fill in the SCSI CDB for an Inquiry command to an Exabyte
 *      EXB-120 robot.
 * 	
 * Results:
 *	None.
 *
 * Side effects:
 * 	
 *
 *---------------------------------------------------------------------
 */
void
ExbRobotInquiry(scsiRobotCmdPtr, inquiryDataPtr)
     ScsiCmd			*scsiRobotCmdPtr;
     ExbRobotInquiryData	*inquiryDataPtr;
{
    ScsiInquiryCommand *cdbPtr;
    
    cdbPtr = (ScsiInquiryCommand *) scsiRobotCmdPtr->commandBlock;
    bzero((char *) cdbPtr, sizeof(ScsiInquiryCommand));
    
    scsiRobotCmdPtr->dataToDevice = FALSE;
    scsiRobotCmdPtr->bufferLen = sizeof(ExbRobotInquiryData);
    scsiRobotCmdPtr->buffer = (Address) inquiryDataPtr;
    scsiRobotCmdPtr->commandBlockLen = sizeof(ScsiInquiryCommand);

    /*
     * Fill in Command Descriptor Block. All fields are initially assumed
     * to contain 0.
     */
    
    cdbPtr->command = SCSI_INQUIRY;
    cdbPtr->unitNumber = 0; 
    cdbPtr->allocLength = sizeof(ExbRobotInquiryData);
}


/*----------------------------------------------------------------
 *
 * ExbModeSelect --
 *
 *	Fill in the SCSI CDB for a Mode Select Command to an
 *	Exabyte EXB-120 robot.
 *
 * Results:
 *	None.
 *
 * Side effects:
 * 	None.
 *
 *-----------------------------------------------------------------
 */ 
void
ExbModeSelect(scsiRobotCmdPtr, cmdPtr, dataPtr, dataLength)
     ScsiCmd		*scsiRobotCmdPtr;
     Dev_RobotCommand	*cmdPtr;
     Address		dataPtr;
     unsigned int	dataLength;
{
    ScsiModeSelectCommand *cdbPtr;
    
    cdbPtr = (ScsiModeSelectCommand *) scsiRobotCmdPtr->commandBlock;

    if (dataPtr != NULL) {
	scsiRobotCmdPtr->dataToDevice = TRUE;
	((ExbRobotModeSelectData *) dataPtr)->vendorUnique.mesgDispCntrl = 
		cmdPtr->mesgDisplay;
	strncpy(((ExbRobotModeSelectData *) dataPtr)->vendorUnique.displayMessage, 
		cmdPtr->mesgString, 60);
	((ExbRobotModeSelectData *) dataPtr)->vendorUnique.paramListLength = 62;
    }
    else {
	scsiRobotCmdPtr->dataToDevice = FALSE;
    }
    scsiRobotCmdPtr->commandBlockLen = sizeof(ScsiModeSelectCommand);
    scsiRobotCmdPtr->buffer = dataPtr;
    scsiRobotCmdPtr->bufferLen = 68;

    /* Fill in Command Descriptor Block. */    
    
    cdbPtr->command = SCSI_MODE_SELECT;
    cdbPtr->unitNumber = 0;
    cdbPtr->pageFormat = 1;		
    cdbPtr->savedPage = cmdPtr->savedPage;
    cdbPtr->paramListLength = dataLength;
}



/*
 *---------------------------------------------------------------------
 *
 * ExbMoveMedium -- 
 *
 *	Fill in the SCSI CDB for a MoveMedium command to an Exabyte
 *      EXB-120 robot.
 * 	
 * Results:
 *	None.
 *
 * Side effects:
 * 	None.
 *
 *---------------------------------------------------------------------
 */
void
ExbMoveMedium(scsiRobotCmdPtr, robotPtr, cmdPtr)
     ScsiCmd		*scsiRobotCmdPtr;
     ScsiExbRobot	*robotPtr;
     Dev_RobotCommand	*cmdPtr;
{
    MoveMediumCommand *cdbPtr;
    
    cdbPtr = (MoveMediumCommand *) scsiRobotCmdPtr->commandBlock;
    bzero((char *) cdbPtr, sizeof(MoveMediumCommand));
    
    scsiRobotCmdPtr->dataToDevice = FALSE;
    scsiRobotCmdPtr->bufferLen = 0;
    scsiRobotCmdPtr->commandBlockLen = sizeof(MoveMediumCommand);

    /* Fill in CDB. */

    cdbPtr->command = SCSI_MOVE_MEDIUM;
    cdbPtr->unitNumber = 0;
    cdbPtr->transpElemAddr[1] = (robotPtr->chmAddr << 24) >> 24;
    cdbPtr->transpElemAddr[0] = robotPtr->chmAddr >> 8;
    cdbPtr->sourceAddr[1] = ((cmdPtr->sourceAddr) << 24) >> 24;
    cdbPtr->sourceAddr[0] = (cmdPtr->sourceAddr) >> 8;
    cdbPtr->destAddr[1] = ((cmdPtr->destAddr) << 24) >> 24;
    cdbPtr->destAddr[0] = (cmdPtr->destAddr) >> 8;
    cdbPtr->eePos = cmdPtr->eePos;
}



/*--------------------------------------------------------------------
 *
 * ExbPosElem --
 *
 *	Fill in the SCSI CDB for a PositionToElement command to an 
 *      Exabyte EXB-120 robot.
 * 	
 * Results:
 *	None.
 *
 * Side effects:
 * 	None.
 *
 *---------------------------------------------------------------------
 */
void
ExbPosElem(scsiRobotCmdPtr, robotPtr, cmdPtr)
     ScsiCmd		*scsiRobotCmdPtr;
     ScsiExbRobot	*robotPtr;
     Dev_RobotCommand	*cmdPtr;
{
    PositionRobotCommand *cdbPtr;

    cdbPtr = (PositionRobotCommand *) scsiRobotCmdPtr->commandBlock;
    bzero((char *) cdbPtr, sizeof(PositionRobotCommand));

    scsiRobotCmdPtr->commandBlockLen = sizeof(PositionRobotCommand);
    scsiRobotCmdPtr->bufferLen = 0;

    /* Fill in CDB. */

    cdbPtr->command = SCSI_POSITION_ELEMENT;	/* Position to Element Command. */
    cdbPtr->unitNumber = 0;
    cdbPtr->transpElemAddr[1] = (robotPtr->chmAddr << 24) >> 24;
    cdbPtr->transpElemAddr[0] = robotPtr->chmAddr >> 8;
    cdbPtr->destAddr[1] = ((cmdPtr->destAddr) << 24) >> 24;
    cdbPtr->destAddr[0] = (cmdPtr->destAddr) >> 8;
}



/*--------------------------------------------------------------------
 *
 * ExbPrevRemoval -- 
 *
 *	Fill in the SCSI CDB for a Prevent/Allow Medium Removal
 *      command to an Exabyte EXB-120 robot.
 * 	
 * Results:
 *	None.
 *
 * Side effects:
 * 	None.
 *
 *---------------------------------------------------------------------
 */
void
ExbPrevRemoval(scsiRobotCmdPtr, cmdPtr)
     ScsiCmd 		*scsiRobotCmdPtr;
     Dev_RobotCommand 	*cmdPtr;
{
    ScsiPreventAllowCmd *cdbPtr;

    cdbPtr = (ScsiPreventAllowCmd *) scsiRobotCmdPtr->commandBlock;
    
    scsiRobotCmdPtr->commandBlockLen = sizeof(ScsiPreventAllowCmd);
    scsiRobotCmdPtr->bufferLen = 0;

    /* Fill in CDB. */

    cdbPtr->command = SCSI_PREVENT_ALLOW;	/* 0x1e */
    cdbPtr->unitNumber = 0;
    cdbPtr->prevent = cmdPtr->prevAllow;
}



/*--------------------------------------------------------------------
 *
 * ExbReqSense --
 *
 *	Fill in the SCSI CDB for a RequestSense command
 *      to be sent to an Exabyte EXB-120 robot.
 * 	
 * Results:
 *	None.
 *
 * Side effects:
 * 	None.
 *
 *---------------------------------------------------------------------
 */
void
ExbReqSense(scsiRobotCmdPtr, senseDataPtr)
     ScsiCmd 		*scsiRobotCmdPtr;
     ExbRobotSenseData	*senseDataPtr;
{
    ScsiRequestSenseCmd *cdbPtr;

    cdbPtr = (ScsiRequestSenseCmd *) scsiRobotCmdPtr->commandBlock;
    bzero((char *) cdbPtr, sizeof(ScsiRequestSenseCmd));

    scsiRobotCmdPtr->dataToDevice = FALSE;
    scsiRobotCmdPtr->commandBlockLen = sizeof(ScsiRequestSenseCmd);
    scsiRobotCmdPtr->bufferLen = sizeof(ExbRobotSenseData);
    scsiRobotCmdPtr->buffer = (Address) senseDataPtr;
    
    /* Fill in CDB. */

    cdbPtr->command = SCSI_REQUEST_SENSE;		/* 0x3 */
    cdbPtr->unitNumber = 0;
    cdbPtr->allocLen = SCSI_EXB_ROBOT_SENSE_LEN;
}    
