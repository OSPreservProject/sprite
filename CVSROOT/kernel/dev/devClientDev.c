/* 
 * devClientDev.c --
 *
 *	This file contains the code for the kernel to interact with
 *	a user process via a device.  This is used to keep track of
 *	currently active client machines of the server's kernel.
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

#include <sprite.h>
#include <stdio.h>
#include <stdlib.h>
#include <hostd.h>
#include <sync.h>
#include <devClientDev.h>
#include <procTypes.h>
#include <timer.h>
#include <fsioDevice.h>

#define	MAX_INDEX 100

typedef struct	DeviceState {
    ClientInfo		toUserQueue[MAX_INDEX];		/* Data to daemon. */
    ClientInfo		toKernelQueue[MAX_INDEX];	/* Data from daemon. */
    int			userUnreadIndex;		/* Unread by daemon. */
    int			userUnusedIndex;		/* Unwritten by os. */
    int			kernelUnreadIndex;		/* Unread by os. */
    int			kernelUnusedIndex;		/* Unwritten by d. */
    Fs_NotifyToken	dataReadyToken;			/* Read/write notify. */
    Sync_Condition	waitForDaemon;			/* Wait for daemon. */
} DeviceState;

static Boolean		deviceOpen = FALSE;
static Fs_Device	*theDevicePtr = (Fs_Device *) NULL;
#define	INC_QUEUEPTR(theIndex)	\
    ((theIndex) == (MAX_INDEX - 1) ? (theIndex) = 0 :  (theIndex)++)


/*
 * Need to do something about queue suddenly becoming empty because it's
 * all filled up!
 */

    


static	Sync_Lock	clientStateLock;
#define	LOCKPTR		(&clientStateLock)


void
RecoverFunc(clientData, callInfoPtr)
    ClientData		clientData;
    Proc_CallInfo	*callInfoPtr;
{
    Fs_Device		*devicePtr;
    DeviceState		*statePtr;
    int			currentUnreadIndex;

    LOCK_MONITOR;
    if (!deviceOpen) {
	/* Don't reschedule - device is closed. */
	UNLOCK_MONITOR;
	return;
    }
    devicePtr = (Fs_Device *) clientData;
    statePtr = (DeviceState *) devicePtr->data;
    /* Start removing stuff from queue of hosts to recover with. */
    printf("RecoverFunc called.\n");
    printf("Setting hostNumber for index %d\n", statePtr->userUnusedIndex);
    statePtr->toUserQueue[statePtr->userUnusedIndex].hostNumber = 66;
    statePtr->toUserQueue[statePtr->userUnusedIndex].hostState =
	    DEV_CLIENT_STATE_NEW_HOST;

    INC_QUEUEPTR(statePtr->userUnusedIndex);
    printf("Index is now %d\n", statePtr->userUnusedIndex);
    printf("Notifying waiters on token.\n");
    currentUnreadIndex = statePtr->userUnreadIndex;
    Fsio_DevNotifyReader(statePtr->dataReadyToken);
    while (statePtr->userUnreadIndex == currentUnreadIndex) {
	(void) Sync_Wait(&(statePtr->waitForDaemon), FALSE);
    }
    printf("RecovFunc calling RecovFunc\n");
    Proc_CallFunc(RecoverFunc, (ClientData) devicePtr, 10 * timer_IntOneSecond);
    UNLOCK_MONITOR;
    return;

}


