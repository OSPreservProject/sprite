/* 
 * devRaidLog.c --
 *
 *	Implements logging and recovery for raid devices.
 *
 * Copyright 1990 Regents of the University of California
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
#include <sprite.h>
#include <stdio.h>
#include <stdlib.h>
#include "devRaid.h"
#include "devRaidDisk.h"
#include "devRaidLog.h"
#include "bitvec.h"
#include "semaphore.h"
#include "devRaidProto.h"

#define ConfigLoc(raidPtr) ((int)(raidPtr)->logDevOffset)

#define DiskLoc(raidPtr, col, row)	\
    ((int)(raidPtr->logDevOffset+((raidPtr)->logHandlePtr->minTransferUnit*((col)*(raidPtr)->numRow+(row)+1))))

#define VecOffset(raidPtr, bit)	\
	((int)((((bit)/8)/(raidPtr)->log.logHandlePtr->minTransferUnit) \
		*(raidPtr)->log.logHandlePtr->minTransferUnit))

#define VecLoc(raidPtr, bit)	\
	((int)((raidPtr)->log.logDevOffset + VecOffset(raidPtr, bit)))

#define VecNextLoc(raidPtr, bit)	\
	((int)((raidPtr)->log.logDevOffset +		\
		(((bit)/8)/(raidPtr)->log.logHandlePtr->minTransferUnit + 1) \
		*(raidPtr)->log.logHandlePtr->minTransferUnit))

/*
 *----------------------------------------------------------------------
 *
 * Raid_InitLog --
 *
 * Results:
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */
void
Raid_InitLog(raidPtr)
    Raid *raidPtr;
{
    Sync_SemInitDynamic(&raidPtr->log.mutex, "RAID log mutex");
    raidPtr->log.enabled = 0;
    raidPtr->log.busy = 0;
    raidPtr->log.logHandlePtr = raidPtr->logHandlePtr;
    raidPtr->log.logDevOffset =
	    DiskLoc(raidPtr, raidPtr->numCol, raidPtr->numRow);
    raidPtr->log.diskLockVecNum = raidPtr->numStripe;
    raidPtr->log.diskLockVecSize = VecSize(raidPtr->log.diskLockVecNum);
    raidPtr->log.diskLockVec = MakeBitVec(8 *
	(VecNextLoc(raidPtr,raidPtr->log.diskLockVecNum) - VecLoc(raidPtr, 0)));
    raidPtr->log.minLogElem = raidPtr->log.diskLockVecNum;
    raidPtr->log.maxLogElem = 0;
    raidPtr->log.waitCurBufPtr = &raidPtr->log.flushed1;
    raidPtr->log.waitNextBufPtr = &raidPtr->log.flushed2;
#ifdef TESTING
    Sync_CondInit(&raidPtr->log.flushed1);
    Sync_CondInit(&raidPtr->log.flushed2);
#endif TESTING
}


/*
 *----------------------------------------------------------------------
 *
 * Raid_EnableLog --
 *
 * Results:
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */
void
Raid_EnableLog(raidPtr)
    Raid *raidPtr;
{
    raidPtr->log.enabled = 1;
}


/*
 *----------------------------------------------------------------------
 *
 * Raid_DisableLog --
 *
 * Results:
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */
void
Raid_DisableLog(raidPtr)
    Raid *raidPtr;
{
    raidPtr->log.enabled = 0;
}


/*
 *----------------------------------------------------------------------
 *
 * ProcessRaidLog --
 *
 * Results:
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */
typedef struct {
    Sync_Semaphore mutex;         /* Lock for synronizing updates of
                                   * this structure with the call back
                                   * function. */
    Sync_Condition wait;          /* Condition valued used to wait for
                                   * callback. */
    int            numIO;         /* Is the operation finished or not? */
    ReturnStatus   status;
} InitControlBlock;

static void initDoneProc();

static ReturnStatus
ProcessRaidLog(raidPtr)
    Raid	*raidPtr;
{
    InitControlBlock controlBlock;
    int		 stripeID;
    char	*statusCtrl = "sssssssssssssssssssssssssssssssssssssssssssss";

#ifdef TESTING
    Sync_CondInit(&controlBlock.wait);
#endif TESTING
    controlBlock.numIO = 0;
    controlBlock.numIO++;
    FOR_ALL_VEC(raidPtr->log.diskLockVec, stripeID, raidPtr->numStripe) {
	MASTER_LOCK(&controlBlock.mutex);
	controlBlock.numIO++;
	MASTER_UNLOCK(&controlBlock.mutex);
	Raid_InitiateHardInit(raidPtr, stripeID, 1,
		initDoneProc, (ClientData) &controlBlock, (int) &statusCtrl);
    }
    MASTER_LOCK(&controlBlock.mutex);
    controlBlock.numIO--;
    if (controlBlock.numIO == 0) {
        MASTER_UNLOCK(&controlBlock.mutex);
    } else {
	Sync_MasterWait(&controlBlock.wait, &controlBlock.mutex, FALSE);
        MASTER_UNLOCK(&controlBlock.mutex);
    }
    return controlBlock.status;
}


