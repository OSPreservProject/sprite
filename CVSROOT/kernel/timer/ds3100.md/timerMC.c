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
 * This chip has a real-time clock (RTC) and an interval timer.  The real-time
 * clock is basically useless because it only has a resolution of seconds.
 * The machine-independent part assumes that we have a free-running counter
 * which has a resolution of milliseconds.  What we end up doing is
 * faking the free running counter. The RTC generates an interrupt every
 * second which we use to increment the seconds counter and clear the
 * microseconds counter.  The interval timer is used to increment the
 * microseconds counter.  We have a problem when interrupts are turned off
 * since the counter doesn't get incremented.  This is fixed by comparing
 * the difference in the hardware clock since the last interrupt against
 * the difference in the software clock.  The hardware clock is supposed
 * to go invalid 244 microseconds after the interrupt.  It isn't clear
 * when the update-in-progress flag is set. My reading of the
 * manual leads me to believe that it is set at the same time as the
 * interrupt.  In this were true we could never read the hardware clock
 * in the interrupt handler.  Either this isn't true, or we are usually
 * delayed enough getting to the handler so that the clock is valid 
 * again because the update-in-progress bit is never set (I ran a few
 * tests).  
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif

#include <sprite.h>
#include <sys.h>
#include <timerInt.h>
#include <timerTick.h>
#include <spriteTime.h>
#include <mach.h>
#include <machMon.h>
#include <prof.h>
#include <timer.h>
#include <machAddrs.h>
#include <assert.h>

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
 * Set up the interval structures. These are passed to Timer_CallBack
 * and are used to compute the interval between callbacks. Since we have
 * an interval of 7812.5 on the stupid ds3100 we have two structures and
 * alternate passing them to Timer_CallBack.
 * The interval is represented both as a Time, and as an integer. It is
 * too expensive to convert between them later so set them both up here.
 */



static Time lowTime = {0, 7812};
static int lowInterval = 7812;
static Time highTime = {0, 7813};
static int highInterval = 7813;

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
 * Control register D.
 */

#define REGD_VALID_TIME		0x80
/*
 * Pointers to registers. 
 */
volatile unsigned char	*secPtr  = (unsigned char *)(MACH_CLOCK_ADDR + 0x00);
volatile unsigned char	*minPtr  = (unsigned char *)(MACH_CLOCK_ADDR + 0x08);
volatile unsigned char	*hourPtr = (unsigned char *)(MACH_CLOCK_ADDR + 0x10);
volatile unsigned char	*dayPtr  = (unsigned char *)(MACH_CLOCK_ADDR + 0x1C);
volatile unsigned char	*monPtr  = (unsigned char *)(MACH_CLOCK_ADDR + 0x20);
volatile unsigned char	*yearPtr = (unsigned char *)(MACH_CLOCK_ADDR + 0x24);
volatile unsigned char	*regAPtr = (unsigned char *)(MACH_CLOCK_ADDR + 0x28);
volatile unsigned char	*regBPtr = (unsigned char *)(MACH_CLOCK_ADDR + 0x2C);
volatile unsigned char	*regCPtr = (unsigned char *)(MACH_CLOCK_ADDR + 0x30);
volatile unsigned char	*regDPtr = (unsigned char *)(MACH_CLOCK_ADDR + 0x34);

/*
 * We store a couple of things in the non-volatile ram.
 */

#define NVR_ADDR (MACH_CLOCK_ADDR + 0x38)

volatile unsigned char *localOffsetNVMPtr = (unsigned char *) (NVR_ADDR + 0x00);
volatile unsigned char *DSTAllowNVMPtr    = (unsigned char *) (NVR_ADDR + 0x04);

#define ONE_MILLION	1000000

/*
 * The "free running counter"
 */
Timer_Ticks counter;

Boolean	callbackIntrsWanted = FALSE;
Boolean profileIntrsWanted = FALSE;


/*
 * The RTC registers can only be accessed one byte at a time. This routine
 * is used to write words into the non-volatile storage.
 */

