/*
 * fsflatInt.h --
 *
 *	Internal declarations for fsflat display.
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
 * $Header: fsflatInt.h,v 1.1 88/10/03 12:49:03 mlgray Exp $ SPRITE (Berkeley)
 */

#ifndef _FSFLATINT
#define _FSFLATINT

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

#define	FSFLAT_ROW_SPACING	1	/* a pleasant amount */
#define	FSFLAT_COLUMN_SPACING	15	/* another pleasant amount */
#define UNINITIALIZED		-1	/* number of elements uninitialized */

/*
 * To get a control character.
 */
#define	ctrl(c)	(c - 'a' + 1)

/*
 * To determine whether finding file system attributes for a file is necessary.
 */
#define FSFLAT_ATTR_NECESSARY_P\
    ((aWindow->displayInstructions & ~FSFLAT_NAME_FIELD) != 0) || \
    ((aWindow->sortingInstructions & ~FSFLAT_ALPHA_SORT) != 0) ? TRUE : FALSE

#define FSFLAT_CHAR_TO_WIDTH(numChars, fontPtr)\
    ((numChars) * XTextWidth(fontPtr, "W", 1))

/*
 * Sorting flags are a bitwise and of one of the methods
 * with or without "reverse."
 */
/* methods */ 
#define	FSFLAT_ALPHA_SORT	0x01	/* by alphabetical order */
#define	FSFLAT_ATIME_SORT	0x02	/* by access time */
#define	FSFLAT_MTIME_SORT	0x04	/* by data modify time */
#define	FSFLAT_DTIME_SORT	0x08	/* by descriptor modify time */
#define	FSFLAT_SIZE_SORT	0x10	/* by size */
/* reverse */
#define	FSFLAT_REVERSE_SORT	0x20	/* whether to show in reverse order */

/*
 * Display flags show which fields should be present in the display.
 */
#define	FSFLAT_NAME_FIELD	0x001	/* name of the file */
#define	FSFLAT_FULLNAME_FIELD	0x002	/* full pathname to the file */
#define	FSFLAT_ATIME_FIELD	0x004	/* access time */
#define	FSFLAT_MTIME_FIELD	0x008	/* data modify time */
#define	FSFLAT_DTIME_FIELD	0x010	/* descriptor modify time */
#define	FSFLAT_SIZE_FIELD	0x020	/* size of the file in bytes */
#define	FSFLAT_DEVICE_FIELD	0x040	/* major and minor device numbers */
#define	FSFLAT_PERM_FIELD	0x080	/* permissions, owner and group */
#define	FSFLAT_TYPE_FIELD	0x100	/* directory? link? ascii? etc.... */

#define	FSFLAT_LEFT_BUTTON	0x01
#define	FSFLAT_MIDDLE_BUTTON	0x02
#define	FSFLAT_RIGHT_BUTTON	0x04
#define	FSFLAT_META_BUTTON	0x08
#define	FSFLAT_SHIFT_BUTTON	0x10

#define	FSFLAT_MAX_RULE_LENGTH	(2 * 1024)	/* for now the max length
						 * of a rule stored in a
						 * .fsflat file is twice
						 * the max length of a path.
						 * No good reason, just easy. */
/* typedefs for use below */
typedef	struct FsflatFile	FsflatFile;
typedef	struct FsflatGroup	FsflatGroup;
typedef	struct FsflatColumn	FsflatColumn;
typedef	struct FsflatWindow	FsflatWindow;
typedef	struct FsflatSelection	FsflatSelection;
typedef	struct FsflatGroupBinding	FsflatGroupBinding;

/*
 * Stuff we need to know about particular file entries.
 * This will contain more information along the lines of a struct	direct
 * soon.  For now, we only keep the name.
 */
struct FsflatFile {
    char	*name;		/* Name of the file */
    struct stat	*attrPtr;	/* file attributes */
    int		length;		/* length of name in pixels */
    int		x;		/* x and y coords of file name */
    int		y;
    int		myColumn;	/* which column the file is in */
    Boolean	selectedP;	/* is the file selected? */
    Boolean	lineP;		/* is whole line selected? */
    Boolean	highlightP;	/* whether or not it is highlighted */
    FsflatGroup	*myGroupPtr;	/* ptr back to my group */
    struct	FsflatFile	*nextPtr;	/* the next one */
};

/*
 * Each group has a selection rule associated with it, and the files selected
 * by that rule.
 */
#define	COMPARISON	0
#define	PROC		1
struct FsflatGroup {
    int		myColumn;		/* which column I'm in */
    Window	headerWindow;		/* window for header */
    int		x;			/* x and y coords of group header */
    int		y;
    int		width;			/* width and height of header */
    int		height;
    int		entry_x;		/* coords of old entry window */
    int		entry_y;
    int		entry_width;
    FsflatFile	*fileList;		/* list of selected files */
    int		defType;		/* type of rule, tcl proc or pattern */
    char	*rule;			/* selection rule */
    FsflatGroupBinding	*groupBindings;	/* mouse button/command bindings */
    int		length;			/* length of rule or name in pixels */
    char	editHeader[FSFLAT_MAX_RULE_LENGTH];	/* for entry window */
    Boolean	selectedP;		/* is the group selected? */
    Boolean	highlightP;		/* whether or not it is highlighted */
    struct	FsflatGroup	*nextPtr;	/* the next one in list */
};

