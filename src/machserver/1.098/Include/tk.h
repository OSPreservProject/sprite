/*
 * tk.h --
 *
 *	Declarations for Tk-related things that are visible
 *	outside of the Tk module itself.
 *
 * Copyright 1989, 1990 Regents of the University of California.
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /user5/ouster/wish/RCS/tk.h,v 1.63 91/04/06 14:47:30 ouster Exp $ SPRITE (Berkeley)
 */

#ifndef _TK
#define _TK

#ifndef _TCL
#include <tcl.h>
#endif
#ifndef _XLIB_H
#include <X11/Xlib.h>
#endif

/*
 * Definitions that allow Tk to be used either with or without
 * ANSI C features like function prototypes.
 */

#undef _ARGS_
#undef const
#if defined(USE_ANSI) && defined(__STDC__)
#   define _ARGS_(x)	x
#else
#   define _ARGS_(x)	()
#   define const
#endif

#ifndef _CLIENTDATA
typedef int *ClientData;
#define _CLIENTDATA
#endif

/*
 * Dummy types that are used by clients:
 */

typedef int Tk_ErrorHandler;
typedef int Tk_TimerToken;
typedef struct Tk_Window *Tk_Window;
typedef struct Tk_3DBorder *Tk_3DBorder;

/*
 * Additional types exported to clients.
 */

typedef char *Tk_Uid;

/*
 * Structure used to specify how to handle argv options.
 */

typedef struct {
    char *key;		/* The key string that flags the option in the
			 * argv array. */
    int type;		/* Indicates option type;  see below. */
    char *src;		/* Value to be used in setting dst;  usage
			 * depends on type. */
    char *dst;		/* Address of value to be modified;  usage
			 * depends on type. */
    char *help;		/* Documentation message describing this option. */
} Tk_ArgvInfo;

/*
 * Legal values for the type field of a Tk_ArgvInfo: see the user
 * documentation for details.
 */

#define TK_ARGV_CONSTANT		15
#define TK_ARGV_INT			16
#define TK_ARGV_STRING			17
#define TK_ARGV_UID			18
#define TK_ARGV_REST			19
#define TK_ARGV_FLOAT			20
#define TK_ARGV_FUNC			21
#define TK_ARGV_GENFUNC			22
#define TK_ARGV_HELP			23
#define TK_ARGV_CONST_OPTION		24
#define TK_ARGV_OPTION_VALUE		25
#define TK_ARGV_OPTION_NAME_VALUE	26
#define TK_ARGV_END			27

/*
 * Flag bits for passing to Tk_ParseArgv:
 */

#define TK_ARGV_NO_DEFAULTS		0x1
#define TK_ARGV_NO_LEFTOVERS		0x2
#define TK_ARGV_NO_ABBREV		0x4
#define TK_ARGV_DONT_SKIP_FIRST_ARG	0x8

/*
 * Structure used to specify information for Tk_ConfigureWidget.  Each
 * structure gives complete information for one option, including
 * how the option is specified on the command line, where it appears
 * in the option database, etc.
 */

typedef struct {
    int type;			/* Type of option, such as
				 * TK_CONFIG_COLOR;  see definitions
				 * below.  Last option in table
				 * has type TK_CONFIG_END. */
    char *argvName;		/* Switch used to specify option
				 * in argv.  NULL means this spec is
				 * part of a group. */
    char *dbName;		/* Name for option in option database. */
    char *dbClass;		/* Class for option in database. */
    char *defValue;		/* Default value for option if not
				 * specified in command line or database. */
    int offset;			/* Where in widget record to store value;
				 * use Tk_Offset macro to generate values
				 * for this. */
    int specFlags;		/* For use by Tk_Configure;  client must
				 * initialize to zero. */
} Tk_ConfigSpec;

/*
 * Type values for Tk_ConfigSpec structures.  See the user
 * documentation for details.
 */

#define TK_CONFIG_BOOLEAN	1
#define TK_CONFIG_INT		2
#define TK_CONFIG_DOUBLE	3
#define TK_CONFIG_STRING	4
#define TK_CONFIG_UID		5
#define TK_CONFIG_COLOR		6
#define TK_CONFIG_FONT		7
#define TK_CONFIG_BITMAP	8
#define TK_CONFIG_BORDER	9
#define TK_CONFIG_RELIEF	10
#define TK_CONFIG_CURSOR	11
#define TK_CONFIG_SYNONYM	12
#define TK_CONFIG_END		13

