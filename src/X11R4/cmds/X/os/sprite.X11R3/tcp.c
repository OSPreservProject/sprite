/*-
 * tcp.c --
 *	Functions to communicate with clients over a TCP connection.
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
"$Header: /b/X/src/cmds/Xsprite/os/RCS/tcp.c,v 1.7 89/08/15 21:34:45 ouster Exp $ SPRITE (Berkeley)";
#endif lint

#ifdef TCPCONN

/*
 * The following includes must be first, or the file won't compile.
 */

#define Time SpriteTime
#include    <fs.h>
#undef Time
#include    <stdlib.h>

#include    "spriteos.h"
#include    "Xproto.h"
#include    "opaque.h"

#include    <bit.h>
#include    <errno.h>
#include    <signal.h>
#include    <sys/types.h>
#include    <sys/socket.h>
#include    <sys/time.h>
#include    <netinet/in.h>

int	TCP_Conn; 	/* Passive socket for listening */

typedef struct {
    int	    	  streamID; 	/* Active socket for client */
    ClientPtr	  client;   	/* Back pointer to Client descriptor */
    Address 	  buffer;   	/* Input buffer */
    Address 	  bufPtr;   	/* Current position in buffer */
    int	    	  numBytes; 	/* Bytes remaining in buffer */
    int    	  needData; 	/* 1 if need to read data */
} TCPPrivRec, *TCPPrivPtr;

typedef enum {
    TCP_READ, TCP_WRITE
} IoOperation;

/*
 * REASONABLE_TIME is the amount of time we'll wait for a client connection
 * to unblock, while MAX_BLOCK is the number of times we'll let the connection
 * block us before aborting the whole shebang.
 */
#define REASONABLE_TIME	    4
#define MAX_BLOCK 	    2

static void TCPCloseClient();
static char *TCPReadClient();
static int  TCPWriteClient();

/*-
 *-----------------------------------------------------------------------
 *
 * TCP_Init --
 *	Initialize the TCP module by creating the passive socket and
 *	storing its stream ID in TCP_Conn.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	A passive socket is opened and its streamID placed in TCP_Conn.
 *
 *-----------------------------------------------------------------------
 */
void
TCP_Init()
{
    struct sockaddr_in	inAddr;
    int	    	  	displayNum;
    struct sigvec  	ignore;

    displayNum = atoi(display);
    inAddr.sin_port = htons(X_TCP_PORT + displayNum);
    inAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    TCP_Conn = socket(AF_INET, SOCK_STREAM, 0);
    if (TCP_Conn < 0) {
	Error("Creating TCP socket");
	FatalError("Couldn't create TCP socket...\n");
	/*NOTREACHED*/
    }
    if (bind(TCP_Conn, (struct sockaddr *) &inAddr, sizeof(inAddr)) < 0) {
	Error("Binding TCP socket");
	FatalError("Couldn't bind TCP socket...\n");
	/*NOTREACHED*/
    }
    if (listen(TCP_Conn, 5) < 0) {
	Error("listen(TCP_Conn)");
	FatalError("Couldn't listen on TCP socket...\n");
	/*NOTREACHED*/
    }
    (void)Ioc_SetBits(TCP_Conn, IOC_NON_BLOCKING);
    ignore.sv_handler = SIG_IGN;
    ignore.sv_mask = ignore.sv_onstack = 0;
    sigvec(SIGPIPE, &ignore, (struct sigvec *) 0);
}

/*-
 *-----------------------------------------------------------------------
 *
 * TCPConnIO --
 *	Perform some I/O on a tcp connection with timeouts.
 *
 * Results:
 *	Zero is returned if the operation completed successfully,
 *	a non-zero errno otherwise.
 *
 * Side Effects:
 *	The data are transmitted or received.
 *
 *-----------------------------------------------------------------------
 */
