/* 
 * fsCacheConsist.c --
 *
 *	Routines used to keep the file system caches consistent.  The
 *	server maintains a list of client machines that have the file
 *	open.  This list indicates how many opens per client, if the file
 *	is being cached, and if the file is open for writing on a client.
 *	The client list is updated when files are opened, closed, and removed.
 *
 *	SYNCHRONIZATION:  There are two classes of procedures here: those
 *	that make call-backs to clients, and those that just examine the
 *	client list.  All access to a client list is synchronized using
 *	a monitor lock embedded in the consist structure.  Furthermore,
 *	consistency actions are serialized by setting a flag during
 *	call-backs (CONSIST_IN_PROGRESS).  There is a possible deadlock
 *	if the monitor lock is held during a call-back because this
 *	prevents a close operation from completing. (At close time the
 *	client list is adjusted but no call-backs are made.  Also, the client
 *	has its handle locked which blocks our call-back.)  Accordingly,
 *	both the handle lock and the monitor lock are released during
 *	a call-back.  The CONSIST_IN_PROGRESS flag is left on to prevent
 *	other consistency actions during the call-back.
 *
 * Copyright 1986 Regents of the University of California.
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint

#include	"sprite.h"
#include	"fs.h"
#include	"fsInt.h"
#include	"fsClient.h"
#include	"fsConsist.h"
#include	"fsBlockCache.h"
#include	"fsCacheOps.h"
#include	"fsOpTable.h"
#include	"fsLocalDomain.h"
#include	"fsDebug.h"
#include	"hash.h"
#include	"vm.h"
#include	"proc.h"
#include	"sys.h"
#include	"rpc.h"
#include	"recov.h"
#include	"dbg.h"

#define	LOCKPTR	(&consistPtr->lock)

Boolean	fsCacheDebug = FALSE;
Boolean	fsClientCaching = TRUE;

/*
 * Flags for the FsConsistInfo struct that's defined in fsInt.h
 *
 *	FS_CONSIST_IN_PROGRESS	Cache consistency is being performed on this
 *				file.
 *	FS_CONSIST_ERROR	There was an error during the cache 
 *				consistency.
 */
#define	FS_CONSIST_IN_PROGRESS	0x1
#define	FS_CONSIST_ERROR	0x2

/*
 * Rpc to send when forcing a client to invalidate or write back a file.
 */

typedef struct ConsistMsg {
    FsFileID	fileID;		/* Which file to invalidate. */
    int		flags;		/* One of the flags defined below. */
    int		openTimeStamp;	/* Open that this rpc pertains to. */
} ConsistMsg;

/*
 * Message sent when the client has completed the work requested by the server.
 * This is actually the request part of the rpc transaction.
 */

typedef struct ConsistReply {
    FsFileID 		fileID;
    FsCachedAttributes	cachedAttr;
    ReturnStatus	status;
} ConsistReply;

/*
 * Flags to determine what type of operation is required.
 *
 *    	FS_WRITE_BACK_BLOCKS	Write back all dirty blocks.
 *    	FS_INVALIDATE_BLOCKS	Invalidate all block in the cache for this
 *				file.  This means that the file is no longer
 *				cacheable.
 *    	FS_DELETE_FILE		Delete the file from the local cache and
 *				the file handle table.
 *    	FS_CANT_READ_CACHE_PIPE	The named pipe is no longer read cacheable
 *				on the client.  This would happen if two
 *				separate clients tried to read the named pipe
 *				at the same time.
 *	FS_WRITE_BACK_ATTRS	Write back the cached attributes.
 */

#define	FS_WRITE_BACK_BLOCKS		0x01
#define	FS_INVALIDATE_BLOCKS		0x02
#define	FS_DELETE_FILE			0x04
#define	FS_CANT_READ_CACHE_PIPE		0x08
#define	FS_WRITE_BACK_ATTRS		0x10

/*
 * Structure used to keep track of outstanding cache consistency requests.
 */
typedef struct {
    List_Links	links;
    int		clientID;
    int		flags;
} ConsistMsgInfo;

/*
 * Global time stamp.  A time stamp is returned to clients on each open
 * and on cache consistency messages.  This way clients can detect races
 * between open replies and consistency actions.
 */
static	int	openTimeStamp = 0;

/*
 * Cache conistency statistics.
 */

FsCacheConsistStats fsConsistStats;

/*
 * Forward declarations.
 */
ReturnStatus	StartConsistency();
void 		UpdateList();
ReturnStatus	EndConsistency();
ReturnStatus 	ClientCommand();
void		ProcessConsist();
void		ProcessConsistReply();


/*
 * ----------------------------------------------------------------------------
 *
 * FsConsistInit --
 *
 *      Initialize the client use information for a file.  This is done
 *	before adding any clients so it just resets all the fields.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Reset the client use state..
 *
 * ----------------------------------------------------------------------------
 *
 */
void
FsConsistInit(consistPtr, hdrPtr)
    register FsConsistInfo *consistPtr;	/* State to initialize */
    FsHandleHeader *hdrPtr;		/* Back pointer to handle */
{
    consistPtr->flags = 0;
    consistPtr->lastWriter = -1;
    consistPtr->openTimeStamp = 0;
    consistPtr->hdrPtr = hdrPtr;
    consistPtr->lock.inUse = 0;
    consistPtr->lock.waiting = 0;
    consistPtr->consistDone.waiting = 0;
    consistPtr->repliesIn.waiting = 0;
    List_Init(&consistPtr->clientList);
    List_Init(&consistPtr->msgList);
    List_Init(&consistPtr->migList);
}

/*
 * ----------------------------------------------------------------------------
 *
 * FsFileConsistency --
 *
 *	Take action to ensure that the caches are consistency for this
 *	file.  This checks against use conflicts and will return an
 *	non-SUCCESS status if the open should fail.  Otherwise this
 *	makes call-backs to other clients to keep caches consistent.
 *
 * Results:
 *	SUCCESS or FS_FILE_BUSY.  Also, *cacheablePtr set to TRUE if the
 *	file is cacheable on the client.  *openTimeStampPtr is set to the
 *	next open time stamp on the file.  Clients use this timeStamp
 *	to catch races between open replies, which have the next timeStamp,
 *	and consistency messages from other opens happening
 *	at about the same time, which have the current timeStamp.
 *
 * Side effects:
 *	Issues cache consistency messages and adds the client to the
 *	lis of clients of the file.
 *	THIS UNLOCKS THE HANDLE.
 *
 * ----------------------------------------------------------------------------
 *
 */
ENTRY ReturnStatus
FsFileConsistency(handlePtr, clientID, useFlags, cacheablePtr,
    openTimeStampPtr)
    FsLocalFileIOHandle *handlePtr;	/* File to check consistency of. */
    int 		clientID;	/* ID of the host doing the open */
    register int 	useFlags;	/* useFlags from the open call */
    Boolean		*cacheablePtr;	/* TRUE if file is cacheable. */
    int			*openTimeStampPtr;/* Timestamp of open.  Used by clients
					 * to catch races between open replies
					 * and cache consistency messages */
{
    register FsConsistInfo *consistPtr = &handlePtr->consist;
    ReturnStatus status;

    /*
     * After getting the lock on the client state we unlock the handle
     * so that doesn't interfere with cache consistency actions (like
     * write-backs) by other clients.
     */
    LOCK_MONITOR;
    FsHandleUnlock(handlePtr);

    /*
     * Go through the list of other clients using the file checking
     * for conflicts and issuing cache consistency messages.
     */
    status = StartConsistency(consistPtr, clientID, useFlags, cacheablePtr);
    if (status == SUCCESS) {
	/*
	 * Add ourselves to the list of clients using the file.
	 */
	UpdateList(consistPtr, clientID, useFlags, *cacheablePtr,
		    openTimeStampPtr);
	/*
	 * Now that we are all set up, and have told all the other clients
	 * using the file what they have to do, we wait for them to finish.
	 */
	status = EndConsistency(consistPtr);
    }
    UNLOCK_MONITOR;
    return(status);
}

