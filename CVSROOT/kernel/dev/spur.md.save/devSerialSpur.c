/* 
 *  devSerialSpur.c --
 *
 *     	This file contains machine-dependent routines that handle the
 *	output queue for the serial lines.
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
#include "devSerial.h"
#include "serial.h"
#include "devConsole.h"
#include "devUart.h"
#include "vm.h"


/*
 *----------------------------------------------------------------------
 *
 * Dev_SerialInitAddr --
 *
 *	Initializes the address of the device for a serial port.
 *	Called from Dev_KbdInit at boot time.  Combines the address
 *	and unit into an opaque type and calls the machine-independent
 *	initialization routine.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory is allocated for the machine-dependent structure.
 *
 *----------------------------------------------------------------------
 */

void
Dev_SerialInitAddr(unit, devAddr)
    int			unit;
    Address		devAddr;
{
    Dev_UartInfo 	*infoPtr;

    infoPtr = (Dev_UartInfo *) Vm_BootAlloc(sizeof(Dev_UartInfo));
    infoPtr->uartAddress = devAddr;
    switch (unit) {
        case DEV_SERIAL_A_UNIT:
	    infoPtr->channel = DEV_UART_CHANNEL_A;
	    break;
	case DEV_SERIAL_B_UNIT:
	    infoPtr->channel = DEV_UART_CHANNEL_B;
	    break;
	/*
	 * Unclear yet what this will be once implemented.
	 */
	case DEV_EVENT_UNIT:
	    infoPtr->channel = DEV_UART_CHANNEL_A;
	    break;
	default: 
	    Sys_Panic(SYS_FATAL, "Dev_SerialInitAddr: unknown unit %d",
		      unit);
	    break;
    }
    DevSerialInitQueue(unit, (ClientData) infoPtr);
}
