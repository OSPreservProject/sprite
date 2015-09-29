/* 
 * sockMisc.c --
 *
 *	Miscellaneous routines to test and socket state information.
 *
 * Copyright 1987 Regents of the University of California
 * All rights reserved.
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/daemons/ipServer/RCS/sockMisc.c,v 1.3 89/03/23 09:56:10 brent Exp $ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"
#include "dev/net.h"
#include "ipServer.h"
#include "socket.h"
#include "sockInt.h"



/*
 *----------------------------------------------------------------------
 *
 * Sock_SetError --
 *
 *	Saves the error status in the socket and notifies the client
 *	so it can find out about the error. This routine is used when
 *	the exact socket is known, unlike Sock_ReturnError.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The socket state is updated. The client is woken up.
 *
 *----------------------------------------------------------------------
 */

void
Sock_SetError(sockPtr, status)
    Sock_SharedInfo	*sockPtr;	/* Sock to be notified. */
    ReturnStatus	status;		/* Error to be saved. */
{
    sockPtr->error = status;
    Sock_NotifyWaiter(sockPtr, FS_READABLE|FS_WRITABLE|FS_EXCEPTION);
}


/*
 *----------------------------------------------------------------------
 *
 * Sock_IsOptionSet --
 *
 *	Tests the state of the option for a socket.
 *
 * Results:
 *	TRUE		- the option is set.
 *	FALSE		- the option is not set.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Boolean
Sock_IsOptionSet(sockPtr, option)
    Sock_SharedInfo	*sockPtr;	/* Sock of interest. */
    int		option;
{
    return(sockPtr->options & option);
}


/*
 *----------------------------------------------------------------------
 *
 * Sock_HasUsers --
 *
 *	Checks if the socket is being used by at least 1 client.
 *
 * Results:
 *	TRUE	- The socket has clients.
 *	FALSE	- No clients are using the socket.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Boolean
Sock_HasUsers(sockPtr)
    Sock_SharedInfo *		sockPtr;	/* Sock of interest. */
{
    return(sockPtr->clientCount > 0);
}


/*
 *----------------------------------------------------------------------
 *
 * Sock_Connected --
 *
 *	Changes the socket state to CONNECTED and notifies
 *	the client that this change has occurred.  Called from 
 *	protocol-related routines.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The client is woken up.
 *
 *----------------------------------------------------------------------
 */

