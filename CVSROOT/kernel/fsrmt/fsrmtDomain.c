/* 
 * fsSpriteDomain.c --
 *
 *	This has the stubs for remote naming operations in a Sprite domain.
 *	These routines are presented in pairs, the client stub followed
 *	by the server stub.  The general style is for the server stub
 *	to call the LocalDomain equivalent of the SpriteDomain client stub.
 *	i.e. FsrmtOpen invokes via RPC Fsrmt_RpcOpen which calls
 *	FslclOpen.  Occasionally a client or server stub will do some
 *	extra processing, or use lower level primatives for efficiency.
 *
 * Copyright (C) 1987 Regents of the University of California
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
#include "fsio.h"
#include "fsutil.h"
#include "fsNameOps.h"
#include "fsNameOpsInt.h"
#include "fsprefix.h"
#include "fsrmtInt.h"
#include "fslcl.h"
#include "fsutilTrace.h"
#include "fsStat.h"
#include "recov.h"
#include "proc.h"
#include "rpc.h"
#include "vm.h"
#include "dbg.h"


/*
 * Used to contain fileID and stream data results from open calls.
 */
typedef	struct	FsPrefixReplyParam {
    FsrmtUnionData	openData;
    Fs_FileID	fileID;
} FsPrefixReplyParam;


