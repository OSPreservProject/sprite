/* 
 * devPrint.c --
 *
 *	Routines for printing out various RAID related data structures.
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
#include <sprite.h>
#include <stdio.h>
#include <assert.h>
#include "fs.h"
#include "devBlockDevice.h"
#include "devRaid.h"
#include "devRaidUtil.h"
#include "schedule.h"


/*
 *----------------------------------------------------------------------
 *
 * PrintRequests --
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Prints to stdout.
 *
 *----------------------------------------------------------------------
 */

#define MAX_ROW		2
#define MAX_COL		4
#define MAX_SECTORS	4


void
NPrint(string, n)
    char	*string;
    int		n;
{
    int		i;
    for (i = 0; i < n; i++) {
	printf("%s", string);
    }
}

#ifdef TESTING
void
PrintRequests(reqControlPtr)
    RaidRequestControl  *reqControlPtr;
{
    RaidBlockRequest    *reqPtr;
    int			col, row, sector;
    int			i, j;
    extern Raid		*raidPtr;
    static char		outputBuf[MAX_COL][MAX_ROW][MAX_SECTORS];			

    assert(raidPtr->numCol <= MAX_COL && raidPtr->numRow <= MAX_ROW &&
	    raidPtr->sectorsPerDisk <= MAX_SECTORS);

    bzero((char *) outputBuf, sizeof(outputBuf));
    for ( i = 0; i < reqControlPtr->numReq; i++ ) {
	reqPtr = &reqControlPtr->reqPtr[i];
	if (reqPtr->state == REQ_READY) {
	    int		startSector, numSector;
	    startSector = ByteToSector(raidPtr, reqPtr->devReq.startAddress);
	    numSector   = ByteToSector(raidPtr, reqPtr->devReq.bufferLen);
	    for (j = 0; j < numSector; j++) {
		outputBuf[reqPtr->col][reqPtr->row][startSector+j] =
			(reqPtr->devReq.operation == FS_READ ? 'r' : 'w');
	    }
	}
    }
    for (row = 0; row < raidPtr->numRow; row++) {
	printf("+");
	NPrint("-", raidPtr->numCol);
	printf("+\n");
	for (sector = 0; sector < raidPtr->sectorsPerDisk; sector++) {
	    printf("%c", '|');
	    for (col = 0; col < raidPtr->numCol; col++) {
		printf("%c", (outputBuf[col][row][sector] ?
			outputBuf[col][row][sector] : ' '));
	    }
	    printf("%c\n", '|');
	}
    }
}
#endif TESTING


/*
 *----------------------------------------------------------------------
 *
 * PrintHandle --
 *
 *	Print DevBlockDeviceHandle.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Prints to stdout.
 *
 *----------------------------------------------------------------------
 */

void
PrintHandle(handlePtr)
    DevBlockDeviceHandle  *handlePtr; /* Handle pointer of device. */
{
    printf("DevBlockDeviceHandle %x:\n", handlePtr);
    printf("    blockIOProc  : %x\n", handlePtr->blockIOProc);
    printf("    IOControlProc: %x\n", handlePtr->IOControlProc);
    printf("    releaseProc  : %x\n", handlePtr->releaseProc);
    printf("    minTransferUnit: %x\n", handlePtr->minTransferUnit);
    printf("    maxTransferSize: %x\n", handlePtr->maxTransferSize);
    printf("    clientData: %x\n", handlePtr->clientData);
}


/*
 *----------------------------------------------------------------------
 *
 * PrintDevice --
 *
 *	Print Fs_Device.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Prints to stdout.
 *
 *----------------------------------------------------------------------
 */

void
PrintDevice(devicePtr)
    Fs_Device  *devicePtr; /* Handle pointer of device. */
{
    printf("Device %u %02u\n", devicePtr->type, devicePtr->unit);
/*
    printf("Device %x:\n", devicePtr);
    printf("    type: %d  unit: %02u\n", devicePtr->type, devicePtr->unit);
*/
}


