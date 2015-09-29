/* 
 * wishSelect.c --
 *
 *	Routines for selection of listed files and groups.
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
static char rcsid[] = "$Header: /a/newcmds/wish/RCS/wishSelect.c,v 1.4 89/01/11 11:58:41 mlgray Exp $ SPRITE (Berkeley)";
#endif not lint


#include "sx.h"
#include "string.h"
#include "wishInt.h"

/*
 * Forward references for procedures defined below.
 */
extern	void	WishSelChange();
extern	int	WishSelFetch();


/*
 *----------------------------------------------------------------------
 *
 * WishChangeSelection --
 *
 *	Change the selection variable.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The selection variable is modified.
 *
 *----------------------------------------------------------------------
 */
void
WishChangeSelection(aWindow, clientData, fileP, lineP, addToP)
    WishWindow	*aWindow;
    ClientData		clientData;
    Boolean		fileP;	/* selecting a file or a group? */
    Boolean		lineP;	/* select a whole line, or just file name? */
    Boolean		addToP;	/* just change sel. status of one entry */
{
    WishFile	*filePtr;
    WishGroup	*groupPtr;
    WishSelection	*selPtr, *backPtr;

    if (fileP) {
	filePtr = (WishFile *) clientData;
    } else {
	groupPtr = (WishGroup *) clientData;
    }

    if (lineP) {
	aWindow->notifierP = TRUE;
	Sx_Notify(wishDisplay, aWindow->surroundingWindow, -1, -1, 0,
		"Line-mode selection temporarily disabled.",
		NULL, TRUE, "Continue", (char *) NULL);
	aWindow->notifierP = FALSE;
	return;
    }
	
    if (fileP) {
	if (!addToP) {
	    if (filePtr->selectedP && !lineP) {
		WishClearWholeSelection(aWindow);
		return;
	    } else {		/* make the new selection */
		/* check if it's already selected and needs lineP changed. */
		if (filePtr->selectedP == TRUE) {
#ifdef NOTDEF
		    for (selPtr = aWindow->selectionList; selPtr != NULL;
			    selPtr = selPtr->nextPtr) {
			if (selPtr->selected.filePtr == filePtr) {
			    break;
			}
		    }
		    if (selPtr == NULL) {
			sprintf(wishErrorMsg, "%s%s%s", "The selected file `",
				filePtr->name,
				"' couldn't be found on the selection list.");
			Sx_Panic(wishDisplay, wishErrorMsg);
		    }
		    filePtr->lineP = lineP;
		    selPtr->lineP = lineP;
		    WishRedrawFile(aWindow, filePtr);
		    return;
#endif NOTDEF
		}
		/* new selection */
		WishClearWholeSelection(aWindow);
		/*
		 * After the first time, Sx_SelectionSet will call
		 * WishSelChange, which calls WishClearWholeSelection().
		 * This is a warning...
		 */
		Sx_SelectionSet(wishDisplay, WishSelFetch, WishSelChange,
			(ClientData) aWindow);
		Tcl_SetVar(aWindow->interp, "sel", filePtr->name, 1);
		Tcl_SetVar(aWindow->interp, "selection", filePtr->name, 1);
		filePtr->selectedP = TRUE;
		selPtr = (WishSelection *)
			malloc(sizeof (WishSelection));
		selPtr->fileP = TRUE;
		selPtr->selected.filePtr = filePtr;
		selPtr->nextPtr = NULL;
		if (lineP) {
		    filePtr->lineP = TRUE;
		    selPtr->lineP = TRUE;
		} else {
		    selPtr->lineP = FALSE;
		}
		if (aWindow->selectionList != NULL) {
		    sprintf(wishErrorMsg,
			    "Selection list should have been null and wasn't.");
		    Sx_Panic(wishDisplay, wishErrorMsg);
		}
		aWindow->selectionList = selPtr;
		WishRedrawFile(aWindow, filePtr);
	    }
	    return;
	}
	/* addToP is TRUE */
	if (filePtr->selectedP && !lineP) {
	    char	*tclSel;
	    char	*removeEntry;

	    filePtr->selectedP = FALSE;
	    filePtr->lineP = FALSE;
	    tclSel = Tcl_GetVar(aWindow->interp, "sel", 1);
	    if (tclSel == NULL) {
		sprintf(wishErrorMsg, "Tcl selection variable wasn't found.");
		Sx_Panic(wishDisplay, wishErrorMsg);
	    }
	    removeEntry = (char *) malloc(strlen(tclSel) + 1);
	    removeEntry[0] = '\0';
	    /* remove it from selection */
	    for (selPtr = aWindow->selectionList, backPtr = selPtr;
		    selPtr != NULL; selPtr = selPtr->nextPtr) {
		if (!selPtr->fileP) {
		    backPtr = selPtr;
		    continue;
		}
		if (selPtr->selected.filePtr != filePtr) {
		    strcat(removeEntry, filePtr->name);
		    strcat(removeEntry, " ");
		    backPtr = selPtr;
		    continue;
		}
		/* check if nothing is selected anymore */
		if (selPtr == aWindow->selectionList) {
		    aWindow->selectionList = selPtr->nextPtr;
		} else {
		    backPtr->nextPtr = selPtr->nextPtr;
		}
		free(selPtr);
		WishRedrawFile(aWindow, filePtr);
		break;
	    }
	    if (removeEntry[strlen(removeEntry) - 1] == ' ') {
		removeEntry[strlen(removeEntry) - 1] = '\0';
	    }
	    Tcl_SetVar(aWindow->interp, "sel", removeEntry, 1);
	    Tcl_SetVar(aWindow->interp, "selection", removeEntry, 1);
	    free(removeEntry);
	} else {
	    char	*tclSel;
	    char	*addEntry;

	    /* add it to selection */
	    /* check if it's already selected and needs lineP changed. */
	    if (filePtr->selectedP == TRUE) {
#ifdef NOTDEF
		for (selPtr = aWindow->selectionList; selPtr != NULL;
			selPtr = selPtr->nextPtr) {
		    if (selPtr->selected.filePtr == filePtr) {
			break;
		    }
		}
		if (selPtr == NULL) {
		    sprintf(wishErrorMsg, "%s%s%s", "The selected file `",
			    filePtr->name,
			    "' couldn't be found on the selection list.");
		    Sx_Panic(wishDisplay, wishErrorMsg);
		}
		filePtr->lineP = lineP;
		selPtr->lineP = lineP;
		WishRedrawFile(aWindow, filePtr);
		return;
#endif NOTDEF
	    }
	    /* add new selection */	
	    filePtr->selectedP = TRUE;
	    if (lineP) {
		filePtr->lineP = TRUE;
	    }
	    selPtr = (WishSelection *) malloc(sizeof (WishSelection));
	    selPtr->fileP = TRUE;
	    if (lineP) {
		selPtr->lineP = TRUE;
	    } else {
		selPtr->lineP = FALSE;
	    }
	    selPtr->nextPtr = NULL;
	    selPtr->selected.filePtr = filePtr;
	    if (aWindow->selectionList == NULL) {
		/*
		 * After the first time, Sx_SelectionSet will call
		 * WishSelChange, which calls WishClearWholeSelection().
		 * This is a warning...
		 */
		Sx_SelectionSet(wishDisplay, WishSelFetch, WishSelChange,
			(ClientData) aWindow);
		aWindow->selectionList = selPtr;
	    } else {
		for (backPtr = aWindow->selectionList; backPtr->nextPtr != NULL;
			backPtr = backPtr->nextPtr) {
		    /* do nothing */
		}
		backPtr->nextPtr = selPtr;
	    }
	    tclSel = Tcl_GetVar(aWindow->interp, "sel", 1);
	    /* space for 2 strings, blank between and null char */
	    addEntry = (char *) malloc(strlen(tclSel) +
		    strlen(filePtr->name + 2));
	    addEntry[0] = '\0';
	    strcat(addEntry, tclSel);
	    strcat(addEntry, " ");
	    strcat(filePtr->name, tclSel);
	    Tcl_SetVar(aWindow->interp, "sel", addEntry, 1);
	    Tcl_SetVar(aWindow->interp, "selection", addEntry, 1);
	    free(addEntry);
	    WishRedrawFile(aWindow, filePtr);
	    return;
	}
    } else {		/* group selection instead of file selection. */
	/* I don't do tcl selection variable stuff with groups yet. */
	/* lineP stuff doesn't make sense with groups. */
	if (groupPtr->selectedP) {
	    /* remove it from selection */
	    groupPtr->selectedP = FALSE;
	    for (selPtr = aWindow->selectionList, backPtr = selPtr;
		    selPtr != NULL; selPtr = selPtr->nextPtr) {
		if (selPtr->fileP) {
		    backPtr = selPtr;
		    continue;
		}
		if (selPtr->selected.groupPtr != groupPtr) {
		    backPtr = selPtr;
		    continue;
		}
		/* check if nothing is selected anymore */
		if (selPtr == aWindow->selectionList) {
		    aWindow->selectionList = selPtr->nextPtr;
		    if (aWindow->selectionList == NULL) {
		    /*
		     * After the first time, Sx_SelectionSet will call
		     * WishSelChange, which calls WishClearWholeSelection().
		     * This is a warning...
		     */
			Sx_SelectionSet(wishDisplay, WishSelFetch,
				WishSelChange, (ClientData) aWindow);
		    }
		} else {
		    backPtr->nextPtr = selPtr->nextPtr;
		}
		free(selPtr);
		/* not yet implemented */
		WishRedrawGroup(aWindow, groupPtr);
		break;
	    }
	} else {
	    /* add it to selection */
	    groupPtr->selectedP = TRUE;
	    selPtr = (WishSelection *) malloc(sizeof (WishSelection));
	    selPtr->fileP = FALSE;
	    selPtr->lineP = FALSE;
	    selPtr->nextPtr = NULL;
	    selPtr->selected.groupPtr = groupPtr;
	    if (aWindow->selectionList == NULL) {
		if (aWindow->selectionList == NULL) {
		    /*
		     * After the first time, Sx_SelectionSet will call
		     * WishSelChange, which calls WishClearWholeSelection().
		     * This is a warning...
		     */
		    Sx_SelectionSet(wishDisplay, WishSelFetch,
			    WishSelChange, (ClientData) aWindow);
		}
		aWindow->selectionList = selPtr;
	    } else {
		for (backPtr = aWindow->selectionList; backPtr->nextPtr != NULL;
			backPtr = backPtr->nextPtr) {
		    /* do nothing */
		}
		backPtr->nextPtr = selPtr;
	    }
	    /* not yet implemented */
	    WishRedrawGroup(aWindow, groupPtr);
	}
    }

    return;
}



