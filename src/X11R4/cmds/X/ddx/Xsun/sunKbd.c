/*-
 * sunKbd.c --
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
 */

/************************************************************
Copyright 1987 by Sun Microsystems, Inc. Mountain View, CA.

                    All Rights Reserved

Permission  to  use,  copy,  modify,  and  distribute   this
software  and  its documentation for any purpose and without
fee is hereby granted, provided that the above copyright no-
tice  appear  in all copies and that both that copyright no-
tice and this permission notice appear in  supporting  docu-
mentation,  and  that the names of Sun or MIT not be used in
advertising or publicity pertaining to distribution  of  the
software  without specific prior written permission. Sun and
M.I.T. make no representations about the suitability of this
software for any purpose. It is provided "as is" without any
express or implied warranty.

SUN DISCLAIMS ALL WARRANTIES WITH REGARD TO  THIS  SOFTWARE,
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FIT-
NESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL SUN BE  LI-
ABLE  FOR  ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,  DATA  OR
PROFITS,  WHETHER  IN  AN  ACTION OF CONTRACT, NEGLIGENCE OR
OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION  WITH
THE USE OR PERFORMANCE OF THIS SOFTWARE.

********************************************************/

#ifndef	lint
static char sccsid[] = "%W %G Copyright 1987 Sun Micro";
#endif


#define Time SpriteTime
#include <fs.h>
#undef Time
#define NEED_EVENTS
#include "sun.h"
#include <bit.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/time.h>
#include "keysym.h"
#include "keymap.h"

#define AUTO_GENERATED	0x10000
#define MIN_KEYCODE	8   	/* Keycode below which we may not transmit */
#define ALL_KEYS_UP	127	/* all keys are up now */

/*
 * Auto-repeat stuff.
 */
static enum {
    REPEAT_LONG,	    /* Start repeat with long timeout */
    REPEAT_SHORT,	    /* Start repeat with short timeout */
    REPEAT_TIMEOUT,	    /* In the middle of a timeout */
    REPEAT_PENDING,	    /* Repeat should be taken next */
    REPEAT_NONE	  	    /* No repeat should happen */
} repeatPhase = REPEAT_NONE;
static Mouse_Event	repeatEvent;	/* Event that will be repeated */
static struct timeval 	repeatTimeout;	/* Timeout to use for repeating */
static unsigned int	repeatDelta;	/* Timeout length (ms) */
/* The following two are set in sunIo.c */
unsigned int		repeatLong=300;	/* Ms delay to begin repeat */
unsigned int		repeatShort=10;	/* Ms delay to begin repeat */

typedef struct {
    int	    	  trans;          	/* Original translation form */
} SunKbPrivRec, *SunKbPrivPtr;

extern void	ProcessInputEvents();
extern void	miPointerPosition();

static void 	  sunBell();
static void 	  sunKbdCtrl();
static Mouse_Event *sunKbdGetEvents();
static void 	  sunKbdProcessEvent();
static void 	  sunKbdDoneEvents();
static KeybdCtrl  sysKbCtrl;

static SunKbPrivRec	sunKbPriv;  
static KbPrivRec  	sysKbPriv = {
    -1,				/* Type of keyboard */
    -1,				/* Descriptor open to device */
    sunKbdGetEvents,		/* Function to read events */
    sunKbdProcessEvent,		/* Function to process an event */
    sunKbdDoneEvents,		/* Function called when all events */
				/* have been handled. */
    (pointer)&sunKbPriv,	/* Private to keyboard device */
    0,				/* offset for device keycodes */
    &sysKbCtrl,			/* Initial full duration = .25 sec. */
};

/*-
 *-----------------------------------------------------------------------
 * sunKbdProc --
 *	Handle the initialization, etc. of a keyboard.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *
 * Note:
 *	When using sunwindows, all input comes off a single fd, stored in the
 *	global windowFd.  Therefore, only one device should be enabled and
 *	disabled, even though the application still sees both mouse and
 *	keyboard.  We have arbitrarily chosen to enable and disable windowFd
 *	in the keyboard routine sunKbdProc rather than in sunMouseProc.
 *
 *-----------------------------------------------------------------------
 */
