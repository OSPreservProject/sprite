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
#include "fslcl.h"
#include "fsconsist.h"
#include "fscache.h"
#include "fsdm.h"
#include "fsStat.h"
#include "rpc.h"


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
    register Fs_HandleHeader	*hdrPtr = streamPtr->ioHandlePtr;
    register Fs_NameInfo		*nameInfoPtr = streamPtr->nameInfoPtr;

    if (!Fsutil_HandleValid(hdrPtr)) {
	status = FS_STALE_HANDLE;
    } else {
	if (nameInfoPtr == (Fs_NameInfo *)NIL) {
	    /*
	     * Anonymous pipes have no name info.
	     */
	    bzero((Address)attrPtr, sizeof(Fs_Attributes));
	    status = SUCCESS;
	} else {
	    /*
	     * Get the initial version of the attributes from the file server
	     * that has the name of the file.
	     */
	    status = (*fs_AttrOpTable[nameInfoPtr->domainType].getAttr)
			(&nameInfoPtr->fileID, rpc_SpriteID, attrPtr);
#ifdef lint
	    status = FslclGetAttr(&nameInfoPtr->fileID, rpc_SpriteID,attrPtr);
	    status = FsrmtGetAttr(&nameInfoPtr->fileID,rpc_SpriteID,attrPtr);
	    status = FspdevPseudoGetAttr(&nameInfoPtr->fileID,rpc_SpriteID,attrPtr);
#endif lint
	    if (status != SUCCESS) {
		printf(
		    "Fs_GetAttrStream: can't get name attributes <%x> for %s\n",
		    status, Fsutil_GetFileName(streamPtr));
	    }
	}
	if (status == SUCCESS) {
	    /*
	     * Update the attributes by contacting the I/O server.
	     */
	    fs_Stats.cltName.getIOAttrs++;
	    status = (*fsio_StreamOpTable[hdrPtr->fileID.type].getIOAttr)
			(&hdrPtr->fileID, rpc_SpriteID, attrPtr);
#ifdef lint
	    status = Fsrmt_GetIOAttr(&hdrPtr->fileID, rpc_SpriteID, attrPtr);
	    status = FsrmtFileGetIOAttr(&hdrPtr->fileID, rpc_SpriteID, attrPtr);
	    status = Fsio_DeviceGetIOAttr(&hdrPtr->fileID, rpc_SpriteID, attrPtr);
	    status = Fsio_PipeGetIOAttr(&hdrPtr->fileID, rpc_SpriteID, attrPtr);
	    status = FsRmtControlGetIOAttr(&hdrPtr->fileID, rpc_SpriteID,
			attrPtr);
	    status = FspdevControlGetIOAttr(&hdrPtr->fileID, rpc_SpriteID,
			attrPtr);
#endif lint
	}
	fs_Stats.cltName.getAttrIDs++;
    }
    return(status);
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
    Fs_UserIDs *idPtr;		/* Owner and groups of calling process */
    int flags;			/* Specify what attributes to set. */
{
    register ReturnStatus	status;
    register Fs_HandleHeader	*hdrPtr = streamPtr->ioHandlePtr;
    register Fs_NameInfo		*nameInfoPtr = streamPtr->nameInfoPtr;

    if (!Fsutil_HandleValid(hdrPtr)) {
	status = FS_STALE_HANDLE;
    } else {
	if (streamPtr->nameInfoPtr != (Fs_NameInfo *)NIL) {
	    /*
	     * Set the attributes at the name server.
	     */
	    status = (*fs_AttrOpTable[nameInfoPtr->domainType].setAttr)
			(&nameInfoPtr->fileID, attrPtr, idPtr, flags);
#ifdef lint
	    status = FslclSetAttr(&nameInfoPtr->fileID, attrPtr,idPtr,flags);
	    status = FsrmtSetAttr(&nameInfoPtr->fileID, attrPtr,idPtr,flags);
	    status = FspdevPseudoSetAttr(&nameInfoPtr->fileID, attrPtr,idPtr,flags);
#endif lint
	} else {
	    status = SUCCESS;
	}
	if (status == SUCCESS) {
	    /*
	     * Set the attributes at the I/O server.
	     */
	    fs_Stats.cltName.setIOAttrs++;
	    status = (*fsio_StreamOpTable[hdrPtr->fileID.type].setIOAttr)
			(&hdrPtr->fileID, attrPtr, flags);
#ifdef lint
	    status = Fsrmt_SetIOAttr(&hdrPtr->fileID, attrPtr, flags);
	    status = FsrmtFileSetIOAttr(&hdrPtr->fileID, attrPtr, flags);
	    status = Fsio_DeviceSetIOAttr(&hdrPtr->fileID, attrPtr, flags);
	    status = Fsio_PipeSetIOAttr(&hdrPtr->fileID, attrPtr, flags);
	    status = FsRmtControlSetIOAttr(&hdrPtr->fileID, attrPtr, flags);
	    status = FspdevControlSetIOAttr(&hdrPtr->fileID, attrPtr, flags);
#endif lint
	}
    fs_Stats.cltName.setAttrIDs++;
    }
    return(status);
}
