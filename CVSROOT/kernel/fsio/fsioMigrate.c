/*
 * fsMigrate.c --
 *
 * Procedures to handle migrating open files between machines.  The basic
 * strategy is to first do some local book-keeping on the client we are
 * leaving, then ship state to the new client, then finally tell the
 * I/O server about it, and finish up with local book-keeping on the
 * new client.  There are three stream-type procedures used: 'migStart'
 * does the initial book-keeping on the original client, 'migEnd' does
 * the final book-keeping on the new client, and 'migrate' is called
 * on the I/O server to shift around state associated with the client.
 *
 * Copyright (C) 1985, 1988, 1989 Regents of the University of California
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
#include "fsio.h"
#include "fsconsist.h"
#include "fspdev.h"
#include "fsprefix.h"
#include "fsNameOps.h"
#include "byte.h"
#include "rpc.h"
#include "procMigrate.h"

Boolean fsio_MigDebug = FALSE;
#define DEBUG( format ) \
	if (fsio_MigDebug) { printf format ; }


/*
 * ----------------------------------------------------------------------------
 *
 * Fsio_EncapStream --
 *
 *	Package up a stream's state for migration to another host.  This
 *	copies the stream's offset, streamID, ioFileID, nameFileID, and flags.
 *	This routine is side-effect free with respect to both
 *	the stream and the I/O handles.  The bookkeeping is done later
 *	during Fsio_DeencapStream so proper syncronization with Fs_Close
 *	bookkeeping can be done.
 *	It is reasonable to call Fsio_DeencapStream again on this host,
 *	for example, to back out an aborted migration.
 *
 * Results:
 *	This always returns SUCCESS.
 *	
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 *
 */

ReturnStatus
Fsio_EncapStream(streamPtr, bufPtr)
    Fs_Stream	*streamPtr;	/* Stream to be migrated */
    Address	bufPtr;		/* Buffer to hold encapsulated stream */
{
    register	FsMigInfo	*migInfoPtr;
    register Fs_HandleHeader	*ioHandlePtr;

    /*
     * Synchronize with stream duplication and closes
     */
    Fsutil_HandleLock(streamPtr);

    /*
     * The encapsulated stream state includes the read/write offset,
     * the I/O server, the useFlags of the stream, and our SpriteID so
     * the target of migration and the server can do the right thing later. 
     */
    migInfoPtr = (FsMigInfo *) bufPtr;
    ioHandlePtr = streamPtr->ioHandlePtr;
    migInfoPtr->streamID = streamPtr->hdr.fileID;
    migInfoPtr->ioFileID = ioHandlePtr->fileID;
    if (streamPtr->nameInfoPtr == (Fs_NameInfo *)NIL) {
	/*
	 * Anonymous pipes have no name information.
	 */
	migInfoPtr->nameID.type = -1;
    } else {
	migInfoPtr->nameID = streamPtr->nameInfoPtr->fileID;
	migInfoPtr->rootID = streamPtr->nameInfoPtr->rootID;
    }
    migInfoPtr->offset = streamPtr->offset;
    migInfoPtr->srcClientID = rpc_SpriteID;
    migInfoPtr->flags = streamPtr->flags;

    Fsutil_HandleUnlock(streamPtr);
    return(SUCCESS);
}

/*
 * ----------------------------------------------------------------------------
 *
 * Fsio_DeencapStream --
 *
 *	Deencapsulate the stream that was packaged up on another machine
 *	and recreate the stream on this machine.  This uses two stream-type
 *	routines to complete the setup of the stream.  First, the
 *	migrate routine is called to shift client references on the
 *	server.  Then the migEnd routine is called to do local book-keeping.
 *
 * Results:
 *	A return status, plus *streamPtrPtr is set to the new stream.
 *
 * Side effects:
 *	Ensures that the stream exists on this host, along with the
 *	associated I/O handle.  This calls a stream-type specific routine
 *	to shuffle reference counts and detect cross-machine stream
 *	sharing.  If a stream is shared by proceses on different machines
 *	its flags field is marked with FS_RMT_SHARED.  This also calls
 *	a stream-type specific routine to create the I/O handle when the
 *	first reference to a stream migrates to this host.
 *
 * ----------------------------------------------------------------------------
 *
 */

