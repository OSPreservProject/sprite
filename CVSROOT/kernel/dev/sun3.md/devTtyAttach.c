/* 
 * devTtyAttach.c --
 *
 *	This file manages the configuration of Z8530 chips on Sun-3
 *	and Sun-4 workstations, and provides glue to attach the device
 *	drivers for those chips to standard Sprite devices like
 *	/dev/console and /dev/serialA.
 *
 * Copyright 1989 Regents of the University of California
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
#endif /* not lint */

#include "sprite.h"
#include "stdio.h"
#include "console.h"
#include "devAddrs.h"
#include "mouse.h"
#include "mach.h"
#include "machMon.h"
#include "tty.h"
#include "z8530.h"
#ifndef sun2
#include <sys/types.h>
#include <mon/eeprom.h>
#endif

/*
 * Forward references to procedures declared in this file:
 */

static int	NullOutputChar();
static int	TtyInterrupt();

/*
 * Pre-initialized data structures for three of the four channels in
 * the two Z8530 SCC chips (see devMouse.c for the fourth).
 */

static DevTty ttys[3];

static DevZ8530 keyboard = {
    "keyboard",					/* name */
    (DevZ8530Device *) DEV_KBD_ADDR,		/* address */
    &ttys[0],					/* ttyPtr */
    DEV_UART_VECTOR,				/* vector */
    1200,					/* baud */
    WRITE3_RX_8BIT,				/* wr3 */
    WRITE5_TX_8BIT,				/* wr5 */
    DevConsoleInputProc,			/* inputProc */
    (ClientData) &ttys[0],			/* inputData */
    NullOutputChar,				/* outputProc */
    (ClientData) &ttys[0],			/* outputData */
    0,						/* oldRr0 */
    Z_CHANNEL_A | Z_INACTIVE			/* flags */
};

static DevZ8530 serialA = {
    "serialA",					/* name */
    (DevZ8530Device *) DEV_SERIALA_ADDR,	/* address */
    &ttys[1],					/* ttyPtr */
    DEV_UART_VECTOR,				/* vector */
    9600,					/* baud */
    WRITE3_RX_8BIT,				/* wr3 */
    WRITE5_TX_8BIT,				/* wr5 */
    DevTtyInputChar,				/* inputProc */
    (ClientData) &ttys[1],			/* inputData */
    DevTtyOutputChar,				/* outputProc */
    (ClientData) &ttys[1],			/* outputData */
    0,						/* oldRr0 */
    Z_CHANNEL_A | Z_INACTIVE			/* flags */
};

static DevZ8530 serialB = {
    "serialB",					/* name */
    (DevZ8530Device *) DEV_SERIALB_ADDR,	/* address */
    &ttys[2],					/* ttyPtr */
    DEV_UART_VECTOR,				/* vector */
    9600,					/* baud */
    WRITE3_RX_8BIT,				/* wr3 */
    WRITE5_TX_8BIT,				/* wr5 */
    DevTtyInputChar,				/* inputProc */
    (ClientData) &ttys[2],			/* inputData */
    DevTtyOutputChar,				/* outputProc */
    (ClientData) &ttys[2],			/* outputData */
    0,						/* oldRr0 */
    Z_CHANNEL_B | Z_INACTIVE			/* flags */
};

/*
 * The following variable is filled in with the unit that should
 * be used whenever "/dev/console" is opened (may be a serial unit
 * if the machine doesn't have a keyboard+display).
 */

static int consoleUnit = 0;

/*
 *----------------------------------------------------------------------
 *
 * DevTtyInit --
 *
 *	Called during bootstrapping to initialize terminal-related
 *	things.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Resets serial devices and performs other initialization.
 *
 *----------------------------------------------------------------------
 */

void
DevTtyInit()
{
    char promConsoleType;

    /*
     * Figure out whether this machine has a display for a console or
     * just a terminal on a serial line.
     */

#ifndef sun2
    promConsoleType = ((struct eeprom *) EEPROM_BASE)->ee_diag.eed_console;
    switch (promConsoleType) {
	case EED_CONS_TTYA:
	    consoleUnit = 1;
	    break;
	case EED_CONS_TTYB:
	    consoleUnit = 2;
	    break;
	case EED_CONS_BW:
	case EED_CONS_COLOR:
	case EED_CONS_P4:
	    consoleUnit = 0;
	    break;
	default:
	    printf("Warning: %s 0x%x\n",
		    "Dev_TtyAttach couldn't identify console type",
		    promConsoleType);
	    break;
    }
#endif /* sun2 */

    /*
     * Reset the devices.
     */

    DevZ8530RawProc(&keyboard, TD_RAW_SHUTDOWN, 0, (char *) NULL,
	    0, (char *) NULL);
    DevZ8530RawProc(&serialA, TD_RAW_SHUTDOWN, 0, (char *) NULL,
	    0, (char *) NULL);
    DevZ8530RawProc(&serialB, TD_RAW_SHUTDOWN, 0, (char *) NULL,
	    0, (char *) NULL);
    DevMouseInit();

#ifdef sun4
    Mach_SetHandler(12, TtyInterrupt, (ClientData) 0);
#else
    Mach_SetHandler(DEV_UART_VECTOR, TtyInterrupt, (ClientData) 0);
#endif
    Mach_MonStopNmi();
}

