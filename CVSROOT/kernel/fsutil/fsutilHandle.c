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

static Sync_Lock handleTableLock = Sync_LockInitStatic("Fs:handleTable");
#define	LOCKPTR	&handleTableLock

/*
 * Synchronization and termination variables for LRU replacement.
 */
static Sync_Condition lruDone;
static Boolean lruInProgress;
static int lruHandlesChecked;

/*
 * Hash tables for object handles.  These are kept in LRU order and
 * a soft limit on their number is enforced.  If the number of handles
 * gets beyond fsStats.handle.maxNumber then LRU replacement is done until
 * handleScavengeThreashold are replaced.  If all handles are in
 * use the max table size is allowed to grow by handleLimitInc.
 * The table never shrinks on the premise that once it has grown the
 * memory allocator establishes a high water mark and shrinking the table
 * won't really help overall kernel memory usage.
 */
Hash_Table	fileHashTableStruct;
Hash_Table	*fileHashTable = &fileHashTableStruct;
static List_Links lruListHeader;
static List_Links *lruList = &lruListHeader;
/*
 * FS_HANDLE_TABLE_SIZE is the initial size of the handle table.
 *	Observation reveals that it takes about 120 handles to get
 *	through multi-user bootstrap, and 135 to get through the first login.
 *	Starting the window system jumps the number of handles to about 250.
 *	Running a large compilation pushes the table up to about 450 handles.
 *	The file servers ramp up to 2500 handles, but this is a
 *	function of the activity of their clients and the number of
 *	directories they have.
 * LIMIT_INC defines the amount the table grows by
 * THREASHOLD defines how many handles are reclaimed before stopping.  This
 *	is set very low so in the steady state we don't waste much time
 *	looking at un-reclaimable handles.
 */
#define		FS_HANDLE_TABLE_SIZE	   400
#define		LIMIT_INC(max)		   ( 25 )
int		handleLimitInc =	   LIMIT_INC(FS_HANDLE_TABLE_SIZE);
#define		THREASHOLD(max)		   ( 1 )
int		handleScavengeThreashold = THREASHOLD(FS_HANDLE_TABLE_SIZE);

FsHandleHeader *GetNextLRUHandle();
void DoneLRU();

/*
 * Flags for handles referenced from the hash table.
 *	FS_HANDLE_INSTALLED - the structure has been put into the hash table.
 *	FS_HANDLE_LOCKED    - the lock bit on the handle.  Handles are locked
 *		when they are fetched from the hash table.
 *	FS_HANDLE_REMOVED   - set instead of freeing the handle if the
 *		remove procedure sees the 'wanted' or 'don't move' bit.
 *	FS_HANDLE_INVALID   - set when a handle becomes invalid after
 *		a recovery error.
 *	FS_HANDLE_DONT_CACHE - set if the handle should be removed after
 *		the last reference is released.
 */
#define FS_HANDLE_INSTALLED	0x1
#define FS_HANDLE_LOCKED	0x2
#define FS_HANDLE_REMOVED	0x8
#define FS_HANDLE_FREED		0x10
#define FS_HANDLE_INVALID	0x20
#define FS_HANDLE_DONT_CACHE	0x40

/*
 * LOCK_HANDLE --
 * Macro to lock a file handle.   The handle lock is used during open/close
 * and migration to serialize fetching, releasing, installing,
 * and removing the handle.  Additionally, some object types use the
 * handle lock for synchronization of I/O operations.
 * If CLEAN is defined we don't remember the process ID of the locker,
 * nor do we count events.
 */

#ifdef CLEAN

#define	LOCK_HANDLE(hdrPtr) \
	while (((hdrPtr)->flags & FS_HANDLE_LOCKED) && !sys_ShuttingDown) { \
	    (void) Sync_Wait(&((hdrPtr)->unlocked), FALSE); \
	} \
	(hdrPtr)->flags |= FS_HANDLE_LOCKED;

#else

