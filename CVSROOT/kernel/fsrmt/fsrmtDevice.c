/* 
 * fsRmtDevice.c --
 *
 *	Routines to manage Remote device.
 *
 *
 * Copyright 1987 Regents of the University of California
 * All rights reserved.
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
#include <fsconsist.h>
#include <fsio.h>
#include <fsutil.h>
#include <fsNameOps.h>
#include <fsrmtNameOpsInt.h>
#include <fsdm.h>
#include <fsioLock.h>
#include <fsprefix.h>
#include <fsrmt.h>
#include <dev.h>
#include <rpc.h>
#include <recov.h>
#include <fsStat.h>
#include <fsioDevice.h>
#include <devFsOpTable.h>

#include <stdio.h>

/*
 * Parameters for RPC_FS_DEV_OPEN remote procedure call.
 * The return value from this call is a new I/O fileID.
 */
typedef struct FsDeviceRemoteOpenParam {
    Fs_FileID	fileID;		/* I/O fileID from name server. */
    int		useFlags;	/* FS_READ | FS_WRITE ... */
    int		dataSize;	/* size of openData */
    FsrmtUnionData	openData;	/* Fsio_DeviceState or Fspdev_State.
				 * NOTE. be careful when assigning this.
				 * bcopy() of the whole union can cause
				 * bus errors if really only a small object
				 * exists and it's at the end of a page. */
} FsDeviceRemoteOpenParam;




