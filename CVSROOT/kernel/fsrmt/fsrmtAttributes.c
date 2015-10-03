/*
 * fsAttributes.c --
 *
 *	This has procedures for operations done on remote file attributes.
 *	The general strategy when getting attributes is to make one call to
 *	the name server to get an initial version of the attributes, and
 *	then make another call to the I/O server to update things like
 *	the access time and modify time.  There are two ways to get to
 *	the name server, either via a pathname or an open file.  The
 *	I/O server is always contacted using a fileID.
 *	The 3 corrsponding RPCs are:
 *	RPC_FS_GET_ATTR_PATH	To get attributes from name server given path.
 *	RPC_FS_GET_ATTR		To get attributes from name server given fileID
 *	RPC_FS_GET_IO_ATTR	To get (a few) attributes from the I/O server.
 *	Setting attributes is done the same way, and there are three more
 *	corresponding RPCs.
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
#include <fslcl.h>
#include <fsNameOps.h>
#include <fsconsist.h>
#include <fscache.h>
#include <fsdm.h>
#include <fsStat.h>
#include <recov.h>
#include <rpc.h>

#include <string.h>
#include <stdio.h>

static Fs_HandleHeader *VerifyIOHandle _ARGS_((Fs_FileID *fileIDPtr));


/*
 * Return parameters from RPC_FS_GET_ATTR_PATH.  The file ID is used to
 * get to the I/O server.
 */
typedef struct FsRemoteGetAttrResults {
    Fs_Attributes	attributes;
    Fs_FileID		fileID;
} FsRemoteGetAttrResults;


