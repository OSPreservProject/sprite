/* 
 * fsDevice.c --
 *
 *	Routines to manage devices.  
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


#include <sprite.h>

#include <fs.h>
#include <fsutil.h>
#include <fsconsist.h>
#include <fsio.h>
#include <fsNameOps.h>
#include <fsdm.h>
#include <fsioLock.h>
#include <fsprefix.h>
#include <fsrmt.h>
#include <dev.h>
#include <rpc.h>
#include <fsStat.h>
#include <fsioDevice.h>
#include <devFsOpTable.h>
#include <devTypes.h>

#include <stdio.h>

/*
 * INET is defined so a file server can be used to open the
 * special device file corresponding to a kernel-based ipServer
 */
#undef INET
#ifdef INET
#include <netInet.h>
/*
 * DEV_PLACEHOLDER_2	is defined in devTypesInt.h, which is not exported.
 *	This is an ugly hack, anyway, so we just define it here.  3/89
 *	The best solution is to define a new disk file type and not
 *	use Fsio_DeviceNameOpen at all.
 */
#define DEV_PLACEHOLDER_2	3
static int sockCounter = 0;
#endif

static void ReadNotify _ARGS_((ClientData data, Proc_CallInfo *callInfoPtr));
static void WriteNotify _ARGS_((ClientData data, Proc_CallInfo *callInfoPtr));
static void ExceptionNotify _ARGS_((ClientData data, 
				Proc_CallInfo *callInfoPtr));

extern ReturnStatus FsioDeviceCloseInt _ARGS_((
		Fsio_DeviceIOHandle *devHandlePtr, int useFlags, int refs, 
		int writes));