/*
 * Per-column information.  This data structure could be extended to point to
 * the first file name and group appearing in the column so that redraw
 * could be done be region, perhaps just columns.  Things seem fast enough
 * for now, though, so I won't bother.
 */
struct FsflatColumn {
    int		x;		/* x coordinate of left side of column */
};

/*
 * For each browser instance, there is a structure of this sort stored
 * in the FsflatWindowTable.  The structure contains information about
 * the window that the file system client wants to keep.
 */
struct FsflatWindow {
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
    FsflatColumn	*columns;		/* dynamic array of columns */
    int		usedCol;			/* index of last column in
						 * display currently used */
    int		maxColumns;			/* max # of columns allocated */
    FsflatGroup	*groupList;			/* list of groups */
    FsflatSelection	*selectionList;		/* list of selected stuff */
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
};

/*
 * For keeping lists of selected stuff.
 */
struct FsflatSelection {
    Boolean		fileP;		/* if false, it's a group */
    Boolean		lineP;		/* if true, then the whole line */
    union {
	FsflatFile	*filePtr;
	FsflatGroup	*groupPtr;
    } selected;
    struct FsflatSelection	*nextPtr;
};

struct FsflatGroupBinding {
    int		button;
    char	*command;
    struct FsflatGroupBinding	*nextPtr;
};

/* Structure used to build command tables. */
typedef	struct	{
    char	*name;			/* command name */
    int		(*proc)();		/* procedure to process command */
} CmdInfo;


/* externs */

extern	XContext	fsflatWindowContext;	/* table of window data */
extern	XContext	fsflatGroupWindowContext;/* table of group data */
extern	int		fsflatWindowCount;	/* reference count of windows */
							/* error reporting */
extern	char		fsflatErrorMsg[/* (2*MAXPATHLEN + 50) */];
extern	Boolean		fsflatDebugP;		/* whether debugging is on */
extern	Boolean		fsflatResizeP;		/* can application resize? */
extern	Boolean		fsflatPickSizeP;	/* can appl. pick size? */
extern	Display		*fsflatDisplay;		/* the display */
extern	Boolean		fsflatShowEmptyGroupsP;	/* show headers with no files */
extern	int		fsflatRootHeight;	/* info about root window */
extern	int		fsflatRootWidth;
extern	char		*fsflatApplication;	/* application name */
extern	char		fsflatCurrentDirectory[];	/* keep track of
							 * current directory */

/*	Commands in manual page. */
extern	int	FsflatQuitCmd();
extern	int	FsflatToggleSelectionCmd();
extern	int	FsflatToggleSelEntryCmd();
extern	int	FsflatCloseCmd();
extern	int	FsflatRedrawCmd();
extern	int	FsflatSelectionCmd();
extern	int	FsflatChangeDirCmd();
extern	int	FsflatMenuCmd();
extern	int	FsflatSortFilesCmd();
extern	int	FsflatChangeFieldsCmd();
extern	int	FsflatChangeGroupCmd();
extern	int	FsflatDefineGroupCmd();
extern	int	FsflatOpenCmd();
extern	int	FsflatExecCmd();
extern	int	FsflatBindCmd();
extern	int	FsflatGroupBindCmd();
extern	int	FsflatResizeCmd();
extern	int	FsflatPatternCompareCmd();

/* Other externs. */
extern	void	FsflatEditDir();
extern	void	FsflatEditRule();
extern	void	FsflatSelectDir();
extern	void	FsflatScroll();
extern	FsflatWindow	*FsflatCreate();
extern	void	FsflatInit();
extern	int	FsflatGatherNames();
extern	int	FsflatGatherSingleGroup();
extern	void	FsflatSetPositions();
extern	void	FsflatRedraw();
extern	void	FsflatRedrawFile();
extern	void	FsflatGetFileFields();
extern	void	FsflatRedrawGroup();
extern	void	FsflatSetWindowAndRowInfo();
extern	void	FsflatHandleDrawingEvent();
extern	void	FsflatHandleDestructionEvent();
extern	void	FsflatHandleMonitorUpdates();
extern	void	FsflatMouseEvent();
extern	void	FsflatHighlightMovement();
extern	void	FsflatHandleEnterEvent();
extern	char	*FsflatCanonicalDir();
extern	void	FsflatGarbageCollect();
extern	void	FsflatChangeSelection();
extern	void	FsflatClearWholeSelection();
extern	FsflatGroup	*FsflatMapCoordsToGroup();
extern	FsflatFile	*FsflatMapCoordsToFile();
extern	void	FsflatCmdTableInit();
extern	void	FsflatAddGroupBinding();
extern	void	FsflatDeleteGroupBindings();
extern	char	*FsflatGetGroupBinding();
extern	int	FsflatDoCmd();
extern	void	FsflatChangeDir();
extern	void	FsflatSourceConfig();
extern	void	Cmd_BindingCreate();
extern	char	*Cmd_BindingGet();
extern	void	Cmd_BindingDelete();
extern	Boolean	Cmd_EnumBindings();
extern	int	Cmd_MapKey();
extern	Cmd_Table	Cmd_TableCreate();
extern	void	Cmd_TableDelete();
extern	void	FsflatCvtToPrintable();
extern	void	FsflatKeyProc();
extern	void	FsflatMenuProc();
extern	void	FsflatMenuEntryProc();
extern	void	FsflatSetSort();
extern	void	FsflatGetCompareProc();
extern	void	FsflatGarbageGroup();
extern	int	FsflatDoTclSelect();

/* should be in string.h! */
extern	char	*index();

#endif _FSFLATINT
