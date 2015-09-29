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
static char rcsid[] = "$Header: /cdrom/src/kernel/Cvsroot/kernel/dev/devClientDev.c,v 9.3 92/12/13 18:11:54 mgbaker Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <sprite.h>
#include <stdio.h>
#include <stdlib.h>
#include <sync.h>
#include <devClientDev.h>
#include <procTypes.h>
#include <timer.h>
#include <fsioDevice.h>
#include <recov.h>
#include <fsutil.h>

#define	MAX_INDEX 100

typedef struct	DeviceState {
    Dev_ClientInfo	toUserQueue[MAX_INDEX];		/* Data to daemon. */
    Dev_ClientInfo	toKernelQueue[MAX_INDEX];	/* Data from daemon. */
    int			userUnreadIndex;		/* Unread by daemon. */
    int			userUnusedIndex;		/* Unwritten by os. */
    int			kernelUnreadIndex;		/* Unread by os. */
    int			kernelUnusedIndex;		/* Unwritten by d. */
    Fs_NotifyToken	dataReadyToken;			/* Read/write notify. */
    Sync_Condition	waitForDaemon;			/* Wait for daemon. */
    Boolean		recovInProgress;		/* Only one instance. */
    Boolean		waitingForRecov;		/* Condition check. */
    Sync_Condition	waitForRecovResponse;		/* Wait for reopen to
							 * client to return. */
} DeviceState;

static Boolean		deviceOpen = FALSE;
static Fs_Device	*theDevicePtr = (Fs_Device *) NULL;
#define	INC_QUEUEPTR(theIndex)	\
    ((theIndex) == (MAX_INDEX - 1) ? (theIndex) = 0 :  (theIndex)++)
int			currentUnreadIndex;


/*
 * Need to do something about queue suddenly becoming empty because it's
 * all filled up!
 */

static	Sync_Lock	clientStateLock;
#define	LOCKPTR		(&clientStateLock)


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
    statePtr->recovInProgress = FALSE;
    statePtr->waitingForRecov = FALSE;

    /*
     * We're receiving recovery list from user-level daemon if the device
     * is opened for write as well as read.
     */
    if (useFlags & FS_WRITE) {
	Recov_InitServerDriven();
    }
    deviceOpen = TRUE;
    /* Should put in timeout here for getting recov info from client daemon. */
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
    Recov_StopServerDriven();
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

    statePtr = (DeviceState *) devicePtr->data;
    if (statePtr->userUnreadIndex == statePtr->userUnusedIndex) {
	status = FS_WOULD_BLOCK;
	UNLOCK_MONITOR;
	return status;
    }

    if (sizeof (Dev_ClientInfo) > readPtr->length) {
	status = GEN_INVALID_ARG;
	UNLOCK_MONITOR;
	return status;
    }
    bcopy(&(statePtr->toUserQueue[statePtr->userUnreadIndex]), readPtr->buffer,
	    sizeof (Dev_ClientInfo));

    replyPtr->length = sizeof (Dev_ClientInfo);
    INC_QUEUEPTR(statePtr->userUnreadIndex);
    UNLOCK_MONITOR;
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * DevClientStateWrite --
 *
 *	A user process writes info to kernel on device about state of hosts
 *	before crash.
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

    statePtr = (DeviceState *) devicePtr->data;
    if (statePtr->recovInProgress) {
	panic("DevClientStateWrite: only one instance of recovery allowed.\n");
    }
    if (writePtr->offset != 0) {
	printf("DevClientStateWrite: write offset didn't start at 0: %d.\n",
		writePtr->offset);
    }
    if ((writePtr->length % sizeof (Dev_ClientInfo)) != 0) {
	printf(
	"DevClientStateWrite: Write info not multiple of client structure.\n");
	UNLOCK_MONITOR;
	return GEN_INVALID_ARG;
    }

    statePtr->recovInProgress = TRUE;
    bcopy(writePtr->buffer, &(statePtr->toKernelQueue[0]), writePtr->length);
    statePtr->kernelUnreadIndex = 0;
    statePtr->kernelUnusedIndex = writePtr->length / sizeof (Dev_ClientInfo);
    if (statePtr->kernelUnusedIndex > MAX_INDEX) {
	panic("DevClientStateWrite: too many clients!\n");
    }

    Recov_ServerStartingRecovery();
    while (statePtr->kernelUnreadIndex != statePtr->kernelUnusedIndex) {
	int	clientID;

	clientID =
		statePtr->toKernelQueue[statePtr->kernelUnreadIndex].hostNumber;
	if (statePtr->toKernelQueue[statePtr->kernelUnreadIndex].hostState !=
		DEV_CLIENT_STATE_NEW_HOST) {
	    panic("DevClientStateWrite: Bad host state from daemon.");
	}
	Recov_MarkDoingServerRecovery(clientID);
	status = Fsutil_DoServerRecovery(clientID);
#ifdef NOTDEF
	XX Mark going through recovery.
	Proc_CallFunc(Fsutil_DoServerRecovery, (ClientData) clientID, 0);
	statePtr->waitingForRecov = TRUE;
	while (statePtr->waitingForRecov) {
	    (void) Sync_Wait(&(statePtr->waitForRecovResponse), FALSE);
	}
	XX Mark not going through recovery.
#endif NOTDEF
	Recov_UnmarkDoingServerRecovery(clientID);

	INC_QUEUEPTR(statePtr->kernelUnreadIndex);
    }
    Recov_ServerFinishedRecovery();

    statePtr->recovInProgress = FALSE;
    replyPtr->length = writePtr->length;
    UNLOCK_MONITOR;

    return SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * Dev_ClientStateWakeRecovery --
 *
 *	Call-back for another module to rewaken us for us to keep on
 *	doing recovery.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Wakeup signal.
 *
 *----------------------------------------------------------------------
 */