#define LOCK_HANDLE(hdrPtr) \
	while (((hdrPtr)->flags & FS_HANDLE_LOCKED) && !sys_ShuttingDown) { \
	    fsStats.handle.lockWaits++; \
	    (void) Sync_Wait(&((hdrPtr)->unlocked), FALSE); \
	} \
	fsStats.handle.locks++; \
	(hdrPtr)->lockProcID = (int)Proc_GetEffectiveProc(); \
	(hdrPtr)->flags |= FS_HANDLE_LOCKED;

#endif

/*
 * UNLOCK_HANDLE --
 *	Unlock a handle so it can be fetched by another process.
 *	This clears the lock bit and notifies the waiting condition.
 */

#ifdef CLEAN

#define	UNLOCK_HANDLE(hdrPtr) \
	(hdrPtr)->flags &= ~FS_HANDLE_LOCKED; \
	Sync_Broadcast(&((hdrPtr)->unlocked));

#else

#define	UNLOCK_HANDLE(hdrPtr) \
	(hdrPtr)->flags &= ~FS_HANDLE_LOCKED; \
	(hdrPtr)->lockProcID = NIL; \
	fsStats.handle.unlocks++; \
	Sync_Broadcast(&((hdrPtr)->unlocked));

#endif
/*
 * REMOVE_HANDLE --
 * Macro to remove a handle, so no details get fogotten.  Note, removal
 * from the hash table is separate, because we do that first, and then
 * clean it up completely later with this macro.
 */

#define REMOVE_HANDLE(hdrPtr) \
	if ((hdrPtr)->lruLinks.nextPtr != (List_Links *)NIL) {	\
	    List_Remove(&(hdrPtr)->lruLinks);		 	\
	    fsStats.handle.lruEntries--;			\
	}							\
	if ((hdrPtr)->name != (char *)NIL) {			\
	    free((hdrPtr)->name);				\
	}							\
	free((char *)(hdrPtr));

/*
 * MOVE_HANDLE --
 * Macro to move a handle to the back (most recent) end of the LRU list.
 * Not all handle types are in the LRU list, hence the check against NIL.
 */

#define MOVE_HANDLE(hdrPtr) \
	    if ((hdrPtr)->lruLinks.nextPtr != (List_Links *)NIL) {	\
		List_Move(&(hdrPtr)->lruLinks, LIST_ATREAR(lruList));	\
	    }

