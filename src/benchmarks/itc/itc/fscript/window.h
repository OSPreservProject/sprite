/* Header file for the window manager */

#ifndef FONTMAGIC
#include "font.h"
#endif

#ifdef DEBUG
#define debug(f) (fileno(stdout)>0 ? (printf f,fflush(stdout)) : 0)
#else
#define debug(f)
#endif

enum type {
    WindowType,
    SplitWindowType
};

/* A ViewPort defines a rectangle within which operations are clipped and
   translated.  It is a patch of the screen whose top left-hand corner is at
   (top,left) and is dimensioned width by height.  Operations within a
   viewport see a coordinate system which runs from (0,0) in the upper left
   to (width,height) in the lower right. */
struct ViewPort {
    int     top,
            left,
            width,
            height;
};

/* True iff (x,y) is within the viewport */
#define PointInView(x,y,v) ((x)>=(v)->left && (y)>=(v)->top \
	&& (x)<(v)->left+(v)->width && (y)<(v)->top+(v)->height)

struct ViewPort *CurrentViewPort;/* The current view port, used by all the
				    rasterop, character, and vector routines
				    */

struct raster {			/* general definition of a bit raster */
	short *bits;
	short height, width;
};

int     CursorX,
        CursorY,
	CursorVisible;

#define CursorDown() (CursorVisible ? CURSORDOWN() : 0)

struct TypedObject {
    enum type type:5;
    int TotallyChanged:1;
    int Changed:1;
    int deleted:1;		/* True iff this window is (being)
				   deleted */
    struct ViewPort ViewPort;	/* Region currently occupied by this
				   object */
    struct SplitWindow *parent;	/* The window containing this window */
    struct display *display;	/* The display on which this window resides */
};

struct Window {
    struct TypedObject  t;
    struct ViewPort SubViewPort;/* Viewport with borders clipped out and
				   with users clipping rectangle applied */
    struct ViewPort BasicSubViewPort;/* Viewport with borders clipped out */
    struct ViewPort ModeViewPort;/* Viewport for the mode line */
    struct icon *Cursor;	/* Cursor associated with this window */
    int     x,			/* Current cursor position */
            y;
    int     ClearFrom;
    int     stx;
    struct font *CurrentFont;	/* Font in which output is being done */
    int     pgrp;		/* Process group of the other end of the
				   channel */
    char    MousePrefixString[4];	/* String prefixed to all mouse
					   positioning reports */
    short   minheight,
            maxheight,
            minwidth,
            maxwidth;
    short   SpaceShim,		/* Number of padding pixels to add after each char */
	    CharShim;
    char    buf[200];
    char    args[200];		/* arguments to the current RPC call */
    char    ArgsExpected;	/* Number of argument bytes expect */
    char    KnowsAboutChange:1;	/* true iff the process knows about the most
				   recent change in the window size and
				   placement */
    char    HasStringArgument:1;/* true iff this RPC call also takes a string
				   argument */
    char    RawInput:1;		/* true iff keyboard input to this window is
				   to be transmitted untouched to the
				   subprocess */
    char    InputDisabled:1;	/* true iff the user isn't allowed to type to
				   this window */
    char    Hidden:1;		/* true iff this process is hidden -- it
				   doesn't have an attached window and shouldn't have one */
    char    Visible:1;		/* true iff this process is visible */
    char    DoNewlines:1;	/* true iff newlines should scroll the window */
    char    IHandleAquisition:1;/* true iff the process that controls this
				   window has taken responsibility for input
				   focus acquisition */
    char    AcquireFocusOnExpose:1;	/* The next time that this window is
					   exposed, it will be given the
					   input focus */
    char    ThisArg;
    char    SubProcess;		/* File ID of the process associated with the
				   window */
    short   OldMouseButtons;
    short   OldMouseX;
    short   OldMouseY;
    short   MouseMotionGranularity;
    int     func;		/* RasterOp function */
    int     MouseInterest;	/* Interesting mouse event mask */
    int     len;
    int     dot;
    char    name[40];		/* The name associated with this process */
    short   NameWidth;		/* String width of the name */
    char    ProgramName[20];	/* The name of the program -- name[] should
				   usually name the object being worked on by
				   ProgramName[] */
    short   ProgramNameWidth;
    char    MenuTitle[40];	/* String for use in the "Expose" menu */
    struct  menu *menu;
    char    hostname[20];	/* The name of the host on the other end of
				   this connection */
    short   RegionsAllocated;	/* The number of region structures allocated
				   to this window */
    short   MaxRegion;		/* The higest ID of a region that is in use */
    struct  WindowRegion *regions;	/* The regions (subwindows)
					   associated with this window */
    short   CurrentRegion;	/* The currently selected region */
    int     KeyState;		/* The accumulated prefix keys.  Used to
				   handle keymaps */
    char    MenuPrefix[4];	/* The string to be prefixed to all mouse
				   inputs.  This is to be used by programs
				   that need to know whether or not input
				   came from the keyboard or the mouse.
				   Normally, none should.  An acceptable
				   example of its use is when xyzzy does
				   statistical sampling it helps to know
				   whether the person used the mouse or the
				   keyboard */
};

struct Window WindowChannel[32];	/* One for each possible file ID */

struct SplitWindow {
    struct TypedObject t;
    struct SplitWindow *top, *bottom, *parent;
    int SplitValue;
    int SplitHorizontally:1;
};

struct SplitWindow *RootWindow[10];	/* One for each display */

struct WindowRegion {		/* A subwindow */
    struct ViewPort region;
    char	   *name;	/* Optional name */
    int             area;
    int		   linked;	/* Region # to get menu, cursor from */
    struct menu    *menu;
    struct icon    *Cursor;
};

#define new(t) ((t *) malloc(sizeof(t)))

struct Window *CurrentWindow;
struct Window *CursorWindow;
#define ScreenWidth CurrentViewPort->width
#define ScreenHeight CurrentViewPort->height
struct font *ModeFont;
int  MouseX;
int  MouseY;

int ValidDescriptors;
int HighestDescriptor;
int errno;
#define MinWindowWidth 40
#define MinWindowHeight 50
char ProgramName[];
char *getprofile();

struct FontSet {
    struct font *this;
    struct FontName fn;
    char *directory;
} *fonts;
int nFonts, FontASize;
struct font *bodyfont;
struct font *shapefont;
struct icon *gray;
int InputFocusFollowsMouse;
struct Window *WindowInFocus;
struct Window *EmphasizedWindow;
int AcquireFocusOnCreate;
int AcquireFocusOnExpose;
int FocusFollowsCursor;
int zoomX, zoomY, zoomW, zoomH;	/* for window creation zooming animation */

extern int shutdown;		/* True when exiting the window manager */
extern struct display *SnapShotDisplay;	/* points to display that should be dumped */
					/* when the next character is typed */
