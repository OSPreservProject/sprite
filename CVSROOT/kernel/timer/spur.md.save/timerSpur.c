/*
 * timerSpur.c --
 *
 *	This file contains routines that manipulate the SPUR Cache Controller's
 *	timers. 
 *
 *	For a detailed explanation of the timers, see the reference [Wood87] 
 *	in devTimerSpurInt.h
 *
 *	From [Wood87] Section 4.3
 *
 *	There are three counters on the chip. Counter T0 is a 64bit timer
 *	that is incremented on every cycle. T0 is left running
 *	at all times and is used to compute time of day. Counter T1 and T2
 *	on the chip are 32 bit interval timers. T1 and T2 count up from a
 *	interval and will "stop within a few cycles of zero". Although it 
 *	is not stated in the reference manual, it is assumed that T1 and T2
 *	are also updated every cycle.
 *
 *	Many of the routines in here interact with the routines in the
 *	timer module. The timer module uses a free-running counter to
 *	calculate system up-time and the time of day. The timer module
 *	provides 64-bit (Timer_Ticks) and 32-bit (interval) time values 
 *	based on the value of the counter. See timerClock.c for details.
 *
 * Copyright 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "sys.h"
#include "timer.h"
#include "timerSpurInt.h"
#include "mach.h"
#include "sync.h"
#include "spriteTime.h"

/*
 * The timers must be loaded with the number of ticks to count
 * before causing an interrupt. The number of ticks is calculated
 * from the timer's interrupt interval (in milliseconds) and
 * the timer's counting frequency in routine Timer_TimerInit().
 */

static	unsigned int callbackTicks;
static  unsigned int profileTicks;

/*
 * A zero valued counter.
 */
static	 Timer_Ticks	zeroCount = {0,0};

/*
 * The largest interval value.
 */
#define	MAXINT	((unsigned int ) 0xffffffff)

/*
 * Processor containing the timer for each timer and counter
 */

#define	CALLBACK_COUNTER	0
#define	PROFILE_COUNTER		1
#define	CLOCK_COUNTER		2
#define	NUM_COUNTERS		3

static	int	processor[NUM_COUNTERS]; 

/*
 * Last value read from counter. This number is used to provide access to a
 * recent value of the counter for processors other than the processor with
 * the counter. It is assumed that the processor with the counter will also 
 * be the processor with the inteval times to Timer_GetCurrentTicks will
 * be called frequently. 
 */

static Timer_Ticks	currentTicks;

/*
 * Semaphore protecting the currentTicks variable.
 */

Sync_Semaphore	currentTicksMutex = 
    Sync_SemInitStatic("Timer:currentTicksMutex");

/*
 *  Time between callbacks.  */

static int interval;

static Time time = { 0, TIMER_CALLBACK_INTERVAL_APPROX};

/*
 *----------------------------------------------------------------------
 *
 * Timer_TimerInit --
 *
 *	Initialize the specified timer on the Cache Controller chip to
 *	cause interrupts at regular intervals. Initialize the call back
 *	interval ticks.
 *
 *	N.B. This routine must be called before Timer_TimerStart.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The specified timer is initialized and ready to start ticking.
 *
 *----------------------------------------------------------------------
 */

