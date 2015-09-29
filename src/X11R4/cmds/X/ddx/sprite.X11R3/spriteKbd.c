/*-
 * spriteKbd.c --
 *	Functions for retrieving data from a keyboard.
 *
 * Copyright (c) 1987 by the Regents of the University of California
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 *
 */
#ifndef lint
static char rcsid[] =
	"$Header: /a/X/src/cmds/Xsprite/ddx/sun3.md/RCS/spriteKbd.c,v 5.4 88/10/01 10:39:05 ouster Exp Locker: ouster $ SPRITE (Berkeley)";
#endif lint

#define Time SpriteTime
#include <fs.h>
#undef Time

#define NEED_EVENTS
#include    "spriteddx.h"

#include    <bit.h>

#include    <errno.h>
#include    <fcntl.h>
#include    <sys/file.h>
#include    <sys/time.h>

#include    "keysym.h"

#define AUTO_GENERATED	0x10000

#define MIN_KEYCODE	8   	/* Keycode below which we may not transmit */

/*
 * Keycode that means "all keys are now up":
 */

#define ALL_KEYS_UP	127

static void		spriteBell();
static void		spriteKbdCtrl();
static Mouse_Event	*spriteKbdGetEvents();
static void		spriteKbdProcessEvent();
static void		spriteKbdDoneEvents();

/*
 * Auto-repeat stuff.
 */
static enum {
    REPEAT_LONG,	    /* Start repeat with long timeout */
    REPEAT_SHORT,	    /* Start repeat with short timeout */
    REPEAT_TIMEOUT,	    /* In the middle of a timeout */
    REPEAT_PENDING,	    /* Repeat should be taken next */
    REPEAT_NONE	  	    /* No repeat should happen */
}	    	  	repeatPhase = REPEAT_NONE;

static Mouse_Event	repeatEvent;	/* Event that will be repeated */
static struct timeval 	repeatTimeout;	/* Timeout to use for repeating */
static unsigned int	repeatDelta;	/* Timeout length (ms) */
#define REPEAT_LONG_TIMEOUT	500 	    /* Ms delay to begin repeat */
#define REPEAT_SHORT_TIMEOUT	10 	    /* Ms delay to continue repeat */

static KbPrivRec  	sysKbPriv = {
    -1,				/* Type of keyboard */
    -1,				/* Descriptor open to device */
    spriteKbdGetEvents,		/* Function to read events */
    spriteKbdProcessEvent,	/* Function to process an event */
    spriteKbdDoneEvents,	/* Function called when all events */
				/* have been handled. */
    0,	    	  	    	/* Keycode offset */
    &defaultKeyboardControl,	/* Current keyboard control */ 	
    (pointer)NULL,	    	/* Private to keyboard (nothing needed) */
};


/*
 *	XXX - Its not clear what to map these to for now.
 *	keysyms.h doesn't define enough function key names.
 */

#ifndef XK_L1
#define	XK_L1	XK_Cancel
#define	XK_L2	XK_Redo
#define	XK_L3	XK_Menu
#define	XK_L4	XK_Undo
#define	XK_L5	XK_Insert
#define	XK_L6	XK_Select
#define	XK_L7	XK_Execute
#define	XK_L8	XK_Print
#define	XK_L9	XK_Find
#define	XK_L10	XK_Help
#define	XK_R1	NoSymbol
#define	XK_R2	NoSymbol
#define	XK_R3	NoSymbol
#define	XK_R4	NoSymbol
#define	XK_R5	NoSymbol
#define	XK_R6	NoSymbol
#define	XK_R7	NoSymbol
#define	XK_R8	XK_Up
#define	XK_R9	NoSymbol
#define	XK_R10	XK_Left
#define	XK_R11	XK_Home
#define	XK_R12	XK_Right
#define	XK_R13	NoSymbol
#define	XK_R14	XK_Down
#define	XK_R15	NoSymbol
#endif

