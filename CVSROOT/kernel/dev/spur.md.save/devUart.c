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
#include "uartConst.h"
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
Dev_UartInit(uartAddr, channel, baudRate)
    Address uartAddr;			/* address of uart registers (phys.) */
    register Dev_UartChannel channel;   /* which channel (A,B) to initialize */
    int baudRate;			/* speed to which to initialize
					   channel */
{
    Address uartChanAddr;
    int speed;

    if (channel == DEV_UART_CHANNEL_A) {
	uartChanAddr = ADDR_OFFSET(uartAddr, CHANNEL_A_OFFSET);
    } else if (channel == DEV_UART_CHANNEL_B) {
	uartChanAddr = ADDR_OFFSET(uartAddr, CHANNEL_B_OFFSET);
    } else {
	Sys_Panic(SYS_FATAL, "Dev_UartInit: illegal UART Channel");
	return;
    }
    /*
     * Reset channel.
     */
    Dev_UartWriteReg(uartChanAddr, COMMAND, TX_OFF|RX_OFF|RESET_MR1);
    Dev_UartWriteReg(uartChanAddr, COMMAND, TX_OFF|RX_OFF|RESET_RX);
    Dev_UartWriteReg(uartChanAddr, COMMAND, TX_OFF|RX_OFF|RESET_TX);
    Dev_UartWriteReg(uartChanAddr, COMMAND, TX_OFF|RX_OFF|EXIT_TX_BREAK);
    
    /*
     * Set up default values, to ignore counter and to have 1 stop
     * bit, have no flow control from other end, and have the
     * appropriate baud rate . Note that writing the mode register
     * after resetting it causes the MODE1 register to be written, and
     * subsequent writes go to MODE2.  For the baud rate, we map from
     * integers into the particular constants the uart expects.
     */
    Dev_UartWriteReg(uartAddr, AUX_CTL, AUX_CMD_VAL);
    Dev_UartWriteReg(uartChanAddr, MODE, MODE1_VAL);
    Dev_UartWriteReg(uartChanAddr, MODE, MODE2_VAL);
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

    Dev_UartWriteReg(uartChanAddr, BAUD, speed);

    /*
     * Disable all interrupts and clear status bits.
     */
    Dev_UartWriteReg(uartAddr, INTR_MASK, 0);
    Dev_UartWriteReg(uartChanAddr, COMMAND, TX_OFF|RX_OFF|CLR_ERR_STATUS);
    Dev_UartWriteReg(uartChanAddr, COMMAND, TX_OFF|RX_OFF|CLR_BRK_STATUS);

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
    Address uartAddr;
    Address uartChanAddr;
    Dev_UartChannel channel;
    int status;

    infoPtr = (Dev_UartInfo *) uartPtr;
    uartAddr = infoPtr->uartAddress;
    channel = infoPtr->channel;

    if (channel == DEV_UART_CHANNEL_A) {
	uartChanAddr = ADDR_OFFSET(uartAddr, CHANNEL_A_OFFSET);
    } else if (channel == DEV_UART_CHANNEL_B) {
	uartChanAddr = ADDR_OFFSET(uartAddr, CHANNEL_B_OFFSET);
    } else {
	Sys_Panic(SYS_FATAL, "Dev_UartStartTx: illegal UART Channel");
	return;
    }
    status = Dev_UartReadReg(uartChanAddr, XFER_STATUS);
    if (status & TX_NOT_FULL) {
	Dev_UartWriteReg(uartChanAddr, COMMAND, RX_ON | TX_ON);
	Dev_UartWriteReg(uartChanAddr, XFER_REG, ch);
    }
}
