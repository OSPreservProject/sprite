/* 
 * wishHandlers.c --
 *
 *	Event handlers and such for flat display.
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
static char rcsid[] = "$Header: /a/newcmds/wish/RCS/wishHandlers.c,v 1.4 89/01/11 11:32:00 mlgray Exp Locker: mgbaker $ SPRITE (Berkeley)";
#endif not lint

#include <sys/types.h>
#include <sys/stat.h>
#include "string.h"
#include "util.h"
#include "monitorClient.h"
#include "wishInt.h"

extern	char	wishStartUp[];


/*
 *----------------------------------------------------------------------
 *
 * WishToggleSelection --
 *
 *	Toggle the selection status of an entry.  The event was selected in
 *	the display window, but we are passed the surrounding window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If the coordinates are those of an entry, it is selected if it
 *	wasn't already selected and it is de-selected if it was already
 *	selected.
 *
 *----------------------------------------------------------------------
 */
void
WishToggleSelection(window, eventPtr, lineP)
    Window			window;
    XButtonPressedEvent		*eventPtr;
    Boolean			lineP;		/* double click for a line? */
{
    short	x, y;
    WishWindow	*aWindow;
    char	buffer[100];

    if (eventPtr->type != ButtonPress && eventPtr->type != ButtonRelease) {
	return;
    }

    if (XFindContext(wishDisplay, window, wishWindowContext,
	(caddr_t) &aWindow) != 0) {
	Sx_Panic(wishDisplay, "Wish didn't recognize given window.");
    }

    /* the event was selected in the display window, so coordinates are ok */
    x = ((XButtonEvent *) eventPtr)->x;
    y = ((XButtonEvent *) eventPtr)->y;

    sprintf(buffer, "toggleSelection %d %d %d", x, y, lineP);

    (void) WishDoCmd(aWindow, buffer);

    return;
}


/*
 *----------------------------------------------------------------------
 *
 * WishEditRule --
 *
 *	The user has typed something inside a group header entry window.
 *	If he has typed a carriage return, then I take the new string and
 *	try to make it the new selection rule for the window.  If there's
 *	something wrong with the new rule, I tell the user and put back the
 *	old rule. If the rule is ok, I ask him (or will do this soon) whether
 *	he wants to make this rule permanent (put it in his .wish file).
 *	Either way, I go and reselect stuff according to the new rule. For now
 *	I do this the dumb way (I don't just redo the particular group --
 *	I redo all groups), but I'll change it if it's too slow.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Many.  Perhaps a new rule set, newly selected files, an edited .wish,
 *	and a newly built display.
 *
 *----------------------------------------------------------------------
 */
