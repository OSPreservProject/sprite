/*-
 * pdev.c --
 *	Functions for handling a connection to a client over a pseudo-device.
 *
 *	For each client, we have two buffers -- a 2K output buffer, from
 *	which the client reads, and a 2K request buffer, to which it writes.
 *	The size for the output buffer is based on examination of the size
 *	of buffers used in the old pseudo-device implementation. None
 *	was larger than 2K, so... The input buffer size is based on the
 *	size of Xlib's output buffer -- 2K. This is obviously inadequate for
 *	large amounts of image data, but for most applications it should
 *	be fine.
 *
 *	Requests are processed from the request buffer in order, obviously,
 *	with a write request remaining current until all its data have been
 *	consumed. If an X request crosses a pdev request boundary, it is
 *	copied and collected in a separate area, which is deallocated on the
 *	next call. X requests wholy inside a pdev request are left in the
 *	buffer. The kernel is not told the pdev request has been processed
 *	until all X requests in the pdev request have been handled.
 *
 *	Each time data are written to the read buffer, the pointers are
 *	updated. The stream is not read until the request buffer has
 *	been exhausted.
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
"$Header: /a/X/src/cmds/Xsprite/os/RCS/pdev.c,v 1.12 89/06/22 09:38:41 ouster Exp $ SPRITE (Berkeley)";
#endif lint

#define NEED_REPLIES /* For Debugging Only */

/*
 * These first two header files must indeed be first, or else this
 * file won't compile.
 */

#define Time SpriteTime
#include    <fs.h>
#undef Time
#include    <stdlib.h>

#include    "spriteos.h"

#include    "Xproto.h"
#include    "opaque.h"

#include    <bit.h>
#include    <dev/pdev.h>
#include    <errno.h>
#include    <status.h>
#include    <stdio.h>
#include    <sys/time.h>

/*
 * Template for the pseudo-device through which we communicate. Given to
 * one of the printf functions and expects two arguments: the name of the
 * local host and the display number we're using.
 */
#define	DEVICE_TEMPLATE	"/hosts/%s/X%s"

#define REASONABLE_TIME	    5
#define OUT_BUF_SIZE	    2048
#define IN_BUF_SIZE	    2048

/*
 * The private data maintained for a pseudo-device client
 */
typedef struct {
    int	    	  	streamID;   /* Server stream over which we get the
				     * buffer pointers */
    ClientPtr	  	client;	    /* Client for which this is */
    int	    	  	state;
#define PDEV_REQ_EMPTY	    	0x0002	/* Request buffer is empty */
#define PDEV_READ_EMPTY	    	0x0004 	/* Read buffer is empty */
#define PDEV_COLLECTING	    	0x0008	/* Collecting broken X request */
#define PDEV_COLLECTING_HEADER	0x0010	/* Collecting broken X header */

    /*
     * Request (inBuf) and read-ahead (outBuf) buffers
     */
    char  	  	outBuf[OUT_BUF_SIZE];
    char    	  	*outPtr;    /* Next place to store data */

    char    	  	inBuf[IN_BUF_SIZE];
    Pdev_Request	*reqPtr;    /* Address of next request to process */
    char    	  	*inPtr;	    /* Position in current request */

    Pdev_BufPtrs  	curPtrs;    /* Current pointers for the two buffers */

    /*
     * Overflow. If the outBuf fills up, we copy the output data into
     * 'overflow' and copy as much down as possible when the kernel has
     * emptied outBuf.
     *
     * If an X request comes in that is broken across two PDEV_WRITE requests,
     * we allocate enough room to hold it and point bigReq at it, then copy
     * data from inBuf, as it becomes available, into bigReq until the
     * request is fulfilled.
     */
    Buffer  	  	overflow;   /* Any output that won't fit in outBuf */
    char    	  	*bigReq;    /* Points to space for any broken request
				     * that is being assembled. NULL if none */
    char    	  	*bigReqPtr; /* Current position in bigReq */
    int	    	  	need;	    /* Number of bytes still needed */
} PdevPrivRec, *PdevPrivPtr;

static void 	  PdevCloseClient();
static char 	  *PdevReadClient();
static int  	  PdevWriteClient();

int	    	  Pdev_Conn;

/*-
 *-----------------------------------------------------------------------
 * Pdev_Init --
 *	Initialize pseudo-device connections.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The pseudo-device for this display is opened. The process will
 *	exit if the device cannot be opened.
 *
 *-----------------------------------------------------------------------
 */
void
Pdev_Init(hostname)
    char    *hostname;	/* The name of the local host */
{
    char    	  deviceName[100];  /* Path to pseudo-device */
    int	    	  oldPermMask;	    /* Previous permission mask */
    ReturnStatus  status;

    /*
     * Create the pseudo-device, making sure it's readable and writable
     * by everyone.
     */

    sprintf (deviceName, DEVICE_TEMPLATE, hostname, display);
    oldPermMask = umask(0);
    status = Fs_Open (deviceName,
	    FS_NON_BLOCKING | FS_CREATE | FS_READ | FS_PDEV_MASTER,
	    0666, &Pdev_Conn);
    if (status != 0) {
	errno = Compat_MapCode(status);
	Error (deviceName);
	FatalError ("Could not open pseudo-device %s",
		deviceName);
    }

    (void) umask(oldPermMask);
}

/*-
 *-----------------------------------------------------------------------
 * PdevSetPtrs --
 *	Set our idea of the state of the two buffers. Called when a
 *	Pdev_BufPtrs structure is read from the kernel. If the read-ahead
 *	buffer has been emptied by the kernel and there's overflow waiting
 *	to go, copy it down and inform the kernel of it.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	curPtrs is altered.
 *
 *-----------------------------------------------------------------------
 */