/*
 *----------------------------------------------------------------------
 *
 * WishClearWholeSelection --
 *
 *	Empty the selection.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The selection list disappears.
 *
 *----------------------------------------------------------------------
 */
void
WishClearWholeSelection(aWindow)
    WishWindow	*aWindow;
{
    WishSelection	*selPtr, *nextPtr;

    for (selPtr = aWindow->selectionList; selPtr != NULL; ) {
	if (selPtr->fileP) {
	    selPtr->selected.filePtr->selectedP = FALSE;
	    selPtr->selected.filePtr->lineP = FALSE;
	    WishRedrawFile(aWindow, selPtr->selected.filePtr);
	} else {
	    selPtr->selected.groupPtr->selectedP = FALSE;
	    /* not yet implemented */
	    WishRedrawGroup(aWindow, selPtr->selected.groupPtr);
	}
	nextPtr = selPtr->nextPtr;
	free(selPtr);
	selPtr = nextPtr;
    }
    aWindow->selectionList = NULL;
    Tcl_SetVar(aWindow->interp, "sel", "", 1);
    Tcl_SetVar(aWindow->interp, "selection", "", 1);

    return;
}


/*
 *----------------------------------------------------------------------
 *
 * WishSelFetch --
 *
 *	Called by the Sx selection package when someone wants to know
 *	what's selected.
 *
 * Results:
 *	The number of bytes in the fetched selection, or -1 if there's an error.
 *	(See the Sx_SelectionGet() documentation.)
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
int
WishSelFetch(clientData, askedFormat, firstByte, numBytes, valuePtr,
	formatPtr)
    ClientData	clientData;
    char	*askedFormat;
    int		firstByte;
    int		numBytes;
    char	*valuePtr;
    char	*formatPtr;
{
    WishWindow	*aWindow;
    int			number, n;
    char		*space = NULL;
    char		*copyString;
    WishSelection	*tmpPtr;

    /* Use aWindow as the clientData */
    aWindow = (WishWindow *) clientData;
    strncpy(formatPtr, "text", SX_FORMAT_SIZE);
    if (numBytes < 1) {
	return 0;
    }
    if (firstByte < 0) {
	return -1;
    }
    valuePtr[0] = '\0';

    /*
     * temporary lunacy test - set number 1 less than numBytes for null char.
     * this fixes stack-trashing problem, but i don't see below where I'm
     * writing off the end of the buffer...
     */
    number = numBytes - 1;
    for (tmpPtr = aWindow->selectionList; tmpPtr != NULL && number > 0;
	    tmpPtr = tmpPtr->nextPtr) {
	Boolean	endP = FALSE;

	if (!(tmpPtr->selected.filePtr->selectedP)) {
	    Sx_Panic(wishDisplay,
		    "Inconsistency in selected list discovered.");
	}
	/*
	 * Here I'm assuming that if a group is selected, it won't mean
	 * to go through and grab all the files...  Maybe it can mean the
	 * matching rule or something?  Maybe I need some other selection
	 * mechanism for internal group selection?  Or maybe selecting a
	 * group is just an interface for actual selection of all the
	 * files in it...
	 */
	if (!(tmpPtr->fileP)) {
	    Sx_Panic(wishDisplay, "Can't fetch group selections yet.");
	}
	if (tmpPtr->nextPtr == NULL) {
	    endP = TRUE;
	}
	if (tmpPtr->lineP) {
	    int	i;

	    /*
	     * This is how it's done in WishRedrawFile() and
	     * the column width calculator, but the calculation should
	     * be centralized!
	     */
	    if (space == NULL) {
		space = (char *) malloc(aWindow->maxEntryWidth + 1);
	    }
	    strcpy(space, tmpPtr->selected.filePtr->name);
	    for (i = strlen(space); i < aWindow->maxEntryWidth + 1;
		    i++) {
		space[i] = ' ';
	    }
	    /*
	     * Put in null char so WishGetFileFields concatenates correctly
	     * at space[aWindow->maxNameLength + 2].  The 2 is for spaces
	     * between the name and info fields.  (Again, from
	     * WishRedrawFile().)
	     */
	    space[aWindow->maxNameLength + 2] = '\0';
	    WishGetFileFields(aWindow, tmpPtr->selected.filePtr,
		    &(space[aWindow->maxNameLength + 2]));
	    copyString = space;
	    n = aWindow->maxEntryWidth;
	} else {
	    copyString = tmpPtr->selected.filePtr->name;
	    n = strlen(copyString);
	}

	if (firstByte > 0) {
	    if (n <= firstByte) {
		firstByte -= n;
		if (!endP) {		/* for space between strings */
		    firstByte--;	/* may leave firstByte -= for space */
		}
		continue;
	    } else {
		copyString = copyString + firstByte;
		n -= firstByte;
		firstByte = 0;
	    }
	} else if (firstByte < 0) {
	    /* firstByte is -1 to show we must first copy a space */
	    number--;
	    firstByte = 0;
	    strcat(valuePtr, " ");
	}
	strncat(valuePtr, copyString, number);
	if (number - n < 0) {
	    number = 0;
	    break;
	}
	number -= n;
	if (number > 0 && !endP) {
	    strcat(valuePtr, " ");
	    number--;
	}
    }
    if (space != NULL) {
	free(space);
	space = NULL;
    }

