/* 
 * ttyPdev.c --
 *
 *	This file provides a bridge between the pdev and td modules to
 *	produce a pseudo-device that behaves like a terminal.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/etc/RCS/ttyPdev.c,v 1.8 90/02/28 11:10:55 brent Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <pdev.h>
#include <status.h>
#include <stdlib.h>
#include <string.h>
#include <td.h>

/*
 * Library imports:
 */

extern void panic();

/*
 * One of the following structures is created for each call to
 * Td_CreatePdev:
 */

typedef struct {
    Pdev_Token pdev;			/* Token returned by Pdev_Open. */
    char *pdevName;			/* Name of pseudo-device file
					 * (malloc'ed). */
    Td_Terminal term;			/* Token returned by Td_Create. */
    int selectState;			/* Current select state of terminal;
					 * a combination of FS_READABLE,
					 * FS_WRITABLE, and FS_EXCEPTION. */
} PdevTerm;

/*
 * Forward declarations to procedures defined later in this file:
 */

static int	ChangeReady();
static int	CookedProc();
static int	PdevClose();
static int	PdevIoctl();
static int	PdevOpen();
static int	PdevRead();
static int	PdevWrite();
static int	SendSignal();

/*
 *----------------------------------------------------------------------
 *
 * Td_CreatePdev --
 *
 *	Create a terminal with a pseudo-device attached to it.
 *
 * Results:
 *	The return value is a handle that may be passed to
 *	Td_DeletePdev to close the pseudo-terminal.  The Td_Terminal
 *	token for the terminal gets stored at *termPtr, for the
 *	caller's use in communicating with the terminal driver.
 *	If a pseudo-device couldn't be opened, then the return value
 *	is NULL and an error message is stored in pdev_ErrorMsg.
 *
 * Side effects:
 *	A Td_Terminal is created with its "cooked" side attached
 *	to a pseudo-device managed by this module.  The caller
 *	must use the Fs_Select facilities so that this module
 *	gets callbacks from the Pdev library.
 *
 *----------------------------------------------------------------------
 */