static KeySym sunKbdMap[] = {
	XK_L1,		NoSymbol,		/* 0x01 */
	NoSymbol,	NoSymbol,		/* 0x02 */
	XK_L2,		NoSymbol,		/* 0x03 */
	NoSymbol,	NoSymbol,		/* 0x04 */
	XK_F1,		NoSymbol,		/* 0x05 */
	XK_F2,		NoSymbol,		/* 0x06 */
	NoSymbol,	NoSymbol,		/* 0x07 */
	XK_F3,		NoSymbol,		/* 0x08 */
	NoSymbol,	NoSymbol,		/* 0x09 */
	XK_F4,		NoSymbol,		/* 0x0a */
	NoSymbol,	NoSymbol,		/* 0x0b */
	XK_F5,		NoSymbol,		/* 0x0c */
	NoSymbol,	NoSymbol,		/* 0x0d */
	XK_F6,		NoSymbol,		/* 0x0e */
	NoSymbol,	NoSymbol,		/* 0x0f */
	XK_F7,		NoSymbol,		/* 0x10 */
	XK_F8,		NoSymbol,		/* 0x11 */
	XK_F9,		NoSymbol,		/* 0x12 */
	XK_Break,	NoSymbol,		/* 0x13 */
	NoSymbol,	NoSymbol,		/* 0x14 */
	XK_R1,		NoSymbol,		/* 0x15 */
	XK_R2,		NoSymbol,		/* 0x16 */
	XK_R3,		NoSymbol,		/* 0x17 */
	NoSymbol,	NoSymbol,		/* 0x18 */
	XK_L3,		NoSymbol,		/* 0x19 */
	XK_L4,		NoSymbol,		/* 0x1a */
	NoSymbol,	NoSymbol,		/* 0x1b */
	NoSymbol,	NoSymbol,		/* 0x1c */
	XK_Escape,	NoSymbol,		/* 0x1d */
	XK_1,		XK_exclam,		/* 0x1e */
	XK_2,		XK_at,			/* 0x1f */
	XK_3,		XK_numbersign,		/* 0x20 */
	XK_4,		XK_dollar,		/* 0x21 */
	XK_5,		XK_percent,		/* 0x22 */
	XK_6,		XK_asciicircum,		/* 0x23 */
	XK_7,		XK_ampersand,		/* 0x24 */
	XK_8,		XK_asterisk,		/* 0x25 */
	XK_9,		XK_parenleft,		/* 0x26 */
	XK_0,		XK_parenright,		/* 0x27 */
	XK_minus,	XK_underscore,		/* 0x28 */
	XK_equal,	XK_plus,		/* 0x29 */
	XK_quoteleft,	XK_asciitilde,		/* 0x2a */
	XK_BackSpace,	NoSymbol,		/* 0x2b */
	NoSymbol,	NoSymbol,		/* 0x2c */
	XK_R4,		NoSymbol,		/* 0x2d */
	XK_R5,		NoSymbol,		/* 0x2e */
	XK_R6,		NoSymbol,		/* 0x2f */
	NoSymbol,	NoSymbol,		/* 0x30 */
	XK_L5,		NoSymbol,		/* 0x31 */
	NoSymbol,	NoSymbol,		/* 0x32 */
	XK_L6,		NoSymbol,		/* 0x33 */
	NoSymbol,	NoSymbol,		/* 0x34 */
	XK_Tab,		NoSymbol,		/* 0x35 */
	XK_Q,		NoSymbol,		/* 0x36 */
	XK_W,		NoSymbol,		/* 0x37 */
	XK_E,		NoSymbol,		/* 0x38 */
	XK_R,		NoSymbol,		/* 0x39 */
	XK_T,		NoSymbol,		/* 0x3a */
	XK_Y,		NoSymbol,		/* 0x3b */
	XK_U,		NoSymbol,		/* 0x3c */
	XK_I,		NoSymbol,		/* 0x3d */
	XK_O,		NoSymbol,		/* 0x3e */
	XK_P,		NoSymbol,		/* 0x3f */
	XK_bracketleft,	XK_braceleft,		/* 0x40 */
	XK_bracketright,	XK_braceright,	/* 0x41 */
	XK_Delete,	NoSymbol,		/* 0x42 */
	NoSymbol,	NoSymbol,		/* 0x43 */
	XK_R7,		NoSymbol,		/* 0x44 */
	XK_Up,		XK_R8,			/* 0x45 */
	XK_R9,		NoSymbol,		/* 0x46 */
	NoSymbol,	NoSymbol,		/* 0x47 */
	XK_L7,		NoSymbol,		/* 0x48 */
	XK_L8,		NoSymbol,		/* 0x49 */
	NoSymbol,	NoSymbol,		/* 0x4a */
	NoSymbol,	NoSymbol,		/* 0x4b */
	XK_Control_L,	NoSymbol,		/* 0x4c */
	XK_A,		NoSymbol,		/* 0x4d */
	XK_S,		NoSymbol,		/* 0x4e */
	XK_D,		NoSymbol,		/* 0x4f */
	XK_F,		NoSymbol,		/* 0x50 */
	XK_G,		NoSymbol,		/* 0x51 */
	XK_H,		NoSymbol,		/* 0x52 */
	XK_J,		NoSymbol,		/* 0x53 */
	XK_K,		NoSymbol,		/* 0x54 */
	XK_L,		NoSymbol,		/* 0x55 */
	XK_semicolon,	XK_colon,		/* 0x56 */
	XK_quoteright,	XK_quotedbl,		/* 0x57 */
	XK_backslash,	XK_bar,			/* 0x58 */
	XK_Return,	NoSymbol,		/* 0x59 */
	NoSymbol,	NoSymbol,		/* 0x5a */
	XK_Left,	XK_R10,			/* 0x5b */
	XK_R11,		NoSymbol,		/* 0x5c */
	XK_Right,	NoSymbol,		/* 0x5d */
	NoSymbol,	NoSymbol,		/* 0x5e */
	XK_L9,		NoSymbol,		/* 0x5f */
	NoSymbol,	NoSymbol,		/* 0x60 */
	XK_L10,		NoSymbol,		/* 0x61 */
	NoSymbol,	NoSymbol,		/* 0x62 */
	XK_Shift_L,	NoSymbol,		/* 0x63 */
	XK_Z,		NoSymbol,		/* 0x64 */
	XK_X,		NoSymbol,		/* 0x65 */
	XK_C,		NoSymbol,		/* 0x66 */
	XK_V,		NoSymbol,		/* 0x67 */
	XK_B,		NoSymbol,		/* 0x68 */
	XK_N,		NoSymbol,		/* 0x69 */
	XK_M,		NoSymbol,		/* 0x6a */
	XK_comma,	XK_less,		/* 0x6b */
	XK_period,	XK_greater,		/* 0x6c */
	XK_slash,	XK_question,		/* 0x6d */
	XK_Shift_R,	NoSymbol,		/* 0x6e */
	XK_Linefeed,	NoSymbol,		/* 0x6f */
	XK_R13,		NoSymbol,		/* 0x70 */
	XK_Down,	XK_R14,			/* 0x71 */
	XK_R15,		NoSymbol,		/* 0x72 */
	NoSymbol,	NoSymbol,		/* 0x73 */
	NoSymbol,	NoSymbol,		/* 0x74 */
	NoSymbol,	NoSymbol,		/* 0x75 */
	NoSymbol,	NoSymbol,		/* 0x76 */
	XK_Caps_Lock,	NoSymbol,		/* 0x77 */
	XK_Meta_L,	NoSymbol,		/* 0x78 */
	XK_space,	NoSymbol,		/* 0x79 */
	XK_Meta_R,	NoSymbol,		/* 0x7a */
	NoSymbol,	NoSymbol,		/* 0x7b */
	NoSymbol,	NoSymbol,		/* 0x7c */
	NoSymbol,	NoSymbol,		/* 0x7d */
	NoSymbol,	NoSymbol,		/* 0x7e */
	NoSymbol,	NoSymbol,		/* 0x7f */
};

