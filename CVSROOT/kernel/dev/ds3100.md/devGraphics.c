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
#include "dev/graphics.h"

/*
 * Macro to translate from a time struct to milliseconds.
 */
#define TO_MS(time) ((time.seconds * 1000) + (time.microseconds / 1000))

/*
 * System control status register pointer.
 */
static unsigned short	*sysCSRPtr = (unsigned short *)MACH_SYS_CSR_ADDR;

/*
 * Pointer to VDAC regs.
 */
static DevVDACRegs	*vdacPtr = (DevVDACRegs *)MACH_VDAC_ADDR;

/* 
 * Pointer to the cursor registers.
 */
static DevPCCRegs	*pccPtr = (DevPCCRegs *)MACH_CURSOR_REG_ADDR;

static volatile	unsigned short	curReg = 0;	/* Register to keep track of
						 * the pcc command register
						 * bits. */
Boolean	devGraphicsOpen = FALSE;		/* TRUE => the mouse is open.*/
					/* Process waiting for select.*/
static Boolean	isMono;			/* TRUE 

/*
 * These need to mapped into user space.
 */
static DevScreenInfo	scrInfo;
static DevEvent		events[DEV_MAXEVQ] = {0};	
static DevTimeCoord	tcs[MOTION_BUFFER_SIZE] = {0};
static char		*frameBuffer = (char *)MACH_UNCACHED_FRAME_BUFFER_ADDR;
static char		*planeMask = (char *)MACH_PLANE_MASK_ADDR;

static unsigned short	cursorBits [32];

Boolean			inKBDReset = FALSE;

MouseReport		lastRep;
MouseReport		currentRep;

static unsigned char bgRgb[3];	/* background and foreground colors 	*/
static unsigned char fgRgb[3];  /* 	for the cursor. */

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

/*
 * The default cursor.
 */
unsigned short defCursor[32] = { 
/* plane A */ 0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF,
	      0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF,
/* plane B */ 0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF,
              0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF

};

/*
 * Font mask bits used by Blitc().
 */
static unsigned int fontmaskBits[16] = {
	0x00000000,
	0x00000001,
	0x00000100,
	0x00000101,
	0x00010000,
	0x00010001,
	0x00010100,
	0x00010101,
	0x01000000,
	0x01000001,
	0x01000100,
	0x01000101,
	0x01010000,
	0x01010001,
	0x01010100,
	0x01010101
};

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

    isMono = *sysCSRPtr & MACH_CSR_MONO;
    if (isMono) {
	printf("pm0 ( monochrome display )\n");
    } else {
	printf("pm0 ( color display )\n");
    }

    /*
     * Initialize the screen.
     */
    MACH_DELAY(100000);
    pccPtr->cmdReg = DEV_CURSOR_FOPB | DEV_CURSOR_VBHI;
    /*
     * Initialize the cursor register.
     */
    curReg = DEV_CURSOR_ENPA | DEV_CURSOR_ENPB;
    pccPtr->cmdReg = curReg;

    /*
     * Initialize screen info.
     */
    InitScreenDefaults(&scrInfo);
    scrInfo.eventQueue.events = events;
    scrInfo.eventQueue.tcs = tcs;
    scrInfo.bitmap = (char *)MACH_UNCACHED_FRAME_BUFFER_ADDR;
    scrInfo.cursorBits = (short *)(cursorBits);
    Timer_GetRealTimeOfDay(&time, (int *) NIL, (Boolean *) NIL);
    scrInfo.eventQueue.timestampMS = TO_MS(time);
    scrInfo.eventQueue.eSize = DEV_MAXEVQ;
    scrInfo.eventQueue.eHead = scrInfo.eventQueue.eTail = 0;
    scrInfo.eventQueue.tcSize = MOTION_BUFFER_SIZE;
    scrInfo.eventQueue.tcNext = 0;

    /*
     * Initialize the color map, the screen, and the mouse.
     */
    InitColorMap();
    ScreenInit();
    MouseInit();
    Scroll();

    /*
     * Init the "latest mouse report" structure
     */
    lastRep.state = 0;
    lastRep.dx = 0;
    lastRep.dy = 0;
    lastRep.byteCount = 0;

    /*
     * Clear out the cursor.
     */
    if (isMono) { 
	int i;
	for (i = 0; i < 32; i++) {
	    scrInfo.cursorBits[i] = 0;
	}
    }

    /*
     * Reset the keyboard.
     */
    if(!inKBDReset) {
	KBDReset();
    }

    initialized = TRUE;
}    