static void
PdevSetPtrs(pdevPriv, bufPtrs)
    PdevPrivPtr	pdevPriv;
    Pdev_BufPtrs  	*bufPtrs;
{
    if (DBG(PDEV)) {
	ErrorF("SetPtrs(%d): req %d:%d read %d:%d\n",
	       pdevPriv->client ? pdevPriv->client->index : -1,
	       bufPtrs->requestFirstByte,
	       bufPtrs->requestLastByte,
	       bufPtrs->readFirstByte,
	       bufPtrs->readLastByte);
    }

    /*
     * First update the extent of the request buffer.
     */
    pdevPriv->curPtrs.requestLastByte = bufPtrs->requestLastByte;
    if ((bufPtrs->requestLastByte != -1)  &&
	(pdevPriv->state & PDEV_REQ_EMPTY)) {
	    /*
	     * We only pay attention to the requestFirstByte if going from
	     * an empty to a non-empty buffer, since we think we know where
	     * the first request byte actually is.
	     */
	    pdevPriv->curPtrs.requestFirstByte = bufPtrs->requestFirstByte;
	    pdevPriv->reqPtr =
		(Pdev_Request *)&pdevPriv->inBuf[bufPtrs->requestFirstByte];
	    pdevPriv->inPtr = (char *)NULL;
	    pdevPriv->state &= ~PDEV_REQ_EMPTY;
    }

    /*
     * Then the extent of the read buffer.
     */
    pdevPriv->curPtrs.readFirstByte = bufPtrs->readFirstByte;
    if (bufPtrs->readFirstByte == -1) {
	int   numBytes;

	pdevPriv->state |= PDEV_READ_EMPTY;
	pdevPriv->outPtr = pdevPriv->outBuf;

	numBytes = Buf_Size(pdevPriv->overflow);
	if (numBytes > OUT_BUF_SIZE) {
	    numBytes = OUT_BUF_SIZE;
	}
	if (numBytes != 0) {
	    /*
	     * Copy as much overflow data into the output buffer as possible.
	     * The number of bytes actually copied is left in numBytes.
	     * pdevPriv->outPtr is set to point to the next place to store
	     * data in the buffer.
	     */
	    numBytes = Buf_GetBytes(pdevPriv->overflow, numBytes,
				    (Byte *)pdevPriv->outBuf);
	    pdevPriv->outPtr = &pdevPriv->outBuf[numBytes];

	    if (numBytes != 0) {
		/*
		 * If bytes actually copied, the buffer is no longer empty.
		 * Adjust the pointers and inform the kernel of the existence
		 * of the data.
		 */
		pdevPriv->state &= ~PDEV_READ_EMPTY;
		pdevPriv->curPtrs.readFirstByte = 0;
		pdevPriv->curPtrs.readLastByte = numBytes - 1;
		(void)Fs_IOControl (pdevPriv->streamID,
				    IOC_PDEV_SET_PTRS,
				    sizeof(pdevPriv->curPtrs),
				    (Address)&pdevPriv->curPtrs,
				    0,
				    (Address)NULL);
	    }
	} else {
	    /*
	     * Need to make this -1 so PdevWriteClient works correctly
	     * (it adds the number of bytes written to readLastByte without
	     * checking its value)
	     */
	    pdevPriv->curPtrs.readLastByte = -1;
	}
    }
}
	
/*-
 *-----------------------------------------------------------------------
 * PdevRequestHandled --
 *	Tell the kernel that we have handled the request at
 *	pdevPriv->reqPtr and update our own idea of the request buffer.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	curPtrs.requestFirstByte is altered, inPtr is NULLed, the
 *	PDEV_REQ_EMPTY flag will be set if the buffer is now empty.
 *
 *-----------------------------------------------------------------------
 */
static void
PdevRequestHandled(pdevPriv)
    PdevPrivPtr	pdevPriv;
{
    ReturnStatus status;

    pdevPriv->inPtr = (char *)NULL;
    pdevPriv->curPtrs.requestFirstByte += pdevPriv->reqPtr->hdr.messageSize;
    pdevPriv->reqPtr =
	(Pdev_Request *)&pdevPriv->inBuf[pdevPriv->curPtrs.requestFirstByte];

    if (pdevPriv->curPtrs.requestFirstByte >
	pdevPriv->curPtrs.requestLastByte) {
	    pdevPriv->state |= PDEV_REQ_EMPTY;
    }

    if (DBG(PDEV)) {
	ErrorF("RequestHandled(%d): req %d:%d read %d:%d\n",
	       pdevPriv->client ? pdevPriv->client->index : -1,
	       pdevPriv->curPtrs.requestFirstByte,
	       pdevPriv->curPtrs.requestLastByte,
	       pdevPriv->curPtrs.readFirstByte,
	       pdevPriv->curPtrs.readLastByte);
    }

    status = Fs_IOControl(pdevPriv->streamID, IOC_PDEV_SET_PTRS,
	    sizeof(pdevPriv->curPtrs), (Address) &pdevPriv->curPtrs,
	    0, (Address) NULL);
    if (status != 0) {
	errno = Compat_MapCode(status);
	if (DBG(PDEV)) {
	    Error("RequestHandled: SET_PTRS");
	}
    }
    if (pdevPriv->state & PDEV_REQ_EMPTY) {
	pdevPriv->curPtrs.requestFirstByte =
	    pdevPriv->curPtrs.requestLastByte = -1;
    }
}

/*-
 *-----------------------------------------------------------------------
 * PdevWaitForReadable --
 *	Wait for a connection to become readable, but only wait a
 *	reasonable amount of time.
 *
 * Results:
 *	Returns 0 if successful, -1 if an error occurs or no
 *	files were readable.
 *
 * Side Effects:
 *	The curPtrs field of the connection is updated.
 *
 *-----------------------------------------------------------------------
 */
static ReturnStatus
PdevWaitForReadable (pdevPriv, selMask)
    PdevPrivPtr	pdevPriv;   /* Connection to read */
    int	    	  	*selMask;   /* Pre-allocated select mask (zeroed) */
{
    Pdev_BufPtrs  	bufPtrs;    /* New buffer pointers for stream */
    struct timeval  	timeout;    /* Timeout interval for select */
    int	    	  	numReady;   /* Number of ready streams */
    int	    	  	numBytes;   /* Number of bytes read */
    
    Bit_Set (pdevPriv->streamID, selMask);
    timeout.tv_sec = REASONABLE_TIME;
    timeout.tv_usec = 0;

    numReady = select(pdevPriv->streamID+1, selMask, (int *) 0, (int *)0,
	    &timeout);
    if (numReady < 1) {
	return -1;
    }

    numBytes = read(pdevPriv->streamID, (char *) &bufPtrs, sizeof(bufPtrs));
    if (numBytes == -1) {
	if (DBG(PDEV)) {
	    Error ("PdevWaitForReadable");
	}
	return (-1);
    }
    if (bufPtrs.magic != PDEV_BUF_PTR_MAGIC) {
	if (DBG(PDEV)) {
	    ErrorF ("Buffer magic numbers don't match\n");
	}
	return (-1);
    }
    PdevSetPtrs(pdevPriv, &bufPtrs);
    return (0);
}

