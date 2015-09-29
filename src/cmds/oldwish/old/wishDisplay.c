/* 
 * fsflatDisplay.c --
 *
 *	Routines for layout and display for the file system flat display.
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
static char rcsid[] = "$Header: fsflatDisplay.c,v 1.1 88/10/03 12:47:25 mlgray Exp $ SPRITE (Berkeley)";
#endif not lint

typedef	int	Boolean;
#define	FALSE	0
#define TRUE	1
#include <sys/types.h>
#include <sys/stat.h>
#include "time.h"
#include <sys/dir.h>	/* included only for MAXNAMLEN */
#include "string.h"
#include "sx.h"
#include "util.h"
#include "monitorClient.h"
#include "fsflatInt.h"

/*
 * Include cursor declarations and definitions.
 * flat.cursor defines the regular flat display cursor and wait.cursor defines
 * the cursor used while the display is busy.
 */
#include "flat.cursor"
#include "wait.cursor"

/*
 * The flat cursor is the regular cursor when the display is
 * idle.  The wait cursor indicates that the display is working
 * away on something.  The regular cursor can be redefined by
 * users, if they wish. (Well... sometime that will be possible...)
 */
static		Cursor		flatCursor = -1;
static		Cursor		waitCursor = -1;

/* Default commands in the overall command table. */
static	CmdInfo	commands[] = {
    {"bind",		FsflatBindCmd},
    {"changeDirectory",	FsflatChangeDirCmd},
    {"quit",		FsflatQuitCmd},
    {"close",		FsflatCloseCmd},
    {"redraw",		FsflatRedrawCmd},
    {"selection",	FsflatSelectionCmd},
    {"resize",		FsflatResizeCmd},
    {"toggleSelection",	FsflatToggleSelectionCmd},
    {"toggleSelEntry",	FsflatToggleSelEntryCmd},
    {"groupBind",	FsflatGroupBindCmd},
    {"open",		FsflatOpenCmd},
    {"sortFiles",	FsflatSortFilesCmd},
    {"setFields",	FsflatChangeFieldsCmd},
    {"defineGroup",	FsflatDefineGroupCmd},
    {"changeGroup",	FsflatChangeGroupCmd},
    {"pattern",		FsflatPatternCompareCmd},
    {"menu",		FsflatMenuCmd},
    {"exec",		FsflatExecCmd},
    {(char *) NULL,	(int (*)()) NULL}
};

/*
 * Number of open windows, for graceful exit when we close them all.
 */
int	fsflatWindowCount = 0;

/*
/*
 * Used before defined.
 */
extern	void	GetColumnWidthInfo();
extern	int	GetMaxEntryWidth();
extern	void	FigureWindowSize();



/*
 *----------------------------------------------------------------------
 *
 * FsflatCreate --
 *
 *	Create an fsflat window.
 *
 * Results:
 *	A pointer to the created FsflatWindow structure, or NULL if the
 *	call failed.
 *
 * Side effects:
 *	A new fsflat window will be created and displayed.
 *
 *----------------------------------------------------------------------
 */
