/*
 * devTimer.c --
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
#include "devTimer.h"
#include "devTimerSpurInt.h"
#include "mach.h"
#include "time.h"

/*
 * The timers must be loaded with the number of ticks to count
 * before causing an interrupt. The number of ticks is calculated
 * from the timer's interrupt interval (in milliseconds) and
 * the timer's counting frequency in routine Dev_TimerInit().
 */

static	unsigned int callbackTicks;
static  unsigned int profileTicks;

/*
 * A zero valued counter.
 */
static	DevCounter	zeroCount = {0,0};

/*
 * Time per tick of high 32bits of counter.
 */
static Time		timeHigh;		

/*
 * The largest interval value.
 */
#define	MAXINT	((unsigned int ) 0xffffffff)

/*
 * Scale factor to make integer division more accurate. 
 */
#define	SCALE_FACTOR	100


/*
 *----------------------------------------------------------------------
 *
 * Dev_TimerInit --
 *
 *	Initialize the specified timer on the Cache Controller chip to
 *	cause interrupts at regular intervals. Initialize the call back
 *	interval ticks.
 *
 *	N.B. This routine must be called before Dev_TimerStart.
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
Dev_TimerInit(timer)
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

     if (timer == DEV_CALLBACK_TIMER) {
	intrMaskBit = CALLBACK_TIMER_MASK_BIT;
	modeRegBit = CALLBACK_TIMER_MODE_BIT;
	callbackTicks = (int) (TIMER_FREQ/1000.0 * DEV_CALLBACK_INTERVAL); 
#ifdef notdef
    } else if (timer == DEV_PROFILE_TIMER) {
	intrMaskBit = PROFILE_TIMER_MASK_BIT;
	modeRegBit = PROFILE_TIMER_MODE_BIT;
	profileTicks = (int) (TIMER_FREQ/1000.0 * DEV_PROFILE_INTERVAL); 
#endif
    } else {
	Sys_Panic(SYS_FATAL,"Dev_TimerInit: unknown timer %d\n", timer);
    }
    /*
     * Stop the timer in case it is ticking.
     */
    DISABLE_INTR();
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
    Mach_SetHandler(intrMaskBit,Dev_TimerServiceInterrupts);
    /*
     * Enable interrupts from the timer. 
     *
     * The current thinking is that the imask register should always be 
     * either on or off. In that cause we wont need this code.
     */
#ifdef notdef
    imaskRegister = Mach_Read32bitCCReg(MACH_INTR_MASK_0);
    imaskRegister |= intrMaskBit;
    Mach_Write32bitCCReg(MACH_INTR_MASK_0,imaskRegister);
#endif
    ENABLE_INTR();
}


