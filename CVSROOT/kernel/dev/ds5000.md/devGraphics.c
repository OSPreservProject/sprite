/* 
 *  devGraphics.c --
 *
 *     	This file contains machine-dependent routines for the graphics device.
 *
 *	Copyright (C) 1989 Digital Equipment Corporation.
 *	Permission to use, copy, modify, and distribute this software and
 *	its documentation for any purpose and without fee is hereby granted,
 *	provided that the above copyright notice appears in all copies.  
 *	Digital Equipment Corporation makes no representations about the
 *	suitability of this software for any purpose.  It is provided "as is"
 *	without express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (DECWRL)";
#endif not lint

#include "sprite.h"
#include "machMon.h"
#include "mach.h"
#include "dev.h"
#include "fs.h"
#include "fsio.h"
#include "sys.h"
#include "sync.h"
#include "timer.h"
#include "dbg.h"
#include "machAddrs.h"
#include "console.h"
#include "dc7085.h"
#include "graphics.h"
#include "vm.h"
#include "vmMach.h"
#include "dev/graphics.h"

/*
 * Macro to translate from a time struct to milliseconds.
 */
#define TO_MS(time) ((time.seconds * 1000) + (time.microseconds / 1000))

/*
 * System control status register pointer.
 */
static unsigned short	*sysCSRPtr = (unsigned short *)MACH_CSR_ADDR;


static volatile	unsigned short	curReg = 0;	/* Register to keep track of
						 * the pcc command register
						 * bits. */
Boolean	devGraphicsOpen = FALSE;		/* TRUE => the mouse is open.*/
					/* Process waiting for select.*/
static Boolean	isMono;			/* TRUE */

/*
 * These need to mapped into user space.
 */
static DevScreenInfo	scrInfo;
static DevEvent		events[DEV_MAXEVQ] = {0};	
static DevTimeCoord	tcs[MOTION_BUFFER_SIZE] = {0};


static unsigned short	cursorBits [32];

Boolean			inKBDReset = FALSE;

MouseReport		lastRep;
MouseReport		currentRep;


/*
 * Keyboard defaults.
 */
static short divDefaults[15] = { 
	LK_DOWN,	/* 0 doesn't exist */
	LK_AUTODOWN, 
	LK_AUTODOWN, 
	LK_AUTODOWN, 
	LK_DOWN,
	LK_UPDOWN,   
	LK_UPDOWN,   
	LK_AUTODOWN, 
	LK_AUTODOWN, 
	LK_AUTODOWN, 
	LK_AUTODOWN, 
	LK_AUTODOWN, 
	LK_AUTODOWN, 
	LK_DOWN, 
	LK_AUTODOWN 
};

/* 
 * Keyboard initialization string.
 */
short kbdInitString[] = {		/* reset any random keyboard stuff */
	LK_AR_ENABLE,			/* we want autorepeat by default */
	LK_CL_ENABLE,			/* keyclick */
	0x84,				/* keyclick volume */
	LK_KBD_ENABLE,			/* the keyboard itself */
	LK_BELL_ENABLE,			/* keyboard bell */
	0x84,				/* bell volume */
	LK_LED_DISABLE,			/* keyboard leds */
	LED_ALL
};

#define KBD_INIT_LENGTH	sizeof(kbdInitString) / sizeof(short)



static Boolean initialized = FALSE;

/*
 * Mutex to synchronize access.
 */
Sync_Semaphore graphicsMutex;

/*
 * Token used to notify process that has graphics device open that events
 * are available.
 */
ClientData	notifyToken;

/*
 * Redefine MASTER_LOCK to be DISABLE_INTR for two reasons.  First, it
 * is more efficient and sufficient on a uni-processor.  Second, MASTER_LOCK
 * can cause deadlock because this file contains the routine which blits
 * a character to the screen.  As a result no routine in here can do a printf
 * underneath the MASTER_LOCK because the Blitc routine grabs the master lock.
 * Once things are debugged and the printfs are removed it should be OK to use
 * a TRUE master lock.
 */
#ifdef MASTER_LOCK
#undef MASTER_LOCK
#undef MASTER_UNLOCK
#endif
#define MASTER_LOCK(mutexPtr)	DISABLE_INTR()
#define MASTER_UNLOCK(mutexPtr)	ENABLE_INTR()

/*
 * Forward references.
 */
static void		InitScreenDefaults();
static void		ScreenInit();
static void		LoadCursor();
static void		RestoreCursorColor();
static void		CursorColor();
static void		MouseInit();
static void		KBDReset();
static void		InitColorMap();
static void		VDACInit();
static void		LoadColorMap();
static void 		RecvIntr();
static void		MouseEvent();
static void		MouseButtons();
static void		PosCursor();
static void		XmitIntr();
static void		Scroll();
static void		Blitc();