FsflatWindow *
FsflatCreate(aWindow, dir)
    FsflatWindow	*aWindow;	/* parent window */
    char		*dir;		/* directory to start in */
{
    FsflatWindow	*newWindow;
    struct	stat	dirAtts;
    Tx_WindowInfo	info;
    static		char	*args[] = {"/bin/csh", "-i", NULL};
    XSizeHints		sizeHints;
    XGCValues		gcValues;

    newWindow = (FsflatWindow *) malloc(sizeof (FsflatWindow));
    if (dir != NULL) {
	/* quick hack until i fix selection problems */
	if (dir[strlen(dir) - 1] == ' ') {
	    dir[strlen(dir) - 1] = '\0';
	}
	if (Util_CanonicalDir(dir, NULL, newWindow->dir) == NULL) {
	    /* error message returned in newWindow->dir */
	    /* if null, then we were called from main() anyway */
	    if (aWindow->interp == NULL) {
		Sx_Panic(fsflatDisplay, newWindow->dir);
	    }
	    strcpy(aWindow->interp->result, newWindow->dir);
	    free(newWindow);
	    return NULL;
	}
    } else {
	strcpy(newWindow->dir, aWindow->dir);
    }
    if (lstat(newWindow->dir, &dirAtts)
	    != 0) {
	/* if null, then we were called from main() anyway */
	if (aWindow->interp == NULL) {
	    sprintf(fsflatErrorMsg,
		    "Can't get attributes for %s.  Maybe it doesn't exist",
		    newWindow->dir);
	    Sx_Panic(fsflatDisplay, fsflatErrorMsg);
	}
	sprintf(aWindow->interp->result,
		"Can't get attributes for %s.  Maybe it doesn't exist",
		newWindow->dir);
	free(newWindow);
	return NULL;
    }
    /* Put name of dir in the editable string for the title display window */
    strcpy(newWindow->editDir, newWindow->dir);

    if ((dirAtts.st_mode & S_IFMT) != S_IFDIR) {	/* not a directory */
	if (aWindow->interp == NULL) {
	    sprintf(fsflatErrorMsg, "%s is not a directory", newWindow->dir);
	    Sx_Panic(fsflatDisplay, fsflatErrorMsg);
	}
	sprintf(aWindow->interp->result, "%s is not a directory",
		newWindow->dir);
	free(newWindow);
	return NULL;
    }
     
    if (chdir(newWindow->dir) != 0) {
	if (aWindow->interp == NULL) {
	    sprintf(fsflatErrorMsg,
		    "Couldn't change directories to %s", newWindow->dir);
	    Sx_Panic(fsflatDisplay, fsflatErrorMsg);
	}
	sprintf(aWindow->interp->result,
		"Couldn't change directories to %s", newWindow->dir);
	free(newWindow);
	return NULL;
    }
    strcpy(fsflatCurrentDirectory, newWindow->dir);

    newWindow->foreground = aWindow->foreground;
    newWindow->background = aWindow->background;
    newWindow->selection = aWindow->selection;
    newWindow->border = aWindow->border;
    newWindow->borderWidth = aWindow->borderWidth;
    newWindow->fontPtr = aWindow->fontPtr;
    newWindow->titleForeground = aWindow->titleForeground;
    newWindow->titleBackground = aWindow->titleBackground;
    newWindow->titleBorder = aWindow->titleBorder;
    newWindow->titleFontPtr = aWindow->titleFontPtr;
    newWindow->txForeground = aWindow->txForeground;
    newWindow->txBackground = aWindow->txBackground;
    newWindow->txBorder = aWindow->txBorder;
    newWindow->menuForeground = aWindow->menuForeground;
    newWindow->menuBackground = aWindow->menuBackground;
    newWindow->sortForeground = aWindow->sortForeground;
    newWindow->sortBackground = aWindow->sortBackground;
    newWindow->fieldsForeground = aWindow->fieldsForeground;
    newWindow->fieldsBackground = aWindow->fieldsBackground;
    newWindow->entryForeground = aWindow->entryForeground;
    newWindow->entryBackground = aWindow->entryBackground;
    newWindow->scrollForeground = aWindow->scrollForeground;
    newWindow->scrollBackground = aWindow->scrollBackground;
    newWindow->scrollElevator = aWindow->scrollElevator;
    newWindow->geometry = aWindow->geometry;
    newWindow->firstElement = UNINITIALIZED;
    newWindow->numGroups = UNINITIALIZED;
    newWindow->numHiddenGroups = 0;
    newWindow->hideEmptyGroupsP = aWindow->hideEmptyGroupsP;

    /*
     * Create the first window and subwindows and use the window id to
     * identify the application's data.
     */
    if (newWindow->geometry != NULL) {
	(void) XParseGeometry(newWindow->geometry, &sizeHints.x, &sizeHints.y,
		&sizeHints.width, &sizeHints.height);
    }
    newWindow->surroundingWindow = XCreateSimpleWindow(fsflatDisplay,
	    DefaultRootWindow(fsflatDisplay), sizeHints.x, sizeHints.y,
	sizeHints.width, sizeHints.height, newWindow->borderWidth,
	    newWindow->border, newWindow->background);
    if (newWindow->surroundingWindow == 0) {
	if (aWindow->interp == NULL) {
	    Sx_Panic(fsflatDisplay, "Couldn't create a new window.");
	}
	sprintf(aWindow->interp->result, "couldn't create a new window");
	goto cleanUp;
    }
    /* for window icon */
    XStoreName(fsflatDisplay, newWindow->surroundingWindow, newWindow->dir);

    /* Create textGc */
    gcValues.foreground = aWindow->foreground;
    gcValues.background = aWindow->background;
    gcValues.font = newWindow->fontPtr->fid;
    newWindow->textGc = XCreateGC(fsflatDisplay, newWindow->surroundingWindow,
	    GCForeground | GCBackground | GCFont, &gcValues);
    /* Create reverseGc */
    gcValues.foreground = aWindow->background;
    gcValues.background = aWindow->foreground;
    gcValues.font = newWindow->fontPtr->fid;
    newWindow->reverseGc = XCreateGC(fsflatDisplay,
	    newWindow->surroundingWindow,
	    GCForeground | GCBackground | GCFont, &gcValues);


    /*
     * To get a nice divider between the title/entry window and the display
     * window, create the windows with no border and create a divider as
     * a separate, thin, window.  The title window is also a command entry
     * window which is why I don't just use Sx_TitleCreate().
     */
    newWindow->titleWindow = Sx_CreatePacked(fsflatDisplay,
	    newWindow->surroundingWindow,
	    SX_TOP, Sx_DefaultHeight(fsflatDisplay, newWindow->titleFontPtr),
	    0, 0, newWindow->titleBorder, (Window) 0,
	    newWindow->titleBackground);
    if (newWindow->titleWindow == 0) {
	if (aWindow->interp == NULL) {
	    Sx_Panic(fsflatDisplay, "Couldn't create title window.");
	}
	sprintf(aWindow->interp->result, "couldn't create title window");
	goto cleanUp;
    }
    /* Put "title" in window. */
    Sx_EntryMake(fsflatDisplay, newWindow->titleWindow, "Directory:  ",
	    newWindow->titleFontPtr, newWindow->titleForeground,
	    newWindow->titleBackground, newWindow->editDir,
	    sizeof (newWindow->editDir));

    newWindow->txOutsideWindow = Sx_CreatePacked(fsflatDisplay,
	    newWindow->surroundingWindow,
	    SX_BOTTOM, 8 * Sx_DefaultHeight(fsflatDisplay, newWindow->fontPtr),
	    0, 0, newWindow->txBorder, (Window) 0, newWindow->txBackground);

    /* Unless i can get this info from Sx, how else can I do it? */
    {
	Window	dummy1;
	int	x, y, width, height, border_width, dummy2;

	if (XGetGeometry(fsflatDisplay, newWindow->txOutsideWindow, &dummy1,
		&x, &y, &width, &height, &border_width, &dummy2) == 0) {
	    Sx_Panic(fsflatDisplay, "Couldn't get tx window geometry.");
	}
	info.width = width;
    }

    /* Create first divider */
    newWindow->divider1Window = Sx_CreatePacked(fsflatDisplay,
	    newWindow->surroundingWindow,
	    SX_TOP, 1, 0, 0, newWindow->border, (Window) 0, newWindow->border);
    if (newWindow->divider1Window == 0) {
	if (aWindow->interp == NULL) {
	    Sx_Panic(fsflatDisplay,
		    "Couldn't create the first border between windows.");
	}
	sprintf(aWindow->interp->result,
		"couldn't create the first border between windows");
	goto cleanUp;
    }
    newWindow->menuBar = Sx_CreatePacked(fsflatDisplay,
	    newWindow->surroundingWindow, SX_TOP,
	    Sx_DefaultHeight(fsflatDisplay, newWindow->titleFontPtr), 0, 0,
	    newWindow->menuForeground, (Window) 0, newWindow->menuBackground);
    /* Create next divider */
    newWindow->divider2Window = Sx_CreatePacked(fsflatDisplay,
	    newWindow->surroundingWindow,
	    SX_TOP, 1, 0, 0, newWindow->border, (Window) 0, newWindow->border);
    if (newWindow->divider2Window == 0) {
	if (aWindow->interp == NULL) {
	    Sx_Panic(fsflatDisplay,
		    "Couldn't create the second border between windows.");
	}
	sprintf(aWindow->interp->result,
		"couldn't create the second border between windows");
	goto cleanUp;
    }
    newWindow->sortWindow = Sx_TitleCreate(fsflatDisplay,
	newWindow->surroundingWindow,
	SX_TOP, Sx_DefaultHeight(fsflatDisplay, newWindow->fontPtr), 0,
	newWindow->fontPtr,
	newWindow->sortForeground, newWindow->sortBackground,
	newWindow->sortBackground, NULL, NULL, NULL);
    if (newWindow->sortWindow == 0) {
	if (aWindow->interp == NULL) {
	    Sx_Panic(fsflatDisplay, "Couldn't create the sort window.");
	}
	sprintf(aWindow->interp->result, "couldn't create the sort window");
	goto cleanUp;
    }
    newWindow->divider3Window = Sx_CreatePacked(fsflatDisplay,
	    newWindow->surroundingWindow,
	    SX_TOP, 1, 0, 0, newWindow->border, (Window) 0, newWindow->border);
    if (newWindow->divider3Window == 0) {
	if (aWindow->interp == NULL) {
	    Sx_Panic(fsflatDisplay,
		    "Couldn't create the third border between windows.");
	}
	sprintf(aWindow->interp->result,
		"couldn't create the third border between windows");
	goto cleanUp;
    }
    newWindow->fieldsWindow = Sx_TitleCreate(fsflatDisplay,
	newWindow->surroundingWindow,
	SX_TOP, Sx_DefaultHeight(fsflatDisplay, newWindow->fontPtr), 0,
	newWindow->fontPtr,
	newWindow->fieldsForeground, newWindow->fieldsBackground,
	newWindow->fieldsBackground, NULL, NULL, NULL);
    if (newWindow->fieldsWindow == 0) {
	if (aWindow->interp == NULL) {
	    Sx_Panic(fsflatDisplay, "Couldn't create the fields window.");
	}
	sprintf(aWindow->interp->result, "couldn't create fields window");
	goto cleanUp;
    }
    /* Create next divider */
    newWindow->divider4Window = Sx_CreatePacked(fsflatDisplay,
	    newWindow->surroundingWindow,
	    SX_TOP, 1, 0, 0, newWindow->border, (Window) 0, newWindow->border);
    if (newWindow->divider4Window == 0) {
	if (aWindow->interp == NULL) {
	    Sx_Panic(fsflatDisplay,
		    "Couldn't create the fourth border between windows.");
	}
	sprintf(aWindow->interp->result,
		"couldn't create the fourth border between windows");
	goto cleanUp;
    }

    /*
     * Is 1 really the size I want for the scrollbar border?
     */
    newWindow->scrollWindow = Sx_ScrollbarCreate(fsflatDisplay,
	    newWindow->surroundingWindow,
	    SX_RIGHT, 1, newWindow->scrollForeground,
	    newWindow->scrollBackground,
	    newWindow->scrollElevator, FsflatScroll, newWindow);
    if (newWindow->scrollWindow == 0) {
	if (aWindow->interp == NULL) {
	    Sx_Panic(fsflatDisplay, "Couldn't create the scroll bar.");
	}
	sprintf(aWindow->interp->result, "couldn't create the scrollbar");
	goto cleanUp;
    }
    /* Create next divider */
    newWindow->divider5Window = Sx_CreatePacked(fsflatDisplay,
	    newWindow->surroundingWindow, SX_RIGHT, 1, 0, 0, newWindow->border,
	    (Window) 0, newWindow->border);
    if (newWindow->divider5Window == 0) {
	if (aWindow->interp == NULL) {
	    Sx_Panic(fsflatDisplay,
		    "Couldn't create the fifth border between windows.");	
	}
	sprintf(aWindow->interp->result,
		"couldn't create the fifth border between windows");
	goto cleanUp;
    }
#ifdef NOTDEF
/* what was this for in X10? */
    tx_RegisterPtr = FALSE;
#endif /* NOTDEF */
    info.source = NULL;
    info.fontPtr = newWindow->fontPtr;
    info.height = 4 * Sx_DefaultHeight(fsflatDisplay, newWindow->fontPtr);
    info.foreground = newWindow->txForeground;
    info.background = newWindow->txBackground;
    info.flags = MX_RAW;
    Tx_GetTermAndTitle(newWindow->txOutsideWindow, NULL, &info);
    XStoreName(fsflatDisplay, newWindow->txOutsideWindow, info.title);
    Tx_Make(fsflatDisplay, newWindow->txOutsideWindow, &info, Tx_InputProc,
	    (ClientData) NULL);
    Tx_Shell(fsflatDisplay, newWindow->txOutsideWindow, &info, args);
    /* Create next divider */
    newWindow->divider6Window = Sx_CreatePacked(fsflatDisplay,
	    newWindow->surroundingWindow,
	    SX_BOTTOM, 1, 0, 0, newWindow->border, (Window) 0,
	    newWindow->border);
    if (newWindow->divider6Window == 0) {
	if (aWindow->interp == NULL) {
	    Sx_Panic(fsflatDisplay,
		    "Couldn't create the sixth border between windows.");
	}
	sprintf(aWindow->interp->result,
		"couldn't create the sixth border between windows");
	goto cleanUp;
    }
    /* Create display subwindow */
    newWindow->displayWindow = Sx_CreatePacked(fsflatDisplay,
	    newWindow->surroundingWindow,
	    SX_TOP, 0, 1, 0, newWindow->border, (Window) 0,
	    newWindow->background);
    if (newWindow->displayWindow == 0) {
	if (aWindow->interp == NULL) {
	    Sx_Panic(fsflatDisplay, "Couldn't create the display window.");
	}
	sprintf(aWindow->interp->result, "couldn't create display window");
cleanUp:
	if (newWindow->surroundingWindow != NULL) {
	    XDestroySubwindows(fsflatDisplay, newWindow->surroundingWindow);
	    XDestroyWindow(fsflatDisplay, newWindow->surroundingWindow);
	}
	free(newWindow);
	return NULL;
    }

    /*
     * Increment the window reference count and enter the window information
     * structure into the fsflatWindowContext.
     */
    fsflatWindowCount++;
    XSaveContext(fsflatDisplay, newWindow->surroundingWindow,
	    (caddr_t) fsflatWindowContext, newWindow);

    /*
     * Start up the display with newWindow->dir as the directory of the display.
     * FsflatInit() will map the surrounding window.  It may first change
     * its size.
     */
    FsflatInit(newWindow);
    newWindow->firstElement = 1;	/* start displaying from beginning */
    /*
     * FsflatSetPositions() and FsflatRedraw() will be called as a result
     * of the handlers, since Sx stuff generates an ExposureMask event.
     * This means I don't need to call them here explicitly.
     */

    /* For the handlers, should I pass the window or newWindow as clientData? */
    Sx_HandlerCreate(fsflatDisplay, newWindow->surroundingWindow,
	    EnterWindowMask, FsflatHandleEnterEvent, newWindow);
    /*
     * Select these button events in the display window, but pass the
     * surrounding window to the handlers.
     */
    Sx_HandlerCreate(fsflatDisplay, newWindow->displayWindow, ButtonPressMask
	    | PointerMotionMask | LeaveWindowMask, FsflatMouseEvent,
	    newWindow->surroundingWindow);
    /*
     * Select these key press events in the title subwindow, but
     * pass the surrounding window to the handlers.
     */
    Sx_HandlerCreate(fsflatDisplay, newWindow->titleWindow, KeyPressMask,
	    FsflatEditDir, newWindow->surroundingWindow);

    /*
     * Move this further up before various datastructures are created?
     * Add dir to fsflat's list.  Use windowID to match this dir for this
     * window.
     */
    if (!MonClient_AddDir(newWindow->dir, newWindow->surroundingWindow)) {
	/* What should I do here? */
	Sx_Panic(fsflatDisplay,
		"File system monitor failed in FsflatCreate().");
    }

    return newWindow;
}


