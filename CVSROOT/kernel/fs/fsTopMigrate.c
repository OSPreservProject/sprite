/*
 * fsMigrate.c --
 *
 * Procedures to handle migrating open files between machines.  The basic
 * strategy is to first do some local book-keeping on the client we are
 * leaving, then ship state to the new client, then finally tell the
 * I/O server about it, and finish up with local book-keeping on the
 * new client.  There are three stream-type procedures used: 'migStart'
 * does the initial book-keeping on the original client, 'migEnd' does
 * the final book-keeping on the new client, and 'srvMigrate' is called
 * on the I/O server to shift around state associated with the client.
 *
 * Copyright (C) 1985, 1988 Regents of the University of California
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
#include "fsMigrate.h"
#include "fsStream.h"
#include "fsPdev.h"
#include "fsFile.h"
#include "fsDevice.h"
#include "fsSpriteDomain.h"
#include "fsPrefix.h"
#include "fsOpTable.h"
#include "fsDebug.h"
#include "mem.h"
#include "rpc.h"

Boolean fsMigDebug = FALSE;
#define DEBUG( format ) \
	if (fsMigDebug) { Sys_Printf format ; }

/*
 * The following record defines what parameters the I/O server returns
 * after being told about a migration.  (Note, it will also return
 * data that is specific to the type of the I/O handle.)
 */
typedef struct MigrateReply {
    int flags;		/* New stream flags, the FS_RMT_SHARED bit is modified*/
    int offset;		/* New stream offset */
} MigrateReply;

/*
 * This structure is for byte-swapping the rpc parameters correctly.
 */
typedef struct  FsMigParam {
    int                 dataSize;
    FsUnionData         data;
    MigrateReply        migReply;
} FsMigParam;

static void LocalToRemoteDomain();
static void RemoteToLocalDomain();


/*
 * ----------------------------------------------------------------------------
 *
 * Fs_EncapStream --
 *
 *	Package up a stream's state for migration to another host.  This
 *	copies the stream's offset, streamID, ioFileID, nameFileID, and flags.
 *	The server of the file does not perceive the migration at this
 *	time; only local book-keeping is done.  A future call to Fs_Deencap...
 *	will result in an RPC to the server to 'move' the client.
 *	It is reasonable to call Fs_DeencapStream again on this host,
 *	for example, to back out an aborted migration.
 *
 * Results:
 *	FS_REMOTE_OP_INVALID if the domain of the file does not support 
 *	migration.
 *	
 *
 * Side effects:
 *	One reference to the stream is removed, and when the last one
 *	goes away the stream itself is removed.  The migStart procedure
 *	for the I/O handle will do some local book-keeping, although
 *	this is side-effect free with respect to the book-keeping on
 *	the server.
 *
 * ----------------------------------------------------------------------------
 *
 */