/*-
 *-----------------------------------------------------------------------
 * PdevConnFail --
 *	Inform a client that it has been denied access.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------
 */
static void
PdevConnFail (pdevPriv, swapped, reason)
    PdevPrivPtr	pdevPriv;   	/* Failed connection */
    Bool    	  	swapped;    	/* TRUE if client is byte-swapped */
    char    	  	*reason;    	/* Reason for failure */
{
    int	    	  	*selMask;   	/* Mask for selecting on stream */
    struct timeval  	timeout;    	/* Timeout for select */
    struct timeval  	*pTimeOut;
    int	    	  	numReady;   	/* Number of streams from select*/
    int	    	  	numBytes;   	/* Number of bytes read/written */
    int	    	  	length;	    	/* Length of reason */
    xConnSetupPrefix	*c; 	    	/* Pointer to returned structure in
					 * read-ahead buffer */
    Pdev_BufPtrs  	bufPtrs;    	/* New pointers for buffer */


    if (reason == (char *)NULL) {
	length = 0;
    } else {
	length = strlen (reason);
    }

    c = (xConnSetupPrefix *)pdevPriv->outBuf;
    c->success = xFalse;
    c->lengthReason = length;
    c->length = (length + 3) >> 2;

    pdevPriv->curPtrs.readFirstByte = 0;
    pdevPriv->curPtrs.readLastByte =
	sizeof(xConnSetupPrefix) + (c->length << 2);

    if (!swapped) {
	c->majorVersion = X_PROTOCOL;
	c->minorVersion = X_PROTOCOL_REVISION;
    } else {
	short	  n;

	swaps(&c->length, n);
	c->majorVersion = lswaps(X_PROTOCOL);
	c->minorVersion = lswaps(X_PROTOCOL_REVISION);
    }
    
    strcpy((char *)&c[1], reason);

    if (Fs_IOControl(pdevPriv->streamID, IOC_PDEV_SET_PTRS,
		     sizeof(pdevPriv->curPtrs),
		     (Address)&pdevPriv->curPtrs,
		     0, (Address)NULL) != 0) {
			 return;
    }
    
    Bit_Alloc (pdevPriv->streamID+1, selMask);

    timeout.tv_sec = REASONABLE_TIME;
    timeout.tv_usec = 0;
    if (DBG(PDEV)) {
	pTimeOut = (struct timeval *)0;
    } else {
	pTimeOut = &timeout;
    }
    
    /*
     * We give the client a REASONABLE_TIME to read the failure structure
     * and message. If it doesn't read it in that time, it loses. When the
     * read-ahead buffer is empty, the client has read the info.
     */
    do {
	Bit_Set (pdevPriv->streamID, selMask);
	numReady = select(pdevPriv->streamID+1, selMask, (int *)0, (int *)0,
		pTimeOut);
	if (numReady < 1) {
	    if (DBG(PDEV)) {
		ErrorF("PdevConnFail: didn't read in reasonable time\n");
	    }
	    break;
	}
	numBytes = read(pdevPriv->streamID, (char *) &bufPtrs,
		sizeof(bufPtrs));
	if ((numBytes != sizeof(bufPtrs))
		|| (bufPtrs.magic != PDEV_BUF_PTR_MAGIC)) {
	    break;
	}
	if (bufPtrs.readFirstByte == -1) {
	    break;
	} else {
	    PdevSetPtrs(pdevPriv, &bufPtrs);
	    while (!(pdevPriv->state & PDEV_REQ_EMPTY)) {
		if (pdevPriv->reqPtr->hdr.operation == PDEV_CLOSE) {
		    /*
		     * If we get a close message before the data are all read,
		     * just allow the client to go away gracefully -- reply to
		     * the close and get the h*** out of here.
		     */
		    Pdev_Reply	reply;

		    reply.magic = PDEV_REPLY_MAGIC;
		    reply.status = SUCCESS;
		    reply.selectBits = 0;
		    reply.replySize = 0;
		    reply.replyBuf = (Address)NULL;
		    reply.signal = 0;
		    reply.code = 0;
		    (void)Fs_IOControl(pdevPriv->streamID,
				       IOC_PDEV_REPLY,
				       sizeof(reply), (Address)&reply,
				       0, (Address)NULL);
		    PdevRequestHandled(pdevPriv);
		    goto done;
		}
		/*
		 * Anything else we just drop on the floor
		 */
		PdevRequestHandled(pdevPriv);
	    }
	}
    } while (!pTimeOut || (pTimeOut->tv_sec || pTimeOut->tv_usec));

done:
    Bit_Free (selMask);
}

/*-
 *-----------------------------------------------------------------------
 * Pdev_EstablishNewConnections --
 *	Accept connections from new clients. 
 *
 * Results:
 *	None returned. pNewClients and *pNumNew are filled in.
 *
 * Side Effects:
 *	AllClientsMask and AllStreamsMask are altered. Fields in the
 *	private structure for the new clients are filled and several
 *	private structures are created.
 *
 *-----------------------------------------------------------------------
 */