#define BYTECOPY(a,b,num) { \
    int	i; \
    for (i = 0; i < (num); i++) { \
	((char *) (b))[i] = ((char *) (a))[i]; \
    } \
}

/*
 * Used for debugging. Counts number of times free-running counter was
 * corrected.
 */
int 	timerCorrectedClock;


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
    *regBPtr = REGB_DATA_MODE | REGB_HOURS_FORMAT;
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
#ifndef lint
    unsigned char	dummy;
#endif

    *regBPtr |= REGB_PER_INT_ENA;
#ifndef lint
    dummy = *regCPtr;
#endif

    Mach_MonPrintf("Starting timer interrupts.\n");
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
	*regBPtr &= ~REGB_PER_INT_ENA;
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
Timer_TimerServiceInterrupt(statusReg, causeReg, pc)
    unsigned int    statusReg;
    unsigned int    causeReg;
    Address         pc;
{ 
    static unsigned	addOne = 0;
    unsigned char 	timerStatus;
    Time		*timePtr;
    unsigned int 	interval;
    Time_Parts		currentTODParts;
    static int		previousSoftSeconds;
    static int		previousHardSeconds;
    int			softSeconds;
    int			hardSeconds;
    int			diff;
    static Boolean	initialized = FALSE;

    static	int	eventDebug[1000];
    static	int	dbgCtr = 0;
#define INC(a) { (a) = ((a) + 1) % 1000; }

    timerStatus = *regCPtr;
    eventDebug[dbgCtr] = 0;
    if (timerStatus & REGC_PER_INT_PENDING) {
	/*
	 * Increment the counter.
	 */
	counter.microseconds += RATE_US;
	if ((addOne & WHEN_TO_ADD_ONE) == WHEN_TO_ADD_ONE) {
	    counter.microseconds++;
	    timePtr = &highTime;
	    interval = highInterval;
	} else {
	    timePtr = &lowTime;
	    interval = lowInterval;
	}
	addOne++;
	if (counter.microseconds >= ONE_MILLION) {
	    /*
	     * We wrapped around. 
	     */
	    counter.microseconds = ONE_MILLION - 1;
	}
    }
    if ((timerStatus & REGC_UPDATE_INT_PENDING)) {
	/*
	 * RTC interrupt.
	 */
	counter.seconds++;
	counter.microseconds = 0;
	eventDebug[dbgCtr] |= 0x1;
	if ((*regAPtr & REGA_UIP) == 0) {
	    currentTODParts.seconds = *secPtr;
	    currentTODParts.minutes = *minPtr;
	    currentTODParts.hours = *hourPtr;
	    currentTODParts.dayOfMonth = *dayPtr;
	    currentTODParts.dayOfYear = -1;
	    currentTODParts.month = *monPtr-1;
	    currentTODParts.year = *yearPtr;
	    (void) Time_FromParts(&currentTODParts, FALSE, &hardSeconds);
	    softSeconds = counter.seconds;
	    diff = hardSeconds - previousHardSeconds;
	    eventDebug[dbgCtr] |= 0x4;
	    if (softSeconds - previousSoftSeconds != diff && initialized) {
		/*
		 * Note that the software counter cannot be ahead of
		 * the hardware counter. We are only looking at the
		 * seconds, and the seconds are only incremented 
		 * during an interrupt. We only get one interrupt
		 * a second so we may be behind but not ahead.
		 */
		if (softSeconds - previousSoftSeconds > diff) {
		    panic("Software time is ahead of hardware!\n");
		}
		counter.seconds = previousSoftSeconds + diff;
		softSeconds = counter.seconds;
		timerCorrectedClock++;
		eventDebug[dbgCtr] |= 0x8;
	    }
	    previousHardSeconds = hardSeconds;
	    previousSoftSeconds = softSeconds;
	    initialized = TRUE;
	}
    }    
    if (timerStatus & REGC_PER_INT_PENDING) {
	if (mach_KernelMode) {
	    assert((statusReg & MACH_SR_KU_PREV) == 0);
	    /*
	     * Check for kernel profiling.  We'll sample the PC here.
	     */
	    if (profileIntrsWanted) {
		TIMER_PROFILE_ROUTINE(pc);
	    } 
	} else {
	    assert((statusReg & MACH_SR_KU_PREV) != 0);
	    Proc_GetCurrentProc()->Prof_PC = (int) pc;
	}
	TIMER_CALLBACK_ROUTINE(interval, *timePtr);
    }
    INC(dbgCtr);
}


