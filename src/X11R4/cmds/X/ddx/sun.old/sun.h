/*-
 * sun.h --
 *	Internal declarations for the sun ddx interface
 *
 * Copyright (c) 1987 by the Regents of the University of California
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 *	"$XConsortium: sun.h,v 5.7 89/12/06 09:37:35 rws Exp $ SPRITE (Berkeley)"
 */
#ifndef _SUN_H_
#define _SUN_H_

#include    <errno.h>
extern int  errno;
#include    <sys/param.h>
#include    <sys/types.h>
#include    <sys/time.h>
#include    <sys/file.h>
#include    <sys/fcntl.h>
#include    <sys/signal.h>
#include    <sundev/kbd.h>
#include    <sundev/kbio.h>
#include    <sundev/msio.h>
#include    <sun/fbio.h>

/*
 * SUN_WINDOWS is now defined (or not) by the Makefile
 * variable $(SUNWINDOWSFLAGS) in server/Makefile.
 */

#ifdef SUN_WINDOWS
#include    <varargs.h>
#include    <sys/ioctl.h>
#include    <stdio.h>
#include    <pixrect/pixrect_hs.h>
#include    <sunwindow/rect.h>
#include    <sunwindow/rectlist.h>
#include    <sunwindow/pixwin.h>
#include    <sunwindow/win_screen.h>
#include    <sunwindow/win_input.h>
#include    <sunwindow/cms.h>
#include    <sunwindow/win_struct.h>
#else 
/* already included by sunwindow/win_input.h */
#include    <sundev/vuid_event.h>
#endif SUN_WINDOWS

#include    "X.h"
#include    "Xproto.h"
#include    "scrnintstr.h"
#include    "screenint.h"
#ifdef NEED_EVENTS
#include    "inputstr.h"
#endif NEED_EVENTS
#include    "input.h"
#include    "cursorstr.h"
#include    "cursor.h"
#include    "pixmapstr.h"
#include    "pixmap.h"
#include    "windowstr.h"
#include    "gc.h"
#include    "gcstruct.h"
#include    "regionstr.h"
#include    "colormap.h"
#include    "miscstruct.h"
#include    "dix.h"
#include    "mfb.h"
#include    "mi.h"

/*
 * MAXEVENTS is the maximum number of events the mouse and keyboard functions
 * will read on a given call to their GetEvents vectors.
 */
#define MAXEVENTS 	32

/*
 * Data private to any sun keyboard.
 *	GetEvents reads any events which are available for the keyboard
 *	ProcessEvent processes a single event and gives it to DIX
 *	DoneEvents is called when done handling a string of keyboard
 *	    events or done handling all events.
 *	devPrivate is private to the specific keyboard.
 *	map_q is TRUE if the event queue for the keyboard is memory mapped.
 */
typedef struct kbPrivate {
    int	    	  type;           	/* Type of keyboard */
    int	    	  fd;	    	    	/* Descriptor open to device */
    Firm_event	  *(*GetEvents)();  	/* Function to read events */
    void    	  (*ProcessEvent)();	/* Function to process an event */
    void    	  (*DoneEvents)();  	/* Function called when all events */
					/* have been handled. */
    pointer 	  devPrivate;	    	/* Private to keyboard device */
    Bool	  map_q;		/* TRUE if fd has a mapped event queue */
    int		  offset;		/* to be added to device keycodes */
    KeybdCtrl	  *ctrl;    	    	/* Current control structure (for
 					 * keyclick, bell duration, auto-
 					 * repeat, etc.) */
} KbPrivRec, *KbPrivPtr;

#define	MIN_KEYCODE	8	/* necessary to avoid the mouse buttons */
#ifndef KB_SUN4
#define KB_SUN4		0x04	/* Type 4 Sun keyboard */
#endif

/*
 * Data private to any sun pointer device.
 *	GetEvents, ProcessEvent and DoneEvents have uses similar to the
 *	    keyboard fields of the same name.
 *	pScreen is the screen the pointer is on (only valid if it is the
 *	    main pointer device).
 *	dx and dy are relative coordinates on that screen (they may be negative)
 */
typedef struct ptrPrivate {
    int	    	  fd;	    	    	/* Descriptor to device */
    Firm_event 	  *(*GetEvents)(); 	/* Function to read events */
    void    	  (*ProcessEvent)();	/* Function to process an event */
    void    	  (*DoneEvents)();  	/* When all the events have been */
					/* handled, this function will be */
					/* called. */
    short   	  dx,	    	    	/* Current X coordinate of pointer */
		  dy;	    	    	/* Current Y coordinate */
    pointer    	  devPrivate;	    	/* Field private to device */
} PtrPrivRec, *PtrPrivPtr;

/*
 * Cursor-private data
 *	screenBits	saves the contents of the screen before the cursor
 *	    	  	was placed in the frame buffer.
 *	source	  	a bitmap for placing the foreground pixels down
 *	srcGC	  	a GC for placing the foreground pixels down.
 *	    	  	Prevalidated for the cursor's screen.
 *	invSource 	a bitmap for placing the background pixels down.
 *	invSrcGC  	a GC for placing the background pixels down.
 *	    	  	Also prevalidated for the cursor's screen Pixmap.
 *	temp	  	a temporary pixmap for low-flicker cursor motion --
 *	    	  	exists to avoid the overhead of creating a pixmap
 *	    	  	whenever the cursor must be moved.
 *	fg, bg	  	foreground and background pixels. For a color display,
 *	    	  	these are allocated once and the rgb values changed
 *	    	  	when the cursor is recolored.
 *	scrX, scrY	the coordinate on the screen of the upper-left corner
 *	    	  	of screenBits.
 *	state	  	one of CR_IN, CR_OUT and CR_XING to track whether the
 *	    	  	cursor is in or out of the frame buffer or is in the
 *	    	  	process of going from one state to the other.
 */
