/* 
 * devDebug.c --
 *
 *	This module implements the BlockDevice interface for a device
 *	which echos requests to stdout.
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

#include "sync.h"
#include <sprite.h>
#include <stdio.h>
#include <stdlib.h>
#include "fs.h"
#include "devRaid.h"
#include "devBlockDevice.h"
#include "schedule.h"
#include "timer.h"
#include "devRaidProto.h"

typedef struct DebugHandle {
    DevBlockDeviceHandle blockHandle;	/* Must be FIRST field. */
    Fs_Device		*devPtr;	/* Device corresponding to handle */
} DebugHandle;

#ifdef TESTING
/*
 * Storage for contents of simulated disks.
 * (16 disks of 32 bytes each)
 */
static char devDebugData[4][4][32] = {
"A0A1A2A3A4A5A6A7a0a1a2a3a4a5a6a7",
"B0B1B2B3B4B5B6B7b0b1b2b3b4b5b6b7",
"C0C1C2C3C4C5C6C7c0c1c2c3c4c5c6c7",
"D0D1D2D3D4D5D6D7d0d1d2d3d4d5d6d7",
"E0E1E2E3E4E5E6E7e0e1e2e3e4e5e6e7",
"F0F1F2F3F4F5F6F7f0f1f2f3f4f5f6f7",
"G0G1G2G3G4G5G6G7g0g1g2g3g4g5g6g7",
"H0H1H2H3H4H5H6H7h0h1h2h3h4h5h6h7",
"I0I1I2I3I4I5I6I7i0i1i2i3i4i5i6i7",
"J0J1J2J3J4J5J6J7j0j1j2j3j4j5j6j7",
"K0K1K2K3K4K5K6K7k0k1k2k3k4k5k6k7",
"L0L1L2L3L4L5L6L7l0l1l2l3l4l5l6l7",
"M0M1M2M3M4M5M6M7m0m1m2m3m4m5m6m7",
"N0N1N2N3N4N5N6N7n0n1n2n3n4n5n6n7",
"O0O1O2O3O4O5O6O7o0o1o2o3o4o5o6o7",
"P0P1P2P3P4P5P6P7p0p1p2p3p4p5p6p7",
};

static char logData[30000];
#endif

/*
 * Forward Declarations.
 */
static ReturnStatus BlockIOProc();
static ReturnStatus BlockIOProc1();
static ReturnStatus ReleaseProc();
static ReturnStatus IOControlProc();


/*
 *----------------------------------------------------------------------
 *
 * DevDebugAttach --
 *
 *	Attach a Debug logical device to the system.
 *
 * Results:
 *	The DevBlockDeviceHandle of the device.
 *
 * Side effects:
 *	Modifies Debug data structure.
 *
 *----------------------------------------------------------------------
 */

