/*
 * fsAttributes.c --
 *
 *	This has procedures for operations done on file attributes.
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


#include "sprite.h"
#include "fs.h"
#include "fsInt.h"
#include "fsOpTable.h"
#include "fsLocalDomain.h"
#include "fsNameOps.h"
#include "fsConsist.h"
#include "fsRecovery.h"
#include "fsDisk.h"
#include "fsStat.h"
#include "mem.h"
#include "rpc.h"

FsHandleHeader *VerifyIOHandle();


/*
 *----------------------------------------------------------------------
 *
 * Fs_GetAttrStream --
 *
 *	Get the attributes of an open file.  The name server for the
 *	file (if any, anonymous pipes won't have one) is contacted
 *	to get the initial version of the attributes.  This includes
 *	ownership, permissions, and a guess as to the size.  Then
 *	a stream-specific call is made to update the attributes
 *	from info at the I/O server.  For example, there may be
 *	more up-to-date access and modify times at the I/O server.
 *
 * Results:
 *	An error code and the attibutes.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Fs_GetAttrStream(streamPtr, attrPtr)
    Fs_Stream *streamPtr;
    Fs_Attributes *attrPtr;
{
    register ReturnStatus	status;
    register FsHandleHeader	*hdrPtr = streamPtr->ioHandlePtr;
    register FsNameInfo		*nameInfoPtr = streamPtr->nameInfoPtr;

    if (!FsHandleValid(hdrPtr)) {
	status = FS_STALE_HANDLE;
    } else {
	if (nameInfoPtr == (FsNameInfo *)NIL) {
	    /*
	     * Anonymous pipes have no name info.
	     */
	    Byte_Zero(sizeof(Fs_Attributes), (Address)attrPtr);
	    status = SUCCESS;
	} else {
	    /*
	     * Get the initial version of the attributes from the file server
	     * that has the name of the file.
	     */
	    status = (*fsAttrOpTable[nameInfoPtr->domainType].getAttr)
			(&nameInfoPtr->fileID, rpc_SpriteID, attrPtr);
#ifdef lint
	    status = FsLocalGetAttr(&nameInfoPtr->fileID, rpc_SpriteID,attrPtr);
	    status = FsSpriteGetAttr(&nameInfoPtr->fileID,rpc_SpriteID,attrPtr);
#endif lint
	    if (status != SUCCESS) {
		Sys_Panic(SYS_WARNING,
		    "Fs_GetAttrStream: can't get name attributes <%x> for %s\n",
		    status,
		    (nameInfoPtr->name == (char *)NIL) ? "(no name)" :
							 nameInfoPtr->name);
	    }
	}
	if (status == SUCCESS) {
	    /*
	     * Update the attributes by contacting the I/O server.
	     */
	    status = (*fsStreamOpTable[hdrPtr->fileID.type].getIOAttr)
			(&hdrPtr->fileID, rpc_SpriteID, attrPtr);
#ifdef lint
	    status = FsRemoteGetIOAttr(&hdrPtr->fileID, rpc_SpriteID, attrPtr);
	    status = FsRmtFileGetIOAttr(&hdrPtr->fileID, rpc_SpriteID, attrPtr);
	    status = FsDeviceGetIOAttr(&hdrPtr->fileID, rpc_SpriteID, attrPtr);
	    status = FsPipeGetIOAttr(&hdrPtr->fileID, rpc_SpriteID, attrPtr);
#endif lint
	}
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsLocalGetAttr --
 *
 *	Get the attributes of a local file given its fileID.  This is called
 *	from Fs_GetAttrStream to get the attributes from the file descriptor.
 *	Also, as a special case, files that are cached remotely have their
 *	attributes updated here (on the server) by doing a call-back to
 *	clients to get the most recent access time, modify time, and size.
 *
 * Results:
 *	An error code.
 *
 * Side effects:
 *	Fills in the attributes structure with info from the disk descriptor.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