static KeySymsRec sunMapDesc = {
/*  map        minKeyCode  maxKeyCode  width */
    sunKbdMap,	  1,	    0x7a,	2
};

#define	cT	(ControlMask)
#define	sH	(ShiftMask)
#define	lK	(LockMask)
#define	mT	(Mod1Mask)
static CARD8 sunModMap[MAP_LENGTH] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, /* 00-0f */
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, /* 10-1f */
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, /* 20-2f */
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, /* 30-3f */
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, /* 40-4f */
    0,  0,  0,  cT, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, /* 50-5f */
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  sH, 0,  0,  0,  0,  0, /* 60-6f */
    0,  0,  0,  0,  0,  sH, 0,  0,  0,  0,  0,  0,  0,  0,  lK, mT,/* 70-7f */
    0,  mT, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, /* 80-8f */
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, /* 90-9f */
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, /* a0-af */
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, /* b0-bf */
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, /* c0-cf */
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, /* d0-df */
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, /* e0-ef */
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, /* f0-ff */
};


/*-
 *-----------------------------------------------------------------------
 * spriteKbdProc --
 *	Handle the initialization, etc. of a keyboard.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------
 */
/*ARGSUSED*/
int
spriteKbdProc (pKeyboard, what, argc, argv)
    DevicePtr	  pKeyboard;	/* Keyboard to manipulate */
    int	    	  what;	    	/* What to do to it */
    int	    	  argc;
    char    	  **argv;
{
    register int  fd;
    int		  tmp;
    KbPrivPtr	  pPriv;
    int	    	  i;
    char	  reset = KBD_RESET;
    BYTE    	  map[MAP_LENGTH];

    switch (what) {
	case DEVICE_INIT:
	    if (pKeyboard != LookupKeyboardDevice()) {
		ErrorF ("Cannot open non-system keyboard");
		return (!Success);
	    }
	    /*
	     * First open and find the current state of the keyboard
	     * If we are reinitializing, then turn the device OFF first.
	     */
	    if (sysKbPriv.fd >= 0) {
		fd = sysKbPriv.fd;
	    } else {
	        ReturnStatus status;

		status = Fs_Open("/dev/mouse",
			FS_NON_BLOCKING|FS_READ|FS_WRITE, 0, &sysKbPriv.fd);
	        if (status != 0) {
		    errno = Compat_MapCode(status);
		    Error ("Opening /dev/mouse");
		    return (!Success);
		}
		fd = sysKbPriv.fd;
	    }

	    /*
	     * Perform final initialization of the system private keyboard
	     * structure and fill in various slots in the device record
	     * itself which couldn't be filled in before.
	     */
	    pKeyboard->devicePrivate = (pointer)&sysKbPriv;

	    pKeyboard->on = FALSE;
	    /*
	     * Make sure keycodes we send out are >= MIN_KEYCODE
	     */
	     if (sunMapDesc.minKeyCode < MIN_KEYCODE) {
		 int	offset;

		 offset = MIN_KEYCODE - sunMapDesc.minKeyCode;
		 sunMapDesc.minKeyCode += offset;
		 sunMapDesc.maxKeyCode += offset;
		 sysKbPriv.offset = offset;
	     }

	    InitKeyboardDeviceStruct(pKeyboard, &sunMapDesc, sunModMap,
				     spriteBell, spriteKbdCtrl);

	    /*
	     * Reset keyboard to avoid spurious events (No!  don't do now:
	     * doesn't work in Sprite).
	    (void) write(fd, &reset, 1);
	     */
	    break;
	case DEVICE_ON:
	    pKeyboard->on = TRUE;
	    AddEnabledDevice(((KbPrivPtr)pKeyboard->devicePrivate)->fd);
	    /*
	     * Initialize auto-repeat.
	     */
	    repeatPhase = REPEAT_NONE;
	    break;
	case DEVICE_CLOSE:
	case DEVICE_OFF:
	    pKeyboard->on = FALSE;
	    RemoveEnabledDevice(((KbPrivPtr)pKeyboard->devicePrivate)->fd);
	    /*
	     * Make sure auto-repeat doesn't generate events now that the
	     * keyboard is off.
	     */
	    repeatPhase = REPEAT_NONE;
	    break;
    }
    return (Success);
}

