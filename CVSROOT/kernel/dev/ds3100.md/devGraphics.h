/* 
 *  devGraphics.h --
 *
 *     	Defines of procedures and variables used by other files.
 *
 *	Copyright (C) 1989 Digital Equipment Corporation.
 *	Permission to use, copy, modify, and distribute this software and
 *	its documentation for any purpose and without fee is hereby granted,
 *	provided that the above copyright notice appears in all copies.  
 *	Digital Equipment Corporation makes no representations about the
 *	suitability of this software for any purpose.  It is provided "as is"
 *	without express or implied warranty.
 *
 * $Header$ SPRITE (DECWRL)
 */

#ifndef _DEVGRAPHICS
#define _DEVGRAPHICS

/*
 * Cursor command register bits.
 *
 *     DEV_CURSOR_ENPA	Enable cursor plane A.
 *     DEV_CURSOR_FOPA	Force cursor plane A to output 1.
 *     DEV_CURSOR_ENPB	Enable cursor plane B.
 *     DEV_CURSOR_FOPB	Force cursor plane B to output 1.
 *     DEV_CURSOR_XHAIR	
 *     DEV_CURSOR_XHCLP	Clip crosshair inside region.	
 *     DEV_CURSOR_XHCL1	Select crosshair clipping region.
 *     DEV_CURSOR_XHWID	Crosshair cursor line width.
 *     DEV_CURSOR_ENRG1	Enable region detector 1.
 *     DEV_CURSOR_FORG1	Force region detector 1 to output 1.
 *     DEV_CURSOR_ENRG2	Enable region detector 2.
 *     DEV_CURSOR_FORG2	Force region detector 2 to output 1.
 *     DEV_CURSOR_LODSA	Load/display sprite array.
 *     DEV_CURSOR_VBHI	Vertical blank polarity.
 *     DEV_CURSOR_HSHI	Horizontal sync polarity.
 *     DEV_CURSOR_TEST	Diagnostic test.
 */
#define DEV_CURSOR_ENPA		0x0001
#define DEV_CURSOR_FOPA		0x0002
#define DEV_CURSOR_ENPB		0x0004
#define DEV_CURSOR_FOPB		0x0008
#define DEV_CURSOR_XHAIR	0x0010
#define DEV_CURSOR_XHCLP	0x0020
#define DEV_CURSOR_XHCL1	0x0040
#define DEV_CURSOR_XHWID	0x0080
#define DEV_CURSOR_ENRG1	0x0100
#define DEV_CURSOR_FORG1	0x0200
#define DEV_CURSOR_ENRG2	0x0400
#define DEV_CURSOR_FORG2	0x0800
#define DEV_CURSOR_LODSA	0x1000
#define DEV_CURSOR_VBHI		0x2000
#define DEV_CURSOR_HSHI		0x4000
#define DEV_CURSOR_TEST		0x8000

/*
 * The cursor register.
 */
typedef volatile struct {
    unsigned short	cmdReg;	/* Command register. */
    unsigned short	pad1;
    unsigned short	xPos;	/* X position. */
    unsigned short	pad2;
    unsigned short	yPos;	/* Y position. */
    unsigned short	pad3;
    unsigned short	xMin1;
    unsigned short	pad4;
    unsigned short	xMax1;
    unsigned short	pad5;
    unsigned short	yMin1;
    unsigned short	pad6;
    unsigned short	yMax1;
    unsigned short	pad7;
    unsigned short	unused7;
    unsigned short	padUnused7;
    unsigned short	unused8;
    unsigned short	padUnused8;
    unsigned short	unused9;
    unsigned short	padUnused9;
    unsigned short	unusedA;
    unsigned short	padUnusedA;
    unsigned short	xMin2;
    unsigned short	pad8;
    unsigned short	xMax2;
    unsigned short	pad9;
    unsigned short	yMin2;
    unsigned short	padA;
    unsigned short	yMax2;
    unsigned short	padB;
    unsigned short	memory;
    unsigned short	padC;
} DevPCCRegs;

/*
 * The VDAC register.
 */
typedef volatile struct {
    unsigned char   mapWA;
    unsigned char   pad0;
    unsigned short  pad1;

    unsigned char   map;
    unsigned char   pad2;
    unsigned short  pad3;

    unsigned char   mask;
    unsigned char   pad4;
    unsigned short  pad5;

    unsigned char   mapRA;
    unsigned char   pad6;
    unsigned short  pad7;

    unsigned char   overWA;
    unsigned char   pad8;
    unsigned short  pad9;

    unsigned char   over;
    unsigned char   pad10;
    unsigned short  pad11;

    unsigned char   reserved;
    unsigned char   pad12;
    unsigned short  pad13;

    unsigned char   overRA;
    unsigned char   pad14;
    unsigned short  pad15;
} DevVDACRegs;

/*
 *  Events.
 */
