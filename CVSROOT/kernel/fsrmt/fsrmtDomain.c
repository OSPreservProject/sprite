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
#include "fsUnixDomain.h"
#include "fsSpriteDomain.h"
#include "fsLocalDomain.h"
#include "fsOpTable.h"
#include "fsTrace.h"
#include "fsDebug.h"
#include "fsStat.h"
#include "recov.h"
#include "proc.h"
#include "rpc.h"
#include "vm.h"
#include "dbg.h"


/*
 *----------------------------------------------------------------------
 *
 * FsSpritePrefix --
 *
 *	Get a handle for a prefix.  This conducts an RPC_FS_SPRITE_PREFIX
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
    FsFileID		fileID;		/* Returned from server */
    ClientData		streamData;	/* Returned from server */
    FsHandleHeader	**hdrPtrPtr = (FsHandleHeader **)resultsPtr;
    int			flags = 0;

    *hdrPtrPtr = (FsHandleHeader *)NIL;

    storage.requestParamPtr = (Address) NIL;
    storage.requestParamSize = 0;
    storage.requestDataPtr = (Address)fileName;
    storage.requestDataSize = String_Length(fileName)+1;

    storage.replyParamPtr = (Address)&fileID;
    storage.replyParamSize = sizeof(FsFileID);
    streamData = (ClientData)Mem_Alloc(256);	/* Free'ed by cltOpen proc */
    storage.replyDataPtr = (Address)streamData;
    storage.replyDataSize = 256;

    status = Rpc_Call(RPC_BROADCAST_SERVER_ID, RPC_FS_SPRITE_PREFIX, &storage);

    if (status == SUCCESS) {
	/*
	 * Use the client-open routine to set up an I/O handle for the prefix.
	 */
	status = (*fsStreamOpTable[fileID.type].cltOpen)(&fileID, &flags,
		    rpc_SpriteID, (ClientData)streamData, (FsNameInfo *)NIL,
		    hdrPtrPtr);
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
 *	Server stub for RPC_FS_SPRITE_PREFIX.  This looks in the prefix
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
    int					domainType;
    ReturnStatus			status;

    status = FsPrefixLookup((char *) storagePtr->requestDataPtr,
			    FS_EXPORTED_PREFIX|FS_EXACT_PREFIX, clientID,
			    &hdrPtr, &lookupName, &domainType, &prefixPtr);
    if (status == SUCCESS) {
	register Rpc_ReplyMem		*replyMemPtr;
	register FsLocalFileIOHandle	*handlePtr;
	register FsFileID		*fileIDPtr;
	ClientData			streamData;
	int				dataSize;

	if (hdrPtr->fileID.type != FS_LCL_FILE_STREAM) {
	    Sys_Panic(SYS_FATAL,
		"Fs_RpcPrefix, found non-local exported prefix\n");
	    return(RPC_NO_REPLY);
	}
	/*
	 * Use the server-open routine to set up the streamData.
	 */
	handlePtr = (FsLocalFileIOHandle *)hdrPtr;
	fileIDPtr = Mem_New(FsFileID);
	FsHandleLock(handlePtr);
	status = (*fsOpenOpTable[handlePtr->descPtr->fileType].srvOpen)
		    (handlePtr, clientID, 0, (FsFileID *)NIL, fileIDPtr,
		     &dataSize, &streamData);
	FsHandleUnlock(handlePtr);

	if (status == SUCCESS) {
	    storagePtr->replyParamPtr = (Address) fileIDPtr;
	    storagePtr->replyParamSize = sizeof(FsFileID);
	    storagePtr->replyDataPtr = (Address) streamData;
	    storagePtr->replyDataSize = dataSize;

	    replyMemPtr = (Rpc_ReplyMem *) Mem_Alloc(sizeof(Rpc_ReplyMem));
	    replyMemPtr->paramPtr = storagePtr->replyParamPtr;
	    replyMemPtr->dataPtr = storagePtr->replyDataPtr;
	    Rpc_Reply(srvToken, SUCCESS, storagePtr, Rpc_FreeMem,
		    (ClientData)replyMemPtr);
	    return(SUCCESS);
	} else {
	    Mem_Free((Address) fileIDPtr);
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
 *	Open a remote file.  This sets up and conducts an RPC_FS_SPRITE_OPEN
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
    char		replyData[sizeof(FsRedirectInfo)];	 /* This
					 * gets filled with either streamData
					 * from the file-type srvOpen routine,
					 * or with a redirected pathname */
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
    storage.replyParamPtr = (Address) openResultsPtr;
    storage.replyParamSize = sizeof(FsOpenResults);
    storage.replyDataPtr = (Address) replyData;
    storage.replyDataSize = sizeof(FsRedirectInfo);

    status = Rpc_Call(prefixHandle->fileID.serverID, RPC_FS_SPRITE_OPEN,
		      &storage);
    if (status == SUCCESS) {
	/*
	 * Allocate space for the stream data returned by the server.
	 * We then copy the streamData from our stack buffer.
	 */
	openResultsPtr->dataSize = storage.replyDataSize;
	openResultsPtr->streamData =
		(ClientData)Mem_Alloc(storage.replyDataSize);
	Byte_Copy(storage.replyDataSize, (Address)replyData, 
		    (Address)openResultsPtr->streamData);
    } else if (status == FS_LOOKUP_REDIRECT) {
	/*
	 * Allocate enough space to fit the prefix length and the file name and
	 * copy over the structure that we have on our stack.
	 */
	register FsRedirectInfo *newNameInfoPtr = (FsRedirectInfo *)replyData;
	register int redirectSize;

	redirectSize = sizeof(int) + String_Length(newNameInfoPtr->fileName)+1;
	*newNameInfoPtrPtr = (FsRedirectInfo *) Mem_Alloc(redirectSize);
	(*newNameInfoPtrPtr)->prefixLength = newNameInfoPtr->prefixLength;
	String_Copy(newNameInfoPtr->fileName, (*newNameInfoPtrPtr)->fileName);
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


    if (Recov_GetClientState(clientID) & RECOV_IN_PROGRESS) {
	Net_HostPrint(clientID, "Dropping regular open during recovery\n");
	return(RPC_SERVICE_DISABLED);
    }
    openArgsPtr = (FsOpenArgs *) storagePtr->requestParamPtr;

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
    openResultsPtr = Mem_New(FsOpenResults);
    status = FsLocalOpen(prefixHandlePtr, (char *)storagePtr->requestDataPtr,
		(Address)openArgsPtr, (Address)openResultsPtr, &newNameInfoPtr);
    FsHandleRelease(prefixHandlePtr, FALSE);
    if (status == SUCCESS) {
	/*
	 * The open worked.  We return the whole FsOpenResults structure
	 * in the RPC parameter area, but it contains a pointer to
	 * stream data and a dataSize. That stream data is returned in
	 * the RPC data area.
	 */
	storagePtr->replyParamPtr = (Address)openResultsPtr;
	storagePtr->replyParamSize = sizeof(FsOpenResults);
	storagePtr->replyDataPtr = (Address)openResultsPtr->streamData;
	storagePtr->replyDataSize = openResultsPtr->dataSize;
    } else if (status == FS_LOOKUP_REDIRECT) {
	/*
	 * The file is not found on this server.
	 */
	storagePtr->replyParamPtr = (Address) NIL;
	storagePtr->replyParamSize = 0;
	storagePtr->replyDataPtr = (Address)newNameInfoPtr;
	storagePtr->replyDataSize = sizeof(FsRedirectInfo);
	Mem_Free((Address)openResultsPtr);
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
	Mem_Free((Address)openResultsPtr);
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
	Net_HostPrint(clientID, "starting recovery");
    }

    fileIDPtr = (FsFileID *)storagePtr->requestParamPtr;
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
 * Request params for the close RPC.  The request data area is used for stream
 * specific data that gets pushed back to the server when the client closes.
 * 
 */
typedef struct FsSpriteCloseParams {
    FsFileID	fileID;		/* File to close */
    int		flags;		/* Flags from the stream */
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
FsSpriteClose(rmtHandlePtr, clientID, flags, dataSize, closeData)
    FsRemoteIOHandle	*rmtHandlePtr;	/* Handle to close */
    int			clientID;	/* IGNORED, implicitly passed by RPC */
    int			flags;		/* Flags from the stream being closed */
    int			dataSize;	/* Size of *closeData, or Zero */
    ClientData		closeData;	/* Copy of cached I/O attributes. */
{
    Rpc_Storage 	storage;
    ReturnStatus 	status;
    FsSpriteCloseParams	params;

    params.fileID = rmtHandlePtr->hdr.fileID;
    params.flags = flags;

    storage.requestParamPtr = (Address)&params;
    storage.requestParamSize = sizeof(params);
    storage.requestDataPtr = (Address)closeData;
    storage.requestDataSize = dataSize;
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
    register	FsHandleHeader		*hdrPtr;
    ReturnStatus			status;

    paramsPtr = (FsSpriteCloseParams *) storagePtr->requestParamPtr;

    hdrPtr = (*fsStreamOpTable[paramsPtr->fileID.type].clientVerify)
		(&paramsPtr->fileID, clientID);
    if (hdrPtr == (FsHandleHeader *) NIL) {
	status = FS_STALE_HANDLE;
    } else  {
	/*
	 * Call the file type close routine to release the I/O handle
	 * and clean up.  This call unlocks and decrements the reference
	 * count on the handle.
	 */
	status = (*fsStreamOpTable[hdrPtr->fileID.type].close)(hdrPtr,
		clientID, paramsPtr->flags, storagePtr->requestDataSize,
		(ClientData)storagePtr->requestDataPtr);
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

    storage.requestParamPtr = (Address) argsPtr;
    storage.requestParamSize = sizeof(FsLookupArgs);
    storage.requestDataPtr = (Address) relativeName;
    storage.requestDataSize = String_Length(relativeName) + 1;
    storage.replyParamPtr = (Address) NIL;
    storage.replyParamSize = 0;
    storage.replyDataPtr = (Address)&redirectInfo;
    storage.replyDataSize = sizeof(FsRedirectInfo);

    status = Rpc_Call(prefixHandle->fileID.serverID, RPC_FS_UNLINK, &storage);
    if (status == FS_LOOKUP_REDIRECT) {
	register int redirectSize;
	redirectSize = sizeof(int) + String_Length(redirectInfo.fileName) + 1;
	*newNameInfoPtrPtr = (FsRedirectInfo *) Mem_Alloc(redirectSize);
	(*newNameInfoPtrPtr)->prefixLength = redirectInfo.prefixLength;
	String_Copy(redirectInfo.fileName, (*newNameInfoPtrPtr)->fileName);
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

    storage.requestParamPtr = (Address) argsPtr;
    storage.requestParamSize = sizeof(FsLookupArgs);
    storage.requestDataPtr = (Address) relativeName;
    storage.requestDataSize = String_Length(relativeName) + 1;
    storage.replyParamPtr = (Address) NIL;
    storage.replyParamSize = 0;
    storage.replyDataPtr = (Address)&redirectInfo;
    storage.replyDataSize = sizeof(FsRedirectInfo);

    status = Rpc_Call(prefixHandle->fileID.serverID, RPC_FS_RMDIR, &storage);
    if (status == FS_LOOKUP_REDIRECT) {
	register int redirectSize;
	redirectSize = sizeof(int) + String_Length(redirectInfo.fileName) + 1;
	*newNameInfoPtrPtr = (FsRedirectInfo *) Mem_Alloc(redirectSize);
	(*newNameInfoPtrPtr)->prefixLength = redirectInfo.prefixLength;
	String_Copy(redirectInfo.fileName, (*newNameInfoPtrPtr)->fileName);
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

    storage.requestParamPtr = (Address) argsPtr;
    storage.requestParamSize = sizeof(FsOpenArgs);
    storage.requestDataPtr = (Address) relativeName;
    storage.requestDataSize = String_Length(relativeName) + 1;
    storage.replyParamPtr = (Address) NIL;
    storage.replyParamSize = 0;
    storage.replyDataPtr = (Address)&redirectInfo;
    storage.replyDataSize = sizeof(FsRedirectInfo);

    status = Rpc_Call(prefixHandle->fileID.serverID, RPC_FS_MKDIR, &storage);
    if (status == FS_LOOKUP_REDIRECT) {
	register int redirectSize;
	redirectSize = sizeof(int) + String_Length(redirectInfo.fileName) + 1;
	*newNameInfoPtrPtr = (FsRedirectInfo *) Mem_Alloc(redirectSize);
	(*newNameInfoPtrPtr)->prefixLength = redirectInfo.prefixLength;
	String_Copy(redirectInfo.fileName, (*newNameInfoPtrPtr)->fileName);
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
    ReturnStatus		status;
    Rpc_Storage			storage;
    FsRedirectInfo		redirectInfo;

    storage.requestParamPtr = (Address) argsPtr;
    storage.requestParamSize = sizeof(FsMakeDeviceArgs);
    storage.requestDataPtr = (Address) relativeName;
    storage.requestDataSize = String_Length(relativeName) + 1;
    storage.replyParamPtr = (Address) NIL;
    storage.replyParamSize = 0;
    storage.replyDataPtr = (Address)Mem_Alloc(sizeof(FsRedirectInfo));
    storage.replyDataSize = sizeof(FsRedirectInfo);

    status = Rpc_Call(prefixHandle->fileID.serverID, RPC_FS_MKDEV, &storage);
    if (status == FS_LOOKUP_REDIRECT) {
	register int redirectSize;
	redirectSize = sizeof(int) + String_Length(redirectInfo.fileName) + 1;
	*newNameInfoPtrPtr = (FsRedirectInfo *) Mem_Alloc(redirectSize);
	(*newNameInfoPtrPtr)->prefixLength = redirectInfo.prefixLength;
	String_Copy(redirectInfo.fileName, (*newNameInfoPtrPtr)->fileName);
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
		 relativeName2, lookupArgsPtr, newNameInfoPtrPtr,
		 name1RedirectPtr)
    FsHandleHeader 	*prefixHandle1;		/* Handle from prefix table */
    char 		*relativeName1;		/* The new name of the file. */
    FsHandleHeader 	*prefixHandle2;		/* Handle from prefix table */
    char 		*relativeName2;		/* The new name of the file. */
    FsLookupArgs	*lookupArgsPtr;		/* Contains IDs */
    FsRedirectInfo	**newNameInfoPtrPtr;	/* We return this if the server
						 * leaves its domain during the
						 * lookup. */
    Boolean 		*name1RedirectPtr;	/* TRUE if redirect info is for
						 * first path */
{
    FsSprite2PathParams	params;
    int			nameLength;
    Rpc_Storage		storage;
    ReturnStatus	status;
    FsRedirectInfo	redirectInfo;

    nameLength = String_Length(relativeName1);
    params.lookupArgs = *lookupArgsPtr;
    params.lookupArgs.prefixID = prefixHandle1->fileID;
    params.prefixID2 = prefixHandle2->fileID;
    Byte_Copy(nameLength, (Address) relativeName1, (Address) params.path1);
    params.path1[nameLength] = '\0';

    storage.requestParamPtr = (Address) &params;
    storage.requestParamSize = sizeof(FsLookupArgs) + sizeof(FsFileID) + 
			       nameLength + 1;
    storage.requestDataPtr = (Address) relativeName2;
    storage.requestDataSize = String_Length(relativeName2) + 1;
    storage.replyParamPtr = (Address) name1RedirectPtr;
    storage.replyParamSize = sizeof(Boolean);
    storage.replyDataPtr = (Address)&redirectInfo;
    storage.replyDataSize = sizeof(FsRedirectInfo);

    status = Rpc_Call(prefixHandle1->fileID.serverID, command, &storage);
    if (status == FS_LOOKUP_REDIRECT) {
	register int redirectSize;
	redirectSize = sizeof(int) + String_Length(redirectInfo.fileName) + 1;
	*newNameInfoPtrPtr = (FsRedirectInfo *) Mem_Alloc(redirectSize);
	(*newNameInfoPtrPtr)->prefixLength = redirectInfo.prefixLength;
	String_Copy(redirectInfo.fileName, (*newNameInfoPtrPtr)->fileName);
    }

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
    Boolean				name1Redirect;
    Boolean				*name1RedirectPtr;
    ReturnStatus			status;

    paramsPtr = (FsSprite2PathParams *)storagePtr->requestParamPtr;
    lookupArgsPtr = &paramsPtr->lookupArgs;
    prefixHandle1Ptr =
	(*fsStreamOpTable[lookupArgsPtr->prefixID.type].clientVerify)
	    (&lookupArgsPtr->prefixID, clientID);

    if (prefixHandle1Ptr == (FsHandleHeader *)NIL) {
	return(FS_STALE_HANDLE);
    } else {
	FsHandleUnlock(prefixHandle1Ptr);
    }
    prefixHandle2Ptr =
	(*fsStreamOpTable[paramsPtr->prefixID2.type].clientVerify)
	    (&paramsPtr->prefixID2, clientID);
    if (prefixHandle2Ptr == (FsHandleHeader *)NIL) {
	FsHandleRelease(prefixHandle1Ptr, FALSE);
	return(FS_STALE_HANDLE);
    } else {
	FsHandleUnlock(prefixHandle2Ptr);
    }

    newNameInfoPtr = (FsRedirectInfo *) NIL;
    if (command == RPC_FS_RENAME) {
	status = FsLocalRename(prefixHandle1Ptr, paramsPtr->path1, 
		  		prefixHandle2Ptr, 
				(char *) storagePtr->requestDataPtr,
				lookupArgsPtr, &newNameInfoPtr, 
				&name1Redirect);
    } else if (command == RPC_FS_LINK) {
	status = FsLocalHardLink(prefixHandle1Ptr, paramsPtr->path1, 
				prefixHandle2Ptr, 
				(char *) storagePtr->requestDataPtr, 
				lookupArgsPtr, &newNameInfoPtr, 
				&name1Redirect);
    } else {
	Sys_Panic(SYS_FATAL, "Fs_Rpc2Path: Bad command %d\n", command);
    }
    FsHandleRelease(prefixHandle1Ptr, FALSE);
    FsHandleRelease(prefixHandle2Ptr, FALSE);

    if (status != FS_LOOKUP_REDIRECT) {
	Rpc_Reply(srvToken, status, storagePtr, (int (*)())NIL,
		(ClientData)NIL);
    } else {
	name1RedirectPtr = (Boolean *) Mem_Alloc(sizeof(Boolean));
	*name1RedirectPtr = name1Redirect;
	storagePtr->replyParamPtr = (Address) name1RedirectPtr;
	storagePtr->replyParamSize = sizeof(Boolean);
	storagePtr->replyDataPtr = (Address) newNameInfoPtr;
	storagePtr->replyDataSize = sizeof(int) +
				    String_Length(newNameInfoPtr->fileName) + 1;
	replyMemPtr = (Rpc_ReplyMem *) Mem_Alloc(sizeof(Rpc_ReplyMem));
	replyMemPtr->paramPtr = (Address) name1RedirectPtr;
	replyMemPtr->dataPtr = (Address) newNameInfoPtr;
	Rpc_Reply(srvToken, SUCCESS, storagePtr, 
		  (int (*)()) Rpc_FreeMem, (ClientData) replyMemPtr);
    }

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
			lookupArgsPtr, newNameInfoPtrPtr, name1redirectPtr)
    FsHandleHeader *prefixHandle1;	/* Handle from the prefix table */
    char *relativeName1;		/* The new name of the file. */
    FsHandleHeader *prefixHandle2;	/* Token from the prefix table */
    char *relativeName2;		/* The new name of the file. */
    FsLookupArgs *lookupArgsPtr;	/* Contains IDs */
    FsRedirectInfo **newNameInfoPtrPtr;	/* We return this if the server leaves 
					 * its domain during the lookup. */
    Boolean *name1redirectPtr;	/* TRUE if redirect info is for first path */
{
    return(TwoNameOperation(RPC_FS_RENAME, prefixHandle1, relativeName1, 
		     	    prefixHandle2, relativeName2, lookupArgsPtr, 
		     	    newNameInfoPtrPtr, name1redirectPtr));
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
			lookupArgsPtr, newNameInfoPtrPtr, name1redirectPtr)
    FsHandleHeader *prefixHandle1;	/* Token from the prefix table */
    char *relativeName1;		/* The new name of the file. */
    FsHandleHeader *prefixHandle2;	/* Token from the prefix table */
    char *relativeName2;		/* The new name of the file. */
    FsLookupArgs *lookupArgsPtr;	/* Contains IDs */
    FsRedirectInfo **newNameInfoPtrPtr;	/* We return this if the server 
					 * leaves its domain during the lookup*/
    Boolean *name1redirectPtr;	/* TRUE if redirect info is for first path */
{
    return(TwoNameOperation(RPC_FS_LINK, prefixHandle1, relativeName1, 
		     	    prefixHandle2, relativeName2, lookupArgsPtr, 
		     	    newNameInfoPtrPtr, name1redirectPtr));
}

