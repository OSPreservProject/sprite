/* 
 *  devKeyboard.c --
 *
 *	This file contains routines that initialize and read 
 *	the Signetics uart chip for the two serial lines.  
 *	Interrupts are serviced and the input characters are
 *	stored in unprocessed form in a buffer. 
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
#endif

#include "sprite.h"
#include "machMon.h"
#include "mach.h"
#include "devKeyboard.h"
#include "devKeyboardInt.h"
#include "devKbdQueue.h"
#include "devConsole.h"
#include "devSerial.h"
#include "devAddrs.h"
#include "dev.h"
#include "uartConst.h"
#include "fs.h"
#include "sys.h"
#include "sync.h"
#include "timer.h"

#include "dbg.h"

/*
 * Store the address of the uart for the serial line channels. If a keyboard
 * and mouse are ever added, this will need to be extended.
 */

static Address uartAddr;		/* Struct to access the uart (both
					   channels A and B). */

/* 
 * Instrumentation to keep track of the number of interrupts and other
 * interesting information, plus a static pointer into one of its fields.
 */

Dev_KbdInstrument dev_KbdInstrument;	
static Time *kbdMostRecentIntrPtr = &dev_KbdInstrument.mostRecentInterrupt;

/*
 * Count of extraneous interrupts that have been received
 * since booting.
 */

int dev_KbdExtraIntrs = 0;



/*
 * ----------------------------------------------------------------------------
 *
 * Dev_KbdInit --
 *
 *	Initializes both channels of the keyboard and serial Uart chips.
 *	(The keyboard chip is nonexistent for the indefinite future.)
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The global uart addresses for the 4 (2) channels are initialized.
 *
 * ----------------------------------------------------------------------------
 */

void
Dev_KbdInit()
{
    DevKbdQueueInit();

    bzero((Address) &dev_KbdInstrument, sizeof(dev_KbdInstrument));

#ifdef notdef
    mouseUartAddr = (volatile Dev_UartDevice *) DEV_KBD_ADDR;
    kbdUartAddr	= (volatile Dev_UartDevice *) (DEV_KBD_ADDR | 4);
#endif
    uartAddr = (Address) DEV_UART_SERIAL_ADDR;


    Dev_SerialInitAddr(DEV_SERIAL_A_UNIT, uartAddr);
    Dev_SerialInitAddr(DEV_SERIAL_B_UNIT, uartAddr);
#ifdef notdef
    /*
     * Initialize for output to the keyboard chip.
     */
    Dev_SerialInitAddr(DEV_EVENT_UNIT, kbdUartAddr);

    /*
     * Initialize keyboard and mouse channels.
     */
    Dev_UartInit(kbdUartAddr, DEV_UART_CHANNEL_A, KBD_SPEED);
    Dev_UartInit(mouseUartAddr, DEV_UART_CHANNEL_B, MOUSE_SPEED);
#endif notdef

    /*
     * Initialize serial channels.
     */
    Dev_UartInit(uartAddr, DEV_UART_CHANNEL_A, DEV_UART_SERIAL_SPEED);
    Dev_UartInit(uartAddr, DEV_UART_CHANNEL_B, DEV_UART_SERIAL_SPEED);

    /*
     * Turn on interrupts.  For the serial ports, enable Receive and Transmit
     * Buffer Empty interrupts.
     *
     * Note: the set of interrupts to enable is unclear.
     */
    Dev_UartWriteReg(ADDR_OFFSET(uartAddr, CHANNEL_A_OFFSET), COMMAND,
		     TX_ON|RX_ON|RESET_RX|RESET_TX);
    Dev_UartWriteReg(ADDR_OFFSET(uartAddr, CHANNEL_B_OFFSET), COMMAND,
		     TX_ON|RX_ON|RESET_RX|RESET_TX);
    Dev_UartWriteReg(uartAddr, INTR_MASK,
		     B_FIFO_INTR|B_TX_RDY|A_FIFO_INTR|A_TX_RDY);
}    


#ifdef notdef
/*
 * Definition of states for mouse input:
 *	MOUSE_WAIT_SYNC		waiting for a sync byte
 *	MOUSE_WAIT_DELTA_X1	waiting for first delta x
 *	MOUSE_WAIT_DELTA_Y1	waiting for first delta y
 *	MOUSE_WAIT_DELTA_X2	waiting for second delta x
 *	MOUSE_WAIT_DELTA_Y2	waiting for second delta y
 *
 * States are defined constants to allow for modulo arithmetic.
 */

#define MOUSE_NUM_STATES	5

