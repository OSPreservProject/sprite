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
#include "fsNamedPipe.h"
#include "hash.h"

static Sync_Lock fileHashLock = {0, 0};
#define	LOCKPTR	&fileHashLock

/*
 * Macro for debug trace prints.
 */
int fsHandleTrace = FALSE;
#ifndef CLEAN
#define HANDLE_TRACE(hdrPtr, comment) \
    if (fsHandleTrace) {						\
	Sys_Printf("<%d, %d, %d, %d> flags %x ref %d : %s\n",		\
	(hdrPtr)->fileID.type, (hdrPtr)->fileID.serverID,		\
	(hdrPtr)->fileID.major, (hdrPtr)->fileID.minor,		\
	(hdrPtr)->flags, (hdrPtr)->refCount, comment);			\
    }
#else
#define HANDLE_TRACE
#endif

/*
 * Hash tables for files and blocks.
 */

Hash_Table	fileHashTableStruct;
Hash_Table	*fileHashTable = &fileHashTableStruct;

/*
 * Flags for handles referenced from the hash table.
 *	FS_HANDLE_INSTALLED - the structure has been put into the hash table.
 *	FS_HANDLE_LOCKED    - the lock bit on the handle.  Handles are locked
 *		when they are fetched from the hash table.
 *	FS_HANDLE_WANTED    - set when iterating through the handle table
 *		so that the handle doens't get free'ed by FsHandleRemoveInt
 *	FS_HANDLE_REMOVED   - set instead of freeing the handle if the
 *		remove procedure sees the 'wanted' bit.
 *	FS_HANDLE_INVALID   - set when a handle becomes invalid after
 *		a recovery error.
 *	FS_HANDLE_DONT_CACHE - set if the handle should be removed after
 *		the last reference is released.
 */
#define FS_HANDLE_INSTALLED	0x1
#define FS_HANDLE_LOCKED	0x2
#define FS_HANDLE_WANTED	0x4
#define FS_HANDLE_REMOVED	0x8
#define FS_HANDLE_FREED		0x10
#define FS_HANDLE_INVALID	0x20
#define FS_HANDLE_DONT_CACHE	0x40

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
 * Macro to give name associated with a handle.
 */
#define HDR_FILE_NAME(hdrPtr) \
	(((hdrPtr)->name == (char *)NIL) ? "(no name)" : hdrPtr->name)
void	HandleWakeup();


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
    Hash_Init(fileHashTable, fileHashSize, Hash_Size(sizeof(Fs_FileID)));
}

int	fsMaxNumHandles = 1024;
int	fsMinScavengeInterval = 5;
extern	fsLastScavengeTime;


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
    register	Hash_Entry	*hashEntryPtr;
    register	FsHandleHeader	*hdrPtr;
    Boolean			found;

    LOCK_MONITOR;
    fsStats.handle.installCalls++;
again:
    hashEntryPtr = Hash_Find(fileHashTable, (Address) fileIDPtr);
    if (hashEntryPtr->value == (Address) NIL) {
	found = FALSE;
	fsStats.handle.created++;
	fsStats.handle.exists++;
	/*
	 * Use a crude mechanism to bound the number of file handles.
	 * This should be fixed to use an LRU mechanism with a flexible
	 * limit on the number of handles.
	 */
	if (fsStats.handle.exists > fsMaxNumHandles &&
	    fsTimeInSeconds - fsLastScavengeTime > fsMinScavengeInterval) {
	    Fs_HandleScavengeStub((ClientData)0);
	}
	/*
	 * Allocate and install a new file handle.  Because there are
	 * various things behind the handle header our caller specifies the
	 * size.
	 */
	hdrPtr = (FsHandleHeader *) Mem_Alloc(size);
	Byte_Zero(size, (Address) hdrPtr);
	hdrPtr->fileID = *fileIDPtr;
	hdrPtr->refCount = 1;
	hdrPtr->flags |= FS_HANDLE_INSTALLED;
#ifndef FS_NO_HDR_NAMES
	if (name != (char *)NIL) {
	    hdrPtr->name = (char *)Mem_Alloc(String_Length(name) + 1);
	    (void)String_Copy(name, hdrPtr->name);
	} else {
	    hdrPtr->name = (char *)NIL;
	}
#else
	hdrPtr->name = (char *)NIL;
#endif
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
	FS_TRACE_HANDLE(FS_TRACE_INSTALL_HIT, hdrPtr);
#ifndef FS_NO_HDR_NAMES
	if ((hdrPtr->name == (char *)NIL) && (name != (char *)NIL)) {
	    hdrPtr->name = (char *)Mem_Alloc(String_Length(name) + 1);
	    (void)String_Copy(name, hdrPtr->name);
	}
#endif FS_NO_HDR_NAMES
    }
    hdrPtr->flags |= FS_HANDLE_LOCKED;
    fsStats.handle.locks++;
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
 *	the name component hash table.
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
 *	None.
 *
 * Side effects:
 *	The lock bit is removed from the flags filed for the handle.
 *
 *----------------------------------------------------------------------------
 *
 */
