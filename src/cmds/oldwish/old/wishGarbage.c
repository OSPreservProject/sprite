/* 
 * fsflatGarbage.c --
 *
 *	Garbage collection routines.
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
static char rcsid[] = "$Header: fsflatGarbage.c,v 1.1 88/10/03 12:47:37 mlgray Exp $ SPRITE (Berkeley)";
#endif not lint


#ifndef AllPlanes
#include "X11/Xlib.h"
#endif
#include "fsflatInt.h"


/*
 *----------------------------------------------------------------------
 *
 * FsflatGarbageCollect --
 *
 *	Garbage collection.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Stuff is deallocated, destroyed, deleted and otherwise caused to
 *	be missing.
 *
 *----------------------------------------------------------------------
 */
void
FsflatGarbageCollect(aWindow)
    FsflatWindow	*aWindow;
{
    FsflatGroup	*tmpGroupPtr;
    FsflatGroup	*tmpNextGPtr;
    FsflatSelection	*tmpSelPtr, *selPtr;

    FsflatClearWholeSelection(aWindow);
    for (tmpGroupPtr = aWindow->groupList; tmpGroupPtr != NULL;
	    tmpGroupPtr = tmpNextGPtr) {
	tmpNextGPtr = tmpGroupPtr->nextPtr;
	FsflatGarbageGroup(aWindow, tmpGroupPtr);
    }
    aWindow->groupList = NULL;
    aWindow->numElements = -1;
    aWindow->numGroups = -1;
    aWindow->numHiddenGroups = 0;
    aWindow->firstElement = -1;
    aWindow->lastElement = -1;
    for (selPtr = aWindow->selectionList;
	    selPtr != NULL && selPtr->nextPtr != NULL; ) {
	tmpSelPtr = selPtr->nextPtr;
	free(selPtr);
	selPtr = tmpSelPtr;
    }
    aWindow->totalDisplayEntries = 0;

    return;
}


/*
 *----------------------------------------------------------------------
 *
 * FsflatGarbageGroup --
 *
 *	Garbage collection for a single group.  It frees the space for the
 *	group structure as well as all resources allocated to the group.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Stuff is deallocated, destroyed, deleted and otherwise caused to
 *	be missing.
 *
 *----------------------------------------------------------------------
 */
void
FsflatGarbageGroup(aWindow, grpPtr)
    FsflatWindow	*aWindow;
    FsflatGroup		*grpPtr;
{
    FsflatFile	*tmpFilePtr;
    FsflatFile	*tmpNextFPtr;

    if (grpPtr->headerWindow != UNINITIALIZED && grpPtr->headerWindow != 0) {
	XDeleteContext(fsflatDisplay, grpPtr->headerWindow,
		fsflatGroupWindowContext);
	XDeleteContext(fsflatDisplay, grpPtr->headerWindow,
		fsflatWindowContext);
	XDestroyWindow(fsflatDisplay, grpPtr->headerWindow);
    }
    if (grpPtr->fileList == NULL && aWindow->hideEmptyGroupsP) {
	aWindow->numHiddenGroups--;
    }
    aWindow->numGroups--;
    for (tmpFilePtr = grpPtr->fileList; tmpFilePtr != NULL;
	    tmpFilePtr = tmpNextFPtr) {
	tmpNextFPtr = tmpFilePtr->nextPtr;
	if (tmpFilePtr->name != NULL) {
	    free(tmpFilePtr->name);
	}
	aWindow->numElements--;
	free(tmpFilePtr);
    }
    if (grpPtr->rule != NULL) {
	free(grpPtr->rule);
    }
    FsflatDeleteGroupBindings(grpPtr);
    free(grpPtr);

    /*
     * Reset total number of visible elements to the number of files plus
     * the number of headers for visible groups + the number of spaces
     * between visible groups.  This calculation is performed in
     * FsflatGatherNames() and FsflatGatherSingleGroup() as well, but garbage
     * collection of a single group affects this number as well.
     */
    aWindow->totalDisplayEntries = aWindow->numElements +
	    (2 * (aWindow->numGroups - aWindow->numHiddenGroups)) - 1;

    return;
}
