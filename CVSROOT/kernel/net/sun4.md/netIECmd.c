/* netIECmd.c -
 *
 * Routines to execute commands for the Intel 82586 device driver.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "sys.h"
#include "list.h"
#include "vm.h"
#include "sync.h"
#include "netIE.h"
#include "net.h"
#include "netInt.h"


/*
 *----------------------------------------------------------------------
 *
 * NetIECheckSCBCmdAccept --
 *
 *	Check to see if the scb command was accepted.  Panic if not accepted.
 *	This routine is always called with interrupts disabled.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 */

void
NetIECheckSCBCmdAccept(scbPtr)
    NetIESCB	*scbPtr;
{
    int	 i;

    for (i = 0; i < 5; i++) {
	NET_IE_DELAY((*(short *) &(scbPtr->cmdWord) == 0));

	if (*(short *) &(scbPtr->cmdWord) != 0) {
	    Sys_Panic(SYS_WARNING, "Intel: scb command not accepted\n");
	} else {
	    return;
	}
    }
    Sys_Panic(SYS_FATAL, "Intel: scb command not accepted\n");
}


/*
 *----------------------------------------------------------------------
 *
 * NetIEExecCommand --
 *
 *	Execute a simple Intel command.  This is done by executing the command
 *	and then waiting until it completes.
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
NetIEExecCommand(cmdPtr)
    register	NetIECommandBlock	*cmdPtr;
{
    /*
     * Initialize the command header.
     */

    *(short *) cmdPtr = 0;	/* Clear the status bits. */
    cmdPtr->endOfList = 1;	/* Mark this as the end of the list. */
    cmdPtr->interrupt = 1;	/* Have the command unit interrupt us when
				   it is done. */

    /*
     * Start the command unit.
     */

    netIEState.scbPtr->cmdWord.cmdUnitCmd = NET_IE_CUC_START;
    NET_IE_CHANNEL_ATTENTION;
    NetIECheckSCBCmdAccept(netIEState.scbPtr);

    /*
     * Wait for the command to complete.
     */

    NET_IE_DELAY(cmdPtr->cmdDone && netIEState.scbPtr->statusWord.cmdDone &&
	         netIEState.scbPtr->statusWord.cmdUnitNotActive);
    if (!cmdPtr->cmdDone ||
        !netIEState.scbPtr->statusWord.cmdDone ||
	!netIEState.scbPtr->statusWord.cmdUnitNotActive) {
	Sys_Panic(SYS_FATAL, "Intel: Could not execute a simple command: %d %d %d\n", 
			cmdPtr->cmdDone, netIEState.scbPtr->statusWord.cmdDone,
			netIEState.scbPtr->statusWord.cmdUnitNotActive);
	return;
    }

    /*
     * Ack the the command completion and the command unit going not active.
     */

    netIEState.scbPtr->cmdWord.ackCmdDone = 1;
    netIEState.scbPtr->cmdWord.ackCmdUnitNotActive = 1;

    NET_IE_CHANNEL_ATTENTION;
    NetIECheckSCBCmdAccept(netIEState.scbPtr);
}
