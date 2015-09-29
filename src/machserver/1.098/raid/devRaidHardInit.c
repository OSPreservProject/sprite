/* 
 * devRaidHardInit.c --
 *
 *	This file implements routines for regenerating the parity.
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

#include "sync.h"
#include <stdio.h>
#include <string.h>
#include "sprite.h"
#include "fs.h"
#include "dev.h"
#include "devBlockDevice.h"
#include "devRaid.h"
#include "semaphore.h"
#include "stdlib.h"
#include "devRaidUtil.h"
#include "schedule.h"
#include "devRaidProto.h"


/*
 *----------------------------------------------------------------------
 *
 * Raid_InitiateHardInit --
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
Raid_InitiateHardInit(raidPtr, startStripe, numStripe, doneProc,clientData,ctrlData)
    Raid	*raidPtr;
    int		 startStripe;
    int		 numStripe;
    void       (*doneProc)();
    ClientData   clientData;
    int		 ctrlData;
{
    RaidReconstructionControl	*reconstructionControlPtr;
    reconstructionControlPtr =
	    Raid_MakeReconstructionControl(raidPtr, (int) NIL, (int) NIL,
		    (RaidDisk *) NIL, doneProc, clientData, ctrlData);
    reconstructionControlPtr->stripeID = startStripe;
    reconstructionControlPtr->numStripe = numStripe;
    printf("RAID:MSG:Initiating reconstruction %d %d.\n",startStripe,numStripe);
    InitiateStripeHardInit(reconstructionControlPtr);
}


/*
 *----------------------------------------------------------------------
 *
 * hardInitDoneProc --
 *
 *	Callback procedure for Raid_InitiateHardInit.
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
hardInitDoneProc(reconstructionControlPtr)
    RaidReconstructionControl	*reconstructionControlPtr;
{
    reconstructionControlPtr->doneProc(reconstructionControlPtr->clientData,
	    reconstructionControlPtr->status);
    Raid_FreeReconstructionControl(reconstructionControlPtr);
}


/*
 *----------------------------------------------------------------------
 *
 * Raid_InitiateHardInitFailure --
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
Raid_InitiateHardInitFailure(reconstructionControlPtr)
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
    Raid_XLockStripe(raidPtr, stripeID);
    reqControlPtr->numReq = reqControlPtr->numFailed = 0;
    AddRaidDataRequests(reqControlPtr, raidPtr, FS_READ,
	    firstSector, nthSector, readBuf, ctrlData);
    if (reqControlPtr->numFailed == 0) {
	Raid_InitiateIORequests(reqControlPtr,
		hardInitReadDoneProc,
		(ClientData) reconstructionControlPtr);
    } else {
	Raid_InitiateHardInitFailure(reconstructionControlPtr);
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
	Raid_InitiateHardInitFailure(reconstructionControlPtr);
    } else {
#ifndef NODATA
	bzero(parityBuf, raidPtr->bytesPerStripeUnit);
#endif
	XorRaidRequests(reqControlPtr, raidPtr, parityBuf);
	AddRaidParityRequest(reqControlPtr, raidPtr, FS_WRITE,
		(unsigned) StripeIDToSector(raidPtr, stripeID),
		parityBuf, ctrlData);
	Raid_InitiateIORequests(reqControlPtr,
		hardInitWriteDoneProc,
		(ClientData) reconstructionControlPtr);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * hardInitWriteDoneProc --
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
#ifdef TESTING
#define NUM_REPORT_STRIPE 1
#else
#define NUM_REPORT_STRIPE 100
#endif

static void
hardInitWriteDoneProc(reconstructionControlPtr, numFailed)
    RaidReconstructionControl	*reconstructionControlPtr;
    int				 numFailed;
{
    Raid	*raidPtr = reconstructionControlPtr->raidPtr;
    int		stripeID = reconstructionControlPtr->stripeID;

    if (numFailed > 0) {
        Raid_ReportHardInitFailure(stripeID);
	reconstructionControlPtr->status = FAILURE;
    }
    if (stripeID % NUM_REPORT_STRIPE == 0) {
	printf("RAID:MSG:%d", stripeID);
    }
    Raid_XUnlockStripe(raidPtr, stripeID);
    reconstructionControlPtr->stripeID++;
    reconstructionControlPtr->numStripe--;
    InitiateStripeHardInit(reconstructionControlPtr);
}