/*
 *----------------------------------------------------------------------
 *
 * FsrmtDeviceIoOpen --
 *
 *      Set up the stream IO handle for a remote device.  This makes
 *	an RPC to the I/O server, which will invoke Fsio_DeviceIoOpen there,
 *	and then sets up the remote device handle.
 *
 * Results:
 *	SUCCESS or a device dependent open error code.
 *
 * Side effects:
 *	Sets up and installs the remote device's ioHandle.
 *	The use counts on the handle are updated.
 *	The handle is returned unlocked, but with a new
 *	reference than gets released when the device is closed.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
FsrmtDeviceIoOpen(ioFileIDPtr, flagsPtr, clientID, streamData, name, ioHandlePtrPtr)
    Fs_FileID		*ioFileIDPtr;	/* I/O fileID */
    int			*flagsPtr;	/* FS_READ | FS_WRITE ... */
    int			clientID;	/* Host doing the open */
    ClientData		streamData;	/* Fsio_DeviceState */
    char		*name;		/* Device file name */
    Fs_HandleHeader	**ioHandlePtrPtr;/* Return - a handle set up for
					 * I/O to a device, NIL if failure. */
{
    register ReturnStatus 	status;

    /*
     * Do a device open at the I/O server.  We set the ioFileID type so
     * that the local device client open procedure gets called at the I/O
     * server, as opposed to the local pipe (or whatever) open routine.
     * NAME note: are not passing the file name to the I/O server.
     */
    ioFileIDPtr->type = FSIO_LCL_DEVICE_STREAM;
    status = Fsrmt_DeviceOpen(ioFileIDPtr, *flagsPtr,	sizeof(Fsio_DeviceState),
				streamData);
    if (status == SUCCESS) {
	ioFileIDPtr->type = FSIO_RMT_DEVICE_STREAM;
	Fsrmt_IOHandleInit(ioFileIDPtr, *flagsPtr, name, ioHandlePtrPtr);
    } else {
	*ioHandlePtrPtr = (Fs_HandleHeader *)NIL;
    }
    free((Address)streamData);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsrmt_IOHandleInit --
 *
 *	Initialize a handle for a remote device/pseudo-device/whatever.
 *
 * Results:
 *	Sets its *ioHandlePtrPtr to reference the installed handle.
 *
 * Side effects:
 *	Create and install a handle for remote thing.  The handle is
 *	returned unlocked.  The recovery use counts are incremented
 *	to reflect the use of the handle.
 *
 *----------------------------------------------------------------------
 */
void
Fsrmt_IOHandleInit(ioFileIDPtr, useFlags, name, newHandlePtrPtr)
    Fs_FileID		*ioFileIDPtr;		/* Remote IO File ID */
    int			useFlags;		/* Stream usage flags */
    char		*name;			/* File name */
    Fs_HandleHeader	**newHandlePtrPtr;	/* Return - installed handle */
{
    register Boolean found;
    register Fsutil_RecoveryInfo *recovPtr;

    found = Fsutil_HandleInstall(ioFileIDPtr, sizeof(Fsrmt_IOHandle), name,
		    FALSE, newHandlePtrPtr);
    recovPtr = &((Fsrmt_IOHandle *)*newHandlePtrPtr)->recovery;
    if (!found) {
	Fsutil_RecoveryInit(recovPtr);
	fs_Stats.object.remote++;
	/*
	 * Register a callback with the recovery module.  This ensures that
	 * someone is paying attention to the I/O server and the filesystem
	 * will get called back when the I/O server reboots.
	 */
	Recov_RebootRegister(ioFileIDPtr->serverID, Fsutil_Reopen,
			    (ClientData)NIL);
    }
    recovPtr->use.ref++;
    if (useFlags & FS_WRITE) {
	recovPtr->use.write++;
    }
    Fsutil_HandleUnlock(*newHandlePtrPtr);
}

/*----------------------------------------------------------------------
 *
 * Fsrmt_DeviceOpen --
 *
 *	Client side stub to open a remote device, remote named pipe,
 *	or remote pseudo stream.  Uses the RPC_FS_DEV_OPEN remote
 *	procedure call to invoke the pipe, device, or pseudo device
 *	open routine on the I/O server.  We are given an ioFileID from
 *	the name server, although we just use the serverID part here.
 *	The I/O server returns a new fileID that connects to the device.
 *
 * Results:
 *	(May) modify *ioFileIDPtr.  Returns a status from the RPC or the
 *	I/O device driver.
 *
 * Side effects:
 *	Sets up a recovery reboot call-back for the I/O server if the remote
 *	device open succeeds.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Fsrmt_DeviceOpen(ioFileIDPtr, useFlags, inSize, inBuffer)
    Fs_FileID	*ioFileIDPtr;	/* Indicates I/O server.  This is modified
				 * by the I/O server and returned to our
				 * caller for use in the dev/pipe/etc handle */
    int		useFlags;	/* FS_READ | FS_WRITE ... */
    int		inSize;		/* Size of input parameters */
    ClientData	inBuffer;	/* Input data for remote server */
{
    ReturnStatus	status;		/* Return code from RPC */
    Rpc_Storage		storage;	/* Specifies inputs/outputs to RPC */
    FsDeviceRemoteOpenParam param;

    param.fileID = *ioFileIDPtr;
    param.useFlags = useFlags;
    param.dataSize = inSize;
    if (inSize > 0) {
	bcopy((Address)inBuffer, (Address)&param.openData, inSize);
    }
    storage.requestParamPtr = (Address) &param;
    storage.requestParamSize = sizeof(FsDeviceRemoteOpenParam);
    storage.requestDataPtr = (Address) NIL;
    storage.requestDataSize = 0;
    storage.replyParamPtr = (Address) ioFileIDPtr;
    storage.replyParamSize = sizeof(Fs_FileID);
    storage.replyDataPtr = (Address) NIL;
    storage.replyDataSize = 0;

    status = Rpc_Call(ioFileIDPtr->serverID, RPC_FS_DEV_OPEN, &storage);

    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsrmt_RpcDevOpen --
 *
 *	Server stub for the RPC_FS_DEV_OPEN call.
 *	This host is the IO server for a handle.  This message from the
 *	remote host indicates that a client process there will be
 *	using us as the IO server.  This adds that client to the handle's
 *	client list for recovery and consistency checks.  This then branches
 *	to the file type client-open procedure to set up an I/O
 *	handle for the (device, pipe, pseudo-device).
 *
 * Results:
 *	If this procedure returns SUCCESS then a reply has been sent to
 *	the client.  If the arguments are bad then an error is 
 *	returned and the main RPC server level sends back an error reply.
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
Fsrmt_RpcDevOpen(srvToken, clientID, command, storagePtr)
    ClientData 		 srvToken;	/* Handle on server process passed to
					 * Rpc_Reply */
    int 		 clientID;	/* Sprite ID of client host */
    int 		 command;	/* Command identifier */
    register Rpc_Storage *storagePtr;	/* The request fields refer to the 
					 * request buffers and also indicate 
					 * the exact amount of data in the 
					 * request buffers.  The reply fields 
					 * are initialized to NIL for the
				 	 * pointers and 0 for the lengths.  
					 * This can be passed to Rpc_Reply */
{
    Fs_HandleHeader	*hdrPtr;	/* I/O handle created by type-specific
					 * open routine. */
    ReturnStatus	status;
    FsDeviceRemoteOpenParam *paramPtr;
    register int	dataSize;
    ClientData		streamData;

    /*
     * Call the client-open routine to set up an I/O handle here on the
     * I/O server for the device.  This is either the device, pipe, or
     * named pipe open routine.  We allocate storage and copy the stream
     * data so the CltOpen routine can free it, as it expects to do.
     * NAME note: we don't have a name for the device here.
     */
    paramPtr = (FsDeviceRemoteOpenParam *) storagePtr->requestParamPtr;
    dataSize = paramPtr->dataSize;
    if (dataSize > 0) {
	streamData = (ClientData)malloc(dataSize);
	bcopy((Address)&paramPtr->openData, (Address)streamData, dataSize);
    } else {
	streamData = (ClientData)NIL;
    }
    paramPtr->fileID.type = Fsio_MapRmtToLclType(paramPtr->fileID.type);
    if (paramPtr->fileID.type < 0) {
	return(GEN_INVALID_ARG);
    }
    status = (fsio_StreamOpTable[paramPtr->fileID.type].ioOpen)
		    (&paramPtr->fileID, &paramPtr->useFlags,
		     clientID, streamData, (char *)NIL, &hdrPtr);
    if (status == SUCCESS) {
	/*
	 * Return the fileID to the other host so it can
	 * set up its own I/O handle.
	 */
	storagePtr->replyParamPtr = (Address)&hdrPtr->fileID;
	storagePtr->replyParamSize = sizeof(Fs_FileID);
    }
    Rpc_Reply(srvToken, status, storagePtr, (int (*)())NIL, (ClientData)NIL);
    return(SUCCESS);	/* So that Rpc_Server doesn't send error reply */
}

/*
 *----------------------------------------------------------------------
 *
 * Fsrmt_IOClose --
 *
 *	Close a stream to a remote device/pipe.  We just need to clean up our
 *	connection to the I/O server.  (The file server that named the
 *	device file keeps no state about us, so we don't have to contact it.)
 *	We make an RPC to the I/O server which invokes close routine there.
 *	We also update our own use counts.
 *
 * Results:
 *	SUCCESS.
 *
 * Side effects:
 *	RPC to the I/O server to invoke Fsio_DeviceClose/Fsio_PipeClose.
 *	Release the remote I/O handle.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Fsrmt_IOClose(streamPtr, clientID, procID, flags, dataSize, closeData)
    Fs_Stream		*streamPtr;	/* Stream to remote object */
    int			clientID;	/* ID of closing host */
    Proc_PID		procID;		/* ID of closing process */
    int			flags;		/* Flags from the stream being closed */
    int			dataSize;	/* Size of *closeData, or Zero */
    ClientData		closeData;	/* Copy of cached I/O attributes. */
{
    ReturnStatus		status;
    register Fsrmt_IOHandle *rmtHandlePtr =
	    (Fsrmt_IOHandle *)streamPtr->ioHandlePtr;

    /*
     * Decrement local references.
     */
    rmtHandlePtr->recovery.use.ref--;
    if (flags & FS_WRITE) {
	rmtHandlePtr->recovery.use.write--;
    }

    if (rmtHandlePtr->recovery.use.ref < 0 ||
	rmtHandlePtr->recovery.use.write < 0) {
	panic( "Fsrmt_IOClose: <%d,%d> ref %d write %d\n",
	    rmtHandlePtr->hdr.fileID.major, rmtHandlePtr->hdr.fileID.minor,
	    rmtHandlePtr->recovery.use.ref,
	    rmtHandlePtr->recovery.use.write);
    }

    if (!Fsutil_HandleValid(streamPtr->ioHandlePtr)) {
	status = FS_STALE_HANDLE;
    } else {
	status = Fsrmt_Close(streamPtr, clientID, procID, flags,
			       dataSize, closeData);
    }
    /*
     * Check the number of users with the handle still locked, then
     * remove the handle if we aren't using it anymore.  Note that if
     * we get an RPC timeout we hold onto the handle and will do a
     * reopen later to reconcile the server with our state.  A transient
     * communication failure, for example, would otherwise cause a close
     * to be dropped and leave lingering references to the device
     * on the I/O server.
     */
    if (status == SUCCESS && rmtHandlePtr->recovery.use.ref == 0) {
	/*
	 * Undo the callback we registered when we created the remote handle.
	 * Then nuke the handle itself.
	 */
	Recov_RebootUnRegister(rmtHandlePtr->hdr.fileID.serverID, Fsutil_Reopen,
			    (ClientData)NIL);
	Fsutil_RecoverySyncLockCleanup(&rmtHandlePtr->recovery);
	Fsutil_HandleRelease(rmtHandlePtr, TRUE);
	Fsutil_HandleRemove(rmtHandlePtr);
	fs_Stats.object.remote--;
    } else {
	Fsutil_HandleRelease(rmtHandlePtr, TRUE);
    }
    return(status);
}

/*
 * Parameters for a device reopen RPC used to reestablish state on
 * the I/O server for a device.
 */
typedef struct FsRmtDeviceReopenParams {
    Fs_FileID	fileID;		/* File ID of file to reopen. MUST BE FIRST! */
    Fsio_UseCounts use;		/* Device usage information */
} FsRmtDeviceReopenParams;

/*
 *----------------------------------------------------------------------
 *
 * FsrmtDeviceReopen --
 *
 *	Reopen a remote device at the I/O server.  This sets up and conducts an 
 *	RPC_FS_DEV_REOPEN remote procedure call to re-open the remote device.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *	
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
FsrmtDeviceReopen(hdrPtr, clientID, inData, outSizePtr, outDataPtr)
    Fs_HandleHeader	*hdrPtr;	/* Device I/O handle to reopen */
    int			clientID;	/* Client doing the reopen */
    ClientData		inData;		/* IGNORED */
    int			*outSizePtr;	/* Size of returned data, 0 here */
    ClientData		*outDataPtr;	/* Returned data, NIL here */
{
    register Fsrmt_IOHandle	*handlePtr = (Fsrmt_IOHandle *)hdrPtr;
    ReturnStatus		status;
    int				outSize;
    FsRmtDeviceReopenParams	reopenParams;

    /*
     * Set up reopen parameters.  fileID must be first in order
     * to use the generic FsrmtReopen/Fsrmt_RpcReopen stubs.
     */
    reopenParams.fileID = handlePtr->hdr.fileID;
    reopenParams.use = handlePtr->recovery.use;

    outSize = 0;
    status = FsrmtReopen(hdrPtr, sizeof(reopenParams),
			    (Address)&reopenParams, &outSize, (Address)NIL);
    return(status);
}


/*
 * ----------------------------------------------------------------------------
 *
 * Fsrmt_IOMigClose --
 *
 *	Release a reference on a remote I/O handle.  This decrements
 *	recovery use counts as well as releasing the handle.
 *	
 * Results:
 *	SUCCESS.
 *
 * Side effects:
 *	Decrement recovery use counts and release the I/O handle.
 *
 * ----------------------------------------------------------------------------
 *
 */
/*ARGSUSED*/
ReturnStatus
Fsrmt_IOMigClose(hdrPtr, flags)
    Fs_HandleHeader *hdrPtr;	/* File being encapsulated */
    int flags;			/* Use flags from the stream */
{
    register Fsrmt_IOHandle *rmtHandlePtr = (Fsrmt_IOHandle *)hdrPtr;

    Fsutil_HandleLock(rmtHandlePtr);
    rmtHandlePtr->recovery.use.ref--;
    if (flags & FS_WRITE) {
	rmtHandlePtr->recovery.use.write--;
    }
    if (flags & FS_EXECUTE) {
	rmtHandlePtr->recovery.use.exec--;
    }
    Fsutil_HandleRelease(rmtHandlePtr, TRUE);
    return(SUCCESS);
}

/*
 * ----------------------------------------------------------------------------
 *
 * FsrmtDeviceMigrate --
 *
 *	This takes care of transfering references from one client to the other.
 *	A useful side-effect of this routine is	to properly set the type in
 *	the ioFileID, either FSIO_LCL_DEVICE_STREAM or FSIO_RMT_DEVICE_STREAM.
 *	In the latter case FsDevoceMigrate is called to do all the work.
 *
 * Results:
 *	An error status if the I/O handle can't be set-up.
 *	Otherwise SUCCESS is returned, *flagsPtr may have the FS_RMT_SHARED
 *	bit set, and *sizePtr and *dataPtr are set to reference Fsio_DeviceState.
 *
 * Side effects:
 *	Sets the correct stream type on the ioFileID.
 *	Shifts client references from the srcClient to the destClient.
 *	Set up and return Fsio_DeviceState for use by the MigEnd routine.
 *
 * ----------------------------------------------------------------------------
 *
 */
/*ARGSUSED*/
ReturnStatus
FsrmtDeviceMigrate(migInfoPtr, dstClientID, flagsPtr, offsetPtr, sizePtr, dataPtr)
    Fsio_MigInfo	*migInfoPtr;	/* Migration state */
    int		dstClientID;	/* ID of target client */
    int		*flagsPtr;	/* In/Out Stream usage flags */
    int		*offsetPtr;	/* Return - the new stream offset */
    int		*sizePtr;	/* Return - sizeof(Fsio_DeviceState) */
    Address	*dataPtr;	/* Return - pointer to Fsio_DeviceState */
{
    register ReturnStatus		status;

    if (migInfoPtr->ioFileID.serverID == rpc_SpriteID) {
	/*
	 * The device was remote, which is why we were called, but is now local.
	 */
	migInfoPtr->ioFileID.type = FSIO_LCL_DEVICE_STREAM;
	return(Fsio_DeviceMigrate(migInfoPtr, dstClientID, flagsPtr, offsetPtr,
		sizePtr, dataPtr));
    }
    migInfoPtr->ioFileID.type = FSIO_RMT_DEVICE_STREAM;
    status = Fsrmt_NotifyOfMigration(migInfoPtr, flagsPtr, offsetPtr,
				0, (Address)NIL);
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
 * ----------------------------------------------------------------------------
 *
 * Fsrmt_IOMigOpen --
 *
 *	Create a FSIO_RMT_DEVICE_STREAM or FSIO_RMT_PIPE_STREAM after migration.
 *	The srvMigrate routine has done most all the work.
 *	We just grab a reference on the I/O handle for the stream.
 *
 * Results:
 *	Sets the I/O handle.
 *
 * Side effects:
 *	May install the handle.  Ups use and reference counts.
 *
 * ----------------------------------------------------------------------------
 *
 */
/*ARGSUSED*/
ReturnStatus
Fsrmt_IOMigOpen(migInfoPtr, size, data, hdrPtrPtr)
    Fsio_MigInfo	*migInfoPtr;	/* Migration state */
    int		size;		/* Zero */
    ClientData	data;		/* NIL */
    Fs_HandleHeader **hdrPtrPtr;	/* Return - handle for the file */
{
    register Fsrmt_IOHandle *rmtHandlePtr;
    register Fsutil_RecoveryInfo *recovPtr;
    Boolean found;

    found = Fsutil_HandleInstall(&migInfoPtr->ioFileID, sizeof(Fsrmt_IOHandle),
		(char *)NIL, FALSE, hdrPtrPtr);
    rmtHandlePtr = (Fsrmt_IOHandle *)*hdrPtrPtr;
    recovPtr = &rmtHandlePtr->recovery;
    if (!found) {
	Fsutil_RecoveryInit(recovPtr);
	fs_Stats.object.remote++;
	/*
	 * Register a callback with the recovery module.  This ensures that
	 * someone is paying attention to the I/O server and the filesystem
	 * will get called back when the I/O server reboots.
	 */
	Recov_RebootRegister(migInfoPtr->ioFileID.serverID, Fsutil_Reopen,
			    (ClientData)NIL);
    }
    recovPtr->use.ref++;
    if (migInfoPtr->flags & FS_WRITE) {
	recovPtr->use.write++;
    }
    Fsutil_HandleUnlock(rmtHandlePtr);
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * FsrmtDeviceVerify --
 *
 *	Verify that the remote client is known for the device, and return
 *	a locked pointer to the device's I/O handle.
 *
 * Results:
 *	A pointer to the I/O handle for the device, or NIL if
 *	the client is bad.
 *
 * Side effects:
 *	The handle is returned locked and with its refCount incremented.
 *	It should be released with Fsutil_HandleRelease.
 *
 *----------------------------------------------------------------------
 */

Fs_HandleHeader *
FsrmtDeviceVerify(fileIDPtr, clientID, domainTypePtr)
    Fs_FileID	*fileIDPtr;	/* Client's I/O file ID */
    int		clientID;	/* Host ID of the client */
    int		*domainTypePtr;	/* Return - FS_LOCAL_DOMAIN */
{
    register Fsio_DeviceIOHandle	*devHandlePtr;
    register Fsconsist_ClientInfo	*clientPtr;
    Boolean			found = FALSE;

    fileIDPtr->type = Fsio_MapRmtToLclType(fileIDPtr->type);
    if (fileIDPtr->type != FSIO_LCL_DEVICE_STREAM) {
	printf("FsrmtDeviceVerify, bad file ID type\n");
	return((Fs_HandleHeader *)NIL);
    }
    devHandlePtr = Fsutil_HandleFetchType(Fsio_DeviceIOHandle, fileIDPtr);
    if (devHandlePtr != (Fsio_DeviceIOHandle *)NIL) {
	LIST_FORALL(&devHandlePtr->clientList, (List_Links *) clientPtr) {
	    if (clientPtr->clientID == clientID) {
		found = TRUE;
		break;
	    }
	}
	if (!found) {
	    Fsutil_HandleRelease(devHandlePtr, TRUE);
	    devHandlePtr = (Fsio_DeviceIOHandle *)NIL;
	}
    }
    if (!found) {
	printf("FsrmtDeviceVerify, client %d not known for device <%d,%d>\n",
	    clientID, fileIDPtr->major, fileIDPtr->minor);
    }
    if (domainTypePtr != (int *)NIL) {
	*domainTypePtr = FS_LOCAL_DOMAIN;
    }
    return((Fs_HandleHeader *)devHandlePtr);
}


/*
 *----------------------------------------------------------------------
 *
 * Fsrmt_DeviceReopen --
 *
 *	Reopen a device here on the I/O server.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *	
REMOVE this code whenever the fsio and fsrmt modules are both reinstalled.
This routine has been renamed Fsio_DeviceReopen and is now found in the
fsio module.  It has been left here for temporary backwards-compatibility.
			Mary Baker 9/20/91
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
Fsrmt_DeviceReopen(hdrPtr, clientID, inData, outSizePtr, outDataPtr)
    Fs_HandleHeader	*hdrPtr;	/* NIL on the I/O server */
    int			clientID;	/* Client doing the reopen */
    ClientData		inData;		/* Ref. to FsRmtDeviceReopenParams */
    int			*outSizePtr;	/* Size of returned data, 0 here */
    ClientData		*outDataPtr;	/* Returned data, NIL here */
{
    Fsio_DeviceIOHandle	*devHandlePtr;
    ReturnStatus	status;
    register		devIndex;
    register FsRmtDeviceReopenParams *paramPtr =
	    (FsRmtDeviceReopenParams *)inData;

    *outDataPtr = (ClientData) NIL;
    *outSizePtr = 0;

    (void) FsioDeviceHandleInit(&paramPtr->fileID, (char *)NIL, &devHandlePtr); 

    devIndex = DEV_TYPE_INDEX(devHandlePtr->device.type);
    if (devIndex >= devNumDevices) {
	status = FS_DEVICE_OP_INVALID;
    } else {
	/*
	 * Compute the difference between the client's and our version
	 * of the client's use state, and then call the device driver
	 * with that information.  We may have missed opens (across a
	 * reboot) or closes (during transient communication failures)
	 * so the net difference may be positive or negative.
	 */
	Fsconsist_IOClientStatus(&devHandlePtr->clientList, clientID, &paramPtr->use);
	if (paramPtr->use.ref == 0) {
	    status = SUCCESS;	/* No change visible to driver */
	} else if (paramPtr->use.ref > 0) {
	    /*
	     * Reestablish open connections.
	     */
	    status = (*devFsOpTable[devIndex].reopen)(&devHandlePtr->device,
				    paramPtr->use.ref, paramPtr->use.write,
				    (Fs_NotifyToken)devHandlePtr,
				    &devHandlePtr->flags);
	    if (status == SUCCESS) {
		(void)Fsconsist_IOClientReopen(&devHandlePtr->clientList, clientID,
					 &paramPtr->use);
		devHandlePtr->use.ref += paramPtr->use.ref;
		devHandlePtr->use.write += paramPtr->use.write;
	    }
	} else {
	    /*
	     * Clean up closed connections.  Note, we assume that
	     * the client was reading, even though it may have had
	     * a write-only stream.  This could break syslog, which
	     * is a single-reader/multiple-writer stream.  "ref" should
	     * be changed to "read".
	     */
	    int useFlags = FS_READ;
	    if (paramPtr->use.write > 0) {
		useFlags |= FS_WRITE;
	    }
	    status = FsioDeviceCloseInt(devHandlePtr, useFlags, paramPtr->use.ref,
						    paramPtr->use.write);
	 }
    }
    Fsutil_HandleRelease(devHandlePtr, TRUE);
    return(status);
}