/*-
 *-----------------------------------------------------------------------
 * spriteBell --
 *	Ring the terminal/keyboard bell
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	None, really...
 *
 *-----------------------------------------------------------------------
 */
static void
spriteBell (loudness, pKeyboard)
    int	    	  loudness;	    /* Percentage of full volume */
    DevicePtr	  pKeyboard;	    /* Keyboard to ring */
{
    KbPrivPtr		pPriv = (KbPrivPtr)pKeyboard->devicePrivate;
    char		kbdCmd;
    struct timeval	sleepTime;

    if (loudness == 0) {
	return;
    }

    kbdCmd = KBD_BELL_ON;
    (void) write(pPriv->fd, &kbdCmd, 1);

    sleepTime.tv_usec = pPriv->ctrl->bell_duration * 1000;
    sleepTime.tv_sec = 0;
    while (sleepTime.tv_usec >= 1000000) {
	sleepTime.tv_sec += 1;
	sleepTime.tv_usec -= 1000000;
    }
    (void) select(0, (int *) 0, (int *) 0, (int *) 0, &sleepTime);

    kbdCmd = KBD_BELL_OFF;
    (void) write(pPriv->fd, &kbdCmd, 1);
}

/*-
 *-----------------------------------------------------------------------
 * spriteKbdCtrl --
 *	Alter some of the keyboard control parameters
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Some...
 *
 *-----------------------------------------------------------------------
 */
