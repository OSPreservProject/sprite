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
#include "fsutil.h"
#include "fsNameOps.h"
#include "fslclInt.h"
#include "fsconsist.h"
#include "fscache.h"
#include "fsdm.h"
#include "fsStat.h"
#include "rpc.h"


/*
 *----------------------------------------------------------------------
 *
 * FslclGetAttr --
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
FslclGetAttr(fileIDPtr, clientID, attrPtr)
    register Fs_FileID		*fileIDPtr;	/* Identfies file */
    int				clientID;	/* Host ID of process asking
						 * for the attributes */
    register Fs_Attributes	*attrPtr;	/* Return - the attributes */
{
    if (fileIDPtr->type != FSIO_LCL_FILE_STREAM) {
	panic( "FslclGetAttr, bad fileID type <%d>\n",
	    fileIDPtr->type);
	return(GEN_INVALID_ARG);
    } else {
	Fsio_FileIOHandle *handlePtr;
	Boolean isExeced;
	ReturnStatus status;

	handlePtr = Fsutil_HandleFetchType(Fsio_FileIOHandle, fileIDPtr);
	if (handlePtr == (Fsio_FileIOHandle *)NIL) {
	    status = Fsio_LocalFileHandleInit(fileIDPtr, (char *)NIL, &handlePtr);
	    if (status != SUCCESS) {
		bzero((Address)attrPtr, sizeof(Fs_Attributes));
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
	Fsconsist_GetClientAttrs(handlePtr, clientID, &isExeced);
	FslclAssignAttrs(handlePtr, isExeced, attrPtr);
	Fsutil_HandleRelease(handlePtr, TRUE);
	return(SUCCESS);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * FslclAssignAttrs --
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
FslclAssignAttrs(handlePtr, isExeced, attrPtr)
    register	Fsio_FileIOHandle	*handlePtr;
    Boolean				isExeced;  /* TRUE if being executed,
						    * use current time for
						    * the access time. */
    register	Fs_Attributes		*attrPtr;
{
    register Fsdm_FileDescriptor *descPtr = handlePtr->descPtr;
    register Fscache_FileInfo *cacheInfoPtr = &handlePtr->cacheInfo;

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
     * Take creation and descriptor modify time from disk descriptor,
     * except that the descModifyTime is >= dataModifyTime.  This
     * constraint is enforced later when the descriptor is written back,
     * so the descriptor may still be out-of-date at this point.
     */
    attrPtr->createTime.seconds		= descPtr->createTime;
    attrPtr->createTime.microseconds	= 0;
    attrPtr->descModifyTime.seconds	= descPtr->descModifyTime;
    attrPtr->descModifyTime.microseconds= 0;
    if (cacheInfoPtr->attr.modifyTime > attrPtr->descModifyTime.seconds) {
	attrPtr->descModifyTime.seconds = cacheInfoPtr->attr.modifyTime;
    }
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
	attrPtr->accessTime.seconds	= fsutil_TimeInSeconds;
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
 * FslclSetAttr --
 *
 *	Set the attributes of a local file given its fileID.  This is
 *	called from Fs_SetAttrStream to set the attributes at the name server.
 *	The flags argument defines which attributes are updated.
 *	This updates the disk descriptor and copies the new information
 * 	into the cache.  It will go to disk on the next sync.
 *
 *	Various constraints are implemented here.
 *	1) You must be super-user or own the file to succeed at all.
 *	2) You must be super-user to change the owner of a file.
 *	3) You must be super-user or a member of the new group
 *		to change the group of a file.  The SETGID bit is
 *		cleared if a non-super-user changes the group.
 *	4) You must be super-user or a member of the file's group
 *		to set the SETGID bit of the file.
 *	5) If you've made it this far you can set the access time,
 *		modify time, and user-defined file type.
 *
 * Results:
 *	FS_NOT_OWNER if you don't own the file
 *	FS_NO_ACCESS if you violate one of the above constraints.
 *
 * Side effects:
 *	Sets the attributes of the file, subject to the above constraints.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
