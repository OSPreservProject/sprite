/* 
 *  devGraphics.c --
 *
 *     	This file contains machine-dependent routines for the graphics device.
 *
 *	Most of this assumes that you have a standard color frame buffer.
 *	Support for the fancier graphics displays does not exist yet.
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

#include <sprite.h>
#include <machMon.h>
#include <mach.h>
#include <dev.h>
#include <fs.h>
#include <fsio.h>
#include <sys.h>
#include <sync.h>
#include <timer.h>
#include <dbg.h>
#include <machAddrs.h>
#include <console.h>
#include <dc7085.h>
#include <graphics.h>
#include <vm.h>
#include <vmMach.h>
#include <dev/graphics.h>
#include <devGraphicsInt.h>

/*
 * Macro to translate from a time struct to milliseconds.
 */
#define TO_MS(time) ((time.seconds * 1000) + (time.microseconds / 1000))


Boolean	devGraphicsOpen = FALSE;		/* TRUE => the mouse is open.*/
					/* Process waiting for select.*/

typedef struct {
    char	*vendor;
    char	*module;
    int		romOffset;
    int		type;
} DisplayInfo;

DisplayInfo	configDisplays[] = {
    {"DEC", "PMAG-BA", PMAGBA_ROM_OFFSET, PMAGBA},
    {"DEC", "PMAG-DA", 0, PMAGBA},
};
int numConfigDisplays = sizeof(configDisplays) / sizeof(DisplayInfo);

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
 * These need to mapped into user space.
 */
static DevScreenInfo	scrInfoCached;
static DevEvent		eventsCached[DEV_MAXEVQ] = {0};	
static DevTimeCoord	tcsCached[MOTION_BUFFER_SIZE] = {0};
static char		*frameBuffer = (char *) NIL;

static DevScreenInfo	*scrInfoPtr;
static DevEvent		*events;
static DevTimeCoord	*tcs;

static unsigned short	cursorBits [32];

Boolean			inKBDReset = FALSE;

Address			mappedAddrs[5];

/*
 * DEBUGGING STUFF.
 */

#define DEBUG_SIZE	256
#define DEBUG_ADD	1
#define DEBUG_UPDATE	2
#define DEBUG_PTR(ptr) { 			\
    if (debugPtr == &debugArray[DEBUG_SIZE]) {	\
	debugPtr = debugArray;			\
    }						\
    ptr = debugPtr++;				\
}

Boolean			devGraphicsDebug = TRUE;
typedef struct {
    int		type;
    int		x;
    int		y;
} DebugInfo;

DebugInfo		debugArray[DEBUG_SIZE];
DebugInfo		*debugPtr = debugArray;

int foo;
int bar;
/*
 * END OF DEBUGGING STUFF.
 */

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
 * The default cursor.  The X server does things in terms of the ds3100,
 * which had the notion of A and B planes.  The LoadCursor routine
 * converts this into the correct format.
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
static void		RAMDACInit();
static void		LoadColorMap();
static void 		RecvIntr();
static void		MouseEvent();
static void		MouseButtons();
static void		PosCursor();
static void		XmitIntr();
static void		Scroll();
static void		Blitc();


typedef struct ramdac {
    unsigned char		*addrLowPtr;
    unsigned char		*addrHighPtr;
    volatile unsigned char	*regPtr;
    volatile unsigned char	*colorMap;
} Ramdac;

