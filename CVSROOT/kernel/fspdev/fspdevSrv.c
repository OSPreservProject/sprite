/* 
 * fsPdev.c --  
 *
 *	Routines to implement pseudo-devices. A pseudo-device is a file
 *	that acts as a communication channel between a user-level server
 *	process (hereafter called the "server"), and one or more client
 *	processes (hereafter called the "client"). Regular filesystem system
 *	calls (Fs_Read, Fs_Write, Fs_IOControl, Fs_Close) by a client process
 *	are forwarded to a special user process called the "server".  The
 *	server process can implement any sort of sementics for the file
 *	operations. The general format of Fs_IOControl, in particular, 
 *	lets the server implement any remote procedure call it cares to define.
 *	
 *	There are three kinds of streams involved in the implementation,
 *	a "control" stream that is returned to the server when it first
 *	opens the pseudo-device, a "client" stream that is returned to
 *	client processes when they open the pseudo-device, and finally
 *	there is a "server" stream for each client stream.  The server
 *	stream is created when a client opens the pseudo-device and it
 *	is passed to the server process using the control stream.
 *
 *	The first routines in this file are the open and close routines
 *	for these kinds of streams.  The next main routine is RequestResponse
 *	which implements the client side of the communication protocol
 *	between the client and the server.  The server side is implemented
 *	mainly by the server stream IOControl procedure.  Finally there
 *	are the various routines for I/O, recovery, etc.
 *
 *	A buffering scheme is used to improve the performance of pseudo streams.
 *	The server process declares a request buffer and optionally a read
 *	ahead buffer in its address space.  The kernel puts requests, which
 *	are generated when a client does an operation on the pseudo stream,
 *	into the request buffer directly.  The server learns of new requests
 *	by reading messages from the server stream that contain offsets within
 *	the request buffer.  Write requests do not require a response from
 *	the server.  Instead the kernel just puts the request into the buffer
 *	and returns to the client.  This allows many requests to be buffered
 *	before a context switch to the server process.  Similarly,
 *	the server can put read data into the read ahead buffer for a pseudo
 *	stream.  In this case a client's read will be satisfied from the
 *	buffer and the server won't be contacted.
 *
 * Copyright 1987 Regents of the University of California
 * All rights reserved.
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
/*
 * Prevent tracing by defining CLEAN here before this next include
 */
#undef CLEAN
#include "fsPdev.h"

Boolean fsPdevDebug = FALSE;		/* Turns on print statements */
Trace_Header pdevTraceHdr;
Trace_Header *pdevTraceHdrPtr = &pdevTraceHdr;
int pdevTraceLength = 200;
Boolean pdevTracing = TRUE;		/* Turns on circular trace */
int pdevMaxTraceDataSize;
int pdevTraceIndex = 0;

/*
 * PdevControlHandle is the I/O handle for the server's control stream.
 * There is a control handle kept on both on the file server and on the
 * host running the server process.  The one on the file server is used
 * by the SrvOpen routine to detect if a master exists, and the one on
 * the server's host is used for control messages, and it also is used
 * to detect if the server process is still alive (by looking at serverID).
 * There are not two different types of control handles because the differences
 * only come into play at close time.
 */
typedef struct PdevControlIOHandle {
    FsRemoteIOHandle rmt;	/* Type FS_CONTROL_STREAM.  This needs to
				 * be a remote I/O handle in order to do
				 * a remote close to the name server so
				 * the serverID field gets cleaned up right. */
    int serverID;		/* Host ID of server process.  If NIL it
				 * means there is no server.  This is kept */
    List_Links	queueHdr;	/* Control message queue */
    int	seed;			/* Used to make FileIDs for client handles */
    List_Links readWaitList;	/* So the server can wait for control msgs */
    FsLockState lock;		/* So the server can lock the pdev file */
} PdevControlIOHandle;

/*
 * Because there are corresponding control handles on the file server,
 * which records which host has the pdev server, and on the pdev server
 * itself, we need to be able to reopen the control handle on the
 * file server after it reboots.
 */
typedef struct PdevControlReopenParams {
    FsFileID	fileID;		/* FileID of the control handle */
    int		serverID;	/* ServerID recorded in control handle.
				 * This may be NIL if the server closes
				 * while the file server is down. */
    int		seed;		/* Used to create unique pseudo-stream fileIDs*/
} PdevControlReopenParams;

/*
 * The following control messages are passed internally from the
 * ServerStreamCreate routine to the FsControlRead routine.
 * They contain a streamPtr for a new server stream
 * that gets converted to a user-level streamID in FsControlRead.
 */

typedef struct PdevNotify {
    List_Links links;
    Fs_Stream *streamPtr;
} PdevNotify;

/*
 * Circular buffers are used for a request buffer and a read data buffer.
 * These buffers are in the address space of the server process so the
 * server can access them without system calls.  The server uses I/O controls
 * to change the pointers.
 */
typedef struct CircBuffer {
    Address data;		/* Location of the buffer in user-space */
    int firstByte;		/* Byte index of first valid data in buffer.
				 * if -1 then the buffer is empty */
    int lastByte;		/* Byte index of last valid data in buffer. */
    int size;			/* Number of bytes in the circular buffer */
} CircBuffer;

/*
 * PdevServerIOHandle has the main state for a client-server connection.
 * The client's handle is a stub which just has a pointer to this handle.
 */
typedef struct PdevServerIOHandle {
    FsHandleHeader hdr;		/* Standard header, type FS_LCL_PSEUDO_STREAM */
    Sync_Lock lock;		/* Used to synchronize access to this struct.
				 * The handle lock won't do because we
				 * use condition variables. */
    int flags;			/* Flags defined below */
    int selectBits;		/* Select state of the pseudo-stream */
    Proc_PID serverPID;		/* Server's processID needed for copy out */
    Proc_PID clientPID;		/* Client's processID needed for copy out */
    CircBuffer	requestBuf;	/* This buffer contains requests and any data
				 * that needs to follow the request header.
				 * The kernel fills this buffer and the
				 * server takes the requests and data out */
    Address nextRequestBuffer;	/* The address of the next request buffer in
				 * the server's address space to use.  We let
				 * the server change buffers in mid-flight. */
    int nextRequestBufSize;	/* Size of the new request buffer */
    CircBuffer readBuf;		/* This buffer contains read-ahead data for
				 * the pseudo-device.  The server program puts
				 * data here and the kernel removes it to
				 * satisfy client reads. If empty, the kernel
				 * asks the server explicitly for data and
				 * this buffer isn't used. */
    Pdev_Op operation;		/* Current operation.  Checked when handling
				 * the reply. */
    Pdev_Reply reply;	/* The current reply header is stuck here */
    Address replyBuf;		/* Pointer to reply data buffer.  This is in
				 * the client's address space unless the
				 * FS_USER flag is set */
    Sync_Condition setup;	/* This is notified after the server has set
				 * up buffer space for us.  A pseudo stream
				 * can't be used until this is done. */
    Sync_Condition access;	/* Notified after a RequestResponse to indicate
				 * that another client process can use the
				 * pseudo-stream. */
    Sync_Condition caughtUp;	/* This is notified after the server has read
				 * or set the buffer pointers.  The kernel
				 * waits for the server to catch up
				 * before safely resetting the pointers to
				 * the beginning of the buffer */
    Sync_Condition replyReady;	/* Notified after the server has replied */
    List_Links srvReadWaitList;	/* To remember the waiting server process */
    Sync_RemoteWaiter clientWait;/* Client process info for I/O waiting */
    List_Links cltReadWaitList;	/* These lists are used to remember clients */
    List_Links cltWriteWaitList;/*   waiting to read, write, or detect */
    List_Links cltExceptWaitList;/*   exceptions on the pseudo-stream. */
} PdevServerIOHandle;

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
 *	PDEV_READ_BUF_EMPTY	When set there is no data in the read ahead buf.
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
 *	PDEV_REMOTE_CLIENT	This is set when the client process is
 *				on a remote host.  This is needed in order
 *				to properly set the FS_USER flag (defined next).
 *				If the client is remote the I/O control
 *				buffers are always in kernel space.
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
#define PDEV_REMOTE_CLIENT	0x0400
/*resrv FS_USER			0x8000 */

/*
 * The client side stream for a pseudo-device.  This keeps a reference
 * to the server's handle with all the state.
 */
