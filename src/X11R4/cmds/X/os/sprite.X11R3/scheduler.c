/*-
 * scheduler.c --
 *	OS dependent scheduling routines:
 *	WaitForSomething, SchedPacket, SchedYield
 *
 * Copyright (c) 1987 by the Regents of the University of California
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 *
 */
#ifndef lint
static char rcsid[] =
	"$Header: /a/X/src/cmds/Xsprite/os/RCS/scheduler.c,v 1.11 89/06/01 10:44:49 brent Exp $ SPRITE (Berkeley)";
#endif lint

#include    "spriteos.h"
#include    <bit.h>
#include    <errno.h>
#include    <sys/time.h>

#include    "dixstruct.h"
#include    "opaque.h"

#include    "misc.h"

extern long ScreenSaverTime;            /* milliseconds */
extern long ScreenSaverInterval;        /* milliseconds */

Bool isItTimeToYield = TRUE;

/*-
 *-----------------------------------------------------------------------
 * WaitForSomething --
 *	Hang out until something interesting happens:
 *	    - data from clients
 *	    - new client connecting
 *	    - device input available
 *	If no input events come in for a sufficiently long time
 *	(ScreenSaverTime), turns on the screen saver.
 *
 *     	For more info on ClientsWithInput, see ReadRequestFromClient().
 *     	pClientsReady is a mask, the bits set are 
 *     	indices into the o.s. depedent table of available clients.
 *     	(In this case, there is no table -- the index is the socket
 *     	file descriptor.)  
 *
 * Results:
 *
 * Side Effects:
 *
 *-----------------------------------------------------------------------
 */

static int intervalCount = 0;