/*
 *----------------------------------------------------------------------
 *
 * PrintRequest --
 *
 *	Print DevBlockDeviceRequest.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Prints to stdout.
 *
 *----------------------------------------------------------------------
 */

void
PrintRequest(requestPtr)
    DevBlockDeviceRequest *requestPtr; /* IO Request to be performed. */
{
    char *opStr;

    opStr = (requestPtr->operation == FS_READ ? "READ": "WRITE");

    printf("Block Request %s %x %x:\n", opStr, requestPtr->startAddress,
	    requestPtr->bufferLen);
/*
    printf("Block Request %x:\n", requestPtr);
    printf("    operation: %s\n", (requestPtr->operation == FS_READ ?
	    						"READ": "WRITE") );
    printf("    startAddrHigh: %x\n", requestPtr->startAddrHigh);
    printf("    startAddress : %x\n", requestPtr->startAddress);
    printf("    bufferLen    : %x\n", requestPtr->bufferLen);
    printf("    buffer: %x %40s\n", requestPtr->buffer, requestPtr->buffer);
    printf("    doneProc  : %x\n", requestPtr->doneProc);
    printf("    clientData: %x\n", requestPtr->clientData);
*/
}


/*
 *----------------------------------------------------------------------
 *
 * PrintRaid --
 *
 *	Print Raid.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Prints to stdout.
 *
 *----------------------------------------------------------------------
 */

void
PrintRaid(raidPtr)
    Raid *raidPtr;
{
    int		 col, row;
    RaidDisk	*diskPtr;

    printf("RAID=================================================\n");
    printf("    state %d\n", raidPtr->state);
    PrintDevice(raidPtr->devicePtr);
    printf("    numRow %d\n", raidPtr->numRow);
    printf("    numCol %d\n", raidPtr->numCol);
    printf("    numSector %u\n", raidPtr->numSector);
    printf("    numStripe %u\n", raidPtr->numStripe);
    printf("    dataSectorsPerStripe %u\n",raidPtr->dataSectorsPerStripe);
    printf("    sectorsPerDisk %u\n",raidPtr->sectorsPerDisk);
    printf("    bytesPerStripeUnit %u\n",raidPtr->bytesPerStripeUnit);
    printf("    dataBytesPerStripe %u\n",raidPtr->dataBytesPerStripe);
    printf("    numDataCol %d\n", raidPtr->numDataCol);
    printf("    logBytesPerSector %d\n", raidPtr->logBytesPerSector);
    printf("    sectorsPerStripeUnit %d\n", raidPtr->sectorsPerStripeUnit);
    printf("    rowsPerGroup %d\n", raidPtr->rowsPerGroup);
    printf("    stripeUnitsPerDisk %d\n", raidPtr->stripeUnitsPerDisk);
    printf("    groupsPerArray %d\n", raidPtr->groupsPerArray);
    printf("    parityConfig %c\n", raidPtr->parityConfig);
    if (raidPtr->disk != NULL) {
	for ( row = 0; row < raidPtr->numRow; row++ ) {
	    for ( col = 0; col < raidPtr->numCol; col++ ) {
		diskPtr = raidPtr->disk[col][row];
		if (diskPtr != (RaidDisk *) NIL) {
		    printf("Disk: %d %d %d  state: %d  numValidSector: %d\n",
			    row, col, diskPtr->version,
			    diskPtr->state, diskPtr->numValidSector);
		    PrintDevice(&diskPtr->device);
		}
	    }
	}
    }
    printf("LogDisk: offset=%d\n", raidPtr->log.logDevOffset);
    printf("LogDisk: endOffset=%d\n", raidPtr->log.logDevEndOffset);
    PrintDevice(&raidPtr->log.logDev);
    printf("=====================================================\n");
}


/*
 *----------------------------------------------------------------------
 *
 * PrintTime --
 *
 *	Print current time.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Prints to stdout.
 *
 *----------------------------------------------------------------------
 */

void
PrintTime()
{
    printf("TIME: %lg\n", LocalTime());
}
