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
#include "sys.h"
#include "mach.h"

static int RefreshCount = 0;


/*
 * Use timer T2 for refresh timer.
 */


#define	MACH_REFRESH_TIMER_MODE_BIT	MACH_MODE_T2_ENABLE
#define	MACH_REFRESH_TIMER_MASK_BIT	MACH_TIMER_T2_INTR
#define	MACH_REFRESH_TIMER_ADDR		MACH_T_20

/*
 * Cycles between refreshes. Around 100ms at 5.814 Mhz.
 */

#define	MACH_REFRESH_TIMER_TICKS	581395	
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
			(unsigned int) -MACH_REFRESH_TIMER_TICKS);

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

    /*
     * Stop the timer in case it is ticking.
     */
    modeRegister = Mach_Read8bitCCReg(MACH_MODE_REG);
    if (modeRegister & MACH_REFRESH_TIMER_MODE_BIT) { 
	modeRegister &= ((~MACH_REFRESH_TIMER_MODE_BIT) & 0xff);
	Mach_Write8bitCCReg(MACH_MODE_REG,modeRegister);
    }


    RefreshCount++;
    asm("cmp_trap	always,r0,r0,$3");
    /*
     * If switch 3 set then interrupt.
     */
    switches = read_physical_word(0x40000);
    if (switches & 0x4) {
	Sys_Panic(SYS_FATAL,"Aborted by DIP switch\n");
    }
    /*
     * Initialize the timer. Since the timers count up the a negative number
     * is loaded.
     */
    Mach_Write32bitCCReg((unsigned int)MACH_REFRESH_TIMER_ADDR,
			(unsigned int) -MACH_REFRESH_TIMER_TICKS);

    /*
     * Start the timer ticking. We re-read the mode register to narrow the
     * window for the race condition with the other counter reseting themselves.
     *
     */
    modeRegister = Mach_Read8bitCCReg(MACH_MODE_REG);
    modeRegister |= MACH_REFRESH_TIMER_MODE_BIT;
    Mach_Write8bitCCReg(MACH_MODE_REG,modeRegister);

}



