/*
 * wishInt.h --
 *
 *	Internal declarations for wish display.
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
 *
 * $Header: /a/newcmds/wish/RCS/wishInt.h,v 1.4 89/01/11 11:58:53 mlgray Exp $ SPRITE (Berkeley)
 */

#ifndef _WISHINT
#define _WISHINT

#include <sys/param.h>
#ifndef AllPlanes
#include <X11/Xlib.h>
#endif
#ifndef USPosition
#include <X11/Xutil.h>
#endif
#include "cmd.h"
#include "mx.h"
#include "tcl.h"

#ifndef Boolean
#define Boolean	int
#define FALSE	0
#define TRUE	1
#endif /* Boolean */


/*
 * Largest number of ASCII characters required to represent an integer.
 */
#define CVT_INT_BUF_SIZE 34

#define	WISH_ROW_SPACING	1	/* a pleasant amount */
#define	WISH_COLUMN_SPACING	15	/* another pleasant amount */
#define UNINITIALIZED		-1	/* number of elements uninitialized */

/*
 * To get a control character.
 */
#define	ctrl(c)	(c - 'a' + 1)

/*
 * To determine whether finding file system attributes for a file is necessary.
 */
#define WISH_ATTR_NECESSARY_P\
    ((aWindow->displayInstructions & ~WISH_NAME_FIELD) != 0) || \
    ((aWindow->sortingInstructions & ~WISH_ALPHA_SORT) != 0) ? TRUE : FALSE

#define WISH_CHAR_TO_WIDTH(numChars, fontPtr)\
    ((numChars) * XTextWidth(fontPtr, "W", 1))

/*
 * Sorting flags are a bitwise and of one of the methods
 * with or without "reverse."
 */
/* methods */ 
#define	WISH_ALPHA_SORT	0x01	/* by alphabetical order */
#define	WISH_ATIME_SORT	0x02	/* by access time */
#define	WISH_MTIME_SORT	0x04	/* by data modify time */
#define	WISH_DTIME_SORT	0x08	/* by descriptor modify time */
#define	WISH_SIZE_SORT	0x10	/* by size */
/* reverse */
#define	WISH_REVERSE_SORT	0x20	/* whether to show in reverse order */

/*
 * Display flags show which fields should be present in the display.
 */
#define	WISH_NAME_FIELD	0x001	/* name of the file */
#define	WISH_FULLNAME_FIELD	0x002	/* full pathname to the file */
#define	WISH_ATIME_FIELD	0x004	/* access time */
#define	WISH_MTIME_FIELD	0x008	/* data modify time */
#define	WISH_DTIME_FIELD	0x010	/* descriptor modify time */
#define	WISH_SIZE_FIELD	0x020	/* size of the file in bytes */
#define	WISH_DEVICE_FIELD	0x040	/* major and minor device numbers */
#define	WISH_PERM_FIELD	0x080	/* permissions, owner and group */
#define	WISH_TYPE_FIELD	0x100	/* directory? link? ascii? etc.... */

#define	WISH_LEFT_BUTTON	0x01
#define	WISH_MIDDLE_BUTTON	0x02
#define	WISH_RIGHT_BUTTON	0x04
#define	WISH_META_BUTTON	0x08
#define	WISH_SHIFT_BUTTON	0x10

#define	WISH_MAX_RULE_LENGTH	(2 * 1024)	/* for now the max length
						 * of a rule stored in a
						 * .wish file is twice
						 * the max length of a path.
						 * No good reason, just easy. */
/* typedefs for use below */
typedef	struct WishFile	WishFile;
typedef	struct WishGroup	WishGroup;
typedef	struct WishColumn	WishColumn;
typedef	struct WishWindow	WishWindow;
typedef	struct WishSelection	WishSelection;
typedef	struct WishGroupBinding	WishGroupBinding;

/*
 * Stuff we need to know about particular file entries.
 * This will contain more information along the lines of a struct	direct
 * soon.  For now, we only keep the name.
 */
struct WishFile {
    char	*name;		/* Name of the file */
    struct stat	*attrPtr;	/* file attributes */
    int		length;		/* length of name in pixels */
    int		x;		/* x and y coords of file name */
    int		y;
    int		myColumn;	/* which column the file is in */
    Boolean	selectedP;	/* is the file selected? */
    Boolean	lineP;		/* is whole line selected? */
    Boolean	highlightP;	/* whether or not it is highlighted */
    WishGroup	*myGroupPtr;	/* ptr back to my group */
    struct	WishFile	*nextPtr;	/* the next one */
};

/*
 * Each group has a selection rule associated with it, and the files selected
 * by that rule.
 */
