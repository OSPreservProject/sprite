/* devUart.c -
 *
 *     This file contains routines that read, write, and initialize 
 *     the SPUR Signetics SCN68681 Dual Universal Asynchronous
 *     Receiver/Transmitter (DUART) chip.
 *
 *     For an explanation of the use of the chip see the "SPUR Memory
 *     System Architecture", Report. UCB/CSD 87/394.
 *
 * Copyright 1988 Regents of the University of California
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
#endif not lint

#include "sprite.h"
#include "mach.h"
#include "dev.h"
#include "sys.h"


/*
 * ----------------------------------------------------------------------------
 *
 * Dev_UartInit --
 *
 *     Initialize the given channel of the Uart chip.
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
Dev_UartInit(uartPtr, channel, baudRate)
    register Dev_UartDevice *uartPtr;   /* pointer to the interface
					   registers */
    register Dev_UartChannel channel;   /* which channel (A,B) to initialize */
    int baudRate;			/* speed to which to initialize
					   channel */
{
    int speed;
    Dev_UartWriteChannel *chanPtr;	/* pointer to channel-dependent
					   part of structure */
    if (channel == DEV_UART_CHANNEL_A) {
	chanPtr = &uartPtr->channelA.write;
    } else if (channel == DEV_UART_CHANNEL_B) {
	chanPtr = &uartPtr->channelB.write;
    } else {
	Sys_Panic(SYS_FATAL, "Dev_UartInit: illegal UART Channel");
	return;
    }
    /*
     * Reset both channels.
     */
    chanPtr->command = TX_OFF|RX_OFF|RESET_MR1;
    chanPtr->command = TX_OFF|RX_OFF|RESET_RX;
    chanPtr->command = TX_OFF|RX_OFF|RESET_TX;
    chanPtr->command = TX_OFF|RX_OFF|EXIT_TX_BREAK;
    
    /*
     * Set up default values, to ignore counter and to have 1 stop
     * bit, have no flow control from other end, and have the
     * appropriate baud rate . Note that writing the mode register
     * after resetting it causes the MODE1 register to be written, and
     * subsequent writes go to MODE2.  For the baud rate, we map from
     * integers into the particular constants the uart expects.
     */
    uartPtr->misc.write.auxCtl = AUX_CMD_VAL;
    chanPtr->mode = MODE1_VAL;
    chanPtr->mode = MODE2_VAL;
    switch (baudRate) {
	case 9600: {
	    speed = BAUD_9600;
	    break;
	}
	case 2400: {
	    speed = BAUD_2400;
	    break;
	}
	case 1200: {
	    speed = BAUD_1200;
	    break;
	}
	case 110: {
	    speed = BAUD_110;
	    break;
	}
	case 19200: {
	    speed = BAUD_19200;
	    break;
	}
	default: {
	    Sys_Panic(SYS_FATAL, "Dev_UartInit: illegal baud rate");
	    return;
	}
    }

    chanPtr->baudRate = speed;

    /*
     * Disable all interrupts and clear status bits.
     */
    uartPtr->misc.write.intrMask = 0;
    chanPtr->command = TX_OFF|RX_OFF|CLR_ERR_STATUS;
    chanPtr->command = TX_OFF|RX_OFF|CLR_BRK_STATUS;

    /*
     * Reset the uart's bit in the CC's Istatus register. 
     */
    Mach_Write32bitCCReg(MACH_INTR_STATUS_0, MACH_UART_INTR);
    /*
     * Register the routine to call on this interrupt.
     */
    Mach_SetHandler(MACH_UART_INTR, Dev_KbdServiceInterrupt);
}


/*
 *----------------------------------------------------------------------
 *
 * Dev_UartStartTx --
 *
 *	Causes the uart to start printing characters, starting
 *	with the character specified.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A "Transmit Buffer Empty" interrupt is enabled. A character
 *	is printed.
 *
 *----------------------------------------------------------------------
 */

void
Dev_UartStartTx(uartPtr, ch)
    ClientData uartPtr;
    char ch;
{
    Dev_UartInfo *infoPtr;
    Dev_UartDevice *uartAddr;
    Dev_UartWriteChannel *writePtr;
    Dev_UartReadChannel *readPtr;

    infoPtr = (Dev_UartInfo *) uartPtr;
    uartAddr = infoPtr->uartAddress;
    
    readPtr = &uartAddr->channelA.read;
    if (readPtr->status & TX_NOT_FULL) {
	writePtr = (Dev_UartWriteChannel *) readPtr;
	writePtr->command = RX_ON | TX_ON;
	writePtr->transmit = ch;
    }
}
