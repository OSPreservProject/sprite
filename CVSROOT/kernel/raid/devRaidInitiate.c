/* 
 * devRaidInitiate.c --
 *
 *	This file implements the BlockDevice interface for homogeneous disk
 *	arrays.
 *
 * Copyright 1989 Regents of the University of California
 * All rights reserved.
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#include <stdio.h>
#include <string.h>
#include "sync.h"
#include "sprite.h"
#include "fs.h"
#include "dev.h"
/* #include "devInt.h" */
#include "devDiskLabel.h"
#include "devDiskStats.h"
#include "devBlockDevice.h"
#include "devRaid.h"
#include "devRaidLock.h"
#include "stdlib.h"
#include "dbg.h"
#include "devRaidUtil.h"
#include "devRaidIOC.h"
#include "debugMem.h"


/*
 *----------------------------------------------------------------------
 *
 * InitiateIORequests --
 *
 *	Initiates IO requests specified by reqControlPtr.
 *	Calls doneProc with clientData, the number of requests that have
 *	failed and a pointer to the last failed request when the IO is complete.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The IO operations.
 *
 *----------------------------------------------------------------------
 */

static void blockIODoneProc();
static void nonInterruptLevelCallBackProc();

static void 
InitiateIORequests(reqControlPtr, doneProc, clientData)
    RaidRequestControl	*reqControlPtr;
    void	       (*doneProc)();
    ClientData		 clientData;
{
    RaidIOControl	*IOControlPtr;
    RaidBlockRequest	*reqPtr;
    int			 i;

    /*
     * Inititate IO's.
     */
    IOControlPtr = MakeIOControl(doneProc, clientData);
    IOControlPtr->numIO++;
    for ( i = 0; i < reqControlPtr->numReq; i++ ) {
	reqPtr = &reqControlPtr->reqPtr[i];
	if (reqPtr->state == REQ_READY) {
	    reqPtr->state = REQ_PENDING;
	    MASTER_LOCK(&IOControlPtr->mutex);
	    IOControlPtr->numIO++;
	    MASTER_UNLOCK(&IOControlPtr->mutex);
	    reqPtr->devReq.doneProc   = blockIODoneProc;
	    reqPtr->devReq.clientData = (ClientData) IOControlPtr;
	    (void) Dev_BlockDeviceIO(
		    reqPtr->raidPtr->disk[reqPtr->col][reqPtr->row]->handlePtr,
		    (DevBlockDeviceRequest *) reqPtr);
	}
    }

    MASTER_LOCK(&IOControlPtr->mutex);
    IOControlPtr->numIO--;
    if (IOControlPtr->numIO == 0) {
        MASTER_UNLOCK(&IOControlPtr->mutex);
        IOControlPtr->doneProc(IOControlPtr->clientData,
		IOControlPtr->numFailed, IOControlPtr->failedReqPtr);
	FreeIOControl(IOControlPtr);
    } else {
        MASTER_UNLOCK(&IOControlPtr->mutex);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * blockIODoneProc --
 *
 *	Callback procedure for InitiateIORequests.
 *	This procedure is called once each time an individual IO reqeust
 *	completes.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Reports errors.
 *
 *----------------------------------------------------------------------
 */

static void
blockIODoneProc(reqPtr, status, amountTransferred)
    RaidBlockRequest	*reqPtr;
    ReturnStatus	 status;
    int			 amountTransferred;
{
    RaidIOControl	*IOControlPtr;
    IOControlPtr = (RaidIOControl *) reqPtr->devReq.clientData;

    /*
     * Check to see if disk has failed since request was initiated.
     */
    if (!IsInRange(reqPtr->diskPtr,
	    ByteToSector(reqPtr->raidPtr, reqPtr->devReq.startAddress),
	    ByteToSector(reqPtr->raidPtr, reqPtr->devReq.bufferLen))) {
	status = FAILURE;
    }

    reqPtr->status = status;
    if (status != SUCCESS) {
        reqPtr->state = REQ_FAILED;
	ReportRequestError(reqPtr);
	if (reqPtr->devReq.operation == FS_WRITE) {
	    FailRaidDisk(reqPtr->raidPtr,
		    reqPtr->col, reqPtr->row, reqPtr->version);
	}
    } else {
        reqPtr->state = REQ_COMPLETED;
    }

    MASTER_LOCK(&IOControlPtr->mutex);
    IOControlPtr->amountTransferred += amountTransferred;

    /*
     * A Raid IO operation fails if any of the component operations fail.
     * Therefore, don't overwrite status if a previous operation has failed.
     */
    if (status != SUCCESS) {
        IOControlPtr->numFailed++;
        IOControlPtr->failedReqPtr = reqPtr;
    }

    /*
     * Check if all component IO's done.
     */
    IOControlPtr->numIO--;
    if (IOControlPtr->numIO == 0) {
        MASTER_UNLOCK(&IOControlPtr->mutex);
	/*
	 * this forces the call-back to happen at non-interrupt level
	 */
	Proc_CallFunc(nonInterruptLevelCallBackProc,(ClientData)IOControlPtr,0);
    } else {
        MASTER_UNLOCK(&IOControlPtr->mutex);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * nonInterruptLevelCallBackProc --
 *
 *	None-interrupt level callback procedure for InitiateIORequests.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */

static void
nonInterruptLevelCallBackProc(IOControlPtr)
    RaidIOControl	*IOControlPtr;
{
    IOControlPtr->doneProc(IOControlPtr->clientData,
	    IOControlPtr->numFailed, IOControlPtr->failedReqPtr);
    FreeIOControl(IOControlPtr);
}


/*
 *----------------------------------------------------------------------
 *
 * InitiateStripeIOFailure --
 *
 *	Causes the IO operation to fail, presumably because it can not
 *	be completely (i.e. more than one disk in a group has failed.)
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void stripeIODoneProc();

static void
InitiateStripeIOFailure(stripeIOControlPtr)
    RaidStripeIOControl	*stripeIOControlPtr;
{
    stripeIODoneProc(stripeIOControlPtr, 2);
}


/*
 *----------------------------------------------------------------------
 *
 * InitiateStripeWrite --
 *
 *	Initiates a stripe write (i.e. an IO that does not span stripe
 *	boundaries) via either InitiateIORequests, InitiateReconstructWrite
 *	or InitiateReadModifyWrite.
 *	Sets up the recovery procedure if recovery is possible.  Note that
 *	the recovery procedure for InitiateReconstructWrite is
 *	InitiateReadModifyWrite and visa versa.
 *	Calls callback procedure specified by stripeIOControlPtr with
 *	stripeIOControlPtr, number of requests that have failed and a
 *	pointer to the last failed request, when the IO is complete.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The IO operation.
 *
 *----------------------------------------------------------------------
 */

static void InitiateReadModifyWrite();
static void InitiateReconstructWrite();
static void oldInfoReadDoneProc();
static void stripeWriteDoneProc();

static void
InitiateStripeWrite(stripeIOControlPtr)
    RaidStripeIOControl	*stripeIOControlPtr;
{
    Raid		*raidPtr       = stripeIOControlPtr->raidPtr;
    unsigned		 firstSector   = stripeIOControlPtr->firstSector;
    unsigned		 nthSector     = stripeIOControlPtr->nthSector;
    Address		 buffer        = stripeIOControlPtr->buffer;
    int			 ctrlData      = stripeIOControlPtr->ctrlData;
    RaidRequestControl	*reqControlPtr = stripeIOControlPtr->reqControlPtr;
    char		*parityBuf     = stripeIOControlPtr->parityBuf;

    reqControlPtr->numReq = reqControlPtr->numFailed = 0;
    AddRaidParityRequest(reqControlPtr, raidPtr, FS_READ,
	    firstSector, parityBuf, ctrlData);
    /*
     * Check to see if parity disk has failed.
     */
    if (reqControlPtr->numFailed > 0) {
	/*
	 * If parity disk has failed, just write the data.
	 */
	reqControlPtr->numReq = reqControlPtr->numFailed = 0;
	AddRaidDataRequests(reqControlPtr, raidPtr, FS_WRITE,
		firstSector, nthSector, buffer, ctrlData);
	if (reqControlPtr->numFailed == 0) {
	    InitiateIORequests(stripeIOControlPtr->reqControlPtr,
		    stripeIODoneProc, (ClientData) stripeIOControlPtr);
	} else {
	    InitiateStripeIOFailure(stripeIOControlPtr);
	}
    } else if (raidPtr->dataSectorsPerStripe/(nthSector-firstSector) >= 2) {
	/*
	 * If half or more of the stripe is being written, do a
	 * reconstruct write.
	 */
	stripeIOControlPtr->recoverProc = InitiateReconstructWrite;
	InitiateReadModifyWrite(stripeIOControlPtr);
    } else {
	/*
	 * If less than half of the stripe is being written, do a
	 * read modity write.
	 */
	stripeIOControlPtr->recoverProc = InitiateReadModifyWrite;
	InitiateReconstructWrite(stripeIOControlPtr);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * InitiateReadModifyWrite --
 *
 *	Initiates a read modify write.  (i.e. read old data and old parity,
 *	computes the new parity and then writes the new data and new parity)
 *	Calls the recovery procedure if a read modify wirte can not complete.
 *	Calls callback procedure specified by stripeIOControlPtr with
 *	stripeIOControlPtr, number of requests that have failed and a
 *	pointer to the last failed request, when the IO is complete.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The IO operation.
 *
 *----------------------------------------------------------------------
 */

static void
InitiateReadModifyWrite(stripeIOControlPtr)
    RaidStripeIOControl	*stripeIOControlPtr;
{
    Raid		*raidPtr       = stripeIOControlPtr->raidPtr;
    unsigned		 firstSector   = stripeIOControlPtr->firstSector;
    unsigned		 nthSector     = stripeIOControlPtr->nthSector;
/*    Address		 buffer        = stripeIOControlPtr->buffer; */
    int			 ctrlData      = stripeIOControlPtr->ctrlData;
    RaidRequestControl	*reqControlPtr = stripeIOControlPtr->reqControlPtr;
    char		*parityBuf     = stripeIOControlPtr->parityBuf;
    char		*readBuf       = stripeIOControlPtr->readBuf;
    void	       (*initiateRecoveryProc)();

    reqControlPtr->numReq = reqControlPtr->numFailed = 0;
    AddRaidDataRequests(reqControlPtr, raidPtr, FS_READ,
	    firstSector, nthSector, readBuf, ctrlData);
    if (reqControlPtr->numReq == 1) {
	stripeIOControlPtr->rangeOff =
		reqControlPtr->reqPtr[0].devReq.startAddress;
	stripeIOControlPtr->rangeLen =
		reqControlPtr->reqPtr[0].devReq.bufferLen;
    } else {
	stripeIOControlPtr->rangeOff = 0;
	stripeIOControlPtr->rangeLen = raidPtr->bytesPerStripeUnit;
    }
    AddRaidParityRangeRequest(reqControlPtr, raidPtr, FS_READ,
	    firstSector, parityBuf, ctrlData,
	    stripeIOControlPtr->rangeOff, stripeIOControlPtr->rangeLen);
    if (reqControlPtr->numFailed == 0) {
	InitiateIORequests(stripeIOControlPtr->reqControlPtr,
		oldInfoReadDoneProc, (ClientData) stripeIOControlPtr);
    } else {
	initiateRecoveryProc = stripeIOControlPtr->recoverProc;
	stripeIOControlPtr->recoverProc = InitiateStripeIOFailure;
	initiateRecoveryProc(stripeIOControlPtr);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * InitiateReconstructWrite --
 *
 *	Initiates a reconstruct write.  (i.e. read rest of stripe if any
 *	computes the new parity and then writes the new data and new parity)
 *	Calls the recovery procedure if a read modify wirte can not complete.
 *	Calls callback procedure specified by stripeIOControlPtr with
 *	stripeIOControlPtr, number of requests that have failed and a
 *	pointer to the last failed request, when the IO is complete.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The IO operation.
 *
 *----------------------------------------------------------------------
 */

static void
InitiateReconstructWrite(stripeIOControlPtr)
    RaidStripeIOControl	*stripeIOControlPtr;
{
    Raid		*raidPtr       = stripeIOControlPtr->raidPtr;
    unsigned		 firstSector   = stripeIOControlPtr->firstSector;
    unsigned		 nthSector     = stripeIOControlPtr->nthSector;
/*    Address		 buffer        = stripeIOControlPtr->buffer; */
    int			 ctrlData      = stripeIOControlPtr->ctrlData;
    RaidRequestControl	*reqControlPtr = stripeIOControlPtr->reqControlPtr;
/*    char		*parityBuf     = stripeIOControlPtr->parityBuf; */
    char		*readBuf       = stripeIOControlPtr->readBuf;
    void	       (*initiateRecoveryProc)();

    /*
     * If writing only one stripe unit, range restrict the write.
     */
    if (SectorToStripeUnitID(raidPtr, firstSector) ==
	    SectorToStripeUnitID(raidPtr, nthSector-1)) {
	stripeIOControlPtr->rangeOff = SectorToByte(raidPtr, firstSector);
        stripeIOControlPtr->rangeLen =
		SectorToByte(raidPtr, nthSector-firstSector);
    } else {
	stripeIOControlPtr->rangeOff = 0;
	stripeIOControlPtr->rangeLen = raidPtr->bytesPerStripeUnit;
    }
    reqControlPtr->numReq = reqControlPtr->numFailed = 0;
    AddRaidDataRangeRequests(reqControlPtr, raidPtr, FS_READ,
	    FirstSectorOfStripe(raidPtr, firstSector), firstSector,
	    readBuf, ctrlData,
	    stripeIOControlPtr->rangeOff, stripeIOControlPtr->rangeLen);
    AddRaidDataRangeRequests(reqControlPtr, raidPtr, FS_READ,
	    nthSector, NthSectorOfStripe(raidPtr, firstSector),
	    readBuf + SectorToByte(raidPtr,
	    	    firstSector - FirstSectorOfStripe(raidPtr, firstSector)),
	    ctrlData,stripeIOControlPtr->rangeOff,stripeIOControlPtr->rangeLen);
    if (reqControlPtr->numFailed == 0) {
	InitiateIORequests(stripeIOControlPtr->reqControlPtr,
		oldInfoReadDoneProc, (ClientData) stripeIOControlPtr);
    } else {
	initiateRecoveryProc = stripeIOControlPtr->recoverProc;
	stripeIOControlPtr->recoverProc = InitiateStripeIOFailure;
	initiateRecoveryProc(stripeIOControlPtr);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * oldInfoReadDoneProc --
 *
 *	Callback procedure for InitiateReadModifyWrite and
 *	InitiateReconstructWrite.
 *	This procedure is called after the old data and parity have been read
 *	in the process of writing new data.
 *	If an error has occured, the recovery procedure is called.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	IO operations.
 *
 *----------------------------------------------------------------------
 */

static void
oldInfoReadDoneProc(stripeIOControlPtr, numFailed)
    RaidStripeIOControl	*stripeIOControlPtr;
    int		 	 numFailed;
{
    if (numFailed == 0) {
	char			*parityBuf;

        parityBuf = Malloc((unsigned)
			stripeIOControlPtr->raidPtr->bytesPerStripeUnit);
	bzero(parityBuf, stripeIOControlPtr->raidPtr->bytesPerStripeUnit);

	XorRaidRangeRequests(stripeIOControlPtr->reqControlPtr,
		stripeIOControlPtr->raidPtr, parityBuf,
		stripeIOControlPtr->rangeOff, stripeIOControlPtr->rangeLen);
        stripeIOControlPtr->reqControlPtr->numReq = 0;
        stripeIOControlPtr->reqControlPtr->numFailed = 0;
        AddRaidDataRangeRequests(stripeIOControlPtr->reqControlPtr,
		stripeIOControlPtr->raidPtr, FS_WRITE,
		stripeIOControlPtr->firstSector, stripeIOControlPtr->nthSector,
                stripeIOControlPtr->buffer, stripeIOControlPtr->ctrlData,
		stripeIOControlPtr->rangeOff, stripeIOControlPtr->rangeLen);
	XorRaidRangeRequests(stripeIOControlPtr->reqControlPtr,
		stripeIOControlPtr->raidPtr, parityBuf,
		stripeIOControlPtr->rangeOff, stripeIOControlPtr->rangeLen);
	Free(stripeIOControlPtr->parityBuf);
	stripeIOControlPtr->parityBuf = parityBuf;
        AddRaidParityRangeRequest(stripeIOControlPtr->reqControlPtr,
		stripeIOControlPtr->raidPtr, FS_WRITE,
	        stripeIOControlPtr->firstSector, stripeIOControlPtr->parityBuf,
		stripeIOControlPtr->ctrlData,
		stripeIOControlPtr->rangeOff, stripeIOControlPtr->rangeLen);
	switch (stripeIOControlPtr->reqControlPtr->numFailed) {
	case 0:
            InitiateIORequests(stripeIOControlPtr->reqControlPtr,
		    stripeWriteDoneProc, (ClientData) stripeIOControlPtr);
	    break;
	case 1:
            InitiateIORequests(stripeIOControlPtr->reqControlPtr,
		    stripeIODoneProc, (ClientData) stripeIOControlPtr);
	    break;
	default:
	    InitiateStripeIOFailure(stripeIOControlPtr);
	    break;
	}
    } else {
        void       (*initiateRecoveryProc)();
	initiateRecoveryProc = stripeIOControlPtr->recoverProc;
	stripeIOControlPtr->recoverProc = InitiateStripeIOFailure;
	initiateRecoveryProc(stripeIOControlPtr);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * stripeWriteDoneProc --
 *
 *	Callback procedure for oldInfoReadDoneProc.
 *	This procedure is called after the new data and parity have
 *	been written.
 *	Since one of the writes is redundant, the IO is considered to have
 *	succeeded as long as the number of failures is less than or equal
 *	to one.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
stripeWriteDoneProc(stripeIOControlPtr, numFailed)
    RaidStripeIOControl	*stripeIOControlPtr;
    int			 numFailed;
{
    if (numFailed <= 1) {
	stripeIODoneProc(stripeIOControlPtr, 0);
    } else {
	stripeIODoneProc(stripeIOControlPtr, numFailed);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * InitiateStripeRead --
 *
 *	Initiates a stripe read (i.e. an IO that does not span stripe
 *	boundaries).
 *	Calls callback procedure specified by stripeIOControlPtr with
 *	stripeIOControlPtr, number of requests that have failed and a
 *	pointer to the last failed request, when the IO is complete.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The IO operation.
 *
 *----------------------------------------------------------------------
 */

static void stripeReadDoneProc();
static void reconstructStripeReadDoneProc();
static void InitiateReconstructRead();

static void
InitiateStripeRead(stripeIOControlPtr)
    RaidStripeIOControl	*stripeIOControlPtr;
{
    Raid		*raidPtr       = stripeIOControlPtr->raidPtr;
    unsigned		 firstSector   = stripeIOControlPtr->firstSector;
    unsigned		 nthSector     = stripeIOControlPtr->nthSector;
    Address		 buffer        = stripeIOControlPtr->buffer;
    int			 ctrlData      = stripeIOControlPtr->ctrlData;
    RaidRequestControl	*reqControlPtr = stripeIOControlPtr->reqControlPtr;
/*    char		*parityBuf     = stripeIOControlPtr->parityBuf; */
/*    char		*readBuf       = stripeIOControlPtr->readBuf; */

    reqControlPtr->numReq = reqControlPtr->numFailed = 0;
    AddRaidDataRequests(reqControlPtr, raidPtr, FS_READ,
	    firstSector, nthSector, buffer, ctrlData);
    switch (reqControlPtr->numFailed) {
    case 0:
        InitiateIORequests(reqControlPtr,
		stripeReadDoneProc, (ClientData) stripeIOControlPtr);
	break;
    case 1:
	InitiateReconstructRead(stripeIOControlPtr);
	break;
    default:
	InitiateStripeIOFailure(stripeIOControlPtr);
	break;
    }
}


/*
 *----------------------------------------------------------------------
 *
 * stripeReadDoneProc --
 *
 *	Callback procedure for InitiateStripeRead.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
stripeReadDoneProc(stripeIOControlPtr, numFailed, failedReqPtr)
    RaidStripeIOControl	*stripeIOControlPtr;
    int			 numFailed;
    RaidBlockRequest	*failedReqPtr;
{
    switch (numFailed) {
    case 0:
	stripeIODoneProc(stripeIOControlPtr, numFailed);
	break;
    case 1:
	stripeIOControlPtr->reqControlPtr->failedReqPtr = failedReqPtr;
	InitiateReconstructRead(stripeIOControlPtr);
	break;
    default:
	stripeIODoneProc(stripeIOControlPtr, numFailed);
	break;
    }
}


/*
 *----------------------------------------------------------------------
 *
 * InitiateReconstructRead --
 *
 *	Initiates a reconstruct read (i.e. computes requested data by reading
 *	the rest of the stripe and parity).
 *	Calls callback procedure specified by stripeIOControlPtr with
 *	stripeIOControlPtr, number of requests that have failed and a
 *	pointer to the last failed request, when the IO is complete.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The IO operation.
 *
 *----------------------------------------------------------------------
 */

static void
InitiateReconstructRead(stripeIOControlPtr)
    RaidStripeIOControl	*stripeIOControlPtr;
{
    Raid		*raidPtr       = stripeIOControlPtr->raidPtr;
    unsigned		 firstSector   = stripeIOControlPtr->firstSector;
    unsigned		 nthSector     = stripeIOControlPtr->nthSector;
/*    Address		 buffer        = stripeIOControlPtr->buffer; */
    int			 ctrlData      = stripeIOControlPtr->ctrlData;
    RaidRequestControl	*reqControlPtr = stripeIOControlPtr->reqControlPtr;
    char		*parityBuf     = stripeIOControlPtr->parityBuf;
    char		*readBuf       = stripeIOControlPtr->readBuf;

    reqControlPtr->numFailed = 0;
    AddRaidDataRangeRequests(reqControlPtr, raidPtr, FS_READ,
	    FirstSectorOfStripe(raidPtr, firstSector), firstSector,
	    readBuf, ctrlData,
	    (int) reqControlPtr->failedReqPtr->devReq.startAddress,
	    reqControlPtr->failedReqPtr->devReq.bufferLen);
    AddRaidDataRangeRequests(reqControlPtr, raidPtr, FS_READ,
	    nthSector, NthSectorOfStripe(raidPtr, firstSector),
	    readBuf + SectorToByte(raidPtr,
		    firstSector - FirstSectorOfStripe(raidPtr, firstSector)),
	    ctrlData,
	    (int) reqControlPtr->failedReqPtr->devReq.startAddress,
	    reqControlPtr->failedReqPtr->devReq.bufferLen);
    AddRaidParityRangeRequest(reqControlPtr, raidPtr, FS_READ,
	    firstSector, parityBuf, ctrlData,
	    (int) reqControlPtr->failedReqPtr->devReq.startAddress,
	    reqControlPtr->failedReqPtr->devReq.bufferLen);
    switch (reqControlPtr->numFailed) {
    case 0:
	InitiateIORequests(stripeIOControlPtr->reqControlPtr,
		reconstructStripeReadDoneProc, (ClientData) stripeIOControlPtr);
	break;
    default:
	InitiateStripeIOFailure(stripeIOControlPtr);
	break;
    }
}


/*
 *----------------------------------------------------------------------
 *
 * reconstructStripeReadDoneProc --
 *
 *	Callback procedure for InitiateReconstructRead.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
reconstructStripeReadDoneProc(stripeIOControlPtr, numFailed)
    RaidStripeIOControl	*stripeIOControlPtr;
    int			 numFailed;
{
    RaidBlockRequest	*failedReqPtr;
    failedReqPtr = stripeIOControlPtr->reqControlPtr->failedReqPtr;

    switch (numFailed) {
    case 0:
	bzero(failedReqPtr->devReq.buffer, failedReqPtr->devReq.bufferLen);
	XorRaidRangeRequests(stripeIOControlPtr->reqControlPtr,
		stripeIOControlPtr->raidPtr, failedReqPtr->devReq.buffer,
		(int) failedReqPtr->devReq.startAddress,
		failedReqPtr->devReq.bufferLen);
	stripeIODoneProc(stripeIOControlPtr, numFailed);
	break;
    default:
	stripeIODoneProc(stripeIOControlPtr, numFailed);
	break;
    }
}


/*
 *----------------------------------------------------------------------
 *
 * InitiateSingleStripeIO --
 *
 *	Initiates a single stripe IO request.
 *	Locks stripe, does IO and then unlocks the stripe in order to
 *	guarantee the consistency of parity.  (The unlocking is done in the
 *	associated callback procedure.)
 *	Calls doneProc with clientData, status and the amount transferreed
 *	as arguments when the IO is completed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The IO operation.
 *	Locks stripe.
 *
 *----------------------------------------------------------------------
 */

static void stripeIODoneProc();

static void 
InitiateSingleStripeIO(raidPtr, operation, firstSector, nthSector,
				buffer, doneProc, clientData, ctrlData)
    Raid       *raidPtr;
    int		operation;
    unsigned 	firstSector, nthSector;
    Address  	buffer;
    void      (*doneProc)();
    ClientData	clientData;
    int         ctrlData;
{
    RaidStripeIOControl	*stripeIOControlPtr;
    stripeIOControlPtr = MakeStripeIOControl(raidPtr, operation,
	    firstSector, nthSector, buffer, doneProc, clientData, ctrlData);

    LockStripe(SectorToStripeID(raidPtr, stripeIOControlPtr->firstSector));
    switch (stripeIOControlPtr->operation) {
    case FS_READ:
	InitiateStripeRead(stripeIOControlPtr);
	break;
    case FS_WRITE:
	InitiateStripeWrite(stripeIOControlPtr);
	break;
    }
}


/*
 *----------------------------------------------------------------------
 *
 * stripeIODoneProc --
 *
 *	Callback procedure for InitiateSingleStripeIO.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Unlocks stripe.
 *
 *----------------------------------------------------------------------
 */

static void
stripeIODoneProc(stripeIOControlPtr, numFailed)
    RaidStripeIOControl	*stripeIOControlPtr;
    int			 numFailed;
{
    UnlockStripe(SectorToStripeID(stripeIOControlPtr->raidPtr,
	    stripeIOControlPtr->firstSector));
    if (numFailed == 0) {
    	stripeIOControlPtr->doneProc(stripeIOControlPtr->clientData, SUCCESS, 
		SectorToByte(stripeIOControlPtr->raidPtr,
			stripeIOControlPtr->nthSector -
			stripeIOControlPtr->firstSector));
    } else {
    	stripeIOControlPtr->doneProc(stripeIOControlPtr->clientData, FAILURE,0);
    }
    FreeStripeIOControl(stripeIOControlPtr);
}


/*
 *----------------------------------------------------------------------
 *
 * InitiateStripeIOs --
 *
 *	Breaks IO requests into single stripe requests.
 *	Calls doneProc with clientData, status and the amount transferreed
 *	as arguments when the IO is completed.
 *
 * Results:
 *	The return code from queuing the I/O operation.
 *
 * Side effects:
 *	The IO operation.
 *
 *----------------------------------------------------------------------
 */

static void singleStripeIODoneProc();

void 
InitiateStripeIOs(raidPtr, operation, firstSector, nthSector,
				buffer, doneProc, clientData, ctrlData)
    Raid       *raidPtr;
    int		operation;
    unsigned 	firstSector, nthSector;
    Address  	buffer;
    void      (*doneProc)();
    ClientData	clientData;
    int         ctrlData;
{
    RaidIOControl 	  *IOControlPtr;
    int 		   numSectorsToTransfer;
    unsigned 		   currentSector;

    /*
     * Break up IO request into stripe requests.
     */
    IOControlPtr = MakeIOControl(doneProc, clientData);
    IOControlPtr->numIO++;
    currentSector = firstSector;
    while ( currentSector < nthSector ) {
        numSectorsToTransfer = MIN( raidPtr->dataSectorsPerStripe -
                currentSector%raidPtr->dataSectorsPerStripe,
                nthSector - currentSector );

        MASTER_LOCK(&IOControlPtr->mutex);
	IOControlPtr->numIO++;
        MASTER_UNLOCK(&IOControlPtr->mutex);

        InitiateSingleStripeIO(raidPtr, operation,
                 currentSector, currentSector+numSectorsToTransfer, buffer,
                 singleStripeIODoneProc, (ClientData) IOControlPtr,
                 ctrlData);

        currentSector += numSectorsToTransfer;
	buffer += SectorToByte(raidPtr, numSectorsToTransfer);
    }

    MASTER_LOCK(&IOControlPtr->mutex);
    IOControlPtr->numIO--;
    if (IOControlPtr->numIO == 0) {
        MASTER_UNLOCK(&IOControlPtr->mutex);
        IOControlPtr->doneProc(IOControlPtr->clientData,
		IOControlPtr->status, IOControlPtr->amountTransferred);
	FreeIOControl(IOControlPtr);
    } else {
        MASTER_UNLOCK(&IOControlPtr->mutex);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * singleStripeIODoneProc --
 *
 *	Callback procedure for InitiateStripeIOs.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
singleStripeIODoneProc(IOControlPtr, status, amountTransferred)
    RaidIOControl	*IOControlPtr;
    ReturnStatus  	 status;
    int		  	 amountTransferred;
{
    MASTER_LOCK(&IOControlPtr->mutex);
    IOControlPtr->amountTransferred += amountTransferred;

    /*
     * A Raid IO operation fails if any of the component operations fail.
     * Therefore, don't overwrite status if a previous operation has failed.
     */
    if (IOControlPtr->status == SUCCESS) {
        IOControlPtr->status = status;
    }

    /*
     * Check if all component IO's done.
     */
    IOControlPtr->numIO--;
    if (IOControlPtr->numIO == 0) {
        MASTER_UNLOCK(&IOControlPtr->mutex);
        IOControlPtr->doneProc(IOControlPtr->clientData,
		IOControlPtr->status, IOControlPtr->amountTransferred);
	FreeIOControl(IOControlPtr);
    } else {
        MASTER_UNLOCK(&IOControlPtr->mutex);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * InitiateHardInit --
 *	
 *	Reconstructs the parity beginning at startStripe for numStripe.
 *	If numStripe is negative, all stripes will be reconstucted.
 *	(ctrlData is used by the debug device when debugging in user mode.)
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Parity is updated.
 *
 *----------------------------------------------------------------------
 */

static void InitiateStripeHardInit();
static void hardInitReadDoneProc();
static void hardInitWriteDoneProc();

void
InitiateHardInit(raidPtr, startStripe, numStripe, ctrlData)
    Raid	*raidPtr;
    int		 startStripe;
    int		 numStripe;
    int		 ctrlData;
{
    RaidReconstructionControl	*reconstructionControlPtr;
    reconstructionControlPtr =
	    MakeReconstructionControl(raidPtr, (int) NIL, (int) NIL,
		    (RaidDisk *) NIL, ctrlData);
    reconstructionControlPtr->stripeID = startStripe;
    reconstructionControlPtr->numStripe = numStripe;
    InitiateStripeHardInit(reconstructionControlPtr);
}


/*
 *----------------------------------------------------------------------
 *
 * hardInitDoneProc --
 *
 *	Callback procedure for InitiateHardInit.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

hardInitDoneProc(reconstructionControlPtr)
    RaidReconstructionControl	*reconstructionControlPtr;
{
    FreeReconstructionControl(reconstructionControlPtr);
}


/*
 *----------------------------------------------------------------------
 *
 * InitiateHardInitFailure --
 *
 *	Causes the initialization of the current stripe to fail.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Prints an error message.
 *
 *----------------------------------------------------------------------
 */

static void
InitiateHardInitFailure(reconstructionControlPtr)
    RaidReconstructionControl	*reconstructionControlPtr;
{
    hardInitWriteDoneProc(reconstructionControlPtr, 1);
}


/*
 *----------------------------------------------------------------------
 *
 * InitiateStripeHardInit --
 *
 *	Reconstructs the parity on a single stripe.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Locks stripe.
 *	Parity is updated.
 *
 *----------------------------------------------------------------------
 */

static void
InitiateStripeHardInit(reconstructionControlPtr)
    RaidReconstructionControl	*reconstructionControlPtr;
{
    Raid	       *raidPtr       = reconstructionControlPtr->raidPtr;


    int	       		ctrlData      = reconstructionControlPtr->ctrlData;
    RaidRequestControl *reqControlPtr = reconstructionControlPtr->reqControlPtr;
    char	       *readBuf       = reconstructionControlPtr->readBuf;

    int		        stripeID      = reconstructionControlPtr->stripeID;
    int		        numStripe     = reconstructionControlPtr->numStripe;
    unsigned	        firstSector;
    unsigned	        nthSector;

    firstSector = StripeIDToSector(raidPtr, stripeID);
    nthSector   = NthSectorOfStripe(raidPtr, firstSector);
    if (stripeID >= raidPtr->numStripe || stripeID < 0 || numStripe == 0) {
	hardInitDoneProc(reconstructionControlPtr);
	return;
    }
    LockStripe(stripeID);
    reqControlPtr->numReq = reqControlPtr->numFailed = 0;
    AddRaidDataRequests(reqControlPtr, raidPtr, FS_READ,
	    firstSector, nthSector, readBuf, ctrlData);
    if (reqControlPtr->numFailed == 0) {
	InitiateIORequests(reqControlPtr,
		hardInitReadDoneProc,
		(ClientData) reconstructionControlPtr);
    } else {
	InitiateHardInitFailure(reconstructionControlPtr);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * hardInitReadDoneProc --
 *
 *	Callback procedure for InitiateStripeHardInit.
 *	Called after the data on a stripe is read.
 *	Calculates the parity and then writes it.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Parity write.
 *
 *----------------------------------------------------------------------
 */

static void
hardInitReadDoneProc(reconstructionControlPtr, numFailed)
    RaidReconstructionControl	*reconstructionControlPtr;
    int			 	 numFailed;
{
    Raid	       *raidPtr       = reconstructionControlPtr->raidPtr;
    char	       *parityBuf     = reconstructionControlPtr->parityBuf;
    int	       		ctrlData      = reconstructionControlPtr->ctrlData;
    RaidRequestControl *reqControlPtr = reconstructionControlPtr->reqControlPtr;
    int		        stripeID      = reconstructionControlPtr->stripeID;

    if (numFailed > 0) {
	InitiateHardInitFailure(reconstructionControlPtr);
    } else {
	bzero(parityBuf, raidPtr->bytesPerStripeUnit);
	XorRaidRequests(reqControlPtr, raidPtr, parityBuf);
	AddRaidParityRequest(reqControlPtr, raidPtr, FS_WRITE,
		(unsigned) StripeIDToSector(raidPtr, stripeID),
		parityBuf, ctrlData);
	InitiateIORequests(reqControlPtr,
		hardInitWriteDoneProc,
		(ClientData) reconstructionControlPtr);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * blockIODoneProc --
 *
 *	Callback procedure for hardInitReadDoneProc.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Unlocks stripe and initiates reconstruction for the next stripe.
 *
 *----------------------------------------------------------------------
 */

static void
hardInitWriteDoneProc(reconstructionControlPtr, numFailed)
    RaidReconstructionControl	*reconstructionControlPtr;
    int				 numFailed;
{
    if (numFailed > 0) {
	ReportHardInitFailure(reconstructionControlPtr->stripeID);
    }
    UnlockStripe(reconstructionControlPtr->stripeID);
    reconstructionControlPtr->stripeID++;
    reconstructionControlPtr->numStripe--;
    InitiateStripeHardInit(reconstructionControlPtr);
}


/*
 *----------------------------------------------------------------------
 *
 * InitiateReconstruction --
 *
 *	Reconstruct the contents of the failed disk.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Reconstructs the contents of the failed disk.
 *
 *----------------------------------------------------------------------
 */

static void InitiateStripeReconstruction();

void
InitiateReconstruction(raidPtr, col, row, version, numSector, uSec, ctrlData)
    Raid	*raidPtr;
    int		 col, row, version;
    int		 numSector;
    int		 uSec;
    int		 ctrlData;
{
    RaidReconstructionControl	*reconstructionControlPtr;
    RaidDisk			*diskPtr = raidPtr->disk[col][row];

    MASTER_LOCK(&diskPtr->mutex);
    if (version == diskPtr->version && diskPtr->state == RAID_DISK_READY) {
        diskPtr->state = RAID_DISK_RECONSTRUCT;
        MASTER_UNLOCK(&diskPtr->mutex);
    } else {
        MASTER_UNLOCK(&diskPtr->mutex);
	return;
    }
    reconstructionControlPtr =
	    MakeReconstructionControl(raidPtr, col, row, diskPtr, ctrlData);
    InitiateStripeReconstruction(reconstructionControlPtr);
}


/*
 *----------------------------------------------------------------------
 *
 * reconstructionDoneProc --
 *
 *	Callback procedure for InitiateReconstruction.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

reconstructionDoneProc(reconstructionControlPtr)
    RaidReconstructionControl	*reconstructionControlPtr;
{
    RaidDisk	*diskPtr = reconstructionControlPtr->diskPtr;

    MASTER_LOCK(&diskPtr->mutex);
    if (diskPtr->state == RAID_DISK_RECONSTRUCT) {
	diskPtr->state = RAID_DISK_READY;
    }
    MASTER_UNLOCK(&diskPtr->mutex);
    FreeReconstructionControl(reconstructionControlPtr);
}


/*
 *----------------------------------------------------------------------
 *
 * InitiateReconstructionFailure --
 *
 *	Causes the reconstruction to fail.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
InitiateReconstructionFailure(reconstructionControlPtr)
    RaidReconstructionControl	*reconstructionControlPtr;
{
    int	         stripeID = reconstructionControlPtr->stripeID;

    UnlockStripe(stripeID);
    ReportReconstructionFailure(reconstructionControlPtr->col,
    	    reconstructionControlPtr->row);
    reconstructionDoneProc(reconstructionControlPtr);
}


/*
 *----------------------------------------------------------------------
 *
 * InitiateStripeReconstruction --
 *
 *	Reconstructs a single stripe.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void reconstructionReadDoneProc();
static void reconstructionWriteDoneProc();

static void
InitiateStripeReconstruction(reconstructionControlPtr)
    RaidReconstructionControl	*reconstructionControlPtr;
{
    Raid	       *raidPtr       = reconstructionControlPtr->raidPtr;
    int		        col           = reconstructionControlPtr->col;
    int		        row           = reconstructionControlPtr->row;
    RaidDisk	       *diskPtr       = reconstructionControlPtr->diskPtr;
    int	       		ctrlData      = reconstructionControlPtr->ctrlData;
    RaidRequestControl *reqControlPtr = reconstructionControlPtr->reqControlPtr;
    char	       *readBuf       = reconstructionControlPtr->readBuf;
    char	       *parityBuf     = reconstructionControlPtr->parityBuf;
    int		        stripeID;
    unsigned	        firstSector;
    unsigned	        nthSector;

    if (diskPtr->numValidSector == raidPtr->sectorsPerDisk) {
	reconstructionDoneProc(reconstructionControlPtr);
	return;
    }
    if (diskPtr->state != RAID_DISK_RECONSTRUCT) {
        printf("RAID:MSG:Reconctruction aborted.\n");
	reconstructionDoneProc(reconstructionControlPtr);
	return;
    }
    MapPhysicalToStripeID(raidPtr, col,row, diskPtr->numValidSector, &stripeID);
    reconstructionControlPtr->stripeID = stripeID;
    firstSector = StripeIDToSector(raidPtr, stripeID);
    nthSector   = NthSectorOfStripe(raidPtr, firstSector);
    LockStripe(stripeID);
    reqControlPtr->numReq = reqControlPtr->numFailed = 0;
    AddRaidDataRequests(reqControlPtr, raidPtr, FS_READ,
	    firstSector, nthSector, readBuf, ctrlData);
    AddRaidParityRequest(reqControlPtr, raidPtr, FS_READ,
	    firstSector, parityBuf, ctrlData);
    if (reqControlPtr->numFailed == 1) {
	InitiateIORequests(reqControlPtr,
		reconstructionReadDoneProc,
		(ClientData) reconstructionControlPtr);
    } else {
	InitiateReconstructionFailure(reconstructionControlPtr);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * reconstructionReadDoneProc --
 *
 *	Callback procedure for InitiateStripeReconstruction.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Computes and writes the reconstructed information.
 *
 *----------------------------------------------------------------------
 */

static void
reconstructionReadDoneProc(reconstructionControlPtr, numFailed)
    RaidReconstructionControl	*reconstructionControlPtr;
    int			 	 numFailed;
{
    Raid	       *raidPtr       = reconstructionControlPtr->raidPtr;
    RaidDisk	       *diskPtr       = reconstructionControlPtr->diskPtr;
    RaidRequestControl *reqControlPtr = reconstructionControlPtr->reqControlPtr;
    RaidBlockRequest   *failedReqPtr  =
	    	          reconstructionControlPtr->reqControlPtr->failedReqPtr;

    if (numFailed > 0) {
	InitiateReconstructionFailure(reconstructionControlPtr);
    } else {
	bzero(failedReqPtr->devReq.buffer, failedReqPtr->devReq.bufferLen);
	XorRaidRangeRequests(reqControlPtr, raidPtr,
		failedReqPtr->devReq.buffer,
		(int) failedReqPtr->devReq.startAddress,
		failedReqPtr->devReq.bufferLen);
	reqControlPtr->failedReqPtr->devReq.operation = FS_WRITE;
	reqControlPtr->failedReqPtr->state = REQ_READY;
	diskPtr->numValidSector =
		NthSectorOfStripeUnit(raidPtr, diskPtr->numValidSector);
	InitiateIORequests(reqControlPtr,
		reconstructionWriteDoneProc,
		(ClientData) reconstructionControlPtr);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * reconstructionWriteDoneProc --
 *
 *	Callback procedure for reconstructionReadDoneProc.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Initiates the reconstruction of the next sector on the failed device.
 *
 *----------------------------------------------------------------------
 */

static void
reconstructionWriteDoneProc(reconstructionControlPtr, numFailed)
    RaidReconstructionControl	*reconstructionControlPtr;
    int				 numFailed;
{
    Raid	       *raidPtr       = reconstructionControlPtr->raidPtr;
    RaidDisk	       *diskPtr       = reconstructionControlPtr->diskPtr;
    int		        stripeID      = reconstructionControlPtr->stripeID;

    if (numFailed > 0) {
        diskPtr->numValidSector -= raidPtr->sectorsPerStripeUnit;
	InitiateReconstructionFailure(reconstructionControlPtr);
    } else {
	UnlockStripe(stripeID);
	InitiateStripeReconstruction(reconstructionControlPtr);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * InitiateSimpleStripeIOs --
 *
 *	Breaks up IO requests in stripes and then initiates them.
 *	This procedure is used when the RAID device is configured without
 *	parity.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The IO operation.
 *
 *----------------------------------------------------------------------
 */

static void simpleStripeIODoneProc();

void 
InitiateSimpleStripeIOs(raidPtr, operation, firstSector, nthSector,
				buffer, doneProc, clientData, ctrlData)
    Raid       *raidPtr;
    int		operation;
    unsigned 	firstSector, nthSector;
    Address  	buffer;
    void      (*doneProc)();
    ClientData	clientData;
    int         ctrlData;
{
    RaidIOControl 	  *IOControlPtr;
    DevBlockDeviceRequest *devReqPtr;
    int 		   numSectorsToTransfer;
    unsigned 		   currentSector;
    unsigned 		   diskSector;
    int			   col, row;

    /*
     * Break up IO request into stripe requests.
     */
    IOControlPtr = MakeIOControl(doneProc, clientData);
    IOControlPtr->numIO++;
    currentSector = firstSector;
    while ( currentSector < nthSector ) {
        numSectorsToTransfer = MIN( raidPtr->sectorsPerStripeUnit -
                currentSector%raidPtr->sectorsPerStripeUnit,
                nthSector - currentSector );
	MapSector(raidPtr, currentSector, &col, &row, &diskSector);
	devReqPtr = MakeBlockDeviceRequest(raidPtr, operation,
		diskSector, numSectorsToTransfer, buffer,
		simpleStripeIODoneProc, (ClientData) IOControlPtr, ctrlData);

        MASTER_LOCK(&IOControlPtr->mutex);
	IOControlPtr->numIO++;
        MASTER_UNLOCK(&IOControlPtr->mutex);

        (void) Dev_BlockDeviceIO(raidPtr->disk[col][row]->handlePtr, devReqPtr);

        currentSector += numSectorsToTransfer;
	buffer += SectorToByte(raidPtr, numSectorsToTransfer);
    }

    MASTER_LOCK(&IOControlPtr->mutex);
    IOControlPtr->numIO--;
    if (IOControlPtr->numIO == 0) {
        MASTER_UNLOCK(&IOControlPtr->mutex);
        IOControlPtr->doneProc(IOControlPtr->clientData,
		IOControlPtr->status, IOControlPtr->amountTransferred);
	FreeIOControl(IOControlPtr);
    } else {
        MASTER_UNLOCK(&IOControlPtr->mutex);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * simpleStripeIODoneProc --
 *
 *	Callback procedure for InitiateSimpleStripeIOs.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Reports errors.
 *	Calls callback procedure.
 *
 *----------------------------------------------------------------------
 */

static void
simpleStripeIODoneProc(devReqPtr, status, amountTransferred)
    DevBlockDeviceRequest *devReqPtr;
    ReturnStatus  	 status;
    int		  	 amountTransferred;
{
    RaidIOControl	*IOControlPtr = (RaidIOControl *) devReqPtr->clientData;

    FreeBlockDeviceRequest(devReqPtr);

    MASTER_LOCK(&IOControlPtr->mutex);
    IOControlPtr->amountTransferred += amountTransferred;

    /*
     * A Raid IO operation fails if any of the component operations fail.
     * Therefore, don't overwrite status if a previous operation has failed.
     */
    if (IOControlPtr->status == SUCCESS) {
        IOControlPtr->status = status;
    }

    /*
     * Check if all component IO's done.
     */
    IOControlPtr->numIO--;
    if (IOControlPtr->numIO == 0) {
        MASTER_UNLOCK(&IOControlPtr->mutex);
        IOControlPtr->doneProc(IOControlPtr->clientData,
		IOControlPtr->status, IOControlPtr->amountTransferred);
	FreeIOControl(IOControlPtr);
    } else {
        MASTER_UNLOCK(&IOControlPtr->mutex);
    }
}
