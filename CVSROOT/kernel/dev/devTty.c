/* 
 * devTty.c --
 *
 *	This file implements terminals and terminal-like devices for
 *	Sprite.  It consists of a bunch of glue that hooks together a
 *	generic terminal driver (the Td_ library), one or more
 *	machine- and device-specific drivers for RS232 hardware,
 *	and the generic Sprite kernel-call interfaces for devices.
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
#include "dev.h"
#include "ttyAttach.h"
#include <errno.h>
#include <fs.h>
#include "proc.h"
#include <status.h>
#include "sync.h"
#include "tty.h"

/*
 * Monitor lock used for synchronizing access to DevTty and Td_Terminal
 * structures.  See the comment above the declaration for DevTty structs
 * (in devTty.h) for more information.  This lock may also be used by
 * a few other files that interact closely with this one.
 */

Sync_Lock devTtyLock = Sync_LockInitStatic("Dev:devTtyLock");
#define LOCKPTR (&devTtyLock)

/*
 * The following variable tells when the last user interaction occurred
 * on the console.  It doesn't exactly belong here, but there isn't
 * anyplace where it fits better.
 */

Time		dev_LastConsoleInput;

/*
 * Forward declarations for procedures defined in this file:
 */

static int	CookedProc();
static int	RawProc();
static void	Signal();
static void	Transfer();
static void	TransferInProc();
static void	TransferOutProc();