/*
 * HDR_FILE_NAME --
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
	(hdrPtr)->fileID.major, (hdrPtr)->fileID.minor,			\
	(hdrPtr)->flags, (hdrPtr)->refCount, comment);			\
    }
#else
#define HANDLE_TRACE(hdrPtr, comment)
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
    fsStats.handle.maxNumber = FS_HANDLE_TABLE_SIZE;
    List_Init(lruList);
    Hash_Init(fileHashTable, fileHashSize, Hash_Size(sizeof(Fs_FileID)));
}


/*
 *----------------------------------------------------------------------------
 *
 * FsHandleInstall --
 *
 *      Install a file handle given its fileID.  The caller is responsible
 *	for initializing type-specific fields if this procedure returns
 *	FALSE to indicate the handle was newly created.  The handle is
 *	returned locked and with a single reference that has to be
 *	released with FsHandleRelease.
 *
 * Results:
 *	TRUE is returned if the file handle was already in the 
 *	hash table.  Upon return, *hdrPtrPtr references the installed handle.
 *
 * Side effects:
 *	New handle may be allocated and installed into the hash table.
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
    Boolean tableFull;
    register int numScavenged;
    register FsHandleHeader *hdrPtr;
    register FsHandleHeader *newHdrPtr = (FsHandleHeader *)NIL;

    fsStats.handle.installCalls++;
    do {
	/*
	 * Due to memory limitations we structure this so we malloc()
	 * outside the handle monitor lock.  That way we can still
	 * sync the disks if malloc fails.
	 * 1. Try a fetch.  This returns a locked handle, or NIL.
	 * 2. Allocate memory for the new handle.
	 * 3. Install the handle in the table.
	 * 4. If the install fails we do LRU replacment and loop to step 1.
	 */
	hdrPtr = FsHandleFetch(fileIDPtr);
	if (hdrPtr != (FsHandleHeader *)NIL) {
	    found = TRUE;
	    break;
	}
	found = FALSE;
	if (newHdrPtr == (FsHandleHeader *)NIL) {
	    newHdrPtr = (FsHandleHeader *)malloc(size);
	    if (name != (char *)NIL) {
		newHdrPtr->name = (char *)malloc(strlen(name) + 1);
		(void)strcpy(newHdrPtr->name, name);
	    } else {
		newHdrPtr->name = (char *)NIL;
	    }
	}
	tableFull = HandleInstallInt(fileIDPtr, fsStats.handle.maxNumber,
				     newHdrPtr, &found);
	if (!tableFull) {
	    /*
	     * Got the handle.  'found' indicates if the handle is new or not.
	     */
	    hdrPtr = newHdrPtr;
	} else {
	    /*
	     * Limit would be exceeded, recycle some handles.
	     */
	    numScavenged = 0;
	    fsStats.handle.lruScans++;
	    for (hdrPtr = GetNextLRUHandle();
		 hdrPtr != (FsHandleHeader *)NIL;
		 hdrPtr = GetNextLRUHandle()) {
		if ((*fsStreamOpTable[hdrPtr->fileID.type].scavenge)(hdrPtr)) {
		    numScavenged++;
		    fsStats.handle.lruHits++;
		    if (numScavenged >= handleScavengeThreashold) {
			break;
		    }
		} else {
		    fsStats.handle.lruChecks++;
		}
	    }
	    /*
	     * Finish LRU and grow the table if needed.
	     */
	    DoneLRU(numScavenged);
	    hdrPtr = (FsHandleHeader *)NIL;
	}
    } while (hdrPtr == (FsHandleHeader *)NIL);

    if (found) {
	/*
	 * Handle exists. Free up the handle we may have allocated.
	 * Adjust the name on the handle we found if we have a better one.
	 */
	fsStats.handle.installHits++;
	if (newHdrPtr != (FsHandleHeader *)NIL) {
	    if (newHdrPtr->name != (char *)NIL) {
		free(newHdrPtr->name);
	    }
	    free((Address)newHdrPtr);
	}
	if ((hdrPtr->name == (char *)NIL) && (name != (char *)NIL)) {
	    hdrPtr->name = (char *)malloc(strlen(name) + 1);
	    (void)strcpy(hdrPtr->name, name);
	}
    }
    *hdrPtrPtr = hdrPtr;
    return(found);
}

/*
 *----------------------------------------------------------------------------
 *
 * HandleInstallInt --
 *
 *      Install a file handle.  This enforces a soft limit on the number
 *	of handles that can exist.  Our caller has to allocate space for
 *	the handle.
 *
 * Results:
 *	TRUE is returned if the file handle table was full and our
 *	caller should do LRU replacement on the table.
 *	*foundPtr is set to TRUE if the handle was found.  This is possible
 *	on a multiprossor even if our caller has first tried a fetch.
 *
 * Side effects:
 *	The new handle is installed into the hash table.
 *
 *----------------------------------------------------------------------------
 *
 */
