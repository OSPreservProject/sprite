/*
 * timerMC.c --
 *
 *	This file contains routines that manipulate the MC 146818 real-time
 *	clock.
 *
 *	For a detailed explanation of the chip, see the "PMAX Desktop
 *	Workstation Functional Specification, Revision 1.1" pages 62-66.
 *
 * Copyright (C) 1989 Digital Equipment Corporation.
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appears in all copies.  
 * Digital Equipment Corporation makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 */

/*
 * This chip has a real-time clock and an interval timer.  The real-time
 * clock is basically useless because it only has a resolution of seconds.
 * The machine-independent part assumes that we have a free-running counter
 * which has a resolution of milliseconds.  What we end up doing is
 * faking the free running counter by adding in the appropriate number
 * of milliseconds to a counter whenever the interval timer goes off.  This
 * isn't too accurate but what can we do?  One improvement might be to
 * set the real time clock and correct our notion of time in millseconds
 * every second.  This will be necessary if we miss interrupts.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "sys.h"
#include "timerInt.h"
#include "timerTick.h"
#include "spriteTime.h"
#include "mach.h"
#include "prof.h"
#include "timer.h"
#include "machAddrs.h"

/*
 * Control register A.
 */
#define REGA_UIP		0x80
#define REGA_TIME_DIV		0x70
#define REGA_RATE_SELECT	0x0F

/*
 * Time base to use in the REGA_TIME_DIV field.
 */
#define REGA_TIME_BASE		0x20

/*
 * Set the interval at 7.8125 ms.  RATE_US is the number of microseconds
 * to add to the counter at each interrupt.  WHEN_TO_ADD_ONE says after
 * how many intervals should one extra microsecond be added in.  This
 * is necessary because the interval is actually 7812.5 microseconds.
 */
#define SELECTED_RATE		0x9
#define RATE_US			7812
#define WHEN_TO_ADD_ONE		0x1

/*
 * Control register B.
 */
#define REGB_SET_TIME		0x80
#define REGB_PER_INT_ENA	0x40
#define REGB_UPDATE_INT_ENA	0x10
#define REGB_DATA_MODE		0x04
#define REGB_HOURS_FORMAT	0x02

/*
 * Control register C.
 */
#define REGC_INT_PENDING	0x80
#define REGC_PER_INT_PENDING	0x40
#define REGC_UPDATE_INT_PENDING	0x10

/*
 * Pointers to the three registers.
 */
volatile unsigned char	*regAPtr = (unsigned char *)(MACH_CLOCK_ADDR + 0x28);
volatile unsigned char	*regBPtr = (unsigned char *)(MACH_CLOCK_ADDR + 0x2C);
volatile unsigned char	*regCPtr = (unsigned char *)(MACH_CLOCK_ADDR + 0x30);

#define ONE_MILLION	1000000

/*
 * The "free running counter"
 */
Timer_Ticks counter;

Boolean	callbackIntrsWanted = FALSE;
Boolean profileIntrsWanted = FALSE;


/*
 *----------------------------------------------------------------------
 *
 * Timer_TimerInit --
 *
 *	Initialize the periodic timer.
 *
 *	N.B. This routine must be called before Timer_TimerStart.
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
    *regAPtr = REGA_TIME_BASE | SELECTED_RATE;
    *regBPtr = 0;
}


/*
 *----------------------------------------------------------------------
 *
 * Timer_TimerStart --
 *
 *	Start the timer ticking.
 *	ands starts the timer.
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
    unsigned char	dummy;

    *regBPtr = REGB_PER_INT_ENA;
    dummy = *regCPtr;

    if (timer == TIMER_CALLBACK_TIMER) {
	callbackIntrsWanted = TRUE;
    } else if (timer == TIMER_PROFILE_TIMER) {
	profileIntrsWanted = TRUE;
    } else {
	panic("Timer_TimerStart: unknown timer %d\n", timer);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * Timer_TimerInactivate --
 *
 *      Stops the specified timer such that it will cease counting and
 *      also resests the mode register to 0.  If the timer has already
 *      stopped and has set its output line high, clear the output so it
 *      won't cause an interrupt (because we don't care that it has
 *      expired).
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
	callbackIntrsWanted = FALSE;
    } else if (timer == TIMER_PROFILE_TIMER) {
	profileIntrsWanted = FALSE;
    } else {
	panic("Timer_TimerInactivate: unknown timer %d\n", timer);
    }

    /*
     * If neither type of timer interrupt is wanted, then disable
     * timer interrupts.
     */
    if (!callbackIntrsWanted && !profileIntrsWanted) {
	*regBPtr = 0;
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
Timer_TimerServiceInterrupt()
{ 
    static unsigned	addOne = 0;
    unsigned char timerStatus;

    timerStatus = *regCPtr;
    if (!(timerStatus & REGC_PER_INT_PENDING)) {
	/*
	 * Spurious interrupt.
	 */
	return;
    }
    /*
     * Increment the counter.
     */
    counter.microseconds += RATE_US;
    if ((addOne & WHEN_TO_ADD_ONE) == WHEN_TO_ADD_ONE) {
	counter.microseconds++;
    }
    addOne++;
    if (counter.microseconds > ONE_MILLION) {
	/*
	 * We wrapped around.
	 */
	counter.seconds++;
	counter.microseconds -= ONE_MILLION;
    }
    /*
     * Check for kernel profiling.  We'll sample the PC here.
     */
    if (profileIntrsWanted) {
	TIMER_PROFILE_ROUTINE(0);
    } 
    TIMER_CALLBACK_ROUTINE();
}


/*
 *----------------------------------------------------------------------
 *
 * Timer_CounterInit --
 *
 *	Set up a free-running counter by concatenating two
 *	counters on the AM9513 chip. The modes are set to
 *	count forever and never cause interrupts.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The specified counters begin to count.
 *
 *----------------------------------------------------------------------
 */
void
Timer_CounterInit()
{
    counter.seconds = 0;
    counter.microseconds = 0;
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

    *ticksPtr = counter;

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