/*
 * ----------------------------------------------------------------------------
 *
 * StartConsistency --
 *
 *      Initiate cache consistency action on a file.  This decides if the
 *	client can cache the file, and then makes call-backs to other
 *	clients so that caches stay consistent.  Some conflict checking
 *	is done first and this bails out with a non-SUCCESS return code
 *	if the open should fail altogether.  EndConsistency
 *	should be called later to wait for the client replies.
 *
 * Results:
 *	*cacheablePtr set to TRUE if the file is cacheable.   *fileBusyPtr
 *	is set to FS_FILE_BUSY if the file is either being opened for execution
 *	and it is already open for writing or vice versa.
 *
 * Side effects:
 *	Sets the FS_CONSIST_IN_PROGRESS flag and makes call-backs to
 *	clients.  The flag is cleared if the call-backs can't be made.
 *
 * ----------------------------------------------------------------------------
 *
 */

INTERNAL ReturnStatus
StartConsistency(consistPtr, clientID, useFlags, cacheablePtr)
    FsConsistInfo	*consistPtr;	/* File's consistency state. */
    int			clientID;	/* ID of host opening the file */
    int			useFlags;	/* Indicates how they are using it */
    Boolean		*cacheablePtr;	/* Return, TRUE if client can cache */
{
    register FsClientInfo *clientPtr;
    register FsClientInfo *nextClientPtr;
    register FsCacheConsistStats *statPtr = &fsConsistStats;
    register int openForWriting = useFlags & FS_WRITE;
    register ReturnStatus status;
    register Boolean cacheable;

    /*
     * Make sure that noone else is in the middle of performing cache
     * consistency on this file.
     */
    while (consistPtr->flags & FS_CONSIST_IN_PROGRESS) {
	(void) Sync_Wait(&consistPtr->consistDone, FALSE);
    }
    consistPtr->flags = FS_CONSIST_IN_PROGRESS;

    /*
     * Determine cacheability of the file.  Note the system-wide switch
     * to disable client caching.  There are other special cases that
     * are not cached:
     *  1. Swap files are not cached on clients.
     *	2. Non-files, (dirs, links) are not cacheable so we don't have to
     *	   worry about keeping them consistent.  (This could be done.)
     *  3. Files that are being concurrently write-shared are not cached.
     *     This includes one writer and one or more readers on other hosts,
     *	   and writers on multiple hosts.
     */
    cacheable = fsClientCaching;
    if ((useFlags & FS_SWAP) && (clientID != rpc_SpriteID)) {
	cacheable = FALSE;
    } else if (((FsLocalFileIOHandle *)consistPtr->hdrPtr)->descPtr->fileType
		    != FS_FILE) {
	cacheable = FALSE;
    }
    if (cacheable) {
	LIST_FORALL(&consistPtr->clientList, (List_Links *)clientPtr) {
	    if (clientPtr->clientID != clientID) {
		if ((clientPtr->use.write > 0) ||
		    ((clientPtr->use.ref > 0) && openForWriting)) {
		    cacheable = FALSE;
		    goto done;
		}
	    }
	}
    }
done:

    /*
     * Now that we know the cacheable state of the file, check the use
     * by other clients, perhaps sending them cache consistency
     * messages.  For each message we send out (the client replies
     * right-away without actually doing anything yet) ClientCommand
     * adds an entry to the consistInfo's message list.  Note also that
     * client list entries can get removed as side effects of call-backs
     * so we can't use a simple LIST_FOR_ALL here.
     */
    statPtr->numConsistChecks++;
    status = SUCCESS;
    nextClientPtr = (FsClientInfo *)List_First(&consistPtr->clientList);
    while (!List_IsAtEnd(&consistPtr->clientList, (List_Links *)nextClientPtr)){
	clientPtr = nextClientPtr;
	nextClientPtr = (FsClientInfo *)List_Next((List_Links *)clientPtr);

	statPtr->numClients++;
	if (!clientPtr->cached) {
	    /*
	     * Case 1, the other client isn't caching the file. Do nothing.
	     */
	    statPtr->notCaching++;
	} else if (cacheable) {
	    if (consistPtr->lastWriter != clientPtr->clientID) {
		/*
		 * Case 2, the other client is caching and it's ok.
		 */
		statPtr->readCaching++;
	    } else if (consistPtr->lastWriter == clientID) {
		/*
		 * Case 3, the last writer is now opening for reading.
		 * Its dirty cache is ok.
		 */
		statPtr->writeCaching++;
	    } else {
		/*
		 * Case 4, the last writer needs to give us back the
		 * dirty blocks so the opening client will get good data.
		 */
		status = ClientCommand(consistPtr, clientPtr,
					FS_WRITE_BACK_BLOCKS);
		statPtr->writeBack++;
	    }
	} else {
	    if (clientPtr->use.write == 0) {
		/*
		 * Case 5, another reader needs to stop caching.
		 */
		status = ClientCommand(consistPtr, clientPtr,
					FS_INVALIDATE_BLOCKS);
		statPtr->readInvalidate++;
	    } else {
		/*
		 * Case 6, the writer needs to stop caching and give
		 * us back its dirty blocks.
		 */
		status = ClientCommand(consistPtr, clientPtr,
			    FS_WRITE_BACK_BLOCKS | FS_INVALIDATE_BLOCKS);
		statPtr->writeInvalidate++;
	    }
	}
	if (status != SUCCESS) {
	    Sys_Panic(SYS_WARNING,
		"StartConsistency, error 0x%x client %d\n",
		status, clientPtr->clientID);
	    consistPtr->flags = 0;
	}
    }
    *cacheablePtr = cacheable;
    return(status);
}

/*
 * ----------------------------------------------------------------------------
 *
 * UpdateList --
 *
 *	Update the state of the client that is using one of our files.
 *	A timestamp is generated for return to the client so it can
 *	catch races between the open return message and cache consistency
 *	messages associated with this file.
 *
 * Results:
 *	The timestamp.
 *
 * Side effects:
 *	The version number is incremented when open for writing.  The
 *	lastWriter is remembered when opening for writing.  Use counts
 *	are kept to reflect the clients use of the file.  Finally,
 *	client list entries for hosts no longer using or caching the
 *	file are deleted.
 *
 * ----------------------------------------------------------------------------
 */

INTERNAL void
UpdateList(consistPtr, clientID, useFlags, cacheable,
	   openTimeStampPtr)
    register FsConsistInfo *consistPtr;	/* Consistency state for the file. */
    int			clientID;	/* ID of client using the file */
    int			useFlags;	/* FS_READ|FS_WRITE|FS_EXECUTE */
    Boolean		cacheable;	/* TRUE if client is caching the file */
    int			*openTimeStampPtr;/* Generated for the client so it can
					 * catch races between the return from
					 * this open and other cache messages */
{
    register	FsClientInfo	*clientPtr;	/* State for other clients */

    /*
     * Add the client to the I/O handle client list.
     */
    clientPtr = FsIOClientOpen(&consistPtr->clientList, clientID, useFlags,
				cacheable);

    if (cacheable && (useFlags & FS_WRITE)) {
	consistPtr->lastWriter = clientID;
    }
    /*
     * Return a time stamp for the open.  This timestamp is used by clients
     * to catch races between the reply message for an open, and a cache
     * consistency message generated from a different open happening at
     * about the same time.
     */
    if (openTimeStampPtr != (int *)NIL) {
	openTimeStamp++;
	*openTimeStampPtr =
	    consistPtr->openTimeStamp =
		clientPtr->openTimeStamp = openTimeStamp;
    } else {
	consistPtr->openTimeStamp = clientPtr->openTimeStamp = 0;
    }
}