static ReturnStatus
TCPConnIO(streamID, numBytes, bufPtr, ioType)
    int	    	  streamID;
    int	    	  numBytes;
    Address 	  bufPtr;
    IoOperation	  ioType;
{
    struct timeval	  timeout;
    int	    		  *selMask;
    int	    		  numIO;
    int	    		  timesBlocked;

    Bit_Alloc(streamID+1, selMask);
    timesBlocked = 0;
    while (1) {
	if (ioType == TCP_WRITE) {
	    numIO = write(streamID, bufPtr, numBytes);
	} else {
	    numIO = read(streamID, bufPtr, numBytes);
	}
	if (numIO != -1) {
	    /*
	     * I/O was successful, but it might have been short. Subtract
	     * the number of bytes dealt with from that we have to handle
	     * and if it's zero, we've finished successfully. Else keep
	     * going.
	     */
	    numBytes -= numIO;
	    bufPtr += numIO;
	    if (numBytes == 0) {
		Bit_Free(selMask);
		return (0);
	    }
	} else if (errno == EWOULDBLOCK) {
	    /*
	     * The operation blocked. We only allow the client to block us
	     * a certain number of times before we return an error. If we're
	     * still being patient, we select on the stream for a reasonable
	     * time and loop once it's ready or the time has expired.
	     */
	    timesBlocked += 1;
	    if (timesBlocked > MAX_BLOCK) {
		Bit_Free(selMask);
		return (EWOULDBLOCK);
	    }

	    timeout.tv_sec = REASONABLE_TIME;
	    timeout.tv_usec = 0;
	    do {
		Bit_Set(streamID, selMask);
		numIO = select(streamID+1, 
		       (ioType == TCP_READ ? selMask : (int *) 0),
		       (ioType == TCP_WRITE ? selMask : (int *) 0),
		       (int *) 0, &timeout);
	    } while (errno == EINTR);
	} else {
	    /*
	     * Some other error occurred. Abort and return it.
	     */
	    Bit_Free(selMask);
	    return (errno);
	}
    }
}

/*-
 *-----------------------------------------------------------------------
 *
 * TCPClientAuthorized --
 *	See if a client that is connecting over TCP is authorized to do
 *	so.
 *
 * Results:
 *	1 if the connection is OK. 0 otherwise.
 *
 * Side Effects:
 *	*pSwapped is set 1 if the client needs to be byte-swapped.
 *	*pReason points to a reason for the refusal, if such there be.
 *
 *-----------------------------------------------------------------------
 */
static int
TCPClientAuthorized(conn, pSwapped, pInAddr, pReason)
    int	    	  	conn;  	    /* StreamID of connection */
    Bool    	  	*pSwapped;  /* OUT: True if client is byte-swapped */
    struct sockaddr	*pInAddr;   /* Address of connected client */
    char    	  	**pReason;  /* OUT: Reason for refusal */
{
    xConnClientPrefix	xccp;	    /* Prefix of info from client */
    
    *pReason = (char *)NULL;
    
    if (TCPConnIO(conn, sizeof(xccp), &xccp, TCP_READ) != 0) {
	if (DBG(TCP)) {
	    Error("Reading client prefix");
	}
	*pReason = "Couldn't read client prefix";
	return (0);
    }
    if (xccp.byteOrder != whichByteIsFirst) {
	SwapConnClientPrefix(&xccp);
	*pSwapped = 1;
    } else {
	*pSwapped = 0;
    }

    if ((xccp.majorVersion != X_PROTOCOL) ||
	(xccp.minorVersion != X_PROTOCOL_REVISION)) {
	    *pReason = "Protocol version mismatch";
	    return (0);
    }

    if (InvalidHost(FamilyInternet, pInAddr)) {
	*pReason = "Permission denied";
	return (0);
    }
    if (xccp.nbytesAuthProto != 0) {
	char *authProto;

	authProto = (char *)ALLOCATE_LOCAL(xccp.nbytesAuthProto);
	if (TCPConnIO(conn, xccp.nbytesAuthProto, authProto,
		      TCP_READ) != 0) {
		    *pReason = "Couldn't read authorization protocol string";
		    DEALLOCATE_LOCAL(authProto);
		    return (0);
	}
	DEALLOCATE_LOCAL(authProto);
    }
    if (xccp.nbytesAuthString != 0) {
	char *authString;

	authString = (char *)ALLOCATE_LOCAL(xccp.nbytesAuthString);
	if (TCPConnIO(conn, xccp.nbytesAuthString, authString,
		      TCP_READ) != 0) {
			    *pReason = "Couldn't read authorization string";
			    DEALLOCATE_LOCAL(authString);
			    return (0);
	}
	DEALLOCATE_LOCAL(authString);
    }
    return (1);
}

/*-
 *-----------------------------------------------------------------------
 *
 * TCPConnFail --
 *	Report connection failure to the client.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	An xConnSetupPrefix is sent.
 *
 *-----------------------------------------------------------------------
 */
