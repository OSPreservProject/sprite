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


#include "sprite.h"

#include "fs.h"
#include "fsInt.h"
#include "fsPipe.h"
#include "fsOpTable.h"
#include "fsStream.h"
#include "fsMigrate.h"
#include "fsRecovery.h"
#include "fsClient.h"
#include "fsStat.h"

#include "vm.h"
#include "proc.h"
#include "rpc.h"
#include "swapBuffer.h"

/*
 * Monitor to synchronize access to the openInstance in GetFileID.
 */
static	Sync_Lock	pipeLock = Sync_LockInitStatic("Fs:pipeLock");
#define	LOCKPTR	&pipeLock

/*
 * Forward references.
 */
void GetFileID();
FsPipeIOHandle *FsPipeHandleInit();


/*
 *----------------------------------------------------------------------
 *
 * Fs_CreatePipe --
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
Fs_CreatePipe(inStreamPtrPtr, outStreamPtrPtr)
    Fs_Stream **inStreamPtrPtr;		/* Return - in (reading) stream */
    Fs_Stream **outStreamPtrPtr;	/* Return - out (writing) stream */
{
    Fs_FileID		fileID;
    register FsPipeIOHandle	*handlePtr;
    register Fs_Stream		*streamPtr;

    /*
     * Set up the I/O handle for the pipe.  The installation puts
     * one reference on the I/O handle.
     */

    GetFileID(&fileID);
    handlePtr = FsPipeHandleInit(&fileID, FALSE);
    (void)FsIOClientOpen(&handlePtr->clientList, rpc_SpriteID, FS_READ, FALSE);
    (void)FsIOClientOpen(&handlePtr->clientList, rpc_SpriteID, FS_WRITE, FALSE);

    /*
     * Allocate and initialize the read, or "in", end of the stream.
     */
    streamPtr = FsStreamNewClient(rpc_SpriteID, rpc_SpriteID,
			    (FsHandleHeader *)handlePtr,
			    FS_READ | FS_CONSUME | FS_USER, "read-pipe");
    FsHandleUnlock(streamPtr);
    *inStreamPtrPtr = streamPtr;

    /*
     * Set up the writing end.  Note that we get a second reference to
     * the I/O handle by duping it.
     */
    FsHandleUnlock(handlePtr);
    (void)FsHandleDup((FsHandleHeader *)handlePtr);
    streamPtr = FsStreamNewClient(rpc_SpriteID, rpc_SpriteID,
			(FsHandleHeader *)handlePtr,
			FS_WRITE | FS_APPEND | FS_USER, "write-pipe");
    FsHandleUnlock(handlePtr);
    FsHandleUnlock(streamPtr);
    *outStreamPtrPtr = streamPtr;

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

ENTRY void
GetFileID(fileIDPtr)
    Fs_FileID	*fileIDPtr;
{
    static int openInstance = 0;

    LOCK_MONITOR;

    fileIDPtr->type = FS_LCL_PIPE_STREAM;
    fileIDPtr->serverID = rpc_SpriteID;
    fileIDPtr->major = 0;
    fileIDPtr->minor = openInstance;
    openInstance++;

    UNLOCK_MONITOR;
}

/*
 *----------------------------------------------------------------------
 *
 * FsPipeHandleInit --
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
FsPipeIOHandle *
FsPipeHandleInit(fileIDPtr, findIt)
    Fs_FileID	*fileIDPtr;	/* Pipe file ID */
    Boolean	findIt;		/* TRUE if we expect to find its handle */
{
    FsHandleHeader *hdrPtr;
    register FsPipeIOHandle *handlePtr;
    register Boolean found;

    found = FsHandleInstall(fileIDPtr, sizeof(FsPipeIOHandle), "pipe", &hdrPtr);
    handlePtr = (FsPipeIOHandle *)hdrPtr;
    if (!found) {
	if (findIt) {
	    panic( "FsPipeHandleInit, didn't find handle\n");
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
	fsStats.object.pipes++;
    }
    return(handlePtr);
}

/*
 *----------------------------------------------------------------------
 *
 * FsPipeClose --
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
ReturnStatus
FsPipeClose(streamPtr, clientID, procID, flags, dataSize, closeData)
    Fs_Stream		*streamPtr;	/* Stream to a pipe */
    int			clientID;	/* Host ID of closing process */
    Proc_PID		procID;		/* Process closing */
    int			flags;		/* Flags from the stream being closed */
    int			dataSize;	/* Should be 0 */
    ClientData		closeData;	/* Should be NIL */
{
    register FsPipeIOHandle *handlePtr = 
	    (FsPipeIOHandle *)streamPtr->ioHandlePtr;
    Boolean cache = FALSE;

    if (!FsIOClientClose(&handlePtr->clientList, clientID, flags, &cache)) {
	printf( "FsPipeClose, unknown client %d\n", clientID);
	FsHandleUnlock(handlePtr);
    } else {
	/*
	 * Update the global/summary use counts for the file.
	 */
	handlePtr->use.ref--;
	if (flags & FS_WRITE) {
	    handlePtr->use.write--;
	}
	if (handlePtr->use.ref < 0 || handlePtr->use.write < 0) {
	    panic(
		      "FsPipeClose <%d,%d> use %d, write %d\n",
		      handlePtr->hdr.fileID.major, handlePtr->hdr.fileID.minor,
		      handlePtr->use.ref, handlePtr->use.write);
	}
	if ((flags & FS_WRITE) && handlePtr->use.write == 0) {
	    /*
	     * Notify reader that the writer has closed.
	     */
	    handlePtr->flags |= PIPE_WRITER_GONE;
	    FsFastWaitListNotify(&handlePtr->readWaitList);
	} else if ((flags & FS_READ) &&
		    handlePtr->use.ref == handlePtr->use.write) {
	    /*
	     * Update state and notify any blocked writers.  Their write
	     * will fail with no remaining readers.
	     */
	    handlePtr->flags |= PIPE_READER_GONE;
	    FsFastWaitListNotify(&handlePtr->writeWaitList);
	}
	FsHandleRelease(handlePtr, TRUE);
	if (handlePtr->flags == (PIPE_WRITER_GONE|PIPE_READER_GONE)) {
	    free(handlePtr->buffer);
	    FsWaitListDelete(&handlePtr->readWaitList);
	    FsWaitListDelete(&handlePtr->writeWaitList);
	    FsHandleRemove(handlePtr);
	    fsStats.object.pipes--;
	}
    }
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * FsPipeRead --
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
FsPipeRead(streamPtr, flags, buffer, offsetPtr, lenPtr, waitPtr)
    Fs_Stream           *streamPtr;     /* Stream to read from */
    int			flags;		/* FS_USER | FS_CONSUME */
    register Address    buffer;         /* Buffer to fill with file data */
    int                 *offsetPtr;     /* In/Out byte offset */
    int                 *lenPtr;        /* In/Out byte count */
    Sync_RemoteWaiter   *waitPtr;	 /* Process wait info */
{
    ReturnStatus 	status = SUCCESS;
    register FsPipeIOHandle *handlePtr =
	    (FsPipeIOHandle *)streamPtr->ioHandlePtr;
    int 		toRead;
    int 		startOffset;
    int			startByte;
    int			endByte;

    FsHandleLock(handlePtr);

    if (handlePtr->firstByte == -1) {
	/*
	 * No data in the pipe.  If there is no writer left then
	 * return SUCCESS so the user process thinks of it as end of file,
	 * otherwise block waiting for input.
	 */
	*lenPtr = 0;
	if (handlePtr->flags & PIPE_WRITER_GONE) {
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
    if (toRead > *lenPtr) {
	toRead = *lenPtr;
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
	if (flags & FS_USER) {
	    if (Vm_CopyOut(toRead, handlePtr->buffer + startOffset, buffer)
			  != SUCCESS) {
		status = SYS_ARG_NOACCESS;
	    }
	} else {
	    bcopy(handlePtr->buffer + startOffset, buffer, toRead);
	}
    } else {
	int	numBytes;
	/*
	 * Have to wrap around in the block so do it in two copies.
	 */
	numBytes = FS_BLOCK_SIZE - startOffset;
	if (flags & FS_USER) {
	    if (Vm_CopyOut(numBytes, handlePtr->buffer + startOffset, buffer)
			  != SUCCESS) {
		status = SYS_ARG_NOACCESS;
	    } else if (Vm_CopyOut(toRead - numBytes, handlePtr->buffer,
				buffer + numBytes) != SUCCESS) {
		status = SYS_ARG_NOACCESS;
	    }
	} else {
	    bcopy(handlePtr->buffer + startOffset, buffer, numBytes);
	    bcopy(handlePtr->buffer, buffer + numBytes, toRead - numBytes);
	}
    }

    /*
     * We just made space in the pipe so wake up blocked writers.
     */
    FsFastWaitListNotify(&handlePtr->writeWaitList);

    /*
     * Update the first byte and the parameters.
     */
    handlePtr->firstByte += toRead;
    if (handlePtr->firstByte > handlePtr->lastByte) {
	handlePtr->firstByte = -1;
	handlePtr->lastByte = -1;
    }
    *lenPtr = toRead;
    *offsetPtr += toRead;
exit:
    if (status == FS_WOULD_BLOCK) {
	FsFastWaitListInsert(&handlePtr->readWaitList, waitPtr);
    }
    FsHandleUnlock(handlePtr);
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * FsPipeWrite --
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
FsPipeWrite(streamPtr, flags, buffer, offsetPtr, lenPtr, waitPtr)
    Fs_Stream           *streamPtr;     /* Stream to write to */
    int			flags;		/* FS_NON_BLOCKING checked so the
					 * correct error code is returned, plus
					 * FS_USER for copying data. */
    register Address    buffer;         /* Buffer to add to the pipe */
    int                 *offsetPtr;     /* In/Out byte offset */
    int                 *lenPtr;        /* In/Out byte count */
    Sync_RemoteWaiter   *waitPtr;	 /* Process waiting info */
{
    register ReturnStatus 	status = SUCCESS;
    register FsPipeIOHandle	*handlePtr =
	    (FsPipeIOHandle *)streamPtr->ioHandlePtr;
    int 			startOffset;
    register int 		toWrite;
    int				startByte;
    int				endByte;

    FsHandleLock(handlePtr);

    if (handlePtr->flags & PIPE_READER_GONE) {
	*lenPtr = 0;
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
	*lenPtr = 0;
	status = FS_WOULD_BLOCK;
	goto exit;
    } else if (toWrite > *lenPtr) {
	toWrite = *lenPtr;
    } else if ((*lenPtr > toWrite) && ((flags & FS_NON_BLOCKING) == 0)) {
	/*
	 * If there is more data to write we must block after writing the
	 * data that we can.  If the stream is non-blocking, however, we
	 * return a successful error code after writing what we can.
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
	if (flags & FS_USER) {
	    if (Vm_CopyIn(toWrite, buffer, handlePtr->buffer + startOffset)
			  != SUCCESS) {
		status = SYS_ARG_NOACCESS;
	    }
	} else {
	    bcopy(buffer, handlePtr->buffer + startOffset, toWrite);
	}
    } else {
	int	numBytes;
	/*
	 * Have to wrap around in the block so do it in two copies.
	 */
	numBytes = FS_BLOCK_SIZE - startOffset;
	if (flags & FS_USER) {
	    if (Vm_CopyIn(numBytes, buffer, handlePtr->buffer + startOffset)
			  != SUCCESS) {
		status = SYS_ARG_NOACCESS;
	    } else if (Vm_CopyIn(toWrite - numBytes, buffer + numBytes, 
			    handlePtr->buffer) != SUCCESS) {
		status = SYS_ARG_NOACCESS;
	    }
	} else {
	    bcopy(buffer, handlePtr->buffer + startOffset, numBytes);
	    bcopy(buffer + numBytes, handlePtr->buffer, toWrite - numBytes);
	}
    }

    /*
     * We just wrote to the pipe so wake up blocked readers.
     */
    FsFastWaitListNotify(&handlePtr->readWaitList);

    /*
     * Update first byte, last byte and the parameters.
     */
    if (handlePtr->firstByte == -1) {
	handlePtr->firstByte = 0;
    }
    handlePtr->lastByte = endByte;
    *lenPtr = toWrite;
    *offsetPtr += toWrite;
exit:
    if (status == FS_WOULD_BLOCK) {
	FsFastWaitListInsert(&handlePtr->writeWaitList, waitPtr);
    }
    FsHandleUnlock(handlePtr);
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * FsPipeIOControl --
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
FsPipeIOControl(streamPtr, command, byteOrder, inBufPtr, outBufPtr)
    Fs_Stream *streamPtr;
    int command;
    int byteOrder;
    Fs_Buffer *inBufPtr;
    Fs_Buffer *outBufPtr;
{
    register FsPipeIOHandle *handlePtr =
	    (FsPipeIOHandle *)streamPtr->ioHandlePtr;
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
	    if (inBufPtr->size >= sizeof(int) && byteOrder != mach_ByteOrder) {
		size = sizeof(int);
		Swap_Buffer(inBufPtr->addr, inBufPtr->size, byteOrder,
			    mach_ByteOrder, "w", (Address)&flags, &size);
		if (size != sizeof(int)) {
		    status = GEN_INVALID_ARG;
		}
	    } else if (inBufPtr->size != sizeof(int)) {
		status = GEN_INVALID_ARG;
	    } else {
		flags = *(int *)inBufPtr->addr;
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
	    int bytesAvailable;
	    bytesAvailable = handlePtr->lastByte - handlePtr->firstByte;
	    if (outBufPtr->size >= sizeof(int) && byteOrder != mach_ByteOrder) {
		int size = sizeof(int);
		Swap_Buffer((Address)&bytesAvailable, sizeof(int),
		    mach_ByteOrder, byteOrder, "w", outBufPtr->addr, &size);
		if (size != sizeof(int)) {
		    status = GEN_INVALID_ARG;
		}
	    } else if (outBufPtr->size != sizeof(int)) {
		status = GEN_INVALID_ARG;
	    } else {
		*(int *)outBufPtr->addr = bytesAvailable;
	    }
	    return(status);
	}
	case IOC_GET_OWNER:
	case IOC_SET_OWNER:
	case IOC_MAP:
	    return(GEN_NOT_IMPLEMENTED);
	default:
	    return(GEN_INVALID_ARG);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * FsPipeSelect --
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
FsPipeSelect(hdrPtr, waitPtr, readPtr, writePtr, exceptPtr)
    FsHandleHeader *hdrPtr;	/* The handle of the file */
    Sync_RemoteWaiter *waitPtr;	/* Process info for waiting */
    int		*readPtr;	/* Read bit */
    int		*writePtr;	/* Write bit */
    int		*exceptPtr;	/* Exception bit */
{
    register FsPipeIOHandle *handlePtr = (FsPipeIOHandle *)hdrPtr;

    FsHandleLock(hdrPtr);
    *exceptPtr = 0;
    if (*writePtr) {
	/*
	 * Turn off writability if the pipe is full and there
	 * are still readers.  If there are no readers we allow
	 * writing but the next write will fail.
	 */
	if ((((handlePtr->firstByte + 1) % FS_BLOCK_SIZE) ==
	     handlePtr->lastByte) && 
	     ((handlePtr->flags & PIPE_READER_GONE) == 0)) {
	    if (waitPtr != (Sync_RemoteWaiter *)NIL) {
		FsFastWaitListInsert(&handlePtr->writeWaitList, waitPtr);
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
	    ((handlePtr->flags & PIPE_WRITER_GONE) == 0)) {
	    *readPtr = 0;
	    if (waitPtr != (Sync_RemoteWaiter *)NIL) {
		FsFastWaitListInsert(&handlePtr->readWaitList, waitPtr);
	    }
        }
    }
    FsHandleUnlock(hdrPtr);
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * FsPipeGetIOAttr --
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
FsPipeGetIOAttr(fileIDPtr, clientID, attrPtr)
    Fs_FileID			*fileIDPtr;	/* FileID of pipe */
    int 			clientID;	/* IGNORED */
    register Fs_Attributes	*attrPtr;	/* Attributes to update */
{
    FsPipeIOHandle *handlePtr;

    handlePtr = FsHandleFetchType(FsPipeIOHandle, fileIDPtr);
    if (handlePtr != (FsPipeIOHandle *)NIL) {
	attrPtr->serverID	= fileIDPtr->serverID;
	attrPtr->domain		= fileIDPtr->major;
	attrPtr->fileNumber	= fileIDPtr->minor;
	attrPtr->type		= FS_LOCAL_PIPE;
	attrPtr->size		= handlePtr->lastByte - handlePtr->firstByte +1;
	attrPtr->devServerID	= fileIDPtr->serverID;
	FsHandleUnlock(handlePtr);
    }
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * FsPipeSetIOAttr --
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
FsPipeSetIOAttr(fileIDPtr, attrPtr, flags)
    Fs_FileID		*fileIDPtr;	/* FileID of pipe */
    Fs_Attributes	*attrPtr;	/* Attributes to update */
    int			flags;		/* What attrs to set */
{
    return(SUCCESS);
}

/*
 * ----------------------------------------------------------------------------
 *
 * FsPipeRelease --
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
FsPipeRelease(hdrPtr, flags)
    FsHandleHeader *hdrPtr;	/* File being released */
    int flags;			/* Use flags from the stream */
{
    panic( "FsPipeRelease called\n");
    FsHandleRelease(hdrPtr, FALSE);
    return(SUCCESS);
}

/*
 * ----------------------------------------------------------------------------
 *
 * FsPipeMigrate --
 *
 *	This takes care of transfering references from one client to the other.
 *	A useful side-effect of this routine is	to properly set the type in
 *	the ioFileID, either FS_LCL_PIPE_STREAM or FS_RMT_PIPE_STREAM.
 *	In the latter case FsRmtPipeMigrate is called to do all the work.
 *
 * Results:
 *	An error status if the I/O handle can't be set-up.
 *	Otherwise SUCCESS is returned, *flagsPtr may have the FS_RMT_SHARED
 *	bit set, and *sizePtr and *dataPtr are set to reference FsDeviceState.
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
FsPipeMigrate(migInfoPtr, dstClientID, flagsPtr, offsetPtr, sizePtr, dataPtr)
    FsMigInfo	*migInfoPtr;	/* Migration state */
    int		dstClientID;	/* ID of target client */
    int		*flagsPtr;	/* In/Out Stream usage flags */
    int		*offsetPtr;	/* Return - new stream offset (not needed) */
    int		*sizePtr;	/* Return - sizeof(FsDeviceState) */
    Address	*dataPtr;	/* Return - pointer to FsDeviceState */
{
    FsPipeIOHandle			*handlePtr;
    Boolean				closeSrcClient;

    if (migInfoPtr->ioFileID.serverID != rpc_SpriteID) {
	/*
	 * The pipe was local, which is usually true, but is now remote.
	 */
	migInfoPtr->ioFileID.type = FS_RMT_PIPE_STREAM;
	return(FsRmtPipeMigrate(migInfoPtr, dstClientID, flagsPtr, offsetPtr,
		sizePtr, dataPtr));
    }
    migInfoPtr->ioFileID.type = FS_LCL_PIPE_STREAM;
    handlePtr = FsPipeHandleInit(&migInfoPtr->ioFileID, TRUE);

    /*
     * At the stream level, add the new client to the set of clients
     * for the stream, and check for any cross-network stream sharing.
     */
    FsStreamMigClient(migInfoPtr, dstClientID, (FsHandleHeader *)handlePtr,
		    &closeSrcClient);
    /*
     * Adjust use counts on the I/O handle to reflect any new sharing.
     */
    FsMigrateUseCounts(migInfoPtr->flags, closeSrcClient, &handlePtr->use);

    /*
     * Move the client at the I/O handle level.
     */
    FsIOClientMigrate(&handlePtr->clientList, migInfoPtr->srcClientID,
			dstClientID, migInfoPtr->flags, closeSrcClient);

    *sizePtr = 0;
    *dataPtr = (Address)NIL;
    *flagsPtr = migInfoPtr->flags;
    *offsetPtr = migInfoPtr->offset;
    /*
     * We don't need this reference on the I/O handle; there is no change.
     */
    FsHandleRelease(handlePtr, TRUE);
    return(SUCCESS);
}

/*
 * ----------------------------------------------------------------------------
 *
 * FsRmtPipeMigrate --
 *
 *	This takes care of transfering references from one client to the other.
 *	A useful side-effect of this routine is	to properly set the type in
 *	the ioFileID, either FS_LCL_PIPE_STREAM or FS_RMT_PIPE_STREAM,
 *	so that the subsequent call to FsRemoteIOMigEnd will set up
 *	the right I/O handle.
 *
 * Results:
 *	An error status if the I/O handle can't be set-up.
 *	Otherwise SUCCESS is returned, *flagsPtr may have the FS_RMT_SHARED
 *	bit set, and *sizePtr and *dataPtr are set to reference FsDeviceState.
 *
 * Side effects:
 *	Sets the correct stream type on the ioFileID.
 *	Shifts client references from the srcClient to the destClient.
 *	Set up and return FsDeviceState for use by the MigEnd routine.
 *
 * ----------------------------------------------------------------------------
 *
 */
/*ARGSUSED*/
ReturnStatus
FsRmtPipeMigrate(migInfoPtr, dstClientID, flagsPtr, offsetPtr, sizePtr, dataPtr)
    FsMigInfo	*migInfoPtr;	/* Migration state */
    int		dstClientID;	/* ID of target client */
    int		*flagsPtr;	/* In/Out Stream usage flags */
    int		*offsetPtr;	/* Return - new stream offset (not needed) */
    int		*sizePtr;	/* Return - sizeof(FsDeviceState) */
    Address	*dataPtr;	/* Return - pointer to FsDeviceState */
{
    register ReturnStatus		status;

    if (migInfoPtr->ioFileID.serverID == rpc_SpriteID) {
	/*
	 * The pipe was remote, which is why we were called, but is now local.
	 */
	migInfoPtr->ioFileID.type = FS_LCL_PIPE_STREAM;
	return(FsPipeMigrate(migInfoPtr, dstClientID, flagsPtr, offsetPtr,
		sizePtr, dataPtr));
    }
    migInfoPtr->ioFileID.type = FS_RMT_PIPE_STREAM;
    status = FsNotifyOfMigration(migInfoPtr, flagsPtr, offsetPtr,
				 0, (Address)NIL);
    if (status != SUCCESS) {
	printf( "FsRmtPipeMigrate, server error <%x>\n",
	    status);
    } else {
	*dataPtr = (Address)NIL;
	*sizePtr = 0;
    }
    return(status);
}

/*
 * ----------------------------------------------------------------------------
 *
 * FsPipeMigEnd --
 *
 *	Complete setup of a FS_LCL_PIPE_STREAM after migration (back) to the
 *	pipe I/O server.  FsPipeMigrate has done the work of shifting use
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
FsPipeMigEnd(migInfoPtr, size, data, hdrPtrPtr)
    FsMigInfo	*migInfoPtr;	/* Migration state */
    int		size;		/* Zero */
    ClientData	data;		/* NIL */
    FsHandleHeader **hdrPtrPtr;	/* Return - handle for the file */
{
    register FsPipeIOHandle *handlePtr;

    handlePtr = FsHandleFetchType(FsPipeIOHandle, &migInfoPtr->ioFileID);
    if (handlePtr == (FsPipeIOHandle *)NIL) {
	panic( "FsPipeMigEnd, no handle\n");
	return(FAILURE);
    } else {
	FsHandleUnlock(handlePtr);
	*hdrPtrPtr = (FsHandleHeader *)handlePtr;
	return(SUCCESS);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * FsRmtPipeVerify --
 *
 *	Verify that the remote client is known for the pipe, and return
 *	a locked pointer to the pipe's I/O handle.
 *
 * Results:
 *	A pointer to the I/O handle for the pipe, or NIL if
 *	the client is bad.
 *
 * Side effects:
 *	The handle is returned locked and with its refCount incremented.
 *	It should be released with FsHandleRelease.
 *
 *----------------------------------------------------------------------
 */

FsHandleHeader *
FsRmtPipeVerify(fileIDPtr, clientID, domainTypePtr)
    Fs_FileID	*fileIDPtr;	/* Client's I/O file ID */
    int		clientID;	/* Host ID of the client */
    int		*domainTypePtr;	/* Return - FS_LOCAL_DOMAIN */
{
    register FsPipeIOHandle	*handlePtr;
    register FsClientInfo	*clientPtr;
    Boolean			found = FALSE;

    fileIDPtr->type = FS_LCL_PIPE_STREAM;
    handlePtr = FsHandleFetchType(FsPipeIOHandle, fileIDPtr);
    if (handlePtr != (FsPipeIOHandle *)NIL) {
	LIST_FORALL(&handlePtr->clientList, (List_Links *) clientPtr) {
	    if (clientPtr->clientID == clientID) {
		found = TRUE;
		break;
	    }
	}
	if (!found) {
	    FsHandleRelease(handlePtr, TRUE);
	    handlePtr = (FsPipeIOHandle *)NIL;
	}
    }
    if (!found) {
	printf(
	    "FsRmtPipeVerify, client %d not known for pipe <%d,%d>\n",
	    clientID, fileIDPtr->major, fileIDPtr->minor);
    }
    if (domainTypePtr != (int *)NIL) {
	*domainTypePtr = FS_LOCAL_DOMAIN;
    }
    return((FsHandleHeader *)handlePtr);
}

/*
 *----------------------------------------------------------------------
 *
 * FsRmtPipeReopen --
 *
 *	Reopen a remote pipe.  This sets up and conducts an 
 *	RPC_FS_REOPEN remote procedure call to re-open the remote pipe.
 *
 * Results:
 *	A  non-SUCCESS return code if the re-open failed.
 *
 * Side effects:
 *	If the reopen works we'll have a valid I/O handle.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
FsRmtPipeReopen(hdrPtr, clientID, inData, outSizePtr, outDataPtr)
    FsHandleHeader	*hdrPtr;
    int			clientID;		/* Should be rpc_SpriteID */
    ClientData		inData;			/* IGNORED */
    int			*outSizePtr;		/* IGNORED */
    ClientData		*outDataPtr;		/* IGNORED */
{
    register FsRemoteIOHandle	*rmtHandlePtr;
    ReturnStatus		status;
    FsPipeReopenParams		reopenParams;
    int				outSize;

    rmtHandlePtr = (FsRemoteIOHandle *)hdrPtr;
    reopenParams.fileID = hdrPtr->fileID;
    reopenParams.fileID.type = FS_LCL_PIPE_STREAM;
    reopenParams.use = rmtHandlePtr->recovery.use;

    /*
     * Contact the server to do the reopen, and then notify waiters.
     */
    outSize = 0;
    status = FsSpriteReopen(hdrPtr, sizeof(FsPipeReopenParams),
		(Address)&reopenParams, &outSize, (Address)NIL);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsPipeReopen --
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
FsPipeReopen(hdrPtr, clientID, inData, outSizePtr, outDataPtr)
    FsHandleHeader	*hdrPtr;		/* NIL */
    int			clientID;		/* Host ID of client */
    ClientData		inData;			/* sizeof FsPipeReopenParams */
    int			*outSizePtr;		/* Returns 0 */
    ClientData		*outDataPtr;		/* Returns NIL */
{
    register FsPipeIOHandle	*handlePtr;
    ReturnStatus		status;
    FsPipeReopenParams		*reopenParamsPtr;
    register FsClientInfo	*clientPtr;
    Boolean			found;

    reopenParamsPtr = (FsPipeReopenParams *)inData;
    handlePtr = FsHandleFetchType(FsPipeIOHandle, &reopenParamsPtr->fileID);
    if (handlePtr == (FsPipeIOHandle *)NIL) {
	status = FAILURE;
    } else {
	/*
	 * Loop through the client list to see if we know about the client.
	 */
	found = FALSE;
	LIST_FORALL(&handlePtr->clientList, (List_Links *)clientPtr) {
	    if (clientPtr->clientID == clientID) {
		found = TRUE;
		break;
	    }
	}
	if (!found) {
	    clientPtr = mnew(FsClientInfo);
	    clientPtr->clientID = clientID;
	    List_Insert((List_Links *) clientPtr,
		    LIST_ATFRONT(&handlePtr->clientList));
	}
	clientPtr->use = reopenParamsPtr->use;
	status = SUCCESS;
    }
    *outSizePtr = 0;
    *outDataPtr = (ClientData)NIL;
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsPipeClientKill --
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
FsPipeClientKill(hdrPtr, clientID)
    FsHandleHeader *hdrPtr;     /* Handle to clean up */
    int clientID;		/* Host assumed down */
{
    register FsPipeIOHandle *handlePtr = (FsPipeIOHandle *)hdrPtr;
    int refs, writes, execs;

    FsIOClientKill(&handlePtr->clientList, clientID, &refs, &writes, &execs);
    if (refs > 0) {
	if (writes) {
	    handlePtr->flags |= PIPE_WRITER_GONE;
	} else {
	    handlePtr->flags |= PIPE_READER_GONE;
	}
    }
    FsHandleUnlock(handlePtr);
}


/*
 *----------------------------------------------------------------------
 *
 * FsPipeScavenge --
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
FsPipeScavenge(hdrPtr)
    FsHandleHeader *hdrPtr;     /* Handle about to be deleted */
{
    register FsPipeIOHandle *handlePtr = (FsPipeIOHandle *)hdrPtr;
#ifdef notdef
    if (List_IsEmpty(&handlePtr->clientList) &&
	(handlePtr->flags == (PIPE_WRITER_GONE|PIPE_READER_GONE))) {
	/*
	 * Never scavenge pipe handles.  Regular recovery cleanup
	 * should invoke the pipe close routines and do proper cleanup.
	 */
	free(handlePtr->buffer);
	FsWaitListDelete(&handlePtr->readWaitList);
	FsWaitListDelete(&handlePtr->writeWaitList);
	FsHandleRemove(hdrPtr);
	fsStats.object.pipes--;
	return(TRUE);
    }
#endif notdef
    FsHandleUnlock(hdrPtr);
    return(FALSE);
}