/*
 *----------------------------------------------------------------------
 *
 * Dev_TimerStart --
 *
 *	Loads the specified timer with a new value to count from
 *	ands starts the timer.
 *
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
    register unsigned short timer;
{
    unsigned int modeRegister;		/* Local copy of mode register. */
    unsigned int modeRegBit = 0;	/* Timer's bit in mode register. */
    unsigned int timerAddress = 0;	/* Timer's addresses in CC. */
    int ticks = 0;			/* Number of ticks to set timer to. */

    if (timer == DEV_CALLBACK_TIMER) {
	modeRegBit = CALLBACK_TIMER_MODE_BIT;
	timerAddress = CALLBACK_TIMER_ADDR;
	ticks = callbackTicks;
#ifdef notdef
    } else if (timer == DEV_PROFILE_TIMER) {
	modeRegBit = PROFILE_TIMER_MODE_BIT;
	timerAddress = PROFILE_TIMER_ADDR;
	ticks = profileTicks;
#endif
    } else {
	Sys_Panic(SYS_FATAL,"Dev_TimerStart: unknown timer %d\n", timer);
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

#ifdef notdef
    /*
     * Setup the nonMaskable interupt for the PROFILE/refresh timer.
     */
    if (timer == DEV_PROFILE_TIMER) {
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

    if (timer == DEV_CALLBACK_TIMER) {
	modeRegBit = CALLBACK_TIMER_MODE_BIT;
	timerAddress = CALLBACK_TIMER_ADDR;
	ticks = callbackTicks;
#ifdef notdef
    } else if (timer == DEV_PROFILE_TIMER) {
	modeRegBit = PROFILE_TIMER_MODE_BIT;
	timerAddress = PROFILE_TIMER_ADDR;
	ticks = profileTicks;
#endif
    } else {
	Sys_Panic(SYS_FATAL,"RestartTimer: unknown timer %d\n", timer);
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
 * Dev_TimerInactivate --
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
Dev_TimerInactivate(timer)
    register unsigned short timer;
{
    unsigned int modeRegister;
    unsigned int imaskRegister;
    unsigned int modeRegBit = 0;
    unsigned int timerAddress = 0;
    unsigned int intrMaskBit = 0; 

    if (timer == DEV_CALLBACK_TIMER) {
	modeRegBit = CALLBACK_TIMER_MODE_BIT;
	timerAddress = CALLBACK_TIMER_ADDR;
	intrMaskBit = CALLBACK_TIMER_MASK_BIT;
#ifdef notdef
    } else if (timer == DEV_PROFILE_TIMER) {
	modeRegBit = PROFILE_TIMER_MODE_BIT;
	timerAddress = PROFILE_TIMER_ADDR;
	intrMaskBit = PROFILE_TIMER_MASK_BIT;
#endif
    } else {
	Sys_Panic(SYS_FATAL,"Dev_TimerStart: unknown timer %d\n", timer);
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
    /*
     * Currently, we leave the interrupt mask turned on at all times.
     */

#ifdef notdef
    imaskRegister = Mach_Read32bitCCReg(MACH_INTR_MASK_0);
    imaskRegister &= (~intrMaskBit) & 0xff;
    Mach_Write32bitCCReg(MACH_INTR_MASK_0,imaskRegister);
#endif

    ENABLE_INTR();

}

/*
 *----------------------------------------------------------------------
 *
 *  Dev_TimerServiceInterrupts --
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
Dev_TimerServiceInterrupts(intrStatusPtr)
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
	RestartTimer(DEV_PROFILE_TIMER);
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
	Timer_CallBack();

	RestartTimer(DEV_CALLBACK_TIMER);
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
 * Dev_CounterInit --
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
Dev_CounterInit()
{
    unsigned int modeRegister;	/* Local copy of CC mode register */

    /*
     * Initialized the number of seconds per tick of high word of 
     * counter.
     */

    timeHigh.seconds =  (int) (0xffffffff / TIMER_FREQ);
    timeHigh.microseconds = (int) (((0xffffffff % TIMER_FREQ) * SCALE_FACTOR) /
				((TIMER_FREQ * SCALE_FACTOR) / ONE_SECOND));

    DISABLE_INTR();
    /*
     * Make sure the timer is not ticking.
     */
    Mach_DisableNonmaskableIntr();
    modeRegister = Mach_Read8bitCCReg(MACH_MODE_REG);
    if (modeRegister & FREERUNNING_TIMER_MODE_BIT) {
	modeRegister &= (~(FREERUNNING_TIMER_MODE_BIT) & 0xff);
	Mach_Write8bitCCReg(MACH_MODE_REG,modeRegister);
    }
    Mach_EnableNonmaskableIntr();
    /*
     * Load the 64 bit register.
     */
    Dev_TimerT0Write(&zeroCount);
    /*
     * Start the counter running.
     */
    Mach_DisableNonmaskableIntr();
    modeRegister = Mach_Read8bitCCReg(MACH_MODE_REG);
    modeRegister |= (FREERUNNING_TIMER_MODE_BIT);
    Mach_Write8bitCCReg(MACH_MODE_REG,modeRegister);
    Mach_EnableNonmaskableIntr();
    ENABLE_INTR();

}


/*
 *----------------------------------------------------------------------
 *
 * Dev_CounterRead --
 *
 *	Read the contents of the free running counter.
 *	To be used with interrupts disabled to assure that the
 *	counter is read atomically.
 *
 * Results:
 *	The current value of the counter.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
Dev_CounterRead(counterPtr)
    DevCounter *counterPtr;
{
    ReturnStatus	status;

    DISABLE_INTR();
    status = Dev_TimerT0Read(counterPtr);
    if (status != SUCCESS) {
	Sys_Panic(SYS_FATAL,"Dev_CounterRead: Can not read counter T0\n");
    }
    ENABLE_INTR();
}


/*
 *----------------------------------------------------------------------
 *
 *  Dev_CounterIntToTime --
 *
 *      Converts an interval value into a standard time value.
 *
 *	This routine is meant for use by the Timer module only.
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
Dev_CounterIntToTime(interval, resultPtr)
    unsigned int interval;
    Time *resultPtr;
{
    DevCounter		tmp;
    unsigned int	overflow;

    Dev_CounterAddIntToCount(zeroCount,interval,&tmp,&overflow);
    Dev_CounterCountToTime(tmp,resultPtr);

}


/*
 *----------------------------------------------------------------------
 *
 *  Dev_CounterCountToTime --
 *
 *      Converts DevCounter into a standard time value.
 *
 *	This routine is meant for use by the Timer module only.
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
Dev_CounterCountToTime(count, resultPtr)
    DevCounter	count;
    Time *resultPtr;
{
    unsigned int leftOver;
    Time	tmp;

    resultPtr->seconds = (int) (count.low / TIMER_FREQ);
    leftOver = (unsigned int) (count.low - (resultPtr->seconds * TIMER_FREQ));
    resultPtr->microseconds = (int) ((leftOver * SCALE_FACTOR) /
				((TIMER_FREQ * SCALE_FACTOR) / ONE_SECOND));

    Time_Multiply(timeHigh,count.high,&tmp);
    Time_Add(*resultPtr, tmp, resultPtr);

}


/*
 *----------------------------------------------------------------------
 *
 *  Dev_CounterTimeToInt --
 *
 *      Converts a standard time value into an interval  value.
 *	This routine is meant for use by the Timer module only.
 *
 *  Results:
 *	A counter interval value.
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
    DevCounter		tmp;

    /*
     * Convert time to a DevCounter value 
     */
    Dev_CounterTimeToCount(time,&tmp);

    /*
     * Check to see if value is too bit for an interval.
     */
    if (tmp.high > INTERVAL_HIGH(MAXINT) || 
       ((tmp.high == INTERVAL_HIGH(MAXINT)) && 
	(tmp.low > INTERVAL_LOW(MAXINT)))) {
	Sys_Panic(SYS_WARNING, "Dev_CounterTimeToInt: time value too large\n");
	*resultPtr = MAXINT;
    } else {
        *resultPtr = COUNTER_TO_INTERVAL(tmp);
    }
}


/*
 *----------------------------------------------------------------------
 *
 *  Dev_CounterTimeToCount --
 *
 *      Converts a standard time value into an DevCounter  value.
 *	This routine is meant for use by the Timer module only.
 *
 *  Results:
 *	A DevCounter value.
 *
 *  Side Effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
Dev_CounterTimeToCount(time, resultPtr)
    Time time;
    DevCounter *resultPtr;
{
    unsigned int intervalLow;

    /*
     * Number of seconds in each DevCounter.high tick.
     */
#define	SECONDS_HIGH	((int)(MAXINT/TIMER_FREQ))

    resultPtr->high =  (time.seconds / SECONDS_HIGH);
    resultPtr->low = (int) ((time.seconds % SECONDS_HIGH) * TIMER_FREQ);
    intervalLow = (int) ((time.microseconds / SCALE_FACTOR) *
		((TIMER_FREQ * SCALE_FACTOR)/ONE_SECOND));
    resultPtr->low += intervalLow;
    if (resultPtr->low < intervalLow) {
	resultPtr->high += 1;
    }
#undef SECONDS_HIGH
}


/*
 *----------------------------------------------------------------------
 *
 *  Dev_CounterAddIntToCount --
 *
 *      Add an interval value to a DevCounter value returning a
 *	DevCounter value Int time units in the future. Specify in the
 *	variable overflow the number of times the addition caused the
 *	DevCounter to overflow.  
 *	
 *	This routine is meant for use by the Timer module only.
 *
 *  Results:
 *	A counter interval value.
 *
 *  Side Effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
Dev_CounterAddIntToCount(count, interval, resultPtr, overflowPtr)
    DevCounter	count;		/* Counter to add to */
    unsigned int	interval;	/* Interval to add */	
    DevCounter	*resultPtr;	/* Buffer to place the results */
    unsigned int	*overflowPtr;	/* Overflow count */
{
    unsigned	int	intervalLow;
    unsigned	int	intervalHigh;

    intervalLow = INTERVAL_LOW(interval);  
    intervalHigh = INTERVAL_HIGH(interval);
    resultPtr->low = count.low + intervalLow; 
    resultPtr->high = count.high + intervalHigh;

    if (resultPtr->low < intervalLow) {
	 resultPtr->high++;
    } 
    if (resultPtr->high < intervalHigh) {
	*overflowPtr = 1;
    } else {
	*overflowPtr = 0;
    }
}


/*  @#@#@#@#@#@#@#@#@#@#@    DEBUGGING CODE    @#@#@#@#@#@#@#@#@#@#@  */

/*
 *----------------------------------------------------------------------
 *
 * Dev_TimerGetInfo --
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
Dev_TimerGetInfo(data)
    ClientData data;	/* Ignored. */
{
    DevCounter		timer;
    ReturnStatus	status;
    unsigned	int	modeRegister;
    unsigned	int	imaskRegister;

    DISABLE_INTR();
    status = Dev_TimerT0Read(&timer);
    modeRegister = Mach_Read8bitCCReg(MACH_MODE_REG);
    imaskRegister = Mach_Read32bitCCReg(MACH_INTR_MASK_0);
    ENABLE_INTR();

    if (status == SUCCESS) {
	Sys_Printf("CounterT0 0x%x 0x%x\n",timer.high, timer.low);
    } else {
	Sys_Printf("CounterT0 CAN NOT READ\n");
    }
    Sys_Printf("Mode Reg:\n");
    Sys_Printf("TIMER ENABLE %d\tCALLBACK ENABLE %d\tPROFILE ENABLE %d\n",
		((modeRegister & FREERUNNING_TIMER_MODE_BIT) != 0),
		((modeRegister & CALLBACK_TIMER_MODE_BIT) != 0),
		((modeRegister & PROFILE_TIMER_MODE_BIT) != 0));

    Sys_Printf("Mask Reg:\n");
    Sys_Printf("CALLBACK MASK %d PROFILE MASK %d\n",
		((imaskRegister & CALLBACK_TIMER_MASK_BIT) != 0),
		((imaskRegister & PROFILE_TIMER_MASK_BIT) != 0));
}