void
WishEditRule(window, eventPtr)
    Window	window;
    XKeyPressedEvent	*eventPtr;
{
    WishWindow	*aWindow;
    WishGroup		*groupPtr;
    Boolean		editRuleP = FALSE;
    char		*mapping;
    char		keyString[20];
    int			length;
    char		*buffer;

fprintf(stderr, "WishEditRule called\n");
    if (eventPtr->type != KeyPress && eventPtr->type != KeyRelease) {
	return;
    }
fprintf(stderr, "In WishEditRule.  event was KeyPress or Release.\n");
    if (XFindContext(wishDisplay, window, wishWindowContext,
	    (caddr_t) &aWindow) != 0) {
	Sx_Panic(wishDisplay, "Wish didn't recognize given window.");
    }
    if (XFindContext(wishDisplay, window, wishGroupWindowContext,
	    (caddr_t) &groupPtr) != 0) {
	Sx_Panic(wishDisplay, "Wish didn't recognize given header window.");
    }
    /* Check to see if they've typed a carriage return. */
    length = XLookupString(eventPtr, keyString, 20, (KeySym *) NULL,
	    (XComposeStatus *) NULL);
    
    /* John does stuff here to force ctrl chars and meta bits.  Should I? */
    for (mapping = keyString; length > 0; length--) {
	if (*mapping++ == ctrl('m')) {
	    editRuleP = TRUE;
	    break;
	}
    }
    if (editRuleP == FALSE) {
	return;
    }
    if (groupPtr->defType != COMPARISON) {
	sprintf(wishErrorMsg, "%s %s %s.", "Currently it is only possible to",
		"edit the rules graphically for groups defined",
		"with simple comparisons");
	aWindow->notifierP = TRUE;
	Sx_Notify(wishDisplay, aWindow->displayWindow, -1, -1, 0,
		wishErrorMsg, NULL, TRUE, "Continue", NULL);
	aWindow->notifierP = FALSE;
	/* restore header */
	strcpy(groupPtr->editHeader, groupPtr->rule);
	Sx_EntryMake(wishDisplay, groupPtr->headerWindow,
		aWindow->surroundingWindow, NULL,
		aWindow->fontPtr, aWindow->foreground, aWindow->background,
		groupPtr->editHeader, sizeof (groupPtr->editHeader));
	return;
    }
	
    buffer = (char *) malloc(strlen("changeGroup") + strlen(groupPtr->rule) +
	    strlen("comparison") + strlen(groupPtr->editHeader) + 4); 

    sprintf(buffer, "changeGroup %s %s %s", groupPtr->rule,
	    "comparison", groupPtr->editHeader);

    if (WishDoCmd(aWindow, buffer) != TCL_OK) {
	/* restore header */
	strcpy(groupPtr->editHeader, groupPtr->rule);
	Sx_EntryMake(wishDisplay, groupPtr->headerWindow,
		aWindow->surroundingWindow, NULL,
		aWindow->fontPtr, aWindow->foreground, aWindow->background,
		groupPtr->editHeader, sizeof (groupPtr->editHeader));
    }

    free(buffer);

    return;
}



/*
 *----------------------------------------------------------------------
 *
 * WishEditDir --
 *
 *	The user has typed something in the directory name window.  If he
 *	has typed a carriage return, then I look at the edited string and try to
 *	make it the new dir of the display.  If the string isn't a valid
 *	file or directory, then we notify them, and redisplay the current
 *	dir. If a proper node has been chosen, call WishChangeDir() with it.
 *	The event was selected in the title window, but the window we
 *	are passed is the surrounding window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If a good dir was selected, it becomes the new dir of the display
 *	and we redisplay.
 *
 *----------------------------------------------------------------------
 */
