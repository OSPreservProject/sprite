/*-
 * sunIo.c --
 *	Functions to handle input from the keyboard and mouse.
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

#include    "sun.h"
#include    "opaque.h"

int	    	lastEventTime = 0;
extern int	sunSigIO;
extern int      screenIsSaved;
extern void	SaveScreens();

#ifdef SUN_WINDOWS
int	windowFd = 0;
int	sunIgnoreEvent = TRUE;
#define	INPBUFSIZE	128
#endif SUN_WINDOWS

/*-
 *-----------------------------------------------------------------------
 * TimeSinceLastInputEvent --
 *	Function used for screensaver purposes by the os module.
 *
 * Results:
 *	The time in milliseconds since there last was any
 *	input.
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------
 */
int
TimeSinceLastInputEvent()
{
    struct timeval	now;

    gettimeofday (&now, (struct timezone *)0);

    if (lastEventTime == 0) {
	lastEventTime = TVTOMILLI(now);
    }
    return TVTOMILLI(now) - lastEventTime;
}

/*-
 *-----------------------------------------------------------------------
 * ProcessInputEvents --
 *	Retrieve all waiting input events and pass them to DIX in their
 *	correct chronological order. Only reads from the system pointer
 *	and keyboard.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Events are passed to the DIX layer.
 *
 *-----------------------------------------------------------------------
 */
void
ProcessInputEvents ()
{
    register Firm_event    *ptrEvents,    	/* Current pointer event */
			   *kbdEvents;	    	/* Current keyboard event */
    register int	    numPtrEvents, 	/* Number of remaining pointer
						 * events */
			    numKbdEvents;   	/* Number of remaining
						 * keyboard events */
    int	    	  	    nPE,    	    	/* Original number of pointer
						 * events */
			    nKE;    	    	/* Original number of
						 * keyboard events */
    Bool		    PtrAgain,		/* need to (re)read */
			    KbdAgain;		/* need to (re)read */
    DevicePtr		    pPointer;
    DevicePtr		    pKeyboard;
    register PtrPrivPtr     ptrPriv;
    register KbPrivPtr	    kbdPriv;
    Firm_event	  	    *lastEvent;	    	/* Last event processed */
    enum {
	NoneYet, Ptr, Kbd
    }			    lastType = NoneYet;	/* Type of last event */

#ifdef SUN_WINDOWS
    struct inputevent sunevents[INPBUFSIZE];
    register struct inputevent *se = sunevents, *seL;
    int         n;
#endif SUN_WINDOWS

    sunSigIO = 0;
    pPointer = LookupPointerDevice();
    pKeyboard = LookupKeyboardDevice();

    if ( sunUseSunWindows() ) {
#ifdef SUN_WINDOWS
	if ((n=read(windowFd,sunevents,INPBUFSIZE*sizeof sunevents[0])) < 0 
			    && errno != EWOULDBLOCK) {
	    /*
	     * Error reading events; should do something. XXX
	     */
/*debug*/
	ErrorF("ProcessInputEvents: read(windowFd)  n=%d\n",n);
	    return;
	}

	if (autoRepeatKeyDown && autoRepeatReady && n <= 0) {
		/* fake a sunwindows kbd event */
		n = sizeof(struct inputevent);
		se->ie_code = AUTOREPEAT_EVENTID;
		tvplus(event_time(se), autoRepeatLastKeyDownTv,
							autoRepeatDeltaTv);
		if (autoRepeatDebug)
		    ErrorF("ProcessInputEvents: sw auto event\n");
	}

	for (seL = sunevents + (n/(sizeof sunevents[0]));  se < seL; se++) {
	    if (screenIsSaved == SCREEN_SAVER_ON)
		SaveScreens(SCREEN_SAVER_OFF, ScreenSaverReset);
	    lastEventTime = TVTOMILLI(event_time(se));

	    /*
	     * Decide whether or not to pay attention to events.
	     * Ignore the events if the locator has exited X Display.
	     */
	    switch (event_id(se)) {
		case KBD_DONE:
		    sunChangeKbdTranslation( pKeyboard, FALSE );
		    break;
		case KBD_USE:
		    sunChangeKbdTranslation( pKeyboard, TRUE );
		    break;
		case LOC_WINENTER:
		    sunIgnoreEvent = FALSE;
		    break;
		case LOC_WINEXIT:
		    sunIgnoreEvent = TRUE;
		    break;
	    }

	    if (sunIgnoreEvent) {
		continue;
	    }

	    /*
	     * Figure out the X device this event should be reported on.
	     */
	    switch (event_id(se)) {
		case LOC_MOVE:
		case MS_LEFT:
		case MS_MIDDLE:
		case MS_RIGHT:
		    sunMouseProcessEventSunWin(pPointer,se);
		    break;
		case LOC_WINEXIT:
		case LOC_WINENTER:
		case KBD_DONE:
		case KBD_USE:
		    break;
		default:
		    sunKbdProcessEventSunWin(pKeyboard,se);
		    break;
	    }
	}
#endif SUN_WINDOWS
    } 
    else {
	ptrPriv = (PtrPrivPtr)pPointer->devicePrivate;
	kbdPriv = (KbPrivPtr)pKeyboard->devicePrivate;
	
	numPtrEvents = 0;
	PtrAgain = TRUE;
	numKbdEvents = 0;
	KbdAgain = TRUE;
	lastEvent = (Firm_event *)0;

	/*
	 * So long as one event from either device remains unprocess, we loop:
	 * Take the oldest remaining event and pass it to the proper module
	 * for processing. The DDXEvent will be sent to ProcessInput by the
	 * function called.
	 */
	while (1) {
	    /*
	     * Get events from both the pointer and the keyboard, storing the number
	     * of events gotten in nPE and nKE and keeping the start of both arrays
	     * in pE and kE
	     */
	    if ((numPtrEvents == 0) && PtrAgain) {
		ptrEvents = (* ptrPriv->GetEvents) (pPointer, &nPE, &PtrAgain);
		numPtrEvents = nPE;
	    }
	    if ((numKbdEvents == 0) && KbdAgain) {
		kbdEvents = (* kbdPriv->GetEvents) (pKeyboard, &nKE, &KbdAgain);
		numKbdEvents = nKE;
	    }
	    if ((numPtrEvents == 0) && (numKbdEvents == 0))
		break;
	    if (numPtrEvents && numKbdEvents) {
		if (timercmp (&kbdEvents->time, &ptrEvents->time, <)) {
		    if (lastType == Ptr) {
			(* ptrPriv->DoneEvents) (pPointer, FALSE);
		    }
		    (* kbdPriv->ProcessEvent) (pKeyboard, kbdEvents);
		    numKbdEvents--;
		    lastEvent = kbdEvents++;
		    lastType = Kbd;
		} else {
		    if (lastType == Kbd) {
			(* kbdPriv->DoneEvents) (pKeyboard, FALSE);
		    }
		    (* ptrPriv->ProcessEvent) (pPointer, ptrEvents);
		    numPtrEvents--;
		    lastEvent = ptrEvents++;
		    lastType = Ptr;
		}
	    } else if (numKbdEvents) {
		if (lastType == Ptr) {
		    (* ptrPriv->DoneEvents) (pPointer, FALSE);
		}
		(* kbdPriv->ProcessEvent) (pKeyboard, kbdEvents);
		numKbdEvents--;
		lastEvent = kbdEvents++;
		lastType = Kbd;
	    } else {
		if (lastType == Kbd) {
		    (* kbdPriv->DoneEvents) (pKeyboard, FALSE);
		}
		(* ptrPriv->ProcessEvent) (pPointer, ptrEvents);
		numPtrEvents--;
		lastEvent = ptrEvents++;
		lastType = Ptr;
	    }
	}

	if (lastEvent) {
	    lastEventTime = TVTOMILLI(lastEvent->time);
	    if (screenIsSaved == SCREEN_SAVER_ON) {
		SaveScreens(SCREEN_SAVER_OFF, ScreenSaverReset);
	    }
	}
	
	(* kbdPriv->DoneEvents) (pKeyboard, TRUE);
	(* ptrPriv->DoneEvents) (pPointer, TRUE);

    }

}


