/*
 * devTimerSun3.c --
 *
 *	This file contains routines that manipulate the Intersil
 *	ICM7170 real-time clock chip for the SUN-3.
 *	(For a detailed explanation of the chip, see the data sheet
 *	for the ICM7170 [Dec. 1985, 301680-005].)
 *
 *	The Intersil chip is used to provide perdiodic interrupts
 *	and contains a set of free-running counters that keep track of time.
 *	The routine in this file are used to setup the chip to
 *	cause interrupts every 10 milliseconds (100th of second)
 *	and maintain the time since the counters were initialized.
 *
 *      According to the Sun-3 Architecture manual (rev. 2.0, 25 July
 *      1985), The chip's "interrupt output signal causes an interrupt
 *      request on level 5 or 7 via the interrupt register." The routines
 *      in this file set and reset the level-5 interrupt enable bit in the
 *      interrupt register.  It also assumed that level-7 interrupts are
 *      disabled.
 *
 *	The AMD timer chip on the Sun-2 provides several timers and
 *	two of them are used by the Timer_ module for controlling the 
 *	callback queue and profiling. Unfortunately the Intersil chip 
 *	provides just one timer. The two timers used by the Timer module
 *	are built from the chip's single timer. Thus whenever the
 *	Intersil chip's timer interrupts, the two virtual timers
 *	can appear to have gone off. Unlike the timers on the Sun-2,
 *	the virtual timers can not cause indepdent interrupts.
 *
 *	The free-running counters on the Intersil chip can be used to
 *	keep the time of day in Unix format (i.e. the time since 1/1/1970).
 *	The counter is used here to keep track of the amount of time
 *	since the system was booted.
 *
 *	Additional Dev_Counter routines are provided to convert from/to
 *	32-bit interval values (described in timerClock.c) to/from time.
 *	The interval represents an machine-dependent format for small 
 *	time values. For the Sun-3, an interval is a fraction of second
 *	because it is easily converted to the units that the free-running 
 *	counters count in.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 */

#ifdef SUN3

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "sys.h"
#include "machine.h"
#include "sunMon.h"
#include "devAddrs.h"
#include "devTimer.h"
#include "devTimerSun3Int.h"
#include "time.h"

static TimerDevice *timerRegsPtr = (TimerDevice *) DEV_TIMER_ADDR;

/*
 * Flags to indicate whether a timer is enabled or not.
 */
static Boolean profileIntrsWanted = FALSE;
static Boolean callbackIntrsWanted = FALSE;

/*
 * Initial state of the timer's interrupt enable/disable bit.
 */

static int timerIntrState = INTR_DISABLE;

/*
 * Frequency of the timer's interrupts.
 */
#define TIMER_INTR_FREQ 	S100TH_SEC_MASK

/*
 * Initial values of the free-running counters.
 * According to the Intersil data sheet, the month and day
 * counters start counting at 1.
 */
static    IntersilCounters	initialCounter = {
    0,	/* hundredths */
    0,	/* hours */
    0,	/* minutes */
    0,	/* seconds */
    1,	/* month */
    1,	/* day */
    0,	/* year */
    0,	/* dayOfWeek */
};

/*
 * The interval value to represent one second. It must be at least 1000
 * so that one milliscond can be be represented in an interval.
 */

#define ONE_SEC_INTERVAL	1000


/*
 * The maximum amount of time that an interval can represent.
 */

#define MAXINT ((unsigned int) 0xFFFFFFFF)
static Time maxIntervalTime = {
    MAXINT / ONE_SEC_INTERVAL,
    ((MAXINT % ONE_SEC_INTERVAL) * ONE_SECOND) / ONE_SEC_INTERVAL,
};

/*
 * Routine to calculate a time value from the values in the free-running
 * counters.
 */

static void CountersToTime();

/*
 * Constants used to convert the contents of the free-running counters
 * to time in seconds and microseconds.
 */

#define SECS_PER_MIN	60
#define SECS_PER_HOUR	SECS_PER_MIN * 60
#define SECS_PER_DAY 	SECS_PER_HOUR * 24
#define SECS_PER_YEAR	SECS_PER_DAY * 365

/*
 * Number of seconds since the beginning of the year to the
 * beginning of a month.
 */
static int accumSecsPerMonth[13] = {
    0,		/* nothing */
    0 * SECS_PER_DAY,		/* Jan */
    31 * SECS_PER_DAY,		/* Feb */
    59 * SECS_PER_DAY,		/* Mar */
    90 * SECS_PER_DAY,		/* Apr */
    120 * SECS_PER_DAY,		/* May */
    151 * SECS_PER_DAY,		/* Jun */
    181 * SECS_PER_DAY,		/* Jul */
    212 * SECS_PER_DAY,		/* Aug */
    243 * SECS_PER_DAY,		/* Sep */
    273 * SECS_PER_DAY,		/* Oct */
    304 * SECS_PER_DAY,		/* Nov */
    334 * SECS_PER_DAY,		/* Dec */
};