ReturnStatus
Fsio_DeencapStream(bufPtr, streamPtrPtr)
    Address	bufPtr;		/* Encapsulated stream information. */
    Fs_Stream	**streamPtrPtr;	/* Where to return pointer to the new stream */
{
    register	Fs_Stream	*streamPtr;
    register	FsMigInfo	*migInfoPtr;
    register	Fs_NameInfo	*nameInfoPtr;
    ReturnStatus		status = SUCCESS;
    Boolean			foundClient;
    Boolean			foundStream;
    int				savedOffset;
    int				size;
    ClientData			data;

    migInfoPtr = (FsMigInfo *) bufPtr;

    if (migInfoPtr->srcClientID == rpc_SpriteID) {
	/*
	 * Migrating to ourselves.  Just fetch the stream.
	 */
	*streamPtrPtr = Fsutil_HandleFetchType(Fs_Stream, &migInfoPtr->streamID);
	if (*streamPtrPtr == (Fs_Stream *)NIL) {
	    return(FS_FILE_NOT_FOUND);
	} else {
	    return(SUCCESS);
	}
    }
    /*
     * Create a top-level stream and note if this is a new stream.  This is
     * important because extra things happen when the first reference to
     * a stream migrates to this host.  FS_NEW_STREAM is used to indicate this.
     * Note that the stream has (at least) one reference count and a client
     * list entry that will be cleaned up by a future call to Fs_Close.
     */
    streamPtr = Fsio_StreamAddClient(&migInfoPtr->streamID, rpc_SpriteID,
			     (Fs_HandleHeader *)NIL,
			     migInfoPtr->flags & ~FS_NEW_STREAM, (char *)NIL,
			     &foundClient, &foundStream);
    savedOffset = migInfoPtr->offset;
    if (!foundClient) {
	migInfoPtr->flags |= FS_NEW_STREAM;
	streamPtr->offset = migInfoPtr->offset;
	DEBUG( ("Deencap NEW stream %d, migOff %d, ",
		streamPtr->hdr.fileID.minor, migInfoPtr->offset) );
    } else {
	migInfoPtr->flags &= ~FS_NEW_STREAM;
	DEBUG( ("Deencap OLD stream %d, migOff %d, ",
		streamPtr->hdr.fileID.minor,
		migInfoPtr->offset, streamPtr->offset) );
    }
    if (streamPtr->nameInfoPtr == (Fs_NameInfo *)NIL) {
	if (migInfoPtr->nameID.type == -1) {
	    /*
	     * No name info to re-create.  This happens when anonymous
	     * pipes get migrated.
	     */
	    streamPtr->nameInfoPtr = (Fs_NameInfo *)NIL;
	} else {
	    /*
	     * Set up the nameInfo.  We sacrifice the name string as it is only
	     * used in error messages.  The fileID is used with get/set attr.
	     * If this file is the current directory then rootID is passed
	     * to the server to trap "..", domainType is used to index the
	     * name lookup operation switch, and prefixPtr is used for
	     * efficient handling of lookup redirections.
	     * Convert from remote to local file types, and vice-versa,
	     * as needed.
	     */
	    streamPtr->nameInfoPtr = nameInfoPtr = mnew(Fs_NameInfo);
	    nameInfoPtr->fileID = migInfoPtr->nameID;
	    nameInfoPtr->rootID = migInfoPtr->rootID;
	    if (nameInfoPtr->fileID.serverID != rpc_SpriteID) {
		nameInfoPtr->domainType = FS_REMOTE_SPRITE_DOMAIN;
		nameInfoPtr->fileID.type =
		    fsio_LclToRmtType[nameInfoPtr->fileID.type];
		nameInfoPtr->rootID.type =
		    fsio_LclToRmtType[nameInfoPtr->rootID.type];
	    } else {
		/*
		 * FIX HERE PROBABLY TO HANDLE PSEUDO_FILE_SYSTEMS.
		 */
		nameInfoPtr->domainType = FS_LOCAL_DOMAIN;
		nameInfoPtr->fileID.type =
		    fsio_RmtToLclType[nameInfoPtr->fileID.type];
		nameInfoPtr->rootID.type =
		    fsio_RmtToLclType[nameInfoPtr->rootID.type];
	    }
	    nameInfoPtr->prefixPtr = Fsprefix_FromFileID(&migInfoPtr->rootID);
	    if (nameInfoPtr->prefixPtr == (struct Fsprefix *)NIL) {
		printf("Fsio_DeencapStream: No prefix entry for <%d,%d,%d>\n",
		    migInfoPtr->rootID.serverID,
		    migInfoPtr->rootID.major, migInfoPtr->rootID.minor);
	    }
	}
    }
    /*
     * Contact the I/O server to tell it that the client moved.  The I/O
     * server checks for cross-network stream sharing and sets the
     * FS_RMT_SHARED flag if it is shared.  Note that we set FS_NEW_STREAM
     * in the migInfoPtr->flags, and this flag often gets rammed into
     * the streamPtr->flags, which we don't want because it would confuse
     * Fsio_MigrateUseCounts on subsequent migrations.
     */
    Fsutil_HandleUnlock(streamPtr);
    status = (*fsio_StreamOpTable[migInfoPtr->ioFileID.type].migrate)
		(migInfoPtr, rpc_SpriteID, &streamPtr->flags,
		 &streamPtr->offset, &size, &data);
    streamPtr->flags &= ~FS_NEW_STREAM;

    DEBUG( (" Type %d <%d,%d> offset %d, ", migInfoPtr->ioFileID.type,
		migInfoPtr->ioFileID.major, migInfoPtr->ioFileID.minor,
		streamPtr->offset) );

    if (status == SUCCESS && !foundClient) {
	/*
	 * The stream is newly created on this host so we call down to
	 * the I/O handle level to ensure that the I/O handle exists and
	 * so the local object manager gets told about the new stream.
	 */
	migInfoPtr->flags = streamPtr->flags;
	status = (*fsio_StreamOpTable[migInfoPtr->ioFileID.type].migEnd)
		(migInfoPtr, size, data, &streamPtr->ioHandlePtr);
	DEBUG( ("migEnd status %x\n", status) );
    } else {
	DEBUG( ("migrate status %x\n", status) );
    }
    if (streamPtr->offset != savedOffset) {
	printf("Fsio_DeencapStream \"%s\" srcClientOffset %d ioSrvrOffset %d\n",
	    Fsutil_HandleName(streamPtr->ioHandlePtr),
	    savedOffset, streamPtr->offset);
    }

    if (status == SUCCESS) {
	*streamPtrPtr = streamPtr;
    } else {
	Fsutil_HandleLock(streamPtr);
	if (!foundStream &&
	    Fsio_StreamClientClose(&streamPtr->clientList, rpc_SpriteID)) {
	    Fsio_StreamDestroy(streamPtr);
	} else {
	    Fsutil_HandleRelease(streamPtr, TRUE);
	}
    }

    return(status);
}

