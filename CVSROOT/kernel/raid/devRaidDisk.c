/* 
 * devRaidDisk.c --
 *
 *	Description.
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

#include "devRaid.h"


/*
 *----------------------------------------------------------------------
 *
 * MakeRaidDisk --
 *
 *	Allocate and initialize RaidDisk.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

RaidDisk *
MakeRaidDisk(col, row, type, unit, numSector)
    int 	 col, row;
    int 	 type, unit;
    int		 numSector;
{
    RaidDisk	*diskPtr;

    diskPtr = (RaidDisk *) malloc(sizeof(RaidDisk));
    Sync_SemInitDynamic(&diskPtr->mutex, "RaidDisk Mutex");
    diskPtr->row = row;
    diskPtr->col = col;
    diskPtr->version = 1;
    diskPtr->useCount = 1;
    diskPtr->device.type = type;
    diskPtr->device.unit = unit;
    diskPtr->handlePtr = Dev_BlockDeviceAttach(&diskPtr->device);
    if (diskPtr->handlePtr == (DevBlockDeviceHandle *) NIL) {
        diskPtr->numValidSector = 0;
        diskPtr->state = RAID_DISK_INVALID;
	ReportRaidDiskAttachError(type, unit);
    } else {
        diskPtr->numValidSector = numSector;
        diskPtr->state = RAID_DISK_READY;
    }
    return diskPtr;
}


/*
 *----------------------------------------------------------------------
 *
 * FreeRaidDisk --
 *
 *	Free RaidDisk.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void FreeRaidDisk(diskPtr)
    RaidDisk	*diskPtr;
{
    Sync_LockClear(&diskPtr->mutex);
    if (diskPtr->handlePtr != (DevBlockDeviceHandle *) NIL) {
        (void) Dev_BlockDeviceRelease(diskPtr->handlePtr);
    }
    free((char *) diskPtr);
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
    char	buf[120];

    MASTER_LOCK(&diskPtr->mutex);
    if (version == diskPtr->version &&
	    (diskPtr->state == RAID_DISK_READY || 
		    diskPtr->state == RAID_DISK_RECONSTRUCT) ){
	diskPtr->state = RAID_DISK_FAILED;
	diskPtr->numValidSector = 0;
        MASTER_UNLOCK(&diskPtr->mutex);
	printf("RAID:DISK_FAILED:raid %d %d:pos %d %d %d:dev %d %d\n",
	        raidPtr->devicePtr->type, raidPtr->devicePtr->unit,
		row, col, version,
		diskPtr->device.type, diskPtr->device.unit);
        sprintf(buf, "F %d %d %d\n", row, col, diskPtr->version);
	LogEntry(raidPtr, buf);
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
    RaidDisk	*newDiskPtr;
    char	 buf[120];

    diskPtr = raidPtr->disk[col][row];
    MASTER_LOCK(&diskPtr->mutex);
    if (diskPtr->version != version) {
        MASTER_UNLOCK(&diskPtr->mutex);
	printf("RAID:VERSION_MISMATCH:curVersion %d:spcifiedVersion %d\n",
		diskPtr->version, version);
	return;
    }
    if (diskPtr->state != RAID_DISK_FAILED) {
        MASTER_UNLOCK(&diskPtr->mutex);
	printf("RAID:MSG:Attempted to replace non-failed disk.\n");
	return;
    }
    /*
     * Currently, the useCount is not updated, therefore we do not know
     * whether the old disk structure is still in use, therfore, we
     * can not deallocate it.  (i.e. it will stay around forever)
     */
    newDiskPtr = MakeRaidDisk(col, row, type, unit, numValidSector);
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
    sprintf(buf, "R %d %d %d  %d %d\n", row, col, diskPtr->version,
		diskPtr->device.type, diskPtr->device.unit);
    LogEntry(raidPtr, buf);
}