/*
 * Macro to use to fill in "offset" fields of Tk_ConfigInfos.
 * Computes number of bytes from beginning of structure to a
 * given field.
 */

#define Tk_Offset(type, field) ((int) ((char *) &((type *) 0)->field))

/*
 * User-visible flag values for Tk_ConfigInfo structures and
 * also for calls to Tk_ConfigureWidget.  Before changing
 * any values here, coordinate with
 */

#define TK_CONFIG_ARGV_ONLY	1
#define TK_CONFIG_COLOR_ONLY	2
#define TK_CONFIG_MONO_ONLY	4
#define TK_CONFIG_USER_BIT	0x100

/*
 * Need to have tkInt.h included in order for access macros below
 * to work.  However, can't include tkInt.h at the top of the file
 * because it depends on some of the definitions in this file.
 */

#ifndef _TKINT
#include <tkInt.h>
#endif

/*
 * Bits to pass to Tk_CreateFileHandler to indicate what sorts
 * of events are of interest:
 */

#define TK_READABLE	1
#define TK_WRITABLE	2
#define TK_EXCEPTION	4

/*
 * Priority levels to pass to Tk_AddOption:
 */

#define TK_WIDGET_DEFAULT_PRIO	20
#define TK_STARTUP_FILE_PRIO	40
#define TK_USER_DEFAULT_PRIO	60
#define TK_INTERACTIVE_PRIO	80
#define TK_MAX_PRIO		100

/*
 * Relief values returned by Tk_GetRelief:
 */

#define TK_RELIEF_RAISED	1
#define TK_RELIEF_FLAT		2
#define TK_RELIEF_SUNKEN	4

/*
 * Special EnterNotify/LeaveNotify "mode" for use in events
 * generated by tkShare.c.  Pick a high enough value that it's
 * unlikely to conflict with existing values (like NotifyNormal)
 * or any new values defined in the future.
 */

#define TK_NOTIFY_SHARE		20

/*
 *--------------------------------------------------------------
 *
 * Macros for querying Tk_Window structures.  See the
 * manual entries for documentation.
 *
 *--------------------------------------------------------------
 */

#define Tk_Display(tkwin)		(((TkWindow *) (tkwin))->display)
#define Tk_ScreenNumber(tkwin)		(((TkWindow *) (tkwin))->screenNum)
#define Tk_Screen(tkwin)		(ScreenOfDisplay(Tk_Display(tkwin), \
	Tk_ScreenNumber(tkwin)))
#define Tk_WindowId(tkwin)		(((TkWindow *) (tkwin))->window)
#define Tk_Name(tkwin)			(((TkWindow *) (tkwin))->nameUid)
#define Tk_Class(tkwin) 		(((TkWindow *) (tkwin))->classUid)
#define Tk_PathName(tkwin) 		(((TkWindow *) (tkwin))->pathName)
#define Tk_X(tkwin)			(((TkWindow *) (tkwin))->changes.x)
#define Tk_Y(tkwin)			(((TkWindow *) (tkwin))->changes.y)
#define Tk_Width(tkwin)			(((TkWindow *) (tkwin))->changes.width)
#define Tk_Height(tkwin) \
    (((TkWindow *) (tkwin))->changes.height)
#define Tk_Changes(tkwin)		(&((TkWindow *) (tkwin))->changes)
#define Tk_Attributes(tkwin)		(&((TkWindow *) (tkwin))->atts)
#define Tk_IsMapped(tkwin) \
    (((TkWindow *) (tkwin))->flags & TK_MAPPED)
#define Tk_ReqWidth(tkwin)		(((TkWindow *) (tkwin))->reqWidth)
#define Tk_ReqHeight(tkwin)		(((TkWindow *) (tkwin))->reqHeight)
#define Tk_InternalBorderWidth(tkwin) \
    (((TkWindow *) (tkwin))->internalBorderWidth)
#define Tk_Parent(tkwin) \
    ((Tk_Window) (((TkWindow *) (tkwin))->parentPtr))
#define Tk_DisplayName(tkwin)		(((TkWindow *) (tkwin))->dispPtr->name)

