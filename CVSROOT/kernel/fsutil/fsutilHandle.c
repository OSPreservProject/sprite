/* 
 * fsHandle.c --
 *
 *	Routines to manage file handles.  They are kept in a table hashed
 *	by the Fs_FileID type.  They are referenced counted and eligible for
 *	removal when their reference count goes to zero.  FsHandleInstall
 *	adds handles to the table.  FsHandleFetch returns a locked handle.
 *	FsHandleLock locks a handle that you already have.
 *	Installing initializes the refCount to 1, and Fetching increments it.
 *	Use FsHandleUnlock and FsHandleReleaseHdr to unlock and decrement the
 *	reference count, respectively.  The macros FsHandleFetchType and
 *	FsHandleRelease do type casting and are defined in fsInt.h
 *	FsHandleRemove deletes a handle from the table, and FsGetNextHandle
 *	is used to iterate through the whole hash table.
 *
 * Copyright 1986 Regents of the University of California.
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "fs.h"
#include "fsInt.h"
#include "fsStat.h"
#include "fsLocalDomain.h"
#include "fsDisk.h"
#include "fsFile.h"
#include "fsTrace.h"
#include "fsOpTable.h"
#include "hash.h"

static Sync_Lock fileHashLock = SYNC_LOCK_INIT_STATIC();
#define	LOCKPTR	&fileHashLock

/*
 * Hash tables for object handles.  These are kept in LRU order and
 * a soft limit on their number is enforced.  If the number of handles
 * gets beyond fsStat.handle.maxNumber then LRU replacement is done until
 * handleScavengeThreashold (1/16) are replaced.  If all handles are in
 * use the max table size is allowed to grow by handleLimitInc (1/64).
 * The table never shrinks on the premise that once it has grown the
 * memory allocator establishes a high water mark and shrinking the table
 * won't really help overall kernel memory usage - we'll still LRU scavenge
 * to free what is already in the malloc arena.
 */
Hash_Table	fileHashTableStruct;
Hash_Table	*fileHashTable = &fileHashTableStruct;
static List_Links lruListHeader;
static List_Links *lruList = &lruListHeader;
/*
 * FS_HANDLE_TABLE_SIZE has to be at least 64, or the shifts for
 *	handleLimitInc and ScavengeThreashold have to be fixed.
 * LIMIT_INC defines the amount the table grows by.
 * THREASHOLD defines how many handles are reclaimed before stopping.
 */
#define		FS_HANDLE_TABLE_SIZE	   256
#define		LIMIT_INC(max)		   ( (max) >> 6 )
int		handleLimitInc =	   LIMIT_INC(FS_HANDLE_TABLE_SIZE);
#define		THREASHOLD(max)		   ( (max) >> 4 )
int		handleScavengeThreashold = THREASHOLD(FS_HANDLE_TABLE_SIZE);

FsHandleHeader *FsGetNextLRUHandle();
void FsDoneLRU();
int		fsLRUinProgress = FALSE;

/*
 * Flags for handles referenced from the hash table.
 *	FS_HANDLE_INSTALLED - the structure has been put into the hash table.
 *	FS_HANDLE_LOCKED    - the lock bit on the handle.  Handles are locked
 *		when they are fetched from the hash table.
 *	FS_HANDLE_WANTED    - set when iterating through the handle table
 *		so that the handle doesn't get free'ed by FsHandleRemoveInt
 *	FS_HANDLE_REMOVED   - set instead of freeing the handle if the
 *		remove procedure sees the 'wanted' or 'don't move' bit.
 *	FS_HANDLE_INVALID   - set when a handle becomes invalid after
 *		a recovery error.
 *	FS_HANDLE_DONT_CACHE - set if the handle should be removed after
 *		the last reference is released.
 *	FS_HANDLE_DONT_MOVE - set during LRU iteration to lock down the
 *		next handle in the list.
 *	FS_HANDLE_MOVE_LATER - set if we should move this handle as soon
 *		as the DONT_MOVE bit is cleared.
 */
#define FS_HANDLE_INSTALLED	0x1
#define FS_HANDLE_LOCKED	0x2
#define FS_HANDLE_WANTED	0x4
#define FS_HANDLE_REMOVED	0x8
#define FS_HANDLE_FREED		0x10
#define FS_HANDLE_INVALID	0x20
#define FS_HANDLE_DONT_CACHE	0x40
#define FS_HANDLE_DONT_MOVE	0x80
#define FS_HANDLE_MOVE_LATER	0x100

/*
 * Macro to lock a file handle.
 */

#define	LOCK_HANDLE(hdrPtr) \
	while (((hdrPtr)->flags & FS_HANDLE_LOCKED) && !sys_ShuttingDown) { \
	    (void) Sync_Wait(&((hdrPtr)->unlocked), FALSE); \
	} \
	fsStats.handle.locks++; \
	(hdrPtr)->flags |= FS_HANDLE_LOCKED;

/*
 * Macro to unlock a file handle.
 */

#define	UNLOCK_HANDLE(hdrPtr) \
	(hdrPtr)->flags &= ~FS_HANDLE_LOCKED; \
	Sync_Broadcast(&((hdrPtr)->unlocked));

