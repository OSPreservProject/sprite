/* 
 * fsflatMain.c --
 *
 *	Main routine for fsflat stuff.  Right now I'm just testing the
 *	new scrollbar stuff.
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
static char rcsid[] = "$Header: fsflatMain.c,v 1.1 88/10/03 12:48:21 mlgray Exp $ SPRITE (Berkeley)";
#endif not lint

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#define	Time	SpriteTime
#include <sys/time.h>
#include "string.h"
#include <fs.h>
#include "sx.h"
#include "util.h"
#include "monitorClient.h"
#include "fsflatInt.h"
#include <signal.h>

int	size;

/*
 * The global table in which pointers to window information for each
 * fsflat window is kept.
 */
XContext	fsflatWindowContext;
XContext	fsflatGroupWindowContext;

/* Whether debugging is turned on or not */
Boolean		fsflatDebugP = FALSE;

/* Whether the application should pick the initial window size */
Boolean		fsflatPickSizeP = TRUE;
/* whether the application can resize the window */
Boolean		fsflatResizeP = FALSE;
/* whether to display headers for groups with no files. */
Boolean		fsflatShowEmptyGroupsP = FALSE;
/* The display */
Display		*fsflatDisplay;

/*
 * The application name.
 */
char	*fsflatApplication = "fsflat";

/*
 * For formulating error strings.  How big is big enough?
 */
char	fsflatErrorMsg[2 * MAXPATHLEN + 50];

/* Display info for calculating max size of windows */
int		fsflatRootHeight;
int		fsflatRootWidth;

/* Keep track of current directory. */
char	fsflatCurrentDirectory[MAXPATHLEN + 1];

#ifdef ICON
/*
 * Icon window information.
 */

#define ICONDIR	"/usr2/icon/X"

typedef struct {
    int x;			/* Initial X, Y positions. */
    int y;			
    int width;
    int height;
    int foreground;		/* Foreground, background colors 
				 * are same as the main window. */
    int background;
    char *name;			/* Filename of the icon bitmap file. */
    Pixmap bitmap;		/* Bitmap data. */
    Window window;		/* ID of the icon window. */
} TxIconInfo;
static TxIconInfo iconInfo = { 0, 0, 0, 0, 0, 0, NULL, NULL,};
static void CreateIcon();
#endif /* ICON */


/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	Main routine for fsflat.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Many.
 *
 *----------------------------------------------------------------------
 */
