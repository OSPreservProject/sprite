/*
 * mx.h --
 *
 *	This file declares things that are exported by one file within
 *	the Mx editor and imported by other files.  These are also things
 *	that are available to outside clients since the Mx/Tx
 *	stuff has been made into a library.
 *
 * Copyright (C) 1986 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /sprite/src/lib/mx/RCS/mx.h,v 1.18 90/03/25 15:16:51 ouster Exp $ SPRITE (Berkeley)
 */

#ifndef _MX
#define _MX

#ifndef _XLIB_H_
#include <X11/Xlib.h>
#endif
#include <stdio.h>

/*
 * The structure below is used as an indicator of a location within
 * a file.  Below it are macros for comparing two positions.
 */

typedef struct {
    int lineIndex;	/* Index of line (0 means first line in file). */
    int charIndex;	/* Index of character within a line. */
} Mx_Position;

#define MX_POS_EQUAL(a, b) \
	(((a).lineIndex == (b).lineIndex) \
	&& ((a).charIndex == (b).charIndex))

#define MX_POS_LESS(a, b) \
	(((a).lineIndex < (b).lineIndex) \
	|| (((a).lineIndex == (b).lineIndex) \
	&& ((a).charIndex < (b).charIndex)))

#define MX_POS_LEQ(a, b) \
	(((a).lineIndex < (b).lineIndex) \
	|| (((a).lineIndex == (b).lineIndex) \
	&& ((a).charIndex <= (b).charIndex)))
/*
 * The definitions below are dummy ones, so that clients don't have
 * to see the real contents of data structures.  Instead, clients just
 * pass tokens to the procedures that manipulate the structures.
 */

typedef struct Mx_File		*Mx_File;
typedef struct Mx_Floater	*Mx_Floater;
typedef struct Mx_Spy		*Mx_Spy;
typedef struct Undo_Log		*Undo_Log;

/*
 * Records of the following type are used to pass in information
 * to Mx_Make when creating an editable window.
 */

typedef struct {
    Mx_File file;		/* File that's already been loaded and should
				 * be displayed in the window.  If NULL, name
				 * is used to open file. */
    char *name;			/* Name of file to be loaded in window,
				 * or NULL if file doesn't correspond to
				 * anything on disk. */
    XFontStruct *fontPtr;	/* Font to use for window.  If NULL, use
				 * Sx default font. */
    int width, height;		/* Dimensions of containing window,
    				 * in pixels. */
    unsigned long foreground;	/* Foreground color to use for window. */
    unsigned long background;	/* Background color to use for window. */
    unsigned long border;	/* Color to use for border. */
    unsigned long sbForeground;	/* Color to use for scrollbar foreground. */
    unsigned long sbBackground;	/* Color to use for scrollbar background. */
    unsigned long sbElevator;	/* Color to use for scrollbar elevator. */
    unsigned long titleForeground;/* Color to use for title foreground. */
    unsigned long titleBackground;/* Color to use for title background. */
    unsigned long titleStripe;	/* Color to use for title stripe. */
    int flags;			/* Miscellaneous flag values.  See below. */
} Mx_WindowInfo;

/*
 * Flag values for Mx_WindowInfo structures:
 *
 * MX_UNDO:		1 means create undo log and use it to track changes
 *			in the file (name must be non-NULL).  0 means no
 *			undoing.
 * MX_DELETE:		1 means close the Mx_File when the last window on
 *			the window is deleted.  0 means don't ever close
 *			the file:  the caller will handle it.
 * MX_NO_TITLE:		Don't actually display a title bar:  a window
 *			manager will take care of it.
 */

#define MX_UNDO		1
#define MX_DELETE	2
#define MX_NO_TITLE	8

/*
 * Records of the following type are used to pass information to
 * Tx_Make when creating typescript windows.
 */

typedef struct {
    Window source;		/* If non-NULL, the new window should
    				 * view the same typescript as this window.
				 * If NULL, a new typescript is created. */
    char *title;		/* Character string to display in title.  */
    XFontStruct *fontPtr;	/* Font to use for window.  If NULL, use
				 * Sx default font. */
    int width, height;		/* Dimensions of containing window,
    				 * in pixels. */
    unsigned long foreground;	/* Foreground color to use for window. */
    unsigned long background;	/* Background color to use for window. */
    unsigned long border;	/* Color to use for border. */
    unsigned long sbForeground;	/* Color to use for scrollbar foreground. */
    unsigned long sbBackground;	/* Color to use for scrollbar background. */
    unsigned long sbElevator;	/* Color to use for scrollbar elevator. */
    unsigned long titleForeground;/* Color to use for title foreground. */
    unsigned long titleBackground;/* Color to use for title background. */
    unsigned long titleStripe;	/* Color to use for title stripe. */
    int flags;			/* Miscellaneous flags:  see below. */
} Tx_WindowInfo;

/*
 * Flag values from Tx_WindowInfo structure:
 *
 * TX_NO_TITLE:		Don't actually display a title bar:  a window
 *			manager will take care of it.
 */

#define TX_NO_TITLE	1

/*
 * The flag bits below are used as parameters to Mx_CreateSpy to
 * indicate when a spy procedure should be called.
 */

#define MX_BEFORE		1
#define MX_AFTER		2

/*
 * The values below may be passed to Mx_HighlightCreate to specify
 * how a highlight is to be displayed:
 */

#define MX_REVERSE		1
#define MX_GRAY			2
#define MX_UNDERLINE		3
#define MX_BOX			4

/*
 * Constants for undo module:
 */

#define UNDO_ID_LENGTH 60
#define UNDO_NAME_LENGTH 600

/*
 * Exported procedures:
 */

extern void		Mx_MarkClean();
extern Mx_Position	Mx_EndOfFile();
extern Mx_File		Mx_FileLoad();
extern void		Mx_FileClose();
extern int		Mx_FileWrite();
extern Mx_Floater	Mx_FloaterCreate();
extern void		Mx_FloaterDelete();
extern char *		Mx_GetLine();
extern Mx_Position	Mx_Offset();
extern void		Mx_ReplaceBytes();
extern Mx_Spy		Mx_SpyCreate();
extern void		Mx_SpyDelete();
extern Mx_Position	Mx_ZeroPosition;

extern int		Mx_SearchMask();
extern int		Mx_SearchParen();
extern int		Mx_SearchPattern();
extern int		Mx_SearchRegExp();
extern void		Mx_ReplaceRegExp();
extern int		Mx_SearchWord();
extern char *		Mx_CompileRegExp();
extern int		Mx_GetTag();

extern Undo_Log		Undo_LogCreate();
extern void		Undo_LogDelete();
extern int		Undo_FindLog();
extern void		Undo_Mark();
extern void		Undo_SetVersion();
extern int		Undo_Last();
extern int		Undo_More();
extern int		Undo_Recover();

extern void		Mx_Cleanup();
extern int		Mx_Make();
extern void		Mx_Update();
extern int		mx_FileCount;
extern Display *	mx_Display;

extern int		Tx_Command();
extern int		Tx_Make();
extern void		Tx_Output();
extern int		Ts_SetupAndFork();
extern void		Tx_Update();
extern void		Tx_WindowEventProc();
extern int		tx_TypescriptCount;
extern Display *	tx_Display;

#endif _MX