void
Timer_TimerInit(timer)
    register unsigned short 	timer;
{
     unsigned int modeRegister;		/* Local copy of CC mode register */
     unsigned int imaskRegister; 	/* Local copy of CC imask register */
     unsigned int intrMaskBit = 0; 	/* Set timer's bit in imask register */
     unsigned int modeRegBit = 0;	/* Set timer's bit in mode register */

    /*
     * Find the imask/istatus and the mode register bit for the specified
     * timer.  Also compute the timer's value.
     */

     if (timer == TIMER_CALLBACK_TIMER) {
	intrMaskBit = CALLBACK_TIMER_MASK_BIT;
	modeRegBit = CALLBACK_TIMER_MODE_BIT;
	callbackTicks = (int) (TIMER_FREQ/1000000.0 * 
		TIMER_CALLBACK_INTERVAL_APPROX); 
	interval = callbackTicks >> TIMER_INTERVAL_SHIFT;
	processor[CALLBACK_COUNTER] = Mach_GetProcessorNumber();
#ifdef NO_PROFILE_TIMER
    } else if (timer == TIMER_PROFILE_TIMER) {
	intrMaskBit = PROFILE_TIMER_MASK_BIT;
	modeRegBit = PROFILE_TIMER_MODE_BIT;
	profileTicks = (int) (TIMER_FREQ/1000.0 * TIMER_PROFILE_INTERVAL); 
	processor[PROFILE_COUNTER] = Mach_GetProcessorNumber();
#endif
    } else {
	panic("Timer_TimerInit: unknown timer %d\n", timer);
    }
    /*
     * Stop the timer in case it is ticking.
     */
    Mach_DisableNonmaskableIntr();
    modeRegister = Mach_Read8bitCCReg(MACH_MODE_REG);
    if (modeRegister & modeRegBit) { 
	modeRegister &= ((~modeRegBit) & 0xff);
	Mach_Write8bitCCReg(MACH_MODE_REG,modeRegister);
    }
    Mach_EnableNonmaskableIntr();
    /*
     * Reset the timer's bit in the CC's Istatus register. Note that the
     * Istatus register always writes by anding the ones complement of the
     * data. ([Wood87] Section 5.1)
     */
    Mach_Write32bitCCReg(MACH_INTR_STATUS_0,intrMaskBit);
    /*
     * Register the routine to call on this interrupt.
     */
    Mach_SetHandler(intrMaskBit,Timer_TimerServiceInterrupts);
    Mach_EnableNonmaskableIntr();

}


/*
 *----------------------------------------------------------------------
 *
 * Timer_TimerStart --
 *
 *	Loads the specified timer with a new value to count from
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
    unsigned int modeRegister;		/* Local copy of mode register. */
    unsigned int modeRegBit = 0;	/* Timer's bit in mode register. */
    unsigned int timerAddress = 0;	/* Timer's addresses in CC. */
    int ticks = 0;			/* Number of ticks to set timer to. */
    int		counter;		/* Processor number of counter. */

    if (timer == TIMER_CALLBACK_TIMER) {
	modeRegBit = CALLBACK_TIMER_MODE_BIT;
	timerAddress = CALLBACK_TIMER_ADDR;
	ticks = callbackTicks;
	counter	= CALLBACK_COUNTER;
#ifdef NO_PROFILE_TIMER
    } else if (timer == TIMER_PROFILE_TIMER) {
	modeRegBit = PROFILE_TIMER_MODE_BIT;
	timerAddress = PROFILE_TIMER_ADDR;
	ticks = profileTicks;
	counter	= PROFILE_COUNTER;
#endif
    } else {
	panic("Timer_TimerStart: unknown timer %d\n", timer);
    }
    if (processor[counter] != Mach_GetProcessorNumber()) {
	panic(
	    "Timer_TimerStart: timer %d started on wrong processor.\n", timer);
    }

    DISABLE_INTR();
    /*
     * Make sure the timer is really off.
     */
    Mach_DisableNonmaskableIntr();
    modeRegister = Mach_Read8bitCCReg(MACH_MODE_REG);
    if (modeRegister & modeRegBit) { 
	/*
	 * If it's not off, turn it off.
	 */
	modeRegister &= (~modeRegBit) & 0xff;
	Mach_Write8bitCCReg(MACH_MODE_REG,modeRegister);
    }
    Mach_EnableNonmaskableIntr();

#ifdef NO_PROFILE_TIMER
    /*
     * Setup the nonMaskable interupt for the PROFILE/refresh timer.
     */
    if (timer == TIMER_PROFILE_TIMER) {
	Mach_SetNonmaskableIntr(PROFILE_TIMER_MASK_BIT);
	Mach_EnableNonmaskableIntr();
    }