void
Dev_ClientStateWakeRecovery()
{
    DeviceState		*statePtr;

    statePtr = (DeviceState *) theDevicePtr->data;
    statePtr->waitingForRecov = FALSE;
    Sync_Broadcast(&(statePtr->waitForRecovResponse));

    return;
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
    int			numEntries;

    statePtr = (DeviceState *) (devicePtr->data);

    switch (ioctlPtr->command) {
    case DEV_CLIENT_START_LIST:
	/*
	 * Write out state of hosts as we know it to user process.
	 * To do this, we put stuff into the queue starting with a
	 * DEV_CLIENT_START_LIST and ending with a DEV_CLIENT_END_LIST
	 * so that the user process knows when it has the whole list.
	 */
	numEntries = Recov_GetCurrentHostStates(&(statePtr->toUserQueue[1]),
		MAX_INDEX - 2);
	if (numEntries < 0) {
	    panic("DevClientStateIOControl: too many hosts for buffer.");
	}
	statePtr->toUserQueue[0].hostState = DEV_CLIENT_START_LIST;
	statePtr->userUnreadIndex = 0;
	statePtr->userUnusedIndex = numEntries + 2;
	statePtr->toUserQueue[numEntries + 1].hostState = DEV_CLIENT_END_LIST;

	break;
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
 * TimeoutFunc
 *
 *	The user-level daemon hasn't responded quickly enough in writing
 *	our state to disk.  It's probably dead.  Since this must all
 *	happen synchronously, we panic.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May panic.
 *
 *----------------------------------------------------------------------
 */
static void
TimeoutFunc(clientData, callInfoPtr)
    ClientData		clientData;
    Proc_CallInfo	*callInfoPtr;
{
    DeviceState		*statePtr;

    statePtr = (DeviceState *) theDevicePtr->data;

    LOCK_MONITOR;
    if (!deviceOpen) {
	UNLOCK_MONITOR;
	return;
    }
    if (statePtr->userUnreadIndex == currentUnreadIndex) {
	printf("Timeout func for host state device called:\n");
	printf("\tcurrentUnreadIndex is still %d, userUnreadIndex %d\n",
		currentUnreadIndex, statePtr->userUnreadIndex);
	UNLOCK_MONITOR;
	panic("User-level daemon must be dead.");
    }
    UNLOCK_MONITOR;
    return;
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
    currentUnreadIndex = statePtr->userUnreadIndex;
    Fsio_DevNotifyReader(statePtr->dataReadyToken);
    /*
     * This test is good enough if the daemon promises to read all
     * available data whenever it reads data.  We also set a timer in
     * case the user-level daemon has died and doesn't respond.
     */
    Proc_CallFunc(TimeoutFunc, (ClientData) theDevicePtr,
	    5 * timer_IntOneSecond);
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
    currentUnreadIndex = statePtr->userUnreadIndex;
    Fsio_DevNotifyReader(statePtr->dataReadyToken);
    /*
     * This test is good enough if the daemon promises to read all
     * available data whenever it reads data.  We also set a timer in
     * case the user-level daemon has died and doesn't respond.
     */
#ifdef NOTDEF
XX Increase timeout time?  Or make timeout function understand that
if client is still marked as going through recovery, it should allow more
time?
#endif NOTDEF
    printf("userUnread and currentUnread at %d\n", currentUnreadIndex);
    Proc_CallFunc(TimeoutFunc, (ClientData) theDevicePtr,
	    5 * timer_IntOneSecond);
    while (statePtr->userUnreadIndex == currentUnreadIndex) {
	(void) Sync_Wait(&(statePtr->waitForDaemon), FALSE);
    }
    UNLOCK_MONITOR;
    return;
}
