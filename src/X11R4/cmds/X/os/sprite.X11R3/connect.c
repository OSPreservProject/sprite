/*-
 * connect.c --
 *	Functions to handle generic connections and their I/O.
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
"$Header: /mic/X11R3/src/cmds/Xsp/os/sprite/RCS/connect.c,v 1.20 89/10/23 18:21:54 tve Exp $ SPRITE (Berkeley)";
#endif lint

#include    "Xproto.h"
#include    "spriteos.h"
#include    "dix.h"
#include    "osstruct.h"

#include    <bit.h>
#include    <signal.h>
#include    <stdio.h>
#include    <string.h>

/*-
 *-----------------------------------------------------------------------
 * CreateWellKnownSockets --
 *	Create listening points via which we can accept connections from
 *	clients. It also finishes off the initialization needed by the
 *	OS layer.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	PseudoDevice is set to the streamID of the server's pseudo device.
 *	All the select masks are allocated and initialized.
 *	NumActiveStreams is initialized. Vectors interrupts to HangUp.
 *
 *-----------------------------------------------------------------------
 */
void
CreateWellKnownSockets()
{
    char    	  errorfile[50];    /* Path to error file */
    int	    	  whichbyte;	    /* Used to figure out whether this machine
				     * is LSB or MSB */
    void	  HangUp();
    int	    	  Reset();
    struct	  sigvec irq;
    int	    	  oldPermMask;	    /* Previous permission mask */
    char    	  hostname[64];
    char    	  *cp;

    gethostname(hostname, sizeof(hostname));
    cp = index(hostname, '.');
    if (cp != (char *)NULL) {
	*cp = '\0';
    }

    oldPermMask = umask(0);

    sprintf (errorfile, "/tmp/%s:%s.X11R3", hostname, display);
    fclose (stderr);
    fopen (errorfile, "w+");		/* Will go to stderr. */

    (void) umask(oldPermMask);


#ifdef TCPCONN
    TCP_Init();
    NumActiveStreams = max(NumActiveStreams, TCP_Conn+1);
#endif TCPCONN

    Pdev_Init(hostname);
    NumActiveStreams = max(NumActiveStreams, Pdev_Conn+1);
    
    /*
     * Allocate all the global bit masks. I'd prefer to do such things
     * in OsInit, but they leave me little choice, since this function
     * is called after OsInit...
     */
    Bit_Alloc (NumActiveStreams, AllStreamsMask);
    Bit_Alloc (NumActiveStreams, LastSelectMask);
    Bit_Alloc (NumActiveStreams, EnabledDevicesMask);
    Bit_Alloc (NumActiveStreams, ClientsWithInputMask);
    Bit_Alloc (NumActiveStreams, AllClientsMask);
    Bit_Alloc (NumActiveStreams, SavedAllClientsMask);
    Bit_Alloc (NumActiveStreams, SavedAllStreamsMask);

#ifdef TCPCONN
    Bit_Set (TCP_Conn, AllStreamsMask);
#endif TCPCONN
    Bit_Set (Pdev_Conn, AllStreamsMask);

    whichbyte = 1;

    if (*(char *)&whichbyte) {
	whichByteIsFirst = 'l';
    } else {
	whichByteIsFirst = 'B';
    }

    irq.sv_handler = /*(int (*)())*/ HangUp;
    irq.sv_mask = 0;
    irq.sv_onstack = 0;
    sigvec(SIGINT, &irq, (struct sigvec *) 0);
    sigvec(SIGTERM, &irq, (struct sigvec *) 0);
}

    
/*-
 *-----------------------------------------------------------------------
 * CloseDownConnection --
 *	Some client is being booted. Free up its connection resources.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The pseudo-device streams are closed and the private data marked
 *	inactive. The stream is removed from these masks:
 *	    AllStreamsMask, SavedAllStreamsMask, AllClientsMask,
 *	    SavedAllClientsMask, ClientsWithInputMask
 *
 *-----------------------------------------------------------------------
 */