/*
 *----------------------------------------------------------------------
 *
 * initDoneProc --
 *
 *	Callback procedure used by Raid_ApplyLog.
 *      Is called after each parity reconstruction.
 *
 * Results:
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */
static void
initDoneProc(controlBlockPtr, status)
    InitControlBlock	*controlBlockPtr;
    ReturnStatus	 status;
{
    controlBlockPtr->status = status;
    MASTER_LOCK(&controlBlockPtr->mutex); 
    controlBlockPtr->numIO--; 
    if (controlBlockPtr->numIO == 0) { 
	Sync_MasterBroadcast(&controlBlockPtr->wait);
        MASTER_UNLOCK(&controlBlockPtr->mutex); 
    } else {
        MASTER_UNLOCK(&controlBlockPtr->mutex); 
    }
}


/*
 *----------------------------------------------------------------------
 *
 * Raid_ApplyLog --
 *
 * Results:
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Raid_ApplyLog(raidPtr)
    Raid *raidPtr;
{
    int				xferAmt;
    DevBlockDeviceRequest	req;
    ReturnStatus		status;

    req.operation	= FS_READ;
    req.startAddress	= VecLoc(raidPtr, 0);
    req.startAddrHigh	= 0;
    req.bufferLen	=
	VecNextLoc(raidPtr, raidPtr->log.diskLockVecNum) - VecLoc(raidPtr, 0);
    req.buffer		= (char *) raidPtr->log.diskLockVec;
    status = Dev_BlockDeviceIOSync(raidPtr->log.logHandlePtr, &req, &xferAmt);
    if (status != SUCCESS) {
	return status;
    }
    status = ProcessRaidLog(raidPtr);
    if (status != SUCCESS) {
	return status;
    }
    return SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * Raid_SaveDiskState --
 *	
 * Results:
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Raid_SaveDiskState(raidPtr, col, row, type, unit, version, numValidSector)
    Raid	*raidPtr;
    int		 col, row;
    int		 type, unit, version;
    int		 numValidSector;
    
{
    DevBlockDeviceRequest req;
    int xferAmt;
    int *iBuf = (int *) malloc(raidPtr->logHandlePtr->minTransferUnit);
    ReturnStatus	status;

    req.operation	= FS_WRITE;
    req.startAddress    = DiskLoc(raidPtr, col, row);
    req.startAddrHigh	= 0;
    req.buffer		= (char *) iBuf;
    req.bufferLen	= raidPtr->logHandlePtr->minTransferUnit;

    iBuf[0] = type;
    iBuf[1] = unit;
    iBuf[2] = version;
    iBuf[3] = numValidSector;
    status = Dev_BlockDeviceIOSync(raidPtr->logHandlePtr, &req, &xferAmt);
    if (status != SUCCESS) {
	return status;
    }
    free((char *) iBuf);
    return SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * Raid_SaveParam --
 *	
 * Results:
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Raid_SaveParam(raidPtr)
    Raid	*raidPtr;
{
    DevBlockDeviceRequest req;
    int xferAmt;
    int *iBuf = (int *) malloc(raidPtr->logHandlePtr->minTransferUnit);
    ReturnStatus	status;

    req.operation	= FS_WRITE;
    req.startAddress	= ConfigLoc(raidPtr);
    req.startAddrHigh	= 0;
    req.bufferLen	= raidPtr->logHandlePtr->minTransferUnit;
    req.buffer		= (char *) iBuf;
    MASTER_LOCK(&raidPtr->mutex);
    iBuf[0] = raidPtr->numRow;
    iBuf[1] = raidPtr->numCol;
    iBuf[2] = raidPtr->logBytesPerSector;
    iBuf[3] = raidPtr->sectorsPerStripeUnit;
    iBuf[4] = raidPtr->stripeUnitsPerDisk;
    iBuf[5] = raidPtr->rowsPerGroup;
    iBuf[6] = raidPtr->parityConfig;
    MASTER_UNLOCK(&raidPtr->mutex);
    status = Dev_BlockDeviceIOSync(raidPtr->logHandlePtr, &req, &xferAmt);
    if (status != SUCCESS) {
	return status;
    }
    free((char *) iBuf);
    return SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * Raid_SaveLog --
 *	
 * Results:
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Raid_SaveLog(raidPtr)
    Raid	*raidPtr;
{
    DevBlockDeviceRequest	req;
    int xferAmt;
    ReturnStatus	status;

    req.operation       = FS_WRITE;
    req.startAddress    = VecLoc(raidPtr, 0);
    req.startAddrHigh   = 0;
    req.bufferLen       =
	VecNextLoc(raidPtr, raidPtr->log.diskLockVecNum) - VecLoc(raidPtr, 0);
    req.buffer          = (char *) raidPtr->log.diskLockVec;

    status = Dev_BlockDeviceIOSync(raidPtr->logHandlePtr, &req, &xferAmt);
    if (status != SUCCESS) {
	return status;
    }
    return SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * Raid_SaveState --
 *	
 *	Perform a consistent checkpoint of the raid state.
 *      System must be queiesced.
 *
 * Results:
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Raid_SaveState(raidPtr)
    Raid *raidPtr;
{
    int		col, row;
    ReturnStatus status;
    RaidDisk	*diskPtr;

    status = Raid_SaveParam(raidPtr);
    if (status != SUCCESS) {
	return status;
    }
    for ( row = 0; row < raidPtr->numRow; row++ ) {
        for ( col = 0; col < raidPtr->numCol; col++ ) {
	    LockSema(&raidPtr->disk[col][row]->lock);
	    diskPtr = raidPtr->disk[col][row];
	    status = Raid_SaveDiskState(raidPtr, col, row, diskPtr->device.type,
		    diskPtr->device.unit, diskPtr->version,
		    diskPtr->numValidSector);
	    UnlockSema(&diskPtr->lock);
	    if (status != SUCCESS) {
		return status;
	    }
	}
    }
#ifndef TESTING
    ClearBitVec(raidPtr->log.diskLockVec, raidPtr->log.diskLockVecNum);
#endif TESTING
    status = Raid_SaveLog(raidPtr);
    if (status != SUCCESS) {
	return status;
    }
    return status;
}


/*
 *----------------------------------------------------------------------
 *
 * ComputeRaidParam --
 *
 *	Compute redundant but convenient information.
 *
 * Results:
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */
static void
ComputeRaidParam(raidPtr)
    Raid	*raidPtr;
{
    /*
     * Compute redundant but convenient information.
     */
    if (raidPtr->parityConfig == 'S') {
	raidPtr->numDataCol = raidPtr->numCol;
    } else {
	raidPtr->numDataCol = raidPtr->numCol - 1;
    }
    switch (raidPtr->parityConfig) {
    case 'X': case 'x': case 'f':
	raidPtr->stripeUnitsPerDisk -=
		raidPtr->stripeUnitsPerDisk % raidPtr->numCol;
        raidPtr->dataStripeUnitsPerDisk =
                (raidPtr->stripeUnitsPerDisk * raidPtr->numDataCol) /
		raidPtr->numCol;
	break;
    default:
	raidPtr->dataStripeUnitsPerDisk = raidPtr->stripeUnitsPerDisk;
	break;
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
}


/*
 *----------------------------------------------------------------------
 *
 * Raid_Configure --
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

/* static */ ReturnStatus
Raid_Configure(raidPtr, charBuf)
    Raid	*raidPtr;
    char	*charBuf;
{
    char	*charBufPtr;
    int		 col, row;
    int		 type, unit;
    int		 numScanned;
    RaidDisk    *diskPtr;

    charBufPtr = charBuf;
    /*
     * Skip comments.
     */
    for (;;) {
        if (!ScanLine(&charBufPtr, charBuf)) {
	    return FAILURE;
        }
        if (charBuf[0] != '#') {
            break;
        }
    }

    /*
     * Read dimensions of raid device.
     */
    numScanned = sscanf(charBuf, "%d %d %d %d %d %d %c %d %d %d",
 			&raidPtr->numRow,
			&raidPtr->numCol,
			&raidPtr->logBytesPerSector,
			&raidPtr->sectorsPerStripeUnit,
			&raidPtr->stripeUnitsPerDisk,
			&raidPtr->rowsPerGroup,
			&raidPtr->parityConfig,
			&raidPtr->logDev.type,
			&raidPtr->logDev.unit,
			&raidPtr->logDevOffset);
    if (numScanned != 10) {
	return FAILURE;
    }
    ComputeRaidParam(raidPtr);

    /*
     * Attach log device.
     */
    raidPtr->logHandlePtr = Dev_BlockDeviceAttach(&raidPtr->logDev);
    if (raidPtr->logHandlePtr == (DevBlockDeviceHandle *) NIL) {
	printf("RAID:ERR:Could not attach log device %d %d\n",
		raidPtr->logDev.type, raidPtr->logDev.unit);
	return FAILURE;
    }

    /*
     * Allocate RaidDisk structures; one for each logical disk.
     */
    raidPtr->disk = (RaidDisk ***)
		malloc((unsigned)raidPtr->numCol * sizeof(RaidDisk **));
    for ( col = 0; col < raidPtr->numCol; col++ ) {
	raidPtr->disk[col] = (RaidDisk **)
		malloc((unsigned)raidPtr->numRow * sizeof(RaidDisk *));
	bzero((char*)raidPtr->disk[col],raidPtr->numRow*sizeof(RaidDisk *));
    }

    /*
     * Initialize RaidDisk structures.
     */
    for ( row = 0; row < raidPtr->numRow; row++ ) {
        for ( col = 0; col < raidPtr->numCol; col++ ) {
	    if (!ScanWord(&charBufPtr, charBuf)) {
		return FAILURE;
	    }
	    type = atoi(charBuf);
	    if (!ScanWord(&charBufPtr, charBuf)) {
		return FAILURE;
	    }
	    unit = atoi(charBuf);
	    diskPtr = Raid_MakeDisk(col, row, type, unit, 1,
		    raidPtr->sectorsPerDisk);
	    if (diskPtr == (RaidDisk *) NIL) {
		printf("Could not attach disk %d %d\n", type, unit);
		return FAILURE;
	    }
	    raidPtr->disk[col][row] = diskPtr;
	}
    }

    Raid_InitLog(raidPtr);

    raidPtr->state = RAID_VALID;
    return SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * Raid_RestoreState --
 *
 * Results:
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Raid_RestoreState(raidPtr)
    Raid *raidPtr;
{
    DevBlockDeviceRequest	req;
    int *iBuf = (int *) malloc(raidPtr->logHandlePtr->minTransferUnit);
    int xferAmt;
    ReturnStatus	status;
    int	col, row;
    RaidDisk		*diskPtr;

    req.operation	= FS_READ;
    req.startAddress	= ConfigLoc(raidPtr);
    req.startAddrHigh	= 0;
    req.bufferLen	= raidPtr->logHandlePtr->minTransferUnit;
    req.buffer		= (char *) iBuf;
    status = Dev_BlockDeviceIOSync(raidPtr->logHandlePtr, &req, &xferAmt);
    if (status != SUCCESS) {
	return FAILURE;
    }
    raidPtr->numRow			= iBuf[0];
    raidPtr->numCol			= iBuf[1];
    raidPtr->logBytesPerSector		= iBuf[2];
    raidPtr->sectorsPerStripeUnit	= iBuf[3];
    raidPtr->stripeUnitsPerDisk		= iBuf[4];
    raidPtr->rowsPerGroup		= iBuf[5];
    raidPtr->parityConfig		= iBuf[6];
    ComputeRaidParam(raidPtr);

    /*
     * Allocate RaidDisk structures; one for each logical disk.
     */
    raidPtr->disk = (RaidDisk ***)
		malloc((unsigned)raidPtr->numCol * sizeof(RaidDisk **));
    for ( col = 0; col < raidPtr->numCol; col++ ) {
	raidPtr->disk[col] = (RaidDisk **)
		malloc((unsigned)raidPtr->numRow * sizeof(RaidDisk *));
	bzero((char*)raidPtr->disk[col],raidPtr->numRow*sizeof(RaidDisk *));
    }

    /*
     * Initialize RaidDisk structures.
     */
    req.operation	= FS_READ;
    req.startAddrHigh	= 0;
    req.buffer		= (char *) iBuf;
    req.bufferLen	= raidPtr->logHandlePtr->minTransferUnit;
    for ( row = 0; row < raidPtr->numRow; row++ ) {
        for ( col = 0; col < raidPtr->numCol; col++ ) {
	req.startAddress = DiskLoc(raidPtr, col, row);
	status = Dev_BlockDeviceIOSync(raidPtr->logHandlePtr, &req, &xferAmt);
	if (status != SUCCESS) {
	    return FAILURE;
	}
	diskPtr = Raid_MakeDisk(col,row, iBuf[0], iBuf[1], iBuf[2], iBuf[3]);
	if (diskPtr == (RaidDisk *) NIL) {
	    return FAILURE;
	}
	raidPtr->disk[col][row] = diskPtr;
	}
    }

    Raid_InitLog(raidPtr);
    if (Raid_ApplyLog(raidPtr) != SUCCESS) {
	return FAILURE;
    }
    raidPtr->state = RAID_VALID;
    return SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * Raid_MasterFlushLog --
 *
 *	Flush log to disk.
 *
 * Results:
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */

void
Raid_MasterFlushLog(raidPtr)
    Raid *raidPtr;
{
    if (raidPtr->log.busy) {
	Sync_MasterWait(raidPtr->log.waitNextBufPtr, &raidPtr->log.mutex,FALSE);
	return;
    }
    raidPtr->log.busy = 1;
    
    do {
	DevBlockDeviceRequest	req;
	Sync_Condition		*flushedPtr;
	int xferAmt;

	req.operation       = FS_WRITE;
	req.startAddress    =
		 VecLoc(raidPtr, raidPtr->log.minLogElem);
	req.startAddrHigh   = 0;
	req.bufferLen       = VecNextLoc(raidPtr, raidPtr->log.maxLogElem) - 
		VecLoc(raidPtr, raidPtr->log.minLogElem);
	req.buffer          = (char *) raidPtr->log.diskLockVec +
		VecOffset(raidPtr, raidPtr->log.minLogElem);

	flushedPtr = raidPtr->log.waitNextBufPtr;
	raidPtr->log.waitNextBufPtr = raidPtr->log.waitCurBufPtr;
	raidPtr->log.waitCurBufPtr = flushedPtr;
	raidPtr->log.minLogElem = raidPtr->log.diskLockVecNum;
	raidPtr->log.maxLogElem = 0;

	MASTER_UNLOCK(&raidPtr->log.mutex);
	while(Dev_BlockDeviceIOSync(raidPtr->logHandlePtr, &req, &xferAmt) !=
		SUCCESS) {
	    Time time;
	    printf("Error writing log\n");
	    time.seconds = 10;
	    time.microseconds = 0;
	    Sync_WaitTime(time);
	}
	MASTER_LOCK(&raidPtr->log.mutex);
	Sync_MasterBroadcast(raidPtr->log.waitCurBufPtr);
    } while (raidPtr->log.minLogElem != raidPtr->log.diskLockVecNum);
    raidPtr->log.busy = 0;
}


/*
 *----------------------------------------------------------------------
 *
 * Raid_LogStripe --
 *
 *	Make an entry in the specified log.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */
void
Raid_CheckPoint(raidPtr)
    Raid	*raidPtr;
{
    MASTER_LOCK(&raidPtr->log.mutex);
    if (!raidPtr->log.enabled) {
	MASTER_UNLOCK(&raidPtr->log.mutex);
	return;
    }
    MASTER_UNLOCK(&raidPtr->log.mutex);
    printf("RAID:MSG:Checkpointing RAID\n");
    Raid_Lock(raidPtr);
#ifndef TESTING
    ClearBitVec(raidPtr->log.diskLockVec, raidPtr->log.diskLockVecNum);
#endif TESTING
    Raid_SaveLog(raidPtr);
    Raid_Unlock(raidPtr);
    printf("RAID:MSG:Checkpoint Complete\n");
}

#ifdef TESTING
#define NUM_LOG_STRIPE 2
#else
#define NUM_LOG_STRIPE 100
#endif

void
Raid_LogStripe(raidPtr, stripeID)
    Raid *raidPtr;
    int	  stripeID;
{
    if (IsSet(raidPtr->log.diskLockVec, stripeID)) {
	return;
    }

    /*
     * Occasionally checkpoint log.
     */
    MASTER_LOCK(&raidPtr->mutex);
    raidPtr->numStripeLocked++;
    if (raidPtr->numStripeLocked % NUM_LOG_STRIPE == 0) {
	MASTER_UNLOCK(&raidPtr->mutex);
	Proc_CallFunc((void (*)
		_ARGS_((ClientData clientData, Proc_CallInfo *callInfoPtr)))
		Raid_CheckPoint, (ClientData) raidPtr, 0);
    } else {
	MASTER_UNLOCK(&raidPtr->mutex);
    }

    /*
     * Write log.
     */
    MASTER_LOCK(&raidPtr->log.mutex);
    if (!raidPtr->log.enabled) {
	MASTER_UNLOCK(&raidPtr->log.mutex);
	return;
    }
    SetBit(raidPtr->log.diskLockVec, stripeID);
    if (stripeID < raidPtr->log.minLogElem) {
	raidPtr->log.minLogElem = stripeID;
    }
    if (stripeID > raidPtr->log.maxLogElem) {
	raidPtr->log.maxLogElem = stripeID;
    }
    Raid_MasterFlushLog(raidPtr);
    MASTER_UNLOCK(&raidPtr->log.mutex);
}

