/* 
 * timerSun4c.c --
 *
 *	This file contains the routines that control the time of day clock 
 *	and interrupt timers on the sun4c (SparcStation 1) machine. The
 *	sun4c contains a Mostek MK48T12-15 chip for time of day and 
 *	two interrupt timers.  
 *
 *	Until we get the data sheets for the Mostek chip, we use the interrupt
 *	counters for both interupts and TOD.
 *
 * Copyright 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif /* not lint */

#include "sprite.h"
#include "sys.h"
#include "timer.h"
#include "timerInt.h"
#include "timerTick.h"
#include "spriteTime.h"
#include "mach.h"
#include "prof.h"
#include "devAddrs.h"

#include "timerMK48T12Int.h"

/*
 * The sun4c contains two counters that count up to value in the limit 
 * register and interrupt. The counters are defined by the following 
 * structure. (Campus-1 Programmer's Model Version 7.5 page 11).
 * We assign counter0 to be the call back interrupt timer and 
 * counter1 to be the profile interrupt timer. Each counter and limit
 * register is of the form:
 *	struct {
 *	   int	limitReached :1; -- 1 if counter reached limit. 
 *	   int  value	     :21; -- Value updated every microsecond. 
 *	   int  zeros	     :10; -- Read as zero. 
 *     }
 */

typedef struct {
    unsigned int callBackCounter; /* Call back counter */
    unsigned int callBackLimit;   /* Limit register for call back counter. */
    unsigned int profileCounter;  /* Profile counter. */
    unsigned int profileLimit;    /* Limit register for profile counter. */
} Counters;

/*
 * COUNTER_SHIFT	- Number of bits to shift microsecond value for
 *			  counter. 
 * COUNTER_LIMIT_REACH  - Bit signifying count has reached limit.
 */
#define	COUNTER_SHIFT 10
#define	COUNTER_LIMIT_REACH 0x80000000

static volatile Counters *counterPtr = (volatile Counters *) DEV_COUNTER_ADDR;

/*
 * The "free running counter"
 */
static Timer_Ticks todCounter;

void Timer_TimerServiceInterrupt();

/*
 * Timer interval expressed as an integer and as a Time. This is the
 * period between callbacks, so it is actually twice the timer period
 * since we callback every other time. 
 */

static int interval = TIMER_CALLBACK_INTERVAL_APPROX / 1000;
static Time time = { 0, TIMER_CALLBACK_INTERVAL_APPROX};


/* 
 *----------------------------------------------------------------------
 *
 * Timer_TimerInit --
 *
 *	Initialize the periodic timer. Since there is no way to turn off
 *	the counters will wait and do all the work in Timer_TimerStart().
 *
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The timer is initialized.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
void
Timer_TimerInit(timer)
    unsigned short 	timer;
{ 
     if (timer == TIMER_CALLBACK_TIMER) {
	 Mach_SetHandler(10, Timer_TimerServiceInterrupt, (ClientData) timer); 
     } else if (timer == TIMER_PROFILE_TIMER) {
	 Mach_SetHandler(14, Timer_TimerServiceInterrupt, (ClientData) timer); 
     } else {
	panic("Timer_TimerInit: unknown timer %d\n", timer);
     }
}


/*
 *----------------------------------------------------------------------
 *
 * Timer_TimerStart --
 *
 *	Loads the specified timer with a new value to count from
 *	and starts the timer.
 *
 *	N.B. The timer must have been initialized with Timer_TimerInit
 *	before this routine is called.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The timer starts ticking.
 *
 *----------------------------------------------------------------------
 */

void
Timer_TimerStart(timer)
    register unsigned short timer;
{
    unsigned int junk;

    /*
     * To start the timer we load the limit register with the correct 
     * count and clear any pending interrupt. 
     */
     if (timer == TIMER_CALLBACK_TIMER) {
	 counterPtr->callBackLimit =
		     ((TIMER_CALLBACK_INTERVAL_APPROX*1000) << COUNTER_SHIFT);
	 junk = counterPtr->callBackLimit;
	  DISABLE_INTR();
	 *Mach_InterruptReg |= MACH_ENABLE_COUNTER0_INTR_LEVEL;
	 ENABLE_INTR();

     } else if (timer == TIMER_PROFILE_TIMER) {
	 counterPtr->profileLimit =
		     ((TIMER_PROFILE_INTERVAL_APPROX*1000) << COUNTER_SHIFT);
	 junk = counterPtr->profileLimit;
         DISABLE_INTR();
	 *Mach_InterruptReg |= MACH_ENABLE_COUNTER1_INTR_LEVEL;
	 ENABLE_INTR();
     } else {
	panic("Timer_TimerInit: unknown timer %d\n", timer);
     }
}



