/*
 * fsRmtPipe.c --
 *
 *	Routines for remote unnamed pipes.  An unnamed pipe has a fixed length
 *	resident buffer, a reading stream, and a writing stream.
 *	Process migration can result in remotely accessed pipes.
 *
 * Copyright 1989 Regents of the University of California
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
#include <fsconsist.h>
#include <fsio.h>
#include <fsNameOps.h>
#include <fsStat.h>
#include <fsrmt.h>
#include <vm.h>
#include <proc.h>
#include <rpc.h>
#include <fsioPipe.h>
#include <stdio.h>
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
 * ----------------------------------------------------------------------------
 *
 * FsrmtPipeMigrate --
 *
 *	This takes care of transfering references from one client to the other.
 *	A useful side-effect of this routine is	to properly set the type in
 *	the ioFileID, either FSIO_LCL_PIPE_STREAM or FSIO_RMT_PIPE_STREAM,
 *	so that the subsequent call to Fsrmt_IOMigOpen will set up
 *	the right I/O handle.
 *
 * Results:
 *	An error status if the I/O handle can't be set-up.
 *	Otherwise SUCCESS is returned, *flagsPtr may have the FS_RMT_SHARED
 *	bit set, and *sizePtr and *dataPtr are set to reference Fsio_DeviceState.
 *
 * Side effects:
 *	Sets the correct stream type on the ioFileID.
 *	Shifts client references from the srcClient to the destClient.
 *	Set up and return Fsio_DeviceState for use by the MigEnd routine.
 *
 * ----------------------------------------------------------------------------
 *
 */
/*ARGSUSED*/
ReturnStatus
FsrmtPipeMigrate(migInfoPtr, dstClientID, flagsPtr, offsetPtr, sizePtr, dataPtr)
    Fsio_MigInfo	*migInfoPtr;	/* Migration state */
    int		dstClientID;	/* ID of target client */
    int		*flagsPtr;	/* In/Out Stream usage flags */
    int		*offsetPtr;	/* Return - new stream offset (not needed) */
    int		*sizePtr;	/* Return - sizeof(Fsio_DeviceState) */
    Address	*dataPtr;	/* Return - pointer to Fsio_DeviceState */
{
    register ReturnStatus		status;

    if (migInfoPtr->ioFileID.serverID == rpc_SpriteID) {
	/*
	 * The pipe was remote, which is why we were called, but is now local.
	 */
	migInfoPtr->ioFileID.type = FSIO_LCL_PIPE_STREAM;
	return(Fsio_PipeMigrate(migInfoPtr, dstClientID, flagsPtr, offsetPtr,
		sizePtr, dataPtr));
    }
    migInfoPtr->ioFileID.type = FSIO_RMT_PIPE_STREAM;
    status = Fsrmt_NotifyOfMigration(migInfoPtr, flagsPtr, offsetPtr,
				 0, (Address)NIL);
    if (status != SUCCESS) {
	printf( "FsrmtPipeMigrate, server error <%x>\n",
	    status);
    } else {
	*dataPtr = (Address)NIL;
	*sizePtr = 0;
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsrmtPipeVerify --
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
 *	It should be released with Fsutil_HandleRelease.
 *
 *----------------------------------------------------------------------
 */

Fs_HandleHeader *
FsrmtPipeVerify(fileIDPtr, clientID, domainTypePtr)
    Fs_FileID	*fileIDPtr;	/* Client's I/O file ID */
    int		clientID;	/* Host ID of the client */
    int		*domainTypePtr;	/* Return - FS_LOCAL_DOMAIN */
{
    register Fsio_PipeIOHandle	*handlePtr;
    register Fsconsist_ClientInfo	*clientPtr;
    Boolean			found = FALSE;

    fileIDPtr->type = FSIO_LCL_PIPE_STREAM;
    handlePtr = Fsutil_HandleFetchType(Fsio_PipeIOHandle, fileIDPtr);
    if (handlePtr != (Fsio_PipeIOHandle *)NIL) {
	LIST_FORALL(&handlePtr->clientList, (List_Links *) clientPtr) {
	    if (clientPtr->clientID == clientID) {
		found = TRUE;
		break;
	    }
	}
	if (!found) {
	    Fsutil_HandleRelease(handlePtr, TRUE);
	    handlePtr = (Fsio_PipeIOHandle *)NIL;
	}
    }
    if (!found) {
	printf(
	    "FsrmtPipeVerify, client %d not known for pipe <%d,%d>\n",
	    clientID, fileIDPtr->major, fileIDPtr->minor);
    }
    if (domainTypePtr != (int *)NIL) {
	*domainTypePtr = FS_LOCAL_DOMAIN;
    }
    return((Fs_HandleHeader *)handlePtr);
}

/*
 *----------------------------------------------------------------------
 *
 * FsrmtPipeReopen --
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
FsrmtPipeReopen(hdrPtr, clientID, inData, outSizePtr, outDataPtr)
    Fs_HandleHeader	*hdrPtr;
    int			clientID;		/* Should be rpc_SpriteID */
    ClientData		inData;			/* IGNORED */
    int			*outSizePtr;		/* IGNORED */
    ClientData		*outDataPtr;		/* IGNORED */
{
    register Fsrmt_IOHandle	*rmtHandlePtr;
    ReturnStatus		status;
    Fsio_PipeReopenParams		reopenParams;
    int				outSize;

    rmtHandlePtr = (Fsrmt_IOHandle *)hdrPtr;
    reopenParams.fileID = hdrPtr->fileID;
    reopenParams.fileID.type = FSIO_LCL_PIPE_STREAM;
    reopenParams.use = rmtHandlePtr->recovery.use;

    /*
     * Contact the server to do the reopen, and then notify waiters.
     */
    outSize = 0;
    status = FsrmtReopen(hdrPtr, sizeof(Fsio_PipeReopenParams),
		(Address)&reopenParams, &outSize, (Address)NIL);
    return(status);
}
