/* 
 * wishUtils.c --
 *
 *	Useful stuff -- delivering error msgs, etc.
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
static char rcsid[] = "$Header: /a/newcmds/wish/RCS/wishUtils.c,v 1.4 89/01/11 11:58:50 mlgray Exp $ SPRITE (Berkeley)";
#endif not lint


#include "string.h"
#include "sx.h"
#include "util.h"
#include "wishInt.h"

static	WishWindow	*menuWindow = NULL;


/*
 *----------------------------------------------------------------------
 *
 * WishCvtToPrintable --
 *
 *	Given a keystroke binding that may contain control characters
 *	and/or meta characters, this routine produces a printable version
 *	of the string.
 *
 * Results:
 *	Up to length characters are stored at *result (including the
 *	terminating NULL character).
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
void
WishCvtToPrintable(string, length, result)
    char	*string;		/* Binding string to be converted. */
    int		length;			/* No. of bytes available at result. */
    char	*result;		/* Where to store printable form. */
{
    int		chunkSize;
    char	chunk[20];
    char	*p;

    /*
     * Process the input string one character at a time to do the
     * conversion.
     */

    p = result;
    for ( ; *string != 0; string++) {
	int i;

	/*
	 * Figure out how to represent this particular character.
	 */

	i = *string & 0377;
	if (i <= 040) {
	    if (i == 033) {
		strcpy(chunk, "ESC");
	    } else if (i == '\n') {
		strcpy(chunk, "RET");
	    } else if (i == '\t') {
		strcpy(chunk, "TAB");
	    } else if (i == ' ') {
		strcpy(chunk, "SPACE");
	    } else {
		chunk[0] = 'C';
		chunk[1] = '-';
		chunk[2] = i - 1 + 'a';
		chunk[3] = 0;
	    }
	} else if (i < 0177) {
	    chunk[0] = i;
	    chunk[1] = 0;
	} else if (i == 0177) {
	    strcpy(chunk, "DEL");
	} else if ((i > 0240) && (i < 0377)) {
	    chunk[0] = 'M';
	    chunk[1] = '-';
	    chunk[2] = i & 0177;
	    chunk[3] = 0;
	} else {
	    sprintf(chunk, "%#x", i);
	}

	/*
	 * Add this chunk onto the result string (if it fits), with a
	 * preceding space if this isn't the first chunk.
	 */

	if (p != result) {
	    if (length < 1) {
		break;
	    }
	    *p = ' ';
	    p++;
	    length--;
	}
	chunkSize = strlen(chunk);
	if (length < chunkSize) {
	    strncpy(p, chunk, length);
	    p += length;
	    length = 0;
	    break;
	} else {
	    strcpy(p, chunk);
	    p += chunkSize;
	    length -= chunkSize;
	}
    }

    if (length == 0) {
	p--;
    }
    *p = 0;
}



/*
 *----------------------------------------------------------------------
 *
 * WishMenuProc --
 *
 *	This procedure is invoked whenver a menu command is invoked
 *	in an wish window.  It now forces the interpreter to be the
 *	one for the central display window.  Should I have a separate one
 *	for the menus?
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The command.
 *
 *----------------------------------------------------------------------
 */
void
WishMenuProc(command)
    char	*command;		/* Command string */
{
    Window		w;
    WishWindow	*aWindow;

    if (menuWindow == NULL) {
	return;
    }
    w = menuWindow->surroundingWindow;
    if (XFindContext(wishDisplay, w, wishWindowContext, (caddr_t) &aWindow)
	    != 0) {
	Sx_Panic(wishDisplay, "Wish didn't recognize given window.");
    }

    (void) WishDoCmd(aWindow, command);
#ifdef NOTDEF
    /* could have destroyed window stuff again... */
#endif NOTDEF
}



/*
 *----------------------------------------------------------------------
 *
 * WishMakeMenu --
 *
 *	Duplicate the command strings in an array of menu entries, then
 *	invoke Sx to create a menu.  It's needed in order to make sure
 *	that all of the command strings in all menus, even the initial
 *	default menus, are dynamically allocated.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory gets allocated, and a menu gets created.
 *
 *----------------------------------------------------------------------
 */
void
WishMakeMenu(aWindow, name, size, info)
    register	WishWindow	*aWindow;/* Window in which to create menu. */
    char	*name;				/* Name of menu. */
    int		size;				/* Number of entries in menu. */
    Sx_MenuEntry	info[];		/* Menu inforamtion. */
{
    Sx_MenuEntry	entries[SX_MAX_MENU_ENTRIES];
    int			i;

    for (i = 0; i < size; i++) {
	entries[i] = info[i];
	entries[i].clientData = (ClientData) Util_Strcpy((char *) NULL,
		(char *) info[i].clientData);
    }
fprintf(stderr, "windowFg is %d, windowBg is %d\n", aWindow->menuForeground, aWindow->menuBackground);
    (void) Sx_MenuCreate(wishDisplay, aWindow->menuBar, name, size, entries,
	    aWindow->fontPtr, aWindow->menuForeground,
	    aWindow->menuBackground);
}

/*
 *----------------------------------------------------------------------
 *
 * WishMenuEntryProc --
 *
 *	This procedure is invoked by the Sx dispatcher whenever the
 *	mouse enters a menu bar window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The variable menuWindow is updated to keep track of which
 *	Wish window the cursor's in, so that we'll know when a menu
 *	entry gets invoked.
 *
 *----------------------------------------------------------------------
 */
void
WishMenuEntryProc(aWindow, eventPtr)
    WishWindow *aWindow;		/* Window whose menu bar was just
					 * entered. */
    XEnterWindowEvent *eventPtr;	/* Event describing window entry. */
{
#ifdef NOTDEF
    if (eventPtr->subwindow != NULL) {
	return;
    }
#endif /* NOTDEF */
    menuWindow = aWindow;
}
