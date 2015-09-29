/* 
 * server.c --
 *
 *	The server part of some multi-program synchronization primatives.
 *	The routines here control N client programs.  This just means
 *	telling them all to start, and hearing back from them when they're done.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/cmds/bench/RCS/server.c,v 1.12 90/02/16 11:10:41 jhh Exp $ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"
#include "status.h"
#include "fs.h"
#include "dev/pdev.h"
#include "sys/file.h"
#include "stdio.h"
#include "bit.h"
#include "sys.h"

char *pdev="/sprite/daemons/bench.pdev";

typedef int  (*IntProc)();

typedef struct ServerState {
    int cntlStream;		/* StreamID of pdev control stream */
    int numClients;		/* Number of client processes */
    int *clientStream;		/* Array of server streamIDs used to communicate
				 * with the N clients */
    int maxStreamID;		/* Largest value in clientStream array */
    char *clientState;		/* Array of state words, one per client */
    Address *requestBuf;	/* Array of pointers to request buffers.  The
				 * kernel puts request directly into these. */
    int *selectMask;		/* Mask used to wait for requests */
    int selectMaskBytes;	/* Number of bytes in selectMask */
    IntProc opTable[7];		/* Op. switch for servicing requests */
} ServerState;

/*
 * Flags for the client state words.
 */
#define CLIENT_OPENED	0x1
#define CLIENT_STARTED	0x2
#define CLIENT_FINISHED	0x4

/*
 * The size of the request buffers.
 */
#define REQ_BUF_SIZE	128

extern int errno;

/*
 * Forward Declarations
 */
ReturnStatus ServeOpen();
ReturnStatus ServeRead();
ReturnStatus ServeWrite();
ReturnStatus ServeIOControl();
ReturnStatus ServeClose();

Pdev_Op ServeRequest();


/*
 *----------------------------------------------------------------------
 *
 * ServerSetup --
 *
 *	Establish contact with N clients.  A pseudo device is opened
 *	and we are declared its "master", or "server".  After this
 *	other processes can open the pseudo device and we'll get a private
 *	stream back that we use for requests from that process.
 *
 * Results:
 *	A pointer to state about the clients needed by ServerStart and
 *	ServerWait.
 *
 * Side effects:
 *	Opens the pseudo device as the server and waits for numClients
 *	opens by client processes.
 *	This exits (kills the process) upon error.
 *
 *----------------------------------------------------------------------
 */