/*
 * Macro to remove a handle, so no details get fogotten.  Note, removal
 * from the hash table is separate, because we do that first, and then
 * clean it up completely later with this macro.
 */

#define REMOVE_HANDLE(hdrPtr) \
	List_Remove(&(hdrPtr)->lruLinks); 	\
	if ((hdrPtr)->name != (char *)NIL) {	\
	    free((hdrPtr)->name);		\
	}					\
	free((char *)(hdrPtr));

/*
 * Macro to move a handle to the back (most recent) end of the LRU list.
 * We can't move handles that are stepping stones in an LRU scan, they
 * get moved later.
 */

#define MOVE_HANDLE(hdrPtr) \
	if ((hdrPtr)->flags & FS_HANDLE_DONT_MOVE) {			\
	    (hdrPtr)->flags |= FS_HANDLE_MOVE_LATER;			\
	} else {							\
	    hdrPtr->flags &= ~FS_HANDLE_MOVE_LATER;			\
	    List_Move(&(hdrPtr)->lruLinks, LIST_ATREAR(lruList));	\
	}
/*
 * Macro to give name associated with a handle.
 */
#define HDR_FILE_NAME(hdrPtr) \
	(((hdrPtr)->name == (char *)NIL) ? "(no name)" : hdrPtr->name)

/*
 * Macro for debug trace prints.
 */
int fsHandleTrace = FALSE;
#ifndef CLEAN
#define HANDLE_TRACE(hdrPtr, comment) \
    if (fsHandleTrace) {						\
	printf("<%d, %d, %d, %d> flags %x ref %d : %s\n",		\
	(hdrPtr)->fileID.type, (hdrPtr)->fileID.serverID,		\
	(hdrPtr)->fileID.major, (hdrPtr)->fileID.minor,		\
	(hdrPtr)->flags, (hdrPtr)->refCount, comment);			\
    }
#else
#define HANDLE_TRACE
#endif


/*
 *----------------------------------------------------------------------------
 *
 * FsHandleInit --
 *
 * 	Initialize the hash table.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Hash table is initialized.
 *
 *----------------------------------------------------------------------------
 */

void
FsHandleInit(fileHashSize)
    int	fileHashSize;	/* The number of hash table entries to put in the
			 * file hash table for starters. */
{
    fsStat.handle.maxNumber = FS_HANDLE_TABLE_SIZE;
    List_Init(lruList);
    Hash_Init(fileHashTable, fileHashSize, Hash_Size(sizeof(Fs_FileID)));
}


/*
 *----------------------------------------------------------------------------
 *
 * FsHandleInstall --
 *
 *      Install a file handle.  It is retrievable by FsHandleFetch after
 *      this.  If the file handle is not already installed then memory
 *	is allocated for it.
 *
 * Results:
 *	TRUE is returned if the file handle was already in the 
 *	hash table.  Upon return, *hdrPtrPtr references the installed handle.
 *
 * Side effects:
 *	New handle may be allocated.
 *
 *----------------------------------------------------------------------------
 *
 */
ENTRY Boolean
FsHandleInstall(fileIDPtr, size, name, hdrPtrPtr)
    register Fs_FileID	*fileIDPtr;	/* Identfies handle to install. */
    int		 	size;		/* True size of the handle.  This
					 * routine only looks at the header,
					 * but more data follows that. */
    char		*name;		/* File name for error messages */
    FsHandleHeader	**hdrPtrPtr;	/* Return pointer to handle that
					 * is found in the hash table. */
{
    Boolean found;
    register int numScavenged;
    register FsHandleHeader *hdrPtr;
    List_Links *listPtr;

    do {
	found = FsHandleInstallInt(fileIDPtr, size, name,
		    fsStat.handle.maxNumber, hdrPtrPtr);
	if (*hdrPtrPtr == (FsHandleHeader *)NIL) {
	    /*
	     * Limit would be exceeded, recycle some handles.
	     */
	    listPtr = (List_Links *)NIL;
	    numScavenged = 0;
	    if (fsLRUinProgress > 0) {
		printf("Handle LRU already in progress\n");
	    }
	    fsLRUinProgress++;
	    fsStats.handle.lruScans++;
	    for (hdrPtr = FsGetNextLRUHandle(&listPtr);
		 hdrPtr != (FsHandleHeader *)NIL;
		 hdrPtr = FsGetNextLRUHandle(&listPtr)) {
		if ((*fsStreamOpTable[hdrPtr->fileID.type].scavenge)(hdrPtr)) {
		    numScavenged++;
		    fsStats.object.scavenges++;
		    if (numScavenged > handleScavengeThreashold) {
			break;
		    }
		}
	    }
	    FsDoneLRU(&listPtr);
	    fsLRUinProgress--;
	    if (numScavenged == 0) {
		/*
		 * All handles in use, grow the table a bit.
		 */
		fsStat.handle.maxNumber += handleLimitInc;
		handleLimitInc = LIMIT_INC(fsStat.handle.maxNumber);
		handleScavengeThreashold = THREASHOLD(fsStat.handle.maxNumber);
	    }
	}
    } while (*hdrPtrPtr == (FsHandleHeader *)NIL);
    return(found);
}