void
WishEditDir(window, eventPtr)
    Window		window;
    XKeyPressedEvent	*eventPtr;
{
    struct	stat	dirAtts;
    WishWindow	*aWindow;
    int			length;
    Boolean		editDirP = FALSE;
    char		*mapping;
    char		keyString[20];

    if (eventPtr->type != KeyPress && eventPtr->type != KeyRelease) {
	return;
    }
    if (XFindContext(wishDisplay, window, wishWindowContext,
	    (caddr_t) &aWindow) != 0) {
	Sx_Panic(wishDisplay, "Wish didn't recognize a given window.");
    }
    /* Check to see if they've typed a carriage return. */
    length = XLookupString(eventPtr, keyString, 20, (KeySym *) NULL,
	    (XComposeStatus *) NULL);
    /* John does stuff here to force ctrl chars and meta bits.  Should I? */
    for (mapping = keyString; length > 0; length--) {
	if (*mapping++ == ctrl('m')) {
	    editDirP = TRUE;
	    break;
	}
    }
    if (editDirP == FALSE) {
	return;
    }
    /*
     * Put new dir name into canonical form.  If this fails, something is
     * wrong with the new name and we restore the current good one in
     * the title window.
     */
    if (Util_CanonicalDir(aWindow->editDir, aWindow->dir, aWindow->editDir) ==
	    NULL) {
	/* error message returned in aWindow->editDir */
	aWindow->notifierP = TRUE;
	Sx_Notify(wishDisplay, aWindow->surroundingWindow, -1, -1, 0,
		aWindow->editDir, NULL, TRUE, "Skip command", (char *) NULL);
	aWindow->notifierP = FALSE;
	strcpy(aWindow->editDir, aWindow->dir);
	Sx_EntryMake(wishDisplay, aWindow->titleWindow,
		aWindow->surroundingWindow, "Directory:  ",
		aWindow->titleFontPtr, aWindow->foreground, aWindow->background,
		aWindow->editDir, sizeof (aWindow->editDir));

	return;
    }

    /*
     * Check validity of the new dir.  If it's no good, restore the name
     * of the old dir in the title window.
     */
    if (lstat(aWindow->editDir, &dirAtts)
	    != 0) {
	sprintf(wishErrorMsg,
		"Cannot switch to dir %s.  Maybe it doesn't exist?",
		aWindow->editDir);
	aWindow->notifierP = TRUE;
	Sx_Notify(wishDisplay, aWindow->surroundingWindow, -1, -1, 0,
		wishErrorMsg, NULL, TRUE, "Skip command", (char *) NULL);
	aWindow->notifierP = FALSE;
	strcpy(aWindow->editDir, aWindow->dir);
	Sx_EntryMake(wishDisplay, aWindow->titleWindow,
		aWindow->surroundingWindow, "Directory:  ",
		aWindow->titleFontPtr, aWindow->foreground, aWindow->background,
		aWindow->editDir, sizeof (aWindow->editDir));

	return;
    }
    if ((dirAtts.st_mode & S_IFMT) != S_IFDIR) {	/* not a directory */
	sprintf(wishErrorMsg, "%s is not a directory.", aWindow->editDir);
	aWindow->notifierP = TRUE;
	Sx_Notify(wishDisplay, aWindow->surroundingWindow, -1, -1, 0,
		wishErrorMsg, NULL, TRUE, "Skip command", (char *) NULL);
	aWindow->notifierP = FALSE;
	strcpy(aWindow->editDir, aWindow->dir);
	Sx_EntryMake(wishDisplay, aWindow->titleWindow,
		aWindow->surroundingWindow, "Directory:  ",
		aWindow->titleFontPtr, aWindow->foreground, aWindow->background,
		aWindow->editDir, sizeof (aWindow->editDir));
	return;
    }

    WishChangeDir(aWindow, aWindow->editDir);

    return;
}


/*
 *----------------------------------------------------------------------
 *
 * WishChangeDir --
 *
 * 	Make the directory of the display be the given directory "where".
 *	If the current directory has been chosen, ignore it since this
 *	won't change anything.  The directory "where" must already have
 *	been canonicalized and checked for existence.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If a new directory was selected, it becomes the new diretory of the
 *	display	and we rebuild the display.  The title window is updated to
 *	show the new directory.
 *
 *----------------------------------------------------------------------
 */