DevBlockDeviceHandle *
DevDebugAttach(devicePtr)
    Fs_Device	*devicePtr;	/* The device to attach. */
{
    DebugHandle	*handlePtr;

    handlePtr = (DebugHandle *) malloc(sizeof(DebugHandle));
    if (devicePtr->unit < 200) {
        handlePtr->blockHandle.blockIOProc = BlockIOProc;
    } else {
        handlePtr->blockHandle.blockIOProc = BlockIOProc1;
    }
    handlePtr->blockHandle.releaseProc = ReleaseProc;
    handlePtr->blockHandle.IOControlProc = IOControlProc;
    if (devicePtr->unit == 99) {
	handlePtr->blockHandle.minTransferUnit = 64;
    } else {
	handlePtr->blockHandle.minTransferUnit = 1;
    }
    handlePtr->blockHandle.maxTransferSize = 1<<30;
    handlePtr->devPtr = devicePtr;
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
 *
 *----------------------------------------------------------------------
 */
/*ARGUSED*/
static ReturnStatus
ReleaseProc(handlePtr)
    DebugHandle	*handlePtr; /* Handle pointer of device. */
{
    free((char *) handlePtr);
    return SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * IOControlProc --
 *
 *      Do a special operation on a raw SCSI Disk.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Depends on operation.
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
static ReturnStatus
IOControlProc(blockDevHandlePtr, ioctlPtr, replyPtr) 
    DevBlockDeviceHandle
		*blockDevHandlePtr;  /* Handle of the device to operate on. */
    Fs_IOCParam *ioctlPtr;	/* Standard I/O Control parameter block */
    Fs_IOReply *replyPtr;	/* outBuffer length and returned signal */
{
    return SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * BlockIOProc --
 *
 *	Perform block IO on specified Debug device.
 *	Note that this procedure directly calles the callback procedure
 *	before returning.
 *
 * Results:
 *	The return code from the I/O operation.
 *
 * Side effects:
 *	Modifies devDebugData (i.e. the simulated disk contents) if
 *	TESTING is defined and the operation is a write.
 *
 *----------------------------------------------------------------------
 */

static ReturnStatus
BlockIOProc(handlePtr, requestPtr) 
    DevBlockDeviceHandle  *handlePtr; /* Handle pointer of device. */
    DevBlockDeviceRequest *requestPtr; /* IO Request to be performed. */
{
    DebugHandle	   *debugHandlePtr = (DebugHandle *) handlePtr;

    /*
     * Debug device with unit number == 101 is the same as /dev/null except
     * that you can also read infinite amounts of garbage from it as well
     * as writing to it.
     */
    if (debugHandlePtr->devPtr->unit < 100) {
	printf("\nBlockIOProc\n");
	PrintTime();
	PrintHandle(handlePtr);
	PrintDevice(debugHandlePtr->devPtr);
	PrintRequest(requestPtr);
    }

#ifdef TESTING
{
    DevBlockDeviceRequest *localRequestPtr = requestPtr; 
    ReturnStatus    status;
    int		    row, col;
    char	  **statusCtrl;
    int		    i;

    if (debugHandlePtr->devPtr->unit == 99) {
	int *iBuf;
	if (requestPtr->operation == FS_READ) {
	    bcopy(logData+requestPtr->startAddress,
		    requestPtr->buffer,requestPtr->bufferLen);
	} else {
	    bcopy(requestPtr->buffer,
		    logData+requestPtr->startAddress,
		    requestPtr->bufferLen);
	}
	printf("XFER: ");
	iBuf = (int *) requestPtr->buffer;
	for (i = 0; i < requestPtr->bufferLen/4; i++) {
	    printf("%d ", iBuf[i]);
	}
	printf("\n");
	requestPtr->doneProc(requestPtr, SUCCESS, requestPtr->bufferLen);
	return SUCCESS;
    }

    row = debugHandlePtr->devPtr->unit/10;
    col = debugHandlePtr->devPtr->unit%10;

    if (requestPtr->operation == FS_READ) {
	bcopy(devDebugData[row][col]+requestPtr->startAddress,
		requestPtr->buffer,requestPtr->bufferLen);
    } else {
	bcopy(requestPtr->buffer,
		devDebugData[row][col]+requestPtr->startAddress,
		requestPtr->bufferLen);
    }

    printf("XFER: ");
    printf("\"");
    for (i = 0; i < requestPtr->bufferLen; i++) {
        printf("%c", (char)
		(requestPtr->buffer[i] == '\0' ? '_' : requestPtr->buffer[i]));

    }
    printf("\"");
    printf("\n");

    statusCtrl = (char **) requestPtr->ctrlData[0];
    if (statusCtrl == (char **) NIL) {
	status = SUCCESS;
    } else {
        status = (*(*statusCtrl)++ == 's' ? SUCCESS: FAILURE);
    }
    (void) printf("status: %d\n", status);

    if (Spawn("disk") == 0) {
	Delay(10.0);
        localRequestPtr->doneProc(localRequestPtr,
		status, localRequestPtr->bufferLen);
	Terminate();
    } else {
        return SUCCESS;
    }
}
#else
    requestPtr->doneProc(requestPtr, SUCCESS, requestPtr->bufferLen);
    return SUCCESS;
#endif TESTING
}


/*
 *----------------------------------------------------------------------
 *
 * timerCallBackProc --
 *
 *	Callback procedure for BlockIOProc1.
 *	This callback procedure called from timer interrupts which
 *	simulate IO interrupts that an IO to a real device would result in.
 *
 * Results:
 *	The return code from the I/O operation.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

#ifndef TESTING
typedef struct TimerCallBackData {
    Timer_QueueElement	  *timerQueueElemPtr;
    void		 (*proc)();
    DevBlockDeviceRequest *requestPtr;
    ReturnStatus	   status;
    int			   amountTransferred;	
} TimerCallBackData;

static void
timerCallBackProc(time, timerCallBackDataPtr)
    Timer_Ticks		time;
    TimerCallBackData  *timerCallBackDataPtr;
{
    timerCallBackDataPtr->proc(timerCallBackDataPtr->requestPtr,
        timerCallBackDataPtr->status, timerCallBackDataPtr->amountTransferred);
    free((char*) timerCallBackDataPtr->timerQueueElemPtr);
    free((char*) timerCallBackDataPtr);
}
#endif TESTING


/*
 *----------------------------------------------------------------------
 *
 * BlockIOProc1 --
 *
 *	Perform block IO on specified Debug device.
 *	This procedure is similar to BlockIOProc except that the callback
 *	is done at interrupt level via timer interrupts.
 *
 * Results:
 *	The return code from the I/O operation.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static ReturnStatus
BlockIOProc1(handlePtr, requestPtr) 
    DevBlockDeviceHandle  *handlePtr;  /* Handle pointer of device. */
    DevBlockDeviceRequest *requestPtr; /* IO Request to be performed. */
{
#ifdef TESTING
    DevBlockDeviceRequest *localRequestPtr = requestPtr; 
    DebugHandle	   *debugHandlePtr = (DebugHandle *) handlePtr;
    ReturnStatus    status;
    char	  **statusCtrl;
    int		    row, col;
    int		    i;

    printf("\nBlockIOProc\n");
    PrintTime();
    PrintHandle(handlePtr);
    PrintDevice(debugHandlePtr->devPtr);
    PrintRequest(requestPtr);

    row = debugHandlePtr->devPtr->unit/10;
    col = debugHandlePtr->devPtr->unit%10;

    if (requestPtr->operation == FS_READ) {
	bcopy(devDebugData[col][row],requestPtr->buffer,requestPtr->bufferLen);
    } else {
	bcopy(requestPtr->buffer,devDebugData[col][row],requestPtr->bufferLen);
    }
    printf("XFER: ");
    for (i = 0; i < requestPtr->bufferLen; i++) {
        printf("%c", (char) requestPtr->buffer[i]);
    }
    printf("\n");

    /*
     * When using the Debug device, ctrlData should be initialized with
     * status control information.
     */
    statusCtrl = (char **) requestPtr->ctrlData[0];
    status = (*(*statusCtrl)++ == 's' ? SUCCESS: FAILURE);
    (void) printf("status: %d\n", status);

    /*
     * Simulate request queueing.
     */
    if (Spawn("disk")) {
	return SUCCESS;
    } else {
        /*
         * Child simulates IO Device.
         */
        Delay(100.0);
	localRequestPtr->buffer[0] =
		debugHandlePtr->devPtr->unit/10+'0';
	localRequestPtr->buffer[1] =
		debugHandlePtr->devPtr->unit%10+'0';
	localRequestPtr->doneProc(localRequestPtr,
				  status, localRequestPtr->bufferLen);
	Terminate();

        /*
         * This never gets executed and is only here to shutup lint.
         */
        return SUCCESS;
    }
#else
{
    Timer_QueueElement *timerQueueElemPtr;
    TimerCallBackData  *timerCallBackDataPtr;

    timerQueueElemPtr = (Timer_QueueElement *)
            malloc(sizeof(Timer_QueueElement));
    timerCallBackDataPtr = (TimerCallBackData *)
            malloc(sizeof(TimerCallBackData));
    timerQueueElemPtr->routine = timerCallBackProc;
    timerQueueElemPtr->clientData = (ClientData) timerCallBackDataPtr;
    timerQueueElemPtr->interval = 10;
    timerCallBackDataPtr->timerQueueElemPtr = timerQueueElemPtr;
    timerCallBackDataPtr->proc = requestPtr->doneProc;
    timerCallBackDataPtr->requestPtr = requestPtr;
    timerCallBackDataPtr->status = SUCCESS;
    timerCallBackDataPtr->amountTransferred = requestPtr->bufferLen;
    Timer_ScheduleRoutine(timerQueueElemPtr, TRUE);
    return SUCCESS;
}
#endif TESTING
}