/*
 *----------------------------------------------------------------------
 *
 * FsrmtGetAttrPath --
 *
 *	Get the attributes of a remote Sprite file given its name.  This
 *	is called from Fs_GetAttributes for files named by remote servers.
 *	Note:  the attributes from the disk descriptor on the name server
 *	are returned.  The attributes need to be updated by the I/O server
 *	if there is one.
 *
 * Results:
 *	A return code from the RPC or the remote server.
 *
 * Side effects:
 *	FS_RPC_GET_ATTR.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
FsrmtGetAttrPath(prefixHandle, relativeName, argsPtr, resultsPtr,
         	    newNameInfoPtrPtr)
    Fs_HandleHeader *prefixHandle;	/* Handle from the prefix table */
    char           *relativeName;	/* The name of the file. */
    Address        argsPtr;		/* Bundled arguments for us */
    Address        resultsPtr;		/* Where to store attributes */
    Fs_RedirectInfo **newNameInfoPtrPtr;	/* We return this if the server leaves 
					 * its domain during the lookup. */
{
    ReturnStatus 		status;
    Fs_OpenArgs			*openArgsPtr;
    Fs_GetAttrResults		*getAttrResultsPtr;
    Fs_GetAttrResultsParam	getAttrResultsParam;
    Rpc_Storage			storage;
    char			replyName[FS_MAX_PATH_NAME_LENGTH];	 /* This
						     * may get filled with a
						     * redirected pathname. */

    openArgsPtr = (Fs_OpenArgs *) argsPtr;
    getAttrResultsPtr = (Fs_GetAttrResults *)resultsPtr;

    storage.requestParamPtr = (Address) openArgsPtr;
    storage.requestParamSize = sizeof(Fs_OpenArgs);
    storage.requestDataPtr = (Address) relativeName;
    storage.requestDataSize = strlen(relativeName) + 1;
    storage.replyParamPtr = (Address) &(getAttrResultsParam);
    storage.replyParamSize = sizeof(Fs_GetAttrResultsParam);
    storage.replyDataPtr = (Address) replyName;
    storage.replyDataSize = FS_MAX_PATH_NAME_LENGTH;

    status = Rpc_Call(prefixHandle->fileID.serverID, RPC_FS_GET_ATTR_PATH,
			&storage);
     if (status == SUCCESS) {
	/*
	 * The return param area has a fileID that we need.  The
	 * return parameter area has also been filled in with
	 * the attributes.
	 */
	*(getAttrResultsPtr->fileIDPtr) =
		 getAttrResultsParam.attrResults.fileID;
	*(getAttrResultsPtr->attrPtr) =
		getAttrResultsParam.attrResults.attrs;
    } else if (status == FS_LOOKUP_REDIRECT) {
	/*
	 * Copy the info from our stack to a buffer for our caller
	 */
	*newNameInfoPtrPtr = mnew(Fs_RedirectInfo);
	(*newNameInfoPtrPtr)->prefixLength = getAttrResultsParam.prefixLength;
	(void)strcpy((*newNameInfoPtrPtr)->fileName, replyName);
	return(FS_LOOKUP_REDIRECT);
    }

    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsrmt_RpcGetAttrPath --
 *
 *	Service stub for the RPC_FS_GET_ATTR_PATH call.  This is used to
 *	get the attributes from the disk descriptor of a file.  NOTE:  the
 *	attributes are not complete until the I/O server for the (non-regular)
 *	file is contacted.  This routine does, however, suck in attributes
 *	of regular files that are cached.
 *
 * Results:
 *	If this procedure returns SUCCESS then a reply has been sent to
 *	the client.  If the arguments are bad then an error is 
 *	returned and the main level sends back an error reply.
 *
 * Side effects:
 *	Allocates a buffer to return the results in.
 *	
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
Fsrmt_RpcGetAttrPath(srvToken, clientID, command, storagePtr)
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
    register Fs_OpenArgs		*openArgsPtr;	/* Tmp pointer into openParams*/
    Fs_GetAttrResults 		getAttrResults;	/* Results from local  routine */
    Fs_GetAttrResultsParam	*getAttrResultsParamPtr;	/* rpc param
								 * bundle */
    Fs_HandleHeader		*prefixHandle;	/* Handle for domain */
    ReturnStatus		status;		/* General return code */
    Fs_RedirectInfo		*newNameInfoPtr;/* For prefix re-directs,
						 * unallocated since proc call
						 * allocates space for it. */
    int				domainType;

    openArgsPtr = (Fs_OpenArgs *) storagePtr->requestParamPtr;

    if (openArgsPtr->prefixID.serverID != rpc_SpriteID) {
	/*
	 * Filesystem mis-match.
	 */
	return(GEN_INVALID_ARG);
    }
    prefixHandle = (*fsio_StreamOpTable[openArgsPtr->prefixID.type].clientVerify)
	    (&openArgsPtr->prefixID, clientID, &domainType);
    if (prefixHandle == (Fs_HandleHeader *)NIL) {
	return(FS_STALE_HANDLE);
    }
    Fsutil_HandleRelease(prefixHandle, TRUE);

    newNameInfoPtr = (Fs_RedirectInfo *) NIL;
    getAttrResultsParamPtr = mnew(Fs_GetAttrResultsParam);

    getAttrResults.attrPtr = &(getAttrResultsParamPtr->attrResults.attrs);
    getAttrResults.fileIDPtr = &(getAttrResultsParamPtr->attrResults.fileID);

    fs_Stats.srvName.getAttrs++;
    status = (*fs_DomainLookup[domainType][FS_DOMAIN_GET_ATTR])(prefixHandle,
		(char *) storagePtr->requestDataPtr, (Address)openArgsPtr,
		(Address)(&getAttrResults), &newNameInfoPtr);

    if (status == SUCCESS) {
	storagePtr->replyParamPtr = (Address) getAttrResultsParamPtr;
	storagePtr->replyParamSize = sizeof(Fs_GetAttrResultsParam);
	storagePtr->replyDataPtr = (Address) NIL;
	storagePtr->replyDataSize = 0;
    } else if (status == FS_LOOKUP_REDIRECT) {
	/*
	 * The file is not found on this server, but somewhere else.
	 */
	getAttrResultsParamPtr->prefixLength = newNameInfoPtr->prefixLength;
	storagePtr->replyParamPtr = (Address) getAttrResultsParamPtr;
	storagePtr->replyParamSize = sizeof(Fs_GetAttrResultsParam);
	storagePtr->replyDataSize = strlen(newNameInfoPtr->fileName) + 1;
	storagePtr->replyDataPtr =
		(Address) malloc(storagePtr->replyDataSize);
	(void)strcpy((char *) storagePtr->replyDataPtr, newNameInfoPtr->fileName);
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
	free((Address) getAttrResultsParamPtr);
        return(status);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * FsrmtSetAttrPath --
 *
 *	Set the attributes of a remote Sprite file given its path name.
 *	This is called from Fs_SetAttributes.  This used the
 *	RPC_FS_SET_ATTR_PATH rpc to invoke FslclSetAttrPath on the
 *	name server.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
FsrmtSetAttrPath(prefixHandle, relativeName, argsPtr, resultsPtr,
         	    newNameInfoPtrPtr)
    Fs_HandleHeader *prefixHandle;   /* Token from the prefix table */
    char           *relativeName;   /* The name of the file. */
    Address        argsPtr;	    /* Bundled arguments for us */
    Address        resultsPtr;	    /* FileID */
    Fs_RedirectInfo **newNameInfoPtrPtr; /* We return this if the server leaves 
					 * its domain during the lookup. */
{
    ReturnStatus 		status;
    Rpc_Storage			storage;
    char			replyName[FS_MAX_PATH_NAME_LENGTH];
    Fs_GetAttrResultsParam	getAttrResultsParam;
    Fs_FileID			*fileIDPtr;

    storage.requestParamPtr = (Address) argsPtr;
    storage.requestParamSize = sizeof(Fs_SetAttrArgs);
    storage.requestDataPtr = (Address) relativeName;
    storage.requestDataSize = strlen(relativeName) + 1;
    storage.replyParamPtr = (Address) &(getAttrResultsParam);
    storage.replyParamSize = sizeof(Fs_GetAttrResultsParam);
    storage.replyDataPtr = (Address) replyName;
    storage.replyDataSize = FS_MAX_PATH_NAME_LENGTH;

    fileIDPtr = (Fs_FileID *) resultsPtr;

    status = Rpc_Call(prefixHandle->fileID.serverID, RPC_FS_SET_ATTR_PATH,
			&storage);
    if (status == SUCCESS) {
	/*
	 * Copy result fileID to resultsPtr (== fileIDPtr).
	 */
	(* fileIDPtr) = getAttrResultsParam.attrResults.fileID;
    } else if (status == FS_LOOKUP_REDIRECT) {
	/*
	 * Copy the info from our stack to a buffer for our caller.
	 */
	*newNameInfoPtrPtr = mnew(Fs_RedirectInfo);
	(*newNameInfoPtrPtr)->prefixLength = getAttrResultsParam.prefixLength;
	(void)strcpy((*newNameInfoPtrPtr)->fileName, replyName);
    }

    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsrmt_RpcSetAttrPath --
 *
 *	Service stub for the RPC_FS_SET_ATTR_PATH call.  This branches
 *	to FslclSetAttrPath which sets the attributes on the file descriptor.
 *
 * Results:
 *	If this procedure returns SUCCESS then a reply has been sent to
 *	the client.  If the arguments are bad then an error is 
 *	returned and the main level sends back an error reply.
 *
 * Side effects:
 *	Allocates a buffer to return the results in.
 *	
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
Fsrmt_RpcSetAttrPath(srvToken, clientID, command, storagePtr)
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
    Fs_FileID			*ioFileIDPtr;	/* Results from local routine */
    Fs_HandleHeader		*prefixHandle;	/* Handle for domain */
    ReturnStatus		status;		/* General return code */
    Fs_SetAttrArgs		*setAttrArgsPtr;
    Fs_RedirectInfo		*newNameInfoPtr;/* For prefix re-directs */
    Fs_GetAttrResultsParam	*getAttrResultsParamPtr;	/* rpc param
								 * bundle */
    int				domainType;

    setAttrArgsPtr = (Fs_SetAttrArgs *) storagePtr->requestParamPtr;

    prefixHandle =
	(*fsio_StreamOpTable[setAttrArgsPtr->openArgs.prefixID.type].clientVerify)
	    (&setAttrArgsPtr->openArgs.prefixID, clientID, &domainType);
    if (prefixHandle == (Fs_HandleHeader *)NIL) {
	return(FS_STALE_HANDLE);
    }
    Fsutil_HandleRelease(prefixHandle, TRUE);


    fs_Stats.srvName.setAttrs++;
    newNameInfoPtr = (Fs_RedirectInfo *) NIL;
    getAttrResultsParamPtr = mnew(Fs_GetAttrResultsParam);
    ioFileIDPtr = &(getAttrResultsParamPtr->attrResults.fileID);
    status = (*fs_DomainLookup[domainType][FS_DOMAIN_SET_ATTR])(prefixHandle,
		(char *) storagePtr->requestDataPtr,
		(Address) storagePtr->requestParamPtr,
		(Address) ioFileIDPtr, &newNameInfoPtr);

    if (status == SUCCESS) {
	storagePtr->replyParamPtr = (Address) getAttrResultsParamPtr;
	storagePtr->replyParamSize = sizeof(Fs_GetAttrResultsParam);
	storagePtr->replyDataPtr = (Address) NIL;
	storagePtr->replyDataSize = 0;
    } else if (status == FS_LOOKUP_REDIRECT) {
	/*
	 * The file is not found on this server.
	 */
	getAttrResultsParamPtr->prefixLength = newNameInfoPtr->prefixLength;
	storagePtr->replyParamPtr = (Address) getAttrResultsParamPtr;
	storagePtr->replyParamSize = sizeof(Fs_GetAttrResultsParam);
	storagePtr->replyDataSize = strlen(newNameInfoPtr->fileName) + 1;
	storagePtr->replyDataPtr =
		(Address) malloc(storagePtr->replyDataSize);
	(void)strcpy((char *) storagePtr->replyDataPtr, newNameInfoPtr->fileName);
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
	free((Address)getAttrResultsParamPtr);
        return(status);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * FsrmtGetAttr --
 *
 *	Get the attributes of a remote file given its fileID.  This is called
 *	from Fs_GetAttrStream.  This does an RPC to the name server which
 *	then calls FslclGetAttr.
 *
 * Results:
 *	An error code.
 *
 * Side effects:
 *	Fills in the attributes structure with info from the file server.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
FsrmtGetAttr(fileIDPtr, clientID, attrPtr)
    register Fs_FileID		*fileIDPtr;	/* Identfies file */
    int				clientID;	/* IGNORED, implicitly passed
						 * by the RPC system. */
    register Fs_Attributes	*attrPtr;	/* Return - the attributes */
{
    register ReturnStatus	status;
    Rpc_Storage storage;

    storage.requestParamPtr = (Address) fileIDPtr;
    storage.requestParamSize = sizeof(Fs_FileID);
    storage.requestDataPtr = (Address) NIL;
    storage.requestDataSize = 0;
    storage.replyParamPtr = (Address) attrPtr;
    storage.replyParamSize = sizeof(Fs_Attributes);
    storage.replyDataPtr = (Address) NIL;
    storage.replyDataSize = 0;

    status = Rpc_Call(fileIDPtr->serverID, RPC_FS_GET_ATTR, &storage);
    if (status == RPC_TIMEOUT || status == FS_STALE_HANDLE ||
	status == RPC_SERVICE_DISABLED) {
	/*
	 * If there was a problem at the server then try to recover
	 * the handle.  We may not have a handle in the case of devices.
	 * In that case the fileID we have here comes from the stream's
	 * nameInfo, which doesn't get installed into the handle table.
	 */
	register Fs_HandleHeader *hdrPtr = Fsutil_HandleFetch(fileIDPtr);
	if (hdrPtr != (Fs_HandleHeader *)NIL) {
	    Fsutil_WantRecovery(hdrPtr);
	    Fsutil_HandleRelease(hdrPtr, TRUE);
	}
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsrmt_RpcGetAttr --
 *
 *	Service stub for the RPC_FS_GET_ATTR call.  This calls FslclGetAttr
 *	(or FspdevPseudoGetAttr) to get the attributes of a file.  This
 *	is a name server operation used when the file is already open.
 *	Note: Attributes are not complete until the I/O server has also
 *	been contacted.  See Fsrmt_RpcGetIOAttr.
 *
 * Results:
 *	If this procedure returns SUCCESS then a reply has been sent to
 *	the client.  If the arguments are bad then an error is 
 *	returned and the main level sends back an error reply.
 *
 * Side effects:
 *	Allocates a buffer to return the results in.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
Fsrmt_RpcGetAttr(srvToken, clientID, command, storagePtr)
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
    register ReturnStatus	status;
    Fs_HandleHeader		*tHdrPtr;
    register Fs_HandleHeader	*hdrPtr;
    register Fs_FileID		*fileIDPtr;
    register Fs_Attributes	*attrPtr;
    Rpc_ReplyMem		*replyMemPtr;
    int				domainType = FS_LOCAL_DOMAIN;

    fileIDPtr = (Fs_FileID *) storagePtr->requestParamPtr;
    hdrPtr = VerifyIOHandle(fileIDPtr);
    if (fileIDPtr->type == FSIO_LCL_PFS_STREAM) {
	if (hdrPtr == (Fs_HandleHeader *)NIL) {
	    return(FS_STALE_HANDLE);
	}
	domainType = FS_PSEUDO_DOMAIN;
    } else if (hdrPtr == (Fs_HandleHeader *)NIL) {
	status = Fsio_LocalFileHandleInit(fileIDPtr, (char *)NIL,
			(Fsdm_FileDescriptor *) NIL, FALSE, 
			(Fsio_FileIOHandle **)&tHdrPtr);
	if (status != SUCCESS) {
	    return(status);
	}
	hdrPtr = tHdrPtr;
    }
    Fsutil_HandleUnlock(hdrPtr);

    fs_Stats.srvName.getAttrs++;
    attrPtr = mnew(Fs_Attributes);
    status = (*fs_AttrOpTable[domainType].getAttr)(fileIDPtr, clientID, attrPtr);
#ifdef lint
    status = FslclGetAttr(fileIDPtr, clientID, attrPtr);
    status = FspdevPseudoGetAttr(fileIDPtr, clientID, attrPtr);
#endif lint
    Fsutil_HandleRelease(hdrPtr, FALSE);

    storagePtr->replyParamPtr = (Address) attrPtr;
    storagePtr->replyParamSize = sizeof(Fs_Attributes);
    replyMemPtr = (Rpc_ReplyMem *) malloc(sizeof(Rpc_ReplyMem));
    replyMemPtr->paramPtr = storagePtr->replyParamPtr;
    replyMemPtr->dataPtr = (Address) NIL;

    Rpc_Reply(srvToken, status, storagePtr, Rpc_FreeMem,
	    (ClientData)replyMemPtr);
    return(SUCCESS);
}

/*
 * Parameters for RPC_FS_SET_ATTR.  This is a sub-set of Fs_SetAttrArgs
 * (which has extra stuff required to handle path name lookup).
 */

typedef struct FsRemoteSetAttrParams {
    Fs_FileID		fileID;
    Fs_UserIDs		ids;
    Fs_Attributes	attrs;
    int			flags;
} FsRemoteSetAttrParams;

/*
 *----------------------------------------------------------------------
 *
 * FsrmtSetAttr
 *
 *	Set selected attributes of a remote file given its fileID.  This
 *	is called from Fs_SetAttrStream.  This does the RPC_FS_SET_ATTR
 *	remote call which invokes FslclSetAttr on the name server.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
FsrmtSetAttr(fileIDPtr, attrPtr, idPtr, flags)
    Fs_FileID		*fileIDPtr;
    Fs_Attributes	*attrPtr;
    Fs_UserIDs		 *idPtr;
    int			flags;
{
    ReturnStatus status;
    Rpc_Storage storage;
    FsRemoteSetAttrParams params;

    params.fileID = *fileIDPtr;
    params.ids = *idPtr;
    params.attrs = *attrPtr;
    params.flags = flags;
    storage.requestParamPtr = (Address)&params;
    storage.requestParamSize = sizeof(FsRemoteSetAttrParams);
    storage.requestDataPtr = (Address)NIL;;
    storage.requestDataSize = 0;
    storage.replyParamPtr = (Address)NIL;
    storage.replyParamSize = 0;
    storage.replyDataPtr = (Address)NIL;
    storage.replyDataSize = 0;

    status = Rpc_Call(fileIDPtr->serverID, RPC_FS_SET_ATTR, &storage);
    if (status == RPC_TIMEOUT || status == FS_STALE_HANDLE ||
	status == RPC_SERVICE_DISABLED) {
	register Fs_HandleHeader *hdrPtr = Fsutil_HandleFetch(fileIDPtr);
	if (hdrPtr != (Fs_HandleHeader *)NIL) {
	    Fsutil_WantRecovery(hdrPtr);
	    Fsutil_HandleRelease(hdrPtr, TRUE);
	}
    }

    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsrmt_RpcSetAttr --
 *
 *	Service stub for RPC_FS_SET_ATTR.  This calls FslclSetAttr.
 *
 * Results:
 *	If this procedure returns SUCCESS then a reply has been sent to
 *	the client.  If the arguments are bad then FS_STALE_HANDLE is 
 *	returned and the main level sends back an error reply.
 *
 * Side effects:
 *	Uses FslclSetAttr to set attributes in the file descriptor.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
Fsrmt_RpcSetAttr(srvToken, clientID, command, storagePtr)
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
    Fs_HandleHeader		*hdrPtr;
    register Fs_FileID		*fileIDPtr;
    register Fs_Attributes	*attrPtr;
    register ReturnStatus	status;
    FsRemoteSetAttrParams	*paramPtr;
    int				domainType = FS_LOCAL_DOMAIN;

    paramPtr = (FsRemoteSetAttrParams *) storagePtr->requestParamPtr;
    attrPtr = &paramPtr->attrs;
    fileIDPtr = &paramPtr->fileID;

    hdrPtr = VerifyIOHandle(fileIDPtr);
    if (fileIDPtr->type == FSIO_LCL_PFS_STREAM) {
	if (hdrPtr == (Fs_HandleHeader *)NIL) {
	    return(FS_STALE_HANDLE);
	}
	domainType = FS_PSEUDO_DOMAIN;
    } else if (hdrPtr == (Fs_HandleHeader *)NIL) {
	status = Fsio_LocalFileHandleInit(fileIDPtr, (char *)NIL,
		        (Fsdm_FileDescriptor *) NIL, FALSE, 
			(Fsio_FileIOHandle **)&hdrPtr);
	if (status != SUCCESS) {
	    return(status);
	}
    }
    fs_Stats.srvName.setAttrs++;
    Fsutil_HandleUnlock(hdrPtr);
    status = (*fs_AttrOpTable[domainType].setAttr)(fileIDPtr, attrPtr,
						&paramPtr->ids,paramPtr->flags);
#ifdef lint
    status = FslclSetAttr(fileIDPtr, attrPtr, &paramPtr->ids,paramPtr->flags);
    status = FspdevPseudoSetAttr(fileIDPtr,attrPtr, &paramPtr->ids,paramPtr->flags);
#endif lint
    Fsutil_HandleRelease(hdrPtr, FALSE);

    Rpc_Reply(srvToken, status, storagePtr, (int (*)())NIL,
		(ClientData)NIL);
    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * Fsrmt_GetIOAttr --
 *
 *	Get the attributes cached at a remote I/O server.  This makes the
 *	RPC_FS_GET_IO_ATTR RPC to the I/O server which then calls
 *	stream-type getIOAttrs routine.
 *
 * Results:
 *	An error code.
 *
 * Side effects:
 *	Update attributes structure with info from the I/O server.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
Fsrmt_GetIOAttr(fileIDPtr, clientID, attrPtr)
    register Fs_FileID		*fileIDPtr;	/* Remote device/pipe fileID */
    int				clientID;	/* IGNORED, implicitly passed
						 * by the RPC system. */
    register Fs_Attributes	*attrPtr;	/* In/Out - the attributes are
						 * updated by the I/O server */
{
    register ReturnStatus status;
    Rpc_Storage storage;
    Fs_GetAttrResultsParam	getAttrResultsParam;

    /*
     * We don't want to do full RPC's to hosts we're pretty sure are down.
     */
    if (Recov_IsHostDown(fileIDPtr->serverID) == FAILURE) {
	printf(
	    "Fsrmt_GetIOAttr: skipping device <%d,%d> at server %d: host is down.\n",
	    fileIDPtr->major, fileIDPtr->minor, fileIDPtr->serverID);
	return(SUCCESS);
    }

    getAttrResultsParam.attrResults.fileID = *fileIDPtr;
    getAttrResultsParam.attrResults.attrs = *attrPtr;
    /*
     * We have to fix up the fileID of control handles in the case that
     * the pseudo-device server is remote from us.  The problem is that
     * we have to RPC to the pseduo-device server, but the fileID of the
     * control stream (where the access/modify times are) has the
     * file serverID.  Fortuneatly this is in the regular attributes so
     * we can patch it up.  We add two lines of code here instead of
     * nearly duplicating this procedure into a FsRmtControlGetIOAttr
     * routine that has the patch.
     */
    if (fileIDPtr->type == FSIO_RMT_CONTROL_STREAM) {
	getAttrResultsParam.attrResults.fileID.serverID = attrPtr->serverID;
    }

    storage.requestParamPtr = (Address) &getAttrResultsParam;
    storage.requestParamSize = sizeof(Fs_GetAttrResultsParam);
    storage.requestDataPtr = (Address) NIL;
    storage.requestDataSize = 0;

    storage.replyParamPtr = (Address) attrPtr;
    storage.replyParamSize = sizeof(Fs_Attributes);
    storage.replyDataPtr = (Address) NIL;
    storage.replyDataSize = 0;

    status = Rpc_Call(fileIDPtr->serverID, RPC_FS_GET_IO_ATTR, &storage);
    /*
     * We punt on I/O server recovery, and mask errors so a stat() works.
     */
    if (status != SUCCESS) {
	printf(
	    "Fsrmt_GetIOAttr failed <%x>: device <%d,%d> at server %d\n",
	    status, fileIDPtr->major, fileIDPtr->minor, fileIDPtr->serverID);
	status = SUCCESS;
    }

    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsrmt_RpcGetIOAttr --
 *
 *	Service stub for the RPC_FS_GET_IO_ATTR call.  This calls
 *	the stream-type routine to get attributes cached at the I/O server.
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
Fsrmt_RpcGetIOAttr(srvToken, clientID, command, storagePtr)
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
    register ReturnStatus	status;
    register Fs_HandleHeader	*hdrPtr;
    register Fs_FileID		*fileIDPtr;
    Fs_Attributes		*attrPtr;
    Fs_GetAttrResultsParam	*getAttrResultsParamPtr;

    getAttrResultsParamPtr =
	    (Fs_GetAttrResultsParam *) storagePtr->requestParamPtr;
    fileIDPtr = &(getAttrResultsParamPtr->attrResults.fileID);
    attrPtr = &(getAttrResultsParamPtr->attrResults.attrs);

    hdrPtr = VerifyIOHandle(fileIDPtr);
    if (hdrPtr != (Fs_HandleHeader *) NIL) {
	/*
	 * If someone has the I/O device open we'll have a handle and
	 * should go get the access and modify times.
	 */
	fs_Stats.srvName.getIOAttrs++;
	Fsutil_HandleUnlock(hdrPtr);
	status = (*fsio_StreamOpTable[hdrPtr->fileID.type].getIOAttr)
		(&hdrPtr->fileID, clientID, attrPtr);
#ifdef lint
	status = Fsio_DeviceGetIOAttr(&hdrPtr->fileID, rpc_SpriteID, attrPtr);
	status = Fsio_PipeGetIOAttr(&hdrPtr->fileID, rpc_SpriteID, attrPtr);
	status = FspdevControlGetIOAttr(&hdrPtr->fileID, rpc_SpriteID,attrPtr);
#endif lint
	Fsutil_HandleRelease(hdrPtr, FALSE);
    } else {
	/*
	 * No information to add. We just return what was passed to us.
	 */
	status = SUCCESS;
    }
    if (status == SUCCESS) {
	storagePtr->replyParamPtr = (Address) attrPtr;
	storagePtr->replyParamSize = sizeof(Fs_Attributes);
    }
    /*
     * No clean-up call-back needed because we are returning the attributes
     * that are sitting in the request buffer.
     */
    Rpc_Reply(srvToken, status, storagePtr, (int (*)())NIL, (ClientData)NIL);
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsrmt_SetIOAttr --
 *
 *	Set the attributes cached at a remote I/O server.  This makes the
 *	RPC_FS_SET_IO_ATTR RPC to the I/O server which then calls
 *	stream-type setIOAttrs routine.
 *
 * Results:
 *	An error code.
 *
 * Side effects:
 *	Set attributes cached with the I/O server.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
Fsrmt_SetIOAttr(fileIDPtr, attrPtr, flags)
    register Fs_FileID		*fileIDPtr;	/* Remote device/pipe fileID */
    register Fs_Attributes	*attrPtr;	/* Attributes to copy */
    int				flags;		/* What attributes to set */
{
    register ReturnStatus	status;
    Rpc_Storage			storage;
    FsRemoteSetAttrParams	setAttrParam;

    setAttrParam.fileID = *fileIDPtr;
    setAttrParam.attrs = *attrPtr;
    setAttrParam.flags = flags;
    /*
     * We have to fix up the fileID of control handles in the case that
     * the pseudo-device server is remote from us.  The problem is that
     * we have to RPC to the pseduo-device server, but the fileID of the
     * control stream (where the access/modify times are) has the
     * file serverID.  Fortuneatly this is in the regular attributes so
     * we can patch it up.  We add two lines of code here instead of
     * nearly duplicating this procedure into a FsRmtControlSetIOAttr
     * routine that has the patch.
     */
    if (fileIDPtr->type == FSIO_RMT_CONTROL_STREAM) {
	setAttrParam.fileID.serverID = attrPtr->serverID;
    }
    storage.requestParamPtr = (Address) &setAttrParam;
    storage.requestParamSize = sizeof(FsRemoteSetAttrParams);
    storage.requestDataPtr = (Address) NIL;
    storage.requestDataSize = 0;

    storage.replyParamPtr = (Address) NIL;
    storage.replyParamSize = 0;
    storage.replyDataPtr = (Address) NIL;
    storage.replyDataSize = 0;

    status = Rpc_Call(fileIDPtr->serverID, RPC_FS_SET_IO_ATTR, &storage);
    /*
     * We punt on I/O server recovery, and mask errors so a chmod() works.
     */
    if (status != SUCCESS) {
	printf(
	    "Fsrmt_SetIOAttr failed <%x>: device <%d,%d> at server %d\n",
	    status, fileIDPtr->major, fileIDPtr->minor, fileIDPtr->serverID);
	status = SUCCESS;
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsrmt_RpcSetIOAttr --
 *
 *	Service stub for the RPC_FS_SET_IO_ATTR call.  This calls
 *	the stream-type routine to set attributes cached at the I/O server.
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
Fsrmt_RpcSetIOAttr(srvToken, clientID, command, storagePtr)
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
    register ReturnStatus	status;
    register Fs_HandleHeader	*hdrPtr;
    register Fs_FileID		*fileIDPtr;
    Fs_Attributes		*attrPtr;
    FsRemoteSetAttrParams	*setAttrParamPtr;

    setAttrParamPtr =
	    (FsRemoteSetAttrParams *) storagePtr->requestParamPtr;
    fileIDPtr = &(setAttrParamPtr->fileID);
    attrPtr = &(setAttrParamPtr->attrs);

   fs_Stats.srvName.setIOAttrs++;
   hdrPtr = VerifyIOHandle(fileIDPtr);
    if (hdrPtr == (Fs_HandleHeader *) NIL) {
	/*
	 * Noone has the I/O device open so we don't have a handle.
	 * Return SUCCESS but take no action.
	 */
	return(SUCCESS);
    } 
    Fsutil_HandleUnlock(hdrPtr);

    status = (*fsio_StreamOpTable[hdrPtr->fileID.type].setIOAttr)
	    (&hdrPtr->fileID, attrPtr, setAttrParamPtr->flags);
#ifdef lint
    status = Fsio_DeviceSetIOAttr(&hdrPtr->fileID, attrPtr,setAttrParamPtr->flags);
    status = Fsio_PipeSetIOAttr(&hdrPtr->fileID, attrPtr, setAttrParamPtr->flags);
    status = FspdevControlSetIOAttr(&hdrPtr->fileID, attrPtr,
					setAttrParamPtr->flags);
#endif lint

    Fsutil_HandleRelease(hdrPtr, FALSE);

    /*
     * No clean-up call-back needed because there are no reply parameters.
     */
    Rpc_Reply(srvToken, status, storagePtr, (int (*)())NIL, (ClientData)NIL);
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * VerifyIOHandle --
 *
 *	Verify an I/O handle from a remote client.  This is only used when
 *	getting/setting attributes.  We do not require that the client be
 *	listed for the I/O handle because in two important cases it won't be.
 *	The first is if we are the I/O server for a device, and someone
 *	does a Fs_GetAttributes(pathname, ...).  We are contacted for the
 *	latest I/O attributes, and the client won't have opened the device.
 *	The second case is if we are the name server and someone is doing
 *	a Fs_GetAttributesID(streamID, ...).  Here the I/O server has the
 *	client in its list, but we (the name server) do not.
 *
 * Results:
 *	A locked pointer to the I/O handle if it exists.
 *
 * Side effects:
 *	The returned handle must be released because we fetch it here.
 *
 *----------------------------------------------------------------------
 */
static Fs_HandleHeader *
VerifyIOHandle(fileIDPtr)
    Fs_FileID *fileIDPtr;
{
    if (fileIDPtr->type <= 0 || fileIDPtr->type >= FSIO_NUM_STREAM_TYPES) {
	printf(
		"Bad stream type (%d) in VerifyIOHandle.\n",
		fileIDPtr->type);
	return((Fs_HandleHeader *)NIL);
    } else {
	fileIDPtr->type = fsio_RmtToLclType[fileIDPtr->type];
    }
    return(Fsutil_HandleFetch(fileIDPtr));
}