Td_Pdev
Td_CreatePdev(name, realNamePtr, termPtr, rawProc, clientData)
    char *name;			/* Name of file to use for pseudo-device. */
    char **realNamePtr;		/* Where to store pointer to actual name
				 * used. */
    Td_Terminal *termPtr;	/* Token for the Td_Terminal gets written
				 * here, if this is non-NULL. */
    int (*rawProc)();		/* Procedure for Td module to call to
				 * handle control requests on raw side
				 * of terminal. */
    ClientData clientData;	/* Arbitrary data value to pass to rawProc. */
{
    Pdev_CallBacks callbacks;
    register PdevTerm *ptPtr;

    ptPtr = (PdevTerm *) malloc(sizeof(PdevTerm));
    callbacks.open = PdevOpen;
    callbacks.read = PdevRead;
    callbacks.write = PdevWrite;
    callbacks.ioctl = PdevIoctl;
    callbacks.close = PdevClose;
    ptPtr->pdev = Pdev_Open(name, realNamePtr, 1000, 0,
	    &callbacks, (ClientData) ptPtr);
    if (ptPtr->pdev == NULL) {
	free((char *) ptPtr);
	return (Td_Pdev) NULL;
    }
    if (realNamePtr != NULL) {
	name = *realNamePtr;
    }
    ptPtr->pdevName = malloc((unsigned) (strlen(name) + 1));
    strcpy(ptPtr->pdevName, name);
    ptPtr->term = Td_Create(1000, CookedProc, (ClientData) ptPtr,
	    rawProc, clientData);
    ptPtr->selectState = FS_WRITABLE;

    if (termPtr != NULL) {
	*termPtr = ptPtr->term;
    }
    return (Td_Pdev) ptPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Td_DeletePdev --
 *
 *	Delete a pseudo-device and the Td_Terminal associated with it.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The memory and state associated with the pseudo device and
 *	terminal are recycled.  The pseudo-device file is destroyed,
 *	if that is possible.
 *
 *----------------------------------------------------------------------
 */

void
Td_DeletePdev(ttyPdev)
    Td_Pdev ttyPdev;		/* Pseudo-terminal to destroy. */
{
    register PdevTerm *ptPtr = (PdevTerm *) ttyPdev;

    /*
     * Close the terminal first, so that hangups can be sent to
     * processes.
     */

    Td_Delete(ptPtr->term);
    Pdev_Close(ptPtr->pdev);
    unlink(ptPtr->pdevName);
    free(ptPtr->pdevName);
    free((char *) ptPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * PdevOpen --
 *
 *	This procedure is called back by the Pdev module whenever
 *	a pseudo-terminal is being opened.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
static int
PdevOpen(ptPtr, newStream, readBuffer, flags, procID, hostID,
	uid, selectBitsPtr)
    register PdevTerm *ptPtr;	/* Our information about the pdev. */
    Pdev_Stream *newStream;	/* Service stream associated with the
				 * new open. */
    char *readBuffer;		/* Read buffer:  not used here. */
    int flags;			/* Flags from open kernel call (not used). */
    int procID;			/* Process doing open (not used). */
    int hostID;			/* Host where process is running (not used). */
    int uid;			/* Effective user id of pid (not used). */
    int *selectBitsPtr;		/* Store select state of new stream here. */
{
    int result;
    Boolean true = TRUE;
    ReturnStatus status;

    newStream->clientData = (ClientData) ptPtr;
    result = Td_Open(ptPtr->term, &ptPtr->selectState);
    *selectBitsPtr = ptPtr->selectState;
    status = Fs_IOControl(newStream->streamID, IOC_PDEV_WRITE_BEHIND,
	    sizeof(int), (Address) &true, 0, (Address) NULL);
    if (status != SUCCESS) {
	panic("PdevOpen couldn't enable write-behind:  %s",
		Stat_GetMsg(status));
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * PdevClose --
 *
 *	This procedure is called back by the Pdev module when all of
 *	the streams corresponding to one "open" on a pseudo-terminal
 *	have now been closed.
 *
 * Results:
 *	Always returns zero.
 *
 * Side effects:
 *	State in the terminal is updated.
 *
 *----------------------------------------------------------------------
 */

static int
PdevClose(streamPtr)
    Pdev_Stream *streamPtr;	/* Service stream that is about to go away. */
{
    Td_Close(((PdevTerm *) streamPtr->clientData)->term);
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * PdevRead --
 *
 *	This procedure is called back by the Pdev module whenever
 *	a client tries to read the pseudo-device associated with
 *	a terminal.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
static int
PdevRead(streamPtr, readPtr, freeItPtr, selectBitsPtr, sigPtr)
    Pdev_Stream *streamPtr;	/* Service stream the client specified in its
				 * kernel call. */
    Pdev_RWParam *readPtr;	/* Read parameter block.  Indicates size,
				 * buffer, plus various IDs */
    Boolean *freeItPtr;		/* Not used here. */
    int *selectBitsPtr;		/* Store new select state of terminal here. */
    Pdev_Signal *sigPtr;
{
    int result;
    register PdevTerm *ptPtr = (PdevTerm *) streamPtr->clientData;

    result = Td_GetCooked(ptPtr->term, readPtr->procID, readPtr->familyID,
	    &readPtr->length, readPtr->buffer, &sigPtr->signal,
	    &ptPtr->selectState);
    *selectBitsPtr = ptPtr->selectState;
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * PdevWrite --
 *
 *	This procedure is called back by the Pdev module whenever
 *	a client tries to write the pseudo-device associated with
 *	a terminal.  Note:  these writes are always asynchronous.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
static int
PdevWrite(streamPtr, async, writePtr, selectBitsPtr, sigPtr)
    Pdev_Stream *streamPtr;	/* Service stream the client specified in its
				 * kernel call. */
    int async;			/* Non-zero means this is an asynchronous
				 * write request (should always be TRUE). */
    Pdev_RWParam *writePtr;	/* Write parameter block.  Indicates size,
				 * offset, and buffer, among other things */
    int *selectBitsPtr;		/* Store new select state of terminal here. */
    Pdev_Signal *sigPtr;	/* Signal to return, if any */
{
    register PdevTerm *ptPtr = (PdevTerm *) streamPtr->clientData;
    int oldBits, result;

    oldBits = ptPtr->selectState;
    result = Td_PutCooked(ptPtr->term, &writePtr->length, writePtr->buffer,
	    &sigPtr->signal, &ptPtr->selectState);
    if (ptPtr->selectState != oldBits) {
	Pdev_EnumStreams(ptPtr->pdev, ChangeReady,
		(ClientData) ptPtr->selectState);
    }
    *selectBitsPtr = ptPtr->selectState;
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * PdevIoctl --
 *
 *	This procedure is called back by the Pdev module whenever
 *	a client tries to issue an ioctl on the pseudo-device associated
 *	with a terminal.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
static int
PdevIoctl(streamPtr, ioctlPtr, selectBitsPtr, sigPtr)
    Pdev_Stream *streamPtr;	/* Service stream the client specified in its
				 * kernel call. */
    Pdev_IOCParam *ioctlPtr;	/* I/O control parameters */
    int *selectBitsPtr;		/* Store new select state of terminal here. */
    Pdev_Signal *sigPtr;	/* Returned signal, if any */
{
    register PdevTerm *ptPtr = (PdevTerm *) streamPtr->clientData;
    int result;

    result = Td_ControlCooked(ptPtr->term, ioctlPtr->command,
	    ioctlPtr->format,
	    ioctlPtr->inBufSize, ioctlPtr->inBuffer,
	    &ioctlPtr->outBufSize, ioctlPtr->outBuffer,
	    &sigPtr->signal, &ptPtr->selectState);
    *selectBitsPtr = ptPtr->selectState;
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * CookedProc --
 *
 *	This procedure is called back by the Td module to inform
 *	us of various things happening on the cooked side of the
 *	terminal.
 *
 * Results:
 *	The return value is the number of bytes of output data
 *	stored at outBuffer (always 0 right now).
 *
 * Side effects:
 *	Depends on the command;  read the code for details.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
static int
CookedProc(ptPtr, command, inSize, inBuffer, outSize, outBuffer)
    register PdevTerm *ptPtr;	/* Information about the pseudo-device
				 * for the terminal. */
    int command;		/* Identifies control operation being
				 * invoked, e.g. TD_COOKED_SIGNAL. */
    int inSize;			/* Number of bytes of input data available
				 * to us. */
    char *inBuffer;		/* Pointer to input data. */
    int outSize;		/* Maximum number of bytes of output data
				 * we can return to caller. */
    char *outBuffer;		/* Area in which to store output data for
				 * caller. */
{
    int result = 0;

    switch (command) {
	case TD_COOKED_SIGNAL:
	    (void) Pdev_EnumStreams(ptPtr->pdev, SendSignal,
		    (ClientData) inBuffer);
	    break;
	case TD_COOKED_READS_OK:
	    if (!(ptPtr->selectState & FS_READABLE)) {
		ptPtr->selectState |= FS_READABLE;
		(void) Pdev_EnumStreams(ptPtr->pdev, ChangeReady,
			(ClientData) ptPtr->selectState);
	    }
	    break;
	case TD_COOKED_WRITES_OK:
	    if (!(ptPtr->selectState & FS_WRITABLE)) {
		ptPtr->selectState |= FS_WRITABLE;
		(void) Pdev_EnumStreams(ptPtr->pdev, ChangeReady,
			(ClientData) ptPtr->selectState);
	    }
	    break;
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * ChangeReady --
 *
 *	This procedure is called back by Pdev_EnumStreams in order
 *	to reset the readiness of all the streams associated with
 *	a terminal.
 *
 * Results:
 *	Always returns 0.
 *
 * Side effects:
 *	The stream's select state is updated to match.
 *
 *----------------------------------------------------------------------
 */

static int
ChangeReady(streamPtr, selectState)
    Pdev_Stream *streamPtr;		/* Information about the particular
					 * stream. */
    int selectState;			/* New select state for stream. */
{
    ReturnStatus status;

    status = Fs_IOControl(streamPtr->streamID, IOC_PDEV_READY,
	    sizeof(int), (Address) &selectState, 0, (Address) 0);
    if (status != SUCCESS) {
	panic("ChangeReady couldn't reset select state for pdev: %s",
		Stat_GetMsg(status));
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * SendSignal --
 *
 *	This procedure is called back by Pdev_EnumStreams in order
 *	to send a signal to the controlling process for a terminal.
 *
 * Results:
 *	Always returns 1 to abort the enumeration after 1 stream
 *	has been processed (there's no need to generate the signal
 *	more than once).
 *
 * Side effects:
 *	A signal is sent to the pseudo-device's controlling process (group).
 *
 *----------------------------------------------------------------------
 */

static int
SendSignal(streamPtr, sigInfoPtr)
    Pdev_Stream *streamPtr;		/* Information about the particular
					 * stream. */
    Td_Signal *sigInfoPtr;		/* Information about signal to send. */
{
    ReturnStatus status;
    Pdev_Signal sigInfo;

    status = Compat_UnixSignalToSprite(sigInfoPtr->sigNum, &sigInfo.signal);
    if (status != SUCCESS) {
	panic("SendSignal couldn't translate signal %d", sigInfoPtr->sigNum);
    }
    sigInfo.code = 0;

    /*
     * Ignore errors in sending the signal:  they could happen because
     * the user set a non-existent process group.
     */

    (void) Fs_IOControl(streamPtr->streamID, IOC_PDEV_SIGNAL_OWNER,
	    sizeof(sigInfo), (Address) &sigInfo, 0, (Address) 0);
    return 1;
}
