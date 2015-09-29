/*-
 * sunMouse.c --
 *	Functions for playing cat and mouse... sorry.
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

#define NEED_EVENTS
#include    "sun.h"
#include    "mipointer.h"
#include    "misprite.h"

Bool ActiveZaphod = TRUE;

static long sunEventTime();
static Bool sunCursorOffScreen();
static void sunCrossScreen();
extern void miPointerQueueEvent();

miPointerCursorFuncRec sunPointerCursorFuncs = {
    sunEventTime,
    sunCursorOffScreen,
    sunCrossScreen,
    miPointerQueueEvent,
};

typedef struct {
    int	    bmask;	    /* Current button state */
    Bool    mouseMoved;	    /* Mouse has moved */
} SunMsPrivRec, *SunMsPrivPtr;

static void 	  	sunMouseCtrl();
static int 	  	sunMouseGetMotionEvents();
static Firm_event 	*sunMouseGetEvents();
static void 	  	sunMouseProcessEvent();
static void 	  	sunMouseDoneEvents();

static SunMsPrivRec	sunMousePriv;

static PtrPrivRec 	sysMousePriv = {
    -1,				/* Descriptor to device */
    sunMouseGetEvents,		/* Function to read events */
    sunMouseProcessEvent,	/* Function to process an event */
    sunMouseDoneEvents,		/* When all the events have been */
				/* handled, this function will be */
				/* called. */
    0,				/* Current x movement of pointer */
    0,				/* Current y movement */
    (pointer)&sunMousePriv,	/* Field private to device */
};

/*-
 *-----------------------------------------------------------------------
 * sunMouseProc --
 *	Handle the initialization, etc. of a mouse
 *
 * Results:
 *	none.
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
sunMouseProc (pMouse, what)
    DevicePtr	  pMouse;   	/* Mouse to play with */
    int	    	  what;	    	/* What to do with it */
{
    register int  fd;
    int	    	  format;
    static int	  oformat;
    BYTE    	  map[4];

    switch (what) {
	case DEVICE_INIT:
	    if (pMouse != LookupPointerDevice()) {
		ErrorF ("Cannot open non-system mouse");	
		return (!Success);
	    }

	    if (! sunUseSunWindows()) {
		if (sysMousePriv.fd >= 0) {
		    fd = sysMousePriv.fd;
		} else {
		    fd = open ("/dev/mouse", O_RDWR, 0);
		    if (fd < 0) {
			Error ("Opening /dev/mouse");
			return (!Success);
		    }
		    if (fcntl (fd, F_SETFL, (FNDELAY|FASYNC)) < 0
			|| fcntl(fd, F_SETOWN, getpid()) < 0) {
			    perror("sunMouseProc");
			    ErrorF("Can't set up mouse on fd %d\n", fd);
			}
		    
		    sysMousePriv.fd = fd;
		}
	    }

	    sunMousePriv.bmask = 0;
	    sunMousePriv.mouseMoved = FALSE;
	    sysMousePriv.dx = 0;
	    sysMousePriv.dy = 0;

	    pMouse->devicePrivate = (pointer) &sysMousePriv;
	    pMouse->on = FALSE;
	    map[1] = 1;
	    map[2] = 2;
	    map[3] = 3;
	    InitPointerDeviceStruct(
		pMouse, map, 3, sunMouseGetMotionEvents, sunMouseCtrl, 0);
	    break;

	case DEVICE_ON:
	    if (! sunUseSunWindows()) {
		if (ioctl (((PtrPrivPtr)pMouse->devicePrivate)->fd,
			VUIDGFORMAT, &oformat) < 0) {
		    Error ("VUIDGFORMAT");
		    return(!Success);
		}
		format = VUID_FIRM_EVENT;
		if (ioctl (((PtrPrivPtr)pMouse->devicePrivate)->fd,
			VUIDSFORMAT, &format) < 0) {
		    Error ("VUIDSFORMAT");
		    return(!Success);
		}
		AddEnabledDevice (((PtrPrivPtr)pMouse->devicePrivate)->fd);
	    }

	    pMouse->on = TRUE;
	    break;

	case DEVICE_CLOSE:
	    if (! sunUseSunWindows()) {
		if (ioctl (((PtrPrivPtr)pMouse->devicePrivate)->fd,
			VUIDSFORMAT, &oformat) < 0) {
		    Error ("VUIDSFORMAT");
		}
	    }
	    break;

	case DEVICE_OFF:
	    pMouse->on = FALSE;
	    if (! sunUseSunWindows()) {
		RemoveEnabledDevice (((PtrPrivPtr)pMouse->devicePrivate)->fd);
	    }
	    break;
    }
    return (Success);
}
	    