void
Pdev_EstablishNewConnections (pNewClients, pNumNew)
    ClientPtr  	  	*pNewClients;	/* Array to fill in */
    int	    	  	*pNumNew; 	/* Number of new connections
					 * established */
{
    Pdev_Notify	  	note;	    	/* Notification of new stream */
    PdevPrivPtr	pdevPriv;   		/* New private information for us */
    ClntPrivPtr	  	pPriv;	    	/* New private information for client */
    ClientPtr	  	client;	    	/* New client */
    Pdev_Request	*pdevReq;   	/* Pointer to current request */
    Pdev_Reply 	openReply;  		/* Reply to open request */
    Pdev_SetBufArgs	bufArgs;    	/* Structure to set r/w buffers */
    int	    	  	numBytes;   	/* Number of bytes read */
    xConnClientPrefix	*xccp;	    	/* Client's description of itself */
    Boolean    	  	swapped;    	/* Set TRUE if client is byte-swapped.
					 * FALSE otherwise. */
    Boolean 	  	writeBehind;	/* For IOC_PDEV_WRITE_BEHIND */

    pNewClients += *pNumNew;
    writeBehind = TRUE;	    	/* Turn on write-behind */

    while (1) {
	numBytes = read(Pdev_Conn, (char *) &note, sizeof(note));
	if ((numBytes != sizeof(note)) || (note.magic != PDEV_NOTIFY_MAGIC)) {
	    return;
	}

	Ioc_SetBits(note.newStreamID, IOC_NON_BLOCKING);
	
	pdevPriv = (PdevPrivPtr) malloc(sizeof(PdevPrivRec));
	pdevPriv->streamID = 	    	    	note.newStreamID;
	pdevPriv->client =  	    	    	NullClient;
	pdevPriv->state =   	    	    	PDEV_REQ_EMPTY|PDEV_READ_EMPTY;
	pdevPriv->curPtrs.magic =   	    	PDEV_BUF_PTR_MAGIC;
	pdevPriv->curPtrs.requestFirstByte =	-1;
	pdevPriv->curPtrs.requestLastByte = 	-1;
	pdevPriv->curPtrs.readFirstByte =   	-1;
	pdevPriv->curPtrs.readLastByte =    	-1;
	pdevPriv->inPtr =   	    	    	(char *)NULL;
	pdevPriv->outPtr =  	    	    	pdevPriv->outBuf;
	pdevPriv->overflow =	    	    	Buf_Init(0);
	pdevPriv->bigReq =  	    	    	(char *)NULL;
	pdevPriv->bigReqPtr =	    	    	(char *)NULL;

	pPriv = (ClntPrivPtr) malloc(sizeof(ClntPrivRec));
	pPriv->readProc =   	    	    	PdevReadClient;
	pPriv->writeProc =  	    	    	PdevWriteClient;
	pPriv->closeProc =  	    	    	PdevCloseClient;
	pPriv->mask =	    	    	    	(int *)0;
	pPriv->ready =	    	    	    	(int *)0;
	pPriv->maskWidth =  	    	    	0;
	pPriv->devicePrivate = 	    	    	(pointer)pdevPriv;
	
	bufArgs.requestBufAddr = pdevPriv->inBuf;
	bufArgs.requestBufSize = IN_BUF_SIZE;
	bufArgs.readBufAddr = pdevPriv->outBuf;
	bufArgs.readBufSize = OUT_BUF_SIZE;

	(void)Fs_IOControl(pdevPriv->streamID, IOC_PDEV_SET_BUF,
			   sizeof(bufArgs), (Address)&bufArgs,
			   0, (Address)NULL);
	(void)Fs_IOControl(pdevPriv->streamID, IOC_PDEV_WRITE_BEHIND,
			   sizeof(Boolean), (Address) &writeBehind,
			   0, (Address)NULL);

	ExpandMasks (pPriv, pdevPriv->streamID);

	/*
	 * Wait for and handle the OPEN request. If the request comes from
	 * an un-authorized source, we return a status of FS_NO_ACCESS. If
	 * there's an error, we close the stream and free everything.
	 */
	if ((PdevWaitForReadable(pdevPriv, pPriv->ready) != 0) ||
	    (pdevPriv->reqPtr->hdr.operation != PDEV_OPEN)) {
		(void) close(pdevPriv->streamID);
		free((char *)pdevPriv);
		free((char *)pPriv);
		continue;
	}
	pdevReq = pdevPriv->reqPtr;

	openReply.magic =   	PDEV_REPLY_MAGIC;
	openReply.selectBits = 	FS_WRITABLE | FS_READABLE;
	openReply.replySize =	0;
	openReply.replyBuf =	(Address)NULL;
	openReply.signal = 0;
	openReply.code = 0;

	if (InvalidHost(FamilySprite, pdevReq->param.open.uid,
			pdevReq->param.open.hostID)) {
	    openReply.status = FS_NO_ACCESS;
	    (void) Fs_IOControl(pdevPriv->streamID, IOC_PDEV_REPLY,
				sizeof(openReply), (Address)&openReply,
				0, (Address)NULL);
	    
	    (void) close(pdevPriv->streamID);
	    free((char *)pdevPriv);
	    free((char *)pPriv);
	    continue;
	} else {
	    openReply.status = SUCCESS;
	    (void) Fs_IOControl(pdevPriv->streamID, IOC_PDEV_REPLY,
				sizeof(openReply), (Address)&openReply,
				0, (Address)NULL);
	    
	}

	PdevRequestHandled(pdevPriv);

	/*
	 * Once the OPEN operation has been received, we must wait for the
	 * client to write an xConnClientPrefix on the device. Since the
	 * Sprite Xlib doesn't pass any extra authorization string junk,
	 * we only need to read the prefix.
	 */

	if (PdevWaitForReadable(pdevPriv, pPriv->ready) != 0) {
	    (void) close(pdevPriv->streamID);
	    free((char *)pdevPriv);
	    free((char *)pPriv);
	    continue;
	}
	pdevReq = pdevPriv->reqPtr;

	if (pdevReq->hdr.operation == PDEV_WRITE_ASYNC) {
	    pdevReq->hdr.operation = PDEV_WRITE;
	}
	if ((pdevReq->hdr.magic != PDEV_REQUEST_MAGIC) ||
	    (pdevReq->hdr.operation != PDEV_WRITE) ||
	    (pdevReq->hdr.requestSize < sizeof(xConnClientPrefix))) {
		(void) close(pdevPriv->streamID);
		free((char *)pdevPriv);
		free((char *)pPriv);
		continue;
	}

	xccp = (xConnClientPrefix *)&pdevReq[1];
	if (xccp->byteOrder != whichByteIsFirst) {
	    SwapConnClientPrefix (xccp);
	    swapped = TRUE;
	} else {
	    swapped = FALSE;
	}

	if ((xccp->majorVersion != X_PROTOCOL) ||
	    (xccp->minorVersion != X_PROTOCOL_REVISION)) {
		PdevRequestHandled(pdevPriv);
		PdevConnFail(pdevPriv, swapped, "Protocol version mismatch");
		(void) close(pdevPriv->streamID);
		free((char *)pdevPriv);
		free((char *)pPriv);
		continue;
	}
	if ((xccp->nbytesAuthProto != 0) || (xccp->nbytesAuthString != 0)) {
	    PdevRequestHandled(pdevPriv);
	    PdevConnFail(pdevPriv, swapped,
			    "Can't handle authorization strings");
	    (void) close(pdevPriv->streamID);
	    free((char *)pdevPriv);
	    free((char *)pPriv);
	    continue;
	}

	client = (ClientPtr)NextAvailableClient();
	if (client == NullClient) {
	    PdevRequestHandled(pdevPriv);
	    PdevConnFail(pdevPriv, swapped, "Too many clients");
	    (void) close(pdevPriv->streamID);
	    free((char *)pdevPriv);
	    free((char *)pPriv);
	    continue;
	} else {
	    pdevPriv->client = client;
	    client->osPrivate = (pointer)pPriv;
	    client->swapped = swapped;
	}
	
	PdevRequestHandled(pdevPriv);

	if (DBG(PDEV) || DBG(CONN)) {
	    ErrorF ("New Pdev connection: client %d (newID = %d)\n",
		    client->index, pdevPriv->streamID);
	}
	    
	if ((pdevPriv->state & PDEV_REQ_EMPTY) == 0) {
	    /*
	     * If there's still stuff left in the request buffer (beyond
	     * the initial open request), mark the stream as having input...
	     */
	    Bit_Set(pdevPriv->streamID, ClientsWithInputMask);
	}
	
	*pNewClients = client;
	pNewClients++;
	(*pNumNew)++;

	/*
	 * If we're only listening to one client, add the new client's
	 * stream to the saved AllClients and AllStreams masks so it is
	 * included when the grab is released. Otherwise, we can just
	 * add the stream to the regular AllClients and AllStreams
	 * masks.
	 */
	if (GrabDone) {
	    Bit_Set (pdevPriv->streamID, SavedAllClientsMask);
	    Bit_Set (pdevPriv->streamID, SavedAllStreamsMask);
	} else {
	    Bit_Set (pdevPriv->streamID, AllClientsMask);
	    Bit_Set (pdevPriv->streamID, AllStreamsMask);
	}
    }
}
    