#define MOUSE_WAIT_SYNC		0
#define MOUSE_WAIT_DELTA_X1 	1
#define MOUSE_WAIT_DELTA_Y1 	2
#define MOUSE_WAIT_DELTA_X2 	3
#define MOUSE_WAIT_DELTA_Y2 	4
#endif


/*
 * ----------------------------------------------------------------------------
 *
 * Dev_KbdServiceInterrupt --
 *
 *	Service an interrupt from the uart.  Only look for a interrupts caused
 *	by the serial ports.  Eventually, if appropriate, add the mouse
 * 	and keyboard.
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
Dev_KbdServiceInterrupt()
{
#ifdef notdef
    static int		mouseState = MOUSE_WAIT_SYNC;
    static Dev_KbdEvent	mouseEvent;
#endif
    int			intrStatus;
    Boolean		gotValidIntr = FALSE;
    Boolean		gotInput = FALSE;	/* set to true only if
						   interrupt was for output */
    dev_KbdInstrument.numInterrupts++;

    MASTER_LOCK(&devKbdMutex);

    /*
     * Check the serial port Uart chip for interrupts.
     */
    intrStatus = Dev_UartReadReg(uartAddr, INTR_STATUS);

    if (intrStatus & A_FIFO_INTR) {
	Dev_KbdEvent	event;

	/*
	 * Received a character from the A serial port.  Note, reading the
	 * character implicitly clears the interrupt.
	 */

	event.device = DEV_KBD_SERIAL_A;
	event.key = Dev_UartReadReg(ADDR_OFFSET(uartAddr, CHANNEL_A_OFFSET),
				    XFER_REG);
	DevKbdQueueAddInput(&event);

	gotValidIntr = TRUE;
	gotInput = TRUE;
    }

    if (intrStatus & A_TX_RDY) {
	char c;
	/*
	 * Just got the "transmit buffer is empty" interrupt. This means
	 * we can send another character if one is available. If there is
	 * a character available, then stuff it into the transmit buffer.
	 * If there aren't any characters to print at this moment, reset
	 * the interrupt for the time being. When characters become
	 * available for printing then the interrupt will be reenabled.
	 *
	 * Writing another character clears the interrupt, but it is
	 * not clear from the MSA whether disabling TX clears it as well.
	 * We may have to play tricks to disable the interrupt.
	 */
	if (Dev_SerialGetOutChar(DEV_SERIAL_A_UNIT, &c) == SUCCESS) {
	    Dev_UartWriteReg(ADDR_OFFSET(uartAddr, CHANNEL_A_OFFSET),
			     XFER_REG, c);
	} else {
	    Dev_UartWriteReg(ADDR_OFFSET(uartAddr, CHANNEL_A_OFFSET),
			     COMMAND, TX_OFF);
	}

	gotValidIntr = TRUE;
    }


    if (intrStatus & B_FIFO_INTR) {
	Dev_KbdEvent	event;

	/*
	 * Received a character from the B serial port.  Note, reading the
	 * character implicitly clears the interrupt.
	 */

	event.device	= DEV_KBD_SERIAL_B;
	event.key	= Dev_UartReadReg(ADDR_OFFSET(uartAddr,
						      CHANNEL_B_OFFSET),
					  XFER_REG);
	DevKbdQueueAddInput(&event);

	gotValidIntr = TRUE;
	gotInput = TRUE;
    }

    if (intrStatus & B_TX_RDY) {
	char c;
	/*
	 * Just got the "transmit buffer is empty" interrupt. This means
	 * we can send another character if one is available. If there is
	 * a character available, then stuff it into the transmit buffer.
	 * If there aren't any characters to print at this moment, reset
	 * the interrupt for the time being. When characters become
	 * available for printing then the interrupt will be reenabled.
	 *
	 * Writing another character clears the interrupt, but it is
	 * not clear from the MSA whether disabling TX clears it as well.
	 * We may have to play tricks to disable the interrupt.
	 */
	if (Dev_SerialGetOutChar(DEV_SERIAL_B_UNIT, &c) == SUCCESS) {
	    Dev_UartWriteReg(ADDR_OFFSET(uartAddr, CHANNEL_B_OFFSET),
			     XFER_REG, c);
	} else {
	    Dev_UartWriteReg(ADDR_OFFSET(uartAddr, CHANNEL_B_OFFSET),
			     COMMAND, TX_OFF);
	}

	gotValidIntr = TRUE;
    }


    /*
     * Now examine the keyboard/mouse chip to see if it interrupted.
     * [Needs to be converted to Signetics instead of Zilog format
     * if ever un-ifdef'ed.]
     */