/*
 *----------------------------------------------------------------------
 *
 * DevClientStateOpen --
 *
 *	For the user process to open the device by which it communicates
 *	with the kernel about clients.
 *
 * Results:
 *	Return status.
 *
 * Side effects:
 *	State for device set up.  Process created that will cause kernel
 *	to start recovering.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
DevClientStateOpen(devicePtr, useFlags, data, flagsPtr)
    Fs_Device		*devicePtr;	/* Device info. */
    int			useFlags;	/* Flags from stream being opened. */
    Fs_NotifyToken	data;		/* Call-back for input notification. */
    int			*flagsPtr;	/* OUT: Device flags. */
{
    DeviceState		*statePtr;

    LOCK_MONITOR;
    printf("Opening client state device.\n");
    if (deviceOpen) {
	printf("Client state device already open.\n");
	UNLOCK_MONITOR;
	return FS_FILE_BUSY;
    }
    theDevicePtr = devicePtr;
    if ((useFlags & FS_USER) == 0) {
	printf("DevClientStateOpen should only  be called by a user proc.\n");
	UNLOCK_MONITOR;
	return GEN_INVALID_ARG;
    }
    statePtr = (DeviceState *) malloc(sizeof (DeviceState));
    devicePtr->data = (ClientData) statePtr;
    statePtr->userUnreadIndex = 0;
    statePtr->userUnusedIndex = 0;
    statePtr->dataReadyToken = data;

    deviceOpen = TRUE;
    printf("Open calling RecovFunc\n");
    Proc_CallFunc(RecoverFunc, (ClientData) devicePtr, 10 * timer_IntOneSecond);
    UNLOCK_MONITOR;
    return SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * DevClientStateClose --
 *
 *	Close the device so it can be reopened.
 *
 * Results:
 *	Return status.
 *
 * Side effects:
 *	Free space and such.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
DevClientStateClose(devicePtr, useFlags, openCount, writerCount)
    Fs_Device		*devicePtr;	/* Device info. */
    int			useFlags;	/* Flags from stream being opened. */
    int			openCount;
    int			writerCount;
{
    LOCK_MONITOR;
    printf("Closing client state device.\n");
    if (!deviceOpen) {
	printf("Client state device already closed.\n");
	UNLOCK_MONITOR;
	return SUCCESS;
    }
    free((char *) devicePtr->data);

    deviceOpen = FALSE;
    UNLOCK_MONITOR;
    return SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * DevClientStateRead --
 *
 *	A user process reads info from kernel on device about client states.
 *
 * Results:
 *	SUCCESS		- if info was found in the queue.  
 *	FS_WOULD_BLOCK	- no info found.
 *
 * Side effects:
 *	Removes some of the info from the queue.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
DevClientStateRead(devicePtr, readPtr, replyPtr)
    Fs_Device	*devicePtr;
    Fs_IOParam	*readPtr;	/* Read parameter block */
    Fs_IOReply	*replyPtr;	/* Return length and signal */ 
{
    ReturnStatus	status = SUCCESS;
    register DeviceState	*statePtr;

    LOCK_MONITOR;

    printf("Read called.\n");
    statePtr = (DeviceState *) devicePtr->data;
    if (statePtr->userUnreadIndex == statePtr->userUnusedIndex) {
	printf("Would block: unused at %d, unread at %d\n",
		statePtr->userUnusedIndex, statePtr->userUnreadIndex);
	status = FS_WOULD_BLOCK;
	UNLOCK_MONITOR;
	return status;
    }

    if (sizeof (ClientInfo) > readPtr->length) {
	status = GEN_INVALID_ARG;
	UNLOCK_MONITOR;
	return status;
    }
    bcopy(&(statePtr->toUserQueue[statePtr->userUnreadIndex]), readPtr->buffer,
	    sizeof (ClientInfo));

    replyPtr->length = sizeof (ClientInfo);
    INC_QUEUEPTR(statePtr->userUnreadIndex);
    printf("Unread is now %d\n", statePtr->userUnreadIndex);
    UNLOCK_MONITOR;
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * DevClientStateWrite --
 *
 *	A user process writes info to kernel on device.
 *
 * Results:
 *	SUCCESS or invalid arg.
 *
 * Side effects:
 *	Removes some of the info from the queue.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
DevClientStateWrite(devicePtr, writePtr, replyPtr)
    Fs_Device	*devicePtr;
    Fs_IOParam	*writePtr;	/* Write parameter block */
    Fs_IOReply	*replyPtr;	/* Return length and signal */ 
{
    ReturnStatus		status = SUCCESS;
    register DeviceState	*statePtr;

    LOCK_MONITOR;

    printf("Write called.\n");
    statePtr = (DeviceState *) devicePtr->data;
    if (writePtr->length - writePtr->offset > sizeof (ClientInfo)) {
	printf("Write info too large.\n");
	UNLOCK_MONITOR;
	return GEN_INVALID_ARG;
    }

    bcopy(writePtr->buffer + writePtr->offset, &(statePtr->toKernelQueue[0]),
	    sizeof (ClientInfo));

    replyPtr->length = sizeof (ClientInfo);
    /* Show that daemon did read data and poked us about it. */
    Sync_Broadcast(&(statePtr->waitForDaemon));
    UNLOCK_MONITOR;
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * DevClientStateSelect --
 *
 *	Perform device-specific select functions on the device.
 *
 * Results:
 *	SUCCESS		- the operation was successful.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
ReturnStatus
DevClientStateSelect(devicePtr, readPtr, writePtr, exceptPtr)
    Fs_Device		*devicePtr;
    int			*readPtr;
    int			*writePtr;
    int			*exceptPtr;
{
    DeviceState		*statePtr;

    statePtr = (DeviceState *) (devicePtr->data);

    LOCK_MONITOR;

    if (*readPtr) {
	if (statePtr->userUnreadIndex == statePtr->userUnusedIndex) {
	    *readPtr = 0;
	}
    }
    *exceptPtr = 0;
    UNLOCK_MONITOR;
    return SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * DevClientStateIOControl --
 *
 *	For the device to tell the kernel it is done processing client state.
 *
 * Results:
 *	SUCCESS		- the operation was successful.
 *	SYS_INVALID_ARG - bad command, or wrong buffer size.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
DevClientStateIOControl(devicePtr, ioctlPtr, replyPtr)
    Fs_Device	*devicePtr;
    Fs_IOCParam *ioctlPtr;
    Fs_IOReply *replyPtr;

{
    DeviceState		*statePtr;
    ReturnStatus	status = SUCCESS;

    statePtr = (DeviceState *) (devicePtr->data);

    switch (ioctlPtr->command) {
    case DEV_CLIENT_END_LIST:
	/* Show that daemon did read data and poked us about it. */
	Sync_Broadcast(&(statePtr->waitForDaemon));
	break;
    default:
	break;
    }

    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * Dev_ClientHostUp
 *
 *	Tell daemon process to record the fact that another client is
 *	talking with us.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Talk to user process.
 *
 *----------------------------------------------------------------------
 */
void
Dev_ClientHostUp(spriteID)
    int		spriteID;
{
    DeviceState	*statePtr;
    int		currentUnreadIndex;

    printf("Dev_ClientHostUp called.\n");
    LOCK_MONITOR;
    if (!deviceOpen) {
	UNLOCK_MONITOR;
	return;
    }
    statePtr = (DeviceState *) theDevicePtr->data;
    statePtr->toUserQueue[statePtr->userUnusedIndex].hostNumber = spriteID;
    statePtr->toUserQueue[statePtr->userUnusedIndex].hostState =
	    DEV_CLIENT_STATE_NEW_HOST;

    INC_QUEUEPTR(statePtr->userUnusedIndex);
    printf("Index is now %d\n", statePtr->userUnusedIndex);
    printf("Notifying waiters on token.\n");
    currentUnreadIndex = statePtr->userUnreadIndex;
    Fsio_DevNotifyReader(statePtr->dataReadyToken);
    /*
     * This test is good enough if the daemon promises to read all
     * available data whenever it reads data.
     */
    while (statePtr->userUnreadIndex == currentUnreadIndex) {
	(void) Sync_Wait(&(statePtr->waitForDaemon), FALSE);
    }
    UNLOCK_MONITOR;
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * Dev_ClientHostDown
 *
 *	Tell daemon process to record the fact that a client that was
 *	talking with us has died.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Talk to user process.
 *
 *----------------------------------------------------------------------
 */
void
Dev_ClientHostDown(spriteID)
    int		spriteID;
{
    DeviceState	*statePtr;
    int		currentUnreadIndex;

    printf("Dev_ClientHostDown called.\n");
    LOCK_MONITOR;
    if (!deviceOpen) {
	UNLOCK_MONITOR;
	return;
    }
    statePtr = (DeviceState *) theDevicePtr->data;
    statePtr->toUserQueue[statePtr->userUnusedIndex].hostNumber = spriteID;
    statePtr->toUserQueue[statePtr->userUnusedIndex].hostState =
	    DEV_CLIENT_STATE_DEAD_HOST;

    INC_QUEUEPTR(statePtr->userUnusedIndex);
    printf("Index is now %d\n", statePtr->userUnusedIndex);
    printf("Notifying waiters on token.\n");
    currentUnreadIndex = statePtr->userUnreadIndex;
    Fsio_DevNotifyReader(statePtr->dataReadyToken);
    /*
     * This test is good enough if the daemon promises to read all
     * available data whenever it reads data.
     */
    while (statePtr->userUnreadIndex == currentUnreadIndex) {
	(void) Sync_Wait(&(statePtr->waitForDaemon), FALSE);
    }
    UNLOCK_MONITOR;
    return;
}
