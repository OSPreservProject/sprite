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

#include "sync.h"
#include <string.h>
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
#include "dev/raid.h"
#include "devRaidLock.h"
#include "devRaidLog.h"
#include "stdlib.h"
#include "dbg.h"
#include "devRaidProto.h"

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
    MASTER_LOCK(&raidPtr->mutex);
    if ( raidPtr->state == RAID_INVALID ) {
	raidPtr->state = RAID_ATTACHED;
	MASTER_UNLOCK(&raidPtr->mutex);
#ifdef TESTING
	Sync_CondInit(&raidPtr->waitExclusive);
	Sync_CondInit(&raidPtr->waitNonExclusive);
#endif TESTING
	InitStripeLocks();
	InitDebugMem();
	raidPtr->devicePtr = devicePtr;
	LockRaid(raidPtr);
    } else {
	MASTER_UNLOCK(&raidPtr->mutex);
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

typedef struct {
    Sema	 sema;
    ReturnStatus status;
} IOCControl;

static void
iocDoneProc(iocCtrlPtr, status)
    IOCControl	*iocCtrlPtr;
    ReturnStatus status;
{
    printf("RAID:MSG:IOC completed.\n");
    if (status != SUCCESS) {
	printf("RAID:ERR:IOC failed.\n");
    }
    iocCtrlPtr->status = status;
    UpSema(&iocCtrlPtr->sema);
}

static ReturnStatus
IOControlProc(handlePtr, ioctlPtr, replyPtr) 
    DevBlockDeviceHandle	*handlePtr;
    Fs_IOCParam			*ioctlPtr;
    Fs_IOReply			*replyPtr;
{
    static char  *IObuf;
    RaidHandle	 *raidHandlePtr   = (RaidHandle *) handlePtr;
    Raid         *raidPtr         = raidHandlePtr->raidPtr;
    RaidIOCParam *raidIOCParamPtr = (RaidIOCParam *) ioctlPtr->inBuffer;
    DevBlockDeviceRequest *requestPtr =
	    (DevBlockDeviceRequest *) ioctlPtr->inBuffer;
    int		  col;
    int		  row;
    char	  fileName[80];
    ReturnStatus  status;
    IOCControl	  iocCtrl;

    if (raidIOCParamPtr == (RaidIOCParam *) NIL) {
	printf("RAID:MSG:IOControlProc IOC == NIL\n");
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
	status = RaidConfigure(raidPtr, raidIOCParamPtr->buf);
	return status;
    case IOC_DEV_RAID_FAIL:
	if (row < 0 || row >= raidPtr->numRow) {
	    printf("RAID:MSG:row=%d out of range on ioctl call", row);
	    return FAILURE;
	}
	if (col < 0 || col >= raidPtr->numCol) {
	    printf("RAID:MSG:col=%d out of range on ioctl call", col);
	    return FAILURE;
	}
	FailRaidDisk(raidPtr, col, row, raidPtr->disk[col][row]->version);
	return SUCCESS;
    case IOC_DEV_RAID_REPLACE:
	if (row < 0 || row >= raidPtr->numRow) {
	    printf("RAID:MSG:row=%d out of range on ioctl call", row);
	    return FAILURE;
	}
	if (col < 0 || col >= raidPtr->numCol) {
	    printf("RAID:MSG:col=%d out of range on ioctl call", col);
	    return FAILURE;
	}
	ReplaceRaidDisk(raidPtr, col, row, raidPtr->disk[col][row]->version,
		raidIOCParamPtr->type, raidIOCParamPtr->unit, 0);
	return SUCCESS;
    case IOC_DEV_RAID_HARDINIT:
	InitSema(&iocCtrl.sema, "Raid HardInit", 0);
	InitiateHardInit(raidPtr,
		raidIOCParamPtr->startStripe, raidIOCParamPtr->numStripe,
		iocDoneProc, (ClientData) &iocCtrl,
		raidIOCParamPtr->ctrlData);
	DownSema(&iocCtrl.sema);
	return iocCtrl.status;
    case IOC_DEV_RAID_PARITYCHECK:
	InitSema(&iocCtrl.sema, "Raid ParityCheck", 0);
	InitiateParityCheck(raidPtr,
		raidIOCParamPtr->startStripe, raidIOCParamPtr->numStripe,
		iocDoneProc, (ClientData) &iocCtrl,
		raidIOCParamPtr->ctrlData);
	DownSema(&iocCtrl.sema);
	return iocCtrl.status;
    case IOC_DEV_RAID_RECONSTRUCT:
	InitSema(&iocCtrl.sema, "Raid Reconstruct", 0);
	InitiateReconstruction(raidPtr, col, row,
		raidPtr->disk[col][row]->version,
		raidIOCParamPtr->numStripe, raidIOCParamPtr->uSec,
		iocDoneProc, (ClientData) &iocCtrl,
		raidIOCParamPtr->ctrlData);
	DownSema(&iocCtrl.sema);
	return iocCtrl.status;
    case IOC_DEV_RAID_IO:
	if (!IObuf) {
	    IObuf = (char *) malloc(1024*1024);
	}
	requestPtr->buffer = IObuf;
	return Dev_BlockDeviceIOSync(handlePtr, requestPtr,
		(int *)ioctlPtr->outBuffer);
    case IOC_DEV_RAID_LOCK:
	LockRaid(raidPtr);
	return SUCCESS;
    case IOC_DEV_RAID_UNLOCK:
	UnlockRaid(raidPtr);
	return SUCCESS;
    case IOC_DEV_RAID_SAVE_STATE:
	status = SaveRaidState(raidPtr);
	if (status != SUCCESS) {
	    printf("RAID:MSG:Could not checkpoint state.\n");
	}
	return status;
    case IOC_DEV_RAID_ENABLE_LOG:
	EnableLog(raidPtr);
	return SUCCESS;
    case IOC_DEV_RAID_DISABLE_LOG:
	DisableLog(raidPtr);
	return SUCCESS;
    case IOC_DEV_RAID_RESTORE_STATE:
	raidPtr->logDev.type = raidIOCParamPtr->type;
	raidPtr->logDev.unit = raidIOCParamPtr->unit;
	raidPtr->logDevOffset = raidIOCParamPtr->startStripe;
	/*
	 * Attach log device.
	 */
	raidPtr->logHandlePtr = Dev_BlockDeviceAttach(&raidPtr->logDev);
	if (raidPtr->logHandlePtr == (DevBlockDeviceHandle *) NIL) {
	    printf("RAID:ERR:Could not attach log device %d %d\n",
		    raidPtr->logDev.type, raidPtr->logDev.unit);
	    return FAILURE;
	}
	status = RestoreRaidState(raidPtr);
	if (status != SUCCESS) {
	    printf("RAID:MSG:Could not checkpoint state.\n");
	}
	return status;
    default:
	return SUCCESS;
    }
}


/*
 *----------------------------------------------------------------------
 *
 * StripeBlockIOProc --
 *    Perform block IO w/o parity, i.e. data striping only,
 *    on specified RAID device.
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