/*
 * ----------------------------------------------------------------------------
 *
 * InitScreenDefaults --
 *
 *	Set up default screen parameters.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	*scrInfoPtr is filled in with default screen parameters.
 *
 * ----------------------------------------------------------------------------
 */
static void
InitScreenDefaults(scrInfoPtr)
    DevScreenInfo	*scrInfoPtr;
{
    bzero((char *)scrInfoPtr, sizeof(DevScreenInfo));
    scrInfoPtr->maxRow = 56;
    scrInfoPtr->maxCol = 80;
    scrInfoPtr->maxX = 1024;
    scrInfoPtr->maxY = 864;
    scrInfoPtr->maxCurX = 1023;
    scrInfoPtr->maxCurY = 863;
    scrInfoPtr->version = 11;
    scrInfoPtr->mthreshold = 4;	
    scrInfoPtr->mscale = 2;
    scrInfoPtr->minCurX = -15;
    scrInfoPtr->minCurY = -15;
}


/*
 * ----------------------------------------------------------------------------
 *
 * ScreenInit --
 *
 *	Initialize the screen.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The screen is initialized.
 *
 * ----------------------------------------------------------------------------
 */
static void
ScreenInit()
{
    /*
     * Home the cursor.
     * We want an LSI terminal emulation.  We want the graphics
     * terminal to scroll from the bottom. So start at the bottom.
     */
    scrInfo.row = 55;
    scrInfo.col = 0;
    
    
    /*
     * Load the cursor with the default values
     *
     */
    LoadCursor(defCursor);

    /*
     * Reset keyboard to default state.
     */
     if(!inKBDReset) {
	 KBDReset();
     }
}


/*
 * ----------------------------------------------------------------------------
 *
 * LoadCursor --
 *
 *	Routine to load the cursor Sprite pattern.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The cursor is loaded into the hardware cursor.
 *
 * ----------------------------------------------------------------------------
 */
static void
LoadCursor(cur)
    unsigned short *cur;
{
    register int	i;

    curReg |= DEV_CURSOR_LODSA;
    pccPtr->cmdReg = curReg;
    for (i = 0; i < 32; i++ ){
	pccPtr->memory = cur[i];
	Mach_EmptyWriteBuffer();
    }
    curReg &= ~DEV_CURSOR_LODSA;
    pccPtr->cmdReg = curReg;
}


/*
 * ----------------------------------------------------------------------------
 *
 * RestoreCursorColor --
 *
 *	Routine to restore the color of the cursor.
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
RestoreCursorColor()
{
    register int i;

    vdacPtr->overWA = 0x04;
    Mach_EmptyWriteBuffer();

    for (i=0; i < 3; i++) {  
	vdacPtr->over = bgRgb[i];
	Mach_EmptyWriteBuffer();
    }

    vdacPtr->overWA = 0x08;
    Mach_EmptyWriteBuffer();
    vdacPtr->over = 0x00;
    Mach_EmptyWriteBuffer();
    vdacPtr->over = 0x00;
    Mach_EmptyWriteBuffer();
    vdacPtr->over = 0x7f;
    Mach_EmptyWriteBuffer();

    vdacPtr->overWA = 0x0c;
    Mach_EmptyWriteBuffer();

    for (i=0; i < 3; i++) {
	vdacPtr->over = fgRgb[i];
	Mach_EmptyWriteBuffer();
    }
}


/*
 * ----------------------------------------------------------------------------
 *
 * CursorColor --
 *
 *	Set the color of the cursor.
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
CursorColor(color)
    unsigned int color[];
{
    register int i, j;

    for (i = 0; i < 3; i++) {
	vdacPtr->over = (unsigned char)(color[i] >> 8);
	bgRgb[i] = (unsigned char )(color[i] >> 8);
	Mach_EmptyWriteBuffer();
    }

    for (i = 3, j=0 ; i < 6; i++, j++) {
	vdacPtr->over = (unsigned char)(color[i] >> 8);
	fgRgb[j] = (unsigned char)(color[i] >> 8);
	Mach_EmptyWriteBuffer();
    }

    RestoreCursorColor();
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


/*
 * ----------------------------------------------------------------------------
 *
 * InitColorMap --
 *
 *	Initialize the color map.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The colormap is initialized appropriately whether it is color or 
 *	monochrome.
 *
 * ----------------------------------------------------------------------------
 */