/*
 *----------------------------------------------------------------------------
 *
 * FsHandleInstallInt --
 *
 *      Install a file handle.  It is retrievable by FsHandleFetch after
 *      this.  If the file handle is not already installed then memory
 *	is allocated for it.  This enforces a soft limit on the number
 *	of handles that can exist.
 *
 * Results:
 *	TRUE is returned if the file handle was already in the 
 *	hash table.  Upon return, *hdrPtrPtr references the installed handle.
 *	If (*hdrPtrPtr) is NULL then our caller, FsHandleInstall, has
 *	to recycle some handles.
 *
 * Side effects:
 *	New handle may be allocated.
 *
 *----------------------------------------------------------------------------
 *
 */
ENTRY Boolean
FsHandleInstallInt(fileIDPtr, size, name, handleLimit, hdrPtrPtr)
    register Fs_FileID	*fileIDPtr;	/* Identfies handle to install. */
    int		 	size;		/* True size of the handle.  This
					 * routine only looks at the header,
					 * but more data follows that. */
    char		*name;		/* File name for error messages */
    int			handleLimit;	/* Determines how many handles can
					 * exist before we return NULL */
    FsHandleHeader	**hdrPtrPtr;	/* Return pointer to handle that
					 * is found in the hash table. */    
{
    register	Hash_Entry	*hashEntryPtr;
    register	FsHandleHeader	*hdrPtr;
    Boolean			found;

    LOCK_MONITOR;
    fsStats.handle.installCalls++;
again:
    hashEntryPtr = Hash_Find(fileHashTable, (Address) fileIDPtr);
    if (hashEntryPtr->value == (Address) NIL) {
	found = FALSE;
	if (fsStats.handle.exists > handleLimit) {
	    hdrPtr = (FsHandleHeader *)NIL;
	    goto exit;
	}

	fsStats.handle.created++;
	fsStats.handle.exists++;
	/*
	 * Allocate and install a new file handle.  Because there are
	 * various things behind the handle header our caller specifies the
	 * size.
	 */
	hdrPtr = (FsHandleHeader *) malloc(size);
	bzero((Address) hdrPtr, size);
	hdrPtr->fileID = *fileIDPtr;
	hdrPtr->refCount = 1;
	hdrPtr->flags |= FS_HANDLE_INSTALLED;
#ifndef FS_NO_HDR_NAMES
	if (name != (char *)NIL) {
	    hdrPtr->name = (char *)malloc(strlen(name) + 1);
	    (void)strcpy(hdrPtr->name, name);
	} else {
	    hdrPtr->name = (char *)NIL;
	}
#else
	hdrPtr->name = (char *)NIL;
#endif
	List_Insert(&hdrPtr->lruLinks, LIST_ATREAR(lruList));
	Hash_SetValue(hashEntryPtr, hdrPtr);
	FS_TRACE_HANDLE(FS_TRACE_INSTALL_NEW, hdrPtr);
    } else {
	found = TRUE;
	hdrPtr = (FsHandleHeader *) Hash_GetValue(hashEntryPtr);
	if (hdrPtr->flags & FS_HANDLE_LOCKED) {
	    /*
	     * Wait for it to become unlocked.  We can't up the
	     * the reference count until we lock it, so we have to
	     * jump back and rehash as the handle may have been deleted.
	     */
	    (void) Sync_Wait(&hdrPtr->unlocked, FALSE);
	    fsStats.handle.lockWaits++;
	    goto again;
	}
	fsStats.handle.installHits++;
	hdrPtr->refCount++;
	MOVE_HANDLE(hdrPtr);
	FS_TRACE_HANDLE(FS_TRACE_INSTALL_HIT, hdrPtr);
#ifndef FS_NO_HDR_NAMES
	if ((hdrPtr->name == (char *)NIL) && (name != (char *)NIL)) {
	    hdrPtr->name = (char *)malloc(strlen(name) + 1);
	    (void)strcpy(hdrPtr->name, name);
	}
#endif FS_NO_HDR_NAMES
    }
    hdrPtr->flags |= FS_HANDLE_LOCKED;
    fsStats.handle.locks++;
exit:
    *hdrPtrPtr = hdrPtr;
    UNLOCK_MONITOR;
    return(found);
}

/*
 *----------------------------------------------------------------------------
 *
 * FsHandleFetch --
 * FsHandleFetchType --
 *
 *	Return a pointer to a file handle out of the file handle hash table.  
 *	If no file handle is found then NIL is returned.  If one is found
 *	then it is returned locked.  The reference count is increased
 *	on the handle so it needs to be released with FsHandleRelease.
 *	FsHandleFetchType is a macro that does type casting, see fsInt.h 
 *
 * Results:
 *	A pointer to a file handle, NIL if none found.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------------
 *
 */
