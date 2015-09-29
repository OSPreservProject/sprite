/* 
 * FsflatScroll.c --
 *
 *	Routines for dealing with scrolling.
 *
 * Copyright 1987 Regents of the University of California
 * All rights reserved.
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: fsflatScroll.c,v 1.1 88/10/03 12:48:30 mlgray Exp $ SPRITE (Berkeley)";
#endif not lint


#include "string.h"
#include "sx.h"
#include "fsflatInt.h"

/* This is global for now but will go into FsflatWindow structure... */
extern	int	fsflatNumGroupsToHide;
extern	Boolean	fsflatSkipEmptyGroupsP;


/*
 *----------------------------------------------------------------------
 *
 * FsflatScroll --
 *
 *	Scrollbar call-back routine.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The display may scroll.
 *
 *----------------------------------------------------------------------
 */
void
FsflatScroll(clientData, distance, units, window)
    ClientData	clientData;
    float	distance;
    int		units;	/*  SX_SCROLL_ABSOLUTE or ..._PAGES */
    Window	window;
{
    float	top, bottom;
    float	newTop, newBottom;
    FsflatWindow	*aWindow;
    float	displayCover;		
    int		oldFirstElement;
    int		newLastElement;
    int		numDisplayed;
    int		numCouldDisplay;

    aWindow = (FsflatWindow *) clientData;

#ifdef SCROLL_DEBUG
    fprintf(stderr, "Entered FsflatScroll:\n");
    fprintf(stderr, "\tdistance is %f\n", distance);
    fprintf(stderr, "\tunits is %s\n", units == SX_SCROLL_PAGES ?
	    "scroll_pages" : "scroll_absolute");
    fprintf(stderr, "\tfirstElement is %d\n",
	    aWindow->firstElement);
    fprintf(stderr, "\tlastElement is %d\n",
	    aWindow->lastElement);
    fprintf(stderr, "\tnumElements is %d\n",
	    aWindow->numElements);
    fprintf(stderr, "\tnumGroups is %d\n",
	    aWindow->numGroups);
#endif SCROLL_DEBUG

    oldFirstElement = aWindow->firstElement;	/* save to compare */
    numCouldDisplay = aWindow->numRows * (aWindow->usedCol + 1);
    numDisplayed = aWindow->lastElement - aWindow->firstElement + 1;

    if (units == SX_SCROLL_PAGES) {
	/*
	 * If we're supposed to scroll forwards, check to see if the end of
	 * the display has already been reached.
	 */
	if (distance > 0 && (numDisplayed < numCouldDisplay ||
		aWindow->lastElement >= aWindow->totalDisplayEntries)) {
	    return;	/* No point -- already hit end of display */
	}
	aWindow->firstElement += distance * numCouldDisplay;
	/* no point in scrolling so that we waste space in last column */
	if (aWindow->firstElement + numCouldDisplay - 1 >
		aWindow->totalDisplayEntries) {
	    aWindow->firstElement = aWindow->totalDisplayEntries -
		    numCouldDisplay + 1;
	}
	/* The lowest numbered element that makes sense is number 1 */
	if (aWindow->firstElement < 1) {
	    aWindow->firstElement = 1;
	}
	if (aWindow->firstElement == oldFirstElement) {
	    return;	/* no change */
	}
	newLastElement = aWindow->firstElement + numCouldDisplay - 1;

#ifdef SCROLL_DEBUG
	fprintf(stderr,
		"FsflatScroll dealing with page scrolling:\n");
	fprintf(stderr, "\tfirstElement is %d\n",
		aWindow->firstElement);
	fprintf(stderr, "\tnewLastElement is %d\n",
		newLastElement);
	fprintf(stderr, "\tcalling SetPositions and Redraw\n");
#endif SCROLL_DEBUG

	/* scroll */
	FsflatSetPositions(aWindow);
	/*
	 * FsflatRedraw will be called from event caused in
	 * FsflatSetPositions().
	 */

#ifdef SCROLL_DEBUG
	{
	    float	a, b;

	    fprintf(stderr,
		    "End of FsflatScroll dealing with page scrolling:\n");
	    fprintf(stderr, "\tfirstElement is %d\n",
		    aWindow->firstElement);
	    fprintf(stderr, "\tlastElement is %d\n",
		    aWindow->lastElement);
	    Sx_ScrollbarGetRange(fsflatDisplay, window, &a, &b);
	    fprintf(stderr, "\ttop is now %f\n", a);
	    fprintf(stderr, "\tbottom is now %f\n", b);
	}
#endif SCROLL_DEBUG

	return;
    }
    if (units != SX_SCROLL_ABSOLUTE) {
	Sx_Panic(fsflatDisplay,
		"In FsflatScroll(): Unknown scrolling method requested.");
    }
    /* SX_SCROLL_ABSOLUTE from here on */

    Sx_ScrollbarGetRange(fsflatDisplay, window, &top, &bottom);

#ifdef SCROLL_DEBUG
    fprintf(stderr, "\tbefore scrolling, top is %f\n", top);
    fprintf(stderr, "\tbefore scrolling, bottom is %f\n", bottom);
#endif SCROLL_DEBUG

    if (aWindow->numElements <= 0) {
	displayCover = 1.0;
    } else {
	/* The portion of the total entries that can be visible. */
	displayCover = ((float) numCouldDisplay) /
		((float) aWindow->totalDisplayEntries);
	if (displayCover > 1.0) {
	    displayCover = 1.0; 	/* can't cover more than all of it */
	}
    }
#ifdef SCROLL_DEBUG
    fprintf(stderr, "\tdisplayCover is %f\n", displayCover);
#endif SCROLL_DEBUG

    newTop = distance - (displayCover / 2.0);
    newBottom = distance + (displayCover / 2.0);
    if (newTop < 0) {
	newTop = 0.0;
	newBottom = displayCover;
    }
    if (newBottom > 1.0) {
	newBottom = 1.0;
	newTop = 1.0 - displayCover;
    }
    if (newTop == top && newBottom == bottom) {
	return;		/* no change */
    }
    /*
     * I set firstElement from newTop unless newBottom happens to be
     * the very end.  If so, I make sure that the last element should
     * be visible to avoid roundoff errors.
     */
    if (newBottom == 1.0) {
	newLastElement = aWindow->totalDisplayEntries;
	aWindow->firstElement = aWindow->totalDisplayEntries -
		numCouldDisplay + 1;
	/*
	 * If there are fewer things to display than there are spaces,
	 * the above would leave firstElement < 0.  The lowest element
	 * we can start displaying is the first element.  (firstElement
	 * is incremented below by one.)
	 */
	if (aWindow->firstElement < 1) {
	    aWindow->firstElement = 1;
	}
    } else {
	aWindow->firstElement =
		(int) (newTop * aWindow->totalDisplayEntries) + 1;
#ifdef NOTDEF
	/* this seems not to be necessary */
	newLastElement = aWindow->firstElement + numCouldDisplay - 1;
#endif NOTDEF
    }
#ifdef SCROLL_DEBUG
    fprintf(stderr, "In FsflatScroll, after calculations:\n");
    fprintf(stderr, "\tfirstElement is %d\n",
	    aWindow->firstElement);
    fprintf(stderr, "\tnewLastElement is %d\n",
	    newLastElement);
    fprintf(stderr, "\tnewTop is %f\n", newTop);
    fprintf(stderr, "\tnewBottom is %f\n", newBottom);
    fprintf(stderr,
	    "\tcalling FsflatSetPositions, then FsflatRedraw\n");
#endif SCROLL_DEBUG
    if (oldFirstElement == aWindow->firstElement) {
	return;		/* no change */
    }

    FsflatSetPositions(aWindow);

    /* Things are where we want them.  FsflatRedraw() should be called
     * as a result of an event caused in FsflatSetPositions.  FsflatRedraw()
     * will call SetRange again...  Go scroll.
     */
	
#ifdef SCROLL_DEBUG
    {
	float	a, b;

	Sx_ScrollbarGetRange(fsflatDisplay, window, &a, &b);
	fprintf(stderr, "At end of FsflatScroll:\n");
	fprintf(stderr, "\tnewTop was supposed to be %f\n", newTop);
	fprintf(stderr, "\tnewBottom was supposed to be %f\n",
		newBottom);
	fprintf(stderr, "\tRedraw called SetRange and:\n");
	fprintf(stderr, "\ttop is really %f\n", a);
	fprintf(stderr, "\tbottom is really %f\n", b);
	fprintf(stderr, "\tfirstElement is %d\n",
		aWindow->firstElement);
	fprintf(stderr, "\tlastElement is %d\n",
		aWindow->lastElement);
	fprintf(stderr, "\tnumElements is %d\n",
		aWindow->numElements);
    }
#endif SCROLL_DEBUG

    return;
}