ENTRY Boolean
HandleInstallInt(fileIDPtr, handleLimit, hdrPtr, foundPtr)
    register Fs_FileID	*fileIDPtr;	/* Identfies handle to install. */
    int			handleLimit;	/* Determines how many handles can
					 * exist before we return NULL */
    FsHandleHeader	*hdrPtr;	/* Handle to install into table. */    
    Boolean		*foundPtr;	/* TRUE upon return if handle found */
{
    register	Hash_Entry	*hashEntryPtr;
    Boolean			tableFull = FALSE;
    Boolean			found;

    LOCK_MONITOR;
again:
    hashEntryPtr = Hash_Find(fileHashTable, (Address) fileIDPtr);
    if (hashEntryPtr->value == (Address) NIL) {
	found = FALSE;
	if (fsStats.handle.exists >= handleLimit) {
	    /*
	     * Creating a handle would push us over the limit.
	     * If LRU is already in progress we wait so there is only
	     * one process scanning the table at a time.
	     */
	    if (lruInProgress) {
		do {
		    Sync_Wait(&lruDone, FALSE);
		} while (lruInProgress);
		goto again;
	    } else {
		lruInProgress = TRUE;
		lruHandlesChecked = 0;
		tableFull = TRUE;
		goto exit;
	    }
	}
	/*
	 * Initialize the new file handle.
	 */
	fsStats.handle.created++;
	fsStats.handle.exists++;

	hdrPtr->fileID = *fileIDPtr;
	hdrPtr->flags = FS_HANDLE_INSTALLED;
	hdrPtr->unlocked.waiting = FALSE;
	hdrPtr->refCount = 1;
	/*
	 * Put the handle in the LRU list only if it has a scavenging
	 * routine defined for it.  This allows us to avoid checking
	 * un-reclaimable things.
	 */
	if (fsStreamOpTable[fileIDPtr->type].scavenge != (Boolean (*)())NIL) {
	    List_InitElement(&hdrPtr->lruLinks);
	    List_Insert(&hdrPtr->lruLinks, LIST_ATREAR(lruList));
	    fsStats.handle.lruEntries++;
	} else {
	    hdrPtr->lruLinks.nextPtr = (List_Links *)NIL;
	    hdrPtr->lruLinks.prevPtr = (List_Links *)NIL;
	}
	Hash_SetValue(hashEntryPtr, hdrPtr);
	FS_TRACE_HANDLE(FS_TRACE_INSTALL_NEW, hdrPtr);
    } else {
	found = TRUE;
	hdrPtr = (FsHandleHeader *) Hash_GetValue(hashEntryPtr);
	if (hdrPtr->flags & FS_HANDLE_LOCKED) {
	    /*
	     * Wait for it to become unlocked.  We can't increment the
	     * the reference count until we lock it, so we have to
	     * jump back and rehash as the handle may have been deleted.
	     */
	    (void) Sync_Wait(&hdrPtr->unlocked, FALSE);
	    fsStats.handle.lockWaits++;
	    goto again;
	}
	hdrPtr->refCount++;
	MOVE_HANDLE(hdrPtr);
	FS_TRACE_HANDLE(FS_TRACE_INSTALL_HIT, hdrPtr);
    }
    LOCK_HANDLE(hdrPtr);
exit:
    *foundPtr = found;
    UNLOCK_MONITOR;
    return(tableFull);
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
 *	Locks the handle and increments its reference count.  The handle
 *	is also moved to the end of the LRU list.
 *
 *----------------------------------------------------------------------------
 *
 */
ENTRY FsHandleHeader *
FsHandleFetch(fileIDPtr)
    Fs_FileID 	*fileIDPtr;	/* Identfies handle to fetch. */
{
    Hash_Entry	*hashEntryPtr;
    FsHandleHeader	*hdrPtr;

    LOCK_MONITOR;

    fsStats.handle.fetchCalls++;

again:
    /*
     * Look in the hash table.  A bucket might have been installed by
     * FsHandleInstall, but the value might be NIL because the
     * handle table's size would have been exceeded by creating the handle.
     */
    hdrPtr = (FsHandleHeader *)NIL;
    hashEntryPtr = Hash_LookOnly(fileHashTable, (Address) fileIDPtr);
    if (hashEntryPtr != (Hash_Entry *) NIL) {
	hdrPtr = (FsHandleHeader *) Hash_GetValue(hashEntryPtr);
	if (hdrPtr != (FsHandleHeader *)NIL) {
	    fsStats.handle.fetchHits++;
	    if (hdrPtr->flags & FS_HANDLE_LOCKED) {
		/*
		 * Wait for it to become unlocked and then rehash.
		 * The handle could get removed before we get a chance
		 * to increment the reference count on it.
		 */
		fsStats.handle.lockWaits++;
		(void) Sync_Wait(&hdrPtr->unlocked, FALSE);
		goto again;
	    }
	    MOVE_HANDLE(hdrPtr);
	    LOCK_HANDLE(hdrPtr);
	    hdrPtr->refCount++;
	}
    }
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
	return(FALSE);
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
 *	if the handle is already locked, in which case it is unlocked.
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
    fsStats.handle.release++;

    if (locked && ((hdrPtr->flags & FS_HANDLE_LOCKED) == 0)) {
	UNLOCK_MONITOR;
	panic("HandleRelease, handle <%d,%d,%d,%d> \"%s\" not locked\n",
	    hdrPtr->fileID.type, hdrPtr->fileID.serverID,
	    hdrPtr->fileID.major, hdrPtr->fileID.minor, HDR_FILE_NAME(hdrPtr));
	return;
    }
    hdrPtr->refCount--;

    if (hdrPtr->refCount < 0) {
	UNLOCK_MONITOR;
	panic("refCount %d on %s handle <%d,%d,%d> \"%s\"\n",
		hdrPtr->refCount, FsFileTypeToString(hdrPtr->fileID.type),
		hdrPtr->fileID.serverID, hdrPtr->fileID.major,
		hdrPtr->fileID.minor, HDR_FILE_NAME(hdrPtr));
	return;
    } else if ((hdrPtr->refCount == 0) &&
	       (hdrPtr->flags & FS_HANDLE_REMOVED)) {
	/*
	 * The handle has been removed, and we are the last reference.
	 */
	fsStats.handle.limbo--;
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
	    panic("FsHandleRemoveInt: Couldn't find handle in hash table.\n");
	    LOCK_MONITOR;
	    return;
	}
	Hash_SetValue(hashEntryPtr, NIL);
	Hash_Delete(fileHashTable, hashEntryPtr);
    }
    fsStats.handle.exists--;

    /*
     * Wakeup anyone waiting for this handle to become unlocked.
     */
    UNLOCK_HANDLE(hdrPtr);

    if (hdrPtr->refCount > 0) {
	/*
	 * We've removed the handle from the hash table so it won't
	 * be found, but someone has a reference to it.  HandleRelease
	 * will free the handle for us later.
	 */
	fsStats.handle.limbo++;
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
 *	TRUE if it actually removed the handle.
 *
 * Side effects:
 *	Frees the memory for the handles file descriptor.
 *
 *----------------------------------------------------------------------------
 *
 */

ENTRY Boolean
FsHandleAttemptRemove(handlePtr)
    register FsLocalFileIOHandle *handlePtr;	/* Handle to try and remove. */
{
    register Boolean removed;
    LOCK_MONITOR;
    if (handlePtr->hdr.refCount == 0) {
	free((Address)handlePtr->descPtr);
	handlePtr->descPtr = (FsFileDescriptor *)NIL;
	FsFileSyncLockCleanup(handlePtr);
	FsHandleRemoveInt((FsHandleHeader *)handlePtr);
	removed = TRUE;
    } else {
	removed = FALSE;
	UNLOCK_HANDLE((FsHandleHeader *)handlePtr);
    }
    UNLOCK_MONITOR;
    return(removed);
}

/*
 *----------------------------------------------------------------------------
 *
 * FsGetNextHandle --
 *
 *	Get the next handle in the hash table.  Return the handle locked.
 *	The hashSearchPtr is initialized with Hash_StartSearch.  This
 *	always skips locked handles.  The users of this routine are:
 *	recovery stuff:  it is important not to block recovery as
 *		that ends up hanging the whole machine.
 *	unmounting a disk: this is a rare operation, but it should not
 *		be hung-up by a wedged handle.
 *	scavenging:  if a handle is locked, then it should not
 *		be scavenged anyway.
 *
 * Results:
 *	The next handle in the hash table.
 *
 * Side effects:
 *	Locks the handle (but does not increment its reference count).
 *	This prints a warning if a handle is skipped when locked.
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
	if (hdrPtr == (FsHandleHeader *)NIL) {
	    /*
	     * Caught handle in the process of being installed.
	     */
	    continue;
	}
	/*
	 * Skip locked handles to avoid hanging the system on locked handle. 
	 */
	if (hdrPtr->flags & FS_HANDLE_LOCKED) {
	    printf( "GetNextHandle skipping %s <%d,%d> \"%s\"\n",
		FsFileTypeToString(hdrPtr->fileID.type),
		hdrPtr->fileID.major, hdrPtr->fileID.minor,
		HDR_FILE_NAME(hdrPtr));
	    continue;
	}
	if (hdrPtr->flags & FS_HANDLE_REMOVED) {
	    continue;
	}
	LOCK_HANDLE(hdrPtr);
	UNLOCK_MONITOR;
	return(hdrPtr);
    }

    UNLOCK_MONITOR;
    return((FsHandleHeader *) NIL);
}

