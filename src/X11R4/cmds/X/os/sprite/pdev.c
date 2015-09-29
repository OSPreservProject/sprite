/*-
 * pdev.c --
 *	Functions for handling a connection to a client over a pseudo-device.
 *	Goal of these routine is to make a pdev connection look like a 
 *	Unix domain socket.
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
 *
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
"$Header: /X11/R4/src/cmds/X/os/sprite/RCS/pdev.c,v 1.13 91/10/19 13:29:27 mendel Exp $ SPRITE (Berkeley)";
#endif lint

#define NEED_REPLIES /* For Debugging Only */

#define DEBUG_PDEV	0
#define	DBG_PRINT	ErrorF

#define	min(a,b)	(((a) < (b)) ? (a) : (b))

#include    <fs.h>
#include    <stdlib.h>
#include    <dev/pdev.h>
#include    <errno.h>
#include    <status.h>
#include    <stdio.h>
#include    <sys/time.h>
#include <sys/types.h>
#include <sys/uio.h>

#define OUT_BUF_SIZE	    2048
#define IN_BUF_SIZE	    2048

/*
 * The private data maintained for a pseudo-device client
 */
typedef struct {
    int	    	  	streamID;   /* Server stream over which we get the
				     * buffer pointers */
    int	    	  	state;
#define	PDEV_WRITE_STALL	0x0001	/* Stall on write request. */
#define PDEV_REQ_EMPTY	    	0x0002	/* Request buffer is empty */
#define PDEV_READ_EMPTY	    	0x0004 	/* Read buffer is empty */

    /*
     * Request (inBuf) and read-ahead (outBuf) buffers
     */
    char  	  	outBuf[OUT_BUF_SIZE];
    char    	  	*outPtr;    /* Next place to store data */

    char    	  	inBuf[IN_BUF_SIZE];
    Pdev_Request	*reqPtr;    /* Address of next request to process */
    char    	  	*inPtr;	    /* Position in current request */
    int			inSize;	    /* Amount of data valid in inBuf write 
				     * request. */

    Pdev_BufPtrs  	curPtrs;    /* Current pointers for the two buffers */

} PdevPrivRec, *PdevPrivPtr;

#define	MAX_FD	1024
static PdevPrivPtr privRecPtrs[MAX_FD];
static int PdevMaster = -2;

static void PdevSetPtrs();
static void PdevRequestHandled();

/*-
 *-----------------------------------------------------------------------
 * PdevCreate --
 *	Create a pseudo-device connection MASTER.
 *
 * Results:
 *	Open Pdev file descriptor. -1 on error. (errno set)
 *
 * Side Effects:
 *	The pseudo-device MASTER is created are opened.
 *
 *-----------------------------------------------------------------------
 */
int
PdevCreate(pathName)
    char    *pathName;	/* The pathname of PDEV */
{
    int	    	  pdevConn;
    ReturnStatus  status;

    /*
     * Create the pseudo-device, 
     */

    status = Fs_Open (pathName,
	    FS_NON_BLOCKING | FS_CREATE | FS_READ | FS_PDEV_MASTER,
	    0666, &pdevConn);
    if (status != 0) {
	errno = Compat_MapCode(status);
	DBG_PRINT (pathName);
	return -1;
    }
    PdevMaster = pdevConn;
    return pdevConn;
}

/*-
 *-----------------------------------------------------------------------
 * PdevAccept --
 *	Accept connections from new clients. 
 *
 * Results:
 *	Stream ID of new connection -1 if error.
 *
 * Side Effects:
 *	Memory is allocated.
 *
 *-----------------------------------------------------------------------
 */
