/* 
 * fsPdev.c --  
 *
 *	This file contains routines directly related to the request-response
 *	protocol between the client and server processes.  Routines that
 *	setup state, i.e. the SrvOpen, CltOpen, and migration routines,
 *	are in fsPdevSetup.c.  Routines for the Control stream are
 *	found in fsPdevControl.c
 *
 *	Operations are forwarded to a user-level server process using
 *	a "request-response" protocol.
 *	The server process declares a request buffer and optionally a read
 *	ahead buffer in its address space.  The kernel puts requests, which
 *	are generated when a client does an operation on the pseudo stream,
 *	into the request buffer directly.  The server learns of new requests
 *	by reading messages from the server stream that contain offsets within
 *	the request buffer.  Write requests may not require a response from
 *	the server.  Instead the kernel just puts the request into the buffer
 *	and returns to the client.  This allows many requests to be buffered
 *	before a context switch to the server process.  Similarly,
 *	the server can put read data into the read ahead buffer for a pseudo
 *	stream.  In this case a client's read will be satisfied from the
 *	buffer and the server won't be contacted.
 *
 * Copyright 1987, 1988 Regents of the University of California
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
#endif not lint

#include "sprite.h"
#include "fs.h"
#include "fsInt.h"
#include "fsOpTable.h"
#include "fsFile.h"
#include "fsStream.h"
#include "fsClient.h"
#include "fsMigrate.h"
#include "fsLock.h"
#include "fsDisk.h"
#include "proc.h"
#include "rpc.h"
#include "swapBuffer.h"

/*
 * Prevent tracing by defining CLEAN here before this next include
 */
#undef CLEAN
#include "fsPdev.h"

/*
 * Access to PdevServerIOHandle is monitored.
 */
#define LOCKPTR (&pdevHandlePtr->lock)

/*
 * Flags for a pseudo device handle.
 *	PDEV_SETUP		The server has set up state for the stream.
 *	PDEV_BUSY		Set during request-response to ensure that
 *				only one client process is using the stream.
 *	PDEV_REPLY_READY	The server gave us a reply
 *	PDEV_REPLY_FAILED	There was some problem (usually with copy in)
 *				getting the reply from the server.
 *	PDEV_SERVER_GONE	Set after the server closes its stream.
 *	PDEV_READ_BUF_EMPTY	When set there is no data in the read ahead
 *				buf.
 *	PDEV_SERVER_KNOWS_IT	Set after the server has done a read and been
 *				told that we too think the read ahead buffer
 *				is empty.  This synchronization simplifies
 *				things; there is no wrapping; there is no
 *				confusion about full vs. empty.
 *	PDEV_READ_PTRS_CHANGED	Set when we have used data from the read
 *				ahead buffer.  This makes the server stream
 *				selectable so it will find out about the
 *				new pointers.
 *	PDEV_WRITE_BEHIND	Write-behind is enabled for this stream.
 *	PDEV_NO_BIG_WRITES	Causes writes larger than will fit into
 *				the request buffer to fail.  This is to
 *				support UDP socket semantics, sigh.
 *	FS_USER			This flag is borrowed from the stream flags
 *				and it indicates the buffers are in user space
 */
#define PDEV_SETUP		0x0001
#define PDEV_BUSY		0x0002
#define PDEV_REPLY_READY	0x0004
#define PDEV_REPLY_FAILED	0x0008
#define PDEV_SERVER_GONE	0x0010
#define PDEV_READ_BUF_EMPTY	0x0020
#define PDEV_SERVER_KNOWS_IT	0x0040
#define PDEV_READ_PTRS_CHANGED	0x0080
#define PDEV_WRITE_BEHIND	0x0100
#define PDEV_NO_BIG_WRITES	0x0200
/*resrv FS_USER			0x8000 */

/*
 * Forward declarations.
 */

static	ReturnStatus		RequestResponse();
static	void			PdevClientWakeup();
static	void			PdevClientNotify();
void				FsRmtPseudoStreamClose();

/*
 *----------------------------------------------------------------------
 *
 * FsServerStreamCreate --
 *
 *	Set up the stream state for a server's private channel to a client.
 *	This creates a PdevServerIOHandle that has all the state for the
 *	connection to the client.
 *
 * Results:
 *	A pointer to the I/O handle created.  The handle is locked.
 *	NIL is returned if a handle under the fileID already existed.
 *
 * Side effects:
 *	The I/O handle for this connection between a client and the server
 *	is installed and initialized.
 *
 *----------------------------------------------------------------------
 */

PdevServerIOHandle *
FsServerStreamCreate(ioFileIDPtr, slaveClientID, name)
    FsFileID	*ioFileIDPtr;	/* File ID used for pseudo stream handle */
    int		slaveClientID;	/* Host ID of client process */
    char	*name;		/* File name for error messages */
{
    FsHandleHeader *hdrPtr;
    register PdevServerIOHandle *pdevHandlePtr;
    Boolean found;

    ioFileIDPtr->type = FS_SERVER_STREAM;
    found = FsHandleInstall(ioFileIDPtr, sizeof(PdevServerIOHandle), name,
			    &hdrPtr);
    pdevHandlePtr = (PdevServerIOHandle *)hdrPtr;
    if (found) {
	Sys_Panic(SYS_WARNING, "ServerStreamCreate, found handle <%x,%x,%x>\n",
		  hdrPtr->fileID.serverID, hdrPtr->fileID.major,
		  hdrPtr->fileID.minor);
	FsHandleRelease(pdevHandlePtr, TRUE);
	return((PdevServerIOHandle *)NIL);
    }

    DBG_PRINT( ("ServerStreamOpen <%d,%x,%x>\n",
	    ioFileIDPtr->serverID, ioFileIDPtr->major, ioFileIDPtr->minor) );

    /*
     * Initialize the state for the pseudo stream.  Remember that
     * the request and read ahead buffers for the pseudo-stream are set up
     * via IOControls by the server process later.
     */

    pdevHandlePtr->flags = 0;
    pdevHandlePtr->selectBits = 0;

    pdevHandlePtr->requestBuf.data = (Address)NIL;
    pdevHandlePtr->requestBuf.firstByte = -1;
    pdevHandlePtr->requestBuf.lastByte = -1;
    pdevHandlePtr->requestBuf.size = 0;

    pdevHandlePtr->readBuf.data = (Address)NIL;
    pdevHandlePtr->readBuf.firstByte = -1;
    pdevHandlePtr->readBuf.lastByte = -1;
    pdevHandlePtr->readBuf.size = 0;

    pdevHandlePtr->nextRequestBuffer = (Address)NIL;

    pdevHandlePtr->operation = PDEV_INVALID;
    pdevHandlePtr->replyBuf = (Address)NIL;
    pdevHandlePtr->serverPID = (Proc_PID)NIL;
    pdevHandlePtr->clientPID = (Proc_PID)NIL;
    pdevHandlePtr->clientWait.pid = NIL;
    pdevHandlePtr->clientWait.hostID = NIL;
    pdevHandlePtr->clientWait.waitToken = NIL;

    List_Init(&pdevHandlePtr->srvReadWaitList);
    List_Init(&pdevHandlePtr->cltReadWaitList);
    List_Init(&pdevHandlePtr->cltWriteWaitList);
    List_Init(&pdevHandlePtr->cltExceptWaitList);

    return(pdevHandlePtr);
}