/*
 *----------------------------------------------------------------------
 *
 * FsrmtImport --
 *
 *	Get a handle for a prefix.  This conducts an RPC_FS_PREFIX
 *	to see if there is a server for the prefix.  If there is one this
 *	routine installs a handle for it.  The pointer to the handle
 *	is returned.
 *
 * Results:
 *	FAILURE or RPC_TIMEOUT if we couldn't find a server for the prefix.
 *	SUCCESS if we did find a server.  In this case the results are
 *	a pointer to a handle for the prefix.
 *
 * Side effects:
 *	State is left on the server machine about the open prefix.  The
 *	responding file server is registered with the recovery module
 *	so we find out when it goes away and when it reboots.
 *	
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
FsrmtImport(prefix, serverID, idPtr, domainTypePtr, hdrPtrPtr)
    char	*prefix;		/* Prefix for which to find a server. */
    int		serverID;		/* Suggested server ID.  This is the
					 * broadcast address for nearby domains,
					 * or a specific hostID, or a remote
					 * network for remote broadcasting */
    Fs_UserIDs	*idPtr;			/* IGNORED */
    int		*domainTypePtr;		/* Return - FS_REMOTE_SPRITE_DOMAIN or
					 *          FS_REMOTE_PSEUDO_DOMAIN */
    Fs_HandleHeader **hdrPtrPtr;		/* Return - handle for prefix table */
{
    ReturnStatus 	status;
    Rpc_Storage 	storage;
    Fs_FileID		*fileIDPtr;	/* Returned from server */
    ClientData		streamData;	/* Returned from server */
    int			flags = FS_PREFIX;
    FsPrefixReplyParam	prefixReplyParam;

    *hdrPtrPtr = (Fs_HandleHeader *)NIL;
    *domainTypePtr = -1;

    storage.requestParamPtr = (Address) NIL;
    storage.requestParamSize = 0;
    storage.requestDataPtr = (Address)prefix;
    storage.requestDataSize = strlen(prefix)+1;

    storage.replyParamPtr = (Address)&prefixReplyParam;
    storage.replyParamSize = sizeof(FsPrefixReplyParam);
    storage.replyDataPtr = (Address)NIL;
    storage.replyDataSize = 0;
    fileIDPtr = &(prefixReplyParam.fileID);

    status = Rpc_Call(serverID, RPC_FS_PREFIX, &storage);
    /*
     * It is necessary to allocate and copy over the stream data, since
     * the ioOpen proc frees this space.
     */
    streamData = (ClientData)malloc(sizeof(FsrmtUnionData));
    *((FsrmtUnionData *) streamData) = prefixReplyParam.openData;

    if (status == SUCCESS) {
	/*
	 * Use the client-open routine to set up an I/O handle for the prefix.
	 */
	status = (*fsio_StreamOpTable[fileIDPtr->type].ioOpen)(fileIDPtr, &flags,
		    rpc_SpriteID, (ClientData)streamData, prefix, hdrPtrPtr);
	if (status == SUCCESS) {
	    /*
	     * Register the server with the recovery module so we find out
	     * when it goes away and when it reboots.
	     */
	    Recov_RebootRegister((*hdrPtrPtr)->fileID.serverID, Fsutil_Reopen,
				 (ClientData)NIL);
	    *domainTypePtr = FS_REMOTE_SPRITE_DOMAIN;
	}
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsrmt_RpcPrefix --
 *
 *	Server stub for RPC_FS_PREFIX.  This looks in the prefix
 *	table for the given prefix.  If found, the handle is opened
 *	for use by the client, and the resulting streamData is returned.
 *
 * Results:
 *	If this procedure returns SUCCESS then a reply has been sent to
 *	the client.  If the arguments are bad then an error is
 *	returned and the main level sends back an error reply.
 *
 * Side effects:
 *	The nameOpen routine is called on the prefix handle.  This ups
 *	reference counts.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
Fsrmt_RpcPrefix(srvToken, clientID, command, storagePtr)
    ClientData 		 srvToken;	/* Handle on server process passed to
					 * Rpc_Reply */
    int 		 clientID;	/* Sprite ID of client host */
    int 		 command;	/* IGNORED */
    register Rpc_Storage *storagePtr;	/* The request fields refer to the 
					 * request buffers and also indicate 
					 * the exact amount of data in the 
					 * request buffers.  The reply fields 
					 * are initialized to NIL for the
					 * pointers and 0 for the lengths.  
					 * This can be passed to Rpc_Reply */
{
    char				*lookupName;
    Fsprefix				*prefixPtr;
    Fs_HandleHeader			*hdrPtr;
    Fs_FileID				rootID;
    int					domainType;
    int					serverID;
    ReturnStatus			status;
    FsPrefixReplyParam			*prefixReplyPtr;

    status = Fsprefix_Lookup((char *) storagePtr->requestDataPtr,
		FSPREFIX_EXPORTED | FSPREFIX_EXACT, clientID, &hdrPtr,
		&rootID, &lookupName, &serverID, &domainType, &prefixPtr);
    if (status == SUCCESS) {
	register Rpc_ReplyMem		*replyMemPtr;
	ClientData			streamData;
	int				dataSize;

	prefixReplyPtr = mnew(FsPrefixReplyParam);
	status = (*fs_DomainLookup[domainType][FS_DOMAIN_EXPORT])(hdrPtr,
		    clientID, &prefixReplyPtr->fileID, &dataSize, &streamData);
	if (status == SUCCESS) {
	    if (dataSize > 0) {
		bcopy((Address)streamData, (Address)&prefixReplyPtr->openData,
			dataSize);
		free((Address)streamData);
	    }
	    storagePtr->replyParamPtr = (Address) (prefixReplyPtr);
	    storagePtr->replyParamSize = sizeof(FsPrefixReplyParam);
	    storagePtr->replyDataPtr = (Address)NIL;
	    storagePtr->replyDataSize = 0;

	    replyMemPtr = (Rpc_ReplyMem *) malloc(sizeof(Rpc_ReplyMem));
	    replyMemPtr->paramPtr = storagePtr->replyParamPtr;
	    replyMemPtr->dataPtr = storagePtr->replyDataPtr;
	    Rpc_Reply(srvToken, SUCCESS, storagePtr, Rpc_FreeMem,
		    (ClientData)replyMemPtr);
	    return(SUCCESS);
	} else {
	    free((Address)prefixReplyPtr);
	    printf( "Fsrmt_RpcPrefix, export \"%s\" failed %x\n",
		    storagePtr->requestDataPtr, status);
	    Fsprefix_HandleClose(prefixPtr, FSPREFIX_ANY);
	}
    }
    return(RPC_NO_REPLY);
}

/*
 *----------------------------------------------------------------------
 *
 * FsrmtOpen --
 *
 *	Open a remote file.  This sets up and conducts an RPC_FS_OPEN
 *	remote procedure call to open the remote file.  This is called
 *	from Fsprefix_LookupOperation based on the prefix table.  FsrmtOpen
 *	makes an RPC to FslclOpen on the remote machine, and returns
 *	the streamData for use by the client-open routine.
 *
 * RPC: The input parameters are the Fs_OpenArgs defined in fsNameOps.h.
 *	The input data is a relative name.  The return parameter is a file
 *	type used by our caller to branch to the client-open routine.  The
 *	return data area has two possible return values.  In the normal
 *	case it is a lump of data used by the client-open routine to set
 *	up the I/O handle. If the name lookup re-directs to a different
 *	server then the returned data is the new pathname.
 *
 * Results:
 *	SUCCESS, FS_REDIRECT, or some error code from the lookup on the server.
 *	If FS_REDIRECT, then *newNameInfoPtr has prefix information.
 *
 * Side effects:
 *	Allocates memory for the returned streamData or re-directed path.
 *	An openCount is left up during the open as part of the open/re-open
 *	synchronization.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
FsrmtOpen(prefixHandle, relativeName, argsPtr, resultsPtr, 
	     newNameInfoPtrPtr)
    Fs_HandleHeader  *prefixHandle;	/* Token from the prefix table */
    char 	  *relativeName;	/* The name of the file to open. */
    Address 	  argsPtr;		/* Ref. to Fs_OpenArgs */
    Address 	  resultsPtr;		/* Ref. to Fs_OpenResults */
    Fs_RedirectInfo **newNameInfoPtrPtr;	/* We return this if the server leaves 
					 * its domain during the lookup. */
{
    ReturnStatus	status;
    Fs_OpenResults	*openResultsPtr = (Fs_OpenResults *)resultsPtr;
    Rpc_Storage		storage;	/* Specifies RPC parameters/results */
    char		replyName[FS_MAX_PATH_NAME_LENGTH];	 /* This
					 * may get filled with a
					 * redirected pathname */
    FsrmtOpenResultsParam	openResultsParam;
    /*
     * Synchronize with the re-open phase of recovery.
     * We don't want opens to race with the recovery actions.
     */
    status = Fsprefix_OpenCheck(prefixHandle);
    if (status != SUCCESS) {
	return(status);
    }

    /*
     * Set up for the RPC.
     */
    storage.requestParamPtr = (Address) argsPtr;
    storage.requestParamSize = sizeof(Fs_OpenArgs);
    storage.requestDataPtr = (Address) relativeName;
    storage.requestDataSize = strlen(relativeName) + 1;
    storage.replyParamPtr = (Address) &openResultsParam;
    storage.replyParamSize = sizeof(FsrmtOpenResultsParam);
    storage.replyDataPtr = (Address) replyName;
    storage.replyDataSize = FS_MAX_PATH_NAME_LENGTH;

    status = Rpc_Call(prefixHandle->fileID.serverID, RPC_FS_OPEN, &storage);
    if (status == SUCCESS) {
	/*
	 * Allocate space for the stream data returned by the server.
	 * We then copy the streamData from our stack buffer.
	 */
	/* This assumes openResults.dataSize was filled in correctly. */
	*openResultsPtr = openResultsParam.openResults;
	if (openResultsPtr->dataSize == 0) {
	    openResultsPtr->streamData = (ClientData)NIL;
	} else {
	    openResultsPtr->streamData =
		    (ClientData)malloc(openResultsPtr->dataSize);
	    bcopy((Address) &(openResultsParam.openData), (Address) openResultsPtr->streamData, openResultsPtr->dataSize);
	}
    } else if (status == FS_LOOKUP_REDIRECT) {
	/*
	 * Allocate space for the re-directed pathname and
	 * copy over the structure that we have on our stack.  A large
	 * buffer is allocated because it is used as a work area in
	 * FsprefixLookupRedirect to create a new absolute pathname.
	 */
	*newNameInfoPtrPtr = mnew(Fs_RedirectInfo);
	(*newNameInfoPtrPtr)->prefixLength = openResultsParam.prefixLength;
	(void)strcpy((*newNameInfoPtrPtr)->fileName, replyName);
    }
    Fsprefix_OpenDone(prefixHandle);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsrmt_RpcOpen --
 *
 *      Service stub for the RPC_FS_OPEN call.  This unpackages parameters
 *	and branches to the local open routine.
 *
 * Results:
 *	If this procedure returns SUCCESS then a reply has been sent to
 *	the client.  If the arguments are bad then an error is 
 *	returned and the main level sends back an error reply.
 *
 * Side effects:
 *	None here, see FslclOpen.
 *	
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
Fsrmt_RpcOpen(srvToken, clientID, command, storagePtr)
    ClientData 		 srvToken;	/* Handle on server process passed to
					 * Rpc_Reply */
    int 		 clientID;	/* Sprite ID of client host */
    int 		 command;	/* IGNORED */
    register Rpc_Storage *storagePtr;	/* The request fields refer to the 
					 * request buffers and also indicate 
					 * the exact amount of data in the 
					 * request buffers.  The reply fields 
					 * are initialized to NIL for the
				 	 * pointers and 0 for the lengths.  
					 * This can be passed to Rpc_Reply */
{
    ReturnStatus		status;
    register Fs_OpenArgs		*openArgsPtr;		/* RPC parameters */
    register Fs_OpenResults	*openResultsPtr;	/* RPC results */
    Fs_HandleHeader		*prefixHandlePtr;	/* Handle for domain */
    Fs_RedirectInfo		*newNameInfoPtr;	/* prefix info for
							 * redirected lookups */
    FsrmtOpenResultsParam		*openResultsParamPtr;	/* open results, etc. */
    int				domainType;		/* Local or Pseudo */


    if (Recov_GetClientState(clientID) & CLT_RECOV_IN_PROGRESS) {
	Net_HostPrint(clientID, "Dropping regular open during recovery\n");
	return(RPC_SERVICE_DISABLED);
    }
    openArgsPtr = (Fs_OpenArgs *) storagePtr->requestParamPtr;
    /*
     * Get a handle on the prefix.  We need to have it unlocked in case
     * we do I/O on the directory.
     */
    prefixHandlePtr =
	(*fsio_StreamOpTable[openArgsPtr->prefixID.type].clientVerify)
	    (&openArgsPtr->prefixID, clientID, &domainType);
    if (prefixHandlePtr == (Fs_HandleHeader *)NIL) {
	return(FS_STALE_HANDLE);
    }
    Fsutil_HandleUnlock(prefixHandlePtr);

    newNameInfoPtr = (Fs_RedirectInfo *) NIL;
    openResultsParamPtr = mnew(FsrmtOpenResultsParam);
    openResultsPtr = &(openResultsParamPtr->openResults);

    fs_Stats.srvName.numReadOpens++;
    status = (*fs_DomainLookup[domainType][FS_DOMAIN_OPEN])(prefixHandlePtr,
		(char *)storagePtr->requestDataPtr, (Address)openArgsPtr,
		(Address)openResultsPtr, &newNameInfoPtr);
    Fsutil_HandleRelease(prefixHandlePtr, FALSE);
    if (status == SUCCESS) {
	/*
	 * The open worked.  We return the whole Fs_OpenResults structure
	 * in the RPC parameter area, but it contains a pointer to
	 * stream data and a dataSize. That stream data is returned also
	 * as a separate field in the RPC parameter area, so it must be copied.
	 */
	storagePtr->replyParamPtr = (Address)openResultsParamPtr;
	storagePtr->replyParamSize = sizeof(FsrmtOpenResultsParam);
	/* copy openData */
	if (openResultsPtr->dataSize != 0 &&
		((Address)openResultsPtr->streamData) != (Address)NIL) {
	    bcopy((Address)openResultsPtr->streamData, (Address)&openResultsParamPtr->openData, openResultsPtr->dataSize);
	    free((Address)openResultsPtr->streamData);
	    storagePtr->replyDataPtr = (Address)NIL;
	    storagePtr->replyDataSize = 0;
	}
    } else if (status == FS_LOOKUP_REDIRECT) {
	/*
	 * The file is not found on this server.
	 */
	storagePtr->replyParamPtr = (Address)openResultsParamPtr;
	storagePtr->replyParamSize = sizeof(FsrmtOpenResultsParam);
	openResultsParamPtr->prefixLength = newNameInfoPtr->prefixLength;
	storagePtr->replyDataSize = strlen(newNameInfoPtr->fileName) + 1;
	storagePtr->replyDataPtr = (Address)malloc(storagePtr->replyDataSize);
	(void)strcpy(storagePtr->replyDataPtr, newNameInfoPtr->fileName);
	free((Address)newNameInfoPtr);
    }
    if (status == SUCCESS || status == FS_LOOKUP_REDIRECT) {
	Rpc_ReplyMem	*replyMemPtr;

        replyMemPtr = (Rpc_ReplyMem *) malloc(sizeof(Rpc_ReplyMem));
        replyMemPtr->paramPtr = storagePtr->replyParamPtr;
        replyMemPtr->dataPtr = storagePtr->replyDataPtr;
        Rpc_Reply(srvToken, status, storagePtr, Rpc_FreeMem,
		(ClientData)replyMemPtr);
        return(SUCCESS);
    } else {
	free((Address)openResultsParamPtr);
        return(status);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * FsrmtReopen --
 *
 *	Open a handle at its server.  This sets up and conducts an RPC_FS_REOPEN
 *	remote procedure call to reopen the remote file handle.
 *
 * Results:
 *	The return from the RPC.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
FsrmtReopen(hdrPtr, inSize, inData, outSizePtr, outData)
    Fs_HandleHeader  *hdrPtr;		/* Handle to reopen */
    int			inSize;		/* Size of input data */
    Address		inData;		/* Input data to server's reopen proc */
    int			*outSizePtr;	/* In/Out return data size */
    Address		outData;	/* Return parameter block */
{
    register ReturnStatus	status;
    Rpc_Storage		storage;	/* Specifies RPC parameters/results */

    storage.requestParamPtr = inData;
    storage.requestParamSize = inSize;
    storage.requestDataPtr = (Address) NIL;
    storage.requestDataSize = 0;
    storage.replyParamPtr = outData;
    storage.replyParamSize = *outSizePtr;
    storage.replyDataPtr = (Address) NIL;
    storage.replyDataSize = 0;

    status = Rpc_Call(hdrPtr->fileID.serverID, RPC_FS_REOPEN, &storage);
    *outSizePtr = storage.replyParamSize;
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsrmt_RpcReopen --
 *
 *	This is the service stub for RPC_FS_REOPEN.  This switches
 *	out to a stream type reopen procedure.  To do this it must
 *	assume that the first part of the parameter block contains
 *	a fileID, the thing to be re-opened.
 *
 * Results:
 *	If this procedure returns SUCCESS then a reply has been sent to
 *	the client.  If the arguments are bad then an error is 
 *	returned and the main level sends back an error reply.
 *
 * Side effects:
 *	None.
 *	
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
Fsrmt_RpcReopen(srvToken, clientID, command, storagePtr)
    ClientData 		 srvToken;	/* Handle on server process passed to
				 	 * Rpc_Reply */
    int 		 clientID;	/* Sprite ID of client host */
    int 		 command;	/* IGNORED */
    register Rpc_Storage *storagePtr;	/* The request fields refer to the 
					 * request buffers and also indicate 
					 * the exact amount of data in the 
					 * request buffers.  The reply fields 
					 * are initialized to NIL for the
				 	 * pointers and 0 for the lengths.  
					 * This can be passed to Rpc_Reply */
{
    register Fs_FileID *fileIDPtr;
    register ReturnStatus status;

    extern int fsutil_NumRecovering; /* XXX put in fsutil.h */

    if ((Recov_GetClientState(clientID) & CLT_RECOV_IN_PROGRESS) == 0) {
	Recov_SetClientState(clientID, CLT_RECOV_IN_PROGRESS);
	fsutil_NumRecovering++;
	if (fsutil_NumRecovering == 1) {
	    /*
	     * The print statements are tweaked so that the first client
	     * to recover triggers this message, and the last
	     * client to end recovery triggers another message.
	     * See fsRecovery.c for the complementary printf.
	     */
	    Net_HostPrint(clientID, "initiating recovery\n");
	}
    }

    fileIDPtr = (Fs_FileID *)storagePtr->requestParamPtr;
    if (fileIDPtr->serverID != rpc_SpriteID) {
	/*
	 * Filesystem version mis-match.
	 */
	return(GEN_INVALID_ARG);
    }
    fileIDPtr->type = Fsio_MapRmtToLclType(fileIDPtr->type);
    if (fileIDPtr->type < 0) {
	return(GEN_INVALID_ARG);
    }
    status = (*fsio_StreamOpTable[fileIDPtr->type].reopen)((Fs_HandleHeader *)NIL,
		clientID, storagePtr->requestParamPtr,
		&storagePtr->replyParamSize,
		&storagePtr->replyParamPtr);

    if (status == SUCCESS) {
	Rpc_ReplyMem	*replyMemPtr;

        replyMemPtr = (Rpc_ReplyMem *) malloc(sizeof(Rpc_ReplyMem));
        replyMemPtr->paramPtr = storagePtr->replyParamPtr;
        replyMemPtr->dataPtr = storagePtr->replyDataPtr;
        Rpc_Reply(srvToken, SUCCESS, storagePtr, Rpc_FreeMem,
		(ClientData)replyMemPtr);
    }
    return(status);
}

/*
 * Union of things passed as close data.  Right now, it only seems to
 * be cached attributes.
 */
typedef union FsCloseData {
    Fscache_Attributes	attrs;
} FsCloseData;


/*
 * Request params for the close RPC.  The data for the close is put in the
 * closeData field so that it too can be byte-swapped. The field is for stream
 * specific data that gets pushed back to the server when the client closes.
 * Currently, it seems only to be Fscache_Attributes.
 * 
 */
typedef struct FsRemoteCloseParams {
    Fs_FileID	fileID;		/* File to close */
    Fs_FileID	streamID;	/* Stream to close */
    Proc_PID	procID;		/* Process doing the close */
    int		flags;		/* Flags from the stream */
    FsCloseData	closeData;	/* Seems to be only Fscache_Attributes... */
    int		closeDataSize;	/* actual size of info in closeData field. */
} FsRemoteCloseParams;

/*
 *----------------------------------------------------------------------
 *
 * Fsrmt_Close --
 *
 *	Tell the server that we have closed one reference to its file.  This
 *	is used by the remote file and remote device close routines.  This
 *	routine uses RPC_FS_CLOSE to invoke the correct stream-type close
 *	routine on the I/O server.
 *
 * Results:
 *	SUCCESS.
 *
 * Side effects:
 *	This marks the handle as needing recovery if the RPC fails due
 *	to communication problems.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
Fsrmt_Close(streamPtr, clientID, procID, flags, dataSize, closeData)
    Fs_Stream		*streamPtr;	/* Stream to close.  This is needed
					 * (instead of I/O handle) so the
					 * server can close its shadow stream */
    int			clientID;	/* IGNORED, implicitly passed by RPC */
    Proc_PID		procID;		/* Process ID of closer */
    int			flags;		/* Flags from the stream being closed */
    int			dataSize;	/* Size of *closeData, or Zero */
    ClientData		closeData;	/* Copy of cached I/O attributes.
   					 * Sometimes NIL!  */
{
    Fsrmt_IOHandle	*rmtHandlePtr;	/* Handle to close */
    Rpc_Storage 	storage;
    ReturnStatus 	status;
    FsRemoteCloseParams	params;

    rmtHandlePtr = (Fsrmt_IOHandle *)streamPtr->ioHandlePtr;
    params.fileID = rmtHandlePtr->hdr.fileID;
    params.streamID = streamPtr->hdr.fileID;
    params.procID = procID;
    params.flags = flags;
    if (closeData != (ClientData) NIL) {
	params.closeData = *((FsCloseData *)closeData);
	params.closeDataSize = dataSize;
    } else {
	params.closeDataSize = 0;
    }

    storage.requestParamPtr = (Address)&params;
    storage.requestParamSize = sizeof(params);
    storage.requestDataPtr = (Address)NIL;
    storage.requestDataSize = 0;
    storage.replyParamPtr = (Address)NIL;
    storage.replyParamSize = 0;
    storage.replyDataPtr = (Address)NIL;
    storage.replyDataSize = 0;

    status = Rpc_Call(rmtHandlePtr->hdr.fileID.serverID, RPC_FS_CLOSE,&storage);

    if (status == RPC_TIMEOUT || status == FS_STALE_HANDLE ||
	status == RPC_SERVICE_DISABLED) {
	/*
	 * Mark the handle as needing recovery if we can't tell the server
	 * about this close.
	 */
	Fsutil_WantRecovery((Fs_HandleHeader *)rmtHandlePtr);
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsrmt_RpcClose --
 *
 *	Server stub for RPC_FS_CLOSE.  This verifies the client and branches
 *	to the stream-type close routine.
 *
 * Results:
 *	STALE_HANDLE if the handle is out-of-date. FS_FILE_REMOVED is returned
 *	if the file has been removed.  SUCCESS in the normal case, or
 *	an error code from the stream-type close routine.
 *
 * Side effects:
 *	None here, see the stream-type close routines.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
Fsrmt_RpcClose(srvToken, clientID, command, storagePtr)
    ClientData 		 srvToken;	/* Handle on server process passed to
				 	 * Rpc_Reply */
    int 		 clientID;	/* Sprite ID of client host */
    int 		 command;	/* IGNORED */
    register Rpc_Storage *storagePtr;	/* The request fields refer to the 
					 * request buffers and also indicate 
					 * the exact amount of data in the 
					 * request buffers.  The reply fields 
					 * are initialized to NIL for the
				 	 * pointers and 0 for the lengths.  
					 * This can be passed to Rpc_Reply */
{
    register	FsRemoteCloseParams	*paramsPtr;
    register	Fs_Stream		*streamPtr;
    register	Fs_HandleHeader		*hdrPtr;
    ReturnStatus			status;
    Fs_Stream				dummy;
    register ClientData clientData;

    paramsPtr = (FsRemoteCloseParams *) storagePtr->requestParamPtr;

    hdrPtr = (*fsio_StreamOpTable[paramsPtr->fileID.type].clientVerify)
		(&paramsPtr->fileID, clientID, (int *)NIL);
    if (hdrPtr == (Fs_HandleHeader *) NIL) {
	status = FS_STALE_HANDLE;
	goto exit;
    }
    if (paramsPtr->streamID.type == -1) {
	/*
	 * This is a close of a prefix handle which doesn't have a stream.
	 */
	bzero((Address)&dummy, sizeof(Fs_Stream));
	streamPtr = &dummy;
	streamPtr->ioHandlePtr = hdrPtr;
    } else {
	streamPtr = Fsio_StreamClientVerify(&paramsPtr->streamID, hdrPtr,
		    clientID);
	if (streamPtr == (Fs_Stream *)NIL) {
	    printf("Fsrmt_RpcClose no stream <%d> to handle <%d,%d> client %d\n",
		    paramsPtr->streamID.minor,
		    paramsPtr->fileID.major, paramsPtr->fileID.minor,
		    clientID);
	    Fsutil_HandleRelease(hdrPtr, TRUE);
	    status = (paramsPtr->streamID.minor < 0) ? GEN_INVALID_ARG
						     : FS_STALE_HANDLE ;
	    goto exit;
	}
    }
    /*
     * Call the file type close routine to release the I/O handle
     * and clean up.  This call unlocks and decrements the reference
     * count on the handle.
     */
    FSUTIL_TRACE_HANDLE(FSUTIL_TRACE_CLOSE, hdrPtr);
    if (paramsPtr->closeDataSize != 0) {
	clientData = (ClientData)&paramsPtr->closeData;
    } else {
	clientData = (ClientData)NIL;
    }
    status = (*fsio_StreamOpTable[hdrPtr->fileID.type].close)
	    (streamPtr, clientID, paramsPtr->procID,
	    paramsPtr->flags, paramsPtr->closeDataSize, clientData);
#ifdef lint
    status = Fsio_FileClose(streamPtr, clientID, paramsPtr->procID,
	    paramsPtr->flags, paramsPtr->closeDataSize, clientData);
    status = Fsio_PipeClose(streamPtr, clientID, paramsPtr->procID,
	    paramsPtr->flags, paramsPtr->closeDataSize, clientData);
    status = Fsio_DeviceClose(streamPtr, clientID, paramsPtr->procID,
	    paramsPtr->flags, paramsPtr->closeDataSize, clientData);
    status = FspdevPseudoStreamClose(streamPtr, clientID, paramsPtr->procID,
	    paramsPtr->flags, paramsPtr->closeDataSize, clientData);
#endif /* lint */
    if (streamPtr != &dummy) {
	/*
	 * Take the client of the stream's list and nuke the server's
	 * shadow stream if there are no client's left.
	 */
	if (Fsio_StreamClientClose(&streamPtr->clientList, clientID)) {
	    Fsio_StreamDestroy(streamPtr);
	} else {
	    Fsutil_HandleRelease(streamPtr, TRUE);
	}
    }
exit:
    /*
     * Send back the reply.
     */
    Rpc_Reply(srvToken, status, storagePtr, (int (*)())NIL, (ClientData)NIL);

    return(SUCCESS);	/* So Rpc_Server doesn't return a reply msg */
}

/*
 *----------------------------------------------------------------------
 *
 * FsrmtRemove --
 *
 *	This uses the RPC_FS_UNLINK call to invoke FslclRemove
 *	on the file server.
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
FsrmtRemove(prefixHandle, relativeName, argsPtr, resultsPtr, 
	       newNameInfoPtrPtr)
    Fs_HandleHeader   *prefixHandle;	/* Handle from the prefix table */
    char 	   *relativeName;	/* The name of the file to remove */
    Address 	   argsPtr;		/* Ref to Fs_LookupArgs */
    Address 	   resultsPtr;		/* == NIL */
    Fs_RedirectInfo **newNameInfoPtrPtr; /* We return this if the server leaves 
					   its domain during the lookup. */
{
    ReturnStatus	status;
    Rpc_Storage		storage;
    Fs_RedirectInfo	redirectInfo;
    int			prefixLength;

    storage.requestParamPtr = (Address) argsPtr;
    storage.requestParamSize = sizeof(Fs_LookupArgs);
    storage.requestDataPtr = (Address) relativeName;
    storage.requestDataSize = strlen(relativeName) + 1;
    storage.replyParamPtr = (Address) &prefixLength;
    storage.replyParamSize = sizeof (int);
    storage.replyDataPtr = (Address)&redirectInfo;
    storage.replyDataSize = sizeof(Fs_RedirectInfo);

    status = Rpc_Call(prefixHandle->fileID.serverID, RPC_FS_UNLINK, &storage);
    if (status == FS_LOOKUP_REDIRECT) {
	*newNameInfoPtrPtr = mnew(Fs_RedirectInfo);
	(*newNameInfoPtrPtr)->prefixLength = prefixLength;
	(void)strcpy((*newNameInfoPtrPtr)->fileName, redirectInfo.fileName);
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsrmtRemoveDir --
 *
 *	Remove a directory.  This uses the RPC_FS_RMDIR call to invoke
 *	FslclRemoveDir on the file server.
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
FsrmtRemoveDir(prefixHandle, relativeName, argsPtr, resultsPtr, 
	       newNameInfoPtrPtr)
    Fs_HandleHeader   *prefixHandle;	/* Handle from the prefix table */
    char 	   *relativeName;	/* The name of the file to remove */
    Address 	   argsPtr;		/* Ref to Fs_LookupArgs */
    Address 	   resultsPtr;		/* == NIL */
    Fs_RedirectInfo **newNameInfoPtrPtr; /* We return this if the server leaves 
					   its domain during the lookup. */
{
    ReturnStatus	status;
    Rpc_Storage		storage;
    Fs_RedirectInfo	redirectInfo;
    int			prefixLength;

    storage.requestParamPtr = (Address) argsPtr;
    storage.requestParamSize = sizeof(Fs_LookupArgs);
    storage.requestDataPtr = (Address) relativeName;
    storage.requestDataSize = strlen(relativeName) + 1;
    storage.replyParamPtr = (Address) &prefixLength;
    storage.replyParamSize = sizeof (int);
    storage.replyDataPtr = (Address)&redirectInfo;
    storage.replyDataSize = sizeof(Fs_RedirectInfo);

    status = Rpc_Call(prefixHandle->fileID.serverID, RPC_FS_RMDIR, &storage);
    if (status == FS_LOOKUP_REDIRECT) {
	*newNameInfoPtrPtr = mnew(Fs_RedirectInfo);
	(*newNameInfoPtrPtr)->prefixLength = prefixLength;
	(void)strcpy((*newNameInfoPtrPtr)->fileName, redirectInfo.fileName);
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsrmt_RpcRemove --
 *
 *	The service stub for FS_RPC_UNLINK use to remove a file or directory.
 *
 * Results:
 *	If this procedure returns SUCCESS then a reply has been sent to
 *	the client.  If the arguments are bad then an error is 
 *	returned and the main level sends back an error reply.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
Fsrmt_RpcRemove(srvToken, clientID, command, storagePtr)
    ClientData 		 srvToken;	/* Handle on server process passed to
				 	 * Rpc_Reply */
    int 		 clientID;	/* Sprite ID of client host */
    int 		 command;	/* RPC_FS_UNLINK, RPC_FS_RMDIR */
    register Rpc_Storage *storagePtr;	/* The request fields refer to the 
					 * request buffers and also indicate 
					 * the exact amount of data in the 
					 * request buffers.  The reply fields 
					 * are initialized to NIL for the
				 	 * pointers and 0 for the lengths.  
					 * This can be passed to Rpc_Reply */
{
    ReturnStatus	status;
    Fs_HandleHeader	*prefixHandlePtr;
    Fs_RedirectInfo	*newNameInfoPtr;
    Fs_LookupArgs	*lookupArgsPtr;
    int			domainType;

    lookupArgsPtr = (Fs_LookupArgs *)storagePtr->requestParamPtr;
    prefixHandlePtr =
	(*fsio_StreamOpTable[lookupArgsPtr->prefixID.type].clientVerify)
	    (&lookupArgsPtr->prefixID, clientID, &domainType);
    if (prefixHandlePtr == (Fs_HandleHeader *) NIL) {
	return(FS_STALE_HANDLE);
    } 
    Fsutil_HandleRelease(prefixHandlePtr, TRUE);

    newNameInfoPtr = (Fs_RedirectInfo *) NIL;
    switch (command) {
	case RPC_FS_UNLINK:
	    fs_Stats.srvName.removes++;
	    command = FS_DOMAIN_REMOVE;
	    break;
	case RPC_FS_RMDIR:
	    fs_Stats.srvName.removeDirs++;
	    command = FS_DOMAIN_REMOVE_DIR;
	    break;
	default:
	    return(GEN_INVALID_ARG);
    }
    status = (*fs_DomainLookup[domainType][command])(prefixHandlePtr,
		    (char *) storagePtr->requestDataPtr,
		    (Address) lookupArgsPtr, (Address) NIL, &newNameInfoPtr);
    if (status == FS_LOOKUP_REDIRECT) {
	Rpc_ReplyMem	*replyMemPtr;

	storagePtr->replyDataPtr = (Address) newNameInfoPtr;
	storagePtr->replyDataSize = sizeof(Fs_RedirectInfo);
	storagePtr->replyParamPtr = (Address) malloc(sizeof (int));
	storagePtr->replyParamSize = sizeof (int);
	*((int *) (storagePtr->replyParamPtr)) = newNameInfoPtr->prefixLength;

	replyMemPtr = (Rpc_ReplyMem *) malloc(sizeof(Rpc_ReplyMem));
        replyMemPtr->paramPtr = storagePtr->replyParamPtr;
        replyMemPtr->dataPtr = storagePtr->replyDataPtr;
        Rpc_Reply(srvToken, status, storagePtr, Rpc_FreeMem,
		(ClientData)replyMemPtr);
    } else {
	Rpc_Reply(srvToken, status, storagePtr, (int (*)())NIL,
		(ClientData)NIL);
    }

    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * FsrmtMakeDir --
 *
 *	Make the named directory.  This uses the RPC_FS_MAKE_DIR call
 *	to invoke FslclMakeDir on the file server.
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
FsrmtMakeDir(prefixHandle, relativeName, argsPtr, resultsPtr, 
		newNameInfoPtrPtr)
    Fs_HandleHeader *prefixHandle;   /* Handle from the prefix table */
    char 	   *relativeName;   /* The name of the directory to create */
    Address 	   argsPtr;	    /* Ref. to Fs_OpenArgs */
    Address 	   resultsPtr;	    /* == NIL */
    Fs_RedirectInfo **newNameInfoPtrPtr;/* We return this if the server leaves 
					* its domain during the lookup. */
{
    ReturnStatus	status;
    Rpc_Storage		storage;
    Fs_RedirectInfo	redirectInfo;
    int			prefixLength;

    storage.requestParamPtr = (Address) argsPtr;
    storage.requestParamSize = sizeof(Fs_OpenArgs);
    storage.requestDataPtr = (Address) relativeName;
    storage.requestDataSize = strlen(relativeName) + 1;
    storage.replyParamPtr = (Address) &prefixLength;
    storage.replyParamSize = sizeof (int);
    storage.replyDataPtr = (Address)&redirectInfo;
    storage.replyDataSize = sizeof(Fs_RedirectInfo);

    status = Rpc_Call(prefixHandle->fileID.serverID, RPC_FS_MKDIR, &storage);
    if (status == FS_LOOKUP_REDIRECT) {
	*newNameInfoPtrPtr = mnew(Fs_RedirectInfo);
	(*newNameInfoPtrPtr)->prefixLength = prefixLength;
	(void)strcpy((*newNameInfoPtrPtr)->fileName, redirectInfo.fileName);
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsrmt_RpcMakeDir --
 *
 *	Handle a make directory request from a client.
 *
 * Results:
 *	If this procedure returns SUCCESS then a reply has been sent to
 *	the client.  If the arguments are bad then an error is 
 *	returned and the main level sends back an error reply.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
ReturnStatus
Fsrmt_RpcMakeDir(srvToken, clientID, command, storagePtr)
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
    ReturnStatus	status;
    Fs_HandleHeader	*prefixHandlePtr;
    Fs_RedirectInfo	*newNameInfoPtr;
    Fs_OpenArgs		*openArgsPtr;
    int			domainType;

    openArgsPtr = (Fs_OpenArgs *) storagePtr->requestParamPtr;
    if (openArgsPtr->prefixID.serverID != rpc_SpriteID) {
	return(GEN_INVALID_ARG);
    }

    prefixHandlePtr =
	(*fsio_StreamOpTable[openArgsPtr->prefixID.type].clientVerify)
	    (&openArgsPtr->prefixID, clientID, &domainType);
    if (prefixHandlePtr == (Fs_HandleHeader *) NIL) {
	return(FS_STALE_HANDLE);
    }
    Fsutil_HandleRelease(prefixHandlePtr, TRUE);

    fs_Stats.srvName.makeDirs++;
    newNameInfoPtr = (Fs_RedirectInfo *) NIL;
    status = (*fs_DomainLookup[domainType][FS_DOMAIN_MAKE_DIR])(prefixHandlePtr,
	    (char *)storagePtr->requestDataPtr,
	    (Address) openArgsPtr, (Address) NIL, &newNameInfoPtr);
    if (status == FS_LOOKUP_REDIRECT) {
	Rpc_ReplyMem	*replyMemPtr;

	storagePtr->replyDataPtr = (Address)newNameInfoPtr;
	storagePtr->replyDataSize = sizeof(Fs_RedirectInfo);
	storagePtr->replyParamPtr = (Address) malloc(sizeof (int));
	storagePtr->replyParamSize = sizeof (int);
	*((int *)(storagePtr->replyParamPtr)) = newNameInfoPtr->prefixLength;

        replyMemPtr = (Rpc_ReplyMem *) malloc(sizeof(Rpc_ReplyMem));
        replyMemPtr->paramPtr = storagePtr->replyParamPtr;
        replyMemPtr->dataPtr = storagePtr->replyDataPtr;
        Rpc_Reply(srvToken, status, storagePtr, Rpc_FreeMem,
		(ClientData)replyMemPtr);
    } else {
        Rpc_Reply(srvToken, status, storagePtr, (int (*)())NIL,
		(ClientData)NIL);
    }

    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * FsrmtMakeDevice --
 *
 *	Create a device file.  This uses the RPC_FS_MAKE_DEV call to create
 *	the special file on the file server.
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
FsrmtMakeDevice(prefixHandle, relativeName, argsPtr, resultsPtr,
			       newNameInfoPtrPtr)
    Fs_HandleHeader *prefixHandle;   /* Handle from the prefix table */
    char           *relativeName;   /* The name of the file. */
    Address        argsPtr;	    /* Ref. to FsMakeDevArgs */
    Address        resultsPtr;	    /* == NIL */
    Fs_RedirectInfo **newNameInfoPtrPtr;/* We return this if the server leaves 
					* its domain during the lookup. */
{
    ReturnStatus	status;
    Rpc_Storage		storage;
    Fs_RedirectInfo	redirectInfo;
    int			prefixLength;

    storage.requestParamPtr = (Address) argsPtr;
    storage.requestParamSize = sizeof(Fs_MakeDeviceArgs);
    storage.requestDataPtr = (Address) relativeName;
    storage.requestDataSize = strlen(relativeName) + 1;
    storage.replyParamPtr = (Address) &prefixLength;
    storage.replyParamSize = sizeof (int);
    storage.replyDataPtr = (Address)&redirectInfo;
    storage.replyDataSize = sizeof(Fs_RedirectInfo);

    status = Rpc_Call(prefixHandle->fileID.serverID, RPC_FS_MKDEV, &storage);
    if (status == FS_LOOKUP_REDIRECT) {
	*newNameInfoPtrPtr = mnew(Fs_RedirectInfo);
	(*newNameInfoPtrPtr)->prefixLength = prefixLength;
	(void)strcpy((*newNameInfoPtrPtr)->fileName, redirectInfo.fileName);
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsrmt_RpcMakeDev --
 *
 *	Service stub for RPC_FS_MKDEV.  This calls FslclMakeDevice.
 *
 * Results:
 *	If this procedure returns SUCCESS then a reply has been sent to
 *	the client.  If the arguments are bad then an error is 
 *	returned and the main level sends back an error reply.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
ReturnStatus
Fsrmt_RpcMakeDev(srvToken, clientID, command, storagePtr)
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
    ReturnStatus	status;
    Fs_MakeDeviceArgs	*makeDevArgsPtr;
    Fs_HandleHeader	*prefixHandlePtr;
    Fs_RedirectInfo	*newNameInfoPtr;
    int			domainType;

    makeDevArgsPtr = (Fs_MakeDeviceArgs *) storagePtr->requestParamPtr;
    prefixHandlePtr = 
	(*fsio_StreamOpTable[makeDevArgsPtr->open.prefixID.type].clientVerify)
	    (&makeDevArgsPtr->open.prefixID, clientID, &domainType);
    if (prefixHandlePtr == (Fs_HandleHeader *) NIL) {
	return(FS_STALE_HANDLE);
    }
    Fsutil_HandleRelease(prefixHandlePtr, TRUE);

    fs_Stats.srvName.makeDevices++;
    newNameInfoPtr = (Fs_RedirectInfo *) NIL;
    status = (*fs_DomainLookup[domainType][FS_DOMAIN_MAKE_DEVICE])(prefixHandlePtr,
	    (char *)storagePtr->requestDataPtr, (Address) makeDevArgsPtr,
	    (Address) NIL, &newNameInfoPtr);
    if (status == FS_LOOKUP_REDIRECT) {
	Rpc_ReplyMem	*replyMemPtr;

	storagePtr->replyDataPtr = (Address)newNameInfoPtr;
	storagePtr->replyDataSize = sizeof(Fs_RedirectInfo);
	storagePtr->replyParamPtr = (Address) malloc(sizeof (int));
	storagePtr->replyParamSize = sizeof (int);
	*((int *) (storagePtr->replyParamPtr)) = newNameInfoPtr->prefixLength;

        replyMemPtr = (Rpc_ReplyMem *) malloc(sizeof(Rpc_ReplyMem));
        replyMemPtr->paramPtr = storagePtr->replyParamPtr;
        replyMemPtr->dataPtr = storagePtr->replyDataPtr;
        Rpc_Reply(srvToken, status, storagePtr, Rpc_FreeMem,
		    (ClientData)replyMemPtr);
    } else {
        Rpc_Reply(srvToken, status, storagePtr, (int (*)())NIL,
		    (ClientData)NIL);
    }
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * TwoNameOperation --
 *
 *	Common stub for renaming a file or making a hard link.
 *
 * Results:
 *	A return status.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
TwoNameOperation(command, prefixHandle1, relativeName1, prefixHandle2, 
	 relativeName2, lookupArgsPtr, newNameInfoPtrPtr, name1ErrorPtr)
    int			command;		/* Which Rpc: Mv or Ln */
    Fs_HandleHeader 	*prefixHandle1;		/* Handle from prefix table */
    char 		*relativeName1;		/* The new name of the file. */
    Fs_HandleHeader 	*prefixHandle2;		/* Handle from prefix table */
    char 		*relativeName2;		/* The new name of the file. */
    Fs_LookupArgs	*lookupArgsPtr;		/* Contains IDs */
    Fs_RedirectInfo	**newNameInfoPtrPtr;	/* We return this if the server
						 * leaves its domain during the
						 * lookup. */
    Boolean 		*name1ErrorPtr;		/* If we return REDIRECT or
						 * STALE_HANDLE this indicates
						 * if that applies to the first
						 * pathname or the second */
{
    Fs_2PathParams	params;
    Fs_2PathData		*requestDataPtr;	/* too big for stack */
    Fs_2PathReply	replyParams;
    Rpc_Storage		storage;
    ReturnStatus	status;
    Fs_RedirectInfo	redirectInfo;

    requestDataPtr = mnew(Fs_2PathData);

    params.lookup = *lookupArgsPtr;
    params.lookup.prefixID = prefixHandle1->fileID;
    params.prefixID2 = prefixHandle2->fileID;

    (void)strcpy(requestDataPtr->path1, relativeName1);
    (void)strcpy(requestDataPtr->path2, relativeName2);

    storage.requestParamPtr = (Address) &params;
    storage.requestParamSize = sizeof (Fs_2PathParams);
    storage.requestDataPtr = (Address) requestDataPtr;
    storage.requestDataSize = sizeof (Fs_2PathData);

    storage.replyParamPtr = (Address) &replyParams;
    storage.replyParamSize = sizeof (Fs_2PathReply);
    storage.replyDataPtr = (Address)&redirectInfo;
    storage.replyDataSize = sizeof(Fs_RedirectInfo);

    status = Rpc_Call(prefixHandle1->fileID.serverID, command, &storage);
    *name1ErrorPtr = replyParams.name1ErrorP;
    if (status == FS_LOOKUP_REDIRECT) {
	*newNameInfoPtrPtr = mnew(Fs_RedirectInfo);
	(*newNameInfoPtrPtr)->prefixLength = replyParams.prefixLength;
	(void)strcpy((*newNameInfoPtrPtr)->fileName, redirectInfo.fileName);
    }
    free((Address)requestDataPtr);

    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsrmt_Rpc2Path --
 *
 *	Common service stub for FsrmtRename and FsrmtHardLink.
 *
 * Results:
 *	If this procedure returns SUCCESS then a reply has been sent to
 *	the client.  If the arguments are bad then FS_STALE_HANDLE is 
 *	returned and the main level sends back an error reply.
 *
 * Side effects:
 *	Calls the local rename or hard link routine to attempt the lock 
 *	operation.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Fsrmt_Rpc2Path(srvToken, clientID, command, storagePtr)
    ClientData srvToken;	/* Handle on server process passed to
				 * Rpc_Reply */
    int clientID;		/* Sprite ID of client host */
    int command;		/* Command identifier */
    Rpc_Storage *storagePtr;    /* The request fields refer to the request
				 * buffers and also indicate the exact amount
				 * of data in the request buffers.  The reply
				 * fields are initialized to NIL for the
				 * pointers and 0 for the lengths.  This can
				 * be passed to Rpc_Reply */
{
    register	Fs_2PathParams		*paramsPtr;
    register	Fs_LookupArgs		*lookupArgsPtr;
    register	Fs_HandleHeader		*prefixHandle1Ptr;
    register	Fs_HandleHeader		*prefixHandle2Ptr;
    register	Rpc_ReplyMem		*replyMemPtr;
    Fs_RedirectInfo			*newNameInfoPtr;
    Boolean				name1Error = FALSE;
    Fs_2PathReply			*replyParamsPtr;
    Fs_2PathData				*pathDataPtr;
    ReturnStatus			status = SUCCESS;
    int					domainType;

    paramsPtr = (Fs_2PathParams *)storagePtr->requestParamPtr;
    pathDataPtr = (Fs_2PathData *)storagePtr->requestDataPtr;
    lookupArgsPtr = &paramsPtr->lookup;
    prefixHandle1Ptr =
	(*fsio_StreamOpTable[lookupArgsPtr->prefixID.type].clientVerify)
	    (&lookupArgsPtr->prefixID, clientID, &domainType);

    if (prefixHandle1Ptr == (Fs_HandleHeader *)NIL) {
	name1Error = TRUE;
	status = FS_STALE_HANDLE;
	goto exit;
    } else {
	Fsutil_HandleUnlock(prefixHandle1Ptr);
    }
    if (paramsPtr->prefixID2.serverID != rpc_SpriteID) {
	/*
	 * Second pathname doesn't even start with us.  However, we are
	 * called in case the first pathname redirects away from us.
	 */
	prefixHandle2Ptr = (Fs_HandleHeader *)NIL;
    } else {
	prefixHandle2Ptr =
	    (*fsio_StreamOpTable[paramsPtr->prefixID2.type].clientVerify)
		(&paramsPtr->prefixID2, clientID, (int *)NIL);
	if (prefixHandle2Ptr == (Fs_HandleHeader *)NIL) {
	    Fsutil_HandleRelease(prefixHandle1Ptr, FALSE);
	    name1Error = FALSE;
	    status = FS_STALE_HANDLE;
	    goto exit;
	} else {
	    Fsutil_HandleUnlock(prefixHandle2Ptr);
	}
    }

    newNameInfoPtr = (Fs_RedirectInfo *) NIL;
    if (command == RPC_FS_RENAME) {
	fs_Stats.srvName.renames++;
	command = FS_DOMAIN_RENAME;
    } else if (command == RPC_FS_LINK) {
	fs_Stats.srvName.hardLinks++;
	command = FS_DOMAIN_HARD_LINK;
    } else {
	printf( "Fsrmt_Rpc2Path: Bad command %d\n", command);
	status = FS_INVALID_ARG;
    }
    if (status == SUCCESS) {
	status = (*fs_DomainLookup[domainType][command])(prefixHandle1Ptr,
		    pathDataPtr->path1, prefixHandle2Ptr, pathDataPtr->path2,
		    lookupArgsPtr, &newNameInfoPtr, &name1Error);
    }
    Fsutil_HandleRelease(prefixHandle1Ptr, FALSE);
    if (prefixHandle2Ptr != (Fs_HandleHeader *)NIL) {
	Fsutil_HandleRelease(prefixHandle2Ptr, FALSE);
    }
exit:
    replyParamsPtr = (Fs_2PathReply *) malloc(sizeof (Fs_2PathReply));
    replyParamsPtr->name1ErrorP = name1Error;
    storagePtr->replyParamPtr = (Address) replyParamsPtr;
    storagePtr->replyParamSize = sizeof (Fs_2PathReply);
    if (status == FS_LOOKUP_REDIRECT) {
	replyParamsPtr->prefixLength = newNameInfoPtr->prefixLength;
	storagePtr->replyDataPtr = (Address) newNameInfoPtr;
	storagePtr->replyDataSize = sizeof(int) +
				strlen(newNameInfoPtr->fileName) + 1;
    } else {
	replyParamsPtr->prefixLength = 0;
	/*
	 * Reply data ptr already set to NIL
	 */
     }
    replyMemPtr = (Rpc_ReplyMem *) malloc(sizeof(Rpc_ReplyMem));
    replyMemPtr->paramPtr = (Address) replyParamsPtr;
    replyMemPtr->dataPtr = (Address) storagePtr->replyDataPtr;
    Rpc_Reply(srvToken, status, storagePtr, 
	      (int (*)()) Rpc_FreeMem, (ClientData) replyMemPtr);

    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * FsrmtRename --
 *
 *	Stub for renaming a file.
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
FsrmtRename(prefixHandle1, relativeName1, prefixHandle2, relativeName2,
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
    return(TwoNameOperation(RPC_FS_RENAME, prefixHandle1, relativeName1, 
		     	    prefixHandle2, relativeName2, lookupArgsPtr, 
			    newNameInfoPtrPtr, name1ErrorPtr));
}


/*
 *----------------------------------------------------------------------
 *
 * FsrmtHardLink --
 *
 *	Stub for making a hard link between two files.
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
FsrmtHardLink(prefixHandle1, relativeName1, prefixHandle2, relativeName2,
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
    return(TwoNameOperation(RPC_FS_LINK, prefixHandle1, relativeName1, 
		     	    prefixHandle2, relativeName2, lookupArgsPtr, 
			    newNameInfoPtrPtr, name1ErrorPtr));
}