static void
TCPConnFail(streamID, swapped, reason)
    int	    streamID;
    Bool    swapped;
    char    *reason;
{
    xConnSetupPrefix *packet;
    int	    reasonLength;
    int	    totalLength;
    int	    numWritten;

    reasonLength = strlen(reason);
    totalLength = (reasonLength + 3) >> 2;
    
    numWritten = sizeof(xConnSetupPrefix) + (totalLength << 2);
    packet = (xConnSetupPrefix *)ALLOCATE_LOCAL(numWritten);

    packet->success = xFalse;
    packet->lengthReason = reasonLength;
    packet->length = totalLength;
    if (!swapped) {
	packet->majorVersion = X_PROTOCOL;
	packet->minorVersion = X_PROTOCOL_REVISION;
    } else {
	short	  n;

	swaps(&packet->length, n);
	packet->majorVersion = lswaps(X_PROTOCOL);
	packet->minorVersion = lswaps(X_PROTOCOL_REVISION);
    }
    strcpy ((char *)(packet+1), reason);
    (void)TCPConnIO(streamID, numWritten, packet, TCP_WRITE);
    DEALLOCATE_LOCAL(packet);
}

/*-
 *-----------------------------------------------------------------------
 *
 * TCP_EstablishNewConnections --
 *	A connection has been discovered on the passive TCP socket, so
 *	we accept it and do all the things we're supposed to do...
 *
 *	XXX: Only accept one new connection, should we accept more?
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	A new ClientPtr is placed in the passed array and the number of
 *	new clients is incremented.
 *
 *-----------------------------------------------------------------------
 */
void
TCP_EstablishNewConnections(pNewClients, pNumNew)
    ClientPtr	  	*pNewClients;
    int	    	  	*pNumNew;
{
    int	    	  	newID;	    /* New client stream */
    ClientPtr	  	client;	    /* New client record */
    struct sockaddr	inAddr;	    /* Address of new client */
    int	    	  	addrLen;    /* Length of new client's address */
    int 	  	swapped;    /* 1 if client is byte-swapped */
    ClntPrivPtr	  	pPriv;	    /* OS-global private data for client */
    TCPPrivPtr	  	ptcpPriv;   /* Our private data */
    char    	  	*reason;    /* Reason for connection refusal */
	
    pNewClients += *pNumNew;

    addrLen = sizeof(inAddr);
    newID = accept(TCP_Conn, &inAddr, &addrLen);
    if (newID < 0) {
	if (DBG(TCP)) {
	    Error("accept");
	}
	return;
    }
    if (!TCPClientAuthorized(newID, &swapped, &inAddr, &reason)) {
	if (DBG(TCP)) {
	    ErrorF("Unauthorized connection\n");
	}
	TCPConnFail(newID, swapped, reason);
	close(newID);
	return;
    }
    client = (ClientPtr) NextAvailableClient();
    if (client == NullClient) {
	if (DBG(TCP)) {
	    ErrorF("No more clients!\n");
	}
	TCPConnFail(newID, swapped, "No more client slots");
	close(newID);
	return;
    }
    pPriv = (ClntPrivPtr) malloc(sizeof(ClntPrivRec));
    pPriv->readProc = TCPReadClient;
    pPriv->writeProc = TCPWriteClient;
    pPriv->closeProc = TCPCloseClient;
    pPriv->ready = pPriv->mask = (int *)0;
    pPriv->maskWidth = 0;
    
    ptcpPriv = (TCPPrivPtr) malloc(sizeof(TCPPrivRec));
    pPriv->devicePrivate = (pointer)ptcpPriv;
    
    client->osPrivate = (pointer)pPriv;
    client->swapped = swapped;
    
    ptcpPriv->streamID = newID;
    ptcpPriv->client = client;
    ptcpPriv->buffer = (Address)NULL;
    ptcpPriv->numBytes = 0;
    ptcpPriv->needData = 1;
    
    ExpandMasks(pPriv, newID);
    
    Ioc_SetBits(newID, IOC_NON_BLOCKING);
    
    if (GrabDone) {
	Bit_Set(newID, SavedAllClientsMask);
	Bit_Set(newID, SavedAllStreamsMask);
    } else {
	Bit_Set(newID, AllClientsMask);
	Bit_Set(newID, AllStreamsMask);
    }
    if (DBG(TCP) || DBG(CONN)) {
	ErrorF("New TCP connection: client %d (newID = %d)\n", client->index,
	       newID);
    }
    *pNewClients = client;
    pNewClients++;
    (*pNumNew)++;
}

/*-
 *-----------------------------------------------------------------------
 *
 * TCPCloseClient --
 *	Close down a client we're handling and free up our part of the
 *	resources.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The active stream to the client is closed and the private data
 *	freed. The stream is removed from these masks: 
 *	    AllStreamsMask, SavedAllStreamsMask, AllClientsMask,
 *	    SavedAllClientsMask, ClientsWithInputMask
 *
 *-----------------------------------------------------------------------
 */
