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
 * rcsid:  $Header$ SPRITE (Berkeley) 
 */

#ifndef _TIMERINT
#define _TIMERINT

#include "spriteTime.h"
#include "sync.h"


extern Sync_Semaphore 	timerMutex;
extern int 		timerUniversalToLocalOffset;
extern Boolean		timerDSTAllowed;

extern void Timer_CallBack _ARGS_((unsigned int interval, Time time));
extern void TimerTicksInit _ARGS_((void));
extern void Timer_CounterInit _ARGS_((void));
extern void TimerClock_Init _ARGS_((void));
extern void TimerSetSoftwareUniversalTime _ARGS_((Time *newUniversal, 
		    int newLocalOffset, Boolean newDSTAllowed));
extern void TimerSetHardwareUniversalTime _ARGS_((Time *timePtr, 
		    int localOffset, Boolean DST));
extern void TimerHardwareUniversalTimeInit _ARGS_((Time *timePtr, 
		    int *localOffsetPtr, Boolean *DSTPtr));

#endif /* _TIMERINT */