int
sunKbdProc (pKeyboard, what)
    DevicePtr	  pKeyboard;	/* Keyboard to manipulate */
    int	    	  what;	    	/* What to do to it */
{
    KbPrivPtr	  pPriv;
    register int  kbdFd;

    switch (what) {
	case DEVICE_INIT:
	    if (pKeyboard != LookupKeyboardDevice()) {
		ErrorF ("Cannot open non-system keyboard");
		return (!Success);
	    }
	    
	    /*
	     * First open and find the current state of the keyboard.
	     */
	    if (sysKbPriv.fd < 0) {
	        ReturnStatus status;

		status = Fs_Open("/dev/mouse",
			FS_NON_BLOCKING|FS_READ|FS_WRITE, 0, &sysKbPriv.fd);
	        if (status != 0) {
		    errno = Compat_MapCode(status);
		    Error ("Opening /dev/mouse");
		    return (!Success);
		}
	    }

	    /*
	     * Perform final initialization of the system private keyboard
	     * structure and fill in various slots in the device record
	     * itself which couldn't be filled in before.
	     */
	    pKeyboard->devicePrivate = (pointer)&sysKbPriv;
	    pKeyboard->on = FALSE;
	    sysKbCtrl = defaultKeyboardControl;
	    sysKbPriv.ctrl = &sysKbCtrl;

	    /*
	     * ensure that the keycodes on the wire are >= MIN_KEYCODE
	     */
	     if (sunMapDesc.minKeyCode < MIN_KEYCODE) {
		 int	offset;

		 offset = MIN_KEYCODE - sunMapDesc.minKeyCode;
		 sunMapDesc.minKeyCode += offset;
		 sunMapDesc.maxKeyCode += offset;
		 sysKbPriv.offset = offset;
	     }
	    InitKeyboardDeviceStruct(pKeyboard, &sunMapDesc, sunModMap,
				     sunBell, sunKbdCtrl);
	    break;

	case DEVICE_ON:
	    pPriv = (KbPrivPtr)pKeyboard->devicePrivate;
	    kbdFd = pPriv->fd;
	    AddEnabledDevice(kbdFd);
	    pKeyboard->on = TRUE;
	    repeatPhase = REPEAT_NONE;
	    break;

	case DEVICE_CLOSE:
	case DEVICE_OFF:
	    pPriv = (KbPrivPtr)pKeyboard->devicePrivate;
	    kbdFd = pPriv->fd;
	    RemoveEnabledDevice(kbdFd);
	    pKeyboard->on = FALSE;
	    repeatPhase = REPEAT_NONE;
	    break;
    }
    return (Success);
}

/*-
 *-----------------------------------------------------------------------
 * sunBell --
 *	Ring the terminal/keyboard bell
 *
 * Results:
 *	Ring the keyboard bell for an amount of time proportional to
 *	"loudness."
 *
 * Side Effects:
 *	None, really...
 *
 *-----------------------------------------------------------------------
 */
static void
sunBell (loudness, pKeyboard)
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
 * sunKbdCtrl --
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
sunKbdCtrl (pKeyboard, ctrl)
    DevicePtr	  pKeyboard;	    /* Keyboard to alter */
    KeybdCtrl     *ctrl;
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
 * sunKbdGetEvents --
 *	Return the events waiting in the wings for the given keyboard.
 *
 * Results:
 *	A pointer to an array of Firm_events or (Firm_event *)0 if no events
 *	The number of events contained in the array.
 *	A boolean as to whether more events might be available.
 *	If there are no keyboard events ready and autoRepeatKeyDown > 0,
 *	then *pNumEvents is set to 1 and Firm_event id is set to
 *	AUTOREPEAT_EVENTID.  In sunKbdProcessEvent, if autoRepeatKeyDown > 0
 *	and Firm_event id == AUTOREPEAT_EVENTID, then the event buffer is
 *	ignored and the	event is generated from the last KeyPress event.
 *
 * Side Effects:
 *	None.
 *-----------------------------------------------------------------------
 */