/*
 *--------------------------------------------------------------
 *
 * Exported procedures and variables.
 *
 *--------------------------------------------------------------
 */

extern void		Tk_AddOption _ARGS_((Tk_Window tkwin, char *name,
			    char *value, int priority));
extern void		Tk_CancelIdleCall _ARGS_((
			    void (*proc)(ClientData clientData),
				ClientData clientData));
extern void		Tk_ChangeWindowAttributes _ARGS_((Tk_Window tkwin,
			    unsigned long valueMask,
			    XSetWindowAttributes *attsPtr));
extern int		Tk_ConfigureInfo _ARGS_((Tcl_Interp *interp,
			    Tk_Window tkwin, Tk_ConfigSpec *specs,
			    char *widgRec, char *argvName, int flags));
extern int		Tk_ConfigureWidget _ARGS_((Tcl_Interp *interp,
			    Tk_Window tkwin, Tk_ConfigSpec *specs,
			    int argc, char **argv, char *widgRec,
			    int flags));
extern Tk_ErrorHandler	Tk_CreateErrorHandler _ARGS_((Display *display,
			    int error, int request, int minorCode,
			    int (*errorProc)(ClientData clientData,
				XErrorEvent *errEventPtr),
			    ClientData clientData));
extern void		Tk_CreateEventHandler _ARGS_((Tk_Window token,
			    unsigned long mask,
			    void (*proc)(ClientData clientData,
				XEvent *eventPtr),
			    ClientData clientData));
extern void		Tk_CreateFileHandler _ARGS_((int fd, int mask,
			    void (*proc)(ClientData clientData, int mask),
			    ClientData clientData));
extern void		Tk_CreateFocusHandler _ARGS_((Tk_Window tkwin,
			    void (*proc)(ClientData clientData,
				int gotFocus),
			    ClientData clientData));
extern Tk_Window	Tk_CreateMainWindow _ARGS_((Tcl_Interp *interp,
			    char *screenName, char *baseName));
extern void		Tk_CreateSelHandler _ARGS_((Tk_Window tkwin,
			    Atom target,
			    int (*proc)(ClientData clientData,
				int offset, char *buffer, int maxBytes),
			    ClientData clientData, Atom format));
extern Tk_TimerToken	Tk_CreateTimerHandler _ARGS_((int milliseconds,
			    void (*proc)(ClientData clientData),
			    ClientData clientData));
extern Tk_Window	Tk_CreateWindow _ARGS_((Tcl_Interp *interp,
			    Tk_Window parent, char *name, char *screenName));
extern Tk_Window	Tk_CreateWindowFromPath _ARGS_((Tcl_Interp *interp,
			    Tk_Window tkwin, char *pathName,
			    char *screenName));
extern void		Tk_DeleteErrorHandler _ARGS_((Tk_ErrorHandler handler));
extern void		Tk_DeleteEventHandler _ARGS_((Tk_Window token,
			    unsigned long mask,
			    void (*proc)(ClientData clientData,
				XEvent *eventPtr),
			    ClientData clientData));
extern void		Tk_DeleteFileHandler _ARGS_((int fd));
extern void		Tk_DeleteTimerHandler _ARGS_((Tk_TimerToken token));
extern void		Tk_DestroyWindow _ARGS_((Tk_Window tkwin));
extern int		Tk_DoOneEvent _ARGS_((int dontWait));
extern void		Tk_DoWhenIdle _ARGS_((
			    void (*proc)(ClientData clientData),
			    ClientData clientData));
extern void		Tk_Draw3DPolygon _ARGS_((Display *display,
			    Drawable drawable, Tk_3DBorder border,
			    XPoint *pointPtr, int numPoints, int borderWidth,
			    int leftRelief));
extern void		Tk_Draw3DRectangle _ARGS_((Display *display,
			    Drawable drawable, Tk_3DBorder border, int x,
			    int y, int width, int height, int borderWidth,
			    int relief));
extern void		Tk_EventuallyFree _ARGS_((ClientData clientData,
			    void (*freeProc)(ClientData clientData)));
extern void		Tk_Fill3DPolygon _ARGS_((Display *display,
			    Drawable drawable, Tk_3DBorder border,
			    XPoint *pointPtr, int numPoints, int borderWidth,
			    int leftRelief));