CloseDownConnection (client)
    ClientPtr    client;    	/* Client whose connection should
				 * be closed */
{
    register ClntPrivPtr    pPriv;
    register int  	    i;

    pPriv = (ClntPrivPtr) client->osPrivate;

    if (DBG(CONN)) {
	ErrorF ("CloseDownConnection: client %d being booted\n", client->index);
    }

    (* pPriv->closeProc) (pPriv);

    Bit_Free (pPriv->mask);
    Bit_Free (pPriv->ready);
    free ((char *) pPriv);
}

/*-
 *-----------------------------------------------------------------------
 * AddEnabledDevice --
 *	Add another device to keep track of.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The select masks may be expanded. EnabledDevicesMask will be
 *	altered.
 *
 *-----------------------------------------------------------------------
 */
AddEnabledDevice (streamID)
    int	    streamID;
{
    ExpandMasks (0, streamID);

    Bit_Set (streamID, AllStreamsMask);
    Bit_Set (streamID, SavedAllStreamsMask);
    Bit_Set (streamID, EnabledDevicesMask);
}

/*-
 *-----------------------------------------------------------------------
 * RemoveEnabledDevice --
 *	Stop paying attention to some device.
 *
 * Results:
 *
 * Side Effects:
 *
 *-----------------------------------------------------------------------
 */
RemoveEnabledDevice (streamID)
    int	    streamID;
{
    Bit_Clear (streamID, AllStreamsMask);
    Bit_Clear (streamID, SavedAllStreamsMask);
    Bit_Clear (streamID, EnabledDevicesMask);
}

/*-
 *-----------------------------------------------------------------------
 * OnlyListenToOneClient --
 *	Only pay attention to requests from one client. We continue to
 *	accept new connections, of course, but only take protocol requests
 *	from this single connection.
 *	Once this is done, GrabDone will be set TRUE and and changes to
 *	AllStreamsMask and AllClientsMask should be done to their Saved
 *	copies.
 *	Devices are unaffected, of course.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	AllStreamsMask and AllClientsMask are copied into SavedAllStreamsMask
 *	and SavedAllClientsMask, respectively, and have all the other
 *	client bits removed from them.
 *	ClientsWithInputMask has all other clients's bits removed from it.
 *	GrabDone is set TRUE.
 *
 *-----------------------------------------------------------------------
 */
OnlyListenToOneClient (client)
    ClientPtr	    client;
{
    ClntPrivPtr	  pPriv = (ClntPrivPtr) client->osPrivate;
    int	    	  *tempMask;

    /*
     * First take everyone but the given client out of the ClientsWithInputMask
     * We have to allocate a new mask b/c there's no guarantee that the
     * private mask will be as wide as the regular masks, though it
     * usually is.
     */
    Bit_Alloc (NumActiveStreams, tempMask);
    (void) Bit_Copy (pPriv->maskWidth, pPriv->mask, tempMask);
    (void) Bit_Intersect (NumActiveStreams, tempMask,
			  ClientsWithInputMask, ClientsWithInputMask);

    if (DBG(CONN)) {
	ErrorF("Server grabbed by client %d\n", client->index);
    }
    if (!GrabDone) {
	/*
	 * If a grab has not already been done, duplicate the AllStreams
	 * and AllClients masks.
	 */
	  
	Bit_Copy (NumActiveStreams, AllStreamsMask, SavedAllStreamsMask);
	Bit_Copy (NumActiveStreams, AllClientsMask, SavedAllClientsMask);

    }
    (void) Bit_Union (NumActiveStreams, tempMask, EnabledDevicesMask,
		      AllStreamsMask);
    Bit_Copy (NumActiveStreams, pPriv->mask, AllClientsMask);
    Bit_Free (tempMask);

    GrabDone = TRUE;
    grabbingClient = client;
}
	
/*-
 *-----------------------------------------------------------------------
 * ListenToAllClients --
 *	Undo a grab: start paying attention to cries from other clients.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	GrabDone is set FALSE and the AllClients and AllStreams masks are
 *	restored from their Saved counterparts.
 *
 *-----------------------------------------------------------------------
 */
ListenToAllClients()
{
    if (GrabDone) {
	if (DBG(CONN)) {
	    ErrorF ("Server ungrabbed again\n");
	}
	Bit_Copy (NumActiveStreams, SavedAllStreamsMask, AllStreamsMask);
	Bit_Copy (NumActiveStreams, SavedAllClientsMask, AllClientsMask);
	GrabDone = FALSE;
    }
}