typedef struct PdevClientIOHandle {
    FsHandleHeader	hdr;
    PdevServerIOHandle	*pdevHandlePtr;
    List_Links		clientList;
} PdevClientIOHandle;

/*
 * Forward declarations.
 */

static	ReturnStatus		RequestResponse();
static	void			PdevClientWakeup();
static	void			PdevClientNotify();
static	PdevServerIOHandle	*ServerStreamCreate();
static	void			PseudoStreamCloseInt();
void				FsRmtPseudoStreamClose();

/*
 *----------------------------------------------------------------------------
 *
 * FsControlHandleInit --
 *
 *	Fetch and initialize a control handle for a pseudo-device.
 *
 * Results:
 *	A pointer to the control stream I/O handle.  The found parameter is
 *	set to TRUE if the handle was already found, FALSE if we created it.
 *
 * Side effects:
 *	Initializes and installs the control handle.
 *
 *----------------------------------------------------------------------------
 *
 */
PdevControlIOHandle *
FsControlHandleInit(fileIDPtr, foundPtr)
    FsFileID *fileIDPtr;
    Boolean *foundPtr;
{
    register Boolean found;
    register PdevControlIOHandle *ctrlHandlePtr;
    FsHandleHeader *hdrPtr;

    found = FsHandleInstall(fileIDPtr, sizeof(PdevControlIOHandle), &hdrPtr);
    ctrlHandlePtr = (PdevControlIOHandle *)hdrPtr;
    if (!found) {
	ctrlHandlePtr->serverID = NIL;
	List_Init(&ctrlHandlePtr->queueHdr);
	ctrlHandlePtr->seed = 0;
	List_Init(&ctrlHandlePtr->readWaitList);
	FsLockInit(&ctrlHandlePtr->lock);
	FsRecoveryInit(&ctrlHandlePtr->rmt.recovery);
    }
    *foundPtr = found;
    return(ctrlHandlePtr);
}

/*
 *----------------------------------------------------------------------------
 *
 * FsPseudoDevSrvOpen --
 *
 *	Early open time processing, this is called on a fileserver when setting
 *	up state for a call to the CltOpen routines on the client host.
 *	For pseudo-device server processes, which are indicated by the
 *	FS_MASTER flag, check that no other server exists.  For all other
 *	processes, which are referred to as "clients",
 *	make sure that a server process exists and generate a new
 *	ioFileID for the connection between the client and the server.
 *
 * Results:
 *	For server processes, SUCCESS if it is now the server, FS_FILE_BUSY
 *	if there already exists a server process.
 *	For clients, SUCCESS if there is a server or the parameters indicate
 *	this is only for get/set attributes, DEV_OFFLINE if there is no server.
 *
 * Side effects:
 *	Save the hostID of the calling process if
 *	it is to be the server for the pseudo-device.
 *
 *----------------------------------------------------------------------------
 *
 */
ReturnStatus
FsPseudoDevSrvOpen(handlePtr, clientID, useFlags, ioFileIDPtr, streamIDPtr,
	dataSizePtr, clientDataPtr)
     register FsLocalFileIOHandle *handlePtr;	/* A handle from FsLocalLookup.
					 * Should be LOCKED upon entry,
					 * unlocked upon exit. */
     int		clientID;	/* Host ID of client doing the open */
     register int	useFlags;	/* FS_MASTER, plus
					 * FS_READ | FS_WRITE | FS_EXECUTE*/
     register FsFileID	*ioFileIDPtr;	/* Return - I/O handle ID */
     FsFileID		*streamIDPtr;	/* Return - stream ID. 
					 * NIL during set/get attributes */
     int		*dataSizePtr;	/* Return - sizeof(FsPdevState) */
     ClientData		*clientDataPtr;	/* Return - a reference to FsPdevState.
					 * Nothing is returned during set/get
					 * attributes */

