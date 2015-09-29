/*-
 * spriteIo.c --
 *	Functions to handle input from the keyboard and mouse.
 *
 * Copyright (c) 1987, 1989 by the Regents of the University of California
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
	"$Header: spriteIo.c,v 1.3 88/03/27 21:17:12 deboor Exp $ SPRITE (Berkeley)";
#endif lint

#include	"spriteddx.h"
#include	<sys/time.h>


Bool	    	  screenSaved = FALSE;
/*
 * lastEventTime is used by the OS layer to decide when to do the screen
 * saver stuff. lastEventTimeMS is used by the cursor module when it generates
 * motion events in spriteSetCursorPosition. This is necessary because on
 * sun2's, the time in the keyboard events is relative to boot time, while
 * on sun3's it's real time, so what can you do?
 */
unsigned int   	  lastEventTime = 0;
unsigned int	  lastEventTimeMS = 0;

int	    	  spriteCheckInput = 0;
#define RESTORE_CURSOR	0x01
#define READ_INPUT	0x02

/*-
 *-----------------------------------------------------------------------
 * spriteCursorGone --
 *	This is a function required by the cursor code which is
 *	just like spriteInputAvail, except it marks that the cursor
 *	needs to be restored.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	spriteCheckInput is altered to contain the RESTORE_CURSOR flag.
 *
 *-----------------------------------------------------------------------
 */
void
spriteCursorGone()
{
    spriteCheckInput |= RESTORE_CURSOR;
}

/*-
 *-----------------------------------------------------------------------
 * spriteInputAvail --
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
spriteInputAvail()
{
    spriteCheckInput |= READ_INPUT;
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

    gettimeofday(&now, (struct timezone *) NULL);
    lastEventTime = (now.tv_usec/1000) + (now.tv_sec*1000);
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
    struct timeval now;
    int millis;

    gettimeofday(&now, (struct timezone *) NULL);
    millis = (now.tv_usec/1000) + (now.tv_sec*1000);

    if (lastEventTime == 0) {
	lastEventTime = millis;
    }
    return millis - lastEventTime;
}

/*-
 *-----------------------------------------------------------------------
 * ProcessInputEvents --
 *	Retrieve all waiting input events and pass them to DIX in their
 *	correct chronological order. The keyboard driver is responsible
 *	for reading all the events (both for the mouse and the keyboard).
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

    if (spriteCheckInput & READ_INPUT) {
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
	    lastEventTimeMS = events->time;
	    numEvents -= 1;
	    events += 1;
	}
	
	SetTimeSinceLastInputEvent();
	if (screenSaved) {
	    SaveScreens(SCREEN_SAVER_FORCER, ScreenSaverReset);
	}
	
	(* kbdPriv->DoneEvents) (pKeyboard, TRUE);
	(* ptrPriv->DoneEvents) (pPointer, TRUE);
    }


    if (spriteCheckInput & RESTORE_CURSOR) {
	spriteRestoreCursor();
    }
    spriteCheckInput = 0;
}