void
WishChangeDir(aWindow, where)
    WishWindow	*aWindow;
    char		*where;		/* Careful -- this parameter may
					 * be aWindow->editDir */
{
    static char 	*command = "insert cd";
    char		*insertcommand;

    /* if the dir has not changed, just return */
    if (strcmp(aWindow->dir, where) == 0) {
	/* Just refresh titleWindow */
	strcpy(aWindow->editDir, aWindow->dir);
	Sx_EntryMake(wishDisplay, aWindow->titleWindow,
		aWindow->surroundingWindow, "Directory:  ",
		aWindow->titleFontPtr, aWindow->foreground, aWindow->background,
		aWindow->editDir, sizeof (aWindow->editDir));
	return;
    }

    if (chdir(where) != 0) {
	sprintf(wishErrorMsg, "Couldn't change directories to %s", where);
	Sx_Panic(wishDisplay, wishErrorMsg);
    }
    if (!MonClient_ChangeDir(aWindow->dir, where)) {
	sprintf(wishErrorMsg, "WishChangeDir: %s",
		"Changing directories in file system monitor failed.");
	Sx_Panic(wishDisplay, wishErrorMsg);
    }

    strcpy(aWindow->dir, where);
    /* Change the title to new dir */
    strcpy(aWindow->editDir, aWindow->dir);
    Sx_EntryMake(wishDisplay, aWindow->titleWindow,
	    aWindow->surroundingWindow, "Directory:  ",
	    aWindow->titleFontPtr, aWindow->foreground, aWindow->background,
	    aWindow->editDir, sizeof (aWindow->editDir));

    /* for window icon */
    XStoreName(wishDisplay, aWindow->surroundingWindow, aWindow->dir); 

    WishGarbageCollect(aWindow);
    /* delete old interpreter and start new one? */
    strcpy(wishCurrentDirectory, aWindow->dir);

    WishSourceConfig(aWindow);

    aWindow->firstElement = 1;		/* start displaying from beginning */
    WishSetPositions(aWindow);
    /* WishRedraw will be called from event caused in WishSetPositions() */
    /* throw a cd into tx window, if there is one.  +4 for space,'\n', & '\0' */
    insertcommand = (char *) malloc(strlen(command) +
	    strlen(wishCurrentDirectory) + 4);
    sprintf(insertcommand, "%s %s\\n", command, wishCurrentDirectory);
    Tx_Command(wishDisplay, aWindow->txOutsideWindow, insertcommand);
    free(insertcommand);

    return;
}


/*
 *----------------------------------------------------------------------
 *
 * WishSourceConfig --
 *
 * 	Source the config files to rebuild the display.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	After a redisplay, anything could be different.
 *
 *----------------------------------------------------------------------
 */
void
WishSourceConfig(aWindow)
    WishWindow	*aWindow;
{
    char	*environVar;
    char	string[MAXPATHLEN];
    struct	stat	attrs;
    struct	stat	localAttrs;
    FILE	*test1;
    FILE	*test2;

    /*
     * This prevents things that would change the display from doing so
     * until a desired redraw event.
     */
    aWindow->dontDisplayChangesP = TRUE;
    /*
     * The implicit assumption here is that we always try to read a
     * .wish file in the user's home directory.  Then we try to
     * read one in the local directory, if it is different from the home
     * directory.  The local one can override things in the home directory
     * .wish file.  I find this awkward.  I think that it should look first for
     * a local one and tbe local one could source the .wish in the home
     * directory, if the user wishes.  But this is to keep wish compatible
     * with mx and things.
     */
    environVar = (char *) getenv("HOME");
    if (environVar != NULL) {
	strcpy(string, environVar);
	strcat(string, "/.wish");
/*
	sprintf(string, "%s/.wish", environVar);
*/
	/*
	 * use open instead of access to test read permission with
	 * effictive, rather than real, uid.  How wasteful.
	 */
	if ((test1 = fopen(string, "r")) != NULL) {
	    (void) stat(string, &attrs);
	    sprintf(string, "source %s/.wish", environVar);
	    (void) WishDoCmd(aWindow, string);
	    (void) fclose(test1);
	}
    }
    /* Make sure we don't source the same file twice */
    /* What if results of void'd WishDoCmd()'s were not TCL_OK? */

	/*
	 * use open instead of access to test read permission with
	 * effictive, rather than real, uid.  How wasteful.
	 */
    if ((test2 = fopen(".wish", "r")) != NULL) {
	(void) stat(".wish", &localAttrs);
	if (attrs.st_ino != localAttrs.st_ino ||
		attrs.st_dev != localAttrs.st_dev ||
		attrs.st_serverID != localAttrs.st_serverID) {
	    (void) WishDoCmd(aWindow, "source .wish");
	}
	(void) fclose(test2);
    }
    /*
     * If there wasn't a user startup file, get the defaults.
     */
    if (test1 == NULL && test2 == NULL) {
	strcpy(string, wishStartUp);
	strcat(string, "/.wish");
	if ((test1 = fopen(string, "r")) != NULL) {
	    (void) stat(string, &attrs);
	    sprintf(string, "source %s/.wish", wishStartUp);
	    (void) WishDoCmd(aWindow, string);
	    (void) fclose(test1);
	} else {
	    Sx_Panic(wishDisplay, "No .wish file found anywhere.  There should at least be one in the same directory as the wish executable.");
	}
    }

    /* If no groups from startup files, then get default "*" group. */
    if (aWindow->groupList == NULL) {
	if (WishGatherNames(aWindow) != TCL_OK) {
	    /* fix here too */
	}
    }

    /* now that we're finished sourcing the file, it's okay to update things. */
    aWindow->dontDisplayChangesP = FALSE;

    return;
}