extern void		Tk_Fill3DRectangle _ARGS_((Display *display,
			    Drawable drawable, Tk_3DBorder border, int x,
			    int y, int width, int height, int borderWidth,
			    int relief));
extern void		Tk_Free3DBorder _ARGS_((Tk_3DBorder border));
extern void		Tk_FreeBitmap _ARGS_((Pixmap bitmap));
extern void		Tk_FreeColor _ARGS_((XColor *colorPtr));
extern void		Tk_FreeCursor _ARGS_((Cursor cursor));
extern void		Tk_FreeFontStruct _ARGS_((XFontStruct *fontStructPtr));
extern void		Tk_FreeGC _ARGS_((GC gc));
extern void		Tk_GeometryRequest _ARGS_((Tk_Window tkwin,
			    int reqWidth,  int reqHeight));
extern Tk_3DBorder	Tk_Get3DBorder _ARGS_((Tcl_Interp *interp,
			    Tk_Window tkwin, Colormap colormap,
			    Tk_Uid colorName));
extern char *		Tk_GetAtomName _ARGS_((Tk_Window tkwin, Atom atom));
extern Pixmap		Tk_GetBitmap _ARGS_((Tcl_Interp *interp,
			    Tk_Window tkwin, Tk_Uid string));
extern Pixmap		Tk_GetBitmapFromData _ARGS_((Tcl_Interp *interp,
			    Tk_Window tkwin, char *source,
			    unsigned int width, unsigned int height));
extern XColor *		Tk_GetColor _ARGS_((Tcl_Interp *interp,
			    Tk_Window tkwin, Colormap colormap, Tk_Uid name));
extern XColor *		Tk_GetColorByValue _ARGS_((Tcl_Interp *interp,
			    Tk_Window tkwin, Colormap colormap,
			    XColor *colorPtr));
extern Cursor		Tk_GetCursor _ARGS_((Tcl_Interp *interp,
			    Tk_Window tkwin, Tk_Uid string));
extern Cursor		Tk_GetCursorFromData _ARGS_((Tcl_Interp *interp,
			    Tk_Window tkwin, char *source, char *mask,
			    unsigned int width, unsigned int height,
			    int xHot, int yHot, Tk_Uid fg, Tk_Uid bg));
extern XFontStruct *	Tk_GetFontStruct _ARGS_((Tcl_Interp *interp,
			    Tk_Window tkwin, Tk_Uid name));
extern GC		Tk_GetGC _ARGS_((Tk_Window tkwin,
			    unsigned long valueMask, XGCValues *valuePtr));
extern Tk_Uid		Tk_GetOption _ARGS_((Tk_Window tkwin, char *name,
			    char *class));
extern int		Tk_GetRelief _ARGS_((Tcl_Interp *interp, char *name,
			    int *reliefPtr));
extern void		Tk_GetRootCoords _ARGS_ ((Tk_Window tkwin,
			    int *xPtr, int *yPtr));
extern int		Tk_GetSelection _ARGS_((Tcl_Interp *interp,
			    Tk_Window tkwin, Atom target,
			    int (*proc)(ClientData clientData,
				Tcl_Interp *interp, char *portion),
			    ClientData clientData));
extern Tk_Uid		Tk_GetUid _ARGS_((char *string));
extern void		Tk_HandleEvent _ARGS_((XEvent *eventPtr));
extern Atom		Tk_InternAtom _ARGS_((Tk_Window tkwin, char *name));
extern void		Tk_MainLoop _ARGS_((void));
extern void		Tk_MakeWindowExist _ARGS_((Tk_Window tkwin));
extern void		Tk_ManageGeometry _ARGS_((Tk_Window tkwin,
			    void (*proc)(ClientData clientData,
				Tk_Window tkwin),
			    ClientData clientData));
extern void		Tk_MapWindow _ARGS_((Tk_Window tkwin));
extern void		Tk_MoveResizeWindow _ARGS_((Tk_Window tkwin, int x,
			    int y, unsigned int width, unsigned int height));