#endif
    /*
     * Initialize the timer. Since the timers count up the a negative number
     * is loaded.
     */
    Mach_Write32bitCCReg(timerAddress,(unsigned int) -ticks);
    /*
     * Start the timer ticking. We re-read the mode register to narrow the
     * window for the race condition with the other counter reseting themselves.
     *
     */
    Mach_DisableNonmaskableIntr();
    modeRegister = Mach_Read8bitCCReg(MACH_MODE_REG);
    modeRegister |= modeRegBit;
    Mach_Write8bitCCReg(MACH_MODE_REG,modeRegister);
    Mach_EnableNonmaskableIntr();
    ENABLE_INTR();
}

/*
 *----------------------------------------------------------------------
 *
 * RestartStart --
 *
 *	Reloads the specified timer with a new value to count from
 *	and start the timer. Timer is assumed to be stopped and 
 *	interrupts disabled.
 *
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The timer starts ticking.
 *
 *----------------------------------------------------------------------
 */

static void 
RestartTimer(timer)
    register unsigned short timer;
{
    unsigned int modeRegister;		/* Local copy of mode register */
    unsigned int modeRegBit = 0;	/* Timer's mode bit */
    unsigned int timerAddress = 0;	/* Timer's address */
    int ticks = 0;			/* Number of ticks to load */
    int		counter;		/* Processor number of counter. */

    if (timer == TIMER_CALLBACK_TIMER) {
	modeRegBit = CALLBACK_TIMER_MODE_BIT;
	timerAddress = CALLBACK_TIMER_ADDR;
	ticks = callbackTicks;
	counter	= CALLBACK_COUNTER;
#ifdef NO_PROFILE_TIMER
    } else if (timer == TIMER_PROFILE_TIMER) {
	modeRegBit = PROFILE_TIMER_MODE_BIT;
	timerAddress = PROFILE_TIMER_ADDR;
	ticks = profileTicks;
	counter	= PROFILE_COUNTER;
#endif
    } else {
	panic("RestartTimer: unknown timer %d\n", timer);
    }
    if (processor[counter] != Mach_GetProcessorNumber()) {
	panic(
	    "RestartTimer: timer %d restarted on wrong processor.\n", timer);
    }
    /*
     * Initialize the timer. Since the timers count up the a negative number
     * is loaded.
     */
    Mach_Write32bitCCReg(timerAddress,(unsigned int) -ticks);
    /*
     * Start the timer ticking. 
     */
    Mach_DisableNonmaskableIntr();
    modeRegister = Mach_Read8bitCCReg(MACH_MODE_REG);
    modeRegister |= modeRegBit;
    Mach_Write8bitCCReg(MACH_MODE_REG,modeRegister);
    Mach_EnableNonmaskableIntr();

}

/*
 *----------------------------------------------------------------------
 *
 * Timer_TimerInactivate --
 *
 *      Stops the specified timer such that it will cease counting. 
 *      If the timer has already  stopped and has set its interrupt
 *	line high, clear the output so it won't cause an interrupt
 *	(because we don't care that it has expired).
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
    unsigned int modeRegister;
    unsigned int imaskRegister;
    unsigned int modeRegBit = 0;
    unsigned int timerAddress = 0;
    unsigned int intrMaskBit = 0; 
    int		counter;		/* Processor number of counter. */

    if (timer == TIMER_CALLBACK_TIMER) {
	modeRegBit = CALLBACK_TIMER_MODE_BIT;
	timerAddress = CALLBACK_TIMER_ADDR;
	intrMaskBit = CALLBACK_TIMER_MASK_BIT;
	counter	= CALLBACK_COUNTER;
#ifdef NO_PROFILE_TIMER
    } else if (timer == TIMER_PROFILE_TIMER) {
	modeRegBit = PROFILE_TIMER_MODE_BIT;
	timerAddress = PROFILE_TIMER_ADDR;
	intrMaskBit = PROFILE_TIMER_MASK_BIT;
	counter	= PROFILE_COUNTER;
#endif
    } else {
	panic("Timer_TimerStart: unknown timer %d\n", timer);
    }
    if (processor[counter] != Mach_GetProcessorNumber()) {
	panic(
	    "Timer_TimerInactivate: timer %d stopped on wrong processor.\n",
	    timer);
    }
    /*
     * Stop the timer in case it is ticking.
     */
    DISABLE_INTR();
    Mach_DisableNonmaskableIntr();
    modeRegister = Mach_Read8bitCCReg(MACH_MODE_REG);
    if (modeRegister & modeRegBit) { 
	modeRegister &= (~modeRegBit & 0xff);
	Mach_Write8bitCCReg(MACH_MODE_REG,modeRegister);
    }
    Mach_EnableNonmaskableIntr();
    /*
     * Clear the interrupt status bit and turn the interrupt mask bit off.
     */
    Mach_Write32bitCCReg(MACH_INTR_STATUS_0,intrMaskBit);

    ENABLE_INTR();

}

