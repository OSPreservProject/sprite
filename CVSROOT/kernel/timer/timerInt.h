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

extern void		Timer_CallBack();
extern void 		TimerTicksInit();
extern void 		Timer_CounterInit();
extern void		TimerClock_Init();
extern void		TimerSetSoftwareUniversalTime();
extern void		TimerSetHardwareUniversalTime();
extern void		TimerHardwareUniversalTimeInit();

#endif /* _TIMERINT */