/*
 *----------------------------------------------------------------------------
 *
 * GetNextLRUHandle --
 *
 *	Get the next handle in the LRU list.  Return the handle locked.
 *	This skips locked handles because they are obviously in use and not good
 *	candidates for removal.  This also prevents a single locked handle
 *	from clogging up the system.
 *
 * Results:
 *	The next handle in the LRU list.
 *
 * Side effects:
 *	Increments the number of handles checked in this LRU scan so
 *	we know when to terminate.
 *
 *----------------------------------------------------------------------------
 *
 */
ENTRY FsHandleHeader *
GetNextLRUHandle()
{
    register 	FsHandleHeader	*hdrPtr;
    register	List_Links	*listPtr;

    LOCK_MONITOR;
    if (lruHandlesChecked >= fsStats.handle.maxNumber) {
	hdrPtr = (FsHandleHeader *)NIL;
	goto exit;
    }
    /*
     * Get the candidate handle and move it to the young end of
     * the list in case it is not replaced.
     */
    listPtr = List_First(lruList);
    hdrPtr = LRU_LINKS_TO_HANDLE(listPtr);
    MOVE_HANDLE(hdrPtr);
    lruHandlesChecked++;
    /*
     * Now skip over locked and removed handles, moving them to the "young" end.
     */
    while (hdrPtr->flags & (FS_HANDLE_REMOVED|FS_HANDLE_LOCKED)) {
	if (lruHandlesChecked >= fsStats.handle.maxNumber) {
	    hdrPtr = (FsHandleHeader *)NIL;
	    goto exit;
	} else {
	    listPtr = List_First(lruList);
	    hdrPtr = LRU_LINKS_TO_HANDLE(listPtr);
	    MOVE_HANDLE(hdrPtr);
	}
	lruHandlesChecked++;
    }
    LOCK_HANDLE(hdrPtr);
exit:
    UNLOCK_MONITOR;
    return(hdrPtr);
}

