/* 
 * devRaidUtil.c --
 *
 *	Routines for allocating, initializing and deallocating various
 *	RAID data structures.
 *	Routines for mapping logical RAID sectors to physical devices.
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

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif /* not lint */

#include <stdio.h>
#include "sync.h"
#include "sprite.h"
#include "fs.h"
#include "dev.h"
/*
#include "devDiskLabel.h"
#include "devDiskStats.h"
*/
#include "devBlockDevice.h"
#include "devRaid.h"
#include "devRaidIOC.h"
#include "devRaidLock.h"
#include "stdlib.h"
#include "dbg.h"
#include "devRaidUtil.h"
#include "debugMem.h"
#include "machparam.h"


/*
 *----------------------------------------------------------------------
 *
 * MapPhysicalToStripeID --
 *
 *	Maps physical address (raid, col, row, sector) to stripeID.
 *
 * Results:
 *	stripeID
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
MapPhysicalToStripeID(raidPtr, col, row, sector, outStripeIDPtr)
    Raid	*raidPtr;
    int		 col;
    int		 row;
    unsigned	 sector;
    int		*outStripeIDPtr;
{
    int group, groupRow, stripeUnit;
    int stripeID;

    group    = row / raidPtr->rowsPerGroup;
    groupRow = row % raidPtr->rowsPerGroup;
    stripeUnit = sector / raidPtr->sectorsPerStripeUnit;

    stripeID = group * raidPtr->stripeUnitsPerDisk + stripeUnit;
    stripeID = stripeID * raidPtr->rowsPerGroup + groupRow;
    *outStripeIDPtr = stripeID;
}


/*
 *----------------------------------------------------------------------
 *
 * MapParity --
 *
 *	Maps logical sector address to (col, row, sector) of corresponding
 *	parity sector.
 *
 * Results:
 *      (col, row, sector).
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
MapParity(raidPtr, sectorNum, outColPtr, outRowPtr, sectorNumPtr)
    Raid	*raidPtr;
    unsigned	 sectorNum;
    int		*outColPtr, *outRowPtr;
    unsigned	*sectorNumPtr;
{
    int sector, col, groupRow, stripeUnit, group;
    int row, stripeID;

    sector     = sectorNum%raidPtr->sectorsPerStripeUnit;
    sectorNum /= raidPtr->sectorsPerStripeUnit;
    col        = sectorNum%raidPtr->numDataCol;
    sectorNum /= raidPtr->numDataCol;
    groupRow   = sectorNum%raidPtr->rowsPerGroup;
    sectorNum /= raidPtr->rowsPerGroup;
    stripeUnit = sectorNum%raidPtr->stripeUnitsPerDisk;
    sectorNum /= raidPtr->stripeUnitsPerDisk;
    group      = sectorNum%raidPtr->groupsPerArray;
    sectorNum /= raidPtr->groupsPerArray;

    if (sectorNum != 0) {
	(void)printf("Error: MapSector: sectorNum=%d\n", (int) sectorNum);
    }

    row = group * raidPtr->rowsPerGroup + groupRow;
    stripeID = group * raidPtr->stripeUnitsPerDisk + stripeUnit;
    stripeID = stripeID * raidPtr->rowsPerGroup + groupRow;

    /*
     * Rotate sectors/parity.
     */
    switch (raidPtr->parityConfig) {
    case 'R': /* Right Symetric */
	col = (raidPtr->numCol-1 + stripeID) % raidPtr->numCol;
	break;
    case 'L': /* Left Symetric */
	col = (raidPtr->numCol-1 - stripeID) % raidPtr->numCol;
	if (col < 0) {
	    col += raidPtr->numCol;
	}
	break;
    default:  /* No Rotation */
	col = raidPtr->numCol-1;
	break;
    }

    /*
     * Return values.
     */
    *outColPtr = col;
    *outRowPtr = row;
    *sectorNumPtr = stripeUnit*raidPtr->sectorsPerStripeUnit + sector;
    return;
}


/*
 *----------------------------------------------------------------------
 *
 * MapSector --
 *
 *	Maps logical sector address to (col, row, sector).
 *
 * Results:
 *      (col, row, sector).
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
MapSector(raidPtr, sectorNum, outColPtr, outRowPtr, sectorNumPtr)
    Raid	*raidPtr;
    unsigned	 sectorNum;
    int		*outColPtr, *outRowPtr;
    unsigned	*sectorNumPtr;
{
    int sector, col, groupRow, stripeUnit, group;
    int row, stripeID;

    sector     = sectorNum%raidPtr->sectorsPerStripeUnit;
    sectorNum /= raidPtr->sectorsPerStripeUnit;
    col        = sectorNum%raidPtr->numDataCol;
    sectorNum /= raidPtr->numDataCol;
    groupRow   = sectorNum%raidPtr->rowsPerGroup;
    sectorNum /= raidPtr->rowsPerGroup;
    stripeUnit = sectorNum%raidPtr->stripeUnitsPerDisk;
    sectorNum /= raidPtr->stripeUnitsPerDisk;
    group      = sectorNum%raidPtr->groupsPerArray;
    sectorNum /= raidPtr->groupsPerArray;

    if (sectorNum != 0) {
	(void)printf("Error: MapSector: sectorNum=%d\n", (int) sectorNum);
    }

    row = group * raidPtr->rowsPerGroup + groupRow;
    stripeID = group * raidPtr->stripeUnitsPerDisk + stripeUnit;
    stripeID = stripeID * raidPtr->rowsPerGroup + groupRow;

    /*
     * Rotate sectors/parity.
     */
    switch (raidPtr->parityConfig) {
    case 'R': /* Right Symetric */
	col = (col + stripeID) % raidPtr->numCol;
	break;
    case 'L': /* Left Symetric */
	col = (col - stripeID) % raidPtr->numCol;
	if (col < 0) {
	    col += raidPtr->numCol;
	}
	break;
    default:  /* No Rotation */
	break;
    }

    /*
     * Return values.
     */
    *outColPtr = col;
    *outRowPtr = row;
    *sectorNumPtr = stripeUnit*raidPtr->sectorsPerStripeUnit + sector;
}