/*
 *----------------------------------------------------------------------
 *
 *  Timer_TimerServiceInterrupts --
 *
 *      This routine is called at every timer interrupt. 
 *      It calls the timer callback queue handling if the callback timer 
 *	expired and calls the profiling interrupt handling if the 
 *	profile callback timer expired. The interval timers are reset to
 *	call again.
 *
 *  Results:
 *	None.
 *
 *  Side Effects:
 *	Routines on the timer queue may cause side effects. Profile
 *	collect may take place. Interval timers turned on.
 *	
 *
 *----------------------------------------------------------------------
 */

void
Timer_TimerServiceInterrupts(intrStatusPtr)
    unsigned int	*intrStatusPtr;		/* Copy of interrupt status 
						 * register.
						 */
{ 
    unsigned int modeRegister;
    unsigned int istatusReg = *intrStatusPtr;

    /*
     *  Determine if the callback and profile timers have expired.
     *
     *  The profile timer is checked first because routines on the callback
     *  queue might cause a delay in collecting profiling information.
     */
    DISABLE_INTR();
    if (istatusReg & PROFILE_TIMER_MASK_BIT) {
		/*
		 * Time to profile.  First make sure the timer didn't 
		 * accidentally get turned back on. 
		 * Race condition patch: Make sure the timer is really
		 * turned off by the interrupt so it won't both us again.
		 * Consider the following: when performing one of the several
		 * read-modify-writes of the mode register, a timer fires
		 * and clears its mode bit. When we write the mode register
		 * back we set the timer back on.  
		 */
	Mach_DisableNonmaskableIntr();
	modeRegister = Mach_Read8bitCCReg(MACH_MODE_REG);
	if (modeRegister & PROFILE_TIMER_MODE_BIT) {  
	    modeRegister &= (~PROFILE_TIMER_MODE_BIT & 0xff);
	    Mach_Write8bitCCReg(MACH_MODE_REG,modeRegister);
	}
	Mach_EnableNonmaskableIntr();
		/* 
		 * Reseting the interrupt bit in the status word is done in
		 * the routines calling us. 
		 */
		/*
		 * Collect the profile information.
		 */
	Prof_CollectInfo();
	RestartTimer(TIMER_PROFILE_TIMER);
     } 
     if (istatusReg & CALLBACK_TIMER_MASK_BIT) {
	/*
	 * Make sure the timer is off. See comment above.
	 */
	Mach_DisableNonmaskableIntr();
	modeRegister = Mach_Read8bitCCReg(MACH_MODE_REG);
	if (modeRegister & CALLBACK_TIMER_MODE_BIT) {  
	    modeRegister &= (~CALLBACK_TIMER_MODE_BIT & 0xff);
	    Mach_Write8bitCCReg(MACH_MODE_REG,modeRegister);
	}
	Mach_EnableNonmaskableIntr();
		/*
		 * Do the call back stuff. 
		 */
	Timer_CallBack(interval, time);

	RestartTimer(TIMER_CALLBACK_TIMER);
    }
    /*
     * Reset the two timer bits in the status register so we wont get called
     * twice when both timer's interrupt.
     */
    *intrStatusPtr = (*intrStatusPtr) & 
			~(CALLBACK_TIMER_MODE_BIT | PROFILE_TIMER_MODE_BIT);
    ENABLE_INTR();

    return;
}