static void
InitColorMap()
{
    register int i;

    *planeMask = 0xff;
    Mach_EmptyWriteBuffer();

    VDACInit();

    if (isMono) {
        for (i = 0; i < 256; i++) {
	    vdacPtr->mapWA = i; Mach_EmptyWriteBuffer();
	    vdacPtr->map = (i < 128)? 0x00: 0xff; Mach_EmptyWriteBuffer();
	    vdacPtr->map = (i < 128)? 0x00: 0xff; Mach_EmptyWriteBuffer();
	    vdacPtr->map = (i < 128)? 0x00: 0xff; Mach_EmptyWriteBuffer();
        }
    } else {
        vdacPtr->mapWA = 0; Mach_EmptyWriteBuffer();
        vdacPtr->map = 0; Mach_EmptyWriteBuffer();
        vdacPtr->map = 0; Mach_EmptyWriteBuffer();
        vdacPtr->map = 0; Mach_EmptyWriteBuffer();

        for(i = 1; i < 256; i++) {
            vdacPtr->mapWA = i; Mach_EmptyWriteBuffer();
            vdacPtr->map = 0xff; Mach_EmptyWriteBuffer();
            vdacPtr->map = 0xff; Mach_EmptyWriteBuffer();
            vdacPtr->map = 0xff; Mach_EmptyWriteBuffer();
        }
    }

    for (i = 0;i < 3; i++) {
	bgRgb[i] = 0x00;
	fgRgb[i] = 0xff;
    }

}


/*
 * ----------------------------------------------------------------------------
 *
 * VDACInit --
 *
 *	Initialize the VDAC.
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
VDACInit()
{
    /*
     *
     * Initialize the VDAC
     */
    vdacPtr->overWA = 0x04;
    Mach_EmptyWriteBuffer();
    vdacPtr->over = 0x00;
    Mach_EmptyWriteBuffer();
    vdacPtr->over = 0x00;
    Mach_EmptyWriteBuffer();
    vdacPtr->over = 0x00;
    Mach_EmptyWriteBuffer();
    vdacPtr->overWA = 0x08;
    Mach_EmptyWriteBuffer();
    vdacPtr->over = 0x00;
    Mach_EmptyWriteBuffer();
    vdacPtr->over = 0x00;
    Mach_EmptyWriteBuffer();
    vdacPtr->over = 0x7f;
    Mach_EmptyWriteBuffer();
    vdacPtr->overWA = 0x0c;
    Mach_EmptyWriteBuffer();
    vdacPtr->over = 0xff;
    Mach_EmptyWriteBuffer();
    vdacPtr->over = 0xff;
    Mach_EmptyWriteBuffer();
    vdacPtr->over = 0xff;
    Mach_EmptyWriteBuffer();
    vdacPtr->mask = 0xff; 
    Mach_EmptyWriteBuffer();

}


/*
 * ----------------------------------------------------------------------------
 *
 * LoadColorMap --
 *
 *	Load the color map.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The color map is loaded.
 *
 * ----------------------------------------------------------------------------
 */