/*
 *----------------------------------------------------------------------------
 *
 * DoneLRU --
 *
 *	Terminate LRU iteration.  This grows the table if needed and
 *	then notifies anyone waiting for LRU replacement to finish.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Grows the table if it needs to be grown.
 *
 *----------------------------------------------------------------------------
 *
 */
ENTRY void
DoneLRU(numScavenged)
    int numScavenged;		/* Number of handles replaced */
{
    LOCK_MONITOR;
    if (numScavenged == 0) {
	/*
	 * Grow the table a bit because no handles could be reclaimed.
	 */
	fsStats.handle.maxNumber += handleLimitInc;
	handleLimitInc = LIMIT_INC(fsStats.handle.maxNumber);
	handleScavengeThreashold = THREASHOLD(fsStats.handle.maxNumber);
    }
    lruInProgress = FALSE;
    Sync_Broadcast(&lruDone);
    UNLOCK_MONITOR;
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
	    panic("FsHandleInvalidate: Can't find %s handle <%d,%d,%d>\n",
			FsFileTypeToString(hdrPtr->fileID.type),
			hdrPtr->fileID.serverID,
			hdrPtr->fileID.major, hdrPtr->fileID.minor);
	    return;
	}
	Hash_SetValue(hashEntryPtr, NIL);
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
	if (hdrPtr == (FsHandleHeader *)NIL) {
	    /*
	     * Handle has been removed.
	     */
	    continue;
	}
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
	if (descPtr == (FsFileDescriptor *)NIL) {
	    UNLOCK_MONITOR;
	    panic("FsHandleDescWriteBack, no descriptor\n");
	    return (lockedDesc);
	}
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
		printf("FsHandleDescWriteBack: Couldn't store file desc <%x>\n",
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