/*
 *----------------------------------------------------------------------
 *
 * FsflatInit --
 *
 *	Initialize a new flat display.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Lots.
 *
 *----------------------------------------------------------------------
 */
void
FsflatInit(aWindow)
    FsflatWindow	*aWindow;	/* info for this instance of display */
{
    XColor	cursorForeground;
    XColor	cursorBackground;
    Pixmap	source;

    cursorForeground.pixel = aWindow->background;
    cursorBackground.pixel = aWindow->foreground;
    XQueryColor(fsflatDisplay, DefaultColormap(fsflatDisplay,
	    DefaultScreen(fsflatDisplay)), &cursorForeground);
    XQueryColor(fsflatDisplay, DefaultColormap(fsflatDisplay,
	    DefaultScreen(fsflatDisplay)), &cursorBackground);
    if (flatCursor == -1) {
	source = XCreateBitmapFromData(fsflatDisplay,
		DefaultRootWindow(fsflatDisplay), flatCursor_bits,
		flatCursor_width, flatCursor_height);
	flatCursor = XCreatePixmapCursor(fsflatDisplay, source, None,
		&cursorBackground, &cursorForeground, flatCursor_x_hot,
		flatCursor_y_hot);
	XFreePixmap(fsflatDisplay, source);
    }
    /* We'll need the cursor later - don't free it */
    XDefineCursor(fsflatDisplay, aWindow->displayWindow, flatCursor);

    /*
     * When work is being done, the cursor changes to the "wait" cursor.
     */
    if (waitCursor == -1) {
	source = XCreateBitmapFromData(fsflatDisplay,
		DefaultRootWindow(fsflatDisplay), waitCursor_bits,
		waitCursor_width, waitCursor_height);
	waitCursor = XCreatePixmapCursor(fsflatDisplay, source, None,
		&cursorBackground, &cursorForeground, waitCursor_x_hot,
		waitCursor_y_hot);
	XFreePixmap(fsflatDisplay, source);
    }
    {
	Window	dummy1;
	int	x, y, width, height, border_width, dummy2;

	if (XGetGeometry(fsflatDisplay, aWindow->displayWindow, &dummy1,
		&x, &y, &width, &height, &border_width, &dummy2) == 0) {
	    Sx_Panic(fsflatDisplay, "Couldn't get display window geometry.");
	}
	FsflatSetWindowAndRowInfo(aWindow, height, width);
    }

    /* initialize data structures */
    aWindow->displayInstructions = 0;
    aWindow->sortingInstructions = 0;
    aWindow->maxNameLength = 0;		/* longest length name */
    aWindow->maxEntryWidth = 0;		/* longest entry */
    aWindow->numElements = -1;		/* >= 0 means already initialized */
    aWindow->totalDisplayEntries = 0;
    /* numRows and rowHeight set above */
    aWindow->firstElement = aWindow->lastElement = -1;
    /* 8 columns is probably more than plenty to start out with. */
    aWindow->columns = (FsflatColumn *) malloc(8 * sizeof (FsflatColumn));
    aWindow->maxColumns = 8;
    aWindow->usedCol = -1;
    if (fsflatResizeP) {
	aWindow->resizeP = TRUE;
    } else {
	aWindow->resizeP = FALSE;
    }
    if (fsflatPickSizeP) {
	aWindow->pickSizeP = TRUE;
    } else {
	aWindow->pickSizeP = FALSE;
    }
    if (fsflatShowEmptyGroupsP) {
	aWindow->hideEmptyGroupsP = FALSE;
    } else {
	aWindow->hideEmptyGroupsP = TRUE;
    }
    aWindow->groupList = NULL;
    aWindow->selectionList = NULL;
    /* create command bindings table and tcl interpreter. */
    FsflatCmdTableInit(&(aWindow->cmdTable), &(aWindow->interp),
	    commands, (ClientData) aWindow);

    FsflatSourceConfig(aWindow);

    /* figure initial size */
    if (aWindow->pickSizeP) {
	aWindow->pickSizeP = FALSE;		/* only the first time */
	/* This next will cause the surrounding window to change shape */
	FigureWindowSize(aWindow, GetMaxEntryWidth(aWindow));
    }
    /* map surrounding  window */
    XMapWindow(fsflatDisplay, aWindow->surroundingWindow);

    Sx_HandlerCreate(fsflatDisplay, aWindow->displayWindow,
	    ExposureMask | StructureNotifyMask,
	    FsflatHandleDrawingEvent, (ClientData) aWindow);
    Sx_HandlerCreate(fsflatDisplay, aWindow->displayWindow, KeyPressMask,
	    FsflatKeyProc, (ClientData) aWindow);
    Sx_HandlerCreate(fsflatDisplay, aWindow->menuBar, EnterWindowMask,
	    FsflatMenuEntryProc, (ClientData) aWindow);

    return;
}


