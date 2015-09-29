/*
 * mouse.h --
 *
 *	Information about "mouse" devices, which may be read to provide
 *	keystroke and mouse information for use by window systems.
 *
 * Copyright 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /sprite/src/lib/include/dev/RCS/mouse.h,v 1.3 89/06/25 17:06:46 ouster Exp $ SPRITE (Berkeley)
 */

#ifndef _MOUSE
#define _MOUSE

/*
 * The structure of a keyboard or mouse event:  this is what is
 * returned by the read system call.
 */

typedef struct {
    int flags;			/* Miscellaneous flags;  see below. */
    int key;			/* For keyboard, identifies key that went
				 * up or down;  for mouse, identifies state
				 * of buttons. */
    int	deltaX;			/* X-motion of mouse (mouse only). */
    int deltaY;			/* Y-motion of mouse (mouse only). */
    unsigned int time;		/* Time stamp in millisecond units. */
} Mouse_Event;

/*
 * Flag values:
 *
 * MOUSE_EVENT:			1 means this is a mouse event.
 * KEYBOARD_EVENT:		1 means this is a keyboard event.
 * KEY_UP:			1 means key went up, 0 means it went down
 *				(valid only for keyboard events).
 */

#define MOUSE_EVENT	1
#define KEYBOARD_EVENT	2
#define KEY_UP		4

/*
 * Values to write to the mouse to invoke special keyboard functions
 * (valid for Sun keyboards only):
 */

#define KBD_RESET	1
#define KBD_BELL_ON	2
#define KBD_BELL_OFF	3
#define KBD_CLICK_ON	10
#define KBD_CLICK_OFF	11

#endif /* _MOUSE */