{
    register	ReturnStatus status = SUCCESS;
    FsFileID	ioFileID;
    Boolean	found;
    register	PdevControlIOHandle *ctrlHandlePtr;
    register	Fs_Stream *streamPtr;
    register	FsPdevState *pdevStatePtr;

    /*
     * The control I/O handle is identified by the fileID of the pseudo-device
     * file with type CONTROL.  The minor field has the disk decriptor version
     * number or'ed into it to avoid conflict when you delete the
     * pdev file and recreate one with the same file number (minor field).
     */
    ioFileID = handlePtr->hdr.fileID;
    ioFileID.type = FS_CONTROL_STREAM;
    ioFileID.minor |= (handlePtr->descPtr->version << 16);
    ctrlHandlePtr = FsControlHandleInit(&ioFileID, &found);

    if (useFlags & (FS_MASTER|FS_NEW_MASTER)) {
	/*
	 * When a server opens we ensure there is only one.
	 */
	if (found && ctrlHandlePtr->serverID != NIL) {
	    status = FS_FILE_BUSY;
	} else {
	    /*
	     * Return to the pseudo-device server a ControlIOHandle that
	     * has us, the name server, in the serverID field.  This is
	     * used when closing the control stream to get back to us
	     * so we can clear the serverID field here.  We also set up
	     * a shadow stream here, which has us as the server so
	     * recovery and closing work right.
	     */
	    ctrlHandlePtr->serverID = clientID;
	    *ioFileIDPtr = ioFileID;
	    *clientDataPtr = (ClientData)NIL;
	    *dataSizePtr = 0;
	    streamPtr = FsStreamNew(rpc_SpriteID, ctrlHandlePtr, useFlags);
	    *streamIDPtr = streamPtr->hdr.fileID;
	    FsStreamClientOpen(&streamPtr->clientList, clientID, useFlags);
	    FsHandleRelease(streamPtr, TRUE);
	}
    } else {
	if (streamIDPtr == (FsFileID *)NIL) {
	    /*
	     * Set up for get/set attributes.  We point the client
	     * at the name of the pseudo-device, what else?
	     */
	    *ioFileIDPtr = handlePtr->hdr.fileID;
	} else if (!found || ctrlHandlePtr->serverID == NIL) {
	    /*
	     * No server process.
	     */
	    status = DEV_OFFLINE;
	} else {
	    /*
	     * The server exists.  Create a new I/O handle for the client.
	     * The major and minor numbers are generated from the fileID
	     * of the pseudo-device name (to avoid conflict with other
	     * pseudo-devices) and a clone seed (to avoid conflict with
	     * other clients of this pseudo-device).
	     */
	    if (ctrlHandlePtr->serverID == clientID) {
		ioFileIDPtr->type = FS_LCL_PSEUDO_STREAM;
	    } else {
		ioFileIDPtr->type = FS_RMT_PSEUDO_STREAM;
	    }
	    ioFileIDPtr->serverID = ctrlHandlePtr->serverID;
	    ioFileIDPtr->major = (handlePtr->hdr.fileID.serverID << 16) |
				  handlePtr->hdr.fileID.major;
	    ctrlHandlePtr->seed++;
	    ioFileIDPtr->minor = (handlePtr->hdr.fileID.minor << 16) |
				  ctrlHandlePtr->seed;
	    /*
	     * Return the control stream file ID so it can be found again
	     * later when setting up the client's stream and the
	     * corresponding server stream.  The procID and uid fields are
	     * extra here, but will be used later if the client is remote.
	     */
	    pdevStatePtr = Mem_New(FsPdevState);
	    pdevStatePtr->ctrlFileID = ctrlHandlePtr->rmt.hdr.fileID;
	    pdevStatePtr->procID = (Proc_PID)NIL;
	    pdevStatePtr->uid = NIL;
	    *clientDataPtr = (ClientData)pdevStatePtr ;
	    *dataSizePtr = sizeof(FsPdevState);
	    /*
	     * Set up a top level stream for the opening process.  No shadow
	     * stream is kept here.  Instead, the streamID is returned to
	     * the pdev server who sets up a shadow stream.
	     */
	    streamPtr = FsStreamNew(ctrlHandlePtr->serverID,
				    (FsHandleHeader *)NIL, useFlags);
	    *streamIDPtr = streamPtr->hdr.fileID;
	    pdevStatePtr->streamID = streamPtr->hdr.fileID;
	    FsStreamDispose(streamPtr);
	}
    }
    FsHandleRelease(ctrlHandlePtr, TRUE);
    FsHandleUnlock(handlePtr);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsControlCltOpen --
 *
 *	Complete setup of the server's control stream.  Called from
 *	Fs_Open on the host running the server.  We mark the Control
 *	I/O handle as having a server (us).
 * 
 * Results:
 *	SUCCESS.
 *
 * Side effects:
 *	Installs the Control I/O handle and keeps a reference to it.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
FsControlCltOpen(ioFileIDPtr, flagsPtr, clientID, streamData, ioHandlePtrPtr)
    register FsFileID	*ioFileIDPtr;	/* I/O fileID */
    int			*flagsPtr;	/* FS_READ | FS_WRITE ... */
    int			clientID;	/* Host doing the open */
    ClientData		streamData;	/* NIL. */
    FsHandleHeader	**ioHandlePtrPtr;/* Return - a locked handle set up for
					 * I/O to a control stream, or NIL */
{
    register PdevControlIOHandle	*ctrlHandlePtr;
    Boolean		found;

    ctrlHandlePtr = FsControlHandleInit(ioFileIDPtr, &found);
    if (found && !List_IsEmpty(&ctrlHandlePtr->queueHdr)) {
	Sys_Panic(SYS_FATAL, "FsControlStreamCltOpen found control msgs\n");
    }
    ctrlHandlePtr->serverID = clientID;
    *ioHandlePtrPtr = (FsHandleHeader *)ctrlHandlePtr;
    FsHandleUnlock(ctrlHandlePtr);
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * FsPseudoStreamCltOpen --
 *
 *	This is called from Fs_Open, or from the RPC stub if the client
 *	is remote, to complete setup of a client's
 *	stream to the pseudo-device.  The server is running on this
 *	host.  This routine creates a trivial client I/O handle
 *	that references the server's I/O handle that has the main
 *	state for the connection to the server.  ServerStreamCreate
 *	is then called to set up the server's I/O handle, and the control
 *	stream is used to pass a sever stream to the server.  Finally
 *	an open transaction is made with the server process
 *	to see if it will accept the client.
 * 
 * Results:
 *	SUCCESS, unless the server process has died recently, or the
 *	server rejects the open.
 *
 * Side effects:
 *	Creates the client's I/O handle.  Calls ServerStreamCreate
 *	which sets up the servers corresponding I/O handle.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
FsPseudoStreamCltOpen(ioFileIDPtr, flagsPtr, clientID, streamData, ioHandlePtrPtr)
    register FsFileID	*ioFileIDPtr;	/* I/O fileID */
    int			*flagsPtr;	/* FS_READ | FS_WRITE ... */
    int			clientID;	/* Host doing the open */
    ClientData		streamData;	/* Ponter to FsPdevState. */
    FsHandleHeader	**ioHandlePtrPtr;/* Return - a locked handle set up for
					 * I/O to a pseudo device, or NIL */
{
    ReturnStatus		status;
    Boolean			found;
    register PdevClientIOHandle	*cltHandlePtr;
    register PdevControlIOHandle	*ctrlHandlePtr;
    register FsPdevState		*pdevStatePtr;
    Proc_ControlBlock		*procPtr;
    Proc_PID 			procID;
    int				uid;

    pdevStatePtr = (FsPdevState *)streamData;
    ctrlHandlePtr = FsHandleFetchType(PdevControlIOHandle,
				    &pdevStatePtr->ctrlFileID);
    /*
     * If there is no server present the creation of the stream
     * can't succeed.  This case arises when the pseudo-device
     * master goes away between FsPseudoDevSrvOpen and this call.
     */
    if ((ctrlHandlePtr == (PdevControlIOHandle *)NIL) ||
	(ctrlHandlePtr->serverID == NIL)) {
	status = DEV_OFFLINE;
	if (ctrlHandlePtr != (PdevControlIOHandle *)NIL) {
	    FsHandleRelease(ctrlHandlePtr, TRUE);
	}
	goto exit;
    }

    /*
     * Extract the seed from the minor field (see the SrvOpen routine).
     * This done in case of recovery when we'll need to reset the
     * seed kept on the file server.
     */
    ctrlHandlePtr->seed = ioFileIDPtr->minor & 0xFFFF;

    found = FsHandleInstall(ioFileIDPtr, sizeof(PdevClientIOHandle),
			    ioHandlePtrPtr);
    cltHandlePtr = (PdevClientIOHandle *)(*ioHandlePtrPtr);
    if (found) {
	if ((cltHandlePtr->pdevHandlePtr != (PdevServerIOHandle *)NIL) &&
	    (cltHandlePtr->pdevHandlePtr->clientPID != (unsigned int)NIL)) {
	    Sys_Panic(SYS_WARNING,
		"FsPseudoStreamCltOpen found client handle\n");
	    Sys_Printf("Check (and kill) client process %x\n",
		cltHandlePtr->pdevHandlePtr->clientPID);
	}
	/*
	 * Invalidate this lingering handle.  The client process is hung
	 * or suspended and hasn't closed its end of the pdev connection.
	 */
	FsHandleInvalidate(cltHandlePtr);
	FsHandleRelease(cltHandlePtr, TRUE);

	found = FsHandleInstall(ioFileIDPtr, sizeof(PdevClientIOHandle),
			ioHandlePtrPtr);
	cltHandlePtr = (PdevClientIOHandle *)(*ioHandlePtrPtr);
	if (found) {
	    Sys_Panic(SYS_FATAL, "FsPseudoStreamCltOpen handle still there\n");
	}
    }
    /*
     * We have to look around and decide if we are being called
     * from Fs_Open, or via RPC from a remote client.  A remote client's
     * processID and uid are passed to us via the FsPdevState.  We also
     * have to ensure that a FS_STREAM exists and has the remote client
     * on its list so the client's remote I/O ops. are accepted here.
     * The remote client's stream is closed for us by the close rpc stub.
     */
    if (clientID == rpc_SpriteID) {
	procPtr = Proc_GetEffectiveProc();
	procID = procPtr->processID;
	uid = procPtr->effectiveUserID;
    } else {
	register Fs_Stream *cltStreamPtr;

	procID = pdevStatePtr->procID;
	uid = pdevStatePtr->uid;

	cltStreamPtr = FsStreamFind(&pdevStatePtr->streamID,
			    cltHandlePtr, *flagsPtr, &found);
	(void)FsStreamClientOpen(&cltStreamPtr->clientList,
				clientID, *flagsPtr);
	FsHandleUnlock(cltStreamPtr);
    }
    /*
     * Set up a service stream and hook the client handle to it.
     */
    cltHandlePtr->pdevHandlePtr = ServerStreamCreate(ctrlHandlePtr,
						     ioFileIDPtr, clientID);
    List_Init(&cltHandlePtr->clientList);
    FsIOClientOpen(&cltHandlePtr->clientList, clientID, 0, FALSE);
    FsHandleRelease(ctrlHandlePtr, TRUE);
    /*
     * Grab an extra reference to the server's handle so the
     * server close routine can remove the handle and it won't
     * go away until the client also closes.
     */
    FsHandleDup((FsHandleHeader *)cltHandlePtr->pdevHandlePtr);
    FsHandleUnlock(cltHandlePtr->pdevHandlePtr);
    /*
     * Now that the request response stream is set up we do
     * our first transaction with the server process to see if it
     * will accept the open.
     */
    status = PseudoStreamOpen(cltHandlePtr->pdevHandlePtr, *flagsPtr, clientID,
				procID, uid);
    if (status == SUCCESS) {
	*ioHandlePtrPtr = (FsHandleHeader *)cltHandlePtr;
	FsHandleUnlock(cltHandlePtr);
    } else {
	FsHandleRelease(cltHandlePtr, TRUE);
    }
exit:
    Mem_Free((Address)streamData);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsRmtPseudoStreamCltOpen --
 *
 *	Complete a remote client's stream to a pseudo-device.
 *	The client is on a different host than the server process.  This
 *	makes an RPC to the server's host to invoke FsPseudoStreamCltOpen.
 *	This host only keeps a FsRemoteIOHandle, and the FsRemoteIOClose
 *	routine is used to close it.
 * 
 * Results:
 *	SUCCESS unless the server process has died recently, then DEV_OFFLINE.
 *
 * Side effects:
 *	RPC to the server's host to invoke the regular setup routines.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
FsRmtPseudoStreamCltOpen(ioFileIDPtr, flagsPtr, clientID, streamData, ioHandlePtrPtr)
    register FsFileID	*ioFileIDPtr;	/* I/O fileID */
    int			*flagsPtr;	/* FS_READ | FS_WRITE ... */
    int			clientID;	/* IGNORED (== rpc_SpriteID) */
    ClientData		streamData;	/* NIL for us. */
    FsHandleHeader	**ioHandlePtrPtr;/* Return - a locked handle set up for
					 * I/O to a pseudo device, or NIL */
{
    register ReturnStatus status;
    register FsPdevState *pdevStatePtr = (FsPdevState *)streamData;
    register FsRecoveryInfo *recovPtr;
    Proc_ControlBlock *procPtr;
    FsRemoteIOHandle *rmtHandlePtr;
    Boolean found;

    /*
     * Invoke via RPC FsPseudoStreamCltOpen.  Here we use the procID field
     * of the FsPdevState so that FsPseudoStreamCltOpen can pass them
     * to ServerStreamCreate.
     */
    procPtr = Proc_GetEffectiveProc();
    pdevStatePtr->procID = procPtr->processID;
    pdevStatePtr->uid = procPtr->effectiveUserID;
    ioFileIDPtr->type = FS_LCL_PSEUDO_STREAM;
    status = FsDeviceRemoteOpen(ioFileIDPtr, *flagsPtr,	sizeof(FsPdevState),
				(ClientData)pdevStatePtr);
    if (status == SUCCESS) {
	/*
	 * Install a remote I/O handle and initialize its recovery state.
	 */
	ioFileIDPtr->type = FS_RMT_PSEUDO_STREAM;
	found = FsHandleInstall(ioFileIDPtr, sizeof(FsRemoteIOHandle),
		(FsHandleHeader **)&rmtHandlePtr);
	recovPtr = &rmtHandlePtr->recovery;
	if (!found) {
	    FsRecoveryInit(recovPtr);
	}
	recovPtr->use.ref++;
	if (*flagsPtr & FS_WRITE) {
	    recovPtr->use.write++;
	}
	*ioHandlePtrPtr = (FsHandleHeader *)rmtHandlePtr;
	FsHandleUnlock(rmtHandlePtr);
    }
    Mem_Free((Address)pdevStatePtr);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * ServerStreamCreate --
 *
 *	Set up the stream state for a server's private channel to a client.
 *	This creates a PdevServerIOHandle that has all the state for the
 *	connection to the client.  A stream to this handle is created
 *	and passed to the server process via the control stream.  Its
 *	address is enqueued in a control message, and later on FsControlRead
 *	will map the address to a user streamID for the server process.
 *	Finally, the PdevServerIOHandle is also returned to our caller
 *	so the client's I/O handle can reference it.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The I/O handle for this connection between a client and the server
 *	is installed and initialized.  Also, a stream for the server
 *	is created and its address is enqueued in a control message.
 *
 *----------------------------------------------------------------------
 */

static PdevServerIOHandle *
ServerStreamCreate(ctrlHandlePtr, ioFileIDPtr, slaveClientID)
    PdevControlIOHandle *ctrlHandlePtr;	/* Control stream of a pseudo-device.
					 * LOCKED on entry, please. */
    FsFileID	*ioFileIDPtr;	/* File ID used for pseudo stream handle */
    int		slaveClientID;	/* Host ID of client process */
{
    FsHandleHeader *hdrPtr;
    register PdevServerIOHandle *pdevHandlePtr;
    Fs_Stream *streamPtr;		/* Stream created for server process */
    Boolean found;
    PdevNotify *notifyPtr;		/* Notification message */

    ioFileIDPtr->type = FS_SERVER_STREAM;
    found = FsHandleInstall(ioFileIDPtr, sizeof(PdevServerIOHandle), &hdrPtr);
    pdevHandlePtr = (PdevServerIOHandle *)hdrPtr;
    if (found) {
	Sys_Panic(SYS_WARNING, "ServerStreamCreate, found server handle\n");
    }

    DBG_PRINT( ("ServerStreamOpen <%d,%x,%x>\n",
	    ioFileIDPtr->serverID, ioFileIDPtr->major, ioFileIDPtr->minor) );

    /*
     * Initialize the state for the pseudo stream.  Remember that
     * the request and read ahead buffers for the pseudo-stream are set up
     * via IOControls by the server process later.
     */

    pdevHandlePtr->flags = 0;
    if (slaveClientID != rpc_SpriteID) {
	pdevHandlePtr->flags |= PDEV_REMOTE_CLIENT;
    }
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

    /*
     * Create a stream for the server process and notify it of its new
     * client by generating a control message containing the streamID.
     * The server's control read will convert the streamPtr to a streamID
     * suitable for the server process.
     */

    streamPtr = FsStreamNew(rpc_SpriteID, (FsHandleHeader *)pdevHandlePtr,
			    FS_READ|FS_USER);
    notifyPtr = Mem_New(PdevNotify);
    notifyPtr->streamPtr = streamPtr;
    List_InitElement((List_Links *)notifyPtr);
    List_Insert((List_Links *)notifyPtr, LIST_ATREAR(&ctrlHandlePtr->queueHdr));
    FsHandleUnlock(streamPtr);

    FsFastWaitListNotify(&ctrlHandlePtr->readWaitList);

    FsHandleUnlock(pdevHandlePtr);
    return(pdevHandlePtr);
}

/*
 *----------------------------------------------------------------------
 *
 * FsPseudoStreamClose --
 *
 *	Close a pseudo stream that's been used by a client to talk to a server.
 *	This issues a close message to the server and then tears down the
 *	state used to implement the pseudo stream connection.
 *
 * Results:
 *	SUCCESS.
 *
 * Side effects:
 *	Other than the request-response to the server, this releases the
 *	pseudo stream's reference to the handle.  This may also have
 *	to contact a remote host to clean up references there, too.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
FsPseudoStreamClose(streamPtr, clientID, flags, size, data)
    Fs_Stream		*streamPtr;	/* Client pseudo-stream to close */
    int			clientID;	/* HostID of client closing */
    int			flags;		/* IGNORED */
    int			size;		/* Should be zero */
    ClientData		data;		/* IGNORED */
{
    register PdevClientIOHandle *cltHandlePtr =
	    (PdevClientIOHandle *)streamPtr->ioHandlePtr;
    Boolean cache = FALSE;

    DBG_PRINT( ("Client closing pdev %x,%x\n", 
		cltHandlePtr->hdr.fileID.major,
		cltHandlePtr->hdr.fileID.minor) );
    /*
     * Notify the server that a client has gone away.  Then we get rid
     * of our reference to the server's handle and nuke our own.
     */
    PseudoStreamCloseInt(cltHandlePtr->pdevHandlePtr);
    FsIOClientClose(&cltHandlePtr->clientList, clientID, 0, &cache);
    FsHandleRelease(cltHandlePtr->pdevHandlePtr, FALSE);
    FsHandleRelease(cltHandlePtr, TRUE);
    FsHandleRemove(cltHandlePtr);
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * PseudoStreamCloseInt --
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

ENTRY static void
PseudoStreamCloseInt(pdevHandlePtr)
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


/*
 *----------------------------------------------------------------------
 *
 * FsServerStreamClose --
 *
 *	Clean up the state associated with a server stream.  This makes
 *	sure the client processes associated with the pseudo stream get
 *	poked, and it marks the pseudo stream's state as invalid so
 *	the client's will abort their current operations, if any.  The
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
FsServerStreamClose(streamPtr, clientID, flags, size, data)
    Fs_Stream		*streamPtr;	/* Service stream to close */
    int			clientID;	/* HostID of client closing */
    int			flags;		/* Flags from the stream being closed */
    int			size;		/* Should be zero */
    ClientData		data;		/* IGNORED */
{
    register PdevServerIOHandle *pdevHandlePtr =
	    (PdevServerIOHandle *)streamPtr->ioHandlePtr;

    DBG_PRINT( ("Server Closing pdev %x,%x\n", 
	    pdevHandlePtr->hdr.fileID.major, pdevHandlePtr->hdr.fileID.minor) );

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
 * FsControlSelect --
 *
 *	Select on the server's control stream.  This returns readable
 *	if there are control messages in the queue.
 *
 * Results:
 *	SUCCESS.
 *
 * Side effects:
 *	Puts the caller on the handle's read wait list if the control
 *	stream isn't selectable.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
FsControlSelect(hdrPtr, waitPtr, readPtr, writePtr, exceptPtr)
    FsHandleHeader	*hdrPtr;	/* Handle on device to select */
    Sync_RemoteWaiter	*waitPtr;	/* Process info for remote waiting */
    int 		*readPtr;	/* Bit to clear if non-readable */
    int 		*writePtr;	/* Bit to clear if non-writeable */
    int 		*exceptPtr;	/* Bit to clear if non-exceptable */
{
    register PdevControlIOHandle *ctrlHandlePtr = (PdevControlIOHandle *)hdrPtr;

    FsHandleLock(ctrlHandlePtr);
    if (List_IsEmpty(&ctrlHandlePtr->queueHdr)) {
	FsFastWaitListInsert(&ctrlHandlePtr->readWaitList, waitPtr);
	*readPtr = 0;
    }
    *writePtr = *exceptPtr = 0;
    FsHandleUnlock(ctrlHandlePtr);
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * FsControlRead --
 *
 *	Read from the server's control stream.  The server learns of new
 *	clients by reading this stream.  Internally the stream is a list
 *	of addresses of streams created for the server.  This routine maps
 *	those addresses to streamIDs for the user level server process and
 *	returns them to the reader.
 *
 * Results:
 *	SUCCESS, FS_WOULD_BLOCK,
 *	or an error code from setting up a new stream ID.
 *
 * Side effects:
 *	The server's list of stream ptrs in the process table is updated.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
FsControlRead(streamPtr, flags, buffer, offsetPtr, lenPtr, waitPtr)
    Fs_Stream 	*streamPtr;	/* Control stream */
    int		flags;		/* FS_USER is checked */
    Address 	buffer;		/* Where to read into. */
    int		*offsetPtr;	/* IGNORED */
    int 	*lenPtr;	/* In/Out length parameter */
    Sync_RemoteWaiter *waitPtr;	/* Info for waiting */
{
    ReturnStatus 		status = SUCCESS;
    register PdevControlIOHandle *ctrlHandlePtr =
	    (PdevControlIOHandle *)streamPtr->ioHandlePtr;
    Pdev_Notify			notify;		/* Message returned */

    FsHandleLock(ctrlHandlePtr);

    if (List_IsEmpty(&ctrlHandlePtr->queueHdr)) {
	/*
	 * No control messages ready.
	 */
	FsFastWaitListInsert(&ctrlHandlePtr->readWaitList, waitPtr);
	*lenPtr = 0;
	status = FS_WOULD_BLOCK;
    } else {
	register PdevNotify *notifyPtr;
	notifyPtr = (PdevNotify *)List_First(&ctrlHandlePtr->queueHdr);
	List_Remove((List_Links *)notifyPtr);
	notify.magic = PDEV_NOTIFY_MAGIC;
	status = FsGetStreamID(notifyPtr->streamPtr, &notify.newStreamID);
	if (status != SUCCESS) {
	    *lenPtr = 0;
	} else {
	    if (flags & FS_USER) {
		status = Vm_CopyOut(sizeof(notify), (Address) &notify, buffer);
		/*
		 * No need to close on error because the stream is already
		 * installed in the server process's state.  It'll be
		 * closed automatically when the server exits.
		 */
	    } else {
		Byte_Copy(sizeof(notify), (Address)&notify, buffer);
	    }
	    *lenPtr = sizeof(notify);
	}
	Mem_Free((Address)notifyPtr);
    }
    FsHandleUnlock(ctrlHandlePtr);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsControlIOControl --
 *
 *	IOControls for the control stream.
 *
 * Results:
 *	An error code.
 *
 * Side effects:
 *	Command dependent.
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
ReturnStatus
FsControlIOControl(hdrPtr, command, inBufSize, inBuffer, outBufSize, outBuffer)
    FsHandleHeader *hdrPtr;		/* File handle */
    int command;			/* File specific I/O control */
    int inBufSize;			/* Size of inBuffer */
    Address inBuffer;			/* Buffer of input arguments */
    int outBufSize;			/* Size of outBuffer */
    Address outBuffer;			/* Buffer for return parameters */

{
    register PdevControlIOHandle *ctrlHandlePtr = (PdevControlIOHandle *)hdrPtr;
    register ReturnStatus status;

    switch(command) {
	case IOC_REPOSITION:
	    status = SUCCESS;
	    break;
	case IOC_GET_FLAGS:
	    if ((outBufSize >= sizeof(int)) && (outBuffer != (Address)NIL)) {
		*(int *)outBuffer = 0;
	    }
	    status = SUCCESS;
	    break;
	case IOC_SET_FLAGS:
	case IOC_SET_BITS:
	case IOC_CLEAR_BITS:
	    status = SUCCESS;
	    break;
	case IOC_TRUNCATE: {
	    status = SUCCESS;
	    break;
	}
	case IOC_LOCK:
	case IOC_UNLOCK:
	    if (inBufSize < sizeof(Ioc_LockArgs)) {
		status = GEN_INVALID_ARG;
	    } else if (command == IOC_LOCK) {
		status = FsFileLock(&ctrlHandlePtr->lock,
			    (Ioc_LockArgs *)inBuffer);
	    } else {
		status = FsFileUnlock(&ctrlHandlePtr->lock,
			    (Ioc_LockArgs *)inBuffer);
	    }
	    break;
	case IOC_NUM_READABLE: {
	    register int bytesAvailable;

	    if (outBufSize < sizeof(int)) {
		return(GEN_INVALID_ARG);
	    }
	    FsHandleLock(ctrlHandlePtr);
	    if (List_IsEmpty(&ctrlHandlePtr->queueHdr)) {
		bytesAvailable = 0;
	    } else {
		bytesAvailable = sizeof(Pdev_Notify);
	    }
	    FsHandleUnlock(ctrlHandlePtr);
	    status = SUCCESS;
	    *(int *)outBuffer = bytesAvailable;
	    break;
	}
	case IOC_SET_OWNER:
	case IOC_GET_OWNER:
	case IOC_MAP:
	    status = GEN_NOT_IMPLEMENTED;
	    break;
	default:
	    status = GEN_INVALID_ARG;
	    break;
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsControlMigStart --
 *
 *	It's too painful to migrate the pseudo-device server.
 *
 * Results:
 *	GEN_NOT_IMPLEMENTED.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
FsControlMigStart(hdrPtr, flags, clientID, data)
    FsHandleHeader *hdrPtr;	/* File being encapsulated */
    int flags;			/* Use flags from the stream */
    int clientID;		/* Host doing the encapsulation */
    ClientData data;		/* Buffer we fill in */
{
    return(GEN_NOT_IMPLEMENTED);
}

/*
 *----------------------------------------------------------------------
 *
 * FsControlMigEnd --
 *
 *	Reverse the process of encapsulating a control stream
 *
 * Results:
 *	GEN_NOT_IMPLEMENTED.
 *
 * Side effects:
 *	<explain>.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
FsControlMigEnd(migInfoPtr, size, data, hdrPtrPtr)
    FsMigInfo	*migInfoPtr;	/* Migration state */
    int		size;		/* Zero */
    ClientData	data;		/* NIL */
    FsHandleHeader **hdrPtrPtr;	/* Return - handle for the file */
{
    return(FS_REMOTE_OP_INVALID);
}

/*
 *----------------------------------------------------------------------
 *
 * FsControlVerify --
 *
 *	Verify that the remote server is known for the pseudo-device,
 *	and return a locked pointer to the control I/O handle.
 *
 * Results:
 *	A pointer to the control I/O handle, or NIL if the server is bad.
 *
 * Side effects:
 *	The handle is returned locked and with its refCount incremented.
 *	It should be released with FsHandleRelease.
 *
 *----------------------------------------------------------------------
 */

FsHandleHeader *
FsControlVerify(fileIDPtr, pdevServerHostID)
    FsFileID	*fileIDPtr;		/* control I/O file ID */
    int		pdevServerHostID;	/* Host ID of the client */
{
    register PdevControlIOHandle	*ctrlHandlePtr;
    int serverID = -1;

    ctrlHandlePtr = FsHandleFetchType(PdevControlIOHandle, fileIDPtr);
    if (ctrlHandlePtr != (PdevControlIOHandle *)NIL) {
	if (ctrlHandlePtr->serverID != pdevServerHostID) {
	    serverID = ctrlHandlePtr->serverID;
	    FsHandleRelease(ctrlHandlePtr, TRUE);
	    ctrlHandlePtr = (PdevControlIOHandle *)NIL;
	}
    }
    if (ctrlHandlePtr == (PdevControlIOHandle *)NIL) {
	Sys_Panic(SYS_WARNING,
	    "FsControlVerify, server mismatch (%d not %d) for pdev <%x,%x>\n",
	    pdevServerHostID, serverID, fileIDPtr->major, fileIDPtr->minor);
    }
    return((FsHandleHeader *)ctrlHandlePtr);
}

/*
 *----------------------------------------------------------------------
 *
 * FsControlReopen --
 *
 *	Reopen a control stream.  A control handle is kept on both the
 *	file server as well as the pseudo-device server's host.  If the
 *	file server reboots a reopen has to be done in order to set
 *	the serverID field on the file server so subsequent client opens work.
 *	Thus this is called on a remote client to contact the file server,
 *	and then on the file server from the RPC stub.
 *
 * Results:
 *	SUCCESS if there is no conflict with the server reopening.
 *
 * Side effects:
 *	On the file server the serverID field is set.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
FsControlReopen(hdrPtr, clientID, inData, outSizePtr, outDataPtr)
    FsHandleHeader	*hdrPtr;
    int			clientID;		/* ID of pdev server's host */
    ClientData		inData;			/* PdevControlReopenParams */
    int			*outSizePtr;		/* IGNORED */
    ClientData		*outDataPtr;		/* IGNORED */

{
    register PdevControlIOHandle *ctrlHandlePtr;
    register PdevControlReopenParams *reopenParamsPtr;
    register ReturnStatus status = SUCCESS;

    if (hdrPtr != (FsHandleHeader *)NIL) {
	/*
	 * Called on the pdev server's host to contact the remote
	 * file server and re-establish state.
	 */
	PdevControlIOHandle *ctrlHandlePtr;
	PdevControlReopenParams params;
	int outSize = 0;

	ctrlHandlePtr = (PdevControlIOHandle *)hdrPtr;
	reopenParamsPtr = &params;
	reopenParamsPtr->fileID = hdrPtr->fileID;
	reopenParamsPtr->serverID = ctrlHandlePtr->serverID;
	reopenParamsPtr->seed = ctrlHandlePtr->seed;
	status = FsSpriteReopen(hdrPtr, sizeof(PdevControlReopenParams),
		(Address)reopenParamsPtr, &outSize, (Address)NIL);
    } else {
	/*
	 * Called on the file server to re-establish a control handle
	 * that corresponds to a control handle on the pdev server's host.
	 */
	Boolean found;

	reopenParamsPtr = (PdevControlReopenParams *)inData;
	ctrlHandlePtr = FsControlHandleInit(&reopenParamsPtr->fileID, &found);
	if (reopenParamsPtr->serverID != NIL) {
	    /*
	     * The remote host thinks it is running the pdev server process.
	     */
	    if (!found || ctrlHandlePtr->serverID == NIL) {
		ctrlHandlePtr->serverID = reopenParamsPtr->serverID;
		ctrlHandlePtr->seed = reopenParamsPtr->seed;
	    } else if (ctrlHandlePtr->serverID != clientID) {
		Sys_Panic(SYS_WARNING,
		    "PdevControlReopen conflict, %d lost to %d, pdev <%x,%x>\n",
		    clientID, ctrlHandlePtr->serverID,
		    ctrlHandlePtr->rmt.hdr.fileID.major,
		    ctrlHandlePtr->rmt.hdr.fileID.minor);
		status = FS_FILE_BUSY;
	    }
	} else if (ctrlHandlePtr->serverID == clientID) {
	    /*
	     * The pdev server closed while we were down or unable
	     * to communicate.
	     */
	    ctrlHandlePtr->serverID = NIL;
	}
	FsHandleRelease(ctrlHandlePtr, TRUE);
     }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsControlClose --
 *
 *	Close a server process's control stream.  After this the pseudo-device
 *	is no longer active and client operations will fail.
 *
 * Results:
 *	SUCCESS.
 *
 * Side effects:
 *	Reset the control handle's serverID.
 *	Clears out the state for the control message queue.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
FsControlClose(streamPtr, clientID, flags, size, data)
    Fs_Stream		*streamPtr;	/* Control stream */
    int			clientID;	/* HostID of client closing */
    int			flags;		/* Flags from the stream being closed */
    int			size;		/* Should be zero */
    ClientData		data;		/* IGNORED */
{
    register PdevControlIOHandle *ctrlHandlePtr =
	    (PdevControlIOHandle *)streamPtr->ioHandlePtr;
    register PdevNotify *notifyPtr;
    int extra = 0;

    /*
     * Close any server streams that haven't been given to
     * the master process yet.
     */
    while (!List_IsEmpty(&ctrlHandlePtr->queueHdr)) {
	notifyPtr = (PdevNotify *)List_First(&ctrlHandlePtr->queueHdr);
	List_Remove((List_Links *)notifyPtr);
	extra++;
	Fs_Close(notifyPtr->streamPtr);
	Mem_Free((Address)notifyPtr);
    }
    if (extra) {
	Sys_Panic(SYS_WARNING, "FsControlClose found %d left over messages\n",
			extra);
    }
    /*
     * Reset the pseudo-device server ID, both here and at the name server.
     */
    ctrlHandlePtr->serverID = NIL;
    if (ctrlHandlePtr->rmt.hdr.fileID.serverID != rpc_SpriteID) {
	(void)FsSpriteClose(streamPtr, rpc_SpriteID, 0, 0, (ClientData)NIL);
    }
    FsHandleRelease(ctrlHandlePtr, TRUE);
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * FsControlClientKill --
 *
 *	See if a crashed client was running a pseudo-device master.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Clears the serverID field if it matches the crashed host's ID.
 *	This unlocks the handle before returning.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
void
FsControlClientKill(hdrPtr, clientID)
    FsHandleHeader *hdrPtr;	/* File being encapsulated */
{
    register PdevControlIOHandle *ctrlHandlePtr = (PdevControlIOHandle *)hdrPtr;

    if (ctrlHandlePtr->serverID == clientID) {
	ctrlHandlePtr->serverID = NIL;
	FsHandleRemove(ctrlHandlePtr);
    } else {
        FsHandleUnlock(ctrlHandlePtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * FsControlScavenge --
 *
 *	See if this control stream handle is still needed.
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
FsControlScavenge(hdrPtr)
    FsHandleHeader *hdrPtr;	/* File being encapsulated */
{
    register PdevControlIOHandle *ctrlHandlePtr = (PdevControlIOHandle *)hdrPtr;

    if (ctrlHandlePtr->serverID == NIL) {
	FsHandleRemove(ctrlHandlePtr);
    } else {
        FsHandleUnlock(ctrlHandlePtr);
    }
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
    register Pdev_Request	*requestPtr;	/* The caller should fill in the
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
    register ReturnStatus	status;
    Proc_ControlBlock		*serverProcPtr;	/* For VM copy operations */
    register int		firstByte;	/* Offset into request buffer */
    register int		lastByte;	/* Offset into request buffer */
    int				room;		/* Room available in req. buf.*/
    int				savedLastByte;	/* For error recovery */
    int				savedFirstByte;	/*   ditto */

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
	    serverProcPtr, (Address)&pdevHandlePtr->requestBuf.data[firstByte]);
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
	 * If this operation needs a reply we wait for it.  We save the client's
	 * reply buffer address and processID in the stream state so the
	 * kernel can copy the reply directly from the server's address space
	 * to the client's when the server makes the IOC_PDEV_REPLY IOControl.
	 */

	pdevHandlePtr->replyBuf = replyBuf;
	pdevHandlePtr->clientPID = (Proc_GetEffectiveProc())->processID;
	if (waitPtr != (Sync_RemoteWaiter *)NIL) {
	    pdevHandlePtr->clientWait = *waitPtr;
	}
	pdevHandlePtr->flags &= ~PDEV_REPLY_READY;
	while ((pdevHandlePtr->flags & PDEV_REPLY_READY) == 0) {
	    Sync_Wait(&pdevHandlePtr->replyReady, FALSE);
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
 * PseudoStreamOpen --
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
PseudoStreamOpen(pdevHandlePtr, flags, clientID, procID, userID)
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
	replySize = (pdevHandlePtr->flags&PDEV_WRITE_BEHIND) ? -1 : sizeof(int);
	status = RequestResponse(pdevHandlePtr, &request, length, buffer,
			    replySize, (Address)&numBytes, &replySize, waitPtr);
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
FsPseudoStreamIOControl(hdrPtr, command, inBufSize, inBuffer,
		       outBufSize, outBuffer)
    FsHandleHeader *hdrPtr;	/* Handle header for pseudo-stream. */
    int		command;	/* The control operation to be performed. */
    int		inBufSize;	/* Size of input buffer. */
    Address	inBuffer;	/* Data to be sent to the slave/master. */
    int		outBufSize;	/* Size of output buffer. */
    Address	outBuffer;	/* Data to be obtained from the slave/master.*/
{
    ReturnStatus 	status;
    Pdev_Request	request;
    register Proc_ControlBlock *procPtr;
    register PdevClientIOHandle *cltHandlePtr = (PdevClientIOHandle *)hdrPtr;
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
	(pdevHandlePtr->flags & PDEV_REMOTE_CLIENT) == 0) {
	pdevHandlePtr->flags |= FS_USER;

    }

    switch (command) {
	/*
	 * Trap out the IOC_NUM_READABLE here if there's a read ahead buf.
	 */
	case IOC_NUM_READABLE: {
	    if (pdevHandlePtr->readBuf.data != (Address)NIL) {
		register int bytesAvail;
		if (outBuffer == (Address)NIL ||
		    outBufSize < sizeof(int)) {
		    status = GEN_INVALID_ARG;
		    goto exit;
		} else if (pdevHandlePtr->flags & PDEV_READ_BUF_EMPTY) {
		    bytesAvail = 0;
		} else {
		    bytesAvail = pdevHandlePtr->readBuf.lastByte -
				 pdevHandlePtr->readBuf.firstByte + 1;
		}
		*(int *)outBuffer = bytesAvail;
		status = SUCCESS;
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

    status = RequestResponse(pdevHandlePtr, &request, inBufSize, inBuffer,
		outBufSize, outBuffer, (int *) NIL, (Sync_RemoteWaiter *)NIL);
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

ENTRY ReturnStatus
FsServerStreamIOControl(hdrPtr, command, inBufSize, inBuffer,
		       outBufSize, outBuffer)
    FsHandleHeader 	*hdrPtr;	/* Server IO Handle. */
    int		command;	/* The control operation to be performed. */
    int		inBufSize;	/* Size of input buffer. */
    Address	inBuffer;	/* Data to be sent to the slave/master. */
    int		outBufSize;	/* Size of output buffer. */
    Address	outBuffer;	/* Data to be obtained from the slave/master.*/
{
    ReturnStatus	status = SUCCESS;
    register PdevServerIOHandle	*pdevHandlePtr = (PdevServerIOHandle *) hdrPtr;

    LOCK_MONITOR;

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
	    register Pdev_SetBufArgs *argPtr = (Pdev_SetBufArgs *)inBuffer;
	    register Proc_ControlBlock *procPtr;
	    if (inBufSize != sizeof(Pdev_SetBufArgs)) {
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
	    if (inBufSize < sizeof(Boolean)) {
		status = GEN_INVALID_ARG;
	    } else {
		writeBehind = *(Boolean *)inBuffer;
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
	    if (inBufSize < sizeof(Boolean)) {
		status = GEN_INVALID_ARG;
	    } else {
		allowLargeWrites = *(Boolean *)inBuffer;
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
	    register Pdev_BufPtrs *argPtr = (Pdev_BufPtrs *)inBuffer;
	    if (inBufSize != sizeof(Pdev_BufPtrs)) {
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
		if (argPtr->requestFirstByte <= pdevHandlePtr->requestBuf.size &&
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
	    register Pdev_Reply *srvReplyPtr = (Pdev_Reply *)inBuffer;

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

	    if (inBufSize != sizeof(int)) {
		status = FS_INVALID_ARG;
	    } else {
		/*
		 * Update the select state of the pseudo-device and
		 * wake up any clients waiting on their pseudo-stream.
		 */
		pdevHandlePtr->selectBits = *(int *)inBuffer;
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
	    if (outBuffer == (Address)NIL || outBufSize < sizeof(int)) {
		status = GEN_INVALID_ARG;
	    } else {
		*(int *)outBuffer = numReadable;
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
 * FsServerStreamMigStart --
 *
 *	It's too painful to migrate the pseudo-device server.
 *
 * Results:
 *	GEN_NOT_IMPLEMENTED.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
FsServerStreamMigStart(hdrPtr, flags, clientID, data)
    FsHandleHeader *hdrPtr;	/* File being encapsulated */
    int flags;			/* Use flags from the stream */
    int clientID;		/* Host doing the encapsulation */
    ClientData data;		/* Buffer we fill in */
{
    return(GEN_NOT_IMPLEMENTED);
}

/*
 *----------------------------------------------------------------------
 *
 * FsServerStreamMigEnd --
 *
 *	Complete migration for a server stream
 *
 * Results:
 *	GEN_NOT_IMPLEMENTED.
 *
 * Side effects:
 *	The streams that compose the Server stream are also deencapsulated.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
FsServerStreamMigEnd(migInfoPtr, size, data, hdrPtrPtr)
    FsMigInfo	*migInfoPtr;	/* Migration state */
    int		size;		/* Zero */
    ClientData	data;		/* NIL */
    FsHandleHeader **hdrPtrPtr;	/* Return - handle for the file */
{
    return(GEN_NOT_IMPLEMENTED);
}

/*
 *----------------------------------------------------------------------
 *
 * FsPseudoStreamMigStart --
 *
 *	Begin migration of a pseuod-stream client.
 *
 * Results:
 *	GEN_NOT_IMPLEMENTED.
 *
 * Side effects:
 *	<explain after implementing>.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
FsPseudoStreamMigStart(hdrPtr, flags, clientID, data)
    FsHandleHeader *hdrPtr;	/* File being encapsulated */
    int flags;			/* Use flags from the stream */
    int clientID;		/* Host doing the encapsulation */
    ClientData data;		/* Buffer we fill in */
{
    return(GEN_NOT_IMPLEMENTED);
}

/*
 *----------------------------------------------------------------------
 *
 * FsPseudoStreamMigrate --
 *
 *	Migrate a pseudo-stream client.
 *
 * Results:
 *	GEN_NOT_IMPLEMENTED.
 *
 * Side effects:
 *	<explain after implementing>.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
FsPseudoStreamMigrate(migInfoPtr, size, data, hdrPtrPtr)
    FsMigInfo	*migInfoPtr;	/* Migration state */
    int		size;		/* Zero */
    ClientData	data;		/* NIL */
    FsHandleHeader **hdrPtrPtr;	/* Return - handle for the file */
{
    return(GEN_NOT_IMPLEMENTED);
}

/*
 *----------------------------------------------------------------------
 *
 * FsRmtPseudoStreamMigrate --
 *
 *	Migrate a pseudo-stream client.
 *
 * Results:
 *	GEN_NOT_IMPLEMENTED.
 *
 * Side effects:
 *	<explain after implementing>.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
FsRmtPseudoStreamMigrate(migInfoPtr, size, data, hdrPtrPtr)
    FsMigInfo	*migInfoPtr;	/* Migration state */
    int		size;		/* Zero */
    ClientData	data;		/* NIL */
    FsHandleHeader **hdrPtrPtr;	/* Return - handle for the file */
{
    return(GEN_NOT_IMPLEMENTED);
}

/*
 *----------------------------------------------------------------------
 *
 * FsPseudoStreamMigrate --
 *
 *	Complete migration for a pseudo stream
 *
 * Results:
 *	GEN_NOT_IMPLEMENTED.
 *
 * Side effects:
 *	<explain after implementing>.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
FsPseudoStreamMigEnd(migInfoPtr, size, data, hdrPtrPtr)
    FsMigInfo	*migInfoPtr;	/* Migration state */
    int		size;		/* Zero */
    ClientData	data;		/* NIL */
    FsHandleHeader **hdrPtrPtr;	/* Return - handle for the file */
{
    return(GEN_NOT_IMPLEMENTED);
}

/*
 *----------------------------------------------------------------------
 *
 * FsRmtPseudoStreamVerify --
 *
 *	Verify that the remote client is known for the pdev, and return
 *	a locked pointer to the client I/O handle.
 *
 * Results:
 *	A pointer to the client I/O handle, or NIL if
 *	the client is bad.
 *
 * Side effects:
 *	The handle is returned locked and with its refCount incremented.
 *	It should be released with FsHandleRelease.
 *
 *----------------------------------------------------------------------
 */

FsHandleHeader *
FsRmtPseudoStreamVerify(fileIDPtr, clientID)
    FsFileID	*fileIDPtr;	/* Client's I/O file ID */
    int		clientID;	/* Host ID of the client */
{
    register PdevClientIOHandle	*cltHandlePtr;
    register FsClientInfo	*clientPtr;
    Boolean			found = FALSE;

    fileIDPtr->type = FS_LCL_PSEUDO_STREAM;
    cltHandlePtr = FsHandleFetchType(PdevClientIOHandle, fileIDPtr);
    if (cltHandlePtr != (PdevClientIOHandle *)NIL) {
	LIST_FORALL(&cltHandlePtr->clientList, (List_Links *) clientPtr) {
	    if (clientPtr->clientID == clientID) {
		found = TRUE;
		break;
	    }
	}
	if (!found) {
	    FsHandleRelease(cltHandlePtr, TRUE);
	    cltHandlePtr = (PdevClientIOHandle *)NIL;
	}
    }
    if (!found) {
	Sys_Panic(SYS_WARNING,
	    "FsRmtPseudoDeviceVerify, client %d not known for pdev <%x,%x>\n",
	    clientID, fileIDPtr->major, fileIDPtr->minor);
    }
    return((FsHandleHeader *)cltHandlePtr);
}

/*
 *----------------------------------------------------------------------------
 *
 * FsPdevTraceInit --
 *
 *	Initialize the pseudo-device trace buffer.  Used for debuggin
 *	and profiling.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Calls to the Trace module to allocate the trace buffer, etc.
 *
 *----------------------------------------------------------------------------
 *
 */
ReturnStatus
FsPdevTraceInit()
{
    Trace_Init(pdevTraceHdrPtr, pdevTraceLength, sizeof(PdevTraceRecord),
		TRACE_NO_TIMES);
}

/*
 *----------------------------------------------------------------------------
 *
 * Fs_PdevPrintRec --
 *
 *	Print a record of the pseudo-device trace buffer.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sys_Printf's
 *
 *----------------------------------------------------------------------------
 *
 */
ReturnStatus
Fs_PdevPrintRec(clientData, event, printHeaderFlag)
    ClientData clientData;	/* Client data in the trace record */
    int event;			/* Type, or event, from the trace record */
    Boolean printHeaderFlag;	/* If TRUE, a header line is printed */
{
    PdevTraceRecord *recPtr = (PdevTraceRecord *)clientData;
    PdevTraceRecType pdevEvent = (PdevTraceRecType)event;
    if (printHeaderFlag) {
	/*
	 * Print column headers and a newline.
	 */
	Sys_Printf("%6s %17s %8s\n", "REC", "  <File ID>  ", " Event ");
    }
    if (recPtr != (PdevTraceRecord *)NIL) {
	/*
	 * Print out the fileID that's part of each record.
	 */
	Sys_Printf("%5d| ", recPtr->index);
	Sys_Printf("<%8x,%8x> ",
	  recPtr->fileID.major, recPtr->fileID.minor);

	switch(pdevEvent) {
	    case PDEVT_SRV_OPEN:
		Sys_Printf("Srv Open");
		Sys_Printf(" refs %d writes %d",
			    recPtr->un.use.ref,
			    recPtr->un.use.write);
		break;
	    case PDEVT_CLT_OPEN:
		Sys_Printf("Clt Open");
		Sys_Printf(" refs %d writes %d",
			    recPtr->un.use.ref,
			    recPtr->un.use.write);
		 break;
	    case PDEVT_SRV_CLOSE:
		Sys_Printf("Srv Close");
		Sys_Printf(" refs %d writes %d",
			    recPtr->un.use.ref,
			    recPtr->un.use.write);
		 break;
	    case PDEVT_CLT_CLOSE:
		Sys_Printf("Clt Close");
		Sys_Printf(" refs %d writes %d",
			    recPtr->un.use.ref,
			    recPtr->un.use.write);
		 break;
	    case PDEVT_SRV_READ:
		Sys_Printf("Srv Read"); break;
	    case PDEVT_SRV_READ_WAIT:
		Sys_Printf("Srv Read Blocked"); break;
	    case PDEVT_SRV_SELECT:
		Sys_Printf("Srv Select Wait"); break;
	    case PDEVT_SRV_WRITE:
		Sys_Printf("Srv Write"); break;
	    case PDEVT_CNTL_READ:
		Sys_Printf("Control Read"); break;
	    case PDEVT_READ_WAIT:
		Sys_Printf("Wait for Read"); break;
	    case PDEVT_WAIT_LIST:
		Sys_Printf("Wait List Notify"); break;
	    case PDEVT_SELECT: {
		Sys_Printf("Select "); 
		if (recPtr != (PdevTraceRecord *)NIL ) {
		    if (recPtr->un.selectBits & FS_READABLE) {
			Sys_Printf("R");
		    }
		    if (recPtr->un.selectBits & FS_WRITABLE) {
			Sys_Printf("W");
		    }
		    if (recPtr->un.selectBits & FS_EXCEPTION) {
			Sys_Printf("E");
		    }
		}
		break;
	    }
	    case PDEVT_WAKEUP: {
		/*
		 * Print the process ID from the wait info,
		 * and the select bits stashed in the wait info token.
		 */
		Sys_Printf("Wakeup");
		if (recPtr != (PdevTraceRecord *)NIL ) {
		    Sys_Printf(" %x ", recPtr->un.wait.procID);
		    if (recPtr->un.wait.selectBits & FS_READABLE) {
			Sys_Printf("R");
		    }
		    if (recPtr->un.wait.selectBits & FS_WRITABLE) {
			Sys_Printf("W");
		    }
		    if (recPtr->un.wait.selectBits & FS_EXCEPTION) {
			Sys_Printf("E");
		    }
		}
		break;
	    }
	    case PDEVT_REQUEST: {
		Sys_Printf("Request");
		if (recPtr != (PdevTraceRecord *)NIL) {
		    switch(recPtr->un.request.operation) {
			case PDEV_OPEN:
			    Sys_Printf(" OPEN"); break;
			case PDEV_DUP:
			    Sys_Printf(" DUP"); break;
			case PDEV_CLOSE:
			    Sys_Printf(" CLOSE"); break;
			case PDEV_READ:
			    Sys_Printf(" READ"); break;
			case PDEV_WRITE:
			    Sys_Printf(" WRITE"); break;
			case PDEV_IOCTL:
			    Sys_Printf(" IOCTL"); break;
			default:
			    Sys_Printf(" ??"); break;
		    }
		}
		break;
	    }
	    case PDEVT_REPLY: {
		Sys_Printf("Reply");
		if (recPtr != (PdevTraceRecord *)NIL) {
		    Sys_Printf(" <%x> ", recPtr->un.reply.status);
		    if (recPtr->un.reply.selectBits & FS_READABLE) {
			Sys_Printf("R");
		    }
		    if (recPtr->un.reply.selectBits & FS_WRITABLE) {
			Sys_Printf("W");
		    }
		}
		break;
	    }
	    default:
		Sys_Printf("<%d>", event); break;

	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_PdevPrintTrace --
 *
 *	Dump out the pdev trace.
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
Fs_PdevPrintTrace(numRecs)
    int numRecs;
{
    if (numRecs < 0) {
	numRecs = pdevTraceLength;
    }
    Sys_Printf("PDEV TRACE\n");
    Trace_Print(pdevTraceHdrPtr, numRecs, Fs_PdevPrintRec);
}