/*
 * Version of above for leap years.
 */
static int accumSecsPerMonthLeap[13] = {
    0,		/* nothing */
    0 * SECS_PER_DAY,		/* Jan */
    31 * SECS_PER_DAY,		/* Feb */
    60 * SECS_PER_DAY,		/* Mar */
    91 * SECS_PER_DAY,		/* Apr */
    121 * SECS_PER_DAY,		/* May */
    152 * SECS_PER_DAY,		/* Jun */
    182 * SECS_PER_DAY,		/* Jul */
    213 * SECS_PER_DAY,		/* Aug */
    244 * SECS_PER_DAY,		/* Sep */
    274 * SECS_PER_DAY,		/* Oct */
    305 * SECS_PER_DAY,		/* Nov */
    335 * SECS_PER_DAY,		/* Dec */
};




/*
 *----------------------------------------------------------------------
 *
 * Dev_TimerInit --
 *
 *	Initialize the ICM7170 chip.
 *
 *	N.B. This routine must be called before Dev_TimerStart.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The timer is initialized and ready to start ticking.
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
void
Dev_TimerInit(timer)
    unsigned short 	timer;		/* Ignored for Sun-3 version. */
{
    static Boolean init = FALSE;
    unsigned char c;

    if (!init) {
	init = TRUE;

	/*
	 * Tell the chip to prevent interrupts.
	 */
	timerIntrState = INTR_DISABLE;
	timerRegsPtr->commandReg = IntersilCommand(RUN, timerIntrState);

	/*
	 * Tell the chip to cause periodic interrupts.
	 */
	timerRegsPtr->interruptReg = TIMER_INTR_FREQ;

	/*
	 * Read the chip's interrupt register to clear any pending inerrupts.
	 */
	c = timerRegsPtr->interruptReg;
#ifdef lint
	c = c;
#endif
    }
}


/*
 *----------------------------------------------------------------------
 *
 * Dev_TimerStart --
 *
 *	Start the timer so it will cause periodic interrupts.
 *	N.B. The timer must have been initialized with Dev_TimerInit
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
Dev_TimerStart(timer)
    unsigned short timer;
{
    if (timer == DEV_CALLBACK_TIMER) {
	callbackIntrsWanted = TRUE;
    } else if (timer == DEV_PROFILE_TIMER) {
	profileIntrsWanted = TRUE;
    } else {
	Sys_Panic(SYS_FATAL,"Dev_TimerStart: unknown timer %d\n", timer);
    }

    /*
     * Tell the chip to start counting and allow it to cause interrupts.
     */
    timerIntrState = INTR_ENABLE;
    timerRegsPtr->commandReg = IntersilCommand(RUN, INTR_ENABLE);

    /*
     * Enable timer interrupts in the system's interrupt register.
     */

    *SunInterruptReg |= SUN_ENABLE_LEVEL5_INTR;
}