ReturnStatus
Fs_EncapStream(streamPtr, bufPtr)
    Fs_Stream	*streamPtr;	/* Stream to be migrated */
    Address	bufPtr;		/* Buffer to hold encapsulated stream */
{
    register	FsMigInfo	*migInfoPtr;
    ReturnStatus		status = SUCCESS;
    register FsHandleHeader	*ioHandlePtr;

    /*
     * Synchronize with stream duplication.  Then set the shared flag
     * so the I/O handle routines know whether or not to release references;
     * if the stream is still shared they shouldn't clean up refs.
     */
    FsHandleLock(streamPtr);
    if (streamPtr->hdr.refCount <= 1) {
	DEBUG( ("Encap stream %d, last ref", streamPtr->hdr.fileID.minor) );
	streamPtr->flags &= ~FS_RMT_SHARED;
    } else {
	DEBUG( ("Encap stream %d, shared", streamPtr->hdr.fileID.minor) );
	streamPtr->flags |= FS_RMT_SHARED;
    }

    /*
     * The encapsulated stream state includes the read/write offset,
     * the I/O server, the useFlags of the stream, and our SpriteID so
     * the target of migration and the server can do the right thing later. 
     */
    migInfoPtr = (FsMigInfo *) bufPtr;
    ioHandlePtr = streamPtr->ioHandlePtr;
    migInfoPtr->streamID = streamPtr->hdr.fileID;
    migInfoPtr->ioFileID = ioHandlePtr->fileID;
    if (streamPtr->nameInfoPtr == (FsNameInfo *)NIL) {
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

    /*
     * Branch to a stream specific routine to encapsultate any extra state
     * associated with the I/O handle, and to do local book-keeping.
     */
    status = (*fsStreamOpTable[ioHandlePtr->fileID.type].migStart) (ioHandlePtr,
		    streamPtr->flags, rpc_SpriteID, &migInfoPtr->flags);

    if (status == SUCCESS) {
	/*
	 * Clean up our reference to the stream.  We'll remove the
	 * stream from the handle table entirely if this is the last
	 * reference on a client.  If we're the server, we still need
	 * the stream around as a "shadow".
	 */
	if ((ioHandlePtr->fileID.serverID != rpc_SpriteID) &&
	    (streamPtr->hdr.refCount <= 1)) {
	    FsStreamDispose(streamPtr);
	} else {
	    FsHandleRelease(streamPtr, TRUE);
	}
    }
    DEBUG( (" status %x\n", status) );
    return(status);
}

/*
 * ----------------------------------------------------------------------------
 *
 * Fs_DeencapStream --
 *
 *	Deencapsulate the stream that was packaged up on another machine
 *	and recreate the stream on this machine.  This uses two stream-type
 *	routines to complete the setup of the stream.  First, the
 *	srvMigrate routine is called to shift client references on the
 *	server.  Then the migEnd routine is called to do local book-keeping.
 *
 * Results:
 *	A return status.
 *
 * Side effects:
 *	Calls the srvMigrate and migEnd stream-type procedures.  See these
 *	routines for further details.
 *
 * ----------------------------------------------------------------------------
 *
 */

ReturnStatus
Fs_DeencapStream(bufPtr, streamPtrPtr)
    Address	bufPtr;		/* Encapsulated stream information. */
    Fs_Stream	**streamPtrPtr;	/* Where to return pointer to the new stream */
{
    register	Fs_Stream	*streamPtr;
    register	FsMigInfo	*migInfoPtr;
    register	FsNameInfo	*nameInfoPtr;
    ReturnStatus		status = SUCCESS;
    Boolean			found;
    Boolean			disposeOnError;
    int				size;
    ClientData			data;

    migInfoPtr = (FsMigInfo *) bufPtr;

    /*
     * Allocate and set up the stream.  We note if this is the first
     * occurence of the stream on this host so the server can
     * do the right thing.  If we're the server, we have to distinguish
     * between a stream that's in use on this host and a "shadow stream".
     * DisposeOnError
     * is used to keep track of the original value of "found", since
     * we want to dispose if we hit an error if & only if FsStreamFind
     * created a new stream (found == FALSE).
     */

    streamPtr = FsStreamFind(&migInfoPtr->streamID, (FsHandleHeader *)NIL,
			     migInfoPtr->flags, (char *)NIL, &found);

    disposeOnError = !found;

    if (found && (migInfoPtr->ioFileID.serverID == rpc_SpriteID)) {
	if (!FsStreamClientFind(&streamPtr->clientList, rpc_SpriteID)) {
	    found = FALSE;
	}
    }
    if (!found) {
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
    if (streamPtr->nameInfoPtr == (FsNameInfo *)NIL) {
	if (migInfoPtr->nameID.type == -1) {
	    /*
	     * No name info to re-create.  This happens when anonymous
	     * pipes get migrated.
	     */
	    streamPtr->nameInfoPtr = (FsNameInfo *)NIL;
	} else {
	    /*
	     * Set up the nameInfo.  We sacrifice the name as it is only
	     * used in error messages.  The fileID is used with get/set attr.
	     * If this file is the current directory then rootID is passed
	     * to the server to trap "..", domainType is used to index the
	     * name lookup operation switch, and prefixPtr is used for
	     * efficient handling of lookup redirections.
	     * Convert from remote to local file types, and vice-versa,
	     * as needed.
	     */
	    streamPtr->nameInfoPtr = nameInfoPtr = Mem_New(FsNameInfo);
	    nameInfoPtr->fileID = migInfoPtr->nameID;
	    nameInfoPtr->rootID = migInfoPtr->rootID;
	    if (nameInfoPtr->fileID.serverID != rpc_SpriteID) {
		nameInfoPtr->domainType = FS_REMOTE_SPRITE_DOMAIN;
		LocalToRemoteDomain(&nameInfoPtr->fileID);
		LocalToRemoteDomain(&nameInfoPtr->rootID);
	    } else {
		nameInfoPtr->domainType = FS_LOCAL_DOMAIN;
		RemoteToLocalDomain(&nameInfoPtr->fileID);
		RemoteToLocalDomain(&nameInfoPtr->rootID);
	    }
	    nameInfoPtr->prefixPtr = FsPrefixFromFileID(&migInfoPtr->rootID);
	    if (nameInfoPtr->prefixPtr == (struct FsPrefix *)NIL) {
		Sys_Panic(SYS_WARNING, "No prefix entry for <%d,%d,%d>\n",
		    migInfoPtr->rootID.serverID,
		    migInfoPtr->rootID.major, migInfoPtr->rootID.minor);
	    }
	}
    }
    /*
     * Contact the I/O server to tell it that the client moved.  The I/O
     * server checks for cross-network stream sharing and sets the
     * FS_RMT_SHARED flag if it is shared.  It also looks at the
     * FS_NEW_STREAM flag which we've set/unset above.
     *
     * NOTE: It is clear that holding the handle while eventually calling
     * FsFileMigrate will deadlock on the handle, since it gets locked again.
     * it's NOT clear whether freeing it beforehand is actually the right
     * thing to do!!  Help?!
     */
    FsHandleUnlock(streamPtr);
    status = (*fsStreamOpTable[migInfoPtr->ioFileID.type].migrate)
		(migInfoPtr, rpc_SpriteID, &streamPtr->flags,
		 &streamPtr->offset, &size, &data);
    DEBUG( (" Type %d <%d,%d> offset %d, ", migInfoPtr->ioFileID.type,
		migInfoPtr->ioFileID.major, migInfoPtr->ioFileID.minor,
		streamPtr->offset) );

    if (status == SUCCESS && !found) {
	/*
	 * Complete the setup of the stream with local book-keeping.  The
	 * local book-keeping only needs to be done the first time the
	 * stream is created here because stream sharing is not reflected
	 * in the reference/use counts on the I/O handle.
	 */
	migInfoPtr->flags = streamPtr->flags;
	status = (*fsStreamOpTable[migInfoPtr->ioFileID.type].migEnd)
		(migInfoPtr, size, data, &streamPtr->ioHandlePtr);
	DEBUG( ("migEnd status %x\n", status) );
    } else {
	DEBUG( ("srvMigrate status %x\n", status) );
    }

    if (status == SUCCESS) {
	*streamPtrPtr = streamPtr;
    } else if (disposeOnError) {
	FsHandleLock(streamPtr);
	FsStreamDispose(streamPtr);
    } else {
	FsHandleLock(streamPtr);
	FsHandleRelease(streamPtr, TRUE);
    }

    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * FsNotifyOfMigration --
 *
 *	Send an rpc to the server to inform it about a migration.
 *
 * Results:
 *	A return status.
 *
 * Side effects:
 *      None.
 *	
 *----------------------------------------------------------------------
 */
ReturnStatus
FsNotifyOfMigration(migInfoPtr, flagsPtr, offsetPtr, outSize, outData)
    FsMigInfo	*migInfoPtr;	/* Encapsulated information */
    int		*flagsPtr;	/* New flags, may have FS_RMT_SHARED bit set */
    int		*offsetPtr;	/* New stream offset */
    int		outSize;	/* Size of returned data, outData */
    Address	outData;	/* Returned data from server */
{
    register ReturnStatus	status;
    Rpc_Storage 	storage;
    FsMigParam		migParam;

    storage.requestParamPtr = (Address) migInfoPtr;
    storage.requestParamSize = sizeof(FsMigInfo);
    storage.requestDataPtr = (Address)NIL;
    storage.requestDataSize = 0;

    storage.replyParamPtr = (Address)&migParam;
    storage.replyParamSize = sizeof(FsMigParam);
    storage.replyDataPtr = (Address) NIL;
    storage.replyDataSize = 0;

    status = Rpc_Call(migInfoPtr->ioFileID.serverID, RPC_FS_MIGRATE, &storage);

    if (status == SUCCESS) {
	MigrateReply	*migReplyPtr;

	migReplyPtr = &(migParam.migReply);
	*flagsPtr = migReplyPtr->flags;
	*offsetPtr = migReplyPtr->offset;
    }
    if (migParam.dataSize > 0) {
	if (outSize < migParam.dataSize) {
	    Sys_Panic(SYS_WARNING,
		"FsNotifyOfMigration: too much data returned %d not %d\n",
		migParam.dataSize, outSize);
	    status = FAILURE;
	} else {
	    Byte_Copy(migParam.dataSize, (Address)&migParam.data, outData);
	}
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_RpcNotifyOfMigration --
 * Fs_RpcStartMigration --
 *
 *	The service stub for FsNotifyOfMigration.
 *
 * Results:
 *	FS_STALE_HANDLE if handle that if client that is migrating the file
 *	doesn't have the file opened on this machine.  Otherwise return
 *	SUCCESS.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
Fs_RpcStartMigration(srvToken, clientID, command, storagePtr)
    ClientData srvToken;	/* Handle on server process passed to
				 * Rpc_Reply */
    int clientID;		/* Sprite ID of client host */
    int command;		/* Command identifier */
    Rpc_Storage *storagePtr;    /* The request fields refer to the request
				 * buffers and also indicate the exact amount
				 * of data in the request buffers.  The reply
				 * fields are initialized to NIL for the
				 * pointers and 0 for the lengths.  This can
				 * be passed to Rpc_Reply */
{
    register FsMigInfo		*migInfoPtr;
    register FsHandleHeader	*hdrPtr;
    register ReturnStatus	status;
    register MigrateReply	*migReplyPtr;
    register FsMigParam		*migParamPtr;
    register Rpc_ReplyMem	*replyMemPtr;
    Address    			dataPtr;
    int				dataSize;

    migInfoPtr = (FsMigInfo *) storagePtr->requestParamPtr;

    hdrPtr = (*fsStreamOpTable[migInfoPtr->ioFileID.type].clientVerify)
		(&migInfoPtr->ioFileID, migInfoPtr->srcClientID, (int *)NIL);
    if (hdrPtr == (FsHandleHeader *) NIL) {
	Sys_Panic(SYS_WARNING, "Fs_RpcStartMigration, unknown I/O handle <%d,%d,%d>\n",
	    migInfoPtr->ioFileID.type,
	    migInfoPtr->ioFileID.major, migInfoPtr->ioFileID.minor);
	return(FS_STALE_HANDLE);
    }
    FsHandleUnlock(hdrPtr);
    migParamPtr = Mem_New(FsMigParam);
    migReplyPtr = &(migParamPtr->migReply);
    migReplyPtr->flags = migInfoPtr->flags;
    storagePtr->replyParamPtr = (Address)migParamPtr;
    storagePtr->replyParamSize = sizeof(FsMigParam);
    storagePtr->replyDataPtr = (Address)NIL;
    storagePtr->replyDataSize = 0;
    status = (*fsStreamOpTable[hdrPtr->fileID.type].migrate) (migInfoPtr,
		clientID, &migReplyPtr->flags, &migReplyPtr->offset,
		&dataSize, &dataPtr);
    migParamPtr->dataSize = dataSize;
    if (dataSize > 0) {
	if (dataSize <= sizeof(migParamPtr->data)) {
	    Byte_Copy(dataSize, dataPtr, (Address) &migParamPtr->data);
	    Mem_Free(dataPtr);
	} else {
	    Sys_Panic(SYS_FATAL,
		      "Fs_RpcStartMigration: migrate routine returned oversized data buffer.\n");
	    return(FAILURE);
	}
    } 
	
    FsHandleRelease(hdrPtr, FALSE);

    replyMemPtr = (Rpc_ReplyMem *) Mem_Alloc(sizeof(Rpc_ReplyMem));
    replyMemPtr->paramPtr = storagePtr->replyParamPtr;
    replyMemPtr->dataPtr = (Address) NIL;
    Rpc_Reply(srvToken, status, storagePtr, Rpc_FreeMem,
		(ClientData)replyMemPtr);
    return(SUCCESS);
}


/*
 * ----------------------------------------------------------------------------
 *
 * Fs_GetEncapSize --
 *
 *	Return the size of the encapsulated stream.
 *
 * Results:
 *	The size of the migration information structure.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 *
 */

int
Fs_GetEncapSize()
{
    return(sizeof(FsMigInfo));
}


/*
 *----------------------------------------------------------------------
 *
 * Fs_EncapFileState --
 *
 *	Encapsulate the file system state of a process for migration.  
 *
 * Results:
 *	A pointer to the encapsulated state is returned, along with
 *	the size of the buffer allocated and the number of streams
 *	that were encapsulated.  Any error during stream encapsulation
 *	is returned; otherwise, SUCCESS.
 *
 * Side effects:
 *	Memory is allocated for the buffer.  
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Fs_EncapFileState(procPtr, bufPtr, sizePtr, numEncapPtr)
    register Proc_ControlBlock 	*procPtr;  /* The process being migrated */
    Address *bufPtr;			   /* Pointer to allocated buffer */
    int *sizePtr;			   /* Size of allocated buffer */
    int *numEncapPtr;			   /* Number of streams encapsulated */
{
    Fs_ProcessState *fsPtr;
    int numStreams;
    int numGroups;
    int streamFlagsLen;
    register Address ptr;
    Fs_Stream *streamPtr;
    int i;
    int numEncap;
    ReturnStatus status;


    fsPtr = procPtr->fsPtr;
    numStreams = fsPtr->numStreams;
    /*
     * When sending an array of characters, it has to be even-aligned.
     */
    streamFlagsLen = Byte_AlignAddr(numStreams * sizeof(char));
    *numEncapPtr = numEncap = 0;
    
    /*
     * Send the groups, file permissions, number of streams, and encapsulated
     * current working directory.  For each open file, send the
     * streamID and encapsulated stream contents.
     *
     *	        data			size
     *	 	----			----
     * 		# groups		int
     *	        groups			(# groups) * int
     *		permissions		int
     *		# files			int
     *		per-file flags		(# files) * char
     *		encapsulated files	(# files) * (FsMigInfo + int)
     *		cwd			FsMigInfo
     */
    *sizePtr = (3 + fsPtr->numGroupIDs) * sizeof(int) +
	    streamFlagsLen + numStreams * (sizeof(FsMigInfo) + sizeof(int)) +
	    sizeof(FsMigInfo);
    *bufPtr = Mem_Alloc(*sizePtr);
    ptr = *bufPtr;

    /*
     * Send groups, filePermissions, numStreams, the cwd, and each file.
     */
    
    numGroups = fsPtr->numGroupIDs;
    Byte_FillBuffer(ptr, unsigned int, numGroups);
    if (numGroups > 0) {
	Byte_Copy(numGroups * sizeof(int), (Address) fsPtr->groupIDs, ptr);
	ptr += numGroups * sizeof(int);
    }
    Byte_FillBuffer(ptr, unsigned int, fsPtr->filePermissions);
    Byte_FillBuffer(ptr, int, numStreams);
    if (numStreams > 0) {
	Byte_Copy(numStreams * sizeof(char), (Address) fsPtr->streamFlags,
		  ptr);
	ptr += streamFlagsLen;
    }
    
    status = Fs_EncapStream(fsPtr->cwdPtr, ptr);
    if (status != SUCCESS) {
	Sys_Panic(SYS_WARNING,
		  "Fs_EncapFileState: Error %x from Fs_EncapStream on cwd.\n",
		  status);
	Mem_Free(*bufPtr);
	return(status);
    }
    fsPtr->cwdPtr = (Fs_Stream *) NIL;
    ptr += sizeof(FsMigInfo);
    numEncap += 1;

    for (i = 0; i < fsPtr->numStreams; i++) {
	streamPtr = fsPtr->streamList[i];
	if (streamPtr != (Fs_Stream *) NIL) {
	    numEncap += 1;
	    Byte_FillBuffer(ptr, int, i);
	    status = Fs_EncapStream(streamPtr, ptr);
	    if (status != SUCCESS) {
		Sys_Panic(SYS_WARNING,
			  "Fs_EncapFileState: Error %x from Fs_EncapStream.\n",
			  status);
		Mem_Free(*bufPtr);
		return(status);
	    }
	    fsPtr->streamList[i] = (Fs_Stream *) NIL;
	} else {
	    Byte_FillBuffer(ptr, int, NIL);
	    Byte_Zero(sizeof(FsMigInfo), ptr);
	}	
	ptr += sizeof(FsMigInfo);
    }


    *numEncapPtr = numEncap;
    return(SUCCESS);

}



/*
 *----------------------------------------------------------------------
 *
 * Fs_ClearFileState --
 *
 *	Clear the file state associated with a process after it has
 *	been migrated.  FIXME: Perhaps things should be freed here instead?
 *
 *	Actually, the streams are closed as they are encapsulated, so they
 *	can be nil'ed out as they are encapsulated as well.  This routine
 *	is here only for compatibility with the installed proc and can
 *	be removed after proc is reinstalled.  FD
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The current working directory and open streams of the process
 *	are cleared.
 *
 *----------------------------------------------------------------------
 */

void
Fs_ClearFileState(procPtr)
    register Proc_ControlBlock 	*procPtr;  /* The process being migrated */
{
    int i;
    Fs_ProcessState *fsPtr;

    fsPtr = procPtr->fsPtr;
    if (fsPtr == (Fs_ProcessState *) NIL) {
	Sys_Panic(SYS_FATAL, "Fs_ClearFileState: NIL Fs_ProcessState!");
	return;
    }
    for (i = 0; i < fsPtr->numStreams; i++) {
	fsPtr->streamList[i] = (Fs_Stream *) NIL;
    }
    fsPtr->cwdPtr = (Fs_Stream *) NIL;
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_DeencapFileState --
 *
 *	Get the file system state of a process from another node.  The
 *	buffer contains group information, permissions, the encapsulated
 *	current working directory, and encapsulated streams.
 *
 * Results:
 *	If Fs_DeencapStream returns an error, that error is returned.
 *	Otherwise, SUCCESS is returned.  
 *
 * Side effects:
 *	"Local" Fs_Streams are created and allocated to the foreign process.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Fs_DeencapFileState(procPtr, buffer)
    Proc_ControlBlock *procPtr;
    Address buffer;
{
    register Fs_ProcessState *fsPtr;
    int i;
    int index;
    int numGroups;
    int numStreams;
    ReturnStatus status;

    procPtr->fsPtr = fsPtr = Mem_New(Fs_ProcessState);
    

    /*
     * Get group and permissions information.
     */
    Byte_EmptyBuffer(buffer, unsigned int, numGroups);
    fsPtr->numGroupIDs = numGroups;
    if (numGroups > 0) {
	fsPtr->groupIDs = (int *)Mem_Alloc(numGroups * sizeof(int));
	Byte_Copy(numGroups * sizeof(int), buffer, (Address) fsPtr->groupIDs);
	buffer += numGroups * sizeof(int);
    } else {
	fsPtr->groupIDs = (int *)NIL;
    }
    Byte_EmptyBuffer(buffer, unsigned int, fsPtr->filePermissions);

    /*
     * Get numStreams, flags, and the encapsulated cwd.  Allocate memory
     * for the streams and flags arrays if non-empty.  The array of
     * streamFlags may be an odd number of bytes, so we skip past the
     * byte of padding if it exists (using the Byte_AlignAddr macro).
     */

    Byte_EmptyBuffer(buffer, int, numStreams);
    fsPtr->numStreams = numStreams;
    if (numStreams > 0) {
	fsPtr->streamList = (Fs_Stream **)
		Mem_Alloc(numStreams * sizeof(Fs_Stream *));
	fsPtr->streamFlags = (char *)Mem_Alloc(numStreams * sizeof(char));
	Byte_Copy(numStreams * sizeof(char), buffer,
		  (Address) fsPtr->streamFlags);
	buffer += Byte_AlignAddr(numStreams * sizeof(char));
    } else {
	fsPtr->streamList = (Fs_Stream **)NIL;
	fsPtr->streamFlags = (char *)NIL;
    }
    status = Fs_DeencapStream(buffer, &fsPtr->cwdPtr);
    if (status != SUCCESS) {
	Sys_Panic(SYS_FATAL,
		  "GetFileState: Fs_DeencapStream returned %x for cwd.\n",
		  status);
    }
    buffer += sizeof(FsMigInfo);

    /*
     * Get the other streams.
     */
    for (i = 0; i < fsPtr->numStreams; i++) {
	Byte_EmptyBuffer(buffer, int, index);
	if (index != NIL) {
	    status = Fs_DeencapStream(buffer, &fsPtr->streamList[index]);
	    if (status != SUCCESS) {
#ifdef notdef
		if (status != FAILURE) {
#endif
		    Sys_Panic(SYS_WARNING,
      "Fs_DeencapFileState: Fs_DeencapStream for file id %d returned %x.\n",
			      index, status);
		    return(status);
#ifdef notdef
		}
		fsPtr->streamList[index] = (Fs_Stream *) NIL;
#endif
	    }
	} else {
	    fsPtr->streamList[i] = (Fs_Stream *) NIL;
	}
	buffer += sizeof(FsMigInfo);
    }
    return(SUCCESS);
}


/*
 * ----------------------------------------------------------------------------
 *
 * LocalToRemoteDomain --
 *
 *	Convert a file ID from a local file type to a remote file type.
 *	This is used only for naming and root information, so types
 *	are presumably FS_LCL_FILE_STREAM or FS_RMT_FILE_STREAM.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If the file ID references a local type, it is changed to remote.
 *
 * ----------------------------------------------------------------------------
 *
 */

static void
LocalToRemoteDomain(fileIDPtr)
    FsFileID *fileIDPtr;
{
    if (fileIDPtr->type == FS_LCL_FILE_STREAM) {
	fileIDPtr->type = FS_RMT_FILE_STREAM;
    } else if (fileIDPtr->type != FS_RMT_FILE_STREAM) {
	Sys_Panic(SYS_WARNING,
		  "LocalToRemoteDomain: <%d,%d,%d> is %d, not a file stream.\n",
		  fileIDPtr->serverID,  fileIDPtr->major,
		  fileIDPtr->minor, fileIDPtr->type);
    }
}    

/*
 * ----------------------------------------------------------------------------
 *
 * RemoteToLocalDomain --
 *
 *	Convert a file ID from a local file type to a remote file type.
 *	This is used only for naming and root information, so types
 *	are presumably FS_LCL_FILE_STREAM or FS_RMT_FILE_STREAM.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If the file ID references a local type, it is changed to remote.
 *
 * ----------------------------------------------------------------------------
 *
 */

static void
RemoteToLocalDomain(fileIDPtr)
    FsFileID *fileIDPtr;
{
    if (fileIDPtr->type == FS_RMT_FILE_STREAM) {
	fileIDPtr->type = FS_LCL_FILE_STREAM;
    } else if (fileIDPtr->type != FS_LCL_FILE_STREAM) {
	Sys_Panic(SYS_WARNING,
		  "RemoteToLocalDomain: <%d,%d,%d> is %d, not a file stream.\n",
		  fileIDPtr->serverID,  fileIDPtr->major,
		  fileIDPtr->minor, fileIDPtr->type);
    }
}    
