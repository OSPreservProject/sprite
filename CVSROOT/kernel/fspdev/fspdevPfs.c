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

#include "sprite.h"
#include "fs.h"
#include "fsInt.h"
#include "fsOpTable.h"
#include "fsStream.h"
#include "fsClient.h"
#include "fsFile.h"
#include "fsDisk.h"
#include "fsLock.h"
#include "fsPrefix.h"
#include "fsNameOps.h"
#include "proc.h"
#include "rpc.h"
#include "swapBuffer.h"
#include "fsPdev.h"

PdevControlIOHandle *PfsControlHandleInit();

/*
 *----------------------------------------------------------------------------
 *
 * FsRmtLinkSrvOpen --
 *
 *	The user-level server for a pseudo-filesystem is established by
 *	opening a remote link type file with the FS_PFS_MASTER flag.
 *	This procedure is invoked on the file server when remote links
 *	are opened and detects this situation.  (If FS_PFS_MASTER is
 *	not specified then FsFileSrvOpen is called and the remote link
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
 *	who is the server for the pseudo device.
 *
 *----------------------------------------------------------------------------
 *
 */
ReturnStatus
FsRmtLinkSrvOpen(handlePtr, clientID, useFlags, ioFileIDPtr, streamIDPtr,
	dataSizePtr, clientDataPtr)
     register FsLocalFileIOHandle *handlePtr;	/* A handle from FsLocalLookup.
					 * Should be LOCKED upon entry,
					 * unlocked upon exit. */
     int		clientID;	/* Host ID of client doing the open */
     register int	useFlags;	/* FS_MASTER, plus
					 * FS_READ | FS_WRITE | FS_EXECUTE*/
     register Fs_FileID	*ioFileIDPtr;	/* Return - I/O handle ID */
     Fs_FileID		*streamIDPtr;	/* Return - stream ID. 
					 * NIL during set/get attributes */
     int		*dataSizePtr;	/* Return - sizeof(FsPdevState) */
     ClientData		*clientDataPtr;	/* Return - a reference to FsPdevState.
					 * Nothing is returned during set/get
					 * attributes */
{
    register ReturnStatus status = SUCCESS;
    register Fs_Stream *streamPtr;

    if ((useFlags & FS_PFS_MASTER) == 0) {
	return(FsFileSrvOpen(handlePtr, clientID, useFlags, ioFileIDPtr,
			    streamIDPtr, dataSizePtr, clientDataPtr));
    }
    /*
     * Generate an ID which is just like a FS_CONTROL_STREAM, except that
     * the serverID will be set to us, the file server, for exported
     * pseudo-filesystems, or set to the host running the server if the
     * pseudo-filesystem is not exported.
     */
    ioFileIDPtr->type = FS_PFS_CONTROL_STREAM;
    ioFileIDPtr->major = handlePtr->hdr.fileID.major;
    ioFileIDPtr->minor = handlePtr->hdr.fileID.minor ^
			 (handlePtr->descPtr->version << 16);
    if (useFlags & FS_EXCLUSIVE) {
	/*
	 * The pseudo-filesystem server is private to the client host.
	 * We further uniqify its control handle ID to avoid conflict with
	 * files from other servers.  We set the serverID to the host
	 * running the server so we won't see closes or re-opens.
	 */
	ioFileIDPtr->serverID = clientID;
	ioFileIDPtr->major ^= rpc_SpriteID << 16;
	FsStreamNewID(clientID, streamIDPtr);
    } else {
	/*
	 * The pseudo-filesystem will be exported to the network.  Setting
	 * the serverID of the I/O handle to us means we'll see a close
	 * and possibly some reopens, we can do conflict checking, and we
	 * must have a shadow stream so the close and reopens work.
	 */
	register PdevControlIOHandle *ctrlHandlePtr;

	ioFileIDPtr->serverID = rpc_SpriteID;
	ctrlHandlePtr = PfsControlHandleInit(ioFileIDPtr, handlePtr->hdr.name);
	if (ctrlHandlePtr->serverID != NIL) {
	    status = FS_FILE_BUSY;
	} else {
	    ctrlHandlePtr->serverID = clientID;
	    streamPtr = FsStreamNewClient(rpc_SpriteID, clientID,
				    (FsHandleHeader *)ctrlHandlePtr,
				    useFlags, handlePtr->hdr.name);
	    *streamIDPtr = streamPtr->hdr.fileID;
	    FsHandleRelease(streamPtr, TRUE);
	}
	FsHandleRelease(ctrlHandlePtr, TRUE);
    }
    *clientDataPtr = (ClientData)NIL;
    *dataSizePtr = 0;
    FsHandleRelease(handlePtr, TRUE);
    return(status);
}

