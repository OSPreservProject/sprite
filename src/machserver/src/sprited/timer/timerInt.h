/*
 * timerInt.h --
 *
 *     Internal definitions for the clock timer routines.
 *
 * Copyright 1986, 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 *
 * rcsid:  $Header: /r3/kupfer/spriteserver/src/sprited/timer/RCS/timerInt.h,v 1.3 91/11/14 10:03:12 kupfer Exp $ SPRITE (Berkeley) 
 */

#ifndef _TIMERINT
#define _TIMERINT

#include <sprite.h>
#include <spriteTime.h>

extern int 		timerUniversalToLocalOffset;
extern Boolean		timerDSTAllowed;

extern void TimerClock_Init _ARGS_((void));

#endif /* _TIMERINT */