#define	COMPARISON	0
#define	PROC		1
struct WishGroup {
    int		myColumn;		/* which column I'm in */
    Window	headerWindow;		/* window for header */
    int		x;			/* x and y coords of group header */
    int		y;
    int		width;			/* width and height of header */
    int		height;
    int		entry_x;		/* coords of old entry window */
    int		entry_y;
    int		entry_width;
    WishFile	*fileList;		/* list of selected files */
    int		defType;		/* type of rule, tcl proc or pattern */
    char	*rule;			/* selection rule */
    WishGroupBinding	*groupBindings;	/* mouse button/command bindings */
    int		length;			/* length of rule or name in pixels */
    char	editHeader[WISH_MAX_RULE_LENGTH];	/* for entry window */
    Boolean	selectedP;		/* is the group selected? */
    Boolean	highlightP;		/* whether or not it is highlighted */
    struct	WishGroup	*nextPtr;	/* the next one in list */
};

/*
 * Per-column information.  This data structure could be extended to point to
 * the first file name and group appearing in the column so that redraw
 * could be done be region, perhaps just columns.  Things seem fast enough
 * for now, though, so I won't bother.
 */
struct WishColumn {
    int		x;		/* x coordinate of left side of column */
};

/*
 * For each browser instance, there is a structure of this sort stored
 * in the WishWindowTable.  The structure contains information about
 * the window that the file system client wants to keep.
 */
struct WishWindow {
    Window	surroundingWindow;		/* the surrounding window in
						 * which the subwindows are
						 * packed */
    Window	titleWindow;			/* displays the full pathname
						 * of the current directory and
						 * is editable for people to
						 * type in new desired dir */
    Window	divider1Window;			/* thin window for a border
						 * between title and menu */
    Window	menuBar;			/* menu window under title */
    Window	divider2Window;			/* thin window for a border
						 * between menu and sort */
    Window	sortWindow;			/* displays sorting method */
    Window	divider3Window;			/* thin window for a border
						 * between sort and fields */
    Window	fieldsWindow;			/* window listing fields in
						 * display (time, size, etc) */
    Window	divider4Window;			/* thin window for a border
						 * between fields and display */
    Window	scrollWindow;			/* scrollbar window */
    Window	divider5Window;			/* thin window for a border
						 * between scrollbar and
						 * display */
    Window	txOutsideWindow;		/* tx window */
    Window	divider6Window;			/* thin window for a border
						 * between tx and display */
    Window	displayWindow;			/* the display window */
    int		windowHeight;			/* height of display window */
    int		windowWidth;			/* width of display window */
    Boolean	resizeP;			/* can we change geometry? */
    Boolean	pickSizeP;			/* size window initially */
    int		foreground;			/* foreground color */
    int		background;			/* background color */
    int		border;				/* border color */
    int		selection;			/* selection color */
    int		borderWidth;			/* width of surrounding border,
						 * not really used, maybe... */
    XFontStruct	*fontPtr;			/* font used for display */
    GC		textGc;
    GC		reverseGc;
    int		titleForeground;
    int		titleBackground;
    int		titleBorder;
    XFontStruct	*titleFontPtr;			/* font used for title */
    int		txForeground;
    int		txBackground;
    int		txBorder;
    int		menuForeground;
    int		menuBackground;
    int		menuBorder;
    int		sortForeground;
    int		sortBackground;
    int		fieldsForeground;
    int		fieldsBackground;
    int		entryForeground;
    int		entryBackground;
    int		scrollForeground;
    int		scrollBackground;
    int		scrollElevator;
    char	*geometry;			/* window geometry */
    char	dir[MAXPATHLEN+1];	/* The current dir of the
						 * display.  This is the full
						 * pathname of the top node
						 * in the display. */
    char	editDir[MAXPATHLEN+1];	/* Contains dir
						 * name too, but is meant for
						 * editing in the title window.
						 * By having both dir and
						 * editDir, we don't lose the
						 * actual dir when users type
						 * in a potential new dir. */
    int		displayInstructions;		/* display field flags */
    int		sortingInstructions;		/* sorting flags */
    int		maxNameLength;			/* strln of longest name */
    int		maxEntryWidth;			/* longest entry width in chars,
						 * not window units */
    int		numElements;			/* total number of files */
    int		numGroups;			/* number of groups */
    int		numHiddenGroups;		/* number of groups hidden
						 * because they are empty. */
    Boolean	hideEmptyGroupsP;		/* whether or not to display
						 * headers for empty groups. */
    int		totalDisplayEntries;		/* total # of files + headers
						 * plus spaces to display. */
    int		numRows;			/* number of rows in display */
    int		rowHeight;			/* height in pixels of rows */
    int		firstElement;			/* first visible element */
    int		lastElement;			/* last visible element */
    int		columnWidth;			/* width of columns in pixels */
    WishColumn	*columns;		/* dynamic array of columns */
    int		usedCol;			/* index of last column in
						 * display currently used */
    int		maxColumns;			/* max # of columns allocated */
    WishGroup	*groupList;			/* list of groups */
    WishSelection	*selectionList;		/* list of selected stuff */
    Cmd_Table	cmdTable;			/* window command interface */
    Tcl_Interp	*interp;			/* tcl interpreter */
#ifdef NOTDEF
    char	*cmdString;			/* command in a cmd window */
#endif NOTDEF
    Boolean	dontDisplayChangesP;		/* to keep commands from
						 * attempting redisplay while
						 * we're building the
						 * display from sourced
						 * startup files. */
    Boolean	notifierP;			/* prevent redisplay during
						 * Sx_Notifies. */
};

