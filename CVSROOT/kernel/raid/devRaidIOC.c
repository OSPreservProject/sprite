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

#include "devRaid.h"
#include "devRaidUtil.h"


/*
 *----------------------------------------------------------------------
 *
 * ReportRequestError --
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
ReportRequestError(reqPtr)
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
 * ReportRaidDiskAtachError --
 *
 *	This procedure is called when a device specified in a RAID
 *	configuration file can not be attached.
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
ReportRaidDiskAttachError(type, unit)
    int		type, unit;
{
    printf("RAID:ATTACH_ERR:dev %d %d:Could not attach device.\n", type, unit);
}


/*
 *----------------------------------------------------------------------
 *
 * ReportHardInitFailure --
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
ReportHardInitFailure(stripeID)
    int		stripeID;
{
    printf("RAID:HARD_INIT_ERR:stripe %d\n", stripeID);
}


/*
 *----------------------------------------------------------------------
 *
 * ReportReconstructionFailure --
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
ReportReconstructionFailure(col, row)
    int		col, row;
{
    printf("RAID:RECONST_ERR:pos %d %d\n", row, col);
}


/*
 *----------------------------------------------------------------------
 *
 * FailRaidDisk --
 *
 *	Mark specified disk as failed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Marks specified disk as failed.
 *
 *----------------------------------------------------------------------
 */

void
FailRaidDisk(raidPtr, col, row, version)
    Raid	*raidPtr;
    int		 col, row;
    int		 version;
{
    RaidDisk	*diskPtr = raidPtr->disk[col][row];

    MASTER_LOCK(&diskPtr->mutex);
    if (version == diskPtr->version &&
	    (diskPtr->state == RAID_DISK_READY || 
		    diskPtr->state == RAID_DISK_RECONSTRUCT) ){
#ifndef TESTING
	diskPtr->state = RAID_DISK_FAILED;
	diskPtr->numValidSector = 0;
#endif TESTING
        MASTER_UNLOCK(&diskPtr->mutex);
	printf("RAID:DISK_FAILED:raid %d %d:pos %d %d %d:dev %d %d\n",
	        raidPtr->devicePtr->type, raidPtr->devicePtr->unit,
		row, col, version,
		diskPtr->device.type, diskPtr->device.unit);
    } else {
        MASTER_UNLOCK(&diskPtr->mutex);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * ReplaceRaidDisk --
 *
 *	Replace specified disk with new disk.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Replaces specified disk.
 *
 *----------------------------------------------------------------------
 */

void
ReplaceRaidDisk(raidPtr, col, row, version, type, unit, numValidSector)
    Raid	*raidPtr;
    int		 col, row;
    int		 version;
    int		 type, unit;
    int		 numValidSector;
{
    RaidDisk	*diskPtr    = raidPtr->disk[col][row];
    RaidDisk	*newDiskPtr = MakeRaidDisk(type, unit, numValidSector);

    diskPtr = raidPtr->disk[col][row];
    MASTER_LOCK(&diskPtr->mutex);
    if (diskPtr->version == version) {
	/*
	 * Currently, the useCount is not updated, therefore we do not know
	 * whether the old disk structure is still in use, therfore, we
	 * can not deallocate it.  (i.e. it will stay around forever)
	 */
	diskPtr->state = RAID_DISK_REPLACED;
	diskPtr->version = -version;
	newDiskPtr->version = version + 1;
	raidPtr->disk[col][row] = newDiskPtr;
        MASTER_UNLOCK(&diskPtr->mutex);
	printf("RAID:DISK_REPLACED:raid %d %d:pos %d %d %d:oldDev %d %d:newDev %d %d\n",
	        raidPtr->devicePtr->type, raidPtr->devicePtr->unit,
		row, col, version,
		diskPtr->device.type, diskPtr->device.unit,
		newDiskPtr->device.type, newDiskPtr->device.unit);
    } else {
        MASTER_UNLOCK(&diskPtr->mutex);
	FreeRaidDisk(newDiskPtr);
	printf("RAID:VERSION_MISMATCH:curVersion %d:spcifiedVersion %d\n",
		diskPtr->version, version);
    }
}
