/*
 * fsPfs.c --
 *
 *	Routines specific to the pseudo-filesystem implementation.
 *	The pseudo-filesystem server uses the same request response
 *	protocol as with pseudo-devices.  The overall stream setup is
 *	a bit different, however.  The server process gets back a
 *	"naming" stream when it opens a remote link with the FS_PFS_MASTER
 *	flag.  The kernel forwards naming operations (Fs_Open, Fs_Remove,
 *	Fs_MakeDir, Fs_RemoveDir, Fs_MakeDevice, Fs_Rename, Fs_Hardlink)
 *	to the server using the request response protocol over the
 *	naming stream.  Thus the naming stream is like the server stream
 *	returned to pseudo-device servers via its control stream, and the
 *	client side of the naming stream is hung off the prefix table.
 *
 *	The pseudo-filesystem server can either return a pseudo-device
 *	kind of connection in response to opens by clients, or it can
 *	return a stream to a regular file or device.  Also, the server
 *	can be private to a host, or it can export itself to the network.
 *	
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
#include <fsprefix.h>
#include <fsStat.h>
#include <proc.h>
#include <rpc.h>
#include <fspdev.h>
#include <fspdevInt.h>
#include <dev/pfs.h>
#include <string.h>

static FspdevServerIOHandle *PfsGetUserLevelIDs _ARGS_((
	FspdevServerIOHandle *pdevHandlePtr, Fs_FileID *prefixIDPtr, 
	Fs_FileID *rootIDPtr));

/*
 *----------------------------------------------------------------------------
 *
 * FspdevRmtLinkNameOpen --
 *
 *	The user-level server for a pseudo-filesystem is established by
 *	opening a remote link type file with the FS_PFS_MASTER flag.
 *	This procedure is invoked on the file server when remote links
 *	are opened and detects this situation.  (If FS_PFS_MASTER is
 *	not specified then Fsio_FileNameOpen is called and the remote link
 *	is treated normally.)  Futhermore, there are two kinds of
 *	pseudo-filesystem servers, "local agents" and "network agents".
 *	If the FS_EXCLUSIVE flag is specified a local agent is created
 *	and only the opening host sees the pseudo-filesystem.  Otherwise
 *	the host sets up a network agent and exports the pseudo-filesystem
 *	to the whole Sprite network.  In the latter case a control stream
 *	is created here on the file server to record the server's existence.
 *
 * Results:
 *	A status is returned that indicates conflict if another server exists.
 *	A fileID and a streamID are also chosen for the naming stream.
 *
 * Side effects:
 *	A control I/O handle is created here on the file server to record
 *	who is the server for the pseudo device.  The handle for the
 *	remote link file is unlocked.
 *
 *----------------------------------------------------------------------------
 *
 */