ENTRY FsHandleHeader *
FsHandleFetch(fileIDPtr)
    Fs_FileID 	*fileIDPtr;	/* Identfies handle to fetch. */
{
    register	Hash_Entry	*hashEntryPtr;
    register	FsHandleHeader	*hdrPtr;

    LOCK_MONITOR;

    fsStats.handle.fetchCalls++;

again:
    hashEntryPtr = Hash_LookOnly(fileHashTable, (Address) fileIDPtr);
    if (hashEntryPtr == (Hash_Entry *) NIL) {
	UNLOCK_MONITOR;
	return((FsHandleHeader *) NIL);
    }
    fsStats.handle.fetchHits++;
    hdrPtr = (FsHandleHeader *) Hash_GetValue(hashEntryPtr);

    if (hdrPtr->flags & FS_HANDLE_LOCKED) {
	/*
	 * Wait for it to become unlocked.  Once wake up rehash because 
	 * the handle could have been removed before we got a chance
	 * to increment the reference count on it.
	 */
	fsStats.handle.lockWaits++;
	(void) Sync_Wait(&hdrPtr->unlocked, FALSE);
	goto again;
    }
    MOVE_HANDLE(hdrPtr);
    fsStats.handle.locks++;
    hdrPtr->flags |= FS_HANDLE_LOCKED;
    hdrPtr->refCount++;

    UNLOCK_MONITOR;
    return(hdrPtr);
}

/*
 *----------------------------------------------------------------------------
 *
 * FsHandleLockHdr --
 * FsHandleLock --
 *
 *	Get a lock on the handle.  FsHandleLock is a macro defined in fsInt.h
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The lock bit is added to the flags field for the handle.
 *
 *----------------------------------------------------------------------------
 *
 */
ENTRY void
FsHandleLockHdr(hdrPtr)
    register	FsHandleHeader	*hdrPtr;
{
    LOCK_MONITOR;

    LOCK_HANDLE(hdrPtr);

    UNLOCK_MONITOR;
}

/*
 *----------------------------------------------------------------------------
 *
 * FsHandleIncRefCount --
 *
 *	Increment the reference count on the handle.  This is used by
 *	the name hash table when a handle is put there.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The reference count on the handle is incremented.
 *
 *----------------------------------------------------------------------------
 *
 */
ENTRY void
FsHandleIncRefCount(hdrPtr, amount)
    register	FsHandleHeader	*hdrPtr;
    int 			amount;
{
    LOCK_MONITOR;

    hdrPtr->refCount += amount;

    UNLOCK_MONITOR;
}

/*
 *----------------------------------------------------------------------------
 *
 * FsHandleDecRefCount --
 *
 *	Decrement the reference count on the handle.
 *	This is like releasing a handle but the locks on the handle
 *	are not disturbed.  This is used when removing entries from
 *	the name component hash table and during migration.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The reference count on the handle is decremented.
 *
 *----------------------------------------------------------------------------
 *
 */
ENTRY void
FsHandleDecRefCount(hdrPtr)
    register	FsHandleHeader	*hdrPtr;
{
    LOCK_MONITOR;

    hdrPtr->refCount--;

    UNLOCK_MONITOR;
}

/*
 *----------------------------------------------------------------------------
 *
 * FsHandleDup --
 *
 *	Duplicate a reference to a handle.  This locks the handle and
 *	increments the refCount.  This is used by the local lookup routine
 *	as it initiates its path search.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The reference count on the handle is incremented and the handle is
 *	locked.
 *
 *----------------------------------------------------------------------------
 *
 */
ENTRY FsHandleHeader *
FsHandleDup(hdrPtr)
    register	FsHandleHeader	*hdrPtr;
{
    LOCK_MONITOR;

    LOCK_HANDLE(hdrPtr);
    MOVE_HANDLE(hdrPtr);
    hdrPtr->refCount++;

    UNLOCK_MONITOR;
    return(hdrPtr);
}

/*
 *----------------------------------------------------------------------------
 *
 * FsHandleValid --
 *
 *	See if a handle have been marked invalid because of recovery.
 *
 * Results:
 *	TRUE if the handle is still good.  FALSE if it's been marked invalid.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------------
 *
 */

ENTRY Boolean
FsHandleValid(hdrPtr)
    register FsHandleHeader *hdrPtr;	/* Handle to check. */
{
    register Boolean valid;
    LOCK_MONITOR;
    valid = ( (hdrPtr->flags & FS_HANDLE_INVALID) == 0 );
    UNLOCK_MONITOR;
    return(valid);
}

/*
 *----------------------------------------------------------------------------
 *
 * FsHandleUnlockHdr --
 * FsHandleUnlock --
 *
 *	Release the lock on the handle. FsHandleUnlock is a macro defined
 *	in fsInt.h that does type casting.
 *
 * Results:
 *	FALSE because this is used as a scavenging no-op.
 *
 * Side effects:
 *	The lock bit is removed from the flags filed for the handle.
 *
 *----------------------------------------------------------------------------
 *
 */
ENTRY Boolean
FsHandleUnlockHdr(hdrPtr)
    register	FsHandleHeader	*hdrPtr;
{
    LOCK_MONITOR;

    if ((hdrPtr->flags & FS_HANDLE_LOCKED) == 0) {
	UNLOCK_MONITOR;
	panic( "HandleUnlock, un-locked handle\n");
	return;
    }
    UNLOCK_HANDLE(hdrPtr);

    UNLOCK_MONITOR;
    return(FALSE);
}

