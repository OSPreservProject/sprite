/* 
 * pdev.c --
 *
 *	The routines in this module set up a call-back interface for
 *	a pseudo-device server.  Pdev_Open creates a pseudo-device and
 *	installs service procedures.  These service procedures are called
 *	when client processes use the pseudo-device.  The harness procedure
 *	which invokes the call-backs takes care of the kernel interface.
 *	There are also default service procedures so the user of Pdev_Open
 *	need only provide service procedures for the operations of interest.
 *
 * Copyright 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/lib/c/etc/RCS/pdev.c,v 1.15 90/06/27 11:17:12 shirriff Exp $ SPRITE (Berkeley)";
#endif not lint

#include <stdio.h>
#include <sprite.h>
#include <ctype.h>
#include <errno.h>
#include <pdev.h>
#include <list.h>
#include <status.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>

/*
 * The string below holds an error message if Pdev_Open fails.
 */

char pdev_ErrorMsg[150];

/*
 * Boolean that can be toggled by applications to get tracing.
 */
int pdev_Trace;

/*
 * The Pdev structure is the top-level state for the pseudo-device
 * or pseudo-filelsystem server.  The state for each pseudo-device connection
 * is hung in a list off of this.  This also has a set of default service
 * procedures for the various pseudo-device operations.  Two
 * fields of the Pdev struct are made available to the client of the
 * Pdev package so it can do IOControls on the service stream and
 * set a private data pointer.
 */

typedef struct Pdev {
    Pdev_Stream stream;		/* Type PDEV_MAGIC */
    List_Links connectList;	/* List of all service streams for this
				 * pseudo-device (pseudo-filesystem). */
    int requestBufSize;		/* Default request buffer size */
    int readBufSize;		/* Size for optional read buffer */
    Pdev_CallBacks *defaultService;/* Default handlers for pdev operations */
} Pdev;

/*
 * The structure below corresponds to a pseudo-device connection between
 * a client and the server.
 */

typedef struct ServiceStream {
    Pdev_Stream stream;		/* Type PDEV_STREAM_MAGIC */
    List_Links links;		/* pdev streams are linked into a
				 * list of all streams for the same
				 * pseudo-device (pseudo-file-system). */
    Address parent;		/* Pdev state with which this stream is
				 * associated. */
    int streamID;		/* Sprite identifier for request stream. */
    Address requestBuf;		/* A buffer for requests and data from the
				 * client */
    Address readBuf;		/* A buffer for data to be read over the
				 * pseudo-device by the application. This
				 * is allocated by us and passed to the
				 * read call-back. If read buffering is set
				 * then we allocate this at open-time and
				 * pass it off to the open call-back. */
    int readBufSize;		/* Size of the readBuf. */
    Address ioctlOutBuf;	/* Buffer for results of ioctl */
    int ioctlBufSize;		/* Size of ioctlOutBut */
    Pdev_CallBacks *service;	/* Set of procedures to handle the various
				 * pseudo-device operations. */
} ServiceStream;

#define LIST_TO_SRVPTR(listPtr) \
	(ServiceStream *)((int)(listPtr) - sizeof(Pdev_Stream))

static int	PdevDefaultOpen();
static int	PdevDefaultRead();
static int	PdevDefaultWrite();
static int	PdevDefaultIoctl();
static int	PdevDefaultClose();
static int	PdevDefaultGetAttr();
static int	PdevDefaultSetAttr();

static Pdev_CallBacks pdevDefaultCallBacks = {
     PdevDefaultOpen,
     PdevDefaultRead,
     PdevDefaultWrite,
     PdevDefaultIoctl,
     PdevDefaultClose,
     PdevDefaultGetAttr,
     PdevDefaultSetAttr,
};

/*
 * PDEV_MIN_BYTES		Minimum number of data bytes in request buffer.
 * PFS_REQUEST_BUF_SIZE 	Size of the request buffer for naming stream.
 */

#define PDEV_MIN_BYTES	1024
#define PDEV_REQUEST_BUF_SIZE	(sizeof(Pdev_Request) + PDEV_MIN_BYTES)

/*
 * Forward references to procedures in this file:
 */

static void	PdevControlRequest();
static void	PdevServiceRequest();
static int	PdevCleanup();
static void	ReplyNoData();
static void	ReplyWithData();
static int    (*SetHandler())();

/*
 *----------------------------------------------------------------------
 *
 * Pdev_Open --
 *
 *	Arrange to be the server for a pseudo-device.  This creates the
 *	pseudo-device file, choosing a unique name if needed (see below).
 *	The set of service call-backs are installed and will be called
 *	when regular processes operate on the pseudo-device.  See the
 *	man page for Pdev for details of the call-back interface.
 *
 * Results:
 *	The return value is a token for the pseudo-device, which gets
 *	passed to Pdev_Close.  A NULL return value
 *	means that the pseudo-device could not be opened.  If realNamePtr
 *	is non-NULL, *realNamePtr is filled in with a dynamically-
 *	allocated string giving the actual name of the pseudo-device
 *	file.
 *
 * Side effects:
 *	A pseudo-device is opened in master mode.  If realNamePtr is
 *	NULL then name is the complete name of the pseudo device;  if
 *	realNamePtr is not NULL, then this procedure generates a
 *	pseudo-device name of the form hostDir/nameXX, where "hostDir"
 *	is the name of a standard host-specific directory for holding
 *	terminal pseudo-devices and XX is an integer id appended to
 *	"name" in order to find a device that isn't already in use.
 *	Once this procedure returns, this module manages the pseudo
 *	device to provide a simple call-back interface to service procedures.
 *	I,e, for each operation on the pseudo-device by a another process,
 *	a call-back for that operation is made to one of the supplied
 *	service procedures.
 *
 *----------------------------------------------------------------------
 */