FsLocalGetAttr(fileIDPtr, clientID, attrPtr)
    register FsFileID		*fileIDPtr;	/* Identfies file */
    int				clientID;	/* Host ID of process asking
						 * for the attributes */
    register Fs_Attributes	*attrPtr;	/* Return - the attributes */
{
    if (fileIDPtr->type != FS_LCL_FILE_STREAM) {
	Sys_Panic(SYS_FATAL, "FsLocalGetAttr, bad fileID type <%d>\n",
	    fileIDPtr->type);
	return(GEN_INVALID_ARG);
    } else {
	FsLocalFileIOHandle *handlePtr;
	Boolean isExeced;
	ReturnStatus status;

	handlePtr = FsHandleFetchType(FsLocalFileIOHandle, fileIDPtr);
	if (handlePtr == (FsLocalFileIOHandle *)NIL) {
	    status = FsLocalFileHandleInit(fileIDPtr, &handlePtr);
	    if (status != SUCCESS) {
		Byte_Zero(sizeof(Fs_Attributes), (Address)attrPtr);
		return(status);
	    }
	}
	/*
	 * Call-back to clients to get cached attributes, then copy
	 * attributes from the file descriptor to the attributes struct.
	 * NOTE: this only gets cached attributes for regular files.
	 * Device servers may cache attributes too, but that is handled
	 * on the client, not here on the name server.  Why?  Because
	 * generic devices crossed with migration lead to cases where
	 * we, the name server, don't know what's happening on the client.
	 */
	FsGetClientAttrs(handlePtr, clientID, &isExeced);
	FsAssignAttrs(handlePtr, isExeced, attrPtr);
	FsHandleRelease(handlePtr, TRUE);
	return(SUCCESS);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * FsAssignAttrs --
 *
 *	Fill in the Fs_Attributes structure for a regular file.
 *	If the isExeced flag is TRUE then the current time is used as the
 *	access time.  This is an optimization to avoid contacting every
 *	client using the file.  Furthermore, due to segment caching by
 *	VM, we have no accurate access time on an executable anyway.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Attribute structure set to contain attributes from disk descriptor
 *	and the cache information.
 *
 *----------------------------------------------------------------------
 */
void
FsAssignAttrs(handlePtr, isExeced, attrPtr)
    register	FsLocalFileIOHandle	*handlePtr;
    Boolean				isExeced;  /* TRUE if being executed,
						    * use current time for
						    * the access time. */
    register	Fs_Attributes		*attrPtr;
{
    register FsFileDescriptor *descPtr = handlePtr->descPtr;
    register FsCacheFileInfo *cacheInfoPtr = &handlePtr->cacheInfo;

    attrPtr->serverID			= handlePtr->hdr.fileID.serverID;
    attrPtr->domain			= handlePtr->hdr.fileID.major;
    attrPtr->fileNumber			= handlePtr->hdr.fileID.minor;
    attrPtr->type			= descPtr->fileType;
    attrPtr->permissions		= descPtr->permissions;
    attrPtr->numLinks			= descPtr->numLinks;
    attrPtr->uid			= descPtr->uid;
    attrPtr->gid			= descPtr->gid;
    attrPtr->userType			= descPtr->userType;
    attrPtr->devServerID		= descPtr->devServerID;
    attrPtr->devType			= descPtr->devType;
    attrPtr->devUnit			= descPtr->devUnit;
    /*
     * Take creation and descriptor modify time from disk descriptor.
     */
    attrPtr->createTime.seconds		= descPtr->createTime;
    attrPtr->createTime.microseconds	= 0;
    attrPtr->descModifyTime.seconds	= descPtr->descModifyTime;
    attrPtr->descModifyTime.microseconds= 0;
    /*
     * Take size, access time, and modify time from cache info because
     * remote caching means the disk descriptor attributes can be out-of-date.
     */
    attrPtr->size			= cacheInfoPtr->attr.lastByte + 1;
    if (cacheInfoPtr->attr.firstByte >= 0) {
	attrPtr->size			-= cacheInfoPtr->attr.firstByte;
    }
    attrPtr->dataModifyTime.seconds	= cacheInfoPtr->attr.modifyTime;
    attrPtr->dataModifyTime.microseconds= 0;
    if (isExeced) {
	attrPtr->accessTime.seconds	= fsTimeInSeconds;
    } else {
	attrPtr->accessTime.seconds	= cacheInfoPtr->attr.accessTime;
    }
    attrPtr->accessTime.microseconds	= 0;
    /*
     * Again, if delayed writes mean we don't have any blocks for the
     * file then we estimate a block count from the cache size.  This
     * can be wrong due to granularity errors and wholes in files.
     * Also, even if the descriptor has some blocks it may not have them all.
     */
    if (cacheInfoPtr->attr.lastByte > 0 && descPtr->numKbytes == 0) {
	attrPtr->blocks			= cacheInfoPtr->attr.lastByte / 1024 +1;
    } else {
	attrPtr->blocks			= descPtr->numKbytes;
    }
    attrPtr->blockSize			= FS_BLOCK_SIZE;
    attrPtr->version			= descPtr->version;
    attrPtr->userType			= descPtr->userType;
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_SetAttrStream --
 *
 *	Set the attributes of an open file.  First the name server is
 *	contacted to verify permissions and to update the file descriptor.
 *	Then the I/O server is contacted to update any cached attributes.
 *
 * Results:
 *	An error code.
 *
 * Side effects:
 *	The modify and access times are set.
 *	The owner and group ID are set.
 *	The permission bits are set.
 * 	If the operation is successful, the count of setAttrs is incremented.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Fs_SetAttrStream(streamPtr, attrPtr, idPtr, flags)
    Fs_Stream *streamPtr;	/* References file to manipulate. */
    Fs_Attributes *attrPtr;	/* Attributes to give to the file. */
    FsUserIDs *idPtr;		/* Owner and groups of calling process */
    int flags;			/* Specify what attributes to set. */
{
    register ReturnStatus	status;
    register FsHandleHeader	*hdrPtr = streamPtr->ioHandlePtr;
    register FsNameInfo		*nameInfoPtr = streamPtr->nameInfoPtr;

    if (!FsHandleValid(hdrPtr)) {
	status = FS_STALE_HANDLE;
    } else {
	if (streamPtr->nameInfoPtr != (FsNameInfo *)NIL) {
	    /*
	     * Set the attributes at the name server.
	     */
	    status = (*fsAttrOpTable[nameInfoPtr->domainType].setAttr)
			(&nameInfoPtr->fileID, attrPtr, idPtr, flags);
#ifdef lint
	    status = FsLocalSetAttr(&nameInfoPtr->fileID, attrPtr,idPtr,flags);
	    status = FsSpriteSetAttr(&nameInfoPtr->fileID, attrPtr,idPtr,flags);
#endif lint
	} else {
	    status = SUCCESS;
	}
	if (status == SUCCESS) {
	    /*
	     * Set the attributes at the I/O server.
	     */
	    status = (*fsStreamOpTable[hdrPtr->fileID.type].setIOAttr)
			(&hdrPtr->fileID, attrPtr, flags);
#ifdef lint
	    status = FsRemoteSetIOAttr(&hdrPtr->fileID, attrPtr, flags);
	    status = FsRmtFileSetIOAttr(&hdrPtr->fileID, attrPtr, flags);
	    status = FsDeviceSetIOAttr(&hdrPtr->fileID, attrPtr, flags);
	    status = FsPipeSetIOAttr(&hdrPtr->fileID, attrPtr, flags);
#endif lint
	}
	if (status == SUCCESS) {
	    fsStats.gen.numSetAttrs ++;
	}
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsLocalSetAttr --
 *
 *	Set the attributes of a local file given its fileID.  This is
 *	called from Fs_SetAttrStream to set the attributes at the name server.
 *	This updates the disk descriptor and copies the new information
 * 	into the cache.  It will go to disk on the next sync.
 *
 * Results:
 *	An error code.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
FsLocalSetAttr(fileIDPtr, attrPtr, idPtr, flags)
    FsFileID			*fileIDPtr;	/* Target file. */
    register Fs_Attributes	*attrPtr;	/* New attributes */
    register FsUserIDs		*idPtr;		/* Process's owner/group */
    int				flags;		/* What attrs to set */
{
    register ReturnStatus	status = SUCCESS;
    FsLocalFileIOHandle		*handlePtr;
    register FsFileDescriptor	*descPtr;
    FsDomain			*domainPtr;

    handlePtr = FsHandleFetchType(FsLocalFileIOHandle, fileIDPtr);
    if (handlePtr == (FsLocalFileIOHandle *)NIL) {
	Sys_Panic(SYS_WARNING, "FsLocalSetAttr, no handle <%d,%d,%d,%d>\n",
		    fileIDPtr->type, fileIDPtr->serverID,
		    fileIDPtr->major, fileIDPtr->minor);
	return(FS_FILE_NOT_FOUND);
    }
    descPtr = handlePtr->descPtr;
    if (descPtr == (FsFileDescriptor *)NIL) {
	Sys_Panic(SYS_WARNING, "FsLocalSetAttr, NIL descPtr\n");
	status = FAILURE;
	goto exit;
    }
    if (!FsOwner(idPtr, descPtr)) {
	status = FS_NOT_OWNER;
	goto exit;
    }
    if (flags & FS_SET_OWNER) {
	if (attrPtr->uid >= 0 && descPtr->uid != attrPtr->uid) {
	    if (idPtr->user != 0) {
		/*
		 * Don't let ordinary people give away ownership.
		 */
		status = FS_NO_ACCESS;
#ifdef not_yet
		goto exit;
#endif
	    } else {
		descPtr->uid = attrPtr->uid;
	    }
	}
	if (attrPtr->gid >= 0 && descPtr->gid != attrPtr->gid) {
	    register int g;
	    for (g=0 ; g < idPtr->numGroupIDs; g++) {
		if (attrPtr->gid == idPtr->group[g] || idPtr->user == 0) {
		    descPtr->gid = attrPtr->gid;
		    break;
		}
	    }
	    if (g >= idPtr->numGroupIDs) {
		status = FS_NO_ACCESS;
#ifdef not_yet
		goto exit;
#endif
	    }
	}
    }
    if (flags & FS_SET_MODE) {
	descPtr->permissions = attrPtr->permissions;
    }
    if (flags & FS_SET_DEVICE) {
	if (descPtr->fileType == FS_DEVICE ||
		  descPtr->fileType == FS_REMOTE_DEVICE) {
	      descPtr->devServerID = attrPtr->devServerID;
	      descPtr->devType = attrPtr->devType;
	      descPtr->devUnit = attrPtr->devUnit;
	}
    }
    if (flags & FS_SET_TIMES) {
	descPtr->accessTime       = attrPtr->accessTime.seconds;
	descPtr->dataModifyTime   = attrPtr->dataModifyTime.seconds;
	/*
	 * Patch this because it gets copied by FsUpdateCachedAttr.
	 */
	attrPtr->createTime.seconds = descPtr->createTime;
    }

    if (flags & FS_SET_FILE_TYPE) {
	descPtr->userType    = attrPtr->userType;
    }

    /*
     * Copy this new information into the cache block containing the descriptor.
     */
    descPtr->descModifyTime   = fsTimeInSeconds;
    domainPtr = FsDomainFetch(handlePtr->hdr.fileID.major, FALSE);
    if (domainPtr == (FsDomain *)NIL) {
	status = FS_DOMAIN_UNAVAILABLE;
    } else {
	FsStoreFileDesc(domainPtr, handlePtr->hdr.fileID.minor, descPtr);
	FsDomainRelease(handlePtr->hdr.fileID.major);
    }
    /*
     * Update the attributes cached in the file handle.
     */
    FsUpdateCachedAttr(&handlePtr->cacheInfo, attrPtr, flags);
exit:
    FsHandleRelease(handlePtr, TRUE);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsOwner --
 *
 *	Determine if the process owns the file.
 *
 * Results:
 *	TRUE if the owner ID or group owner ID of the process
 *	match with that of the file.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
Boolean
FsOwner(idPtr, descPtr)
    FsUserIDs *idPtr;
    FsFileDescriptor *descPtr;
{
    register int index;

    if (idPtr->user == 0) {
	return(TRUE);
    }
    if (idPtr->user == descPtr->uid) {
	return(TRUE);
    }
    for (index=0 ; index < idPtr->numGroupIDs ; index++) {
	if (idPtr->group[index] == descPtr->gid) {
	    return(TRUE);
	}
    }
    return(FALSE);
}

/*
 * Return parameters from RPC_FS_GET_ATTR_PATH.  The file ID is used to
 * get to the I/O server.
 */
typedef struct FsSpriteGetAttrResults {
    Fs_Attributes	attributes;
    FsFileID		fileID;
} FsSpriteGetAttrResults;


/*
 *----------------------------------------------------------------------
 *
 * FsSpriteGetAttrPath --
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
FsSpriteGetAttrPath(prefixHandle, relativeName, argsPtr, resultsPtr,
         	    newNameInfoPtrPtr)
    FsHandleHeader *prefixHandle;	/* Handle from the prefix table */
    char           *relativeName;	/* The name of the file. */
    Address        argsPtr;		/* Bundled arguments for us */
    Address        resultsPtr;		/* Where to store attributes */
    FsRedirectInfo **newNameInfoPtrPtr;	/* We return this if the server leaves 
					 * its domain during the lookup. */
{
    ReturnStatus 		status;
    FsOpenArgs			*openArgsPtr;
    FsGetAttrResults		*getAttrResultsPtr;
    FsGetAttrResultsParam	getAttrResultsParam;
    Rpc_Storage			storage;
    char			replyName[FS_MAX_PATH_NAME_LENGTH];	 /* This
						     * may get filled with a
						     * redirected pathname. */

    openArgsPtr = (FsOpenArgs *) argsPtr;
    getAttrResultsPtr = (FsGetAttrResults *)resultsPtr;

    storage.requestParamPtr = (Address) openArgsPtr;
    storage.requestParamSize = sizeof(FsOpenArgs);
    storage.requestDataPtr = (Address) relativeName;
    storage.requestDataSize = String_Length(relativeName) + 1;
    storage.replyParamPtr = (Address) &(getAttrResultsParam);
    storage.replyParamSize = sizeof(FsGetAttrResultsParam);
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
	 * Allocate enough space to fit the prefix length and the file name and
	 * copy over the structure that we have on our stack.
	 */
	register int	redirectSize;
	redirectSize = sizeof(int) + String_Length(replyName) + 1;
	*newNameInfoPtrPtr = (FsRedirectInfo *) Mem_Alloc(redirectSize);
	(*newNameInfoPtrPtr)->prefixLength = getAttrResultsParam.prefixLength;
	String_Copy(replyName, (*newNameInfoPtrPtr)->fileName);
	return(FS_LOOKUP_REDIRECT);
    }

    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_RpcGetAttrPath --
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
Fs_RpcGetAttrPath(srvToken, clientID, command, storagePtr)
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
    register FsOpenArgs		*openArgsPtr;	/* Tmp pointer into openParams*/
    FsGetAttrResults 		getAttrResults;	/* Results from local  routine */
    FsGetAttrResultsParam	*getAttrResultsParamPtr;	/* rpc param
								 * bundle */
    FsHandleHeader		*prefixHandle;	/* Handle for domain */
    ReturnStatus		status;		/* General return code */
    FsRedirectInfo		*newNameInfoPtr;/* For prefix re-directs,
						 * unallocated since proc call
						 * allocates space for it. */

    openArgsPtr = (FsOpenArgs *) storagePtr->requestParamPtr;

    if (openArgsPtr->prefixID.serverID != rpc_SpriteID) {
	/*
	 * Filesystem mis-match.
	 */
	return(GEN_INVALID_ARG);
    }
    prefixHandle = (*fsStreamOpTable[openArgsPtr->prefixID.type].clientVerify)
	    (&openArgsPtr->prefixID, clientID);
    if (prefixHandle == (FsHandleHeader *)NIL) {
	return(FS_STALE_HANDLE);
    }
    FsHandleRelease(prefixHandle, TRUE);

    newNameInfoPtr = (FsRedirectInfo *) NIL;
    getAttrResultsParamPtr = Mem_New(FsGetAttrResultsParam);

    getAttrResults.attrPtr = &(getAttrResultsParamPtr->attrResults.attrs);
    getAttrResults.fileIDPtr = &(getAttrResultsParamPtr->attrResults.fileID);

    status = FsLocalGetAttrPath(prefixHandle,
		(char *) storagePtr->requestDataPtr, (Address)openArgsPtr,
		(Address)(&getAttrResults), &newNameInfoPtr);

    if (status == SUCCESS) {
	storagePtr->replyParamPtr = (Address) getAttrResultsParamPtr;
	storagePtr->replyParamSize = sizeof(FsGetAttrResultsParam);
	storagePtr->replyDataPtr = (Address) NIL;
	storagePtr->replyDataSize = 0;
    } else if (status == FS_LOOKUP_REDIRECT) {
	/*
	 * The file is not found on this server, but somewhere else.
	 */
	getAttrResultsParamPtr->prefixLength = newNameInfoPtr->prefixLength;
	storagePtr->replyParamPtr = (Address) getAttrResultsParamPtr;
	storagePtr->replyParamSize = sizeof(FsGetAttrResultsParam);
	storagePtr->replyDataSize = String_Length(newNameInfoPtr->fileName) + 1;
	storagePtr->replyDataPtr =
		(Address) Mem_Alloc(storagePtr->replyDataSize);
	String_Copy(newNameInfoPtr->fileName, (char *) storagePtr->replyDataPtr);
	Mem_Free(newNameInfoPtr);
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
	Mem_Free((Address) getAttrResultsParamPtr);
        return(status);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * FsSpriteSetAttrPath --
 *
 *	Set the attributes of a remote Sprite file given its path name.
 *	This is called from Fs_SetAttributes.  This used the
 *	RPC_FS_SET_ATTR_PATH rpc to invoke FsLocalSetAttrPath on the
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
FsSpriteSetAttrPath(prefixHandle, relativeName, argsPtr, resultsPtr,
         	    newNameInfoPtrPtr)
    FsHandleHeader *prefixHandle;   /* Token from the prefix table */
    char           *relativeName;   /* The name of the file. */
    Address        argsPtr;	    /* Bundled arguments for us */
    Address        resultsPtr;	    /* FileID */
    FsRedirectInfo **newNameInfoPtrPtr; /* We return this if the server leaves 
					 * its domain during the lookup. */
{
    ReturnStatus 		status;
    Rpc_Storage			storage;
    char			replyName[FS_MAX_PATH_NAME_LENGTH];
    FsGetAttrResultsParam	getAttrResultsParam;
    FsFileID			*fileIDPtr;

    storage.requestParamPtr = (Address) argsPtr;
    storage.requestParamSize = sizeof(FsSetAttrArgs);
    storage.requestDataPtr = (Address) relativeName;
    storage.requestDataSize = String_Length(relativeName) + 1;
    storage.replyParamPtr = (Address) &(getAttrResultsParam);
    storage.replyParamSize = sizeof(FsGetAttrResultsParam);
    storage.replyDataPtr = (Address) replyName;
    storage.replyDataSize = FS_MAX_PATH_NAME_LENGTH;

    fileIDPtr = (FsFileID *) resultsPtr;

    status = Rpc_Call(prefixHandle->fileID.serverID, RPC_FS_SET_ATTR_PATH,
			&storage);
    if (status == SUCCESS) {
	/*
	 * Copy result fileID to resultsPtr (== fileIDPtr).
	 */
	(* fileIDPtr) = getAttrResultsParam.attrResults.fileID;
    } else if (status == FS_LOOKUP_REDIRECT) {
	/*
	 * Allocate enough space to fit the prefix length and the file name and
	 * copy over the info.
	 */
	register int redirectSize;
	redirectSize = sizeof(int) + String_Length(replyName) + 1;
	*newNameInfoPtrPtr = (FsRedirectInfo *) Mem_Alloc(redirectSize);
	(*newNameInfoPtrPtr)->prefixLength = getAttrResultsParam.prefixLength;
	String_Copy(replyName, (*newNameInfoPtrPtr)->fileName);
    }

    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_RpcSetAttrPath --
 *
 *	Service stub for the RPC_FS_SET_ATTR_PATH call.  This branches
 *	to FsLocalSetAttrPath which sets the attributes on the file descriptor.
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
Fs_RpcSetAttrPath(srvToken, clientID, command, storagePtr)
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
    FsFileID			*ioFileIDPtr;	/* Results from local routine */
    FsHandleHeader		*prefixHandle;	/* Handle for domain */
    ReturnStatus		status;		/* General return code */
    FsSetAttrArgs		*setAttrArgsPtr;
    FsRedirectInfo		*newNameInfoPtr;/* For prefix re-directs */
    FsGetAttrResultsParam	*getAttrResultsParamPtr;	/* rpc param
								 * bundle */

    setAttrArgsPtr = (FsSetAttrArgs *) storagePtr->requestParamPtr;

    prefixHandle =
	(*fsStreamOpTable[setAttrArgsPtr->openArgs.prefixID.type].clientVerify)
	    (&setAttrArgsPtr->openArgs.prefixID, clientID);
    if (prefixHandle == (FsHandleHeader *)NIL) {
	return(FS_STALE_HANDLE);
    }
    FsHandleRelease(prefixHandle, TRUE);

    newNameInfoPtr = (FsRedirectInfo *) NIL;
    getAttrResultsParamPtr = Mem_New(FsGetAttrResultsParam);
    ioFileIDPtr = &(getAttrResultsParamPtr->attrResults.fileID);
    status = FsLocalSetAttrPath(prefixHandle,
		(char *) storagePtr->requestDataPtr,
		(Address) storagePtr->requestParamPtr,
		(Address) ioFileIDPtr, &newNameInfoPtr);

    if (status == SUCCESS) {
	storagePtr->replyParamPtr = (Address) getAttrResultsParamPtr;
	storagePtr->replyParamSize = sizeof(FsGetAttrResultsParam);
	storagePtr->replyDataPtr = (Address) NIL;
	storagePtr->replyDataSize = 0;
    } else if (status == FS_LOOKUP_REDIRECT) {
	/*
	 * The file is not found on this server.
	 */
	getAttrResultsParamPtr->prefixLength = newNameInfoPtr->prefixLength;
	storagePtr->replyParamPtr = (Address) getAttrResultsParamPtr;
	storagePtr->replyParamSize = sizeof(FsGetAttrResultsParam);
	storagePtr->replyDataSize = String_Length(newNameInfoPtr->fileName) + 1;
	storagePtr->replyDataPtr =
		(Address) Mem_Alloc(storagePtr->replyDataSize);
	String_Copy(newNameInfoPtr->fileName,
		(char *) storagePtr->replyDataPtr);
	Mem_Free(newNameInfoPtr);
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
	Mem_Free(getAttrResultsParamPtr);
        return(status);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * FsSpriteGetAttr --
 *
 *	Get the attributes of a remote file given its fileID.  This is called
 *	from Fs_GetAttrStream.  This does an RPC to the name server which
 *	then calls FsLocalGetAttr.
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
FsSpriteGetAttr(fileIDPtr, clientID, attrPtr)
    register FsFileID		*fileIDPtr;	/* Identfies file */
    int				clientID;	/* IGNORED, implicitly passed
						 * by the RPC system. */
    register Fs_Attributes	*attrPtr;	/* Return - the attributes */
{
    register ReturnStatus	status;
    Rpc_Storage storage;

    if (fileIDPtr->type != FS_RMT_FILE_STREAM) {
	Sys_Panic(SYS_FATAL, "FsSpriteGetAttr, bad fileID type <%d>\n",
	    fileIDPtr->type);
	return(GEN_INVALID_ARG);
    }
    storage.requestParamPtr = (Address) fileIDPtr;
    storage.requestParamSize = sizeof(FsFileID);
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
	register FsHandleHeader *hdrPtr = FsHandleFetch(fileIDPtr);
	if (hdrPtr != (FsHandleHeader *)NIL) {
	    FsWantRecovery(hdrPtr);
	    FsHandleRelease(hdrPtr, TRUE);
	}
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_RpcGetAttr --
 *
 *	Service stub for the RPC_FS_GET_ATTR call.  This calls FsLocalGetAttr
 *	to get the attributes of a file from its disk descriptor.  This
 *	is a name server operation used when the file is already open.
 *	Note: Attributes are not complete until the I/O server has also
 *	been contacted.  See Fs_RpcGetIOAttr.
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
Fs_RpcGetAttr(srvToken, clientID, command, storagePtr)
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
    FsHandleHeader		*tHdrPtr;
    register FsHandleHeader	*hdrPtr;
    register FsFileID		*fileIDPtr;
    register Fs_Attributes	*attrPtr;
    Rpc_ReplyMem		*replyMemPtr;

    fileIDPtr = (FsFileID *) storagePtr->requestParamPtr;
    hdrPtr = VerifyIOHandle(fileIDPtr);
    if (hdrPtr == (FsHandleHeader *)NIL) {
	status = FsLocalFileHandleInit(fileIDPtr,
			(FsLocalFileIOHandle **)&tHdrPtr);
	if (status != SUCCESS) {
	    return(status);
	}
	hdrPtr = tHdrPtr;
    }
    FsHandleUnlock(hdrPtr);

    attrPtr = Mem_New(Fs_Attributes);
    status = FsLocalGetAttr(&hdrPtr->fileID, clientID, attrPtr);
    FsHandleRelease(hdrPtr, FALSE);

    storagePtr->replyParamPtr = (Address) attrPtr;
    storagePtr->replyParamSize = sizeof(Fs_Attributes);
    replyMemPtr = (Rpc_ReplyMem *) Mem_Alloc(sizeof(Rpc_ReplyMem));
    replyMemPtr->paramPtr = storagePtr->replyParamPtr;
    replyMemPtr->dataPtr = (Address) NIL;

    Rpc_Reply(srvToken, status, storagePtr, Rpc_FreeMem,
	    (ClientData)replyMemPtr);
    return(SUCCESS);
}

/*
 * Parameters for RPC_FS_SET_ATTR.  This is a sub-set of FsSetAttrArgs
 * (which has extra stuff required to handle path name lookup).
 */

typedef struct FsSpriteSetAttrParams {
    FsFileID		fileID;
    FsUserIDs		ids;
    Fs_Attributes	attrs;
    int			flags;
} FsSpriteSetAttrParams;

/*
 *----------------------------------------------------------------------
 *
 * FsSpriteSetAttr
 *
 *	Set selected attributes of a remote file given its fileID.  This
 *	is called from Fs_SetAttrStream.  This does the RPC_FS_SET_ATTR
 *	remote call which invokes FsLocalSetAttr on the name server.
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
FsSpriteSetAttr(fileIDPtr, attrPtr, idPtr, flags)
    FsFileID		*fileIDPtr;
    Fs_Attributes	*attrPtr;
    FsUserIDs		 *idPtr;
    int			flags;
{
    ReturnStatus status;
    Rpc_Storage storage;
    FsSpriteSetAttrParams params;

    if (fileIDPtr->type != FS_RMT_FILE_STREAM) {
	Sys_Panic(SYS_FATAL, "FsSpriteSetAttr, bad fileID type <%d>\n",
	    fileIDPtr->type);
	return(GEN_INVALID_ARG);
    }

    params.fileID = *fileIDPtr;
    params.ids = *idPtr;
    params.attrs = *attrPtr;
    params.flags = flags;
    storage.requestParamPtr = (Address)&params;
    storage.requestParamSize = sizeof(FsSpriteSetAttrParams);
    storage.requestDataPtr = (Address)NIL;;
    storage.requestDataSize = 0;
    storage.replyParamPtr = (Address)NIL;
    storage.replyParamSize = 0;
    storage.replyDataPtr = (Address)NIL;
    storage.replyDataSize = 0;

    status = Rpc_Call(fileIDPtr->serverID, RPC_FS_SET_ATTR, &storage);
    if (status == RPC_TIMEOUT || status == FS_STALE_HANDLE ||
	status == RPC_SERVICE_DISABLED) {
	register FsHandleHeader *hdrPtr = FsHandleFetch(fileIDPtr);
	if (hdrPtr != (FsHandleHeader *)NIL) {
	    FsWantRecovery(hdrPtr);
	    FsHandleRelease(hdrPtr, TRUE);
	}
    }

    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_RpcSetAttr --
 *
 *	Service stub for RPC_FS_SET_ATTR.  This calls FsLocalSetAttr.
 *
 * Results:
 *	If this procedure returns SUCCESS then a reply has been sent to
 *	the client.  If the arguments are bad then FS_STALE_HANDLE is 
 *	returned and the main level sends back an error reply.
 *
 * Side effects:
 *	Uses FsLocalSetAttr to set attributes in the file descriptor.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
Fs_RpcSetAttr(srvToken, clientID, command, storagePtr)
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
    FsHandleHeader		*hdrPtr;
    register FsFileID		*fileIDPtr;
    register Fs_Attributes	*attrPtr;
    register ReturnStatus	status;
    FsSpriteSetAttrParams	*paramPtr;

    paramPtr = (FsSpriteSetAttrParams *) storagePtr->requestParamPtr;
    attrPtr = &paramPtr->attrs;
    fileIDPtr = &paramPtr->fileID;

    hdrPtr = VerifyIOHandle(fileIDPtr);
    if (hdrPtr == (FsHandleHeader *)NIL) {
	status = FsLocalFileHandleInit(fileIDPtr,
				       (FsLocalFileIOHandle **)&hdrPtr);
	if (status != SUCCESS) {
	    return(status);
	}
    }
    FsHandleUnlock(hdrPtr);
    status = FsLocalSetAttr(fileIDPtr, attrPtr, &paramPtr->ids,paramPtr->flags);
    FsHandleRelease(hdrPtr, FALSE);

    Rpc_Reply(srvToken, status, storagePtr, (int (*)())NIL,
		(ClientData)NIL);
    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * FsRemoteGetIOAttr --
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
FsRemoteGetIOAttr(fileIDPtr, clientID, attrPtr)
    register FsFileID		*fileIDPtr;	/* Remote device/pipe fileID */
    int				clientID;	/* IGNORED, implicitly passed
						 * by the RPC system. */
    register Fs_Attributes	*attrPtr;	/* In/Out - the attributes are
						 * updated by the I/O server */
{
    register ReturnStatus status;
    Rpc_Storage storage;
    FsGetAttrResultsParam	getAttrResultsParam;

    getAttrResultsParam.attrResults.fileID = *fileIDPtr;
    getAttrResultsParam.attrResults.attrs = *attrPtr;

    storage.requestParamPtr = (Address) &getAttrResultsParam;
    storage.requestParamSize = sizeof(FsGetAttrResultsParam);
    storage.requestDataPtr = (Address) NIL;
    storage.requestDataSize = 0;

    storage.replyParamPtr = (Address) attrPtr;
    storage.replyParamSize = sizeof(Fs_Attributes);
    storage.replyDataPtr = (Address) NIL;
    storage.replyDataSize = 0;

    status = Rpc_Call(fileIDPtr->serverID, RPC_FS_GET_IO_ATTR, &storage);
    /*
     * I/O Server recovery??	TODO
     */
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_RpcGetIOAttr --
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
Fs_RpcGetIOAttr(srvToken, clientID, command, storagePtr)
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
    register FsHandleHeader	*hdrPtr;
    register FsFileID		*fileIDPtr;
    Fs_Attributes		*attrPtr;
    FsGetAttrResultsParam	*getAttrResultsParamPtr;

    getAttrResultsParamPtr =
	    (FsGetAttrResultsParam *) storagePtr->requestParamPtr;
    fileIDPtr = &(getAttrResultsParamPtr->attrResults.fileID);
    attrPtr = &(getAttrResultsParamPtr->attrResults.attrs);

    hdrPtr = VerifyIOHandle(fileIDPtr);
    if (hdrPtr == (FsHandleHeader *) NIL) {
	return(FS_STALE_HANDLE);
    } 
    FsHandleUnlock(hdrPtr);

    status = (*fsStreamOpTable[hdrPtr->fileID.type].getIOAttr)
	    (&hdrPtr->fileID, clientID, attrPtr);
#ifdef lint
    status = FsDeviceGetIOAttr(&hdrPtr->fileID, rpc_SpriteID, attrPtr);
    status = FsPipeGetIOAttr(&hdrPtr->fileID, rpc_SpriteID, attrPtr);
#endif lint

    FsHandleRelease(hdrPtr, FALSE);

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
 * FsRemoteSetIOAttr --
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
FsRemoteSetIOAttr(fileIDPtr, attrPtr, flags)
    register FsFileID		*fileIDPtr;	/* Remote device/pipe fileID */
    register Fs_Attributes	*attrPtr;	/* Attributes to copy */
    int				flags;		/* What attributes to set */
{
    register ReturnStatus	status;
    Rpc_Storage			storage;
    FsSpriteSetAttrParams	setAttrParam;

    setAttrParam.fileID = *fileIDPtr;
    setAttrParam.attrs = *attrPtr;
    setAttrParam.flags = flags;
    storage.requestParamPtr = (Address) &setAttrParam;
    storage.requestParamSize = sizeof(FsSpriteSetAttrParams);
    storage.requestDataPtr = (Address) NIL;
    storage.requestDataSize = 0;

    storage.replyParamPtr = (Address) NIL;
    storage.replyParamSize = 0;
    storage.replyDataPtr = (Address) NIL;
    storage.replyDataSize = 0;

    status = Rpc_Call(fileIDPtr->serverID, RPC_FS_SET_IO_ATTR, &storage);
    /*
     * I/O Server recovery??	TODO
     */
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_RpcSetIOAttr --
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
Fs_RpcSetIOAttr(srvToken, clientID, command, storagePtr)
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
    register FsHandleHeader	*hdrPtr;
    register FsFileID		*fileIDPtr;
    Fs_Attributes		*attrPtr;
    FsSpriteSetAttrParams	*setAttrParamPtr;

    setAttrParamPtr =
	    (FsSpriteSetAttrParams *) storagePtr->requestParamPtr;
    fileIDPtr = &(setAttrParamPtr->fileID);
    attrPtr = &(setAttrParamPtr->attrs);

    hdrPtr = VerifyIOHandle(fileIDPtr);
    if (hdrPtr == (FsHandleHeader *) NIL) {
	return(FS_STALE_HANDLE);
    } 
    FsHandleUnlock(hdrPtr);

    status = (*fsStreamOpTable[hdrPtr->fileID.type].setIOAttr)
	    (&hdrPtr->fileID, attrPtr, setAttrParamPtr->flags);
#ifdef lint
    status = FsDeviceSetIOAttr(&hdrPtr->fileID, attrPtr, flags);
    status = FsPipeSetIOAttr(&hdrPtr->fileID, attrPtr, flags);
#endif lint

    FsHandleRelease(hdrPtr, FALSE);

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
FsHandleHeader *
VerifyIOHandle(fileIDPtr)
    FsFileID *fileIDPtr;
{
    switch(fileIDPtr->type) {
	case FS_LCL_FILE_STREAM:
	case FS_LCL_DEVICE_STREAM:
	    break;
	case FS_RMT_FILE_STREAM:
	    fileIDPtr->type = FS_LCL_FILE_STREAM;
	    break;
	case FS_RMT_DEVICE_STREAM:
	    fileIDPtr->type = FS_LCL_DEVICE_STREAM;
	    break;
        case FS_RMT_PSEUDO_STREAM:
	    fileIDPtr->type = FS_LCL_PSEUDO_STREAM;
	    break;
	case FS_RMT_PIPE_STREAM:
	    fileIDPtr->type = FS_LCL_PIPE_STREAM;
	    break;
	default:
	    Sys_Panic(SYS_WARNING,
		    "Unknown stream type (%d) in VerifyIOHandle.\n",
		    fileIDPtr->type);
	    return((FsHandleHeader *)NIL);
    }
    return(FsHandleFetch(fileIDPtr));
}