/*
 *----------------------------------------------------------------------------
 *
 * FsHandleReleaseHdr --
 * FsHandleRelease --
 *
 *	FsHandleRelease is a macro that does type casting, see fsInt.h
 *	Decrement the reference count on the file handle.  The caller specifies
 *	if the handle is already locked, in which case it is unlocked. If
 *	the handle is unlocked on entry it is no longer locked before
 *	decrementing the reference count because that causes deadlock.
 *	Hoever, this also means that access to the refCount outside this file
 *	has to be explicitly synchronized by ensuring that the handle
 *	is locked before releasing it.  The refCount is only examined outside
 *	this file (and monitor lock) by the routines that migrate top-level
 *	stream objects.  All other kinds of handles have use counts which
 *	are different than the hdr's refCount, and which are protected
 *	explicitly by the handle lock bit.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The reference count is decremented.  If the reference count goes
 *	to zero	and the handle has been removed then it gets nuked here.
 *
 *----------------------------------------------------------------------------
 *
 */
ENTRY void
FsHandleReleaseHdr(hdrPtr, locked)
    register FsHandleHeader *hdrPtr;  /* Header of handle to release. */
    Boolean	      locked;	   /* TRUE if the handle is already locked. */
{
    LOCK_MONITOR;
    fsStats.handle.releaseCalls++;

    if (locked && ((hdrPtr->flags & FS_HANDLE_LOCKED) == 0)) {
	UNLOCK_MONITOR;
	panic(
	    "HandleRelease, handle <%d,%d,%d,%d> \"%s\" not locked\n",
	    hdrPtr->fileID.type, hdrPtr->fileID.serverID,
	    hdrPtr->fileID.major, hdrPtr->fileID.minor, HDR_FILE_NAME(hdrPtr));
	return;
    }
    hdrPtr->refCount--;

    if (hdrPtr->refCount < 0) {
	UNLOCK_MONITOR;
	panic( "refCount %d on %s handle <%d,%d,%d> \"%s\"\n",
		hdrPtr->refCount, FsFileTypeToString(hdrPtr->fileID.type),
		hdrPtr->fileID.serverID, hdrPtr->fileID.major,
		hdrPtr->fileID.minor, HDR_FILE_NAME(hdrPtr));
	return;
    } else if ((hdrPtr->refCount == 0) &&
	       (hdrPtr->flags & FS_HANDLE_REMOVED) &&
	       ((hdrPtr->flags & (FS_HANDLE_WANTED|FS_HANDLE_DONT_MOVE) == 0))){
	/*
	 * The handle has been removed, we are the last reference, and
	 * noone in GetNextHandle is trying to grab this handle.
	 */
	FS_TRACE_HANDLE(FS_TRACE_RELEASE_FREE, hdrPtr);
        REMOVE_HANDLE(hdrPtr);
     } else {
	FS_TRACE_HANDLE(FS_TRACE_RELEASE_LEAVE, hdrPtr);
	if (locked) {
	    UNLOCK_HANDLE(hdrPtr);
	}
    }
    UNLOCK_MONITOR;
}

/*
 *----------------------------------------------------------------------------
 *
 * FsHandleRemoveInt --
 *
 *	Delete the handle from hash table.  The internal version that
 *	knows it's under the monitor lock.  This can be called while
 *	the handle still has references.  It unlocks the handle
 *	and frees it if there are not references
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The file handle is deleted out of the hash table, and memory is freed
 *	if there are no references to the handle.
 *
 *----------------------------------------------------------------------------
 *
 */
INTERNAL void
FsHandleRemoveInt(hdrPtr)
    register FsHandleHeader *hdrPtr;  /* Header of handle to remove. */
{
    register	Hash_Entry	*hashEntryPtr;

    if (!(hdrPtr->flags & FS_HANDLE_INVALID)) {
	hashEntryPtr = Hash_LookOnly(fileHashTable, (Address) &hdrPtr->fileID);
	if (hashEntryPtr == (Hash_Entry *) NIL) {
	    UNLOCK_MONITOR;
	    panic(
		    "FsHandleRemoveInt: Couldn't find handle in hash table.\n");
	    LOCK_MONITOR;
	    return;
	}
	Hash_Delete(fileHashTable, hashEntryPtr);
    }
    fsStats.handle.exists--;

    /*
     * Wakeup anyone waiting for this handle to become unlocked.
     */
    UNLOCK_HANDLE(hdrPtr);

    if ((hdrPtr->flags & (FS_HANDLE_WANTED|FS_HANDLE_DONT_MOVE)) ||
	(hdrPtr->refCount > 0)) {
	/*
	 * We've removed the handle from the hash table so it won't
	 * be found, but either someone is trying to get it in GetNextHandle,
	 * or simply has a reference to it.  GetNextHandle or HandleRelease
	 * will free the handle for us later.
	 */
	FS_TRACE_HANDLE(FS_TRACE_REMOVE_LEAVE, hdrPtr);
	hdrPtr->flags |= FS_HANDLE_REMOVED;
    } else {
	FS_TRACE_HANDLE(FS_TRACE_REMOVE_FREE, hdrPtr);
	REMOVE_HANDLE(hdrPtr);
    }
}

