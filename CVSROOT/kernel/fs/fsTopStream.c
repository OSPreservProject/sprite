/* 
 * fsStream.c --
 *
 *	The  procedures handle the mapping from streams to
 *	user-level stream IDs,	which are indexes into a per-process array
 *	of stream pointers.
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
#include <fsutil.h>
#include <fsio.h>
#include <fsNameOps.h>
#include <fsconsist.h>
#include <fsStat.h>
#include <proc.h>
#include <sync.h>
#include <rpc.h>


/*
 * Forward declarations. 
 */
static ReturnStatus GrowStreamList _ARGS_((Fs_ProcessState *fsPtr, 
					int newLength));


/*
 *----------------------------------------------------------------------
 *
 * Fs_GetStreamID --
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
Fs_GetStreamID(streamPtr, streamIDPtr)
    Fs_Stream	*streamPtr;	/* A reference to an open file */
    int		*streamIDPtr;	/* Return value, the index of the file pointer
				 * in the process's list of open files */
{
    register Fs_ProcessState	*fsPtr;		/* From process's proc table 
						 * entry */
    register Fs_Stream		**streamPtrPtr;	/* Process's list of open 
						 * streams. */
    register int		index;		/* Index into list of open 
						 * streams */
    ReturnStatus		status;		/* Error from growing file 
						 * list. */

    fsPtr = (Proc_GetEffectiveProc())->fsPtr;

    if (streamPtr == (Fs_Stream *)0) {
	panic( "Zero valued streamPtr");
    }
    if (fsPtr->streamList == (Fs_Stream **)NIL) {
	/*
	 * Allocate the initial array of file pointers.
	 */
	(void)GrowStreamList(fsPtr, 8);
    }

    /*
     * Take the first free streamID, or add a new one to the end.
     */
    for (index = 0, streamPtrPtr = fsPtr->streamList; 
	 index < fsPtr->numStreams; 
	 index++, streamPtrPtr++) {
	if (*streamPtrPtr == (Fs_Stream *)NIL) {
	    *streamPtrPtr = streamPtr;
	    *streamIDPtr = index;
	    fsPtr->streamFlags[index] = 0;
	    return(SUCCESS);
	}
    }
    /*
     * Ran out of room in the original array, allocate a larger
     * array, copy the contents of the original into the beginning,
     * then pick the first empty slot.
     */
    index = fsPtr->numStreams;
    status = GrowStreamList(fsPtr, fsPtr->numStreams * 2);
    if (status == SUCCESS) {
	*streamIDPtr = index;
	fsPtr->streamList[index] = streamPtr;
	fsPtr->streamFlags[index] = 0;
    }
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * Fs_ClearStreamID --
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
Fs_ClearStreamID(streamID, procPtr)
    int streamID;		/* Stream ID to invalidate */
    Proc_ControlBlock *procPtr;	/* (Optional) process pointer */
{
    if (procPtr == (Proc_ControlBlock *)NIL) {
	procPtr = Proc_GetEffectiveProc();
    }
    procPtr->fsPtr->streamList[streamID] = (Fs_Stream *)NIL;
}


/*
 *----------------------------------------------------------------------
 *
 * GrowStreamList --
 *
 *	Grow a stream ID list.  This routine
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
 *	Grows the stream list and the associated array of flag bytes.
 *	The number of streams in the file system state is updated.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
GrowStreamList(fsPtr, newLength)
    Fs_ProcessState *fsPtr;	/* The file system state */
    int newLength;		/* The length of the new array */
{
    register int index;
    register Fs_Stream **streamList;
    register char *streamFlags;

    streamList = (Fs_Stream **)malloc(newLength * sizeof(Fs_Stream *));
    streamFlags = (char *)malloc(newLength * sizeof(char));

    if (fsPtr->numStreams > 0) {
	bcopy((Address)fsPtr->streamList, (Address)streamList, sizeof(Fs_Stream *) * fsPtr->numStreams);
	bcopy((Address)fsPtr->streamFlags, (Address)streamFlags, sizeof(char) * fsPtr->numStreams);

	free((Address)fsPtr->streamList);
	free((Address)fsPtr->streamFlags);
    
	for (index=0 ; index < fsPtr->numStreams ; index++) {
	    if ((int)streamList[index] != NIL &&
		(unsigned int)streamList[index] < 1024) {
		panic( "GrowStreamList copied bad streamPtr, %x\n",
				       streamList[index]);
	    }
	}
    }

    fsPtr->streamList = streamList;
    fsPtr->streamFlags =  streamFlags;

    for (index=fsPtr->numStreams ; index < newLength ; index++) {
	fsPtr->streamList[index] = (Fs_Stream *)NIL;
	fsPtr->streamFlags[index] = 0;
    }
    fsPtr->numStreams = newLength;
    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * Fs_GetStreamPtr --
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
Fs_GetStreamPtr(procPtr, streamID, streamPtrPtr)
    Proc_ControlBlock	*procPtr;	/* The owner of an open file list */
    int			streamID;	/* A possible index into the list */
    Fs_Stream		**streamPtrPtr;	/* The pointer from the list*/
{
    if (streamID < 0 || streamID >= procPtr->fsPtr->numStreams) {
	return(FS_INVALID_ARG);
    } else {
	register Fs_Stream *streamPtr;

	streamPtr = procPtr->fsPtr->streamList[streamID];
	if (streamPtr == (Fs_Stream *)NIL) {
	    return(FS_INVALID_ARG);
	} else if ((unsigned int)streamPtr < 1024) {
	    /*
	     * There was a time when control stream pointers were not
	     * being passed right, or being passed a second time after
	     * already being converted to a streamID, which resulted in
	     * small integers being kept in the stream list instead of
	     * valid stream pointers.  Not sure if that still happens.
	     */
	    panic( "Stream Ptr # %d was an int %d!\n",
				streamID, streamPtr);
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
    register ReturnStatus 	status;
    Fs_Stream 			*streamPtr;
    Proc_ControlBlock		*procPtr;
    register Fs_ProcessState	*fsPtr;

    if (newStreamIDPtr == (int *)NIL) {
	return(FS_INVALID_ARG);
    }
    procPtr = Proc_GetEffectiveProc();
    status = Fs_GetStreamPtr(procPtr, streamID, &streamPtr);
    if (status != SUCCESS) {
	return(status);
    }
    fsPtr = procPtr->fsPtr;
    if (*newStreamIDPtr == FS_ANYID) {
	Fs_Stream		*newStreamPtr;

	Fsio_StreamCopy(streamPtr, &newStreamPtr);
	status = Fs_GetStreamID(newStreamPtr, newStreamIDPtr);
	if (status != SUCCESS) {
	    (void)Fs_Close(newStreamPtr);
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
	    if (newStreamID >= fsPtr->numStreams) {
		register int maxID;
		/*
		 * Need to grow the file list to accomodate this stream ID.
		 * We do a sanity check on the value of stream ID so
		 * we don't nuke ourselves with a huge array.
		 */
		maxID = 2 * fsPtr->numStreams;
		maxID = (maxID<128 ? 128 : maxID);
		if (newStreamID > maxID) {
		    return(FS_NEW_ID_TOO_BIG);
		}
		status = GrowStreamList(fsPtr, newStreamID + 1 );
		if (status != SUCCESS) {
		    return(status);
		}
	    } else {
		/*
		 * Check to see if *newStreamIDPtr is a valid (Fs_Stream *)
		 * and close it if it is.
		 */
		register Fs_Stream *oldFilePtr;

		oldFilePtr = fsPtr->streamList[newStreamID];
		if (oldFilePtr != (Fs_Stream *)NIL) {
		    (void)Fs_Close(oldFilePtr);
		}
	    }
	    Fsio_StreamCopy(streamPtr, &fsPtr->streamList[newStreamID]);
	    return(SUCCESS);
	}
    }
}