WaitForSomething(pClientsReady, pNumReady, pNewClients, pNumNew)
    ClientPtr  	  *pClientsReady; 	/* OUT: ready clients */
    int     	  *pNumReady;	    	/* OUT: number of ready clients */
    ClientPtr  	  *pNewClients;   	/* OUT: new client pointers */
    int     	  *pNumNew;	    	/* OUT: number of new clients */
{
    int			i;
    struct timeval	waittime, *wt;
    long		timeout;
    
    *pNumReady = 0;
    *pNumNew = 0;

    if (!Bit_AnySet (NumActiveStreams, ClientsWithInputMask)) {
	/*
	 * We need a while loop here to handle the screen saver timeout
	 */
	while (1) {
	    if (clientsDoomed) {
		Bit_Zero (NumActiveStreams, LastSelectMask);
		break;
	    }
	    
            if (ScreenSaverTime) {
		/*
		 * If screen saver is needed, set 'timeout' to the time
		 * remaining until the screen saver should be activated.
		 */
                timeout = ScreenSaverTime - TimeSinceLastInputEvent();
	        if (timeout < 0) {
		    /*
		     * There is no time left -- activate the screen saver.
		     */
	            if (timeout < intervalCount) {
		        SaveScreens(SCREEN_SAVER_ON, ScreenSaverActive);
		        if (intervalCount)
    		            intervalCount -= ScreenSaverInterval;
                        else
                            intervalCount = 
				-(ScreenSaverInterval + ScreenSaverTime);
		    }
    	            timeout -= intervalCount;
    	        } else {
	            intervalCount = 0;
		}
                waittime.tv_sec = timeout / 1000;
	        waittime.tv_usec = 0;
		wt = &waittime;
	    } else {
                wt = (struct timeval *) NULL;
	    }

	    Bit_Copy (NumActiveStreams, AllStreamsMask, LastSelectMask);

	    BlockHandler(&wt, LastSelectMask);
	    i = select(NumActiveStreams, LastSelectMask, (int *) NULL,
		    (int *) NULL, wt);
	    if (i <= 0) {
		if ((i == -1) && ((errno == EINVAL) || (errno == EIO))) {
		    /*
		     * If a stream was invalid, find each one that's
		     * bogus and close the client down. If it's one
		     * of the listening streams (Pdev_Conn, TCP_Conn,
		     * or NewPdev_Conn), close that down and set the
		     * appropriate variable to be -1 so we don't
		     * pay attention to it any more.
		     */
		    int 	    	    *selMask;
		    struct timeval     	    poll;
		    register ClntPrivPtr    pPriv;
		    register ClientPtr	    client;
		    int			    junk;
		    
		    ErrorF("Bad stream in select -- seek and destroy...");
		    poll.tv_sec = 0;
		    poll.tv_usec = 0;
		    
		    Bit_Alloc(NumActiveStreams, selMask);
		    
		    for (i = 1; i < currentMaxClients; i++) {
			client = clients[i];
			pPriv = (ClntPrivPtr) ((client && !client->clientGone) ?
						client->osPrivate : 0);

			if (pPriv && (pPriv->ready != (int *)0)) {
			    Bit_Copy(pPriv->maskWidth, pPriv->ready,
				     selMask);
			    junk = select(pPriv->maskWidth, selMask,
				    (int *) NULL, (int *) NULL, &poll);
			    if ((junk == -1) && ((errno == EINVAL)
				    || (errno == EIO))) {
				ErrorF("CLIENT(%d) ", client->index);
				CloseDownClient(client);
				break;
			    }
			}
		    }
		    Bit_Zero(NumActiveStreams, selMask);
		    Bit_Set(Pdev_Conn, selMask);
		    junk = select(Pdev_Conn + 1, selMask, (int *) NULL,
				(int *) NULL, &poll);
		    if ((junk == -1) && ((errno == EINVAL) || (errno == EIO))) {
			ErrorF("PDEV ");
			(void)close(Pdev_Conn);
			Bit_Clear(Pdev_Conn, selMask);
			Bit_Clear(Pdev_Conn, AllStreamsMask);
			Pdev_Conn = -1;
		    } else {
			Bit_Clear(Pdev_Conn, selMask);
		    }
#ifdef TCPCONN
		    Bit_Set(TCP_Conn, selMask);
		    junk = select(TCP_Conn + 1, selMask, (int *) NULL,
			    (int *) NULL, &poll);
		    if ((junk == -1) && ((errno == EINVAL) || (errno == EIO))) {
			ErrorF("TCP ");
			(void)close(TCP_Conn);
			Bit_Clear(TCP_Conn, selMask);
			Bit_Clear(TCP_Conn, AllStreamsMask);
			TCP_Conn = -1;
		    } else {
			Bit_Clear(TCP_Conn, selMask);
		    }
#endif /* TCPCONN */
		    ErrorF("done\n");
		} else if (i != 0) {
		    Error("Scheduler Select");
		}
		i = 0;
		Bit_Zero(NumActiveStreams, LastSelectMask);
	    }
	    WakeupHandler (&i, LastSelectMask);

	    if (i > 0) {
		if ((Pdev_Conn >= 0) && Bit_IsSet (Pdev_Conn, LastSelectMask)) {
		    Pdev_EstablishNewConnections (pNewClients, pNumNew);
		}
#ifdef TCPCONN
		if ((TCP_Conn >= 0) && Bit_IsSet (TCP_Conn, LastSelectMask)) {
		    TCP_EstablishNewConnections (pNewClients, pNumNew);
		}
#endif TCPCONN

		if (*pNumNew ||
		    Bit_Intersect (NumActiveStreams, LastSelectMask,
				   EnabledDevicesMask, (int *)NULL) ||
		    Bit_Intersect (NumActiveStreams, LastSelectMask,
				   AllClientsMask, (int *)NULL)) {
				      /*
				       * If there are new connections, or
				       * an enabled device has data available,
				       * or there are requests from clients,
				       * break out of this loop...we're ready
				       * to return.
				       */
				      break;
		}
	    }
	}
    } else {
	register int i;
	Bit_Copy (NumActiveStreams, ClientsWithInputMask, LastSelectMask);
	Bit_Zero (NumActiveStreams, ClientsWithInputMask);
    }
    
    if (Bit_Intersect(NumActiveStreams, LastSelectMask, AllClientsMask, NULL)){
	/*
	 * If any clients are ready, go through the table of clients
	 * and see if any of the client's streams has its bit set in the
	 * select mask. Any that does has its bit set in the client's
	 * "ready" mask.
	 */
	register ClntPrivPtr  	pPriv;
	register ClientPtr  	client;

	for (i = 1; i < currentMaxClients; i++) {
	    client = clients[i];
	    pPriv = (ClntPrivPtr) ((client && !client->clientGone) ?
				    client->osPrivate : 0);

	    if (pPriv && (pPriv->ready != (int *)0) &&
		Bit_Intersect (pPriv->maskWidth, LastSelectMask,
			       pPriv->mask, pPriv->ready)) {
				   pClientsReady[*pNumReady] = client;
				   (*pNumReady)++;
	    }
	}
    }

    /*
     * As a concession to the DDX layer, if any of the streams to devices
     * is ready, call spriteInputAvail() to inform DDX of this.
     */
    if (Bit_Intersect (NumActiveStreams, LastSelectMask, EnabledDevicesMask,
		       NULL)) {
			   spriteInputAvail();
    }
}

static int	  packets = 0;
/*-
 *-----------------------------------------------------------------------
 * SchedPacket --
 *	Record having received another packet for the given client.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	???
 *
 *-----------------------------------------------------------------------
 */
/*ARGSUSED*/
void
SchedPacket (client)
    ClientPtr	  client;
{
    if (!isItTimeToYield) {
	packets += 1;
	if (packets == MAX_PACKETS) {
	    isItTimeToYield = TRUE;
	    packets = 0;
	}
    }
}

/*-
 *-----------------------------------------------------------------------
 * SchedYield --
 *	Yield the server to another client.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	isItTimeToYield is set TRUE.
 *
 *-----------------------------------------------------------------------
 */
void
SchedYield()
{
    isItTimeToYield = TRUE;
    packets = 0;
}