/*-
 *-----------------------------------------------------------------------
 * PdevCloseClient --
 *	Some client is being booted. Free up our part of its connection
 *	resources. XXX: Maybe we should wait for the read-ahead buffer to
 *	drain?
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The pseudo-device stream is closed and the private data freed
 *	The stream is removed from these masks:
 *	    AllStreamsMask, SavedAllStreamsMask, AllClientsMask,
 *	    SavedAllClientsMask, ClientsWithInputMask
 *
 *-----------------------------------------------------------------------
 */
static void
PdevCloseClient (pPriv)
    ClntPrivPtr	  pPriv;    	/* Private data for client whose connection
				 * should be closed */
{
    register PdevPrivPtr pdevPriv;
    register int  	    streamID;

    pdevPriv = (PdevPrivPtr) pPriv->devicePrivate;
    
    streamID = pdevPriv->streamID;
    close(streamID);
    Bit_Clear(streamID, AllStreamsMask);
    Bit_Clear(streamID, SavedAllStreamsMask);
    Bit_Clear(streamID, AllClientsMask);
    Bit_Clear(streamID, SavedAllClientsMask);
    Bit_Clear(streamID, ClientsWithInputMask);
    Buf_Destroy(pdevPriv->overflow, TRUE);

    if (pdevPriv->state & PDEV_COLLECTING) {
	free((char *) pdevPriv->bigReq);
    }
	    
    free((char *) pdevPriv);
}

#define request_length(req, cli) \
 	(((cli)->swapped?lswaps(((xReq *)(req))->length):\
	  ((xReq *)(req))->length) << 2)


/*-
 *-----------------------------------------------------------------------
 * PdevReadClient --
 *	Return one request from the given client. If the client is being
 *	naughty, we call ConnectionClosed to close the thing down.
 *	The client will be closed down as well if the stream which acted
 *	up was the only one open for the client. We still return NULL
 *	with status 0 from there, but the client is gone...
 *
 * Results:
 *	The request buffer.
 *
 * Side Effects:
 *	ClientsWithInputMask may be modified.
 *
 *-----------------------------------------------------------------------
 */