int
PdevAccept (pdevDev)
    int			pdevDev;	/* Pdev master. */
{
    Pdev_Notify	  	note;	    	/* Notification of new stream */
    PdevPrivPtr		pdevPriv;   	/* New private information for us */
    Pdev_Request	*pdevReq;   	/* Pointer to current request */
    Pdev_Reply 	openReply;  		/* Reply to open request */
    Pdev_SetBufArgs	bufArgs;    	/* Structure to set r/w buffers */
    int	    	  	numBytes;   	/* Number of bytes read */
    Boolean 	  	writeBehind;	/* For IOC_PDEV_WRITE_BEHIND */
    Pdev_BufPtrs  	bufPtrs;    /* New buffer pointers for stream */

    writeBehind = TRUE;	    	/* Turn on write-behind */

    /*
     * Read the connection request. 
     */
    numBytes = read(pdevDev, (char *) &note, sizeof(note));
    if ((numBytes != sizeof(note)) || (note.magic != PDEV_NOTIFY_MAGIC)) {
	return -1;
    }
    /*
     * If streamID is too large ignore connection.
     */
    if (note.newStreamID >= MAX_FD) {
	(void) close(note.newStreamID);
	return -1;
    }

    /*
     * Allocate some memory for the private data of request.
     */
    pdevPriv = (PdevPrivPtr) malloc(sizeof(PdevPrivRec));
    pdevPriv->streamID = 	    	    	note.newStreamID;
    pdevPriv->state =   	    	    	PDEV_REQ_EMPTY|PDEV_READ_EMPTY;
    pdevPriv->curPtrs.magic =   	    	PDEV_BUF_PTR_MAGIC;
    pdevPriv->curPtrs.requestFirstByte =	-1;
    pdevPriv->curPtrs.requestLastByte = 	-1;
    pdevPriv->curPtrs.readFirstByte =   	-1;
    pdevPriv->curPtrs.readLastByte =    	-1;
    pdevPriv->inPtr =   	    	    	(char *)NULL;
    pdevPriv->inSize =   	    	    	0;
    pdevPriv->outPtr =  	    	    	pdevPriv->outBuf;

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
    /*
     * Wait for open request to come in from client.
     */
    numBytes = read(pdevPriv->streamID, (char *) &bufPtrs, sizeof(bufPtrs));
    if (numBytes == -1) {
	if (DEBUG_PDEV) {
	    DBG_PRINT ("PdevWaitForReadable");
	}
	free((char *) pdevPriv);
	return (-1);
    }
    if (bufPtrs.magic != PDEV_BUF_PTR_MAGIC) {
	if (DEBUG_PDEV) {
	    DBG_PRINT ("Buffer magic numbers don't match\n");
	}
	free((char *) pdevPriv);
	return (-1);
    }
    PdevSetPtrs(pdevPriv, &bufPtrs);
    if (pdevPriv->reqPtr->hdr.operation == PDEV_OPEN) {
	pdevReq = pdevPriv->reqPtr;

	openReply.magic =   	PDEV_REPLY_MAGIC;
	openReply.selectBits = 	FS_WRITABLE | FS_READABLE;
	openReply.replySize =	0;
	openReply.replyBuf =	(Address)NULL;
	openReply.signal = 0;
	openReply.code = 0;

	openReply.status = SUCCESS;
	(void) Fs_IOControl(pdevPriv->streamID, IOC_PDEV_REPLY,
				sizeof(openReply), (Address)&openReply,
				0, (Address)NULL);
    }

    PdevRequestHandled(pdevPriv);
    /*
     * Mark stream as non-blocking and save private data away.
     */
    Ioc_SetBits(pdevPriv->streamID, IOC_NON_BLOCKING); 
    privRecPtrs[pdevPriv->streamID] = pdevPriv;
    return pdevPriv->streamID;
}
    
/*-
 *-----------------------------------------------------------------------
 * PdevClose --
 *	Close down a connection.
 *	XXX: Maybe we should wait for the read-ahead buffer to drain?
 *
 * Results:
 *	-1 if error. 0 if success. errno set.
 *
 * Side Effects:
 *	The pseudo-device stream is closed and the private data freed
 *	The stream is removed from these masks:
 *	    AllStreamsMask, SavedAllStreamsMask, AllClientsMask,
 *	    SavedAllClientsMask, ClientsWithInputMask
 *
 *-----------------------------------------------------------------------
 */
int
PdevClose (pdev)
    int pdev;
{
    register PdevPrivPtr pdevPriv;

    if ((pdev < 0) || (pdev >= MAX_FD)) {
	errno = EINVAL;
	return -1;
    }
    pdevPriv = privRecPtrs[pdev];
    if (pdevPriv == (PdevPrivPtr) NULL) {
	errno = EINVAL;
	return -1;
    }

    privRecPtrs[pdev] = (PdevPrivPtr) NULL;
    free((char *) pdevPriv);

    return close(pdev);
}



/*-
 *-----------------------------------------------------------------------
 * PdevRead --
 *	Return bytes from the given client. 
 *
 * Results:
 *	Number of bytes read. -1 if error and errno set.
 *
 * Side Effects:
 *
 *-----------------------------------------------------------------------
 */
