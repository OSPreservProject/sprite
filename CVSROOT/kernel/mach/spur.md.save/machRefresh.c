/*
 * machRefresh.c --
 *
 *	This file contains routines that control the refresh of the SPUR 
 *      CPU's dynamic registers.
 *
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

#include "sprite.h"
#include "mach.h"
#include "prof.h"
#include "sched.h"

static int RefreshCount[MACH_MAX_NUM_PROCESSORS];


/*
 * Use timer T2 for refresh timer.
 */


#define	MACH_REFRESH_TIMER_MODE_BIT	MACH_MODE_T2_ENABLE
#define	MACH_REFRESH_TIMER_MASK_BIT	MACH_TIMER_T2_INTR
#define	MACH_REFRESH_TIMER_ADDR		MACH_T_20

/*
 * Cycles between refreshes. Around 100ms at 5.814 Mhz.
 */

#if 0
#define	MACH_REFRESH_TIMER_TICKS	581395	 
#endif
#define	MACH_REFRESH_TIMER_TICKS	40000	


/*
 * Cycles between profile pc sample. Around 20ms at 5.814 Mhz.
 */

#define	MACH_PROFILE_TIMER_TICKS	116279

extern Boolean profEnabled;
extern unsigned int machInterruptAddr;
extern   unsigned int *machIdleCountPtr[MACH_MAX_NUM_PROCESSORS];
extern int 	machLEDValues[MACH_MAX_NUM_PROCESSORS];

/*
 * Value to stick into counter.
 */

static	int	counterTicks = -MACH_REFRESH_TIMER_TICKS;

/*
 *----------------------------------------------------------------------
 *
 * Mach_RefreshStart --
 *
 *	Initialize the refresh timer on the Cache Controller chip to
 *	cause interrupts at regular intervals. 
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The specified timer is initialized and started. NonMaskable interrupts
 *	are enabled.
 *
 *----------------------------------------------------------------------
 */

void
Mach_RefreshStart()
{
     unsigned int modeRegister;		/* Local copy of CC mode register */
     int      i;

     for (i = 0; i < MACH_MAX_NUM_PROCESSORS; i++) {
	machIdleCountPtr[i] = &(sched_Instrument.processor[i].idleTicksLow);
     }
    /*
     * Stop the timer in case it is ticking.
     */
    modeRegister = Mach_Read8bitCCReg(MACH_MODE_REG);
    if (modeRegister & MACH_REFRESH_TIMER_MODE_BIT) { 
	modeRegister &= ((~MACH_REFRESH_TIMER_MODE_BIT) & 0xff);
	Mach_Write8bitCCReg(MACH_MODE_REG,modeRegister);
    }

    /*
     * Reset the timer's bit in the CC's Istatus register. Note that the
     * Istatus register always writes by anding the ones complement of the
     * data. ([Wood87] Section 5.1)
     */
    Mach_Write32bitCCReg((unsigned int)MACH_INTR_STATUS_0,
			 (unsigned int)MACH_REFRESH_TIMER_MASK_BIT);

    /*
     * Initialize the timer. Since the timers count up the a negative number
     * is loaded.
     */
    Mach_Write32bitCCReg((unsigned int)MACH_REFRESH_TIMER_ADDR,
			(unsigned int) counterTicks);

    /*
     * Setup the monMaskable interrupt for the refresh timer.
     */
    Mach_SetNonmaskableIntr((unsigned int)MACH_REFRESH_TIMER_MASK_BIT);
    /*
     * Start the timer ticking. We re-read the mode register to narrow the
     * window for the race condition with the other counter reseting themselves.
     *
     */
    modeRegister = Mach_Read8bitCCReg(MACH_MODE_REG);
    modeRegister |= MACH_REFRESH_TIMER_MODE_BIT;
    Mach_Write8bitCCReg(MACH_MODE_REG,modeRegister);

    Mach_EnableNonmaskableIntr();
}

/*
 *----------------------------------------------------------------------
 *
 * Mach_RefreshInterrupt --
 *
 *  Service a refresh interrupt and restart the refresh timer.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The refresh timer is re-initialized and started. 
 *
 *----------------------------------------------------------------------
 */

void
Mach_RefreshInterrupt()
{
     unsigned int modeRegister;		/* Local copy of CC mode register */
     int	switches;
     int	count;
     int	cpu;

    /*
     * Stop the timer in case it is ticking.
     */
    modeRegister = Mach_Read8bitCCReg(MACH_MODE_REG);
    if (modeRegister & MACH_REFRESH_TIMER_MODE_BIT) { 
	modeRegister &= ((~MACH_REFRESH_TIMER_MODE_BIT) & 0xff);
	Mach_Write8bitCCReg(MACH_MODE_REG,modeRegister);
    }


    cpu = Mach_GetProcessorNumber();
    count = RefreshCount[cpu]++;
    asm("cmp_trap	always,r0,r0,$3");
    /*
     * If switch 3 set then interrupt.
     */
    switches = read_physical_word(0x40000);
    if (switches & 0x4) {
	panic("Aborted by DIP switch\n");
    }
    if (profEnabled) {
	counterTicks = -MACH_PROFILE_TIMER_TICKS;
    } else {
	counterTicks = -MACH_REFRESH_TIMER_TICKS;
    }
    /*
     * Initialize the timer. Since the timers count up the a negative number
     * is loaded.
     */
    Mach_Write32bitCCReg((unsigned int)MACH_REFRESH_TIMER_ADDR,
			(unsigned int) counterTicks);

    /*
     * Start the timer ticking. We re-read the mode register to narrow the
     * window for the race condition with the other counter reseting themselves.
     *
     */
    modeRegister = Mach_Read8bitCCReg(MACH_MODE_REG);
    modeRegister |= MACH_REFRESH_TIMER_MODE_BIT;
    Mach_Write8bitCCReg(MACH_MODE_REG,modeRegister);

    if (profEnabled) {
	 Prof_CollectInfo(machInterruptAddr); 
    }
    /* 
     * Show 0x42 for Sprite on leds. 
     */
    {
	    extern Boolean rpcServiceEnabled;
	    static int refresh_leds[] = { 0x1000, 0x7000, 0x1000, 0x7000 };
	    static int idle_leds[] = { 0x0e00, 0x0d00, 0x0b00, 0x0700,
	    			       0xb00, 0xd00, 0xe00, 0xf00};
	    if (rpcServiceEnabled) { 
		    write_physical_word(0x20000,
			refresh_leds[(count>>6)&0x3] |
			machLEDValues[cpu] |
			idle_leds[(*machIdleCountPtr[cpu]>>16)&0x7]);
	    }
    }

}



