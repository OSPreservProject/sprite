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

#include <dev/graphics.h>

extern Boolean	devGraphicsOpen;

extern void		DevGraphicsInit();
extern void		DevGraphicsInterrupt();
extern ReturnStatus	DevGraphicsOpen();
extern ReturnStatus	DevGraphicsClose();
extern ReturnStatus	DevGraphicsRead();
extern ReturnStatus	DevGraphicsWrite();
extern ReturnStatus	DevGraphicsSelect();
extern ReturnStatus	DevGraphicsIOControl();
extern void		DevGraphicsKbdIntr();
extern void		DevGraphicsMouseIntr();

#endif _DEVGRAPHICS