void
ServerSetup(numClients, dataPtr)
    int numClients;
    ClientData *dataPtr;
{
    ServerState *statePtr;
    int client;
    int len;
    int amountRead;
    ReturnStatus status;
    Pdev_Notify notify;
    int streamID;
    Pdev_SetBufArgs setBuf;
    extern Boolean waitForSignal;
    extern Boolean startClients;

    statePtr = (ServerState *)malloc(sizeof(ServerState));
    statePtr->clientStream = (int *)malloc(numClients * sizeof(int));
    statePtr->clientState = (char *)malloc(numClients);
    statePtr->requestBuf = (Address *)malloc(numClients * sizeof(Address));
    statePtr->numClients = numClients;

    statePtr->opTable[(int)PDEV_OPEN] = ServeOpen;
    statePtr->opTable[(int)PDEV_CLOSE] = ServeClose;
    statePtr->opTable[(int)PDEV_READ] = ServeRead;
    statePtr->opTable[(int)PDEV_WRITE] = ServeWrite;
    statePtr->opTable[(int)PDEV_IOCTL] = ServeIOControl;

    /*
     * Open the pseudo device.
     */
    statePtr->cntlStream = open(pdev, O_RDONLY|O_MASTER, 0666);
    if (statePtr->cntlStream < 0) {
	statePtr->cntlStream = open(pdev, O_CREAT|O_RDONLY|O_MASTER, 0666);
    }
    if (statePtr->cntlStream < 0) {
	Stat_PrintMsg(errno, "Error opening pseudo device as master");
	exit(errno);
    }
    for (client=0 ; client<numClients ; client++) {
	/*
	 * Read on our control stream (the one we just opened) messages
	 * that contain new streamIDs.  These are for private streams
	 * back to the client process.
	 */
	len = sizeof(notify);
	amountRead = read(statePtr->cntlStream, (Address)&notify, len);
	if (amountRead < 0) {
	    Stat_PrintMsg(errno, "Error reading control stream");
	    exit(errno);
	} else if (amountRead != sizeof(notify)) {
	    fprintf(stderr,
		"Warning, short read (%d) on control stream\n", amountRead);
	    fflush(stderr);
	}
	streamID = notify.newStreamID;
	if (streamID > statePtr->maxStreamID) {
	    statePtr->maxStreamID = streamID;
	}
	/*
	 * Set up state for the client.
	 */
	statePtr->clientStream[client] = streamID;
	statePtr->clientState[client] = CLIENT_OPENED;
	statePtr->requestBuf[client] = (Address) malloc(REQ_BUF_SIZE);
	/*
	 * Tell the kernel where the request buffer is.
	 */
	setBuf.requestBufAddr = statePtr->requestBuf[client];
	setBuf.requestBufSize = REQ_BUF_SIZE;
	setBuf.readBufAddr = NULL;
	setBuf.readBufSize = 0;
	Fs_IOControl(streamID, IOC_PDEV_SET_BUF, sizeof(Pdev_SetBufArgs),
			(Address)&setBuf, 0, (Address)NULL);

	fprintf(stderr, "Got client on stream %d\n",streamID);
	fflush(stderr);
    }
    if (waitForSignal) {
	while (!startClients) {
	    sigpause(0);
	}
    }
    /*
     * Now start all clients at once by servicing their first PDEV_OPEN
     * request.
     */
    for (client=0 ; client<numClients ; client++) {
	(void) ServeRequest(statePtr->clientStream[client],
		     statePtr->requestBuf[client], statePtr->opTable);
    }
    fflush(stderr);
    /*
     * Now that we know the largest stream ID used for a client stream
     * we can allocate and initialize the select mask for the streams.
     */
    statePtr->selectMaskBytes = Bit_NumBytes(statePtr->maxStreamID);
    statePtr->selectMask = (int *)malloc(statePtr->selectMaskBytes);
    bzero((Address)statePtr->selectMask, statePtr->selectMaskBytes);
    for (client=0 ; client < numClients ; client++) {
	Bit_Set(statePtr->clientStream[client], statePtr->selectMask);
    }
    *dataPtr = (ClientData)statePtr;
}

/*
 *----------------------------------------------------------------------
 *
 * ServeRequest --
 *
 *	The top level service routine that reads client requests
 *	and branches out to a handler for the request.  This takes
 *	care of error conditions and allocating space for the
 *	request and the reply parameters.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	Reads and writes on the client's stream.  malloc space for the
 *	request and the reply.  Will reset the client stream upon error.
 *
 *----------------------------------------------------------------------
 */

