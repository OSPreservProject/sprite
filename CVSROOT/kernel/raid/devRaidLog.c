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
#include <sys/file.h>
#include <stdio.h>
#include "sprite.h"
#include "devRaid.h"
#include "devRaidDisk.h"
#include "devRaidLog.h"
#include "bitvec.h"


/*
 *----------------------------------------------------------------------
 *
 * InitRaidLog --
 *
 * Results:
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
InitRaidLog(raidPtr)
    Raid *raidPtr;
{
    char fileName[80];

    raidPtr->log.enabled = 1;
    raidPtr->log.busy = 0;
#ifdef TESTING
    Sync_CondInit(&raidPtr->log.notBusy);
#endif TESTING
    sprintf(fileName, "%s%d%s", RAID_ROOT_CONFIG_FILE_NAME,
	    raidPtr->devicePtr->unit, ".log");
    if ((raidPtr->log.streamPtr = (Fs_Stream *) open(fileName,
	    O_WRONLY | O_APPEND | O_CREAT, 0666)) == (Fs_Stream *) -1) {
	return FAILURE;
    }
    raidPtr->log.curBufPtr = raidPtr->log.buf1;
    raidPtr->log.curBuf = raidPtr->log.buf1;
    raidPtr->log.curBufFlushedPtr = &raidPtr->log.flushed1;
#ifdef TESTING
    Sync_CondInit(&raidPtr->log.flushed1);
    Sync_CondInit(&raidPtr->log.flushed2);
#endif TESTING
    return SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * EnableLog --
 *
 * Results:
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */
void
EnableLog(raidPtr)
    Raid *raidPtr;
{
    raidPtr->log.enabled = 1;
}


/*
 *----------------------------------------------------------------------
 *
 * DisableLog --
 *
 * Results:
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */
void
DisableLog(raidPtr)
    Raid *raidPtr;
{
    raidPtr->log.enabled = 0;
}


/*
 *----------------------------------------------------------------------
 *
 * ApplyRaidLog --
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
    int       numIO;           /* Is the operation finished or not? */
} InitControlBlock;

static void initDoneProc();