typedef struct {
        short	        x;		/* x position */
        short 	        y;		/* y position */
        unsigned int    time;		/* 1 millisecond units */
        unsigned char   type;		/* button up/down/raw or motion */
        unsigned char   key;		/* the key (button only) */
        unsigned char   index;		/* which instance of device */
        unsigned char   device;		/* which device */
} DevEvent;

/*
 * type field
 */
#define DEV_BUTTON_UP_TYPE          0
#define DEV_BUTTON_DOWN_TYPE        1
#define DEV_BUTTON_RAW_TYPE         2
#define DEV_MOTION_TYPE             3
/*
 * Key field.
 */
#define DEV_EVENT_LEFT_BUTTON	0x01
#define DEV_EVENT_MIDDLE_BUTTON	0x02
#define DEV_EVENT_RIGHT_BUTTON	0x03
/*
 * device field
 */
#define DEV_NULL_DEVICE	  	0	/* NULL event (for QD_GETEVENT ret) */
#define DEV_MOUSE_DEVICE	1		/* mouse */
#define DEV_KEYBOARD_DEVICE	2		/* main keyboard */
#define DEV_TABLET_DEVICE	3		/* graphics tablet */
#define DEV_AUX_DEVICE	  	4		/* auxiliary */
#define DEV_CONSOLE_DEVICE	5		/* console */
#define DEV_KNOB_DEVICE	  	8
#define DEV_JOYSTICK_DEVICE	9

#define DEV_MAXEVQ	64	/* must be power of 2 */
#define DEV_EVROUND(x)	((x) & (DEV_MAXEVQ - 1))
#define DEV_TABLET_RES	2

typedef struct {
	unsigned int	time;
	short		x, y;
} DevTimeCoord;

/*
 * The event queue. This structure is normally included in the info
 * returned by the device driver.
 */
typedef struct {
	DevEvent	*events;
	unsigned int 	eSize;
        unsigned int    eHead;
        unsigned int    eTail;
	unsigned long	timestampMS;
	DevTimeCoord	*tcs;	/* history of pointer motions */
	unsigned int	tcSize;
	unsigned int	tcNext;	/* simple ring buffer, old events are tossed */
} DevEventQueue;

/* 
 * mouse cursor position
 */
typedef struct {
        short x;
        short y;
} DevCursor;

/* 
 * mouse motion rectangle
 */
typedef struct {
        short bottom;
        short right;
        short left;
        short top;
} DevBox;

/*
 * Structures used by iocontrols.
 */
typedef struct {
	char nbytes;		/* Number of bytes in parameter */
	unsigned char cmd;	/* Command to be sent, peripheral bit will */
				/* be forced by driver */
	unsigned char par[2];	/* Bytes of parameters to be sent */
} DevKpCmd;

/*
 * Information about the screen.
 */
typedef struct {
	DevEventQueue eventQueue;	/* event & motion queues	*/
	short	mswitches;		/* current value of mouse buttons */
	DevCursor tablet;		/* current tablet position	*/
	short	tswitches;		/* current tablet buttons NI!	*/
	DevCursor cursor;		/* current cursor position	*/
	short	row;			/* screen row			*/
	short	col;			/* screen col			*/
	short	maxRow;			/* max character row		*/
	short	maxCol;			/* max character col		*/
	short	maxX;			/* max x position		*/
	short	maxY;			/* max y position		*/
	short	maxCurX;		/* max cursor x position 	*/
	short	maxCurY;		/* max cursor y position	*/
	int	version;		/* version of driver		*/
	char	*bitmap;		/* bit map position		*/
        short   *scanmap;               /* scanline map position        */
	short	*cursorBits;		/* cursor bit position		*/
	short	*vaddr;			/* virtual address           	*/
	char    *planeMask;		/* plane mask virtual location  */
	DevCursor mouse;		/* atomic read/write		*/
	DevBox	mbox;			/* atomic read/write		*/
	short	mthreshold;		/* mouse motion parameter	*/
	short	mscale;			/* mouse scale factor (if 
					   negative, then do square).	*/
	short	minCurX;		/* min cursor x position	*/
	short	minCurY;		/* min cursor y position	*/
} DevScreenInfo;

typedef struct {
	short		map;
	unsigned short	index;
	struct {
		unsigned short red;
		unsigned short green;
		unsigned short blue;
	} entry;
} DevColorMap;

/*
 * The unit number of /dev/mouse.
 */
#define DEV_MOUSE_UNIT	1

#ifdef KERNEL

extern Boolean	devGraphicsOpen;

extern void		DevGraphicsInit();
extern void		DevGraphicsInterrupt();
extern ReturnStatus	DevGraphicsOpen();
extern ReturnStatus	DevGraphicsClose();
extern ReturnStatus	DevGraphicsRead();
extern ReturnStatus	DevGraphicsWrite();
extern ReturnStatus	DevGraphicsSelect();
extern ReturnStatus	DevGraphicsIOControl();

#endif

#endif _DEVGRAPHICS
