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
#include "schedule.h"


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

static void
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
        printf("RAID:MSG:Reconstruction completed.\n");
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
#ifndef NODATA
	bzero(failedReqPtr->devReq.buffer, failedReqPtr->devReq.bufferLen);
#endif
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
	printf("RAID:RECON:%d %d %d\n",
		diskPtr->device.type, diskPtr->device.unit,
		SectorToStripeUnitID(raidPtr, diskPtr->numValidSector));
	UnlockStripe(stripeID);
	InitiateStripeReconstruction(reconstructionControlPtr);
    }
}