/*
 * ----------------------------------------------------------------------------
 *
 * EndConsistency --
 *	Wait for cache consistency actions to complete.
 *
 * Results:
 *	SUCCESS or FS_NO_DISK_SPACE.  We have to abort an open if a client
 *	cannot write back its cache in response to a consistency message.
 *	This only happens if the disk is full.  If the client is down we
 *	won't wait for it.  If it is not really down we treat it as
 *	down and will abort its attempt to re-open later.
 *
 * Side effects:
 *	Notifies the consistDone condition to allow someone else to
 *	open the file.  Clears the FS_CONSIST_IN_PROGRES flag.
 *
 * ----------------------------------------------------------------------------
 *
 */

INTERNAL ReturnStatus
EndConsistency(consistPtr)
    FsConsistInfo	*consistPtr;
{
    register ReturnStatus status;

    while (!List_IsEmpty(&consistPtr->msgList)) {
	(void) Sync_Wait(&consistPtr->repliesIn, FALSE);
    }
    if (consistPtr->flags & FS_CONSIST_ERROR) {
	/*
	 * The only reason consistency actions fail altogether is when
	 * a client can't do a writeback because there isn't enough
	 * disk space here.  Crashed clients don't matter.
	 */
	status = FS_NO_DISK_SPACE;
    } else {
	status = SUCCESS;
    }
    consistPtr->flags = 0;
    Sync_Broadcast(&consistPtr->consistDone);
    return(status);
}

/*
 * ----------------------------------------------------------------------------
 *
 * FsReopenConsistency --
 *
 *	Perform cache consistency actions for a file that the client had 
 *	open before we crashed.  Add the client to the set of clients using 
 *	the file.  As a side effect messages may be sent to other clients to
 *	maintain the consistency of the various clients' caches.  If we can't
 *	provide the cacheability expected by the caller then return an error.
 *
 *	NOTE: This routine must be called with the handle locked.
 *
 * Results:
 *	TRUE if could provide the cacheability expected, FALSE otherwise.
 * Side effects:
 *	*cacheablePtr is TRUE if the file is to be cached by the client.
 *	*openTimeStampPtr is set for use by clients.  They keep this timestamp
 *	in order to catch races between the return from their open request
 *	(which we are part of) and cache consistency messages resulting from
 *	opens occuring at about the same time.
 *      The client is added to the client list for the handle.  RPC's may
 *      be done to the other clients using the file to tell them to write
 *      back dirty blocks, or to invalidate their cached blocks.
 *	THIS UNLOCKS THE HANDLE.
 *
 * ----------------------------------------------------------------------------
 */
ENTRY ReturnStatus
FsReopenConsistency(handlePtr, clientID, use, cacheablePtr,
	useChangePtr, openTimeStampPtr)
    FsLocalFileIOHandle	*handlePtr;	/* Locked upon entry.  This routine
					 * unlocks it before returning. */
    int			clientID;	/* The client who is opening the file.*/
    FsUseCounts		use;		/* Clients usage of the file */
    Boolean 		*cacheablePtr;	/* IN: TRUE if client expects it to be
					 * cacheable.
					 * OUT: Cacheability of the file. */
    FsUseCounts		*useChangePtr;	/* Actual difference between current */
					/* use for the client and the current
					 * use for the whole file. */
    int			*openTimeStampPtr; /* Place to return a timestamp for 
					 * this open.*/
{
    int				useFlags = 0;
    Boolean			cacheable;
    ReturnStatus		status = SUCCESS;
    Boolean			found;
    register FsConsistInfo	*consistPtr = &handlePtr->consist;
    register FsClientInfo	*clientPtr;

    LOCK_MONITOR;
    FsHandleUnlock(handlePtr);

    if (use.ref > use.write + use.exec) {
	useFlags |= FS_READ;
    }
    if (use.write > 0 || *cacheablePtr) {
	useFlags |= FS_WRITE;
    }
    if (use.exec > 0) {
	useFlags |= FS_EXECUTE;
    }

    /*
     * Find the current client.
     */
    found = FALSE;
    LIST_FORALL(&(consistPtr->clientList), (List_Links *)clientPtr) {
	if (clientPtr->clientID == clientID) {
	    found = TRUE;
	    break;
	}
    }

    if (consistPtr->lastWriter == clientID && !*cacheablePtr) {
	/*
	 * Client was the last writer but it no longer has any dirty blocks,
	 * thus it is the last writer no longer.
	 */
	consistPtr->lastWriter = -1;
    }

    if (useFlags != 0) {
	if (!found) {
	    /*
	     * If we don't know about this client yet then perform full
	     * cache consistency on this file.
	     */
	    status = StartConsistency(consistPtr, clientID, useFlags,
				    &cacheable);
	    if (status == SUCCESS && (!cacheable && *cacheablePtr)) {
		/*
		 * Can't give the cacheability needed by the client.  This
		 * is because it had dirty blocks but we let someone else
		 * open for reading or writing.
		 */
		Sys_Panic(SYS_WARNING,
		    "FsReopenConsistency: cacheable conflict file <%d,%d>\n",
		    handlePtr->hdr.fileID.major, handlePtr->hdr.fileID.minor);
		status = FS_VERSION_MISMATCH;
		(void)EndConsistency(consistPtr);
	    }
	    if (status != SUCCESS) {
		goto exit;
	    } 
	} else {
	    /*
	     * We already know about this client.
	     */
	    cacheable = clientPtr->cached;
	}
    } else {
	/*
	 * The client isn't using the file anymore so clean up state.
	 */
	if (found) {
	    List_Remove((List_Links *)clientPtr);
	    Mem_Free((Address)clientPtr);
	}
	*openTimeStampPtr = consistPtr->openTimeStamp;
	useChangePtr->ref = 0;
	useChangePtr->write = 0;
	useChangePtr->exec = 0;
	UNLOCK_MONITOR;
	return(SUCCESS);
    }

    /*
     * At this point the re-open is ok and we have initiated any related
     * cache consistency actions by other clients.  Now we update the
     * client list to reflect what the client's current usage.  We return
     * the change in usage so our caller can update global use counts.
     */
    if (!found) {
	clientPtr = Mem_New(FsClientInfo);
	clientPtr->clientID = clientID;
	List_Insert((List_Links *) clientPtr,
		LIST_ATFRONT(&consistPtr->clientList));
	*useChangePtr = use;
    } else {
	useChangePtr->ref = use.ref - clientPtr->use.ref;
	useChangePtr->write = use.write - clientPtr->use.write;
	useChangePtr->exec = use.exec - clientPtr->use.exec;
    }
    clientPtr->use = use;
    /*
     * Get a new openTimeStamp so the client can detect races between
     * the reopen return and consistency messages generated by other
     * opens happening right now (or real soon).
     */
    openTimeStamp++;
    *openTimeStampPtr =
	consistPtr->openTimeStamp =
	    clientPtr->openTimeStamp = openTimeStamp;
    /*
     * Mark the client as caching or not.
     */
    clientPtr->cached = cacheable;
    if ((useFlags & FS_WRITE) && cacheable) {
	consistPtr->lastWriter = clientID;
    }

    /*
     * Wait for cache consistency call-backs to complete.
     */
    status = EndConsistency(consistPtr);
exit:
    *cacheablePtr = cacheable;
    UNLOCK_MONITOR;
    return(status);
}

