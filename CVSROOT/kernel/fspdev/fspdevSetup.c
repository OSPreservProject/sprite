/* 
 * fsPdevSetup.c --
 *
 *	Open/Close/Migration routines for pseudo-devices.
 *	
 *	There are three kinds of streams involved in the implementation,
 *	a "control" stream that is returned to the server when it first
 *	opens the pseudo-device.  When a client opens the pseudo-device
 *	two streams are created and looked together.  The "client" stream
 *	is returned to the client process, and the "server" stream is
 *	passed to the server process via the control stream.
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

#include <sprite.h>
#include <fs.h>
#include <fsutil.h>
#include <fsNameOps.h>
#include <fsio.h>
#include <fsconsist.h>
#include <fsdm.h>
#include <fsioLock.h>
#include <proc.h>
#include <rpc.h>
#include <fspdevInt.h>
#include <fspdev.h>

/*
 *----------------------------------------------------------------------------
 *
 * FspdevNameOpen --
 *
 *	Early open time processing, this is called on a fileserver
 *	when setting up state for a call to the CltOpen routines on
 *	the client host.  For pseudo-device server processes, which
 *	are indicated by the FS_PDEV_MASTER flag, check that no other
 *	server exists.  For all other processes, which are referred to
 *	as "clients", make sure that a server process exists and
 *	generate a new ioFileID for the connection between the client
 *	and the server.
 *
 * Results:
 *	For server processes, SUCCESS if it is now the server,
 *	FS_FILE_BUSY if there already exists a server process.  For
 *	clients, SUCCESS if there is a server or the parameters
 *	indicate this is only for get/set attributes, DEV_OFFLINE if
 *	there is no server. 
 *
 * Side effects:
 *	Save the hostID of the calling process if
 *	it is to be the server for the pseudo-device.
 *
 *----------------------------------------------------------------------------
 *
 */