/* temporary lunacy, -1 to avoid counting null char if not at end of buffer */
    if (number == 0) {
	return (numBytes - number);
    } else {
	return (numBytes - number - 1);
    }
}



/*
 *----------------------------------------------------------------------
 *
 * WishSelChange --
 *
 *	Called by the Sx selection package whenever the selection changes
 *	out from under us.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The list of selected nodes is cleared.
 *
 *----------------------------------------------------------------------
 */
void
WishSelChange(clientData)
    ClientData	clientData;
{
    WishWindow	*aWindow;

    aWindow = (WishWindow *) clientData;
    WishClearWholeSelection(aWindow);

    return;
}


/*
 *----------------------------------------------------------------------
 *
 * WishHighlightMovement
 *
 *	Highlight mouse movement to show what file the cursor is over.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The display changes.
 *
 *----------------------------------------------------------------------
 */
void
WishHighlightMovement(window, eventPtr)
    Window		window;
    XPointerMovedEvent	*eventPtr;
{
    WishFile	*filePtr;
    WishFile	*last_filePtr = NULL;
    static	int	oldx = -1;	/* Coordinates outside window. */
    static	int	oldy = -1;
    int		x, y;
    WishWindow	*aWindow;

    if (eventPtr->type != MotionNotify && eventPtr->type != LeaveNotify) {
	return;
    }
    if (XFindContext(wishDisplay, window, wishWindowContext,
	    (caddr_t) &aWindow) != 0) {
	Sx_Panic(wishDisplay, "Fstree didn't recognize given window.");
    }

    /* event was selected in display window, so coordinates are ok */
    x = ((XButtonEvent *) eventPtr)->x;
    y = ((XButtonEvent *) eventPtr)->y;

    if (eventPtr->type != LeaveNotify) {
	filePtr = WishMapCoordsToFile(aWindow, x, y);
    } else {
	filePtr = NULL;
    }
    last_filePtr = WishMapCoordsToFile(aWindow, oldx, oldy);

    if (filePtr == last_filePtr) {	/* leave highlighting alone */
	oldx = x;
	oldy = y;
	return;
    }
    
    /*
     * If we've already toggled a node before, re-toggle it to turn it off
     * as we move away from it.
     */
    if (last_filePtr != NULL) {
	/* I used to toggle highlighting here rather than just turn it off. */
	last_filePtr->highlightP = FALSE;
	WishRedrawFile(aWindow, last_filePtr);
	oldx = oldy = -1;	/* Set to coordinates outside window */
    }

    /*
     * If no new file, do nothing.
     */
    if (filePtr == NULL) {
	return;
    }


    /*
     * toggle new file.
     */
    /*
     * I used to toggle highlighting here rather than just turn it on,
     * but now I don't do selection by toggling, so I needn't here either.
     */
    filePtr->highlightP = TRUE;
    WishRedrawFile(aWindow, filePtr);
    oldx = x;
    oldy = y;

    return;
}