main(argc, argv)
    int	argc;
    char	*argv[];
{
    char	*string;
    char	*geometry = NULL;	/* for window set-up */
    char	*font;
    FsflatWindow	*aWindow = NULL;
    char	*startDir = NULL;
    int		displayMask = 0;
    int		numFDs;
#ifdef NOTDEF
    struct	timeval	alarm;
#else /* NOTDEF */
    SpriteTime	alarm;
#endif /* NOTDEF */
    int		monitorMask;
    char	*cPtr;
    extern	void	HandleMonitorEvent();
    extern	void	FsflatDisplayEventProc();
    extern	void	HandleMonitorStats();

    /*
     * Process command line arguments.
     */
    while (argc > 1 ) { 
	argc--;
	argv++;
	if (strcmp(argv[0], "-debug") == 0) {
	    fsflatDebugP = TRUE;
	    continue;
	}
	if (strcmp(argv[0], "-nopick") == 0) {
	    fsflatPickSizeP = FALSE;
	    continue;
	}
	if (strcmp(argv[0], "-resize") == 0) {
	    fsflatResizeP = TRUE;
	    continue;
	}
	if (strcmp(argv[0], "-empty") == 0) {
	    fsflatShowEmptyGroupsP = TRUE;
	    continue;
	}
	if (strcmp(argv[0], "-usage") == 0 ||
		strcmp(argv[0], "-u") == 0) {
	    fprintf(stderr, "%s\n\n%s\n%s\n%s\n%s\n",
		    "fsflat -debug -nopick -resize -empty -usage dirname",
		    "-debug	Print debug messages and don't fork.",
		    "-nopick	Don't automatically pick initial window size.",
		    "-resize	Allow automatic resizing of window.",
		    "-empty	Display headers for groups without entries.");
	    exit(1);
	}
	startDir = argv[0];
    }
    /* Don't fork into background if debugging is on - so dbx works, etc. */
    if (!fsflatDebugP) {
	Proc_Detach(0);
    }
    /*
     * Initialize the display and fsflatWindowContext and
     * fsflatGroupWindowContext.
     */
    if ((fsflatDisplay = XOpenDisplay(NULL)) == NULL) {
	fprintf(stderr, "Could not open display.\n");
	exit(1);
    }
    if (fsflatDebugP) {
	XSynchronize(fsflatDisplay, True);
    }
    Sx_SetErrorHandler();

    fsflatRootHeight = DisplayHeight(fsflatDisplay,
	    DefaultScreen(fsflatDisplay));
    fsflatRootWidth = DisplayWidth(fsflatDisplay,
	    DefaultScreen(fsflatDisplay));

    /*
     * 8 should be more than enough considering there's only 1 window
     * right now!
     */
    fsflatWindowContext = XUniqueContext();
    fsflatGroupWindowContext = XUniqueContext();

    /*
     * Create a dummy structure for passing to FsflatCreate to create
     * the first window and then process XDefault arguments.  
     */
    aWindow = (FsflatWindow *) malloc(sizeof (FsflatWindow));
	
    if (startDir != NULL) {
	if (Util_CanonicalDir(startDir, NULL, aWindow->dir) == NULL) {
	    /* error message returned in aWindow->dir */
	    Sx_Panic(fsflatDisplay, aWindow->dir);
	}
    } else if (getwd(aWindow->dir) == NULL) {
	sprintf(fsflatErrorMsg,
		"%s: Couldn't get current working directory.\n",
		fsflatApplication);
	Sx_Panic(fsflatDisplay, fsflatErrorMsg);
    }

    if ((string = XGetDefault(fsflatDisplay, fsflatApplication,
	    "Background")) != NULL) {
	if ((aWindow->background =
		Util_StringToColor(fsflatDisplay, string)) == -1) {
	    aWindow->background = BlackPixel(fsflatDisplay,
		    DefaultScreen(fsflatDisplay));
	}
    } else {
	aWindow->background = BlackPixel(fsflatDisplay,
		DefaultScreen(fsflatDisplay));
    }

    if ((string = XGetDefault(fsflatDisplay, fsflatApplication,
	    "Foreground")) != NULL) {
	if ((aWindow->foreground =
		Util_StringToColor(fsflatDisplay, string)) == -1) {
	    /* Try to make it the opposite of the background */
	    if (aWindow->background == WhitePixel(fsflatDisplay,
		    DefaultScreen(fsflatDisplay))) {
		aWindow->foreground = BlackPixel(fsflatDisplay,
			DefaultScreen(fsflatDisplay));
	    } else {
		aWindow->foreground = WhitePixel(fsflatDisplay,
			DefaultScreen(fsflatDisplay));
	    }
	}	
    /* At least try to make it the opposite of the background */
    } else if (aWindow->background == WhitePixel(fsflatDisplay,
	    DefaultScreen(fsflatDisplay))) {
	aWindow->foreground = BlackPixel(fsflatDisplay,
		DefaultScreen(fsflatDisplay));
    } else {
	aWindow->foreground = WhitePixel(fsflatDisplay,
	    DefaultScreen(fsflatDisplay));
    }

    if ((string = XGetDefault(fsflatDisplay, fsflatApplication, "Border"))
	    != NULL) {
	if ((aWindow->border =
		Util_StringToColor(fsflatDisplay, string)) == -1) {
	    /* Try to make it the opposite of the background */
	    if (aWindow->background == WhitePixel(fsflatDisplay,
		    DefaultScreen(fsflatDisplay))) {
		aWindow->border = BlackPixel(fsflatDisplay,
			DefaultScreen(fsflatDisplay));
	    } else {
		aWindow->border = WhitePixel(fsflatDisplay,
			DefaultScreen(fsflatDisplay));
	    }
	}
    /* At least try to make it the opposite of the background */
    } else if (aWindow->background == WhitePixel(fsflatDisplay,
	    DefaultScreen(fsflatDisplay))) {
	aWindow->border = BlackPixel(fsflatDisplay,
		DefaultScreen(fsflatDisplay));
    } else {
	aWindow->border = WhitePixel(fsflatDisplay,
		DefaultScreen(fsflatDisplay));
    }

    if ((string = XGetDefault(fsflatDisplay, fsflatApplication, "Selection"))
	    != NULL) {
	if ((aWindow->selection =
		Util_StringToColor(fsflatDisplay, string)) == -1) {
	    /* Try to make it the opposite of the background */
	    if (aWindow->background == WhitePixel(fsflatDisplay,
		    DefaultScreen(fsflatDisplay))) {
		aWindow->selection = BlackPixel(fsflatDisplay,
			DefaultScreen(fsflatDisplay));
	    } else {
		aWindow->selection = WhitePixel(fsflatDisplay,
			DefaultScreen(fsflatDisplay));
	    }
	}	
    /* At least try to make it the opposite of the background */
    } else if (aWindow->background == WhitePixel(fsflatDisplay,
	    DefaultScreen(fsflatDisplay))) {
	aWindow->selection = BlackPixel(fsflatDisplay,
		DefaultScreen(fsflatDisplay));
    } else {
	aWindow->selection = WhitePixel(fsflatDisplay,
		DefaultScreen(fsflatDisplay));
    }
 
    if ((string = XGetDefault(fsflatDisplay, fsflatApplication,
	    "TitleBackground")) != NULL) {
	if ((aWindow->titleBackground =
		Util_StringToColor(fsflatDisplay, string)) == -1) {
	    aWindow->titleBackground = aWindow->background;
	}
    } else {
	aWindow->titleBackground = aWindow->background;
    }

    if ((string = XGetDefault(fsflatDisplay, fsflatApplication,
	    "TxBackground")) != NULL) {
	if ((aWindow->txBackground =
		Util_StringToColor(fsflatDisplay, string)) == -1) {
	    aWindow->txBackground = aWindow->background;
	}
    } else {
	aWindow->txBackground = aWindow->background;
    }
    if ((string = XGetDefault(fsflatDisplay, fsflatApplication,
	    "MenuBackground")) != NULL) {
	if ((aWindow->menuBackground =
		Util_StringToColor(fsflatDisplay, string)) == -1) {
	    aWindow->menuBackground = aWindow->background;
	}
    } else {
	aWindow->menuBackground = aWindow->background;
    }
    if ((string = XGetDefault(fsflatDisplay, fsflatApplication,
	    "SortBackground")) != NULL) {
	if ((aWindow->sortBackground =
		Util_StringToColor(fsflatDisplay, string)) == -1) {
	    aWindow->sortBackground = aWindow->background;
	}
    } else {
	aWindow->sortBackground = aWindow->background;
    }
    if ((string = XGetDefault(fsflatDisplay, fsflatApplication,
	    "FieldsBackground")) != NULL) {
	if ((aWindow->fieldsBackground =
		Util_StringToColor(fsflatDisplay, string)) == -1) {
	    aWindow->fieldsBackground = aWindow->background;
	}
    } else {
	aWindow->fieldsBackground = aWindow->background;
    }

    if ((string = XGetDefault(fsflatDisplay, fsflatApplication,
	    "EntryBackground")) != NULL) {
	if ((aWindow->entryBackground =
		Util_StringToColor(fsflatDisplay, string)) == -1) {
	    aWindow->entryBackground = aWindow->background;
	}
    } else {
	aWindow->entryBackground = aWindow->background;
    }

    if ((string = XGetDefault(fsflatDisplay, fsflatApplication,
	    "ScrollBackground")) != NULL) {
	if ((aWindow->scrollBackground =
		Util_StringToColor(fsflatDisplay, string)) == -1) {
	    aWindow->scrollBackground = aWindow->background;
	}
    } else {
	aWindow->scrollBackground = aWindow->background;
    }

    if ((string = XGetDefault(fsflatDisplay, fsflatApplication,
	    "TitleForeground")) != NULL) {
	if ((aWindow->titleForeground =
		Util_StringToColor(fsflatDisplay, string)) == -1) {
	    aWindow->titleForeground = aWindow->foreground;
	}	
    } else {
	aWindow->titleForeground = aWindow->foreground;
    }

    if ((string = XGetDefault(fsflatDisplay, fsflatApplication,
	    "TxForeground")) != NULL) {
	if ((aWindow->txForeground =
		Util_StringToColor(fsflatDisplay, string)) == -1) {
	    aWindow->txForeground = aWindow->foreground;
	}	
    } else {
	aWindow->txForeground = aWindow->foreground;
    }

    if ((string = XGetDefault(fsflatDisplay, fsflatApplication,
	    "MenuForeground")) != NULL) {
	if ((aWindow->menuForeground =
		Util_StringToColor(fsflatDisplay, string)) == -1) {
	    aWindow->menuForeground = aWindow->foreground;
	}
    } else {
	aWindow->menuForeground = aWindow->foreground;
    }

    if ((string = XGetDefault(fsflatDisplay, fsflatApplication,
	    "SortForeground")) != NULL) {
	if ((aWindow->sortForeground =
		Util_StringToColor(fsflatDisplay, string)) == -1) {
	    aWindow->sortForeground = aWindow->foreground;
	}	
    } else {
	aWindow->sortForeground = aWindow->foreground;
    }

    if ((string = XGetDefault(fsflatDisplay, fsflatApplication,
	    "FieldsForeground")) != NULL) {
	if ((aWindow->fieldsForeground =
		Util_StringToColor(fsflatDisplay, string)) == -1) {
	    aWindow->fieldsForeground = aWindow->foreground;
	}	
    } else {
	aWindow->fieldsForeground = aWindow->foreground;
    }

    if ((string = XGetDefault(fsflatDisplay, fsflatApplication,
	    "EntryForeground")) != NULL) {
	if ((aWindow->entryForeground =
		Util_StringToColor(fsflatDisplay, string)) == -1) {
	    aWindow->entryForeground = aWindow->foreground;
	}	
    } else {
	aWindow->entryForeground = aWindow->foreground;
    }

    if ((string = XGetDefault(fsflatDisplay, fsflatApplication,
	    "ScrollForeground")) != NULL) {
	if ((aWindow->scrollForeground =
		Util_StringToColor(fsflatDisplay, string)) == -1) {
	    aWindow->scrollForeground = aWindow->foreground;
	}	
    } else {
	aWindow->scrollForeground = aWindow->foreground;
    }

    if ((string = XGetDefault(fsflatDisplay, fsflatApplication,
	    "BorderWidth")) != NULL) {
	if ((aWindow->borderWidth = strtol(string, &cPtr, 10)) == 0 &&
		cPtr == string) {
	    /* AtoI failed */
	    aWindow->borderWidth = 2;
	}
    } else {
	aWindow->borderWidth = 2;
    }

    if ((string = XGetDefault(fsflatDisplay, fsflatApplication,
	    "TitleBorder")) != NULL) {
	if ((aWindow->titleBorder =
		Util_StringToColor(fsflatDisplay, string)) == -1) {
	    aWindow->titleBorder = aWindow->border;
	}
    } else {
	aWindow->titleBorder = aWindow->border;
    }

    if ((string = XGetDefault(fsflatDisplay, fsflatApplication, "TxBorder"))
	    != NULL) {
	if ((aWindow->txBorder =
		Util_StringToColor(fsflatDisplay, string)) == -1) {
	    aWindow->txBorder = aWindow->border;
	}
    } else {
	aWindow->txBorder = aWindow->border;
    }

    if ((string = XGetDefault(fsflatDisplay, fsflatApplication,
	    "ScrollElevator")) != NULL) {
	if ((aWindow->scrollElevator =
		Util_StringToColor(fsflatDisplay, string)) == -1) {
	    aWindow->scrollElevator = aWindow->scrollForeground;
	}	
    } else {
	aWindow->scrollElevator = aWindow->scrollForeground;
    }

    if ((geometry = XGetDefault(fsflatDisplay, fsflatApplication, "Geometry"))
	    == NULL) {
	geometry = "=550x350+160+160";
    }
    aWindow->geometry = geometry;

    if ((font = XGetDefault(fsflatDisplay, fsflatApplication, "Font"))
	    != NULL) {
	aWindow->fontPtr = XLoadQueryFont(fsflatDisplay, font);
	if (aWindow->fontPtr != NULL) {
	    Sx_SetDefaultFont(aWindow->fontPtr);
	} else {
	    aWindow->fontPtr = Sx_GetDefaultFont(fsflatDisplay);
	}
    } else {
	aWindow->fontPtr = Sx_GetDefaultFont(fsflatDisplay);
    }

    if ((font = XGetDefault(fsflatDisplay, fsflatApplication, "TitleFont"))
	    != NULL) {
	aWindow->titleFontPtr = XLoadQueryFont(fsflatDisplay, font);
	if (aWindow->titleFontPtr != NULL) {
	    Sx_SetDefaultFont(aWindow->titleFontPtr);
	}
    }
    if (aWindow->titleFontPtr == NULL) {
	aWindow->titleFontPtr = Sx_GetDefaultFont(fsflatDisplay);
    }

    aWindow->interp = NULL;

    if (!MonClient_Register()) {
	Sx_Panic(fsflatDisplay,
		"Initialization of file system monitor failed.");
    }

    /* create the first window */
    if (startDir != NULL) {
	if (FsflatCreate(aWindow, startDir) == NULL) {
	    Sx_Panic(fsflatDisplay, "Couldn't create flat display window.");
	}
    } else {
	if (FsflatCreate(aWindow, NULL) == NULL) {
	    Sx_Panic(fsflatDisplay, "Couldn't create flat display window.");
	}
    }
    free(aWindow);		/* free dummy structure */

#ifdef ICON
    /*
     * Create the icon for the window. This must done after the
     * main window is initialized.
     */

    CreateIcon(display, window, info.foreground, info.background);
#endif /* ICON */

    alarm.seconds = 5;
    alarm.microseconds = 0;
    Fs_EventHandlerCreate(monClient_ReadPort, FS_READABLE,
	    HandleMonitorEvent, NULL);
    Fs_EventHandlerCreate(ConnectionNumber(fsflatDisplay), FS_READABLE,
	    FsflatDisplayEventProc, NULL);
    Fs_TimeoutHandlerCreate(alarm, TRUE, HandleMonitorStats,
	    (ClientData) NULL);
    while (fsflatWindowCount > 0) {
	while ((size = QLength(fsflatDisplay)) > 0) {
	    XEvent	event;

	    XNextEvent(fsflatDisplay, &event);
	    Sx_HandleEvent(&event);
	}
	Tx_FlushStreams();
	Tx_Update();
	Mx_Update();
	if ((size = QLength(fsflatDisplay)) > 0) {
	    continue;
	}
	XFlush(fsflatDisplay);
	Fs_Dispatch();
    }

# ifdef NOTDEF
/*
 * Can't do this yet.  I don't know what I'll do about the utmp
 * stuff for now.
 */
    if (txRegisterPty) {
	char	*ptyName;
   
	if ((ptyName = TxWindowToPtyName(window)) != NULL) {
	    (void) TxUtmpEntry(FALSE, ptyName, (char *) NULL);
	}
    }
# endif /* NOTDEF */
    exit(0);
}