/*
 * ----------------------------------------------------------------------------
 *
 * FsMigrateConsistency --
 *
 *	Shift the references on a file from one client to another.  If
 *	the stream to this handle is not shared accross network this
 *	is done by first removing the useCounts due to the srcClient,
 *	and them performing the regular cache consistency algorithm as
 *	if the dstClient is opening the file.  If the stream is shared
 *	things are more complicated.  We must not close the original
 *	client until all stream references to its I/O handle have migrated,
 *	and we must be careful to not add too many refererences to the
 *	new client.
 *
 * Results:
 *	SUCCESS, unless there is a cache consistency conflict detected.
 *
 * Side effects:
 *	The HANDLE RETURNS UNLOCKED.  Also, full-fledged cache consistency
 *	actions are taken.
 *
 * ----------------------------------------------------------------------------
 */

ENTRY ReturnStatus
FsMigrateConsistency(handlePtr, srcClientID, dstClientID, useFlags,
	cacheablePtr, openTimeStampPtr)
    FsLocalFileIOHandle	*handlePtr;	/* Returned UNLOCKED */
    int			srcClientID;	/* ID of client using the file */
    int			dstClientID;	/* ID of client using the file */
    int			useFlags;	/* FS_READ|FS_WRITE|FS_EXECUTE
					 * FS_RMT_SHARED if shared now.
					 * FS_NEW_STREAM if dstClientID is
					 * getting stream for first time. */
    Boolean		*cacheablePtr;	/* Return - Cachability of file */
    int			*openTimeStampPtr;/* Generated for the client so it can
					 * catch races between the return from
					 * this open and other cache messages */
{
    register FsConsistInfo *consistPtr = &handlePtr->consist;
    Boolean			cache = TRUE;
    register	ReturnStatus	status;

    LOCK_MONITOR;
    FsHandleUnlock(handlePtr);

    if ((useFlags & FS_RMT_SHARED) == 0) {
	/*
	 * Remove references due to the original client so it doesn't confuse
	 * the regular cache consistency algorithm.  Note that this call doesn't
	 * disturb the last writer state so any dirty blocks on the old client
	 * will get handled properly.
	 */
	if (!FsIOClientClose(&consistPtr->clientList, srcClientID, useFlags,
		&cache)) {
	    Sys_Panic(SYS_WARNING,
		    "FsMigrateConsistency, srcClient %d unknown\n",
		    srcClientID);
	}
    }
    /*
     * The rest of this is like regular cache consistency.
     */

    status = StartConsistency(consistPtr, dstClientID, useFlags, cacheablePtr);
    if (status == SUCCESS) {
	if (useFlags & FS_NEW_STREAM) {
	    /*
	     * The client is getting the stream to this I/O handle for
	     * the first time so we should add it as a client.  We are
	     * careful about this because there is only one reference to
	     * the I/O handle per client per stream. 
	     */
	    UpdateList(consistPtr, dstClientID, useFlags, *cacheablePtr,
			openTimeStampPtr);
	}
	status = EndConsistency(consistPtr);
    }

    UNLOCK_MONITOR;
    return(status);
}


/*
 * ----------------------------------------------------------------------------
 *
 * FsGetClientAttrs --
 *
 *	This does call-backs to clients to flush back cached attributes.
 *	Files that are being executed are treated as a special case.  They
 *	are not being written so we, the server, have the right size and
 *	modify time.  Instead of getting access times from clients (there
 *	could be lots and lots) we just return a flag that says the
 *	file is being executed.  Our caller will presumably use the
 *	the current time for the access time in this case.
 *
 * Results:
 *	*isExecedPtr set to TRUE if the file is being executed.
 *
 * Side effects:
 *      RPC's may be done to clients using the file to tell them to write
 *	back their cached attributes.
 *	This unlocks the handle while waiting for clients, but
 *	re-locks it before returning.
 *
 * ----------------------------------------------------------------------------
 */
ENTRY void
FsGetClientAttrs(handlePtr, clientID, isExecedPtr)
    register FsLocalFileIOHandle *handlePtr;
    int			clientID;	/* The client who is doing the stat.*/
    Boolean		*isExecedPtr;	/* TRUE if file is being executed.
					 * Our caller will use the current
					 * time for the access time in this
					 * case */
{
    register Boolean		isExeced = FALSE;
    register FsConsistInfo	*consistPtr = &handlePtr->consist;
    register FsClientInfo	*clientPtr;
    register FsClientInfo	*nextClientPtr;

    LOCK_MONITOR;

    /*
     * Unlock the handle so other operations on the file can proceed while
     * we probe around the network for the most up-to-date attributes.
     * Otherwise the following deadlock is possible:
     * Client 1 does a get attributes about the same time client 2 does
     * a close.  Client 2 has its handle locked during the close, but we
     * will be calling back to it to get its attributes.  Our callback can't
     * start until client 2 unlocks its handle, but it can't unlock it's
     * handle until the close finishes.  The close can't finish because
     * it is blocked here on the locked handle.
     */
    FsHandleUnlock(handlePtr);

    /*
     * Make sure that noone else is in the middle of performing cache
     * consistency on this handle.  If so wait until they are done.
     */
    while (consistPtr->flags & FS_CONSIST_IN_PROGRESS) {
	(void) Sync_Wait(&consistPtr->consistDone, FALSE);
    }
    consistPtr->flags = FS_CONSIST_IN_PROGRESS;

    /*
     * Go through the set of clients using the file and see if they
     * are caching attributes.
     */
    nextClientPtr = (FsClientInfo *)List_First(&consistPtr->clientList);
    while (!List_IsAtEnd(&consistPtr->clientList, (List_Links *)nextClientPtr)){
	clientPtr = nextClientPtr;
	nextClientPtr = (FsClientInfo *)List_Next((List_Links *)clientPtr);

	if (clientPtr->use.exec > 0) {
	    isExeced = TRUE;
	}
	if ((clientPtr->cached) && (clientPtr->use.ref > 0)) {
	    if ((clientPtr->clientID != clientID) &&
		(clientPtr->use.exec == 0)) {
		/*
		 * Don't call back to our caller because it will check
		 * its own attributes later.  Also, don't send rpcs to
		 * clients executing files.  For these clients we just
		 * set the access time to the current time.  This
		 * is an optimization to not make stating of binaries
		 * abysmally slow.
		 */
		ClientCommand(consistPtr, clientPtr, FS_WRITE_BACK_ATTRS);
	    }
	}
    }
    /*
     * Now that we are all set up, and have told all the other clients using
     * the file what they have to do, we wait for them to finish.
     */
    (void)EndConsistency(consistPtr);

    FsHandleLock(handlePtr);
    *isExecedPtr = isExeced;
    UNLOCK_MONITOR;
}

/*
 * ----------------------------------------------------------------------------
 *
 * FsConsistClose --
 *
 *	A thin layer on top of FsIOClientClose that also cleans up
 *	the last writer of a file.
 *
 * Results:
 *	TRUE if there was a record that the client was using the file.
 *	This is used to trap out invalid closes.  Also, *wasCachedPtr
 *	is set to TRUE if the file (in particular, its attributes) was
 *	cached on the client.
 *
 * Side effects:
 *	The client list entry is removed.  If the client was
 *	the last writer but has no dirty blocks (as indicated by the flags)
 *	then the last writer field of the consist info is cleared.
 *
 * ----------------------------------------------------------------------------
 *
 */