/*
 * ----------------------------------------------------------------------------
 *
 * Fsio_MigrateUseCounts --
 *
 *	This updates use counts to reflect any network sharing that
 *	is a result of migration.  The rule adhered to is that there
 *	are use counts kept on the I/O handle for each stream on each client
 *	that uses the I/O handle.  A stream with only one reference
 *	does not change use counts when it migrates, for example, because
 *	the reference just moves.  A stream with two references will
 *	cause a new client host to have a stream after migration, so the
 *	use counts are updated in case both clients do closes.  Finally,
 *	use counts get decremented when a stream completely leaves a
 *	client after being shared.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Adjusts the use counts to reflect sharing of the I/O handle
 *	due to migration.
 *
 * ----------------------------------------------------------------------------
 *
 */
ReturnStatus
Fsio_MigrateUseCounts(flags, closeSrcClient, usePtr)
    register int	 flags;		/* Flags from the stream */
    Boolean		closeSrcClient;	/* TRUE if I/O close was done at src */
    register Fsutil_UseCounts *usePtr;	/* Use counts from the I/O handle */
{
    if ((flags & FS_NEW_STREAM) && !closeSrcClient) {
	/*
	 * The stream is becoming shared across the network because
	 * it is new at the destination and wasn't closed at the source.
	 * Increment the use counts on the I/O handle
	 * to reflect the additional client stream.
	 */
	usePtr->ref++;
	if (flags & FS_WRITE) {
	    usePtr->write++;
	}
	if (flags & FS_EXECUTE) {
	    usePtr->exec++;
	}
    } else if ((flags & FS_NEW_STREAM) == 0 && closeSrcClient) {
	/*
	 * The stream is becoming un-shared.  The last reference from the
	 * source was closed and there is already a reference at the dest.
	 * Decrement the use counts to reflect the fact that the stream on
	 * the original client is not referencing the I/O handle.
	 */
	usePtr->ref--;
	if (flags & FS_WRITE) {
	    usePtr->write--;
	}
	if (flags & FS_EXECUTE) {
	    usePtr->exec--;
	}
    } else {
	/*
	 * The stream moved completly, or a reference just moved between
	 * two existing streams, so there is no change visible to
	 * the I/O handle use counts.
	 */
     }
}

/*
 * ----------------------------------------------------------------------------
 *
 * Fsio_IOClientMigrate --
 *
 *	Move a client of an I/O handle from one host to another.  Flags
 *	indicate if the migration results in a newly shared stream, or
 *	in a stream that is no longer shared, or in a stream with
 *	no change visible at the I/O handle level.  We are careful to only
 *	open the dstClient if it getting the stream for the first time.
 *	Also, if the srcClient is switching from a writer to a reader, we
 *	remove its write reference.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Adds the destination client to the clientList, if needed.
 *	Removes the source client from the list, if needed.
 *
 * ----------------------------------------------------------------------------
 */

ENTRY void
Fsio_IOClientMigrate(clientList, srcClientID, dstClientID, flags, closeSrcClient)
    List_Links	*clientList;	/* List of clients for the I/O handle. */
    int		srcClientID;	/* The original client. */
    int		dstClientID;	/* The destination client. */
    int		flags;		/* FS_RMT_SHARED if a copy of the stream
				 * still exists on the srcClient.
				 * FS_NEW_STREAM if stream is new on dst.
				 * FS_READ | FS_WRITE | FS_EXECUTE */
    Boolean	closeSrcClient;	/* TRUE if we should close src client.  This
				 * is set by Fsio_StreamMigClient */
{
    register Boolean found;
    Boolean cache = FALSE;

    if (closeSrcClient) {
	/*
	 * The stream is not shared so we nuke the original client's use.
	 */
	found = Fsconsist_IOClientClose(clientList, srcClientID, flags, &cache);
	if (!found) {
	    printf("Fsio_IOClientMigrate, srcClient %d not found\n", srcClientID);
	}
    }
    if (flags & FS_NEW_STREAM) {
	/*
	 * The stream is new on the destination host.
	 */
	(void)Fsconsist_IOClientOpen(clientList, dstClientID, flags, FALSE);
    }
}

