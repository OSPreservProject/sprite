/* 
 * fsSpriteDomain.c --
 *
 *	This has the stubs for remote naming operations in a Sprite domain.
 *	These routines are presented in pairs, the client stub followed
 *	by the server stub.  The general style is for the server stub
 *	to call the LocalDomain equivalent of the SpriteDomain client stub.
 *	i.e. FsSpriteOpen invokes via RPC Fs_RpcOpen which calls
 *	FsLocalOpen.  Occasionally a client or server stub will do some
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
#include "fsInt.h"
#include "fsNameOps.h"
#include "fsPrefix.h"
#include "fsSpriteDomain.h"
#include "fsLocalDomain.h"
#include "fsOpTable.h"
#include "fsStream.h"
#include "fsTrace.h"
#include "fsDebug.h"
#include "fsStat.h"
#include "recov.h"
#include "proc.h"
#include "rpc.h"
#include "vm.h"
#include "dbg.h"

/*
 * Used to contain fileID and stream data results from open calls.
 */
typedef	struct	FsOpenReplyParam {
    FsUnionData	openData;
    FsFileID	fileID;
} FsOpenReplyParam;


/*
 *----------------------------------------------------------------------
 *
 * FsSpritePrefix --
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
FsSpritePrefix(prefixHandle, fileName, argsPtr, resultsPtr, newNameInfoPtrPtr)
    FsHandleHeader   *prefixHandle;	/* == NIL */
    char 	   *fileName;		/* File name to determine prefix for. */
    Address        argsPtr;		/* Ref to FsUserIDs, IGNORED */
    Address        resultsPtr;		/* Ref to (FsHandleHeader *) */
    FsRedirectInfo **newNameInfoPtrPtr; /* == NIL */
{
    ReturnStatus 	status;
    Rpc_Storage 	storage;
    FsFileID		*fileIDPtr;	/* Returned from server */
    ClientData		streamData;	/* Returned from server */
    FsHandleHeader	**hdrPtrPtr = (FsHandleHeader **)resultsPtr;
    int			flags = 0;
    FsOpenReplyParam	openReplyParam;

    *hdrPtrPtr = (FsHandleHeader *)NIL;

    storage.requestParamPtr = (Address) NIL;
    storage.requestParamSize = 0;
    storage.requestDataPtr = (Address)fileName;
    storage.requestDataSize = String_Length(fileName)+1;

    storage.replyParamPtr = (Address)&openReplyParam;
    storage.replyParamSize = sizeof(FsOpenReplyParam);
    storage.replyDataPtr = (Address)NIL;
    storage.replyDataSize = 0;
    fileIDPtr = &(openReplyParam.fileID);

    status = Rpc_Call(RPC_BROADCAST_SERVER_ID, RPC_FS_PREFIX, &storage);
    /*
     * It is necessary to allocate and copy over the stream data, since
     * the cltOpen proc frees this space.
     */
    streamData = (ClientData)Mem_Alloc(sizeof(FsUnionData));
    *((FsUnionData *) streamData) = openReplyParam.openData;

    if (status == SUCCESS) {
	/*
	 * Use the client-open routine to set up an I/O handle for the prefix.
	 */
	status = (*fsStreamOpTable[fileIDPtr->type].cltOpen)(fileIDPtr, &flags,
		    rpc_SpriteID, (ClientData)streamData, fileName, hdrPtrPtr);
	if (status == SUCCESS) {
	    /*
	     * Register the server with the recovery module so we find out
	     * when it goes away and when it reboots.
	     */
	    Recov_RebootRegister((*hdrPtrPtr)->fileID.serverID, FsReopen,
				 (ClientData)NIL);
	}
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_RpcPrefix --
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
 *	The srvOpen routine is called on the prefix handle.  This ups
 *	reference counts.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
Fs_RpcPrefix(srvToken, clientID, command, storagePtr)
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
    FsPrefix				*prefixPtr;
    FsHandleHeader			*hdrPtr;
    FsFileID				rootID;
    int					domainType;
    ReturnStatus			status;
    FsOpenReplyParam			*openReplyParamPtr;

    status = FsPrefixLookup((char *) storagePtr->requestDataPtr,
			FS_EXPORTED_PREFIX | FS_EXACT_PREFIX, clientID,
			&hdrPtr, &rootID, &lookupName, &domainType, &prefixPtr);
    if (status == SUCCESS) {
	register Rpc_ReplyMem		*replyMemPtr;
	register FsLocalFileIOHandle	*handlePtr;
	ClientData			streamData;
	int				dataSize;

	if (hdrPtr->fileID.type != FS_LCL_FILE_STREAM) {
	    Sys_Panic(SYS_FATAL,
		"Fs_RpcPrefix, found non-local exported prefix\n");
	    return(RPC_NO_REPLY);
	}
	/*
	 * Use the server-open routine to set up & allocate streamData.
	 */
	handlePtr = (FsLocalFileIOHandle *)hdrPtr;
	FsHandleLock(handlePtr);
	openReplyParamPtr = Mem_New(FsOpenReplyParam);
	status = (*fsOpenOpTable[handlePtr->descPtr->fileType].srvOpen)
		    (handlePtr, clientID, 0, &(openReplyParamPtr->fileID),
		    (FsFileID *)NIL, &dataSize, &streamData);

	if (status == SUCCESS) {
	    Byte_Copy(dataSize, (Address)streamData,
			(Address)openReplyParamPtr->openData);
	    Mem_Free((Address)streamData);
	    storagePtr->replyParamPtr = (Address) (openReplyParamPtr);
	    storagePtr->replyParamSize = sizeof(FsOpenReplyParam);
	    storagePtr->replyDataPtr = (Address)NIL;
	    storagePtr->replyDataSize = 0;

	    replyMemPtr = (Rpc_ReplyMem *) Mem_Alloc(sizeof(Rpc_ReplyMem));
	    replyMemPtr->paramPtr = storagePtr->replyParamPtr;
	    replyMemPtr->dataPtr = storagePtr->replyDataPtr;
	    Rpc_Reply(srvToken, SUCCESS, storagePtr, Rpc_FreeMem,
		    (ClientData)replyMemPtr);
	    return(SUCCESS);
	} else {
	    Mem_Free((Address)openReplyParamPtr);
	    Sys_Panic(SYS_WARNING, "Fs_RpcPrefix, srvOpen \"%s\" failed %x\n",
		    storagePtr->requestDataPtr, status);
	}
    }
    return(RPC_NO_REPLY);
}

/*
 *----------------------------------------------------------------------
 *
 * FsSpriteOpen --
 *
 *	Open a remote file.  This sets up and conducts an RPC_FS_OPEN
 *	remote procedure call to open the remote file.  This is called
 *	from FsLookupOperation based on the prefix table.  FsSpriteOpen
 *	makes an RPC to FsLocalOpen on the remote machine, and returns
 *	the streamData for use by the client-open routine.
 *
 * RPC: The input parameters are the FsOpenArgs defined in fsNameOps.h.
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
FsSpriteOpen(prefixHandle, relativeName, argsPtr, resultsPtr, 
	     newNameInfoPtrPtr)
    FsHandleHeader  *prefixHandle;	/* Token from the prefix table */
    char 	  *relativeName;	/* The name of the file to open. */
    Address 	  argsPtr;		/* Ref. to FsOpenArgs */
    Address 	  resultsPtr;		/* Ref. to FsOpenResults */
    FsRedirectInfo **newNameInfoPtrPtr;	/* We return this if the server leaves 
					 * its domain during the lookup. */
{
    ReturnStatus	status;
    FsOpenResults	*openResultsPtr = (FsOpenResults *)resultsPtr;
    Rpc_Storage		storage;	/* Specifies RPC parameters/results */
    char		replyName[FS_MAX_PATH_NAME_LENGTH];	 /* This
					 * may get filled with a
					 * redirected pathname */
    FsOpenResultsParam	openResultsParam;
    /*
     * Synchronize with the re-open phase of recovery.
     * We don't want opens to race with the recovery actions.
     */
    status = FsPrefixOpenCheck(prefixHandle);
    if (status != SUCCESS) {
	return(status);
    }

    /*
     * Set up for the RPC.
     */
    storage.requestParamPtr = (Address) argsPtr;
    storage.requestParamSize = sizeof(FsOpenArgs);
    storage.requestDataPtr = (Address) relativeName;
    storage.requestDataSize = String_Length(relativeName) + 1;
    storage.replyParamPtr = (Address) &openResultsParam;
    storage.replyParamSize = sizeof(FsOpenResultsParam);
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
		    (ClientData)Mem_Alloc(openResultsPtr->dataSize);
	    Byte_Copy(openResultsPtr->dataSize,
		    (Address) &(openResultsParam.openData),
		    (Address) openResultsPtr->streamData);
	}
    } else if (status == FS_LOOKUP_REDIRECT) {
	/*
	 * Allocate space for the re-directed pathname and
	 * copy over the structure that we have on our stack.  A large
	 * buffer is allocated because it is used as a work area in
	 * FsLookupRedirect to create a new absolute pathname.
	 */
	*newNameInfoPtrPtr = Mem_New(FsRedirectInfo);
	(*newNameInfoPtrPtr)->prefixLength = openResultsParam.prefixLength;
	(void)String_Copy(replyName, (*newNameInfoPtrPtr)->fileName);
    }
    FsPrefixOpenDone(prefixHandle);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_RpcOpen --
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
 *	None here, see FsLocalOpen.
 *	
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
Fs_RpcOpen(srvToken, clientID, command, storagePtr)
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
    register FsOpenArgs		*openArgsPtr;		/* RPC parameters */
    register FsOpenResults	*openResultsPtr;	/* RPC results */
    FsHandleHeader		*prefixHandlePtr;	/* Handle for domain */
    FsRedirectInfo		*newNameInfoPtr;	/* prefix info for
							 * redirected lookups */
    FsOpenResultsParam		*openResultsParamPtr;	/* open results, etc. */


    if (Recov_GetClientState(clientID) & RECOV_IN_PROGRESS) {
	Net_HostPrint(clientID, "Dropping regular open during recovery\n");
	return(RPC_SERVICE_DISABLED);
    }
    openArgsPtr = (FsOpenArgs *) storagePtr->requestParamPtr;

    if (openArgsPtr->prefixID.serverID != rpc_SpriteID) {
	/*
	 * Filesystem mis-match.
	 */
	return(GEN_INVALID_ARG);
    }

    /*
     * Get a handle on the prefix.  We need to have it unlocked in case
     * we do I/O on the directory.
     */
    prefixHandlePtr =
	(*fsStreamOpTable[openArgsPtr->prefixID.type].clientVerify)
	    (&openArgsPtr->prefixID, clientID);
    if (prefixHandlePtr == (FsHandleHeader *)NIL) {
	return(FS_STALE_HANDLE);
    }
    FsHandleUnlock(prefixHandlePtr);

    newNameInfoPtr = (FsRedirectInfo *) NIL;
    openResultsParamPtr = Mem_New(FsOpenResultsParam);
    openResultsPtr = &(openResultsParamPtr->openResults);

    status = FsLocalOpen(prefixHandlePtr, (char *)storagePtr->requestDataPtr,
		(Address)openArgsPtr, (Address)openResultsPtr, &newNameInfoPtr);
    FsHandleRelease(prefixHandlePtr, FALSE);
    if (status == SUCCESS) {
	/*
	 * The open worked.  We return the whole FsOpenResults structure
	 * in the RPC parameter area, but it contains a pointer to
	 * stream data and a dataSize. That stream data is returned also
	 * as a separate field in the RPC parameter area, so it must be copied.
	 */
	storagePtr->replyParamPtr = (Address)openResultsParamPtr;
	storagePtr->replyParamSize = sizeof(FsOpenResultsParam);
	/* copy openData */
	if (openResultsPtr->dataSize != 0 &&
		((Address)openResultsPtr->streamData) != (Address)NIL) {
	    Byte_Copy(openResultsPtr->dataSize,
		    (Address)openResultsPtr->streamData,
		    (Address)openResultsParamPtr->openData);
	    Mem_Free((Address)openResultsPtr->streamData);
	    storagePtr->replyDataPtr = (Address)NIL;
	    storagePtr->replyDataSize = 0;
	}
    } else if (status == FS_LOOKUP_REDIRECT) {
	/*
	 * The file is not found on this server.
	 */
	storagePtr->replyParamPtr = (Address)openResultsParamPtr;
	storagePtr->replyParamSize = sizeof(FsOpenResultsParam);
	openResultsParamPtr->prefixLength = newNameInfoPtr->prefixLength;
	storagePtr->replyDataSize = String_Length(newNameInfoPtr->fileName) + 1;
	storagePtr->replyDataPtr = Mem_Alloc(storagePtr->replyDataSize);
	(void)String_Copy(newNameInfoPtr->fileName, storagePtr->replyDataPtr);
	Mem_Free((Address)newNameInfoPtr);
    }
    if (status == SUCCESS || status == FS_LOOKUP_REDIRECT) {
	Rpc_ReplyMem	*replyMemPtr;

        replyMemPtr = (Rpc_ReplyMem *) Mem_Alloc(sizeof(Rpc_ReplyMem));
        replyMemPtr->paramPtr = storagePtr->replyParamPtr;
        replyMemPtr->dataPtr = storagePtr->replyDataPtr;
        Rpc_Reply(srvToken, status, storagePtr, Rpc_FreeMem,
		(ClientData)replyMemPtr);
        return(SUCCESS);
    } else {
	Mem_Free((Address)openResultsParamPtr);
        return(status);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * FsSpriteReopen --
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
FsSpriteReopen(hdrPtr, inSize, inData, outSizePtr, outData)
    FsHandleHeader  *hdrPtr;		/* Handle to reopen */
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
 * Fs_RpcReopen --
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
Fs_RpcReopen(srvToken, clientID, command, storagePtr)
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
    register FsFileID *fileIDPtr;
    register ReturnStatus status;

    if ((Recov_GetClientState(clientID) & RECOV_IN_PROGRESS) == 0) {
	Recov_SetClientState(clientID, RECOV_IN_PROGRESS);
	Net_HostPrint(clientID, "starting recovery\n");
    }

    fileIDPtr = (FsFileID *)storagePtr->requestParamPtr;
    if (fileIDPtr->serverID != rpc_SpriteID) {
	/*
	 * Filesystem version mis-match.
	 */
	return(GEN_INVALID_ARG);
    }
    status = (*fsStreamOpTable[fileIDPtr->type].reopen)((FsHandleHeader *)NIL,
		clientID, storagePtr->requestParamPtr,
		&storagePtr->replyParamSize,
		&storagePtr->replyParamPtr);

    if (status == SUCCESS) {
	Rpc_ReplyMem	*replyMemPtr;

        replyMemPtr = (Rpc_ReplyMem *) Mem_Alloc(sizeof(Rpc_ReplyMem));
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
    FsCachedAttributes	attrs;
} FsCloseData;


/*
 * Request params for the close RPC.  The data for the close is put in the
 * closeData field so that it too can be byte-swapped. The field is for stream
 * specific data that gets pushed back to the server when the client closes.
 * Currently, it seems only to be FsCachedAttributes.
 * 
 */
typedef struct FsSpriteCloseParams {
    FsFileID	fileID;		/* File to close */
    FsFileID	streamID;	/* Stream to close */
    int		flags;		/* Flags from the stream */
    FsCloseData	closeData;	/* Seems to be only FsCachedAttributes... */
    int		closeDataSize;	/* actual size of info in closeData field. */
} FsSpriteCloseParams;

/*
 *----------------------------------------------------------------------
 *
 * FsSpriteClose --
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
FsSpriteClose(streamPtr, clientID, flags, dataSize, closeData)
    Fs_Stream		*streamPtr;	/* Stream to close.  This is needed
					 * (instead of I/O handle) so the
					 * server can close its shadow stream */
    int			clientID;	/* IGNORED, implicitly passed by RPC */
    int			flags;		/* Flags from the stream being closed */
    int			dataSize;	/* Size of *closeData, or Zero */
    ClientData		closeData;	/* Copy of cached I/O attributes.
   					 * Sometimes NIL!  */
{
    FsRemoteIOHandle	*rmtHandlePtr;	/* Handle to close */
    Rpc_Storage 	storage;
    ReturnStatus 	status;
    FsSpriteCloseParams	params;

    rmtHandlePtr = (FsRemoteIOHandle *)streamPtr->ioHandlePtr;
    params.fileID = rmtHandlePtr->hdr.fileID;
    params.streamID = streamPtr->hdr.fileID;
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
	FsWantRecovery((FsHandleHeader *)rmtHandlePtr);
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_RpcClose --
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
Fs_RpcClose(srvToken, clientID, command, storagePtr)
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
    register	FsSpriteCloseParams	*paramsPtr;
    register	Fs_Stream		*streamPtr;
    register	FsHandleHeader		*hdrPtr;
    ReturnStatus			status;
    Fs_Stream				dummy;

    paramsPtr = (FsSpriteCloseParams *) storagePtr->requestParamPtr;

    if (paramsPtr->streamID.type == -1) {
	/*
	 * This is a close of a prefix handle which doesn't have a stream.
	 */
	streamPtr = &dummy;
    } else {
	streamPtr = FsStreamClientVerify(&paramsPtr->streamID, clientID);
    }
    if (streamPtr == (Fs_Stream *)NIL) {
	status = FS_STALE_HANDLE;
    } else {

	hdrPtr = (*fsStreamOpTable[paramsPtr->fileID.type].clientVerify)
		    (&paramsPtr->fileID, clientID);
	dummy.ioHandlePtr = hdrPtr;
	if (hdrPtr == (FsHandleHeader *) NIL) {
	    status = FS_STALE_HANDLE;
	} else if (streamPtr->ioHandlePtr != hdrPtr) {
	    Sys_Panic(SYS_FATAL, "Fs_RpcClose: Stream/handle mis-match\n");
	} else {
	    /*
	     * Call the file type close routine to release the I/O handle
	     * and clean up.  This call unlocks and decrements the reference
	     * count on the handle.
	     */
	    FS_TRACE_HANDLE(FS_TRACE_CLOSE, hdrPtr);
	    if (paramsPtr->closeDataSize != 0) {
		status = (*fsStreamOpTable[hdrPtr->fileID.type].close)
			(streamPtr, clientID, paramsPtr->flags,
			paramsPtr->closeDataSize,
			(ClientData)&(paramsPtr->closeData));
	    } else {
		status = (*fsStreamOpTable[hdrPtr->fileID.type].close)
			(streamPtr, clientID, paramsPtr->flags,
			0, (ClientData)NIL);
	    }
	}
	if (streamPtr != &dummy) {
	    /*
	     * Take the client of the stream's list and nuke the server's
	     * shadow stream if there are no client's left.
	     */
	    if (FsStreamClientClose(&streamPtr->clientList, clientID)) {
		FsStreamDispose(streamPtr);
	    } else {
		FsHandleRelease(streamPtr, TRUE);
	    }
	}
    }
    /*
     * Send back the reply.
     */
    Rpc_Reply(srvToken, status, storagePtr, (int (*)())NIL, (ClientData)NIL);

    return(SUCCESS);	/* So Rpc_Server doesn't return a reply msg */
}

/*
 *----------------------------------------------------------------------
 *
 * FsSpriteRemove --
 *
 *	Common stub for removing a file and removing a directory.  This
 *	uses the RPC_FS_UNLINK call to invoke FsLocalRemove on the file server.
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
FsSpriteRemove(prefixHandle, relativeName, argsPtr, resultsPtr, 
	       newNameInfoPtrPtr)
    FsHandleHeader   *prefixHandle;	/* Handle from the prefix table */
    char 	   *relativeName;	/* The name of the file to remove */
    Address 	   argsPtr;		/* Ref to FsLookupArgs */
    Address 	   resultsPtr;		/* == NIL */
    FsRedirectInfo **newNameInfoPtrPtr; /* We return this if the server leaves 
					   its domain during the lookup. */
{
    ReturnStatus	status;
    Rpc_Storage		storage;
    FsRedirectInfo	redirectInfo;
    int			prefixLength[3];	/* padded so that network
						 * interface won't pad it
						 * without our being aware
						 * of it. */

    storage.requestParamPtr = (Address) argsPtr;
    storage.requestParamSize = sizeof(FsLookupArgs);
    storage.requestDataPtr = (Address) relativeName;
    storage.requestDataSize = String_Length(relativeName) + 1;
    storage.replyParamPtr = (Address) prefixLength;
    storage.replyParamSize = 3 * sizeof (int);
    storage.replyDataPtr = (Address)&redirectInfo;
    storage.replyDataSize = sizeof(FsRedirectInfo);

    status = Rpc_Call(prefixHandle->fileID.serverID, RPC_FS_UNLINK, &storage);
    if (status == FS_LOOKUP_REDIRECT) {
	*newNameInfoPtrPtr = Mem_New(FsRedirectInfo);
	(*newNameInfoPtrPtr)->prefixLength = prefixLength[0];
	(void)String_Copy(redirectInfo.fileName, (*newNameInfoPtrPtr)->fileName);
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsSpriteRemoveDir --
 *
 *	Remove a directory.  This uses the RPC_FS_RMDIR call to invoke
 *	FsLocalRemoveDir on the file server.
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
FsSpriteRemoveDir(prefixHandle, relativeName, argsPtr, resultsPtr, 
	       newNameInfoPtrPtr)
    FsHandleHeader   *prefixHandle;	/* Handle from the prefix table */
    char 	   *relativeName;	/* The name of the file to remove */
    Address 	   argsPtr;		/* Ref to FsLookupArgs */
    Address 	   resultsPtr;		/* == NIL */
    FsRedirectInfo **newNameInfoPtrPtr; /* We return this if the server leaves 
					   its domain during the lookup. */
{
    ReturnStatus	status;
    Rpc_Storage		storage;
    FsRedirectInfo	redirectInfo;
    int			prefixLength[3];	/* padded so that network
						 * interface won't pad it
						 * without our being aware
						 * of it. */

    storage.requestParamPtr = (Address) argsPtr;
    storage.requestParamSize = sizeof(FsLookupArgs);
    storage.requestDataPtr = (Address) relativeName;
    storage.requestDataSize = String_Length(relativeName) + 1;
    storage.replyParamPtr = (Address) prefixLength;
    storage.replyParamSize = 3 * sizeof (int);
    storage.replyDataPtr = (Address)&redirectInfo;
    storage.replyDataSize = sizeof(FsRedirectInfo);

    status = Rpc_Call(prefixHandle->fileID.serverID, RPC_FS_RMDIR, &storage);
    if (status == FS_LOOKUP_REDIRECT) {
	*newNameInfoPtrPtr = Mem_New(FsRedirectInfo);
	(*newNameInfoPtrPtr)->prefixLength = prefixLength[0];
	(void)String_Copy(redirectInfo.fileName, (*newNameInfoPtrPtr)->fileName);
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_RpcRemove --
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
Fs_RpcRemove(srvToken, clientID, command, storagePtr)
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
    FsHandleHeader	*prefixHandlePtr;
    FsRedirectInfo	*newNameInfoPtr;
    FsLookupArgs	*lookupArgsPtr;

    lookupArgsPtr = (FsLookupArgs *)storagePtr->requestParamPtr;
    prefixHandlePtr =
	(*fsStreamOpTable[lookupArgsPtr->prefixID.type].clientVerify)
	    (&lookupArgsPtr->prefixID, clientID);
    if (prefixHandlePtr == (FsHandleHeader *) NIL) {
	return(FS_STALE_HANDLE);
    } 
    FsHandleRelease(prefixHandlePtr, TRUE);

    newNameInfoPtr = (FsRedirectInfo *) NIL;
    switch (command) {
	case RPC_FS_UNLINK:
	    status = FsLocalRemove(prefixHandlePtr,
		    (char *) storagePtr->requestDataPtr,
		    (Address) lookupArgsPtr, (Address) NIL, &newNameInfoPtr);
	    break;
	case RPC_FS_RMDIR:
	    status = FsLocalRemoveDir(prefixHandlePtr,
		    (char *) storagePtr->requestDataPtr,
		    (Address) lookupArgsPtr, (Address) NIL, &newNameInfoPtr);
	    break;
	default:
	    Sys_Panic(SYS_FATAL, "Fs_RpcRemove, bad command <%d>\n", 
		command);
	    status = GEN_INVALID_ARG;
    }
    if (status == FS_LOOKUP_REDIRECT) {
	Rpc_ReplyMem	*replyMemPtr;

	storagePtr->replyDataPtr = (Address) newNameInfoPtr;
	storagePtr->replyDataSize = sizeof(FsRedirectInfo);
	/*
	 * We are only returning an int, but the Intel ethernet driver
	 * will gratuitously pad this to 12 bytes to avoid DMA overruns.
	 * The routine NetIEXmit needs to be fixed.  Until then we patch here.
	 */
	storagePtr->replyParamPtr = (Address) Mem_Alloc(3 * sizeof (int));
	storagePtr->replyParamSize = 3 * sizeof (int);
	*((int *) (storagePtr->replyParamPtr)) = newNameInfoPtr->prefixLength;

	replyMemPtr = (Rpc_ReplyMem *) Mem_Alloc(sizeof(Rpc_ReplyMem));
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
 * FsSpriteMakeDir --
 *
 *	Make the named directory.  This uses the RPC_FS_MAKE_DIR call
 *	to invoke FsLocalMakeDir on the file server.
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
FsSpriteMakeDir(prefixHandle, relativeName, argsPtr, resultsPtr, 
		newNameInfoPtrPtr)
    FsHandleHeader *prefixHandle;   /* Handle from the prefix table */
    char 	   *relativeName;   /* The name of the directory to create */
    Address 	   argsPtr;	    /* Ref. to FsOpenArgs */
    Address 	   resultsPtr;	    /* == NIL */
    FsRedirectInfo **newNameInfoPtrPtr;/* We return this if the server leaves 
					* its domain during the lookup. */
{
    ReturnStatus	status;
    Rpc_Storage		storage;
    FsRedirectInfo	redirectInfo;
    int			prefixLength[3];	/* padded so that network
						 * interface won't pad it
						 * without our being aware
						 * of it. */

    storage.requestParamPtr = (Address) argsPtr;
    storage.requestParamSize = sizeof(FsOpenArgs);
    storage.requestDataPtr = (Address) relativeName;
    storage.requestDataSize = String_Length(relativeName) + 1;
    storage.replyParamPtr = (Address) prefixLength;
    storage.replyParamSize = 3 * sizeof (int);
    storage.replyDataPtr = (Address)&redirectInfo;
    storage.replyDataSize = sizeof(FsRedirectInfo);

    status = Rpc_Call(prefixHandle->fileID.serverID, RPC_FS_MKDIR, &storage);
    if (status == FS_LOOKUP_REDIRECT) {
	*newNameInfoPtrPtr = Mem_New(FsRedirectInfo);
	(*newNameInfoPtrPtr)->prefixLength = prefixLength[0];
	(void)String_Copy(redirectInfo.fileName, (*newNameInfoPtrPtr)->fileName);
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_RpcMakeDir --
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
Fs_RpcMakeDir(srvToken, clientID, command, storagePtr)
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
    FsHandleHeader	*prefixHandlePtr;
    FsRedirectInfo	*newNameInfoPtr;
    FsOpenArgs		*openArgsPtr;

    openArgsPtr = (FsOpenArgs *) storagePtr->requestParamPtr;
    if (openArgsPtr->prefixID.serverID != rpc_SpriteID) {
	return(GEN_INVALID_ARG);
    }

    prefixHandlePtr =
	(*fsStreamOpTable[openArgsPtr->prefixID.type].clientVerify)
	    (&openArgsPtr->prefixID, clientID);
    if (prefixHandlePtr == (FsHandleHeader *) NIL) {
	return(FS_STALE_HANDLE);
    }
    FsHandleRelease(prefixHandlePtr, TRUE);

    newNameInfoPtr = (FsRedirectInfo *) NIL;
    status = FsLocalMakeDir(prefixHandlePtr, (char *)storagePtr->requestDataPtr,
	    (Address) openArgsPtr, (Address) NIL, &newNameInfoPtr);
    if (status == FS_LOOKUP_REDIRECT) {
	Rpc_ReplyMem	*replyMemPtr;

	storagePtr->replyDataPtr = (Address)newNameInfoPtr;
	storagePtr->replyDataSize = sizeof(FsRedirectInfo);
	/*
	 * prefixLength must be returned in param area for byte-swapping.
	 * We pad the buffer to 12 bytes here to avoid bug in NetIEXmit!
	 */
	storagePtr->replyParamPtr = (Address) Mem_Alloc(3 * sizeof (int));
	storagePtr->replyParamSize = 3 * sizeof (int);
	*((int *)(storagePtr->replyParamPtr)) = newNameInfoPtr->prefixLength;

        replyMemPtr = (Rpc_ReplyMem *) Mem_Alloc(sizeof(Rpc_ReplyMem));
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
 * FsSpriteMakeDevice --
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
FsSpriteMakeDevice(prefixHandle, relativeName, argsPtr, resultsPtr,
			       newNameInfoPtrPtr)
    FsHandleHeader *prefixHandle;   /* Handle from the prefix table */
    char           *relativeName;   /* The name of the file. */
    Address        argsPtr;	    /* Ref. to FsMakeDevArgs */
    Address        resultsPtr;	    /* == NIL */
    FsRedirectInfo **newNameInfoPtrPtr;/* We return this if the server leaves 
					* its domain during the lookup. */
{
    ReturnStatus	status;
    Rpc_Storage		storage;
    FsRedirectInfo	redirectInfo;
    int			prefixLength[3];	/* padded so that network
						 * interface won't pad it
						 * without our being aware
						 * of it. */

    storage.requestParamPtr = (Address) argsPtr;
    storage.requestParamSize = sizeof(FsMakeDeviceArgs);
    storage.requestDataPtr = (Address) relativeName;
    storage.requestDataSize = String_Length(relativeName) + 1;
    storage.replyParamPtr = (Address) prefixLength;
    storage.replyParamSize = 3 * sizeof (int);
    storage.replyDataPtr = (Address)Mem_Alloc(sizeof(FsRedirectInfo));
    storage.replyDataSize = sizeof(FsRedirectInfo);

    status = Rpc_Call(prefixHandle->fileID.serverID, RPC_FS_MKDEV, &storage);
    if (status == FS_LOOKUP_REDIRECT) {
	*newNameInfoPtrPtr = (FsRedirectInfo *) Mem_New(FsRedirectInfo);
	(*newNameInfoPtrPtr)->prefixLength = prefixLength[0];
	(void)String_Copy(redirectInfo.fileName, (*newNameInfoPtrPtr)->fileName);
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_RpcMakeDev --
 *
 *	Service stub for RPC_FS_MKDEV.  This calls FsLocalMakeDevice.
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
Fs_RpcMakeDev(srvToken, clientID, command, storagePtr)
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
    FsMakeDeviceArgs	*makeDevArgsPtr;
    FsHandleHeader	*prefixHandlePtr;
    FsRedirectInfo	*newNameInfoPtr;

    makeDevArgsPtr = (FsMakeDeviceArgs *) storagePtr->requestParamPtr;
    prefixHandlePtr = 
	(*fsStreamOpTable[makeDevArgsPtr->prefixID.type].clientVerify)
	    (&makeDevArgsPtr->prefixID, clientID);
    if (prefixHandlePtr == (FsHandleHeader *) NIL) {
	return(FS_STALE_HANDLE);
    }
    FsHandleRelease(prefixHandlePtr, TRUE);

    newNameInfoPtr = (FsRedirectInfo *) NIL;
    status = FsLocalMakeDevice(prefixHandlePtr,
	    (char *)storagePtr->requestDataPtr, (Address) makeDevArgsPtr,
	    (Address) NIL, &newNameInfoPtr);
    if (status == FS_LOOKUP_REDIRECT) {
	Rpc_ReplyMem	*replyMemPtr;

	storagePtr->replyDataPtr = (Address)newNameInfoPtr;
	storagePtr->replyDataSize = sizeof(FsRedirectInfo);
	/*
	 * Return prefix length in buffer padded to 12 bytes to avoid
	 * a bug in NetIEXmit.
	 */
	storagePtr->replyParamPtr = (Address) Mem_Alloc(3 * sizeof (int));
	storagePtr->replyParamSize = 3 * sizeof (int);
	*((int *) (storagePtr->replyParamPtr)) = newNameInfoPtr->prefixLength;

        replyMemPtr = (Rpc_ReplyMem *) Mem_Alloc(sizeof(Rpc_ReplyMem));
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
    FsHandleHeader 	*prefixHandle1;		/* Handle from prefix table */
    char 		*relativeName1;		/* The new name of the file. */
    FsHandleHeader 	*prefixHandle2;		/* Handle from prefix table */
    char 		*relativeName2;		/* The new name of the file. */
    FsLookupArgs	*lookupArgsPtr;		/* Contains IDs */
    FsRedirectInfo	**newNameInfoPtrPtr;	/* We return this if the server
						 * leaves its domain during the
						 * lookup. */
    Boolean 		*name1ErrorPtr;		/* If we return REDIRECT or
						 * STALE_HANDLE this indicates
						 * if that applies to the first
						 * pathname or the second */
{
    FsSprite2PathParams	params;
    FsSprite2PathData	*requestDataPtr;	/* too big for stack */
    FsSprite2PathReplyParams	replyParams;
    Rpc_Storage		storage;
    ReturnStatus	status;
    FsRedirectInfo	redirectInfo;

    requestDataPtr = Mem_New(FsSprite2PathData);

    params.lookupArgs = *lookupArgsPtr;
    params.lookupArgs.prefixID = prefixHandle1->fileID;
    params.prefixID2 = prefixHandle2->fileID;

    (void)String_Copy(relativeName1, requestDataPtr->path1);
    (void)String_Copy(relativeName2, requestDataPtr->path2);

    storage.requestParamPtr = (Address) &params;
    storage.requestParamSize = sizeof (FsSprite2PathParams);
    storage.requestDataPtr = (Address) requestDataPtr;
    storage.requestDataSize = sizeof (FsSprite2PathData);

    storage.replyParamPtr = (Address) &replyParams;
    storage.replyParamSize = sizeof (FsSprite2PathReplyParams);
    storage.replyDataPtr = (Address)&redirectInfo;
    storage.replyDataSize = sizeof(FsRedirectInfo);

    status = Rpc_Call(prefixHandle1->fileID.serverID, command, &storage);
    *name1ErrorPtr = replyParams.name1ErrorP;
    if (status == FS_LOOKUP_REDIRECT) {
	*newNameInfoPtrPtr = Mem_New(FsRedirectInfo);
	(*newNameInfoPtrPtr)->prefixLength = replyParams.prefixLength;
	(void)String_Copy(redirectInfo.fileName, (*newNameInfoPtrPtr)->fileName);
    }
    Mem_Free((Address)requestDataPtr);

    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_Rpc2Path --
 *
 *	Common service stub for FsSpriteRename and FsSpriteHardLink.
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
Fs_Rpc2Path(srvToken, clientID, command, storagePtr)
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
    register	FsSprite2PathParams	*paramsPtr;
    register	FsLookupArgs		*lookupArgsPtr;
    register	FsHandleHeader		*prefixHandle1Ptr;
    register	FsHandleHeader		*prefixHandle2Ptr;
    register	Rpc_ReplyMem		*replyMemPtr;
    FsRedirectInfo			*newNameInfoPtr;
    Boolean				name1Error = FALSE;
    FsSprite2PathReplyParams		*replyParamsPtr;
    FsSprite2PathData			*pathDataPtr;
    ReturnStatus			status;

    paramsPtr = (FsSprite2PathParams *)storagePtr->requestParamPtr;
    pathDataPtr = (FsSprite2PathData *)storagePtr->requestDataPtr;
    lookupArgsPtr = &paramsPtr->lookupArgs;
    prefixHandle1Ptr =
	(*fsStreamOpTable[lookupArgsPtr->prefixID.type].clientVerify)
	    (&lookupArgsPtr->prefixID, clientID);

    if (prefixHandle1Ptr == (FsHandleHeader *)NIL) {
	name1Error = TRUE;
	status = FS_STALE_HANDLE;
	goto exit;
    } else {
	FsHandleUnlock(prefixHandle1Ptr);
    }
    if (paramsPtr->prefixID2.serverID != rpc_SpriteID) {
	/*
	 * Second pathname doesn't even start with us.  However, we are
	 * called in case the first pathname redirects away from us.
	 */
	prefixHandle2Ptr = (FsHandleHeader *)NIL;
    } else {
	prefixHandle2Ptr =
	    (*fsStreamOpTable[paramsPtr->prefixID2.type].clientVerify)
		(&paramsPtr->prefixID2, clientID);
	if (prefixHandle2Ptr == (FsHandleHeader *)NIL) {
	    FsHandleRelease(prefixHandle1Ptr, FALSE);
	    name1Error = FALSE;
	    status = FS_STALE_HANDLE;
	    goto exit;
	} else {
	    FsHandleUnlock(prefixHandle2Ptr);
	}
    }

    newNameInfoPtr = (FsRedirectInfo *) NIL;
    if (command == RPC_FS_RENAME) {
	status = FsLocalRename(prefixHandle1Ptr, pathDataPtr->path1, 
		  		prefixHandle2Ptr, pathDataPtr->path2,
				lookupArgsPtr, &newNameInfoPtr, &name1Error);
    } else if (command == RPC_FS_LINK) {
	status = FsLocalHardLink(prefixHandle1Ptr, pathDataPtr->path1, 
				prefixHandle2Ptr, pathDataPtr->path2, 
				lookupArgsPtr, &newNameInfoPtr, &name1Error);
    } else {
	Sys_Panic(SYS_FATAL, "Fs_Rpc2Path: Bad command %d\n", command);
    }
    FsHandleRelease(prefixHandle1Ptr, FALSE);
    if (prefixHandle2Ptr != (FsHandleHeader *)NIL) {
	FsHandleRelease(prefixHandle2Ptr, FALSE);
    }
exit:
    replyParamsPtr = (FsSprite2PathReplyParams *)
	    Mem_Alloc(sizeof (FsSprite2PathReplyParams));
    replyParamsPtr->name1ErrorP = name1Error;
    storagePtr->replyParamPtr = (Address) replyParamsPtr;
    storagePtr->replyParamSize = sizeof (FsSprite2PathReplyParams);
    if (status == FS_LOOKUP_REDIRECT) {
	replyParamsPtr->prefixLength = newNameInfoPtr->prefixLength;
	storagePtr->replyDataPtr = (Address) newNameInfoPtr;
	storagePtr->replyDataSize = sizeof(int) +
				String_Length(newNameInfoPtr->fileName) + 1;
    } else {
	replyParamsPtr->prefixLength = 0;
	/*
	 * Reply data ptr already set to NIL
	 */
     }
    replyMemPtr = (Rpc_ReplyMem *) Mem_Alloc(sizeof(Rpc_ReplyMem));
    replyMemPtr->paramPtr = (Address) replyParamsPtr;
    replyMemPtr->dataPtr = (Address) storagePtr->replyDataPtr;
    Rpc_Reply(srvToken, status, storagePtr, 
	      (int (*)()) Rpc_FreeMem, (ClientData) replyMemPtr);

    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * FsSpriteRename --
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
FsSpriteRename(prefixHandle1, relativeName1, prefixHandle2, relativeName2,
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
    return(TwoNameOperation(RPC_FS_RENAME, prefixHandle1, relativeName1, 
		     	    prefixHandle2, relativeName2, lookupArgsPtr, 
			    newNameInfoPtrPtr, name1ErrorPtr));
}


/*
 *----------------------------------------------------------------------
 *
 * FsSpriteHardLink --
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
FsSpriteHardLink(prefixHandle1, relativeName1, prefixHandle2, relativeName2,
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
    return(TwoNameOperation(RPC_FS_LINK, prefixHandle1, relativeName1, 
		     	    prefixHandle2, relativeName2, lookupArgsPtr, 
			    newNameInfoPtrPtr, name1ErrorPtr));
}