ReturnStatus
FspdevRmtLinkNameOpen(handlePtr, openArgsPtr, openResultsPtr)
     register Fsio_FileIOHandle *handlePtr;	/* A handle from FslclLookup.
					 * Should be LOCKED upon entry,
					 * unlocked upon exit. */
     Fs_OpenArgs		*openArgsPtr;	/* Standard open arguments */
     Fs_OpenResults	*openResultsPtr;/* For returning ioFileID, streamID */
{
    register ReturnStatus status = SUCCESS;
    register Fs_FileID *ioFileIDPtr = &openResultsPtr->ioFileID;

    if ((openArgsPtr->useFlags & FS_PFS_MASTER) == 0) {
	return(Fsio_FileNameOpen(handlePtr, openArgsPtr, openResultsPtr));
    }
    /*
     * Generate an ID which is just like a FSIO_CONTROL_STREAM, except that
     * the serverID will be set to us, the file server, for exported
     * pseudo-filesystems, or set to the host running the server if the
     * pseudo-filesystem is not exported.
     */
    ioFileIDPtr->type = FSIO_PFS_CONTROL_STREAM;
    ioFileIDPtr->major = handlePtr->hdr.fileID.major;
    ioFileIDPtr->minor = handlePtr->hdr.fileID.minor ^
			 (handlePtr->descPtr->version << 16);
    if (openArgsPtr->useFlags & FS_EXCLUSIVE) {
	/*
	 * The pseudo-filesystem server is private to the client host.
	 * We further uniqify its control handle ID to avoid conflict with
	 * files from other servers.  We set the serverID to the host
	 * running the server so we won't see closes or re-opens.
	 */
	ioFileIDPtr->serverID = openArgsPtr->clientID;
	ioFileIDPtr->major ^= rpc_SpriteID << 16;
	Fsio_StreamCreateID(openArgsPtr->clientID, &openResultsPtr->streamID);
    } else {
	/*
	 * The pseudo-filesystem will be exported to the network.  Setting
	 * the serverID of the I/O handle to us means we'll see a close
	 * and possibly some reopens, we can do conflict checking.  However,
	 * the streamID we choose is used for the server half of the naming
	 * request-response stream, which sort of points sideways at the
	 * control handle on the client.  Thus there is no shadow stream,
	 * only a control handle here..
	 */
	register FspdevControlIOHandle *ctrlHandlePtr;

	ioFileIDPtr->serverID = rpc_SpriteID;
	ctrlHandlePtr = FspdevControlHandleInit(ioFileIDPtr, handlePtr->hdr.name);
	if (ctrlHandlePtr->serverID != NIL) {
	    status = FS_FILE_BUSY;
	} else {
	    ctrlHandlePtr->serverID = openArgsPtr->clientID;
	    Fsio_StreamCreateID(openArgsPtr->clientID, &openResultsPtr->streamID);
	}
	Fsutil_HandleRelease(ctrlHandlePtr, TRUE);
    }
    openResultsPtr->streamData = (ClientData)NIL;
    openResultsPtr->dataSize = 0;
    Fsutil_HandleUnlock(handlePtr);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FspdevPfsIoOpen --
 *
 *	This is called from Fs_Open to complete setup of the stream
 *	returned to the server of a pseudo-filesystem.  This stream
 *	is called a "naming stream" because it will be used to
 *	forward naming operations from the kernel up to the server.
 *	It is structured just like the regular pseudo-device request
 *	response stream, however.  The service end is returned to
 *	the server and the client end is hung off the prefix table.
 *
 *	Note: we contrain the file name being opened to be the absolute
 *	prefix of the pseudo-filesystem.  To fix this you'd need to extract
 *	the prefix over at the file server and return it in the stream data.
 * 
 * Results:
 *	SUCCESS, unless an exclusive (private) pseudo-filesystem server
 *	already exists on this host.
 *
 * Side effects:
 *	Three handles are created.  They have the same server, major, and minor,
 *	but differ in their types (FSIO_PFS_CONTROL_STREAM, FSIO_SERVER_STREAM,
 *	and FSIO_LCL_PSEUDO_STREAM).  The server stream is returned to our caller,
 *	the client stream is hooked to the prefix table, and the control
 *	stream is left around for conflict checking.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
FspdevPfsIoOpen(ioFileIDPtr, flagsPtr, clientID, streamData, name,
	ioHandlePtrPtr)
    register Fs_FileID	*ioFileIDPtr;	/* I/O fileID */
    int			*flagsPtr;	/* FS_READ | FS_WRITE ... */
    int			clientID;	/* Host doing the open */
    ClientData		streamData;	/* Pointer to Fspdev_State. */
    char		*name;		/* File name for error msgs */
    Fs_HandleHeader	**ioHandlePtrPtr;/* Return - a locked handle set up for
					 * I/O to a pseudo device, or NIL */
{
    register ReturnStatus	status = SUCCESS;
    register FspdevControlIOHandle *ctrlHandlePtr;
    register FspdevServerIOHandle *pdevHandlePtr;
    register FspdevClientIOHandle *cltHandlePtr;
    Fs_HandleHeader		*prefixHdrPtr;
    Fs_FileID			rootID;
    int				domain;
    int				id;
    char			*ignoredName;
    Fsprefix			*prefixPtr;
    int				prefixFlags;

    /*
     * Constrain the name to be an absolute prefix to keep things simple.
     * Then verify that the prefix is not already handled by someone.
     */
    if (name[0] != '/') {
	printf(
	    "Need absolute name (not \"%s\") for pseudo-filesystem\n", name);
	return(FS_INVALID_ARG);
    }
    status = Fsprefix_Lookup(name,
		FSPREFIX_IMPORTED|FSPREFIX_EXPORTED|FSPREFIX_EXACT, -1,
		&prefixHdrPtr, &rootID, &ignoredName, &id, &domain, &prefixPtr);
    if (status == SUCCESS) {
	printf( "Prefix \"%s\" already serviced\n", name);
	return(FS_FILE_BUSY);
    } else {
	status = SUCCESS;
    }
    /*
     * Nuke this meaningless flag so we don't get an I/O control from Fs_Open.
     */
    *flagsPtr &= ~FS_TRUNC;
    /*
     * Create a control handle that contains the seed used to generate
     * pseudo-device connections to the pseudo-filesystem server.  It is
     * important to set the serverID so the control stream won't get scavenged.
     */
    ctrlHandlePtr = FspdevControlHandleInit(ioFileIDPtr, name);
    ctrlHandlePtr->serverID = rpc_SpriteID;
    /*
     * Setup the request-response connection, and return the server
     * end to the calling process.  We save a back pointer to the
     * control stream so we can generate new request-response connections
     * when clients do opens in the pseudo-filesystem, and so we
     * can close it and clean up the prefix table when the server process exits.
     */
    ioFileIDPtr->type = FSIO_LCL_PSEUDO_STREAM;
    ioFileIDPtr->serverID = rpc_SpriteID;
    cltHandlePtr = FspdevConnect(ctrlHandlePtr, ioFileIDPtr, rpc_SpriteID, 1);
    if (cltHandlePtr == (FspdevClientIOHandle *)NIL) {
	status = FAILURE;
	goto cleanup;
    }
    pdevHandlePtr = cltHandlePtr->pdevHandlePtr;
    *ioHandlePtrPtr = (Fs_HandleHeader *)pdevHandlePtr;
    /*
     * This ID gets passed in Fs_LookupArgs
     * as the prefixID if the prefix is the root of the pseudo domain.
     * It can be reset by the user with IOC_PFS_SET_ROOT.
     */
    pdevHandlePtr->userLevelID.type = 0;
    pdevHandlePtr->userLevelID.serverID = 0;
    pdevHandlePtr->userLevelID.major = 0;
    pdevHandlePtr->userLevelID.minor = 0;
    /*
     * Install the client side of the connection in the prefix table.
     */
    prefixFlags = FSPREFIX_IMPORTED;
    if (*flagsPtr & FS_EXCLUSIVE) {
	prefixFlags |= FSPREFIX_LOCAL;
    } else {
	prefixFlags |= FSPREFIX_EXPORTED;
    }
    ctrlHandlePtr->prefixPtr = Fsprefix_Install(name,
		(Fs_HandleHeader *)cltHandlePtr, FS_PSEUDO_DOMAIN, prefixFlags);
    /*
     * No migration of pseudo-filesystem servers.
     */
    Proc_NeverMigrate(Proc_GetCurrentProc());
    Fsutil_HandleUnlock(cltHandlePtr);
cleanup:
    if (status != SUCCESS) {
	Fsutil_HandleRelease(ctrlHandlePtr, TRUE);
	*ioHandlePtrPtr = (Fs_HandleHeader *)NIL;
    } else {
	Fsutil_HandleUnlock(ctrlHandlePtr);
    }
    return(status);

}

/*
 *----------------------------------------------------------------------------
 *
 * FspdevPfsExport --
 *
 *	This is called from the Fsrmt_RpcPrefix stub to complete setup for
 *	a client that will be importing a prefix of a pseudo-filesystem
 *	that has its server process on this host.  This has to add the
 *	client to the client end of the naming request response stream
 *	so future naming operations by that client are accepted here.
 *	The remote Sprite host will call FspdevPfsNamingIoOpen to set
 *	up the handle that it will attach to its own prefix table.
 *
 * Results:
 *	This returns a fileID that the client will use to set up its I/O handle.
 *
 * Side effects:
 *	A control I/O handle is created here on the file server to record
 *	who is the server for the pseudo device.
 *
 *----------------------------------------------------------------------------
 *
 */
ReturnStatus
FspdevPfsExport(hdrPtr, clientID, ioFileIDPtr, dataSizePtr, clientDataPtr)
     Fs_HandleHeader	*hdrPtr;	/* A handle from the prefix table. */
     int		clientID;	/* Host ID of client importing prefix */
     register Fs_FileID	*ioFileIDPtr;	/* Return - I/O handle ID */
     int		*dataSizePtr;	/* Return - 0 */
     ClientData		*clientDataPtr;	/* Return - NIL */
{
    register FspdevClientIOHandle *cltHandlePtr = (FspdevClientIOHandle *)hdrPtr;
    register ReturnStatus status;

    Fsutil_HandleLock(cltHandlePtr);
    if (FspdevPdevServerOK(cltHandlePtr->pdevHandlePtr)) {
	(void)Fsconsist_IOClientOpen(&cltHandlePtr->clientList, clientID, 0, FALSE);
	*ioFileIDPtr = cltHandlePtr->hdr.fileID;
	ioFileIDPtr->type = FSIO_PFS_NAMING_STREAM;
	*dataSizePtr = 0;
	*clientDataPtr = (ClientData)NIL;
	status = SUCCESS;
    } else {
	status = FAILURE;
    }
    Fsutil_HandleUnlock(cltHandlePtr);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FspdevPfsNamingIoOpen --
 *
 *	This is called from FsrmtImport to complete setup of the I/O
 *	handle that hangs off the prefix table.  This stream
 *	is called a "naming stream" because it will be used to
 *	forward naming operations to the pseudo-filesystem server.  This
 *	routine is similar to FspdevRmtPseudoStreamIoOpen, except that at
 *	this point the server already knows about us, so we don't have
 *	to contact it with Fsrmt_DeviceOpen.
 *
 * Results:
 *	SUCCESS.
 *
 * Side effects:
 *	Creates a FSIO_PFS_NAMING_STREAM, which is like a FSIO_RMT_PSEUDO_STREAM
 *	in that its operations are forwarded via RPC to the host running
 *	the pseudo-filesystem server.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
FspdevPfsNamingIoOpen(ioFileIDPtr, flagsPtr, clientID, streamData, name,
	ioHandlePtrPtr)
    register Fs_FileID	*ioFileIDPtr;	/* I/O fileID */
    int			*flagsPtr;	/* FS_READ | FS_WRITE ... */
    int			clientID;	/* Host doing the open */
    ClientData		streamData;	/* Pointer to Fspdev_State. */
    char		*name;		/* File name for error msgs */
    Fs_HandleHeader	**ioHandlePtrPtr;/* Return - a locked handle set up for
					 * I/O to a pseudo device, or NIL */
{
    Boolean found;
    Fsrmt_IOHandle *rmtHandlePtr;

    found = Fsutil_HandleInstall(ioFileIDPtr, sizeof(Fsrmt_IOHandle), name,
			FALSE, (Fs_HandleHeader **)&rmtHandlePtr);
    if (!found) {
	Fsutil_RecoveryInit(&rmtHandlePtr->recovery);
	fs_Stats.object.remote++;
    }
    rmtHandlePtr->recovery.use.ref++;
    *ioHandlePtrPtr = (Fs_HandleHeader *)rmtHandlePtr;
    Fsutil_HandleUnlock(rmtHandlePtr);
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * FspdevPfsDomainInfo --
 *
 *	Get information about a pseudo-file-system.
 *
 * Results:
 *	An error status
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
FspdevPfsDomainInfo(fileIDPtr, domainInfoPtr)
    Fs_FileID *fileIDPtr;
    Fs_DomainInfo *domainInfoPtr;
{
    ReturnStatus		status;
    Pfs_Request			request;
    Fs_RedirectInfo		*redirectPtr;
    FspdevClientIOHandle		*cltHandlePtr;
    int				resultSize;

    status = FS_FILE_NOT_FOUND;
    cltHandlePtr = Fsutil_HandleFetchType(FspdevClientIOHandle, fileIDPtr);
    if (cltHandlePtr != (FspdevClientIOHandle *)NIL) {
	Fsutil_HandleUnlock(cltHandlePtr);
	/*
	 * Go to the pseudo-device server to get the domain information.
	 * We also change the fileID of the domain to be the user-visible
	 * one so that the getwd() library call works right.
	 */
	*fileIDPtr = cltHandlePtr->pdevHandlePtr->userLevelID;
	fileIDPtr->serverID = rpc_SpriteID;

	request.hdr.operation = PFS_DOMAIN_INFO;
	request.param.domainInfo = *fileIDPtr;
	resultSize = sizeof(Fs_DomainInfo);
	status = FspdevPseudoStreamLookup(cltHandlePtr->pdevHandlePtr, &request,
		    0, (Address)NIL,
		    &resultSize, (Address)domainInfoPtr, &redirectPtr);
	if (redirectPtr != (Fs_RedirectInfo *)NIL) {
	    free((Address)redirectPtr);
	}
	Fsutil_HandleRelease(cltHandlePtr, FALSE);
    }
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * FspdevPfsOpen --
 *
 *	Open a file served by a pseudo-filesystem server.  The stream returned
 *	to the client can either be a pseudo-device connection to the
 *	server of the pseudo-filesystem, or a regular stream that has
 *	been passed off from the server process.
 *
 * Results:
 *	SUCCESS, FS_REDIRECT, or some error code from the lookup on the server.
 *	If FS_REDIRECT, then *newNameInfoPtr has prefix information.
 *
 * Side effects:
 *	None here.  The connections are setup in the server IOControl routine.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
FspdevPfsOpen(prefixHandle, relativeName, argsPtr, resultsPtr, 
	     newNameInfoPtrPtr)
    Fs_HandleHeader  *prefixHandle;	/* Handle from prefix table or cwd */
    char 	  *relativeName;	/* The name of the file to open. */
    Address 	  argsPtr;		/* Ref. to Fs_OpenArgs */
    Address 	  resultsPtr;		/* Ref. to Fs_OpenResults */
    Fs_RedirectInfo **newNameInfoPtrPtr;	/* We return this if the server leaves 
					 * its domain during the lookup. */
{
    register FspdevClientIOHandle	*cltHandlePtr;
    register FspdevServerIOHandle *pdevHandlePtr;
    register Fs_OpenArgs		*openArgsPtr = (Fs_OpenArgs *)argsPtr;
    Pfs_Request			request;
    register ReturnStatus	status;
    int				resultSize;

    cltHandlePtr = (FspdevClientIOHandle *)prefixHandle;

    /*
     * Set up the open arguments, and get ahold of the naming stream.
     */
    request.hdr.operation = PFS_OPEN;
    pdevHandlePtr = PfsGetUserLevelIDs(cltHandlePtr->pdevHandlePtr,
			    &openArgsPtr->prefixID, &openArgsPtr->rootID);
    if (pdevHandlePtr == (FspdevServerIOHandle *)NIL) {
	return(FS_FILE_NOT_FOUND);
    }
    request.param.open = *openArgsPtr;

    resultSize = sizeof(Fs_OpenResults);

    /*
     * Do the open.  The openResults are setup by the server-side IOC
     * handler, so just we return.
     */
    status = FspdevPseudoStreamLookup(pdevHandlePtr, &request,
		strlen(relativeName) + 1, (Address)relativeName,
		&resultSize, resultsPtr, newNameInfoPtrPtr);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * PfsGetUserLevelID --
 *
 *	This takes the lookup arguments for the pseudo-domain and maps them to
 *	the correct request-response stream for naming, and to the user-level
 *	versions of the prefix and root IDs.  Because of current directories,
 *	the handle passed to the Pfs lookup routines won't always be the
 *	top-level naming request-response stream.  However, we do get passed
 *	the fileID of the root, from which we can fetch the right handle.
 *
 * Results:
 *	Returns the pdevHandlePtr for the naming request-response stream.  This
 *	is in turn passed to FspdevPseudoStreamLookup.  This also sets the prefixID
 *	to be the user-level version.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static FspdevServerIOHandle *
PfsGetUserLevelIDs(pdevHandlePtr, prefixIDPtr, rootIDPtr)
    FspdevServerIOHandle *pdevHandlePtr;	/* Handle of name prefix */
    Fs_FileID *prefixIDPtr;		/* Prefix fileID */
    Fs_FileID *rootIDPtr;		/* ID of naming request-response */
{
    register FspdevClientIOHandle *cltHandlePtr;

    *prefixIDPtr = pdevHandlePtr->userLevelID;
    rootIDPtr->type = fsio_RmtToLclType[rootIDPtr->type];
    cltHandlePtr = Fsutil_HandleFetchType(FspdevClientIOHandle, rootIDPtr);
    if (cltHandlePtr != (FspdevClientIOHandle *)NIL) {
	Fsutil_HandleRelease(cltHandlePtr, TRUE);
	return(cltHandlePtr->pdevHandlePtr);
    } else {
	return((FspdevServerIOHandle *)NIL);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * FspdevPfsOpenConnection --
 *
 *	This is called when the server does an IOC_PFS_OPEN to respond
 *	to an open request issued by FspdevPfsOpen.  This sets up the server's
 *	half of the pseudo-device connection, while FspdevPfsOpen completes
 *	the connection by opening the client's half.  This knows it is
 *	executing in the kernel context of the server process so it can
 *	establish the user-level streamID needed by the server.
 *
 * Results:
 *	A streamID that has to be returned to the server.
 *
 * Side effects:
 *	Set up the state for a pseudo-device connection.
 *
 *----------------------------------------------------------------------
 */
int
FspdevPfsOpenConnection(namingPdevHandlePtr, srvrFileIDPtr, openResultsPtr)
    FspdevServerIOHandle	*namingPdevHandlePtr;/* From naming request-response */
    Fs_FileID *srvrFileIDPtr;		/* FileID from user-level server */
    Fs_OpenResults *openResultsPtr;	/* Info returned to client's open */
{
    register FspdevControlIOHandle *ctrlHandlePtr;
    register FspdevClientIOHandle *cltHandlePtr;
    register Fs_Stream *srvStreamPtr;
    register Fs_Stream *cltStreamPtr;
    int newStreamID;
    register Fs_FileID	*fileIDPtr;	/* FileID for new connection */

    /*
     * Pick an I/O fileID for the connection.
     */
    ctrlHandlePtr = namingPdevHandlePtr->ctrlHandlePtr;
    ctrlHandlePtr->seed++;
    fileIDPtr = &openResultsPtr->ioFileID;
    fileIDPtr->type = FSIO_LCL_PFS_STREAM;
    fileIDPtr->serverID = rpc_SpriteID;
    fileIDPtr->major = ctrlHandlePtr->rmt.hdr.fileID.major;
    fileIDPtr->minor = (ctrlHandlePtr->rmt.hdr.fileID.minor << 12)
			^ ctrlHandlePtr->seed;

    cltHandlePtr = FspdevConnect(ctrlHandlePtr, fileIDPtr,
		    namingPdevHandlePtr->open.clientID, 0);
    if (cltHandlePtr == (FspdevClientIOHandle *)NIL) {
	printf( "FspdevPfsOpenConnection failing\n");
	return(-1);
    }
    /*
     * Set the ioFileID type. (FspdevConnect has munged it to FSIO_SERVER_STREAM.)
     * The open.clientID has been set curtesy of the PFS_OPEN RequestResponse.
     */
    if (namingPdevHandlePtr->open.clientID == rpc_SpriteID) {
	fileIDPtr->type = FSIO_LCL_PFS_STREAM;
    } else {
	fileIDPtr->type = FSIO_RMT_PFS_STREAM;
    }
    /*
     * Save the server process's ID for the connection.
     */
    cltHandlePtr->pdevHandlePtr->userLevelID = *srvrFileIDPtr;

    /*
     * Set up a stream to the server's half of the connection and
     * then choose a user level streamID.
     */
    srvStreamPtr = Fsio_StreamCreate(rpc_SpriteID, rpc_SpriteID,
		    (Fs_HandleHeader *)cltHandlePtr->pdevHandlePtr,
		    FS_READ|FS_USER, namingPdevHandlePtr->open.name);
    if (Fs_GetStreamID(srvStreamPtr, &newStreamID) != SUCCESS) {
	(void)Fsio_StreamClientClose(&srvStreamPtr->clientList, rpc_SpriteID);
	Fsio_StreamDestroy(srvStreamPtr);
	Sync_LockClear(&cltHandlePtr->pdevHandlePtr->lock);
	Fsutil_HandleRemove(cltHandlePtr->pdevHandlePtr);
	Fsutil_HandleRemove(cltHandlePtr);
	fs_Stats.object.pseudoStreams--;
	newStreamID = -1;
    } else {
	/*
	 * Set up a stream to the client's half of the connection.
	 */
	cltStreamPtr = Fsio_StreamCreate(rpc_SpriteID,
			    namingPdevHandlePtr->open.clientID,
			    (Fs_HandleHeader *)cltHandlePtr,
			    namingPdevHandlePtr->open.useFlags,
			    namingPdevHandlePtr->open.name);
	openResultsPtr->nameID = openResultsPtr->ioFileID;
	openResultsPtr->streamID = cltStreamPtr->hdr.fileID;
	openResultsPtr->dataSize = 0;
	openResultsPtr->streamData = (ClientData)NIL;
	Fsutil_HandleRelease(cltStreamPtr, TRUE);
	Fsutil_HandleUnlock(cltHandlePtr);
	Fsutil_HandleUnlock(srvStreamPtr);
    }
    return(newStreamID);
}

/*
 *----------------------------------------------------------------------
 *
 * FspdevPfsStreamIoOpen --
 *
 *	This is called from Fs_Open to complete setup of a client's
 *	stream to a pseudo-filesystem server.  The server is running on this
 *	host, and the pseudo-device connection has already been established.
 *	This routine just latches onto it and returns.
 * 
 * Results:
 *	SUCCESS, unless the server process has died recently.
 *
 * Side effects:
 *	Fetches the handle, which increments its ref count.  The
 *	handle is unlocked before returning.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
FspdevPfsStreamIoOpen(ioFileIDPtr, flagsPtr, clientID, streamData, name,
	ioHandlePtrPtr)
    register Fs_FileID	*ioFileIDPtr;	/* I/O fileID */
    int			*flagsPtr;	/* FS_READ | FS_WRITE ... */
    int			clientID;	/* Host doing the open */
    ClientData		streamData;	/* Pointer to Fspdev_State. */
    char		*name;		/* File name for error msgs */
    Fs_HandleHeader	**ioHandlePtrPtr;/* Return - a locked handle set up for
					 * I/O to a pseudo device, or NIL */
{
    register FspdevClientIOHandle *cltHandlePtr;

    cltHandlePtr = Fsutil_HandleFetchType(FspdevClientIOHandle, ioFileIDPtr);
    if (cltHandlePtr == (FspdevClientIOHandle *)NIL) {
	printf( "FspdevPfsStreamIoOpen, no handle\n");
	*ioHandlePtrPtr = (Fs_HandleHeader *)NIL;
	return(FS_FILE_NOT_FOUND);
    } else {
	if (cltHandlePtr->hdr.name != (char *)NIL) {
	    free((Address)cltHandlePtr->hdr.name);
	}
	cltHandlePtr->hdr.name = (char *)malloc(strlen(name) + 1);
	(void)strcpy(cltHandlePtr->hdr.name, name);
	*ioHandlePtrPtr = (Fs_HandleHeader *)cltHandlePtr;
	Fsutil_HandleUnlock(cltHandlePtr);
	return(SUCCESS);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * FspdevRmtPfsStreamIoOpen --
 *
 *	This is called from Fs_Open to complete setup of a client's
 *	stream to a remote pseudo-filesystem server.  The server is running
 *	on this	host, and the pseudo-device connection has already been
 *	established.  This routine just sets up a remote handle that
 *	references the connection.
 * 
 * Results:
 *	SUCCESS.
 *
 * Side effects:
 *	Installs a remote I/O handle.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
FspdevRmtPfsStreamIoOpen(ioFileIDPtr, flagsPtr, clientID, streamData, name,
	ioHandlePtrPtr)
    register Fs_FileID	*ioFileIDPtr;	/* I/O fileID */
    int			*flagsPtr;	/* FS_READ | FS_WRITE ... */
    int			clientID;	/* Host doing the open */
    ClientData		streamData;	/* NIL. */
    char		*name;		/* File name for error msgs */
    Fs_HandleHeader	**ioHandlePtrPtr;/* Return - FSIO_RMT_PFS_STREAM handle */
{
    Fsrmt_IOHandleInit(ioFileIDPtr, *flagsPtr, name, ioHandlePtrPtr);
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * FspdevPfsGetAttrPath --
 *
 *	Get the attributes of a file in a pseudo-filesystem.
 *
 * Results:
 *	A return code from the RPC or the remote server.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
FspdevPfsGetAttrPath(prefixHandle, relativeName, argsPtr, resultsPtr,
         	    newNameInfoPtrPtr)
    Fs_HandleHeader *prefixHandle;	/* Handle from the prefix table */
    char           *relativeName;	/* The name of the file. */
    Address        argsPtr;		/* Bundled arguments for us */
    Address        resultsPtr;		/* Where to store attributes */
    Fs_RedirectInfo **newNameInfoPtrPtr;	/* We return this if the server leaves 
					 * its domain during the lookup. */
{
    register FspdevServerIOHandle	*pdevHandlePtr;
    Pfs_Request			request;
    register ReturnStatus	status;
    register Fs_OpenArgs		*openArgsPtr;
    register Fs_GetAttrResults	*getAttrResultsPtr;
    int				resultSize;

    pdevHandlePtr = ((FspdevClientIOHandle *)prefixHandle)->pdevHandlePtr;
    openArgsPtr = (Fs_OpenArgs *)argsPtr;

    request.hdr.operation = PFS_GET_ATTR;
    pdevHandlePtr = PfsGetUserLevelIDs(pdevHandlePtr,
			    &openArgsPtr->prefixID, &openArgsPtr->rootID);
    if (pdevHandlePtr == (FspdevServerIOHandle *)NIL) {
	return(FS_FILE_NOT_FOUND);
    }
    request.param.open = *(Fs_OpenArgs *)argsPtr;

    getAttrResultsPtr = (Fs_GetAttrResults *)resultsPtr;
    resultSize = sizeof(Fs_Attributes);

    status = FspdevPseudoStreamLookup(pdevHandlePtr, &request,
		strlen(relativeName) + 1, (Address)relativeName,
		&resultSize, (Address)getAttrResultsPtr->attrPtr,
		newNameInfoPtrPtr);
    /*
     * Patch the serverID in the attributes so it matches the serverID
     * given in the prefix table.  This is needed to make getwd() work.
     */
    getAttrResultsPtr->attrPtr->serverID = rpc_SpriteID;
    /*
     * The pseudo-filesystem server has given us all the attributes.  There
     * is no reason to do a getIOAttr so we inhibit that with a special
     * ioFileID type.  However, because Fsutil_DomainInfo calls this routine
     * to fill in the file ID for the user-visible prefix table entry
     * we set up the rest of the fields to match the return of a stat() call.
     */
    getAttrResultsPtr->fileIDPtr->type = -1;
    getAttrResultsPtr->fileIDPtr->serverID = getAttrResultsPtr->attrPtr->serverID;
    getAttrResultsPtr->fileIDPtr->major = getAttrResultsPtr->attrPtr->domain;
    getAttrResultsPtr->fileIDPtr->minor = getAttrResultsPtr->attrPtr->fileNumber;
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FspdevPfsSetAttrPath --
 *
 *	Set the attributes of a file in a pseudo-filesystem.
 *
 * Results:
 *	A return code from the RPC or the remote server.
 *
 * Side effects:
 *	Setting those attributes.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
FspdevPfsSetAttrPath(prefixHandle, relativeName, argsPtr, resultsPtr,
         	    newNameInfoPtrPtr)
    Fs_HandleHeader *prefixHandle;	/* Handle from the prefix table */
    char           *relativeName;	/* The name of the file. */
    Address        argsPtr;		/* Bundled arguments for us */
    Address        resultsPtr;		/* Where to store attributes */
    Fs_RedirectInfo **newNameInfoPtrPtr;	/* We return this if the server leaves 
					 * its domain during the lookup. */
{
    register FspdevServerIOHandle	*pdevHandlePtr;
    Pfs_Request			request;
    register ReturnStatus	status;
    register Fs_SetAttrArgs	*setAttrArgsPtr;
    register Pfs_SetAttrData	*setAttrDataPtr;
    register int		nameLength;
    register int		dataLength;
    int				zero = 0;

    pdevHandlePtr = ((FspdevClientIOHandle *)prefixHandle)->pdevHandlePtr;

    setAttrArgsPtr = (Fs_SetAttrArgs *)argsPtr;

    request.hdr.operation = PFS_SET_ATTR;
    pdevHandlePtr = PfsGetUserLevelIDs(pdevHandlePtr,
			    &setAttrArgsPtr->openArgs.prefixID,
			    &setAttrArgsPtr->openArgs.rootID);
    if (pdevHandlePtr == (FspdevServerIOHandle *)NIL) {
	return(FS_FILE_NOT_FOUND);
    }
    request.param.open = setAttrArgsPtr->openArgs;

    /*
     * The dataLength includes 4 bytes of name inside the Pfs_SetAttrData
     * so there is room for the trailing null byte.
     */
    nameLength = strlen(relativeName);
    dataLength = sizeof(Pfs_SetAttrData) + nameLength;

    setAttrDataPtr = (Pfs_SetAttrData *)malloc(dataLength);
    setAttrDataPtr->attr = setAttrArgsPtr->attr;
    setAttrDataPtr->flags = setAttrArgsPtr->flags;
    setAttrDataPtr->nameLength = nameLength;
    (void)strcpy(setAttrDataPtr->name, relativeName);

    status = FspdevPseudoStreamLookup(pdevHandlePtr, &request,
	    dataLength, (Address)setAttrDataPtr,
	    &zero, (Address)NIL, newNameInfoPtrPtr);
    free((Address)setAttrDataPtr);
    /*
     * The pseudo-filesystem server has dealt with all the attributes so
     * we don't fill in the ioFileID.
     */
    ((Fs_FileID *)resultsPtr)->type = -1;
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FspdevPfsMakeDir --
 *
 *	Make the named directory in a pseudo-filesystem.
 *
 * Results:
 *	A return code from the file server or the RPC.
 *
 * Side effects:
 *	Makes the directory.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
FspdevPfsMakeDir(prefixHandle, relativeName, argsPtr, resultsPtr, 
		newNameInfoPtrPtr)
    Fs_HandleHeader *prefixHandle;   /* Handle from the prefix table */
    char 	   *relativeName;   /* The name of the directory to create */
    Address 	   argsPtr;	    /* Ref. to Fs_OpenArgs */
    Address 	   resultsPtr;	    /* == NIL */
    Fs_RedirectInfo **newNameInfoPtrPtr;/* We return this if the server leaves 
					* its domain during the lookup. */
{
    register FspdevServerIOHandle	*pdevHandlePtr;
    register Fs_OpenArgs		*openArgsPtr;
    Pfs_Request			request;
    register ReturnStatus	status;
    int				resultSize;

    pdevHandlePtr = ((FspdevClientIOHandle *)prefixHandle)->pdevHandlePtr;
    openArgsPtr = (Fs_OpenArgs *)argsPtr;

    request.hdr.operation = PFS_MAKE_DIR;
    pdevHandlePtr = PfsGetUserLevelIDs(pdevHandlePtr,
			    &openArgsPtr->prefixID, &openArgsPtr->rootID);
    if (pdevHandlePtr == (FspdevServerIOHandle *)NIL) {
	return(FS_FILE_NOT_FOUND);
    }
    request.param.makeDir = *openArgsPtr;

    resultSize = 0;

    status = FspdevPseudoStreamLookup(pdevHandlePtr, &request,
		strlen(relativeName) + 1, (Address)relativeName,
		&resultSize, resultsPtr, newNameInfoPtrPtr);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FspdevPfsMakeDevice --
 *
 *	Create a device file in a pseudo-filesystem.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Makes a device file.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
FspdevPfsMakeDevice(prefixHandle, relativeName, argsPtr, resultsPtr,
			       newNameInfoPtrPtr)
    Fs_HandleHeader *prefixHandle;   /* Handle from the prefix table */
    char           *relativeName;   /* The name of the file. */
    Address        argsPtr;	    /* Ref. to FsMakeDevArgs */
    Address        resultsPtr;	    /* == NIL */
    Fs_RedirectInfo **newNameInfoPtrPtr;/* We return this if the server leaves 
					* its domain during the lookup. */
{
    register FspdevServerIOHandle	*pdevHandlePtr;
    register Fs_MakeDeviceArgs	*makeDevArgsPtr;
    Pfs_Request			request;
    register ReturnStatus	status;
    int				resultSize;

    pdevHandlePtr = ((FspdevClientIOHandle *)prefixHandle)->pdevHandlePtr;
    makeDevArgsPtr = (Fs_MakeDeviceArgs *)argsPtr;

    request.hdr.operation = PFS_MAKE_DEVICE;
    pdevHandlePtr = PfsGetUserLevelIDs(pdevHandlePtr,
	    &makeDevArgsPtr->open.prefixID, &makeDevArgsPtr->open.rootID);
    if (pdevHandlePtr == (FspdevServerIOHandle *)NIL) {
	return(FS_FILE_NOT_FOUND);
    }
    request.param.makeDevice = *makeDevArgsPtr;

    resultSize = 0;

    status = FspdevPseudoStreamLookup(pdevHandlePtr, &request,
		strlen(relativeName) + 1, (Address)relativeName,
		&resultSize, resultsPtr, newNameInfoPtrPtr);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FspdevPfsRemove --
 *
 *	Remove a file served by a pseudo-filesystem server.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Does the remove.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
FspdevPfsRemove(prefixHandle, relativeName, argsPtr, resultsPtr, 
	       newNameInfoPtrPtr)
    Fs_HandleHeader   *prefixHandle;	/* Handle from the prefix table */
    char 	   *relativeName;	/* The name of the file to remove */
    Address 	   argsPtr;		/* Ref to Fs_LookupArgs */
    Address 	   resultsPtr;		/* == NIL */
    Fs_RedirectInfo **newNameInfoPtrPtr; /* We return this if the server leaves 
					   its domain during the lookup. */
{
    register FspdevServerIOHandle	*pdevHandlePtr;
    register Fs_LookupArgs	*lookupArgsPtr;
    Pfs_Request			request;
    register ReturnStatus	status;
    int				resultSize;

    pdevHandlePtr = ((FspdevClientIOHandle *)prefixHandle)->pdevHandlePtr;
    lookupArgsPtr = (Fs_LookupArgs *)argsPtr;

    request.hdr.operation = PFS_REMOVE;
    pdevHandlePtr = PfsGetUserLevelIDs(pdevHandlePtr,
			    &lookupArgsPtr->prefixID, &lookupArgsPtr->rootID);
    if (pdevHandlePtr == (FspdevServerIOHandle *)NIL) {
	return(FS_FILE_NOT_FOUND);
    }
    request.param.remove = *lookupArgsPtr;

    resultSize = 0;

    status = FspdevPseudoStreamLookup(pdevHandlePtr, &request,
		strlen(relativeName) + 1, (Address)relativeName,
		&resultSize, resultsPtr, newNameInfoPtrPtr);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FspdevPfsRemoveDir --
 *
 *	Remove a directory in a pseudo-filesystem.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Does the remove.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
FspdevPfsRemoveDir(prefixHandle, relativeName, argsPtr, resultsPtr, 
	       newNameInfoPtrPtr)
    Fs_HandleHeader   *prefixHandle;	/* Handle from the prefix table */
    char 	   *relativeName;	/* The name of the file to remove */
    Address 	   argsPtr;		/* Ref to Fs_LookupArgs */
    Address 	   resultsPtr;		/* == NIL */
    Fs_RedirectInfo **newNameInfoPtrPtr; /* We return this if the server leaves 
					   its domain during the lookup. */
{
    register FspdevServerIOHandle	*pdevHandlePtr;
    register Fs_LookupArgs	*lookupArgsPtr;
    Pfs_Request			request;
    register ReturnStatus	status;
    int				resultSize;

    pdevHandlePtr = ((FspdevClientIOHandle *)prefixHandle)->pdevHandlePtr;
    lookupArgsPtr = (Fs_LookupArgs *)argsPtr;

    request.hdr.operation = PFS_REMOVE_DIR;
    pdevHandlePtr = PfsGetUserLevelIDs(pdevHandlePtr,
			    &lookupArgsPtr->prefixID, &lookupArgsPtr->rootID);
    if (pdevHandlePtr == (FspdevServerIOHandle *)NIL) {
	return(FS_FILE_NOT_FOUND);
    }
    request.param.removeDir = *lookupArgsPtr;

    resultSize = 0;

    status = FspdevPseudoStreamLookup(pdevHandlePtr, &request,
		strlen(relativeName) + 1, (Address)relativeName,
		&resultSize, resultsPtr, newNameInfoPtrPtr);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FspdevPfsRename --
 *
 *	Rename a file is a pseudo-filesystem.
 *
 * Results:
 *	A return status.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
FspdevPfsRename(prefixHandle1, relativeName1, prefixHandle2, relativeName2,
	lookupArgsPtr, newNameInfoPtrPtr, name1ErrorPtr)
    Fs_HandleHeader *prefixHandle1;	/* Handle from the prefix table */
    char *relativeName1;		/* The new name of the file. */
    Fs_HandleHeader *prefixHandle2;	/* Token from the prefix table */
    char *relativeName2;		/* The new name of the file. */
    Fs_LookupArgs *lookupArgsPtr;	/* Contains IDs */
    Fs_RedirectInfo **newNameInfoPtrPtr;	/* We return this if the server leaves 
					 * its domain during the lookup. */
    Boolean *name1ErrorPtr;	/* TRUE if redirect info or other error
				 * condition if for the first pathname,
				 * FALSE means error is on second pathname. */
{
    return(FspdevPfs2Path(PFS_RENAME, prefixHandle1, relativeName1, prefixHandle2,
	    relativeName2, lookupArgsPtr, newNameInfoPtrPtr, name1ErrorPtr));
}

/*
 *----------------------------------------------------------------------
 *
 * FspdevPfsHardLink --
 *
 *	Make a hard link between two files in a pseudo-filesystem.
 *
 * Results:
 *	A return status.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
FspdevPfsHardLink(prefixHandle1, relativeName1, prefixHandle2, relativeName2,
	    lookupArgsPtr, newNameInfoPtrPtr, name1ErrorPtr)
    Fs_HandleHeader *prefixHandle1;	/* Token from the prefix table */
    char *relativeName1;		/* The new name of the file. */
    Fs_HandleHeader *prefixHandle2;	/* Token from the prefix table */
    char *relativeName2;		/* The new name of the file. */
    Fs_LookupArgs *lookupArgsPtr;	/* Contains IDs */
    Fs_RedirectInfo **newNameInfoPtrPtr;	/* We return this if the server 
					 * leaves its domain during the lookup*/
    Boolean *name1ErrorPtr;	/* TRUE if redirect info or other error is
				 * for first path, FALSE if for the second. */
{
    return(FspdevPfs2Path(PFS_HARD_LINK, prefixHandle1, relativeName1,prefixHandle2,
	    relativeName2, lookupArgsPtr, newNameInfoPtrPtr, name1ErrorPtr));
}

/*
 *----------------------------------------------------------------------
 *
 * FspdevPfs2Path --
 *
 *	Rename or Hardlink a file is a pseudo-filesystem.  This bundles the
 *	arguments up for shipment to the server.  The prefix fileIDs are
 *	mapped to the server's version of them.
 *
 * Results:
 *	A return status.
 *
 * Side effects:
 *	Either a rename or a link.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
FspdevPfs2Path(operation,prefixHandle1, relativeName1, prefixHandle2, relativeName2,
	lookupArgsPtr, newNameInfoPtrPtr, name1ErrorPtr)
    Pdev_Op operation;			/* PFS_RENAME or PFS_HARD_LINK */
    Fs_HandleHeader *prefixHandle1;	/* Handle from the prefix table */
    char *relativeName1;		/* The new name of the file. */
    Fs_HandleHeader *prefixHandle2;	/* Token from the prefix table */
    char *relativeName2;		/* The new name of the file. */
    Fs_LookupArgs *lookupArgsPtr;	/* Contains IDs */
    Fs_RedirectInfo **newNameInfoPtrPtr;	/* We return this if the server leaves 
					 * its domain during the lookup. */
    Boolean *name1ErrorPtr;	/* TRUE if redirect info or other error
				 * condition if for the first pathname,
				 * FALSE means error is on second pathname. */
{
    register FspdevServerIOHandle	*pdevHandlePtr;
    register FspdevServerIOHandle	*pdevHandle2Ptr;
    Fs_2PathData			*dataPtr;
    Pfs_Request			request;
    register ReturnStatus	status;

    pdevHandlePtr = ((FspdevClientIOHandle *)prefixHandle1)->pdevHandlePtr;

    request.hdr.operation = operation;
    pdevHandlePtr = PfsGetUserLevelIDs(pdevHandlePtr,
			    &lookupArgsPtr->prefixID, &lookupArgsPtr->rootID);
    if (pdevHandlePtr == (FspdevServerIOHandle *)NIL) {
	return(FS_FILE_NOT_FOUND);
    }
    request.param.rename.lookup = *lookupArgsPtr;
    if ((prefixHandle2 == (Fs_HandleHeader *)NIL) ||
	(prefixHandle2->fileID.type != prefixHandle1->fileID.type) ||
	(prefixHandle2->fileID.major != prefixHandle1->fileID.major) ||
	(prefixHandle2->fileID.minor != prefixHandle1->fileID.minor)) {
	/*
	 * Second prefix isn't it the same pseudo-domain. We continue with the
	 * operation in case the first pathname leaves the pseudo-domain.
	 */
	request.param.rename.prefixID2.type = -1;
    } else {
	pdevHandle2Ptr = ((FspdevClientIOHandle *)prefixHandle2)->pdevHandlePtr;
	request.param.rename.prefixID2 = pdevHandle2Ptr->userLevelID;
    }
    dataPtr = (Fs_2PathData *)malloc(sizeof(Fs_2PathData));
    (void)strcpy(dataPtr->path1, relativeName1);
    (void)strcpy(dataPtr->path2, relativeName2);

    status = FspdevPseudoStream2Path(pdevHandlePtr, &request, dataPtr,
		name1ErrorPtr, newNameInfoPtrPtr);
    free((Address)dataPtr);
    return(status);
}