/*
 *----------------------------------------------------------------------------
 *
 * FsHandleRemove --
 * FsHandleRemoveHdr --
 *
 *	Delete the handle from hash table by calling the internal routine.
 *	FsHandleRemove is a macro defined in fsInt.h that does type casting.
 *	Removing a handle deletes it from the hash table and unlocks it.
 *	Then, if there are no references to the handle it is freed.  Otherwise
 *	it is marked as deleted and FsHandleRelease cleans it up.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None (see FsHandleRemoveInt).
 *
 *----------------------------------------------------------------------------
 *
 */

ENTRY void
FsHandleRemoveHdr(hdrPtr)
    register FsHandleHeader *hdrPtr;	/* Handle to remove. */
{
    LOCK_MONITOR;
    FsHandleRemoveInt(hdrPtr);
    UNLOCK_MONITOR;
}

/*
 *----------------------------------------------------------------------------
 *
 * FsHandleAttemptRemove --
 *
 *	Like FsHandleRemove, but specific to local file handles because
 *	they might have extra references from the name component cache.
 *	This will not remove the handle if it is still referenced from
 *	that cache.  The reference count has to be checked inside the
 *	monitor to avoid multi-processor races.	This routine is called by the
 *	scavenging routines if they think it's probably ok to remove the handle.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Frees the memory for the handles file descriptor.
 *
 *----------------------------------------------------------------------------
 *
 */

ENTRY void
FsHandleAttemptRemove(handlePtr)
    register FsLocalFileIOHandle *handlePtr;	/* Handle to try and remove. */
{
    LOCK_MONITOR;
    if (handlePtr->hdr.refCount == 0) {
	free((Address)handlePtr->descPtr);
	handlePtr->descPtr = (FsFileDescriptor *)NIL;
	FsHandleRemoveInt((FsHandleHeader *)handlePtr);
    } else {
	UNLOCK_HANDLE((FsHandleHeader *)handlePtr);
    }
    UNLOCK_MONITOR;
}

/*
 *----------------------------------------------------------------------------
 *
 * FsGetNextHandle --
 *
 *	Get the next handle in the hash table.  Return the handle locked.
 *	The hashSearchPtr is initialized with Hash_StartSearch. 
 *
 * Results:
 *	The next handle in the hash table.
 *
 * Side effects:
 *	Locks the handle (but does not increment its reference count).
 *	While waiting to lock a handle it will mark it as being wanted
 *	so that it doesn't get deleted out from under us.
 *
 *----------------------------------------------------------------------------
 *
 */

ENTRY FsHandleHeader *
FsGetNextHandle(hashSearchPtr)
    Hash_Search	*hashSearchPtr;	/* Iterator for going through the hash table. */
{
    register 	FsHandleHeader	*hdrPtr;
    register	Hash_Entry	*hashEntryPtr;

    LOCK_MONITOR;

    for (hashEntryPtr = Hash_Next(fileHashTable, hashSearchPtr);
         hashEntryPtr != (Hash_Entry *) NIL;  
	 hashEntryPtr = Hash_Next(fileHashTable, hashSearchPtr)) {
	hdrPtr = (FsHandleHeader *) Hash_GetValue(hashEntryPtr);
	/*
	 * See if another iterator is blocked on this handle, and skip
	 * it if there is one.  This is done for two reasons,
	 * 1) to avoid freeing the handle more than once (once for each
	 *	iterator that sets the FS_HANDLE_WANTED bit) if it gets
	 *	removed while we are waiting for it.  This problem could
	 *	be fixed by adding a waitCount field to the handle header.
	 * 2) to avoid hanging the system on single locked handle. 
	 */
	if ((hdrPtr->flags & FS_HANDLE_WANTED) &&
	    (hdrPtr->flags & FS_HANDLE_LOCKED)){
	    printf( "GetNextHandle skipping %s <%d,%d> \"%s\"\n",
		FsFileTypeToString(hdrPtr->fileID.type),
		hdrPtr->fileID.major, hdrPtr->fileID.minor,
		HDR_FILE_NAME(hdrPtr));
	    continue;
	}
	/*
	 * Mark the handle so that it won't get blown away while we are
	 * trying to lock it and then lock it.  If it gets removed from
	 * the hash table while we were locking it, then throw it away.
	 */
	hdrPtr->flags |= FS_HANDLE_WANTED;
	LOCK_HANDLE(hdrPtr);
	hdrPtr->flags &= ~FS_HANDLE_WANTED;
	if (hdrPtr->flags & FS_HANDLE_REMOVED) {
	    UNLOCK_HANDLE(hdrPtr);
	    if ((hdrPtr->refCount == 0) &&
		(hdrPtr->flags & FS_HANDLE_DONT_MOVE) == 0) {
		FS_TRACE_HANDLE(FS_TRACE_GET_NEXT_FREE, hdrPtr);
		REMOVE_HANDLE(hdrPtr);
	    }
	    continue;
	}
	UNLOCK_MONITOR;
	return(hdrPtr);
    }

    UNLOCK_MONITOR;
    return((FsHandleHeader *) NIL);
}

