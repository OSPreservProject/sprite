/*
 * timerInt.h --
 *
 *     Internal definitions for the clock timer routines.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 *
 *
 * rcsid:  $Header$ SPRITE (Berkeley) 
 */

#ifndef _TIMERINT
#define _TIMERINT

#include "time.h"

extern int 	timerMutex;
extern Time 	timerTimeOfDay;

extern void 	TimerTicksInit();

#endif _TIMERINT