static void
TCPCloseClient(pPriv)
    ClntPrivPtr	  pPriv;    	/* Private data for client to be closed */
{
    TCPPrivPtr	  ptcpPriv;

    ptcpPriv = (TCPPrivPtr)pPriv->devicePrivate;

    if (ptcpPriv->buffer) {
	free((char *) ptcpPriv->buffer);
    }
    Bit_Clear(ptcpPriv->streamID, ClientsWithInputMask);

    if (GrabDone) {
	Bit_Clear(ptcpPriv->streamID, SavedAllClientsMask);
	Bit_Clear(ptcpPriv->streamID, SavedAllStreamsMask);
	if (grabbingClient == ptcpPriv->client) {
	    Bit_Clear(ptcpPriv->streamID, AllClientsMask);
	    Bit_Clear(ptcpPriv->streamID, AllStreamsMask);
	}
    } else {
	Bit_Clear(ptcpPriv->streamID, AllClientsMask);
	Bit_Clear(ptcpPriv->streamID, AllStreamsMask);
    }
    free((char *)ptcpPriv);
}

#define request_length(req, cli) \
 	(((cli)->swapped?lswaps(((xReq *)(req))->length):\
	  ((xReq *)(req))->length) << 2)
/*-
 *-----------------------------------------------------------------------
 *
 * TCPReadClient --
 *	Read a request from the given client.
 *
 * Results:
 *	Pointer to the start of the request.
 *
 * Side Effects:
 *	Data may be read from the client's connection.
 *
 *-----------------------------------------------------------------------
 */
