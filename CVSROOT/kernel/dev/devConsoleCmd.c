/* 
 * devConsoleCmd.c --
 *
 *	This file provides the mechanism for invoking certain kernel
 *	operations by typing certain key sequences on the console (e.g.
 *	on Sun-3's, L1-D puts the machine into the debugger).
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
#include "dbg.h"
#include "machMon.h"
#include "net.h"

/*
 * Information about registered commands:
 */

static struct {
    void (*proc)();		/* Procedure to invoke. */
    ClientData clientData;	/* Argument to pass to proc. */
} commands[256];

/*
 * Forward declarations for procedures defined later in this file:
 */

static void	Abort();
static void	Debug();

/*
 *----------------------------------------------------------------------
 *
 * Dev_RegisterConsoleCmd --
 *
 *	This procedure is called to declare the procedure to be invoked
 *	when a particular console command is invoked.  Console commands
 *	are defined by a single ASCII character, e.g. "d" for debug.
 *	The specific invocation sequence depends on the machine and
 *	configuration.  On Sun-3's with console displays, L1-x is
 *	typed to invoke the command associated with "x";  on servers
 *	with no display, BREAK-x is typed to do the same thing.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Whenever the given command is invoked, proc will be called.
 *	It should have the following structure:
 *
 *	void
 *	proc(clientData)
 *	{
 *	}
 *
 *	The clientData argument will be the same as the clientData
 *	argument passed to this procedure.  Note:  proc will always
 *	be invoked at background level in a kernel server process.
 *
 *----------------------------------------------------------------------
 */

void
Dev_RegisterConsoleCmd(commandChar, proc, clientData)
    char commandChar;		/* ASCII character associated with command. */
    void (*proc)();		/* Procedure to call when command is
				 * invoked. */
    ClientData clientData;	/* Arbitrary one-word value to pass to
				 * command. */
{
    int index = commandChar & 0x7f;

    if (commands[index].proc != 0) {
	printf("%s for \"%c\" (0x%x).\n",
		"Warning: Dev_RegisterConsoleCmd replacing procedure",
		commandChar, index);
    }
    commands[index].proc = proc;
    commands[index].clientData = clientData;
}

/*
 *----------------------------------------------------------------------
 *
 * Dev_InvokeConsoleCmd --
 *
 *	Given a command character, this procedure invokes the console
 *	command associated with the character.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If there is a procedure registered for commandChar, it is
 *	invoked.
 *
 *----------------------------------------------------------------------
 */

void
Dev_InvokeConsoleCmd(commandChar)
    int commandChar;
{
    /*
     * The initialization code below should be removed;  the debugger
     * module should register its own commands.
     */

    if (commands['a'].proc == 0) {
	commands['a'].proc = Abort;
    }
    if (commands['b'].proc == 0) {
	commands['b'].proc = Debug;
	commands['b'].clientData = (ClientData) TRUE;
    }
    if (commands['d'].proc == 0) {
	commands['d'].proc = Debug;
	commands['d'].clientData = (ClientData) FALSE;
    }

    commandChar &= 0x7f;
    if (commands[commandChar].proc != 0) {
	(*commands[commandChar].proc)(commands[commandChar].clientData);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Abort, Debug --
 *
 *	These are temporary procedures to handle some of the console
 *	commands;  they should be moved out of this module.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Depends on command.
 *
 *----------------------------------------------------------------------
 */

static void
Abort()
{
    Mach_MonAbort();
    Net_Reset();
}

static void
Debug()
{

    DBG_CALL;
}

/*
 *----------------------------------------------------------------------
 *
 * Dev_KbdQueueAttachProc --
 *
 *	This procedure is a temporary hack during the conversion to
 *	the new tty driver.  All calls to it should be redirected
 *	to Dev_RegisterConsoleCmd.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	See Dev_RegisterConsoleCmd.
 *
 *----------------------------------------------------------------------
 */

int
Dev_KbdQueueAttachProc(character, proc, clientData)
    char character;
    void (*proc)();
    ClientData clientData;
{
    Dev_RegisterConsoleCmd(character, proc, clientData);
}
