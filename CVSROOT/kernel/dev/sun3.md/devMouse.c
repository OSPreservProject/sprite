/* 
 * devMouse.c --
 *
 *	This file implements the "mouse" device, which is used by
 *	window systems to find out about keyboard and mouse events.
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
#include <dev.h>
#include <devAddrs.h>
#include <dev/mouse.h>
#include <errno.h>
#include <mouse.h>
#include <fs.h>
#include <fsio.h>
#include <list.h>
#include <status.h>
#include <stdlib.h>
#include <timer.h>
#include <tty.h>
#include <ttyAttach.h>
#include <z8530.h>
#include <console.h>
#include <user/bstring.h>
#include <fsioDevice.h>
/*
 * For synchronization, use devTty's monitor lock (this module is
 * so closely intertwined with devTty.c that this is the only logical
 * approach).
 */

#define LOCKPTR (&devTtyLock)

/*
 * One of the structures below is dynamically allocated for each
 * input event.
 */

typedef struct {
    List_Links links;		/* Pointers to next & previous items. */
    Mouse_Event event;		/* Event. */
} Event;

static List_Links eventList;	/* List of pending input events. */
static int listCount;		/* Number of elements in list. */
#define MAX_LIST_LENGTH 500

/*
 * Other information about mouse device.  This file is strongly
 * entwined with devTty.c in that this file only uses SOME of the
 * devTty features (namely the ability to buffer input characters
 * and process them at background level).  The output portion of
 * the mouse channel is not used:  output on the mouse device goes
 * to the keyboard channel.
 */

static DevTty mouseTty;			/* Information used by devTty.c. */
static DevZ8530 mouse = {		/* Information used by device driver. */
    "mouse",				/* name */
    (DevZ8530Device *) DEV_MOUSE_ADDR,	/* address */
    &mouseTty,				/* ttyPtr */
    DEV_UART_VECTOR,			/* vector */
    1200,				/* baud */
    WRITE3_RX_8BIT,			/* wr3 */
    WRITE5_TX_8BIT,			/* wr5 */
    DevTtyInputChar,			/* inputProc */
    (ClientData) &mouseTty,		/* inputData */
    DevTtyOutputChar,			/* outputProc */
    (ClientData) &mouseTty,		/* outputData */
    0,					/* oldRr0 */
    Z_CHANNEL_B | Z_INACTIVE		/* flags */
};

static Fs_NotifyToken token;		/* Used for Fs call-backs to wake up
					 * waiting processes. */
static volatile int outputBuffer = -1;	/* One-character output buffer shared
					 * without explicit synchronization
					 * between background and interrupt-
					 * level procedures.  -1 means buffer
					 * empty;  otherwise it contains a
					 * single character to be output. */
static DevZ8530 *keyboardPtr;		/* Information about the keyboard's
					 * UART (needed to start up output). */

/*
 * Forward declarations to procedures declared later in this file:
 */

static void MouseDelayedClose _ARGS_((ClientData clientData,
                                     Proc_CallInfo *callInfoPtr));
static void MouseInputProc _ARGS_((ClientData dummy, int value));
static void KbdInputProc _ARGS_((ClientData dummy, int value));
static int MouseOutputProc _ARGS_((void));


/*
 *----------------------------------------------------------------------
 *
 * DevMouseInit --
 *
 *	Called during bootstrapping to initialize mouse-related
 *	things.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Resets the serial device.
 *
 *----------------------------------------------------------------------
 */

void
DevMouseInit()
{
    DevZ8530RawProc(&mouse, TD_RAW_SHUTDOWN, 0, (char *) NULL,
	    0, (char *) NULL);
}