static Mouse_Event *
sunKbdGetEvents (pKeyboard, pNumEvents)
    DevicePtr	  pKeyboard;	    /* Keyboard to read */
    int	    	  *pNumEvents;	    /* Place to return number of events */
{
    int	    	  nBytes;	    /* number of bytes of events available. */
    KbPrivPtr	  pPriv;
    static Mouse_Event	evBuf[MAXEVENTS];   /* Buffer for Firm_events */

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
        pPriv = (KbPrivPtr) pKeyboard->devicePrivate;
        nBytes = read (pPriv->fd, evBuf, sizeof(evBuf));
    
        if (nBytes < 0) {
	    if (errno == EWOULDBLOCK) {
    		*pNumEvents = 0;
	    } else {
    		Error ("Reading keyboard");
    		FatalError ("Could not read the keyboard");
    	    }
        } else {
	    *pNumEvents = nBytes / sizeof (Mouse_Event);
        }
    }

    return (evBuf);
}

/*-
 *-----------------------------------------------------------------------
 * sunKbdProcessEvent --
 *
 * Results:
 *
 * Side Effects:
 *
 * Caveat:
 *      To reduce duplication of code and logic (and therefore bugs), the
 *      sunwindows version of kbd processing (sunKbdProcessEventSunWin())
 *      counterfeits a firm event and calls this routine.  This
 *      couunterfeiting relies on the fact this this routine only looks at the
 *      id, time, and value fields of the firm event which it is passed.  If
 *      this ever changes, the sunKbdProcessEventSunWin will also have to
 *      change.
 *
 *-----------------------------------------------------------------------
 */
static void
sunKbdProcessEvent (pKeyboard, ev)
    DevicePtr	  pKeyboard;
    Mouse_Event	  *ev;
{
    xEvent		xE;
    register KbPrivPtr	pPriv;
    register int  	smask;
    PtrPrivPtr	  	ptrPriv;

    pPriv = (KbPrivPtr)pKeyboard->devicePrivate;
    ptrPriv = (PtrPrivPtr) LookupPointerDevice()->devicePrivate;

    xE.u.keyButtonPointer.time = ev->time;
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
    miPointerPosition (screenInfo.screens[0],
		       &xE.u.keyButtonPointer.rootX,
		       &xE.u.keyButtonPointer.rootY);

    (* pKeyboard->processInputProc) (&xE, pKeyboard, 1);
}

/*-
 *-----------------------------------------------------------------------
 * sunDoneEvents --
 *	Nothing to do, here...
 *
 * Results:
 *
 * Side Effects:
 *
 *-----------------------------------------------------------------------
 */
/*ARGSUSED*/
static void
sunKbdDoneEvents (pKeyboard)
    DevicePtr	  pKeyboard;
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
LegalModifier(key)
{
    return (TRUE);
}

static KeybdCtrl *pKbdCtrl = (KeybdCtrl *) 0;

/*-
 *-----------------------------------------------------------------------
 * sunBlockHandler --
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
sunBlockHandler (index, pKeyboard, ppTime, pReadMask)
    int	    	    index;    	/* Screen index */
    DevicePtr	    pKeyboard;	/* Keyboard for which to do auto-repeat */
    struct timeval  **ppTime; 	/* Pointer to timeout to use */
    int	    	    *pReadMask;	/* Mask the OS Layer will use for select. */
{
    if (repeatPhase == REPEAT_LONG) {
	/*
	 * Beginning long timeout
	 */
	repeatDelta = repeatLong;
    } else if (repeatPhase == REPEAT_SHORT) {
	/*
	 * Beginning short timeout
	 */
	repeatDelta = repeatShort;
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
 * sunWakeupHandler --
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
sunWakeupHandler (index, pKeyboard, pNumReady, pReadMask)
    int	    	  index;    	/* Screen index */
    DevicePtr	  pKeyboard;	/* Keyboard to repeat */
    int	    	  *pNumReady; 	/* Pointer to number of ready streams */
    int	    	  *pReadMask;	/* Ready streams */
{
    KbPrivPtr	  pPriv;

    pPriv = (KbPrivPtr)pKeyboard->devicePrivate;
    
    if ((repeatPhase == REPEAT_TIMEOUT) && (pNumReady == 0)) {
	repeatPhase = REPEAT_PENDING;
	ProcessInputEvents();
    }
}