#ifdef notdef
    intrStatus  = Dev_UartReadReg(kbdUartAddr, 3);
    if (intrStatus & READ3_RX_IP_B) {

	dev_KbdInstrument.numMouseChars++;
	character = ((DevKbdUart *) mouseUartAddr)->data;

	if (mouseState == MOUSE_WAIT_SYNC && ((character & 0xF0) != 0x80)) {
	    dev_KbdInstrument.numBadSyncs++;
	} else {
	    switch (mouseState) {
		case MOUSE_WAIT_SYNC:
		    mouseEvent.key = character & 0x07;
		    break;
		case MOUSE_WAIT_DELTA_X1:
		    mouseEvent.deltaX = (short) character;
		    break;
		case MOUSE_WAIT_DELTA_Y1:
		    mouseEvent.deltaY = (short) character;
		case MOUSE_WAIT_DELTA_X2:
		    mouseEvent.deltaX += (short) character;
		    break;
		case MOUSE_WAIT_DELTA_Y2:
		    mouseEvent.deltaY += (short) character;
		    mouseEvent.device  = DEV_KBD_MOUSE;
		    Timer_GetCurrentTicks(&(mouseEvent.tickStamp));
		    DevKbdQueueAddInput(&mouseEvent);
		    break;
		default:
		    panic("Dev_KbdServiceInterrupt: unknown state.\n");
		    break;
	    }
	    mouseState = (mouseState + 1) % MOUSE_NUM_STATES;
	}
	Dev_UartWriteReg(mouseUartAddr, 0, WRITE0_CLEAR_INTR); 
	gotValidIntr = TRUE;
	gotInput = TRUE;
	
    } 

    if (intrStatus & A_FIFO_INTR) {
	Dev_KbdEvent	keyEvent;
	/*
	 * Keyboard key transition received.
	 */
	dev_KbdInstrument.numKbdChars++;
	character = ((DevKbdUart *) kbdUartAddr)->data;
	if (character != IDLE_CHAR) {
	    keyEvent.device	= DEV_KBD_KEY;
	    keyEvent.key	= KEY_VALUE(character);
	    keyEvent.keyUp	= KEY_UP(character);
	    Timer_GetCurrentTicks(&(keyEvent.tickStamp));
	    DevKbdQueueAddInput(&keyEvent);
	    gotInput = TRUE;
	}
	Dev_UartWriteReg(kbdUartAddr, 0, WRITE0_CLEAR_INTR); 
	gotValidIntr = TRUE;
    }
    if (intrStatus & READ3_TX_IP_A) {
	char c;
	/*
	 * Just got the "transmit buffer is empty" to the keyboard.  Do the
	 * same thing that we did above for the serial lines.
	 */
	if (Dev_SerialGetOutChar(DEV_EVENT_UNIT, &c) == SUCCESS) {
	    Dev_UartWriteReg(kbdUartAddr, 8, c);
	} else {
	    Dev_UartWriteReg(kbdUartAddr, 0, WRITE0_RESET_TX_INT); 
	}
	Dev_UartWriteReg(kbdUartAddr, 0, WRITE0_CLEAR_INTR); 
	gotValidIntr = TRUE;
    }

#endif notdef

    if (!gotValidIntr) {
	/*
	 * For some reason, an interrupt from the uart occured but the uart
	 * interrupt registers don't tell who caused it or the keyboard/mouse
	 * chip got an unwanted "transmit buffer empty" interrupt.
	 */
	dev_KbdExtraIntrs++;
	printf("Warning: Dev_KbdServiceInterrupt: Extraneous uart interrupt\n");

#ifdef notdef
	Dev_UartWriteReg(kbdUartAddr, 0, WRITE0_CLEAR_INTR); 
	Dev_UartWriteReg(mouseUartAddr, 0, WRITE0_CLEAR_INTR); 
#endif notdef

    Dev_UartWriteReg(ADDR_OFFSET(uartAddr, CHANNEL_A_OFFSET),
		     COMMAND, RESET_RX|RESET_TX);
    Dev_UartWriteReg(ADDR_OFFSET(uartAddr, CHANNEL_B_OFFSET),
		     COMMAND, RESET_RX|RESET_TX);

    } else if (gotInput) {
	Timer_GetTimeOfDay(kbdMostRecentIntrPtr, (int *) NIL,
			   (Boolean *) NIL);
    }
    MASTER_UNLOCK(&devKbdMutex);

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
    /* this is just a dummy routine for the spur */

    return -1;
}