/*
 *----------------------------------------------------------------------
 *
 * Timer_TimerInactivate --
 *
 *	Stops the timer so that it will not cause interrupts.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The timer is stopped.
 *
 *----------------------------------------------------------------------
 */

void
Timer_TimerInactivate(timer)
    register unsigned short timer;
{
    if (timer == TIMER_CALLBACK_TIMER) {
	/*
	 * As long as we use the callback timer to keep track of time
	 * we shouldn't turn it off.
	 */
#ifdef notdef
	 DISABLE_INTR();
	 *Mach_InterruptReg &= ~MACH_ENABLE_COUNTER0_INTR_LEVEL;
	 ENABLE_INTR();
#endif
    } else if (timer == TIMER_PROFILE_TIMER) {
	 DISABLE_INTR();
	 *Mach_InterruptReg &= ~MACH_ENABLE_COUNTER1_INTR_LEVEL;
	 ENABLE_INTR();
    } else {
	panic("Timer_TimerInactivate: unknown timer %d\n", timer);
    }

}

/*
 *----------------------------------------------------------------------
 *
 *  Timer_TimerServiceInterrupt --
 *
 *      This routine is called at every timer interrupt. 
 *      It calls the timer callback queue handling if the callback timer 
 *	expired and calls the profiling interrupt handling if the 
 *	profile callback timer expired.
 *
 *  Results:
 *	None.
 *
 *  Side Effects:
 *	Routines on the timer queue may cause side effects. Profile
 *	collect may take place. 
 *	
 *
 *----------------------------------------------------------------------
 */

void
Timer_TimerServiceInterrupt(clientData, pc)
    ClientData	clientData;	        /* Really Counter number */
    unsigned int	pc;		/* Only for sun4 version. */
{ 
    register unsigned int junk;

    if ((unsigned int) clientData == TIMER_PROFILE_TIMER) {
	/*
	 * Clear interrupt by reading limit register. 
	 */
	junk = counterPtr->profileLimit;
        TIMER_PROFILE_ROUTINE(pc);
#       ifdef GATHER_STAT
        timer_Statistics.profile++;
#       endif
    } else  if ((unsigned int) clientData == TIMER_CALLBACK_TIMER) {
	/*
	 * Clear interrupt by reading limit register. 
	 */
	junk = counterPtr->callBackLimit;
#ifndef TOD_WORKING
	/*
	 * Until we get the TOD chip working. Use the callback counter
	 * to keep track on time of day.
	 */
	 todCounter.microseconds += TIMER_CALLBACK_INTERVAL_APPROX*1000;
	 if (todCounter.microseconds > ONE_SECOND) {
	     todCounter.seconds++;
	     todCounter.microseconds -= ONE_SECOND;
	 }
#endif
        TIMER_CALLBACK_ROUTINE(interval, time);
    } else {
	panic("Timer_TimerServiceInterrupt: Unknown timer %d\n", 
		    (int) clientData);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Timer_CounterInit --
 *
 *	Initializes the chip's free-running counters. Until we get the
 *	TOD chip just use a counter updated by the CALL BACK interrupt
 *	handler.
 *
 *	
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
Timer_CounterInit()
{
    todCounter.seconds = 0;
    todCounter.microseconds = 0;
}

/* 
 *----------------------------------------------------------------------
 *
 *  Timer_GetCurrentTicks --
 *
 *  	Return microseconds since boot.
 *
 *  Results:
 *	The system up-time in ticks.
 *
 *  Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
void
Timer_GetCurrentTicks(ticksPtr)
    Timer_Ticks	*ticksPtr;	/* Buffer to place current time. */
{
    DISABLE_INTR();

    *ticksPtr = todCounter;

    ENABLE_INTR();
}

/* 
 *----------------------------------------------------------------------
 *
 *  Timer_GetInfo --
 *
 *  	Dummy routine to dump timer state.
 *
 *  Results:
 *	None.
 *
 *  Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
void
Timer_TimerGetInfo()
{
}

/*
 *----------------------------------------------------------------------
 *
 * TimerHardwareUniversalTimeInit --
 *
 * 	Not implemented yet.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
void
TimerHardwareUniversalTimeInit(timePtr, localOffsetPtr, DSTPtr)
    Time *timePtr;		/* Buffer to hold universal time. */
    int  *localOffsetPtr;	/* Buffer to hold local offset. */
    Boolean *DSTPtr;		/* Buffer to hold DST allowed flag. */
{
}

/*
 *----------------------------------------------------------------------
 *
 * TimerSetHardwareUniversalTime --
 *
 *	Not implemented yet.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
void
TimerSetHardwareUniversalTime(timePtr, localOffset, DST)
    Time *timePtr;		/* universal time. */
    int  localOffset;		/* local offset. */
    Boolean DST;		/* DST allowed flag. */
{
}