/*-
 *-----------------------------------------------------------------------
 * SetTimeSinceLastInputEvent --
 *	Set the lastEventTime to now.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	lastEventTime is altered.
 *
 *-----------------------------------------------------------------------
 */
void
SetTimeSinceLastInputEvent()
{
    struct timeval now;

    gettimeofday (&now, (struct timezone *)0);
    lastEventTime = TVTOMILLI(now);
}

/*
 * DDX - specific abort routine.  Called by AbortServer().
 */
void
AbortDDX()
{
    sunNonBlockConsoleOff ((char *) 0);
}

/* Called by GiveUp(). */
void
ddxGiveUp()
{
}

int
ddxProcessArgument (argc, argv, i)
    int	argc;
    char *argv[];
    int	i;
{
    extern void UseMsg();
    extern Bool ActiveZaphod;

    if (strcmp (argv[i], "-ar1") == 0) {	/* -ar1 int */
	if (++i >= argc) UseMsg ();
	autoRepeatInitiate = 1000 * (long)atoi(argv[i]);
	return 2;
    }
    if (strcmp (argv[i], "-ar2") == 0) {	/* -ar2 int */
	if (++i >= argc) UseMsg ();
	autoRepeatDelay = 1000 * (long)atoi(argv[i]);
	return 2;
    }
    if (strcmp (argv[i], "-debug") == 0) {	/* -debug */
	return 1;
    }
    if (strcmp (argv[i], "-dev") == 0) {	/* -dev /dev/mumble */
	if (++i >= argc) UseMsg ();
	return 2;
    }
    if (strcmp (argv[i], "-mono") == 0) {	/* -mono */
	return 1;
    }
    if (strcmp (argv[i], "-zaphod") == 0) {	/* -zaphod */
	ActiveZaphod = FALSE;
	return 1;
    }
    return 0;
}

void
ddxUseMsg()
{
    ErrorF("-ar1 int               set autorepeat initiate time\n");
    ErrorF("-ar2 int               set autorepeat interval time\n");
    ErrorF("-debug                 disable non-blocking console mode\n");
    ErrorF("-dev filename          name of device to open\n");
    ErrorF("-mono                  force monochrome-only screen\n");
    ErrorF("-zaphod                disable active Zaphod mode\n");
}