/*
 *----------------------------------------------------------------------
 *
 * FsioDeviceHandleInit --
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
FsioDeviceHandleInit(fileIDPtr, name, newHandlePtrPtr)
    Fs_FileID		*fileIDPtr;
    char		*name;
    Fsio_DeviceIOHandle	**newHandlePtrPtr;
{
    register Boolean found;
    register Fsio_DeviceIOHandle *devHandlePtr;

    found = Fsutil_HandleInstall(fileIDPtr, sizeof(Fsio_DeviceIOHandle), name,
		    FALSE, (Fs_HandleHeader **)newHandlePtrPtr);
    if (!found) {
	devHandlePtr = *newHandlePtrPtr;
	List_Init(&devHandlePtr->clientList);
	devHandlePtr->use.ref = 0;
	devHandlePtr->use.write = 0;
	devHandlePtr->use.exec = 0;
	devHandlePtr->device.serverID = fileIDPtr->serverID;
	devHandlePtr->device.type = fileIDPtr->major;
	devHandlePtr->device.unit = fileIDPtr->minor;
	devHandlePtr->device.data = (ClientData)NIL;
	devHandlePtr->flags = 0;
	Fsio_LockInit(&devHandlePtr->lock);
	devHandlePtr->modifyTime = 0;
	devHandlePtr->accessTime = 0;
	List_Init(&devHandlePtr->readWaitList);
	List_Init(&devHandlePtr->writeWaitList);
	List_Init(&devHandlePtr->exceptWaitList);
	devHandlePtr->notifyFlags = 0;
	fs_Stats.object.devices++;
    }
    return(found);
}


/*
 *----------------------------------------------------------------------
 *
 * Fsio_DeviceNameOpen --
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
Fsio_DeviceNameOpen(handlePtr, openArgsPtr, openResultsPtr)
    Fsio_FileIOHandle	*handlePtr;	/* A handle obtained by FslclLookup.
					 * Should be locked upon entry,
					 * is unlocked upon exit. */
     Fs_OpenArgs		*openArgsPtr;	/* Standard open arguments */
     Fs_OpenResults	*openResultsPtr;/* For returning ioFileID, streamID,
					 * and Fsio_DeviceState */
{
    register Fsdm_FileDescriptor *descPtr = handlePtr->descPtr;
    register Fsio_DeviceState *deviceDataPtr;
    register Fs_FileID *ioFileIDPtr = &openResultsPtr->ioFileID;

    ioFileIDPtr->serverID = descPtr->devServerID;
    if (ioFileIDPtr->serverID == FS_LOCALHOST_ID) {
	/*
	 * Map this "common" or "generic" device to the instance of
	 * the device on the process's logical home node.
	 */
	ioFileIDPtr->serverID = openArgsPtr->migClientID;
    }
    ioFileIDPtr->major = descPtr->devType;
    ioFileIDPtr->minor = descPtr->devUnit;
#ifdef INET
    /*
     * Hack in support for sockets by mapping a special device type
     * to sockets.  This needs to be replaced with a new type of disk file.
     */
    if (descPtr->devType == DEV_PLACEHOLDER_2) {
	ioFileIDPtr->major = rpc_SpriteID;
	ioFileIDPtr->minor = sockCounter++;
	switch(descPtr->devUnit) {
	    case NET_IP_PROTOCOL_IP:
		ioFileIDPtr->type = FSIO_RAW_IP_STREAM;
		break;
	    case NET_IP_PROTOCOL_UDP:
		ioFileIDPtr->type = FSIO_UDP_STREAM;
		break;
	    case NET_IP_PROTOCOL_TCP:
		ioFileIDPtr->type = FSIO_TCP_STREAM;
		break;
	    default:
		ioFileIDPtr->major = descPtr->devType;
		ioFileIDPtr->minor = descPtr->devUnit;
		if (ioFileIDPtr->serverID == openArgsPtr->clientID) {
		    ioFileIDPtr->type = FSIO_LCL_DEVICE_STREAM;
		} else {
		    ioFileIDPtr->type = FSIO_RMT_DEVICE_STREAM;
		}
		break;
	}
    } else
#endif
    if (ioFileIDPtr->serverID == openArgsPtr->clientID) {
	ioFileIDPtr->type = FSIO_LCL_DEVICE_STREAM;
    } else {
	ioFileIDPtr->type = FSIO_RMT_DEVICE_STREAM;
    }
    if (openArgsPtr->useFlags != 0) {
	/*
	 * Truely preparing for an open.
	 * Return the current modify/access times for the I/O server's cache.
	 */
	deviceDataPtr = mnew(Fsio_DeviceState);
	deviceDataPtr->modifyTime = descPtr->dataModifyTime;
	deviceDataPtr->accessTime = descPtr->accessTime;
	/*
	 * Choose a streamID for the client that points to the device server.
	 */
	Fsio_StreamCreateID(ioFileIDPtr->serverID, &openResultsPtr->streamID);
	deviceDataPtr->streamID = openResultsPtr->streamID;

	openResultsPtr->streamData = (ClientData)deviceDataPtr;
	openResultsPtr->dataSize = sizeof(Fsio_DeviceState);
    }
    Fsutil_HandleUnlock(handlePtr);
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsio_DeviceIoOpen --
 *
 *	Complete opening of a local device.  This is called on the I/O
 *	server and sets up state concerning this client.  The device
 *	driver open routine is called to set up the device.  If that
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
Fsio_DeviceIoOpen(ioFileIDPtr, flagsPtr, clientID, streamData, name, ioHandlePtrPtr)
    register Fs_FileID	*ioFileIDPtr;	/* I/O fileID */
    int			*flagsPtr;	/* FS_READ | FS_WRITE ... */
    int			clientID;	/* Host doing the open */
    ClientData		streamData;	/* Reference to Fsio_DeviceState struct */
    char		*name;		/* File name for error msgs */
    Fs_HandleHeader	**ioHandlePtrPtr;/* Return - a locked handle set up for
					 * I/O to a device, NIL if failure. */
{
    ReturnStatus 		status;
    Boolean			found;
    register Fsio_DeviceState	*deviceDataPtr;
    register Fsio_DeviceIOHandle	*devHandlePtr;
    Fsio_DeviceIOHandle		*tDevHandlePtr;	/* tempory devHandlePtr */
    register Fs_Stream		*streamPtr;
    register int		flags = *flagsPtr;

    deviceDataPtr = (Fsio_DeviceState *)streamData;
    *ioHandlePtrPtr = (Fs_HandleHeader *)NIL;

    found = FsioDeviceHandleInit(ioFileIDPtr, name, &tDevHandlePtr);
    devHandlePtr = tDevHandlePtr;
    /*
     * The device driver open routine gets the device specification,
     * the useFlags, and a token passed to Fs_NotifyReader
     * or Fs_NotifyWriter when the device becomes ready for I/O.
     */
    if (DEV_TYPE_INDEX(devHandlePtr->device.type) >= devNumDevices) {
	status = FS_DEVICE_OP_INVALID;
    } else {
	status = (*devFsOpTable[DEV_TYPE_INDEX(devHandlePtr->device.type)].open)
		    (&devHandlePtr->device, flags, 
			 (Fs_NotifyToken)devHandlePtr, &devHandlePtr->flags);
    }
    if (status == SUCCESS) {
	if (!found) {
	    /*
	     * Absorb the I/O attributes returned from the SrvOpen routine
	     * on the file server.
	     */
	    devHandlePtr->modifyTime = deviceDataPtr->modifyTime;
	    devHandlePtr->accessTime = deviceDataPtr->accessTime;
	}
	/*
	 * Trace the client, and then update our overall use counts.
	 * The client is recorded at the stream level to support migration,
	 * and at the I/O handle level for regular I/O.
	 */
	(void)Fsconsist_IOClientOpen(&devHandlePtr->clientList, clientID, flags, FALSE);

	streamPtr = Fsio_StreamAddClient(&deviceDataPtr->streamID, clientID,
			(Fs_HandleHeader *)devHandlePtr, flags,
			name, (Boolean *)NIL, (Boolean *)NIL);
	Fsutil_HandleRelease(streamPtr, TRUE);

	devHandlePtr->use.ref++;
	if (flags & FS_WRITE) {
	    devHandlePtr->use.write++;
	}
	*ioHandlePtrPtr = (Fs_HandleHeader *)devHandlePtr;
	Fsutil_HandleUnlock(devHandlePtr);
    } else {
	Fsutil_HandleRelease(devHandlePtr, TRUE);
    }
    free((Address) deviceDataPtr);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsio_DeviceClose --
 *
 *	Close a stream to a device.  We just need to clean up our state with
 *	the device driver.  The file server that named the device file keeps
 *	no state about us, so we don't have to contact it.
 *
 * FIX ME: need to write back access/modify times to name server
 *	Can use fs_AttrOpTable and the nameInfoPtr->fileID as long
 *	if the shadow stream on the I/O server is set up enough.
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
Fsio_DeviceClose(streamPtr, clientID, procID, flags, size, data)
    Fs_Stream		*streamPtr;	/* Stream to device */
    int			clientID;	/* HostID of client closing */
    Proc_PID		procID;		/* ID of closing process */
    int			flags;		/* Flags from the stream being closed */
    int			size;		/* Should be zero */
    ClientData		data;		/* IGNORED */
{
    ReturnStatus		status;
    register Fsio_DeviceIOHandle	*devHandlePtr =
	    (Fsio_DeviceIOHandle *)streamPtr->ioHandlePtr;
    Boolean			cache = FALSE;

    if (!Fsconsist_IOClientClose(&devHandlePtr->clientList, clientID, flags, &cache)) {
	printf("Fsio_DeviceClose, client %d unknown for device <%d,%d>\n",
		  clientID, devHandlePtr->hdr.fileID.major,
		  devHandlePtr->hdr.fileID.minor);
	Fsutil_HandleRelease(devHandlePtr, TRUE);
	return(FS_STALE_HANDLE);
    }
    /*
     * Clean up locks, then
     * clean up summary use counts and call driver's close routine.
     */
    Fsio_LockClose(&devHandlePtr->lock, &streamPtr->hdr.fileID);

    status = FsioDeviceCloseInt(devHandlePtr, flags, 1, (flags & FS_WRITE) != 0);
    /*
     * We don't bother to remove the handle here if the device isn't
     * being used.  Instead we let the handle get scavenged.
     */
    Fsutil_HandleRelease(devHandlePtr, TRUE);

    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsio_DeviceReopen --
 *
 *	Reopen a device here on the I/O server.
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
Fsio_DeviceReopen(hdrPtr, clientID, inData, outSizePtr, outDataPtr)
    Fs_HandleHeader	*hdrPtr;	/* NIL on the I/O server */
    int			clientID;	/* Client doing the reopen */
    ClientData		inData;		/* Ref. to Fsio_DeviceReopenParams */
    int			*outSizePtr;	/* Size of returned data, 0 here */
    ClientData		*outDataPtr;	/* Returned data, NIL here */
{
    Fsio_DeviceIOHandle	*devHandlePtr;
    ReturnStatus	status;
    register		devIndex;
    register Fsio_DeviceReopenParams *paramPtr =
	    (Fsio_DeviceReopenParams *)inData;

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


/*
 * ----------------------------------------------------------------------------
 *
 * Fsio_DeviceClientKill --
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
ReturnStatus
FsioDeviceCloseInt(devHandlePtr, useFlags, refs, writes)
    Fsio_DeviceIOHandle *devHandlePtr;	/* Device handle */
    int useFlags;			/* Flags from the stream */
    int refs;				/* Number of refs to close */
    int writes;				/* Number of these that were writing */
{
    if (refs > 0) {
	devHandlePtr->use.ref -= refs;
    }
    if (writes > 0) {
	devHandlePtr->use.write -= writes;
    }

    if (devHandlePtr->use.ref < 0 || devHandlePtr->use.write < 0) {
	panic("FsioDeviceCloseInt <%d,%d> ref %d, write %d\n",
	    devHandlePtr->hdr.fileID.major, devHandlePtr->hdr.fileID.minor,
	    devHandlePtr->use.ref, devHandlePtr->use.write);
    }

    return (*devFsOpTable[DEV_TYPE_INDEX(devHandlePtr->device.type)].close)
	    (&devHandlePtr->device, useFlags, devHandlePtr->use.ref,
		devHandlePtr->use.write);
}

/*
 * ----------------------------------------------------------------------------
 *
 * Fsio_DeviceClientKill --
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
Fsio_DeviceClientKill(hdrPtr, clientID)
    Fs_HandleHeader	*hdrPtr;	/* File to clean up */
    int			clientID;	/* Host assumed down */
{
    register Fsio_DeviceIOHandle *devHandlePtr = (Fsio_DeviceIOHandle *)hdrPtr;
    int refs, writes, execs;

    /*
     * Remove the client from the list of users, and see what it was doing.
     */
    Fsconsist_IOClientKill(&devHandlePtr->clientList, clientID, &refs, &writes, &execs);

    Fsio_LockClientKill(&devHandlePtr->lock, clientID);

    if (refs > 0) {
	int useFlags = FS_READ;		/* Have to assume this,
					 * which might be wrong for syslog */
	if (writes > 0) {
	    useFlags |= FS_WRITE;
	}
	(void)FsioDeviceCloseInt(devHandlePtr, useFlags, refs, writes);
    }
    Fsutil_HandleUnlock(devHandlePtr);
}

/*
 * ----------------------------------------------------------------------------
 *
 * Fsio_DeviceScavenge --
 *
 *	Called periodically to see if this handle is still needed.
 *	
 *
 * Results:
 *	TRUE if the handle was removed.
 *
 * Side effects:
 *	Removes the handle if it isn't referenced anymore and there
 *	are no remote clients.
 *
 * ----------------------------------------------------------------------------
 *
 */
Boolean
Fsio_DeviceScavenge(hdrPtr)
    Fs_HandleHeader	*hdrPtr;	/* File to clean up */
{
    register Fsio_DeviceIOHandle *handlePtr = (Fsio_DeviceIOHandle *)hdrPtr;

    if (handlePtr->use.ref == 0) {
	/*
	 * Remove handles for devices with no users.
	 */
	Fsutil_WaitListDelete(&handlePtr->readWaitList);
	Fsutil_WaitListDelete(&handlePtr->writeWaitList);
	Fsutil_WaitListDelete(&handlePtr->exceptWaitList);
	Fsutil_HandleRemove(handlePtr);
	fs_Stats.object.devices--;
	return(TRUE);
    } else {
        Fsutil_HandleUnlock(handlePtr);
	return(FALSE);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Fsio_VanillaDevReopen --
 *
 *	This is a simplified device driver re-open procedure.  It is called
 *	from Fsrmt_DeviceReopen via the device operation switch.  It, in turn,
 *	calls back through the device switch to the regular device open
 *	procedure.  Form many simple devices this is sufficient for a reopen.
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
Fsio_VanillaDevReopen(devicePtr, refs, writes, notifyToken)
    Fs_Device *devicePtr;	/* Identifies the device */
    int refs;			/* Number of streams to the device */
    int writes;			/* Number of those that are for writing */
    Fs_NotifyToken notifyToken;	/* Used with Fsio_DevNotifyReader */
{
    int devIndex = DEV_TYPE_INDEX(devicePtr->type);
    int useFlags = 0;
    int flags;

    if (refs > 0) {
	useFlags |= FS_READ;
    }
    if (writes > 0) {
	useFlags |= FS_WRITE;
    }
    return((*devFsOpTable[devIndex].open)
				(devicePtr, useFlags, notifyToken, &flags));
}

/*
 * ----------------------------------------------------------------------------
 *
 * Fsio_DeviceMigClose --
 *
 *	Release a reference from a Device I/O handle.
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
Fsio_DeviceMigClose(hdrPtr, flags)
    Fs_HandleHeader *hdrPtr;	/* File being encapsulated */
    int flags;			/* Use flags from the stream */
{
    panic( "Fsio_DeviceMigClose called\n");
    Fsutil_HandleRelease(hdrPtr, FALSE);
    return(SUCCESS);
}


/*
 * ----------------------------------------------------------------------------
 *
 * Fsio_DeviceMigrate --
 *
 *	This takes care of transfering references from one client to the other.
 *	A useful side-effect of this routine is	to properly set the type in
 *	the ioFileID, either FSIO_LCL_DEVICE_STREAM or FSIO_RMT_DEVICE_STREAM.
 *	In the latter case FsRmtDevoceMigrate is called to do all the work.
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
Fsio_DeviceMigrate(migInfoPtr, dstClientID, flagsPtr, offsetPtr, sizePtr, dataPtr)
    Fsio_MigInfo	*migInfoPtr;	/* Migration state */
    int		dstClientID;	/* ID of target client */
    int		*flagsPtr;	/* In/Out Stream usage flags */
    int		*offsetPtr;	/* Return - new stream offset */
    int		*sizePtr;	/* Return - sizeof(Fsio_DeviceState) */
    Address	*dataPtr;	/* Return - pointer to Fsio_DeviceState */
{
    Fsio_DeviceIOHandle			*devHandlePtr;
    Boolean				closeSrcClient;

    if (migInfoPtr->ioFileID.serverID != rpc_SpriteID) {
	/*
	 * The device was local, which is why we were called, but is now remote.
	 */
	migInfoPtr->ioFileID.type = FSIO_RMT_DEVICE_STREAM;
	return(FsrmtDeviceMigrate(migInfoPtr, dstClientID, flagsPtr, offsetPtr,
		sizePtr, dataPtr));
    }
    migInfoPtr->ioFileID.type = FSIO_LCL_DEVICE_STREAM;
    if (!FsioDeviceHandleInit(&migInfoPtr->ioFileID, (char *)NIL, &devHandlePtr)){
	printf(
		"Fsio_DeviceMigrate, I/O handle <%d,%d> not found\n",
		 migInfoPtr->ioFileID.major, migInfoPtr->ioFileID.minor);
	return(FS_FILE_NOT_FOUND);
    }
    /*
     * At the stream level, add the new client to the set of clients
     * for the stream, and check for any cross-network stream sharing.
     */
    Fsio_StreamMigClient(migInfoPtr, dstClientID, (Fs_HandleHeader *)devHandlePtr,
		    &closeSrcClient);
    /*
     * Adjust use counts on the I/O handle to reflect any new sharing.
     */
    Fsio_MigrateUseCounts(migInfoPtr->flags, closeSrcClient, &devHandlePtr->use);

    /*
     * Move the client at the I/O handle level.
     */
    Fsio_MigrateClient(&devHandlePtr->clientList, migInfoPtr->srcClientID,
			dstClientID, migInfoPtr->flags, closeSrcClient);

    *sizePtr = 0;
    *dataPtr = (Address)NIL;
    *flagsPtr = migInfoPtr->flags;
    *offsetPtr = migInfoPtr->offset;
    /*
     * We don't need this reference on the I/O handle; there is no change.
     */
    Fsutil_HandleRelease(devHandlePtr, TRUE);
    return(SUCCESS);
}

/*
 * ----------------------------------------------------------------------------
 *
 * Fsio_DeviceMigOpen --
 *
 *	Complete setup of a FS_DEVICE_STREAM after migration to the I/O server.
 *	The migrate routine has done the work of shifting use counts
 *	at the stream and I/O handle level.  This routine's job is
 *	to increment the low level I/O handle reference count to reflect
 *	the existence of a new stream to the I/O handle.
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
Fsio_DeviceMigOpen(migInfoPtr, size, data, hdrPtrPtr)
    Fsio_MigInfo	*migInfoPtr;	/* Migration state */
    int		size;		/* Zero */
    ClientData	data;		/* NIL */
    Fs_HandleHeader **hdrPtrPtr;	/* Return - handle for the file */
{
    register Fsio_DeviceIOHandle *devHandlePtr;

    devHandlePtr = Fsutil_HandleFetchType(Fsio_DeviceIOHandle, &migInfoPtr->ioFileID);
    if (devHandlePtr == (Fsio_DeviceIOHandle *)NIL) {
	panic( "Fsio_DeviceMigOpen, no handlel\n");
	return(FAILURE);
    } else {
	Fsutil_HandleUnlock(devHandlePtr);
	*hdrPtrPtr = (Fs_HandleHeader *)devHandlePtr;
	return(SUCCESS);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Fsio_DeviceRead --
 *
 *      Read on a stream connected to a local peripheral device.
 *	This branches to the driver routine after setting up buffers.
 *	This is called from Fs_Read and from Fsrmt_RpcRead.
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
Fsio_DeviceRead(streamPtr, readPtr, remoteWaitPtr, replyPtr)
    Fs_Stream		*streamPtr;	/* Open stream to the device. */
    Fs_IOParam		*readPtr;	/* Read parameter block. */
    Sync_RemoteWaiter	*remoteWaitPtr;	/* Process info for remote waiting */
    Fs_IOReply		*replyPtr;	/* Signal to return, if any,
					 * plus the amount read. */
{
    register Fsio_DeviceIOHandle	*devHandlePtr =
	    (Fsio_DeviceIOHandle *)streamPtr->ioHandlePtr;
    register ReturnStatus status;
    register Fs_Device	*devicePtr;
    int	     flags;
    Address userBuffer;
    Boolean copy;


    userBuffer = (Address) NIL;
    flags = devHandlePtr->flags;
    /*
     * Don't lock if the device driver informed us upon open that 
     * it doesn't want it.
     */
    if (!(flags & FS_DEV_DONT_LOCK)) { 
	Fsutil_HandleLock(devHandlePtr);
    }
    /*
     * Because the read could take a while and we aren't mapping in
     * buffers, we have to allocate an extra buffer here so the
     * buffer address is valid when the device's interrupt handler
     * does its DMA. Don't do this malloc and copy if the device
     * driver said it would handle it.
     */
    copy = (readPtr->flags & FS_USER) && !(flags & FS_DEV_DONT_COPY);
    if (copy) {
	userBuffer = readPtr->buffer;
	readPtr->buffer = (Address)malloc(readPtr->length);
    }

    /*
     * Put the process onto the read-wait list before attempting the read.
     * This is to prevent races with the device's notification which
     * happens from an interrupt handler.
     */
    Fsutil_WaitListInsert(&devHandlePtr->readWaitList, remoteWaitPtr);
    devicePtr = &devHandlePtr->device;
    status = (*devFsOpTable[DEV_TYPE_INDEX(devicePtr->type)].read)(devicePtr,
		readPtr, replyPtr);
    if (copy) {
        if (Vm_CopyOut(replyPtr->length, readPtr->buffer, userBuffer) != SUCCESS) {
	    if (status == SUCCESS) {
		status = SYS_ARG_NOACCESS;
	    }
	}
	free(readPtr->buffer);
	readPtr->buffer = userBuffer;
    }
    if (status != FS_WOULD_BLOCK) {
	Fsutil_WaitListRemove(&devHandlePtr->readWaitList, remoteWaitPtr);
    }
    devHandlePtr->accessTime = Fsutil_TimeInSeconds();
    fs_Stats.gen.deviceBytesRead += replyPtr->length;
    if (!(flags & FS_DEV_DONT_LOCK)) { 
	Fsutil_HandleUnlock(devHandlePtr);
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsio_DeviceWrite --
 *
 *      Write on a stream connected to a local peripheral device.
 *	This is called from Fs_Write and Fsrmt_RpcWrite.
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
Fsio_DeviceWrite(streamPtr, writePtr, remoteWaitPtr, replyPtr)
    Fs_Stream		*streamPtr;	/* Open stream to the device. */
    Fs_IOParam		*writePtr;	/* Read parameter block */
    Sync_RemoteWaiter	*remoteWaitPtr;	/* Process info for remote waiting */
    Fs_IOReply		*replyPtr;	/* Signal to return, if any */
{
    register Fsio_DeviceIOHandle	*devHandlePtr =
	    (Fsio_DeviceIOHandle *)streamPtr->ioHandlePtr;
    ReturnStatus	status = SUCCESS;
    register Fs_Device	*devicePtr = &devHandlePtr->device;
    Address		userBuffer;
    int			flags;
    Boolean		copy;

    userBuffer = (Address) NIL;
    flags = devHandlePtr->flags;
    /*
     * Don't lock if the device driver informed us upon open that 
     * it doesn't want it.
     */
    if (!(flags & FS_DEV_DONT_LOCK)) { 
	Fsutil_HandleLock(devHandlePtr);
    }
    /*
     * Because the write could take a while and we aren't mapping in
     * buffers, we have to allocate an extra buffer here so the
     * buffer address is valid when the device's interrupt handler
     * does its DMA. Don't do this malloc and copy if the device
     * driver said it would handle it.
     */
    copy = ((writePtr->flags & FS_USER) && !(flags & FS_DEV_DONT_COPY));
    if (copy) {
	userBuffer = writePtr->buffer;
        writePtr->buffer = (Address)malloc(writePtr->length);
	if (Vm_CopyIn(writePtr->length, userBuffer, writePtr->buffer) != SUCCESS) {
	    status = SYS_ARG_NOACCESS;
	}
    }
    if (status == SUCCESS) {
	/*
	 * Put the process onto the write-wait list before attempting the write.
	 * This is to prevent races with the device's notification which
	 * happens from an interrupt handler.
	 */
	Fsutil_WaitListInsert(&devHandlePtr->writeWaitList, remoteWaitPtr);
	status = (*devFsOpTable[DEV_TYPE_INDEX(devicePtr->type)].write)(devicePtr,
		writePtr, replyPtr);
	if (status != FS_WOULD_BLOCK) {
	    Fsutil_WaitListRemove(&devHandlePtr->writeWaitList, remoteWaitPtr);
	}
	devHandlePtr->modifyTime = Fsutil_TimeInSeconds();
	fs_Stats.gen.deviceBytesWritten += replyPtr->length;
    }

    if (copy) {
	free(writePtr->buffer);
	writePtr->buffer = userBuffer;
    }
    if (!(flags & FS_DEV_DONT_LOCK)) { 
	Fsutil_HandleUnlock(devHandlePtr);
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsio_DeviceSelect --
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
Fsio_DeviceSelect(hdrPtr, waitPtr, readPtr, writePtr, exceptPtr)
    Fs_HandleHeader	*hdrPtr;	/* Handle on device to select */
    Sync_RemoteWaiter	*waitPtr;	/* Process info for remote waiting */
    int 		*readPtr;	/* Bit to clear if non-readable */
    int 		*writePtr;	/* Bit to clear if non-writeable */
    int 		*exceptPtr;	/* Bit to clear if non-exceptable */
{
    register Fsio_DeviceIOHandle *devHandlePtr = (Fsio_DeviceIOHandle *)hdrPtr;
    register Fs_Device	*devicePtr = &devHandlePtr->device;
    register ReturnStatus status;

    Fsutil_HandleLock(devHandlePtr);
    if (waitPtr != (Sync_RemoteWaiter *)NIL) {
	if (*readPtr) {
	    Fsutil_WaitListInsert(&devHandlePtr->readWaitList, waitPtr);
	}
	if (*writePtr) {
	    Fsutil_WaitListInsert(&devHandlePtr->writeWaitList, waitPtr);
	}
	if (*exceptPtr) {
	    Fsutil_WaitListInsert(&devHandlePtr->exceptWaitList, waitPtr);
	}
    }
    status = (*devFsOpTable[DEV_TYPE_INDEX(devicePtr->type)].select)(devicePtr,
		    readPtr, writePtr, exceptPtr);

    if (waitPtr != (Sync_RemoteWaiter *)NIL) {
	if (*readPtr != 0) {
	    Fsutil_WaitListRemove(&devHandlePtr->readWaitList, waitPtr);
	}
	if (*writePtr != 0) {
	    Fsutil_WaitListRemove(&devHandlePtr->writeWaitList, waitPtr);
	}
	if (*exceptPtr != 0) {
	    Fsutil_WaitListRemove(&devHandlePtr->exceptWaitList, waitPtr);
	}
    }
    Fsutil_HandleUnlock(devHandlePtr);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsio_DeviceIOControl --
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
Fsio_DeviceIOControl(streamPtr, ioctlPtr, replyPtr)
    Fs_Stream *streamPtr;		/* Stream to a device. */
    Fs_IOCParam *ioctlPtr;		/* I/O Control parameter block */
    Fs_IOReply *replyPtr;		/* Return length and signal */
{
    register Fsio_DeviceIOHandle *devHandlePtr =
	    (Fsio_DeviceIOHandle *)streamPtr->ioHandlePtr;
    register Fs_Device	*devicePtr = &devHandlePtr->device;
    register ReturnStatus status = SUCCESS;

    switch (ioctlPtr->command) {
	case IOC_LOCK:
	case IOC_UNLOCK:
	    Fsutil_HandleLock(devHandlePtr);
	    status = Fsio_IocLock(&devHandlePtr->lock, ioctlPtr,
			&streamPtr->hdr.fileID);
	    Fsutil_HandleUnlock(devHandlePtr);
	    break;
	case IOC_PREFIX:
	    break;
	default:
	    if (!(devHandlePtr->flags & FS_DEV_DONT_LOCK)) { 
		Fsutil_HandleLock(devHandlePtr);
	    }
	    status = (*devFsOpTable[DEV_TYPE_INDEX(devicePtr->type)].ioctl)
		    (devicePtr, ioctlPtr, replyPtr);
	    if (!(devHandlePtr->flags & FS_DEV_DONT_LOCK)) { 
		Fsutil_HandleUnlock(devHandlePtr);
	    }
	    break;
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsio_DeviceGetIOAttr --
 *
 *      Get the I/O attributes for a device.  A copy of the access and
 *	modify times are kept at the I/O server.  This routine is called
 *	either from Fs_GetAttrStream or Fsrmt_RpcGetIOAttr to update
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
Fsio_DeviceGetIOAttr(fileIDPtr, clientID, attrPtr)
    Fs_FileID			*fileIDPtr;	/* FileID of device */
    int 			clientID;	/* IGNORED */
    register Fs_Attributes	*attrPtr;	/* Attributes to update */
{
    register Fsio_DeviceIOHandle *devHandlePtr;

    devHandlePtr = Fsutil_HandleFetchType(Fsio_DeviceIOHandle, fileIDPtr);
    if (devHandlePtr != (Fsio_DeviceIOHandle *)NIL) {
	if (devHandlePtr->accessTime > attrPtr->accessTime.seconds) {
	    attrPtr->accessTime.seconds = devHandlePtr->accessTime;
	}
	if (devHandlePtr->modifyTime > attrPtr->dataModifyTime.seconds) {
	    attrPtr->dataModifyTime.seconds = devHandlePtr->modifyTime;
	}
	Fsutil_HandleRelease(devHandlePtr, TRUE);
    }
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsio_DeviceSetIOAttr --
 *
 *      Set the I/O attributes for a device.  A copy of the access and
 *	modify times are kept at the I/O server.  This routine is called
 *	either from Fs_SetAttrStream or Fsrmt_RpcSetIOAttr to update
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
Fsio_DeviceSetIOAttr(fileIDPtr, attrPtr, flags)
    Fs_FileID			*fileIDPtr;	/* FileID of device */
    register Fs_Attributes	*attrPtr;	/* Attributes to copy */
    int				flags;		/* What attrs to set */
{
    register Fsio_DeviceIOHandle *devHandlePtr;

    if (flags & FS_SET_TIMES) {
	devHandlePtr = Fsutil_HandleFetchType(Fsio_DeviceIOHandle, fileIDPtr);
	if (devHandlePtr != (Fsio_DeviceIOHandle *)NIL) {
	    devHandlePtr->accessTime = attrPtr->accessTime.seconds;
	    devHandlePtr->modifyTime = attrPtr->dataModifyTime.seconds;
	    Fsutil_HandleRelease(devHandlePtr, TRUE);
	}
    }
    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * Fsio_DevNotifyReader --
 *
 *	Fsio_DevNotifyReader is available to device driver interrupt handlers
 *	that need to notify waiting processes that the device is readable.
 *	It schedules a process level call to ReadNotify, which
 *	in turn iterates down the list of handles for the device waking up
 *	all read waiters.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	Schedules a call to DevListNotify, which in turn calls
 *	Fsutil_WaitListNotify to schedule any waiting readers.
 *
 *----------------------------------------------------------------------
 */
void
Fsio_DevNotifyReader(notifyToken)
    Fs_NotifyToken notifyToken;
{
    register Fsio_DeviceIOHandle *devHandlePtr = (Fsio_DeviceIOHandle *)notifyToken;

    if ((devHandlePtr == (Fsio_DeviceIOHandle *)NIL) ||
	(devHandlePtr->notifyFlags & FS_READABLE)) {
	return;
    }
    if (devHandlePtr->hdr.fileID.type != FSIO_LCL_DEVICE_STREAM) {
	printf("Fsio_DevNotifyReader, bad handle\n");
    }
    devHandlePtr->notifyFlags |= FS_READABLE;
    Proc_CallFunc(ReadNotify, (ClientData) devHandlePtr, 0);
}

static void
ReadNotify(data, callInfoPtr)
    ClientData		data;
    Proc_CallInfo	*callInfoPtr;
{
    register Fsio_DeviceIOHandle *devHandlePtr = (Fsio_DeviceIOHandle *)data;
    if (devHandlePtr->hdr.fileID.type != FSIO_LCL_DEVICE_STREAM) {
	printf("ReadNotify, lost device handle\n");
    } else {
	devHandlePtr->notifyFlags &= ~FS_READABLE;
	Fsutil_WaitListNotify(&devHandlePtr->readWaitList);
    }
    callInfoPtr->interval = 0;
}


/*
 *----------------------------------------------------------------------
 *
 * Fsio_DevNotifyWriter --
 *
 *	Fsio_DevNotifyWriter is available to device driver interrupt handlers
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
Fsio_DevNotifyWriter(notifyToken)
    Fs_NotifyToken notifyToken;
{
    register Fsio_DeviceIOHandle *devHandlePtr = (Fsio_DeviceIOHandle *)notifyToken;

    if ((devHandlePtr == (Fsio_DeviceIOHandle *)NIL) ||
	(devHandlePtr->notifyFlags & FS_WRITABLE)) {
	return;
    }
    if (devHandlePtr->hdr.fileID.type != FSIO_LCL_DEVICE_STREAM) {
	printf("Fsio_DevNotifyWriter, bad handle\n");
	return;
    }
    devHandlePtr->notifyFlags |= FS_WRITABLE;
    Proc_CallFunc(WriteNotify, (ClientData) devHandlePtr, 0);
}

static void
WriteNotify(data, callInfoPtr)
    ClientData		data;
    Proc_CallInfo	*callInfoPtr;
{
    register Fsio_DeviceIOHandle *devHandlePtr = (Fsio_DeviceIOHandle *)data;
    if (devHandlePtr->hdr.fileID.type != FSIO_LCL_DEVICE_STREAM) {
	printf( "WriteNotify, lost device handle\n");
    } else {
	devHandlePtr->notifyFlags &= ~FS_WRITABLE;
	Fsutil_WaitListNotify(&devHandlePtr->writeWaitList);
    }
    callInfoPtr->interval = 0;
}



/*
 *----------------------------------------------------------------------
 *
 * Fsio_DevNotifyException --
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
Fsio_DevNotifyException(notifyToken)
    Fs_NotifyToken notifyToken;
{
    register Fsio_DeviceIOHandle *devHandlePtr = (Fsio_DeviceIOHandle *)notifyToken;

    if (devHandlePtr == (Fsio_DeviceIOHandle *)NIL) {
	return;
    }
    Proc_CallFunc(ExceptionNotify, (ClientData) devHandlePtr, 0);
}

static void
ExceptionNotify(data, callInfoPtr)
    ClientData		data;
    Proc_CallInfo	*callInfoPtr;
{
    register Fsio_DeviceIOHandle *devHandlePtr = (Fsio_DeviceIOHandle *)data;
    Fsutil_WaitListNotify(&devHandlePtr->exceptWaitList);
    callInfoPtr->interval = 0;
}


/*
 *----------------------------------------------------------------------
 *
 * Fsio_DeviceRecovTestUseCount --
 *
 *	For recovery testing, return the use count of the io handle.	
 *
 * Results:
 *	The use count.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
int
Fsio_DeviceRecovTestUseCount(handlePtr)
    Fsio_DeviceIOHandle *handlePtr;
{
    return handlePtr->use.ref;
}

/*
 *----------------------------------------------------------------------
 *
 * Fsio_BootTimeTtyOpen --
 *
 *	This is a stripped down version of Fsio_DeviceIoOpen
 *	that is used to invoke the tty driver open routine
 *	at boot time.  The important thing to get right is
 *	that the tty driver has to be passed the real FS
 *	I/O handle for the device so notifications work.
 *
 * Results:
 *	SUCCESS or a device dependent open error code.
 *
 * Side effects:
 *	Sets up and installs the console's ioHandle.  The tty open
 *	routine is called so L1/break/F1 key processing for
 *	Dev_InvokeConsole command is enabled.
 *	The associated FS handle is unlocked
 *	upon exit, but its reference count has been incremented.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Fsio_BootTimeTtyOpen()
{
    ReturnStatus 	status;
    Fs_FileID		ttyFileID;
    Fsio_DeviceIOHandle	*devHandlePtr;

    /*
     * Set up the console's fileID.  This sets the console
     * unit number to 0, although this is cleverly mapped by the
     * ttyDriver to a serial line if the EEPROM is properly configured.
     */
    ttyFileID.type = FSIO_LCL_DEVICE_STREAM;
    ttyFileID.serverID = rpc_SpriteID;
    ttyFileID.major = DEV_TERM;
    ttyFileID.minor = 0;
    (void)FsioDeviceHandleInit(&ttyFileID, "/dev/console", &devHandlePtr);
    /*
     * The device driver open routine gets the device specification,
     * the useFlags, and a token passed to Fs_NotifyReader
     * or Fs_NotifyWriter when the device becomes ready for I/O.
     */
    if (DEV_TYPE_INDEX(devHandlePtr->device.type) >= devNumDevices) {
	status = FS_DEVICE_OP_INVALID;
    } else {
	int flags = 0;
/* XXX */ printf("Fsio_BootTimeTtyOpen: spriteID %d devHandle <%x>\n",
			rpc_SpriteID, devHandlePtr);
	status = (*devFsOpTable[DEV_TYPE_INDEX(devHandlePtr->device.type)].open)
		    (&devHandlePtr->device, FS_READ|FS_WRITE, 
		      (Fs_NotifyToken)devHandlePtr, &flags);
    }
    /*
     * Unlock the handle.  This leaves an extra reference just
     * to make sure the file system handle doesn't get scavenged.
     */
    Fsutil_HandleUnlock(devHandlePtr);
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * Fsio_DeviceMmap --
 *
 *	Do a device-specific mmap operation.
 *
 * Results:
 *	SUCCESS or FAILURE.
 *
 * Side effects:
 *	Kernel memory mapped into user space.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Fsio_DeviceMmap(streamPtr, startAddr, length, offset, newAddrPtr)
    Fs_Stream		*streamPtr;	/* Stream to device. */
    Address		startAddr;	/* Requested starting virt. addr. */
    int			length;		/* Length of mapped segment. */
    int			offset;		/* Offset into mapped file. */
    Address		*newAddrPtr;
{
    Fs_Device		*devicePtr;
    Fsio_DeviceIOHandle	*ioHandlePtr;
    ReturnStatus	status;

    if (streamPtr->ioHandlePtr->fileID.type != FSIO_LCL_DEVICE_STREAM) {
	printf("Fsio_DeviceMmap passed something that wasn't a device.\n");
	return FAILURE;
    }
    ioHandlePtr = (Fsio_DeviceIOHandle *) streamPtr->ioHandlePtr;
    devicePtr = &(ioHandlePtr->device);
    if (DEV_TYPE_INDEX(devicePtr->type) >= devNumDevices) {
	return  FS_DEVICE_OP_INVALID;
    }

    status = (*devFsOpTable[DEV_TYPE_INDEX(devicePtr->type)].mmap)
		(devicePtr, startAddr, length, offset, newAddrPtr);

    return status;
}

