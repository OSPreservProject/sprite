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
#include "bit.h"
#include "miscutil.h"
#include "devRaidProto.h"

/*
 * Forward declaration.
 */
void Raid_UpdateLog();
void Raid_FlushLog();

/*
 *----------------------------------------------------------------------
 *
 * Raid_AttachLogDevice --
 *
 * Results:
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Raid_AttachLogDevice(raidPtr, type, unit, offset)
    Raid		*raidPtr;
    int			type, unit, offset;
{
    RaidLog		*logPtr = &raidPtr->log;

    logPtr->logDev.type = type;
    logPtr->logDev.unit = unit;
    logPtr->logDevOffset = offset;
    logPtr->logHandlePtr = Dev_BlockDeviceAttach(&logPtr->logDev);
    if (logPtr->logHandlePtr == (DevBlockDeviceHandle *) NIL) {
	return FAILURE;
    }
    return SUCCESS;
}

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
    Raid		*raidPtr;
{
    RaidLog		*logPtr = &raidPtr->log;

    Sync_SemInitDynamic(&logPtr->mutex, "RAID log mutex");
    logPtr->enabled = 0;
    logPtr->busy = 0;
    logPtr->diskLockVec = (int *) malloc(VecSize(raidPtr));
    bzero((char *) logPtr->diskLockVec, VecSize(raidPtr));
    logPtr->lockVec = (int *) malloc(VecSize(raidPtr));
    bzero((char *) logPtr->lockVec, VecSize(raidPtr));
    logPtr->numStripeLocked = 0;
    bzero((char *) logPtr->lockVec, VecSize(raidPtr));
    logPtr->logDevEndOffset = logPtr->logDevOffset + LogSize(raidPtr);
#ifdef TESTING
    Sync_CondInit(&logPtr->flushed);
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
 * Raid_SaveParam --
 *	
 * Results:
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
Raid_SaveParam(raidPtr)
    Raid	*raidPtr;
{
    ReturnStatus status;
    status = Raid_DevWriteInt(raidPtr->log.logHandlePtr, ParamLoc(raidPtr),
	    6, raidPtr->numRow, raidPtr->numCol, raidPtr->logBytesPerSector,
	    raidPtr->sectorsPerStripeUnit, raidPtr->stripeUnitsPerDisk,
	    (int) raidPtr->parityConfig);
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
    raidPtr->bytesPerSector = 1 << raidPtr->logBytesPerSector;
}

/*
 *----------------------------------------------------------------------
 *
 * Raid_RestoreParam --
 *	
 * Results:
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
Raid_RestoreParam(raidPtr)
    Raid	*raidPtr;
{
    int		parityConfig;
    ReturnStatus status;

    status = Raid_DevReadInt(raidPtr->log.logHandlePtr, ParamLoc(raidPtr),
	    6, &raidPtr->numRow, &raidPtr->numCol, &raidPtr->logBytesPerSector,
	    &raidPtr->sectorsPerStripeUnit, &raidPtr->stripeUnitsPerDisk,
	    &parityConfig);
    raidPtr->rowsPerGroup = raidPtr->numRow;
    raidPtr->parityConfig = parityConfig;
    ComputeRaidParam(raidPtr);
    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * Raid_SaveDisk --
 *	
 * Results:
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Raid_SaveDisk(raidPtr, col, row, type, unit, version, numValidSector)
    Raid	*raidPtr;
    int		col, row;
    int		type, unit, version, numValidSector;
{
    ReturnStatus status;

    status = Raid_DevWriteInt(raidPtr->log.logHandlePtr,
	    DiskLoc(raidPtr, col, row),
	    4, type, unit, version, numValidSector);
    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * Raid_RestoreDisk --
 *	
 * Results:
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
Raid_RestoreDisk(raidPtr, col, row)
    Raid	*raidPtr;
    int		col, row;
{
    ReturnStatus status;
    int		 type, unit, version, numValidSector;

    status = Raid_DevReadInt(raidPtr->log.logHandlePtr,DiskLoc(raidPtr,col,row),
	    4, &type, &unit, &version, &numValidSector);
    if (status != SUCCESS) {
	return status;
    }
    raidPtr->disk[col][row] =
	    Raid_MakeDisk(col,row, type, unit, version, numValidSector);
    if (raidPtr->disk[col][row] == (RaidDisk *) NIL) {
	return FAILURE;
    }
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
    ReturnStatus	status;

    status = Raid_DevWriteSync(raidPtr->log.logHandlePtr, VecLoc(raidPtr),
    	    (char *) raidPtr->log.diskLockVec, VecSize(raidPtr));
    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * Raid_RestoreLog --
 *	
 * Results:
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Raid_RestoreLog(raidPtr)
    Raid	*raidPtr;
{
    ReturnStatus	status;

    status = Raid_DevReadSync(raidPtr->log.logHandlePtr, VecLoc(raidPtr),
    	    (char *) raidPtr->log.diskLockVec, VecSize(raidPtr));
    return status;
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
    Raid	*raidPtr;
{
    SyncControl		syncControl;
    ReturnStatus	status;
    int			stripeID;

    Raid_InitSyncControl(&syncControl);
    for (stripeID = 0; stripeID < raidPtr->numStripe; stripeID++) {
	if (Bit_IsClear(stripeID, raidPtr->log.diskLockVec)) {
	    continue;
	}
	Raid_StartSyncIO(&syncControl);
	Raid_InitiateHardInit(raidPtr, stripeID, 1,
		(void (*)()) Raid_SyncDoneProc, (ClientData) &syncControl,NULL);
	status = Raid_WaitSyncIO(&syncControl);
	if (status != SUCCESS) {
	    return status;
	}
    }
    return SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * Raid_SaveState --
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
    int			col, row;
    RaidDisk		*diskPtr;
    ReturnStatus	status;

    status = Raid_SaveParam(raidPtr);
    if (status != SUCCESS) {
	return status;
    }
    for ( row = 0; row < raidPtr->numRow; row++ ) {
        for ( col = 0; col < raidPtr->numCol; col++ ) {
	    diskPtr = raidPtr->disk[col][row];
	    status = Raid_SaveDisk(raidPtr, col, row,
		    diskPtr->device.type, diskPtr->device.unit,
		    diskPtr->version, diskPtr->numValidSector);
	    if (status != SUCCESS) {
		return status;
	    }
	}
    }
    MASTER_LOCK(&raidPtr->log.mutex);
    bcopy((char *) raidPtr->log.lockVec, (char *) raidPtr->log.diskLockVec,
	    VecSize(raidPtr));
    MASTER_UNLOCK(&raidPtr->log.mutex);
    status = Raid_SaveLog(raidPtr);
    return status;
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
Raid_RestoreState(raidPtr, type, unit, offset)
    Raid		*raidPtr;
    int			type, unit, offset;
{
    int			col, row;
    ReturnStatus	status;

    status = Raid_AttachLogDevice(raidPtr, type, unit, offset);
    if (status != SUCCESS) {
	return status;
    }
    status = Raid_RestoreParam(raidPtr);
    if (status != SUCCESS) {
	return status;
    }
    /*
     * Restore disk state and attach disks.
     */
    raidPtr->disk = (RaidDisk ***)
		malloc((unsigned)raidPtr->numCol * sizeof(RaidDisk **));
    for ( col = 0; col < raidPtr->numCol; col++ ) {
	raidPtr->disk[col] = (RaidDisk **)
		malloc((unsigned)raidPtr->numRow * sizeof(RaidDisk *));
	bzero((char*)raidPtr->disk[col],raidPtr->numRow*sizeof(RaidDisk *));
    }
    for ( row = 0; row < raidPtr->numRow; row++ ) {
        for ( col = 0; col < raidPtr->numCol; col++ ) {
	    status = Raid_RestoreDisk(raidPtr, col, row);
	    if (status != SUCCESS) {
		return status;
	    }
	}
    }
    /*
     * Restore and apply log.
     */
    Raid_InitLog(raidPtr);
    status = Raid_RestoreLog(raidPtr);
    if (status != SUCCESS) {
	return status;
    }
    status = Raid_ApplyLog(raidPtr);
    if (status != SUCCESS) {
	return status;
    }

    raidPtr->state = RAID_VALID;
    return SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * Raid_Configure --
 *
 *	Configure raid device from configuration buffer.
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
    Raid		*raidPtr;
    char		*charBuf;
{
    char		*charBufPtr;
    int			col, row;
    int			type, unit;
    int			logType, logUnit, logOffset;
    int			numScanned;
    RaidDisk		*diskPtr;
    ReturnStatus	status;

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
    numScanned = sscanf(charBuf, "%d %d %d %d %d %c %d %d %d",
 			&raidPtr->numRow,
			&raidPtr->numCol,
			&raidPtr->logBytesPerSector,
			&raidPtr->sectorsPerStripeUnit,
			&raidPtr->stripeUnitsPerDisk,
			&raidPtr->parityConfig,
			&logType,
			&logUnit,
			&logOffset);
    raidPtr->rowsPerGroup = raidPtr->numRow;
    if (numScanned != 9) {
	return FAILURE;
    }
    ComputeRaidParam(raidPtr);

    /*
     * Attach log device.
     */
    if (raidPtr->parityConfig != 'S') {
	status = Raid_AttachLogDevice(raidPtr, logType, logUnit, logOffset);
	if (status != SUCCESS) {
	    return status;
	}
	Raid_InitLog(raidPtr);
    }

    /*
     * Attach RaidDisk's.
     */
    raidPtr->disk = (RaidDisk ***)
		malloc((unsigned)raidPtr->numCol * sizeof(RaidDisk **));
    for ( col = 0; col < raidPtr->numCol; col++ ) {
	raidPtr->disk[col] = (RaidDisk **)
		malloc((unsigned)raidPtr->numRow * sizeof(RaidDisk *));
	bzero((char*)raidPtr->disk[col],raidPtr->numRow*sizeof(RaidDisk *));
    }
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

    raidPtr->state = RAID_VALID;
    return SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * Raid_FlushLog --
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
Raid_FlushLog(raidPtr)
    Raid *raidPtr;
{
    MASTER_LOCK(&raidPtr->log.mutex);
    if (!raidPtr->log.enabled) {
	MASTER_UNLOCK(&raidPtr->log.mutex);
	return;
    }
    if (raidPtr->log.busy) {
	/*
	 * Wait for log flush in operation to finish.
	 */
	Sync_MasterWait(&raidPtr->log.flushed, &raidPtr->log.mutex, FALSE);
	/*
	 * If someone else is flushing my log, we just need to wait for it
	 * to finish.  Otherwise, I'll have to flush it myself.
	 */
	if (raidPtr->log.busy) {
	    Sync_MasterWait(&raidPtr->log.flushed, &raidPtr->log.mutex, FALSE);
	    MASTER_UNLOCK(&raidPtr->log.mutex);
	    return;
	}
    }
    raidPtr->log.busy = 1;
    MASTER_UNLOCK(&raidPtr->log.mutex);

    while (Raid_SaveLog(raidPtr) != SUCCESS) {
	printf("Error writing log\n");
	Raid_WaitTime(10000);
    }

    MASTER_LOCK(&raidPtr->log.mutex);
    raidPtr->log.busy = 0;
    Sync_MasterBroadcast(&raidPtr->log.flushed);
    MASTER_UNLOCK(&raidPtr->log.mutex);
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
#ifdef TESTING
#define NUM_LOG_STRIPE 2
#else
#define NUM_LOG_STRIPE 100
#endif

void
Raid_LogStripe(raidPtr, stripeID)
    Raid		*raidPtr;
    int			stripeID;
{
    MASTER_LOCK(&raidPtr->log.mutex);
    Bit_Set(stripeID, raidPtr->log.lockVec);
    if (Bit_IsSet(stripeID, raidPtr->log.diskLockVec)) {
	MASTER_UNLOCK(&raidPtr->log.mutex);
	return;
    }
    Bit_Set(stripeID, raidPtr->log.diskLockVec);
    /*
     * Occasionally update log.
     */
    raidPtr->log.numStripeLocked++;
    if (raidPtr->log.numStripeLocked % NUM_LOG_STRIPE == 0) {
	bcopy((char *) raidPtr->log.lockVec, (char *) raidPtr->log.diskLockVec,
		VecSize(raidPtr));
    }
    MASTER_UNLOCK(&raidPtr->log.mutex);

    Raid_FlushLog(raidPtr);
}

void
Raid_UnlogStripe(raidPtr, stripeID)
    Raid		*raidPtr;
    int			stripeID;
{
    MASTER_LOCK(&raidPtr->log.mutex);
    Bit_Clear(stripeID, raidPtr->log.lockVec);
    MASTER_UNLOCK(&raidPtr->log.mutex);
}