/*
 *----------------------------------------------------------------------
 *
 * DevMouseOpen --
 *
 *	Called through devFsOpTable to open the mouse device.
 *	Initializes the device and activates it so that it's ready
 *	to return input.
 *
 * Results:
 *	A standard Sprite ReturnStatus.
 *
 * Side effects:
 *	The mouse-related devices will be "turned on" if they aren't
 *	already, which may involve setting up interrupt handlers.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
ENTRY ReturnStatus
DevMouseOpen(devicePtr, useFlags, notifyToken, flagsPtr)
    Fs_Device *devicePtr;	/* Information about device (e.g. type
				 * and unit number). */
    int useFlags;		/* Flags for the stream being opened:
				 * OR'ed combination of FS_READ and
				 * FS_WRITE. */
    Fs_NotifyToken notifyToken;	/* Used for Fs call-back to notify waiting
				 * processes that the terminal is ready. */
    int *flagsPtr;	        /* OUT: Device IO flags */
{
    if (devicePtr->unit != 0) {
	return Compat_MapToSprite(ENXIO);
    }

    LOCK_MONITOR;

    mouseTty.openCount += 1;
    if (mouseTty.openCount == 1) {
	token = notifyToken;
	keyboardPtr = DevGrabKeyboard(KbdInputProc, (ClientData) 0,
		MouseOutputProc, (ClientData) 0);
	mouseTty.insertOutput = 0;
	mouseTty.extractOutput = 0;
	mouseTty.rawProc = DevZ8530RawProc;
	mouseTty.activateProc = DevZ8530Activate;
	mouseTty.rawData = (ClientData) &mouse;
	mouseTty.inputProc = MouseInputProc;
	mouseTty.inputData = 0;
	mouseTty.consoleFlags = 0;
	List_Init(&eventList);
	(*mouseTty.activateProc)(&mouse);
    }

    UNLOCK_MONITOR;
    return SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * DevMouseRead --
 *
 *	Called through devFsOpTable to read from the mouse device.
 *
 * Results:
 *	A standard Sprite ReturnStatus.  Characters are stored at
 *	*readPtr->buffer, and the fields of *replyPtr are modified
 *	to describe what happened.  The mouse must be read in even
 *	multiples of the size of Mouse_Event.
 *
 * Side effects:
 *	Information may be removed from the mouse input queue.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
ENTRY ReturnStatus
DevMouseRead(devicePtr, readPtr, replyPtr)
    Fs_Device *devicePtr;	/* Information about device. */
    register Fs_IOParam *readPtr;	/* Input parameters. */
    register Fs_IOReply *replyPtr;	/* Place to store return information. */
{
    int stillToDo;
    char *bufPtr;

    replyPtr->length = 0;
    stillToDo = readPtr->length;
    bufPtr = (char *) readPtr->buffer;

    /*
     * Make sure the buffer is large enough to hold at least one
     * event.
     */

    if (stillToDo < sizeof(Mouse_Event)) {
	return Compat_MapToSprite(EINVAL);
    }

    LOCK_MONITOR;


    if (List_IsEmpty(&eventList)) {
	UNLOCK_MONITOR;
	return Compat_MapToSprite(EWOULDBLOCK);
    }
    while ((stillToDo >= sizeof(Mouse_Event)) && !List_IsEmpty(&eventList)) {
	register Event *eventPtr;

	eventPtr = (Event *) List_First(&eventList);
	List_Remove(&eventPtr->links);
	bcopy((char *) &eventPtr->event, (char *) bufPtr, sizeof(Mouse_Event));
	bufPtr += sizeof(Mouse_Event);
	stillToDo -= sizeof(Mouse_Event);
	replyPtr->length += sizeof(Mouse_Event);
	free((char *) eventPtr);
	listCount -= 1;
    }
    UNLOCK_MONITOR;
    return SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * DevMouseWrite --
 *
 *	Called through devFsOpTable to write to the mouse device.
 *
 * Results:
 *	A standard Sprite ReturnStatus.  Fields of *replyPtr are
 *	modified to indicate what happened in the write operation.
 *
 * Side effects:
 *	Information may be added to the output buffer for the keyboard.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
ReturnStatus
DevMouseWrite(devicePtr, writePtr, replyPtr)
    Fs_Device *devicePtr;		/* Information about device. */
    register Fs_IOParam *writePtr;	/* Input parameters. */
    register Fs_IOReply *replyPtr;	/* Place to store result info. */
{
    ReturnStatus status = SUCCESS;
    int stillToDo;
    char *bufPtr;

    LOCK_MONITOR;

    replyPtr->length = 0;
    stillToDo = writePtr->length;
    bufPtr = (char *) writePtr->buffer;
    while (stillToDo != 0) {
	if (outputBuffer != -1) {
	    status = FS_WOULD_BLOCK;
	    break;
	}
	outputBuffer = (*((char *) bufPtr)) & 0xff;
	bufPtr++;
	stillToDo--;
	replyPtr->length++;
	DevZ8530RawProc(keyboardPtr, TD_RAW_OUTPUT_READY, 0, (char *) NIL,
		0, (char *) NIL);
    }
    UNLOCK_MONITOR;
    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * DevMouseIOControl --
 *
 *	Called through devFsOpTable to perform IOControl operations on
 *	the mouse device.
 *
 * Results:
 *	A standard Sprite ReturnStatus.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
ReturnStatus
DevMouseIOControl(devicePtr, iocPtr, replyPtr)
    Fs_Device *devicePtr;		/* Information about device. */
    Fs_IOCParam *iocPtr;		/* Parameter information (buffer sizes
					 * etc.). */
    Fs_IOReply *replyPtr;		/* Place to store result information. */
{
    return Compat_MapToSprite(ENOTTY);
}

/*
 *----------------------------------------------------------------------
 *
 * DevMouseSelect --
 *
 *	Called through devFsOpTable to perform select-related functions
 *	on the mouse device.
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

/* ARGSUSED */
ENTRY ReturnStatus
DevMouseSelect(devicePtr, readPtr, writePtr, exceptPtr)
    Fs_Device *devicePtr;	/* Information about device. */
    int *readPtr;		/* Set to zero if device not readable. */
    int *writePtr;		/* Set to zero if device not writable. */
    int *exceptPtr;		/* Set to zero if no exception pending on
				 * device. */
{
    LOCK_MONITOR;
    if (List_IsEmpty(&eventList)) {
	*readPtr = 0;
    }
    if (outputBuffer != -1) {
	*writePtr = 0;
    }
    *exceptPtr = 0;
    UNLOCK_MONITOR;
    return SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * DevMouseClose --
 *
 *	Called through devFsOpTable when the mouse device is closed.
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
ReturnStatus
DevMouseClose(devicePtr, useFlags, openCount, writerCount)
    Fs_Device *devicePtr;		/* Information about device. */
    int useFlags;			/* Indicates whether stream being
					 * closed was open for reading and/or
					 * writing:  OR'ed combination of
					 * FS_READ and FS_WRITE. */
    int openCount;			/* # of opens still active for this
					 * device. */
    int writerCount;			/* # of active opens that permit
					 * writing. */
{
    LOCK_MONITOR;
    mouseTty.openCount -= 1;
    if (mouseTty.openCount == 0) {
	while (!List_IsEmpty(&eventList)) {
	    List_Links *eventPtr;
    
	    eventPtr = List_First(&eventList);
	    List_Remove(eventPtr);
	    free((char *) eventPtr);
	}
    
	/*
	 * If I/O is still in progress, delay shutting down the device
	 * until the I/O completes.
	 */
    
	if (outputBuffer == -1) {
	    DevReleaseKeyboard();
	    (void) (*mouseTty.rawProc)(mouseTty.rawData, TD_RAW_SHUTDOWN, 0,
		    (char *) NULL, 0, (char *) NULL);
	}
    }
    UNLOCK_MONITOR;
    return SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * MouseDelayedClose --
 *
 *	Background-level procedure to complete the device close if
 *	it couldn't be completed at close-time because I/O was still
 *	in progress.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The mouse gets shutdown and the keyboard reverts to normal
 *	operation.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static void
MouseDelayedClose(clientData, callInfoPtr)
    ClientData     clientData;		/* Not used. */
    Proc_CallInfo *callInfoPtr;         /* Not used. */
{
    LOCK_MONITOR;
    if (mouseTty.openCount == 0) {
	DevReleaseKeyboard();
	(void) (*mouseTty.rawProc)(mouseTty.rawData, TD_RAW_SHUTDOWN, 0,
		(char *) NULL, 0, (char *) NULL);
    }
    UNLOCK_MONITOR;
}

/*
 *----------------------------------------------------------------------
 *
 * DevMouseInterrupt --
 *
 *	This procedure is invoked whenever an interrupt occurs
 *	for one of the Z8530 chips.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The Z8530 interrupt handler gets invoked to see if anything
 *	happened related to the mouse UART.
 *
 *----------------------------------------------------------------------
 */

void
DevMouseInterrupt()
{
    DevZ8530Interrupt((ClientData)&mouse);
}

/*
 *----------------------------------------------------------------------
 *
 * MouseInputProc --
 *
 *	This procedure is invoked at background level (i.e in a kernel
 *	process) to handle input characters from the mouse UART.  The
 *	caller is expected to have acquired the tty monitor lock.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The input character is converted to an event, which is then
 *	enqueued on an input queue.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
INTERNAL static void
MouseInputProc(dummy, value)
    ClientData dummy;		/* Not used. */
    int value;			/* Character that arrived from UART. */
{
    char c;
    Time time;
    Event *eventPtr;

    /*
     * Mouse characters arrive in bunches, with the first character
     * of each bunch specially marked.  A simple state machine here
     * keeps track of where we are in the bunch.
     */

#define WAIT_SYNC	0
#define WAIT_DELTA_X1	1
#define WAIT_DELTA_Y1	2
#define WAIT_DELTA_X2	3
#define WAIT_DELTA_Y2	4

#define NUM_STATES	5
#define SYNC_BIT 	0x80

    static int curState = WAIT_SYNC;
    static Mouse_Event event;

    /*
     * Discard any special characters (breaks, etc.).
     */

    if (DEV_TTY_IS_CONTROL(value)) {
	return;
    }
    c = value;

    /*
     * The check below keeps our state machine back in sync with the
     * mouse by discarding characters up to the beginning of the next
     * sequence.
     */

    if ((curState == WAIT_SYNC) && !(c & SYNC_BIT)) {
	return;
    }
    switch (curState) {
	case WAIT_SYNC:
	    event.key = c & 0x7;
	    break;
	case WAIT_DELTA_X1:
	    event.deltaX = (int) c;
	    break;
	case WAIT_DELTA_Y1:
	    event.deltaY = (int) c;
	    break;
	case WAIT_DELTA_X2:
	    event.deltaX += (int) c;
	    break;
	case WAIT_DELTA_Y2:
	    event.deltaY += (int) c;
	    event.flags = MOUSE_EVENT;
	    Timer_GetRealTimeOfDay(&time, (int *) NIL, (Boolean *) NIL);
	    event.time = (time.seconds*1000) + (time.microseconds/1000);
	    eventPtr = (Event *) malloc(sizeof(Event));
	    List_InitElement(&eventPtr->links);
	    eventPtr->event = event;
	    List_Insert(&eventPtr->links, LIST_ATREAR(&eventList));
	    dev_LastConsoleInput = time;
	    Fsio_DevNotifyReader(token);
	    break;
    }
    curState++;
    if (curState >= NUM_STATES) {
	curState = 0;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * KbdInputProc --
 *
 *	This procedure is invoked at background level (i.e in a kernel
 *	process) to handle input characters from the keyboard UART.
 *	This procedure is invoked only when the mouse device is open.
 *	The caller is expected to have acquired the tty monitor lock.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The input character is converted to an event, which is then
 *	enqueued on an input queue.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
INTERNAL static void
KbdInputProc(dummy, value)
    ClientData dummy;		/* Not used. */
    int value;			/* Character that arrived from UART:
				 * identifies which key, and whether up
				 * or down transition. */
{
    register Event *eventPtr;
    Time time;

    /*
     * Throw away control events and anything corresponding to a console
     * command.
     */

    if (DEV_TTY_IS_CONTROL(value)) {
	return;
    }
    if (DevConsoleConvertKeystroke(value) == -2) {
	return;
    }

    eventPtr = (Event *) malloc(sizeof(Event));
    List_InitElement(&eventPtr->links);
    eventPtr->event.flags = KEYBOARD_EVENT;
    if (value & 0x80) {
	eventPtr->event.flags |= KEY_UP;
    }
    eventPtr->event.key = value & 0x7f;
    eventPtr->event.deltaX = 0;
    eventPtr->event.deltaY = 0;
    Timer_GetRealTimeOfDay(&time, (int *) NIL, (Boolean *) NIL);
    eventPtr->event.time = (time.seconds*1000) + (time.microseconds/1000);
    List_Insert(&eventPtr->links, LIST_ATREAR(&eventList));
    dev_LastConsoleInput = time;
    Fsio_DevNotifyReader(token);
}

/*
 *----------------------------------------------------------------------
 *
 * MouseOutputProc --
 *
 *	This procedure is called at interrupt level to fetch the
 *	next output character for the mouse device (these output
 *	characters go to the keyboard).
 *
 * Results:
 *	Returns the next output character, or -1 if the output buffer
 *	is empty.
 *
 * Side effects:
 *	May wake up processes waiting to write to this device because
 *	the output buffer was full.
 *
 *----------------------------------------------------------------------
 */

static int
MouseOutputProc()
{
    int result;

    result = outputBuffer;
    if (result != -1) {
	outputBuffer = -1;
	if (mouseTty.openCount > 0) {
	    Fsio_DevNotifyWriter(token);
	} else {
	    Proc_CallFunc(MouseDelayedClose, (ClientData) NIL, 0);
	}
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * Dev_ConsoleReset --
 *
 *	This procedure is called as a console command procedure to
 *	switch console keyboard input back and forth between /dev/mouse
 *	and /dev/console.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
Dev_ConsoleReset(toConsole)
    int toConsole;			/* 1 means make keystrokes go to
					 * console.  0 means make keystrokes
					 * go to mouse device. */
{
    if (toConsole) {
	DevReleaseKeyboard();
    } else {
	keyboardPtr = DevGrabKeyboard(KbdInputProc, (ClientData) 0,
		MouseOutputProc, (ClientData) 0);
    }
}