static void
LoadColorMap(ptr)
    DevColorMap  *ptr;
{
    if(ptr->index > 256) {
	 return;
    }

    vdacPtr->mapWA = ptr->index; Mach_EmptyWriteBuffer();
    vdacPtr->map = ptr->entry.red; Mach_EmptyWriteBuffer();
    vdacPtr->map = ptr->entry.green; Mach_EmptyWriteBuffer();
    vdacPtr->map = ptr->entry.blue; Mach_EmptyWriteBuffer();
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
    if (devGraphicsOpen) {
	/*
	 * Move the hardware cursor.
	 */
	PosCursor(scrInfo.mouse.x, scrInfo.mouse.y);
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
 * PosCursor --
 *
 *	Postion the cursor.
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
PosCursor(x, y)
    register int x,y;
{
    if( y < scrInfo.minCurY || y > scrInfo.maxCurY ) {
	y = scrInfo.maxCurY;
    }
    if(x < scrInfo.minCurX || x > scrInfo.maxCurX) {
	x = scrInfo.maxCurX;
    }
    scrInfo.cursor.x = x;		/* keep track of real cursor*/
    scrInfo.cursor.y = y;		/* position, indep. of mouse*/
    pccPtr->xPos = MOUSE_X_OFFSET + x;
    pccPtr->yPos = MOUSE_Y_OFFSET + y;
}


/*
 *----------------------------------------------------------------------
 *
 * Scroll --
 *
 *	Scroll the screen.
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
Scroll()
{
    int    line;
    register int *dest, *src;
    register int *end;
    register int temp0,temp1,temp2,temp3;
    register int i, scanInc, lineCount;

#ifdef notdef
    /*
     * If the mouse is on we don't scroll so that the bit map remains sane.
     */
    if(devGraphicsOpen) {
	scrInfo.row = 0;
	return;
    }
#endif

    /*
     *  The following is an optimization to cause the scrolling 
     *  of text to be memory limited.  Basically the writebuffer is 
     *  4 words (32 bits ea.) long so to achieve maximum speed we 
     *  read and write in multiples of 4 words. We also limit the 
     *  size to be 80 characters for more speed. 
     */
    if(isMono) {
	lineCount = 5;
	line = 1920 * 2;
	scanInc = 44;
    } else {
	lineCount = 40;
	scanInc = 96;
	line = 1920 * 8;
    }
    src = (int *)(frameBuffer+line);
    dest = (int *)(frameBuffer);
    end = (int *)(frameBuffer+ (60 * line) - line);
    do {
	i = 0;
	do {
	    temp0 = src[0];
	    temp1 = src[1];
	    temp2 = src[2];
	    temp3 = src[3];
	    dest[0] = temp0;
	    dest[1] = temp1;
	    dest[2] = temp2;
	    dest[3] = temp3;
	    dest += 4;
	    src += 4;
	    i++;
	} while(i < lineCount);
	src += scanInc;
	dest += scanInc;
    } while(src < end);

    /* 
     * Now zero out the last two lines 
     */
    bzero(frameBuffer+(scrInfo.row * line),  3 * line);
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

    if (initialized) {
	Blitc((unsigned char)(c & 0xff));
    } else {
	if (isascii(c)) {
	    mach_MonFuncs.putchar(c);
	}
    }

    MASTER_UNLOCK(&graphicsMutex);

    return(1);
}


/*
 *----------------------------------------------------------------------
 *
 * Blitc --
 *
 *	Write a character to the screen.
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
Blitc(c)
    register unsigned char c;
{
    register char *bRow, *fRow;
    register int i;
    register int ote = isMono ? 256 : 1024; /* offset to table entry */
    int colMult = isMono ? 1 : 8;
    extern char devFont[];

    c &= 0xff;

    switch ( c ) {
	case '\t':
	    for(i = 8 - (scrInfo.col & 0x7); i > 0; i--) {
		Blitc(' ');
	    }
	    break;
	case '\r':
	    scrInfo.col = 0;
	    break;
	case '\b':
	    scrInfo.col--;
	    if(scrInfo.col < 0) {
		scrInfo.col = 0;
	    }
	    break;
	case '\n':
	    if(scrInfo.row + 1 >= scrInfo.maxRow) {
		Scroll();
	    } else {
		scrInfo.row++;
	    }
	    scrInfo.col = 0;
	    break;
	case '\007':
	    DevDC7085KBDPutc(LK_RING_BELL);
	    break;
	default:
	    /*
	     * If the next character will wrap around then 
	     * increment row counter or scroll screen.
	     */
	    if (scrInfo.col >= scrInfo.maxCol) {
		scrInfo.col = 0;
		if(scrInfo.row + 1 >= scrInfo.maxRow) {
		    Scroll();
		} else {
		    scrInfo.row++;
		}
	    }
	    /*
	     * xA1 to XFD are the printable characters added with 8-bit
	     * support.
	     */
	    if ((c >= ' ' && c <= '~') || (c >= 0xA1 && c <= 0xFD)) {
		bRow = frameBuffer + (scrInfo.row * 15 & 0x3ff) * ote + 
		       scrInfo.col * colMult;
		i = c - ' ';
		if(i < 0 || i > 221) {
		    i = 0;
		} else {
		    /* These are to skip the (32) 8-bit 
		     * control chars, as well as DEL 
		     * and 0xA0 which aren't printable */
		    if (c > '~') {
			i -= 34; 
		    }
		    i *= 15;
		}
		fRow = (char *)((int)devFont + i);

		/* inline expansion for speed */
		if(isMono) {
		    *bRow = *fRow++; bRow += ote;
		    *bRow = *fRow++; bRow += ote;
		    *bRow = *fRow++; bRow += ote;
		    *bRow = *fRow++; bRow += ote;
		    *bRow = *fRow++; bRow += ote;
		    *bRow = *fRow++; bRow += ote;
		    *bRow = *fRow++; bRow += ote;
		    *bRow = *fRow++; bRow += ote;
		    *bRow = *fRow++; bRow += ote;
		    *bRow = *fRow++; bRow += ote;
		    *bRow = *fRow++; bRow += ote;
		    *bRow = *fRow++; bRow += ote;
		    *bRow = *fRow++; bRow += ote;
		    *bRow = *fRow++; bRow += ote;
		    *bRow = *fRow++; bRow += ote;
		} else {
		    int j;
		    unsigned int *pInt;

		    pInt = (unsigned int *) bRow;
		    for(j = 0; j < 15; j++) {
			/*
			 * fontmaskBits converts a nibble
			 * (4 bytes) to a long word 
			 * containing 4 pixels corresponding
			 * to each bit in the nibble.  Thus
			 * we write two longwords for each
			 * byte in font.
			 * 
			 * Remember the font is 8 bits wide
			 * and 15 bits high.
			 *
			 * We add 256 to the pointer to
			 * point to the pixel on the 
			 * next scan line
			 * directly below the current
			 * pixel.
			 */
			*pInt =  fontmaskBits[(*fRow)&0xf];
			*(pInt+1)= fontmaskBits[((*fRow)>>4)&0xf];
			fRow++; 
			pInt += 256;
		    }
		}
		scrInfo.col++; /* increment column counter */
	}
    }
    if (!devGraphicsOpen) {
	PosCursor(scrInfo.col * 8, scrInfo.row * 15);
    }
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
	if (!isMono) {
	    InitColorMap();
	}
	/*
	 * Set up event queue for later
	 */
	scrInfo.eventQueue.events = events;
	scrInfo.eventQueue.tcs = tcs;
	scrInfo.bitmap = (char *)(MACH_UNCACHED_FRAME_BUFFER_ADDR);
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
	if (!isMono) {
	    InitColorMap();
	}
	ScreenInit();
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

    MASTER_LOCK(&graphicsMutex);

    switch (ioctlPtr->command) {
	case IOC_GRAPHICS_GET_INFO: {
	    Address		addr;

	    /*
	     * Map the screen info struct into the user's address space.
	     */
	    addr = VmMach_UserMap(sizeof(scrInfo), (Address)&scrInfo, TRUE);
	    if (addr == (Address)NIL) {
		goto mapError;
	    }
	    bcopy((char *)&addr, ioctlPtr->outBuffer, sizeof(addr));
	    /*
	     * Map the events into the user's address space.
	     */
	    addr = VmMach_UserMap(sizeof(events), (Address)events, FALSE);
	    if (addr == (Address)NIL) {
		goto mapError;
	    }
	    scrInfo.eventQueue.events = (DevEvent *)addr;
	    /*
	     * Map the tcs into the user's address space.
	     */
	    addr = VmMach_UserMap(sizeof(tcs), (Address)tcs, FALSE);
	    if (addr == (Address)NIL) {
		goto mapError;
	    }
	    scrInfo.eventQueue.tcs = (DevTimeCoord *)addr;
	    /*
	     * Map the plane mask into the user's address space.
	     */
	    addr = VmMach_UserMap(4, (Address)MACH_PLANE_MASK_ADDR, FALSE);
	    if (addr == (Address)NIL) {
		goto mapError;
	    }
	    scrInfo.planeMask = (char *)addr;
	    /*
	     * Map the bitmap into the user's address space.
	     */
	    addr = VmMach_UserMap(isMono ? 256*1024 : 1024*1024,
			      (Address)MACH_UNCACHED_FRAME_BUFFER_ADDR, FALSE);
	    if (addr == (Address)NIL) {
		goto mapError;
	    }
	    scrInfo.bitmap = (char *)addr;
	    break;
mapError:	
	    VmMach_UserUnmap();
	    status = FS_BUFFER_TOO_BIG;
	    printf("Cannot map shared data structures\n");
	    break;
	}

	case IOC_GRAPHICS_MOUSE_POS:
	    /*
	     * Set mouse state.
	     */
	    scrInfo.mouse = *((DevCursor *)ioctlPtr->inBuffer);
	    PosCursor(scrInfo.mouse.x, scrInfo.mouse.y );
	    break;

	case IOC_GRAPHICS_INIT_SCREEN:	
	    /*
	     * Initialize the screen.
	     */
	    ScreenInit();
	    break;

	case IOC_GRAPHICS_KBD_CMD: {
	    DevKpCmd		*kpCmdPtr;
	    unsigned char	*cp;

	    kpCmdPtr = (DevKpCmd *)ioctlPtr->inBuffer;
	    if (kpCmdPtr->nbytes == 0) {
		kpCmdPtr->cmd |= 0x80;
	    }
	    if (!devGraphicsOpen) {
		kpCmdPtr->cmd |= 1;
	    }
	    DevDC7085KBDPutc((int)kpCmdPtr->cmd);
	    cp = &kpCmdPtr->par[0];
	    for (cp = &kpCmdPtr->par[0]; 
	         kpCmdPtr->nbytes > 0;
		 cp++, kpCmdPtr->nbytes--) {
		if(kpCmdPtr->nbytes == 1) {
		    *cp |= 0x80;
		}
		DevDC7085KBDPutc((int)*cp);
	    }
	    break;
	}

	case IOC_GRAPHICS_GET_INFO_ADDR:	
	    *(DevScreenInfo **)ioctlPtr->outBuffer = &scrInfo;
	    break;

	case IOC_GRAPHICS_CURSOR_BIT_MAP:
	    LoadCursor((unsigned short *)ioctlPtr->inBuffer);
	    break;

	case IOC_GRAPHICS_CURSOR_COLOR:
	    CursorColor((unsigned int *)ioctlPtr->inBuffer);
	    break;
	     
        case IOC_GRAPHICS_COLOR_MAP:
	    LoadColorMap((DevColorMap *)ioctlPtr->inBuffer);
	    break;

	case IOC_GRAPHICS_KERN_LOOP: 
	    printf("DevGraphicsIOControl: QIOKERNLOOP\n");
	    break;

	case IOC_GRAPHICS_KERN_UNLOOP: 
	    printf("DevGraphicsIOControl: QIOKERNUNLOOP\n");
	    break;

	case IOC_GRAPHICS_VIDEO_ON:
	    if (!isMono) {
		RestoreCursorColor();
	    }
	    curReg |= (DEV_CURSOR_ENPA);
	    curReg &= ~(DEV_CURSOR_FOPB);
	    pccPtr->cmdReg = curReg;
	    break;

	case IOC_GRAPHICS_VIDEO_OFF:
	    if (!isMono) {
		VDACInit();
	    }
	    curReg |= DEV_CURSOR_FOPB;
	    curReg &= ~(DEV_CURSOR_ENPA);
	    pccPtr->cmdReg = curReg;
	    break;
	case	IOC_GET_FLAGS:
	case	IOC_SET_FLAGS:
	case	IOC_SET_BITS:
	case	IOC_CLEAR_BITS:
	    /*
	     * No graphics specific bits are set this way.
	     */
	    break;

	default:
	    printf("DevGraphicsIOControl: Unknown command %d\n", 
		   ioctlPtr->command);
	    status = FS_INVALID_ARG;
	    break;
    }

    MASTER_UNLOCK(&graphicsMutex);
    return(status);
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
    if (onOff) {
	if (!isMono) {
	    RestoreCursorColor();
	}
	curReg |= (DEV_CURSOR_ENPA);
	curReg &= ~(DEV_CURSOR_FOPB);
	pccPtr->cmdReg = curReg;
    } else {
	if (!isMono) {
	    VDACInit();
	}
	curReg |= DEV_CURSOR_FOPB;
	curReg &= ~(DEV_CURSOR_ENPA);
	pccPtr->cmdReg = curReg;
    }
    return(SUCCESS);
}