/*-
 *-----------------------------------------------------------------------
 * sunMouseCtrl --
 *	Alter the control parameters for the mouse. Since acceleration
 *	etc. is done from the PtrCtrl record in the mouse's device record,
 *	there's nothing to do here.
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
static void
sunMouseCtrl (pMouse)
    DevicePtr	  pMouse;
{
}

/*-
 *-----------------------------------------------------------------------
 * sunMouseGetMotionEvents --
 *	Return the (number of) motion events in the "motion history
 *	buffer" (snicker) between the given times.
 *
 * Results:
 *	The number of events stuffed.
 *
 * Side Effects:
 *	The relevant xTimecoord's are stuffed in the passed memory.
 *
 *-----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
sunMouseGetMotionEvents (buff, start, stop, pScreen)
    CARD32 start, stop;
    xTimecoord *buff;
    ScreenPtr pScreen;
{
    return 0;
}

/*-
 *-----------------------------------------------------------------------
 * sunMouseGetEvents --
 *	Return the events waiting in the wings for the given mouse.
 *
 * Results:
 *	A pointer to an array of Firm_events or (Firm_event *)0 if no events
 *	The number of events contained in the array.
 *	A boolean as to whether more events might be available.
 *
 * Side Effects:
 *	None.
 *-----------------------------------------------------------------------
 */
static Firm_event *
sunMouseGetEvents (pMouse, pNumEvents, pAgain)
    DevicePtr	  pMouse;	    /* Mouse to read */
    int	    	  *pNumEvents;	    /* Place to return number of events */
    Bool	  *pAgain;	    /* whether more might be available */
{
    int	    	  nBytes;	    /* number of bytes of events available. */
    register PtrPrivPtr	  pPriv;
    static Firm_event	evBuf[MAXEVENTS];   /* Buffer for Firm_events */

    pPriv = (PtrPrivPtr) pMouse->devicePrivate;

    nBytes = read (pPriv->fd, evBuf, sizeof(evBuf));

    if (nBytes < 0) {
	if (errno == EWOULDBLOCK) {
	    *pNumEvents = 0;
	    *pAgain = FALSE;
	} else {
	    Error ("Reading mouse");
	    FatalError ("Could not read from mouse");
	}
    } else {
	*pNumEvents = nBytes / sizeof (Firm_event);
	*pAgain = (nBytes == sizeof (evBuf));
    }
    return (evBuf);
}


/*-
 *-----------------------------------------------------------------------
 * MouseAccelerate --
 *	Given a delta and a mouse, return the acceleration of the delta.
 *
 * Results:
 *	The corrected delta
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------
 */
static short
MouseAccelerate (pMouse, delta)
    DevicePtr	  pMouse;
    int	    	  delta;
{
    register int  sgn = sign(delta);
    register PtrCtrl *pCtrl;

    delta = abs(delta);
    pCtrl = &((DeviceIntPtr) pMouse)->ptrfeed->ctrl;

    if (delta > pCtrl->threshold) {
	return ((short) (sgn * (pCtrl->threshold +
				((delta - pCtrl->threshold) * pCtrl->num) /
				pCtrl->den)));
    } else {
	return ((short) (sgn * delta));
    }
}

/*-
 *-----------------------------------------------------------------------
 * sunMouseProcessEvent --
 *	Given a Firm_event for a mouse, pass it off the the dix layer
 *	properly converted...
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The cursor may be redrawn...? devPrivate/x/y will be altered.
 *
 *-----------------------------------------------------------------------
 */