Pdev_Op
ServeRequest(clientStreamID, requestBuf, opTable)
    int clientStreamID;		/* StreamID of private channel to client */
    Address requestBuf;		/* Buffer that holds client's requests */
    IntProc *opTable;		/* Operation switch table */
{
    int amountRead;
    ReturnStatus status;
    Pdev_BufPtrs bufPtrs;		/* Used to read ptrs into req. buf. */
    Pdev_Request *requestPtr;	/* Points to request header */
    Pdev_Reply reply;
    Address requestData;		/* Points to data that follows header */
    char replyBuf[1024];
    char *replyData = replyBuf;		/* Points to reply data */
    int replySize;			/* Amount of valid data in replyBuf */
    Boolean alloc;

    /*
     * Read the request header that indicates the operation and the
     * offset within the request buffer of the request message.
     */
    amountRead = read(clientStreamID, (Address)&bufPtrs, sizeof(Pdev_BufPtrs));
    if (amountRead < 0) { 
	Stat_PrintMsg(errno, "ServeRequest: error reading request bufPtrs");
	goto failure;
    } else if (amountRead != sizeof(Pdev_BufPtrs)) {
	fprintf(stderr,
	    "ServeRequest: short read (%d) of request bufPtrs\n", amountRead);
	fflush(stderr);
	goto failure;
    } else if ((int)bufPtrs.magic != PDEV_BUF_PTR_MAGIC) {
	fprintf(stderr, "ServeRequest: bad bufPtr magic 0x%x\n",
		bufPtrs.magic);
	status = FAILURE;
	goto failure;
    }
    /*
     * While there are still requests in the buffer, service them.
     */
    while (bufPtrs.requestFirstByte < bufPtrs.requestLastByte) {
	requestPtr = (Pdev_Request *)&requestBuf[bufPtrs.requestFirstByte];
	if (requestPtr->hdr.magic != PDEV_REQUEST_MAGIC) {
	    /*
	    Sys_Panic(SYS_FATAL, "ServeRequest, bad request magic # 0x%x\n",
			    requestPtr->magic);
	    */
	    abort();
	}
	/*
	 * Set up a buffer for the reply data;
	 */
	if (requestPtr->hdr.replySize > 1024) {
	    alloc = TRUE;
	    replyData = (Address) malloc(requestPtr->hdr.replySize);
	} else {
	    alloc = FALSE;
	    replyData = replyBuf;
	}
	requestData = (Address)((int)requestPtr + sizeof(Pdev_Request));
	/*
	 * Switch out the to the handler for the pdev operation.
	 */
	status = (*opTable[(int)requestPtr->hdr.operation])(clientStreamID,
			requestPtr, requestData, replyData, &replySize);
	/*
	 * Copy the data into the reply and give it to the kernel.
	 */
    
	reply.magic = PDEV_REPLY_MAGIC;
	reply.status = status;
	reply.selectBits = FS_READ|FS_WRITE;
	reply.replySize = replySize;
	reply.replyBuf = replyData;
	reply.signal = 0;
	reply.code = 0;
	status = Fs_IOControl(clientStreamID, IOC_PDEV_REPLY,
			    sizeof(Pdev_Reply), (Address) &reply, 0,
			    (Address) NULL);
	if (status != SUCCESS) {
	    /*
	    Sys_Panic(SYS_FATAL, "%s; status \"%s\"",
		    "ServeRequest couldn't send reply",
		    Stat_GetMsg(status));
	    */
	    abort();
	}
	if (alloc) {
	    free(replyData);
	}
	/*
	 * Tell the kernel we removed a request and see if there are any more.
	 */
	bufPtrs.requestFirstByte += requestPtr->hdr.messageSize;
	Fs_IOControl(clientStreamID, IOC_PDEV_SET_PTRS,
		    sizeof(Pdev_BufPtrs), (Address)&bufPtrs, 0, (Address) NULL);
    }
    return(requestPtr->hdr.operation);

failure:
    /*
     * Couldn't even get started right.  We'd like to be able to "reset
     * the connection" at this point - do something to cause the waiting
     * client to return, and something to flush any remaining input
     * in the request stream.  Need a special IOControl.
     */
    close(clientStreamID);
    return((Pdev_Op)-1);
}

/*
 *----------------------------------------------------------------------
 *
 * Serve --
 *
 *	Listen for requests from client's, returning after all clients
 *	have closed their streams.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	Handle all requests by clients.
 *
 *----------------------------------------------------------------------
 */