extern void		Tk_MoveWindow _ARGS_((Tk_Window tkwin, int x, int y));
extern char *		Tk_NameOf3DBorder _ARGS_((Tk_3DBorder border));
extern char *		Tk_NameOfBitmap _ARGS_((Pixmap bitmap));
extern char *		Tk_NameOfColor _ARGS_((XColor *colorPtr));
extern char *		Tk_NameOfCursor _ARGS_((Cursor cursor));
extern char *		Tk_NameOfFontStruct _ARGS_((
			    XFontStruct *fontStructPtr));
extern char *		Tk_NameOfRelief _ARGS_((int relief));
extern Tk_Window	Tk_NameToWindow _ARGS_((Tcl_Interp *interp,
			    char *pathName, Tk_Window tkwin));
extern void		Tk_OwnSelection _ARGS_((Tk_Window tkwin,
			    void (*proc)(ClientData clientData),
			    ClientData clientData));
extern int		Tk_ParseArgv _ARGS_((Tcl_Interp *interp,
			    Tk_Window tkwin, int *argcPtr, char **argv,
			    Tk_ArgvInfo *argTable, int flags));
extern void		Tk_Preserve _ARGS_((ClientData clientData));
extern int		Tk_RegisterInterp _ARGS_((Tcl_Interp *interp,
			    char *name, Tk_Window tkwin));
extern void		Tk_Release _ARGS_((ClientData clientData));
extern void		Tk_ResizeWindow _ARGS_((Tk_Window tkwin,
			    unsigned int width, unsigned int height));
extern Bool		(*Tk_RestrictEvents _ARGS_((
			    Bool (*proc)(Display *display, XEvent *eventPtr,
				char *arg),
			    char *arg, char **prevArgPtr)))
			    _ARGS_((Display *display, XEvent *eventPtr,
				char *arg));
extern void		Tk_SetBackgroundFromBorder _ARGS_((Tk_Window tkwin,
			    Tk_3DBorder border));
extern void		Tk_SetClass _ARGS_((Tk_Window tkwin, char *class));
extern void		Tk_SetInternalBorder _ARGS_((Tk_Window tkwin,
			    int width));
extern void		Tk_SetWindowBackground _ARGS_((Tk_Window tkwin,
			    unsigned long pixel));
extern void		Tk_SetWindowBackgroundPixmap _ARGS_((Tk_Window tkwin,
			    Pixmap pixmap));
extern void		Tk_SetWindowBorder _ARGS_((Tk_Window tkwin,
			    unsigned long pixel));
extern void		Tk_SetWindowBorderWidth _ARGS_((Tk_Window tkwin,
			    int width));
extern void		Tk_SetWindowBorderPixmap _ARGS_((Tk_Window tkwin,
			    Pixmap pixmap));
extern void		Tk_ShareEvents _ARGS_((Tk_Window tkwin,
			    Tk_Uid groupId));
extern void		Tk_Sleep _ARGS_((int ms));
extern void		Tk_UnmapWindow _ARGS_((Tk_Window tkwin));
extern void		Tk_UnshareEvents _ARGS_((Tk_Window tkwin,
			    Tk_Uid groupId));


extern int		tk_NumMainWindows;

/*
 * Tcl commands exported by Tk:
 */

extern int		Tk_AfterCmd _ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char **argv));
extern int		Tk_ApplicationCmd _ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char **argv));
extern int		Tk_BindCmd _ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char **argv));
extern int		Tk_ButtonCmd _ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char **argv));
extern int		Tk_DestroyCmd _ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char **argv));
extern int		Tk_EntryCmd _ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char **argv));
extern int		Tk_FrameCmd _ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char **argv));
extern int		Tk_FocusCmd _ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char **argv));
extern int		Tk_ListboxCmd _ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char **argv));
extern int		Tk_MenuCmd _ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char **argv));
extern int		Tk_MenubuttonCmd _ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char **argv));
extern int		Tk_MessageCmd _ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char **argv));
extern int		Tk_OptionCmd _ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char **argv));
extern int		Tk_PackCmd _ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char **argv));
extern int		Tk_ScaleCmd _ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char **argv));
extern int		Tk_ScrollbarCmd _ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char **argv));
extern int		Tk_SelectionCmd _ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char **argv));
extern int		Tk_SendCmd _ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char **argv));
extern int		Tk_UpdateCmd _ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char **argv));
extern int		Tk_WinfoCmd _ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char **argv));
extern int		Tk_WmCmd _ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char **argv));

#endif _TK
