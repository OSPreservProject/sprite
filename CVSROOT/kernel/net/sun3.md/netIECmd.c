/* netIECmd.c -
 *
 * Routines to execute commands for the Intel 82586 device driver.
 *
 * Copyright 1985, 1988 Regents of the University of California
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

#include <sprite.h>
#include <sys.h>
#include <list.h>
#include <vm.h>
#include <sync.h>
#include <netIEInt.h>


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
 *----------------------------------------------------------------------
 */

void
NetIECheckSCBCmdAccept(scbPtr)
    volatile NetIESCB	*scbPtr;
{
    int	 i;

    for (i = 0; i < 5; i++) {
	NET_IE_DELAY((*(short *) scbPtr->cmdWord == 0));

	if (*(short *) scbPtr->cmdWord != 0) {
	    printf( "Intel: scb command not accepted\n");
	} else {
	    return;
	}
    }
    panic( "Intel: scb command not accepted\n");
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
NetIEExecCommand(cmdPtr, statePtr)
    register volatile NetIECommandBlock	*cmdPtr;
    NetIEState		*statePtr;
{
    /*
     * Initialize the command header.
     */

    *(short *) cmdPtr = 0;	
    /* 
     * Mark this as the end of the list. 
     */
    NetBfShortSet(cmdPtr->bits, EndOfList, 1); 
    /* 
     * Have the command unit interrupt us when it is done.

     */
    NetBfShortSet(cmdPtr->bits, Interrupt, 1); 

    /*
     * Start the command unit.
     */

    NetBfShortSet(statePtr->scbPtr->cmdWord, CmdUnitCmd, NET_IE_CUC_START); 
    NET_IE_CHANNEL_ATTENTION(statePtr);
    NetIECheckSCBCmdAccept(statePtr->scbPtr);

    /*
     * Wait for the command to complete.
     */

    NET_IE_DELAY(NetBfShortTest(cmdPtr->bits, CmdDone, 1) && 
		 NetBfShortTest(statePtr->scbPtr->statusWord, CmdDone, 1) &&
	         NetBfShortTest(statePtr->scbPtr->statusWord, CmdUnitNotActive,
		     1));
    if (NetBfShortTest(cmdPtr->bits, CmdDone, 0) ||
        NetBfShortTest(statePtr->scbPtr->statusWord, CmdDone, 0) ||
	NetBfShortTest(statePtr->scbPtr->statusWord, CmdUnitNotActive, 0)) {

	int cmdCmdDone;
	int statusCmdDone;
	int cmdNotActive;
	cmdCmdDone = NetBfShortGet(cmdPtr->bits, CmdDone);
	statusCmdDone = NetBfShortGet(statePtr->scbPtr->statusWord, CmdDone);
	cmdNotActive = NetBfShortGet(statePtr->scbPtr->statusWord, 
	    CmdUnitNotActive);
	panic( "Intel: Could not execute a simple command: %d %d %d\n", 
		    cmdCmdDone, statusCmdDone, cmdNotActive);
	return;
    }

    /*
     * Ack the the command completion and the command unit going not active.
     */

    NetBfShortSet(statePtr->scbPtr->cmdWord, AckCmdDone, 1);
    NetBfShortSet(statePtr->scbPtr->cmdWord, AckCmdUnitNotActive, 1);

    NET_IE_CHANNEL_ATTENTION(statePtr);
    NetIECheckSCBCmdAccept(statePtr->scbPtr);
    return;
}
