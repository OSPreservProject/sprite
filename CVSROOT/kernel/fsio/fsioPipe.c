/*
 * fsPipe.c --
 *
 *	Routines for unnamed pipes.  An unnamed pipe has a fixed length
 *	resident buffer, a reading stream, and a writing stream.
 *	Process migration can result in remotely accessed pipes.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
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
#include <fsrmt.h>
#include <vm.h>
#include <proc.h>
#include <rpc.h>
#include <fsioPipe.h>

#include <stdio.h>

/*
 * Monitor to synchronize access to the openInstance in GetFileID.
 */
static	Sync_Lock	pipeLock = Sync_LockInitStatic("Fs:pipeLock");
#define	LOCKPTR	&pipeLock

/*
 * Forward references.
 */
static void GetFileID _ARGS_((Fs_FileID *fileIDPtr));
extern Fsio_PipeIOHandle *Fsio_PipeHandleInit _ARGS_((Fs_FileID *fileIDPtr, 
		Boolean findIt));
static ReturnStatus PipeCloseInt _ARGS_((Fsio_PipeIOHandle *handlePtr, 
		int ref, int write, Boolean release));

/*
 * Migration debugging.
 */
#ifdef MIG_DEBUG

#define PIPE_CREATED(inStreamPtr, outStreamPtr) \
    { \
	printf("Create Pipe: Srvr %d Read <%d> Write <%d> I/O %d <%d,%d>\n", \
		(inStreamPtr)->hdr.fileID.serverID, \
		(inStreamPtr)->hdr.fileID.minor, \
		(outStreamPtr)->hdr.fileID.minor, \
		(inStreamPtr)->ioHandlePtr->fileID.serverID, \
		(inStreamPtr)->ioHandlePtr->fileID.major, \
		(inStreamPtr)->ioHandlePtr->fileID.minor); \
    }

#define PIPE_CLOSE(streamPtr, handlePtr) \
	printf("Pipe Close: Stream %s <%d> I/O <%d,%d> ref %d write %d flags %x\n", \
		((streamPtr)->flags & FS_READ) ? "Read" : "Write", \
		(streamPtr)->hdr.fileID.minor, \
		(handlePtr)->hdr.fileID.major, (handlePtr)->hdr.fileID.minor, \
		(handlePtr)->use.ref, (handlePtr)->use.write, \
		(handlePtr)->flags)

#define PIPE_MIG_1(migInfoPtr, dstClientID) \
	printf("Pipe Migrate: %d => %d Stream %d <%d> I/O <%d,%d> migFlags %x ", \
	    (migInfoPtr)->srcClientID, dstClientID, \
	    (migInfoPtr)->streamID.serverID, (migInfoPtr)->streamID.minor, \
	    (migInfoPtr)->ioFileID.major, (migInfoPtr)->ioFileID.minor, \
	    (migInfoPtr)->flags);

#define PIPE_MIG_2(migInfoPtr, closeSrcClient, handlePtr) \
	printf("=> %x\n    closeSrc %d (ref %d write %d", (migInfoPtr)->flags, \
		closeSrcClient, (handlePtr)->use.ref, (handlePtr)->use.write);

#define PIPE_MIG_3(handlePtr) \
	printf(" | ref %d write %d)\n", \
		(handlePtr)->use.ref, (handlePtr)->use.write);

#define PIPE_MIG_END(handlePtr) \
	printf("PipeMigEnd: I/O <%d,%d> ref %d write %d\n", \
	    (handlePtr)->hdr.fileID.major, (handlePtr)->hdr.fileID.minor, \
	    (handlePtr)->use.ref, (handlePtr)->use.write)
#else

#define PIPE_CREATED(inStreamPtr, outStreamPtr)
#define PIPE_CLOSE(streamPtr, handlePtr)
#define PIPE_MIG_1(migInfoPtr, dstClientID)
#define PIPE_MIG_2(migInfoPtr, closeSrcClient, handlePtr)
#define PIPE_MIG_3(handlePtr)
#define PIPE_MIG_END(handlePtr)

#endif /* MIG_DEBUG */

