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
#include "console.h"
#include "mach.h"
#include "tty.h"
#include "dc7085.h"

/*
 * Forward references to procedures declared in this file:
 */

static int	NullOutputChar();
static int	TtyInterrupt();

/*
 * Pre-initialized data structures for three of the four channels in
 * the dc7085.h chip.
 */

static DevTty ttys[3];

DevDC7085 devKeyboard = {
    "keyboard",					/* name */
    &ttys[0],					/* ttyPtr */
    4800,					/* baud */
    DevTtyInputChar,				/* inputProc */
    (ClientData) &ttys[0],			/* inputData */
    NullOutputChar,				/* outputProc */
    (ClientData) &ttys[0],			/* outputData */
    KBD_PORT,				/* This is the keyboard. */
    0,
};

DevDC7085 devSerialA = {
    "serialA",					/* name */
    &ttys[1],					/* ttyPtr */
    9600,					/* baud */
    DevTtyInputChar,				/* inputProc */
    (ClientData) &ttys[1],			/* inputData */
    DevTtyOutputChar,				/* outputProc */
    (ClientData) &ttys[1],			/* outputData */
    MODEM_PORT,				/* This is the modem line
						 * (or serial A). */
    0,
};

DevDC7085 devSerialB = {
    "serialB",					/* name */
    &ttys[2],					/* ttyPtr */
    9600,					/* baud */
    DevTtyInputChar,				/* inputProc */
    (ClientData) &ttys[2],			/* inputData */
    DevTtyOutputChar,				/* outputProc */
    (ClientData) &ttys[2],			/* outputData */
    PRINTER_PORT,				/* This is the printer line
						 * (or serial B). */
    0,
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
    consoleUnit = 0;

    /*
     * Reset the devices.
     */
    DevDC7085Reset();

    DevDC7085RawProc(&devKeyboard, TD_RAW_SHUTDOWN, 0, (char *) NULL,
		     0, (char *) NULL);
    DevDC7085RawProc(&devSerialA, TD_RAW_SHUTDOWN, 0, (char *) NULL,
		     0, (char *) NULL);
    DevDC7085RawProc(&devSerialB, TD_RAW_SHUTDOWN, 0, (char *) NULL,
		     0, (char *) NULL);
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
    ttyPtr->rawProc = DevDC7085RawProc;
    ttyPtr->activateProc = DevDC7085Activate;
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
	    ttyPtr->rawData = (ClientData) &devKeyboard;
	    break;

	/*
	 * Unit 1 is serialA.
	 */

	case 1:
	    ttyPtr->rawData = (ClientData) &devSerialA;
	    break;

	/*
	 * Unit 2 is serialB.
	 */

	case 2:
	    ttyPtr->rawData = (ClientData) &devSerialB;
	    break;
    }
    return ttyPtr;
}


/*
 *----------------------------------------------------------------------
 *
 * NullOutputChar --
 *
 *	This procedure is entered as the outputProc field of
 *	DevDC7085 structures when the device is not used for
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