/*
 *----------------------------------------------------------------------
 *
 * WishHandleDrawingEvent --
 *
 *	Handle an event that requires drawing the display again.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The display is drawn again.  If the window has changed size,
 *	the diplay may be different.
 *
 *----------------------------------------------------------------------
 */
void
WishHandleDrawingEvent(aWindow, eventPtr)
    WishWindow	*aWindow;
    XExposeEvent	*eventPtr;
{
    int	height, width, dummy1;
    Window	dummy2;
    char	buffer[100];

    /* ignore events from child windows */
    if (eventPtr->window != aWindow->surroundingWindow && eventPtr->window !=
	    aWindow->displayWindow) {
	return;
    }
    switch(eventPtr->type) {
    /* This would only report a child resized, anyway, no? */
    case ConfigureNotify:
    case MapNotify:
	XGetGeometry(wishDisplay, aWindow->displayWindow, &dummy2,
		&dummy1, &dummy1, &width, &height, &dummy1, &dummy1);
	if (width != aWindow->windowWidth || height != aWindow->windowHeight) {
	    sprintf(buffer, "resize %d %d", height, width);
	    (void) WishDoCmd(aWindow, buffer);
	    return;
	}
	strcpy(buffer, "redraw");
	(void) WishDoCmd(aWindow, buffer);
	return;
    case Expose:
	/*
	 * Only redraw on the last expose event.
	 */
	if (eventPtr->count != 0) {
	    return;
	}
	strcpy(buffer, "redraw");
	(void) WishDoCmd(aWindow, buffer);
	return;
    default:
	return;
    }
}



/*
 *----------------------------------------------------------------------
 *
 * WishHandleDestructionEvent --
 *
 *	Not yet implemented.  Should do garbage collection and such
 *	once I have got that in place.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
void
WishHandleDestructionEvent(aWindow, eventPtr)
    WishWindow	*aWindow;
    XExposeEvent	*eventPtr;
{
    return;
}


/*
 *----------------------------------------------------------------------
 *
 * WishMouseEvent --
 *
 *	This routine implements the bindings of various mouse
 *	buttons to various commands.  This routine is temporary and will
 *	go away when the command bindings stuff is done.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If a command was chosen, it is executed.
 *
 *----------------------------------------------------------------------
 */
