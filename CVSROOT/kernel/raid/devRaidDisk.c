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
#include "devRaidProto.h"


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
MakeRaidDisk(col, row, type, unit, version, numSector)
    int 	 col, row;
    int 	 type, unit, version;
    int		 numSector;
{
    RaidDisk	*diskPtr;

    diskPtr = (RaidDisk *) malloc(sizeof(RaidDisk));
    InitSema(&diskPtr->lock, "RaidDisk Mutex", 1);
    diskPtr->row = row;
    diskPtr->col = col;
    diskPtr->version = version;
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
    if (diskPtr->handlePtr != (DevBlockDeviceHandle *) NIL) {
        (void) Dev_BlockDeviceRelease(diskPtr->handlePtr);
    }
    free((char *) diskPtr);
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
    RaidDisk	*diskPtr;

    LockSema(&raidPtr->disk[col][row]->lock);
    diskPtr = raidPtr->disk[col][row];
    if (version == diskPtr->version && diskPtr->state == RAID_DISK_READY) {
	diskPtr->state = RAID_DISK_FAILED;
	printf("RAID:DISK_FAILED:raid %d %d:pos %d %d %d:dev %d %d\n",
	        raidPtr->devicePtr->type, raidPtr->devicePtr->unit,
		row, col, version,
		diskPtr->device.type, diskPtr->device.unit);
	SaveDiskState(raidPtr, col, row,
		diskPtr->device.type, diskPtr->device.unit, diskPtr->version,0);
	diskPtr->numValidSector = 0;
    }
    UnlockSema(&diskPtr->lock);
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

    LockSema(&raidPtr->disk[col][row]->lock);
    diskPtr = raidPtr->disk[col][row];
    if (diskPtr->version != version) {
	printf("RAID:VERSION_MISMATCH:curVersion %d:spcifiedVersion %d\n",
		diskPtr->version, version);
	UnlockSema(&diskPtr->lock);
	return;
    }
    if (diskPtr->state != RAID_DISK_FAILED) {
	printf("RAID:MSG:Attempted to replace non-failed disk.\n");
	UnlockSema(&diskPtr->lock);
	return;
    }
    diskPtr->state = RAID_DISK_REPLACED;
    diskPtr->version = -version;
    newDiskPtr = MakeRaidDisk(col, row, type, unit, 1, numValidSector);
    newDiskPtr->version = version + 1;
    SaveDiskState(raidPtr, col, row,
	    newDiskPtr->device.type, newDiskPtr->device.unit,
	    newDiskPtr->version, newDiskPtr->numValidSector);
printf("RAID:DISK_REPLACED:raid %d %d:pos %d %d %d:oldDev %d %d:newDev %d %d\n",
	    raidPtr->devicePtr->type, raidPtr->devicePtr->unit,
	    row, col, version,
	    diskPtr->device.type, diskPtr->device.unit,
	    newDiskPtr->device.type, newDiskPtr->device.unit);
    raidPtr->disk[col][row] = newDiskPtr;
    UnlockSema(&diskPtr->lock);
}
