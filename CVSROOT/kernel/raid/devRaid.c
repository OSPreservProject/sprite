/* 
 * devRaid.c --
 *
 *	This module implements the BlockDevice interface for
 *	RAID level 5 disk arrays.
 *	Assuming a minimum sector size of 512 bytes, RAID devices with an
 *	address space of upto 2^40 bytes (~1 terra byte) are supported by
 *	this driver.
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
#include <string.h>
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
#include "devRaidUtil.h"
#include "devRaidInitiate.h"
#include "dev/raid.h"
#include "devRaidIOC.h"
#include "devRaidLock.h"
#include "stdlib.h"
#include "dbg.h"
#include "strUtil.h"
#include "debugMem.h"

#define BITS_PER_ADDR			32
#define RAID_MAX_XFER_SIZE		(1<<30)
#ifdef TESTING
#define RAID_ROOT_CONFIG_FILE_NAME	"RAID"
#else
#define RAID_ROOT_CONFIG_FILE_NAME	"/sprite/users/eklee/RAIDconfig/RAID"
#endif TESTING
#define RAID_CONFIG_FILE_SUFFIX		".config"

static ReturnStatus StripeBlockIOProc();
static ReturnStatus RaidBlockIOProc();
static ReturnStatus ReleaseProc();
static ReturnStatus IOControlProc();

/*
 * A RAID device must have a minor number between 0 and 31 inclusive.
 */
Raid raidArray[] = {
    {RAID_INVALID, Sync_SemInitStatic("devRaid.c: RAID unit 0")},
    {RAID_INVALID, Sync_SemInitStatic("devRaid.c: RAID unit 1")},
    {RAID_INVALID, Sync_SemInitStatic("devRaid.c: RAID unit 2")},
    {RAID_INVALID, Sync_SemInitStatic("devRaid.c: RAID unit 3")},
    {RAID_INVALID, Sync_SemInitStatic("devRaid.c: RAID unit 4")},
    {RAID_INVALID, Sync_SemInitStatic("devRaid.c: RAID unit 5")},
    {RAID_INVALID, Sync_SemInitStatic("devRaid.c: RAID unit 6")},
    {RAID_INVALID, Sync_SemInitStatic("devRaid.c: RAID unit 7")},
    {RAID_INVALID, Sync_SemInitStatic("devRaid.c: RAID unit 8")},
    {RAID_INVALID, Sync_SemInitStatic("devRaid.c: RAID unit 9")},
    {RAID_INVALID, Sync_SemInitStatic("devRaid.c: RAID unit 10")},
    {RAID_INVALID, Sync_SemInitStatic("devRaid.c: RAID unit 11")},
    {RAID_INVALID, Sync_SemInitStatic("devRaid.c: RAID unit 12")},
    {RAID_INVALID, Sync_SemInitStatic("devRaid.c: RAID unit 13")},
    {RAID_INVALID, Sync_SemInitStatic("devRaid.c: RAID unit 14")},
    {RAID_INVALID, Sync_SemInitStatic("devRaid.c: RAID unit 15")},
    {RAID_INVALID, Sync_SemInitStatic("devRaid.c: RAID unit 16")},
    {RAID_INVALID, Sync_SemInitStatic("devRaid.c: RAID unit 17")},
    {RAID_INVALID, Sync_SemInitStatic("devRaid.c: RAID unit 18")},
    {RAID_INVALID, Sync_SemInitStatic("devRaid.c: RAID unit 19")},
    {RAID_INVALID, Sync_SemInitStatic("devRaid.c: RAID unit 20")},
    {RAID_INVALID, Sync_SemInitStatic("devRaid.c: RAID unit 21")},
    {RAID_INVALID, Sync_SemInitStatic("devRaid.c: RAID unit 22")},
    {RAID_INVALID, Sync_SemInitStatic("devRaid.c: RAID unit 23")},
    {RAID_INVALID, Sync_SemInitStatic("devRaid.c: RAID unit 24")},
    {RAID_INVALID, Sync_SemInitStatic("devRaid.c: RAID unit 25")},
    {RAID_INVALID, Sync_SemInitStatic("devRaid.c: RAID unit 26")},
    {RAID_INVALID, Sync_SemInitStatic("devRaid.c: RAID unit 27")},
    {RAID_INVALID, Sync_SemInitStatic("devRaid.c: RAID unit 28")},
    {RAID_INVALID, Sync_SemInitStatic("devRaid.c: RAID unit 29")},
    {RAID_INVALID, Sync_SemInitStatic("devRaid.c: RAID unit 30")},
    {RAID_INVALID, Sync_SemInitStatic("devRaid.c: RAID unit 31")},
};
int numRaid = sizeof(raidArray)/sizeof(Raid);