void
WishMouseEvent(window, eventPtr)
    Window		window;		/* the surrounding window */
    XButtonEvent	*eventPtr;
{
    static	int	lastWindow = 0;
    static	int	lastTime = 0;
    static	int	repeatCount = 0;
    static	int	lastX = -1;
    static	int	lastY = -1;
    int		x, y;
    WishFile	*filePtr;
    WishGroup	*groupPtr;
    WishWindow	*aWindow;
    int		button = 0;
    char	*selectButton = NULL;
    char	*command;
    char	buffer[100];


#define	DETAIL_MASK	7

    if (eventPtr->subwindow != NULL) {
	return;
    }
    switch (eventPtr->type) {
    case MotionNotify:
    case LeaveNotify:
	/* Highlight mouse movement */
	WishHighlightMovement(window, eventPtr);
	return;
    case ButtonPress:
	break;
    default:
	return;
    }
    /* Left button is Button1? */
    if ((eventPtr->button & DETAIL_MASK) == Button1) {
	button |= WISH_LEFT_BUTTON;
    }
    if ((eventPtr->button & DETAIL_MASK) == Button2) {
	button |= WISH_MIDDLE_BUTTON;
    }
    if ((eventPtr->button & DETAIL_MASK) == Button3) {
	button |= WISH_RIGHT_BUTTON;
    }
    /* What to do about meta, shift and double buttons??? */

    /*
     * See which group, if any, button is over.  See if it matches a
     * button binding for that group.  If not, see if it matches
     * the selection button.
     */
    
    /* the event was selected in the display window, so coordinates are ok */
    x = ((XButtonEvent *) eventPtr)->x;
    y = ((XButtonEvent *) eventPtr)->y;

    if (XFindContext(wishDisplay, window, wishWindowContext,
	    (caddr_t) &aWindow) != 0) {
	Sx_Panic(wishDisplay, "Wish didn't recognize given window.");
    }
    filePtr = WishMapCoordsToFile(aWindow, x, y);
    if (filePtr != NULL) {
	groupPtr = filePtr->myGroupPtr;
	command = WishGetGroupBinding(groupPtr, button);
	if (command != NULL) {
	    Tcl_SetVar(aWindow->interp, "pointed", filePtr->name, 1);
	    /* do it */
	    (void) WishDoCmd(aWindow, command);
	}
    }
    /* No command, see if selection was meant */
    
    
    /*
     * See if button matches selection button.
     * What do I do about meta's and shifts and such???
     */
    selectButton = Tcl_GetVar(aWindow->interp, "selectionButton", 1);
    if (selectButton == NULL || selectButton[0] == '\0') {
	Tcl_SetVar(aWindow->interp, "selectionButton", "left", 1);
	if (button != WISH_LEFT_BUTTON) {	/* not selection */
	    return;
	}
    } else {
	if (button != WishWhichButton(selectButton)) {	/* not select.*/
	    return;
	}
    }

    /* Prof. Ousterhout does drag scrolling here if the shift key is down */

    /* Count the number of clicks in the same place. */
    if (lastWindow == window && ((eventPtr->time - lastTime) < 50) &&
	    ((lastX + 2) >= eventPtr->x) && ((lastX - 2) <= eventPtr->x) &&
	    ((lastY + 2) >= eventPtr->y) && ((lastY - 2) <= eventPtr->y)) {
	repeatCount += 1;
	if (repeatCount > 2) {
	    repeatCount = 1;
	}
    } else {
	repeatCount = 1;
    }
    lastX = eventPtr->x;
    lastY = eventPtr->y;
    lastTime = eventPtr->time;
    lastWindow = window;
    switch (repeatCount) {
    case 2:
	/* select line */
	sprintf(buffer, "toggleSelection %d %d 1", x, y);
	(void) WishDoCmd(aWindow, buffer);
/*	old...
	WishToggleSelection(window, eventPtr, TRUE);
 */
	break;
    default:
	/* select filename */
	sprintf(buffer, "toggleSelection %d %d 0", x, y);
	(void) WishDoCmd(aWindow, buffer);
/*	old...
	WishToggleSelection(window, eventPtr, FALSE);
 */
	break;
    }
    return;
}


/*
 *----------------------------------------------------------------------
 *
 * WishHandleEnterEvent --
 *
 *	The mouse has entered a window.  Make sure that the current directory
 *	of the program is the directory of that window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The current directory of the program will change to that of the
 *	window, if it is different.
 *
 *----------------------------------------------------------------------
 */