ENTRY Boolean
FsConsistClose(consistPtr, clientID, flags, wasCachedPtr)
    register FsConsistInfo *consistPtr;	/* Handle of file being closed */
    int			clientID;	/* Host ID of client that had it open */
    register int	flags;		/* Flags from the stream. */
    Boolean		*wasCachedPtr;	/* TRUE upon return if the client was
					 * caching (attributes) of the file. */
{
    LOCK_MONITOR;

    if (!FsIOClientClose(&consistPtr->clientList, clientID, flags,
			 wasCachedPtr)) {
	UNLOCK_MONITOR;
	return(FALSE);
    }

    if ((consistPtr->lastWriter != -1) && (flags & FS_LAST_DIRTY_BLOCK)) {
	if (clientID != consistPtr->lastWriter) {
	    Sys_Panic(SYS_FATAL, "FsConsistClose, bad last writer, %d not %d\n",
		    consistPtr->lastWriter, clientID);
	} else {
	    consistPtr->lastWriter = -1;
	}
    }
    UNLOCK_MONITOR;
    return(TRUE);
}


/*
 * ----------------------------------------------------------------------------
 *
 * FsConsistClients --
 *
 *	Returns the number of clients in the client list for a file.
 *	Called to see if it's ok to scavenge the file handle.
 *
 * Results:
 *	The number of clients in the client list.
 *
 * Side effects:
 *	Unused client list entries are cleaned up.  We only need to remember
 *	clients that are actively using the file or who have dirty blocks
 *	because they are the last writer.
 *
 * ----------------------------------------------------------------------------
 *
 */
ENTRY int
FsConsistClients(consistPtr)
    register FsConsistInfo *consistPtr;	/* Handle of file being closed */
{
    register int numClients = 0;
    register FsClientInfo *clientPtr;
    register FsClientInfo *nextClientPtr;

    LOCK_MONITOR;

    nextClientPtr = (FsClientInfo *)List_First(&consistPtr->clientList);
    while (!List_IsAtEnd(&consistPtr->clientList, (List_Links *)nextClientPtr)){
	clientPtr = nextClientPtr;
	nextClientPtr = (FsClientInfo *)List_Next((List_Links *)clientPtr);

	if (clientPtr->use.ref == 0 &&
	    clientPtr->clientID != consistPtr->lastWriter) {
	    List_Remove((List_Links *) clientPtr);
	    Mem_Free((Address) clientPtr);
	} else {
	    numClients++;
	}
    }
    UNLOCK_MONITOR;
    return(numClients);
}


/*
 * ----------------------------------------------------------------------------
 *
 * FsDeleteLastWriter --
 *
 *	Remove the last writer from the consistency list.  This is called
 *	from the write rpc stub when the last block of a file comes
 *	in from a remote client.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Removes the client list entry for the last writer if the last
 *	writer is no longer using the file.
 *
 * ----------------------------------------------------------------------------
 *
 */
ENTRY void
FsDeleteLastWriter(consistPtr, clientID)
    FsConsistInfo *consistPtr;
    int		clientID;
{
    register	FsClientInfo	*clientPtr;

    LOCK_MONITOR;

    LIST_FORALL(&consistPtr->clientList, (List_Links *) clientPtr) {
	if (clientPtr->clientID == clientID) {
	    if (clientPtr->use.ref == 0 &&
		consistPtr->lastWriter == clientID) {
		List_Remove((List_Links  *) clientPtr);
		Mem_Free((Address) clientPtr);
		break;
	    }
	}
    }
    consistPtr->lastWriter = -1;
    UNLOCK_MONITOR;
}

/*
 * ----------------------------------------------------------------------------
 *
 * FsClientRemoveCallback --
 *
 *      Called when a file is deleted.  Send an rpc to everyone who has
 *      this file cached telling them to delete it from their cache.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Any remotely cached data is invalidated.
 *	All the entries in the client list are deleted.
 *
 * ----------------------------------------------------------------------------
 *
 */
ENTRY void
FsClientRemoveCallback(consistPtr, clientID)
    FsConsistInfo *consistPtr;	/* File to check */
    int		clientID;	/* Client who is removing the file.  This
				 * host is not contacted via call-back.
				 * Instead, the current RPC (close) should
				 * return FS_FILE_REMOVED. */
{
    register	FsClientInfo	*clientPtr;

    LOCK_MONITOR;

    FsHandleUnlock(consistPtr->hdrPtr);

    while (consistPtr->flags & FS_CONSIST_IN_PROGRESS) {
	(void) Sync_Wait(&consistPtr->consistDone, FALSE);
    }
    consistPtr->flags = FS_CONSIST_IN_PROGRESS;

    /*
     * Loop through the list notifying clients and deleting client elements.
     */
    while (!List_IsEmpty((List_Links *)&consistPtr->clientList)) {
	clientPtr = (FsClientInfo *)
		    List_First((List_Links *) &consistPtr->clientList);
	if (clientPtr->use.ref > 0) {
	    Sys_Panic(SYS_FATAL,
		"FsClientRemoveCallback: Client using removed file\n");
	} else if (consistPtr->lastWriter != -1) {
	    if (clientPtr->clientID != consistPtr->lastWriter) {
		Sys_Panic(SYS_FATAL,
		    "FsClientRemoveCallback: Non last writer in list.\n");
	    } else if (clientPtr->clientID != clientID) {
		/*
		 * Tell the client caching the file to remove it from its cache.
		 * This should only be the last writer as we are called only
		 * when it is truely time to remove the file.
		 */
		(void)ClientCommand(consistPtr, clientPtr, FS_DELETE_FILE);
		while (!List_IsEmpty(&consistPtr->msgList)) {
		    (void) Sync_Wait(&consistPtr->repliesIn, FALSE);
		}
	    }
	}
	List_Remove((List_Links *) clientPtr);
	Mem_Free((Address) clientPtr);
    }
    consistPtr->flags = 0;
    consistPtr->lastWriter = -1;
    FsHandleLock(consistPtr->hdrPtr);
    UNLOCK_MONITOR;
}

/*
 * ----------------------------------------------------------------------------
 *
 * FsConsistKill --
 *
 *	Find and remove the given client in the list for the handle.  The
 *	number of client references, writers, and executers is returned
 *	so our caller can clean up the reference counts in the handle.
 *
 * Results:
 *	*inUsePtr set to TRUE if the client has the file open, *writingPtr
 *	set to TRUE if the client has the file open for writing, and 
 *	*executingPtr set to TRUE if the client has the file open for
 *	execution.
 *	
 * Side effects:
 *	If this client was the last writer for the file then the last
 *	writer field in the handle is set to -1.
 *
 * ----------------------------------------------------------------------------
 *
 */
ENTRY void
FsConsistKill(consistPtr, clientID, refPtr, writePtr, execPtr)
    FsConsistInfo *consistPtr;	/* Consistency state from which to remove
				 * the client. */
    int		clientID;	/* Client to delete. */
    int		*refPtr;	/* Number of times client has file open. */
    int		*writePtr;	/* Number of times client is writing file. */
    int		*execPtr;	/* Number of times clients is executing file.*/
{
    register ConsistMsgInfo 	*msgPtr;

    LOCK_MONITOR;

    FsIOClientKill(&consistPtr->clientList, clientID, refPtr, writePtr,
		    execPtr);

    if (consistPtr->lastWriter == clientID) {
	consistPtr->lastWriter = -1;
    }
    /*
     * Remove the client from the list of clients involved in a cache
     * consistency action so we don't hang on the dead client.
     */
    LIST_FORALL(&(consistPtr->msgList), (List_Links *) msgPtr) {
	if (msgPtr->clientID == clientID) {
	    List_Remove((List_Links *) msgPtr);
	    Mem_Free((Address) msgPtr);
	    Sync_Broadcast(&consistPtr->consistDone);
	    break;
	}
    }

    UNLOCK_MONITOR;
}