/*
 *----------------------------------------------------------------------
 *
 * RaidDeallocate --
 *
 *	Deallocate data structures associated with the specified raid device.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Deallocates data structures.
 *
 *----------------------------------------------------------------------
 */

static void
RaidDeallocate(raidPtr)
    Raid	*raidPtr;
{
    int		 col, row;

    MASTER_LOCK(&raidPtr->mutex);
    if (raidPtr->state != RAID_VALID) {
        MASTER_UNLOCK(&raidPtr->mutex);
	return;
    } else {
        raidPtr->state = RAID_BUSY;
        MASTER_UNLOCK(&raidPtr->mutex);
    }
    for ( col = 0; col < raidPtr->numCol; col++ ) {
        for ( row = 0; row < raidPtr->numRow; row++ ) {
	    if (raidPtr->disk[col][row] != NULL) {
		FreeRaidDisk(raidPtr->disk[col][row]);
	    }
	}
    }
    for ( col = 0; col < raidPtr->numCol; col++ ) {
	free((char *) raidPtr->disk[col]);
    }
    free((char *) raidPtr->disk);
    raidPtr->state = RAID_INVALID;
    return;
}


/*
 *----------------------------------------------------------------------
 *
 * RaidConfigure --
 *
 *	Configure raid device by reading the appropriate configuration file.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Allocates and initializes data structures for raid device.
 *
 *----------------------------------------------------------------------
 */