/*
 *----------------------------------------------------------------------
 *
 * FsServerStreamClose --
 *
 *	Clean up the state associated with a server stream.  This makes
 *	sure the client processes associated with the pseudo stream get
 *	poked, and it marks the pseudo stream's state as invalid so
 *	the clients will abort their current operations, if any.  The
 *	handle is 'removed' here, but it won't go away until the client
 *	side closes down and releases its reference to it.
 *
 * Results:
 *	SUCCESS.
 *
 * Side effects:
 *	Marks the pseudo stream state with PDEV_SERVER_GONE, notifies
 *	all conditions in pseudo stream state, wakes up all processes
 *	in any of the pseudo stream's wait lists, and then removes
 *	the handle from the hash table.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
FsServerStreamClose(streamPtr, clientID, procID, flags, size, data)
    Fs_Stream		*streamPtr;	/* Service stream to close */
    int			clientID;	/* HostID of client closing */
    Proc_PID		procID;		/* ID of closing process */
    int			flags;		/* Flags from the stream being closed */
    int			size;		/* Should be zero */
    ClientData		data;		/* IGNORED */
{
    register PdevServerIOHandle *pdevHandlePtr =
	    (PdevServerIOHandle *)streamPtr->ioHandlePtr;

    DBG_PRINT( ("Server Closing pdev %x,%x\n", 
		pdevHandlePtr->hdr.fileID.major,
		pdevHandlePtr->hdr.fileID.minor) );

    PdevClientWakeup(pdevHandlePtr);
    FsHandleRelease(pdevHandlePtr, TRUE);
    FsHandleRemove(pdevHandlePtr);	/* No need for scavenging */
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * PdevClientWakeup --
 *
 *	Called when the server's stream is closed.  This
 *	notifies the various conditions that the client might be
 *	waiting on and marks the pdev state as invalid so the
 *	client will bail out when it wakes up.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Notifies condition variables and marks the pseudo stream as invalid.
 *
 *----------------------------------------------------------------------
 */

ENTRY static void
PdevClientWakeup(pdevHandlePtr)
    PdevServerIOHandle *pdevHandlePtr;	/* State for the pseudo stream */
{
    LOCK_MONITOR;
    pdevHandlePtr->flags |= (PDEV_SERVER_GONE|PDEV_REPLY_FAILED);
    Sync_Broadcast(&pdevHandlePtr->setup);
    Sync_Broadcast(&pdevHandlePtr->access);
    Sync_Broadcast(&pdevHandlePtr->caughtUp);
    Sync_Broadcast(&pdevHandlePtr->replyReady);
    FsFastWaitListNotify(&pdevHandlePtr->cltReadWaitList);
    FsFastWaitListNotify(&pdevHandlePtr->cltWriteWaitList);
    FsFastWaitListNotify(&pdevHandlePtr->cltExceptWaitList);
    UNLOCK_MONITOR;
}

/*
 *----------------------------------------------------------------------
 *
 * RequestResponse --
 *
 *	The general request-response protocol between a client's pseudo
 *	stream and the server stream.  This passes a request to the server
 *	that specifies the command and parameters, the size of the input data
 *	and the max size of the reply.  Then, if a reply is needed, this blocks
 *	the client until the server responds.
 *
 * Results:
 *	An error code from the server, or from errors on the vm copies.
 *
 * Side effects:
 *	The server process is locked while we copy the request into the
 *	buffer in the server's address space.  The firstByte and lastByte
 *	offsets into the pseudo stream request buffer are updated to
 *	reflect the new request.  The corresponding server stream is
 *	marked FS_READABLE.
 *
 *----------------------------------------------------------------------
 */

INTERNAL static ReturnStatus
RequestResponse(pdevHandlePtr, requestPtr, inputSize, inputBuf, replySize,
	replyBuf, replySizePtr, waitPtr)
    register PdevServerIOHandle *pdevHandlePtr;	/* Caller should lock this
						 * with the monitor lock. */
    register Pdev_Request *requestPtr;	/* The caller should fill in the
					 * command and parameter parts. The
					 * sizes are filled in here. */
    int			inputSize;	/* Size of input buffer. */
    Address		inputBuf;	/* Inputs of the remote command. */
    int			replySize;	/* Size of output buffer.  0 means
					 * no reply data expected.  -1
					 * means no reply wanted.  This causes
					 * a SUCCESS return immediately
					 * without having to switch out to
					 * the server process for the reply. */
    Address		replyBuf;	/* Results of the remote command. */
    int			*replySizePtr;	/* Amount of data actually in replyBuf.
					 * (May be NIL if not needed.) */
    Sync_RemoteWaiter	*waitPtr;	/* Client process info for waiting.
					 * Only needed for read & write. */
{
    register ReturnStatus  status;
    Proc_ControlBlock	   *serverProcPtr;   /* For VM copy operations */
    register int	   firstByte;	     /* Offset into request buffer */
    register int	   lastByte;	     /* Offset into request buffer */
    int			   room;	     /* Room available in req. buf.*/
    int			   savedLastByte;    /* For error recovery */
    int			   savedFirstByte;   /*   ditto */

    if (replySizePtr != (int *) NIL) {
	*replySizePtr = 0;
    }

    if ((pdevHandlePtr == (PdevServerIOHandle *)NIL) ||
	(pdevHandlePtr->flags & PDEV_SERVER_GONE)) {
	return(DEV_OFFLINE);
    }
    /*
     * See if we have to switch to a new request buffer.  This is needed
     * to support UDP, which wants to set a maximum write size.  The max
     * is implemented by letting the UDP server change the buffer size and
     * setting the property that writes larger than the buffer fail.  We
     * wait and switch after the request buffer empties.
     */
    firstByte = pdevHandlePtr->requestBuf.firstByte;
    lastByte = pdevHandlePtr->requestBuf.lastByte;
    if ((pdevHandlePtr->nextRequestBuffer != (Address)NIL) &&
	((firstByte > lastByte) || (firstByte == -1))) {
	Sys_Panic(SYS_WARNING, "Switching to request buffer at 0x%x\n",
			       pdevHandlePtr->nextRequestBuffer);
	pdevHandlePtr->requestBuf.data = pdevHandlePtr->nextRequestBuffer;
	pdevHandlePtr->requestBuf.size = pdevHandlePtr->nextRequestBufSize;
	pdevHandlePtr->nextRequestBuffer = (Address)NIL;
	firstByte = -1;
    }
    /*
     * Format the request header.  Note that the complete message size is
     * rounded up so subsequent messages start on word boundaries.
     */
    requestPtr->magic = PDEV_REQUEST_MAGIC;
    requestPtr->requestSize = inputSize;
    requestPtr->replySize = replySize;
    requestPtr->messageSize = sizeof(Pdev_Request) +
		((inputSize + sizeof(int) - 1) / sizeof(int)) * sizeof(int);
    if (pdevHandlePtr->requestBuf.size < requestPtr->messageSize) {
	Sys_Panic(SYS_WARNING, "RequestResponse request too large\n");
	return(GEN_INVALID_ARG);
    }

    PDEV_REQUEST_PRINT(&pdevHandlePtr->hdr.fileID, requestPtr);
    PDEV_REQUEST(&pdevHandlePtr->hdr.fileID, requestPtr);

    /*
     * Put the request into the request buffer.
     *
     * We assume that our caller will not give us a request that can't all
     * fit into the buffer.  However, if the buffer is not empty enough we
     * wait for the server to catch up with us.  (Things could be optimized
     * perhaps to wait for just enough room.  To be real clever you have
     * to be careful when you reset the firstByte so that the server only
     * notices after it has caught up with everything at the end of the
     * buffer.  To keep things simple we just wait for the server to catch up
     * completely if we can't fit this request in.)
     */

    savedFirstByte = firstByte;
    savedLastByte = lastByte;
    if (firstByte > lastByte || firstByte == -1) {
	/*
	 * Buffer has emptied.
	 */
	firstByte = 0;
	pdevHandlePtr->requestBuf.firstByte = firstByte;
	lastByte = requestPtr->messageSize - 1;
    } else {
	room = pdevHandlePtr->requestBuf.size - (lastByte + 1);
	if (room < requestPtr->messageSize) {
	    /*
	     * There is no room left at the end of the buffer.
	     * We wait and then put the request at the beginning.
	     */
	    while (pdevHandlePtr->requestBuf.firstByte <
		   pdevHandlePtr->requestBuf.lastByte) {
		DBG_PRINT( (" (catch up) ") );
		(void) Sync_Wait(&pdevHandlePtr->caughtUp, FALSE);
		if (pdevHandlePtr->flags & PDEV_SERVER_GONE) {
		    status = DEV_OFFLINE;
		    goto failure;
		}
	    }
	    savedFirstByte = -1;
	    firstByte = 0;
	    pdevHandlePtr->requestBuf.firstByte = firstByte;
	    lastByte = requestPtr->messageSize - 1;
	} else {
	    /*
	     * Append the message to the other requests.
	     */
	    firstByte = lastByte + 1;
	    lastByte += requestPtr->messageSize;
	}
    }
    pdevHandlePtr->operation = requestPtr->operation;
    pdevHandlePtr->requestBuf.lastByte = lastByte;
    DBG_PRINT( (" first %d last %d\n", firstByte, lastByte) );

    /*
     * Copy the request and data out into the server's request buffer.
     * We map to a proc table pointer for the server which has a side
     * effect of locking down the server process so it can't disappear.
     */

    serverProcPtr = Proc_LockPID(pdevHandlePtr->serverPID);
    if (serverProcPtr == (Proc_ControlBlock *)NIL) {
	status = DEV_OFFLINE;
	goto failure;
    }
    status = Vm_CopyOutProc(sizeof(Pdev_Request), (Address)requestPtr, TRUE,
			    serverProcPtr, (Address)
			        &pdevHandlePtr->requestBuf.data[firstByte]);
    if (status == SUCCESS) {
	firstByte += sizeof(Pdev_Request);
	if (inputSize > 0) {
	    status = Vm_CopyOutProc(inputSize, inputBuf, 
			(pdevHandlePtr->flags & FS_USER) == 0, serverProcPtr,
			(Address)&pdevHandlePtr->requestBuf.data[firstByte]);
	}
    }
    Proc_Unlock(serverProcPtr);
    if (status != SUCCESS) {
	/*
	 * Either the message header or data couldn't get copied out.
	 * Reset the buffer pointers so the bad request isn't seen.
	 */
	pdevHandlePtr->requestBuf.firstByte = savedFirstByte;
	pdevHandlePtr->requestBuf.lastByte = savedLastByte;
	goto failure;
    }

    /*
     * Poke the server so it can read the new pointer values.
     * This is done here even if write-behind is enabled, even though our
     * scheduler tends to wake up the server too soon.
     * Although it is possible to put a notify in about 3 other places
     * to catch cases where the client does a write-behind and then waits,
     * not all clients are clever enough to use select.  That solution results
     * in cases where a write can linger a long time in the request buffer.
     * (cat /dev/syslog in a tx window is a good test case.)
     */
    FsFastWaitListNotify(&pdevHandlePtr->srvReadWaitList);

    if (replySize >= 0) {  
	/*
	 * If this operation needs a reply we wait for it.  We save
	 * the client's reply buffer address and processID in the
	 * stream state so the kernel can copy the reply directly from
	 * the server's address space to the client's when the server
	 * makes the IOC_PDEV_REPLY IOControl.
	 */

	pdevHandlePtr->replyBuf = replyBuf;
	pdevHandlePtr->clientPID = (Proc_GetEffectiveProc())->processID;
	if (waitPtr != (Sync_RemoteWaiter *)NIL) {
	    pdevHandlePtr->clientWait = *waitPtr;
	}
	pdevHandlePtr->flags &= ~PDEV_REPLY_READY;
	while ((pdevHandlePtr->flags & PDEV_REPLY_READY) == 0) {
	    (void)Sync_Wait(&pdevHandlePtr->replyReady, FALSE);
	    if (pdevHandlePtr->flags & (PDEV_REPLY_FAILED|PDEV_SERVER_GONE)) {
		status = DEV_OFFLINE;
		goto failure;
	    }
	}
	if (replySizePtr != (int *) NIL) {
	    *replySizePtr = pdevHandlePtr->reply.replySize;
	}
    } else {
	pdevHandlePtr->reply.status = SUCCESS;
    }
failure:

    if (status == SUCCESS) {
	status = pdevHandlePtr->reply.status;
    }
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * FsServerStreamSelect --
 *
 *	Select a server's request/response stream.  This returns
 *	FS_READABLE in the outFlags if there is data in the
 *	server's request buffer.  The next read on the server stream
 *	will return the current pointers into the buffer.
 *
 * Results:
 *	SUCCESS
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
FsServerStreamSelect(hdrPtr, waitPtr, readPtr, writePtr, exceptPtr)
    FsHandleHeader	*hdrPtr;	/* Handle on device to select */
    Sync_RemoteWaiter	*waitPtr;	/* Process info for remote waiting */
    int 		*readPtr;	/* Bit to clear if non-readable */
    int 		*writePtr;	/* Bit to clear if non-writeable */
    int 		*exceptPtr;	/* Bit to clear if non-exceptable */
{
    register PdevServerIOHandle	*pdevHandlePtr = (PdevServerIOHandle *)hdrPtr;

    LOCK_MONITOR;
    if (*readPtr) {
	if (((pdevHandlePtr->flags & PDEV_READ_PTRS_CHANGED) == 0) &&
	     ((pdevHandlePtr->requestBuf.firstByte == -1) ||
	      (pdevHandlePtr->requestBuf.firstByte >=
		  pdevHandlePtr->requestBuf.lastByte))) {
	    *readPtr = 0;
	    FsFastWaitListInsert(&pdevHandlePtr->srvReadWaitList, waitPtr);
	}
    }
    *writePtr = 0;
    *exceptPtr = 0;
    UNLOCK_MONITOR;
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * FsServerStreamRead --
 *
 *	When the server reads on a server stream it is looking for a
 *	message containing pointers into the request buffer that's
 *	in its address spapce.  This routine returns those values.
 *
 * Results:
 *	SUCCESS unless all clients have gone away.
 *
 * Side effects:
 *	The buffer is filled a Pdev_BufPtrs structure.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
FsServerStreamRead(streamPtr, flags, buffer, offsetPtr, lenPtr, waitPtr)
    register Fs_Stream 	*streamPtr;	/* Stream to read from. */
    int		flags;		/* Flags from stream */
    Address 	buffer;		/* Where to read into. */
    int		*offsetPtr;	/* In/Out byte offset for the read */
    int 	*lenPtr;	/* In/Out byte count parameter */
    Sync_RemoteWaiter *waitPtr;	/* Process info for waiting */
{
    register PdevServerIOHandle *pdevHandlePtr =
	    (PdevServerIOHandle *)streamPtr->ioHandlePtr;
    register ReturnStatus status;
    Pdev_BufPtrs bufPtrs;
    register int reqFirstByte, reqLastByte;

    LOCK_MONITOR;
    /*
     * The server stream is readable only if there are requests in the
     * request buffer or if the read ahead buffers have changed since
     * the last time the server did a read.
     */
    reqFirstByte = pdevHandlePtr->requestBuf.firstByte;
    reqLastByte = pdevHandlePtr->requestBuf.lastByte;
    if (reqFirstByte > pdevHandlePtr->requestBuf.size ||
	reqLastByte > pdevHandlePtr->requestBuf.size) {
	Sys_Panic(SYS_FATAL, "PdevServerRead, pointers inconsistent\n");
	UNLOCK_MONITOR;
	return(GEN_INVALID_ARG);
    }
    if (((pdevHandlePtr->flags & PDEV_READ_PTRS_CHANGED) == 0) &&
	((reqFirstByte == -1) || (reqFirstByte > reqLastByte))) {
	status = FS_WOULD_BLOCK;
	*lenPtr = 0;
	FsFastWaitListInsert(&pdevHandlePtr->srvReadWaitList, waitPtr);
	PDEV_TRACE(&pdevHandlePtr->hdr.fileID, PDEVT_SRV_READ_WAIT);
    } else {
	/*
	 * Copy the current pointers out to the server.  We include the
	 * server's address of the request buffer to support changing
	 * the request buffer after requests have started to flow.
	 */
	PDEV_TRACE(&pdevHandlePtr->hdr.fileID, PDEVT_SRV_READ);
	bufPtrs.magic = PDEV_BUF_PTR_MAGIC;
	bufPtrs.requestAddr = pdevHandlePtr->requestBuf.data;
	if ((reqFirstByte == -1) || (reqFirstByte > reqLastByte)) {
	    /*
	     * Request buffer is empty.
	     */
	    bufPtrs.requestFirstByte = -1;
	    bufPtrs.requestLastByte = -1;
	} else {
	    bufPtrs.requestFirstByte = reqFirstByte;
	    bufPtrs.requestLastByte = reqLastByte;
	}

	/*
	 * The read ahead buffer is filled by the server until there is
	 * no room left.  Only after the kernel has emptied the
	 * read ahead buffer will the server start filling it again.
	 * We use the PDEV_SERVER_KNOWS_IT state bit to know when to
	 * expect new pointer values after the buffer empties.
	 */
	if (pdevHandlePtr->flags & PDEV_READ_BUF_EMPTY) {
	    bufPtrs.readLastByte = -1;
	    bufPtrs.readFirstByte = -1;
	    pdevHandlePtr->flags |= PDEV_SERVER_KNOWS_IT;
	} else {
	    bufPtrs.readFirstByte = pdevHandlePtr->readBuf.firstByte;
	    bufPtrs.readLastByte = pdevHandlePtr->readBuf.lastByte;
	}
	pdevHandlePtr->flags &= ~PDEV_READ_PTRS_CHANGED;
	status = Vm_CopyOut(sizeof(Pdev_BufPtrs), (Address)&bufPtrs, buffer);
	*lenPtr = sizeof(Pdev_BufPtrs);
	/*
	 * Poke the "caughtUp" condition in case anyone is waiting to stuff
	 * more requests into the buffer.  (THIS SEEMS INAPPROPRIATE)
	 */
	Sync_Broadcast(&pdevHandlePtr->caughtUp);
	DBG_PRINT( ("READ %x,%x req %d:%d read %d:%d\n",
		pdevHandlePtr->hdr.fileID.major,
		pdevHandlePtr->hdr.fileID.minor,
		pdevHandlePtr->requestBuf.firstByte,
		pdevHandlePtr->requestBuf.lastByte,
		pdevHandlePtr->readBuf.firstByte,
		pdevHandlePtr->readBuf.lastByte) );
    }
    UNLOCK_MONITOR;
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsServerStreamIOControl --
 *
 *	IOControls for the server's stream.  The server process uses this
 *	to manipulate the request and read ahead buffers in its address
 *	space.  It delcares them, and queries and sets pointers into them.
 *	The server also replies to requests here, and notifies the kernel
 *	when the pseudo-device is selectable.
 *
 * Results:
 *	SUCCESS if all went well, otherwise a status from a Vm operation
 *	or a consistency check.
 *
 * Side effects:
 *	This is the main entry point for the server process to control
 *	the pseudo-device connection to its clients.  The side effects
 *	depend on the I/O control.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ENTRY ReturnStatus
FsServerStreamIOControl(streamPtr, command, byteOrder, inBufPtr, outBufPtr)
    Fs_Stream	*streamPtr;	/* Stream to server handle. */
    int		command;	/* The control operation to be performed. */
    int		byteOrder;	/* Client's byte order, should be same */
    Fs_Buffer *inBufPtr;	/* Command inputs */
    Fs_Buffer *outBufPtr;	/* Buffer for return parameters */
{
    ReturnStatus	status = SUCCESS;
    register PdevServerIOHandle	*pdevHandlePtr =
	    (PdevServerIOHandle *)streamPtr->ioHandlePtr;

    LOCK_MONITOR;

    if (byteOrder != mach_ByteOrder) {
	Sys_Panic(SYS_FATAL, "FsServerStreamIOControl: wrong byte order\n");
    }
    switch (command) {
	case IOC_PDEV_SET_BUF: {
	    /*
	     * The server is declaring the buffer space used for requests
	     * and (optionally) for read ahead.
	     * To support UDP socket semantics, we let the server change
	     * the request buffer after things have already started up.
	     * (This is sort of a pain, as we have to let the current
	     * request buffer empty before switching.)
	     *
	     * Side effects:
	     *		Set the request, and optionally the read-ahead,
	     *		buffer pointers.  The setup condition is notified
	     *		to let the client's open transaction begin.
	     */
	    register Pdev_SetBufArgs *argPtr =
		    (Pdev_SetBufArgs *)inBufPtr->addr;
	    register Proc_ControlBlock *procPtr;
	    if (inBufPtr->size != sizeof(Pdev_SetBufArgs)) {
		status = GEN_INVALID_ARG;
	    } else if ((pdevHandlePtr->flags & PDEV_SETUP) == 0) {
		/*
		 * Normal case, first time initialization.
		 */
		pdevHandlePtr->requestBuf.data = argPtr->requestBufAddr;
		pdevHandlePtr->requestBuf.size = argPtr->requestBufSize;
		pdevHandlePtr->requestBuf.firstByte = -1;
		pdevHandlePtr->requestBuf.lastByte = -1;

		pdevHandlePtr->flags |= PDEV_SETUP|PDEV_READ_BUF_EMPTY|
						 PDEV_SERVER_KNOWS_IT;
		if (argPtr->readBufAddr == (Address)NIL ||
		    argPtr->readBufAddr == (Address)0) {
		    pdevHandlePtr->readBuf.data = (Address)NIL;
		} else {
		    pdevHandlePtr->readBuf.data = argPtr->readBufAddr;
		}
		pdevHandlePtr->readBuf.size = argPtr->readBufSize;
		pdevHandlePtr->readBuf.firstByte = -1;
		pdevHandlePtr->readBuf.lastByte = -1;

		procPtr = Proc_GetEffectiveProc();
		pdevHandlePtr->serverPID = procPtr->processID;

		Sync_Broadcast(&pdevHandlePtr->setup);
	    } else {
		/*
		 * The server is changing request buffers.  We just remember
		 * the new buffer address and size here, and switch over
		 * in RequestResponse after the current request buffer empties.
		 */
		pdevHandlePtr->nextRequestBuffer = argPtr->requestBufAddr;
		pdevHandlePtr->nextRequestBufSize = argPtr->requestBufSize;
		if (pdevHandlePtr->nextRequestBuffer == (Address)0) {
		    pdevHandlePtr->nextRequestBuffer = (Address)NIL;
		}
	    }
	    break;
	}
	case IOC_PDEV_WRITE_BEHIND: {
	    /*
	     * Side effects:
	     *		Set/unset write-behind buffering in the request buffer.
	     */
	    register Boolean writeBehind;
	    if (inBufPtr->size < sizeof(Boolean)) {
		status = GEN_INVALID_ARG;
	    } else {
		writeBehind = *(Boolean *)inBufPtr->addr;
		if (writeBehind) {
		    pdevHandlePtr->flags |= PDEV_WRITE_BEHIND;
		} else {
		    pdevHandlePtr->flags &= ~PDEV_WRITE_BEHIND;
		}
	    }
	    break;
	}
	case IOC_PDEV_BIG_WRITES: {
	    /*
	     * Side effects:
	     *		Set/unset the client's ability to make large writes.
	     */
	    register Boolean allowLargeWrites;
	    if (inBufPtr->size < sizeof(Boolean)) {
		status = GEN_INVALID_ARG;
	    } else {
		allowLargeWrites = *(Boolean *)inBufPtr->addr;
		if (allowLargeWrites) {
		    pdevHandlePtr->flags &= ~PDEV_NO_BIG_WRITES;
		} else {
		    pdevHandlePtr->flags |= PDEV_NO_BIG_WRITES;
		}
	    }
	    break;
	}
	case IOC_PDEV_SET_PTRS: {
	    /*
	     * The server is telling us about new pointer values.  We only
	     * pay attention to the requestFirstByte and readLastByte as
	     * it is our job to modify the other pointers.
	     *
	     * Side effects:
	     *		Set requestBuf.firstByte and readBuf.lastByte if
	     *		the server gives us new values (not equal -1).
	     *		We notify waiting clients if the server has
	     *		added read-ahead data.
	     */
	    register Pdev_BufPtrs *argPtr = (Pdev_BufPtrs *)inBufPtr->addr;
	    if (inBufPtr->size != sizeof(Pdev_BufPtrs)) {
		status = GEN_INVALID_ARG;
	    } else {
		/*
		 * Verify the request buffer pointer.  The server may just
		 * be telling us about read ahead data, in which case we
		 * shouldn't muck with the request pointers. Otherwise we
		 * update the request first byte to reflect the processing
		 * of some requests by the server.
		 */
		DBG_PRINT( ("SET  %x,%x req %d:%d read %d:%d\n",
		    pdevHandlePtr->hdr.fileID.major,
		    pdevHandlePtr->hdr.fileID.minor,
		    argPtr->requestFirstByte, argPtr->requestLastByte,
		    argPtr->readFirstByte, argPtr->readLastByte) );
		if (argPtr->requestFirstByte <=
		            pdevHandlePtr->requestBuf.size &&
		    argPtr->requestFirstByte >= 0) {
		    pdevHandlePtr->requestBuf.firstByte =
			   argPtr->requestFirstByte;
		}
		Sync_Broadcast(&pdevHandlePtr->caughtUp);
	        if ((pdevHandlePtr->readBuf.data == (Address)NIL) ||
		    (argPtr->readLastByte < 0)) {
		    /*
		     * No read ahead info.
		     */
		    break;
		}
		if (argPtr->readLastByte > pdevHandlePtr->readBuf.size) {
		    Sys_Panic(SYS_WARNING,
			"FsServerStreamIOControl: set bad readPtr\n");
		    status = GEN_INVALID_ARG;
		    break;
		}
		if ((pdevHandlePtr->flags & PDEV_READ_BUF_EMPTY) == 0) {
		    /*
		     * Non-empty buffer.  Break out if bad pointer, else
		     * fall through to code that updates the pointer.
		     */
		    if (argPtr->readLastByte <=
			pdevHandlePtr->readBuf.lastByte) {
			    break;	/* No new read ahead data */
		    }
		} else if (pdevHandlePtr->flags & PDEV_SERVER_KNOWS_IT) {
		    /*
		     * Empty buffer and the server already knows this.
		     * We can safely reset firstByte to the beginning.
		     * The server should rely on this behavior.
		     */
		    if (argPtr->readLastByte >= 0) {
			pdevHandlePtr->flags &= ~(PDEV_READ_BUF_EMPTY|
						 PDEV_SERVER_KNOWS_IT);
			pdevHandlePtr->readBuf.firstByte = 0;
		    } else {
			break;	/* No new read ahead data */
		    }
		} else {
		    /*
		     * We emptied the buffer, but the server added data
		     * before seeing it was empty.  Can't reset firstByte.
		     */
		    if (argPtr->readLastByte > 
			    pdevHandlePtr->readBuf.lastByte) {
			pdevHandlePtr->flags &= ~PDEV_READ_BUF_EMPTY;
		    } else {
			break;	/* No new read ahead data */
		    }
		}
		/*
		 * We know here that the lastByte pointer indicates
		 * more data.  Otherwise we've broken out.
		 * Update select state and poke waiting readers.
		 */
		pdevHandlePtr->readBuf.lastByte = argPtr->readLastByte;
		pdevHandlePtr->selectBits |= FS_READABLE;
		FsFastWaitListNotify(&pdevHandlePtr->cltReadWaitList);
	    }
	    break;
	}
	case IOC_PDEV_REPLY: {
	    /*
	     * The server is replying to a request.
	     *
	     * Side effects:
	     *		Copy the reply from the server to the client.
	     *		Put the client on wait lists, if appropriate.
	     *		Notify the replyReady condition, and lastly
	     *		notify waiting clients about new select state.
	     */
	    register Pdev_Reply *srvReplyPtr = (Pdev_Reply *)inBufPtr->addr;

	    pdevHandlePtr->reply = *srvReplyPtr;
	    if (srvReplyPtr->replySize > 0) {
		register Proc_ControlBlock *clientProcPtr;

		/*
		 * Copy the reply into the waiting buffers.  PDEV_WRITE is
		 * handled specially because the reply buffer is just an
		 * integer variable in the kernel, while the input buffer
		 * is in user space, which is indicated by the FS_USER flag.
		 *  - To be fully general we'd need a user space flag for
		 * 	both the input buffer and the reply.
		 */
		if (((pdevHandlePtr->flags & FS_USER) == 0) ||
		    (pdevHandlePtr->operation == PDEV_WRITE)) {
		    status = Vm_CopyIn(srvReplyPtr->replySize,
				       srvReplyPtr->replyBuf,
				       pdevHandlePtr->replyBuf);
	        } else {
		    clientProcPtr = Proc_LockPID(pdevHandlePtr->clientPID);
		    if (clientProcPtr == (Proc_ControlBlock *)NIL) {
			status = FS_BROKEN_PIPE;
		    } else {
			status = Vm_CopyOutProc(srvReplyPtr->replySize,
				srvReplyPtr->replyBuf, FALSE,
				clientProcPtr, pdevHandlePtr->replyBuf);
			Proc_Unlock(clientProcPtr);
		    }
		}
		if (status != SUCCESS) {
		    pdevHandlePtr->flags |= PDEV_REPLY_FAILED;
		}
	    }
	    PDEV_REPLY(&pdevHandlePtr->hdr.fileID, srvReplyPtr);
	    if (srvReplyPtr->status == FS_WOULD_BLOCK) {
		if (pdevHandlePtr->operation == PDEV_READ) {
		    FsFastWaitListInsert(&pdevHandlePtr->cltReadWaitList,
					 &pdevHandlePtr->clientWait);
		} else if (pdevHandlePtr->operation == PDEV_WRITE) {
		    FsFastWaitListInsert(&pdevHandlePtr->cltWriteWaitList,
					 &pdevHandlePtr->clientWait);
		}
	    }
	    /*
	     * Wakeup the client waiting for this reply.
	     */
	    pdevHandlePtr->flags |= PDEV_REPLY_READY;
	    Sync_Broadcast(&pdevHandlePtr->replyReady);
	    /*
	     * Notify other clients that may also be blocked on the stream.
	     */
	    pdevHandlePtr->selectBits = srvReplyPtr->selectBits;
	    PdevClientNotify(pdevHandlePtr);
	    break;
	}
	case IOC_PDEV_READY:
	    /*
	     * Master has made the device ready.  The inBuffer contains
	     * new select bits.
	     *
	     * Side effects:
	     *		Notify waiting clients.
	     */

	    if (inBufPtr->size != sizeof(int)) {
		status = FS_INVALID_ARG;
	    } else {
		/*
		 * Update the select state of the pseudo-device and
		 * wake up any clients waiting on their pseudo-stream.
		 */
		pdevHandlePtr->selectBits = *(int *)inBufPtr->addr;
		PdevClientNotify(pdevHandlePtr);
	    }
	    break;
	case IOC_REPOSITION:
	    status = GEN_INVALID_ARG;
	    break;
	case IOC_GET_FLAGS:
	case IOC_SET_FLAGS:
	case IOC_SET_BITS:
	case IOC_CLEAR_BITS:
	    /*
	     * There are no server stream specific flags.
	     */
	    break;
	case IOC_TRUNCATE:
	case IOC_LOCK:
	case IOC_UNLOCK:
	case IOC_MAP:
	    status = FS_INVALID_ARG;
	    break;
	case IOC_GET_OWNER:
	case IOC_SET_OWNER:
	    status = GEN_NOT_IMPLEMENTED;
	    break;
	case IOC_NUM_READABLE: {
	    /*
	     * The server stream is readable only if there are requests in the
	     * request buffer or if the read ahead buffers have changed since
	     * the last time the server did a read.
	     *
	     * Side effects:
	     *		None.
	     */
	    register int reqFirstByte, reqLastByte;
	    register int numReadable;

	    reqFirstByte = pdevHandlePtr->requestBuf.firstByte;
	    reqLastByte = pdevHandlePtr->requestBuf.lastByte;
	    if (((pdevHandlePtr->flags & PDEV_READ_PTRS_CHANGED) == 0) &&
		((reqFirstByte == -1) || (reqFirstByte > reqLastByte))) {
		numReadable = 0;
	    } else {
		numReadable = sizeof(Pdev_BufPtrs);
	    }
	    if (outBufPtr->addr == (Address)NIL ||
		outBufPtr->size < sizeof(int)) {
		status = GEN_INVALID_ARG;
	    } else {
		*(int *)outBufPtr->addr = numReadable;
		status = SUCCESS;
	    }
	    break;
	}
	default:
	    status = GEN_NOT_IMPLEMENTED;
	    break;
    }
    if (status != SUCCESS) {
	Sys_Printf("PdevServer IOControl #%x returning %x\n", command, status);
    }
    UNLOCK_MONITOR;
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsPseudoStreamOpen --
 *
 *	Do the first request-response with the server to see if it
 *	will accept the open by the client.   This is used to tell
 *	the server something about the new stream it is getting,
 *	and to let it decide if it will accept the open.
 *
 * Results:
 *	The return status from the server's reply.  It can reject the open.
 *
 * Side effects:
 *	An PDEV_OPEN request-response is carried out.
 *
 *----------------------------------------------------------------------
 */

ENTRY ReturnStatus
FsPseudoStreamOpen(pdevHandlePtr, flags, clientID, procID, userID)
    register PdevServerIOHandle *pdevHandlePtr;/* Client's pseudo stream. */
    int		flags;		/* Open flags */
    int		clientID;	/* Host ID of the client */
    Proc_PID	procID;		/* Process ID of the client process */
    int		userID;		/* User ID of the client process */
{
    register ReturnStatus 	status;
    Pdev_Request		request;

    LOCK_MONITOR;

    /*
     * We have to wait for the server to establish buffer space for
     * the new stream before we can try to use it.
     */
    pdevHandlePtr->flags |= PDEV_BUSY;
    while ((pdevHandlePtr->flags & PDEV_SETUP) == 0) {
	if (pdevHandlePtr->flags & PDEV_SERVER_GONE) {
	    /*
	     * Server bailed out before we made contact.
	     */
	    status = DEV_OFFLINE;
	    goto exit;
	}
	(void) Sync_Wait(&pdevHandlePtr->setup, FALSE);
    }

    /*
     * Issue the first request to the server to see if it will accept us.
     */

    request.operation		= PDEV_OPEN;
    request.param.open.flags	= flags;
    request.param.open.pid	= procID;
    request.param.open.hostID	= clientID;
    request.param.open.uid	= userID;

    pdevHandlePtr->flags &= ~FS_USER;
    status = RequestResponse(pdevHandlePtr, &request, 0, (Address) NIL, 0,
			 (Address) NIL, (int *)NIL, (Sync_RemoteWaiter *)NIL);
exit:
    pdevHandlePtr->flags &= ~PDEV_BUSY;
    Sync_Broadcast(&pdevHandlePtr->access);
    UNLOCK_MONITOR;
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsPseudoStreamRead --
 *
 *	Read from a pseudo-device stream. If there is data in the read
 *	ahead buffer (if one exists), that is used to satisfy the read.
 *	Otherwise a request-response exchange with the server is used
 *	to do the read.
 *
 * Results:
 *	SUCCESS, and *lenPtr reflects how much was read.  When the server
 *	goes away EOF is simulated by a SUCCESS return and *lenPtr == 0.
 *	If there is no data in the read ahead buffer FS_WOULD_BLOCK is returned.
 *
 * Side effects:
 *	If applicable, pointers into the read ahead buffer are adjusted.
 *	The buffer is filled with the number of bytes indicated by
 *	the length parameter.  The in/out length parameter specifies
 *	the buffer size on input and is updated to reflect the number
 *	of bytes actually read.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
FsPseudoStreamRead(streamPtr, flags, buffer, offsetPtr, lenPtr, waitPtr)
    register Fs_Stream 	*streamPtr;	/* Stream to read from. */
    Address 	buffer;			/* Where to read into. */
    int		*offsetPtr;		/* In/Out byte offset for the read */
    int 	*lenPtr;		/* In/Out byte count parameter */
    Sync_RemoteWaiter *waitPtr;		/* Process info for waiting */
{
    ReturnStatus 	status;
    register PdevClientIOHandle *cltHandlePtr =
	    (PdevClientIOHandle *)streamPtr->ioHandlePtr;
    register PdevServerIOHandle *pdevHandlePtr = cltHandlePtr->pdevHandlePtr;
    Pdev_Request	request;

    LOCK_MONITOR;
    /*
     * Wait for exclusive access to the stream.  Different clients might
     * be using the shared pseudo stream at about the same time.  Things
     * are kept simple by only letting one process through at a time.
     */
    while (pdevHandlePtr->flags & PDEV_BUSY) {
	(void)Sync_Wait(&pdevHandlePtr->access, FALSE);
	if (pdevHandlePtr->flags & PDEV_SERVER_GONE) {
	    status = DEV_OFFLINE;
	    goto exit;
	}
    }
    pdevHandlePtr->flags |= PDEV_BUSY;

    if (pdevHandlePtr->readBuf.data != (Address)NIL) {
	/*
	 * A read ahead buffer exists so we get data from it.  If it's
	 * empty we put the client on the client I/O handle read wait list.
	 */
	if (pdevHandlePtr->flags & PDEV_READ_BUF_EMPTY) {
	    status = FS_WOULD_BLOCK;
	    FsFastWaitListInsert(&pdevHandlePtr->cltReadWaitList, waitPtr);
	    PDEV_TRACE(&pdevHandlePtr->hdr.fileID, PDEVT_READ_WAIT);
	    DBG_PRINT( ("PDEV %x,%x Read (%d) Blocked\n", 
		    streamPtr->ioHandlePtr->fileID.major,
		    streamPtr->ioHandlePtr->fileID.minor,
		    *lenPtr) );
	    *lenPtr = 0;
	} else {
	    register int dataAvail, firstByte, lastByte, toRead;
	    register Proc_ControlBlock *serverProcPtr;

	    firstByte = pdevHandlePtr->readBuf.firstByte;
	    lastByte = pdevHandlePtr->readBuf.lastByte;
	    dataAvail = lastByte - firstByte + 1;
	    if (dataAvail <= 0) {
		Sys_Panic(SYS_FATAL, 
		    "FsPseudoStreamRead, dataAvail in read buf <= 0 bytes\n");
		status = DEV_OFFLINE;
		goto exit;
	    }
	    /*
	     * Lock down the server process in preparation for copying
	     * from the read ahead buffer.
	     */
	    serverProcPtr = Proc_LockPID(pdevHandlePtr->serverPID);
	    if (serverProcPtr == (Proc_ControlBlock *)NIL) {
		status = DEV_OFFLINE;
		goto exit;
	    }
	    /*
	     * Decide how much to read and note if we empty the buffer.
	     */
	    if (dataAvail > *lenPtr) {
		toRead = *lenPtr;
	    } else {
		toRead = dataAvail;
		pdevHandlePtr->flags |= PDEV_READ_BUF_EMPTY;
	    }
	    DBG_PRINT( ("PDEV %x,%x Read %d Avail %d\n", 
		    pdevHandlePtr->hdr.fileID.major,
		    pdevHandlePtr->hdr.fileID.minor,
		    toRead, dataAvail) );
	    /*
	     * Copy out of the read ahead buffer to the client's buffer.
	     */
	    status = Vm_CopyInProc(toRead, serverProcPtr,
			  pdevHandlePtr->readBuf.data + firstByte,
			  buffer, (flags & FS_USER) == 0);
	    Proc_Unlock(serverProcPtr);
	    /*
	     * Update pointers and poke the server so it can find out.
	     */
	    *lenPtr = toRead;
	    firstByte += toRead;
	    pdevHandlePtr->readBuf.firstByte = firstByte;
	    pdevHandlePtr->flags |= PDEV_READ_PTRS_CHANGED;
	    FsFastWaitListNotify(&pdevHandlePtr->srvReadWaitList);
	}
    } else {
	Proc_ControlBlock	*procPtr;

	/*
	 * No read ahead buffer. Set up and do the request-response exchange.
	 */
	procPtr = Proc_GetEffectiveProc();
	request.operation		= PDEV_READ;
	request.param.read.offset	= *offsetPtr;
	request.param.read.familyID	= procPtr->familyID;
	request.param.read.procID	= procPtr->processID;

	pdevHandlePtr->flags |= (flags & FS_USER);
	status = RequestResponse(pdevHandlePtr, &request, 0, (Address) NIL,
				 *lenPtr, buffer, lenPtr, waitPtr);
    }
exit:
    if (status == DEV_OFFLINE) {
	/*
	 * Simulate EOF
	 */
	status = SUCCESS;
	*lenPtr = 0;
    }
    pdevHandlePtr->flags &= ~(PDEV_BUSY|FS_USER);
    Sync_Broadcast(&pdevHandlePtr->access);
    UNLOCK_MONITOR;
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsPseudoStreamWrite --
 *
 *	Write to a pseudo-device by a client.  The write is done
 *	asynchronously if the stream allows that.  In that case we
 *	always say we could write as much as requested.  However, if
 *	the write is larger than the server's request buffer we
 *	break it into blocks that each fit. To support UDP, the
 *	stream can also be marked to dis-allow these large writes.
 *	Finally, the stream may be marked synchronous in which case
 *	we tell RequestResponse to wait for a reply.
 *
 * Results:
 *	SUCCESS			- the data was written.
 *
 * Side effects:
 *	The data in the buffer is written to the device.  Large writes
 *	are broken into a series of shorter writes, although we keep
 *	the access lock on the pseudo-stream so the whole write completes.
 *	The in/out length parameter specifies the amount of data to write
 *	and is updated to reflect the number of bytes actually written.
 *
 *----------------------------------------------------------------------
 */
ENTRY ReturnStatus
FsPseudoStreamWrite(streamPtr, flags, buffer, offsetPtr, lenPtr, waitPtr)
    Fs_Stream 	*streamPtr;	/* Stream to write to. */
    int		flags;		/* Flags from the stream */
    Address 	buffer;		/* Where to write to. */
    int		*offsetPtr;	/* In/Out byte offset */
    int 	*lenPtr;	/* In/Out byte count */
    Sync_RemoteWaiter *waitPtr;	/* Process info for waiting on I/O */
{
    register Proc_ControlBlock *procPtr;
    register PdevClientIOHandle *cltHandlePtr =
	    (PdevClientIOHandle *)streamPtr->ioHandlePtr;
    register PdevServerIOHandle *pdevHandlePtr = cltHandlePtr->pdevHandlePtr;
    ReturnStatus 	status = SUCCESS;
    Pdev_Request	request;
    register int	toWrite;
    int			amountWritten;
    register int	length;
    int			replySize;
    int			numBytes;
    int			maxRequestSize;

    LOCK_MONITOR;
    /*
     * Wait for exclusive access to the stream.
     */
    while (pdevHandlePtr->flags & PDEV_BUSY) {
	(void)Sync_Wait(&pdevHandlePtr->access, FALSE);
	if (pdevHandlePtr->flags & PDEV_SERVER_GONE) {
	    UNLOCK_MONITOR;
	    return(FS_BROKEN_PIPE);
	}
    }
    pdevHandlePtr->flags |= (PDEV_BUSY|(flags & FS_USER));

    /*
     * The write request parameters are the offset and flags parameters.
     * The buffer contains the data to write.
     */
    procPtr = Proc_GetEffectiveProc();
    request.operation			= PDEV_WRITE;
    request.param.write.offset		= *offsetPtr;
    request.param.write.familyID	= procPtr->familyID;
    request.param.write.procID		= procPtr->processID;

    toWrite = *lenPtr;
    amountWritten = 0;
    maxRequestSize = pdevHandlePtr->requestBuf.size - sizeof(Pdev_Request);
    if (toWrite > maxRequestSize &&
	(pdevHandlePtr->flags & PDEV_NO_BIG_WRITES)) {
	Sys_Panic(SYS_WARNING,
	    "Too large a write (%d bytes) attempted on pseudo-device (UDP?)\n",
	    toWrite);
	status = GEN_INVALID_ARG;
	goto exit;
    }
    while ((toWrite > 0) && (status == SUCCESS)) {
	if (toWrite > maxRequestSize) {
	    length = maxRequestSize;
	} else {
	    length = toWrite;
	}
	/*
	 * Do a synchronous or asynchronous write.
	 */
	replySize = (pdevHandlePtr->flags&PDEV_WRITE_BEHIND) ? -1 :
		sizeof(int);
	status = RequestResponse(pdevHandlePtr, &request, length, buffer,
				 replySize, (Address)&numBytes, &replySize,
				 waitPtr);
	if (replySize == sizeof(int)) {
	    /*
	     * Pay attention to the number of bytes the server accepted.
	     */
	    length = numBytes;
	} else if ((pdevHandlePtr->flags & PDEV_WRITE_BEHIND) == 0) {
	    Sys_Panic(SYS_WARNING, "Pdev_Write, no return amtWritten\n");
	}
	toWrite -= length;
	request.param.write.offset += length;
	amountWritten += length;
	buffer += length;
    }
    *lenPtr = amountWritten;
exit:
    if (status == DEV_OFFLINE) {
	/*
	 * Simulate a broken pipe so writers die.
	 */
	status = FS_BROKEN_PIPE;
    }
    pdevHandlePtr->flags &= ~(PDEV_BUSY|FS_USER);
    Sync_Broadcast(&pdevHandlePtr->access);
    UNLOCK_MONITOR;
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsPseudoStreamIOControl --
 *
 *	IOControls for pseudo-device.  The in parameter block of the
 *	IOControl is passed to the server, and its response is used
 *	to fill the return parameter block of the client.  Remember that
 *	Fs_IOControlStub copies the user buffers into and out of the kernel
 *	so we'll set the FS_USER flag in the serverIOHandle.
 *
 * Results:
 *	SUCCESS			- the operation was successful.
 *
 * Side effects:
 *	None here in the kernel, anyway.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
FsPseudoStreamIOControl(streamPtr, command, byteOrder, inBufPtr, outBufPtr)
    Fs_Stream	*streamPtr;	/* Stream to pseudo-device */
    int		command;	/* The control operation to be performed. */
    int		byteOrder;	/* Client's byte order */
    Fs_Buffer   *inBufPtr;	/* Command inputs */
    Fs_Buffer	*outBufPtr;	/* Buffer for return parameters */
{
    ReturnStatus 	status;
    Pdev_Request	request;
    register Proc_ControlBlock *procPtr;
    register PdevClientIOHandle *cltHandlePtr =
	    (PdevClientIOHandle *)streamPtr->ioHandlePtr;
    register PdevServerIOHandle *pdevHandlePtr = cltHandlePtr->pdevHandlePtr;

    LOCK_MONITOR;
    /*
     * Wait for exclusive access to the stream.
     */
    while (pdevHandlePtr->flags & PDEV_BUSY) {
	(void)Sync_Wait(&pdevHandlePtr->access, FALSE);
	if (pdevHandlePtr->flags & PDEV_SERVER_GONE) {
	    status = DEV_OFFLINE;
	    goto exit;
	}
    }
    pdevHandlePtr->flags |= PDEV_BUSY;

    /*
     * Decide if the buffers are already in the kernel or not.  The buffers
     * for generic I/O controls are copied in, and if we are being called
     * from an RPC stub they are also in kernel space.
     */
    if ((command > IOC_GENERIC_LIMIT) &&
	(inBufPtr->flags & FS_USER) != 0) {
	pdevHandlePtr->flags |= FS_USER;

    }

    switch (command) {
	/*
	 * Trap out the IOC_NUM_READABLE here if there's a read ahead buf.
	 */
	case IOC_NUM_READABLE: {
	    if (pdevHandlePtr->readBuf.data != (Address)NIL) {
		int bytesAvail;
		if (pdevHandlePtr->flags & PDEV_READ_BUF_EMPTY) {
		    bytesAvail = 0;
		} else {
		    bytesAvail = pdevHandlePtr->readBuf.lastByte -
				 pdevHandlePtr->readBuf.firstByte + 1;
		}
		status = SUCCESS;
		if (byteOrder != mach_ByteOrder) {
		    int size = sizeof(int);
		    Swap_Buffer((Address)&bytesAvail, sizeof(int),
			mach_ByteOrder, byteOrder, "w", outBufPtr->addr, &size);
		    if (size != sizeof(int)) {
			status = GEN_INVALID_ARG;
		    }
		} else if (outBufPtr->size != sizeof(int)) {
		    status = GEN_INVALID_ARG;
		} else {
		    *(int *)outBufPtr->addr = bytesAvail;
		}
		DBG_PRINT( ("IOC  %x,%x num readable %d\n",
			pdevHandlePtr->hdr.fileID.major,
			pdevHandlePtr->hdr.fileID.minor,
			bytesAvail) );
		goto exit;
	    }
	    break;
	}
    }

    procPtr = Proc_GetEffectiveProc();
    request.operation			= PDEV_IOCTL;
    request.param.ioctl.command		= command;
    request.param.ioctl.familyID	= procPtr->familyID;
    request.param.ioctl.procID		= procPtr->processID;
    request.param.ioctl.byteOrder	= byteOrder;

    status = RequestResponse(pdevHandlePtr, &request, inBufPtr->size,
			     inBufPtr->addr, outBufPtr->size, outBufPtr->addr,
			     (int *) NIL, (Sync_RemoteWaiter *)NIL);
exit:
    pdevHandlePtr->flags &= ~(PDEV_BUSY|FS_USER);
    Sync_Broadcast(&pdevHandlePtr->access);
    UNLOCK_MONITOR;
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * FsPseudoStreamSelect --
 *
 *	Select on a pseudo-device.  This is done by checking the stream
 *	state kept on the host of the server.  If the pseudo device isn't
 *	selectable we just return and let the server's IOC_PDEV_READY
 *	IOControl do the wakeup for us. (ie. we don't use the handle wait
 *	lists.)
 *
 * Results:
 *	SUCCESS	or FS_WOULD_BLOCK
 *
 * Side effects:
 *	*outFlagsPtr modified to indicate whether the device is
 *	readable or writable.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
FsPseudoStreamSelect(hdrPtr, waitPtr, readPtr, writePtr, exceptPtr)
    FsHandleHeader	*hdrPtr;	/* Handle on pdev to select */
    Sync_RemoteWaiter	*waitPtr;	/* Process info for remote waiting */
    int 		*readPtr;	/* Bit to clear if non-readable */
    int 		*writePtr;	/* Bit to clear if non-writeable */
    int 		*exceptPtr;	/* Bit to clear if non-exceptable */
{
    ReturnStatus status;
    register PdevClientIOHandle *cltHandlePtr = (PdevClientIOHandle *)hdrPtr;
    register PdevServerIOHandle *pdevHandlePtr = cltHandlePtr->pdevHandlePtr;

    LOCK_MONITOR;

    PDEV_TSELECT(&cltHandlePtr->hdr.fileID, *readPtr, *writePtr, *exceptPtr);

    if ((pdevHandlePtr->flags & PDEV_SERVER_GONE) ||
	(pdevHandlePtr->flags & PDEV_SETUP) == 0) {
	status = DEV_OFFLINE;
    } else {
	if (*readPtr) {
	    if (((pdevHandlePtr->readBuf.data == (Address)NIL) &&
		 ((pdevHandlePtr->selectBits & FS_READABLE) == 0)) ||
		((pdevHandlePtr->readBuf.data != (Address)NIL) &&
		 (pdevHandlePtr->flags & PDEV_READ_BUF_EMPTY))) {
		*readPtr = 0;
		FsFastWaitListInsert(&pdevHandlePtr->cltReadWaitList, waitPtr);
	    }
	}
	if (*writePtr && ((pdevHandlePtr->selectBits & FS_WRITABLE) == 0)) {
	    *writePtr = 0;
	    FsFastWaitListInsert(&pdevHandlePtr->cltWriteWaitList, waitPtr);
	}    
	if (*exceptPtr && ((pdevHandlePtr->selectBits & FS_EXCEPTION) == 0)) {
            *exceptPtr = 0;
	    FsFastWaitListInsert(&pdevHandlePtr->cltExceptWaitList, waitPtr);
	}
	status = SUCCESS;
    }
    UNLOCK_MONITOR;
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * PdevClientNotify --
 *
 *	Wakeup any processes selecting or blocking on the pseudo-stream.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

INTERNAL static void
PdevClientNotify(pdevHandlePtr)
    PdevServerIOHandle *pdevHandlePtr;
{
    register int selectBits = pdevHandlePtr->selectBits;

    PDEV_WAKEUP(&pdevHandlePtr->hdr.fileID, pdevHandlePtr->clientPID,
		pdevHandlePtr->selectBits);
    if (selectBits & FS_READABLE) {
	FsFastWaitListNotify(&pdevHandlePtr->cltReadWaitList);
    }
    if (selectBits & FS_WRITABLE) {
	FsFastWaitListNotify(&pdevHandlePtr->cltWriteWaitList);
    }
    if (selectBits & FS_EXCEPTION) {
	FsFastWaitListNotify(&pdevHandlePtr->cltExceptWaitList);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * FsPseudoStreamCloseInt --
 *
 *	Do a close request-response with the server.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ENTRY void
FsPseudoStreamCloseInt(pdevHandlePtr)
    register PdevServerIOHandle *pdevHandlePtr;
{
    Pdev_Request	request;

    LOCK_MONITOR;

    if ((pdevHandlePtr->flags & PDEV_SETUP) == 0) {
	/*
	 * Server never set the stream up so we obviously can't contact it.
	 */
	goto exit;
    }
    /*
     * Wait for exclusive access to the stream.
     */
    while (pdevHandlePtr->flags & PDEV_BUSY) {
	(void)Sync_Wait(&pdevHandlePtr->access, FALSE);
	if (pdevHandlePtr->flags & PDEV_SERVER_GONE) {
	    goto exit;
	}
    }
    pdevHandlePtr->flags |= PDEV_BUSY;
    /*
     * Someday we could set up a timeout call-back here in case
     * the server never replies.
     */
    request.operation = PDEV_CLOSE;
    (void) RequestResponse(pdevHandlePtr, &request, 0, (Address)NIL,
		    0, (Address)NIL, (int *) NIL, (Sync_RemoteWaiter *)NIL);
exit:
    pdevHandlePtr->flags &= ~(PDEV_BUSY|FS_USER);
    Sync_Broadcast(&pdevHandlePtr->access);
    UNLOCK_MONITOR;
}