/*
 *----------------------------------------------------------------------
 *
 * FsflatSetPositions --
 *
 *	Figure out and set the coordinates of things to display.
 *	This means sticking things in columns and determining when
 *	the display would be filled up.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The coordinates change.  A new column or so may be allocated.
 *
 *----------------------------------------------------------------------
 */
void
FsflatSetPositions(aWindow)
    FsflatWindow	*aWindow;
{
    FsflatGroup		*tmpGroupPtr = NULL;
    FsflatFile		*tmpFilePtr = NULL;
    FsflatColumn	*newColumns;
    Boolean		spaceP = FALSE;
    int			numCols;
    int			i;
    int			maxEntryWidth;
    int			numMappedHeaders = 0;	/* Use at end for ignoring
						 * extra expose events. */

#ifdef SCROLL_DEBUG
    fprintf(stderr, "Entered FsflatSetPositions:\n");
    fprintf(stderr, "\tfirstElement is %d\n",
	    aWindow->firstElement);
    fprintf(stderr, "\tlastElement is %d\n",
	    aWindow->lastElement);
    fprintf(stderr, "\tnumElements is %d\n",
	    aWindow->numElements);
    fprintf(stderr, "\tcolumns used is %d\n",
	    aWindow->usedCol + 1);
#endif SCROLL_DEBUG

    if (aWindow->firstElement <= 0) {
	sprintf(fsflatErrorMsg,
		"Entered FsflatSetPositions with firstElement = %d",
		aWindow->firstElement);
	Sx_Panic(fsflatDisplay, fsflatErrorMsg);
    }
    /*
     * Avoid exposure events from creating and resizing windows below.  The
     * window will be remapped later causing FsflatRedraw() to be called.
     */
    XUnmapWindow(fsflatDisplay, aWindow->displayWindow);

    maxEntryWidth = GetMaxEntryWidth(aWindow);
    if (aWindow->resizeP) {
	FigureWindowSize(aWindow, maxEntryWidth);
    }
    GetColumnWidthInfo(aWindow, maxEntryWidth, &(aWindow->columnWidth),
	    &numCols);
#ifdef SCROLL_DEBUG
    fprintf(stderr, "Number of columns to use is %d\n", numCols);
#endif SCROLL_DEBUG

    /* do we need to allocate more columns? */
    if (numCols > aWindow->maxColumns) {
	newColumns = (FsflatColumn *)
		malloc(aWindow->maxColumns * 2 * sizeof (FsflatColumn));
	for (i = 0; i < aWindow->maxColumns; i++) {
	    newColumns[i] = aWindow->columns[i];
	}
	free(aWindow->columns);
	aWindow->columns = newColumns;
	aWindow->maxColumns *= 2;
    }

    /* get to the first one to display */
    i = 0;
    aWindow->lastElement = 0;
    for (tmpGroupPtr = aWindow->groupList; tmpGroupPtr != NULL;
	    tmpGroupPtr = tmpGroupPtr->nextPtr) {
	/*
	 * Check if we've hit the right element to start displaying at.  If
	 * so, then check that either it's okay to display headers for empty
	 * groups, or else this isn't an empty group.
	 */
	if ((i + 1 == aWindow->firstElement) && (!aWindow->hideEmptyGroupsP ||
		tmpGroupPtr->fileList != NULL)) {
	    break;
	}
	tmpGroupPtr->myColumn = -1;
	tmpGroupPtr->x = tmpGroupPtr->y = -1;
	if (tmpGroupPtr->headerWindow != UNINITIALIZED) {
	    XUnmapWindow(fsflatDisplay, tmpGroupPtr->headerWindow);
	}
	/* only increment if this was a visible header */
	if (!aWindow->hideEmptyGroupsP || tmpGroupPtr->fileList != NULL) {
	    i++;
	}
	for (tmpFilePtr = tmpGroupPtr->fileList; tmpFilePtr != NULL;
		tmpFilePtr = tmpFilePtr->nextPtr) {
	    if (i + 1 == aWindow->firstElement) {
		break;
	    } else {
		tmpFilePtr->x = tmpFilePtr->y = -1;
		tmpFilePtr->myColumn = -1;
	    }
	    i++;
	}
	if (tmpFilePtr != NULL) {
	    break;	/* we found it */
	}
	if (i+1 == aWindow->firstElement) {
	    /*
	     * The next thing we would display is a space followed by a
	     * group header, unless the group is an empty group and we aren't
	     * supposed to be displaying empty groups...
	     */
	    if (!aWindow->hideEmptyGroupsP || (tmpGroupPtr->nextPtr != NULL &&
		    tmpGroupPtr->nextPtr->fileList != NULL)) {
		spaceP = TRUE;
		tmpGroupPtr = tmpGroupPtr->nextPtr;
		break;
	    }
	}
	/* only increment if this would be a space after a visible header */
	if (!aWindow->hideEmptyGroupsP || tmpGroupPtr->fileList != NULL) {
	    i++;
	}
    }
    if (tmpGroupPtr == NULL) {		/* all groups were empty, no display */
	goto finished;
    }
    /* We've figured out what to start displaying */

    /* Now fill in the columns */
    for (aWindow->usedCol = 0; aWindow->usedCol < numCols; aWindow->usedCol++) {
	aWindow->columns[aWindow->usedCol].x =
		aWindow->usedCol * aWindow->columnWidth;

	for (i = 0; i < aWindow->numRows; i++) {
	    if (spaceP == TRUE) {
		spaceP = FALSE;
		continue;
	    }
	    /*
	     * If tmpFilePtr is NULL, we have come to a new group header.
	     */
	    if (tmpFilePtr == NULL) {
		/* set tmpFilePtr to fileList for this group. */

		tmpFilePtr = tmpGroupPtr->fileList;
		if (aWindow->hideEmptyGroupsP && tmpFilePtr == NULL) {
		    if (tmpGroupPtr->headerWindow != UNINITIALIZED) {
			XUnmapWindow(fsflatDisplay, tmpGroupPtr->headerWindow);
		    }
		    tmpGroupPtr = tmpGroupPtr->nextPtr;
		    i--;	/* so that i is the same next iteration */
		    if (tmpGroupPtr == NULL) {	/* end of groups */
			goto finished;
		    }
		    continue;
		}
		tmpGroupPtr->myColumn = aWindow->usedCol;
		tmpGroupPtr->x = aWindow->columns[aWindow->usedCol].x;
		tmpGroupPtr->y = i * aWindow->rowHeight;
		strcpy(tmpGroupPtr->editHeader, tmpGroupPtr->rule);
		if (tmpGroupPtr->headerWindow == UNINITIALIZED) {
		    /* Need to create a header window */
/* necessary? */    tmpGroupPtr->entry_width = aWindow->columnWidth;
		    tmpGroupPtr->entry_x = tmpGroupPtr->x;
		    tmpGroupPtr->entry_y = tmpGroupPtr->y;
		    /*
		     * If it's the first column, then start it one pixel
		     * to the left, so that borders overlap.
		     */
		    if (tmpGroupPtr->myColumn > 0) {
			tmpGroupPtr->headerWindow =
				Sx_EntryCreate(fsflatDisplay,
				aWindow->displayWindow,
				tmpGroupPtr->x, tmpGroupPtr->y - 1,
				aWindow->columnWidth - 1,
				aWindow->rowHeight - 1, 1, NULL,
				aWindow->fontPtr, aWindow->entryForeground,
				aWindow->entryBackground,
				tmpGroupPtr->editHeader,
				sizeof (tmpGroupPtr->editHeader));
		    } else {
			tmpGroupPtr->headerWindow =
				Sx_EntryCreate(fsflatDisplay,
				aWindow->displayWindow,
				tmpGroupPtr->x - 1, tmpGroupPtr->y - 1,
				aWindow->columnWidth,
				aWindow->rowHeight - 1, 1, NULL,
				aWindow->fontPtr, aWindow->entryForeground,
				aWindow->entryBackground,
				tmpGroupPtr->editHeader,
				sizeof (tmpGroupPtr->editHeader));
		    }
		    if (tmpGroupPtr->headerWindow == 0) {
			Sx_Panic(fsflatDisplay,
				"You've run out of windows?!  Farewell!");
		    }
		    XSaveContext(fsflatDisplay, tmpGroupPtr->headerWindow,
			    fsflatWindowContext, (caddr_t) aWindow);
		    XSaveContext(fsflatDisplay, tmpGroupPtr->headerWindow,
			    fsflatGroupWindowContext, (caddr_t) tmpGroupPtr);
		    Sx_HandlerCreate(fsflatDisplay,
			    tmpGroupPtr->headerWindow, KeyPressMask,
			    FsflatEditRule, tmpGroupPtr->headerWindow);
		} else {
		    /* is the window in a new place? */
		    if (tmpGroupPtr->entry_x != tmpGroupPtr->x ||
			    tmpGroupPtr->entry_y != tmpGroupPtr->y ||
			    tmpGroupPtr->entry_width !=
			    aWindow->columnWidth) {
			XWindowChanges	changes;

			tmpGroupPtr->entry_x = tmpGroupPtr->x;
			tmpGroupPtr->entry_y = tmpGroupPtr->y;
			tmpGroupPtr->entry_width = aWindow->columnWidth;
			if (tmpGroupPtr->myColumn > 0) {
			    changes.x = tmpGroupPtr->x;
			    changes.y = tmpGroupPtr->y - 1;
			    changes.width = aWindow->columnWidth - 1;
			    changes.height = aWindow->rowHeight - 1;
			} else {
			    changes.x = tmpGroupPtr->x - 1;
			    changes.y = tmpGroupPtr->y - 1;
			    changes.width = aWindow->columnWidth;
			    changes.height = aWindow->rowHeight - 1;
			}
			XConfigureWindow(fsflatDisplay,
				tmpGroupPtr->headerWindow,
				CWX | CWY | CWWidth | CWHeight, &changes);
		    }
		}
		/* in case it wasn't visible last time */
		XMapWindow(fsflatDisplay, tmpGroupPtr->headerWindow);
		numMappedHeaders++;	/* another header window mapped */
		/*
		 * If fileList is NULL for this group, move to next group.
		 * If this new group isn't null, it means we need another space.
		 * Otherwise, we're finished with all the groups.
		 */
		if (tmpFilePtr != NULL) {
		    continue;
		}
		/*
	 	 * The group's fileList is null, move on to new group.
		 * We can only get here with a null fileList if
		 * aWindow->hideEmptyGroupsP is FALSE.
		 */
		tmpGroupPtr = tmpGroupPtr->nextPtr;
		if (tmpGroupPtr == NULL) {
		    /* end of groups */
		    aWindow->lastElement = i + 1;
		    break;
		}
		/* leave tmpFilePtr NULL so that we know we're between groups */
		spaceP = TRUE;
		continue;
	    }
	    /* tmpFilePtr was not NULL, so set positions for the file name. */
	    tmpFilePtr->x = aWindow->columns[aWindow->usedCol].x;
	    tmpFilePtr->y = i * aWindow->rowHeight;
	    tmpFilePtr->myColumn = aWindow->usedCol;

	    /* move on to next file */
	    tmpFilePtr = tmpFilePtr->nextPtr;
	    if (tmpFilePtr == NULL) {
		/* end of group */
		tmpGroupPtr = tmpGroupPtr->nextPtr;
		if (tmpGroupPtr == NULL) {
		    /* end of groups */
		    aWindow->lastElement = i + 1;
		    break;
		}
		spaceP = TRUE;
	    }
	    continue;
	}

	/* We're at the end of a column */
	if (tmpGroupPtr == NULL) {
	    /* end of groups */
	    aWindow->usedCol++;		/* so it ends one bigger than used */
	    break;
	}
    }

    aWindow->usedCol--;		/* loop leaves it one too big */
finished:

    /* mark the rest as not visible */
    for ( ; tmpGroupPtr != NULL; tmpGroupPtr = tmpGroupPtr->nextPtr) {
	/* If we stopped at a header, mark it as not visible */
	if (tmpFilePtr == NULL) {
	    tmpGroupPtr->myColumn = -1;
	    tmpGroupPtr->x = tmpGroupPtr->y = -1;
	    if (tmpGroupPtr->headerWindow != UNINITIALIZED) {
		XUnmapWindow(fsflatDisplay, tmpGroupPtr->headerWindow);
	    }
	    tmpFilePtr = tmpGroupPtr->fileList;
	}

	for ( ; tmpFilePtr != NULL; tmpFilePtr = tmpFilePtr->nextPtr) {
	    tmpFilePtr->x = tmpFilePtr->y = -1;
	    tmpFilePtr->myColumn = -1;
	}
    }

    /* The number of the last element visible -- includes spaces and headers */
    if (aWindow->lastElement == 0) {
	aWindow->lastElement = aWindow->firstElement - 1 +
		((aWindow->usedCol + 1) * aWindow->numRows);
    } else {
	/* We're partway into a column, add on columns up to this one */
	aWindow->lastElement += aWindow->firstElement - 1 +
		(aWindow->usedCol * aWindow->numRows);
    }

    /* Get rid of extraneous redraw events from mapping(?) Sx entry windows. */
    for (i = 0; i < numMappedHeaders; i++) {
	XEvent	tossEvent;	/* dispose of this extra event */

	XCheckWindowEvent(fsflatDisplay, aWindow->displayWindow, ExposureMask,
		&tossEvent);
    }

    /* This causes FsflatRedraw() to be called, so the whole thing appears. */
    XMapWindow(fsflatDisplay, aWindow->displayWindow);

#ifdef SCROLL_DEBUG
    fprintf(stderr, "At end of FsflatSetPositions:\n");
    fprintf(stderr, "\tfirstElement is %d\n",
	    aWindow->firstElement);
    fprintf(stderr, "\tlastElement is %d\n",
	    aWindow->lastElement);
    fprintf(stderr, "\tnumElements is %d\n",
	    aWindow->numElements);
    fprintf(stderr, "\tnumGroups is %d\n", aWindow->numGroups);
    fprintf(stderr, "\tnumRows is %d\n", aWindow->numRows);
    fprintf(stderr, "\tcolumns used is %d\n",
	    aWindow->usedCol + 1);
#endif SCROLL_DEBUG

    return;
}


