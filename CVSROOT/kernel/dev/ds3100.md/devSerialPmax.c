/* 
 *  devSerialPmax.c --
 *
 *     	This file contains machine-dependent routines that handle the
 *	output queue for the serial lines.
 *
 *	Copyright (C) 1989 Digital Equipment Corporation.
 *	Permission to use, copy, modify, and distribute this software and
 *	its documentation for any purpose and without fee is hereby granted,
 *	provided that the above copyright notice appears in all copies.  
 *	Digital Equipment Corporation makes no representations about the
 *	suitability of this software for any purpose.  It is provided "as is"
 *	without express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (DECWRL)";
#endif not lint

#include "sprite.h"
#include "machMon.h"
#include "mach.h"
#include "devKeyboard.h"
#include "devKeyboardInt.h"
#include "devKbdQueue.h"
#include "devConsole.h"
#include "devSerial.h"
#include "dev.h"
#include "fs.h"
#include "sys.h"
#include "sync.h"
#include "timer.h"
#include "dbg.h"
#include "kbdMapMach.h"
#include "dc7085.h"
#include "devGraphics.h"
#include "machAddrs.h"
#include "serial.h"

/* 
 * Instrumentation to keep track of the number of interrupts and other
 * interesting information.
 */
Dev_KbdInstrument dev_KbdInstrument;	

#define REG_ADDR(offset) (unsigned short *)(MACH_SERIAL_INTERFACE_ADDR + (offset))

/*
 * Define the six registers.
 */
volatile unsigned short *devCSRPtr =  REG_ADDR(0x00);
volatile unsigned short *devRBufPtr = REG_ADDR(0x08);
volatile unsigned short *devLPRPtr =  REG_ADDR(0x08);
volatile unsigned short *devTCRPtr =  REG_ADDR(0x10);
volatile unsigned short *devMSRPtr =  REG_ADDR(0x18);
volatile unsigned short *devTDRPtr =  REG_ADDR(0x18);

/*
 * The last character that we received.
 */
static unsigned char lastChar = 0;
static unsigned int  lastSpecial;


/*
 * ----------------------------------------------------------------------------
 *
 * Dev_KbdInit --
 *
 *	Initialize all four channels of the chip.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */
void
Dev_KbdInit()
{
    DevKbdQueueInit();
    bzero((Address) &dev_KbdInstrument,sizeof(dev_KbdInstrument));

    *devCSRPtr = CSR_CLR;
    while ((*devCSRPtr & CSR_CLR) != 0) {
    }
    /*
     * Initialize the keyboard.
     */
    *devLPRPtr = LPR_RXENAB | LPR_B4800 | LPR_8_BIT_CHAR | 
		 KBD_PORT;
    MACH_DELAY(100);
    /*
     * Enable receive interrupts.
     */
    *devCSRPtr = CSR_MSE | CSR_RIE;
}    

static void RecvIntr(), XmitIntr(), KbdServiceInterrupt();

/*
 * ----------------------------------------------------------------------------
 *
 * Dev_DC7085Interrupt --
 *
 *	Service an interrupt from the uart.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Adds serial, keyboard or mouse events to the queue.
 *
 * ----------------------------------------------------------------------------
 */
void
Dev_DC7085Interrupt()
{
    if (devGraphicsOpen) {
	DevGraphicsInterrupt();
    } else {
	KbdServiceInterrupt();
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 * KbdServiceInterrupt --
 *
 *	Service an interrupt from the uart.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Adds serial, keyboard or mouse events to the queue.
 *
 * ----------------------------------------------------------------------------
 */
static void
KbdServiceInterrupt()
{
    unsigned csr;

    MASTER_LOCK(&devKbdMutex);

    csr = *devCSRPtr;

    if (csr & CSR_RDONE) {
	RecvIntr();
    }

    if (csr & CSR_TRDY) {
	XmitIntr();
    }

    MASTER_UNLOCK(&devKbdMutex);
}

static	Dev_KbdEvent	keyEvent;


/*
 *----------------------------------------------------------------------
 *
 * RecvIntr --
 *
 *	Process a received character.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Events added to the queue.
 *
 *----------------------------------------------------------------------
 */
static void
RecvIntr()
{
    unsigned short	recvBuf;

    while (*devCSRPtr & CSR_RDONE) {
	recvBuf = *devRBufPtr;
	switch ((recvBuf & RBUF_LINE_NUM) >> RBUF_LINE_NUM_SHIFT) {
	    case KBD_PORT: {
		unsigned char	ch;

		dev_KbdInstrument.numKbdChars++;
		ch = *devRBufPtr & 0xFF;
		if (ch != KEY_REPEAT) {
		    keyEvent.device = DEV_KBD_KEY;
		    if (ch == KEY_UP) {
			keyEvent.key = lastSpecial;
			keyEvent.keyUp = TRUE;
		    } else {
			if (ch == KEY_SHIFT) {
			    lastSpecial = KEY_SHIFT;
			} else if (ch == KEY_CONTROL) {
			    lastSpecial = KEY_CONTROL;
			}
			keyEvent.key = ch;
			keyEvent.keyUp = FALSE;
		    }
		    lastChar = ch;
		}
		Timer_GetCurrentTicks(&(keyEvent.tickStamp));
		DevKbdQueueAddInput(&keyEvent);
		break;
	    }
	    case MOUSE_PORT:
		break;
	    case MODEM_PORT:
		break;
	    case PRINTER_PORT:
		break;
	}
    }
}


/*
 *----------------------------------------------------------------------
 *
 * XmitIntr --
 *
 *	Handle a transmission interrupt.
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
XmitIntr()
{
}


/*
 *----------------------------------------------------------------------
 *
 * Dev_SerialInitAddr --
 *
 *	Initializes the address of the device for a serial port.
 *	Called from Dev_KbdInit at boot time.
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
Dev_SerialInitAddr(unit, devAddr)
    int			unit;
    Address		devAddr;
{
    DevSerialInitQueue(unit, (ClientData) devAddr);
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
/*ARGSUSED*/
void
Dev_UartStartTx(uartPtr, ch)
    ClientData uartPtr;
    char ch;
{
}

/*
 *----------------------------------------------------------------------
 *
 * Dev_KbdConsole --
 *
 *      Read the EEPROM to determine which device is used as
 *      the system console.
 *
 * Results:
 *      Returns the device number.  A -1 is returned if the
 *      device can't be determined.
 * 
 * Side effects:
 *      None.
 *  
 *----------------------------------------------------------------------
 */ 

int
Dev_KbdConsole()
{
    return(-1);
}