ReturnStatus
ApplyRaidLog(raidPtr, fileName)
    Raid *raidPtr;
    char *fileName;
{
#   define 	 CHAR_BUF_LEN	120
    char	 buf[CHAR_BUF_LEN];
    RaidDisk    *diskPtr;
    int		 row, col, version, type, unit, numValidSector;
    RaidDiskState state;
    int		 stripeID;
    FILE *fp;
    BitVec	 lockedVec = MakeBitVec(raidPtr->numStripe);
    InitControlBlock controlBlock;
    char        *statusBuf = "ssssssssssssssssssssssssssssssssssssssssssssssss";
    char       **ctrlData = &statusBuf;

    if ((fp = fopen(fileName, "r")) == NULL) {
	return FAILURE;
    }
    while (fgets(buf, 120, fp) != NULL) {
	switch (buf[0]) {
	case 'D':
	    if (sscanf(buf, "%*c %d %d %d  %d %d  %d %d\n",&row, &col, &version,
		    &type, &unit, &state, &numValidSector) != 7) {
		return FAILURE;    
	    }
	    diskPtr = raidPtr->disk[col][row];
	    diskPtr->version = version;
	    diskPtr->numValidSector = numValidSector;
	    if (diskPtr->state != RAID_DISK_INVALID) {
		diskPtr->state = state;
	    }
	    break;
	case 'F':
	    if (sscanf(buf, "%*c %d %d %d\n", &row, &col, &version) != 3) {
		return FAILURE;    
	    }
	    FailRaidDisk(raidPtr, col, row, version);
	    break;
	case 'R':
	    if (sscanf(buf, "%*c %d %d %d  %d %d\n", &row, &col, &version,
		    &type, &unit) != 5) {
		return FAILURE;    
	    }
	    ReplaceRaidDisk(raidPtr, col, row, version, type, unit, 0);
	    break;
	case 'L':
	    if (sscanf(buf, "%*c %d\n", &stripeID) != 1) {
		return FAILURE;    
	    }
	    SetBit(lockedVec, stripeID);
	    break;
	case 'U':
	    if (sscanf(buf, "%*c %d\n", &stripeID) != 1) {
		return FAILURE;    
	    }
	    ClrBit(lockedVec, stripeID);
	    break;
	case '#': case ' ': case '\t': case '\n': /* comment */
	    break;
	default:
	    printf("RAID:MSG:Unknown log entry '%s'.\n", buf);
	    break;
	}
    }
    fclose(fp);

#ifdef TESTING
    Sync_CondInit(&controlBlock.wait);
#endif TESTING
    controlBlock.numIO = 0;
    controlBlock.numIO++;
    FOR_ALL_VEC(lockedVec, stripeID, raidPtr->numStripe) {
	MASTER_LOCK(&controlBlock.mutex);
	controlBlock.numIO++;
	MASTER_UNLOCK(&controlBlock.mutex);
	InitiateHardInit(raidPtr, stripeID, 1, initDoneProc,
		(ClientData) &controlBlock,
	   (int) ctrlData);
    }
    MASTER_LOCK(&controlBlock.mutex);
    controlBlock.numIO--;
    if (controlBlock.numIO == 0) {
        MASTER_UNLOCK(&controlBlock.mutex);
    } else {
	Sync_MasterWait(&controlBlock.wait, &controlBlock.mutex, FALSE);
        MASTER_UNLOCK(&controlBlock.mutex);
    }
    free(lockedVec);
    return SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * initDoneProc --
 *
 *	Callback procedure used by ApplyRaidLog.
 *      Is called after each parity reconstruction.
 *
 * Results:
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */
static void
initDoneProc(controlBlockPtr)
    InitControlBlock         *controlBlockPtr;
{
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
 * SaveRaidState --
 *	
 *	Perform a consistent checkpoint of the raid state.
 *      System must first be quiesced.  (lock-raid)
 *
 * Results:
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
SaveRaidState(raidPtr)
    Raid *raidPtr;
{
    char fileName[80], fileName2[80];
    RaidDisk *diskPtr;
    FILE *fp;
    Fs_Stream *streamPtr;
    int col, row;

    /*
     * Save configuration.
     */
    sprintf(fileName, "%s%d%s", RAID_ROOT_CONFIG_FILE_NAME,
	    raidPtr->devicePtr->unit, ".new.state");
    if ((fp = fopen(fileName, "w")) == NULL) {
	return FAILURE;
    }
    if (fprintf(fp, "%s\n",
	    "#row col logSectSize sectPerSU SUPerDisk rowsPerGroup pConfig") ==
	    -1) {
	fclose(fp);
	return FAILURE;
    }
    if (fprintf(fp, "%d %d %d %d %d %d %c\n",
 			raidPtr->numRow,
			raidPtr->numCol,
			raidPtr->logBytesPerSector,
			raidPtr->sectorsPerStripeUnit,
			raidPtr->stripeUnitsPerDisk,
			raidPtr->rowsPerGroup,
			raidPtr->parityConfig) == -1) {
	fclose(fp);
	return FAILURE;
    }
    for ( row = 0; row < raidPtr->numRow; row++ ) {
	for ( col = 0; col < raidPtr->numCol; col++ ) {
	    diskPtr = raidPtr->disk[col][row];
	    if (fprintf(fp, "%d %d	",
		    diskPtr->device.type,
		    diskPtr->device.unit) == -1) {
		fclose(fp);
		return FAILURE;
	    }
	}
	fprintf(fp, "\n");
    }
    if (fclose(fp) == EOF) {
	return FAILURE;
    }
    sprintf(fileName2, "%s%d%s", RAID_ROOT_CONFIG_FILE_NAME,
	    raidPtr->devicePtr->unit, ".state");
    if (rename(fileName, fileName2) == -1) {
	return FAILURE;
    }

    /*
     * Save rest of state in log.
     */
    sprintf(fileName, "%s%d%s", RAID_ROOT_CONFIG_FILE_NAME,
	    raidPtr->devicePtr->unit, ".new.log");
    if ((fp = fopen(fileName, "w")) == NULL) {
	return FAILURE;
    }
    for ( col = 0; col < raidPtr->numCol; col++ ) {
	for ( row = 0; row < raidPtr->numRow; row++ ) {
	    diskPtr = raidPtr->disk[col][row];
	    if (fprintf(fp, "D %d %d %d  %d %d  %d %d\n", row, col,
		    diskPtr->version,
		    diskPtr->device.type, diskPtr->device.unit,
		    diskPtr->state, diskPtr->numValidSector) == -1) {
		fclose(fp);
		return FAILURE;
	    }
	}
    }
    if (fclose(fp) == EOF) {
	return FAILURE;
    }
    if ((streamPtr = (Fs_Stream *) open(fileName, O_WRONLY | O_APPEND, 0666))== 
	    (Fs_Stream *)-1) {
	return FAILURE;
    }
    sprintf(fileName2, "%s%d%s", RAID_ROOT_CONFIG_FILE_NAME,
	    raidPtr->devicePtr->unit, ".log");
    if (rename(fileName, fileName2) == -1) {
	close(streamPtr);
	return FAILURE;
    }
    close(raidPtr->log.streamPtr);
    raidPtr->log.streamPtr = streamPtr;
    return SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * RestoreRaidState --
 *
 *	May only be called from devRaidAttach.
 *	Restore state of raid device by looking for a state file and applying
 *	appropriate logs.
 *	If no state files exist, read config file, lock-raid and exit; if
 *	you are creating a new raid device, you should then perform the
 *	following sequence:  disable-log, hard-init, enable-log, save-state,
 *	unlock-raid.
 *
 * Results:
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
RestoreRaidState(raidPtr)
    Raid *raidPtr;
{
    char fileName[80];
    ReturnStatus status;

    sprintf(fileName, "%s%d%s", RAID_ROOT_CONFIG_FILE_NAME,
	    raidPtr->devicePtr->unit, ".state");
    if (RaidConfigure(raidPtr, fileName) == SUCCESS) {
	sprintf(fileName, "%s%d%s", RAID_ROOT_CONFIG_FILE_NAME,
		raidPtr->devicePtr->unit, ".log");
	printf("RAID:MSG:State restored, applying log.\n");
	if (ApplyRaidLog(raidPtr, fileName) == FAILURE) {
	    printf("RAID:MSG:Log failed.\n");
	    return FAILURE;
	}
	printf("RAID:MSG:Log applied.\n");
	if (InitRaidLog(raidPtr) == FAILURE) {
	    printf("RAID:MSG:Could not open log.\n");
	    return FAILURE;
	}
#ifndef TESTING
	/*
	 * Save new state.
	 */
        if (SaveRaidState(raidPtr) == FAILURE) {
            printf("RAID:MSG:Could not checkpoint state.\n");
        }
#endif
	return SUCCESS;
    }
    printf("RAID:MSG:Invalid state, reading configuration.\n");
    sprintf(fileName, "%s%d%s", RAID_ROOT_CONFIG_FILE_NAME,
	    raidPtr->devicePtr->unit, ".config");
    if (RaidConfigure(raidPtr, fileName) == FAILURE) {
	printf("RAID:MSG:Raid configuration failed.\n");
	return FAILURE;
    }
    printf("RAID:MSG:Configuration completed.\n");
    if (InitRaidLog(raidPtr) == FAILURE) {
	printf("RAID:MSG:Could not open log.\n");
	return FAILURE;
    }
    if (raidPtr->parityConfig != 'S') {
	LockRaid(raidPtr);
    }
    return SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * MasterFlushLog --
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
MasterFlushLog(raidPtr)
    Raid *raidPtr;
{
    if (raidPtr->log.busy) {
	Sync_MasterWait(raidPtr->log.curBufFlushedPtr, &raidPtr->log.mutex,
		FALSE);
	return;
    }
    raidPtr->log.busy = 1;
    do {
	char *buf = raidPtr->log.curBuf;
	int   size = raidPtr->log.curBufPtr - raidPtr->log.curBuf;
	Sync_Condition *flushed = raidPtr->log.curBufFlushedPtr;

	if (raidPtr->log.curBuf == raidPtr->log.buf1) {
	    raidPtr->log.curBuf = raidPtr->log.buf2;
	    raidPtr->log.curBufPtr = raidPtr->log.buf2;
	    raidPtr->log.curBufFlushedPtr = &raidPtr->log.flushed2;
	} else {
	    raidPtr->log.curBuf = raidPtr->log.buf1;
	    raidPtr->log.curBufPtr = raidPtr->log.buf1;
	    raidPtr->log.curBufFlushedPtr = &raidPtr->log.flushed1;
	}
	MASTER_UNLOCK(&raidPtr->log.mutex);
	while (write(raidPtr->log.streamPtr, buf, size) == -1) {
	    Time time;
	    perror("Error writing log");
	    time.seconds = 10;
	    time.microseconds = 0;
	    Sync_WaitTime(time);
	}
	MASTER_LOCK(&raidPtr->log.mutex);
	Sync_MasterBroadcast(flushed);
    } while (raidPtr->log.curBufPtr != raidPtr->log.curBuf);
    raidPtr->log.busy = 0;
    Sync_MasterBroadcast(&raidPtr->log.notBusy);
}


/*
 *----------------------------------------------------------------------
 *
 * LogEntry --
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
LogEntry(raidPtr, msg)
    Raid *raidPtr;
    char *msg;
{
    int n = strlen(msg);
    int i;

    MASTER_LOCK(&raidPtr->log.mutex);
    if (!raidPtr->log.enabled) {
	MASTER_UNLOCK(&raidPtr->log.mutex);
	return;
    }
    while (raidPtr->log.curBufPtr+n > raidPtr->log.curBuf + RAID_LOG_BUF_SIZE) {
	MasterFlushLog(raidPtr);
    }
    for (i = 0; i < n; i++) {
	*raidPtr->log.curBufPtr = msg[i];
	raidPtr->log.curBufPtr++;
    }
    /* Don't have to wait for log if *unlocking* a stripe. */
    if (msg[0] == 'U') {
	MASTER_UNLOCK(&raidPtr->log.mutex);
	return;
    }
    MasterFlushLog(raidPtr);
    MASTER_UNLOCK(&raidPtr->log.mutex);
}


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

    if (raidPtr->state == RAID_VALID) {
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
    }
    raidPtr->state = RAID_INVALID;
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

/* static */ ReturnStatus
RaidConfigure(raidPtr, fileName)
    Raid	*raidPtr;
    char	*fileName;
{
#   define 	 CHAR_BUF_LEN	120
    char	 charBuf[CHAR_BUF_LEN];
    int		 col, row;
    int		 type, unit;
    int		 numScanned;
    RaidDisk    *raidDiskPtr;
    FILE	*fp;


    /*
     * If RAID device is already configured, deallocate it first.
     */
    RaidDeallocate(raidPtr);

    raidPtr->numReqInSys = 0;
#ifdef TESTING
    Sync_CondInit(&raidPtr->waitExclusive);
    Sync_CondInit(&raidPtr->waitNonExclusive);
#endif TESTING

    if ((fp = fopen(fileName, "r")) == NULL) {
	return FAILURE;
    }

    /*
     * Skip comments.
     */
    for (;;) {
        if (fgets(charBuf, CHAR_BUF_LEN, fp) == NULL) {
	    fclose(fp);
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
	fclose(fp);
	return FAILURE;
    }

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
	    if (fscanf(fp, "%d %d", &type, &unit) != 2) {
		fclose(fp);
		return FAILURE;
	    }
	    raidDiskPtr = MakeRaidDisk(col, row, type, unit,
		    raidPtr->sectorsPerDisk);
	    if (raidDiskPtr == (RaidDisk *) NIL) {
		RaidDeallocate(raidPtr);
		fclose(fp);
		return FAILURE;
	    }
	    raidPtr->disk[col][row] = raidDiskPtr;
	}
    }
    raidPtr->state = RAID_VALID;
    return SUCCESS;
}