/*
 *----------------------------------------------------------------------
 *
 * Dev_TimerInactivate --
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
Dev_TimerInactivate(timer)
    register unsigned short timer;
{
    if (timer == DEV_CALLBACK_TIMER) {
	callbackIntrsWanted = FALSE;
    } else if (timer == DEV_PROFILE_TIMER) {
	profileIntrsWanted = FALSE;
    } else {
	Sys_Panic(SYS_FATAL,"Dev_TimerInactivate: unknown timer %d\n", timer);
    }

    /*
     * If neither type of timer interrupt is wanted, then disable
     * timer interrupts.
     */

    if (!callbackIntrsWanted && !profileIntrsWanted) {
	*SunInterruptReg &= ~SUN_ENABLE_LEVEL5_INTR;
	timerRegsPtr->commandReg = IntersilCommand(RUN, INTR_DISABLE);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * Dev_TimerGetStatus --
 *
 *	Clears the interrupt pending bit in the interrupt register
 *	and reads the interrupt status register (ISR) of the chip.
 *	The ISR must be read in order to reset the interrupt, according
 *	to the Intersil data sheet (p.6, "Periodic Interrupts").
 *	(It seems that the value is always 0.) According to the
 *	Sun-3 architure manual (p.31), the level-5 bit in the interrupt 
 *	register must be toggled to clear the interrupt.
 *
 * Results:
 *	The value of the status register.
 *
 * Side effects:
 *	The interrupt is cleared.
 *
 *----------------------------------------------------------------------
 */

unsigned short
Dev_TimerGetStatus()
{
    unsigned char statusReg;
    unsigned char intrReg;

    intrReg = *SunInterruptReg & SUN_ENABLE_LEVEL7_INTR;

    *SunInterruptReg &= ~(SUN_ENABLE_LEVEL5_INTR | 
				SUN_ENABLE_ALL_INTERRUPTS | intrReg);

    statusReg = timerRegsPtr->interruptReg;

    *SunInterruptReg |= (SUN_ENABLE_LEVEL5_INTR | 
				SUN_ENABLE_ALL_INTERRUPTS | intrReg);

    /*
     * Read the chip again in case an obscure race condition occurs.
     * (This is how Sun handles it.)
     */
    statusReg = timerRegsPtr->interruptReg;

    return(statusReg);
}


/*
 *----------------------------------------------------------------------
 *
 * Dev_TimerExamineStatus --
 *
 *	Assuming the interrupt is valid, return TRUE or FALSE if 
 *	the interrupt is or is not allowed for the specified timer.
 *	(The status register is always 0 on the Sun-3 so we can't tell
 *	if the chip really did interrupt.)
 *
 *	This routine is used to decrease the frequency of
 *	a virtual timer. The callback timer's frequency is
 *	one-half of the real timer's frequency.
 *
 * Results:
 *	TRUE		if the timer interrupted.
 *	FALSE		if the timer did not interrupt.
 *
 * Side effects:
 *	The callback virtual timer's toggle is changed.
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
Boolean
Dev_TimerExamineStatus(statusReg, timer, spuriousPtr)
    unsigned char statusReg;	/* Ignored because it is always 0. */
    unsigned int timer;		/* Virtual Timer # */
    Boolean *spuriousPtr;	/* Always set FALSE. */
{
    static Boolean callbackTimerToggle = FALSE;

    /*
     * Assume we never get a spurious interrupt because the statusReg
     * value is always 0 on the Sun-3. 
     */
    *spuriousPtr = FALSE;

    if (timer == DEV_CALLBACK_TIMER) {
	/*
	 * Cut the timer's frequency in half.
	 */
	callbackTimerToggle = !callbackTimerToggle;
	return(callbackTimerToggle);
    } else if (timer == DEV_PROFILE_TIMER) {
	if (profileIntrsWanted) {
	    return(TRUE);
	} else {
	    return(FALSE);
	}
    } else {
	Sys_Panic(SYS_WARNING,"Dev_TimerExamineStatus: unknown timer %d\n", 
				timer);
	return(FALSE);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * Dev_CounterInit --
 *
 *	Initializes the chip's free-running counters.
 *	The STOP command stops the counter and the RUN command turns
 *	it back on.
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
Dev_CounterInit()
{
    timerRegsPtr->commandReg = IntersilCommand(STOP, timerIntrState);
    timerRegsPtr->counter = initialCounter;
    timerRegsPtr->commandReg = IntersilCommand(RUN, timerIntrState);
}


/*
 *----------------------------------------------------------------------
 *
 * Dev_CounterRead --
 *
 *	Read the contents of the counters. Interrupts are assumed
 *	to be disabled to assure that the counters are read atomically.
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
Dev_CounterRead(timePtr)
    Time	*timePtr;	/* Time from the counters. */
{
    IntersilCounters	counter;

    /*
     * Read the chip's counters.
     */

    counter = timerRegsPtr->counter;
    CountersToTime(&counter, timePtr);
}


/*
 *----------------------------------------------------------------------
 *
 * CountersToTime --
 *
 *	Converts the values in the Intersil free-running counters
 *	to a time value.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
CountersToTime(counterPtr, timePtr)
    register IntersilCounters	*counterPtr;
    register Time		*timePtr;
{
    register int seconds = 0;
    register int *accumPtr;

    if (counterPtr->year != 0) {
	seconds += (counterPtr->year * SECS_PER_YEAR) +
		   (((counterPtr->year + 3) / 4) * SECS_PER_DAY);
    }

    /*
     * Is it a leap year?
     */
    if ((counterPtr->year % 4) == 0) {
	accumPtr = accumSecsPerMonthLeap;
    } else {
	accumPtr = accumSecsPerMonth;
    }

    timePtr->seconds = seconds +
		accumPtr[counterPtr->month] +
		((counterPtr->day - 1) * SECS_PER_DAY) +
		(counterPtr->hours * SECS_PER_HOUR) +
		(counterPtr->minutes * 60) +
		counterPtr->seconds;
    timePtr->microseconds = (counterPtr->hundredths * HUNDREDTH_SECOND);
}



/*
 *----------------------------------------------------------------------
 *
 *  Dev_CounterIntToTime --
 *
 *      Converts a 32-bit interval value into a standard time value.
 *
 *  Results:
 *	A time value.
 *
 *  Side Effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
Dev_CounterIntToTime(counter, resultPtr)
    unsigned int counter;
    Time *resultPtr;
{
    resultPtr->seconds = counter / ONE_SEC_INTERVAL;
    resultPtr->microseconds = (counter % ONE_SEC_INTERVAL) * 
				(ONE_SECOND/ONE_SEC_INTERVAL);
}


/*
 *----------------------------------------------------------------------
 *
 *  Dev_CounterTimeToInt --
 *
 *      Converts a standard time value into a 32-bit interval value.
 *
 *  Results:
 *	An interval value.
 *
 *  Side Effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
Dev_CounterTimeToInt(time, resultPtr)
    Time time;
    unsigned int *resultPtr;
{
    if (Time_LE(time, maxIntervalTime)) {
	*resultPtr = (time.seconds * ONE_SEC_INTERVAL) + 
		 ((time.microseconds * ONE_SEC_INTERVAL) / ONE_SECOND);
    } else {
	Sys_Panic(SYS_WARNING, "Dev_CounterTimeToInt: time value too large\n");
	*resultPtr = 0xFFFFFFFF;
    }
}


/*  @#@#@#@#@#@#@#@#@#@#@    DEBUGGING CODE    @#@#@#@#@#@#@#@#@#@#@  */

/*
 *----------------------------------------------------------------------
 *
 * Dev_TimerGetInfo --
 *
 *	Debugging routine to print the contents of the free-running
 *	counters.
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
Dev_TimerGetInfo(data)
    ClientData	data;		/* Ignored. */
{
    IntersilCounters  counter;
    Time	time;

    if ((int) data == 1) {
	DISABLE_INTR();
	timerRegsPtr->commandReg = IntersilCommand(RUN, INTR_ENABLE);
	ENABLE_INTR();
	return;
    }

    DISABLE_INTR();
    counter = timerRegsPtr->counter;
    ENABLE_INTR();

    Sys_Printf("Hundredths	%d\n", counter.hundredths);
    Sys_Printf("Seconds		%d\n", counter.seconds);
    Sys_Printf("Minutes		%d\n", counter.minutes);
    Sys_Printf("Hours		%d\n", counter.hours);
    Sys_Printf("Day		%d\n", counter.day);
    Sys_Printf("Month		%d\n", counter.month);
    Sys_Printf("Year		%d\n", counter.year);
    Sys_Printf("Day of Week	%d\n", counter.dayOfWeek);

    CountersToTime(&counter, &time);
    Sys_Printf("Time:	%d.%06u\n", time.seconds, time.microseconds);
}

#ifdef NOT_USED

static void
TimeToCounters(time, counterPtr)
    Time	time;
    register IntersilCounters	*counterPtr;
{
    int	*accumPtr;
    register int seconds;

#define SECS_SINCE_1970_FOR(year) \
(((year - 1970) * SECS_PER_YEAR) + ((((year - 1970) + 3) / 4) * SECS_PER_DAY))

    seconds = time.seconds - SECS_SINCE_1970_FOR(1984);

    if (seconds < 0) {
	Sys_Panic(SYS_WARNING, "Tried to set time before 1984\n");
	return;
    }

    counterPtr->year = seconds / SECS_PER_YEAR;
    seconds %= SECS_PER_YEAR;
    seconds -= ((counterPtr->year + 3) / 4) * SECS_PER_DAY;

    if ((counterPtr->year % 4) == 0) {
	accumPtr = accumSecsPerMonthLeap;
    } else {
	accumPtr = accumSecsPerMonth;
    }

    for (month = 1; month < 13; month++) {
	if (seconds < accumPtr[month]) {
	    seconds -= accumPtr[month];
	    counterPtr->month = month;
	    break;
	}
    }
    counterPtr->day = (seconds / SECS_PER_DAY) + 1;
    seconds %= SECS_PER_DAY;
    counterPtr->hours = seconds / SECS_PER_HOUR;
    seconds %= SECS_PER_HOUR;
    counterPtr->minutes = seconds / 60;
    counterPtr->seconds = seconds % 60;

    counterPtr->tenths = time.microseconds / TENTH_SECOND;
    counterPtr->hundredths = 
		(time.microseconds % TENTH_SECOND) / HUNDREDTH_SECOND;
}
#endif NOT_USED

#endif SUN3