/*
 *----------------------------------------------------------------------
 *
 * DevTtyOpen --
 *
 *	Called through devFsOpTable to open a terminal device.
 *	Initializes the device and activates it so that it's ready
 *	to return input.
 *
 * Results:
 *	A standard Sprite ReturnStatus.
 *
 * Side effects:
 *	The device will be "turned on" if it isn't already, which may
 *	involve setting up interrupt handlers.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
ENTRY ReturnStatus
DevTtyOpen(devicePtr, useFlags, notifyToken)
    Fs_Device *devicePtr;	/* Information about device (e.g. type
				 * and unit number). */
    int useFlags;		/* Flags for the stream being opened:
				 * OR'ed combination of FS_READ and
				 * FS_WRITE. */
    Fs_NotifyToken notifyToken;	/* Used for Fs call-back to notify waiting
				 * processes that the terminal is ready. */
{
    register DevTty *ttyPtr;
    int result;

    LOCK_MONITOR;

    /*
     * Call machine-specific code to attach to the device, then fill in
     * all the fields that weren't already filled in by the attach procedure.
     */

    ttyPtr = DevTtyAttach(devicePtr->unit);
    if (ttyPtr == NULL) {
	UNLOCK_MONITOR;
	return Compat_MapToSprite(ENXIO);
    }
    if (ttyPtr->openCount == 0) {
	ttyPtr->insertInput = 0;
	ttyPtr->extractInput = 0;
	ttyPtr->insertOutput = 0;
	ttyPtr->extractOutput = 0;
	ttyPtr->term = Td_Create(500, CookedProc, (ClientData) ttyPtr,
		RawProc, (ClientData) ttyPtr);
	ttyPtr->notifyToken = notifyToken;
	ttyPtr->openCount = 0;
	(*ttyPtr->activateProc)(ttyPtr->rawData);
    }

    /*
     * Officially open the terminal.
     */

    result = Td_Open(ttyPtr->term, &ttyPtr->selectState);
    if (result != 0) {
	if (ttyPtr->openCount == 0) {
	    Td_Delete(ttyPtr->term);
	}
	UNLOCK_MONITOR;
	return Compat_MapToSprite(result);
    }
    ttyPtr->openCount++;
    devicePtr->data = (ClientData) ttyPtr;
    UNLOCK_MONITOR;
    return SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * DevTtyRead --
 *
 *	Called through devFsOpTable to read from a terminal device.
 *
 * Results:
 *	A standard Sprite ReturnStatus.  Characters are stored at
 *	*readPtr->buffer, and the fields of *replyPtr are modified
 *	to describe what happened.
 *
 * Side effects:
 *	Information may be removed from the input buffer for the terminal.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
ENTRY ReturnStatus
DevTtyRead(devicePtr, readPtr, replyPtr)
    Fs_Device *devicePtr;		/* Information about device. */
    register Fs_IOParam *readPtr;	/* Input parameters. */
    register Fs_IOReply *replyPtr;	/* Place to store return information. */
{
    register DevTty *ttyPtr = (DevTty *) devicePtr->data;
    int result;

    LOCK_MONITOR;
    replyPtr->length = readPtr->length;
    result = Td_GetCooked(ttyPtr->term, readPtr->procID, readPtr->familyID,
	    &replyPtr->length, (char *) readPtr->buffer, &replyPtr->signal,
	    &ttyPtr->selectState);
    replyPtr->flags = ttyPtr->selectState;
    UNLOCK_MONITOR;
    if (result != 0) {
	return Compat_MapToSprite(result);
    }
    return SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * DevTtyWrite --
 *
 *	Called through devFsOpTable to write to a terminal device.
 *
 * Results:
 *	A standard Sprite ReturnStatus.  Fields of *replyPtr are
 *	modified to indicate what happened in the write operation.
 *
 * Side effects:
 *	Information may be added to the output buffer for the terminal.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
ENTRY ReturnStatus
DevTtyWrite(devicePtr, writePtr, replyPtr)
    Fs_Device *devicePtr;		/* Information about device. */
    register Fs_IOParam *writePtr;	/* Input parameters. */
    register Fs_IOReply *replyPtr;	/* Place to store result info. */
{
    register DevTty *ttyPtr = (DevTty *) devicePtr->data;
    int result, countThisTime, stillToDo;
    char *bufPtr;
    ReturnStatus status = SUCCESS;

    LOCK_MONITOR;

    /*
     * Td_PutCooked will accept anything we give it even if it exceeds
     * the nominal buffer size.  To keep it from growing its output buffer
     * very large, break big blocks up into small ones, and stop as soon
     * as the FS_WRITABLE bit goes away (which means the nominal buffer
     * size has been exceeded).
     */

    replyPtr->length = 0;
    stillToDo = writePtr->length;
    bufPtr = (char *) writePtr->buffer;
    while (stillToDo != 0) {
	if (!(ttyPtr->selectState & FS_WRITABLE)) {
	    status = FS_WOULD_BLOCK;
	    break;
	}
	countThisTime = stillToDo;
	if (countThisTime > 100) {
	    countThisTime = 100;
	}
	result = Td_PutCooked(ttyPtr->term, &countThisTime, bufPtr,
		&replyPtr->signal, &ttyPtr->selectState);
	replyPtr->flags = ttyPtr->selectState;
	replyPtr->length += countThisTime;
	bufPtr += countThisTime;
	stillToDo -= countThisTime;
	if (result != 0) {
	    status = Compat_MapToSprite(result);
	    break;
	}
    }
    UNLOCK_MONITOR;
    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * DevTtyIOControl --
 *
 *	Called through devFsOpTable to perform IOControl operations on
 *	a terminal device.
 *
 * Results:
 *	A standard Sprite ReturnStatus.  *iocPtr->outBuffer may be
 *	modified to hold the output information from the IOControl,
 *	and the fields of *replyPtr are modified to show what happened.
 *
 * Side effects:
 *	Depends on the IOControl operation.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
ENTRY ReturnStatus
DevTtyIOControl(devicePtr, iocPtr, replyPtr)
    Fs_Device *devicePtr;		/* Information about device. */
    register Fs_IOCParam *iocPtr;	/* Parameter information (buffer sizes
					 * etc.). */
    register Fs_IOReply *replyPtr;	/* Place to store result information. */
{
    register DevTty *ttyPtr = (DevTty *) devicePtr->data;
    int result;

    LOCK_MONITOR;
    replyPtr->length = iocPtr->outBufSize;
    result = Td_ControlCooked(ttyPtr->term, iocPtr->command, iocPtr->format,
	    iocPtr->inBufSize, (char *) iocPtr->inBuffer,
	    &replyPtr->length, (char *) iocPtr->outBuffer,
	    &replyPtr->signal, &ttyPtr->selectState);
    replyPtr->flags = ttyPtr->selectState;
    UNLOCK_MONITOR;
    if (result != 0) {
	return Compat_MapToSprite(result);
    }
    return SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * DevTtySelect --
 *
 *	Called through devFsOpTable to perform select-related functions
 *	on a terminal device.
 *
 * Results:
 *	Always SUCCESS.  The values at *readPtr, *writePtr, and *exceptPtr
 *	get set to zero if the device is NOT readable, or writable, or
 *	exception-pending, respectively.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
DevTtySelect(devicePtr, readPtr, writePtr, exceptPtr)
    Fs_Device *devicePtr;	/* Information about device. */
    int *readPtr;		/* Set to zero if device not readable. */
    int *writePtr;		/* Set to zero if device not writable. */
    int *exceptPtr;		/* Set to zero if no exception pending on
				 * device. */
{
    register DevTty *ttyPtr = (DevTty *) devicePtr->data;

    /*
     * Because this is only a simple query, don't need any locking in
     * this procedure.
     */

    if (!(ttyPtr->selectState & FS_READABLE)) {
	*readPtr = 0;
    }
    if (!(ttyPtr->selectState & FS_WRITABLE)) {
	*writePtr = 0;
    }
    if (!(ttyPtr->selectState & FS_EXCEPTION)) {
	*exceptPtr = 0;
    }
    return SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * DevTtyClose --
 *
 *	Called through devFsOpTable whenever a terminal is closed.
 *
 * Results:
 *	A standard Sprite return status.
 *
 * Side effects:
 *	Data structures get cleaned up and possibly deallocated.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
ENTRY ReturnStatus
DevTtyClose(devicePtr, useFlags, openCount, writerCount)
    Fs_Device *devicePtr;		/* Information about device. */
    int useFlags;			/* Indicates whether stream being
					 * closed was open for reading and/or
					 * writing:  OR'ed combination of
					 * FS_READ and FS_WRITE. */
    int openCount;			/* # of times this particular stream
					 * is still open. */
    int writerCount;			/* # of times this particular stream
					 * is still open for writing. */
{
    DevTty *ttyPtr = (DevTty *) devicePtr->data;

    if (openCount > 0) {
	return SUCCESS;
    }

    LOCK_MONITOR;

    Td_Close(ttyPtr->term);
    ttyPtr->openCount --;
    if (ttyPtr->openCount == 0) {
	Td_Delete(ttyPtr->term);
    }
    UNLOCK_MONITOR;
    return SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * CookedProc --
 *
 *	This procedure is called by the Td library to perform control
 *	operations on the "cooked" size of the terminal.
 *
 * Results:
 *	Always zero, to indicate that no information is returned
 *	in outBuffer.
 *
 * Side effects:
 *	Depends on the control operation.  Most likely effect is to
 *	wake up a waiting process.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
INTERNAL static int
CookedProc(ttyPtr, operation, inBufSize, inBuffer, outBufSize, outBuffer)
    register DevTty *ttyPtr;	/* Our information about terminal. */
    int operation;		/* What to do:  TD_COOKED_SIGNAL, etc. */
    int inBufSize;		/* Size of input buffer for operation. */
    char *inBuffer;		/* Input buffer. */
    int outBufSize;		/* Size of output buffer for operation. */
    char *outBuffer;		/* Output buffer. */
{
    int result = 0;
    int sigNum, pID;

    switch (operation) {
	case TD_COOKED_SIGNAL:
	    sigNum = ((int *) inBuffer)[0];
	    pID = ((int *) inBuffer)[1];
	    Signal(sigNum, (Proc_PID) pID, TRUE);
	    break;
	case TD_COOKED_READS_OK:
	    if (!(ttyPtr->selectState & FS_READABLE)) {
		ttyPtr->selectState |= FS_READABLE;
		Fsio_DevNotifyReader(ttyPtr->notifyToken);
	    }
	    break;
	case TD_COOKED_WRITES_OK:
	    if (!(ttyPtr->selectState & FS_WRITABLE)) {
		ttyPtr->selectState |= FS_WRITABLE;
		Fsio_DevNotifyWriter(ttyPtr->notifyToken);
	    }
	    break;
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * RawProc --
 *
 *	This procedure is called by the Td library to perform control
 *	operations on the "raw" size of the terminal.
 *
 * Results:
 *	The return value is the number of bytes returned to the caller
 *	at outBuffer.
 *
 * Side effects:
 *	Depends on the control operation.  Most likely effect is to
 *	start transferring output data.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
INTERNAL static int
RawProc(ttyPtr, operation, inBufSize, inBuffer, outBufSize, outBuffer)
    register DevTty *ttyPtr;	/* Our information about terminal. */
    int operation;		/* What to do:  TD_RAW_OUTPUT_READY etc. */
    int inBufSize;		/* Size of input buffer for operation. */
    char *inBuffer;		/* Input buffer. */
    int outBufSize;		/* Size of output buffer for operation. */
    char *outBuffer;		/* Output buffer. */
{
    /*
     * Move data to the intermediate output buffer, if it's ready.
     * Otherwise pass the operation on to the device-level procedure.
     */

    if (operation == TD_RAW_OUTPUT_READY) {
	Transfer(ttyPtr);
	return 0;
    }

    /*
     * Don't actually close the device if it's the console:  otherwise
     * we'd stop responding to console commands.
     */

    if ((operation == TD_RAW_SHUTDOWN)
	    && (ttyPtr->consoleFlags & DEV_TTY_IS_CONSOLE)) {
	return 0;
    }

    return ttyPtr->rawProc(ttyPtr->rawData, operation, inBufSize, inBuffer,
	    outBufSize, outBuffer);
}

/*
 *----------------------------------------------------------------------
 *
 * DevTtyInputChar --
 *
 *	This procedure is invoked at interrupt level by device drivers
 *	when an input character is received.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The character is added to the buffer for ttyPtr, and arrangements
 *	are made to process the character more fully in a process later.
 *	If the buffer overflows, an error message is printed on the console.
 *
 *----------------------------------------------------------------------
 */

void
DevTtyInputChar(ttyPtr, value)
    register DevTty *ttyPtr;	/* Our information about terminal. */
    int value;			/* Character (or special condition like break)
				 * that just arrived on terminal. */
{
    int next, old;

    old = ttyPtr->insertInput;
    next = old+1;
    if (next >= TTY_IN_BUF_SIZE) {
	next = 0;
    }
    if (next == ttyPtr->extractInput) {
	if (!(ttyPtr->consoleFlags & DEV_TTY_OVERFLOWED)) {
	    ttyPtr->consoleFlags |= DEV_TTY_OVERFLOWED;
	    printf("Buffer overflow in DevTtyInputChar\n");
	} 
	return;
    }
    ttyPtr->inBuffer[old] = value;

    /*
     * Note: must advance insertInput before scheduling background
     * procedure, in order to avoid race.
     */

    ttyPtr->insertInput = next;
    if (old == ttyPtr->extractInput) {
	Proc_CallFunc(TransferInProc, (ClientData) ttyPtr, 0);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TransferInProc --
 *
 *	This procedure is invoked in a kernel server process in response
 *	to a Proc_CallFunc arrangement made by DevTtyInputChar. 
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Characters get removed from ttyPtr->inBuffer and sent off to the
 *	Td library.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
ENTRY static void
TransferInProc(ttyPtr, callInfoPtr)
    register DevTty *ttyPtr;		/* Our information about terminal. */
    Proc_CallInfo *callInfoPtr;		/* Not used. */
{
    int value, next;
    char c;

    LOCK_MONITOR;

    /*
     * If the terminal is no longer open then don't do anything:  there's
     * no Td_Terminal to do it on.
     */

    if (ttyPtr->openCount == 0) {
	UNLOCK_MONITOR;
	return;
    }

    while (ttyPtr->extractInput != ttyPtr->insertInput) {
	value = ttyPtr->inBuffer[ttyPtr->extractInput];
	next = ttyPtr->extractInput + 1;
	if (next >= TTY_IN_BUF_SIZE) {
	    next = 0;
	}
	ttyPtr->extractInput = next;
	if (ttyPtr->inputProc != (void (*)()) NIL) {
	    (*ttyPtr->inputProc)(ttyPtr->inputData, value);
	} else {
	    if (value == DEV_TTY_BREAK) {
		if (ttyPtr->consoleFlags & DEV_TTY_IS_CONSOLE) {
		    ttyPtr->consoleFlags |= DEV_TTY_GOT_BREAK;
		} else {
		    Td_ControlRaw(ttyPtr->term, TD_BREAK);
		}
	    } else {
		if (value == DEV_TTY_HANGUP) {
		    Td_ControlRaw(ttyPtr->term, TD_LOST_CARRIER);
		} else if ((ttyPtr->consoleFlags
			& (DEV_TTY_GOT_BREAK|DEV_TTY_IS_CONSOLE))
			== (DEV_TTY_GOT_BREAK|DEV_TTY_IS_CONSOLE)) {
		    Dev_InvokeConsoleCmd(value);
		} else {
		    c = value;
		    Td_PutRaw(ttyPtr->term, 1, &c);
		}
		ttyPtr->consoleFlags &= ~DEV_TTY_GOT_BREAK;
	    }
	}
	if (ttyPtr->consoleFlags & DEV_TTY_IS_CONSOLE) {
	    Timer_GetTimeOfDay(&dev_LastConsoleInput, (int *) NIL,
		    (Boolean *) NIL);
	}
    }
    ttyPtr->consoleFlags &= ~DEV_TTY_OVERFLOWED;

    UNLOCK_MONITOR;
}

/*
 *----------------------------------------------------------------------
 *
 * DevTtyOutputChar --
 *
 *	This procedure is called at interrupt level to retrieve the
 *	next character ready for output, if there are any.
 *
 * Results:
 *	The return value is either the next ASCII character ready
 *	for output (returned unsigned in an int), or -1 to indicate
 *	that the output buffer is empty.
 *
 * Side effects:
 *	Characters are removed from ttyPtr's outBuffer, and a background-
 *	level procedure call may be scheduled to refill the buffer.
 *
 *----------------------------------------------------------------------
 */

int
DevTtyOutputChar(ttyPtr)
    register DevTty *ttyPtr;	/* Information about the terminal. */
{
    int result, next;

    if (ttyPtr->extractOutput == ttyPtr->insertOutput) {
	return -1;
    }
    result = ttyPtr->outBuffer[ttyPtr->extractOutput];
    next = ttyPtr->extractOutput + 1;
    if (next >= TTY_OUT_BUF_SIZE) {
	next = 0;
    }
    ttyPtr->extractOutput = next;
    if (ttyPtr->extractOutput == ttyPtr->insertOutput) {
	Proc_CallFunc(TransferOutProc, (ClientData) ttyPtr, 0);
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * TransferOutProc --
 *
 *	This procedure is invoked in a kernel server process in response
 *	to a Proc_CallFunc arrangement made by DevTtyOutputChar.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Characters get transferred from Td's output buffer to the
 *	intermediate buffer for Td.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
ENTRY static void
TransferOutProc(ttyPtr, callInfoPtr)
    register DevTty *ttyPtr;		/* Our information about terminal. */
    Proc_CallInfo *callInfoPtr;		/* Not used. */
{
    LOCK_MONITOR;

    /*
     * Make sure that the terminal is still open.  Otherwise its Td_Terminal
     * will have gone away.
     */

    if (ttyPtr->openCount != 0) {
	Transfer(ttyPtr);
    }

    UNLOCK_MONITOR;
}

/*
 *----------------------------------------------------------------------
 *
 * Transfer --
 *
 *	Do all the real work of copying characters from the Td buffer
 *	for a terminal to the intermediate buffer in ttyPtr.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Characters get copied to the intermediate buffer.  If there's
 *	really anything to copy, then the device-level rawProc gets
 *	called to start up output.
 *
 *----------------------------------------------------------------------
 */

INTERNAL static void
Transfer(ttyPtr)
    register DevTty *ttyPtr;	/* Information about terminal.  Caller must
				 * have locked ttyPtr. */
{
    int count, next, tmp, oldDiff;

    /*
     * Transfer characters in the largest possible blocks such that each
     * block fits in the buffer without wrap-around.
     */

    oldDiff = ttyPtr->insertOutput - ttyPtr->extractOutput;
    while (TRUE) {
	count = ttyPtr->extractOutput - ttyPtr->insertOutput - 1;
	if (count < 0) {
	    count += TTY_OUT_BUF_SIZE;
	}
	if (count > (TTY_OUT_BUF_SIZE - ttyPtr->insertOutput)) {
	    count = TTY_OUT_BUF_SIZE - ttyPtr->insertOutput;
	}
	if (count <= 0) {
	    break;
	}
	tmp = Td_GetRaw(ttyPtr->term, count,
		&ttyPtr->outBuffer[ttyPtr->insertOutput]);
	next = ttyPtr->insertOutput + tmp;
	if (next >= TTY_OUT_BUF_SIZE) {
	    next = 0;
	}
	ttyPtr->insertOutput = next;
	if (tmp < count) {
	    break;
	}
    }

    /*
     * Wake up the device if there didn't used to be characters in the
     * buffer but there are now.
     */

    if ((oldDiff == 0) && (ttyPtr->insertOutput != ttyPtr->extractOutput)) {
	(*ttyPtr->rawProc)(ttyPtr->rawData, TD_RAW_OUTPUT_READY,
		0, (char *) NIL, 0, (char *) NIL);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Signal --
 *
 *	This is a utility procedure that converts a signal from UNIX to
 *	Sprite and sends it to a process or group.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A signal get sent.
 *
 *----------------------------------------------------------------------
 */

static void
Signal(sigNum, id, family)
    int sigNum;				/* UNIX signal number to send. */
    Proc_PID id;			/* Id of process or family. */
    Boolean family;			/* TRUE means id is for family,
					 * FALSE means it's for process. */
{
    int spriteSignal;

    if (Compat_UnixSignalToSprite(sigNum, &spriteSignal) != SUCCESS) {
	printf("Warning: %s %d to Sprite.\n",
		"devTty.Signal couldn't translate sigNum", sigNum);
    } else {
	(void) Sig_Send(spriteSignal, SIG_NO_CODE, id, family);
    }
}