void
Sock_Connected(sockPtr)
    Sock_SharedInfo *		sockPtr;	/* Sock of interest. */
{
    sockPtr->state = CONNECTED;
    if (sockPtr->parentPtr != (Sock_SharedInfo *) NULL) {
	/*
	 * SockPtr is a newly-created connection but is not fully
	 * initialized. Notify the parent socket so it can complete
	 * the initialization.
	 */
	Sock_NotifyWaiter(sockPtr->parentPtr, FS_READABLE|FS_WRITABLE);

	/*
	 * Set the justEstablished flag after Sock_NofityWaiter because that
	 * routine indirectly resets it.
	 */
	sockPtr->parentPtr->justEstablished = TRUE;
	Sock_NotifyWaiter(sockPtr, FS_READABLE|FS_WRITABLE);
	Sock_NotifyWaiter(sockPtr, FS_READABLE|FS_WRITABLE);
    } else {
	Sock_NotifyWaiter(sockPtr, FS_READABLE|FS_WRITABLE);
	sockPtr->justEstablished = TRUE;
	Sock_NotifyWaiter(sockPtr, FS_READABLE|FS_WRITABLE);
	Sock_NotifyWaiter(sockPtr, FS_READABLE|FS_WRITABLE);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * Sock_Disconnected --
 *
 *	Changes the socket state to DISCONNECTED and sets flags
 *	so that further I/O on the socket is prevented. The client
 *	is notified of the change. Called from protocol-related routines
 *	when the connection is completely closed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The socket state is updated. The client is woken up.
 *
 *----------------------------------------------------------------------
 */

void
Sock_Disconnected(sockPtr)
    Sock_SharedInfo *		sockPtr;	/* Sock_ of interest. */
{
    sockPtr->state = DISCONNECTED;
    sockPtr->flags |= (SOCK_STOP_RECV|SOCK_STOP_SEND);
    Sock_NotifyWaiter(sockPtr, FS_READABLE|FS_WRITABLE);
}


/*
 *----------------------------------------------------------------------
 *
 * Sock_Disconnecting --
 *
 *	Changes the socket state to DISCONNECTING and sets flags
 *	so that further I/O on the socket is prevented. Called from 
 *	protocol-related routines that need to close down a connection 
 *	in several steps.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The socket state is updated.
 *
 *----------------------------------------------------------------------
 */

void
Sock_Disconnecting(sockPtr)
    Sock_SharedInfo *		sockPtr;	/* Sock of interest. */
{
    sockPtr->state = DISCONNECTING;
    sockPtr->flags |= (SOCK_STOP_RECV|SOCK_STOP_SEND);
}

/*
 *----------------------------------------------------------------------
 *
 * Sock_IsRecvStopped --
 *
 *	Returns the state of the STOP_RECV flag.
 *
 * Results:
 *	TRUE	- no more data will be received from the network.
 *	FALSE	- data can be received.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Boolean
Sock_IsRecvStopped(sockPtr)
    Sock_SharedInfo *		sockPtr;	/* Sock of interest. */
{
    return(sockPtr->flags & SOCK_STOP_RECV);
}

/*
 *----------------------------------------------------------------------
 *
 * Sock_IsSendStopped --
 *
 *	Returns the state of the STOP_SEND flag.
 *
 * Results:
 *	TRUE	- no more data can be sent.
 *	FALSE	- data can be sent.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Boolean
Sock_IsSendStopped(sockPtr)
    Sock_SharedInfo *		sockPtr;	/* Sock of interest. */
{
    return(sockPtr->flags & SOCK_STOP_SEND);
}

/*
 *----------------------------------------------------------------------
 *
 * Sock_StopRecv --
 *
 *	Sets a flag to indicate that further reception of data from
 *	the network has been stopped.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The socket state is updated.
 *
 *----------------------------------------------------------------------
 */

void
Sock_StopRecv(sockPtr)
    Sock_SharedInfo *		sockPtr;	/* Sock_ of interest. */
{
    sockPtr->flags |= SOCK_STOP_RECV;
}

/*
 *----------------------------------------------------------------------
 *
 * Sock_StopSending --
 *
 *	Sets a flag to indicate that further sending of data to
 *	the network has been stopped.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The socket state is updated.
 *
 *----------------------------------------------------------------------
 */

void
Sock_StopSending(sockPtr)
    Sock_SharedInfo *		sockPtr;	/* Sock_ of interest. */
{
    sockPtr->flags |= SOCK_STOP_SEND;
}

/*
 *----------------------------------------------------------------------
 *
 * Sock_UrgentDataNext --
 *
 *	Sets a flag to indicate that urgent data is logically at the
 *	front of the receive buffer.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The socket flags are updated.
 *
 *----------------------------------------------------------------------
 */

void
Sock_UrgentDataNext(sockPtr)
    Sock_SharedInfo *		sockPtr;	/* Sock_S of interest. */
{
    sockPtr->flags |= SOCK_URGENT_DATA_NEXT;
}


/*
 *----------------------------------------------------------------------
 *
 * Sock_HaveUrgentData --
 *
 *	Notifies the client and controlling process or family that
 *	urgent data has arrived.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A signal may be sent to the controlling process or family.
 *	Any users waiting for urgent data are woken up.
 *
 *----------------------------------------------------------------------
 */

void
Sock_HaveUrgentData(sockPtr)
    Sock_SharedInfo *		sockPtr;	/* Sock_ of interest. */
{
    /*
     * The owner PID is non-zero, then signal the owner process or family
     * that there's urgent data.
     */
    Sock_NotifyWaiter(sockPtr, FS_EXCEPTION);
    if (sockPtr->owner.id != 0) {
	if (ips_Debug) {
	    (void) fprintf(stderr, 
		"Sock_HaveUrgentData: sending URGENT signal to %x (%s)\n",
		sockPtr->owner.id, 
		sockPtr->owner.procOrFamily == IOC_OWNER_FAMILY ? 
			"family" : "process");
	}

	Sig_Send(SIG_URGENT, sockPtr->owner.id, 
		    sockPtr->owner.procOrFamily == IOC_OWNER_FAMILY);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * Sock_BadRoute --
 *
 *	Called when a protocol-dependent routine has determined
 *	that a route is bad.
 *
 *	Just a stub procedure for now.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
void
Sock_BadRoute(sockPtr)
    Sock_SharedInfo 	*sockPtr;	/* Socket with the bad route. */
{
}