/*
 *----------------------------------------------------------------------------
 *
 * FsGetNextLRUHandle --
 *
 *	Get the next handle in the LRU list.  Return the handle locked.
 *	The listPtrPtr should reference a NIL pointer on the first call.
 *	This skips locked handles because they are obviously in use and not good
 *	candidates for removal.  This also prevents a single locked handle
 *	from clogging up the system.
 *
 * Results:
 *	The next handle in the LRU list.
 *
 * Side effects:
 *	Marks the next handle in the LRU list as "not movable" so we
 *	won't get warped to the wrong spot in the list and we won't
 *	get an element yanked out from under us.
 *
 *----------------------------------------------------------------------------
 *
 */
ENTRY FsHandleHeader *
FsGetNextLRUHandle(listPtrPtr)
    List_Links **listPtrPtr;	/* Indirect pointer to current spot in list.
				 * Modified to reference the next entry.
				 * This should reference a NIL pointer to
				 * begin the lru scan. */
{
    register 	FsHandleHeader	*hdrPtr;
    register 	FsHandleHeader	*nextHdrPtr;
    register	List_Links	*listPtr;

    LOCK_MONITOR;

    if (*listPtrPtr == (List_Links *)NIL) {
	listPtr = List_First(lruList);
    } else {
	listPtr = *listPtrPtr;
	if (List_IsAtEnd(lruList, listPtr)) {
	    hdrPtr = (FsHandleHeader *)NIL;
	    goto exit;
	}
    }
    hdrPtr = LRU_LINKS_TO_HANDLE(listPtr);
    hdrPtr->flags &= ~FS_HANDLE_DONT_MOVE;
    listPtr = List_Next(&hdrPtr->lruLinks);
    if (hdrPtr->flags & FS_HANDLE_MOVE_LATER) {
	MOVE_HANDLE(hdrPtr);
    }
    /*
     * Have the candidate handle, plus a pointer to the next one in the list.
     * The candidate has been unmarked and moved if necessary.
     * Now skip over locked and removed handles.
     */
    while (hdrPtr->flags & (FS_HANDLE_REMOVED|FS_HANDLE_LOCKED)) {
	if ((hdrPtr->flags & FS_HANDLE_REMOVED) &&
	    ((hdrPtr->flags & (FS_HANDLE_DONT_MOVE|FS_HANDLE_WANTED)) == 0) &&
	    (hdrPtr->refCount == 0)) {
	    FS_TRACE_HANDLE(FS_TRACE_LRU_FREE, hdrPtr);
	    REMOVE_HANDLE(hdrPtr);
	}
	if (List_IsAtEnd(lruList, listPtr)) {
	    hdrPtr = (FsHandleHeader *)NIL;
	    goto exit;
	} else {
	    hdrPtr = LRU_LINKS_TO_HANDLE(listPtr);
	    listPtr = List_Next(listPtr);
	}
    }
    LOCK_HANDLE(hdrPtr);
    /*
     * Mark the next handle in the LRU list so it wont be moved around
     * or removed.  If we are at the end of the list we can't map to
     * a handle because we're at the list header, not in a handle.
     */
    if (!List_IsAtEnd(lruList, listPtr)) {
	nextHdrPtr = LRU_LINKS_TO_HANDLE(listPtr);
	nextHdrPtr->flags |= FS_HANDLE_DONT_MOVE;
    }
exit:
    *listPtrPtr = listPtr;
    UNLOCK_MONITOR;
    return(hdrPtr);
}

/*
 *----------------------------------------------------------------------------
 *
 * FsDoneLRU --
 *
 *	Terminate LRU iteration.  This clears the don't move flag of what
 *	would have been the next handle returned. 
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Clears the don't move flag and nukes the handle if it should be.
 *
 *----------------------------------------------------------------------------
 *
 */
ENTRY void
FsDoneLRU(listPtrPtr)
    List_Links **listPtrPtr;	/* Reference to next spot in LRU list */
{
    register FsHandleHeader *hdrPtr;

    if ((*listPtrPtr != (List_Links *)NIL) &&
	(!List_IsAtEnd(lruList, *listPtrPtr))) {
	hdrPtr = LRU_LINKS_TO_HANDLE(*listPtrPtr);
	hdrPtr->flags &= ~FS_HANDLE_DONT_MOVE;
	if ((hdrPtr->flags & FS_HANDLE_REMOVED) &&
	    ((hdrPtr->flags & FS_HANDLE_WANTED) == 0) &&
	    (hdrPtr->refCount == 0)) {
	    FS_TRACE_HANDLE(FS_TRACE_LRU_DONE_FREE, hdrPtr);
	    REMOVE_HANDLE(hdrPtr);
	} else if (hdrPtr->flags & FS_HANDLE_MOVE_LATER) {
	    MOVE_HANDLE(hdrPtr);
	}
    }
}

/*
 *----------------------------------------------------------------------------
 *
 * FsHandleInvalidate --
 *
 *	Mark a handle as bogus because of a failed recovery attempt.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sets the INVALID flag in the handle, negates the minor
 *	field of the fileID so that the handle won't get found,
 *	removes the handle from the hash table.
 *
 *----------------------------------------------------------------------------
 *
 */