/*
 *----------------------------------------------------------------------
 *
 * Fsio_CreatePipe --
 *
 *      Create an unnamed pipe.  Pointers to streams for both ends of the pipe
 *	are returned in *inStreamPtrPtr and *outStreamPtrPtr.
 *
 * Results:
 *	SUCCESS.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Fsio_CreatePipe(inStreamPtrPtr, outStreamPtrPtr)
    Fs_Stream **inStreamPtrPtr;		/* Return - in (reading) stream */
    Fs_Stream **outStreamPtrPtr;	/* Return - out (writing) stream */
{
    Fs_FileID		fileID;
    register Fsio_PipeIOHandle	*handlePtr;
    register Fs_Stream		*streamPtr;

    /*
     * Set up the I/O handle for the pipe.  The installation puts
     * one reference on the I/O handle.
     */

    GetFileID(&fileID);
    handlePtr = Fsio_PipeHandleInit(&fileID, FALSE);
    (void)Fsconsist_IOClientOpen(&handlePtr->clientList, rpc_SpriteID, FS_READ, FALSE);
    (void)Fsconsist_IOClientOpen(&handlePtr->clientList, rpc_SpriteID, FS_WRITE, FALSE);

    /*
     * Allocate and initialize the read, or "in", end of the stream.
     */
    streamPtr = Fsio_StreamCreate(rpc_SpriteID, rpc_SpriteID,
			    (Fs_HandleHeader *)handlePtr,
			    FS_READ | FS_USER, "read-pipe");
    Fsutil_HandleUnlock(streamPtr);
    *inStreamPtrPtr = streamPtr;

    /*
     * Set up the writing end.  Note that we get a second reference to
     * the I/O handle by duping it.
     */
    Fsutil_HandleUnlock(handlePtr);
    (void)Fsutil_HandleDup((Fs_HandleHeader *)handlePtr);
    streamPtr = Fsio_StreamCreate(rpc_SpriteID, rpc_SpriteID,
			(Fs_HandleHeader *)handlePtr,
			FS_WRITE | FS_APPEND | FS_USER, "write-pipe");
    Fsutil_HandleUnlock(handlePtr);
    Fsutil_HandleUnlock(streamPtr);
    *outStreamPtrPtr = streamPtr;

    PIPE_CREATED(*inStreamPtrPtr, *outStreamPtrPtr);

    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * GetFileID --
 *
 *      Get a unique file ID for the pipe.  This is a monitor to synchronize
 *	access to the openInstance variable that gives us unique file
 *	ids for unnamed pipes.
 *
 * Results:
 *	Unique file ID.
 *
 * Side effects:
 *	Open instance incremented.
 *
 *----------------------------------------------------------------------
 */

static ENTRY void
GetFileID(fileIDPtr)
    Fs_FileID	*fileIDPtr;
{
    static int openInstance = 0;

    LOCK_MONITOR;

    fileIDPtr->type = FSIO_LCL_PIPE_STREAM;
    fileIDPtr->serverID = rpc_SpriteID;
    fileIDPtr->major = 0;
    fileIDPtr->minor = openInstance;
    openInstance++;

    UNLOCK_MONITOR;
}

/*
 *----------------------------------------------------------------------
 *
 * Fsio_PipeHandleInit --
 *
 *	Initialize a handle for a pipe.
 *
 * Results:
 *	A pointer to the pipe handle.
 *
 * Side effects:
 *	Create and install a handle for the file.  It is returned locked
 *	and with its reference count incremented if SUCCESS is returned.
 *
 *----------------------------------------------------------------------
 */
Fsio_PipeIOHandle *
Fsio_PipeHandleInit(fileIDPtr, findIt)
    Fs_FileID	*fileIDPtr;	/* Pipe file ID */
    Boolean	findIt;		/* TRUE if we expect to find its handle */
{
    Fs_HandleHeader *hdrPtr;
    register Fsio_PipeIOHandle *handlePtr;
    register Boolean found;

    found = Fsutil_HandleInstall(fileIDPtr, sizeof(Fsio_PipeIOHandle), "pipe",
		FALSE, &hdrPtr);
    handlePtr = (Fsio_PipeIOHandle *)hdrPtr;
    if (!found) {
	if (findIt) {
	    panic( "Fsio_PipeHandleInit, didn't find handle\n");
	}
	/*
	 * When a pipe is created, it has one read and one write
	 * reference on the handle.
	 */
	handlePtr->use.ref = 2;
	handlePtr->use.write = 1;
	handlePtr->use.exec = 0;
	List_Init(&handlePtr->clientList);
	handlePtr->flags = 0;
	handlePtr->firstByte = handlePtr->lastByte = -1;
	handlePtr->buffer = (Address)malloc(FS_BLOCK_SIZE);
	handlePtr->bufSize = FS_BLOCK_SIZE;
	List_Init(&handlePtr->readWaitList);
	List_Init(&handlePtr->writeWaitList);
	fs_Stats.object.pipes++;
    }
    return(handlePtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsio_PipeClose --
 *
 *      Close a local pipe.  Other processes waiting on the pipe are
 *	unblocked and the pipe's buffer is freed when the last
 *	user goes away.
 *
 * Results:
 *      SUCCESS.
 *
 * Side effects:
 *      Unblock local waiting reader (or writer) waiting on the pipe.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
#ifndef SOSP91
ReturnStatus
Fsio_PipeClose(streamPtr, clientID, procID, flags, dataSize, closeData)
#else
ReturnStatus
Fsio_PipeClose(streamPtr, clientID, procID, flags, dataSize, closeData,
    offsetPtr, rwFlagsPtr)
#endif
    Fs_Stream		*streamPtr;	/* Stream to a pipe */
    int			clientID;	/* Host ID of closing process */
    Proc_PID		procID;		/* Process closing */
    int			flags;		/* Flags from the stream being closed */
    int			dataSize;	/* Should be 0 */
    ClientData		closeData;	/* Should be NIL */
#ifdef SOSP91
    int			*offsetPtr;	/* Not used. */
    int			*rwFlagsPtr;	/* Not used. */
#endif

{
    register Fsio_PipeIOHandle *handlePtr = 
	    (Fsio_PipeIOHandle *)streamPtr->ioHandlePtr;
    Boolean cache = FALSE;
    ReturnStatus status;

    if (!Fsconsist_IOClientClose(&handlePtr->clientList, clientID, flags, &cache)) {
	printf( "Fsio_PipeClose, unknown client %d\n", clientID);
	Fsutil_HandleUnlock(handlePtr);
    } else {
	PIPE_CLOSE(streamPtr, handlePtr);
	status = PipeCloseInt(handlePtr, 1, (flags & FS_WRITE) != 0, TRUE);
	if (status != FS_FILE_REMOVED) {
	    Fsutil_HandleRelease(handlePtr, TRUE);
	}
    }
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * PipeCloseInt --
 *
 *      Do the real work of closing a pipe, given a variable number
 *	of references. 
 *
 * Results:
 *      SUCCESS or FS_FILE_REMOVED, which indicates that the pipe has been
 * 	freed up.
 *
 * Side effects:
 *      Unblock local waiting reader (or writer) waiting on the pipe.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static ReturnStatus
PipeCloseInt(handlePtr, ref, write, release)
    Fsio_PipeIOHandle *handlePtr;	/* Pipe to clean up */
    int ref;				/* Number of uses to remove */
    int write;				/* Number of writers to remove */
    Boolean release;			/* Whether to release handle to
					   remove it. */
{
    /*
     * Update the global/summary use counts for the file.
     */
    handlePtr->use.ref -= ref;
    handlePtr->use.write -= write;
    if (handlePtr->use.ref < 0 || handlePtr->use.write < 0) {
	panic("PipeCloseInt <%d,%d> use %d, write %d\n",
	      handlePtr->hdr.fileID.major, handlePtr->hdr.fileID.minor,
	      handlePtr->use.ref, handlePtr->use.write);
    }
    if (write && handlePtr->use.write == 0) {
	/*
	 * Notify reader that the writer has closed.
	 */
	handlePtr->flags |= FSIO_PIPE_WRITER_GONE;
	Fsutil_FastWaitListNotify(&handlePtr->readWaitList);
    } else if (ref && handlePtr->use.ref == handlePtr->use.write) {
	/*
	 * Update state and notify any blocked writers.  Their write
	 * will fail with no remaining readers.
	 */
	handlePtr->flags |= FSIO_PIPE_READER_GONE;
	Fsutil_FastWaitListNotify(&handlePtr->writeWaitList);
    }
    if (handlePtr->flags == (FSIO_PIPE_WRITER_GONE|FSIO_PIPE_READER_GONE)) {
	free(handlePtr->buffer);
	Fsutil_WaitListDelete(&handlePtr->readWaitList);
	Fsutil_WaitListDelete(&handlePtr->writeWaitList);
	if (release) {
	    Fsutil_HandleRelease(handlePtr, FALSE);
	}
	Fsutil_HandleRemove(handlePtr);
	fs_Stats.object.pipes--;
	return (FS_FILE_REMOVED);
    }
    /*
     * Handle will be released or unlocked by caller as appropriate.
     */
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsio_PipeRead --
 *
 *      Read on a pipe.  Data is copied out of the pipe to satisfy the
 *	read.  If the pipe is empty this routine returns FS_WOULD_BLOCK.
 *	If the pipe writer is gone this returns SUCCESS and 0 bytes
 *	to simulate EOF.
 *
 * Results:
 *      SUCCESS unless there was an address error or I/O error.
 *
 * Side effects:
 *      Fill in the buffer.  Notifies the pipe's writeWaitList after
 *	removing data from the pipe.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
Fsio_PipeRead(streamPtr, readPtr, waitPtr, replyPtr)
    Fs_Stream           *streamPtr;     /* Stream to read from */
    Fs_IOParam		*readPtr;	/* Read parameter block. */
    Sync_RemoteWaiter	*waitPtr;	/* Process info for remote waiting */
    Fs_IOReply		*replyPtr;	/* Signal to return, if any,
					 * plus the amount read. */
{
    ReturnStatus 	status = SUCCESS;
    register Fsio_PipeIOHandle *handlePtr =
	    (Fsio_PipeIOHandle *)streamPtr->ioHandlePtr;
    int 		toRead;
    int 		startOffset;
    int			startByte;
    int			endByte;

    Fsutil_HandleLock(handlePtr);

    if (handlePtr->firstByte == -1) {
	/*
	 * No data in the pipe.  If there is no writer left then
	 * return SUCCESS so the user process thinks of it as end of file,
	 * otherwise block waiting for input.
	 */
	replyPtr->length = 0;
	if (handlePtr->flags & FSIO_PIPE_WRITER_GONE) {
	    status = SUCCESS;
	    goto exit;
	} else {
	    status = FS_WOULD_BLOCK;
	    goto exit;
	}
    }
    /*
     * Compute the number of bytes that we can read from the pipe.  
     */
    toRead = handlePtr->lastByte - handlePtr->firstByte + 1;
    if (toRead > readPtr->length) {
	toRead = readPtr->length;
    }

    /*
     * Compute offsets into the pipe buffer.  The offsets float out past
     * the size of the buffer, but we use masks to take of the top bits.
     * We can also compare these top bits to determine if the valid range
     * of the buffer wraps around the end of the pipe.
     */
    startByte = handlePtr->firstByte;
    startOffset = startByte & FS_BLOCK_OFFSET_MASK;
    endByte = handlePtr->firstByte + toRead - 1;

    if ((startByte & ~FS_BLOCK_OFFSET_MASK) ==
	(endByte & ~FS_BLOCK_OFFSET_MASK)) {
	/*
	 * Can do a straight copy, no wrap around necessary.
	 */
	if (readPtr->flags & FS_USER) {
	    if (Vm_CopyOut(toRead, handlePtr->buffer + startOffset, readPtr->buffer)
			  != SUCCESS) {
		status = SYS_ARG_NOACCESS;
	    }
	} else {
	    bcopy(handlePtr->buffer + startOffset, readPtr->buffer, toRead);
	}
    } else {
	int	numBytes;
	/*
	 * Have to wrap around in the block so do it in two copies.
	 */
	numBytes = FS_BLOCK_SIZE - startOffset;
	if (readPtr->flags & FS_USER) {
	    if (Vm_CopyOut(numBytes, handlePtr->buffer + startOffset, readPtr->buffer)
			  != SUCCESS) {
		status = SYS_ARG_NOACCESS;
	    } else if (Vm_CopyOut(toRead - numBytes, handlePtr->buffer,
				readPtr->buffer + numBytes) != SUCCESS) {
		status = SYS_ARG_NOACCESS;
	    }
	} else {
	    bcopy(handlePtr->buffer + startOffset, readPtr->buffer, numBytes);
	    bcopy(handlePtr->buffer, readPtr->buffer + numBytes, toRead - numBytes);
	}
    }

    /*
     * We just made space in the pipe so wake up blocked writers.
     */
    Fsutil_FastWaitListNotify(&handlePtr->writeWaitList);

    /*
     * Update the first byte and the parameters.
     */
    handlePtr->firstByte += toRead;
    if (handlePtr->firstByte > handlePtr->lastByte) {
	handlePtr->firstByte = -1;
	handlePtr->lastByte = -1;
    }
    replyPtr->length = toRead;
exit:
    if (status == FS_WOULD_BLOCK) {
	Fsutil_FastWaitListInsert(&handlePtr->readWaitList, waitPtr);
    }
    Fsutil_HandleUnlock(handlePtr);
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * Fsio_PipeWrite --
 *
 *      Write on a pipe.  This will put as much data as possible into the
 *	pipe buffer, and then block the process (return would-block) if
 *	there is any left over data.  *lenPtr is updated to reflect how
 *	much data was written to the pipe.
 *
 * Results:
 *      SUCCESS unless there was an address error or I/O error.
 *
 * Side effects:
 *      Fill the pipe from the buffer.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Fsio_PipeWrite(streamPtr, writePtr, waitPtr, replyPtr)
    Fs_Stream           *streamPtr;     /* Stream to write to */
    Fs_IOParam		*writePtr;	/* Read parameter block */
    Sync_RemoteWaiter	*waitPtr;	/* Process info for remote waiting */
    Fs_IOReply		*replyPtr;	/* Signal to return, if any */
{
    register ReturnStatus 	status = SUCCESS;
    register Fsio_PipeIOHandle	*handlePtr =
	    (Fsio_PipeIOHandle *)streamPtr->ioHandlePtr;
    int 			startOffset;
    register int 		toWrite;
    int				startByte;
    int				endByte;

    Fsutil_HandleLock(handlePtr);

    if (handlePtr->flags & FSIO_PIPE_READER_GONE) {
	replyPtr->length = 0;
	replyPtr->signal = SIG_PIPE;
	status = FS_BROKEN_PIPE;
	goto exit;
    }

    /*
     * Compute the number of bytes that will fit in the pipe.  Remember
     * that the pipe can only hold one block of data.
     */
    if (handlePtr->firstByte == -1) {
	toWrite = FS_BLOCK_SIZE;
    } else {
	toWrite = FS_BLOCK_SIZE - 
		    (handlePtr->lastByte - handlePtr->firstByte + 1);
    }
    if (toWrite == 0) {
	/*
	 * No room in the pipe.
	 */
	replyPtr->length = 0;
	status = FS_WOULD_BLOCK;
	goto exit;
    } else if (toWrite > writePtr->length) {
	toWrite = writePtr->length;
    } else if (writePtr->length > toWrite) {
	/*
	 * If there is more data to write we must block after writing the
	 * data that we can.  
	 */
	status = FS_WOULD_BLOCK;
    }
    /*
     * Determine where to start and stop writing.  Note that the firstByte
     * and lastByte offsets float out beyond the size of the pipe but
     * we use masks to clear off the extra high order bits.
     */
    startByte = handlePtr->lastByte + 1;
    startOffset = startByte & FS_BLOCK_OFFSET_MASK;
    endByte = handlePtr->lastByte + toWrite;

    if ((startByte & ~FS_BLOCK_OFFSET_MASK) ==
	(endByte & ~FS_BLOCK_OFFSET_MASK)) {
	/*
	 * Can do a straight copy, no wrap around necessary.
	 */
	if (writePtr->flags & FS_USER) {
	    if (Vm_CopyIn(toWrite, writePtr->buffer, handlePtr->buffer + startOffset)
			  != SUCCESS) {
		status = SYS_ARG_NOACCESS;
	    }
	} else {
	    bcopy(writePtr->buffer, handlePtr->buffer + startOffset, toWrite);
	}
    } else {
	int	numBytes;
	/*
	 * Have to wrap around in the block so do it in two copies.
	 */
	numBytes = FS_BLOCK_SIZE - startOffset;
	if (writePtr->flags & FS_USER) {
	    if (Vm_CopyIn(numBytes, writePtr->buffer, handlePtr->buffer + startOffset)
			  != SUCCESS) {
		status = SYS_ARG_NOACCESS;
	    } else if (Vm_CopyIn(toWrite - numBytes, writePtr->buffer + numBytes, 
			    handlePtr->buffer) != SUCCESS) {
		status = SYS_ARG_NOACCESS;
	    }
	} else {
	    bcopy(writePtr->buffer, handlePtr->buffer + startOffset, numBytes);
	    bcopy(writePtr->buffer + numBytes, handlePtr->buffer, toWrite - numBytes);
	}
    }

    /*
     * We just wrote to the pipe so wake up blocked readers.
     */
    Fsutil_FastWaitListNotify(&handlePtr->readWaitList);

    /*
     * Update first byte, last byte and the parameters.
     */
    if (handlePtr->firstByte == -1) {
	handlePtr->firstByte = 0;
    }
    handlePtr->lastByte = endByte;
    replyPtr->length = toWrite;
exit:
    if (status == FS_WOULD_BLOCK) {
	Fsutil_FastWaitListInsert(&handlePtr->writeWaitList, waitPtr);
    }
    Fsutil_HandleUnlock(handlePtr);
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * Fsio_PipeIOControl --
 *
 *	Check the IOC_CLEAR_FLAG control, and allow the IOC_SET_FLAG control.
 *
 * Results:
 *      SUCCESS unless they try to clear the FS_APPEND flag.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
Fsio_PipeIOControl(streamPtr, ioctlPtr, replyPtr)
    Fs_Stream *streamPtr;
    Fs_IOCParam *ioctlPtr;		/* I/O Control parameter block */
    Fs_IOReply *replyPtr;		/* Return length and signal */
{
    register Fsio_PipeIOHandle *handlePtr =
	    (Fsio_PipeIOHandle *)streamPtr->ioHandlePtr;
    register int command = ioctlPtr->command;
    ReturnStatus status = SUCCESS;

    switch(command) {
	case IOC_REPOSITION:
	    return(FS_BAD_SEEK);
	case IOC_GET_FLAGS:
	case IOC_SET_BITS:
	    return(SUCCESS);
	case IOC_CLEAR_BITS:
	case IOC_SET_FLAGS: {
	    /*
	     * Guard against turning off append mode in pipes.
	     */
	    int flags;
	    int size;
	    int inSize;

	    if (ioctlPtr->inBufSize != sizeof(int)) {
		status = GEN_INVALID_ARG;
	    } else if (ioctlPtr->format != mach_Format) {
		int fmtStatus;
		size = sizeof(int);
		inSize = ioctlPtr->inBufSize;
		fmtStatus = Fmt_Convert("w", ioctlPtr->format, &inSize,
				ioctlPtr->inBuffer, mach_Format, &size,
				(Address) &flags);
		if (fmtStatus != 0) {
		    printf("Format of ioctl failed <0x%x>\n", fmtStatus);
		    status = GEN_INVALID_ARG;
		}
		if (size != sizeof(int)) {
		    status = GEN_INVALID_ARG;
		}
	    } else {
		flags = *(int *)ioctlPtr->inBuffer;
	    }
	    if (status != SUCCESS) {
		return(status);
	    }
	    if ((command == IOC_SET_FLAGS && (flags & IOC_APPEND) == 0) ||
		(command == IOC_CLEAR_BITS && (flags & IOC_APPEND))) {
		return(GEN_INVALID_ARG);
	    }
	    return(SUCCESS);
	}
	case IOC_TRUNCATE:
	case IOC_LOCK:
	case IOC_UNLOCK:
	    return(GEN_NOT_IMPLEMENTED);
	case IOC_NUM_READABLE: {
	    int bytesAvailable = handlePtr->lastByte - handlePtr->firstByte;

	    if (ioctlPtr->outBufSize != sizeof(int)) {
		status = GEN_INVALID_ARG;
	    } else if (ioctlPtr->format != mach_Format) {
		int size = sizeof(int);
		int inSize = sizeof(int);
		int fmtStatus;
		fmtStatus = Fmt_Convert("w", mach_Format, &inSize,
				(Address) &bytesAvailable, ioctlPtr->format, 
				&size, (Address) ioctlPtr->outBuffer);
		if (fmtStatus != 0) {
		    printf("Format of ioctl failed <0x%x>\n", fmtStatus);
		    status = GEN_INVALID_ARG;
		}
		if (size != sizeof(int)) {
		    status = GEN_INVALID_ARG;
		}
	    } else {
		*(int *)ioctlPtr->outBuffer = bytesAvailable;
	    }
	    return(status);
	}
	case IOC_GET_OWNER:
	case IOC_SET_OWNER:
	case IOC_MAP:
	case IOC_PREFIX:
	    return(GEN_NOT_IMPLEMENTED);
	default:
	    return(GEN_INVALID_ARG);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Fsio_PipeSelect --
 *
 *	Select on a pipe.
 *
 * Results:
 *      SUCCESS unless there was an address error or I/O error.
 *
 * Side effects:
 *      Fill the pipe from the buffer.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Fsio_PipeSelect(hdrPtr, waitPtr, readPtr, writePtr, exceptPtr)
    Fs_HandleHeader *hdrPtr;	/* The handle of the file */
    Sync_RemoteWaiter *waitPtr;	/* Process info for waiting */
    int		*readPtr;	/* Read bit */
    int		*writePtr;	/* Write bit */
    int		*exceptPtr;	/* Exception bit */
{
    register Fsio_PipeIOHandle *handlePtr = (Fsio_PipeIOHandle *)hdrPtr;

    Fsutil_HandleLock(hdrPtr);
    *exceptPtr = 0;
    if (*writePtr) {
	/*
	 * Turn off writability if the pipe is full and there
	 * are still readers.  If there are no readers we allow
	 * writing but the next write will fail.
	 */
	if ((((handlePtr->firstByte + 1) % FS_BLOCK_SIZE) ==
	     handlePtr->lastByte) && 
	     ((handlePtr->flags & FSIO_PIPE_READER_GONE) == 0)) {
	    if (waitPtr != (Sync_RemoteWaiter *)NIL) {
		Fsutil_FastWaitListInsert(&handlePtr->writeWaitList, waitPtr);
	    }
	    *writePtr = 0;
	}
    }
    if (*readPtr) {
	/*
	 * If there's data to be read then set the READABLE bit.
	 * If all the writers have died, then this process will never be 
	 * woken up again so lie and say the pipe is readble. The process
	 * discover there's no writer when it tries to read the pipe.
	 */
	if ((handlePtr->firstByte == -1) &&
	    ((handlePtr->flags & FSIO_PIPE_WRITER_GONE) == 0)) {
	    *readPtr = 0;
	    if (waitPtr != (Sync_RemoteWaiter *)NIL) {
		Fsutil_FastWaitListInsert(&handlePtr->readWaitList, waitPtr);
	    }
        }
    }
    Fsutil_HandleUnlock(hdrPtr);
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsio_PipeGetIOAttr --
 *
 *	Get the most up-to-date I/O attributes for a pipe.
 *
 * Results:
 *	SUCCESS
 *
 * Side effects:
 *      Updates the first and last byte indexes of the handle.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
Fsio_PipeGetIOAttr(fileIDPtr, clientID, attrPtr)
    Fs_FileID			*fileIDPtr;	/* FileID of pipe */
    int 			clientID;	/* IGNORED */
    register Fs_Attributes	*attrPtr;	/* Attributes to update */
{
    Fsio_PipeIOHandle *handlePtr;

    handlePtr = Fsutil_HandleFetchType(Fsio_PipeIOHandle, fileIDPtr);
    if (handlePtr != (Fsio_PipeIOHandle *)NIL) {
	attrPtr->serverID	= fileIDPtr->serverID;
	attrPtr->domain		= fileIDPtr->major;
	attrPtr->fileNumber	= fileIDPtr->minor;
	attrPtr->type		= FS_LOCAL_PIPE;
	attrPtr->size		= handlePtr->lastByte - handlePtr->firstByte +1;
	attrPtr->devServerID	= fileIDPtr->serverID;
	Fsutil_HandleRelease(handlePtr, TRUE);
    }
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsio_PipeSetIOAttr --
 *
 *	Set the I/O attributes for a pipe.  This doesn't do anything.
 *
 * Results:
 *	SUCCESS
 *
 * Side effects:
 *      Updates the first and last byte indexes of the handle.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
Fsio_PipeSetIOAttr(fileIDPtr, attrPtr, flags)
    Fs_FileID		*fileIDPtr;	/* FileID of pipe */
    Fs_Attributes	*attrPtr;	/* Attributes to update */
    int			flags;		/* What attrs to set */
{
    return(SUCCESS);
}

/*
 * ----------------------------------------------------------------------------
 *
 * Fsio_PipeMigClose --
 *
 *	Release a reference on a Pipe I/O handle.
 *	
 * Results:
 *	SUCCESS.
 *
 * Side effects:
 *	Release the I/O handle.
 *
 * ----------------------------------------------------------------------------
 *
 */
/*ARGSUSED*/
ReturnStatus
Fsio_PipeMigClose(hdrPtr, flags)
    Fs_HandleHeader *hdrPtr;	/* File being released */
    int flags;			/* Use flags from the stream */
{
    panic("Fsio_PipeMigClose called\n");
    Fsutil_HandleRelease(hdrPtr, FALSE);
    return(SUCCESS);
}

/*
 * ----------------------------------------------------------------------------
 *
 * Fsio_PipeMigrate --
 *
 *	This takes care of transfering references from one client to the other.
 *	A useful side-effect of this routine is	to properly set the type in
 *	the ioFileID, either FSIO_LCL_PIPE_STREAM or FSIO_RMT_PIPE_STREAM.
 *	In the latter case FsrmtPipeMigrate is called to do all the work.
 *
 * Results:
 *	An error status if the I/O handle can't be set-up.
 *	Otherwise SUCCESS is returned, *flagsPtr may have the FS_RMT_SHARED
 *	bit set, and *sizePtr and *dataPtr are set to reference Fsio_DeviceState.
 *
 * Side effects:
 *	Sets the correct stream type on the ioFileID.
 *	Shifts client references from the srcClient to the destClient.
 *
 * ----------------------------------------------------------------------------
 *
 */
/*ARGSUSED*/
ReturnStatus
Fsio_PipeMigrate(migInfoPtr, dstClientID, flagsPtr, offsetPtr, sizePtr, dataPtr)
    Fsio_MigInfo	*migInfoPtr;	/* Migration state */
    int		dstClientID;	/* ID of target client */
    int		*flagsPtr;	/* In/Out Stream usage flags */
    int		*offsetPtr;	/* Return - new stream offset (not needed) */
    int		*sizePtr;	/* Return - sizeof(Fsio_DeviceState) */
    Address	*dataPtr;	/* Return - pointer to Fsio_DeviceState */
{
    Fsio_PipeIOHandle			*handlePtr;
    Boolean				closeSrcClient;

    if (migInfoPtr->ioFileID.serverID != rpc_SpriteID) {
	/*
	 * The pipe was local, which is usually true, but is now remote.
	 */
	migInfoPtr->ioFileID.type = FSIO_RMT_PIPE_STREAM;
	return(FsrmtPipeMigrate(migInfoPtr, dstClientID, flagsPtr, offsetPtr,
		sizePtr, dataPtr));
    }
    migInfoPtr->ioFileID.type = FSIO_LCL_PIPE_STREAM;
    handlePtr = Fsio_PipeHandleInit(&migInfoPtr->ioFileID, TRUE);

    PIPE_MIG_1(migInfoPtr, dstClientID);
    /*
     * At the stream level, add the new client to the set of clients
     * for the stream, and check for any cross-network stream sharing.
     */
    Fsio_StreamMigClient(migInfoPtr, dstClientID, (Fs_HandleHeader *)handlePtr,
		    &closeSrcClient);
    PIPE_MIG_2(migInfoPtr, closeSrcClient, handlePtr);

    /*
     * Adjust use counts on the I/O handle to reflect any new sharing.
     */
    Fsio_MigrateUseCounts(migInfoPtr->flags, closeSrcClient, &handlePtr->use);
    PIPE_MIG_3(handlePtr);

    /*
     * Move the client at the I/O handle level.
     */
    Fsio_MigrateClient(&handlePtr->clientList, migInfoPtr->srcClientID,
			dstClientID, migInfoPtr->flags, closeSrcClient);

    *sizePtr = 0;
    *dataPtr = (Address)NIL;
    *flagsPtr = migInfoPtr->flags;
    *offsetPtr = migInfoPtr->offset;
    /*
     * We don't need this reference on the I/O handle; there is no change.
     */
    Fsutil_HandleRelease(handlePtr, TRUE);
    return(SUCCESS);
}

/*
 * ----------------------------------------------------------------------------
 *
 * Fsio_PipeMigOpen --
 *
 *	Complete setup of a FSIO_LCL_PIPE_STREAM after migration (back) to the
 *	pipe I/O server.  Fsio_PipeMigrate has done the work of shifting use
 *	counts at the stream and I/O handle level.  This routine has to
 *	increment the low level reference count on the pipe I/O handle
 *	to reflect the existence of a new stream to the I/O handle.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 *
 */
/*ARGSUSED*/
ReturnStatus
Fsio_PipeMigOpen(migInfoPtr, size, data, hdrPtrPtr)
    Fsio_MigInfo	*migInfoPtr;	/* Migration state */
    int		size;		/* Zero */
    ClientData	data;		/* NIL */
    Fs_HandleHeader **hdrPtrPtr;	/* Return - handle for the file */
{
    register Fsio_PipeIOHandle *handlePtr;

    handlePtr = Fsutil_HandleFetchType(Fsio_PipeIOHandle, &migInfoPtr->ioFileID);
    if (handlePtr == (Fsio_PipeIOHandle *)NIL) {
	panic( "Fsio_PipeMigOpen, no handle\n");
	return(FAILURE);
    } else {
	PIPE_MIG_END(handlePtr);
	Fsutil_HandleUnlock(handlePtr);
	*hdrPtrPtr = (Fs_HandleHeader *)handlePtr;
	return(SUCCESS);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Fsio_PipeReopen --
 *
 *	Reopen a pipe for a client.  This executed on a server and will
 *	only work if it was a network partition that made us forget
 *	about the client.  If we've already removed the pipe handle
 *	then the clien't won't be able to recover.
 *
 * Results:
 *	A  non-SUCCESS return code if the re-open failed.
 *
 * Side effects:
 *	Adds the client to the set of client's for the pipe.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
Fsio_PipeReopen(hdrPtr, clientID, inData, outSizePtr, outDataPtr)
    Fs_HandleHeader	*hdrPtr;		/* NIL */
    int			clientID;		/* Host ID of client */
    ClientData		inData;			/* sizeof Fsio_PipeReopenParams */
    int			*outSizePtr;		/* Returns 0 */
    ClientData		*outDataPtr;		/* Returns NIL */
{
    register Fsio_PipeIOHandle	*handlePtr;
    ReturnStatus		status;
    Fsio_PipeReopenParams		*reopenParamsPtr;

    reopenParamsPtr = (Fsio_PipeReopenParams *)inData;
    handlePtr = Fsutil_HandleFetchType(Fsio_PipeIOHandle, &reopenParamsPtr->fileID);
    if (handlePtr == (Fsio_PipeIOHandle *)NIL) {
	status = FAILURE;
    } else {
	Fsconsist_IOClientStatus(&handlePtr->clientList, clientID,
				 &reopenParamsPtr->use);
	(void)Fsconsist_IOClientReopen(&handlePtr->clientList, clientID,
				 &reopenParamsPtr->use);
	handlePtr->use.ref += reopenParamsPtr->use.ref;
	handlePtr->use.write += reopenParamsPtr->use.write;
	Fsutil_HandleRelease(handlePtr, TRUE);
	status = SUCCESS;
    }
    *outSizePtr = 0;
    *outDataPtr = (ClientData)NIL;
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fsio_PipeClientKill --
 *
 *	Clean up after a crashed client.  Note, this doesn't handle the
 *	obscure case of one end of a pipe being duplicated and then
 *	one of the sharers migrating to a host which crashes.  In this
 *	funny case we'll wrongly think that all users of one end of
 *	the pipe have crashed.
 *
 * Results:
 *      SUCCESS.
 *
 * Side effects:
 *      Removes the client (if applicable) from the pipe's set of clients.
 *	This will cause a FS_BROKEN_PIPE error unless the pipe is re-opened,
 *	which might happen in a network partition.  This unlocks the handle.
 *
 *----------------------------------------------------------------------
 */
void
Fsio_PipeClientKill(hdrPtr, clientID)
    Fs_HandleHeader *hdrPtr;     /* Handle to clean up */
    int clientID;		/* Host assumed down */
{
    register Fsio_PipeIOHandle *handlePtr = (Fsio_PipeIOHandle *)hdrPtr;
    int refs, writes, execs;
    register ReturnStatus status;

    Fsconsist_IOClientKill(&handlePtr->clientList, clientID, &refs, &writes, &execs);
    status = PipeCloseInt(handlePtr, refs, writes, FALSE);
    if (status != FS_FILE_REMOVED) {
	Fsutil_HandleUnlock(handlePtr);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * Fsio_PipeScavenge --
 *
 *	Scavenge a pipe.  The handle might be usless if all its
 *	client's have crashed.
 *
 * Results:
 *      TRUE if the handle was removed.
 *
 * Side effects:
 *      Unlocks the pipe's handle.
 *
 *----------------------------------------------------------------------
 */
Boolean
Fsio_PipeScavenge(hdrPtr)
    Fs_HandleHeader *hdrPtr;     /* Handle about to be deleted */
{
#ifdef notdef
    register Fsio_PipeIOHandle *handlePtr = (Fsio_PipeIOHandle *)hdrPtr;
    if (List_IsEmpty(&handlePtr->clientList) &&
	(handlePtr->flags == (FSIO_PIPE_WRITER_GONE|FSIO_PIPE_READER_GONE))) {
	/*
	 * Never scavenge pipe handles.  Regular recovery cleanup
	 * should invoke the pipe close routines and do proper cleanup.
	 */
	free(handlePtr->buffer);
	Fsutil_WaitListDelete(&handlePtr->readWaitList);
	Fsutil_WaitListDelete(&handlePtr->writeWaitList);
	Fsutil_HandleRemove(hdrPtr);
	fs_Stats.object.pipes--;
	return(TRUE);
    }
#endif notdef
    Fsutil_HandleUnlock(hdrPtr);
    return(FALSE);
}


/*
 *----------------------------------------------------------------------
 *
 * Fsio_PipeRecovTestUseCount --
 *
 *	For recovery testing, return the use count on the io handle.
 *
 * Results:
 *      The use count.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */
int
Fsio_PipeRecovTestUseCount(handlePtr)
    Fsio_PipeIOHandle *handlePtr;
{
    return handlePtr->use.ref;
}