/*ARGSUSED*/
static char *
PdevReadClient (pPriv, pStatus, oldbuf)
    ClntPrivPtr	  	pPriv;	    /* Client with input */
    int	    	  	*pStatus;   /* Result of read:
				     *	>0 -- number of bytes in the request
				     *   0  -- not all the request is there
				     *	<0 -- indicates an error */
    char    	  	*oldbuf;    /* The previous buffer */
{
    PdevPrivPtr  	pdevPriv;   /* Private data for the client */
    int	    	  	need;	    /* Number of bytes needed for the
				     * current request */
    xReq    	  	*reqPtr;    /* Request to give back */
    Pdev_BufPtrs  	bufPtrs;    /* New pointers for buffer */
    int	    	  	numBytes;
    Pdev_ReplyData 	reply;

    pdevPriv = (PdevPrivPtr) pPriv->devicePrivate;

    Bit_Clear (pdevPriv->streamID, ClientsWithInputMask);
    
    if (pdevPriv->state & PDEV_REQ_EMPTY) {
	if (Bit_IsSet(pdevPriv->streamID, pPriv->ready)) {
	    /*
	     * If the request buffer is empty, we think, then we need to see if
	     * the kernel has anything for us. However, we only do the read on
	     * the device if we actually got here because selecdt says the
	     * stream is ready. This prevents a client from hogging the server
	     * in an obnoxious fashion...
	     */
	    numBytes = read(pdevPriv->streamID, &bufPtrs, sizeof(bufPtrs));
	    if (numBytes == -1) {
		if (errno != EWOULDBLOCK) {
		    if (DBG(PDEV)) {
			Error("Reading buffer pointers");
		    }
		    *pStatus = -1;
		    return((char *)NULL);
		} else {
		    /*
		     * Wasn't actually anything to read. Go on to
		     * the next client...
		     */
		    *pStatus = 0;
		    SchedYield();
		    return ((char *)NULL);
		}
	    }
	    if ((numBytes != sizeof(bufPtrs)) ||
		(bufPtrs.magic != PDEV_BUF_PTR_MAGIC)) {
		    if (DBG(PDEV)) {
			ErrorF("Improper data when reading buffer pointers");
		    }
		    *pStatus = -1;
		    return ((char *)NULL);
	    }
	    PdevSetPtrs(pdevPriv, &bufPtrs);
	}
	if (pdevPriv->state & PDEV_REQ_EMPTY) {
	    /*
	     * If there are still no requests, there's nothing more to
	     * be done. PdevSetPtrs will have copied any overflow
	     * to the read-ahead buffer, so...
	     */
	    *pStatus = 0;
	    SchedYield();
	    return ((char *)NULL);
	}
    }

    Bit_Clear (pdevPriv->streamID, pPriv->ready);
    
    /*
     * Process the requests until we get one that's a WRITE request,
     * then set inPtr to the data for the request and break out
     * of the loop.
     */
    while ((pdevPriv->inPtr == (char *)NULL) &&
	   !(pdevPriv->state & PDEV_REQ_EMPTY))
    {
	switch (pdevPriv->reqPtr->hdr.operation) {
	    case PDEV_WRITE_ASYNC:
	    case PDEV_WRITE:
		if (DBG(PDEV)) {
		    ErrorF("client %d: PDEV_WRITE(%d)\n",
			   pdevPriv->client->index,
			   pdevPriv->reqPtr->hdr.requestSize);
		}
		pdevPriv->inPtr = (char *)&pdevPriv->reqPtr[1];
		break;
	    case PDEV_CLOSE:
		/*
		 * We like closes. We just return -1 to cause the
		 * connection to be aborted.
		 */
		if (DBG(PDEV) || DBG(CONN)) {
		    ErrorF("client %d: PDEV_CLOSE\n", pdevPriv->client->index);
		}
		reply.magic = PDEV_REPLY_MAGIC;
		reply.status = SUCCESS;
		reply.selectBits = 0;
		reply.replySize = 0;
		reply.replyBuf = (Address)NULL;
		reply.signal = 0;
		reply.code = 0;
		(void)Fs_IOControl(pdevPriv->streamID,
				   IOC_PDEV_REPLY,
				   sizeof(reply), (Address)&reply,
				   0, (Address)NULL);
		PdevRequestHandled(pdevPriv);
		*pStatus = -1;
		return ((char *)NULL);
	    case PDEV_IOCTL: {
		/*
		 * All this should be unnecessary, but we reply to it
		 * anyway and pretend we know what we're doing when it
		 * comes to the IOC_NUM_READABLE
		 */
		char    *inBuffer;
		int	replyBuf;
		int	command;
		
		reply.magic = PDEV_REPLY_MAGIC;
		reply.status = SUCCESS;
		reply.selectBits =
		    ((pdevPriv->state & PDEV_READ_EMPTY)?0:FS_READABLE) |
			FS_WRITABLE;
		reply.replySize = 0;
		reply.replyBuf = (Address)&replyBuf;
		
		if (pdevPriv->reqPtr->hdr.requestSize) {
		    inBuffer = (char *)&pdevPriv->reqPtr[1];
		} else {
		    inBuffer = (char *)NULL;
		}
		if (DBG(PDEV)) {
		    ErrorF("client %d: PDEV_IOCTL(%x, %d, %x, %d, ...)\n",
			   pdevPriv->client->index,
			   pdevPriv->reqPtr->param.ioctl.command,
			   pdevPriv->reqPtr->hdr.requestSize,
			   inBuffer,
			   pdevPriv->reqPtr->hdr.replySize);
		}
		
		switch (pdevPriv->reqPtr->param.ioctl.command) {
		    case IOC_NUM_READABLE:
			/*
			 * XXX: This is already handled by the
			 * kernel since it knows how much it has
			 * more accurately than I do.
			 */
			reply.replySize = sizeof(int);
			replyBuf = pdevPriv->curPtrs.readLastByte -
			    pdevPriv->curPtrs.readFirstByte;
			break;
		    case IOC_SET_BITS:
		    case IOC_SET_FLAGS:
			break;
		    case IOC_CLEAR_BITS:
		    case IOC_GET_FLAGS:
			replyBuf = 0;
			reply.replySize = sizeof(int);
			break;
		    default:
			if (DBG(PDEV)) {
			    ErrorF("Invalid IOCTL %x\n",
				   pdevPriv->reqPtr->param.ioctl.command);
			}
			reply.status = FS_DEVICE_OP_INVALID;
			break;
		}
		reply.signal = 0;
		reply.code = 0;
		if (reply.replySize > 0) {
		    *(int *)reply.data = replyBuf;
		    command = IOC_PDEV_SMALL_REPLY;
		} else {
		    command = IOC_PDEV_REPLY;
		}
		(void)Fs_IOControl(pdevPriv->streamID,
				   command,
				   sizeof(reply), (Address)&reply,
				   0, (Address)NULL);
		PdevRequestHandled(pdevPriv);
		break;
	    }
	    default:
		/*
		 * Bad pseudo-device request -- close the beastie down
		 */
		reply.magic = PDEV_REPLY_MAGIC;
		reply.status = FS_DEVICE_OP_INVALID;
		reply.selectBits = 0;
		reply.replySize = 0;
		reply.replyBuf = (Address)NULL;
		reply.signal = 0;
		reply.code = 0;
		if (DBG(PDEV)) {
		    ErrorF("Bad new pdev request %d client %d\n",
			   pdevPriv->reqPtr->hdr.operation,
			   pdevPriv->client->index);
		}
		(void)Fs_IOControl(pdevPriv->streamID,
				   IOC_PDEV_REPLY,
				   sizeof(reply), (Address)&reply,
				   0, (Address)NULL);
		PdevRequestHandled(pdevPriv);
		*pStatus = -1;
		return ((char *)NULL);
	}
    }
    if (pdevPriv->inPtr == (char *)NULL) {
	/*
	 * If didn't reach a PDEV_WRITE request, there's nothing more
	 * to do...
	 */
	Bit_Clear(pdevPriv->streamID, ClientsWithInputMask);
	SchedYield();
	*pStatus = 0;
	return ((char *)NULL);
    }
    
    if (pdevPriv->state & PDEV_COLLECTING) {
	/*
	 * Collecting an X request that was broken across a WRITE boundary.
	 * bigReqPtr points to the next place to store stuff for the
	 * request, while need contains the number of bytes still needed
	 * to fill the request. bigReq has been allocated to contain
	 * the requisite space.
	 */
	need = min(pdevPriv->need, pdevPriv->reqPtr->hdr.requestSize);
	bcopy(pdevPriv->inPtr, pdevPriv->bigReqPtr, need);
	pdevPriv->bigReqPtr += need;
	pdevPriv->need -= need;
	pdevPriv->inPtr += need;
	if (pdevPriv->need != 0) {
	    PdevRequestHandled(pdevPriv);
	    *pStatus = 0;
	    return ((char *)NULL);
	} else {
	    pdevPriv->reqPtr->hdr.requestSize -= need;
	    need = request_length(pdevPriv->bigReq, pdevPriv->client);
	    *pStatus = need;
	    pdevPriv->state &= ~PDEV_COLLECTING;
	    Bit_Set(pdevPriv->streamID, ClientsWithInputMask);
	    if (DBG(PDEV)) {
		ErrorF("XRequest(%d, %d) #%d\n", pdevPriv->client->index,
		       ((xReq *)pdevPriv->bigReq)->reqType,
		       pdevPriv->client->sequence + 1);
	    }
	    return (pdevPriv->bigReq);
	}
    } else if (pdevPriv->state & PDEV_COLLECTING_HEADER) {
	/*
	 * The header for the request was broken across a WRITE boundary.
	 * bigReqPtr points to the place we left off in the header,
	 * while bigReq points to a piece of memory big enough for
	 * an xReq and no more.  Once the entire request header is seen,
	 * we can allocate a large enough buffer for the entire request.
	 */
	need = min(pdevPriv->need, pdevPriv->reqPtr->hdr.requestSize);
	bcopy(pdevPriv->inPtr, pdevPriv->bigReqPtr, need);
	pdevPriv->bigReqPtr += need;
	pdevPriv->need -= need;
	pdevPriv->inPtr += need;
	if (pdevPriv->need != 0) {
	    /*
	     * XXX: Shouldn't happen
	     */
	    PdevRequestHandled(pdevPriv);
	    *pStatus = 0;
	    return ((char *)NULL);
	} else {
	    /*
	     * We've now gotten a complete header. To keep this simple
	     * we just allocate the needed buffer, copy the header in,
	     * switch to COLLECTING state from COLLECTING_HEADER and
	     * return a blocked read. We'll get called again (very
	     * soon) to fill out the request...
	     */
	    xReq	*xreq;

	    xreq = (xReq *)pdevPriv->bigReq;
	    need = request_length(xreq, pdevPriv->client);
	    pdevPriv->bigReq = malloc((unsigned) need);
	    bcopy(xreq, pdevPriv->bigReq, sizeof(xReq));
	    pdevPriv->bigReqPtr += sizeof(xReq);
	    pdevPriv->need = need - sizeof(xReq);
	    pdevPriv->state ^= PDEV_COLLECTING|PDEV_COLLECTING_HEADER;
	    free((char *) xreq);
	    *pStatus = 0;
	    return ((char *)NULL);
	}
    }
    if (pdevPriv->bigReq != (char *)NULL) {
	/*
	 * If we're not collecting anything and there's something
	 * pointed to by bigReq, it must be old and should be
	 * deallocated.
	 */
	free((char *) pdevPriv->bigReq);
	pdevPriv->bigReq = (char *)NULL;
    }
    if (pdevPriv->reqPtr->hdr.requestSize < sizeof(xReq)) {
	if (pdevPriv->reqPtr->hdr.requestSize != 0) {
	    /*
	     * Only go into the COLLECTING_HEADER state if there are
	     * data bytes left in the request (i.e. it's really a broken
	     * header, not just an exhausted WRITE request).
	     */
	    pdevPriv->state |= PDEV_COLLECTING_HEADER;
	    pdevPriv->bigReq = (char *) malloc(sizeof(xReq));
	    bcopy(pdevPriv->inPtr, pdevPriv->bigReq,
		    pdevPriv->reqPtr->hdr.requestSize);
	    pdevPriv->bigReqPtr =
		pdevPriv->bigReq + pdevPriv->reqPtr->hdr.requestSize;
	    pdevPriv->need = sizeof(xReq) - pdevPriv->reqPtr->hdr.requestSize;
	}
	PdevRequestHandled(pdevPriv);
	*pStatus = 0;
	return ((char *)NULL);
    } else {
	/*
	 * Can actually figure out the size of the request. Store it
	 * in 'need'
	 */
	need = request_length (pdevPriv->inPtr, pdevPriv->client);
	
	if (need > pdevPriv->reqPtr->hdr.requestSize) {
	    /*
	     * The X request didn't fit in PDEV_WRITE request, so
	     * we mark the stream as collecting a (big) request, copy
	     * in whatever data we have and return a blocked read status.
	     * Note we DO NOT yield the server since there may be another
	     * write request pending.
	     */
	    pdevPriv->state |= PDEV_COLLECTING;
	    pdevPriv->bigReq = (char *) malloc((unsigned) need);
	    bcopy(pdevPriv->inPtr, pdevPriv->bigReq,
		    pdevPriv->reqPtr->hdr.requestSize);
	    pdevPriv->bigReqPtr =
		pdevPriv->bigReq + pdevPriv->reqPtr->hdr.requestSize;
	    pdevPriv->need = need - pdevPriv->reqPtr->hdr.requestSize;
	    PdevRequestHandled(pdevPriv);
	    *pStatus = 0;
	    return ((char *)NULL);
	} else {
	    reqPtr = (xReq *)pdevPriv->inPtr;
	    pdevPriv->inPtr += need;
	    pdevPriv->reqPtr->hdr.requestSize -= need;
	    *pStatus = need;
	    Bit_Set(pdevPriv->streamID, ClientsWithInputMask);
	    if (DBG(PDEV)) {
		ErrorF("XRequest(%d, %d) #%d\n", pdevPriv->client->index,
		       reqPtr->reqType, pdevPriv->client->sequence+1);
	    }
	    return ((char *)reqPtr);
	}
    }
}