/*
 *----------------------------------------------------------------------
 *
 * DevTtyAttach --
 *
 *	Given a unit number, return the DevTty for the unit, properly
 *	initialized.  This procedure is called as part of the open
 *	sequence for a terminal.
 *
 * Results:
 *	The return value is a pointer to a DevTty for the given unit,
 *	with some of its fields filled in (see devTty.h for details).
 *	If the unit number is bad, then NULL is returned.
 *
 * Side effects:
 *	Device-specific data structures get initialized.
 *
 *----------------------------------------------------------------------
 */

DevTty *
DevTtyAttach(unit)
    int unit;			/* Unit number for device. */
{
    register DevTty *ttyPtr;

    if ((unit > 2) || (unit < 0)) {
	return NULL;
    }

    /*
     * If the console is one of the serial units, then disallow the
     * serial unit for any use other than console.  Otherwise the
     * wait tokens will get confused (two different wait tokens from
     * higher-level software, but only one stored in the tty structure).
     */

    if ((unit != 0) && (unit == consoleUnit)) {
	return NULL;
    }
    
    if (unit == 0) {
	unit = consoleUnit;
    }
    ttyPtr = &ttys[unit];

    /*
     * If the terminal is already open then there's nothing more to
     * do;  otherwise, initialize the information relating to the
     * unit.
     */

    if (ttyPtr->openCount > 0) {
	return ttyPtr;
    }
    ttyPtr->rawProc = DevZ8530RawProc;
    ttyPtr->activateProc = DevZ8530Activate;
    ttyPtr->inputProc = (void (*)()) NIL;
    ttyPtr->inputData = (ClientData) 0;
    if (unit == consoleUnit) {
	ttyPtr->consoleFlags = DEV_TTY_IS_CONSOLE;
    } else {
	ttyPtr->consoleFlags = 0;
    }

    switch (unit) {

	/*
	 * Unit 0 is the display+keyboard, which serves as console
	 * if it exists.
	 */

	case 0:
	    ttyPtr->rawProc = DevConsoleRawProc;
	    ttyPtr->rawData = (ClientData) &keyboard;
	    break;

	/*
	 * Unit 1 is serialA.
	 */

	case 1:
	    ttyPtr->rawData = (ClientData) &serialA;
	    break;

	/*
	 * Unit 2 is serialB.
	 */

	case 2:
	    ttyPtr->rawData = (ClientData) &serialB;
	    break;
    }
    return ttyPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * DevGrabKeyboard --
 *
 *	This procedure is a special hack to allow events from the
 *	keyboard to be redirected back and forth between /dev/console
 *	and /dev/event.  DevGrabKeyboard is called to redirect input
 *	keystrokes away from their normal target (/dev/console) to
 *	some other place.
 *
 * Results:
 *	The return value is a pointer to the DevZ8530 structure for
 *	the keyboard (in case the caller should need to use some of
 *	its fields).
 *
 * Side effects:
 *	From now on, inputProc will be used to process input characters
 *	from the keyboard and outpuProc will be used to supply output
 *	characters to the keyboard.
 *
 *----------------------------------------------------------------------
 */

DevZ8530 *
DevGrabKeyboard(inputProc, inputData, outputProc, outputData)
    void (*inputProc)();		/* Procedure for devTty.c to call
					 * to process input characters
					 * at background level. */
    ClientData inputData;		/* Argument to pass to inputProc. */
    int (*outputProc)();		/* Interrupt-level procedure to
					 * provide next output character. */
    ClientData outputData;		/* Argument to pass to outputProc. */
{
    ttys[0].inputProc = inputProc;
    ttys[0].inputData = inputData;
    keyboard.outputProc = outputProc;
    keyboard.outputData = outputData;
    return &keyboard;
}

/*
 *----------------------------------------------------------------------
 *
 * DevReleaseKeyboard --
 *
 *	This procedure is also part of the special hack to allow events
 *	from the keyboard to be redirected back and forth between
 *	/dev/console and /dev/event.  When the event device is closed
 *	for the last time, this procedure is called to redirect keyboard
 *	input back to /dev/console.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Keyboard input goes to /dev/console again.
 *
 *----------------------------------------------------------------------
 */

void
DevReleaseKeyboard()
{
    ttys[0].inputProc = (void (*)()) NIL;
    ttys[0].inputData = (ClientData) 0;
    keyboard.outputProc = NullOutputChar;
    keyboard.outputData = (ClientData) &ttys[0];
}

/*
 *----------------------------------------------------------------------
 *
 * TtyInterrupt --
 *
 *	This procedure is called whenever an interrupt occurs on
 *	one of the Z8530 chips.  Various models of Sun's don't
 *	handle "real" interrupt vectors from the UART chips, so the
 *	safest thing is to auto-vector them all through this procedure.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Each chip's interrupt handler gets invoked in turn to
 *	process input and output characters for that chip.
 *
 *----------------------------------------------------------------------
 */

static int
TtyInterrupt()
{
    DevZ8530Interrupt((ClientData)&keyboard);
    DevZ8530Interrupt((ClientData)&serialA);
    DevZ8530Interrupt((ClientData)&serialB);
    DevMouseInterrupt();
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * NullOutputChar --
 *
 *	This procedure is entered as the outputProc field of
 *	DevZ8530 structures when the device is not used for
 *	output.
 *
 * Results:
 *	Always returns -1, which means "no output characters available".
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
static int
NullOutputChar(clientData)
    ClientData clientData;		/* Not used. */
{
    return -1;
}