static Ramdac 	ramdac;
static int 	planeMask;
static int	displayType = UNKNOWN;


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
    Time		time;
    int			i;
    int			slot;
    Mach_SlotInfo	slotInfo;
    ReturnStatus	status;
    char		*slotAddr;
    DisplayInfo		*displayInfoPtr = (DisplayInfo *) NIL;

    Sync_SemInitDynamic(&graphicsMutex, "graphicsMutex");
    for (i = 0; i < numConfigDisplays; i++) {
	displayInfoPtr = &configDisplays[i];
	for (slot = 0; slot < 3; slot++) {
	    slotAddr = (char *) MACH_IO_SLOT_ADDR(slot);
	    status = Mach_GetSlotInfo(slotAddr + displayInfoPtr->romOffset, 
			&slotInfo);
	    if (status == SUCCESS) {
		if ((!strcmp(slotInfo.vendor, displayInfoPtr->vendor)) && 
		    (!strcmp(slotInfo.module, displayInfoPtr->module))) {
		    displayType = displayInfoPtr->type;
		    break;
		}
	    }
	}
	if (displayType != UNKNOWN) {
	    break;
	}
    }
    switch (displayType) {
	case UNKNOWN :
	    Mach_MonPrintf(
		"Assuming you have one of those fancy graphics displays.\n");
	    displayType = PMAGDA;
	    break;
	case PMAGBA:
	    Mach_MonPrintf("Color frame buffer in slot %d, (%s %s %s %s)\n",
			slot, slotInfo.module, slotInfo.vendor,
			slotInfo.revision, slotInfo.type);
	    break;
    }
    if (displayType == PMAGBA) {
	ramdac.addrLowPtr = (unsigned char *) (slotAddr + PMAGBA_RAMDAC_OFFSET);
	ramdac.addrHighPtr = (unsigned char *) 
				(slotAddr + PMAGBA_RAMDAC_OFFSET + 0x4);
	ramdac.regPtr = (unsigned char *) 
				(slotAddr + PMAGBA_RAMDAC_OFFSET + 0x8);
	ramdac.colorMap = (unsigned char *) 
				(slotAddr + PMAGBA_RAMDAC_OFFSET + 0xc);

	/*
	 * Initialize screen info.
	 */
	scrInfoPtr = (DevScreenInfo *) MACH_UNCACHED_ADDR(&scrInfoCached);
	events = (DevEvent *) MACH_UNCACHED_ADDR(eventsCached);
	tcs = (DevTimeCoord *)  MACH_UNCACHED_ADDR(tcsCached);

	InitScreenDefaults(scrInfoPtr);
	scrInfoPtr->eventQueue.events = events;
	scrInfoPtr->eventQueue.tcs = tcs;
	frameBuffer = (char *) (slotAddr + PMAGBA_BUFFER_OFFSET);
	scrInfoPtr->bitmap = (char *) frameBuffer;
	scrInfoPtr->cursorBits = (short *)(cursorBits);
	Timer_GetRealTimeOfDay(&time, (int *) NIL, (Boolean *) NIL);
	scrInfoPtr->eventQueue.timestampMS = TO_MS(time);
	scrInfoPtr->eventQueue.eSize = DEV_MAXEVQ;
	scrInfoPtr->eventQueue.eHead = scrInfoPtr->eventQueue.eTail = 0;
	scrInfoPtr->eventQueue.tcSize = MOTION_BUFFER_SIZE;
	scrInfoPtr->eventQueue.tcNext = 0;

	/*
	 * Initialize the color map, and the screen, and the mouse.
	 */
	InitColorMap();
	ScreenInit();
    }
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
     * Reset the keyboard.
     */
    if(!inKBDReset) {
	KBDReset();
    }

    if (displayType == PMAGBA) {
	/*
	 * Clear any pending video interrupts.
	 */

	* ((int *) (slotAddr + PMAGBA_IREQ_OFFSET)) = 1;
    }

    initialized = TRUE;

    bzero((char *) debugArray, sizeof(debugArray));
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
    scrInfoPtr->row = 55;
    scrInfoPtr->col = 0;

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
 *	Routine to load the cursor Sprite pattern.  The hardware cursor is
 *	64x64x2.  Each byte loaded into the bt459 is 4 pixels (2 bits each),
 *	and the most-significant bit is the leftmost on the screen.
 *	The parameter to this routine is an array of bytes, in the format
 *	used by the ds3100.  The array can be thought of as two arrays
 *	of 16-bit words each with 16 words.  The first array is one bit
 * 	for each pixel, and the second array is the other bit.  Also, the
 *	least-significant bit of the byte is the leftmost on the screen, and
 *	the cursor is 16x16.  This routine has to convert between the
 *	two formats, hence the mess.  Any unused bits in the hardware
 *	cursor are set to 0.
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
LoadCursor(curPtr)
    unsigned char *curPtr;
{
    register int i, j;
    int addr;
    unsigned char value, a, b;
    unsigned char *aPtr, *bPtr;

    aPtr = curPtr;
    bPtr = curPtr + 32;
    addr = 0x400;
    *ramdac.addrHighPtr = (addr >> 8);
    *ramdac.addrLowPtr = (addr & 0xff);
    Mach_EmptyWriteBuffer();
    for (i = 0; i < 16; i++) {
	for (j = 0; j < 4; j += 2) {
	    a = *aPtr;
	    b = *bPtr;
	    value = ((a << 7) & 0x80) |
		    ((b << 6) & 0x40) |
		    ((a << 4) & 0x20) |
		    ((b << 3) & 0x10) |
		    ((a << 1) & 0x08) |
		    ((b << 0) & 0x04) |
		    ((a >> 2) & 0x02) |
		    ((b >> 3) & 0x01);
	    *ramdac.regPtr = value;
	    Mach_EmptyWriteBuffer();
	    value = ((a << 3) & 0x80) |
		    ((b << 2) & 0x40) |
		    ((a << 0) & 0x20) |
		    ((b >> 1) & 0x10) |
		    ((a >> 3) & 0x08) |
		    ((b >> 4) & 0x04) |
		    ((a >> 6) & 0x02) |
		    ((b >> 7) & 0x01);
	    *ramdac.regPtr = value;
	    Mach_EmptyWriteBuffer();
	    aPtr++;
	    bPtr++;
	}
	for(; j < 16; j++) {
	    *ramdac.regPtr = 0;
	    Mach_EmptyWriteBuffer();
	}
    }
    for (; i < 64; i++) {
	for (j = 0; j < 16; j++) {
	    *ramdac.regPtr = 0;
	    Mach_EmptyWriteBuffer();
	}
    }
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


    *ramdac.addrHighPtr = 0x1;
    *ramdac.addrLowPtr = 0x81;
    Mach_EmptyWriteBuffer();
    for (i=0; i < 3; i++) {  
	*ramdac.regPtr = bgRgb[i];
	Mach_EmptyWriteBuffer();
    }

    *ramdac.addrHighPtr = 0x1;
    *ramdac.addrLowPtr = 0x82;
    Mach_EmptyWriteBuffer();
    for (i=0; i < 3; i++) {  
	*ramdac.regPtr = fgRgb[i];
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
	bgRgb[i] = (unsigned char )(color[i] >> 8);
    }
    for (j = 0; j < 3; j++) {
	fgRgb[i] = (unsigned char )(color[j] >> 8);
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

    RAMDACInit();

    *ramdac.addrHighPtr = 0;
    *ramdac.addrLowPtr = 0;
    Mach_EmptyWriteBuffer();
    *ramdac.colorMap = 0;
    Mach_EmptyWriteBuffer();
    *ramdac.colorMap = 0;
    Mach_EmptyWriteBuffer();
    *ramdac.colorMap = 0;
    Mach_EmptyWriteBuffer();
    for(i = 1; i < 256; i++) {
	*ramdac.colorMap = 0xff;
	Mach_EmptyWriteBuffer();
	*ramdac.colorMap = 0xff;
	Mach_EmptyWriteBuffer();
	*ramdac.colorMap = 0xff;
	Mach_EmptyWriteBuffer();
    }
    for (i = 0;i < 3; i++) {
	bgRgb[i] = 0x00;
	fgRgb[i] = 0xff;
    }

}


/*
 * ----------------------------------------------------------------------------
 *
 * RAMDACInit --
 *
 *	Initialize the RAMDAC.
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
RAMDACInit()
{

#if 0
    /*
     * Set up command register 0.
     */
    *ramdac.addrHighPtr = 0x2;
    *ramdac.addrLowPtr = 0x1;
    Mach_EmptyWriteBuffer();
    *ramdac.regPtr = 0x80;
    /*
     * See note on page 4 of PMAG-BA doc.
     */
    {
	volatile char	*addr = (char *) ROM2_ADDR;
	*addr = 1;
    }
    /*
     * Set up command register 1.
     */
    *ramdac.addrHighPtr = 0x2;
    *ramdac.addrLowPtr = 0x2;
    Mach_EmptyWriteBuffer();
    *ramdac.regPtr = 0x00;
    /*
     * Set up command register 2.
     */
    *ramdac.addrHighPtr = 0x2;
    *ramdac.addrLowPtr = 0x3;
    Mach_EmptyWriteBuffer();
    *ramdac.regPtr = 0x00;

#endif
    /*
     * Set up cursor command register. Enable both planes of the cursor
     * and stop the damn blinking.
     */
    *ramdac.addrHighPtr = 0x3;
    *ramdac.addrLowPtr = 0x0;
    Mach_EmptyWriteBuffer();
    *ramdac.regPtr = 0xc0;
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

    *ramdac.addrHighPtr = 0x0;
    *ramdac.addrLowPtr = ptr->index;
    Mach_EmptyWriteBuffer();
    *ramdac.colorMap = ptr->entry.red;
    Mach_EmptyWriteBuffer();
    *ramdac.colorMap = ptr->entry.green;
    Mach_EmptyWriteBuffer();
    *ramdac.colorMap = ptr->entry.blue;
    Mach_EmptyWriteBuffer();
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
	int asciiChar;

	consoleCmdDown = FALSE;
	asciiChar = (int) DevDC7085TranslateKey(ch, FALSE, FALSE);
	if (asciiChar != -1) {
	    Dev_InvokeConsoleCmd(asciiChar);
	    return;
	}
    }

    /*
     * See if there is room in the queue.
     */
    i = DEV_EVROUND(scrInfoPtr->eventQueue.eTail + 1);
    if (i == scrInfoPtr->eventQueue.eHead) {
	return;
    }

    /*
     * Add the event to the queue.
     */
    eventPtr = &events[scrInfoPtr->eventQueue.eTail];
    eventPtr->type = DEV_BUTTON_RAW_TYPE;
    eventPtr->device = DEV_KEYBOARD_DEVICE;
    eventPtr->x = scrInfoPtr->mouse.x;
    eventPtr->y = scrInfoPtr->mouse.y;
    Timer_GetRealTimeOfDay(&time, (int *) NIL, (Boolean *) NIL);
    eventPtr->time = TO_MS(time);
    eventPtr->key = ch;
    scrInfoPtr->eventQueue.eTail = i;
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
    if (scrInfoPtr->mscale >=0) {
	if (newRepPtr->dx >= scrInfoPtr->mthreshold) {
	    newRepPtr->dx +=
		    (newRepPtr->dx - scrInfoPtr->mthreshold) * scrInfoPtr->mscale;
	}
	if (newRepPtr->dy >= scrInfoPtr->mthreshold) {
	    newRepPtr->dy +=
		(newRepPtr->dy - scrInfoPtr->mthreshold) * scrInfoPtr->mscale;
	}
    }
    
    /*
     * Update mouse position
     */
    if( newRepPtr->state & MOUSE_X_SIGN) {
	scrInfoPtr->mouse.x += newRepPtr->dx;
	if (scrInfoPtr->mouse.x > scrInfoPtr->maxCurX) {
	    scrInfoPtr->mouse.x = scrInfoPtr->maxCurX;
	}
    } else {
	scrInfoPtr->mouse.x -= newRepPtr->dx;
	if (scrInfoPtr->mouse.x < scrInfoPtr->minCurX) {
	    scrInfoPtr->mouse.x = scrInfoPtr->minCurX;
	}
    }
    if( newRepPtr->state & MOUSE_Y_SIGN) {
	scrInfoPtr->mouse.y -= newRepPtr->dy;
	if (scrInfoPtr->mouse.y < scrInfoPtr->minCurY) {
	    scrInfoPtr->mouse.y = scrInfoPtr->minCurY;
	}
    } else {
	scrInfoPtr->mouse.y += newRepPtr->dy;
	if (scrInfoPtr->mouse.y > scrInfoPtr->maxCurY) {
	    scrInfoPtr->mouse.y = scrInfoPtr->maxCurY;
	}
    }
    if (devGraphicsOpen) {
	/*
	 * Move the hardware cursor.
	 */
	PosCursor(scrInfoPtr->mouse.x, scrInfoPtr->mouse.y);
    }
    /*
     * Store the motion event in the motion buffer.
     */
    tcs[scrInfoPtr->eventQueue.tcNext].time = milliSec;
    tcs[scrInfoPtr->eventQueue.tcNext].x = scrInfoPtr->mouse.x;
    tcs[scrInfoPtr->eventQueue.tcNext].y = scrInfoPtr->mouse.y;
    scrInfoPtr->eventQueue.tcNext++;
    if (scrInfoPtr->eventQueue.tcNext >= MOTION_BUFFER_SIZE) {
	scrInfoPtr->eventQueue.tcNext = 0;
    }
    if (scrInfoPtr->mouse.y < scrInfoPtr->mbox.bottom &&
	scrInfoPtr->mouse.y >=  scrInfoPtr->mbox.top &&
	scrInfoPtr->mouse.x < scrInfoPtr->mbox.right &&
	scrInfoPtr->mouse.x >=  scrInfoPtr->mbox.left) {
	return;
    }

    scrInfoPtr->mbox.bottom = 0;
    if (DEV_EVROUND(scrInfoPtr->eventQueue.eTail + 1) == scrInfoPtr->eventQueue.eHead) {
	return;
    }

    i = DEV_EVROUND(scrInfoPtr->eventQueue.eTail -1);
    if ((scrInfoPtr->eventQueue.eTail != scrInfoPtr->eventQueue.eHead) && 
        (i != scrInfoPtr->eventQueue.eHead)) {
	DevEvent	*eventPtr;
	eventPtr = &events[i];
	if(eventPtr->type == DEV_MOTION_TYPE) {
	    /*
	     * On the ds5000/200 there is some kind of bug when doing partial
	     * writes.  Sometimes the x,y fields of one event are in the first
	     * word of a cache line, and the x,y fields of the next event are
	     * in the last word of the same cache line.  In this case the line
	     * is probably in the cache because the X server just read it, but
	     * the write done by the kernel to the last word in the cache line
	     * will not be seen by the Xserver, causing the mouse to jump.
	     * It is almost as if the kernel is bypassing the cache and writing
	     * directly to memory.  If we read the word before writing it the
	     * problem goes away.  JHH 2/7/90.
	     */
	    eventPtr->x = scrInfoPtr->mouse.x;
	    eventPtr->y = scrInfoPtr->mouse.y;
	    eventPtr->time = milliSec;
	    eventPtr->device = DEV_MOUSE_DEVICE;
	    return;
	}
    } 
    /*
     * Put event into queue and wakeup any waiters.
     */
    eventPtr = &events[scrInfoPtr->eventQueue.eTail];
    /*
     * See comment above.
     */
    eventPtr->type = DEV_MOTION_TYPE;
    eventPtr->time = milliSec;
    eventPtr->x = scrInfoPtr->mouse.x;
    eventPtr->y = scrInfoPtr->mouse.y;
    eventPtr->device = DEV_MOUSE_DEVICE;
    scrInfoPtr->eventQueue.eTail = DEV_EVROUND(scrInfoPtr->eventQueue.eTail + 1);
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
	    i = DEV_EVROUND(scrInfoPtr->eventQueue.eTail+1);
	    if (i == scrInfoPtr->eventQueue.eHead) {
		return;
	    }

	    /*
	     * Put event into queue.
	     */
	    eventPtr = &events[scrInfoPtr->eventQueue.eTail];
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
	    eventPtr->x = scrInfoPtr->mouse.x;
	    eventPtr->y = scrInfoPtr->mouse.y;
	}
	scrInfoPtr->eventQueue.eTail = i;
	if (devGraphicsOpen) {
	    Fsio_DevNotifyReader(notifyToken);
	}

	/* 
	 * Update the last report 
	 */
	lastRep = currentRep;
	scrInfoPtr->mswitches = newSwitch;
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
    int		pos;
    if( y < scrInfoPtr->minCurY || y > scrInfoPtr->maxCurY ) {
	y = scrInfoPtr->maxCurY;
    }
    if(x < scrInfoPtr->minCurX || x > scrInfoPtr->maxCurX) {
	x = scrInfoPtr->maxCurX;
    }
    scrInfoPtr->cursor.x = x;		/* keep track of real cursor*/
    scrInfoPtr->cursor.y = y;		/* position, indep. of mouse*/

    pos = x + 241 - 52 + 31;
    *ramdac.addrHighPtr = 0x3;
    *ramdac.addrLowPtr = 0x1;
    Mach_EmptyWriteBuffer();
    *ramdac.regPtr = (pos & 0xff);
    *ramdac.addrHighPtr = 0x3;
    *ramdac.addrLowPtr = 0x2;
    Mach_EmptyWriteBuffer();
    *ramdac.regPtr = ((pos >> 8) & 0xff);
    Mach_EmptyWriteBuffer();

    pos = y + 36 - 32 + 31;
    *ramdac.addrHighPtr = 0x3;
    *ramdac.addrLowPtr = 0x3;
    Mach_EmptyWriteBuffer();
    *ramdac.regPtr = (pos & 0xff);
    *ramdac.addrHighPtr = 0x3;
    *ramdac.addrLowPtr = 0x4;
    Mach_EmptyWriteBuffer();
    *ramdac.regPtr = ((pos >> 8) & 0xff);
    Mach_EmptyWriteBuffer();
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


    if (displayType != PMAGBA) {
	return;
    }
    /*
     *  The following is an optimization to cause the scrolling 
     *  of text to be memory limited.  Basically the writebuffer is 
     *  4 words (32 bits ea.) long so to achieve maximum speed we 
     *  read and write in multiples of 4 words. We also limit the 
     *  size to be 80 characters for more speed. 
     */
    lineCount = 40;
    scanInc = 96;
    line = 1920 * 8;
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
    bzero(frameBuffer+(scrInfoPtr->row * line),  3 * line);
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

    if (initialized && (displayType == PMAGBA)) {
	Blitc((unsigned char)(c & 0xff));
    } else {
	if (isascii(c)) {
	    mach_MonFuncs.mputchar(c);
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
    register int ote = 1024; /* offset to table entry */
    int colMult = 8;
    extern char devFont[];

    c &= 0xff;

    switch ( c ) {
	case '\t':
	    for(i = 8 - (scrInfoPtr->col & 0x7); i > 0; i--) {
		Blitc(' ');
	    }
	    break;
	case '\r':
	    scrInfoPtr->col = 0;
	    break;
	case '\b':
	    scrInfoPtr->col--;
	    if(scrInfoPtr->col < 0) {
		scrInfoPtr->col = 0;
	    }
	    break;
	case '\n':
	    if(scrInfoPtr->row + 1 >= scrInfoPtr->maxRow) {
		Scroll();
	    } else {
		scrInfoPtr->row++;
	    }
	    scrInfoPtr->col = 0;
	    break;
	case '\007':
	    DevDC7085KBDPutc(LK_RING_BELL);
	    break;
	default:
	    /*
	     * If the next character will wrap around then 
	     * increment row counter or scroll screen.
	     */
	    if (scrInfoPtr->col >= scrInfoPtr->maxCol) {
		scrInfoPtr->col = 0;
		if(scrInfoPtr->row + 1 >= scrInfoPtr->maxRow) {
		    Scroll();
		} else {
		    scrInfoPtr->row++;
		}
	    }
	    /*
	     * xA1 to XFD are the printable characters added with 8-bit
	     * support.
	     */
	    if ((c >= ' ' && c <= '~') || (c >= 0xA1 && c <= 0xFD)) {
		bRow = frameBuffer + (scrInfoPtr->row * 15 & 0x3ff) * ote + 
		       scrInfoPtr->col * colMult;
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
		{
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
		scrInfoPtr->col++; /* increment column counter */
	}
    }
    if (!devGraphicsOpen) {
	PosCursor(scrInfoPtr->col * 8, scrInfoPtr->row * 15);
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
    Time		time;
    ReturnStatus	status = SUCCESS;

    MASTER_LOCK(&graphicsMutex);

    if (displayType != PMAGBA) {
	status = DEV_NO_DEVICE;
	goto exit;
    }
    if (devicePtr->unit == DEV_MOUSE_UNIT) {
	if (devGraphicsOpen) {
	    status = FS_FILE_BUSY;
	    goto exit;
	}
	devGraphicsOpen = TRUE;
	devDivertXInput = FALSE;
	notifyToken = inNotifyToken;
	InitColorMap();
	/*
	 * Set up event queue for later
	 */
	scrInfoPtr->eventQueue.events = events;
	scrInfoPtr->eventQueue.tcs = tcs;
	scrInfoPtr->cursorBits = (short *)(cursorBits);
	scrInfoPtr->bitmap = (Address) (frameBuffer);
	scrInfoPtr->eventQueue.eSize = DEV_MAXEVQ;
	scrInfoPtr->eventQueue.eHead = scrInfoPtr->eventQueue.eTail = 0;
	scrInfoPtr->eventQueue.tcSize = MOTION_BUFFER_SIZE;
	scrInfoPtr->eventQueue.tcNext = 0;
	Timer_GetRealTimeOfDay(&time, (int *) NIL, (Boolean *) NIL);
	scrInfoPtr->eventQueue.timestampMS = TO_MS(time);
    }
exit:
    MASTER_UNLOCK(&graphicsMutex);
    return(status);
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
	InitColorMap();
	ScreenInit();
	VmMach_UserUnmap((Address) NIL);
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

    switch (ioctlPtr->command) {
	case IOC_GRAPHICS_GET_INFO: {
	    char		*addr;
	    /*
	     * Map the screen info struct into the user's address space.
	     */
	    status = VmMach_UserMap(sizeof(DevScreenInfo), (Address) NIL,
		(Address)scrInfoPtr, FALSE, &addr);
	    if (status != SUCCESS) {
		goto mapError;
	    }
	    bcopy((char *)&addr, ioctlPtr->outBuffer, sizeof(addr));
	    /*
	     * Map the events into the user's address space.
	     */
	    status = VmMach_UserMap(sizeof(eventsCached), (Address) NIL, 
			(Address)events, FALSE, &addr); 
	    if (status != SUCCESS) {
		goto mapError;
	    }
	    scrInfoPtr->eventQueue.events = (DevEvent *)addr;
	    /*
	     * Map the tcs into the user's address space.
	     */
	    status = VmMach_UserMap(sizeof(tcsCached), (Address) NIL,
				(Address)tcs, FALSE, &addr);
	    if (status != SUCCESS) {
		goto mapError;
	    }
	    scrInfoPtr->eventQueue.tcs = (DevTimeCoord *)addr;
	    /*
	     * Map the plane mask into the user's address space.
	     */
	    status = VmMach_UserMap(sizeof(planeMask), (Address) NIL, 
		    (Address)&planeMask, FALSE, &addr);
	    if (status != SUCCESS) {
		goto mapError;
	    }
	    scrInfoPtr->planeMask = (char *)addr;
	    /*
	     * Map the bitmap into the user's address space.
	     */
	    status = VmMach_UserMap(1024*1024, (Address) NIL,
			      (Address)scrInfoPtr->bitmap, FALSE, &addr);
	    if (status != SUCCESS) {
		goto mapError;
	    }
	    scrInfoPtr->bitmap = (char *)addr;
	    break;
mapError:	
	    VmMach_UserUnmap((Address) NIL);
	    status = FS_BUFFER_TOO_BIG;
	    printf("Cannot map shared data structures\n");
	    break;
	}

	case IOC_GRAPHICS_MOUSE_POS:
	    /*
	     * Set mouse state.
	     */
	    scrInfoPtr->mouse = *((DevCursor *)ioctlPtr->inBuffer);
	    PosCursor(scrInfoPtr->mouse.x, scrInfoPtr->mouse.y );
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
	    *(DevScreenInfo **)ioctlPtr->outBuffer = scrInfoPtr;
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
	    break;

	case IOC_GRAPHICS_VIDEO_OFF:
	    break;
	case	IOC_GET_FLAGS:
	case	IOC_SET_FLAGS:
	case	IOC_SET_BITS:
	case	IOC_CLEAR_BITS:
	    /*
	     * No graphics specific bits are set this way.
	     */
	    break;
	case IOC_GRAPHICS_IS_COLOR:
	    isColor = 1;
	    bcopy((char *)&isColor, ioctlPtr->outBuffer, sizeof (int));
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
	    if (scrInfoPtr->eventQueue.eHead == scrInfoPtr->eventQueue.eTail) {
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
/*ARGSUSED*/
ReturnStatus
Dev_VidEnable(onOff)
Boolean onOff;		/* TRUE if video is to be on. */
{
    return(SUCCESS);
}