/*
 *----------------------------------------------------------------------
 *
 * FsflatRedraw --
 *
 *	Redraw display with positions already set.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The display is redrawn.
 *
 *----------------------------------------------------------------------
 */
void
FsflatRedraw(aWindow)
    FsflatWindow	*aWindow;
{
    int	i;
    FsflatGroup	*tmpGroupPtr = NULL;
    FsflatFile	*tmpPtr = NULL;
    float	top, bottom;
    char	fields[MAXNAMLEN + (3 * 26) + 11];	/* space allocation
							 * described below */
    char	sorting[50];
    Boolean	somethingDrawnP = FALSE;

    strcpy(fields, "Filename");
    for (i = 0; i < aWindow->maxNameLength - strlen("Filename"); i++) {
	/* Very wasteful... */
	strcat(fields, " ");
    }
    if (aWindow->displayInstructions & FSFLAT_ATIME_FIELD) {
	/* 24 chars needed for time, plus 2 spaces == 26 */
	strcat(fields, "  AccessTime              ");
    }
    if (aWindow->displayInstructions & FSFLAT_MTIME_FIELD) {
	/* 24 chars needed for time, plus 2 spaces == 26 */
	strcat(fields, "  DataModifyTime          ");
    }
    if (aWindow->displayInstructions & FSFLAT_DTIME_FIELD) {
	/* 24 chars needed for time, plus 2 spaces == 26 */
	/*
	 * This is one of the few places where Descriptor isn't spelled out -
	 * for space considerations.
	 */
	strcat(fields, "  DescModifyTime          ");
    }
    if (aWindow->displayInstructions & FSFLAT_SIZE_FIELD) {
	/* 9 chars needed for size, plus 2 spaces == 11 */
	strcat(fields, "      Bytes");
    }

    strcpy(sorting, "Sorted by:  ");
    if (aWindow->sortingInstructions & FSFLAT_ALPHA_SORT) {
	if (aWindow->sortingInstructions & FSFLAT_REVERSE_SORT) {
	    strcat(sorting, "Reverse Alphabet");
	} else {
	    strcat(sorting, "Alphabet"); }
    }
    if (aWindow->sortingInstructions & FSFLAT_ATIME_SORT) {
	if (aWindow->sortingInstructions & FSFLAT_REVERSE_SORT) {
	    strcat(sorting, "Reverse AccessTime");
	} else {
	    strcat(sorting, "AccessTime");
	}
    }
    if (aWindow->sortingInstructions & FSFLAT_MTIME_SORT) {
	if (aWindow->sortingInstructions & FSFLAT_REVERSE_SORT) {
	    strcat(sorting, "Reverse DataModifyTime");
	} else {
	    strcat(sorting, "DataModifyTime");
	}
    }
    if (aWindow->sortingInstructions & FSFLAT_DTIME_SORT) {
	if (aWindow->sortingInstructions & FSFLAT_REVERSE_SORT) {
	    strcat(sorting, "Reverse DescriptorModifyTime");
	} else {
	    strcat(sorting, "DescriptorModifyTime");
	}
    }
    if (aWindow->sortingInstructions & FSFLAT_SIZE_SORT) {
	if (aWindow->sortingInstructions & FSFLAT_REVERSE_SORT) {
	    strcat(sorting, "Reverse Size");
	} else {
	    strcat(sorting, "Size");
	}
    }

#ifdef SCROLL_DEBUG
    fprintf(stderr, "Entered FsflatRedraw:\n");
    fprintf(stderr, "\tfirstElement is %d\n",
	    aWindow->firstElement);
    fprintf(stderr, "\tlastElement is %d\n",
	    aWindow->lastElement);
    fprintf(stderr, "\tnumElements is %d\n",
	    aWindow->numElements);
    fprintf(stderr, "\tnumGroups is %d\n", aWindow->numGroups);
#endif SCROLL_DEBUG

    XClearArea(fsflatDisplay, aWindow->displayWindow, 0, 0, 0, 0, False);

    for (tmpGroupPtr = aWindow->groupList;
	    tmpGroupPtr != NULL; tmpGroupPtr = tmpGroupPtr->nextPtr) {
	for (tmpPtr = tmpGroupPtr->fileList;
		tmpPtr != NULL; tmpPtr = tmpPtr->nextPtr) {
	    FsflatRedrawFile(aWindow, tmpPtr);
	    somethingDrawnP = TRUE;
	}
    }
    /*
     * Now draw the column dividers.
     */
    if (somethingDrawnP) {
	for (i = 1; i <= aWindow->usedCol; i++) {
	    XDrawLine(fsflatDisplay, aWindow->displayWindow,
		    aWindow->textGc,
		    aWindow->columns[i].x, 0,
		    aWindow->columns[i].x, aWindow->windowHeight);
	}
    }
    /*
     * Now set the scroll bar to match what will be displayed.
     */
    if (aWindow->numRows == 0) {
	top = 0.0;
	bottom = 1.0;
    } else {
	/*
	 * First (last) entry divided by number of entries required.  (This
	 * includes any displayed spaces and groups.)
	 */
	top = ((float) aWindow->firstElement - 1) /
		((float) aWindow->totalDisplayEntries);
	bottom = ((float) aWindow->lastElement) /
		((float) aWindow->totalDisplayEntries);
    }
#ifdef SCROLL_DEBUG
    fprintf(stderr, "At end of FsflatRedraw:\n");
    fprintf(stderr, "\ttop is %f\n", top);
    fprintf(stderr, "\tbottom is %f\n", bottom);
    fprintf(stderr, "\tcalling SetRange.\n");
#endif SCROLL_DEBUG

    Sx_ScrollbarSetRange(fsflatDisplay, aWindow->scrollWindow, top, bottom);
    Sx_TitleMake(fsflatDisplay, aWindow->sortWindow, aWindow->fontPtr,
	    aWindow->sortForeground, aWindow->sortBackground,
	    aWindow->sortBackground, sorting, NULL, NULL);
    Sx_TitleMake(fsflatDisplay, aWindow->fieldsWindow, aWindow->fontPtr,
	    aWindow->fieldsForeground, aWindow->fieldsBackground,
	    aWindow->fieldsBackground, fields, NULL, NULL);

    return;
}