ENTRY void
FsHandleInvalidate(hdrPtr)
    FsHandleHeader *hdrPtr;
{
    Hash_Entry	*hashEntryPtr;

    LOCK_MONITOR;

    if ((hdrPtr->flags & FS_HANDLE_INVALID) == 0) {
	hdrPtr->flags |= FS_HANDLE_INVALID;
	/*
	 * Invalid handles are deleted from the hash table and the fileID is
	 * smashed so that all subsequent operations using this handle that
	 * go to the server will fail with a stale handle return code.
	 */
	hashEntryPtr = Hash_LookOnly(fileHashTable, (Address) &hdrPtr->fileID);
	if (hashEntryPtr == (Hash_Entry *) NIL) {
	    UNLOCK_MONITOR;
	    panic(
		"FsHandleInvalidate: Can't find %s handle <%d,%d,%d>\n",
			FsFileTypeToString(hdrPtr->fileID.type),
			hdrPtr->fileID.serverID,
			hdrPtr->fileID.major, hdrPtr->fileID.minor);
	    return;
	}
	Hash_Delete(fileHashTable, hashEntryPtr);
	hdrPtr->fileID.minor = -hdrPtr->fileID.minor;
    }

    UNLOCK_MONITOR;
}


/*
 *----------------------------------------------------------------------------
 *
 * FsHandleDescWriteBack --
 *
 *	Go through all of the handles and write back all descriptors that
 *	are dirty and have a modify date before the given time.
 *
 * Results:
 *	The number of locked file handles.
 *
 * Side effects:
 *	The write backs.  Propogates modify times from handle to desciptor.
 *
 *----------------------------------------------------------------------------
 *
 */

ENTRY int
FsHandleDescWriteBack(shutdown, domain)
    Boolean	shutdown;	/* TRUE if the kernel is being shutdowned. */
    int		domain;		/* Domain number, -1 means all local domains */
{
    Hash_Search			hashSearch;
    FsDomain			*domainPtr;
    register FsFileDescriptor	*descPtr;
    register FsHandleHeader	*hdrPtr;
    register Hash_Entry		*hashEntryPtr;
    register FsCachedAttributes	*cachedAttrPtr;
    int				lockedDesc = 0;
    ReturnStatus		status;

    LOCK_MONITOR;

    Hash_StartSearch(&hashSearch);
    for (hashEntryPtr = Hash_Next(fileHashTable, &hashSearch);
         hashEntryPtr != (Hash_Entry *) NIL;  
	 hashEntryPtr = Hash_Next(fileHashTable, &hashSearch)) {
	hdrPtr = (FsHandleHeader *) Hash_GetValue(hashEntryPtr);
	switch (hdrPtr->fileID.type) {
	    case FS_LCL_FILE_STREAM: {
		register FsLocalFileIOHandle *handlePtr =
			(FsLocalFileIOHandle *) hdrPtr;
		descPtr = handlePtr->descPtr;
		cachedAttrPtr = &handlePtr->cacheInfo.attr;
		break;
	    }
#ifdef notdef
	    case FS_LCL_NAMED_PIPE_STREAM: {
		register FsNamedPipeIOHandle *handlePtr =
			(FsNamedPipeIOHandle *) hdrPtr;
		descPtr = handlePtr->descPtr;
		cachedAttrPtr = &handlePtr->cacheInfo.attr;
		break;
	    }
#endif
	    default:
		continue;
	}
	if (domain >= 0 && hdrPtr->fileID.major != domain) {
	    continue;
	}
	if (hdrPtr->flags & FS_HANDLE_LOCKED) {
	    /*
	     * The handle is locked.
	     */
	    lockedDesc++;
	    continue;
	}
	domainPtr = FsDomainFetch(hdrPtr->fileID.major, FALSE);
	if (domainPtr == (FsDomain *)NIL) {
	    /*
	     * The domain for the file has been detached.
	     */
	    continue;
	}
	/*
	 * Propogate times from the cache info.
	 */
	if (descPtr->accessTime < cachedAttrPtr->accessTime) {
	    descPtr->accessTime = cachedAttrPtr->accessTime;
	    descPtr->flags |= FS_FD_DIRTY;
	}
	if (descPtr->dataModifyTime < cachedAttrPtr->modifyTime) {
	    descPtr->dataModifyTime = cachedAttrPtr->modifyTime;
	    descPtr->flags |= FS_FD_DIRTY;
	}
	/*
	 * Write the descriptor back to the cache.
	 */
	if (descPtr->flags & FS_FD_DIRTY) {
	    descPtr->flags &= ~FS_FD_DIRTY;
	    status =  FsStoreFileDesc(domainPtr, hdrPtr->fileID.minor, descPtr);
	    if (status != SUCCESS) {
		printf(
		    "FsHandleDescWriteBack: Couldn't store file desc <%x>\n",
		    status);
	    }
	}
	FsDomainRelease(hdrPtr->fileID.major);
    }

    if (shutdown & lockedDesc) {
	printf("FsHandleDescWriteBack: %d descriptors still locked\n",
		    lockedDesc);
    }

    UNLOCK_MONITOR;

    return(lockedDesc);
}