void
FsflatDisplayEventProc(clientData, streamID, eventMask)
    ClientData	clientData;
    int		streamID;
    int		eventMask;
{
    static	int	count = 0;
    XEvent	event;

    do {
	count++;
	XNextEvent(fsflatDisplay, &event);
	Sx_HandleEvent(&event);
    } while (QLength(fsflatDisplay) > 0);

    return;
}

void
HandleMonitorStats(clientData, time)
    ClientData	clientData;
    SpriteTime	time;
{
    Mon_StatDirs();
    return;
}

void
HandleMonitorEvent(clientData, streamID, eventMask)
    ClientData	clientData;
    int		streamID;
    int		eventMask;
{
    FsflatHandleMonitorUpdates();
    return;
}

#ifdef ICON

/*
 *----------------------------------------------------------------------
 *
 * CreateIcon --
 *
 *	This procedure creates an icon window for the main window 
 *	using data read in from a X bitmap file.
 *
 *	If the icon file name is "localhost", the hostname of the machine
 *	is used to locate an icon file for the host in a standard directory.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	An icon window is created.
 *
 *----------------------------------------------------------------------
 */

static void
CreateIcon(display, window, foreground, background)
    Display	*display;
    Window window;		/* An icon will be created for this window. */
    unsigned long foreground;	/* Foreground color of the icon. */
    unsigned long background;	/* Background color of the icon. */
{
    char iconfile[256];		/* Buffer to hold a complete icon file name. */
    char hostname[100];		/* Buffer to hold the hostname. */
    char *dot;
    int status;
    XWMHints hints;

    /*
     * See if a icon file name was given on the command line or is in the
     * .Xdefaults file.
     */
    if (iconInfo.name == (char *) NULL) {
	iconInfo.name = XGetDefault(display, "tx", "icon");
	if (iconInfo.name == (char *) NULL) {
	    return;
	}
    }

    /*
     * If the file name is localhost, look in a standard directory for
     * a file named after the host.
     */
    if (strcmp(iconInfo.name, "localhost") == 0) {
	gethostname(hostname, sizeof(hostname));
	dot = strchr(hostname, '.');
	if (dot != (char *) NULL) {
	    *dot = '\0';
	}
	sprintf(iconfile, "%s/%s", ICONDIR, hostname);
	iconInfo.name = iconfile;
    }

    status = XReadBitmapFile(display, window, iconInfo.name, &iconInfo.width,
	    &iconInfo.height, &iconInfo.bitmap, 0, 0);

    if (status == 0) {
	fprintf(stderr,
		"Tx: can't open icon file %s, using default icon.\n", 
		iconInfo.name);
	return;
    } else if (status < 0) {
	fprintf(stderr, 
		"Tx: icon file %s has invalid format.\n", iconInfo.name);
	return;
    }

    hints.flags = InputHint|StateHint|IconWindowHint
	    |IconPixmapHint|IconPositionHint;
    hints.input = False;
    hints.icon_pixmap = iconInfo.bitmap;
    hints.icon_window = XCreateSimpleWindow(display,
	    RootWindow(display, DefaultScreen(display)),
	    iconInfo.x, iconInfo.y, iconInfo.width, iconInfo.height,
	    0, foreground, background);
    hints.icon_x = iconInfo.x;
    hints.icon_y = iconInfo.y;
    XSetWMHints(display, window, &hints);

#ifdef notdef	/* Shouldn't be needed under X11. */
    (void) Sx_HandlerCreate(display, iconInfo.window, ExposureMask,
	    RefreshIcon, (ClientData) 0);
#endif
}
#endif /* ICON */