/*
 *----------------------------------------------------------------------
 *
 * FsflatRedrawFile --
 *
 *	Redraw a file entry.  Highlight with a box, or indicate that it is
 *	selected, if required.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The file entry is redisplayed.
 *
 *----------------------------------------------------------------------
 */
void
FsflatRedrawFile(aWindow, filePtr)
    FsflatWindow	*aWindow;
    FsflatFile		*filePtr;
{
    char	*space = NULL;
    int		secondFieldStart;
    int		ascent;
    int		lbearing;	/* to indent string enough that we don't
				 * draw a box over its first char */

    if (filePtr->x == -1) {		/* is it visible? */
	return;
    }
    ascent = aWindow->fontPtr->ascent;
    lbearing =  -4;
    if ((aWindow->displayInstructions & ~FSFLAT_NAME_FIELD) != 0) {
	space = (char *) malloc(aWindow->maxEntryWidth + 1 -
		aWindow->maxNameLength);
	space[0] = '\0';
	FsflatGetFileFields(aWindow, filePtr, &space);
    }

    secondFieldStart =
	    FSFLAT_CHAR_TO_WIDTH(aWindow->maxNameLength + 2, aWindow->fontPtr);
    if (filePtr->selectedP || filePtr->myGroupPtr->selectedP) {
	/* should highlight */
	XDrawImageString(fsflatDisplay, aWindow->displayWindow,
		aWindow->reverseGc, filePtr->x + -lbearing + 1,
		filePtr->y + ascent, filePtr->name, strlen(filePtr->name));
	if (space != NULL) {
	    if (filePtr->lineP) {
		XDrawImageString(fsflatDisplay, aWindow->displayWindow,
			aWindow->reverseGc,
			filePtr->x + secondFieldStart,
			filePtr->y + ascent, space, strlen(space));
	    } else {
		XDrawImageString(fsflatDisplay, aWindow->displayWindow,
			aWindow->reverseGc,
			filePtr->x + secondFieldStart,
			filePtr->y + ascent, space, strlen(space));
	    }
	}
    } else {
	XDrawImageString(fsflatDisplay, aWindow->displayWindow, aWindow->textGc,
		filePtr->x + -lbearing + 1, filePtr->y + ascent, filePtr->name,
		strlen(filePtr->name)); 
	if (space != NULL) {
	    XDrawImageString(fsflatDisplay, aWindow->displayWindow,
		    aWindow->textGc, filePtr->x + secondFieldStart,
		    filePtr->y + ascent, space, strlen(space));
	}
    }

    /* draw or undraw a box around it */
    {
	XPoint	vlist[5];

	vlist[0].x = filePtr->x + 1;
	vlist[0].y = filePtr->y + aWindow->rowHeight;

	/*
	 * To make up for indentation of string by -lbearing amount.
	 */
	vlist[1].x = -lbearing +
		FSFLAT_CHAR_TO_WIDTH(aWindow->maxNameLength, aWindow->fontPtr);
	vlist[1].y = 0;

	vlist[2].x = 0;
	vlist[2].y = -(aWindow->rowHeight);

	vlist[3].x = -(vlist[1].x);
	vlist[3].y = 0;

	/* should be equal to first point - if not, make all rel. to origin. */
	vlist[4].x = 0;
	vlist[4].y = -(vlist[2].y);

	if (filePtr->highlightP) {
	    /* should be selectionGc */
	    XDrawLines(fsflatDisplay, aWindow->displayWindow,
		    aWindow->textGc, vlist, 5, CoordModePrevious);
	} else {
	    XDrawLines(fsflatDisplay, aWindow->displayWindow,
		    aWindow->reverseGc, vlist, 5, CoordModePrevious);
	}
    }
    if (space != NULL) {
	free(space);
    }

    return;
}