static void
spriteKbdCtrl (pKeyboard, ctrl)
    DevicePtr	  pKeyboard;	    /* Keyboard to alter */
    KeybdCtrl	  *ctrl;    	    /* New control info */
{
    KbPrivPtr	  pPriv;
    char	  kbCmd;

    pPriv = (KbPrivPtr)pKeyboard->devicePrivate;

    pPriv->ctrl = ctrl;
    if (ctrl->click) {
	kbCmd = KBD_CLICK_ON;
    } else {
	kbCmd = KBD_CLICK_OFF;
    }

    (void) write(pPriv->fd, &kbCmd, 1);

    if (!ctrl->autoRepeat) {
	repeatPhase = REPEAT_NONE;
    }
}

/*-
 *-----------------------------------------------------------------------
 * spriteKbdGetEvents --
 *	Return the events waiting in the wings for the given keyboard.
 *
 * Results:
 *	A pointer to an array of Firm_events or (Firm_event *)0 if no events
 *	The number of events contained in the array.
 *
 * Side Effects:
 *	None.
 *-----------------------------------------------------------------------
 */
static Mouse_Event *
spriteKbdGetEvents (pKeyboard, pNumEvents)
    DevicePtr	  pKeyboard;	    /* Keyboard to read */
    int	    	  *pNumEvents;	    /* Place to return number of events */
{
    static Mouse_Event	evBuf[MAXEVENTS];   /* Buffer for input events */

    if (repeatPhase == REPEAT_PENDING) {
	/*
	 * This will only have been set if no streams were really ready, thus
	 * there are no events to read from /dev/mouse.
	 */
	repeatEvent.flags |= KEY_UP;
	repeatEvent.time += repeatDelta / 2;
	evBuf[0] = repeatEvent;

	repeatEvent.flags &= ~KEY_UP;
	repeatEvent.time += repeatDelta / 2;
	evBuf[1] = repeatEvent;

	*pNumEvents = 2;

	repeatPhase = REPEAT_SHORT;
    } else {
	int	    	  nBytes;    /* number of bytes of events available. */
	KbPrivPtr	  pPriv;
	int	    	  i;
	
	pPriv = (KbPrivPtr) pKeyboard->devicePrivate;
	nBytes = read(pPriv->fd, (char *) evBuf, sizeof(evBuf));
	if (nBytes >= 0) {
	    *pNumEvents = nBytes / sizeof (Mouse_Event);
	} else if (errno == EWOULDBLOCK) {
	    *pNumEvents = 0;
	} else {
	    FatalError ("Could not read the keyboard");
	}
    }
    return (evBuf);
}


/*-
 *-----------------------------------------------------------------------
 * spriteKbdProcessEvent --
 *	Transform a Sprite event into an X one.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	An event is passed to DIX. The last key press event is stored in
 *	repeatEvent and repeatPhase set to REPEAT_INITIAL if we're doing
 *	autorepeat and the key is to be repeated.
 *
 *-----------------------------------------------------------------------
 */
static void
spriteKbdProcessEvent (pKeyboard, ev)
    DevicePtr	  pKeyboard;
    Mouse_Event  *ev;
{
    xEvent		xE;
    register KbPrivPtr	pPriv;
    register int  	smask;
    PtrPrivPtr	  	ptrPriv;

    pPriv = (KbPrivPtr)pKeyboard->devicePrivate;
    ptrPriv = (PtrPrivPtr) LookupPointerDevice()->devicePrivate;

    xE.u.keyButtonPointer.time = ev->time;
    xE.u.keyButtonPointer.rootX = ptrPriv->x;
    xE.u.keyButtonPointer.rootY = ptrPriv->y;
    xE.u.u.detail = ev->key + pPriv->offset;
    if (ev->key == ALL_KEYS_UP) {
	xE.u.u.type = KeyRelease;
	repeatPhase = REPEAT_NONE;
    } else if (ev->flags & KEY_UP) {
	xE.u.u.type = KeyRelease;
	if (!(ev->flags & AUTO_GENERATED) &&
	     (repeatPhase != REPEAT_NONE) &&
	     (repeatEvent.key == ev->key)) {
		/*
		 * Turn off repeat if we got a real up event for the key
		 * being repeated
		 */
		repeatPhase = REPEAT_NONE;
	}
    } else {
	xE.u.u.type = KeyPress;
	if ((repeatPhase != REPEAT_SHORT) &&
	    (pPriv->ctrl->autoRepeat ||
	     Bit_IsSet (xE.u.u.detail, pPriv->ctrl->autoRepeats))) {
		 repeatEvent = *ev;
		 repeatEvent.flags |= AUTO_GENERATED;
		 repeatPhase = REPEAT_LONG;
	}
    }

    (* pKeyboard->processInputProc) (&xE, pKeyboard);
}