/*
 *----------------------------------------------------------------------
 *
 * Timer_CounterInit --
 *
 *	Initialize free-running counter.
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

/*
 *----------------------------------------------------------------------
 *
 * TimerHardwareUniversalTimeInit --
 *
 *	Checks the battery backed up clock. If the contents are still
 *	valid then we return them. 
 *
 *	IMPORTANT:  right now it looks like the ds3100 RTC is reset
 *	by the prom during the boot.
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
TimerHardwareUniversalTimeInit(timePtr, localOffsetPtr, DSTPtr)
    Time *timePtr;		/* Buffer to hold universal time. */
    int  *localOffsetPtr;	/* Buffer to hold local offset. */
    Boolean *DSTPtr;		/* Buffer to hold DST allowed flag. */
{
    Time_Parts		timeParts;
    int			seconds;
    ReturnStatus	status;

    while ((*regAPtr & REGA_UIP) == 1) {
    }
    timeParts.seconds = *secPtr;
    timeParts.minutes = *minPtr;
    timeParts.hours = *hourPtr;
    timeParts.dayOfMonth = *dayPtr;
    timeParts.dayOfYear = -1;
    timeParts.month = *monPtr-1;
    timeParts.year = *yearPtr;
    status = Time_FromParts(&timeParts, FALSE, &seconds);
    if (status != SUCCESS) {
	Mach_MonPrintf("Time stored in RTC is bogus.\n");
	return;
    }
    if (*regDPtr & REGD_VALID_TIME == 0) {
	Mach_MonPrintf(
	    "Warning: battery backed up TOD clock is invalid.\n");
	return;
    }
    bzero((Address) timePtr, sizeof(Time));
    timePtr->seconds = seconds;
    BYTECOPY(localOffsetNVMPtr, localOffsetPtr, 4);
    BYTECOPY(DSTAllowNVMPtr, DSTPtr, 4);
}

/*
 *----------------------------------------------------------------------
 *
 * TimerSetHardwareUniversalTime --
 *
 *	Sets the hardware RTC clock. Has to be called with interrupts
 *	off.
 *
 *	IMPORTANT: The RTC is used to keep the free-running counter
 *	up-to-date if we miss a few interrupts. DO NOT RESET THE RTC
 *	WHILE THE KERNEL IS RUNNING!!! If you do system time will
 *	not be linear and bad things will happen. Only call this routine
 *	during startup and shutdown. As things now stand there is no
 *	reason to call this routine at all since the RTC does not
 *	appear to persist across reboots.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The hardware RTC is set.
 *
 *----------------------------------------------------------------------
 */

void
TimerSetHardwareUniversalTime(timePtr, localOffset, DST)
    Time *timePtr;		/* universal time. */
    int  localOffset;		/* local offset. */
    Boolean DST;		/* DST allowed flag. */
{
    Time_Parts		timeParts;

    Time_ToParts(timePtr->seconds, FALSE, &timeParts);
    *regBPtr |= REGB_SET_TIME;
    *secPtr = (unsigned char) timeParts.seconds;
    *minPtr = (unsigned char) timeParts.minutes;
    *hourPtr = (unsigned char) timeParts.hours;
    *dayPtr = (unsigned char) timeParts.dayOfMonth;
    *monPtr = (unsigned char) timeParts.month+1;
    *yearPtr = (unsigned char) timeParts.year;
    BYTECOPY(&localOffset,localOffsetNVMPtr,  4);
    BYTECOPY(&DST, DSTAllowNVMPtr, 4);
    *regBPtr &= ~REGB_SET_TIME;
}