static void
sunMouseProcessEvent (pMouse, fe)
    DevicePtr	  pMouse;   	/* Mouse from which the event came */
    Firm_event	  *fe;	    	/* Event to process */
{
    xEvent		xE;
    register PtrPrivPtr	pPriv;	/* Private data for pointer */
    register SunMsPrivPtr pSunPriv; /* Private data for mouse */
    register int  	bmask;	/* Temporary button mask */

    pPriv = (PtrPrivPtr)pMouse->devicePrivate;
    pSunPriv = (SunMsPrivPtr) pPriv->devPrivate;

    xE.u.keyButtonPointer.time = TVTOMILLI(fe->time);

    switch (fe->id)
    {
    case MS_LEFT:
    case MS_MIDDLE:
    case MS_RIGHT:
	/*
	 * A button changed state. Sometimes we will get two events
	 * for a single state change. Should we get a button event which
	 * reflects the current state of affairs, that event is discarded.
	 *
	 * Mouse buttons start at 1.
	 */
	xE.u.u.detail = (fe->id - MS_LEFT) + 1;
	bmask = 1 << xE.u.u.detail;
	if (fe->value == VKEY_UP) {
	    if (pSunPriv->bmask & bmask) {
		xE.u.u.type = ButtonRelease;
		pSunPriv->bmask &= ~bmask;
	    } else {
		return;
	    }
	} else {
	    if ((pSunPriv->bmask & bmask) == 0) {
		xE.u.u.type = ButtonPress;
		pSunPriv->bmask |= bmask;
	    } else {
		return;
	    }
	}
	/*
	 * If the mouse has moved, we must update any interested client
	 * as well as DIX before sending a button event along.
	 */
	if (pSunPriv->mouseMoved) {
	    sunMouseDoneEvents (pMouse, FALSE);
	}
    
	miPointerPosition (screenInfo.screens[0],
			   &xE.u.keyButtonPointer.rootX,
			   &xE.u.keyButtonPointer.rootY);
    
	(* pMouse->processInputProc) (&xE, pMouse, 1);
	break;
    case LOC_X_DELTA:
	/*
	 * When we detect a change in the mouse coordinates, we call
	 * the cursor module to move the cursor. It has the option of
	 * simply removing the cursor or just shifting it a bit.
	 * If it is removed, DIX will restore it before we goes to sleep...
	 *
	 * What should be done if it goes off the screen? Move to another
	 * screen? For now, we just force the pointer to stay on the
	 * screen...
	 */
	pPriv->dx += MouseAccelerate (pMouse, fe->value);

#ifdef	SUN_ALL_MOTION
	miPointerDeltaCursor (screenInfo.screens[0], pPriv->dx, pPriv->dy, TRUE);
	pPriv->dx = 0;
	pPriv->dy = 0;
#else
	((SunMsPrivPtr)pPriv->devPrivate)->mouseMoved = TRUE;
#endif
	break;
    case LOC_Y_DELTA:
	/*
	 * For some reason, motion up generates a positive y delta
	 * and motion down a negative delta, so we must subtract
	 * here instead of add...
	 */
	pPriv->dy -= MouseAccelerate (pMouse, fe->value);
#ifdef SUN_ALL_MOTION
	miPointerDeltaCursor (screenInfo.screens[0], pPriv->dx, pPriv->dy, TRUE);
	pPriv->dx = 0;
	pPriv->dy = 0;
#else
	((SunMsPrivPtr)pPriv->devPrivate)->mouseMoved = TRUE;
#endif SUN_ALL_MOTION
	break;
    default:
	FatalError ("sunMouseProcessEvent: unrecognized id\n");
	break;
    }
}

/*ARGSUSED*/
static Bool
sunCursorOffScreen (pScreen, x, y)
    ScreenPtr	*pScreen;
    int		*x, *y;
{
    int	    index;

    /*
     * Active Zaphod implementation:
     *    increment or decrement the current screen
     *    if the x is to the right or the left of
     *    the current screen.
     */
    if (ActiveZaphod &&
	screenInfo.numScreens > 1 && (*x >= (*pScreen)->width || *x < 0))
    {
	index = (*pScreen)->myNum;
	if (*x < 0)
	{
	    index = (index ? index : screenInfo.numScreens) - 1;
	    *pScreen = screenInfo.screens[index];
	    *x += (*pScreen)->width;
	}
	else
	{
	    *x -= (*pScreen)->width;
	    index = (index + 1) % screenInfo.numScreens;
	    *pScreen = screenInfo.screens[index];
	}
	return TRUE;
    }
    return FALSE;
}

/*ARGSUSED*/
static long
sunEventTime (pScreen)
    ScreenPtr	pScreen;
{
    return lastEventTime;
}

