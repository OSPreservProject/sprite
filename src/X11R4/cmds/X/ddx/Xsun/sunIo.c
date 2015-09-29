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
#include    <sys/time.h>

int	    	lastEventTime = 0;
extern int	spriteCheckInput;
extern int      screenIsSaved;
extern void	SaveScreens();

extern unsigned int	repeatLong, repeatShort;

/*-
 *-----------------------------------------------------------------------
 * sunInputAvail --
 *	This function is called from the scheduler whenever one of
 *	the devices we told it to look for has input waiting.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	spriteCheckInput has its READ_INPUT flag set.
 *
 *-----------------------------------------------------------------------
 */
void
sunInputAvail()
{
    spriteCheckInput = 1;
}

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
    register Mouse_Event    *events; 	    /* Array of events */
    register int  	    numEvents;	    /* Number of events left */
    int	    	  	    nE;	    	    /* Total number of events */
    DevicePtr		    pPointer;	    /* System pointer */
    DevicePtr		    pKeyboard;	    /* System keyboard */
    register PtrPrivPtr     ptrPriv;	    /* Private data for pointer */
    register KbPrivPtr	    kbdPriv;	    /* Private data for keyboard */
    Mouse_Event  	    *lastEvent;	    /* Last event processed */
    enum {
	NoneYet, Ptr, Kbd
    }			    lastType = NoneYet;	/* Type of last event */

    pPointer = LookupPointerDevice();
    pKeyboard = LookupKeyboardDevice();
    
    ptrPriv = (PtrPrivPtr)pPointer->devicePrivate;
    kbdPriv = (KbPrivPtr)pKeyboard->devicePrivate;
    
    /*
     * Get events from both the pointer and the keyboard via the keyboard's
     * GetEvents vector. The number of events read is stored in numEvents.
     */
    events = (* kbdPriv->GetEvents) (pKeyboard, &nE);
    numEvents = nE;
    
    while (numEvents) {
	if (events->flags & KEYBOARD_EVENT) {
	    if (lastType == Ptr) {
	        (* ptrPriv->DoneEvents) (pPointer, FALSE);
	        lastType = Kbd;
	    }
	    (* kbdPriv->ProcessEvent) (pKeyboard, events);
	} else if (events->flags & MOUSE_EVENT) {
	    if (lastType == Kbd) {
	        (* kbdPriv->DoneEvents) (pKeyboard, FALSE);
	        lastType = Ptr;
	    }
	    (* ptrPriv->ProcessEvent) (pPointer, events);
	} else {
	    /* ??? */
	}
	lastEvent = events;
	lastEventTime = events->time;
	numEvents -= 1;
	events += 1;
    }
    
    SetTimeSinceLastInputEvent();
    if (lastEvent) {
	if (screenIsSaved == SCREEN_SAVER_ON) {
	    SaveScreens(SCREEN_SAVER_OFF, ScreenSaverReset);
	}
    }
    
    (* kbdPriv->DoneEvents) (pKeyboard, TRUE);
    (* ptrPriv->DoneEvents) (pPointer, TRUE);

    spriteCheckInput = 0;
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
	repeatLong = 1000 * (long)atoi(argv[i]);
	return 2;
    }
    if (strcmp (argv[i], "-ar2") == 0) {	/* -ar2 int */
	if (++i >= argc) UseMsg ();
	repeatShort = 1000 * (long)atoi(argv[i]);
	return 2;
    }
#ifndef	sprite
    if (strcmp (argv[i], "-debug") == 0) {	/* -debug */
	return 1;
    }
#endif	sprite
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
#ifndef	sprite
    ErrorF("-debug                 disable non-blocking console mode\n");
    ErrorF("-dev filename          name of device to open\n");
#endif	sprite
    ErrorF("-mono                  force monochrome-only screen\n");
    ErrorF("-zaphod                disable active Zaphod mode\n");
}