void
WishHandleEnterEvent(aWindow, eventPtr)
    WishWindow	*aWindow;
    XEnterWindowEvent	*eventPtr;
{
    if (eventPtr->type != EnterNotify) {
	return;
    }
    if (strcmp(wishCurrentDirectory, aWindow->dir) == 0) {
	return;
    }
    if (chdir(aWindow->dir) != 0) {
	sprintf(wishErrorMsg, "%s %s.  %s %s",
		"Couldn't change back to directory", aWindow->dir,
		"Does it still exist?  This window no longer makes sense",
		"and will now disappear.");
	aWindow->notifierP = TRUE;
	Sx_Notify(wishDisplay, aWindow->displayWindow, -1, -1, 0,
		wishErrorMsg, NULL, TRUE, "Continue", NULL);
	aWindow->notifierP = FALSE;
	(void) WishDoCmd(aWindow, "close");
	return;
    }
    strcpy(wishCurrentDirectory, aWindow->dir);

    return;
}



/*
 *----------------------------------------------------------------------
 *
 * WishKeyProc --
 *
 *	This procedure is invoked by the Sx dispatcher whenever a
 *	key is typed in an Wish window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Can be almost arbitrary.  Depends completely on the key.
 *
 *----------------------------------------------------------------------
 */
void
WishKeyProc(aWindow, eventPtr)
    WishWindow	*aWindow;	/* Keeps track of information for
					 * window. */
    XKeyEvent *eventPtr;		/* Describes key that was pressed. */
{
    char	keyString[20];
    char	*command, *mapping;
    register	char	c;
    int		result, nBytes;
    char	insertCommand[20];
    Window	w;

    w = aWindow->surroundingWindow;
    if (eventPtr->subwindow != NULL) {
fprintf(stderr, "WishKeyProc w = %d, eventPtr->subwindow == %d\n", w, eventPtr->subwindow);
	return;
    }

    /*
     * Convert from raw key number to its ASCII equivalent, then pass
     * each equivalent character to the keystroke binder.  Process
     * any commands that are matched this way.
     */
    
    nBytes = XLookupString(eventPtr, keyString, 20, (KeySym *) NULL,
	    (XComposeStatus *) NULL);
    /* John does stuff here to force ctrl chars and meta bits.  Should I? */
    for (mapping = keyString; nBytes > 0; nBytes--, mapping++) {
	result = Cmd_MapKey(aWindow->cmdTable, *mapping, &command);
	if (result == CMD_PARTIAL) {
	    continue;
	} else if (result == CMD_UNBOUND) {
	    char	msg[30];

	    WishCvtToPrintable(command, 30, msg);
#ifdef NOTDEF
	    if (aWindow->msgWindow != NULL) {
#endif NOTDEF
		sprintf(wishErrorMsg,
			"The keystroke sequence \"%s\" has no function.", msg);
		aWindow->notifierP = TRUE;
		Sx_Notify(wishDisplay, w, -1, -1, 0, wishErrorMsg,
			aWindow->fontPtr, TRUE, "Continue", (char *) NULL);
		aWindow->notifierP = FALSE;
		return;
#ifdef NOTDEF
	    } else {
		sprintf(wishErrorMsg,
			"The keystroke sequence\n\"%s\"\nhas no function.",
			msg);
		aWindow->notifierP = TRUE;
		(void) Sx_Notify(wishDisplay, aWindow->surroundingWindow,
			-1, -1, 0, wishErrorMsg, aWindow->fontPtr, TRUE,
			"Ignore and go on", NULL);
		aWindow->notifierP = FALSE;
	    }
	    break;
#endif NOTDEF
	} else if (result == CMD_OK) {
	    if (*command == '!') {
		command++;
	    } else {
#ifdef NOTDEF
		Undo_Mark(aWindow->fileInfoPtr->log);
#endif NOTDEF
	    }

	    /*
	     * If the command is "@", then generate an insert command for
	     * the last character typed.
	     */

	    if ((command[0] == '@') && (command[1] == 0)) {
		c = *mapping;
		if (isprint(c) && !isspace(c) && (c != '\\')
			&& (c != '"') && (c != ';') && (c != '$')) {
		    sprintf(insertCommand, "ins %c", c);
		} else {
		    int i = c & 0xff;

		    sprintf(insertCommand, "ins \\%o", i);
		}
		command = insertCommand;
	    }

	    (void) WishDoCmd(aWindow, command);
#ifdef NOTDEF

	    /*
	     * Watch out!  The command could have destroyed the window.
	     */
    
	    if (MxGetMxWindow(w) != aWindow) {
		return;
	    }
#endif NOTDEF
	}
    }
}



