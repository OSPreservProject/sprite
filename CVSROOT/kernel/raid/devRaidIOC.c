/* 
 * devRaidIOC.c --
 *
 *	Declarations for RAID device drivers.
 *
 * Copyright 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif /* not lint */

#include "sync.h"
#include <stdio.h>
#include "devRaid.h"
#include "devRaidUtil.h"


/*
 *----------------------------------------------------------------------
 *
 * Raid_ReportRequestError --
 *
 *	This procedure is called when an IO request issued by the RAID
 *	device driver failes.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Prints an error message.
 *
 *----------------------------------------------------------------------
 */

void
Raid_ReportRequestError(reqPtr)
    RaidBlockRequest	*reqPtr;
{
    printf("RAID:REQ_ERR:raid %d %d:pos %d %d %d:op %d:status %d:dev %d %d:req %d %d\n",
	    reqPtr->raidPtr->devicePtr->type, reqPtr->raidPtr->devicePtr->unit,
	    reqPtr->row, reqPtr->col, reqPtr->version,
	    reqPtr->devReq.operation, reqPtr->status,
	    reqPtr->diskPtr->device.type, reqPtr->diskPtr->device.unit,
	    reqPtr->devReq.startAddress, reqPtr->devReq.bufferLen);
}


/*
 *----------------------------------------------------------------------
 *
 * Raid_ReportHardInitFailure --
 *
 *	This procedure is called when a hard initialization failes on a stripe.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Prints an error message.
 *
 *----------------------------------------------------------------------
 */

void
Raid_ReportHardInitFailure(stripeID)
    int		stripeID;
{
    printf("RAID:HARD_INIT_ERR:stripe %d\n", stripeID);
}


/*
 *----------------------------------------------------------------------
 *
 * Raid_ReportParityCheckFailure --
 *
 *	This procedure is called when a parity check failes on a stripe.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Prints an error message.
 *
 *----------------------------------------------------------------------
 */

void
Raid_ReportParityCheckFailure(stripeID)
    int		stripeID;
{
    printf("RAID:PARITY_CHECK_ERR:stripe %d\n", stripeID);
}


/*
 *----------------------------------------------------------------------
 *
 * Raid_ReportReconstructionFailure --
 *
 *	This procedure is called when a reconstruction failes.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Prints an error message.
 *
 *----------------------------------------------------------------------
 */

void
Raid_ReportReconstructionFailure(col, row)
    int		col, row;
{
    printf("RAID:RECONST_ERR:pos %d %d\n", row, col);
}

