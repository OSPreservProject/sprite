/* 
 * fsDevice.c --
 *
 *	Routines to manage devices.  Remote devices are	supported.
 *	The handle for a device is uniquified using the device type and
 *	unit number.  This ensures that different filesystem names for
 *	the same device map to the same device I/O handle.
 *
 *	There are two sets of routines here.  There are routines internal
 *	to the filesystem that are used to open, close, read, write, etc.
 *	a device stream.  Then there are some external routines exported
 *	for use by device drivers.  Fs_NotifyReader and Fs_NotifyWriter
 *	are used by interrupt handlers to indicate a device is ready.
 *	Then there are a handful of conversion routines for mapping
 *	from file system block numbers to disk addresses.
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


#include "sprite.h"

#include "fs.h"
#include "fsInt.h"
#include "fsDevice.h"
#include "fsOpTable.h"
#include "fsDebug.h"
#include "fsFile.h"
#include "fsDisk.h"
#include "fsClient.h"
#include "fsStream.h"
#include "fsLock.h"
#include "fsMigrate.h"
#include "fsNameOps.h"

#include "dev.h"
#include "rpc.h"
#include "fsStat.h"

/*
 * Parameters for RPC_FS_DEV_OPEN remote procedure call.
 * The return value from this call is a new I/O fileID.
 */
typedef struct FsDeviceRemoteOpenPrm {
    FsFileID	fileID;		/* I/O fileID from name server. */
    int		useFlags;	/* FS_READ | FS_WRITE ... */
    int		dataSize;	/* size of openData */
    FsUnionData	openData;	/* FsFileState, FsDeviceState or PdevState */
} FsDeviceRemoteOpenPrm;

void ReadNotify();
void WriteNotify();
void ExceptionNotify();


/*
 *----------------------------------------------------------------------
 *
 * FsDeviceHandleInit --
 *
 *	Initialize a handle for a local device.
 *
 * Results:
 *	TRUE if the handle was already found, FALSE if not.
 *
 * Side effects:
 *	Create and install a handle for the device.  It is returned locked
 *	and with its reference count incremented if SUCCESS is returned.
 *
 *----------------------------------------------------------------------
 */
Boolean
FsDeviceHandleInit(fileIDPtr, name, newHandlePtrPtr)
    FsFileID		*fileIDPtr;
    char		*name;
    FsDeviceIOHandle	**newHandlePtrPtr;
{
    register Boolean found;
    register FsDeviceIOHandle *devHandlePtr;

    found = FsHandleInstall(fileIDPtr, sizeof(FsDeviceIOHandle), name,
		    (FsHandleHeader **)newHandlePtrPtr);
    if (!found) {
	devHandlePtr = *newHandlePtrPtr;
	devHandlePtr->use.ref = 0;
	devHandlePtr->use.write = 0;
	devHandlePtr->use.exec = 0;
	devHandlePtr->device.serverID = fileIDPtr->serverID;
	devHandlePtr->device.type = fileIDPtr->major;
	devHandlePtr->device.unit = fileIDPtr->minor;
	devHandlePtr->device.data = (ClientData)NIL;
	List_Init(&devHandlePtr->clientList);
	FsLockInit(&devHandlePtr->lock);
	devHandlePtr->modifyTime = 0;
	devHandlePtr->accessTime = 0;
	List_Init(&devHandlePtr->readWaitList);
	List_Init(&devHandlePtr->writeWaitList);
	List_Init(&devHandlePtr->exceptWaitList);
	devHandlePtr->readNotifyScheduled = FALSE;
	devHandlePtr->writeNotifyScheduled = FALSE;
    }
    return(found);
}