ENTRY void
FsHandleUnlockHdr(hdrPtr)
    register	FsHandleHeader	*hdrPtr;
{
    LOCK_MONITOR;

    if ((hdrPtr->flags & FS_HANDLE_LOCKED) == 0) {
	UNLOCK_MONITOR;
	Sys_Panic(SYS_FATAL, "HandleUnlock, un-locked handle\n");
	return;
    }
    UNLOCK_HANDLE(hdrPtr);

    UNLOCK_MONITOR;
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
	Sys_Panic(SYS_FATAL,
	    "HandleRelease, handle <%d,%d,%d,%d> \"%s\" not locked\n",
	    hdrPtr->fileID.type, hdrPtr->fileID.serverID,
	    hdrPtr->fileID.major, hdrPtr->fileID.minor, HDR_FILE_NAME(hdrPtr));
	return;
    }
    hdrPtr->refCount--;

    if (hdrPtr->refCount < 0) {
	UNLOCK_MONITOR;
	Sys_Panic(SYS_FATAL, "refCount %d on %s handle <%d,%d,%d> \"%s\"\n",
		hdrPtr->refCount, FsFileTypeToString(hdrPtr->fileID.type),
		hdrPtr->fileID.serverID, hdrPtr->fileID.major,
		hdrPtr->fileID.minor, HDR_FILE_NAME(hdrPtr));
	return;
    } else if ((hdrPtr->refCount == 0) &&
	       (hdrPtr->flags & FS_HANDLE_REMOVED) &&
	       ((hdrPtr->flags & FS_HANDLE_WANTED) == 0)) {
	/*
	 * The handle has been removed, we are the last reference, and
	 * noone in GetNextHandle is trying to grab this handle.
	 */
	FS_TRACE_HANDLE(FS_TRACE_RELEASE_FREE, hdrPtr);
        Mem_Free((Address)hdrPtr);
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
	    Sys_Panic(SYS_FATAL,
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

    if ((hdrPtr->flags & FS_HANDLE_WANTED) ||
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
	if (hdrPtr->name != (char *)NIL) {
	    Mem_Free((Address) hdrPtr->name);
	}
	Mem_Free((Address) hdrPtr);
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
	Mem_Free((Address)handlePtr->descPtr);
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
 *	None.
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
	    Sys_Panic(SYS_WARNING, "GetNextHandle skipping %s <%d,%d> \"%s\"\n",
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
	if (hdrPtr->flags & FS_HANDLE_REMOVED) {
	    hdrPtr->flags &= ~FS_HANDLE_WANTED;
	    UNLOCK_HANDLE(hdrPtr);
	    if (hdrPtr->refCount == 0) {
		Mem_Free((Address) hdrPtr);
	    }
	    continue;
	} else {
	    hdrPtr->flags &= ~FS_HANDLE_WANTED;
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
	    Sys_Panic(SYS_FATAL,
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
	    case FS_LCL_NAMED_PIPE_STREAM: {
		register FsNamedPipeIOHandle *handlePtr =
			(FsNamedPipeIOHandle *) hdrPtr;
		descPtr = handlePtr->descPtr;
		cachedAttrPtr = &handlePtr->cacheInfo.attr;
		break;
	    }
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
		Sys_Panic(SYS_WARNING,
		    "FsHandleDescWriteBack: Couldn't store file desc <%x>\n",
		    status);
	    }
	}
	FsDomainRelease(hdrPtr->fileID.major);
    }

    if (shutdown & lockedDesc) {
	Sys_Printf("FsHandleDescWriteBack: %d descriptors still locked\n",
		    lockedDesc);
    }

    UNLOCK_MONITOR;

    return(lockedDesc);
}