static void
sunCrossScreen (pScreen, entering)
    ScreenPtr	pScreen;
    Bool	entering;
{
    u_char  select;

    select = 1;
    if (entering)
	select = 0;
    if (sunFbs[pScreen->myNum].EnterLeave)
	(*sunFbs[pScreen->myNum].EnterLeave) (pScreen, select);
}

/*-
 *-----------------------------------------------------------------------
 * sunMouseDoneEvents --
 *	Finish off any mouse motions we haven't done yet. (At the moment
 *	this code is unused since we never save mouse motions as I'm
 *	unsure of the effect of getting a keystroke at a given [x,y] w/o
 *	having gotten a motion event to that [x,y])
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	A MotionNotify event may be generated.
 *
 *-----------------------------------------------------------------------
 */
/*ARGSUSED*/
static void
sunMouseDoneEvents (pMouse,final)
    DevicePtr	  pMouse;
    Bool	  final;
{
    PtrPrivPtr	  pPriv;
    SunMsPrivPtr  pSunPriv;
    int		  dx, dy;

    pPriv = (PtrPrivPtr) pMouse->devicePrivate;
    pSunPriv = (SunMsPrivPtr) pPriv->devPrivate;

    if (pSunPriv->mouseMoved) {
	dx = pPriv->dx;
	dy = pPriv->dy;
	pPriv->dx = 0;
	pPriv->dy = 0;
	pSunPriv->mouseMoved = FALSE;
	miPointerDeltaCursor (screenInfo.screens[0], dx, dy, TRUE);
    }
}

#ifdef SUN_WINDOWS

/*
 * Process a sunwindows mouse event.  The possible events are
 *   LOC_MOVE
 *   MS_LEFT
 *   MS_MIDDLE
 *   MS_RIGHT
 */

void
sunMouseProcessEventSunWin(pMouse,se)
    DeviceRec *pMouse;
    register struct inputevent *se;
{   
    xEvent			xE;
    register int	  	bmask;	/* Temporary button mask */
    register PtrPrivPtr		pPriv;	/* Private data for pointer */
    register SunMsPrivPtr	pSunPriv; /* Private data for mouse */
    short			x, y;

    pPriv = (PtrPrivPtr)pMouse->devicePrivate;

    switch (event_id(se)) {
        case MS_LEFT:
        case MS_MIDDLE:
        case MS_RIGHT:
	    /*
	     * A button changed state. Sometimes we will get two events
	     * for a single state change. Should we get a button event which
	     * reflects the current state of affairs, that event is discarded.
	     *
	     * Mouse buttons start at 1.
	     */
	    pSunPriv = (SunMsPrivPtr) pPriv->devPrivate;
	    xE.u.keyButtonPointer.time = TVTOMILLI(event_time(se));
	    xE.u.u.detail = (event_id(se) - MS_LEFT) + 1;
	    bmask = 1 << xE.u.u.detail;
	    if (win_inputnegevent(se)) {
		if (pSunPriv->bmask & bmask) {
		    xE.u.u.type = ButtonRelease;
		    pSunPriv->bmask &= ~bmask;
		} else {
		    return;
		}
	    } else {
		if ((pSunPriv->bmask & bmask) == 0) {
		    xE.u.u.type = ButtonPress;
		    pSunPriv->bmask |= bmask;
		} else {
		    return;
		}
	    }
	    miPointerPosition (screenInfo.screens[0],
	        &xE.u.keyButtonPointer.rootX, &xE.u.keyButtonPointer.rootY);
            (* pMouse->processInputProc) (&xE, pMouse, 1);
    	    break;
        case LOC_MOVE:
	    /*
	     * Tell mi to go ahead and generate the event.
	     */
	    miPointerMoveCursor(screenInfo.screens[0], event_x(se),
		event_y(se), TRUE);

	    /*
	     * Find out if the mouse got constrained. If it did
	     * then we have to tell SunWindows about it.
	     */
	    miPointerPosition (screenInfo.screens[0], &x, &y);
	    if (x != event_x(se) || y != event_y(se))
	        /*
                 * Tell SunWindows that X is constraining the mouse
                 * cursor so that the server and SunWindows stay in sync.
	         */
	        win_setmouseposition(windowFd, x, y);
	    break;
	default:
	    FatalError ("sunMouseProcessEventSunWin: unrecognized id\n");
	    break;
    }
}
#endif SUN_WINDOWS