FslclSetAttr(fileIDPtr, attrPtr, idPtr, flags)
    Fs_FileID			*fileIDPtr;	/* Target file. */
    register Fs_Attributes	*attrPtr;	/* New attributes */
    register Fs_UserIDs		*idPtr;		/* Process's owner/group */
    register int		flags;		/* What attrs to set */
{
    register ReturnStatus	status = SUCCESS;
    Fsio_FileIOHandle		*handlePtr;
    register Fsdm_FileDescriptor	*descPtr;
    Fsdm_Domain			*domainPtr;

    handlePtr = Fsutil_HandleFetchType(Fsio_FileIOHandle, fileIDPtr);
    if (handlePtr == (Fsio_FileIOHandle *)NIL) {
	printf(
		"FslclSetAttr, no handle for %s <%d,%d,%d>\n",
		Fsutil_FileTypeToString(fileIDPtr->type),
		fileIDPtr->serverID, fileIDPtr->major, fileIDPtr->minor);
	return(FS_FILE_NOT_FOUND);
    }
    descPtr = handlePtr->descPtr;
    if (descPtr == (Fsdm_FileDescriptor *)NIL) {
	printf( "FslclSetAttr, NIL descPtr\n");
	status = FAILURE;
	goto exit;
    }
    if ((idPtr->user != 0) && (idPtr->user != descPtr->uid)) {
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
		goto exit;
	    } else {
		descPtr->uid = attrPtr->uid;
	    }
	}
	if (attrPtr->gid >= 0 && descPtr->gid != attrPtr->gid) {
	    register int g;
	    /*
	     * Can only set to a group you belong to.  The set-gid
	     * bit is also reset as an extra precaution.
	     */
	    for (g=0 ; g < idPtr->numGroupIDs; g++) {
		if (attrPtr->gid == idPtr->group[g] || idPtr->user == 0) {
		    descPtr->gid = attrPtr->gid;
		    if (idPtr->user != 0) {
			descPtr->permissions &= ~FS_SET_GID;
		    }
		    break;
		}
	    }
	    if (g >= idPtr->numGroupIDs) {
		status = FS_NO_ACCESS;
		goto exit;
	    }
	}
    }
    if (flags & FS_SET_MODE) {
	if (attrPtr->permissions & FS_SET_GID) {
	    /*
	     * Have to verify that the process belongs to the
	     * new group of the file.  We have already verified that
	     * the process's user ID matches the file's owner.
	     */
	    register int g;
	    for (g=0 ; g < idPtr->numGroupIDs; g++) {
		if (attrPtr->gid == idPtr->group[g] || idPtr->user == 0) {
#ifndef lint
		    goto setMode;
#endif not lint
		}
	    }
	    status = FS_NO_ACCESS;
	    goto exit;		/* Note: can't have changed *descPtr by now. */
	}
#ifndef lint
setMode:
#endif not lint
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
	 * Patch this because it gets copied below by Fscache_UpdateCachedAttr.
	 */
	attrPtr->createTime.seconds = descPtr->createTime;
    }

    if (flags & FS_SET_FILE_TYPE) {
	descPtr->userType    = attrPtr->userType;
    }

    /*
     * Copy this new information into the cache block containing the descriptor.
     */
    descPtr->descModifyTime   = fsutil_TimeInSeconds;
    domainPtr = Fsdm_DomainFetch(handlePtr->hdr.fileID.major, FALSE);
    if (domainPtr == (Fsdm_Domain *)NIL) {
	status = FS_DOMAIN_UNAVAILABLE;
    } else {
	status = Fsdm_FileDescStore(domainPtr, handlePtr->hdr.fileID.minor,
		 descPtr);
	Fsdm_DomainRelease(handlePtr->hdr.fileID.major);
    }
    if (status == SUCCESS) {
	/*
	 * Update the attributes cached in the file handle.
	 */
	Fscache_UpdateCachedAttr(&handlePtr->cacheInfo, attrPtr, flags);
    }
exit:
    Fsutil_HandleRelease(handlePtr, TRUE);
    return(status);
}
