/* 
 * fslclDomain.c --
 *
 *	Implementation of name-space operations in the local domain.
 *	The routines here are called via the prefix table.
 *	They use FslclLookup (in fsLocalLookup.c) to do the guts of
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


#include <sprite.h>
#include <fs.h>
#include <fsconsist.h>
#include <fsio.h>
#include <fsutil.h>
#include <fsNameOps.h>
#include <fsprefix.h>
#include <fslclInt.h>
#include <fsdm.h>
#include <fsutilTrace.h>
#include <rpc.h>
#include <vm.h>
#include <string.h>
#include <proc.h>
#include <spriteTime.h>

#ifdef SOSP91
#include <sospRecord.h>
static Fs_FileID NullFileID = {0};
#endif

char *fslclEmptyDirBlock;

/*
 *----------------------------------------------------------------------
 *
 * Fslcl_DomainInit --
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
Fslcl_DomainInit()
{
    register Fslcl_DirEntry *dirEntryPtr;

    Fsdm_Init();

    fslclEmptyDirBlock = (char *)malloc(FSLCL_DIR_BLOCK_SIZE);
    dirEntryPtr = (Fslcl_DirEntry *)fslclEmptyDirBlock;
    dirEntryPtr->fileNumber = FSDM_ROOT_FILE_NUMBER;
    dirEntryPtr->nameLength = strlen(".");
    dirEntryPtr->recordLength = Fslcl_DirRecLength(dirEntryPtr->nameLength);
    (void)strcpy(dirEntryPtr->fileName, ".");
    dirEntryPtr = (Fslcl_DirEntry *)((int)dirEntryPtr + dirEntryPtr->recordLength);
    dirEntryPtr->fileNumber = FSDM_ROOT_FILE_NUMBER;
    dirEntryPtr->nameLength = strlen("..");
    dirEntryPtr->recordLength = FSLCL_DIR_BLOCK_SIZE - Fslcl_DirRecLength(1);
    (void)strcpy(dirEntryPtr->fileName, "..");
}

/*
 *----------------------------------------------------------------------
 *
 * FslclExport --
 *
 *	This is called from the RPC_FS_PREFIX stub to export a domain
 *	to a remote Sprite host.  The prefix table has already been
 *	examined, and we are passed in the handle that's hooked to it.
 *	This uses Fsio_FileNameOpen to setup Fsio_FileState so the client
 *	can set up a remote file handle for its own prefix table.
 *
 * Results:
 *	That of Fsio_FileNameOpen
 *
 * Side effects:
 *	Adds the client to the set of clients using the directory that
 *	is the top of the local domain.
 *	
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
FslclExport(hdrPtr, clientID, ioFileIDPtr, dataSizePtr, clientDataPtr)
     Fs_HandleHeader	*hdrPtr;	/* A handle from the prefix table. */
     int		clientID;	/* Host ID of client importing prefix */
     register Fs_FileID	*ioFileIDPtr;	/* Return - I/O handle ID */
     int		*dataSizePtr;	/* Return - sizeof(Fsio_FileState) */
     ClientData		*clientDataPtr;	/* Return - ref to Fsio_FileState */
{
    register Fsio_FileIOHandle *handlePtr = (Fsio_FileIOHandle *)hdrPtr;
    register ReturnStatus status;
    Fs_OpenArgs openArgs;
    Fs_OpenResults openResults;

    bzero((Address)&openArgs, sizeof(openArgs));
    openArgs.clientID = clientID;
    openArgs.useFlags = FS_PREFIX;

    Fsutil_HandleLock(handlePtr);
    status = Fsio_FileNameOpen(handlePtr, &openArgs, &openResults);
    if (status == SUCCESS) {
	*ioFileIDPtr = openResults.ioFileID;
	*dataSizePtr = openResults.dataSize;
	*clientDataPtr = openResults.streamData;
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FslclOpen --
 *
 *      Open a file stored locally.  This uses FslclLookup to get a
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
FslclOpen(prefixHandlePtr, relativeName, argsPtr, resultsPtr, 
	    newNameInfoPtrPtr)
    Fs_HandleHeader	*prefixHandlePtr; /* Handle that indicates the starting 
					   * point of the lookup */
    char 	*relativeName;		  /* The name of the file to open */
    Address 	argsPtr;		  /* Bundled arguments for us */
    Address 	resultsPtr;		  /* Bundled results for us */
    Fs_RedirectInfo **newNameInfoPtrPtr;   /* We return this if the server 
					   * leaves its domain during the 
					   * lookup. */
{
    register Fs_OpenArgs *openArgsPtr = (Fs_OpenArgs *)argsPtr;
    register Fs_OpenResults *openResultsPtr = (Fs_OpenResults *)resultsPtr;
    Fsio_FileIOHandle *handlePtr;	/* The handle returned for the file */
    ReturnStatus 	status;		/* Error return from RPC */



#ifdef SOSP91
    SOSP_REMEMBERED_OP = FS_DOMAIN_OPEN;
    SOSP_REMEMBERED_CLIENT = openArgsPtr->clientID;
    SOSP_REMEMBERED_MIG = openArgsPtr->migClientID;
#endif
    status = FslclLookup(prefixHandlePtr, relativeName, &openArgsPtr->rootID,
	    openArgsPtr->useFlags, openArgsPtr->type, openArgsPtr->clientID,
	    &openArgsPtr->id, openArgsPtr->permissions, 0, &handlePtr,
	    newNameInfoPtrPtr);
    if (status == SUCCESS) {
	/*
	 * Call the file-type server-open routine to set up any state
	 * needed later by the client to open a stream to the file.
	 * For regular files, this is when cache consistency is done.
	 */
	status = (*fsio_OpenOpTable[handlePtr->descPtr->fileType].nameOpen)
		(handlePtr, openArgsPtr, openResultsPtr);
	openResultsPtr->nameID = handlePtr->hdr.fileID;
	if (openArgsPtr->clientID != rpc_SpriteID) {
	    openResultsPtr->nameID.type = FSIO_RMT_FILE_STREAM;
	}
	Fsutil_HandleRelease(handlePtr, FALSE);
	Fsdm_DomainRelease(handlePtr->hdr.fileID.major);
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FslclGetAttrPath --
 *
 *	Get the attributes of a local file given its path name.  The attributes
 *	are copied from the disk descriptor need to be updated by contacting
 *	the I/O server for the file (for non-regular files).
 *
 * Results:
 *	Return code from FslclLookup.
 *
 * Side effects:
 *	Does call-backs to clients to grab up-to-date access and modify
 *	times for regular files.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
FslclGetAttrPath(prefixHandlePtr, relativeName, argsPtr, resultsPtr,
				  newNameInfoPtrPtr)
    Fs_HandleHeader	*prefixHandlePtr;	/* Handle from prefix table */
    char		*relativeName;		/* The name of the file. */
    Address		argsPtr;		/* Bundled arguments for us */
    Address		resultsPtr;		/* == NIL */
    Fs_RedirectInfo	**newNameInfoPtrPtr;	/* We return this if the server
						 * leaves its domain during
						 * the lookup. */
{
    ReturnStatus	status;
    Boolean		isExeced;
    Fs_OpenArgs 		*openArgsPtr;
    Fsio_FileIOHandle *handlePtr;
    Fs_GetAttrResults	*attrResultsPtr;
    Fs_OpenResults	openResults;


    openArgsPtr =  (Fs_OpenArgs *)argsPtr;
    attrResultsPtr = (Fs_GetAttrResults *)resultsPtr;

#ifdef SOSP91
    SOSP_ADD_GET_ATTR_TRACE(openArgsPtr->clientID, openArgsPtr->migClientID, 
	    NullFileID);
    SOSP_REMEMBERED_OP = FS_DOMAIN_GET_ATTR;
    SOSP_REMEMBERED_CLIENT = openArgsPtr->clientID;
    SOSP_REMEMBERED_MIG = openArgsPtr->migClientID;
#endif
    status = FslclLookup(prefixHandlePtr, relativeName, &openArgsPtr->rootID,
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
    Fsconsist_GetClientAttrs(handlePtr, openArgsPtr->clientID, &isExeced);
    FslclAssignAttrs(handlePtr, isExeced, attrResultsPtr->attrPtr);
    /*
     * Get the I/O fileID so our client can contact the I/O server.
     */
    openArgsPtr->useFlags = 0;
    status = (*fsio_OpenOpTable[handlePtr->descPtr->fileType].nameOpen)
	    (handlePtr, openArgsPtr, &openResults);
    *attrResultsPtr->fileIDPtr = openResults.ioFileID;

    if (status != SUCCESS) {
	printf("FslclGetAttrPath, nameOpen of \"%s\" <%d,%d> failed <%x>\n",
	    relativeName, handlePtr->hdr.fileID.minor,
	    handlePtr->hdr.fileID.major, status);
    }
    Fsutil_HandleRelease(handlePtr, FALSE);
    Fsdm_DomainRelease(handlePtr->hdr.fileID.major);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FslclSetAttrPath --
 *
 *	Set the attributes of a local file given its pathname.  First
 *	we update the disk descriptor, then call the nameOpen routine
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
FslclSetAttrPath(prefixHandlePtr, relativeName, argsPtr, resultsPtr,
				  newNameInfoPtrPtr)
    Fs_HandleHeader *prefixHandlePtr;	/* File handle from prefix table */
    char *relativeName;		/* The name of the file. */
    Address argsPtr;		/* Bundled arguments for us */
    Address resultsPtr;		/* Bundled results from us */
    Fs_RedirectInfo **newNameInfoPtrPtr;/*We return this if the server leaves its
				        * domain during the lookup. */
{
    ReturnStatus		status;
    Fs_SetAttrArgs		*setAttrArgsPtr;
    Fs_OpenArgs			*openArgsPtr;
    Fs_FileID			*fileIDPtr;
    Fsio_FileIOHandle		*handlePtr;
    Fs_OpenResults		openResults;

    setAttrArgsPtr =  (Fs_SetAttrArgs *)argsPtr;
    openArgsPtr = &setAttrArgsPtr->openArgs;
    fileIDPtr = (Fs_FileID *)resultsPtr;

    status = FslclLookup(prefixHandlePtr, relativeName, &openArgsPtr->rootID,
			openArgsPtr->useFlags, openArgsPtr->type,
			openArgsPtr->clientID,
			&openArgsPtr->id, openArgsPtr->permissions, 0,
			&handlePtr, newNameInfoPtrPtr);
    if (status != SUCCESS) {
	return(status);
    }
#ifdef SOSP91
    SOSP_ADD_SET_ATTR_TRACE(openArgsPtr->clientID, openArgsPtr->migClientID, 
	    handlePtr->hdr.fileID);
    SOSP_REMEMBERED_OP = FS_DOMAIN_SET_ATTR;
    SOSP_REMEMBERED_CLIENT = openArgsPtr->clientID;
    SOSP_REMEMBERED_MIG = openArgsPtr->migClientID;
#endif
    /*
     * Set the attributes on the disk descriptor.
     */
    Fsutil_HandleUnlock(handlePtr);
    status = FslclSetAttr(&handlePtr->hdr.fileID, &setAttrArgsPtr->attr,
			    &openArgsPtr->id, setAttrArgsPtr->flags);
    /*
     * Get the I/O handle so our client can contact the I/O server.
     */
    if (status == SUCCESS) {
	Fsutil_HandleLock(handlePtr);
	openArgsPtr->useFlags = 0;
	status = (*fsio_OpenOpTable[handlePtr->descPtr->fileType].nameOpen)
		(handlePtr, openArgsPtr, &openResults);
	*fileIDPtr = openResults.ioFileID;

	if (status != SUCCESS) {
	    printf(
		"FslclSetAttrPath, nameOpen of \"%s\" <%d,%d> failed <%x>\n",
		relativeName, handlePtr->hdr.fileID.minor,
		handlePtr->hdr.fileID.major, status);
	}
    }
    Fsutil_HandleRelease(handlePtr, FALSE);
    Fsdm_DomainRelease(handlePtr->hdr.fileID.major);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FslclMakeDevice --
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
FslclMakeDevice(prefixHandle, relativeName, argsPtr, resultsPtr,
				newNameInfoPtrPtr)
    Fs_HandleHeader	*prefixHandle;	/* Reference to prefix of the domain */
    char		*relativeName;	/* The name of the file. */
    Address		argsPtr;	/* Bundled arguments for us */
    Address		resultsPtr;	/* == NIL */
    Fs_RedirectInfo	**newNameInfoPtrPtr;	/* We return this if the server
					 *leaves its domain during lookup. */
{
    ReturnStatus	status;
    Fs_MakeDeviceArgs	*makeDevArgsPtr;
    Fsio_FileIOHandle *handlePtr;
    register Fsdm_FileDescriptor *descPtr;

    makeDevArgsPtr = (Fs_MakeDeviceArgs *)argsPtr;
#ifdef SOSP91
    SOSP_REMEMBERED_OP = FS_DOMAIN_MAKE_DEVICE;
    SOSP_REMEMBERED_CLIENT = makeDevArgsPtr->open.clientID;
    SOSP_REMEMBERED_MIG = makeDevArgsPtr->open.migClientID;
#endif
    status = FslclLookup(prefixHandle, relativeName,
		&makeDevArgsPtr->open.rootID,
		FS_CREATE | FS_EXCLUSIVE | FS_FOLLOW, FS_DEVICE,
		makeDevArgsPtr->open.clientID,
		&makeDevArgsPtr->open.id, makeDevArgsPtr->open.permissions,
		0, &handlePtr, newNameInfoPtrPtr);
    if (status == SUCCESS) {
	descPtr = handlePtr->descPtr;
	descPtr->devServerID = makeDevArgsPtr->device.serverID;
	descPtr->devType = makeDevArgsPtr->device.type;
	descPtr->devUnit = makeDevArgsPtr->device.unit;
	descPtr->flags |= FSDM_FD_OTHERS_DIRTY;
	status = Fsdm_FileDescStore(handlePtr, FALSE);
	Fsutil_HandleRelease(handlePtr, TRUE);
    }
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * FslclMakeDir --
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
FslclMakeDir(prefixHandle, relativeName, argsPtr, resultsPtr, 
	       newNameInfoPtrPtr)
    Fs_HandleHeader	*prefixHandle;	/* Reference to prefix of the domain */
    char		*relativeName;	/* The name of the dir. to create */
    Address		argsPtr;	/* Ref. to Fs_OpenArgs */
    Address		resultsPtr;	/* == NIL */
    Fs_RedirectInfo	**newNameInfoPtrPtr;	/* We return this if the server
					 * leaves its domain during lookup. */
{
    ReturnStatus	status;
    Fs_OpenArgs		*openArgsPtr;	/* Pointer to bundled arguments */
    Fsio_FileIOHandle	*handlePtr;

    openArgsPtr = (Fs_OpenArgs *)argsPtr;

#ifdef SOSP91
    SOSP_REMEMBERED_OP = FS_DOMAIN_MAKE_DIR;
    SOSP_REMEMBERED_CLIENT = openArgsPtr->clientID;
    SOSP_REMEMBERED_MIG = openArgsPtr->migClientID;
#endif
    status = FslclLookup(prefixHandle, relativeName, &openArgsPtr->rootID,
	    openArgsPtr->useFlags, openArgsPtr->type, openArgsPtr->clientID,
	    &openArgsPtr->id, openArgsPtr->permissions, 0, &handlePtr,
	    newNameInfoPtrPtr);
    if (status == SUCCESS) {
	Fsutil_HandleRelease(handlePtr, TRUE);
	Fsdm_DomainRelease(handlePtr->hdr.fileID.major);
    }
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * FslclRemove --
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
FslclRemove(prefixHandle, relativeName, argsPtr, resultsPtr, 
	      newNameInfoPtrPtr)
    Fs_HandleHeader	*prefixHandle;	/* Reference to prefix of the domain */
    char		*relativeName;	/* The name of the file to remove */
    Address		argsPtr;	/* Bundled arguments for us */
    Address		resultsPtr;	/* == NIL */
    Fs_RedirectInfo	**newNameInfoPtrPtr;	/* We return this if the server 
					 * leaves its domain during lookup. */
{
    register ReturnStatus status;
    register Fs_LookupArgs *lookupArgsPtr;

    lookupArgsPtr = (Fs_LookupArgs *)argsPtr;

#ifdef SOSP91
    SOSP_REMEMBERED_OP = FS_DOMAIN_REMOVE;
    SOSP_REMEMBERED_CLIENT = lookupArgsPtr->clientID;
    SOSP_REMEMBERED_MIG = lookupArgsPtr->migClientID;
#endif
    status = FslclLookup(prefixHandle, relativeName, &lookupArgsPtr->rootID,
	    lookupArgsPtr->useFlags, FS_FILE, lookupArgsPtr->clientID,
	    &lookupArgsPtr->id, 0, 0, (Fsio_FileIOHandle **)NIL,
	    newNameInfoPtrPtr);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FslclRemoveDir --
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
FslclRemoveDir(prefixHandle, relativeName, argsPtr, resultsPtr, 
	      newNameInfoPtrPtr)
    Fs_HandleHeader	*prefixHandle;	/* Reference to prefix of the domain */
    char		*relativeName;	/* The name of the file to remove */
    Address		argsPtr;	/* Bundled arguments for us */
    Address		resultsPtr;	/* == NIL */
    Fs_RedirectInfo	**newNameInfoPtrPtr;	/* We return this if the server 
					 * leaves its domain during lookup. */
{
    register ReturnStatus status;
    register Fs_LookupArgs *lookupArgsPtr;

    lookupArgsPtr = (Fs_LookupArgs *)argsPtr;

#ifdef SOSP91
    SOSP_REMEMBERED_OP = FS_DOMAIN_REMOVE_DIR;
    SOSP_REMEMBERED_CLIENT = lookupArgsPtr->clientID;
    SOSP_REMEMBERED_MIG = lookupArgsPtr->migClientID;
#endif
    status = FslclLookup(prefixHandle, relativeName, &lookupArgsPtr->rootID,
	    lookupArgsPtr->useFlags, FS_DIRECTORY, lookupArgsPtr->clientID,
	    &lookupArgsPtr->id, 0, 0, (Fsio_FileIOHandle **)NIL,
	    newNameInfoPtrPtr);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FslclRename --
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
FslclRename(prefixHandle1, relativeName1, prefixHandle2, relativeName2,
	    lookupArgsPtr, newNameInfoPtrPtr, name1ErrorPtr)
    Fs_HandleHeader	*prefixHandle1;	/* Token from the prefix table */
    char		*relativeName1;	/* The new name of the file. */
    Fs_HandleHeader	*prefixHandle2;	/* Token from the prefix table */
    char		*relativeName2;	/* The new name of the file. */
    Fs_LookupArgs	*lookupArgsPtr;	/* Contains ID info */
    Fs_RedirectInfo	**newNameInfoPtrPtr;	/* We return this if the server
						 * leaves its domain during
						 * lookup. */
    Boolean		*name1ErrorPtr;	/* TRUE if redirect info or stale
					 * handle error is for the first name,
					 * FALSE if for the second. */
{
    ReturnStatus status;

    lookupArgsPtr->useFlags = FS_LINK | FS_RENAME;
    status = FslclHardLink(prefixHandle1, relativeName1, prefixHandle2,
	    relativeName2, lookupArgsPtr, newNameInfoPtrPtr, name1ErrorPtr);
    if (status == SUCCESS) {
	lookupArgsPtr->useFlags = FS_DELETE | FS_RENAME;
	status = FslclRemove(prefixHandle1, relativeName1, 
		    (Address) lookupArgsPtr, (Address)NIL, newNameInfoPtrPtr);
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FslclHardLink --
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
FslclHardLink(prefixHandle1, relativeName1, prefixHandle2, relativeName2,
	    lookupArgsPtr, newNameInfoPtrPtr, name1ErrorPtr)
    Fs_HandleHeader	*prefixHandle1;	/* Token from the prefix table */
    char		*relativeName1;	/* The new name of the file. */
    Fs_HandleHeader	*prefixHandle2;	/* Token from the prefix table */
    char		*relativeName2;	/* The new name of the file. */
    Fs_LookupArgs	*lookupArgsPtr;	/* Contains ID info */
    Fs_RedirectInfo	**newNameInfoPtrPtr;	/* We return this if the server
						 * leaves its domain during
						 * lookup. */
    Boolean		*name1ErrorPtr;	/* TRUE if redirect-info or stale
					 * handle error is for first pathname,
					 * FALSE if for the second. */
{
    ReturnStatus status;
    Fsio_FileIOHandle *handle1Ptr;
    Fsio_FileIOHandle *handle2Ptr;

    *name1ErrorPtr = FALSE;

    /*
     * This lookup gets a locked handle on the (presumably) existing file.
     */
#ifdef SOSP91
    SOSP_ADD_MKLINK_TRACE(lookupArgsPtr->clientID, lookupArgsPtr->migClientID, 
	    NullFileID);
    SOSP_REMEMBERED_OP = FS_DOMAIN_HARD_LINK;
    SOSP_REMEMBERED_CLIENT = lookupArgsPtr->clientID;
    SOSP_REMEMBERED_MIG = lookupArgsPtr->migClientID;
#endif
    status = FslclLookup(prefixHandle1, relativeName1, &lookupArgsPtr->rootID,
	   lookupArgsPtr->useFlags & FS_FOLLOW, FS_FILE,
	   lookupArgsPtr->clientID, &lookupArgsPtr->id,
	   0, 0, (Fsio_FileIOHandle **)&handle1Ptr, newNameInfoPtrPtr);
    if (status != SUCCESS) {
	*name1ErrorPtr = TRUE;
	return(status);
    }
    Fsutil_HandleUnlock(handle1Ptr);
    if (prefixHandle2 == (Fs_HandleHeader *)NIL ||
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
#ifdef SOSP91
	SOSP_REMEMBERED_OP = FS_DOMAIN_HARD_LINK|0x80;
	SOSP_REMEMBERED_CLIENT = lookupArgsPtr->clientID;
	SOSP_REMEMBERED_MIG = lookupArgsPtr->migClientID;
#endif
	status = FslclLookup(prefixHandle2, relativeName2,
		&lookupArgsPtr->rootID,
		lookupArgsPtr->useFlags, handle1Ptr->descPtr->fileType,
		lookupArgsPtr->clientID,
		&lookupArgsPtr->id, 0, handle1Ptr->hdr.fileID.minor,
		(Fsio_FileIOHandle **)&handle2Ptr, newNameInfoPtrPtr);
    }
    if (status == SUCCESS) {
	Fsutil_HandleRelease(handle2Ptr, TRUE);
	Fsdm_DomainRelease(handle2Ptr->hdr.fileID.major);
    }
    Fsutil_HandleRelease(handle1Ptr, FALSE);
    Fsdm_DomainRelease(handle1Ptr->hdr.fileID.major);
    return(status);
}