/*-
 *-----------------------------------------------------------------------
 * ReadRequestFromClient --
 *	Read a single request from a client. This is just a front-end that
 *	passes the buck to the connection-specific function...
 *
 * Results:
 *	What he said.
 *
 * Side Effects:
 *	Not here..
 *
 *-----------------------------------------------------------------------
 */
char *
ReadRequestFromClient (client, pStatus, oldbuf)
    ClientPtr	  	client;	    /* Client with input */
    int	    	  	*pStatus;   /* Result of read:
				     * >0 -- number of bytes in the request
				     * 0 -- not all the request is there
				     * <0 -- indicates an error */
    char    	  	*oldbuf;    /* The previous buffer (IGNORED) */
{
    ClntPrivPtr	  	pPriv;	    /* Private data for the client */

    pPriv = (ClntPrivPtr) client->osPrivate;

    oldbuf = (* pPriv->readProc) (pPriv, pStatus, oldbuf);
    if (*pStatus > 0) {
	SchedPacket(client);
    }
    return (oldbuf);
}

/*-
 *-----------------------------------------------------------------------
 * WriteToClient --
 *	Function to write stuff to a client. Just a front-end for the
 *	connection-specific stuff.
 *
 * Results:
 *	What he said.
 *
 * Side Effects:
 *	Elsewhere.
 *
 *-----------------------------------------------------------------------
 */
int
WriteToClient (client, numBytes, buf)
    ClientPtr	  	client;	    /* Client to receive the data */
    int	    	  	numBytes;   /* Number of bytes to transmit */
    char    	  	*buf;	    /* Buffer to send */
{
    ClntPrivPtr	  	pPriv;	    /* OS-private data */

    pPriv = (ClntPrivPtr) client->osPrivate;

    return ((* pPriv->writeProc) (pPriv, numBytes, buf));
}

/*-
 *-----------------------------------------------------------------------
 * ExpandMasks --
 *	Given a newly-opened stream, expand all the thousands of
 *	select masks to encompass that stream. This includes the private
 *	mask for the client.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The masks will be moved and the old ones freed..
 *
 *-----------------------------------------------------------------------
 */
void
ExpandMasks (pPriv, newStream)
    ClntPrivPtr	  pPriv;    	/* Private data for client.
				 * Should be 0 if it wasn't opened for a client
				 * (e.g. it's for a device) */
    int	    	  newStream;	/* The new stream's ID number */
{
    if (newStream >= NumActiveStreams) {
	register int newNumStreams = newStream + 1;
	
	AllStreamsMask =    	Bit_Expand (newNumStreams, NumActiveStreams,
					    AllStreamsMask);
	LastSelectMask =    	Bit_Expand (newNumStreams, NumActiveStreams,
					    LastSelectMask);
	EnabledDevicesMask = 	Bit_Expand (newNumStreams, NumActiveStreams,
					    EnabledDevicesMask);
	ClientsWithInputMask = 	Bit_Expand (newNumStreams, NumActiveStreams,
					    ClientsWithInputMask);
	AllClientsMask =    	Bit_Expand (newNumStreams, NumActiveStreams,
					    AllClientsMask);
	SavedAllClientsMask =	Bit_Expand (newNumStreams, NumActiveStreams,
					    SavedAllClientsMask);
	SavedAllStreamsMask =	Bit_Expand (newNumStreams, NumActiveStreams,
					    SavedAllStreamsMask);
	NumActiveStreams = newNumStreams;
    }

    /*
     * We need to also expand the private mask for the client to
     * encompass the new stream.
     */
    if (pPriv) {
	if (newStream >= pPriv->maskWidth) {
	    pPriv->mask = Bit_Expand (newStream+1, pPriv->maskWidth,
				      pPriv->mask);
	    pPriv->ready = Bit_Expand (newStream+1, pPriv->maskWidth,
				       pPriv->ready);
	    pPriv->maskWidth = newStream + 1;
	}
	Bit_Set (newStream, pPriv->mask);
    }
}