/*
 *----------------------------------------------------------------------
 *
 * Timer_CounterInit --
 *
 *	Start the free running 64 bit counter on the Spur CC chip running.
 *	
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The T0 counter begin to count.
 *
 *----------------------------------------------------------------------
 */

void
Timer_CounterInit()
{
    unsigned int modeRegister;	/* Local copy of CC mode register */

    /*
     * Make sure the timer is not ticking.
     */
    Mach_DisableNonmaskableIntr();
    modeRegister = Mach_Read8bitCCReg(MACH_MODE_REG);
    if (modeRegister & FREERUNNING_TIMER_MODE_BIT) {
	modeRegister &= (~(FREERUNNING_TIMER_MODE_BIT) & 0xff);
	Mach_Write8bitCCReg(MACH_MODE_REG,modeRegister);
    }
    /*
     * Load the 64 bit register.
     */
    currentTicks = zeroCount;
    Timer_WriteT0(&zeroCount);
    /*
     * Start the counter running.
     */
    modeRegister = Mach_Read8bitCCReg(MACH_MODE_REG);
    modeRegister |= (FREERUNNING_TIMER_MODE_BIT);
    Mach_Write8bitCCReg(MACH_MODE_REG,modeRegister);

    processor[CLOCK_COUNTER] = Mach_GetProcessorNumber();
    Mach_EnableNonmaskableIntr();

}


/*
 *----------------------------------------------------------------------
 *
 * Timer_GetCurrentTicks --
 *
 *	Read the contents of the clock counter. 
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	CurrentTicks is updated if we are called on the processor with the
 *	counter.
 *
 *----------------------------------------------------------------------
 */

void
Timer_GetCurrentTicks(timePtr)
    Timer_Ticks	*timePtr;	/* Time from the counters. */
{
    MASTER_LOCK(&currentTicksMutex);
	/*
	 * Are we running on the CPU with the counter. If so then
	 * update the currentTicks structure.
	 */
    if (Mach_GetProcessorNumber() == processor[CLOCK_COUNTER]) {
	Timer_ReadT0(&currentTicks);
    }
    *timePtr = currentTicks;
    MASTER_UNLOCK(&currentTicksMutex);


}



/*  @#@#@#@#@#@#@#@#@#@#@    DEBUGGING CODE    @#@#@#@#@#@#@#@#@#@#@  */

/*
 *----------------------------------------------------------------------
 *
 * Timer_TimerGetInfo --
 *
 *	Reads the status and master mode registers of the chip and the
 *	specified counter's load, hold and mode registers and prints them
 *	on the console.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None
 *
 *----------------------------------------------------------------------
 */

void
Timer_TimerGetInfo(data)
    ClientData data;	/* Ignored. */
{
    Timer_Ticks		timer;
    ReturnStatus	status;
    unsigned	int	modeRegister;
    unsigned	int	imaskRegister;

    DISABLE_INTR();
    Timer_GetCurrentTicks(&timer);
    modeRegister = Mach_Read8bitCCReg(MACH_MODE_REG);
    imaskRegister = Mach_Read32bitCCReg(MACH_INTR_MASK_0);
    ENABLE_INTR();

    printf("CounterT0 0x%x 0x%x\n",timer.high, timer.low);
    printf("Mode Reg:\n");
    printf("TIMER ENABLE %d\tCALLBACK ENABLE %d\tPROFILE ENABLE %d\n",
		((modeRegister & FREERUNNING_TIMER_MODE_BIT) != 0),
		((modeRegister & CALLBACK_TIMER_MODE_BIT) != 0),
		((modeRegister & PROFILE_TIMER_MODE_BIT) != 0));

    printf("Mask Reg:\n");
    printf("CALLBACK MASK %d PROFILE MASK %d\n",
		((imaskRegister & CALLBACK_TIMER_MASK_BIT) != 0),
		((imaskRegister & PROFILE_TIMER_MASK_BIT) != 0));
}

/*
 *----------------------------------------------------------------------
 *
 * TimerHardwareUniversalTimeInit --
 *
 * 	Not implemented.
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
 *	Not implemented.
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