/*
 *----------------------------------------------------------------------
 *
 * FsDeviceSrvOpen --
 *
 *	This routine sets up an ioFileID for the device based on the
 *	device file found on the name server.  This is called on the name
 *	server in two cases, when a client is doing an open, and when
 *	it is doing a Get/Set attributes on a device file name.   At
 *	open time some additional attributes are returned to the client
 *	for caching at the I/O server, and a streamID is chosen.
 *	Note that no state is kept about the device client here on the
 *	name server.  The device client open routine sets up that state.
 *
 * Results:
 *	SUCCESS, *ioFileIDPtr has the I/O file ID, and *clientDataPtr
 *	references the device state.
 *
 * Side effects:
 *	Choose the fileID for the clients stream.
 *	Allocates memory to hold the stream data.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
FsDeviceSrvOpen(handlePtr, clientID, useFlags, ioFileIDPtr, streamIDPtr,
	    dataSizePtr, clientDataPtr)
    FsLocalFileIOHandle	*handlePtr;	/* A handle obtained by FsLocalLookup.
					 * Should be locked upon entry,
					 * is unlocked upon exit. */
    int		clientID;		/* ID of client doing the open.
					 * SHOULD REFLECT MIGRATION SOMEHOW */
    int		useFlags;		/* Use flags from the stream */
    register FsFileID	*ioFileIDPtr;	/* Return - FileID used for I/O or
					 * to get/set I/O attributes */
    FsFileID	*streamIDPtr;		/* ID of stream being opened.  NIL
					 * during set/get attributes */
    int		*dataSizePtr;		/* Return - sizeof(FsDeviceState) */
    ClientData	*clientDataPtr;		/* Return - FsDeviceState.  Nothing
					 * returned during set/get attrs */
{
    register FsFileDescriptor *descPtr = handlePtr->descPtr;
    register FsDeviceState *deviceDataPtr;

    ioFileIDPtr->serverID = descPtr->devServerID;
    if (ioFileIDPtr->serverID == FS_LOCALHOST_ID) {
	ioFileIDPtr->serverID = clientID;
    }
    if (ioFileIDPtr->serverID == clientID) {
	ioFileIDPtr->type = FS_LCL_DEVICE_STREAM;
    } else {
	ioFileIDPtr->type = FS_RMT_DEVICE_STREAM;
    }
    ioFileIDPtr->major = descPtr->devType;
    ioFileIDPtr->minor = descPtr->devUnit;

    if (streamIDPtr != (FsFileID *)NIL) {
	/*
	 * Truely preparing for an open.  Return the current modify
	 * and access times for the I/O server's cache.  Return the
	 * fileID from the device file for client get/set attribute calls.
	 */
	deviceDataPtr = Mem_New(FsDeviceState);
	deviceDataPtr->modifyTime = descPtr->dataModifyTime;
	deviceDataPtr->accessTime = descPtr->accessTime;
	/*
	 * Choose a streamID for the client that points to the device server.
	 */
	FsStreamNewID(ioFileIDPtr->serverID, streamIDPtr);
	deviceDataPtr->streamID = *streamIDPtr;

	*clientDataPtr = (ClientData)deviceDataPtr;
	*dataSizePtr = sizeof(FsDeviceState);
    }
    FsHandleUnlock(handlePtr);
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * FsDeviceCltOpen --
 *
 *	Complete opening of a local device.  This is called on the I/O
 *	server and sets up state concerning this client.  The device
 *	driver open routine is called to set up the device.  If this
 *	succeeds then the device's Handle is installed and set up.
 *	This includes incrementing client's use counts and the
 *	global use counts in the handle.
 *
 * Results:
 *	SUCCESS or a device dependent open error code.
 *
 * Side effects:
 *	Sets up and installs the device's ioHandle.  The device-type open
 *	routine is called on the I/O server.  The handle is unlocked
 *	upon exit, but its reference count has been incremented.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
FsDeviceCltOpen(ioFileIDPtr, flagsPtr, clientID, streamData, name, ioHandlePtrPtr)
    register FsFileID	*ioFileIDPtr;	/* I/O fileID */
    int			*flagsPtr;	/* FS_READ | FS_WRITE ... */
    int			clientID;	/* Host doing the open */
    ClientData		streamData;	/* Reference to FsDeviceState struct */
    char		*name;		/* File name for error msgs */
    FsHandleHeader	**ioHandlePtrPtr;/* Return - a locked handle set up for
					 * I/O to a device, NIL if failure. */
{
    ReturnStatus 		status;
    Boolean			found;
    register FsDeviceState	*deviceDataPtr;
    register FsDeviceIOHandle	*devHandlePtr;
    FsDeviceIOHandle		*tDevHandlePtr;
    register Fs_Stream		*streamPtr;
    register int		flags = *flagsPtr;

    deviceDataPtr = (FsDeviceState *)streamData;
    *ioHandlePtrPtr = (FsHandleHeader *)NIL;

    found = FsDeviceHandleInit(ioFileIDPtr, name, &tDevHandlePtr);
    devHandlePtr = tDevHandlePtr;
    /*
     * The device driver gets the device specification, ie. type
     * and unit number, the useFlags, and a token passed to Fs_NotifyReader
     * or Fs_NotifyWriter when the device becomes ready for I/O.
     */
    if (devHandlePtr->device.type >= devNumDevices) {
	status = FS_DEVICE_OP_INVALID;
    } else {
	status = (*devFsOpTable[devHandlePtr->device.type].open)
		    (&devHandlePtr->device, flags, (ClientData)devHandlePtr);
    }
    if (status == SUCCESS) {
	if (!found) {
	    devHandlePtr->modifyTime = deviceDataPtr->modifyTime;
	    devHandlePtr->accessTime = deviceDataPtr->accessTime;
	}
	/*
	 * Trace the client, and then update our overall use counts.
	 * The client is recorded at the stream level to support migration,
	 * and at the I/O handle level for regular I/O.
	 */
	(void)FsIOClientOpen(&devHandlePtr->clientList, clientID, flags, FALSE);

	streamPtr = FsStreamAddClient(&deviceDataPtr->streamID, clientID,
			(FsHandleHeader *)devHandlePtr, flags,
			name, (Boolean *)NIL);
	FsHandleRelease(streamPtr, TRUE);

	devHandlePtr->use.ref++;
	if (flags & FS_WRITE) {
	    devHandlePtr->use.write++;
	}
	*ioHandlePtrPtr = (FsHandleHeader *)devHandlePtr;
	FsHandleUnlock(devHandlePtr);
    } else {
	FsHandleRelease(devHandlePtr, TRUE);
    }
    Mem_Free((Address) deviceDataPtr);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsRmtDeviceCltOpen --
 *
 *      Set up the stream IO handle for a remote device.  This makes
 *	an RPC to the I/O server, which will invoke FsDeviceCltOpen there,
 *	and then sets up the remote device handle.
 *
 * Results:
 *	SUCCESS or a device dependent open error code.
 *
 * Side effects:
 *	Sets up and installs the remote device's ioHandle.  The device-type open
 *	routine is called on the I/O server.  The use counts on the handle
 *	are updated.  The handle is returned unlocked, but with a new
 *	reference than gets released when the device is closed.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
FsRmtDeviceCltOpen(ioFileIDPtr, flagsPtr, clientID, streamData, name, ioHandlePtrPtr)
    FsFileID		*ioFileIDPtr;	/* I/O fileID */
    int			*flagsPtr;	/* FS_READ | FS_WRITE ... */
    int			clientID;	/* Host doing the open */
    ClientData		streamData;	/* FsDeviceState */
    char		*name;		/* Device file name */
    FsHandleHeader	**ioHandlePtrPtr;/* Return - a handle set up for
					 * I/O to a device, NIL if failure. */
{
    ReturnStatus 	status;
    Boolean		found;
    FsDeviceState	*deviceStatePtr;

    if (clientID != rpc_SpriteID) {
	Sys_Panic(SYS_FATAL, "FsRmtDeviceCltOpen, bad clientID for rmtOpen\n");
    }
    *ioHandlePtrPtr = (FsHandleHeader *)NIL;

    /*
     * Do a device open at the I/O server.  We set the ioFileID type so
     * that the local device client open procedure gets called at the I/O
     * server, as opposed to the local pipe (or whatever) open routine.
     * NAME note: are not passing the file name to the I/O server.
     */
    deviceStatePtr = (FsDeviceState *)streamData;
    ioFileIDPtr->type = FS_LCL_DEVICE_STREAM;
    status = FsDeviceRemoteOpen(ioFileIDPtr, *flagsPtr,	sizeof(FsDeviceState),
				(ClientData)deviceStatePtr);

    if (status == SUCCESS) {
	/*
	 * Install the handle and initialize its recovery state.
	 */
	register FsRecoveryInfo *recovPtr;
	FsRemoteIOHandle *rmtHandlePtr;

	ioFileIDPtr->type = FS_RMT_DEVICE_STREAM;
	found = FsHandleInstall(ioFileIDPtr, sizeof(FsRemoteIOHandle), name,
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
    Mem_Free((Address)deviceStatePtr);
    return(status);
}

/*----------------------------------------------------------------------
 *
 * FsDeviceRemoteOpen --
 *
 *	Client side stub to open a remote device, remote named pipe,
 *	or remote pseudo stream.  Uses the RPC_FS_DEV_OPEN remote
 *	procedure call to invoke the pipe, device, or pseudo device
 *	open routine on the I/O server.  We are given an ioFileID from
 *	the name server, although we just use the sererID part here.
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
FsDeviceRemoteOpen(ioFileIDPtr, useFlags, inSize, inBuffer)
    FsFileID	*ioFileIDPtr;	/* Indicates I/O server.  This is modified
				 * by the I/O server and returned to our
				 * caller for use in the dev/pipe/etc handle */
    int		useFlags;	/* FS_READ | FS_WRITE ... */
    int		inSize;		/* Size of input parameters */
    ClientData	inBuffer;	/* Input data for remote server */
{
    ReturnStatus	status;		/* Return code from RPC */
    Rpc_Storage		storage;	/* Specifies inputs/outputs to RPC */
    FsDeviceRemoteOpenPrm param;

    param.fileID = *ioFileIDPtr;
    param.useFlags = useFlags;
    param.dataSize = inSize;
    if (inSize > 0) {
	param.openData = *((FsUnionData *) inBuffer);	/* copy data */
    }
    storage.requestParamPtr = (Address) &param;
    storage.requestParamSize = sizeof(FsDeviceRemoteOpenPrm);
    storage.requestDataPtr = (Address) NIL;
    storage.requestDataSize = 0;
    storage.replyParamPtr = (Address) ioFileIDPtr;
    storage.replyParamSize = sizeof(FsFileID);
    storage.replyDataPtr = (Address) NIL;
    storage.replyDataSize = 0;

    status = Rpc_Call(ioFileIDPtr->serverID, RPC_FS_DEV_OPEN, &storage);

    if (status == SUCCESS) {
	/*
	 * Register a callback with the recovery module.  This ensures that
	 * someone is paying attention to the I/O server and the filesystem
	 * will get called back when the I/O server reboots.
	 */
	Recov_RebootRegister(ioFileIDPtr->serverID, FsReopen, (ClientData)NIL);
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_RpcDevOpen --
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
Fs_RpcDevOpen(srvToken, clientID, command, storagePtr)
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
    FsHandleHeader	*hdrPtr;	/* I/O handle created by type-specific
					 * open routine. */
    ReturnStatus	status;
    FsDeviceRemoteOpenPrm *paramPtr;
    register int	dataSize;
    ClientData		streamData;

    /*
     * Call the client-open routine to set up an I/O handle here on the
     * I/O server for the device.  This is either the device, pipe, or
     * named pipe open routine.  We allocate storage and copy the stream
     * data so the CltOpen routine can free it, as it expects to do.
     * NAME note: we don't have a name for the device here.
     */
    paramPtr = (FsDeviceRemoteOpenPrm *) storagePtr->requestParamPtr;
    dataSize = paramPtr->dataSize;
    if (dataSize > 0) {
	streamData = (ClientData)Mem_Alloc(dataSize);
	Byte_Copy(dataSize, (Address)&paramPtr->openData, (Address)streamData);
    } else {
	streamData = (ClientData)NIL;
    }
    status = (fsStreamOpTable[paramPtr->fileID.type].cltOpen)
		    (&paramPtr->fileID, &paramPtr->useFlags,
		     clientID, streamData, (char *)NIL, &hdrPtr);
    if (status == SUCCESS) {
	/*
	 * Return the fileID to the other host so it can
	 * set up its own I/O handle.
	 */
	storagePtr->replyParamPtr = (Address)&hdrPtr->fileID;
	storagePtr->replyParamSize = sizeof(FsFileID);
    }
    Rpc_Reply(srvToken, status, storagePtr, (int (*)())NIL, (ClientData)NIL);
    return(SUCCESS);	/* So that higher level doesn't send error reply */
}

/*
 *----------------------------------------------------------------------
 *
 * FsDeviceClose --
 *
 *	Close a stream to a device.  We just need to clean up our state with
 *	the device driver.  The file server that named the device file keeps
 *	no state about us, so we don't have to contact it.
 *
 * FIX ME: need to write back access/modify times to name server
 *	BUT, have no handle for the name server.
 *
 * Results:
 *	SUCCESS.
 *
 * Side effects:
 *	Calls the device type close routine.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
FsDeviceClose(streamPtr, clientID, procID, flags, size, data)
    Fs_Stream		*streamPtr;	/* Stream to device */
    int			clientID;	/* HostID of client closing */
    Proc_PID		procID;		/* ID of closing process */
    int			flags;		/* Flags from the stream being closed */
    int			size;		/* Should be zero */
    ClientData		data;		/* IGNORED */
{
    ReturnStatus		status;
    register FsDeviceIOHandle	*devHandlePtr =
	    (FsDeviceIOHandle *)streamPtr->ioHandlePtr;
    Boolean			cache = FALSE;

    if (!FsIOClientClose(&devHandlePtr->clientList, clientID, flags, &cache)) {
	Sys_Panic(SYS_WARNING,
		  "FsDeviceClose, client %d unknown for device <%d,%d>\n",
		  clientID, devHandlePtr->hdr.fileID.major,
		  devHandlePtr->hdr.fileID.minor);
	FsHandleRelease(devHandlePtr, TRUE);
	return(FS_STALE_HANDLE);
    }
    /*
     * Decrement use counts.
     */
    FsLockClose(&devHandlePtr->lock, procID, &streamPtr->hdr.fileID);

    devHandlePtr->use.ref--;
    if (flags & FS_WRITE) {
	devHandlePtr->use.write--;
    }
    /*
     * Call the driver's close routine to clean up.
     */
    status = (*devFsOpTable[devHandlePtr->device.type].close)
		(&devHandlePtr->device, flags, devHandlePtr->use.ref,
			devHandlePtr->use.write);

    if (devHandlePtr->use.ref < 0 || devHandlePtr->use.write < 0) {
	Sys_Panic(SYS_FATAL, "FsDeviceClose <%d,%d> ref %d, write %d\n",
	    devHandlePtr->hdr.fileID.major, devHandlePtr->hdr.fileID.minor,
	    devHandlePtr->use.ref, devHandlePtr->use.write);
    }
    /*
     * We don't bother to remove the handle here if the device isn't
     * being used.  Instead we let the handle get scavenged.
     */
    FsHandleRelease(devHandlePtr, TRUE);

    return(status);
}

/*
 * ----------------------------------------------------------------------------
 *
 * FsDeviceClientKill --
 *
 *	Called when a client is assumed down.  This cleans up the
 *	references due to the client.
 *	
 *
 * Results:
 *	SUCCESS.
 *
 * Side effects:
 *	Removes the client list entry for the client and adjusts the
 *	use counts on the file.  This unlocks the handle.
 *
 * ----------------------------------------------------------------------------
 *
 */
/*ARGSUSED*/
void
FsDeviceClientKill(hdrPtr, clientID)
    FsHandleHeader	*hdrPtr;	/* File to clean up */
    int			clientID;	/* Host assumed down */
{
    register FsDeviceIOHandle *devHandlePtr = (FsDeviceIOHandle *)hdrPtr;
    register int flags;
    int refs, writes, execs;

    /*
     * Remove the client from the list of users, and see what it was doing.
     */
    FsIOClientKill(&devHandlePtr->clientList, clientID, &refs, &writes, &execs);

    FsLockClientKill(&devHandlePtr->lock, clientID);

    if (refs > 0) {
	/*
	 * Set up flags to emulate a close by the client.
	 */
	flags = FS_READ;
	if (writes) {
	    flags |= FS_WRITE;
	}
	/*
	 * Decrement use counts and call the driver close routine.
	 */
	devHandlePtr->use.ref -= refs;
	if (flags & FS_WRITE) {
	    devHandlePtr->use.write -= writes;
	}
	(void)(*devFsOpTable[devHandlePtr->device.type].close)
		(&devHandlePtr->device, flags, devHandlePtr->use.ref,
		    devHandlePtr->use.write);
    
	if (devHandlePtr->use.ref < 0 || devHandlePtr->use.write < 0) {
	    Sys_Panic(SYS_FATAL, "FsDeviceClose <%d,%d> ref %d, write %d\n",
		hdrPtr->fileID.major, hdrPtr->fileID.minor,
		devHandlePtr->use.ref, devHandlePtr->use.write);
	}
    }
    FsHandleUnlock(devHandlePtr);
}

/*
 *----------------------------------------------------------------------
 *
 * FsRemoteIOClose --
 *
 *	Close a stream to a remote device/pipe.  We just need to clean up our
 *	connection to the I/O server.  (The file server that named the
 *	device file keeps no state about us, so we don't have to contact it.)
 *	We make an RPC to the I/O server which invokes close routine there.
 *	We also update our own use counts.
 *
 * FIX ME: need to write back access/modify times to name server
 *
 * Results:
 *	SUCCESS.
 *
 * Side effects:
 *	RPC to the I/O server to invoke FsDeviceClose/FsPipeClose.
 *	Release the remote I/O handle.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
FsRemoteIOClose(streamPtr, clientID, procID, flags, dataSize, closeData)
    Fs_Stream		*streamPtr;	/* Stream to remote object */
    int			clientID;	/* ID of closing host */
    Proc_PID		procID;		/* ID of closing process */
    int			flags;		/* Flags from the stream being closed */
    int			dataSize;	/* Size of *closeData, or Zero */
    ClientData		closeData;	/* Copy of cached I/O attributes. */
{
    ReturnStatus		status;
    register FsRemoteIOHandle *rmtHandlePtr =
	    (FsRemoteIOHandle *)streamPtr->ioHandlePtr;

    /*
     * Decrement local references.
     */
    rmtHandlePtr->recovery.use.ref--;
    if (flags & FS_WRITE) {
	rmtHandlePtr->recovery.use.write--;
    }

    if (rmtHandlePtr->recovery.use.ref < 0 ||
	rmtHandlePtr->recovery.use.write < 0) {
	Sys_Panic(SYS_FATAL, "FsRemoteIOClose: <%d,%d> ref %d write %d\n",
	    rmtHandlePtr->hdr.fileID.major, rmtHandlePtr->hdr.fileID.minor,
	    rmtHandlePtr->recovery.use.ref,
	    rmtHandlePtr->recovery.use.write);
    }

    if (!FsHandleValid(streamPtr->ioHandlePtr)) {
	status = FS_STALE_HANDLE;
    } else {
	status = FsSpriteClose(streamPtr, clientID, procID, flags,
			       dataSize, closeData);
    }
    /*
     * HAVE TO HANDLE RECOVERY CORRECTLY.  SpriteClose has marked the handle.
     *
     * Check the number of users with the handle still locked, then
     * remove the handle if we aren't using it anymore.
     */
    if (status == SUCCESS && rmtHandlePtr->recovery.use.ref == 0) {
	FsHandleRelease(rmtHandlePtr, TRUE);
	FsHandleRemove(rmtHandlePtr);
    } else {
	FsHandleRelease(rmtHandlePtr, TRUE);
    }
    return(status);
}

/*
 * ----------------------------------------------------------------------------
 *
 * FsDeviceScavenge --
 *
 *	Called periodically to see if this handle is still needed.
 *	
 *
 * Results:
 *	SUCCESS.
 *
 * Side effects:
 *	Removes the handle if it isn't referenced anymore and there
 *	are no remote clients.
 *
 * ----------------------------------------------------------------------------
 *
 */
void
FsDeviceScavenge(hdrPtr)
    FsHandleHeader	*hdrPtr;	/* File to clean up */
{
    register FsDeviceIOHandle *handlePtr = (FsDeviceIOHandle *)hdrPtr;

    if (handlePtr->use.ref == 0) {
	/*
	 * Remove handles for devices with no users.
	 */
	FsWaitListDelete(&handlePtr->readWaitList);
	FsWaitListDelete(&handlePtr->writeWaitList);
	FsWaitListDelete(&handlePtr->exceptWaitList);
	FsHandleRemove(handlePtr);
    } else {
        FsHandleUnlock(handlePtr);
    }
}

/*
 * Parameters for a device reopen RPC used to reestablish state on
 * the I/O server for a device.
 */
typedef struct FsRmtDeviceReopenParams {
    FsFileID	fileID;		/* File ID of file to reopen. */
    int		openCount;	/* Number of times we have the device open. */
    int		writerCount;	/* Number of writers we have. */
} FsRmtDeviceReopenParams;

/*
 *----------------------------------------------------------------------
 *
 * FsRmtDeviceReopen --
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
FsRmtDeviceReopen(hdrPtr, inData, outSizePtr, outDataPtr)
    FsHandleHeader	*hdrPtr;	/* Remove device I/O handle to reopen */
    ClientData		inData;		/* IGNORED */
    int			*outSizePtr;	/* Size of returned data, 0 here */
    ClientData		*outDataPtr;	/* Returned data, NIL here */
{
    register FsRemoteIOHandle	*handlePtr = (FsRemoteIOHandle *)hdrPtr;
    ReturnStatus		status;
    FsRmtDeviceReopenParams	reopenParams;
    Rpc_Storage			storage;

    reopenParams.fileID = handlePtr->hdr.fileID;
    reopenParams.openCount = handlePtr->recovery.use.ref;
    reopenParams.writerCount = handlePtr->recovery.use.write;

    /*
     * Set up for the RPC.
     */
    storage.requestParamPtr = (Address) &reopenParams;
    storage.requestParamSize = sizeof(FsRmtDeviceReopenParams);
    storage.requestDataPtr = (Address) NIL;
    storage.requestDataSize = 0;
    storage.replyParamPtr = (Address) NIL;
    storage.replyParamSize = 0;
    storage.replyDataPtr = (Address) NIL;
    storage.replyDataSize = 0;

    status = Rpc_Call(hdrPtr->fileID.serverID, RPC_FS_DEV_REOPEN, &storage);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_RpcDevReopen --
 *
 *	This host is the IO server for a handle.  This message from the
 *	remote host indicates that a client process is reopening the handle.
 *	This adds that client to the handle's client list.  File type open
 *	routines will be called as necessary to reopen the file.
 *
 * Results:
 *	If this procedure returns SUCCESS then a reply has been sent to
 *	the client.  If the arguments are bad then an error is 
 *	returned and the main level sends back an error reply.
 *
 * Side effects:
 *	Set up the handle and call the file type remote open routine.
 *	The reference count on the handle is handled the same as for
 *	other open's; there is one reference and writer count
 *	for each remote open of the file.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
Fs_RpcDevReopen(srvToken, clientID, command, storagePtr)
    ClientData 		 srvToken;	/* Handle on server process passed to
				 	 * Rpc_Reply */
    int 		 clientID;	/* Sprite ID of client host */
    int 		 command;	/* Command identifier */
    Rpc_Storage		 *storagePtr;	/* The request fields refer to the 
					 * request buffers and also indicate 
					 * the exact amount of data in the 
					 * request buffers.  The reply fields 
					 * are initialized to NIL for the
					 * pointers and 0 for the lengths.  
					 * This can be passed to Rpc_Reply */
{
    return(FAILURE);
}

/*
 * ----------------------------------------------------------------------------
 *
 * FsDeviceMigStart --
 *
 *	Begin migration of a FS_LCL_DEVICE_STREAM.  There is no extra
 *	state that needs saving, but we do release a reference to the I/O
 *	handle.
 *	
 * Results:
 *	SUCCESS.
 *
 * Side effects:
 *	Release the I/O handle.
 *
 * ----------------------------------------------------------------------------
 *
 */
/*ARGSUSED*/
ReturnStatus
FsDeviceMigStart(hdrPtr, flags, clientID, data)
    FsHandleHeader *hdrPtr;	/* File being encapsulated */
    int flags;			/* Use flags from the stream */
    int clientID;		/* Host doing the encapsulation */
    ClientData data;		/* Buffer we fill in */
{
    if ((flags & FS_RMT_SHARED) == 0) {
	FsHandleRelease(hdrPtr, FALSE);
    }
    return(SUCCESS);
}


/*
 * ----------------------------------------------------------------------------
 *
 * FsRemoteIOMigStart --
 *
 *	Begin migration of a remote stream.  There is no extra
 *	state that needs saving, but we do decrement our recovery related
 *	use counts (because the stream is moving away), and we release our
 *	reference to the I/O handle.
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
FsRemoteIOMigStart(hdrPtr, flags, clientID, migFlagsPtr)
    FsHandleHeader *hdrPtr;	/* File being encapsulated */
    int flags;			/* Use flags from the stream */
    int clientID;		/* Host doing the encapsulation */
    int *migFlagsPtr;		/* Migration flags we may modify */
{
    register FsRemoteIOHandle *rmtHandlePtr = (FsRemoteIOHandle *)hdrPtr;

    FsHandleLock(rmtHandlePtr);
    if ((flags & FS_RMT_SHARED) == 0) {
	rmtHandlePtr->recovery.use.ref--;
	if (flags & FS_WRITE) {
	    rmtHandlePtr->recovery.use.write--;
	    if (rmtHandlePtr->recovery.use.write == 0) {
		*migFlagsPtr |= FS_LAST_WRITER;
	    }
	}
	if (flags & FS_EXECUTE) {
	    rmtHandlePtr->recovery.use.exec--;
	}
	FsHandleRelease(rmtHandlePtr, TRUE);
    }

    return(SUCCESS);
}

/*
 * ----------------------------------------------------------------------------
 *
 * FsDeviceMigrate --
 *
 *	This takes care of transfering references from one client to the other.
 *	A useful side-effect of this routine is	to properly set the type in
 *	the ioFileID, either FS_LCL_DEVICE_STREAM or FS_RMT_DEVICE_STREAM.
 *	In the latter case FsRmtDevoceMigrate is called to do all the work.
 *
 * Results:
 *	An error status if the I/O handle can't be set-up.
 *	Otherwise SUCCESS is returned, *flagsPtr may have the FS_RMT_SHARED
 *	bit set, and *sizePtr and *dataPtr are set to reference FsDeviceState.
 *
 * Side effects:
 *	Sets the correct stream type on the ioFileID.
 *	Shifts client references from the srcClient to the destClient.
 *	Set up and return FsDeviceState for use by the MigEnd routine.
 *
 * ----------------------------------------------------------------------------
 *
 */
/*ARGSUSED*/
ReturnStatus
FsDeviceMigrate(migInfoPtr, dstClientID, flagsPtr, offsetPtr, sizePtr, dataPtr)
    FsMigInfo	*migInfoPtr;	/* Migration state */
    int		dstClientID;	/* ID of target client */
    int		*flagsPtr;	/* In/Out Stream usage flags */
    int		*offsetPtr;	/* Return - new stream offset */
    int		*sizePtr;	/* Return - sizeof(FsDeviceState) */
    Address	*dataPtr;	/* Return - pointer to FsDeviceState */
{
    FsDeviceIOHandle			*devHandlePtr;

    if (migInfoPtr->ioFileID.serverID != rpc_SpriteID) {
	/*
	 * The device was local, which is why we were called, but is now remote.
	 */
	migInfoPtr->ioFileID.type = FS_RMT_DEVICE_STREAM;
	return(FsRmtDeviceMigrate(migInfoPtr, dstClientID, flagsPtr, offsetPtr,
		sizePtr, dataPtr));
    }
    migInfoPtr->ioFileID.type = FS_LCL_DEVICE_STREAM;
    if (!FsDeviceHandleInit(&migInfoPtr->ioFileID, (char *)NIL, &devHandlePtr)){
	Sys_Panic(SYS_WARNING,
		"FsDeviceMigrate, I/O handle <%d,%d> not found\n",
		 migInfoPtr->ioFileID.major, migInfoPtr->ioFileID.minor);
	return(FS_FILE_NOT_FOUND);
    }
    /*
     * At the stream level, add the new client to the set of clients
     * for the stream, and check for any cross-network stream sharing.
     */
    FsStreamMigClient(&migInfoPtr->streamID, migInfoPtr->srcClientID,
			dstClientID, (FsHandleHeader *)devHandlePtr,
			&migInfoPtr->offset, &migInfoPtr->flags);
    /*
     * Adjust use counts on the I/O handle to reflect any new sharing.
     */
    FsMigrateUseCounts(migInfoPtr->flags, &devHandlePtr->use);

    /*
     * Move the client at the I/O handle level.
     */
    FsIOClientMigrate(&devHandlePtr->clientList, migInfoPtr->srcClientID,
			dstClientID, migInfoPtr->flags);

    *sizePtr = 0;
    *dataPtr = (Address)NIL;
    *flagsPtr = migInfoPtr->flags;
    *offsetPtr = migInfoPtr->offset;
    /*
     * We don't need this reference on the I/O handle; there is no change.
     */
    FsHandleRelease(devHandlePtr, TRUE);
    return(SUCCESS);
}

/*
 * ----------------------------------------------------------------------------
 *
 * FsRmtDeviceMigrate --
 *
 *	This takes care of transfering references from one client to the other.
 *	A useful side-effect of this routine is	to properly set the type in
 *	the ioFileID, either FS_LCL_DEVICE_STREAM or FS_RMT_DEVICE_STREAM.
 *	In the latter case FsDevoceMigrate is called to do all the work.
 *
 * Results:
 *	An error status if the I/O handle can't be set-up.
 *	Otherwise SUCCESS is returned, *flagsPtr may have the FS_RMT_SHARED
 *	bit set, and *sizePtr and *dataPtr are set to reference FsDeviceState.
 *
 * Side effects:
 *	Sets the correct stream type on the ioFileID.
 *	Shifts client references from the srcClient to the destClient.
 *	Set up and return FsDeviceState for use by the MigEnd routine.
 *
 * ----------------------------------------------------------------------------
 *
 */
/*ARGSUSED*/
ReturnStatus
FsRmtDeviceMigrate(migInfoPtr, dstClientID, flagsPtr, offsetPtr, sizePtr, dataPtr)
    FsMigInfo	*migInfoPtr;	/* Migration state */
    int		dstClientID;	/* ID of target client */
    int		*flagsPtr;	/* In/Out Stream usage flags */
    int		*offsetPtr;	/* Return - the new stream offset */
    int		*sizePtr;	/* Return - sizeof(FsDeviceState) */
    Address	*dataPtr;	/* Return - pointer to FsDeviceState */
{
    register ReturnStatus		status;

    if (migInfoPtr->ioFileID.serverID == rpc_SpriteID) {
	/*
	 * The device was remote, which is why we were called, but is now local.
	 */
	migInfoPtr->ioFileID.type = FS_LCL_DEVICE_STREAM;
	return(FsDeviceMigrate(migInfoPtr, dstClientID, flagsPtr, offsetPtr,
		sizePtr, dataPtr));
    }
    migInfoPtr->ioFileID.type = FS_RMT_DEVICE_STREAM;
    status = FsNotifyOfMigration(migInfoPtr, flagsPtr, offsetPtr,
				0, (Address)NIL);
    if (status != SUCCESS) {
	Sys_Panic(SYS_WARNING, "FsRmtDeviceMigrate, server error <%x>\n",
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
 * FsDeviceMigEnd --
 *
 *	Complete setup of a FS_RMT_DEVICE_STREAM after migration.
 *	The srvMigrate routine has done most all the work.
 *	We just grab a reference on the I/O handle for the stream.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 *
 */
/*ARGSUSED*/
ReturnStatus
FsDeviceMigEnd(migInfoPtr, size, data, hdrPtrPtr)
    FsMigInfo	*migInfoPtr;	/* Migration state */
    int		size;		/* Zero */
    ClientData	data;		/* NIL */
    FsHandleHeader **hdrPtrPtr;	/* Return - handle for the file */
{
    register FsDeviceIOHandle *devHandlePtr;

    devHandlePtr = FsHandleFetchType(FsDeviceIOHandle, &migInfoPtr->ioFileID);
    if (devHandlePtr == (FsDeviceIOHandle *)NIL) {
	Sys_Panic(SYS_FATAL, "FsDeviceMigEnd, no handlel\n");
	return(FAILURE);
    } else {
	FsHandleUnlock(devHandlePtr);
	*hdrPtrPtr = (FsHandleHeader *)devHandlePtr;
	return(SUCCESS);
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 * FsRemoteIOMigEnd --
 *
 *	Create a FS_RMT_DEVICE_STREAM or FS_RMT_PIPE_STREAM after migration.
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
FsRemoteIOMigEnd(migInfoPtr, size, data, hdrPtrPtr)
    FsMigInfo	*migInfoPtr;	/* Migration state */
    int		size;		/* Zero */
    ClientData	data;		/* NIL */
    FsHandleHeader **hdrPtrPtr;	/* Return - handle for the file */
{
    register FsRemoteIOHandle *rmtHandlePtr;
    register FsRecoveryInfo *recovPtr;
    Boolean found;

    found = FsHandleInstall(&migInfoPtr->ioFileID, sizeof(FsRemoteIOHandle),
		(char *)NIL, hdrPtrPtr);
    rmtHandlePtr = (FsRemoteIOHandle *)*hdrPtrPtr;
    recovPtr = &rmtHandlePtr->recovery;
    if (!found) {
	FsRecoveryInit(recovPtr);
    }
    recovPtr->use.ref++;
    if (migInfoPtr->flags & FS_WRITE) {
	recovPtr->use.write++;
    }
    FsHandleUnlock(rmtHandlePtr);
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * FsRmtDeviceVerify --
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
 *	It should be released with FsHandleRelease.
 *
 *----------------------------------------------------------------------
 */

FsHandleHeader *
FsRmtDeviceVerify(fileIDPtr, clientID, domainTypePtr)
    FsFileID	*fileIDPtr;	/* Client's I/O file ID */
    int		clientID;	/* Host ID of the client */
    int		*domainTypePtr;	/* Return - FS_LOCAL_DOMAIN */
{
    register FsDeviceIOHandle	*devHandlePtr;
    register FsClientInfo	*clientPtr;
    Boolean			found = FALSE;

    fileIDPtr->type = FS_LCL_DEVICE_STREAM;
    devHandlePtr = FsHandleFetchType(FsDeviceIOHandle, fileIDPtr);
    if (devHandlePtr != (FsDeviceIOHandle *)NIL) {
	LIST_FORALL(&devHandlePtr->clientList, (List_Links *) clientPtr) {
	    if (clientPtr->clientID == clientID) {
		found = TRUE;
		break;
	    }
	}
	if (!found) {
	    FsHandleRelease(devHandlePtr, TRUE);
	    devHandlePtr = (FsDeviceIOHandle *)NIL;
	}
    }
    if (!found) {
	Sys_Panic(SYS_WARNING,
	    "FsRmtDeviceVerify, client %d not known for device <%d,%d>\n",
	    clientID, fileIDPtr->major, fileIDPtr->minor);
    }
    if (domainTypePtr != (int *)NIL) {
	*domainTypePtr = FS_LOCAL_DOMAIN;
    }
    return((FsHandleHeader *)devHandlePtr);
}

/*
 *----------------------------------------------------------------------
 *
 * FsDeviceRead --
 *
 *      Read on a stream connected to a local peripheral device.
 *	This branches to the driver routine after setting up buffers.
 *	This is called from Fs_Read and from Fs_RpcRead.
 *
 * Results:
 *	SUCCESS unless there was an address error or I/O error.
 *
 * Side effects:
 *	Read the device.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
FsDeviceRead(streamPtr, flags, buffer, offsetPtr, lenPtr, remoteWaitPtr)
    Fs_Stream		*streamPtr;	/* Open stream to the device. */
    int			flags;		/* Flags from the stream struct. */
    register Address	buffer;		/* Buffer to fill with file data */
    int 		*offsetPtr;	/* In/Out byte offset */
    int 		*lenPtr;	/* In/Out byte count */
    Sync_RemoteWaiter	*remoteWaitPtr;	/* Process info for remote waiting */
{
    register FsDeviceIOHandle	*devHandlePtr =
	    (FsDeviceIOHandle *)streamPtr->ioHandlePtr;
    register ReturnStatus status;
    register Address	readBuffer;
    register Fs_Device	*devicePtr;
    register int	length = *lenPtr;

    FsHandleLock(devHandlePtr);
    /*
     * Because the read could take a while and we aren't mapping in
     * buffers, we have to allocate an extra buffer here so the
     * buffer address is valid when the device's interrupt handler
     * does its DMA.
     */
    if (flags & FS_USER) {
        readBuffer = Mem_Alloc(length);
    } else {
        readBuffer = buffer;
    }

    /*
     * Put the process onto the read-wait list before attempting the read.
     * This is to prevent races with the device's notification which
     * happens from an interrupt handler.
     */
    FsWaitListInsert(&devHandlePtr->readWaitList, remoteWaitPtr);
    devicePtr = &devHandlePtr->device;
    status = (*devFsOpTable[devicePtr->type].read)(devicePtr,
		*offsetPtr, length, readBuffer, lenPtr);
    length = *lenPtr;
    if (flags & FS_USER) {
        if (Vm_CopyOut(length, readBuffer, buffer) != SUCCESS) {
	    if (status == SUCCESS) {
		status = SYS_ARG_NOACCESS;
	    }
	}
	Mem_Free(readBuffer);
    }
    if (status != FS_WOULD_BLOCK) {
	FsWaitListRemove(&devHandlePtr->readWaitList, remoteWaitPtr);
    }
    devHandlePtr->accessTime = fsTimeInSeconds;
    *offsetPtr += length;
    fsStats.gen.deviceBytesRead += length;
    FsHandleUnlock(devHandlePtr);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsDeviceWrite --
 *
 *      Write on a stream connected to a local peripheral device.
 *	This is called from Fs_Write and Fs_RpcWrite.
 *
 * Results:
 *	SUCCESS unless there was an address error or I/O error.
 *
 * Side effects:
 *	Write to the device.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
FsDeviceWrite(streamPtr, flags, buffer, offsetPtr, lenPtr, remoteWaitPtr)
    Fs_Stream		*streamPtr;	/* Open stream to the device. */
    int			flags;		/* Flags from the stream struct. */
    register Address	buffer;		/* Buffer to fill with file data */
    int 		*offsetPtr;	/* In/Out byte offset */
    int 		*lenPtr;	/* In/Out byte count */
    Sync_RemoteWaiter	*remoteWaitPtr;	/* Process info for remote waiting */
{
    register FsDeviceIOHandle	*devHandlePtr =
	    (FsDeviceIOHandle *)streamPtr->ioHandlePtr;
    ReturnStatus	status = SUCCESS;
    register Address	writeBuffer;
    register Fs_Device	*devicePtr = &devHandlePtr->device;
    register int	length = *lenPtr;

    FsHandleLock(devHandlePtr);
    /*
     * Because the write could take a while and we aren't mapping in
     * buffers, we have to allocate an extra buffer here so the
     * buffer address is valid when the device's interrupt handler
     * does its DMA.
     */
    if (flags & FS_USER) {
        writeBuffer = Mem_Alloc(length);
	if (Vm_CopyIn(length, buffer, writeBuffer) != SUCCESS) {
	    status = SYS_ARG_NOACCESS;
	}
    } else {
        writeBuffer = buffer;
	status = SUCCESS;
    }
    if (status == SUCCESS) {
	/*
	 * Put the process onto the write-wait list before attempting the write.
	 * This is to prevent races with the device's notification which
	 * happens from an interrupt handler.
	 */
	FsWaitListInsert(&devHandlePtr->writeWaitList, remoteWaitPtr);
	status = (*devFsOpTable[devicePtr->type].write)(devicePtr,
		*offsetPtr, length, writeBuffer, lenPtr);
	length = *lenPtr;
	if (status != FS_WOULD_BLOCK) {
	    FsWaitListRemove(&devHandlePtr->readWaitList, remoteWaitPtr);
	}
	devHandlePtr->modifyTime = fsTimeInSeconds;
	*offsetPtr += length;
	fsStats.gen.deviceBytesWritten += length;
    }

    if (flags & FS_USER) {
	Mem_Free(writeBuffer);
    }
    FsHandleUnlock(devHandlePtr);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsDeviceSelect --
 *
 *      Select on a stream connected to a local peripheral device.  This
 *	ensures that the calling process is on a waiting list, then calls
 *	the device driver's select routine.  If the select succeeds, then
 *	the wait list items are removed.  The ordering of this is done to
 *	prevent races between the select routine and the notification that
 *	occurs at interrupt time.
 *
 * Results:
 *	A return from the driver, should be SUCCESS unless the
 *	device is offline or something.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
FsDeviceSelect(hdrPtr, waitPtr, readPtr, writePtr, exceptPtr)
    FsHandleHeader	*hdrPtr;	/* Handle on device to select */
    Sync_RemoteWaiter	*waitPtr;	/* Process info for remote waiting */
    int 		*readPtr;	/* Bit to clear if non-readable */
    int 		*writePtr;	/* Bit to clear if non-writeable */
    int 		*exceptPtr;	/* Bit to clear if non-exceptable */
{
    register FsDeviceIOHandle *devHandlePtr = (FsDeviceIOHandle *)hdrPtr;
    register Fs_Device	*devicePtr = &devHandlePtr->device;
    register ReturnStatus status;
    register int inFlags;		/* Until we fix the device drivers */
    int outFlags = 0;

    FsHandleLock(devHandlePtr);
    inFlags = 0;
    if (*readPtr) {
	inFlags |= FS_READABLE;
	FsWaitListInsert(&devHandlePtr->readWaitList, waitPtr);
    }
    if (*writePtr) {
	inFlags |= FS_WRITABLE;
	FsWaitListInsert(&devHandlePtr->writeWaitList, waitPtr);
    }
    if (*exceptPtr) {
	inFlags |= FS_EXCEPTION;
	FsWaitListInsert(&devHandlePtr->exceptWaitList, waitPtr);
    }
    status = (*devFsOpTable[devicePtr->type].select)(devicePtr,
		    inFlags, &outFlags);
    if ((outFlags & FS_READABLE) == 0) {
	*readPtr = 0;
    } else if (*readPtr != 0) {
	FsWaitListRemove(&devHandlePtr->readWaitList, waitPtr);
    }
    if ((outFlags & FS_WRITABLE) == 0) {
	*writePtr = 0;
    } else if (*writePtr != 0) {
	FsWaitListRemove(&devHandlePtr->writeWaitList, waitPtr);
    }
    if ((outFlags & FS_EXCEPTION) == 0) {
	*exceptPtr = 0;
    } else if (*exceptPtr != 0) {
	FsWaitListRemove(&devHandlePtr->exceptWaitList, waitPtr);
    }
    FsHandleUnlock(devHandlePtr);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsDeviceIOControl --
 *
 *      Write on a stream connected to a peripheral device.  Called from
 *	FsDomainWrite.
 *
 * Results:
 *	SUCCESS unless there was an address error or I/O error.
 *
 * Side effects:
 *	Write to the device.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
FsDeviceIOControl(streamPtr, command, byteOrder, inBufPtr, outBufPtr)
    Fs_Stream *streamPtr;		/* Stream to a device. */
    int command;			/* Device specific I/O control */
    int byteOrder;			/* Client's byte order */
    Fs_Buffer *inBufPtr;		/* Buffer of input arguments */
    Fs_Buffer *outBufPtr;		/* Buffer for return parameters */
{
    register FsDeviceIOHandle *devHandlePtr =
	    (FsDeviceIOHandle *)streamPtr->ioHandlePtr;
    register Fs_Device	*devicePtr = &devHandlePtr->device;
    register ReturnStatus status;
    static Boolean warned = FALSE;

    FsHandleLock(devHandlePtr);
    switch (command) {
	case IOC_LOCK:
	case IOC_UNLOCK:
	    status = FsIocLock(&devHandlePtr->lock, command, byteOrder,
				inBufPtr, &streamPtr->hdr.fileID);
	    break;
	default:
	    if ((byteOrder != mach_ByteOrder) && !warned) {
		warned = TRUE;
		FsFileError(streamPtr->ioHandlePtr,
		    "Device I/O control byte swapping not done",
		    SUCCESS);
	    }
	    status = (*devFsOpTable[devicePtr->type].ioControl) (devicePtr,
		    command, inBufPtr->size, inBufPtr->addr,
		    outBufPtr->size, outBufPtr->addr);
	    break;
    }
    FsHandleUnlock(devHandlePtr);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsDeviceGetIOAttr --
 *
 *      Get the I/O attributes for a device.  A copy of the access and
 *	modify times are kept at the I/O server.  This routine is called
 *	either from Fs_GetAttrStream or Fs_RpcGetIOAttr to update
 *	the initial copy of the attributes obtained from the name server.
 *
 * Results:
 *	SUCCESS.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
FsDeviceGetIOAttr(fileIDPtr, clientID, attrPtr)
    FsFileID			*fileIDPtr;	/* FileID of device */
    int 			clientID;	/* IGNORED */
    register Fs_Attributes	*attrPtr;	/* Attributes to update */
{
    register FsDeviceIOHandle *devHandlePtr;

    devHandlePtr = FsHandleFetchType(FsDeviceIOHandle, fileIDPtr);
    if (devHandlePtr != (FsDeviceIOHandle *)NIL) {
	if (devHandlePtr->accessTime > attrPtr->accessTime.seconds) {
	    attrPtr->accessTime.seconds = devHandlePtr->accessTime;
	}
	if (devHandlePtr->modifyTime > attrPtr->dataModifyTime.seconds) {
	    attrPtr->dataModifyTime.seconds = devHandlePtr->modifyTime;
	}
	FsHandleRelease(devHandlePtr, TRUE);
    }
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * FsDeviceSetIOAttr --
 *
 *      Set the I/O attributes for a device.  A copy of the access and
 *	modify times are kept at the I/O server.  This routine is called
 *	either from Fs_SetAttrStream or Fs_RpcSetIOAttr to update
 *	the cached copy of the attributes.
 *
 * Results:
 *	SUCCESS.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
FsDeviceSetIOAttr(fileIDPtr, attrPtr, flags)
    FsFileID			*fileIDPtr;	/* FileID of device */
    register Fs_Attributes	*attrPtr;	/* Attributes to copy */
    int				flags;		/* What attrs to set */
{
    register FsDeviceIOHandle *devHandlePtr;

    if (flags & FS_SET_TIMES) {
	devHandlePtr = FsHandleFetchType(FsDeviceIOHandle, fileIDPtr);
	if (devHandlePtr != (FsDeviceIOHandle *)NIL) {
	    devHandlePtr->accessTime = attrPtr->accessTime.seconds;
	    devHandlePtr->modifyTime = attrPtr->dataModifyTime.seconds;
	    FsHandleRelease(devHandlePtr, TRUE);
	}
    }
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_NotifyReader --
 * Fs_DevNotifyReader --	this is a better name
 *
 *	Fs_NotifyReader is available to device driver interrupt handlers
 *	that need to notify waiting processes that the device is readable.
 *	It schedules a process level call to DevListNotifyReader, which
 *	in turn iterates down the list of handles for the device waking up
 *	all read waiters.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	Schedules a call to DevListNotify, which in turn calls
 *	FsWaitListNotify to schedule any waiting readers.
 *
 *----------------------------------------------------------------------
 */
void
Fs_NotifyReader(data)
    ClientData data;
{
    register	FsDeviceIOHandle *devHandlePtr = (FsDeviceIOHandle *)data;

    if ((devHandlePtr == (FsDeviceIOHandle *)NIL) ||
	(devHandlePtr->readNotifyScheduled)) {
	return;
    }
    devHandlePtr->readNotifyScheduled = TRUE;
    Proc_CallFunc(ReadNotify, (ClientData) devHandlePtr, 0);
}

static void
ReadNotify(data, callInfoPtr)
    ClientData		data;
    Proc_CallInfo	*callInfoPtr;
{
    register FsDeviceIOHandle *devHandlePtr = (FsDeviceIOHandle *)data;
    devHandlePtr->readNotifyScheduled = FALSE;
    FsWaitListNotify(&devHandlePtr->readWaitList);
    callInfoPtr->interval = 0;
}


/*
 *----------------------------------------------------------------------
 *
 * Fs_NotifyWriter --
 * Fs_DevNotifyWriter --	this is a better name
 *
 *	Fs_NotifyWriter is available to device driver interrupt handlers
 *	that need to notify waiting processes that the device is writeable.
 *	It schedules a process level call to Fs_WaitListNotifyStub on the
 *	devices's write wait list.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	Schedules a call to Fs_WaitListNotifyStub.
 *
 *----------------------------------------------------------------------
 */
void
Fs_NotifyWriter(data)
    ClientData data;
{
    register	FsDeviceIOHandle *devHandlePtr = (FsDeviceIOHandle *)data;

    if ((devHandlePtr == (FsDeviceIOHandle *)NIL) ||
	(devHandlePtr->writeNotifyScheduled)) {
	return;
    }
    devHandlePtr->writeNotifyScheduled = TRUE;
    Proc_CallFunc(WriteNotify, (ClientData) devHandlePtr, 0);
}

static void
WriteNotify(data, callInfoPtr)
    ClientData		data;
    Proc_CallInfo	*callInfoPtr;
{
    register FsDeviceIOHandle *devHandlePtr = (FsDeviceIOHandle *)data;
    devHandlePtr->writeNotifyScheduled = FALSE;
    FsWaitListNotify(&devHandlePtr->writeWaitList);
    callInfoPtr->interval = 0;
}



/*
 *----------------------------------------------------------------------
 *
 * Fs_DevNotifyException --
 *
 *	This is available to device driver interrupt handlers
 *	that need to notify waiting processes that there is an exception
 *	on the device.  This is only useful for processes waiting on
 *	exceptions in select.  This is not currently used.
 *	It schedules a process level call to Fs_WaitListNotifyStub on the
 *	devices's exception wait list.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	Schedules a call to Fs_WaitListNotifyStub.
 *
 *----------------------------------------------------------------------
 */
void
Fs_DevNotifyException(data)
    ClientData data;
{
    register	FsDeviceIOHandle *devHandlePtr = (FsDeviceIOHandle *)data;

    if (devHandlePtr == (FsDeviceIOHandle *)NIL) {
	return;
    }
    Proc_CallFunc(ExceptionNotify, (ClientData) devHandlePtr, 0);
}

static void
ExceptionNotify(data, callInfoPtr)
    ClientData		data;
    Proc_CallInfo	*callInfoPtr;
{
    register FsDeviceIOHandle *devHandlePtr = (FsDeviceIOHandle *)data;
    FsWaitListNotify(&devHandlePtr->exceptWaitList);
    callInfoPtr->interval = 0;
}