ReturnStatus
FspdevNameOpen(handlePtr, openArgsPtr, openResultsPtr)
     register Fsio_FileIOHandle *handlePtr;	/* A handle from FslclLookup.
					 * Should be LOCKED upon entry,
					 * unlocked upon exit. */
     Fs_OpenArgs		*openArgsPtr;	/* Standard open arguments */
     Fs_OpenResults	*openResultsPtr;/* For returning ioFileID, streamID,
					 * and Fsio_DeviceState */
{
    register	ReturnStatus status = SUCCESS;
    Fs_FileID	ioFileID;
    register	Fspdev_ControlIOHandle *ctrlHandlePtr;
    register	Fs_Stream *streamPtr;
    register	Fspdev_State *pdevStatePtr;

    /*
     * The control I/O handle is identified by the fileID of the pseudo-device
     * file with type CONTROL, and with the decriptor version number
     * xor'ed into the minor number to avoid conflict when you delete the
     * pdev file and recreate one with the same file number (minor field).
     */
    ioFileID = handlePtr->hdr.fileID;
    ioFileID.type = FSIO_CONTROL_STREAM;
    ioFileID.serverID = rpc_SpriteID;
    ioFileID.major = handlePtr->hdr.fileID.major;
    ioFileID.minor = handlePtr->hdr.fileID.minor ^
		    (handlePtr->descPtr->version << 16);
    ctrlHandlePtr = FspdevControlHandleInit(&ioFileID, handlePtr->hdr.name);

    if (openArgsPtr->useFlags & FS_PDEV_MASTER) {
	/*
	 * When a server opens we ensure there is only one.
	 */
	if (ctrlHandlePtr->serverID != NIL) {
	    status = FS_FILE_BUSY;
	} else {
	    /*
	     * Note which host is running the pseudo-device server.
	     */
	    ctrlHandlePtr->serverID = openArgsPtr->clientID;
	    /*
	     * Note our hostID is still in the hdr.serverID field of the
	     * control handle being returned to the opening process. This is
	     * used when closing the control stream to get back to us
	     * so we can clear the serverID field here.  We also set up
	     * a shadow stream here, which has us as the server so
	     * recovery and closing work right.
	     */
	    openResultsPtr->ioFileID = ioFileID;
	    openResultsPtr->streamData = (ClientData)NIL;
	    openResultsPtr->dataSize = 0;
	    streamPtr = Fsio_StreamCreate(rpc_SpriteID, openArgsPtr->clientID,
				    (Fs_HandleHeader *)ctrlHandlePtr,
				    openArgsPtr->useFlags, handlePtr->hdr.name);
	    openResultsPtr->streamID = streamPtr->hdr.fileID;
	    Fsutil_HandleRelease(streamPtr, TRUE);
	}
    } else {
	if (openArgsPtr->useFlags == 0) {
	    /*
	     * Set up for get/set attributes.  We point the client
	     * at the name of the pseudo-device if it is not active,
	     * otherwise we point it at the control stream handle that
	     * has the current access and modify times.
	     */
	    if (ctrlHandlePtr->serverID == NIL) {
		openResultsPtr->ioFileID = handlePtr->hdr.fileID;
	    } else {
		openResultsPtr->ioFileID = ctrlHandlePtr->rmt.hdr.fileID;
		if (openArgsPtr->clientID != ctrlHandlePtr->serverID) {
		    /*
		     * The requesting client is different than the pdev
		     * server host.  Unfortunately the serverID in the
		     * control handle is us, the file server.  We have
		     * to hack the fileID so the client makes the RPC to
		     * the pdev server.  This relies on a parallel hack
		     * in Fsrmt_GetIOAttr to fix up the serverID by
		     * using the Fs_Attributes.serverID, which is us,
		     * so that the correct control handle is found.
		     */
		    openResultsPtr->ioFileID.type = FSIO_RMT_CONTROL_STREAM;
		    openResultsPtr->ioFileID.serverID = ctrlHandlePtr->serverID;
		}
	    }
	} else if (ctrlHandlePtr->serverID == NIL) {
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
	    if (ctrlHandlePtr->serverID == openArgsPtr->clientID) {
		openResultsPtr->ioFileID.type = FSIO_LCL_PSEUDO_STREAM;
	    } else {
		openResultsPtr->ioFileID.type = FSIO_RMT_PSEUDO_STREAM;
	    }
	    openResultsPtr->ioFileID.serverID = ctrlHandlePtr->serverID;
	    openResultsPtr->ioFileID.major =
				(handlePtr->hdr.fileID.serverID << 16) |
				 handlePtr->hdr.fileID.major;
	    ctrlHandlePtr->seed++;
	    openResultsPtr->ioFileID.minor =
				((handlePtr->descPtr->version << 24) ^
				 (handlePtr->hdr.fileID.minor << 12)) |
				 ctrlHandlePtr->seed;
	    /*
	     * Return the control stream file ID so it can be found again
	     * later when setting up the client's stream and the
	     * corresponding server stream.  The procID and uid fields are
	     * extra here, but will be used later if the client is remote.
	     */
	    pdevStatePtr = mnew(Fspdev_State);
	    pdevStatePtr->ctrlFileID = ctrlHandlePtr->rmt.hdr.fileID;
	    pdevStatePtr->procID = (Proc_PID)NIL;
	    pdevStatePtr->uid = NIL;
	    openResultsPtr->streamData = (ClientData)pdevStatePtr ;
	    openResultsPtr->dataSize = sizeof(Fspdev_State);
	    /*
	     * Create a streamID for the opening process.  No shadow
	     * stream is kept here.  Instead, the streamID is returned to
	     * the pdev server who sets up the shadow stream.
	     */
	    Fsio_StreamCreateID(ctrlHandlePtr->serverID, &openResultsPtr->streamID);
	    pdevStatePtr->streamID = openResultsPtr->streamID;
	}
    }
    Fsutil_HandleRelease(ctrlHandlePtr, TRUE);
    Fsutil_HandleUnlock(handlePtr);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FspdevPseudoStreamIoOpen --
 *
 *	This is called from Fs_Open, or from the RPC stub if the client
 *	is remote, to complete setup of a client's
 *	stream to the pseudo-device.  The server is running on this
 *	host.  This routine creates a trivial client I/O handle
 *	that references the server's I/O handle that has the main
 *	state for the connection to the server.  FspdevServerStreamCreate
 *	is then called to set up the server's I/O handle, and the control
 *	stream is used to pass a server stream to the server.  Finally
 *	an open transaction is made with the server process
 *	to see if it will accept the client.
 * 
 * Results:
 *	SUCCESS, unless the server process has died recently, or the
 *	server rejects the open.
 *
 * Side effects:
 *	Creates the client's I/O handle.  Calls FspdevServerStreamCreate
 *	which sets up the servers corresponding I/O handle.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
FspdevPseudoStreamIoOpen(ioFileIDPtr, flagsPtr, clientID, streamData, name,
	ioHandlePtrPtr)
    register Fs_FileID	*ioFileIDPtr;	/* I/O fileID */
    int			*flagsPtr;	/* FS_READ | FS_WRITE ... */
    int			clientID;	/* Host doing the open */
    ClientData		streamData;	/* Pointer to Fspdev_State. */
    char		*name;		/* File name for error msgs */
    Fs_HandleHeader	**ioHandlePtrPtr;/* Return - a locked handle set up for
					 * I/O to a pseudo device, or NIL */
{
    ReturnStatus		status;
    Boolean			foundStream;
    register Fspdev_ClientIOHandle	*cltHandlePtr;
    register Fspdev_ControlIOHandle *ctrlHandlePtr;
    register Fspdev_State	*pdevStatePtr;
    Fs_Stream			*cltStreamPtr;
    Fs_Stream			*srvStreamPtr;
    FspdevNotify			*notifyPtr;
    Proc_ControlBlock		*procPtr;
    Proc_PID 			procID;
    int				uid;

    pdevStatePtr = (Fspdev_State *)streamData;
    ctrlHandlePtr = Fsutil_HandleFetchType(Fspdev_ControlIOHandle,
				    &pdevStatePtr->ctrlFileID);
    /*
     * If there is no server present the creation of the stream
     * can't succeed.  This case arises when the pseudo-device
     * master goes away between FspdevNameOpen and this call.
     */
    if ((ctrlHandlePtr == (Fspdev_ControlIOHandle *)NIL) ||
	(ctrlHandlePtr->serverID == NIL)) {
	status = DEV_OFFLINE;
	goto exit;
    }

    if (ctrlHandlePtr->rmt.hdr.fileID.serverID != rpc_SpriteID) {
	/*
	 * Extract the seed from the minor field (see the SrvOpen routine).
	 * This done in case of recovery when we'll need to reset the
	 * seed kept on the file server.
	 */
	ctrlHandlePtr->seed = ioFileIDPtr->minor & 0x0FFF;
    }

    cltHandlePtr = FspdevConnect(ctrlHandlePtr, ioFileIDPtr, clientID, 0);
    if (cltHandlePtr == (Fspdev_ClientIOHandle *)NIL) {
	status = DEV_OFFLINE;
	goto exit;
    }
    /*
     * Put the client on its own stream list.
     */
    cltStreamPtr = Fsio_StreamAddClient(&pdevStatePtr->streamID, clientID,
		(Fs_HandleHeader *)cltHandlePtr, *flagsPtr, name,
		(Boolean *)NIL, &foundStream);
    Fsutil_HandleRelease(cltStreamPtr, TRUE);
    Fsutil_HandleUnlock(cltHandlePtr);
    /*
     * Set up a stream for the server process.  This will be picked
     * up by FspdevControlRead and converted to a user-level streamID.
     */
    srvStreamPtr = Fsio_StreamCreate(rpc_SpriteID, rpc_SpriteID,
			    (Fs_HandleHeader *)cltHandlePtr->pdevHandlePtr,
			    FS_READ|FS_USER, name);
    notifyPtr = mnew(FspdevNotify);
    notifyPtr->streamPtr = srvStreamPtr;
    List_InitElement((List_Links *)notifyPtr);
    List_Insert((List_Links *)notifyPtr,
		LIST_ATREAR(&ctrlHandlePtr->queueHdr));
    Fsutil_HandleUnlock(srvStreamPtr);

    Fsutil_FastWaitListNotify(&ctrlHandlePtr->readWaitList);
    Fsutil_HandleRelease(ctrlHandlePtr, TRUE);
    ctrlHandlePtr = (Fspdev_ControlIOHandle *)NIL;
    /*
     * Now that the request response stream is set up we do
     * our first transaction with the server process to see if it
     * will accept the open.  We unlock the handle and rely on the
     * per-connection monitor lock instead.  This is important because a
     * buggy pseudo-device server could be ignoring this connection
     * request indefinitely, and leaving handles locked for long periods
     * clogs up handle scavenging, and potentially recovery callbacks too.
     */
    if (clientID == rpc_SpriteID) {
	procPtr = Proc_GetEffectiveProc();
	procID = procPtr->processID;
	uid = procPtr->effectiveUserID;
    } else {
	procID = pdevStatePtr->procID;
	uid = pdevStatePtr->uid;
    }
    status = FspdevPseudoStreamOpen(cltHandlePtr->pdevHandlePtr, *flagsPtr,
				clientID, procID, uid);
    if (status == SUCCESS) {
	*ioHandlePtrPtr = (Fs_HandleHeader *)cltHandlePtr;
    } else {
	/*
	 * Clean up client side, we assume server closes its half.
	 */
	Fsutil_HandleInvalidate((Fs_HandleHeader *) cltHandlePtr);
	Fsutil_HandleRemove(cltHandlePtr);
	(void)Fsio_StreamClientClose(&cltStreamPtr->clientList, clientID);
	if (!foundStream) {
	    /*
	     * The client's stream wasn't already around from being installed
	     * in Fs_Open, so we nuke the shadow stream we've created.
	     */
	    cltStreamPtr = Fsutil_HandleFetchType(Fs_Stream,
					     &cltStreamPtr->hdr.fileID);
	    Fsio_StreamDestroy(cltStreamPtr);
	}
    }
exit:
    if (ctrlHandlePtr != (Fspdev_ControlIOHandle *)NIL) {
	Fsutil_HandleRelease(ctrlHandlePtr, TRUE);
    }
    free((Address)streamData);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FspdevConnect --
 *
 *	This sets up a pseduo-device connection.  This is called from
 *	the PseudoStreamCltOpen routine with ordinary pseudo-devices,
 *	and from FspdevPfsIoOpen to set up the naming connection to a
 *	pseudo-filesystem server, and during an IOC_PFS_OPEN by a
 *	pseudo-filesystem server to set up a connection to a client.
 *	The case of the naming stream is distinguished by the last
 *	parameter.  The state of this needs to be marked specially
 *	so proper clean up can be made later.
 * 
 * Results:
 *	A pointer to a Fspdev_ClientIOHandle that references a Fspdev_ServerIOHandle.
 *	The client handle is returned locked, but the server handle it
 *	references is not locked.
 *
 * Side effects:
 *	Creates the client's I/O handle.  Calls FspdevServerStreamCreate
 *	which sets up the servers corresponding I/O handle.
 *	This changes the ioFileIDPtr->type from FSIO_LCL_PSEUDO_STREAM to
 *	FSIO_SERVER_STREAM.
 *
 *----------------------------------------------------------------------
 */

Fspdev_ClientIOHandle *
FspdevConnect(ctrlHandlePtr, ioFileIDPtr, clientID, naming)
    Fspdev_ControlIOHandle *ctrlHandlePtr;	/* Control stream handle */
    register Fs_FileID	*ioFileIDPtr;	/* I/O fileID */
    int			clientID;	/* Host ID of client-side */
    Boolean		naming;		/* TRUE if called from FspdevPfsIoOpen
					 * to set up the naming stream */
{
    Boolean			found;
    Fs_HandleHeader		*hdrPtr;
    register Fspdev_ClientIOHandle	*cltHandlePtr;

    found = Fsutil_HandleInstall(ioFileIDPtr, sizeof(Fspdev_ClientIOHandle),
		ctrlHandlePtr->rmt.hdr.name, FALSE, &hdrPtr);
    cltHandlePtr = (Fspdev_ClientIOHandle *)hdrPtr;
    if (found) {
	if ((cltHandlePtr->pdevHandlePtr != (Fspdev_ServerIOHandle *)NIL) &&
	    (cltHandlePtr->pdevHandlePtr->clientPID != (unsigned int)NIL)) {
	    printf(
		"FspdevConnect found client handle\n");
	    printf("Check (and kill) client process %x\n",
		cltHandlePtr->pdevHandlePtr->clientPID);
	}
	/*
	 * Invalidate this lingering handle.  The client process is hung
	 * or suspended and hasn't closed its end of the pdev connection.
	 */
	Fsutil_HandleInvalidate((Fs_HandleHeader *)cltHandlePtr);
	Fsutil_HandleRelease(cltHandlePtr, TRUE);

	found = Fsutil_HandleInstall(ioFileIDPtr, sizeof(Fspdev_ClientIOHandle),
			ctrlHandlePtr->rmt.hdr.name, FALSE, &hdrPtr);
	cltHandlePtr = (Fspdev_ClientIOHandle *)hdrPtr;
	if (found) {
	    panic( "FspdevConnect handle still there\n");
	}
    }
    /*
     * Set up the connection state and hook the client handle to it.
     */
    cltHandlePtr->pdevHandlePtr = FspdevServerStreamCreate(ioFileIDPtr,
				    ctrlHandlePtr->rmt.hdr.name, naming);
    if (cltHandlePtr->pdevHandlePtr == (Fspdev_ServerIOHandle *)NIL) {
	Fsutil_HandleRemove(cltHandlePtr);
	return((Fspdev_ClientIOHandle *)NIL);
    }
    cltHandlePtr->pdevHandlePtr->ctrlHandlePtr = ctrlHandlePtr;
    cltHandlePtr->segPtr = (struct Vm_Segment *)NIL; /* JMS */
    /*
     * Set up the client list in case the client is remote.
     */
    List_Init(&cltHandlePtr->clientList);
    (void)Fsconsist_IOClientOpen(&cltHandlePtr->clientList, clientID, 0, FALSE);
    /*
     * Grab an extra reference to the server's handle so the
     * server close routine can remove the handle and it won't
     * go away until the client also closes.
     */
    Fsutil_HandleUnlock(cltHandlePtr->pdevHandlePtr);
    (void)Fsutil_HandleDup((Fs_HandleHeader *)cltHandlePtr->pdevHandlePtr);
    Fsutil_HandleUnlock(cltHandlePtr->pdevHandlePtr);

    return(cltHandlePtr);
}

/*
 *----------------------------------------------------------------------
 *
 * FspdevRmtPseudoStreamIoOpen --
 *
 *	Complete a remote client's stream to a pseudo-device.
 *	The client is on a different host than the server process.  This
 *	makes an RPC to the pseudo-device server's host to invoke
 *	FspdevPseudoStreamIoOpen, which sets up the pdev connection.
 *	This host only keeps a Fsrmt_IOHandle that implicitly references
 *	the pdev connection on the pdev server's host.
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
FspdevRmtPseudoStreamIoOpen(ioFileIDPtr, flagsPtr, clientID, streamData, name,
	ioHandlePtrPtr)
    register Fs_FileID	*ioFileIDPtr;	/* I/O fileID */
    int			*flagsPtr;	/* FS_READ | FS_WRITE ... */
    int			clientID;	/* IGNORED (== rpc_SpriteID) */
    ClientData		streamData;	/* NIL for us. */
    char		*name;		/* File name for error msgs */
    Fs_HandleHeader	**ioHandlePtrPtr;/* Return - a locked handle set up for
					 * I/O to a pseudo device, or NIL */
{
    register ReturnStatus status;
    register Proc_ControlBlock *procPtr;
    register Fspdev_State *pdevStatePtr = (Fspdev_State *)streamData;

    /*
     * Use RPC to invoke FspdevPseudoStreamIoOpen which sets up the connection.
     */
    procPtr = Proc_GetEffectiveProc();
    pdevStatePtr->procID = procPtr->processID;
    pdevStatePtr->uid = procPtr->effectiveUserID;
    ioFileIDPtr->type = FSIO_LCL_PSEUDO_STREAM;
    status = Fsrmt_DeviceOpen(ioFileIDPtr, *flagsPtr,	sizeof(Fspdev_State),
				(ClientData)pdevStatePtr);
    if (status == SUCCESS) {
	ioFileIDPtr->type = FSIO_RMT_PSEUDO_STREAM;
	Fsrmt_IOHandleInit(ioFileIDPtr, *flagsPtr, name, ioHandlePtrPtr);
    }
    free((Address)pdevStatePtr);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FspdevPseudoStreamClose --
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
FspdevPseudoStreamClose(streamPtr, clientID, procID, flags, size, data)
    Fs_Stream		*streamPtr;	/* Client pseudo-stream to close */
    int			clientID;	/* HostID of client closing */
    Proc_PID		procID;		/* ID of closing process, IGNORED */
    int			flags;		/* IGNORED */
    int			size;		/* Should be zero */
    ClientData		data;		/* IGNORED */
{
    register Fspdev_ClientIOHandle *cltHandlePtr =
	    (Fspdev_ClientIOHandle *)streamPtr->ioHandlePtr;
    Boolean cache = FALSE;
    Vm_Segment *segPtr = cltHandlePtr->segPtr;

    DBG_PRINT( ("Client closing pdev %x,%x\n", 
		cltHandlePtr->hdr.fileID.major,
		cltHandlePtr->hdr.fileID.minor) );

    if (!Fsconsist_IOClientClose(&cltHandlePtr->clientList, clientID, 0, &cache)) {
	/*
	 * Invalid client trying to close.
	 */
	printf( "FspdevPseudoStreamClose: client %d not found\n",
	    clientID);
	Fsutil_HandleUnlock(cltHandlePtr);
	return(GEN_INVALID_ARG);
    } else if (!List_IsEmpty(&cltHandlePtr->clientList)) {
	/*
	 * Still clients out there.
	 */
	Fsutil_HandleUnlock(cltHandlePtr);
    } else {
	/*
	 * No clients remaining so we can close down the connection.
	 * Notify the server that a client has gone away.  Then we get rid
	 * of our reference to the server's handle and nuke our own.
	 * Note we unlock the client handle before the request response
	 * in case the server process is buggy and hangs us.
	 */
	if ((segPtr != (struct Vm_Segment *)NIL) &&
	    (segPtr != (struct Vm_Segment *)0) &&
	    (segPtr->type == VM_CODE)) {
	    Vm_FileChanged(&segPtr);
	}
	Fsutil_HandleUnlock(cltHandlePtr);
	FspdevPseudoStreamCloseInt(cltHandlePtr->pdevHandlePtr);
	Fsutil_HandleRelease(cltHandlePtr->pdevHandlePtr, FALSE);
	Fsutil_HandleRelease(cltHandlePtr, FALSE);
	Fsutil_HandleRemove(cltHandlePtr);
    }
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * FspdevRmtPseudoStreamClose --
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
FspdevRmtPseudoStreamClose(streamPtr, clientID, procID, flags, size, data)
    Fs_Stream		*streamPtr;	/* Client pseudo-stream to close */
    int			clientID;	/* HostID of client closing */
    Proc_PID		procID;		/* ID of closing process, IGNORED */
    int			flags;		/* IGNORED */
    int			size;		/* Should be zero */
    ClientData		data;		/* IGNORED */
{
    register Fspdev_ClientIOHandle *cltHandlePtr =
	    (Fspdev_ClientIOHandle *)streamPtr->ioHandlePtr;
    Vm_Segment *segPtr = cltHandlePtr->segPtr;

    DBG_PRINT( ("Client closing rmt pdev %x,%x\n", 
		cltHandlePtr->hdr.fileID.major,
		cltHandlePtr->hdr.fileID.minor) );

    if ((segPtr != (struct Vm_Segment *)NIL) &&
	(segPtr != (struct Vm_Segment *)0) &&
	(segPtr->type == VM_CODE)) {
	Vm_FileChanged(&segPtr);
    }
    Fsrmt_IOClose(streamPtr, clientID, procID, flags, size, data);
    
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * FspdevPseudoStreamMigClose --
 *
 *	Called to release a reference on a pseudo stream.  However, there
 *	is always only one refernece on the handle so we do nothing.
 *
 * Results:
 *	SUCCESS.
 *
 * Side effects:
 *	Release the I/O handle.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
FspdevPseudoStreamMigClose(hdrPtr, flags)
    Fs_HandleHeader *hdrPtr;	/* File being encapsulated */
    int flags;			/* Use flags from the stream */
{
    panic( "FspdevPseudoStreamMigClose called\n");

    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * FspdevPseudoStreamMigrate --
 *
 *	Migrate a pseudo-stream client.
 *
 *	This takes care of transfering references from one client to the other.
 *	A useful side-effect of this routine is	to properly set the type in
 *	the ioFileID, either FSIO_LCL_PSEUDO_STREAM or FSIO_RMT_PSEUDO_STREAM.
 *	In the latter case FspdevRmtPseudoStreamMigrate is called to do all
 *	the work.
 *
 * Results:
 *	An error status if the I/O handle can't be set-up.
 *	Otherwise SUCCESS is returned, *flagsPtr may have the FS_RMT_SHARED
 *	bit set, and *sizePtr and *dataPtr are set to reference Fspdev_State.
 *
 * Side effects:
 *	Sets the correct stream type on the ioFileID.
 *	Shifts client references from the srcClient to the destClient.
 *	Set up and return Fspdev_State for use by the MigEnd routine.
 
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
FspdevPseudoStreamMigrate(migInfoPtr, dstClientID, flagsPtr, offsetPtr, sizePtr,
		      dataPtr)
    Fsio_MigInfo	*migInfoPtr;	/* Migration state */
    int		dstClientID;	/* ID of target client */
    int		*flagsPtr;	/* In/Out Stream usage flags */
    int		*offsetPtr;	/* Return - new stream offset */
    int		*sizePtr;	/* Return - sizeof(Fspdev_State) */
    Address	*dataPtr;	/* Return - pointer to Fspdev_State */
{
    Fspdev_ClientIOHandle			*cltHandlePtr;
    Boolean				closeSrcClient;

    if (migInfoPtr->ioFileID.serverID != rpc_SpriteID) {
	/*
	 * The device was local, which is why we were called, but is
	 * now remote.
	 */
	migInfoPtr->ioFileID.type = FSIO_RMT_PSEUDO_STREAM;
	return(FspdevRmtPseudoStreamMigrate(migInfoPtr, dstClientID, flagsPtr,
					offsetPtr, sizePtr, dataPtr));
    }
    migInfoPtr->ioFileID.type = FSIO_LCL_PSEUDO_STREAM;
    cltHandlePtr = Fsutil_HandleFetchType(Fspdev_ClientIOHandle, &migInfoPtr->ioFileID);
    if (cltHandlePtr == (Fspdev_ClientIOHandle *)NIL) {
	panic( "FspdevPseudoStreamMigrate, no client handle <%d,%x,%x>\n",
		migInfoPtr->ioFileID.serverID,
		migInfoPtr->ioFileID.major, migInfoPtr->ioFileID.minor);
	return(FAILURE);
    }
    DBG_PRINT( ("Migrating pdev %x,%x, ref %d.\n", 
		cltHandlePtr->hdr.fileID.major,
		cltHandlePtr->hdr.fileID.minor,
		cltHandlePtr->hdr.refCount) );
    /*
     * At the stream level, add the new client to the set of clients
     * for the stream, and check for any cross-network stream sharing.
     */
    Fsio_StreamMigClient(migInfoPtr, dstClientID, (Fs_HandleHeader *)cltHandlePtr,
			&closeSrcClient);

    /*
     * Move the client at the I/O handle level.  The flags are used
     * by FsIOClient{Open,Close} and are different for pdevs than
     * other files -- namely, the flags are set to 0 before calls to these
     * routines.  The only flag we have to make sure to pass is
     * whether it's a new stream, since this is used by Fsio_MigrateClient
     * itself.
     */
    Fsio_MigrateClient(&cltHandlePtr->clientList, migInfoPtr->srcClientID,
		      dstClientID, (int)(migInfoPtr->flags & FS_NEW_STREAM),
		      closeSrcClient);

    *sizePtr = 0;
    *dataPtr = (Address)NIL;
    *flagsPtr = migInfoPtr->flags;
    *offsetPtr = migInfoPtr->offset;
    /*
     * We don't need this reference on the I/O handle; there is no change.
     */
    Fsutil_HandleRelease(cltHandlePtr, TRUE);
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * FspdevRmtPseudoStreamMigrate --
 *
 *	Migrate a pseudo-stream client.
 *	This takes care of transfering references from one client to the other.
 *	A useful side-effect of this routine is	to properly set the type in
 *	the ioFileID, either FSIO_LCL_PSEUDO_STREAM or FSIO_RMT_PSEUDO_STREAM.
 *	In the former case FspdevPseudoStreamMigrate is called to do all the work.
 *
 * Results:
 *	An error status if the I/O handle can't be set-up.
 *	Otherwise SUCCESS is returned, *flagsPtr may have the FS_RMT_SHARED
 *	bit set, and *sizePtr and *dataPtr are set to reference Fspdev_State.
 *
 * Side effects:
 *	Sets the correct stream type on the ioFileID.
 *	Shifts client references from the srcClient to the destClient.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
FspdevRmtPseudoStreamMigrate(migInfoPtr, dstClientID, flagsPtr, offsetPtr,
			 sizePtr, dataPtr)
    Fsio_MigInfo	*migInfoPtr;	/* Migration state */
    int		dstClientID;	/* ID of target client */
    int		*flagsPtr;	/* In/Out Stream usage flags */
    int		*offsetPtr;	/* Return - the new stream offset */
    int		*sizePtr;	/* Return - 0 */
    Address	*dataPtr;	/* Return - NIL */
{
    register ReturnStatus		status;

    if (migInfoPtr->ioFileID.serverID == rpc_SpriteID) {
	/*
	 * The device was remote, which is why we were called, but is now local.
	 */
	migInfoPtr->ioFileID.type = FSIO_LCL_PSEUDO_STREAM;
	return(FspdevPseudoStreamMigrate(migInfoPtr, dstClientID, flagsPtr,
				     offsetPtr, sizePtr, dataPtr));
    }
    migInfoPtr->ioFileID.type = FSIO_RMT_PSEUDO_STREAM;
    status = Fsrmt_NotifyOfMigration(migInfoPtr, flagsPtr, offsetPtr,
				0, (Address)NIL);
    DBG_PRINT( ("Migrating remote pdev %x,%x.\n", 
		migInfoPtr->ioFileID.major,
		migInfoPtr->ioFileID.minor) );
    if (status != SUCCESS) {
	printf( "FsrmtDeviceMigrate, server error <%x>\n",
	    status);
    } else {
	*dataPtr = (Address)NIL;
	*sizePtr = 0;
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FspdevPseudoStreamMigOpen --
 *
 *	Complete setup of a pdev client I/O handle after migrating a stream
 *	to the I/O server of the pseudo-device connection (the host running
 *	the user-level server process).  FspdevPseudoStreamMigrate has done
 *	the work of shifting use counts at the stream and I/O handle level.
 *	This routine fills in the stream's ioHandlePtr, but doens't adjust
 *	the low-level reference count on the I/O handle (like other MigEnd
 *	procedures) because the reference count isn't used the same way.
 *	With pseudo-device connections, there is always only one refCount
 *	on the client handle, but there may be entries in the clientList
 *	to reflect remote clients.
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
ReturnStatus
FspdevPseudoStreamMigOpen(migInfoPtr, size, data, hdrPtrPtr)
    Fsio_MigInfo	*migInfoPtr;	/* Migration state */
    int		size;		/* Zero */
    ClientData	data;		/* NIL */
    Fs_HandleHeader **hdrPtrPtr;	/* Return - handle for the file */
{
    register Fspdev_ClientIOHandle *cltHandlePtr;

    cltHandlePtr = Fsutil_HandleFetchType(Fspdev_ClientIOHandle,
				     &migInfoPtr->ioFileID);
    if (cltHandlePtr == (Fspdev_ClientIOHandle *)NIL) {
	panic( "FspdevPseudoStreamMigOpen, no handle.\n");
	return(FAILURE);
    } else {
	/*
	 * Release this reference so the client handle always has
	 * just one reference.  Instead of refcounts, an empty
	 * client list indicates there are no more clients.
	 */
	Fsutil_HandleRelease(cltHandlePtr, TRUE);
	*hdrPtrPtr = (Fs_HandleHeader *)cltHandlePtr;
	return(SUCCESS);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * FspdevRmtPseudoStreamVerify --
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
 *	It should be released with Fsutil_HandleRelease.
 *
 *----------------------------------------------------------------------
 */

Fs_HandleHeader *
FspdevRmtPseudoStreamVerify(fileIDPtr, clientID, domainTypePtr)
    Fs_FileID	*fileIDPtr;	/* Client's I/O file ID */
    int		clientID;	/* Host ID of the client */
    int		*domainTypePtr;	/* Return - FS_PSEUDO_DOMAIN */
{
    register Fspdev_ClientIOHandle	*cltHandlePtr;
    register Fsconsist_ClientInfo	*clientPtr;
    Boolean			found = FALSE;

    if (fileIDPtr->type > 0 && fileIDPtr->type < FSIO_NUM_STREAM_TYPES) {
	fileIDPtr->type = fsio_RmtToLclType[fileIDPtr->type];
    }
    if (fileIDPtr->type != FSIO_LCL_PSEUDO_STREAM &&
	fileIDPtr->type != FSIO_LCL_PFS_STREAM) {
	printf( "FspdevRmtPseudoStreamVerify, bad type <%d>\n",
	    fileIDPtr->type);
	return((Fs_HandleHeader *)NIL);
    }
    cltHandlePtr = Fsutil_HandleFetchType(Fspdev_ClientIOHandle, fileIDPtr);
    if (cltHandlePtr != (Fspdev_ClientIOHandle *)NIL) {
	LIST_FORALL(&cltHandlePtr->clientList, (List_Links *) clientPtr) {
	    if (clientPtr->clientID == clientID) {
		found = TRUE;
		break;
	    }
	}
	if (!found) {
	    Fsutil_HandleRelease(cltHandlePtr, TRUE);
	    cltHandlePtr = (Fspdev_ClientIOHandle *)NIL;
	}
    }
    if (!found) {
	printf(
	    "FspdevRmtPseudoStreamVerify, client %d not known for %s <%x,%x>\n",
	    clientID, Fsutil_FileTypeToString(fileIDPtr->type),
	    fileIDPtr->major, fileIDPtr->minor);
    }
    if (domainTypePtr != (int *)NIL) {
	*domainTypePtr = FS_PSEUDO_DOMAIN;
    }
    return((Fs_HandleHeader *)cltHandlePtr);
}