/*
 *----------------------------------------------------------------------
 *
 * FsflatGetFileFields --
 *
 *	Get the additional fields to display for a file entry beyond the
 *	file name.
 *	If *buf is null, then allocate space for the string to return.
 *	Otherwise copy the return value into *buf.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The file entry is redisplayed.
 *
 *----------------------------------------------------------------------
 */
void
FsflatGetFileFields(aWindow, filePtr, buf)
    FsflatWindow	*aWindow;
    FsflatFile		*filePtr;
    char		**buf;
{
    char	timeBuf[26];	/* 26 chars according to ctime man page */
    char	intBuf[CVT_INT_BUF_SIZE + 1]; /* const defined in my .h file */
    int		i;
    char	*space;

    /* What to do here? */
    if (*buf == NULL) {
	*buf = (char *) malloc(aWindow->maxEntryWidth + 1 -
		aWindow->maxNameLength);
	*buf[0] = '\0';
    }
    space = *buf;
    /* Any fields except for name? */
    if (!(aWindow->displayInstructions & (~FSFLAT_NAME_FIELD))) {
	/* Could free up space from attrPtr field here, if I want. */
	return;
    }
    if (filePtr->attrPtr == NULL) {
	filePtr->attrPtr = (struct stat *) malloc(sizeof (struct stat));
	if (lstat(filePtr->name, filePtr->attrPtr)
		!= 0) {
	    sprintf(fsflatErrorMsg, "%s %s.  %s.",
		    "Couldn't get attributes for file",
		    filePtr->name,  "The file may no longer exist");
	    Sx_Notify(fsflatDisplay, aWindow->surroundingWindow, -1, -1, 0,
		    fsflatErrorMsg, NULL, TRUE, "Continue",
		    (char *) NULL);
	    bzero(filePtr->attrPtr, sizeof (struct	stat)); 
	    strcat(space, "  X  ");
	    return;
	}
    }
    if (aWindow->displayInstructions & FSFLAT_ATIME_FIELD) {
	/* 24 chars needed for time, plus 2 spaces == 26 */
	strcpy(timeBuf, ctime(&(filePtr->attrPtr->st_atime)));
	/* wasteful to do twice, I suppose */
	if (index(timeBuf, '\n') != NULL) {
	    *(index(timeBuf, '\n')) = '\0';
	}
	strcat(space, timeBuf);
	strcat(space, "  ");
    }
    if (aWindow->displayInstructions & FSFLAT_MTIME_FIELD) {
	/* 24 chars needed for time, plus 2 spaces */
	strcpy(timeBuf, ctime(&(filePtr->attrPtr->st_mtime)));
	/* wasteful to do twice, I suppose */
	if (index(timeBuf, '\n') != NULL) {
	    *(index(timeBuf, '\n')) = '\0';
	}
	strcat(space, timeBuf);
	strcat(space, "  ");
    }
    if (aWindow->displayInstructions & FSFLAT_DTIME_FIELD) {
	/* 24 chars needed for time, plus 2 spaces */
	strcpy(timeBuf, ctime(&(filePtr->attrPtr->st_ctime)));
	/* wasteful to do twice, I suppose */
	if (index(timeBuf, '\n') != NULL) {
	    *(index(timeBuf, '\n')) = '\0';
	}
	strcat(space, timeBuf);
	strcat(space, "  ");
    }
    if (aWindow->displayInstructions & FSFLAT_SIZE_FIELD) {
	/* 9 chars needed for size, plus 2 spaces */
	sprintf(intBuf, "%d", filePtr->attrPtr->st_size);
	for (i = 0; i < 9 - strlen(intBuf); i++) {
	    strcat(space, " ");
	}
	strcat(space, intBuf);
	strcat(space, "  ");
    }
#ifdef NOTDEF
    if (aWindow->displayInstructions & FSFLAT_OTHERSTUFF_FIELD) {
	/* 9 chars needed for size, plus 2 spaces */
    }
#endif NOTDEF

    return;
}


/*
 *----------------------------------------------------------------------
 *
 * FsflatSetWindowAndRowInfo --
 *
 *	Given the dimensions of the window, calculate
 *	the height of rows and the number of usable rows in the window,
 *	and set these fields (and the window dimension fields) in the
 *	given FsflatWindow structure.  The row height calculation is
 *	made relative to the current font height specified in the fontPtr
 *	field of the FsflatWindow structure.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The fields in the FsflatWindow structure that are set are
 *	the rowHeight, numRows, windowHeight and windowWidth fields.
 *
 *----------------------------------------------------------------------
 */
void
FsflatSetWindowAndRowInfo(aWindow, height, width)
    FsflatWindow	*aWindow;
    int			height, width;
{
    aWindow->windowHeight = height;
    aWindow->windowWidth = width;
    aWindow->rowHeight = aWindow->fontPtr->max_bounds.ascent +
	    aWindow->fontPtr->max_bounds.descent + FSFLAT_ROW_SPACING;
    aWindow->numRows = aWindow->windowHeight / aWindow->rowHeight;

    return;
}



/*
 *----------------------------------------------------------------------
 *
 * FsflatRedrawGroup --
 *
 *	Not yet implemented.
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
FsflatRedrawGroup(aWindow, groupPtr)
    FsflatWindow	*aWindow;
    FsflatGroup		*groupPtr;
{
    return;
}


/*
 *----------------------------------------------------------------------
 *
 * FsflatMapCoordsToFile --
 *
 *	Map the given coordinates to a file entry.  The mapping will work
 *	if the cursor is on the same line as a file entry.  It does not
 *	have to be directory over the file entry.
 *
 * Results:
 *	A pointer to the appropriate file entry, or NULL if there isn't
 *	one under the given coordinates.	
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
FsflatFile *
FsflatMapCoordsToFile(aWindow, x, y)
    FsflatWindow	*aWindow;
    int			x, y;
{
    FsflatGroup	*groupPtr;
    FsflatFile	*filePtr;
    int		ascent;

    ascent = aWindow->fontPtr->ascent;
    for (groupPtr = aWindow->groupList; groupPtr != NULL;
	    groupPtr = groupPtr->nextPtr) {
	for (filePtr = groupPtr->fileList; filePtr != NULL;
		filePtr = filePtr->nextPtr) {
	    if (filePtr->x == -1) {	/* off the display */
		continue;
	    }
	    if ((filePtr->x <= x && x <= filePtr->x + aWindow->columnWidth) &&
		    (filePtr->y + ascent <= y && y <= filePtr->y + ascent +
		    aWindow->rowHeight)) {
		return filePtr;
	    }
	}
    }

    return NULL;
}