typedef enum {
    CR_IN,		/* Cursor in frame buffer */
    CR_OUT,		/* Cursor out of frame buffer */
    CR_XING	  	/* Cursor in flux */
} CrState;

typedef struct crPrivate {
    PixmapPtr  	        screenBits; /* Screen before cursor put down */
    PixmapPtr  	        source;     /* Cursor source (foreground bits) */
    GCPtr   	  	srcGC;	    /* Foreground GC */
    PixmapPtr  	        invSource;  /* Cursor source inverted (background) */
    GCPtr   	  	invSrcGC;   /* Background GC */
    PixmapPtr  	        temp;	    /* Temporary pixmap for merging screenBits
				     * and the sources. Saves creation time */
    Pixel   	  	fg; 	    /* Foreground color */
    Pixel   	  	bg; 	    /* Background color */
    int	    	  	scrX,	    /* Screen X coordinate of screenBits */
			scrY;	    /* Screen Y coordinate of screenBits */
    CrState		state;      /* Current state of the cursor */
} CrPrivRec, *CrPrivPtr;

/*
 * Frame-buffer-private info.
 *	fd  	  	file opened to the frame buffer device.
 *	info	  	description of the frame buffer -- type, height, depth,
 *	    	  	width, etc.
 *	fb  	  	pointer to the mapped image of the frame buffer. Used
 *	    	  	by the driving routines for the specific frame buffer
 *	    	  	type.
 *	mapped	  	flag set true by the driver when the frame buffer has
 *	    	  	been mapped in.
 *	parent	  	set true if the frame buffer is actually a SunWindows
 *	    	  	window.
 *	fbPriv	  	Data private to the frame buffer type.
 */
typedef struct {
    pointer 	  	fb; 	    /* Frame buffer itself */
    Bool    	  	mapped;	    /* TRUE if frame buffer already mapped */
    Bool		parent;	    /* TRUE if fd is a SunWindows window */
    int	    	  	fd; 	    /* Descriptor open to frame buffer */
    struct fbtype 	info;	    /* Frame buffer characteristics */
    void		(*EnterLeave)();    /* screen switch */
    pointer 	  	fbPriv;	    /* Frame-buffer-dependent data */
} fbFd;

extern Bool sunSupportsDepth8;
extern unsigned long sunGeneration;

typedef struct _sunFbDataRec {
    Bool    (*probeProc)();	/* probe procedure for this fb */
    char    *devName;		/* device filename */
    Bool    (*createProc)();	/* create procedure for this fb */
} sunFbDataRec;

extern sunFbDataRec sunFbData[];
/*
 * Cursor functions
 */
extern void 	  sunInitCursor();
#ifdef SUN_WINDOWS
extern Bool	  sunSetCursorPosition();
extern Bool	  (*realSetCursorPosition)();
#endif

/*
 * Initialization
 */
extern Bool 	  sunScreenInit();
extern int  	  sunOpenFrameBuffer();

extern fbFd 	  sunFbs[];

extern int  	  lastEventTime;    /* Time (in ms.) of last event */
extern void 	  SetTimeSinceLastInputEvent();

extern int monitorResolution;

#define AUTOREPEAT_INITIATE	(200)		/* milliseconds */
#define AUTOREPEAT_DELAY	(50)		/* milliseconds */
/*
 * We signal autorepeat events with the unique Firm_event
 * id AUTOREPEAT_EVENTID.
 * Because inputevent ie_code is set to Firm_event ids in
 * sunKbdProcessEventSunWin, and ie_code is short whereas
 * Firm_event id is u_short, we use 0x7fff.
 */
#define AUTOREPEAT_EVENTID      (0x7fff)        /* AutoRepeat Firm_event id */

extern int	autoRepeatKeyDown;		/* TRUE if key down */
extern int	autoRepeatReady;		/* TRUE if time out */
extern int	autoRepeatDebug;		/* TRUE if debugging */
extern long	autoRepeatInitiate;
extern long 	autoRepeatDelay;
extern struct timeval autoRepeatLastKeyDownTv;
extern struct timeval autoRepeatDeltaTv;

#define tvminus(tv, tv1, tv2)	/* tv = tv1 - tv2 */ \
		if ((tv1).tv_usec < (tv2).tv_usec) { \
			(tv1).tv_usec += 1000000; \
			(tv1).tv_sec -= 1; \
		} \
		(tv).tv_usec = (tv1).tv_usec - (tv2).tv_usec; \
		(tv).tv_sec = (tv1).tv_sec - (tv2).tv_sec;

#define tvplus(tv, tv1, tv2)	/* tv = tv1 + tv2 */ \
		(tv).tv_sec = (tv1).tv_sec + (tv2).tv_sec; \
		(tv).tv_usec = (tv1).tv_usec + (tv2).tv_usec; \
		if ((tv).tv_usec > 1000000) { \
			(tv).tv_usec -= 1000000; \
			(tv).tv_sec += 1; \
		}

/*-
 * TVTOMILLI(tv)
 *	Given a struct timeval, convert its time into milliseconds...
 */
#define TVTOMILLI(tv)	(((tv).tv_usec/1000)+((tv).tv_sec*1000))

#ifdef SUN_WINDOWS
extern int windowFd;
#endif SUN_WINDOWS

#endif _SUN_H_