/*ARGSUSED*/
int
PdevRead (pdev, bufferPtr, bufLen)
    int			pdev;
    char		*bufferPtr;
    int			bufLen;
{
    PdevPrivPtr  	pdevPriv;   /* Private data for the client */
    int	    	  	need;	    /* Number of bytes needed for the
				     * current request */
    Pdev_BufPtrs  	bufPtrs;    /* New pointers for buffer */
    int	    	  	numBytes;
    Pdev_ReplyData 	reply;

    if ((pdev < 0) || (pdev >= MAX_FD)) {
	errno = EINVAL;
	return -1;
    }
    pdevPriv = privRecPtrs[pdev];
    if (pdevPriv == (PdevPrivPtr) NULL) {
	errno = EINVAL;
	return -1;
    }
    numBytes = 0;
    /*
     * Use any data waiting from the last WRITE request. 
     */
    if (pdevPriv->inSize > 0) {
	numBytes = min(pdevPriv->inSize, bufLen);
	bcopy(pdevPriv->inPtr, bufferPtr, numBytes);
	bufLen -= numBytes;
	pdevPriv->inSize -= numBytes;
	if (pdevPriv->inSize == 0) {
	    PdevRequestHandled(pdevPriv);
	}
    }
    if (bufLen == 0) {
	return numBytes;
    }
    if (pdevPriv->state & PDEV_REQ_EMPTY) {
	int count;
	/*
	 * If the request buffer is empty, we think, then we need to see if
	 * the kernel has anything for us. 
	 */
	count = read(pdev, &bufPtrs, sizeof(bufPtrs));
	if (count == -1) {
	    if (errno != EWOULDBLOCK) {
		if (DEBUG_PDEV) {
		    DBG_PRINT ("Reading buffer pointers");
		}
		return(-1);
	    } 
	} else if ((count != sizeof(bufPtrs)) ||
	    (bufPtrs.magic != PDEV_BUF_PTR_MAGIC)) {
		if (DEBUG_PDEV) {
		    DBG_PRINT ("Improper data when reading buffer pointers");
		}
		return (-1);
	} else {
	    PdevSetPtrs(pdevPriv, &bufPtrs);
	}
    }

    /*
     * Process the requests until we run out or get enought data.
     */
    while (!(pdevPriv->state & PDEV_REQ_EMPTY) && (bufLen > 0)) {
	switch (pdevPriv->reqPtr->hdr.operation) {
	    case PDEV_WRITE_ASYNC:
	    case PDEV_WRITE: {
		int got;
		if (DEBUG_PDEV) {
		    DBG_PRINT ("pdev %d: PDEV_WRITE(%d)\n",
			   pdev,
			   pdevPriv->reqPtr->hdr.requestSize);
		}
		pdevPriv->inPtr = (char *)&pdevPriv->reqPtr[1];
		pdevPriv->inSize = pdevPriv->reqPtr->hdr.requestSize;
		got = min(bufLen, pdevPriv->inSize);
		bcopy(pdevPriv->inPtr, bufferPtr + numBytes, got);
		bufLen -= got;
		pdevPriv->inSize -= got;
		pdevPriv->inPtr += got;
		numBytes += got;
		if (pdevPriv->inSize == 0) { 
		    PdevRequestHandled(pdevPriv);
		}
		break;
	    }
	    case PDEV_CLOSE:
		/*
		 * We like closes. We just return -1 to cause the
		 * connection to be aborted.
		 */
		if (DEBUG_PDEV) {
		    DBG_PRINT ("client %d: PDEV_CLOSE\n", pdevPriv->streamID);
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
		errno = ECONNRESET;
		return (-1);
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
		if (DEBUG_PDEV) {
		    DBG_PRINT ("pdev %d: PDEV_IOCTL(%x, %d, %x, %d, ...)\n",
			   pdevPriv->streamID,
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
			if (DEBUG_PDEV) {
			    DBG_PRINT ("Invalid IOCTL %x\n",
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
		if (DEBUG_PDEV) {
		    DBG_PRINT ("Bad new pdev request %d pdev %d\n",
			   pdevPriv->reqPtr->hdr.operation,
			   pdevPriv->streamID);
		}
		(void)Fs_IOControl(pdevPriv->streamID,
				   IOC_PDEV_REPLY,
				   sizeof(reply), (Address)&reply,
				   0, (Address)NULL);
		PdevRequestHandled(pdevPriv);
		errno = ECONNRESET;
		return (-1);
	}
    }
    if (numBytes == 0) {
	errno = EWOULDBLOCK;
	return (-1);
    }
    return numBytes;
}

/*-
 *-----------------------------------------------------------------------
 * PdevWritev --
 *	Write data to the client. Data are copied into the read buffer
 *	as much as possible. 
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
int
PdevWritev(pdev, iov, iovcnt)
    int		pdev;
    struct iovec *iov;
    int iovcnt;
{
    register PdevPrivPtr    pdevPriv;	    /* Data private to client */
    ReturnStatus	    status;
    int 	  numWrite, numWritten, i;


    if ((pdev < 0) || (pdev >= MAX_FD)) {
	errno = EINVAL;
	return -1;
    }
    pdevPriv = privRecPtrs[pdev];
    if (pdevPriv == (PdevPrivPtr) NULL) {
	errno = EINVAL;
	return -1;
    }

    if (pdevPriv->curPtrs.readLastByte >= (OUT_BUF_SIZE - 1)) {
	pdevPriv->state |= PDEV_WRITE_STALL;
	errno = EWOULDBLOCK;
	return -1;
    }
    pdevPriv->state &= ~PDEV_WRITE_STALL;

    numWritten = 0;
    for (i = 0; (i < iovcnt) && 
		(pdevPriv->curPtrs.readLastByte < (OUT_BUF_SIZE - 1)); i++) {
	int spaceAvailable;

	spaceAvailable = (pdevPriv->curPtrs.readLastByte == -1) ? OUT_BUF_SIZE 
			    : (OUT_BUF_SIZE - pdevPriv->curPtrs.readLastByte);

	numWrite = min(iov[i].iov_len, spaceAvailable);
	bcopy((char *) iov[i].iov_base, (char *) pdevPriv->outPtr, numWrite);

	/*
	 * Update the output pointers. Note that since readLastByte starts
	 * at -1, adding numWrite will point to the last valid byte of data
	 * in the buffer...
	 */
	pdevPriv->curPtrs.readLastByte += numWrite;
	pdevPriv->outPtr += numWrite;
	numWritten += numWrite;
    }


    /*
     * Inform kernel of new read buffer extents
     */
    status = Fs_IOControl(pdevPriv->streamID, IOC_PDEV_SET_PTRS,
	    sizeof(pdevPriv->curPtrs), (Address) &pdevPriv->curPtrs,
	    0, (Address) NULL);
    if (status != 0) {
	errno = Compat_MapCode(status);
	if (DEBUG_PDEV) {
	    DBG_PRINT ("WriteClient: SET_PTRS");
	}
	return -1;
    }
    return (numWritten);
}

/*-
 *-----------------------------------------------------------------------
 * PdevWrite --
 *	Write data to the client. Data are copied into the read buffer
 *	as much as possible. 
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
int
PdevWrite(pdev, buf, nbytes)
    int		pdev;
    char *buf;
    int	nbytes;
{
    struct iovec io;
    io.iov_base = buf;
    io.iov_len = nbytes;
    return PdevWritev(pdev, &io, 1);
}

int 
PdevIsMaster(pdev)
    int	pdev;
{
    return (pdev == PdevMaster);
}
int
PdevIsPdevConn(pdev) 
{
    return ((pdev >= 0) && (pdev <= MAX_FD) && privRecPtrs[pdev]);
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
    if (DEBUG_PDEV) {
	DBG_PRINT ("SetPtrs(%d): req %d:%d read %d:%d\n",
	       pdevPriv->streamID,
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
	    pdevPriv->inSize = 0;
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

	/*
	 * Need to make this -1 so PdevWriteClient works correctly
	 * (it adds the number of bytes written to readLastByte without
	 * checking its value)
	 */
	pdevPriv->curPtrs.readLastByte = -1;
    }
/*
 * This is needed until select() on a pdev returns when it is writable.
 */
    if ((pdevPriv->state & PDEV_WRITE_STALL) && 
	(pdevPriv->curPtrs.readLastByte < (OUT_BUF_SIZE - 1))) {
	PdevClearWriteBlockHack(pdevPriv->streamID);
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
    pdevPriv->inSize = 0;
    pdevPriv->curPtrs.requestFirstByte += pdevPriv->reqPtr->hdr.messageSize;
    pdevPriv->reqPtr =
	(Pdev_Request *)&pdevPriv->inBuf[pdevPriv->curPtrs.requestFirstByte];

    if (pdevPriv->curPtrs.requestFirstByte >
	pdevPriv->curPtrs.requestLastByte) {
	    pdevPriv->state |= PDEV_REQ_EMPTY;
    }

    if (DEBUG_PDEV) {
	DBG_PRINT ("RequestHandled(%d): req %d:%d read %d:%d\n",
	       pdevPriv->streamID, 
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
	if (DEBUG_PDEV) {
	    DBG_PRINT ("RequestHandled: SET_PTRS");
	}
    }
    if (pdevPriv->state & PDEV_REQ_EMPTY) {
	pdevPriv->curPtrs.requestFirstByte =
	    pdevPriv->curPtrs.requestLastByte = -1;
    }
}

