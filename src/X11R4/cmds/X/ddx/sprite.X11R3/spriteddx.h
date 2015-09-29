/*-
 * sprite.h --
 *	Internal declarations for the sprite ddx interface
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
 *	"$Header: /mic/X11R3/src/cmds/Xsp/ddx/sprite/RCS/spriteddx.h,v 1.7 89/11/18 20:57:32 tve Exp $ SPRITE (Berkeley)"
 */
#ifndef _SPRITEDDX_H_
#define _SPRITEDDX_H_

#define Time	  SpriteTime

#include    <sprite.h>
#include    <kernel/devVid.h>
#include    <dev/mouse.h>
#include    <vm.h>

#undef Time

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
#include    "gc.h"
#include    "gcstruct.h"
#include    "region.h"
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
 * Data private to any sprite keyboard.
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
    Mouse_Event   *(*GetEvents)();  	/* Function to read events */
    void    	  (*ProcessEvent)();	/* Function to process an event */
    void    	  (*DoneEvents)();  	/* Function called when all events */
					/* have been handled. */
    int	    	  offset;   	    	/* Offset for keyboard codes */
    KeybdCtrl	  *ctrl;    	    	/* Current control info */
    pointer 	  devPrivate;	    	/* Private to keyboard device */
} KbPrivRec, *KbPrivPtr;

/*
 * Data private to any sprite pointer device.
 *	GetEvents, ProcessEvent and DoneEvents have uses similar to the
 *	    keyboard fields of the same name.
 *	pScreen is the screen the pointer is on (only valid if it is the
 *	    main pointer device).
 *	x and y are absolute coordinates on that screen (they may be negative)
 */
typedef struct ptrPrivate {
    int	    	  fd;	    	    	/* Descriptor to device */
    Mouse_Event   *(*GetEvents)(); 	/* Function to read events */
    void    	  (*ProcessEvent)();	/* Function to process an event */
    void    	  (*DoneEvents)();  	/* When all the events have been */
					/* handled, this function will be */
					/* called. */
    short   	  x,	    	    	/* Current X coordinate of pointer */
		  y;	    	    	/* Current Y coordinate */
    ScreenPtr	  pScreen;  	    	/* Screen pointer is on */
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
 *	fb  	  	pointer to the mapped image of the frame buffer. Used
 *	    	  	by the driving routines for the specific frame buffer
 *	    	  	type.
 *	cmap		pointer to the mapped colormap of tha frame buffer.
 *			Used by the driving routines for the specific frame
 *	    	  	buffer type.
 *	pGC 	  	A graphics context to be used by the cursor functions
 *	    	  	when drawing the cursor on the screen.
 *	GetImage  	Original GetImage function for this screen.
 *	CreateGC  	Original CreateGC function
 *	CreateWindow	Original CreateWindow function
 *	ChangeWindowAttributes	Original function
 *	GetSpans  	GC function which needs to be here b/c GetSpans isn't
 *	    	  	called with the GC as an argument...
 *	mapped	  	flag set true by the driver when the frame buffer has
 *	    	  	been mapped in.
 */
typedef struct {
    pointer 	  	fb; 	    /* Frame buffer itself */
    pointer 	  	cmap; 	    /* Color map */
    GCPtr   	  	pGC;	    /* GC for cursor operations */

    void    	  	(*GetImage)();
    Bool	      	(*CreateGC)();/* GC Creation function previously in the
				       * Screen structure */
    Bool	      	(*CreateWindow)();
    Bool		(*ChangeWindowAttributes)();
    unsigned int  	*(*GetSpans)(); /* XXX: Shouldn't need this */
    
    Bool    	  	mapped;	    /* TRUE if frame buffer already mapped */
} fbFd;

/*
 * Cursor functions in spriteCursor.c
 */
extern void 	  spriteInitCursor();
extern Bool 	  spriteRealizeCursor();
extern Bool 	  spriteUnrealizeCursor();
extern Bool 	  spriteDisplayCursor();
extern Bool 	  spriteSetCursorPosition();
extern void 	  spriteCursorLimits();
extern void 	  spritePointerNonInterestBox();
extern void 	  spriteConstrainCursor();
extern void 	  spriteRecolorCursor();
extern Bool	  spriteCursorLoc();
extern void 	  spriteRemoveCursor();
extern void	  spriteRestoreCursor();
extern void 	  spriteMoveCursor();

/*
 * Initialization
 */
extern void 	  spriteScreenInit();

/*
 * GC Interceptions in spriteGC.c and spriteCursor.c
 */
extern GCPtr	  spriteCreatePrivGC();
extern Bool	  spriteCreateGC();
extern Bool	  spriteCreateWindow();
extern Bool	  spriteChangeWindowAttributes();

extern void 	  spriteGetImage();
extern unsigned int *spriteGetSpans();

extern fbFd 	  spriteFbs[];

extern unsigned int lastEventTime;    /* Time (in real ms.) of last event */
extern unsigned int lastEventTimeMS;  /* Time (in ms.) of last event */
extern void 	  SetTimeSinceLastInputEvent();

extern Bool 	  screenSaved;	    /* TRUE if any screen is saved */

extern int  	  spriteCheckInput;
extern void 	  spriteInputAvail();
extern void 	  spriteCursorGone();
extern void 	  spriteBlockHandler();
extern void 	  spriteWakeupHandler();
#endif _SPRITEDDX_H_