/*
 * ----------------------------------------------------------------------------
 *
 * DevGraphicsInit --
 *
 *	Initialize the mouse and the screen.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The world is initialized.
 *
 * ----------------------------------------------------------------------------
 */
void
DevGraphicsInit()
{
    Time	time;

    Sync_SemInitDynamic(&graphicsMutex, "graphicsMutex");
    MouseInit();
    if(!inKBDReset) {
	KBDReset();
    }

    initialized = TRUE;
}    



/*
 * ----------------------------------------------------------------------------
 *
 * MouseInit --
 *
 *	Initialize the mouse.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */
static void
MouseInit()
{
    int	id_byte1, id_byte2, id_byte3, id_byte4;

    /*
     * Initialize the mouse.
     */
    DevDC7085MouseInit();
    DevDC7085MousePutCh(MOUSE_SELF_TEST);
    id_byte1 = DevDC7085MouseGetCh();
    if (id_byte1 < 0) {
	printf("MouseInit: Timeout on 1st byte of self-test report\n");
	return;
    }
    id_byte2 = DevDC7085MouseGetCh();
    if (id_byte2 < 0) {
	printf("MouseInit: Timeout on 2nd byte of self-test report\n");
	return;
    }
    id_byte3 = DevDC7085MouseGetCh();
    if (id_byte3 < 0) {
	printf("MouseInit: Timeout on 3rd byte of self-test report\n");
	return;
    }
    id_byte4 = DevDC7085MouseGetCh();
    if (id_byte4 < 0) {
	printf("MouseInit: Timeout on 4th byte of self-test report\n");
	return;
    }
    if ((id_byte2 & 0x0f) != 0x2) {
	printf("MouseInit: We don't have a mouse!!!\n");
    }
    DevDC7085MousePutCh(MOUSE_INCREMENTAL);
}


/*
 * ----------------------------------------------------------------------------
 *
 * KBDReset --
 *
 *	Reset the keyboard to default characteristics.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */
static void
KBDReset()
{
    register int i;

    inKBDReset = TRUE;
    DevDC7085KBDPutc(LK_DEFAULTS);
    for (i=1; i < 15; i++) {
	DevDC7085KBDPutc(divDefaults[i] | (i << 3));
    }
    for (i = 0; i < KBD_INIT_LENGTH; i++) {
	DevDC7085KBDPutc ((int)kbdInitString[i]);
    }
    inKBDReset = FALSE;
}

static Boolean	consoleCmdDown = FALSE;


/*
 *----------------------------------------------------------------------
 *
 * DevGraphicsKbdIntr --
 *
 *	Process a received character.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Events added to the queue.
 *
 *----------------------------------------------------------------------
 */
void
DevGraphicsKbdIntr(ch)
    unsigned char ch;
{
    int		i;
    Time	time;
    DevEvent	*eventPtr;
    
    if (ch == LK_POWER_ERROR || ch == LK_KDOWN_ERROR ||
	ch == LK_INPUT_ERROR || ch == LK_OUTPUT_ERROR) {
	if(!inKBDReset) {
	    printf("\n: RecvIntr: keyboard error,code=%x", ch);
	    KBDReset();
	}
	return;
    }
    if (ch < LK_LOWEST) {
       return;
    }
    if (ch == KEY_UP) {
	consoleCmdDown = FALSE;
    } else if (ch == KEY_COMMAND) {
	consoleCmdDown = TRUE;
    } else if (consoleCmdDown) {
	char asciiChar;

	consoleCmdDown = FALSE;
	asciiChar = DevDC7085TranslateKey(ch, FALSE, FALSE);
	if (asciiChar != -1) {
	    Dev_InvokeConsoleCmd(asciiChar);
	    return;
	}
    }

    /*
     * See if there is room in the queue.
     */
    i = DEV_EVROUND(scrInfo.eventQueue.eTail + 1);
    if (i == scrInfo.eventQueue.eHead) {
	return;
    }

    /*
     * Add the event to the queue.
     */
    eventPtr = &events[scrInfo.eventQueue.eTail];
    eventPtr->type = DEV_BUTTON_RAW_TYPE;
    eventPtr->device = DEV_KEYBOARD_DEVICE;
    eventPtr->x = scrInfo.mouse.x;
    eventPtr->y = scrInfo.mouse.y;
    Timer_GetRealTimeOfDay(&time, (int *) NIL, (Boolean *) NIL);
    eventPtr->time = TO_MS(time);
    eventPtr->key = ch;
    scrInfo.eventQueue.eTail = i;
    dev_LastConsoleInput = time;
    Fsio_DevNotifyReader(notifyToken);
}