/*
 *----------------------------------------------------------------------------
 *
 * FsGetAllDirtyBlocks --
 *
 *	Retrieve dirty blocks from all clients that have files open on the
 *	given domain.  This is called when a disk is being detached.  We
 *	have to get any outstanding data before taking the disk off-line.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------------
 */
void
FsGetAllDirtyBlocks(domain, invalidate)
    int		domain;		/* Domain to get dirty blocks for. */
    Boolean	invalidate;	/* Remove file from cache after getting blocks
				 * back. */
{
    Hash_Search				hashSearch;
    register	FsHandleHeader		*hdrPtr;

    Hash_StartSearch(&hashSearch);
    for (hdrPtr = FsGetNextHandle(&hashSearch);
	 hdrPtr != (FsHandleHeader *) NIL;
         hdrPtr = FsGetNextHandle(&hashSearch)) {
	if (hdrPtr->fileID.type == FS_LCL_FILE_STREAM &&
	    hdrPtr->fileID.major == domain) {
	    register FsLocalFileIOHandle *handlePtr =
		    (FsLocalFileIOHandle *) hdrPtr;
	    FsFetchDirtyBlocks(&handlePtr->consist, invalidate);
	}
	FsHandleUnlock(hdrPtr);
    }
}

/*
 * ----------------------------------------------------------------------------
 *
 * FsFetchDirtyBlocks --
 *
 *      Fetch dirty blocks back from the last writer of the file.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If the last writer doesn't have the file actively open, then it is
 *	no longer the last writer and it is deleted from the client list.
 *
 * ----------------------------------------------------------------------------
 *
 */
ENTRY void
FsFetchDirtyBlocks(consistPtr, invalidate)
    FsConsistInfo *consistPtr;	/* Consistency state for file. */
    Boolean	invalidate;	/* If TRUE the client is told to invalidate
				 * after writing back the blocks */
{
    register	FsClientInfo	*clientPtr;
    register	FsClientInfo	*nextClientPtr;

    LOCK_MONITOR;
    FsHandleUnlock(consistPtr->hdrPtr);

    /*
     * Make sure that noone else is in the middle of performing cache
     * consistency on this handle.  If so wait until they are done.
     */
    while (consistPtr->flags & FS_CONSIST_IN_PROGRESS) {
	(void) Sync_Wait(&consistPtr->consistDone, FALSE);
    }
    /*
     * See if there are any dirty blocks to fetch.
     */
    if (consistPtr->lastWriter == -1 ||
	consistPtr->lastWriter == rpc_SpriteID) {
	FsHandleLock(consistPtr->hdrPtr);
        UNLOCK_MONITOR;
	return;
    }
    consistPtr->flags = FS_CONSIST_IN_PROGRESS;

    /*
     * There is a last writer.  In this case the only thing on the list is
     * the client that is the last writer because we know the domain for
     * the file is in-active.
     */
    nextClientPtr = (FsClientInfo *)List_First(&consistPtr->clientList);
    while (!List_IsAtEnd(&consistPtr->clientList, (List_Links *)nextClientPtr)){
	clientPtr = nextClientPtr;
	nextClientPtr = (FsClientInfo *)List_Next((List_Links *)clientPtr);

	if (clientPtr->clientID != consistPtr->lastWriter) {
	    FsHandleLock(consistPtr->hdrPtr);
	    consistPtr->flags = 0;
	    UNLOCK_MONITOR;
	    Sys_Panic(SYS_FATAL,
		    "FsFetchDirtyBlocks: Non last writer in list.\n");
	    return;
	} else if (clientPtr->use.write > 0) {
	    /*
	     * The client has it actively open for writing.  In this case don't
	     * do anything because he can have more dirty blocks anyway.
	     */
	} else {
	    ReturnStatus	status;
	    register int	flags = FS_WRITE_BACK_BLOCKS;
	    if (invalidate) {
		flags |= FS_INVALIDATE_BLOCKS;
	    }
	    status = ClientCommand(consistPtr, clientPtr, flags);
	    if (status == SUCCESS) {
		(void)EndConsistency(consistPtr);
	    }
	}
    }
    consistPtr->flags = 0;
    FsHandleLock(consistPtr->hdrPtr);
    UNLOCK_MONITOR;
    return;
}

/*
 * The consistency messages are issued in two parts.  In the first phase
 * the server issues all the clients commands, but the clients return
 * an RPC reply immediately and schedule ProcessConsist in the background
 * to actually effect the cache consistency.  The second phase consists
 * of the server waiting around for a return RPC by the client that
 * indicates that it is done.
 */
void	ProcessConsist();
void	ProcessConsistReply();
/*
 * Consist message information.
 */
typedef struct ConsistItem {
    int		serverID;
    ConsistMsg args;
} ConsistItem;

/*
 * ----------------------------------------------------------------------------
 *
 * ClientCommand --
 *
 *	Send an rpc to a client telling him to perform some cache 
 *	consistency operation.  Also put the client onto a list of 
 *	outstanding cache consistency messages.
 *
 * Results:
 *	The status from the RPC.
 *
 * Side effects:
 *      Client added to list of outstanding cache consistency messages.
 *
 * ----------------------------------------------------------------------------
 *
 */