void
Serve(data)
    ClientData data;
{
    ServerState *statePtr;
    int client;
    ReturnStatus status;
    int *selectMask;
    int numFinishedClients;
    int numReady;
    int numBits;

    statePtr = (ServerState *)data;
    selectMask = (int *)malloc(statePtr->selectMaskBytes);
    numBits = statePtr->selectMaskBytes * BIT_NUM_BITS_PER_BYTE;
    numFinishedClients = 0;
    do {
	bcopy( (Address)statePtr->selectMask, (Address)selectMask, 
		statePtr->selectMaskBytes);
	status = Fs_Select(numBits, (Time *)NULL, selectMask,
				(int *)NULL, (int *)NULL, &numReady);
	if (status != SUCCESS) {
	    /*
	    Sys_Panic(SYS_FATAL, "Serve: select failed, %s\n",
		Stat_GetMsg(status));
	   */
	   abort();
	}
	for (client=0 ; client < statePtr->numClients ; client++) {
	    /*
	     * Look for the each client's bit in the select mask and
	     * service requests that have arrived.
	     */
	    if (Bit_IsSet(statePtr->clientStream[client], selectMask)) {
		/*
		 * Handle the client's request.  If it's a close
		 * then clear the client's bit from the select mask so
		 * don't bother checking it again.
		 */
		if (ServeRequest(statePtr->clientStream[client],
				 statePtr->requestBuf[client],
				 statePtr->opTable) == PDEV_CLOSE) {
		    fprintf(stderr, "Client %d closed... ", client);
		    fflush(stderr);
		    numFinishedClients++;
		    statePtr->clientState[client] |= CLIENT_FINISHED;
		    Bit_Clear(statePtr->clientStream[client],
			    statePtr->selectMask);
		}
	    }
	}
    } while (numFinishedClients < statePtr->numClients);
    fprintf(stderr, "\n");
    fflush(stderr);
}

/*
 *----------------------------------------------------------------------
 *
 * ServeOpen --
 *
 *	React to an Open or Dup request.
 *
 * Results:
 *	SUCCESS
 *
 * Side effects:
 *	Print statement.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
ServeOpen(streamID, requestPtr, requestData, replyData, replySizePtr)
    int streamID;
    Pdev_Request *requestPtr;
    Address requestData;
    Address replyData;
    int *replySizePtr;
{
    fprintf(stderr, "Open request, streamID %d, pid %x\n",
		streamID, requestPtr->param.open.pid);
    fflush(stderr);
    *replySizePtr = 0;
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * ServeRead --
 *
 *	Return data for a read request.
 *
 * Results:
 *	SUCCESS
 *
 * Side effects:
 *	Zeroes out the reply buffer.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
ServeRead(streamID, requestPtr, requestBuf, replyBuf, replySizePtr)
    int streamID;
    Pdev_Request *requestPtr;
    Address requestBuf;
    Address replyBuf;
    int *replySizePtr;
{
    if (requestPtr->hdr.replySize > 0) {
	bzero(replyBuf, requestPtr->hdr.replySize);
    }
    *replySizePtr = requestPtr->hdr.replySize;
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * ServeWrite --
 *
 *	Handle a write request.
 *
 * Results:
 *	SUCCESS
 *
 * Side effects:
 *	Sets up the return value, the number of bytes written.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
ServeWrite(streamID, requestPtr, requestBuf, replyBuf, replySizePtr)
    int streamID;
    Pdev_Request *requestPtr;
    Address requestBuf;
    Address replyBuf;
    int *replySizePtr;
{
    *replySizePtr = 0;
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * ServeIOControl --
 *
 *	Handle an IOControl.  This acts like an echo now.
 *
 * Results:
 *	SUCCESS
 *
 * Side effects:
 *	Copies the request buffer to the reply buffer.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
ServeIOControl(streamID, requestPtr, requestBuf, replyBuf, replySizePtr)
    int streamID;
    Pdev_Request *requestPtr;
    Address requestBuf;
    Address replyBuf;
    int *replySizePtr;
{
    if (requestPtr->hdr.replySize <= requestPtr->hdr.requestSize) {
	bcopy(requestBuf, replyBuf, requestPtr->hdr.replySize);
    } else {
	bcopy(requestBuf, replyBuf, requestPtr->hdr.requestSize);
	bzero( replyBuf[requestPtr->hdr.requestSize],
		requestPtr->hdr.replySize - requestPtr->hdr.requestSize);
    }
    *replySizePtr = requestPtr->hdr.replySize;
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * ServeClose --
 *
 *	Handle a close request.
 *
 * Results:
 *	SUCCESS
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
ServeClose(streamID, requestPtr, requestBuf, replyBuf, replySizePtr)
    int streamID;
    Pdev_Request *requestPtr;
    Address requestBuf;
    Address replyBuf;
    int *replySizePtr;
{
    *replySizePtr = 0;
    return(SUCCESS);
}
