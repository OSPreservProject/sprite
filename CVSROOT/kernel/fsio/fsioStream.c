/* 
 * fsStream.c --
 *
 *	There are two sets of procedures here.  The first manage the stream
 *	as it relates to the handle table; streams are installed in this
 *	table so that handle synchronization primitives can be used, and
 *	so that streams can be found after migration.  The second set of
 *	procedures handle the mapping from streams to user-level stream IDs,
 *	which are indexes into a per-process array of stream pointers.
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
#include "fsStream.h"
#include "fsOpTable.h"
#include "fsClient.h"
#include "proc.h"
#include "mem.h"
#include "byte.h"
#include "sync.h"

/*
 * Monitor to synchronize access to the streamCount variable.
 */
static	Sync_Lock	streamLock = {0, 0};
#define LOCKPTR (&streamLock)

static int	streamCount;	/* Used to generate fileIDs for streams*/



/*
 *----------------------------------------------------------------------
 *
 * FsStreamNew --
 *
 *	Create a new stream.  This chooses a fileID for the stream (because
 *	the stream gets put into the handle table) and initializes fields.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Install the new stream into the handle table and increment the global
 *	streamCount used to generate IDs.
 *
 *----------------------------------------------------------------------
 */
ENTRY Fs_Stream *
FsStreamNew(serverID, ioHandlePtr, useFlags)
    int serverID;
    FsHandleHeader *ioHandlePtr;
    int useFlags;
{
    register Boolean found;
    register Fs_Stream *streamPtr;
    Fs_Stream *newStreamPtr;
    FsFileID fileID;

    LOCK_MONITOR;

    fileID.type = FS_STREAM;
    fileID.serverID = serverID;
    fileID.major = 0;

    do {
	fileID.minor = ++streamCount;
	found = FsHandleInstall(&fileID, sizeof(Fs_Stream),
				(FsHandleHeader **)&newStreamPtr);
	if (found) {
	    /*
	     * Don't want to conflict with existing streams.
	     */
	    FsHandleRelease(newStreamPtr, TRUE);
	}
    } while (found);

    streamPtr = newStreamPtr;
    streamPtr->offset = 0;
    streamPtr->flags = useFlags;
    streamPtr->ioHandlePtr = ioHandlePtr;
    streamPtr->nameInfoPtr = (FsNameInfo *)NIL;
    List_Init(&streamPtr->clientList);

    UNLOCK_MONITOR;
    return(streamPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * FsStreamFind --
 *
 *	Find a stream given its fileID.  If it isn't found then a local
 *	instance is installed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Install the new stream into the handle table.
 *
 *----------------------------------------------------------------------
 */
Fs_Stream *
FsStreamFind(streamIDPtr, ioHandlePtr, useFlags, foundPtr)
    FsFileID *streamIDPtr;
    FsHandleHeader *ioHandlePtr;
    int useFlags;
    Boolean *foundPtr;
{
    register Boolean found;
    register Fs_Stream *streamPtr;
    Fs_Stream *newStreamPtr;
    FsFileID;

    found = FsHandleInstall(streamIDPtr, sizeof(Fs_Stream),
			    (FsHandleHeader **)&newStreamPtr);
    streamPtr = newStreamPtr;
    if (!found) {
	streamPtr->offset = 0;
	streamPtr->flags = useFlags;
	streamPtr->ioHandlePtr = ioHandlePtr;
	streamPtr->nameInfoPtr = (FsNameInfo *)NIL;
	List_Init(&streamPtr->clientList);
    } else if (streamPtr->ioHandlePtr == (FsHandleHeader *)NIL) {
	streamPtr->ioHandlePtr = ioHandlePtr;
    }
    if (foundPtr != (Boolean *)NIL) {
	*foundPtr = found;
    }
    return(streamPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_StreamCopy --
 *
 *	Duplicate a stream.  This ups the reference count on the stream
 *	so that it won't go away until its last user closes it.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The reference count on the stream is incremented.
 *
 *----------------------------------------------------------------------
 */
ENTRY ReturnStatus
Fs_StreamCopy(oldStreamPtr, newStreamPtrPtr)
    Fs_Stream *oldStreamPtr;
    Fs_Stream **newStreamPtrPtr;
{
    *newStreamPtrPtr = FsHandleDupType(Fs_Stream, oldStreamPtr);
    FsHandleUnlock(oldStreamPtr);
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * FsStreamClientVerify --
 *
 *	Verify that the remote client is known for the stream, and return
 *	a locked pointer to the stream's handle.
 *
 * Results:
 *	A pointer to the handle for the stream, or NIL if
 *	the client is bad.
 *
 * Side effects:
 *	The handle is returned unlocked but with its refCount incremented.
 *	It should be released with FsHandleRelease(..., FALSE)
 *
 *----------------------------------------------------------------------
 */

Fs_Stream *
FsStreamClientVerify(streamIDPtr, clientID)
    FsFileID	*streamIDPtr;	/* Client's stream ID */
    int		clientID;	/* Host ID of the client */
{
    register FsStreamClientInfo *clientPtr;
    register Fs_Stream *streamPtr;
    Boolean found = FALSE;

    streamPtr = FsHandleFetchType(Fs_Stream, streamIDPtr);
    if (streamPtr != (Fs_Stream *)NIL) {
	LIST_FORALL(&streamPtr->clientList, (List_Links *) clientPtr) {
	    if (clientPtr->clientID == clientID) {
		found = TRUE;
		break;
	    }
	}
	if (!found) {
	    register FsHandleHeader *tHdrPtr = streamPtr->ioHandlePtr;
	    Sys_Panic(SYS_WARNING,
		"FsClientVerify, client %d not known for stream <%d>\n",
		clientID, tHdrPtr->fileID.minor);
	    FsHandleRelease(streamPtr, TRUE);
	} else {
	    FsHandleUnlock(streamPtr);
	}
    } else {
	Sys_Panic(SYS_WARNING, "No stream <%d> for client %d\n",
	    streamIDPtr->minor, clientID);
    }
    return(streamPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * FsStreamDispose --
 *
 *	Discard a stream.  This call removes the stream from the handle
 *	table and frees associated storage.  The I/O handle pointer part
 *	should have already been cleaned up by its handler.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Remove the stream handle from the handle table.
 *
 *----------------------------------------------------------------------
 */
ENTRY void
FsStreamDispose(streamPtr)
    Fs_Stream *streamPtr;
{
    FsHandleRelease(streamPtr, TRUE);
    if (streamPtr->nameInfoPtr != (FsNameInfo *)NIL) {
	Mem_Free((Address)streamPtr->nameInfoPtr);
    }
    FsHandleRemove(streamPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * FsStreamScavenge --
 *
 *	Scavenge a stream.  Servers may have no references to a stream handle,
 *	but still have some things on its client list.  Clients have
 *	references, but no client list.  A stream with neither references
 *	or a client list is scavengable.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Removes the stream handle if it has no references and no clients.
 *
 *----------------------------------------------------------------------
 */
ENTRY void
FsStreamScavenge(hdrPtr)
    FsHandleHeader *hdrPtr;
{
    register Fs_Stream *streamPtr = (Fs_Stream *)hdrPtr;

    if (streamPtr->hdr.refCount == 0 &&
	List_IsEmpty(&streamPtr->clientList)) {
	Sys_Panic(SYS_WARNING, "FsStreamScavenge, removing stream <%d,%d>\n",
		streamPtr->hdr.fileID.serverID,
		streamPtr->hdr.fileID.minor);
	FsHandleRemove((FsHandleHeader *)streamPtr);
    } else {
	FsHandleUnlock((FsHandleHeader *)streamPtr);
    }
}


#ifdef notdef
/*
 *----------------------------------------------------------------------
 *
 * FsStreamCloseCheck --
 *
 *	Return TRUE if an actual close should be performed on the stream
 *	because this is the last reference to a particular open.
 *	MUST BE CALLED WITH stream handle LOCKED so it is safe to
 *	look at the reference count of the handle.
 *
 * Results:
 *	TRUE if an actual close should be performed on this stream.
 *
 * Side effects:
 *	Reference count of the stream is decremented.
 *
 *----------------------------------------------------------------------
 */
ENTRY Boolean
FsStreamCloseCheck(streamPtr)
    Fs_Stream	*streamPtr;
{
    register Boolean	shouldClose;
    LOCK_MONITOR;
    streamPtr->refCount--;
    shouldClose = (streamPtr->refCount == 0);
    UNLOCK_MONITOR;
    return(shouldClose);
}
#endif notdef

typedef struct StreamReopenParams {
    FsFileID	streamID;
    FsFileID	ioFileID;
    int		useFlags;
    int		offset;
} StreamReopenParams;

/*
 *----------------------------------------------------------------------
 *
 * FsStreamReopen --
 *
 *	This is called initially on the client side from FsHandleReopen.
 *	That instance then does an RPC to the server, which again invokes
 *	this routine.  On the client side we don't do much except pass
 *	over the streamID and the ioHandle fileID so the server can
 *	re-create state.  On the server we have to re-setup the stream,
 *	which is sort of a pain because it must reference the correct
 *	I/O handle.
 *
 * Results:
 *	SUCCESS if the stream was reopened.
 *
 * Side effects:
 *	On the client, do an RPC to the server.
 *	On the server, re-create the stream.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ENTRY ReturnStatus
FsStreamReopen(hdrPtr, clientID, inData, outSizePtr, outDataPtr)
    FsHandleHeader	*hdrPtr;	/* Stream's handle header */
    int			clientID;
    ClientData		inData;		/* Non-NIL on the server */
    int			*outSizePtr;	/* Non-NIL on the server */
    ClientData		*outDataPtr;	/* Non-NIL on the server */
{
    register Fs_Stream	*streamPtr = (Fs_Stream *)hdrPtr;
    ReturnStatus status;
    LOCK_MONITOR;

    if (inData == (ClientData)NIL) {
	/*
	 * Called on the client side.  We contact the server to invoke
	 * this procedure there with some input parameters.
	 */
	StreamReopenParams reopenParams;
	int outSize = 0;

	reopenParams.streamID = hdrPtr->fileID;
	reopenParams.ioFileID = streamPtr->ioHandlePtr->fileID;
	reopenParams.useFlags = streamPtr->flags;
	reopenParams.offset   = streamPtr->offset;
	status = FsSpriteReopen(hdrPtr, sizeof(reopenParams),
		    (Address)&reopenParams, &outSize, (Address)NIL);
    } else {
	/*
	 * Called on the server side.  We need to first make sure there
	 * is a corresponding I/O handle for the stream, and then we
	 * can set up the stream.
	 */
	StreamReopenParams	*reopenParamsPtr;
	register FsFileID	*fileIDPtr;
	FsHandleHeader		*ioHandlePtr;

	reopenParamsPtr = (StreamReopenParams *)inData;
	fileIDPtr = &reopenParamsPtr->ioFileID;
#ifdef notdef
	switch (fileIDPtr->type) {
	    case FS_RMT_FILE_STREAM:
		fileIDPtr->type = FS_LCL_FILE_STREAM;
		status = FsLocalFileHandleInit(fileIDPtr,
			    (FsLocalFileIOHandle **)ioHandlePtr);
		break;
	    case FS_RMT_DEVICE_STREAM:
		fileIDPtr->type = FS_LCL_DEVICE_STREAM;
		(void)FsDeviceHandleInit(fileIDPtr,
			    (FsLocalFileIOHandle **)ioHandlePtr);
		status = SUCCESS;
		break;
	    default:
		Sys_Panic(SYS_FATAL, "Unsupported type in FsStreamReopen\n");
		return(FAILURE);
	}
#endif
	ioHandlePtr = (*fsStreamOpTable[fileIDPtr->type].clientVerify)
			(fileIDPtr, clientID);
	if (ioHandlePtr != (FsHandleHeader *)NIL) {
	    Boolean found;

	    streamPtr = FsStreamFind(&reopenParamsPtr->streamID, ioHandlePtr,
				     reopenParamsPtr->useFlags, &found);
	    /*
	     * BRENT Have to worry about the shared offset here.
	     */
	    streamPtr->offset = reopenParamsPtr->offset;

	    FsStreamClientOpen(&streamPtr->clientList, clientID,
			reopenParamsPtr->useFlags);

	    if (!found) {
		/*
		 * If the stream wasn't found it means we have to leave
		 * a refernece on the I/O handle for it.
		 */
		FsHandleUnlock(ioHandlePtr);
	    } else {
		FsHandleRelease(ioHandlePtr, TRUE);
	    }
	    FsHandleRelease(streamPtr, TRUE);
	    status = SUCCESS;
	} else {
	    Sys_Panic(SYS_WARNING, "FsStreamReopen, I/O handle not found\n");
	    status = FAILURE;
	}
    }
    UNLOCK_MONITOR;
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsGetStreamID --
 *
 *	Save the stream pointer in the process's list of stream pointers
 *	and return its index in that list.  The index is used as
 *	a handle for the stream.  E.g. the user supplies the index
 *	in read and write calls and the kernel gets the file pointer
 *	from the list.
 *
 * Results:
 *	SUCCESS		- the operation was successful.
 *	others		- value returned by GrowStreamList.
 *
 * Side effects:
 *	It adds the input streamPtr to the end of the process's list,
 *	if the list is too short (or empty) it is expanded (or created).
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
FsGetStreamID(streamPtr, streamIDPtr)
    Fs_Stream	*streamPtr;	/* A reference to an open file */
    int		*streamIDPtr;	/* Return value, the index of the file pointer
				 * in the process's list of open files */
{
    register Proc_ControlBlock	*procPtr;	/* This process's proc table 
						 * entry */
    register Fs_Stream		**streamPtrPtr;	/* Process's list of open 
						 * streams. */
    register int		index;		/* Index into list of open 
						 * streams */
    ReturnStatus		status;		/* Error from growing file 
						 * list. */

    procPtr = Proc_GetEffectiveProc(Sys_GetProcessorNumber());

    if (streamPtr == (Fs_Stream *)0) {
	Sys_Panic(SYS_FATAL, "Zero valued streamPtr");
    }
    if (procPtr->streamList == (Fs_Stream **)NIL) {
	/*
	 * Allocate the initial array of file pointers.
	 */
#define LENGTH	8
	streamPtrPtr = 
		(Fs_Stream **)Mem_Alloc(LENGTH * sizeof(Fs_Stream *));
	procPtr->streamList = streamPtrPtr;
	procPtr->numStreams = LENGTH;
	for (index = 0; index < LENGTH; index++, streamPtrPtr++) {
	    *streamPtrPtr = (Fs_Stream *)NIL;
	}
#undef LENGTH
    }

    /*
     * Take the first free streamID, or add a new one to the end.
     */
    for (index = 0, streamPtrPtr = procPtr->streamList; 
	 index < procPtr->numStreams; 
	 index++, streamPtrPtr++) {
	if (*streamPtrPtr == (Fs_Stream *)NIL) {
	    *streamPtrPtr = streamPtr;
	    *streamIDPtr = index;
	    return(SUCCESS);
	}
    }
    /*
     * Ran out of room in the original array, allocate a larger
     * array, copy the contents of the original into the beginning,
     * then pick the first empty slot.
     */
    status = GrowStreamList(&procPtr->streamList, procPtr->numStreams,
			procPtr->numStreams * 2);
    if (status == SUCCESS) {
	*streamIDPtr = procPtr->numStreams;
	procPtr->streamList[*streamIDPtr] = streamPtr;
	procPtr->numStreams *= 2;
    }
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * FsClearStreamID --
 *
 *	This invalidates a stream ID.  This is called in conjuction
 *	with Fs_Close to close a stream.  The open stream is identified
 *	by the stream ID which this routine invalidates.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The stream pointer entry in the open stream list is cleared.
 *
 *----------------------------------------------------------------------
 */
void
FsClearStreamID(streamID, procPtr)
    int streamID;		/* Stream ID to invalidate */
    Proc_ControlBlock *procPtr;	/* (Optional) process pointer */
{
    if (procPtr == (Proc_ControlBlock *)NIL) {
	procPtr = Proc_GetEffectiveProc(Sys_GetProcessorNumber());
    }
    procPtr->streamList[streamID] = (Fs_Stream *)NIL;
}


/*
 *----------------------------------------------------------------------
 *
 * GrowStreamList --
 *
 *	Grow a stream ID list.  The caller passes in a pointer
 *	to an array of file pointers => ***Fs_Stream.  This routine
 *	allocates another array of file pointers and copies the
 *	values from the original array into the new one.  It also
 *	initializes the new array elements to NIL.  The original
 *	array of pointers is free'd and the pointer to the
 *	array is reset to point to the new array.
 *
 * Results:
 *	SUCCESS		- the operation was successful.
 *	(One could limit the number of streams here, but we don't.)
 *
 * Side effects:
 *	The original array refered to by *streamListPtr is free'd.
 *	A new array of (Fs_Stream *) is allocated and *streamListPtr
 *	is reset to point to it.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
GrowStreamList(streamListPtr, oldLength, newLength)
    Fs_Stream ***streamListPtr;	/* Pointer to an array of (Fs_Stream *) */
    int oldLength;		/* The length of the orignal array */
    int newLength;		/* The length of the new array */
{
    register int index;
    Fs_Stream **streamList;

    if (newLength <= oldLength) {
	Sys_Panic(SYS_FATAL, "GrowStreamList, stupid call\n");
    }
    streamList = (Fs_Stream **)Mem_Alloc(newLength * sizeof(Fs_Stream *));

    Byte_Copy(sizeof(Fs_Stream *) * oldLength,
	      (Address)*streamListPtr, (Address)streamList);

    Mem_Free((Address)*streamListPtr);

    for (index=0 ; index<oldLength ; index++) {
	if (streamList[index] == (Fs_Stream *)0) {
	    Sys_Panic(SYS_FATAL, "FsGrowList copied zero streamPtr, #%d\n",
				   index);
	}
    }
    for (index=oldLength ; index<newLength ; index++) {
	streamList[index] = (Fs_Stream *)NIL;
    }
    *streamListPtr = streamList;
    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * FsGetStreamPtr --
 *
 *	This converts a users stream id into a pointer to the
 *	stream structure for the open stream.  The stream id is
 *	an index into a per-process open stream list.  This does
 *	bounds checking the open stream list and returns the
 *	indexed stream pointer.
 *
 * Results:
 *	SUCCESS		- the operation was successful.
 *	FS_INVALID_ARG	- the stream ID was out of range or the streamPtr
 *			  for streamID was NIL.
 *
 * Side effects:
 *	*streamPtrPtr is set to reference the stream structure indexed
 *	by the streamID.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
FsGetStreamPtr(procPtr, streamID, streamPtrPtr)
    Proc_ControlBlock	*procPtr;	/* The owner of an open file list */
    int			streamID;	/* A possible index into the list */
    Fs_Stream		**streamPtrPtr;	/* The pointer from the list*/
{
    if (streamID < 0 || streamID >= procPtr->numStreams) {
	return(FS_INVALID_ARG);
    } else {
	register Fs_Stream *streamPtr;

	streamPtr = procPtr->streamList[streamID];
	if (streamPtr == (Fs_Stream *)NIL) {
	    return(FS_INVALID_ARG);
	} else if (streamPtr == (Fs_Stream *)0) {
	    Sys_Panic(SYS_WARNING, "Stream Ptr # %d was zero\n", streamID);
	    return(FS_INVALID_ARG);
	} else {
	    *streamPtrPtr = streamPtr;
	    return(SUCCESS);
	}
    }
}


/*
 *----------------------------------------------------------------------
 *
 * Fs_GetNewID --
 *
 *	This gets a new stream ID that refers to the same open file
 *	as the first argument.  After the call the new stream ID will
 *	be equivalent to the old one - system calls that take a stream
 *	ID could be passed either stream ID.  There are two uses of this
 *	routine.  If it doesn't matter what the new stream ID is then
 *	the second argument should point to FS_ANYID.  If the new stream
 *	ID should have a value then the second argument should point
 *	to that value.  If that value was a valid stream ID then the
 *	stream is first closed.
 *
 * Results:
 *	SUCCESS		- the operation was successful.
 *	FS_INVALID_ARG	- newStreamIDPtr was bad or had a bad value.
 *	other		- value returned by FsGrowList.
 *
 * Side effects:
 *	The second argument gets instantiated to a new stream ID.
 *	If the second argument refered to a valid stream ID on entry
 *	(as opposed to pointing to FS_ANYID) then that stream is first closed.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Fs_GetNewID(streamID, newStreamIDPtr)
    int streamID;
    int *newStreamIDPtr;
{
    ReturnStatus 	status;
    Fs_Stream 		*streamPtr;
    Proc_ControlBlock	*procPtr;

    if (newStreamIDPtr == (int *)NIL) {
	return(FS_INVALID_ARG);
    }
    procPtr = Proc_GetEffectiveProc(Sys_GetProcessorNumber());
    status = FsGetStreamPtr(procPtr, streamID, &streamPtr);
    if (status != SUCCESS) {
	return(status);
    }
    if (*newStreamIDPtr == FS_ANYID) {
	Fs_Stream		*newStreamPtr;

	status = Fs_StreamCopy(streamPtr, &newStreamPtr);
	if (status == SUCCESS) {
	    status = FsGetStreamID(newStreamPtr, newStreamIDPtr);
	    if (status != SUCCESS) {
		Fs_Close(newStreamPtr);
	    }
	}
	return(status);
    } else {
	if (*newStreamIDPtr == streamID) {
	    /*
	     * Probably a user error.  We just return without fiddling
	     * with reference counts.
	     */
	    return(SUCCESS);
	} else {
	    /*
	     * Trying to get a specific stream ID.
	     */
	    register int newStreamID;

	    newStreamID = *newStreamIDPtr;
	    if (newStreamID < 0) {
		return(FS_INVALID_ARG);
	    }
	    if (newStreamID >= procPtr->numStreams) {
		register int maxID;
		/*
		 * Need to grow the file list to accomodate this stream ID.
		 * We do a sanity check on the value of stream ID so
		 * we don't nuke ourselves with a huge array.
		 */
		maxID = 2 * procPtr->numStreams;
		maxID = (maxID<128 ? 128 : maxID);
		if (newStreamID > maxID) {
		    return(FS_NEW_ID_TOO_BIG);
		}
		status = GrowStreamList(&procPtr->streamList,
				   procPtr->numStreams,
				   newStreamID + 1 );
		if (status != SUCCESS) {
		    return(status);
		} else {
		    procPtr->numStreams = newStreamID + 1;
		}
	    } else {
		/*
		 * Check to see if *newStreamIDPtr is a valid (Fs_Stream *)
		 * and close it if it is.
		 */
		register Fs_Stream *oldFilePtr;

		oldFilePtr = procPtr->streamList[newStreamID];
		if (oldFilePtr != (Fs_Stream *)NIL) {
		    (void)Fs_Close(oldFilePtr);
		}
	    }
	    status =
		Fs_StreamCopy(streamPtr, &procPtr->streamList[newStreamID]);
	    return(status);
	}
    }
}