/*
 *----------------------------------------------------------------------
 *
 * FsflatMapCoordsToGroup --
 *
 *	Map the given coordinates to a group header.
 *
 * Results:
 *	A pointer to the appropriate group header, or NULL if there isn't
 *	one under the given coordinates.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
FsflatGroup *
FsflatMapCoordsToGroup(aWindow, x, y)
    FsflatWindow	*aWindow;
    int			x, y;
{
    FsflatGroup	*groupPtr;

    for (groupPtr = aWindow->groupList; groupPtr != NULL;
	    groupPtr = groupPtr->nextPtr) {
	if ((groupPtr->x <= x && x <= groupPtr->x + groupPtr->length) &&
		(groupPtr->y <= y && y <= groupPtr->y + aWindow->rowHeight)) {
	    return groupPtr;
	}
    }

    return NULL;
}



/*
 *----------------------------------------------------------------------
 *
 * GetMaxEntryWidth --
 *
 *	Find the length of the longest column entry.
 *
 * Results:
 *	The length.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static int
GetMaxEntryWidth(aWindow)
    FsflatWindow	*aWindow;
{
    FsflatGroup	*tmpGroupPtr;
    FsflatFile	*tmpFilePtr;
    Boolean	getAttrsP = FALSE;
    int		charLength = 0;
    int		lbearing;

    if ((aWindow->displayInstructions & ~FSFLAT_NAME_FIELD) != 0) {
	getAttrsP = TRUE;
    }
    lbearing = -4;
    aWindow->maxNameLength = 0;
    /*
     * Warning: this allocates space width for the rules, even if they
     * are hidden groups.
     */
    for (tmpGroupPtr = aWindow->groupList; tmpGroupPtr != NULL;
	    tmpGroupPtr = tmpGroupPtr->nextPtr) {
	charLength = strlen(tmpGroupPtr->rule);
	tmpGroupPtr->length =
		XTextWidth(aWindow->fontPtr, tmpGroupPtr->rule, charLength);
	if (charLength > aWindow->maxNameLength) {
	    aWindow->maxNameLength = charLength;
	}
	for (tmpFilePtr = tmpGroupPtr->fileList; tmpFilePtr != NULL;
		tmpFilePtr = tmpFilePtr->nextPtr) {
	    charLength = strlen(tmpFilePtr->name);
	    tmpFilePtr->length =
		    XTextWidth(aWindow->fontPtr, tmpFilePtr->name, charLength);
	    if (charLength > aWindow->maxNameLength) {
		aWindow->maxNameLength = charLength;
	    }
	}
    }
    aWindow->maxEntryWidth = aWindow->maxNameLength;
    /* 2 chars for spaces */
    aWindow->maxEntryWidth += 2;
    if (getAttrsP) {
	if (aWindow->displayInstructions & FSFLAT_ATIME_FIELD) {
	    /* 24 chars needed for time, plus 2 spaces */
	    aWindow->maxEntryWidth += 26;
	}
	if (aWindow->displayInstructions & FSFLAT_MTIME_FIELD) {
	    /* 24 chars needed for time, plus two spaces */
	    aWindow->maxEntryWidth += 26;
	}
	if (aWindow->displayInstructions & FSFLAT_DTIME_FIELD) {
	    /* 24 chars needed for time, plus two spaces */
	    aWindow->maxEntryWidth += 26;
	}
	if (aWindow->displayInstructions & FSFLAT_SIZE_FIELD) {
	    /* 9 chars needed for time, plus two spaces */
	    aWindow->maxEntryWidth += 11;
	}
#ifdef NOTDEF
	/* what to do about owner sizes, groups, plus perm. display? */
	if (aWindow->displayInstructions & FSFLAT_PERM_FIELD) {
	    /* 1 chars needed for time, plus two spaces */
	    aWindow->maxEntryWidth += 12;
	}
	if (aWindow->displayInstructions & FSFLAT_TYPE_FIELD) {
	    /* 1 chars needed for time, plus two spaces */
	    aWindow->maxEntryWidth += 12;
	}
#endif NOTDEF
    }

    /*
     * Plus lbearing cause we want to indent string enough that we don't draw
     * a box around it.
     */
    return (-lbearing +
	    FSFLAT_CHAR_TO_WIDTH(aWindow->maxEntryWidth, aWindow->fontPtr));
}


/*
 *----------------------------------------------------------------------
 *
 * GetColumnWidthInfo --
 *
 *	Figure out the best fixed column width.  This includes the
 *	extra FSFLAT_COLUMN_SPACING added.  Also figure out the number
 *	of columns.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The columnWidth and numCols parameters are filled in with their values.
 *
 *----------------------------------------------------------------------
 */
static void
GetColumnWidthInfo(aWindow, maxEntryWidth, columnWidth, numCols)
    FsflatWindow	*aWindow;
    int			maxEntryWidth;
    int			*columnWidth;
    int			*numCols;
{
    int		numToDisplay;
    int		numColsRequired;
    int		widthLeft;

    if (maxEntryWidth < 0) {
	*columnWidth = GetMaxEntryWidth(aWindow) + FSFLAT_COLUMN_SPACING;
    } else {
	*columnWidth = maxEntryWidth + FSFLAT_COLUMN_SPACING;
    }
    *numCols = aWindow->windowWidth / *columnWidth;
    if (*numCols == 0) {
	return ;
    }
    numToDisplay = aWindow->totalDisplayEntries - aWindow->firstElement + 1;
    if (aWindow->numRows == 0) {
	return ;
    }
    numColsRequired = numToDisplay / aWindow->numRows;
    if (numToDisplay % aWindow->numRows != 0) {
	numColsRequired++;
    }
    if (numColsRequired < *numCols) {
	*numCols = numColsRequired;
    }
    if (*numCols == 0) {
	return ;
    }
    widthLeft = aWindow->windowWidth - (*numCols * *columnWidth);
    /*
     * This could allow the last column to fail to meet the scroll bar by
     * a number of pixels one less than the number of columns.
     * We also add 1 pixel here so that entry window borders will overlap
     * with the window borders if we start them at 1 pixel before the
     * actual text area.
     */
    *columnWidth += (widthLeft / *numCols) + 1;

    return ;
}


/*
 *----------------------------------------------------------------------
 *
 * FigureWindowSize --
 *
 *	Figure out a good window size.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The window size changes and stuff is laid out again to match.
 *
 *----------------------------------------------------------------------
 */
static void
FigureWindowSize(aWindow, maxEntryWidth)
    FsflatWindow	*aWindow;
    int			maxEntryWidth;
{
    int		maxWidth;
    int		width, height;
    int		columnWidth, numCols, numRows, numColsRequired;

    columnWidth = maxEntryWidth;

    maxWidth = fsflatRootWidth / 8 * 7;
    width = maxWidth;

    numCols = width / (columnWidth + FSFLAT_COLUMN_SPACING);
    if (numCols == 0) {
	/* just set to max size */
    }

    for ( ; ; ){
	/* Make it line up on column boundary -- no wasted space */
	width = numCols  * (columnWidth + FSFLAT_COLUMN_SPACING); 
	height = width / 3 * 2;		/* 2/3's of width */
	numRows = height / aWindow->rowHeight;
	if (aWindow->numRows == 0) {
	    /* just set to max size */
	}
	numColsRequired = aWindow->totalDisplayEntries / numRows;

	if (aWindow->totalDisplayEntries % numRows != 0) {
	    numColsRequired++;
	}
	if (numColsRequired < numCols) {
	    numCols--;
	} else {
	    break;
	}
    }
    /* last shrinking may have gone too far since we shrank rows as well */
    if (numColsRequired > numCols && (width + columnWidth +
	    FSFLAT_COLUMN_SPACING <= maxWidth)) {
	numCols++;
	width += columnWidth + FSFLAT_COLUMN_SPACING;
	/* leave height and numRows alone */
    }
    /*
     * This next will generate an ExposureMask event, but I'm not sure
     * yet just where it fit in, so I may change some things.
     * It is an incredible pain that I can't change the displayWindow
     * to the correct dimensions and have the packer know to resize the
     * parent window.  Right now I have to resize to parent (surrounding)
     * window to what I guess should be its size.  I can't
     * XGetGeometry() them here since they all seem to have a size of 1
     * in Sx until some event occurs...  IS THIS STILL TRUE IN X11???
     */
    if (aWindow->windowWidth != width || aWindow->windowHeight != height) {
	XWindowChanges	changes;

	changes.width = width + 1 + Sx_ScrollbarWidth();
	changes.height = height + 
		(2 * Sx_DefaultHeight(fsflatDisplay, aWindow->titleFontPtr))
		+ 3 + (4 * Sx_DefaultHeight(fsflatDisplay, aWindow->fontPtr));
	XConfigureWindow(fsflatDisplay, aWindow->surroundingWindow,
		CWWidth | CWHeight, &changes);
    }

    return;
}