Pdev_Token
Pdev_Open(name, realNamePtr, requestBufSize, readBufSize, service, clientData)
    char *name;			/* Name of pseudo-device file to use for
				 * application interface, or key for generating
				 * name (if realNamePtr != NULL).  If no
				 * pseudo-device by that name exists, one
				 * will be created. */
    char **realNamePtr;		/* If not NULL, then use "name" as a key for
				 * a name (see above) and store actual name of
				 * pseudo-device here.  The memory for the
				 * string is dynamically allocated. */
    int requestBufSize;		/* Preferred size for the request buffer.
				 * Can be <= 0 for a default size */
    int readBufSize;		/* Size for optional read buffer.  Zero means
				 * no buffering and we use the read call-back*/
    Pdev_CallBacks *service;	/* A set of service procedures.
				 * This can be NULL (or any element can be
				 * NULL) in order to get a default handler
				 * for all (some) operations.  The callbacks
				 * can be changed with Pdev_SetupHandler */
    ClientData clientData;	/* Client data associated with pseudo-device */
{
    int streamID;
    register Pdev *pdevPtr;
    int pdevOpenFlags;

    /*
     * Pick a file name to use for the pseudo-device, if the caller didn't
     * give us one, then open the pseudo-device as the controlling process.
     */

    pdevOpenFlags = O_MASTER|O_RDWR|O_CREAT;
    if (realNamePtr != NULL) {
	char hostName[50];
	int i;
	char *actualName;

	if (gethostname(hostName, 20) != 0) {
	    sprintf(pdev_ErrorMsg, "couldn't get host name (%s)",
		    strerror(errno));
	    return (Pdev_Token) NULL;
	} else {
	    /*
	     * Trim off domain name, if any
	     */
	    char *cp;

	    cp = index(hostName, '.');
	    if (cp != (char *)NULL) {
		*cp = '\0';
	    }
	}
	actualName = (char *) malloc((unsigned) (12 + strlen(hostName)
		+ strlen(name)));
	for (i = 1; i < 100; i++) {
	    sprintf(actualName, "/hosts/%s/%s%d", hostName,
		    name, i);

	    /*
	     * Because of umask, the file's mode may not actually get set
	     * to 0666.  Once the file is open, change the mode explicitly
	     * to force it.
	     */

	    streamID = open(actualName, pdevOpenFlags, 0666);
	    if (streamID >= 0) {
		fchmod(streamID, 0666);
		*realNamePtr = actualName;
		goto gotStream;
	    }
	}
	free((char *) actualName);
	sprintf(pdev_ErrorMsg, 
		"couldn't open a pseudo-device in \"/hosts/%s\"", hostName);
	return (Pdev_Token) NULL;
    } else {
	streamID = open(name, pdevOpenFlags, 0666);
	if (streamID < 0) {
	    sprintf(pdev_ErrorMsg, "couldn't open \"%s\" (%s)",
		    name, strerror(errno));
	    return (Pdev_Token) NULL;
	} else {
	    fchmod(streamID, 0666);
	}
    }

gotStream:
    pdevPtr = (Pdev *) malloc(sizeof(Pdev));
    pdevPtr->stream.magic = PDEV_MAGIC;
    pdevPtr->stream.streamID = streamID;
    pdevPtr->stream.clientData = clientData;
    pdevPtr->requestBufSize = requestBufSize;
    pdevPtr->readBufSize = readBufSize;
    List_Init(&pdevPtr->connectList);
    pdevPtr->defaultService = (Pdev_CallBacks *)malloc(sizeof(Pdev_CallBacks));
    if (service == (Pdev_CallBacks *)NULL) {
	bzero((Address) pdevPtr->defaultService, sizeof(Pdev_CallBacks));
    } else {
	bcopy((Address) service, (Address) pdevPtr->defaultService,
		    sizeof(Pdev_CallBacks));
    }
    Fs_EventHandlerCreate(streamID, FS_READABLE, PdevControlRequest,
	    (ClientData) pdevPtr);

    return (Pdev_Token) pdevPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Pdev_Close --
 *
 *	Close down a pseudo-device and release all of the
 *	state associated with it.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory gets recycled, and clients on the other end of the
 *	pseudo-device will probably terminate.  After this call,
 *	the caller should never again use the pseudo-device.
 *
 *----------------------------------------------------------------------
 */

void
Pdev_Close(pdevToken)
    Pdev_Token pdevToken;	/* Token identifying the pseudo-device.
				 * This is returned from Pdev_Open. */
{
    register Pdev *pdevPtr = (Pdev *) pdevToken;
    List_Links *listPtr;
    register ServiceStream *srvPtr;

    while (!List_IsEmpty(&pdevPtr->connectList)) {
	listPtr = List_First(&pdevPtr->connectList);
	srvPtr = LIST_TO_SRVPTR(listPtr);
	(void)PdevCleanup(srvPtr, FALSE);
    }
    Fs_EventHandlerDestroy(pdevPtr->stream.streamID);
    close(pdevPtr->stream.streamID);
    free((Address)pdevToken);
}

/*
 *----------------------------------------------------------------------
 *
 * Pdev_GetStreamID --
 *
 *	Return the descriptor associated with a master's pdev.  
 *
 * Results:
 *	The streamID is returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Pdev_GetStreamID(pdevToken)
    Pdev_Token pdevToken;	/* Token identifying the pseudo-device.
				 * This is returned from Pdev_Open. */
{
    register Pdev *pdevPtr = (Pdev *) pdevToken;
    return(pdevPtr->stream.streamID);
}


/*
 *----------------------------------------------------------------------
 *
 * PdevCleanup --
 *
 *	Called when a client has closed in order to
 *	clean up any associated state.
 *
 * Results:
 *	Returns the status from the close call-back.
 *
 * Side effects:
 *	Calls the close-call back, then cleans up all state associated
 *	with the pseudo-device.
 *
 *----------------------------------------------------------------------
 */

static int
PdevCleanup(srvPtr, sendReply)
    register ServiceStream *srvPtr;	/* Service stream info. */
    Boolean sendReply;			/* TRUE if we should reply to the
					 * close request.  This is done in
					 * normal termination.  FALSE means
					 * no reply needed, used to close
					 * pending connections when the
					 * server is bailing out */
{
    int status;

    status = (*srvPtr->service->close)(&srvPtr->stream);
#ifdef lint
    status = PdevDefaultClose(&srvPtr->stream);
#endif /* lint */

    if (sendReply) {
	ReplyNoData(srvPtr, status, 0);
    }
    List_Remove(&srvPtr->links);
    Fs_EventHandlerDestroy(srvPtr->stream.streamID);
    close(srvPtr->stream.streamID);
    free((char *) srvPtr->requestBuf);
    if (srvPtr->service != (Pdev_CallBacks *)NULL) {
	free((char *) srvPtr->service);
    }
    if (srvPtr->readBuf != NULL) {
	free(srvPtr->readBuf);
    }
    if (srvPtr->ioctlOutBuf != NULL) {
	free(srvPtr->ioctlOutBuf);
    }
    free((char *) srvPtr);
    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * PdevControlRequest --
 *
 *	This procedure is invoked by Fs_Dispatcher when the control
 *	stream for a pseudo-device is readable.  This means that
 *	a new stream is being opened on the pdev.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Add a new service stream to the pdev.
 *
 *----------------------------------------------------------------------
 */

static void
PdevControlRequest(pdevPtr)
    register Pdev *pdevPtr;	/* Pdev whose control stream is ready. */
{
    Pdev_Notify notify;
    int numBytes;

    /*
     * Read the control stream for a message containing a new streamID.
     */

    numBytes = read(pdevPtr->stream.streamID, (char *) &notify, sizeof(notify));
    if (numBytes != sizeof(notify)) {
	panic("%s; status \"%s\", count %d\n",
		"PdevControlRequest couldn't read control stream",
		strerror(errno), numBytes);
    }
    if (notify.magic != PDEV_NOTIFY_MAGIC) {
	panic("%s: %d\n", "PdevControlRequest got bad notify magic number",
		notify.magic);
    }
    (void) PdevSetup(notify.newStreamID, (Address)pdevPtr,
		&pdevPtr->connectList, pdevPtr->requestBufSize,
		pdevPtr->readBufSize, (char *)0, pdevPtr->defaultService, 0);
}

/*
 *----------------------------------------------------------------------
 *
 * PdevSetup --
 *
 *	Set up state for a pseudo-device connection. This includes a request
 *	buffer used by the kernel to pass client requests to us and a set
 *	of service call-back procedures.
 *	
 * Results:
 *	A pointer to the user's handle on the service stream.
 *
 * Side effects:
 *	Allocates and initializes state.  Makes IOControls to set
 *	attributes of the pdev connection.
 *
 *----------------------------------------------------------------------
 */

Pdev_Stream *
PdevSetup(streamID, backPointer, streamList, reqBufSize, readBufSize, readBuf, service, selectBits)
    int streamID;		/* Service stream */
    Address backPointer;	/* Pointer to global state */
    List_Links *streamList;	/* List of pseudo-device streams */
    int reqBufSize;		/* Size for the request buffer */
    int readBufSize;		/* Size for the read buffer, if any */
    char *readBuf;		/* The read buffer itself, NULL if it should
				 * be allocated here */
    Pdev_CallBacks *service;	/* Set of service call-backs */
    int selectBits;		/* Initial select state of the connection */
{
    register ServiceStream *srvPtr;
    Pdev_SetBufArgs setBuf;
    int false = 0;

    srvPtr = (ServiceStream *) malloc(sizeof(ServiceStream));
    List_InitElement(&srvPtr->links);
    List_Insert(&srvPtr->links, LIST_ATFRONT(streamList));
    srvPtr->stream.magic = PDEV_STREAM_MAGIC;
    srvPtr->stream.streamID = streamID;
    srvPtr->stream.clientData = 0;
    srvPtr->parent = backPointer;
    srvPtr->service = (Pdev_CallBacks *)malloc(sizeof(Pdev_CallBacks));
    bcopy((Address) service, (Address) srvPtr->service,
	    sizeof(Pdev_CallBacks));
    if (srvPtr->service->open == (int (*)())NULL) {
	srvPtr->service->open = pdevDefaultCallBacks.open;
    }
    if (srvPtr->service->read == (int (*)())NULL) {
	srvPtr->service->read = pdevDefaultCallBacks.read;
    }
    if (srvPtr->service->write == (int (*)())NULL) {
	srvPtr->service->write = pdevDefaultCallBacks.write;
    }
    if (srvPtr->service->ioctl == (int (*)())NULL) {
	srvPtr->service->ioctl = pdevDefaultCallBacks.ioctl;
    }
    if (srvPtr->service->close == (int (*)())NULL) {
	srvPtr->service->close = pdevDefaultCallBacks.close;
    }
    if (srvPtr->service->getAttr == (int (*)())NULL) {
	srvPtr->service->getAttr = pdevDefaultCallBacks.getAttr;
    }
    if (srvPtr->service->setAttr == (int (*)())NULL) {
	srvPtr->service->setAttr = pdevDefaultCallBacks.setAttr;
    }
    if (readBufSize > 0) {
	/*
	 * Server wants a read buffer.  We allocate it now and declare
	 * it to the kernel with the IOC_PDEV_SET_BUFS call below.
	 */
	if (readBuf == NULL) {
	    srvPtr->readBuf = malloc(readBufSize);
	} else {
	    srvPtr->readBuf = readBuf;
	}
	srvPtr->readBufSize = readBufSize;
    } else {
	/*
	 * No read buffering, but we will allocate a buffer for passing
	 * to the read service call-back later.
	 */
	srvPtr->readBuf = NULL;
	srvPtr->readBufSize = 0;
    }
    srvPtr->ioctlOutBuf = NULL;
    srvPtr->ioctlBufSize = 0;

    reqBufSize = max(PDEV_REQUEST_BUF_SIZE, reqBufSize);
    srvPtr->requestBuf = (Address) malloc(reqBufSize);
    setBuf.requestBufAddr = srvPtr->requestBuf;
    setBuf.requestBufSize = reqBufSize;
    setBuf.readBufAddr = srvPtr->readBuf;
    setBuf.readBufSize = srvPtr->readBufSize;
    Fs_IOControl(streamID, IOC_PDEV_WRITE_BEHIND,
		    sizeof(int), (Address)&false, 0, (Address) NULL);
    Fs_IOControl(streamID, IOC_PDEV_SET_BUF,
		    sizeof(Pdev_SetBufArgs), (Address)&setBuf,
		    0, (Address) NULL);
    Fs_IOControl(streamID, IOC_PDEV_READY,
		    sizeof(int), (Address)&selectBits, 0, (Address) NULL);

    Fs_EventHandlerCreate(streamID, FS_READABLE, PdevServiceRequest,
	    (ClientData) srvPtr);

    return ((Pdev_Stream *)srvPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * PdevServiceRequest --
 *
 *	This procedure is invoked by Fs_Dispatch when a request appears
 *	for an service stream.   This procedure reads the request and
 *	dispatches to a routine to handle the request.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Makes the call-backs to the service procedures passed to Pdev_Open.
 *	Generates pseudo-device replies to complete the protocol with
 *	the kernel.
 *
 *----------------------------------------------------------------------
 */

static void
PdevServiceRequest(srvPtr)
    register ServiceStream *srvPtr;	/* Application stream that has a
					 * request ready for processing. */
{
    Pdev_BufPtrs bufPtrs;
    Pdev_Signal signalInfo;
    Pdev_Request *requestPtr;
    Address dataPtr;		/* pointer to data following header */
    Pdev_Reply reply;
    int numBytes;
    int status;
    int selectBits;
    int savedFirstByte;

    /*
     * Read the current pointers for the request buffer.
     */

    numBytes = read(srvPtr->stream.streamID, (char *) &bufPtrs,
	    sizeof(Pdev_BufPtrs));
    if (numBytes != sizeof(Pdev_BufPtrs)) {
	panic("%s; status \"%s\", count %d\n",
	    "PdevServiceRequest had trouble reading request buffer pointers",
	    strerror(errno), numBytes);
    }
    if (bufPtrs.magic != PDEV_BUF_PTR_MAGIC) {
	panic("%s: %d\n", "PdevServiceRequest got bad pointer magic number",
		bufPtrs.magic);
    }
    savedFirstByte = bufPtrs.requestFirstByte;
    /*
     * While there are still requests in the buffer, service them.
     */
    while (bufPtrs.requestFirstByte < bufPtrs.requestLastByte) {
	requestPtr =
	    (Pdev_Request *)&srvPtr->requestBuf[bufPtrs.requestFirstByte];
	if (requestPtr->hdr.magic != PDEV_REQUEST_MAGIC) {
	    printf("PdevServiceRequest, alignment error: firstByte %d currentByte %d lastByte %d magic %x\n",
		  savedFirstByte,
		  bufPtrs.requestFirstByte, bufPtrs.requestLastByte,
		  requestPtr->hdr.magic);
	    /*
	     * Ignore bogus request.
	     */
	    bufPtrs.requestFirstByte = bufPtrs.requestLastByte + 1;
	    break;
	}
	dataPtr = (Address)((int)requestPtr + sizeof(Pdev_Request));
	signalInfo.signal = 0;
	signalInfo.code = 0;

	switch (requestPtr->hdr.operation) {
	    case PDEV_OPEN: {
		/*
		 * This is the first operation on a pseudo-device.  The
		 * server can set srvPtr->private here, which will be
		 * passed into the other handlers.
		 */
		Pdev *pdevPtr;
		if (pdev_Trace) {
		    fprintf(stderr, "OPEN %d: uid %d use %x",
			    srvPtr->stream.streamID,
			    requestPtr->param.open.uid,
			    requestPtr->param.open.flags);
		}
		pdevPtr = (Pdev *)srvPtr->parent;
		status = (*srvPtr->service->open)
			    (pdevPtr->stream.clientData, &srvPtr->stream,
				srvPtr->readBuf,
				requestPtr->param.open.flags,
				requestPtr->param.open.pid,
				requestPtr->param.open.hostID,
				requestPtr->param.open.uid,
				&selectBits);
#ifdef lint
		status = PdevDefaultOpen(pdevPtr->stream.clientData, &srvPtr->stream,
				srvPtr->readBuf,
				requestPtr->param.open.flags,
				requestPtr->param.open.pid,
				requestPtr->param.open.hostID,
				requestPtr->param.open.uid,
				&selectBits);
#endif /* lint */
		ReplyNoData(srvPtr, status, selectBits);
		break;
	    }
	    case PDEV_CLOSE:
		if (pdev_Trace) {
		    fprintf(stderr, "CLOSE %d", srvPtr->stream.streamID);
		}
		status = PdevCleanup(srvPtr, TRUE);
		break;
	    case PDEV_READ: {
		/*
		 * For reading we pass in a pre-allocated buffer, but also
		 * allow the read procedure to use a different buffer.  If it
		 * does change the buffer it also indicates if we should
		 * free the new buffer after we reply.  We hold onto our
		 * own srvPtr->readBuf until the connection is closed.
		 */
		Boolean freeIt = FALSE;

		if (pdev_Trace) {
		    fprintf(stderr, "READ %d bytes at offset %d\n",
			requestPtr->hdr.replySize,
			requestPtr->param.read.offset);
		}

		reply.replySize = requestPtr->hdr.replySize;
		if (reply.replySize > srvPtr->readBufSize) {
		    /*
		     * Increase the read buffer size.
		     */
		    if (srvPtr->readBuf != NULL) {
			free(srvPtr->readBuf);
		    }
		    srvPtr->readBuf = malloc(reply.replySize);
		    srvPtr->readBufSize = reply.replySize;
		}
		requestPtr->param.read.buffer = srvPtr->readBuf;
		status = (*srvPtr->service->read)(&srvPtr->stream,
				&requestPtr->param.read, &freeIt,
				&reply.selectBits,
				&signalInfo);
#ifdef lint
		status = PdevDefaultRead(&srvPtr->stream,
				&requestPtr->param.read, &freeIt,
				&reply.selectBits,
				&signalInfo);
#endif /* lint */

		reply.replySize = requestPtr->param.read.length;
		reply.replyBuf = requestPtr->param.read.buffer;
		ReplyWithData(srvPtr, status, &reply, &signalInfo);
		if (freeIt &&
		    (requestPtr->param.read.buffer != srvPtr->readBuf)) {
		    free(requestPtr->param.read.buffer);
		}
		break;
	    }
	    case PDEV_WRITE_ASYNC:
	    case PDEV_WRITE: {
		if (pdev_Trace) {
		    fprintf(stderr, "WRITE %d bytes at offset %d",
			requestPtr->hdr.requestSize,
			requestPtr->param.read.offset);
		}
		requestPtr->param.write.buffer = dataPtr;
		status = (*srvPtr->service->write)(&srvPtr->stream,
				(requestPtr->hdr.operation == PDEV_WRITE_ASYNC),
				&requestPtr->param.write,
				&reply.selectBits,
				&signalInfo);
#ifdef lint
		status = PdevDefaultWrite(&srvPtr->stream,
				(requestPtr->hdr.operation == PDEV_WRITE_ASYNC),
				&requestPtr->param.write,
				&reply.selectBits,
				&signalInfo);
#endif /* lint */
		if (requestPtr->hdr.operation == PDEV_WRITE) {
		    reply.replySize = sizeof(int);
		    reply.replyBuf = (Address)&requestPtr->param.write.length;
		    ReplyWithData(srvPtr, status, &reply, &signalInfo);
		}
		break;
	    }
	    case PDEV_IOCTL: {
		if (pdev_Trace) {
		    fprintf(stderr, "IOCTL %d", requestPtr->param.ioctl.command);
		}
		/*
		 * The kernel sets up the sizes of the two buffers,
		 * but we have our own notion of where they are.
		 * We grow the out buffer if needed, for example,
		 * and the inBuffer is sitting in the request buffer at dataPtr.
		 */
		reply.replySize = requestPtr->hdr.replySize;
		if (reply.replySize > srvPtr->ioctlBufSize) {
		    if (srvPtr->ioctlOutBuf != NULL) {
			free(srvPtr->ioctlOutBuf);
		    }
		    srvPtr->ioctlOutBuf = malloc(reply.replySize);
		    srvPtr->ioctlBufSize = reply.replySize;
		}
		requestPtr->param.ioctl.inBuffer = dataPtr;
		requestPtr->param.ioctl.outBuffer = srvPtr->ioctlOutBuf;
		status = (*srvPtr->service->ioctl)(&srvPtr->stream,
				 &requestPtr->param.ioctl,
				 &reply.selectBits,
				 &signalInfo);
#ifdef lint
		status = PdevDefaultIoctl(&srvPtr->stream,
				 &requestPtr->param.ioctl,
				 &reply.selectBits,
				 &signalInfo);
#endif /* lint */
		reply.replyBuf = srvPtr->ioctlOutBuf;
		reply.replySize = requestPtr->param.ioctl.outBufSize;
		ReplyWithData(srvPtr, status, &reply, &signalInfo);
		break;
	    }
	    case PDEV_GET_ATTR: {
		Fs_Attributes attr;

		if (pdev_Trace) {
		    fprintf(stderr, "GET ATTR");
		}
		status = (*srvPtr->service->getAttr)(&srvPtr->stream,
				&attr, &reply.selectBits);
#ifdef lint
		status = PdevDefaultGetAttr(&srvPtr->stream,
				&attr, &reply.selectBits);
#endif /* lint */
		if (status == SUCCESS) {
		    reply.replyBuf = (Address)&attr;
		    reply.replySize = sizeof(Fs_Attributes);
		    ReplyWithData(srvPtr, status, &reply, (Pdev_Signal *)NIL);
		} else {
		    ReplyNoData(srvPtr, status, reply.selectBits);
		}
		break;
	    }
	    case PDEV_SET_ATTR: {
		if (pdev_Trace) {
		    fprintf(stderr, "SET ATTR %x", requestPtr->param.setAttr.flags);
		}
		status = (*srvPtr->service->setAttr)(&srvPtr->stream,
				requestPtr->param.setAttr.flags,
				requestPtr->param.setAttr.uid,
				requestPtr->param.setAttr.gid,
				(Fs_Attributes *)dataPtr, &reply.selectBits);
#ifdef lint
		status = PdevDefaultSetAttr(&srvPtr->stream,
				requestPtr->param.setAttr.flags,
				requestPtr->param.setAttr.uid,
				requestPtr->param.setAttr.gid,
				(Fs_Attributes *)dataPtr, &reply.selectBits);
#endif /* lint */
		ReplyNoData(srvPtr, status, reply.selectBits);
		break;
	    }
	    default:
		panic("PdevServiceRequest: bad request on request stream: %d\n",
			requestPtr->hdr.operation);
	}
	if (pdev_Trace) {
	    fprintf(stderr, " Returns %x\n", status);
	}
	/*
	 * Move to the next request message.
	 */
	bufPtrs.requestFirstByte += requestPtr->hdr.messageSize;
    }
    /*
     * Tell the kernel we processed the requests.
     */
    Fs_IOControl(srvPtr->stream.streamID, IOC_PDEV_SET_PTRS,
		    sizeof(Pdev_BufPtrs), (Address)&bufPtrs,
		    0, (Address) NULL);
}

/*
 *----------------------------------------------------------------------
 *
 * ReplyNoData --
 *
 *	Send a reply back with no data;  just a return status.  This
 *	procedure is most often used for error returns.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The application will receive status as the return from the
 *	system call it invoked.
 *
 *----------------------------------------------------------------------
 */

static void
ReplyNoData(srvPtr, status, selectBits)
    ServiceStream *srvPtr;	/* Application stream info. */
    int status;	/* Error code to send to application. */
    int selectBits;		/* Current select state for the stream */
{
    Pdev_Reply reply;

    reply.magic = PDEV_REPLY_MAGIC;
    reply.selectBits = selectBits;
    if (status == EWOULDBLOCK) {
	status = FS_WOULD_BLOCK;
    } else {
	status = Compat_MapToSprite(status);
    }
    reply.status = status;
    reply.replySize = 0;
    reply.replyBuf = NULL;
    reply.signal = 0;
    reply.code = 0;
    status = Fs_IOControl(srvPtr->stream.streamID, IOC_PDEV_REPLY,
		sizeof(Pdev_Reply), (Address) &reply, 0, (Address) NULL);
    if (status != SUCCESS) {
	panic("%s; status \"%s\"\n", "Reply couldn't send pdev reply",
		Stat_GetMsg(status));
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ReplyWithData --
 *
 *	Send a reply back along with some data.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The application will receive status as the return from the
 *	system call it invoked.
 *
 *----------------------------------------------------------------------
 */

static void
ReplyWithData(srvPtr, status, replyPtr, sigPtr)
    ServiceStream *srvPtr;	/* Application stream info. */
    int status;			/* Error code to send to application. */
    Pdev_Reply *replyPtr;	/* Partially completed reply.  The replySize,
				 * data area, and selectBits should be set. */
    Pdev_Signal *sigPtr;	/* Signal to return, if any */
{
    if (status == EWOULDBLOCK) {
	status = FS_WOULD_BLOCK;
    } else {
	status = Compat_MapToSprite(status);
    }
    if (sigPtr != (Pdev_Signal *)NIL) {
	replyPtr->signal = sigPtr->signal;
	replyPtr->code = sigPtr->code;
    } else {
	replyPtr->signal = 0;
	replyPtr->code = 0;
    }
    if (replyPtr->replySize <= PDEV_SMALL_DATA_LIMIT) {
	Pdev_ReplyData replyData;

	replyData.magic = PDEV_REPLY_DATA_MAGIC;
	replyData.status = status;
	replyData.selectBits = replyPtr->selectBits;
	replyData.replySize = replyPtr->replySize;
	replyData.signal = replyPtr->signal;
	replyData.code = replyPtr->code;
	bcopy(replyPtr->replyBuf, replyData.data, replyPtr->replySize);
	status = Fs_IOControl(srvPtr->stream.streamID, IOC_PDEV_SMALL_REPLY,
		sizeof(Pdev_ReplyData), (Address)&replyData, 0, (Address) NULL);
    } else {
	replyPtr->magic = PDEV_REPLY_MAGIC;
	replyPtr->status = status;
	status = Fs_IOControl(srvPtr->stream.streamID, IOC_PDEV_REPLY,
		    sizeof(Pdev_Reply), (Address) replyPtr, 0, (Address) NULL);
    }
    if (status != SUCCESS) {
	panic("%s; status \"%s\"\n", "ReplyWithData couldn't send pdev reply",
		Stat_GetMsg(status));
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Pdev_EnumStreams --
 *
 *	Apply a function to all service streams associated with a pdev.
 *
 * Results:
 *	0 means all the applications of the function returned 0 as well.
 *	If the function returns non-zero when applied to a stream then
 *	the enumeration is stopped and that value is returned.
 *
 * Side effects:
 *	None here.
 *
 *----------------------------------------------------------------------
 */

int
Pdev_EnumStreams(pdevToken, func, clientData)
    Pdev_Token pdevToken;
    int (*func)();
    ClientData clientData;
{
    register Pdev *pdevPtr = (Pdev *)pdevToken;
    register List_Links *listPtr;
    register ServiceStream *srvPtr;
    register int status;

    if (pdevPtr->stream.magic != (unsigned int)PDEV_MAGIC) {
	fprintf(stderr, "Pdev_EnumStreams passed bad pdevToken\n");
	return -1;
    }
    LIST_FORALL(&pdevPtr->connectList, listPtr) {
	srvPtr = LIST_TO_SRVPTR(listPtr);
	status = (*func)((Pdev_Stream *)srvPtr, clientData);
	if (status != 0) {
	    return(status);
	}
    }
    return(0);
}


/*
 *----------------------------------------------------------------------
 *
 * Pfs_SetDefaultHandler --
 *
 *	Set a default handler for a particular PDEV request.  If the handler is
 *	NULL then a default procedure replaces the existing handler.
 *	The default handlers are used when new connections are established
 *	to the pseudo-device.
 * 
 * Results:
 *	The old handler.
 *
 * Side effects:
 *	Updates the call-back list.
 *
 *----------------------------------------------------------------------
 */
int (*
Pdev_SetDefaultHandler(token, operation, handler))()
    Pdev_Token token;		/* Returned from Pdev_Open */
    int operation;		/* Which operation to set the handler for */
    int (*handler)();		/* The callback procedure */
{
    register Pdev		*pdevPtr = (Pdev *)token;
    register int		(*oldHandler)();

    if (pdevPtr->stream.magic != (unsigned int)PDEV_MAGIC) {
	fprintf(stderr, "Bad token passed to Pdev_SetHandler\n");
	return (int (*)())NULL;
    }

    oldHandler = SetHandler(pdevPtr->defaultService, operation, handler);
    return oldHandler;
}

/*
 *----------------------------------------------------------------------
 *
 * Pfs_SetStreamHandler --
 *
 *	Set a handler for a particular stream and PDEV request.  If the handler
 *	is NULL then a default procedure replaces the existing handler.
 *	This call only affects the handler for the given stream to the
 *	pseudo-device.
 * 
 * Results:
 *	The old handler.
 *
 * Side effects:
 *	Updates the call-back list.
 *
 *----------------------------------------------------------------------
 */
int (*
Pdev_SetStreamHandler(streamPtr, operation, handler))()
    Pdev_Stream *streamPtr;/* Handler for stream to pseudo-device */
    int operation;		/* Which operation to set the handler for */
    int (*handler)();		/* The callback procedure */
{
    register ServiceStream	*srvPtr = (ServiceStream *)streamPtr;
    register int		(*oldHandler)();

    if (srvPtr->stream.magic != (unsigned int)PDEV_STREAM_MAGIC) {
	fprintf(stderr, "Bad token passed to Pdev_SetHandler\n");
	return (int (*)())NULL;
    }

    oldHandler = SetHandler(srvPtr->service, operation, handler);
    return oldHandler;
}

/*
 *----------------------------------------------------------------------
 *
 * SetHandler --
 *
 *	Set the handler for a pseudo-device operation and return the
 *	old handler.
 *
 * Results:
 *	The old handler.
 *
 * Side effects:
 *	Change the handler.
 *
 *----------------------------------------------------------------------
 */

static int (*
SetHandler(service, operation, handler))()
    Pdev_CallBacks *service;
    int operation;
    int (*handler)();
{
    int (*oldHandler)();

    switch (operation) {
	case PDEV_OPEN:
	    oldHandler = service->open;
	    if (handler == (int (*)())NULL) {
		service->open = pdevDefaultCallBacks.open;
	    } else {
		service->open = handler;
	    }
	    break;
	case PDEV_CLOSE:
	    oldHandler = service->close;
	    if (handler == (int (*)())NULL) {
		service->close = pdevDefaultCallBacks.close;
	    } else {
		service->close = handler;
	    }
	    break;
	case PDEV_READ:
	    oldHandler = service->read;
	    if (handler == (int (*)())NULL) {
		service->read = pdevDefaultCallBacks.read;
	    } else {
		service->read = handler;
	    }
	    break;
	case PDEV_WRITE:
	    oldHandler = service->write;
	    if (handler == (int (*)())NULL) {
		service->write = pdevDefaultCallBacks.write;
	    } else {
		service->write = handler;
	    }
	    break;
	case PDEV_IOCTL:
	    oldHandler = service->ioctl;
	    if (handler == (int (*)())NULL) {
		service->ioctl = pdevDefaultCallBacks.ioctl;
	    } else {
		service->ioctl = handler;
	    }
	    break;
	case PDEV_GET_ATTR:
	    oldHandler = service->getAttr;
	    if (handler == (int (*)())NULL) {
		service->getAttr = pdevDefaultCallBacks.getAttr;
	    } else {
		service->getAttr = handler;
	    }
	    break;
	case PDEV_SET_ATTR:
	    oldHandler = service->setAttr;
	    if (handler == (int (*)())NULL) {
		service->setAttr = pdevDefaultCallBacks.setAttr;
	    } else {
		service->setAttr = handler;
	    }
	    break;
	default:
	    fprintf(stderr, "Bad operation passed to Pdev_SetHandler");
	    oldHandler = (int (*)())NULL;
	    break;
    }
    return oldHandler;
}

/*
 *----------------------------------------------------------------------
 *
 * PdevDefaultOpen --
 *
 *	Default procedure is called when an PDEV_OPEN request is
 *	received over an service stream.
 *
 * Results:
 *	Returns SUCCESS and the select state of the pseudo-device.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
PdevDefaultOpen(clientData, streamPtr, readBuf, flags, procID, hostID,
	uid, selectBitsPtr)
    ClientData clientData;	/* Client data associated with pseudo-device */
    Pdev_Stream *streamPtr;	/* Identifies open stream */
    char *readBuf;		/* Optional read buffer */
    int flags;			/* Flags passed to open system call */
    int procID;			/* Process ID doing the open */
    int hostID;			/* Host on which process is executing */
    int uid;			/* User id of process */
    int *selectBitsPtr;		/* Return - select state of pseudo-device */
{
    *selectBitsPtr = FS_READABLE | FS_WRITABLE;
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * PdevDefaultClose --
 *
 *	Default procedure is called when an PDEV_CLOSE request is
 *	received over an service stream.
 *
 * Results:
 *	Returns SUCCESS and the select state of the pseudo-device.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
PdevDefaultClose(streamPtr)
    Pdev_Stream *streamPtr;
{
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * PdevDefaultRead --
 *
 *	The default read procedure.  This simulates EOF by returning
 *	SUCCESS but no characters.
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
static int
PdevDefaultRead(streamPtr, readPtr, freeItPtr, selectBitsPtr, sigPtr)
    Pdev_Stream *streamPtr;	/* Service stream. */
    Pdev_RWParam *readPtr;	/* Read parameter block. */
    Boolean *freeItPtr;		/* Return indicates if *bufferPtr is malloc'd */
    int *selectBitsPtr;		/* Return - the select state of the pdev */
    Pdev_Signal *sigPtr;	/* Return - signal to generate. */
{
    *freeItPtr = FALSE;
    readPtr->length = 0;
    *selectBitsPtr = FS_READABLE | FS_WRITABLE;
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * PdevDefaultWrite --
 *
 *	The default write procedure.  This accepts and discards all data.
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
static int
PdevDefaultWrite(streamPtr, async, writePtr, selectBitsPtr, sigPtr)
    Pdev_Stream *streamPtr;	/* Private data */
    int async;			/* TRUE if asynchronous and no reply needed */
    Pdev_RWParam *writePtr;	/* Write parameter block. */
    int *selectBitsPtr;		/* Result - select state of the pseudo-device */
    Pdev_Signal *sigPtr;	/* Return - signal to generate. */
{
    *selectBitsPtr = FS_READABLE | FS_WRITABLE;
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * PdevDefaultIoctl --
 *
 *	The default IOControl handling procedure called when an
 *	PDEV_IOCONTROL request is received over a request stream.
 *
 * Results:
 *	None.
 *
 * Side effects
 *	None.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
PdevDefaultIoctl(streamPtr, ioctlPtr, selectBitsPtr, sigPtr)
    Pdev_Stream *streamPtr;	/* Stream to service. */
    Pdev_IOCParam *ioctlPtr;	/* ioctl parameter block. */
    int *selectBitsPtr;		/* Return - select state of pdev. */
    Pdev_Signal *sigPtr;	/* Return - signal to generate. */
{
    ioctlPtr->outBufSize = 0;
    *selectBitsPtr = FS_READABLE | FS_WRITABLE;
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * PdevDefaultGetAttr --
 *
 *	The default GetAttributes handling procedure called when an
 *	PDEV_GET_ATTR request is received over a request stream.
 *
 * Results:
 *	None.
 *
 * Side effects
 *	None.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
PdevDefaultGetAttr(streamPtr, attrPtr, selectBitsPtr)
    Pdev_Stream *streamPtr;
    Fs_Attributes *attrPtr;
    int *selectBitsPtr;
{
    bzero((Address)attrPtr, sizeof(Fs_Attributes));
    attrPtr->fileNumber = (int)streamPtr->clientData;
    attrPtr->permissions = 0644;
    attrPtr->type = FS_FILE;
    *selectBitsPtr = FS_READABLE | FS_WRITABLE;
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * PdevDefaultSetAttr --
 *
 *	The default GetAttributes handling procedure called when an
 *	PDEV_SET_ATTR request is received over a request stream.
 *
 * Results:
 *	None.
 *
 * Side effects
 *	None.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
PdevDefaultSetAttr(streamPtr, flags, uid, gid, attrPtr, selectBitsPtr)
    Pdev_Stream *streamPtr;
    int flags;
    int uid;
    int gid;
    Fs_Attributes *attrPtr;
    int *selectBitsPtr;
{
    *selectBitsPtr = FS_READABLE | FS_WRITABLE;
    return(SUCCESS);
}
