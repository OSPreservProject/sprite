/* dbgRs232.c -
 *
 *     This file contains routines that read, write, and initialize the
 *     Zilog RS232 chip.
 *
 * Copyright 1985 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "dev.h"
#include "devAddrs.h"
#include "dbg.h"
#include "dbgRs232.h"
#include "dbgInt.h"
#include "sys.h"
#include "mach.h"

/*
 * Channel A is setup to be the debuggers channel.  It gets configured
 * to be a polling device, so we avoid configuring the other channel
 * in the same way.  That would screw up any attached terminals.
 */
#define USE_CHAN_A


static Dev_ZilogDevice *zilogAddrA;	/* Struct to access channel A control
					   register */

static Dev_ZilogDevice *zilogAddrB;	/* Struct to access channel B control
					   register */


/*
 * ----------------------------------------------------------------------------
 *
 * DbgRs232Init --
 *
 *	Initialize the Zilog chip.
 *
 *	Currently, only channel A is used. Channel B is not initialized
 *	because it is used a serial port.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The two global zilog addresses for the two channels are initialized.
 *
 * ----------------------------------------------------------------------------
 */

void
DbgRs232Init()
{
#ifdef USE_CHAN_A
    zilogAddrA = (Dev_ZilogDevice *) (DEV_ZILOG_SERIAL_ADDR | 4);
#endif USE_CHAN_A

#ifdef USE_CHAN_B
    zilogAddrB = (Dev_ZilogDevice *) DEV_ZILOG_SERIAL_ADDR;
#endif USE_CHAN_B

    /*
     * Reset the channel.
     */

#ifdef USE_CHAN_A
    Dev_ZilogWriteReg(zilogAddrA, 9, WRITE9_RESET_CHAN_A); 
    MACH_DELAY(10);
#endif USE_CHAN_A

#ifdef USE_CHAN_B
    Dev_ZilogWriteReg(zilogAddrB, 9, WRITE9_RESET_WORLD); 
    MACH_DELAY(10);
#endif USE_CHAN_B

    /*
     * Initialize channel A.
     */

#ifdef USE_CHAN_A
    Dev_ZilogInit(zilogAddrA, DEV_ZILOG_SERIAL_SPEED);
#endif USE_CHAN_A

    /*
     * Initialize channel B.
     */

#ifdef USE_CHAN_B
    Dev_ZilogInit(zilogAddrB, DEV_ZILOG_SERIAL_SPEED);
#endif USE_CHAN_B
}


/*
 * ----------------------------------------------------------------------------
 *
 * DbgRs232ReadChar --
 *
 *     Return the next character that is available on the given channel.  If
 *     no character available then busy wait.
 *
 * Results:
 *     The next available character.
 *
 * Side effects:
 *     None.
 *
 * ----------------------------------------------------------------------------
 */
char
DbgRs232ReadChar(channel)
    Dbg_Rs232Channel	channel;	/* Channel to read from */
{
    int reg;

    /*
     * Busy wait until a character is ready.
     */

    while(1) {
	if (channel == DBG_RS232_CHANNELA) {
#ifdef USE_CHAN_A
	    reg = Dev_ZilogReadReg(zilogAddrA, 0); 
#else
	    printf("DbgRs232ReadChar called on channel A.\n");
	    return(0);
#endif USE_CHAN_A
	} else {
#ifdef USE_CHAN_B
	    reg = Dev_ZilogReadReg(zilogAddrB, 0); 
#else
	    printf("DbgRs232ReadChar called on channel B.\n");
	    return(0);
#endif USE_CHAN_B
	}
	if (reg & READ0_RX_READY) {
	    break;
	}
	DbgCheckNmis();
    }

    /*
     * Read the character.
     */

    if (channel == DBG_RS232_CHANNELA) {
	return(Dev_ZilogReadReg(zilogAddrA, 8));
    } else {
	return(Dev_ZilogReadReg(zilogAddrB, 8));
    }
}


/*
 * ----------------------------------------------------------------------------
 *
 * DbgRs232WriteChar --
 *
 *     Write a character to the given channel.  Will busy wait until the 
 *     transmit buffer is empty.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     None.
 *
 * ----------------------------------------------------------------------------
 */

void
DbgRs232WriteChar(channel, value)
    Dbg_Rs232Channel channel;	/* Channel to write to */
    char 	     value;	/* Value to write */
{
    int	reg;

    /*
     * Busy wait until the transmit buffer is empty.
     */

    while (1) {
	if (channel == DBG_RS232_CHANNELA) {
#ifdef USE_CHAN_A
	    reg = Dev_ZilogReadReg(zilogAddrA, 0);
#else
	    printf("DbgRs232WriteChar called on channel A.\n");
	    return;
#endif USE_CHAN_A
	} else {
#ifdef USE_CHAN_B
	    reg = Dev_ZilogReadReg(zilogAddrB, 0);
#else
	    printf("DbgRs232WriteChar called on channel B.\n");
	    return;
#endif USE_CHAN_B
	}
	if (reg & READ0_TX_READY) {
	    break;
	}
    }

    /*
     * Write the character
     */

    if (channel == DBG_RS232_CHANNELA) {
#ifdef USE_CHAN_A
	Dev_ZilogWriteReg(zilogAddrA, 8, (int) value);
#endif USE_CHAN_A
    } else {
#ifdef USE_CHAN_B
	Dev_ZilogWriteReg(zilogAddrB, 8, (int) value);
#endif USE_CHAN_B
    }
}