/*ARGSUSED*/
static char *
TCPReadClient(pPriv, pStatus, oldbuf)
    ClntPrivPtr	  pPriv;    	/* Client with input */
    int	    	  *pStatus; 	/* Result of read:
				 *  >0 -- number of bytes in request
				 *   0 -- not all the request is there
				 *  <0 -- error */
    char    	  *oldbuf;  	/* Previous buffer */
{
    xReq    	  *reqPtr;  	/* Request being returned */
    int	    	  numRead;  	/* Number of bytes read from the socket */
    int	    	  need;	    	/* Number of bytes needed */
    TCPPrivPtr	  ptcpPriv; 	/* Our private data for the client */

    ptcpPriv = (TCPPrivPtr)pPriv->devicePrivate;

    Bit_Clear(ptcpPriv->streamID, ClientsWithInputMask);

    if (ptcpPriv->needData) {
	/*
	 * If previous buffer was exhausted and we haven't freed it, do so
	 * now. While we could just keep the buffer around, thereby adjusting
	 * it to the usual size the client will need, it could easily get
	 * out of hand if someone sends a good bit of image data, so
	 * we pay the price of copying rather than that of having buffers be
	 * too large.
	 */
	if ((ptcpPriv->numBytes == 0) && (ptcpPriv->buffer != (Address)NULL)){
	    free((char *) ptcpPriv->buffer);
	    ptcpPriv->buffer = (Address)NULL;
	}
	need = -1;
	if ((Ioc_NumReadable(ptcpPriv->streamID, &need) != SUCCESS) ||
	    (need <= 0)) {
		/*
		 * If there are no bytes waiting, the other side has closed its
		 * connection (we could only have gotten here if the select
		 * returned readable, which implies that there are bytes to be
		 * read...), so we mark an error. (Also mark an error if
		 * we couldn't find how many bytes were available).
		 */
		if (DBG(TCP)) {
		    ErrorF("Ioc_NumReadable: %d bytes readable\n", need);
		}
		*pStatus = -1;
		return((char *)NULL);
	}

	if (ptcpPriv->numBytes != 0) {
	    /*
	     * There were insufficient data in the buffer last time. We
	     * need to compress the buffer as much as possible to keep it
	     * from growing without bound (hard to do, but possible).
	     * If the space we've allocated already is big enough to
	     * hold the current data plus what's coming in now, we just copy
	     * the data in place, else we copy it to a new buffer and free
	     * the old one.
	     */
	    char  *newBuffer;
	    
	    if (Mem_Size(ptcpPriv->buffer) >= need + ptcpPriv->numBytes) {
		newBuffer = ptcpPriv->buffer;
	    } else {
		newBuffer = malloc(need + ptcpPriv->numBytes);
	    }
	    if (ptcpPriv->bufPtr != newBuffer) {
		bcopy(ptcpPriv->bufPtr, newBuffer, ptcpPriv->numBytes);
	    }
	    if (newBuffer != ptcpPriv->buffer) {
		free((char *) ptcpPriv->buffer);
		ptcpPriv->buffer = newBuffer;
	    }
	} else {
	    ptcpPriv->buffer = malloc(need);
	}
	/*
	 * Read whatever data are available from the socket to the end of the
	 * buffer. Any error (including FS_WOULD_BLOCK) causes the connection
	 * to be severed (we shouldn't block since we're reading what's there)
	 */
	numRead = read(ptcpPriv->streamID,
		ptcpPriv->buffer + ptcpPriv->numBytes, need);
	if (numRead == -1) {
	    if (DBG(TCP)) {
		Error("Reading TCP connection %d",
		      ptcpPriv->streamID);
	    }
	    *pStatus = -1;
	    return ((char *)NULL);
	}
	ptcpPriv->bufPtr = ptcpPriv->buffer;
	ptcpPriv->numBytes += numRead;

    }
    /*
     * bufPtr now points to the current request for the client. Figure out how
     * much data must be in the buffer for the request to be complete and store
     * it in 'need'. If there isn't that much data, we have an incomplete
     * request and need to read more data, so we set needData 1 and return
     * a status of 0.
     *
     * If there is enough data, we update the various pointers and whatnot to
     * reflect the taking of the request, then if there's still data in the
     * buffer, note this client as having more input by setting the bit for the
     * stream in the ClientsWithInputMask. The scheduler will exhaust this
     * data before going to sleep again. While it is possible for a client to
     * monopolize the server by cleverly sending incomplete data packets
     * at just the right time, it seems unlikely that it could manage it.
     */
    if (ptcpPriv->numBytes < sizeof(xReq)) {
	ptcpPriv->needData = 1;
	*pStatus = 0;
	SchedYield();
	return ((char *)NULL);
    }
    need = request_length(ptcpPriv->bufPtr, ptcpPriv->client);
    if (need > ptcpPriv->numBytes) {
	ptcpPriv->needData = 1;
	*pStatus = 0;
	SchedYield();
	return ((char *)NULL);
    }
    reqPtr = (xReq *)ptcpPriv->bufPtr;
    ptcpPriv->bufPtr += need;
    ptcpPriv->numBytes -= need;
    *pStatus = need;
    ptcpPriv->needData = 1;
    
    if (ptcpPriv->numBytes) {
	if (ptcpPriv->numBytes >= sizeof(xReq)) {
	    need = request_length(ptcpPriv->bufPtr, ptcpPriv->client);
	    if (ptcpPriv->numBytes >= need) {
		/*
		 * We only don't need data in the buffer if we have a
		 * complete request sitting there. In such a case, we mark
		 * the client as already having input and set needData 0.
		 */
		Bit_Set(ptcpPriv->streamID, ClientsWithInputMask);
		ptcpPriv->needData = 0;
	    }
	}
    }
    if (ptcpPriv->needData) {
	/*
	 * Once we run out of data in the request buffer, we have to wait
	 * for another select to return this stream's readiness.
	 */
	SchedYield();
    }
    return ((char *)reqPtr);
}

/*-
 *-----------------------------------------------------------------------
 *
 * TCPWriteClient --
 *	Write information to the client. We have a choice of making
 *	two write calls or copying the data to longword-align it. For
 *	now, just make the two system calls. We'll see which is worse...
 *	We could use writev, but that's not restartable in case
 *	of an interrupt.
 *
 * Results:
 *	The number of bytes written.
 *
 * Side Effects:
 *	Data are written to
 *
 *-----------------------------------------------------------------------
 */
static int
TCPWriteClient (pPriv, numBytes, xRepPtr)
    ClntPrivPtr	  pPriv;
    int	    	  numBytes;
    Address 	  xRepPtr;
{
    TCPPrivPtr	  ptcpPriv;
    int	    	  pad = 0;
    int	    	  numPadBytes;

    ptcpPriv = (TCPPrivPtr)pPriv->devicePrivate;
    numPadBytes = (4 - (numBytes & 3)) & 3;

    if (TCPConnIO(ptcpPriv->streamID, numBytes, xRepPtr, TCP_WRITE)!=0){
	MarkClientException(ptcpPriv->client);
	return(-1);
    }
    if (numPadBytes &&
	(TCPConnIO(ptcpPriv->streamID, numPadBytes,&pad,TCP_WRITE)!=0)){
	MarkClientException(ptcpPriv->client);
	return(-1);
    }
    return (numBytes);
}
#endif TCPCONN