/*-
 *-----------------------------------------------------------------------
 * spriteDoneEvents --
 *	Nothing to do, here...
 *
 * Results:
 *
 * Side Effects:
 *
 *-----------------------------------------------------------------------
 */
static void
spriteKbdDoneEvents (pKeyboard, final)
    DevicePtr	  pKeyboard;
    Bool	  final;
{
}

/*-
 *-----------------------------------------------------------------------
 * LegalModifier --
 *	See if a key is legal as a modifier. We're very lenient around,
 *	here, so we always return true.
 *
 * Results:
 *	TRUE.
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------
 */
Bool
LegalModifier (key)
    int	    key;
{
    return (TRUE);
}

/*-
 *-----------------------------------------------------------------------
 * spriteBlockHandler --
 *	Tell the OS layer when to timeout to implement auto-repeat.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The timeout value may be overwritten.
 *
 *-----------------------------------------------------------------------
 */
void
spriteBlockHandler (index, pKeyboard, ppTime, pReadMask)
    int	    	    index;    	/* Screen index */
    DevicePtr	    pKeyboard;	/* Keyboard for which to do auto-repeat */
    struct timeval  **ppTime; 	/* Pointer to timeout to use */
    int	    	    *pReadMask;	/* Mask the OS Layer will use for select. */
{
    if (repeatPhase == REPEAT_LONG) {
	/*
	 * Beginning long timeout
	 */
	repeatDelta = REPEAT_LONG_TIMEOUT;
    } else if (repeatPhase == REPEAT_SHORT) {
	/*
	 * Beginning short timeout
	 */
	repeatDelta = REPEAT_SHORT_TIMEOUT;
    } else if (repeatPhase == REPEAT_NONE) {
	/*
	 * No repeat necessary -- it can block as long as it wants to
	 */
	return;
    } else if (repeatPhase == REPEAT_TIMEOUT) {
	/*
	 * Interrupted timeout -- use old timeout (that was modified by
	 * select in the OS layer)
	 */
	*ppTime = &repeatTimeout;
	return;
    }
    repeatTimeout.tv_sec = repeatDelta / 1000;
    repeatTimeout.tv_usec = repeatDelta * 1000;
    repeatPhase = REPEAT_TIMEOUT;
    *ppTime = &repeatTimeout;
}

/*-
 *-----------------------------------------------------------------------
 * spriteWakeupHandler --
 *	Figure out if should do a repeat when the server wakes up. Because
 *	select will modify repeatTimeout to contain the time left, we
 *	can tell if the thing timed out.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	repeatPhase may be changed to REPEAT_PENDING. If it is, *pNumReady
 *	will be set to 1 and the keyboard's stream marked ready in the
 *	result mask.
 *
 *-----------------------------------------------------------------------
 */
void
spriteWakeupHandler (index, pKeyboard, pNumReady, pReadMask)
    int	    	  index;    	/* Screen index */
    DevicePtr	  pKeyboard;	/* Keyboard to repeat */
    int	    	  *pNumReady; 	/* Pointer to number of ready streams */
    int	    	  *pReadMask;	/* Ready streams */
{
    KbPrivPtr	  pPriv;

    pPriv = (KbPrivPtr)pKeyboard->devicePrivate;
    
    if ((repeatPhase == REPEAT_TIMEOUT) && (*pNumReady == 0)) {
	repeatPhase = REPEAT_PENDING;
	Bit_Set (pPriv->fd, pReadMask);
	*pNumReady = 1;
    }
}