/*
 *----------------------------------------------------------------------
 *
 * DevGraphicsMouseIntr --
 *
 *	Process a received mouse character.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Events added to the queue.
 *
 *----------------------------------------------------------------------
 */
void
DevGraphicsMouseIntr(ch)
    unsigned char ch;
{
    MouseReport	*newRepPtr;

    newRepPtr = &currentRep;
    newRepPtr->byteCount++;
    if (ch & MOUSE_START_FRAME) {
	/*
	 * The first mouse report byte (button state).
	 */
	newRepPtr->state = ch;
	if (newRepPtr->byteCount > 1) {
	    newRepPtr->byteCount = 1;
	}
    } else if (newRepPtr->byteCount == 2) {
	/*
	 * The second mouse report byte (delta x).
	 */
	newRepPtr->dx = ch;
    } else if (newRepPtr->byteCount == 3) {
	/*
	 * The final mouse report byte (delta y).
	 */
	newRepPtr->dy = ch;
	newRepPtr->byteCount = 0;
	if (newRepPtr->dx != 0 || newRepPtr->dy != 0) {
	    /*
	     * If the mouse moved, post a motion event.
	     */
	    MouseEvent(newRepPtr);
	}
	MouseButtons(newRepPtr);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * MouseEvent --
 *
 *	Process a mouse event.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	An event is added to the event queue.
 *
 *----------------------------------------------------------------------
 */
static void
MouseEvent(newRepPtr) 
    MouseReport	*newRepPtr;
{
    Time	time;
    unsigned	milliSec;
    int		i;
    DevEvent	*eventPtr;

    Timer_GetRealTimeOfDay(&time, (int *) NIL, (Boolean *) NIL);
    milliSec = TO_MS(time);

    /*
     * Check to see if we have to accelerate the mouse
     */
    if (scrInfo.mscale >=0) {
	if (newRepPtr->dx >= scrInfo.mthreshold) {
	    newRepPtr->dx +=
		    (newRepPtr->dx - scrInfo.mthreshold) * scrInfo.mscale;
	}
	if (newRepPtr->dy >= scrInfo.mthreshold) {
	    newRepPtr->dy +=
		(newRepPtr->dy - scrInfo.mthreshold) * scrInfo.mscale;
	}
    }
    
    /*
     * Update mouse position
     */
    if( newRepPtr->state & MOUSE_X_SIGN) {
	scrInfo.mouse.x += newRepPtr->dx;
	if (scrInfo.mouse.x > scrInfo.maxCurX) {
	    scrInfo.mouse.x = scrInfo.maxCurX;
	}
    } else {
	scrInfo.mouse.x -= newRepPtr->dx;
	if (scrInfo.mouse.x < scrInfo.minCurX) {
	    scrInfo.mouse.x = scrInfo.minCurX;
	}
    }
    if( newRepPtr->state & MOUSE_Y_SIGN) {
	scrInfo.mouse.y -= newRepPtr->dy;
	if (scrInfo.mouse.y < scrInfo.minCurY) {
	    scrInfo.mouse.y = scrInfo.minCurY;
	}
    } else {
	scrInfo.mouse.y += newRepPtr->dy;
	if (scrInfo.mouse.y > scrInfo.maxCurY) {
	    scrInfo.mouse.y = scrInfo.maxCurY;
	}
    }
    /*
     * Store the motion event in the motion buffer.
     */
    tcs[scrInfo.eventQueue.tcNext].time = milliSec;
    tcs[scrInfo.eventQueue.tcNext].x = scrInfo.mouse.x;
    tcs[scrInfo.eventQueue.tcNext].y = scrInfo.mouse.y;
    scrInfo.eventQueue.tcNext++;
    if (scrInfo.eventQueue.tcNext >= MOTION_BUFFER_SIZE) {
	scrInfo.eventQueue.tcNext = 0;
    }
    if (scrInfo.mouse.y < scrInfo.mbox.bottom &&
	scrInfo.mouse.y >=  scrInfo.mbox.top &&
	scrInfo.mouse.x < scrInfo.mbox.right &&
	scrInfo.mouse.x >=  scrInfo.mbox.left) {
	return;
    }

    scrInfo.mbox.bottom = 0;
    if (DEV_EVROUND(scrInfo.eventQueue.eTail + 1) == scrInfo.eventQueue.eHead) {
	return;
    }

    i = DEV_EVROUND(scrInfo.eventQueue.eTail -1);
    if ((scrInfo.eventQueue.eTail != scrInfo.eventQueue.eHead) && 
        (i != scrInfo.eventQueue.eHead)) {
	DevEvent	*eventPtr;
	eventPtr = &events[i];
	if(eventPtr->type == DEV_MOTION_TYPE) {
	    eventPtr->x = scrInfo.mouse.x;
	    eventPtr->y = scrInfo.mouse.y;
	    eventPtr->time = milliSec;
	    eventPtr->device = DEV_MOUSE_DEVICE;
	    return;
	}
    }
    /*
     * Put event into queue and wakeup any waiters.
     */
    eventPtr = &events[scrInfo.eventQueue.eTail];
    eventPtr->type = DEV_MOTION_TYPE;
    eventPtr->time = milliSec;
    eventPtr->x = scrInfo.mouse.x;
    eventPtr->y = scrInfo.mouse.y;
    eventPtr->device = DEV_MOUSE_DEVICE;
    scrInfo.eventQueue.eTail = DEV_EVROUND(scrInfo.eventQueue.eTail + 1);
    dev_LastConsoleInput = time;
    if (devGraphicsOpen) {
	Fsio_DevNotifyReader(notifyToken);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * MouseButtons --
 *
 *	Process mouse buttons.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static void
MouseButtons(newRepPtr)
    MouseReport	*newRepPtr;
{
    static char temp, oldSwitch, newSwitch;
    int		i, j;
    DevEvent	*eventPtr;
    Time	time;

    newSwitch = newRepPtr->state & 0x07;
    oldSwitch = lastRep.state & 0x07;
    
    temp = oldSwitch ^ newSwitch;
    if (temp != 0) {
	for (j = 1; j < 8; j <<= 1) {
	    if ((j & temp) == 0) {
		continue;
	    }

	    /*
	     * Check for room in the queue
	     */
	    i = DEV_EVROUND(scrInfo.eventQueue.eTail+1);
	    if (i == scrInfo.eventQueue.eHead) {
		return;
	    }

	    /*
	     * Put event into queue.
	     */
	    eventPtr = &events[scrInfo.eventQueue.eTail];
    
	    switch (j) {
		case RIGHT_BUTTON:
		    eventPtr->key = DEV_EVENT_RIGHT_BUTTON;
		    break;
		case MIDDLE_BUTTON:
		    eventPtr->key = DEV_EVENT_MIDDLE_BUTTON;
		    break;
		case LEFT_BUTTON:
		    eventPtr->key = DEV_EVENT_LEFT_BUTTON;
		    break;
	    }
	    if (newSwitch & j) {
		eventPtr->type = DEV_BUTTON_DOWN_TYPE;
	    } else {
		eventPtr->type = DEV_BUTTON_UP_TYPE;
	    }
	    eventPtr->device = DEV_MOUSE_DEVICE;

	    Timer_GetRealTimeOfDay(&time, (int *) NIL, (Boolean *) NIL);
	    eventPtr->time = TO_MS(time);
	    eventPtr->x = scrInfo.mouse.x;
	    eventPtr->y = scrInfo.mouse.y;
	}
	scrInfo.eventQueue.eTail = i;
	if (devGraphicsOpen) {
	    Fsio_DevNotifyReader(notifyToken);
	}

	/* 
	 * Update the last report 
	 */
	lastRep = currentRep;
	scrInfo.mswitches = newSwitch;
    }
}


/*
 *----------------------------------------------------------------------
 *
 * Dev_GraphicsPutc --
 *
 *	Write a character to the console.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
int 
Dev_GraphicsPutc(c)
register char c;
{
    MASTER_LOCK(&graphicsMutex);

	if (isascii(c)) {
	    mach_MonFuncs.mputchar(c);
	}

    MASTER_UNLOCK(&graphicsMutex);

    return(1);
}



/*
 *----------------------------------------------------------------------
 *
 * DevGraphicsOpen --
 *
 *	Open the graphics device.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
DevGraphicsOpen(devicePtr, useFlags, inNotifyToken, flagsPtr)
    Fs_Device *devicePtr;	/* Specifies type and unit number. */
    int useFlags;		/* Flags from the stream being opened */
    ClientData inNotifyToken;	/* Used for Fs call-back to notify waiting
				 * processes that the console device is ready.*/
    int		*flagsPtr;	/* Device open flags. */
{
    Time	time;

    MASTER_LOCK(&graphicsMutex);

    if (devicePtr->unit == DEV_MOUSE_UNIT) {
	if (devGraphicsOpen) {
	    MASTER_UNLOCK(&graphicsMutex);
	    return(FS_FILE_BUSY);
	}
	devGraphicsOpen = TRUE;
	devDivertXInput = FALSE;
	notifyToken = inNotifyToken;
	/*
	 * Set up event queue for later
	 */
	scrInfo.eventQueue.events = events;
	scrInfo.eventQueue.tcs = tcs;
#if 0
	scrInfo.bitmap = (char *)(MACH_UNCACHED_FRAME_BUFFER_ADDR);
#endif
	scrInfo.cursorBits = (short *)(cursorBits);
	scrInfo.eventQueue.eSize = DEV_MAXEVQ;
	scrInfo.eventQueue.eHead = scrInfo.eventQueue.eTail = 0;
	scrInfo.eventQueue.tcSize = MOTION_BUFFER_SIZE;
	scrInfo.eventQueue.tcNext = 0;
	Timer_GetRealTimeOfDay(&time, (int *) NIL, (Boolean *) NIL);
	scrInfo.eventQueue.timestampMS = TO_MS(time);
    }
    MASTER_UNLOCK(&graphicsMutex);
    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * DevGraphicsClose --
 *
 *	Close the graphics device.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
DevGraphicsClose(devicePtr, useFlags, openCount, writerCount)
    Fs_Device	*devicePtr;	/* Device information. */
    int		useFlags;	/* FS_READ | FS_WRITE */
    int		openCount;	/* Number of times still open. */
    int		writerCount;	/* Number of times still open for writing. */
{
    MASTER_LOCK(&graphicsMutex);

    if (devicePtr->unit == DEV_MOUSE_UNIT) {
	if (!devGraphicsOpen) {
	    MASTER_UNLOCK(&graphicsMutex);
	    return(FS_FILE_BUSY);
	}
	devGraphicsOpen = FALSE;
	VmMach_UserUnmap();
    }
    MASTER_UNLOCK(&graphicsMutex);
    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * DevGraphicsRead --
 *
 *	Read from the graphics device.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
DevGraphicsRead(devicePtr, readPtr, replyPtr)
    Fs_Device	*devicePtr;	/* Device to read from */
    Fs_IOParam	*readPtr;	/* Read parameter block */
    Fs_IOReply	*replyPtr;	/* Return length and signal */ 
{
}


/*
 *----------------------------------------------------------------------
 *
 * DevGraphicsWrite --
 *
 *	Write to the graphics device.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
DevGraphicsWrite(devicePtr, writePtr, replyPtr)
    Fs_Device	*devicePtr;	/* Indicates device */	
    Fs_IOParam	*writePtr;	/* Standard write parameter block */
    Fs_IOReply	*replyPtr;	/* Return length and signal */
{
}


/*
 *----------------------------------------------------------------------
 *
 * DevGraphicsIOControl --
 *
 *	Perform an io control on the graphics device.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
DevGraphicsIOControl(devicePtr, ioctlPtr, replyPtr)
    Fs_Device		*devicePtr;
    Fs_IOCParam		*ioctlPtr;
    Fs_IOReply		*replyPtr;
{
    ReturnStatus status = SUCCESS;
    int		isColor;

    MASTER_LOCK(&graphicsMutex);

    MASTER_UNLOCK(&graphicsMutex);
    return(FS_INVALID_ARG);
}


/*
 *----------------------------------------------------------------------
 *
 * DevGraphicsSelect --
 *
 *	Perform a select on the graphics device.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
DevGraphicsSelect(devicePtr, readPtr, writePtr, exceptPtr)
    Fs_Device	        *devicePtr;
    int			*readPtr;
    int			*writePtr;
    int			*exceptPtr;
{
    ReturnStatus status = SUCCESS;

    MASTER_LOCK(&graphicsMutex);

    if (*readPtr) {
	if (devicePtr->unit == DEV_MOUSE_UNIT) {
	    if (scrInfo.eventQueue.eHead == scrInfo.eventQueue.eTail) {
		*readPtr = 0;
	    }
	} else {
	    *readPtr = 0;
	}
    }

    MASTER_UNLOCK(&graphicsMutex);

    *writePtr = *exceptPtr = 0;

    return(status);
}



/*
 *----------------------------------------------------------------------
 *
 * Dev_VidEnable --
 *
 *	Enables or disables the video display.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Turns video on or off.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Dev_VidEnable(onOff)
Boolean onOff;		/* TRUE if video is to be on. */
{
    return(SUCCESS);
}
