/* 
 * fsLocalDomain.c --
 *
 *	Implementation of name-space operations in the local domain.
 *	The routines here are called via the prefix table.
 *	They use FsLocalLookup (in fsLocalLookup.c) to do the guts of
 *	recursive name lookup.
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
#include "fsLocalDomain.h"
#include "fsConsist.h"
#include "fsOpTable.h"
#include "fsTrace.h"
#include "fsDebug.h"
#include "rpc.h"
#include "vm.h"
#include "string.h"
#include "proc.h"
#include "time.h"

char *fsEmptyDirBlock;

/*
 *----------------------------------------------------------------------
 *
 * FsLocalDomainInit --
 *
 *	Do general initialization for the local domain.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Initializes the domain table and the image of a new directory.
 *	
 *
 *----------------------------------------------------------------------
 */
void
FsLocalDomainInit()
{
    register int index;
    register FsDirEntry *dirEntryPtr;

    for (index = 0; index < FS_MAX_LOCAL_DOMAINS; index++) {
        fsDomainTable[index] = (FsDomain *) NIL;
    }
    fsEmptyDirBlock = (char *)Mem_Alloc(FS_DIR_BLOCK_SIZE);
    dirEntryPtr = (FsDirEntry *)fsEmptyDirBlock;
    dirEntryPtr->fileNumber = FS_ROOT_FILE_NUMBER;
    dirEntryPtr->nameLength = String_Length(".");
    dirEntryPtr->recordLength = FsDirRecLength(dirEntryPtr->nameLength);
    (void)String_Copy(".", dirEntryPtr->fileName);
    dirEntryPtr = (FsDirEntry *)((int)dirEntryPtr + dirEntryPtr->recordLength);
    dirEntryPtr->fileNumber = FS_ROOT_FILE_NUMBER;
    dirEntryPtr->nameLength = String_Length("..");
    dirEntryPtr->recordLength = FS_DIR_BLOCK_SIZE - FsDirRecLength(1);
    (void)String_Copy("..", dirEntryPtr->fileName);
}

/*
 *----------------------------------------------------------------------
 *
 * FsLocalPrefix --
 *
 *	Establish a handle for a local prefix.  Currently this is done
 *	by FsAttachDisk, not by this routine.
 *
 * Results:
 *	FAILURE
 *
 * Side effects:
 *	None.
 *	
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
FsLocalPrefix(token, relativeName, argsPtr, resultsPtr, newNameInfoPtrPtr)
    ClientData token;	/* == NIL for prefix installation */
    char *relativeName;	/* The name of the file to open (eventually this
			 * will be relative to the domain root) */
    Address argsPtr;	/* Bundled arguments for us */
    Address resultsPtr;	/* == NIL */
    FsRedirectInfo **newNameInfoPtrPtr; /* We return this if the server 
					 * leaves its domain during the 
					 * lookup. */
{
    /*
     * Now all local prefixes are installed with a valid handle so this
     * routine doesn't try to look for local domains.
     */
    return(FAILURE);
}