/*
 *----------------------------------------------------------------------------
 *
 * PfsControlHandleInit --
 *
 *	Fetch and initialize a control handle for a pseudo-filesystem.
 *
 * Results:
 *	A pointer to the control stream I/O handle.
 *
 * Side effects:
 *	Initializes and installs the control handle.
 *
 *----------------------------------------------------------------------------
 *
 */
PdevControlIOHandle *
PfsControlHandleInit(fileIDPtr, name)
    Fs_FileID *fileIDPtr;
    char *name;
{
    register Boolean found;
    register PdevControlIOHandle *ctrlHandlePtr;
    FsHandleHeader *hdrPtr;

    found = FsHandleInstall(fileIDPtr, sizeof(PdevControlIOHandle), name,
			    &hdrPtr);
    ctrlHandlePtr = (PdevControlIOHandle *)hdrPtr;
    if (!found) {
	ctrlHandlePtr->serverID = NIL;
	ctrlHandlePtr->seed = 0;
	FsLockInit(&ctrlHandlePtr->lock);
	FsRecoveryInit(&ctrlHandlePtr->rmt.recovery);
	/*
	 * These next two lists aren't used.
	 */
	List_Init(&ctrlHandlePtr->queueHdr);
	List_Init(&ctrlHandlePtr->readWaitList);
    }
    return(ctrlHandlePtr);
}