/*
 *----------------------------------------------------------------------
 *
 * WishHandleMonitorUpdates --
 *
 *	This routine should be called whenever the monitor informs us
 *	that a change has occured in a directory we are viewing.
 *	Note that the directory-changing code here is almost identical to
 *	that in WishHandlerEnterEvent().  This means I should have
 *	it as a separate routine, soon.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The given window will be updated.
 *
 *----------------------------------------------------------------------
 */
void
WishHandleMonitorUpdates()
{
    int	windowID;
    int	cc;
    char	*ptr;
    int		numbytes;
    WishWindow	*aWindow;
    int		firstElement;
#ifdef MON_DEBUG
    char	buffer[MAXPATHLEN + 1];
#endif MON_DEBUG

    ptr = (char *) (&windowID);
    numbytes = sizeof (windowID);
    cc = 0;
    for ( ; cc != -1 && cc < numbytes; numbytes -= cc, ptr += cc) {
	cc = read(monClient_ReadPort, ptr, numbytes);
    }
    if (cc == -1) {
	perror("WishHandleMonitorUpdates, windowID read: ");
	exit(1);
    }

#ifdef MON_DEBUG
    ptr = buffer;
    numbytes = sizeof (buffer);
    cc = 0;
    for ( ; cc != -1 && cc < numbytes ; numbytes -= cc, ptr += cc) {
	cc = read(monClient_ReadPort, ptr, numbytes);
	if (*(ptr + cc - 1) == '\0') {
	    /* came to end of string */
	    break;
	}
    }
    if (cc == -1) {
	perror("WishHandleMonitorUpdates, path read: ");
	exit(1);
    }
#endif MON_DEBUG

    if (XFindContext(wishDisplay, windowID, wishWindowContext,
	    (caddr_t) &aWindow) != 0) {
	Sx_Panic(wishDisplay, "Wish didn't recognize given window.");
    }
    /*
     * leave aWindow->firstElement the same so that scrolling and such
     * doesn't change.  Later, worry about selection changing.  Also,
     * make sure we're in the same directory as the window we are redoing.
     */
    if (strcmp(wishCurrentDirectory, aWindow->dir) != 0) {
	if (chdir(aWindow->dir) != 0) {
	    sprintf(wishErrorMsg, "%s %s.  %s %s",
		    "Couldn't change back to directory", aWindow->dir,
		    "Does it still exist?  This window no longer makes",
		    "sense and will now disappear.");
	    aWindow->notifierP = TRUE;
	    Sx_Notify(wishDisplay, aWindow->displayWindow, -1, -1, 0,
		    wishErrorMsg, NULL, TRUE, "Continue", NULL);
	    aWindow->notifierP = FALSE;
	    (void) WishDoCmd(aWindow, "close");
	    return;
	}
	strcpy(wishCurrentDirectory, aWindow->dir);
    }
    firstElement = aWindow->firstElement;
    WishGarbageCollect(aWindow);
    WishSourceConfig(aWindow);
    aWindow->firstElement = firstElement;
    WishSetPositions(aWindow);
    /* WishRedraw will be called from event caused in WishSetPositions() */

    return;
}