/*
 *----------------------------------------------------------------------
 *
 * FsLocalOpen --
 *
 *      Open a file stored locally.  This uses FsLocalLookup to get a
 *	regular handle on the file, and then calls the server-open
 *	routine to bundle up state needed later by the client-open routine.
 *	That routine will set up a handle for I/O, which for devices and
 *	other things will be different than the regular handle.
 *
 * Results:
 *	An error code.  The results include a pointer to some client data
 *	packaged up by the server-open routine.
 *
 * Side effects:
 *	The file-type server-open routine is called to package
 *	up state for the client-open routine.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
FsLocalOpen(prefixHandlePtr, relativeName, argsPtr, resultsPtr, 
	    newNameInfoPtrPtr)
    FsHandleHeader	*prefixHandlePtr; /* Handle that indicates the starting 
					   * point of the lookup */
    char 	*relativeName;		  /* The name of the file to open */
    Address 	argsPtr;		  /* Bundled arguments for us */
    Address 	resultsPtr;		  /* Bundled results for us */
    FsRedirectInfo **newNameInfoPtrPtr;   /* We return this if the server 
					   * leaves its domain during the 
					   * lookup. */
{
    register FsOpenArgs *openArgsPtr = (FsOpenArgs *)argsPtr;
    register FsOpenResults *openResultsPtr = (FsOpenResults *)resultsPtr;
    FsLocalFileIOHandle *handlePtr;	/* The handle returned for the file */
    ReturnStatus 	status;		/* Error return from RPC */


    status = FsLocalLookup(prefixHandlePtr, relativeName, openArgsPtr->useFlags,
			  openArgsPtr->type, openArgsPtr->clientID,
			  &openArgsPtr->id, openArgsPtr->permissions,
			  0, &handlePtr, newNameInfoPtrPtr);
    if (status == SUCCESS) {
	/*
	 * Call the file-type server-open routine to set up any state
	 * needed later by the client to open a stream to the file.
	 * For regular files, this is when cache consistency is done.
	 */
	status = (*fsOpenOpTable[handlePtr->descPtr->fileType].srvOpen)
		(handlePtr, openArgsPtr->clientID, openArgsPtr->useFlags,
		 &openResultsPtr->ioFileID, &openResultsPtr->streamID,
		 &openResultsPtr->dataSize, &openResultsPtr->streamData);
	openResultsPtr->nameID = handlePtr->hdr.fileID;
	if (openArgsPtr->clientID != rpc_SpriteID) {
	    openResultsPtr->nameID.type = FS_RMT_FILE_STREAM;
	}
	FsHandleRelease(handlePtr, FALSE);
	FsDomainRelease(handlePtr->hdr.fileID.major);
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsLocalGetAttrPath --
 *
 *	Get the attributes of a local file given its path name.  The attributes
 *	are copied from the disk descriptor need to be updated by contacting
 *	the I/O server for the file (for non-regular files).
 *
 * Results:
 *	Return code from FsLocalLookup.
 *
 * Side effects:
 *	Does call-backs to clients to grab up-to-date access and modify
 *	times for regular files.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
FsLocalGetAttrPath(prefixHandlePtr, relativeName, argsPtr, resultsPtr,
				  newNameInfoPtrPtr)
    FsHandleHeader	*prefixHandlePtr;	/* Handle from prefix table */
    char		*relativeName;		/* The name of the file. */
    Address		argsPtr;		/* Bundled arguments for us */
    Address		resultsPtr;		/* == NIL */
    FsRedirectInfo	**newNameInfoPtrPtr;	/* We return this if the server
						 * leaves its domain during
						 * the lookup. */
{
    ReturnStatus	status;
    Boolean		isExeced;
    FsOpenArgs 		*openArgsPtr;
    FsLocalFileIOHandle *handlePtr;
    FsGetAttrResults	*attrResultsPtr;

    openArgsPtr =  (FsOpenArgs *)argsPtr;
    attrResultsPtr = (FsGetAttrResults *)resultsPtr;

    status = FsLocalLookup(prefixHandlePtr, relativeName,
			openArgsPtr->useFlags, openArgsPtr->type,
			openArgsPtr->clientID,
			&openArgsPtr->id, openArgsPtr->permissions, 0,
			&handlePtr, newNameInfoPtrPtr);
    if (status != SUCCESS) {
	return(status);
    }
    /*
     * Do call-backs to get attributes cached (for regular files) on clients,
     * then copy the attributes from the disk descriptor.
     */
    FsGetClientAttrs(handlePtr, openArgsPtr->clientID, &isExeced);
    FsAssignAttrs(handlePtr, isExeced, attrResultsPtr->attrPtr);
    /*
     * Get the I/O fileID so our client can contact the I/O server.
     */
    status = (*fsOpenOpTable[handlePtr->descPtr->fileType].srvOpen)
	    (handlePtr, openArgsPtr->clientID, 0, attrResultsPtr->fileIDPtr,
	     (FsFileID *)NIL, (int *)NIL, (ClientData *)NIL);
    if (status != SUCCESS) {
	Sys_Panic(SYS_WARNING,
	    "FsLocalGetAttrPath, srvOpen of \"%s\" <%d,%d> failed <%x>\n",
	    relativeName, handlePtr->hdr.fileID.minor,
	    handlePtr->hdr.fileID.major, status);
    }
    FsHandleRelease(handlePtr, FALSE);
    FsDomainRelease(handlePtr->hdr.fileID.major);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsLocalSetAttrPath --
 *
 *	Set the attributes of a local file given its pathname.  First
 *	we update the disk descriptor, then call the srvOpen routine
 *	to get an I/O handle for the file.  This is used by our caller
 *	to branch to the stream-type setIOAttr routine.
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
FsLocalSetAttrPath(prefixHandlePtr, relativeName, argsPtr, resultsPtr,
				  newNameInfoPtrPtr)
    FsHandleHeader *prefixHandlePtr;	/* File handle from prefix table */
    char *relativeName;		/* The name of the file. */
    Address argsPtr;		/* Bundled arguments for us */
    Address resultsPtr;		/* Bundled results from us */
    FsRedirectInfo **newNameInfoPtrPtr;/*We return this if the server leaves its
				        * domain during the lookup. */
{
    ReturnStatus		status;
    FsSetAttrArgs		*setAttrArgsPtr;
    FsOpenArgs			*openArgsPtr;
    FsFileID			*fileIDPtr;
    FsLocalFileIOHandle		*handlePtr;

    setAttrArgsPtr =  (FsSetAttrArgs *)argsPtr;
    openArgsPtr = &setAttrArgsPtr->openArgs;
    fileIDPtr = (FsFileID *)resultsPtr;

    status = FsLocalLookup(prefixHandlePtr, relativeName,
			openArgsPtr->useFlags, openArgsPtr->type,
			openArgsPtr->clientID,
			&openArgsPtr->id, openArgsPtr->permissions, 0,
			&handlePtr, newNameInfoPtrPtr);
    if (status != SUCCESS) {
	return(status);
    }
    /*
     * Set the attributes on the disk descriptor.
     */
    FsHandleUnlock(handlePtr);
    status = FsLocalSetAttr(&handlePtr->hdr.fileID, &setAttrArgsPtr->attr,
			    &openArgsPtr->id, setAttrArgsPtr->flags);
    /*
     * Get the I/O handle so our client can contact the I/O server.
     */
    if (status == SUCCESS) {
	FsHandleLock(handlePtr);
	status = (*fsOpenOpTable[handlePtr->descPtr->fileType].srvOpen)
		(handlePtr, openArgsPtr->clientID, 0, fileIDPtr,
		 (FsFileID *)NIL, (int *)NIL, (ClientData *)NIL);
	if (status != SUCCESS) {
	    Sys_Panic(SYS_WARNING,
		"FsLocalSetAttrPath, srvOpen of \"%s\" <%d,%d> failed <%x>\n",
		relativeName, handlePtr->hdr.fileID.minor,
		handlePtr->hdr.fileID.major, status);
	}
    }
    FsHandleRelease(handlePtr, FALSE);
    FsDomainRelease(handlePtr->hdr.fileID.major);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsLocalMakeDevice --
 *
 *	Create a device file.  A file is created with type FS_DEVICE and
 *	then the handle and the descriptor have their device information
 *	updated from the MakeDevice parameters.  This device information,
 *	along with the FS_DEVICE file type, causes I/O operations on the
 *	file to be directed to the device driver routines.
 *
 * Results:
 *	The results of the lookup if it fails, or SUCCESS.
 *
 * Side effects:
 *	Create the file and set up the descriptor's and handle' device info.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
FsLocalMakeDevice(prefixHandle, relativeName, argsPtr, resultsPtr,
				newNameInfoPtrPtr)
    FsHandleHeader	*prefixHandle;	/* Reference to prefix of the domain */
    char		*relativeName;	/* The name of the file. */
    Address		argsPtr;	/* Bundled arguments for us */
    Address		resultsPtr;	/* == NIL */
    FsRedirectInfo	**newNameInfoPtrPtr;	/* We return this if the server
					 *leaves its domain during lookup. */
{
    ReturnStatus	status;
    FsMakeDeviceArgs	*makeDevArgsPtr;
    FsLocalFileIOHandle *handlePtr;
    FsDomain		*domainPtr;
    register FsFileDescriptor *descPtr;

    makeDevArgsPtr = (FsMakeDeviceArgs *)argsPtr;
    status = FsLocalLookup(prefixHandle, relativeName,
		FS_CREATE | FS_EXCLUSIVE | FS_FOLLOW, FS_DEVICE,
		makeDevArgsPtr->clientID,
		&makeDevArgsPtr->id, makeDevArgsPtr->permissions,
		0, &handlePtr, newNameInfoPtrPtr);
    if (status == SUCCESS) {
	descPtr = handlePtr->descPtr;
	descPtr->devServerID = makeDevArgsPtr->device.serverID;
	descPtr->devType = makeDevArgsPtr->device.type;
	descPtr->devUnit = makeDevArgsPtr->device.unit;
	domainPtr = FsDomainFetch(handlePtr->hdr.fileID.major, FALSE);
	if (domainPtr == (FsDomain *)NIL) {
	    status = FS_DOMAIN_UNAVAILABLE;
	    Sys_Panic(SYS_WARNING, "FsLocalMakeDevice: Domain unavailable\n");
	} else {
	    status = FsStoreFileDesc(domainPtr, handlePtr->hdr.fileID.minor,
				     descPtr);
	    FsDomainRelease(handlePtr->hdr.fileID.major);
	}
	FsHandleRelease(handlePtr, TRUE);
    }
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * FsLocalMakeDir --
 *
 *	Make the named directory.
 *
 * Results:
 *	An error code.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
FsLocalMakeDir(prefixHandle, relativeName, argsPtr, resultsPtr, 
	       newNameInfoPtrPtr)
    FsHandleHeader	*prefixHandle;	/* Reference to prefix of the domain */
    char		*relativeName;	/* The name of the dir. to create */
    Address		argsPtr;	/* Ref. to FsOpenArgs */
    Address		resultsPtr;	/* == NIL */
    FsRedirectInfo	**newNameInfoPtrPtr;	/* We return this if the server
					 * leaves its domain during lookup. */
{
    ReturnStatus	status;
    FsOpenArgs		*openArgsPtr;	/* Pointer to bundled arguments */
    FsLocalFileIOHandle	*handlePtr;

    openArgsPtr = (FsOpenArgs *)argsPtr;

    status = FsLocalLookup(prefixHandle, relativeName, openArgsPtr->useFlags,
			  openArgsPtr->type, openArgsPtr->clientID,
			  &openArgsPtr->id, openArgsPtr->permissions,
			  0, &handlePtr, newNameInfoPtrPtr);
    if (status == SUCCESS) {
	FsHandleRelease(handlePtr, TRUE);
	FsDomainRelease(handlePtr->hdr.fileID.major);
    }
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * FsLocalRemove --
 *
 *	Remove a file from the local domain.
 *
 * Results:
 *	An error code.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
FsLocalRemove(prefixHandle, relativeName, argsPtr, resultsPtr, 
	      newNameInfoPtrPtr)
    FsHandleHeader	*prefixHandle;	/* Reference to prefix of the domain */
    char		*relativeName;	/* The name of the file to remove */
    Address		argsPtr;	/* Bundled arguments for us */
    Address		resultsPtr;	/* == NIL */
    FsRedirectInfo	**newNameInfoPtrPtr;	/* We return this if the server 
					 * leaves its domain during lookup. */
{
    register ReturnStatus status;
    register FsLookupArgs *lookupArgsPtr;

    lookupArgsPtr = (FsLookupArgs *)argsPtr;

    status = FsLocalLookup(prefixHandle, relativeName, lookupArgsPtr->useFlags,
			 FS_FILE, lookupArgsPtr->clientID, &lookupArgsPtr->id,
			 0, 0, (FsLocalFileIOHandle **)NIL, newNameInfoPtrPtr);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsLocalRemoveDir --
 *
 *	Remove a directory from the local domain.
 *
 * Results:
 *	An error code.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
FsLocalRemoveDir(prefixHandle, relativeName, argsPtr, resultsPtr, 
	      newNameInfoPtrPtr)
    FsHandleHeader	*prefixHandle;	/* Reference to prefix of the domain */
    char		*relativeName;	/* The name of the file to remove */
    Address		argsPtr;	/* Bundled arguments for us */
    Address		resultsPtr;	/* == NIL */
    FsRedirectInfo	**newNameInfoPtrPtr;	/* We return this if the server 
					 * leaves its domain during lookup. */
{
    register ReturnStatus status;
    register FsLookupArgs *lookupArgsPtr;

    lookupArgsPtr = (FsLookupArgs *)argsPtr;

    status = FsLocalLookup(prefixHandle, relativeName, lookupArgsPtr->useFlags,
		     FS_DIRECTORY, lookupArgsPtr->clientID, &lookupArgsPtr->id,
		     0, 0, (FsLocalFileIOHandle **)NIL, newNameInfoPtrPtr);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsLocalRename --
 *
 *	Rename a local file.
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
FsLocalRename(prefixHandle1, relativeName1, prefixHandle2, relativeName2,
	    lookupArgsPtr, newNameInfoPtrPtr, name1ErrorPtr)
    FsHandleHeader	*prefixHandle1;	/* Token from the prefix table */
    char		*relativeName1;	/* The new name of the file. */
    FsHandleHeader	*prefixHandle2;	/* Token from the prefix table */
    char		*relativeName2;	/* The new name of the file. */
    FsLookupArgs	*lookupArgsPtr;	/* Contains ID info */
    FsRedirectInfo	**newNameInfoPtrPtr;	/* We return this if the server
						 * leaves its domain during
						 * lookup. */
    Boolean		*name1ErrorPtr;	/* TRUE if redirect info or stale
					 * handle error is for the first name,
					 * FALSE if for the second. */
{
    ReturnStatus status;

    lookupArgsPtr->useFlags = FS_LINK | FS_RENAME;
    status = FsLocalHardLink(prefixHandle1, relativeName1, prefixHandle2,
	    relativeName2, lookupArgsPtr, newNameInfoPtrPtr, name1ErrorPtr);
    if (status == SUCCESS) {
	lookupArgsPtr->useFlags = FS_DELETE | FS_RENAME;
	status = FsLocalRemove(prefixHandle1, relativeName1, 
		    (Address) lookupArgsPtr, (Address)NIL, newNameInfoPtrPtr);
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsLocalHardLink --
 *
 *	Make another name for an existing file.
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
FsLocalHardLink(prefixHandle1, relativeName1, prefixHandle2, relativeName2,
	    lookupArgsPtr, newNameInfoPtrPtr, name1ErrorPtr)
    FsHandleHeader	*prefixHandle1;	/* Token from the prefix table */
    char		*relativeName1;	/* The new name of the file. */
    FsHandleHeader	*prefixHandle2;	/* Token from the prefix table */
    char		*relativeName2;	/* The new name of the file. */
    FsLookupArgs	*lookupArgsPtr;	/* Contains ID info */
    FsRedirectInfo	**newNameInfoPtrPtr;	/* We return this if the server
						 * leaves its domain during
						 * lookup. */
    Boolean		*name1ErrorPtr;	/* TRUE if redirect-info or stale
					 * handle error is for first pathname,
					 * FALSE if for the second. */
{
    ReturnStatus status;
    FsLocalFileIOHandle *handle1Ptr;
    FsLocalFileIOHandle *handle2Ptr;

    *name1ErrorPtr = FALSE;

    /*
     * This lookup gets a locked handle on the (presumably) existing file.
     */
    status = FsLocalLookup(prefixHandle1, relativeName1,
	   lookupArgsPtr->useFlags & FS_FOLLOW, FS_FILE,
	   lookupArgsPtr->clientID, &lookupArgsPtr->id,
	   0, 0, (FsLocalFileIOHandle **)&handle1Ptr, newNameInfoPtrPtr);
    if (status != SUCCESS) {
	*name1ErrorPtr = TRUE;
	return(status);
    }
    FsHandleUnlock(handle1Ptr);
    if (prefixHandle2 == (FsHandleHeader *)NIL ||
	prefixHandle2->fileID.major != prefixHandle1->fileID.major) {
	/*
	 * The second pathname which isn't in our domain.  We have
	 * been called in this case to see if the first pathname would
	 * redirect away from us, but it didn't.
	 */
	status = FS_CROSS_DOMAIN_OPERATION;
    } else {
	/*
	 * This lookup does the linking.  If our caller has set FS_RENAME in
	 * lookupArgsPtr->useFlags then directories can be linked.  Handle1
	 * is unlocked because the linking will end up locking it again.
	 * The result of a successful return from this call is that
	 * both handle1 and handle2 reference the same handle, and that
	 * handle is locked.
	 */
	status = FsLocalLookup(prefixHandle2, relativeName2,
		lookupArgsPtr->useFlags, handle1Ptr->descPtr->fileType,
		lookupArgsPtr->clientID,
		&lookupArgsPtr->id, 0, handle1Ptr->hdr.fileID.minor,
		(FsLocalFileIOHandle **)&handle2Ptr, newNameInfoPtrPtr);
    }
    if (status == SUCCESS) {
	FsHandleRelease(handle2Ptr, TRUE);
	FsDomainRelease(handle2Ptr->hdr.fileID.major);
    }
    FsHandleRelease(handle1Ptr, FALSE);
    FsDomainRelease(handle1Ptr->hdr.fileID.major);
    return(status);
}