INTERNAL ReturnStatus
ClientCommand(consistPtr, clientPtr, flags)
    register FsConsistInfo *consistPtr;	/* Consistency state of file */
    FsClientInfo	*clientPtr;	/* State of other client's cache */
    int			flags;		/* Command for the other client */
{
    Rpc_Storage		storage;
    ConsistMsg		consistRpc;
    ReturnStatus	status;
    ConsistMsgInfo	*msgPtr;

    if (clientPtr->clientID == rpc_SpriteID) {
	/*
	 * Don't issue commands to ourselves (the server) because the commands
	 * issued to the other clients (write-back, invalidate, etc.) will
	 * all result in a consistent server cache anyway.
	 */
	if (flags & FS_INVALIDATE_BLOCKS) {
	    /*
	     * If we told ourselves to invalidate the file then mark us
	     * as not caching the file.
	     */
	    clientPtr->cached = FALSE;
	}
	if (flags & FS_WRITE_BACK_BLOCKS) {
	    /*
	     * We already have the most recent blocks.
	     */
	    consistPtr->lastWriter = -1;
	}
	if (clientPtr->use.ref == 0 && 
	    consistPtr->lastWriter != clientPtr->clientID) {
	    FS_CACHE_DEBUG_PRINT1("ClientCommand: Removing %d ",
					clientPtr->clientID);
	    List_Remove((List_Links *) clientPtr);
	    Mem_Free((Address) clientPtr);
	}
	return(SUCCESS);
    }
    /*
     * Map to the client's view of the file (i.e. remote).  The openTimeStamp
     * lets the client catch races between this message and the reply
     * to an open it may be making at the same time.
     */
    consistRpc.fileID = consistPtr->hdrPtr->fileID;
    switch (consistPtr->hdrPtr->fileID.type) {
	case FS_LCL_FILE_STREAM:
	    consistRpc.fileID.type = FS_RMT_FILE_STREAM;
	    break;
	default:
	    Sys_Panic(SYS_FATAL, "ClientCommand, bad stream type <%d>\n",
		consistPtr->hdrPtr->fileID.type);
	    break;
    }
    consistRpc.flags = flags;
    consistRpc.openTimeStamp = clientPtr->openTimeStamp;

    storage.requestParamPtr = (Address) &consistRpc;
    storage.requestParamSize = sizeof(ConsistMsg);
    storage.requestDataPtr = (Address) NIL;
    storage.requestDataSize = 0;

    storage.replyParamPtr = (Address) NIL;
    storage.replyParamSize = 0;
    storage.replyDataPtr = (Address) NIL;
    storage.replyDataSize = 0;

    /*
     * Put the client onto the list of outstanding cache consistency
     * messages.
     */

    msgPtr = (ConsistMsgInfo *) Mem_Alloc(sizeof(ConsistMsgInfo));
    msgPtr->clientID = clientPtr->clientID;
    msgPtr->flags = consistRpc.flags;
    List_Insert((List_Links *) msgPtr, LIST_ATREAR(&consistPtr->msgList));

    /*
     * Have to release this monitor during the call-back so that
     * an unrelated close can complete its call to FsConsistClose.
     */
    if ((consistPtr->flags & FS_CONSIST_IN_PROGRESS) == 0) {
	Sys_Panic(SYS_FATAL, "Client CallBack - consist flag not set\n");
    }
    UNLOCK_MONITOR;
    while ( TRUE ) {
	/*
	 * Send the rpc to the client.  The client will return FAILURE if the
	 * open that we are talking about hasn't returned to the client yet.
	 */
	status = Rpc_Call(clientPtr->clientID, RPC_FS_CONSIST, &storage);
	if (status != FAILURE) {
	    break;
	} else {
	    Sys_Panic(SYS_WARNING, "Client %d dropping consist request %x\n",
		clientPtr->clientID, flags);
	}
    }
    LOCK_MONITOR;

    if (status != SUCCESS) {
	/*
	 * Couldn't post call-back to the client.
	 */
	List_Remove((List_Links *)msgPtr);
	Mem_Free((Address)msgPtr);
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Fs_RpcConsist --
 *
 *	Service stub for RPC_FS_CONSIST.  This is executed on a filesystem
 *	client in response to a cache consistency command.  This schedules
 *	a call to ProcessConsist and returns a reply to the server.
 *	If, by remote chance, the message concerns an in-progress open
 *	that hasn't yet completed, we just return an error code to the
 *	server so that it will re-try.
 *
 * Results:
 *	SUCCESS.
 *
 * Side effects:
 *	Add work to the queue for the client consistency process.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
Fs_RpcConsist(srvToken, clientID, command, storagePtr)
    ClientData srvToken;	/* Handle on server process passed to
				 * Rpc_Reply */
    int clientID;		/* ID of server controlling the file */
    int command;		/* Command identifier */
    Rpc_Storage *storagePtr;	/* The request fields refer to the request
				 * buffers and also indicate the exact amount
				 * of data in the request buffers.  The reply
				 * fields are initialized to NIL for the
				 * pointers and 0 for the lengths.  This can
				 * be passed to Rpc_Reply */
{
    register ConsistMsg	*consistArgPtr;
    register ConsistItem	*consistPtr;
    register FsRmtFileIOHandle	*rmtHandlePtr;
    register ReturnStatus	status;

    consistArgPtr = (ConsistMsg *)storagePtr->requestParamPtr;
    if (consistArgPtr->fileID.type != FS_RMT_FILE_STREAM) {
	Sys_Panic(SYS_FATAL, "Fs_RpcConsist, bad stream type <%d>\n",
		    consistArgPtr->fileID.type);
    }
    /*
     * This fetch locks the handle.  In earlier versions of the kernel
     * this could cause deadlock.  This may still be true.  3/9/88.
     */
    rmtHandlePtr = FsHandleFetchType(FsRmtFileIOHandle, &consistArgPtr->fileID);
    if ((rmtHandlePtr != (FsRmtFileIOHandle *)NIL) &&
	(rmtHandlePtr->openTimeStamp == consistArgPtr->openTimeStamp)) {
	status = SUCCESS;
    } else if (FsPrefixOpenInProgress(&consistArgPtr->fileID) == 0) {
	status = FS_STALE_HANDLE;
    } else {
	/*
	 * This message may in fact pertain to an open that we have
	 * in progress.  In this case return FAILURE and the server will retry.
	 */
	status = FAILURE;
    }
    if (rmtHandlePtr != (FsRmtFileIOHandle *)NIL) {
	FsHandleRelease(rmtHandlePtr, TRUE);
    }
    if (status == SUCCESS) {
	/*
	 * This is a message corresponding to our current notion of the
	 * file.  Pass the message to a consistency handler process.
	 */
	consistPtr = (ConsistItem *) Mem_Alloc(sizeof(ConsistItem));
	consistPtr->serverID = clientID;
	consistPtr->args = *consistArgPtr;
	Proc_CallFunc(ProcessConsist, (ClientData) consistPtr, 0);
    } else {
	Sys_Panic(SYS_WARNING, "Fs_RpcConsist: %s, timeStamp %d, flags %x\n",
		    (status == FS_STALE_HANDLE) ? "stale handle" :
						  "open/consist race",
		    consistArgPtr->openTimeStamp,
		    consistArgPtr->flags);
    }
    Rpc_Reply(srvToken, status, storagePtr, (int(*)())NIL, (ClientData)NIL);
    return(SUCCESS);
}


/*
 * ----------------------------------------------------------------------------
 *
 * ProcessConsist --
 *
 *	Process a client's cache consistency request.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The cache may be flushed or invalidated.
 *
 * ----------------------------------------------------------------------------
 *
 */
void
ProcessConsist(data, callInfoPtr)
    ClientData		data;
    Proc_CallInfo	*callInfoPtr;
{
    register FsRmtFileIOHandle 	*handlePtr;
    register FsCacheFileInfo	*cacheInfoPtr;
    ReturnStatus		status;
    Rpc_Storage			storage;
    ConsistReply		reply;
    register	ConsistItem	*consistPtr;
    int				firstBlock;
    int				lastBlock;
    int				numSkipped;

    consistPtr = (ConsistItem *) data;
    callInfoPtr->interval = 0;

    if (consistPtr->args.fileID.type != FS_RMT_FILE_STREAM) {
	Sys_Panic(SYS_FATAL, "ProcessConsist, unexpected file type %d\n",
	    consistPtr->args.fileID.type);
	return;
    }
    handlePtr = FsHandleFetchType(FsRmtFileIOHandle, &consistPtr->args.fileID);
    if (handlePtr == (FsRmtFileIOHandle *)NIL) {
	Sys_Printf("<%d, %d, %d, %d>:", consistPtr->args.fileID.type,
		     consistPtr->args.fileID.serverID,
		     consistPtr->args.fileID.major,
		     consistPtr->args.fileID.minor);
	Sys_Panic(SYS_WARNING, "ProcessConsist: lost the handle\n");
	return;
    }

    FS_CACHE_DEBUG_PRINT2("ProcessConsist: Got request %x for file %d\n", 
		    consistPtr->args.flags, handlePtr->rmt.hdr.fileID.minor);

    /*
     * Process the request.
     */
    cacheInfoPtr = &handlePtr->cacheInfo;
    if (cacheInfoPtr->attr.firstByte == -1) {
	firstBlock = 0;
    } else {
	firstBlock = cacheInfoPtr->attr.firstByte / FS_BLOCK_SIZE;
    }
    lastBlock = FS_LAST_BLOCK;

    reply.status = SUCCESS;
    switch (consistPtr->args.flags) {
	case FS_WRITE_BACK_BLOCKS:
	    reply.status = FsCacheFileWriteBack(cacheInfoPtr, firstBlock,
					lastBlock, FS_FILE_WB_WAIT,
					 &numSkipped);
	    break;
	case FS_CANT_READ_CACHE_PIPE:
	case FS_INVALIDATE_BLOCKS:
	    FsCacheFileInvalidate(cacheInfoPtr, firstBlock, lastBlock);
	    cacheInfoPtr->flags |= FS_FILE_NOT_CACHEABLE;
	    break;
	case FS_INVALIDATE_BLOCKS | FS_WRITE_BACK_BLOCKS:
	    reply.status = FsCacheFileWriteBack(cacheInfoPtr, firstBlock,
					lastBlock, 
				FS_FILE_WB_INVALIDATE | FS_FILE_WB_WAIT,
					&numSkipped);
	    cacheInfoPtr->flags |= FS_FILE_NOT_CACHEABLE;
	    break;
	case FS_DELETE_FILE:
	    FsCacheFileInvalidate(cacheInfoPtr, firstBlock, lastBlock);
	    cacheInfoPtr->flags |= FS_FILE_NOT_CACHEABLE;
	    if (handlePtr->rmt.recovery.use.ref > 1) {
		Sys_Panic(SYS_FATAL,
		    "ProcessConsist: Ref count > 1 on deleted file handle\n");
	    }
	    break;
	case FS_WRITE_BACK_ATTRS:
	    break;
	default:
	    Sys_Panic(SYS_WARNING, 
		      "ProcessConsist: Bad consistency action %x\n",
		      consistPtr->args.flags);

    }
    /*
     * The server wants the cached file attributes that we have.
     */
    reply.cachedAttr = cacheInfoPtr->attr;
    FS_CACHE_DEBUG_PRINT2("Returning: mod (%d), acc (%d),",
		       reply.cachedAttr.modifyTime,
		       reply.cachedAttr.accessTime);
    FS_CACHE_DEBUG_PRINT2(" first (%d), last (%d)\n",
		       reply.cachedAttr.firstByte, reply.cachedAttr.lastByte);
    reply.fileID = handlePtr->rmt.hdr.fileID;
    reply.fileID.type = FS_LCL_FILE_STREAM;
    FsHandleRelease(handlePtr, TRUE);
    /*
     * Set up the reply buffer.
     */
    storage.requestParamPtr = (Address)&reply;
    storage.requestParamSize = sizeof(ConsistReply);
    storage.requestDataPtr = (Address)NIL;
    storage.requestDataSize = 0;
    storage.replyParamPtr = (Address)NIL;
    storage.replyParamSize = 0;
    storage.replyDataPtr = (Address)NIL;
    storage.replyDataSize = 0;

    status = Rpc_Call(consistPtr->serverID, RPC_FS_CONSIST_REPLY, &storage);
    if (status != SUCCESS) {
	Sys_Panic(SYS_WARNING, "Got error (%x) from consist reply\n", status);
    }
    Mem_Free((Address)consistPtr);
}


/*
 *----------------------------------------------------------------------
 *
 * Fs_RpcConsistReply --
 *
 *	Service stub for RPC_FS_CONSIST_REPLY.  This indicates that
 *	the client has handled the consistency command.
 *
 * Results:
 *	SUCCESS.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
Fs_RpcConsistReply(srvToken, clientID, command, storagePtr)
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
    FsLocalFileIOHandle	*handlePtr;
    ConsistReply	*replyPtr;

    replyPtr = (ConsistReply *) storagePtr->requestParamPtr;
    if (replyPtr->fileID.type != FS_LCL_FILE_STREAM) {
	Sys_Panic(SYS_FATAL, "Fs_RpcConsistReply bad stream type <%d>\n",
	    replyPtr->fileID.type);
    }
    handlePtr = FsHandleFetchType(FsLocalFileIOHandle, &(replyPtr->fileID));
    if (handlePtr == (FsLocalFileIOHandle *) NIL) {
	Sys_Panic(SYS_FATAL, "Fs_RpcConsistReply: no handle\n");
    }
    ProcessConsistReply(&handlePtr->consist, clientID, replyPtr);
    FsHandleRelease(handlePtr, TRUE);
    Rpc_Reply(srvToken, SUCCESS, storagePtr, 
	      (int (*)())NIL, (ClientData)NIL);
}


/*
 *----------------------------------------------------------------------
 *
 * ProcessConsistReply --
 *
 *	Process the reply sent by the client for the given handle.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Element deleted from the list of outstanding client consist 
 *	messages.  Also if the message was for invalidation then the
 *	client list entry is marked as non-cacheable.  IMPORTANT:
 *	client list entry may be REMOVED here which prevents safe
 *	use of LIST_FORALL iteration over the client list by any
 *	routine that calls ClientCommand (which eventually gets us called)
 *
 *----------------------------------------------------------------------
 */
ENTRY void
ProcessConsistReply(consistPtr, clientID, replyPtr)
    FsConsistInfo		*consistPtr;	/* File to process reply for*/
    int				clientID;	/* Client who sent us the 
						 * reply. */
    register	ConsistReply	*replyPtr;	/* The reply that was sent. */
{
    register	ConsistMsgInfo		*msgPtr;
    register	FsClientInfo 		*clientPtr;
    Boolean				found = FALSE;
    FsLocalFileIOHandle			*handlePtr;

    LOCK_MONITOR;

    /*
     * Find the client in the list of pending messages.  Notify the
     * opening process after all the clients have responded.
     */
    LIST_FORALL(&(consistPtr->msgList), (List_Links *) msgPtr) {
	if (msgPtr->clientID == clientID) {
	    List_Remove((List_Links *) msgPtr);
	    found = TRUE;
	    break;
	}
    }
    if (List_IsEmpty(&(consistPtr->msgList))) {
	Sync_Broadcast(&consistPtr->repliesIn);
    }
    if (!found) {
	/*
	 * Old message from the client, probably queued in the network
	 * interface or from a gateway.
	 */
	Sys_Panic(SYS_WARNING, "ProcessConsistReply: Client %d not found\n",
			clientID);
	UNLOCK_MONITOR;
	return;
    }
    if (replyPtr->status != SUCCESS) {
	Sys_Panic(SYS_WARNING, "ProcessConsist: consistency failed <%x>\n",
				    replyPtr->status);
	consistPtr->flags |= FS_CONSIST_ERROR;
    } else {
	if (msgPtr->flags & FS_WRITE_BACK_BLOCKS) {
	    /*
	     * We just got the most recent blocks so we don't care who the
	     * last writer is anymore.
	     */
	    consistPtr->lastWriter = -1;
	}
	/*
	 * Look for client list entry that match the client, and update
	 * its state to reflect the consistency action by the client.
	 * Because we've release the monitor lock during the previous
	 * call-back, an unrelated action may have removed the client
	 * from the list - no big deal.
	 */
	found = FALSE;
	LIST_FORALL(&(consistPtr->clientList), (List_Links *) clientPtr) {
	    if (clientPtr->clientID == clientID) {
		found = TRUE;
		if (msgPtr->flags & FS_INVALIDATE_BLOCKS) {
		    clientPtr->cached = FALSE;
		}
		if (clientPtr->use.ref == 0 &&
		    consistPtr->lastWriter != clientID) {
		    List_Remove((List_Links *) clientPtr);
		    Mem_Free((Address) clientPtr);
		}
		break;
	    }
	}
	if (found) {
	    handlePtr = (FsLocalFileIOHandle *)consistPtr->hdrPtr;
	    FsUpdateAttrFromClient(&handlePtr->cacheInfo,
		&replyPtr->cachedAttr);
	}
    }
    Mem_Free((Address) msgPtr);

    UNLOCK_MONITOR;
}
