/*
 * timerIntersilInt.h --
 *
 *	Types and constants for the Intersil ICM7170 real-time clock chip.
 *
 *	The definitions which define the bit fields for each register are
 *	not fully explained in this file.  For a detailed explanation of the 
 *	chip's functionality and the definitions listed below, see the
 *	data sheet for the Intersil ICM7170 (order #301680-005, Dec. 1985).
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _DEVTIMERINT
#define _DEVTIMERINT



/*
 *-----------------------------------------------------------------
 *
 * Command Register bit assigments:  (p. 6)
 *
 *	The command register is 8 bits wide and is write-only. 
 *	The 2 most significant bits (D7, D6) are not used.
 *	One of the defines for each bit group must be used to compose
 *	a command.
 *
 */

/* bits 0,1 (least significant bits) */
#define FREQ_32K	0x00
#define FREQ_1M		0x01
#define FREQ_2M		0x02
#define FREQ_4M		0x03

/* bit 2 */
#define H12_HOUR_MODE	0x00
#define H24_HOUR_MODE	0x04

/* bit 3 */
#define STOP		0x00
#define RUN		0x08

/* bit 4 */
#define INTR_ENABLE	0x10
#define INTR_DISABLE	0x00

/* bit 5 */
#define NORMAL_MODE	0x00
#define TEST_MODE	0x20

/*
 * IntersilCommand macro is used to start or stop the clock and
 * enable or disable interrupts from the chip. 
 * The run argument can be one of RUN, STOP.
 * The intr argument can be one of INTR_ENABLE, INTR_DISABLE.
 */

#define IntersilCommand(run, intr)	 \
	(run | intr | FREQ_32K | H24_HOUR_MODE | NORMAL_MODE)



/*
 *-----------------------------------------------------------------
 *
 *  Interrupt Mask and Status Register -- (pp. 6 and 7) 
 *
 *	
 *	The interrupt register is 8 bits wide. Writing to the
 *	register enables or disables interrupts from one or more  
 *	periodic counters. Reading the register is used to determine 
 *	which counter caused the interrupt.
 *
 *	There is an alarm register that can be used to cause an interrupt
 *	when the time equals the value in the alarm register. This
 *	feature is enabled with the ALARM_MASK.
 *
 */

#define ALARM_MASK	0x01
#define S100TH_SEC_MASK	0x02
#define S10TH_SEC_MASK	0x04
#define SECOND_MASK	0x80
#define MINUTE_MASK	0x10
#define HOUR_MASK	0x20
#define DAY_MASK	0x40
#define INTR_PENDING_MASK	0x80


/*
 *-----------------------------------------------------------------
 *
 *  Intersil ICM7170 --
 *
 *	The chip's registers are composed of two sets of counters,
 *	an interrupt mask/status register and a command register.
 */

typedef struct {
    unsigned char	hundredths;
    unsigned char	hours;
    unsigned char	minutes;
    unsigned char	seconds;
    unsigned char	month;
    unsigned char	day;
    unsigned char	year;
    unsigned char	dayOfWeek;
} IntersilCounters;

typedef struct {
    IntersilCounters		counter;
    IntersilCounters		alarm;
    unsigned char		interruptReg;
    unsigned char		commandReg;
} TimerDevice;

#endif _DEVTIMERINT