/*
 * For keeping lists of selected stuff.
 */
struct WishSelection {
    Boolean		fileP;		/* if false, it's a group */
    Boolean		lineP;		/* if true, then the whole line */
    union {
	WishFile	*filePtr;
	WishGroup	*groupPtr;
    } selected;
    struct WishSelection	*nextPtr;
};

struct WishGroupBinding {
    int		button;
    char	*command;
    struct WishGroupBinding	*nextPtr;
};

/* Structure used to build command tables. */
typedef	struct	{
    char	*name;			/* command name */
    int		(*proc)();		/* procedure to process command */
} CmdInfo;


/* externs */

extern	XContext	wishWindowContext;	/* table of window data */
extern	XContext	wishGroupWindowContext;/* table of group data */
extern	int		wishWindowCount;	/* reference count of windows */
							/* error reporting */
extern	char		wishErrorMsg[/* (2*MAXPATHLEN + 50) */];
extern	Boolean		wishDebugP;		/* whether debugging is on */
extern	Boolean		wishResizeP;		/* can application resize? */
extern	Boolean		wishPickSizeP;	/* can appl. pick size? */
extern	Display		*wishDisplay;		/* the display */
extern	Boolean		wishShowEmptyGroupsP;	/* show headers with no files */
extern	int		wishRootHeight;	/* info about root window */
extern	int		wishRootWidth;
extern	char		*wishApplication;	/* application name */
extern	char		wishCurrentDirectory[];	/* keep track of
							 * current directory */

/*	Commands in manual page. */
extern	int	WishQuitCmd();
extern	int	WishToggleSelectionCmd();
extern	int	WishToggleSelEntryCmd();
extern	int	WishCloseCmd();
extern	int	WishRedrawCmd();
extern	int	WishSelectionCmd();
extern	int	WishChangeDirCmd();
extern	int	WishMenuCmd();
extern	int	WishSortFilesCmd();
extern	int	WishChangeFieldsCmd();
extern	int	WishChangeGroupCmd();
extern	int	WishDefineGroupCmd();
extern	int	WishOpenCmd();
extern	int	WishExecCmd();
extern	int	WishBindCmd();
extern	int	WishGroupBindCmd();
extern	int	WishResizeCmd();
extern	int	WishPatternCompareCmd();

/* Other externs. */
extern	void	WishEditDir();
extern	void	WishEditRule();
extern	void	WishSelectDir();
extern	void	WishScroll();
extern	WishWindow	*WishCreate();
extern	void	WishInit();
extern	int	WishGatherNames();
extern	int	WishGatherSingleGroup();
extern	void	WishSetPositions();
extern	void	WishRedraw();
extern	void	WishRedrawFile();
extern	void	WishGetFileFields();
extern	void	WishRedrawGroup();
extern	void	WishSetWindowAndRowInfo();
extern	void	WishHandleDrawingEvent();
extern	void	WishHandleDestructionEvent();
extern	void	WishHandleMonitorUpdates();
extern	void	WishMouseEvent();
extern	void	WishHighlightMovement();
extern	void	WishHandleEnterEvent();
extern	char	*WishCanonicalDir();
extern	void	WishGarbageCollect();
extern	void	WishChangeSelection();
extern	void	WishClearWholeSelection();
extern	WishGroup	*WishMapCoordsToGroup();
extern	WishFile	*WishMapCoordsToFile();
extern	void	WishCmdTableInit();
extern	void	WishAddGroupBinding();
extern	void	WishDeleteGroupBindings();
extern	char	*WishGetGroupBinding();
extern	int	WishDoCmd();
extern	void	WishChangeDir();
extern	void	WishSourceConfig();
extern	void	Cmd_BindingCreate();
extern	char	*Cmd_BindingGet();
extern	void	Cmd_BindingDelete();
extern	Boolean	Cmd_EnumBindings();
extern	int	Cmd_MapKey();
extern	Cmd_Table	Cmd_TableCreate();
extern	void	Cmd_TableDelete();
extern	void	WishCvtToPrintable();
extern	void	WishKeyProc();
extern	void	WishMenuProc();
extern	void	WishMenuEntryProc();
extern	void	WishSetSort();
extern	void	WishGetCompareProc();
extern	void	WishGarbageGroup();
extern	int	WishDoTclSelect();
extern	void	WishDumpState();

/* should be in string.h! */
extern	char	*index();

#endif _WISHINT