/*-
 *-----------------------------------------------------------------------
 * PdevWriteClient --
 *	Write data to the client. Data are copied into the read buffer
 *	as much as possible. Data that cannot fit in the read buffer are
 *	placed in the overflow buffer until the kernel catches up with us,
 *	at which point the data are copied back down to the read buffer.
 *	Output packets may be broken at arbitrary points and all
 *	transmissions are rounded to be a multiple of 32 bits long. Because
 *	of this, if the entire packet fits into the read buffer, the
 *	pad bytes do too.
 *
 * Results:
 *	Number of bytes written.
 *
 * Side Effects:
 *	The data bytes are stuffed into the buffer for the client, awaiting
 *	a read request from the client.
 *
 *-----------------------------------------------------------------------
 */
static int
PdevWriteClient (pPriv, numBytes, xRepPtr)
    ClntPrivPtr	  pPriv;    	/* Client to receive the data */
    int	    	  numBytes; 	/* Number of bytes (unrounded) */
    Address    	  xRepPtr; 	/* The data bytes */
{
    int	    	  	    rndNumBytes;    /* Number of bytes, rounded to a
					     * longword */
    int	    	  	    pad = 0;	    /* Padding bytes */
    register int  	    padding;	    /* Number of bytes needed */
    register PdevPrivPtr    pdevPriv;	    /* Data private to client */
    ReturnStatus	    status;

    rndNumBytes = (numBytes + 3) & ~3;

    pdevPriv = (PdevPrivPtr) pPriv->devicePrivate;
    
    
    padding = rndNumBytes - numBytes;

    if (pdevPriv->curPtrs.readLastByte == (OUT_BUF_SIZE - 1)) {
	/*
	 * Output buffer is full -- must copy data to overflow buffer
	 */
	if (DBG(PDEV)) {
	    ErrorF("WriteClient(%d): overflow -- ", pdevPriv->client->index);
	}
	Buf_AddBytes(pdevPriv->overflow, numBytes, (Byte *)xRepPtr);
	if (padding != 0) {
	    Buf_AddBytes(pdevPriv->overflow, padding, (Byte *)&pad);
	}
    } else {
	int 	  numWrite;

	numWrite = min(numBytes,OUT_BUF_SIZE - pdevPriv->curPtrs.readLastByte);
	bcopy((char *) xRepPtr, (char *) pdevPriv->outPtr, numWrite);
	numBytes -= numWrite;

	/*
	 * Update the output pointers. Note that since readLastByte starts
	 * at -1, adding numWrite will point to the last valid byte of data
	 * in the buffer...
	 */
	pdevPriv->curPtrs.readLastByte += numWrite;
	pdevPriv->outPtr += numWrite;
	
	if (numBytes != 0) {
	    /*
	     * Didn't manage to fit the entire request into the read-ahead
	     * buffer. Copy excess to overflow.
	     */
	    Buf_AddBytes(pdevPriv->overflow,
			 numBytes,
			 ((Byte *)xRepPtr) + numWrite);
	    if (padding != 0) {
		Buf_AddBytes(pdevPriv->overflow,
			     padding,
			     (Byte *)&pad);
	    }
	} else if (padding != 0) {
	    /*
	     * Because the buffer is a multiple of 32-bits long, any rounding
	     * that needs to be done is guaranteed to fit if the unrounded
	     * packet fit.
	     */
	    bcopy((char *) &pad, pdevPriv->outPtr, padding);
	    pdevPriv->curPtrs.readLastByte += padding;
	    pdevPriv->outPtr += padding;
	}

	/*
	 * Inform kernel of new read buffer extents
	 */
	status = Fs_IOControl(pdevPriv->streamID, IOC_PDEV_SET_PTRS,
		sizeof(pdevPriv->curPtrs), (Address) &pdevPriv->curPtrs,
		0, (Address) NULL);
	if (status != 0) {
	    errno = Compat_MapCode(status);
	    if (DBG(PDEV)) {
		Error("WriteClient: SET_PTRS");
	    }
	}
	if (DBG(PDEV)) {
	    ErrorF("WriteClient(%d): req %d:%d read %d:%d ",
		   pdevPriv->client->index,
		   pdevPriv->curPtrs.requestFirstByte,
		   pdevPriv->curPtrs.requestLastByte,
		   pdevPriv->curPtrs.readFirstByte,
		   pdevPriv->curPtrs.readLastByte);
	    
	    switch (((xReply *)xRepPtr)->generic.type) {
		case X_Reply:
		    ErrorF("Reply to %d\n",
			   ((xReply *)xRepPtr)->generic.sequenceNumber);
		    break;
		case X_Error:
		    ErrorF("Error for %d\n",
			   ((xReply *)xRepPtr)->generic.sequenceNumber);
		    break;
		case KeyPress: ErrorF("KeyPress event\n"); break;
		case KeyRelease: ErrorF("KeyRelease event\n"); break;
		case ButtonPress: ErrorF("ButtonPress event\n"); break;
		case ButtonRelease: ErrorF("ButtonRelease event\n"); break;
		case MotionNotify: ErrorF("MotionNotify event\n"); break;
		case EnterNotify: ErrorF("EnterNotify event\n"); break;
		case LeaveNotify: ErrorF("LeaveNotify event\n"); break;
		case FocusIn: ErrorF("FocusIn event\n"); break;
		case FocusOut: ErrorF("FocusOut event\n"); break;
		case KeymapNotify: ErrorF("KeymapNotify event\n"); break;
		case Expose: ErrorF("Expose event\n"); break;
		case GraphicsExpose: ErrorF("GraphicsExpose event\n"); break;
		case NoExpose: ErrorF("NoExpose event\n"); break;
		case VisibilityNotify: ErrorF("VisibilityNotify event\n"); break;
		case CreateNotify: ErrorF("CreateNotify event\n"); break;
		case DestroyNotify: ErrorF("DestroyNotify event\n"); break;
		case UnmapNotify: ErrorF("UnmapNotify event\n"); break;
		case MapNotify: ErrorF("MapNotify event\n"); break;
		case MapRequest: ErrorF("MapRequest event\n"); break;
		case ReparentNotify: ErrorF("ReparentNotify event\n"); break;
		case ConfigureNotify: ErrorF("ConfigureNotify event\n"); break;
		case ConfigureRequest: ErrorF("ConfigureRequest event\n"); break;
		case GravityNotify: ErrorF("GravityNotify event\n"); break;
		case ResizeRequest: ErrorF("ResizeRequest event\n"); break;
		case CirculateNotify: ErrorF("CirculateNotify event\n"); break;
		case CirculateRequest: ErrorF("CirculateRequest event\n"); break;
		case PropertyNotify: ErrorF("PropertyNotify event\n"); break;
		case SelectionClear: ErrorF("SelectionClear event\n"); break;
		case SelectionRequest: ErrorF("SelectionRequest event\n"); break;
		case SelectionNotify: ErrorF("SelectionNotify event\n"); break;
		case ColormapNotify: ErrorF("ColormapNotify event\n"); break;
		case ClientMessage: ErrorF("ClientMessage event\n"); break;
		case MappingNotify: ErrorF("MappingNotify event\n"); break;
		default:
		    ErrorF("Data\n");
	    }
	}
    }
    return (numBytes);
}