static ReturnStatus
RaidConfigure(raidPtr, devicePtr)
    Raid	*raidPtr;
    Fs_Device   *devicePtr;
{
#   define 	 CHAR_BUF_LEN	80
#   define 	 FILE_BUF_LEN	2000
    char	 fileName[CHAR_BUF_LEN], charBuf[CHAR_BUF_LEN];
    char	 fileBuf[FILE_BUF_LEN];
    char        *fileBufPtr;
    int		 col, row;
    int		 type, unit;
    int		 numScanned;
    RaidDisk    *raidDiskPtr;
    ReturnStatus status;

    InitStripeLocks();
    InitDebugMem();

    /*
     * If RAID device is already configured, deallocate it first.
     */
    RaidDeallocate(raidPtr);

    MASTER_LOCK(&raidPtr->mutex);
    if (raidPtr->state != RAID_INVALID) {
        MASTER_UNLOCK(&raidPtr->mutex);
	return FAILURE;
    } else {
        raidPtr->state = RAID_BUSY;
        MASTER_UNLOCK(&raidPtr->mutex);
    }

    /*
     * Create name of RAID configuration file.
     */
    sprintf(charBuf, "%d", devicePtr->unit);
    strcpy(fileName, RAID_ROOT_CONFIG_FILE_NAME);
    strcat(fileName, charBuf);
    strcat(fileName, RAID_CONFIG_FILE_SUFFIX);

    /*
     * Open and read configuration file into buffer.
     */
    status = ReadFile(fileName, FILE_BUF_LEN, fileBuf);
    if (status != SUCCESS) {
        raidPtr->state = RAID_INVALID;
	return status;
    }

    /*
     * Skip comments.
     */
    fileBufPtr = fileBuf;
    for (;;) {
        if (ScanLine(&fileBufPtr, charBuf) == (char *) NIL) {
    	    raidPtr->state = RAID_INVALID;
            return FAILURE;
        }
        if (charBuf[0] != '#') {
            break;
        }
    }

    /*
     * Read dimensions of raid device.
     */
    numScanned = sscanf(charBuf, "%d %d %d %d %d %d %c",
 			&raidPtr->numRow,
			&raidPtr->numCol,
			&raidPtr->logBytesPerSector,
			&raidPtr->sectorsPerStripeUnit,
			&raidPtr->stripeUnitsPerDisk,
			&raidPtr->rowsPerGroup,
			&raidPtr->parityConfig);
    if (numScanned != 7) {
    	raidPtr->state = RAID_INVALID;
	return FAILURE;
    }

    /*
     * Redundant but useful information.
     */
    if (raidPtr->parityConfig == 'S') {
	raidPtr->numDataCol = raidPtr->numCol;
    } else {
	raidPtr->numDataCol = raidPtr->numCol - 1;
    }
    raidPtr->groupsPerArray = raidPtr->numRow / raidPtr->rowsPerGroup;
    raidPtr->numSector  = (unsigned) raidPtr->numRow * raidPtr->numDataCol
	    * raidPtr->sectorsPerStripeUnit * raidPtr->stripeUnitsPerDisk;
    raidPtr->numStripe  = raidPtr->stripeUnitsPerDisk * raidPtr->numRow;
    raidPtr->dataSectorsPerStripe =
	    raidPtr->numDataCol * raidPtr->sectorsPerStripeUnit;
    raidPtr->sectorsPerDisk =
	    raidPtr->stripeUnitsPerDisk * raidPtr->sectorsPerStripeUnit;
    raidPtr->bytesPerStripeUnit = raidPtr->sectorsPerStripeUnit <<
	    raidPtr->logBytesPerSector;
    raidPtr->dataBytesPerStripe = raidPtr->dataSectorsPerStripe <<
	    raidPtr->logBytesPerSector;

    /*
     * Allocate RaidDisk structures; one for each logical disk.
     */
    raidPtr->disk = (RaidDisk ***)
		malloc((unsigned)raidPtr->numCol * sizeof(RaidDisk *));
    for ( col = 0; col < raidPtr->numCol; col++ ) {
	raidPtr->disk[col] = (RaidDisk **)
		malloc((unsigned)raidPtr->numRow * sizeof(RaidDisk));
	bzero((char*)raidPtr->disk[col],raidPtr->numRow*sizeof(RaidDisk));
    }

    /*
     * Initialize RaidDisk structures.
     */
    for ( row = 0; row < raidPtr->numRow; row++ ) {
        for ( col = 0; col < raidPtr->numCol; col++ ) {
	    if (ScanWord(&fileBufPtr, charBuf) == (char *)NIL) {
    		raidPtr->state = RAID_VALID;
		RaidDeallocate(raidPtr);
		return FAILURE;
	    }
	    type = atoi(charBuf);
	    if (ScanWord(&fileBufPtr, charBuf) == (char *)NIL) {
    		raidPtr->state = RAID_VALID;
		RaidDeallocate(raidPtr);
		return FAILURE;
	    }
	    unit = atoi(charBuf);

	    raidDiskPtr = MakeRaidDisk(type, unit, raidPtr->sectorsPerDisk);
	    if (raidDiskPtr == (RaidDisk *) NIL) {
    		raidPtr->state = RAID_VALID;
		RaidDeallocate(raidPtr);
		return FAILURE;
	    }
	    raidPtr->disk[col][row] = raidDiskPtr;
	}
    }
    raidPtr->devicePtr = devicePtr;
    raidPtr->state = RAID_VALID;
    return SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * DevRaidAttach --
 *
 *	Attach a RAID logical device.
 *
 * Results:
 *	The DevBlockDeviceHandle of the device.
 *
 * Side effects:
 *	Modifies raidArray data structure.
 *
 *----------------------------------------------------------------------
 */

DevBlockDeviceHandle *
DevRaidAttach(devicePtr)
    Fs_Device	*devicePtr;	/* The device to attach. */
{
    RaidHandle	*handlePtr;
    Raid	*raidPtr;

    if ( devicePtr->unit >= numRaid ) {
        return (DevBlockDeviceHandle *) NIL;
    }
    raidPtr = &raidArray[devicePtr->unit];
    if ( raidPtr->state == RAID_INVALID ) {
	if (RaidConfigure(raidPtr, devicePtr) != SUCCESS) {
        	return (DevBlockDeviceHandle *) NIL;
	}
    }

    handlePtr = (RaidHandle *) malloc(sizeof(RaidHandle));

    /*
     * 'S' means data striping only, no parity.
     * We use a different blockIOproc to support this function.
     */
    if (raidPtr->parityConfig == 'S') {
	handlePtr->blockHandle.blockIOProc = StripeBlockIOProc;
    } else {
	handlePtr->blockHandle.blockIOProc = RaidBlockIOProc;
    }
    handlePtr->blockHandle.releaseProc = ReleaseProc;
    handlePtr->blockHandle.IOControlProc = IOControlProc;
    handlePtr->blockHandle.minTransferUnit = 1 << raidPtr->logBytesPerSector;
    handlePtr->blockHandle.maxTransferSize = RAID_MAX_XFER_SIZE;
    handlePtr->devPtr = devicePtr;
    handlePtr->raidPtr = &raidArray[devicePtr->unit];
    return (DevBlockDeviceHandle *) handlePtr;
}


/*
 *----------------------------------------------------------------------
 *
 * ReleaseProc --
 *
 *	Block device release proc.
 *
 * Results:
 *	Return status.
 *
 * Side effects:
 *	Frees device handle.
 *
 *----------------------------------------------------------------------
 */

static ReturnStatus
ReleaseProc(handlePtr)
    RaidHandle	*handlePtr; /* Handle pointer of device. */
{
    /*
     * Note:  Only the handle is deallocated.
     * The Raid data structures are never deallocated and stay around forever.
     */
    free((char *) handlePtr);
    return SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * IOControlProc --
 *
 *      Do a special operation on a RAID device.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Depends on operation.
 *
 *----------------------------------------------------------------------
 */

static ReturnStatus
IOControlProc(handlePtr, ioctlPtr, replyPtr) 
    DevBlockDeviceHandle	*handlePtr;
    Fs_IOCParam			*ioctlPtr;
    Fs_IOReply			*replyPtr;
{
    RaidHandle	 *raidHandlePtr   = (RaidHandle *) handlePtr;
    Raid         *raidPtr         = raidHandlePtr->raidPtr;
    RaidIOCParam *raidIOCParamPtr = (RaidIOCParam *) ioctlPtr->inBuffer;
    int		  col;
    int		  row;

    if (raidIOCParamPtr == (RaidIOCParam *) NIL) {
	printf("Error:Raid:IOControlProc IOC == NIL\n");
	return FAILURE;
    }
    col = raidIOCParamPtr->col;
    row = raidIOCParamPtr->row;

    bzero((char *) replyPtr, sizeof(Fs_IOReply));

    switch(ioctlPtr->command) {
    case IOC_DEV_RAID_PRINT:
	PrintRaid(raidPtr);
	return SUCCESS;
    case IOC_DEV_RAID_RECONFIG:
	return RaidConfigure(raidPtr, raidPtr->devicePtr);
    case IOC_DEV_RAID_HARDINIT:
	InitiateHardInit(raidPtr,
		raidIOCParamPtr->startStripe, raidIOCParamPtr->numStripe,
		raidIOCParamPtr->ctrlData);
	return SUCCESS;
    case IOC_DEV_RAID_FAIL:
	if (row < 0 || row >= raidPtr->numRow) {
	    printf("RAID:ERRMSG:row=%d out of range on ioctl call", row);
	    return FAILURE;
	}
	if (col < 0 || col >= raidPtr->numCol) {
	    printf("RAID:ERRMSG:col=%d out of range on ioctl call", col);
	    return FAILURE;
	}
	FailRaidDisk(raidPtr, col, row, raidPtr->disk[col][row]->version);
	return SUCCESS;
    case IOC_DEV_RAID_REPLACE:
	if (row < 0 || row >= raidPtr->numRow) {
	    printf("RAID:ERRMSG:row=%d out of range on ioctl call", row);
	    return FAILURE;
	}
	if (col < 0 || col >= raidPtr->numCol) {
	    printf("RAID:ERRMSG:col=%d out of range on ioctl call", col);
	    return FAILURE;
	}
	ReplaceRaidDisk(raidPtr, col, row, raidPtr->disk[col][row]->version,
		raidIOCParamPtr->type, raidIOCParamPtr->unit, 0);
	return SUCCESS;
    case IOC_DEV_RAID_RECONSTRUCT:
	InitiateReconstruction(raidPtr, col, row,
		raidPtr->disk[col][row]->version,
		raidIOCParamPtr->numStripe, raidIOCParamPtr->uSec,
		raidIOCParamPtr->ctrlData);
	return SUCCESS;
    default:
	return SUCCESS;
    }
}


/*
 *----------------------------------------------------------------------
 *
 * StripeBlockIOProc --
 *
 *	Perform block IO (w/ parity) on specified RAID device.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The disk write, if operation == FS_WRITE.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
StripeBlockIOProc(handlePtr, requestPtr) 
    DevBlockDeviceHandle  *handlePtr;
    DevBlockDeviceRequest *requestPtr;
{
    RaidHandle	   *raidHandlePtr;
    Raid 	   *raidPtr;
    unsigned 	    firstSector;
    unsigned	    nthSector;
    unsigned	    numSector;

    raidHandlePtr = (RaidHandle *) handlePtr;
    raidPtr	  = raidHandlePtr->raidPtr;

    /*
     * Check if operation valid.
     */
    if (!((requestPtr->operation == FS_READ) ||
          (requestPtr->operation == FS_WRITE))) {
	panic("Unknown operation %d in RAID blockIOProc.\n", 
		requestPtr->operation);
	return DEV_INVALID_ARG;
    }

    /*
     * Convert byte addresses to sector addresses.
     */
    firstSector = (requestPtr->startAddress >> raidPtr->logBytesPerSector) |
        (requestPtr->startAddrHigh<<(BITS_PER_ADDR-raidPtr->logBytesPerSector));
    numSector = requestPtr->bufferLen >> raidPtr->logBytesPerSector;
    nthSector = firstSector + numSector;

    /*
     * Check that addresses are within the range supported by the device.
     */
    if ( firstSector >= raidPtr->numSector ) {
	requestPtr->doneProc(requestPtr, FAILURE, 0);
	return SUCCESS;
    } 

    /*
     * Prevent overruns.
     */
    if ( nthSector > raidPtr->numSector ) {
        nthSector = raidPtr->numSector;
    } 

    InitiateSimpleStripeIOs(raidPtr, requestPtr->operation,
            firstSector, nthSector, requestPtr->buffer,
            requestPtr->doneProc, (ClientData) requestPtr,
            requestPtr->ctrlData[0]);
    return SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * RaidBlockIOProc --
 *
 *	Perform block IO (w/ parity) on specified RAID device.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The disk write, if operation == FS_WRITE.
 *
 *----------------------------------------------------------------------
 */

static ReturnStatus
RaidBlockIOProc(handlePtr, requestPtr) 
    DevBlockDeviceHandle  *handlePtr;
    DevBlockDeviceRequest *requestPtr;
{
    RaidHandle	   *raidHandlePtr;
    Raid 	   *raidPtr;
    unsigned 	    firstSector;
    unsigned	    nthSector;
    unsigned	    numSector;

    raidHandlePtr = (RaidHandle *) handlePtr;
    raidPtr	  = raidHandlePtr->raidPtr;

    /*
     * Check if operation valid.
     */
    if (!((requestPtr->operation == FS_READ) ||
          (requestPtr->operation == FS_WRITE))) {
	panic("Unknown operation %d in RAID blockIOProc.\n", 
		requestPtr->operation);
	return DEV_INVALID_ARG;
    }

    /*
     * Convert byte addresses to sector addresses.
     */
    firstSector = (requestPtr->startAddress >> raidPtr->logBytesPerSector) |
        (requestPtr->startAddrHigh<<(BITS_PER_ADDR-raidPtr->logBytesPerSector));
    numSector = requestPtr->bufferLen >> raidPtr->logBytesPerSector;
    nthSector = firstSector + numSector;

    /*
     * Check that addresses are within the range supported by the device.
     */
    if ( firstSector >= raidPtr->numSector ) {
	requestPtr->doneProc(requestPtr, FAILURE, 0);
	return SUCCESS;
    } 

    /*
     * Prevent overruns.
     */
    if ( nthSector > raidPtr->numSector ) {
        nthSector = raidPtr->numSector;
    } 

    InitiateStripeIOs(raidPtr, requestPtr->operation,
            firstSector, nthSector, requestPtr->buffer,
            requestPtr->doneProc, (ClientData) requestPtr,
            requestPtr->ctrlData[0]);
    return SUCCESS;
}