/*
 *----------------------------------------------------------------------
 *
 * FsPfsCltOpen --
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
 *	but differ in their types (FS_PFS_CONTROL_STREAM, FS_SERVER_STREAM,
 *	and FS_LCL_PSEUDO_STREAM).  The server stream is returned to our caller,
 *	the client stream is hooked to the prefix table, and the control
 *	stream is left around for conflict checking.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
FsPfsCltOpen(ioFileIDPtr, flagsPtr, clientID, streamData, name,
	ioHandlePtrPtr)
    register Fs_FileID	*ioFileIDPtr;	/* I/O fileID */
    int			*flagsPtr;	/* FS_READ | FS_WRITE ... */
    int			clientID;	/* Host doing the open */
    ClientData		streamData;	/* Pointer to FsPdevState. */
    char		*name;		/* File name for error msgs */
    FsHandleHeader	**ioHandlePtrPtr;/* Return - a locked handle set up for
					 * I/O to a pseudo device, or NIL */
{
    register ReturnStatus	status = SUCCESS;
    register PdevControlIOHandle *ctrlHandlePtr;
    register PdevClientIOHandle *cltHandlePtr;
    FsHandleHeader		*prefixHdrPtr;
    Fs_FileID			rootID;
    int				domain;
    char			*ignoredName;
    FsPrefix			*prefixPtr;
    int				prefixFlags;

    /*
     * Constrain the name to be an absolute prefix to keep things simple.
     * Then verify that the prefix is not already handled by someone.
     */
    if (name[0] != '/') {
	Sys_Panic(SYS_WARNING,
	    "Need absolute name (not \"%s\") for pseudo-filesystem\n", name);
	return(FS_INVALID_ARG);
    }
    status = FsPrefixLookup(name,
		FS_IMPORTED_PREFIX|FS_EXPORTED_PREFIX|FS_EXACT_PREFIX, -1,
		&prefixHdrPtr, &rootID, &ignoredName, &domain, &prefixPtr);
    if (status == SUCCESS) {
	Sys_Panic(SYS_WARNING, "Prefix \"%s\" already serviced\n", name);
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
     * pseudo-device connections to the pseudo-filesystem server.
     */
    ctrlHandlePtr = FsControlHandleInit(ioFileIDPtr, name);
    /*
     * Setup the request-response connection, and return the server
     * end to the calling process.
     */
    ioFileIDPtr->type = FS_LCL_PSEUDO_STREAM;
    cltHandlePtr = FsPdevConnect(ioFileIDPtr, rpc_SpriteID, name);
    if (cltHandlePtr == (PdevClientIOHandle *)NIL) {
	status = FAILURE;
	goto cleanup;
    }
    /*
     * Install the client side of the connection in the prefix table.
     */
    prefixFlags = FS_IMPORTED_PREFIX;
    if (*flagsPtr & FS_EXCLUSIVE) {
	prefixFlags |= FS_LOCAL_PREFIX;
    } else {
	prefixFlags |= FS_EXPORTED_PREFIX;
    }
    FsPrefixInstall(name, (FsHandleHeader *)cltHandlePtr, FS_PSEUDO_DOMAIN,
			    prefixFlags);
    FsHandleUnlock(cltHandlePtr);
cleanup:
    if (status != SUCCESS) {
	FsHandleRelease(ctrlHandlePtr, TRUE);
	*ioHandlePtrPtr = (FsHandleHeader *)NIL;
    } else {
	FsHandleUnlock(ctrlHandlePtr);
    }
    return(status);

}

/*
 *----------------------------------------------------------------------------
 *
 * FsPfsExport --
 *
 *	This is called from the Fs_RpcPrefix stub to complete setup for
 *	a client that will be importing a prefix of a pseudo-filesystem
 *	that has its server process on this host.  This has to add the
 *	client to the client end of the naming request response stream
 *	so future naming operations by that client are accepted here.
 *	The remote Sprite host will call FsPfsNamingCltOpen to set
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
FsPfsExport(hdrPtr, clientID, ioFileIDPtr, dataSizePtr, clientDataPtr)
     FsHandleHeader	*hdrPtr;	/* A handle from the prefix table. */
     int		clientID;	/* Host ID of client importing prefix */
     register Fs_FileID	*ioFileIDPtr;	/* Return - I/O handle ID */
     int		*dataSizePtr;	/* Return - 0 */
     ClientData		*clientDataPtr;	/* Return - NIL */
{
    register PdevClientIOHandle *cltHandlePtr = (PdevClientIOHandle *)hdrPtr;

    FsHandleLock(cltHandlePtr);
    (void)FsIOClientOpen(&cltHandlePtr->clientList, clientID, 0, FALSE);
    *ioFileIDPtr = cltHandlePtr->hdr.fileID;
    ioFileIDPtr->type = FS_PFS_NAMING_STREAM;
    *dataSizePtr = 0;
    *clientDataPtr = (ClientData)NIL;
    FsHandleUnlock(cltHandlePtr);
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * FsPfsNamingCltOpen --
 *
 *	This is called from FsSpriteImport to complete setup of the I/O
 *	handle that hangs off the prefix table.  This stream
 *	is called a "naming stream" because it will be used to
 *	forward naming operations to the pseudo-filesystem server.  This
 *	routine is similar to FsRmtPseudoStreamCltOpen, except that at
 *	this point the server already knows about us, so we don't have
 *	to contact it with FsDeviceRemoteOpen.
 *
 * Results:
 *	SUCCESS.
 *
 * Side effects:
 *	Creates a FS_PFS_NAMING_STREAM, which is like a FS_RMT_PSEUDO_STREAM
 *	in that its operations are forwarded via RPC to the host running
 *	the pseudo-filesystem server.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
FsPfsNamingCltOpen(ioFileIDPtr, flagsPtr, clientID, streamData, name,
	ioHandlePtrPtr)
    register Fs_FileID	*ioFileIDPtr;	/* I/O fileID */
    int			*flagsPtr;	/* FS_READ | FS_WRITE ... */
    int			clientID;	/* Host doing the open */
    ClientData		streamData;	/* Pointer to FsPdevState. */
    char		*name;		/* File name for error msgs */
    FsHandleHeader	**ioHandlePtrPtr;/* Return - a locked handle set up for
					 * I/O to a pseudo device, or NIL */
{
    Boolean found;
    FsRemoteIOHandle *rmtHandlePtr;

    found = FsHandleInstall(ioFileIDPtr, sizeof(FsRemoteIOHandle), name,
	    (FsHandleHeader **)&rmtHandlePtr);
    if (!found) {
	FsRecoveryInit(&rmtHandlePtr->recovery);
    }
    rmtHandlePtr->recovery.use.ref++;
    *ioHandlePtrPtr = (FsHandleHeader *)rmtHandlePtr;
    FsHandleUnlock(rmtHandlePtr);
    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * FsPfsOpen --
 *
 *	Open a file served by a pseudo-filesystem server.  This is called
 *	from FsLookupOperation based on the prefix table.  The stream returned
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
FsPfsOpen(prefixHandle, relativeName, argsPtr, resultsPtr, 
	     newNameInfoPtrPtr)
    FsHandleHeader  *prefixHandle;	/* Token from the prefix table */
    char 	  *relativeName;	/* The name of the file to open. */
    Address 	  argsPtr;		/* Ref. to FsOpenArgs */
    Address 	  resultsPtr;		/* Ref. to FsOpenResults */
    FsRedirectInfo **newNameInfoPtrPtr;	/* We return this if the server leaves 
					 * its domain during the lookup. */
{
    PdevServerIOHandle		*pdevHandlePtr;
    Pfs_Request			request;
    register ReturnStatus	status;
    int				resultSize;

    pdevHandlePtr = ((PdevClientIOHandle *)prefixHandle)->pdevHandlePtr;

    request.hdr.operation = PFS_OPEN;
    request.param.open = *(FsOpenArgs *)argsPtr;

    resultSize = sizeof(FsOpenResults);

    status = FsPseudoStreamLookup(pdevHandlePtr, &request,
		String_Length(relativeName), (Address)relativeName,
		&resultSize, resultsPtr, newNameInfoPtrPtr);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsPfsOpenConnection --
 *
 *	This is called when the server does an IOC_PFS_OPEN to respond
 *	to an open request issued by FsPfsOpen.  This sets up the server's
 *	half of the pseudo-device connection, while FsPfsOpen completes
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
FsPfsOpenConnection(pdevHandlePtr, fileIDPtr)
    PdevServerIOHandle	*pdevHandlePtr;	/* From naming request-response */
    Fs_FileID		*fileIDPtr;	/* FileID for new connection */
{
    PdevControlIOHandle *ctrlHandlePtr;
    PdevClientIOHandle *cltHandlePtr;
    Fs_Stream *srvStreamPtr;
    int newStreamID;

    /*
     * Fetch the control stream associated with the pfs naming stream
     * in order to get a seed for the Fs_FileID's generated here.
     */
    *fileIDPtr = pdevHandlePtr->hdr.fileID;
    fileIDPtr->type = FS_PFS_CONTROL_STREAM;
    ctrlHandlePtr = FsHandleFetchType(PdevControlIOHandle, fileIDPtr);
    if (ctrlHandlePtr == (PdevControlIOHandle *)NIL) {
	Sys_Panic(SYS_WARNING, "FsPfsOpenConnection, no control handle\n");
	return(-1);
    }

    /*
     * Pick an I/O fileID for the connection.
     */
    ctrlHandlePtr->seed++;
    fileIDPtr->type = FS_LCL_PFS_STREAM;
    fileIDPtr->serverID = rpc_SpriteID;
    fileIDPtr->major = ctrlHandlePtr->rmt.hdr.fileID.major;
    fileIDPtr->minor = (ctrlHandlePtr->rmt.hdr.fileID.minor << 12)
			^ ctrlHandlePtr->seed;

    cltHandlePtr = FsPdevConnect(fileIDPtr, rpc_SpriteID,
				ctrlHandlePtr->rmt.hdr.name);
    FsHandleRelease(ctrlHandlePtr, TRUE);
    if (cltHandlePtr == (PdevClientIOHandle *)NIL) {
	Sys_Panic(SYS_WARNING, "FsPfsOpenConnection failing\n");
	return(-1);
    }

    /*
     * Pick a fileID for the server's stream, and map this to a
     * user-level streamID.
     */
    srvStreamPtr = FsStreamNewClient(rpc_SpriteID, rpc_SpriteID,
		    (FsHandleHeader *)cltHandlePtr->pdevHandlePtr,
		    FS_READ|FS_USER, ctrlHandlePtr->rmt.hdr.name);
    if (FsGetStreamID(srvStreamPtr, &newStreamID) != SUCCESS) {
	(void)FsStreamClientClose(&srvStreamPtr->clientList, rpc_SpriteID);
	FsStreamDispose(srvStreamPtr);
	FsHandleRemove(cltHandlePtr->pdevHandlePtr);
	FsHandleRemove(cltHandlePtr);
	newStreamID = -1;
    } else {
	FsHandleUnlock(cltHandlePtr);
	FsHandleUnlock(srvStreamPtr);
    }
    return(newStreamID);
}

/*
 *----------------------------------------------------------------------
 *
 * FsPfsStreamCltOpen --
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
 *	None.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
FsPfsStreamCltOpen(ioFileIDPtr, flagsPtr, clientID, streamData, name,
	ioHandlePtrPtr)
    register Fs_FileID	*ioFileIDPtr;	/* I/O fileID */
    int			*flagsPtr;	/* FS_READ | FS_WRITE ... */
    int			clientID;	/* Host doing the open */
    ClientData		streamData;	/* Pointer to FsPdevState. */
    char		*name;		/* File name for error msgs */
    FsHandleHeader	**ioHandlePtrPtr;/* Return - a locked handle set up for
					 * I/O to a pseudo device, or NIL */
{
    register PdevClientIOHandle *cltHandlePtr;

    cltHandlePtr = FsHandleFetchType(PdevClientIOHandle, ioFileIDPtr);
    if (cltHandlePtr == (PdevClientIOHandle *)NIL) {
	Sys_Panic(SYS_WARNING, "FsPfsStreamCltOpen, no handle\n");
	*ioHandlePtrPtr = (FsHandleHeader *)NIL;
	return(FS_FILE_NOT_FOUND);
    } else {
	if (cltHandlePtr->hdr.name != (char *)NIL) {
	    Mem_Free((Address)cltHandlePtr->hdr.name);
	}
	cltHandlePtr->hdr.name = (char *)Mem_Alloc(String_Length(name) + 1);
	(void)String_Copy(name, cltHandlePtr->hdr.name);
	*ioHandlePtrPtr = (FsHandleHeader *)cltHandlePtr;
	return(SUCCESS);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * FsPfsGetAttrPath --
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
FsPfsGetAttrPath(prefixHandle, relativeName, argsPtr, resultsPtr,
         	    newNameInfoPtrPtr)
    FsHandleHeader *prefixHandle;	/* Handle from the prefix table */
    char           *relativeName;	/* The name of the file. */
    Address        argsPtr;		/* Bundled arguments for us */
    Address        resultsPtr;		/* Where to store attributes */
    FsRedirectInfo **newNameInfoPtrPtr;	/* We return this if the server leaves 
					 * its domain during the lookup. */
{
    register PdevServerIOHandle	*pdevHandlePtr;
    Pfs_Request			request;
    register ReturnStatus	status;
    FsGetAttrResults		*getAttrResultsPtr;
    int				resultSize;

    pdevHandlePtr = ((PdevClientIOHandle *)prefixHandle)->pdevHandlePtr;

    request.hdr.operation = PFS_GET_ATTR;
    request.param.open = *(FsOpenArgs *)argsPtr;

    getAttrResultsPtr = (FsGetAttrResults *)resultsPtr;
    resultSize = sizeof(Fs_Attributes);

    status = FsPseudoStreamLookup(pdevHandlePtr, &request,
		String_Length(relativeName), (Address)relativeName,
		&resultSize, (Address)getAttrResultsPtr->attrPtr,
		newNameInfoPtrPtr);
    /*
     * The pseudo-filesystem server has given us all the attributes so
     * we don't fill in the ioFileID.
     */
    getAttrResultsPtr->fileIDPtr->type = -1;
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsPfsSetAttrPath --
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
FsPfsSetAttrPath(prefixHandle, relativeName, argsPtr, resultsPtr,
         	    newNameInfoPtrPtr)
    FsHandleHeader *prefixHandle;	/* Handle from the prefix table */
    char           *relativeName;	/* The name of the file. */
    Address        argsPtr;		/* Bundled arguments for us */
    Address        resultsPtr;		/* Where to store attributes */
    FsRedirectInfo **newNameInfoPtrPtr;	/* We return this if the server leaves 
					 * its domain during the lookup. */
{
    register PdevServerIOHandle	*pdevHandlePtr;
    Pfs_Request			request;
    register ReturnStatus	status;
    FsSetAttrArgs		*setAttrArgsPtr;
    int				nameLength;
    int				zero = 0;
    Pfs_SetAttrData		*setAttrDataPtr;

    pdevHandlePtr = ((PdevClientIOHandle *)prefixHandle)->pdevHandlePtr;

    setAttrArgsPtr = (FsSetAttrArgs *)argsPtr;

    request.hdr.operation = PFS_SET_ATTR;
    request.param.open = setAttrArgsPtr->openArgs;

    nameLength = String_Length(relativeName);
    setAttrDataPtr = (Pfs_SetAttrData *)Mem_Alloc(sizeof(Pfs_SetAttrData) +
						    nameLength);
    setAttrDataPtr->attr = setAttrArgsPtr->attr;
    setAttrDataPtr->flags = setAttrArgsPtr->flags;
    setAttrDataPtr->nameLength = nameLength;
    (void)String_Copy(relativeName, setAttrDataPtr->name);

    status = FsPseudoStreamLookup(pdevHandlePtr, &request,
	    sizeof(Pfs_SetAttrData) + nameLength, (Address)setAttrDataPtr,
	    &zero, (Address)NIL, newNameInfoPtrPtr);
    Mem_Free((Address)setAttrDataPtr);
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
 * FsPfsMakeDir --
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
FsPfsMakeDir(prefixHandle, relativeName, argsPtr, resultsPtr, 
		newNameInfoPtrPtr)
    FsHandleHeader *prefixHandle;   /* Handle from the prefix table */
    char 	   *relativeName;   /* The name of the directory to create */
    Address 	   argsPtr;	    /* Ref. to FsOpenArgs */
    Address 	   resultsPtr;	    /* == NIL */
    FsRedirectInfo **newNameInfoPtrPtr;/* We return this if the server leaves 
					* its domain during the lookup. */
{
    register PdevServerIOHandle	*pdevHandlePtr;
    Pfs_Request			request;
    register ReturnStatus	status;
    int				resultSize;

    pdevHandlePtr = ((PdevClientIOHandle *)prefixHandle)->pdevHandlePtr;

    request.hdr.operation = PFS_MAKE_DIR;
    request.param.lookup = *(FsLookupArgs *)argsPtr;

    resultSize = 0;

    status = FsPseudoStreamLookup(pdevHandlePtr, &request,
		String_Length(relativeName), (Address)relativeName,
		&resultSize, resultsPtr, newNameInfoPtrPtr);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsPfsMakeDevice --
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
FsPfsMakeDevice(prefixHandle, relativeName, argsPtr, resultsPtr,
			       newNameInfoPtrPtr)
    FsHandleHeader *prefixHandle;   /* Handle from the prefix table */
    char           *relativeName;   /* The name of the file. */
    Address        argsPtr;	    /* Ref. to FsMakeDevArgs */
    Address        resultsPtr;	    /* == NIL */
    FsRedirectInfo **newNameInfoPtrPtr;/* We return this if the server leaves 
					* its domain during the lookup. */
{
    register PdevServerIOHandle	*pdevHandlePtr;
    Pfs_Request			request;
    register ReturnStatus	status;
    int				resultSize;

    pdevHandlePtr = ((PdevClientIOHandle *)prefixHandle)->pdevHandlePtr;

    request.hdr.operation = PFS_MAKE_DEVICE;
    request.param.makeDevice = *(FsMakeDeviceArgs *)argsPtr;

    resultSize = 0;

    status = FsPseudoStreamLookup(pdevHandlePtr, &request,
		String_Length(relativeName), (Address)relativeName,
		&resultSize, resultsPtr, newNameInfoPtrPtr);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsPfsRemove --
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
FsPfsRemove(prefixHandle, relativeName, argsPtr, resultsPtr, 
	       newNameInfoPtrPtr)
    FsHandleHeader   *prefixHandle;	/* Handle from the prefix table */
    char 	   *relativeName;	/* The name of the file to remove */
    Address 	   argsPtr;		/* Ref to FsLookupArgs */
    Address 	   resultsPtr;		/* == NIL */
    FsRedirectInfo **newNameInfoPtrPtr; /* We return this if the server leaves 
					   its domain during the lookup. */
{
    register PdevServerIOHandle	*pdevHandlePtr;
    Pfs_Request			request;
    register ReturnStatus	status;
    int				resultSize;

    pdevHandlePtr = ((PdevClientIOHandle *)prefixHandle)->pdevHandlePtr;

    request.hdr.operation = PFS_REMOVE;
    request.param.lookup = *(FsLookupArgs *)argsPtr;

    resultSize = 0;

    status = FsPseudoStreamLookup(pdevHandlePtr, &request,
		String_Length(relativeName), (Address)relativeName,
		&resultSize, resultsPtr, newNameInfoPtrPtr);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsPfsRemoveDir --
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
FsPfsRemoveDir(prefixHandle, relativeName, argsPtr, resultsPtr, 
	       newNameInfoPtrPtr)
    FsHandleHeader   *prefixHandle;	/* Handle from the prefix table */
    char 	   *relativeName;	/* The name of the file to remove */
    Address 	   argsPtr;		/* Ref to FsLookupArgs */
    Address 	   resultsPtr;		/* == NIL */
    FsRedirectInfo **newNameInfoPtrPtr; /* We return this if the server leaves 
					   its domain during the lookup. */
{
    register PdevServerIOHandle	*pdevHandlePtr;
    Pfs_Request			request;
    register ReturnStatus	status;
    int				resultSize;

    pdevHandlePtr = ((PdevClientIOHandle *)prefixHandle)->pdevHandlePtr;

    request.hdr.operation = PFS_REMOVE_DIR;
    request.param.lookup = *(FsLookupArgs *)argsPtr;

    resultSize = 0;

    status = FsPseudoStreamLookup(pdevHandlePtr, &request,
		String_Length(relativeName), (Address)relativeName,
		&resultSize, resultsPtr, newNameInfoPtrPtr);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsPfsRename --
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
/*ARGSUSED*/
ReturnStatus
FsPfsRename(prefixHandle1, relativeName1, prefixHandle2, relativeName2,
	lookupArgsPtr, newNameInfoPtrPtr, name1ErrorPtr)
    FsHandleHeader *prefixHandle1;	/* Handle from the prefix table */
    char *relativeName1;		/* The new name of the file. */
    FsHandleHeader *prefixHandle2;	/* Token from the prefix table */
    char *relativeName2;		/* The new name of the file. */
    FsLookupArgs *lookupArgsPtr;	/* Contains IDs */
    FsRedirectInfo **newNameInfoPtrPtr;	/* We return this if the server leaves 
					 * its domain during the lookup. */
    Boolean *name1ErrorPtr;	/* TRUE if redirect info or other error
				 * condition if for the first pathname,
				 * FALSE means error is on second pathname. */
{
    return(FAILURE);
}

/*
 *----------------------------------------------------------------------
 *
 * FsPfsHardLink --
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
FsPfsHardLink(prefixHandle1, relativeName1, prefixHandle2, relativeName2,
	    lookupArgsPtr, newNameInfoPtrPtr, name1ErrorPtr)
    FsHandleHeader *prefixHandle1;	/* Token from the prefix table */
    char *relativeName1;		/* The new name of the file. */
    FsHandleHeader *prefixHandle2;	/* Token from the prefix table */
    char *relativeName2;		/* The new name of the file. */
    FsLookupArgs *lookupArgsPtr;	/* Contains IDs */
    FsRedirectInfo **newNameInfoPtrPtr;	/* We return this if the server 
					 * leaves its domain during the lookup*/
    Boolean *name1ErrorPtr;	/* TRUE if redirect info or other error is
				 * for first path, FALSE if for the second. */
{
    return(FAILURE);
}
