/* 
 * fsutilHandle.c --
 *
 *	Routines to manage file handles.  They are kept in a table hashed
 *	by the Fs_FileID type.  They are referenced counted and eligible for
 *	removal when their reference count goes to zero.  Fsutil_HandleInstall
 *	adds handles to the table.  Fsutil_HandleFetch returns a locked handle.
 *	Fsutil_HandleLock locks a handle that you already have.
 *	Installing initializes the refCount to 1, and Fetching increments it.
 *	Use Fsutil_HandleUnlock and Fsutil_HandleReleaseHdr to unlock and
 *	decrement the reference count, respectively.  The macros
 *	Fsutil_HandleFetchType and Fsutil_HandleRelease do type casting and
 *	are defined in fsInt.h.  Fsutil_HandleRemove deletes a handle from
 *	the table, and Fsutil_GetNextHandle is used to iterate through the
 *	whole hash table.
 *
 * Copyright 1986 Regents of the University of California.
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include <fs.h>
#include <fsutil.h>
#include <fsStat.h>
#include <fslcl.h>
#include <fsdm.h>
#include <fsio.h>
#include <fsutilTrace.h>
#include <fsNameOps.h>
#include <hash.h>

#include <string.h>
#include <stdio.h>

static Sync_Lock handleTableLock = Sync_LockInitStatic("Fs:handleTable");
#define	LOCKPTR	&handleTableLock

/*
 * Synchronization and termination variables for LRU replacement.
 */
Sync_Condition lruDone;
static Boolean lruInProgress;
static int lruHandlesChecked;
/*
 * NOTE: The current implementation of handle scavenging leads to serious
 * performance problems.  On systems with large file caches almost all the
 * handles on the LRU will have files in the cache and wont be scavenged.
 * Everytime a new handle needs to be created the entire LRU is scanned 
 * before the code gives up and malloc's memory. This needs to be fixed. 
 */

/*
 * Hash tables for object handles.  These are kept in LRU order and
 * a soft limit on their number is enforced.  If the number of handles
 * gets beyond fs_Stats.handle.maxNumber then LRU replacement is done until
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

#define		LIMIT_INC(max)		   ( 100 )
int		handleLimitInc =	   LIMIT_INC(FS_HANDLE_TABLE_SIZE);
#define		THREASHOLD(max)		   ( 1 )
int		handleScavengeThreashold = THREASHOLD(FS_HANDLE_TABLE_SIZE);

extern Fs_HandleHeader *GetNextLRUHandle _ARGS_((void));
extern void DoneLRU _ARGS_((int numScavenged));

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
	    fs_Stats.handle.lockWaits++; \
	    (void) Sync_Wait(&((hdrPtr)->unlocked), FALSE); \
	} \
	fs_Stats.handle.locks++; \
	(hdrPtr)->lockProcPtr = Proc_GetEffectiveProc(); \
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
	(hdrPtr)->lockProcPtr = (Proc_ControlBlock *)NIL; \
	fs_Stats.handle.unlocks++; \
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
	    fs_Stats.handle.lruEntries--;			\
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

extern Boolean HandleInstallInt _ARGS_((Fs_FileID *fileIDPtr, 
		unsigned int handleLimit, Fs_HandleHeader **hdrPtrPtr, 
		Boolean *foundPtr, Boolean returnLocked));


/*
 *----------------------------------------------------------------------------
 *
 * Fsutil_HandleInit --
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
Fsutil_HandleInit(fileHashSize)
    int	fileHashSize;	/* The number of hash table entries to put in the
			 * file hash table for starters. */
{
    /*
     * Set the initial number of handle to be the maximum of 
     * FS_HANDLE_TABLE_SIZE and 1/4 of the maximum number of blocks in the
     * file cache.  This hack prevents the large overheads from
     * the initial growing of the handle table. 
     */
    fs_Stats.handle.maxNumber = FS_HANDLE_TABLE_SIZE;
    if (fs_Stats.handle.maxNumber < fs_Stats.blockCache.maxNumBlocks/4) {
	fs_Stats.handle.maxNumber = fs_Stats.blockCache.maxNumBlocks/4;
    }
    List_Init(lruList);
    Hash_Init(fileHashTable, fileHashSize, Hash_Size(sizeof(Fs_FileID)));
}


/*
 *----------------------------------------------------------------------------
 *
 * Fsutil_HandleInstall --
 *
 *      Install a file handle given its fileID.  The caller is responsible
 *	for initializing type-specific fields if this procedure returns
 *	FALSE to indicate the handle was newly created.  The handle is
 *	returned locked and with a single reference that has to be
 *	released with Fsutil_HandleRelease.
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
Fsutil_HandleInstall(fileIDPtr, size, name, cantBlock, hdrPtrPtr)
    register Fs_FileID	*fileIDPtr;	/* Identfies handle to install. */
    int		 	size;		/* True size of the handle.  This
					 * routine only looks at the header,
					 * but more data follows that. */
    char		*name;		/* File name for error messages */
    Boolean		cantBlock;	/* TRUE if this call shouldn't block
					 * because the handle already is locked.
					 */
    Fs_HandleHeader	**hdrPtrPtr;	/* Return pointer to handle that
					 * is found in the hash table. */
{
    Boolean found;
    Boolean tableFull;
    int numScavenged;
    Fs_HandleHeader *hdrPtr;
    Fs_HandleHeader *newHdrPtr = (Fs_HandleHeader *)NIL;
    Boolean returnLocked = TRUE;	/* For now, always return locked */

    fs_Stats.handle.installCalls++;
    do {
	Boolean wouldWait;
	/*
	 * Due to memory limitations we structure this so we malloc()
	 * outside the handle monitor lock.  That way we can still
	 * sync the disks if malloc fails.
	 * 1. Try a fetch.  This returns a locked handle, or NIL.
	 * 2. Allocate memory for the new handle.
	 * 3. Install the handle in the table.
	 * 4. If the install fails we do LRU replacment and loop to step 1.
	 */
	if (cantBlock) { 
	    hdrPtr = Fsutil_HandleFetchNoWait(fileIDPtr, &wouldWait);
	    if ((hdrPtr == (Fs_HandleHeader *)NIL) && wouldWait) {
		*hdrPtrPtr = hdrPtr;
		return TRUE;
	    }
	} else {
	    hdrPtr = Fsutil_HandleFetch(fileIDPtr);
	}
	if (hdrPtr != (Fs_HandleHeader *)NIL) {
	    found = TRUE;
	    break;
	}
	found = FALSE;
	if (newHdrPtr == (Fs_HandleHeader *)NIL) {
	    newHdrPtr = (Fs_HandleHeader *)malloc(size);
	    if (name != (char *)NIL) {
		newHdrPtr->name = (char *)malloc(strlen(name) + 1);
		(void)strcpy(newHdrPtr->name, name);
	    } else {
		newHdrPtr->name = (char *)NIL;
	    }
	}
	hdrPtr = newHdrPtr;
	tableFull = HandleInstallInt(fileIDPtr, fs_Stats.handle.maxNumber,
				     &hdrPtr, &found, returnLocked);
	if (tableFull) {
	    /*
	     * Size limit would be exceeded.  Recycle some handles.  The
	     * new handle has not been installed into the hash table yet.
	     */
	    numScavenged = 0;
	    fs_Stats.handle.lruScans++;
	    for (hdrPtr = GetNextLRUHandle();
		 hdrPtr != (Fs_HandleHeader *)NIL;
		 hdrPtr = GetNextLRUHandle()) {
		if ((*fsio_StreamOpTable[hdrPtr->fileID.type].scavenge)(hdrPtr)) {
		    numScavenged++;
		    fs_Stats.handle.lruHits++;
		    if (numScavenged >= handleScavengeThreashold) {
			break;
		    }
		} else {
		    fs_Stats.handle.lruChecks++;
		}
	    }
	    /*
	     * Finish LRU, grow the table if needed, and then
	     * loop back and try to fetch or install the handle again.
	     */
	    DoneLRU(numScavenged);
	    hdrPtr = (Fs_HandleHeader *)NIL;
	}
    } while (hdrPtr == (Fs_HandleHeader *)NIL);

    if (found) {
	/*
	 * Handle exists. Free up the handle we may have allocated.
	 * Adjust the name on the handle we found if we have a better one.
	 */
	fs_Stats.handle.installHits++;
	if (newHdrPtr != (Fs_HandleHeader *)NIL) {
	    if (newHdrPtr->name != (char *)NIL) {
		free(newHdrPtr->name);
	    }
	    free((Address)newHdrPtr);
	}
#ifdef sun4c
	if (name != (char *)NIL) {
	    if (hdrPtr->name != (char *) NIL) {
		free(hdrPtr->name);
	    }
	    hdrPtr->name = (char *)malloc(strlen(name) + 1);
	    (void)strcpy(hdrPtr->name, name);
	}
#else
	if ((hdrPtr->name == (char *)NIL) && (name != (char *)NIL)) {
	    hdrPtr->name = (char *)malloc(strlen(name) + 1);
	    (void)strcpy(hdrPtr->name, name);
	}
#endif /* sun4c */
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
HandleInstallInt(fileIDPtr, handleLimit, hdrPtrPtr, foundPtr, returnLocked)
    register Fs_FileID	*fileIDPtr;	/* Identfies handle to install. */
    unsigned int	handleLimit;	/* Determines how many handles can
					 * exist before we return NULL */
    Fs_HandleHeader	**hdrPtrPtr;	/* In - handle to install into table
					 * Out - handle found in table. */    
    Boolean		*foundPtr;	/* TRUE upon return if handle found */
    Boolean		returnLocked;	/* TRUE the handle is locked upon
					 * return.  Otherwise it is not
					 * locked, but its reference count
					 * is up so it won't go away. */
{
    register	Hash_Entry	*hashEntryPtr;
    register	Fs_HandleHeader	*hdrPtr;
    Boolean			tableFull = FALSE;
    Boolean			found;

    LOCK_MONITOR;
again:
    if (fs_Stats.handle.exists >= handleLimit) {
	/*
	 * Creating a handle will push us past the soft limit on handles.
	 * We just look into the hash table, but do not create a new
	 * entry if the handle isn't found.
	 */
	hashEntryPtr = Hash_LookOnly(fileHashTable, (Address) fileIDPtr);
	if (hashEntryPtr == (Hash_Entry *)NIL) {
	    /*
	     * Table is full so our caller has to do LRU replacement.
	     * If LRU is already in progress we wait so there is only
	     * one process scanning the table at a time.
	     */
	    if (lruInProgress) {
		do {
		    (void)Sync_Wait(&lruDone, FALSE);
		} while (lruInProgress);
		goto again;
	    } else {
		lruInProgress = TRUE;
		lruHandlesChecked = 0;
		found = FALSE;
		tableFull = TRUE;
		goto exit;
	    }
	}
    } else {
	/*
	 * Lookup the handle.  If a hash table entry doesn't exist
	 * it will be created by Hash_Find.
	 */
	hashEntryPtr = Hash_Find(fileHashTable, (Address) fileIDPtr);
    }
    if (hashEntryPtr->value == (Address) NIL) {
	/*
	 * Initialize the newly created file handle.  Our caller has
	 * allocated the space for the new handle.
	 */
	hdrPtr = *hdrPtrPtr;
	Hash_SetValue(hashEntryPtr, hdrPtr);
	found = FALSE;
	fs_Stats.handle.created++;
	fs_Stats.handle.exists++;

	hdrPtr->fileID = *fileIDPtr;
	hdrPtr->flags = FS_HANDLE_INSTALLED;
	hdrPtr->unlocked.waiting = FALSE;
	hdrPtr->refCount = 1;
	/*
	 * Put the handle in the LRU list only if it has a scavenging
	 * routine defined for it.  This allows us to avoid checking
	 * un-reclaimable things.
	 */
	if (fsio_StreamOpTable[fileIDPtr->type].scavenge != (Boolean (*)())NIL) {
	    List_InitElement(&hdrPtr->lruLinks);
	    List_Insert(&hdrPtr->lruLinks, LIST_ATREAR(lruList));
	    fs_Stats.handle.lruEntries++;
	} else {
	    hdrPtr->lruLinks.nextPtr = (List_Links *)NIL;
	    hdrPtr->lruLinks.prevPtr = (List_Links *)NIL;
	}
	FSUTIL_TRACE_HANDLE(FSUTIL_TRACE_INSTALL_NEW, hdrPtr);
    } else {
	hdrPtr = (Fs_HandleHeader *) Hash_GetValue(hashEntryPtr);
	if (hdrPtr->flags & FS_HANDLE_LOCKED) {
	    /*
	     * Wait for it to become unlocked.  We can't increment the
	     * the reference count until it is unlocked because it
	     * may be getting deleted.  If its locked we wait and retry.
	     */
	    (void) Sync_Wait(&hdrPtr->unlocked, FALSE);
	    fs_Stats.handle.lockWaits++;
	    goto again;
	}
	found = TRUE;
	hdrPtr->refCount++;
	MOVE_HANDLE(hdrPtr);
	FSUTIL_TRACE_HANDLE(FSUTIL_TRACE_INSTALL_HIT, hdrPtr);
	*hdrPtrPtr = hdrPtr;
    }
    if (returnLocked) {
	LOCK_HANDLE(hdrPtr);
    }
exit:
    *foundPtr = found;
    UNLOCK_MONITOR;
    return(tableFull);
}

/*
 *----------------------------------------------------------------------------
 *
 * Fsutil_HandleFetch --
 * Fsutil_HandleFetchType --
 *
 *	Return a pointer to a file handle out of the file handle hash table.  
 *	If no file handle is found then NIL is returned.  If one is found
 *	then it is returned locked.  The reference count is increased
 *	on the handle so it needs to be released with Fsutil_HandleRelease.
 *	Fsutil_HandleFetchType is a macro that does type casting, see fsInt.h 
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
ENTRY Fs_HandleHeader *
Fsutil_HandleFetch(fileIDPtr)
    Fs_FileID 	*fileIDPtr;	/* Identfies handle to fetch. */
{
    Hash_Entry	*hashEntryPtr;
    Fs_HandleHeader	*hdrPtr;

    LOCK_MONITOR;

    fs_Stats.handle.fetchCalls++;

again:
    /*
     * Look in the hash table.  A bucket might have been installed by
     * Fsutil_HandleInstall, but the value might be NIL because the
     * handle table's size would have been exceeded by creating the handle.
     */
    hdrPtr = (Fs_HandleHeader *)NIL;
    hashEntryPtr = Hash_LookOnly(fileHashTable, (Address) fileIDPtr);
    if (hashEntryPtr != (Hash_Entry *) NIL) {
	hdrPtr = (Fs_HandleHeader *) Hash_GetValue(hashEntryPtr);
	if (hdrPtr != (Fs_HandleHeader *)NIL) {
	    fs_Stats.handle.fetchHits++;
	    if (hdrPtr->flags & FS_HANDLE_LOCKED) {
		/*
		 * Wait for it to become unlocked and then rehash.
		 * The handle could get removed before we get a chance
		 * to increment the reference count on it.
		 */
		fs_Stats.handle.lockWaits++;
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
 * Fsutil_HandleFetchNoWait --
 *
 *	This routine does the same thing as Fsutil_HandleFetch except that
 *	it wouldn't wait around for a handle to because unlocked.
 *
 * Results:
 *	A pointer to a file handle, NIL if none found or handle locked.
 *
 * Side effects:
 *	Locks the handle and increments its reference count.  The handle
 *	is also moved to the end of the LRU list.
 *
 *----------------------------------------------------------------------------
 *
 */
ENTRY Fs_HandleHeader *
Fsutil_HandleFetchNoWait(fileIDPtr, wouldWaitPtr)
    Fs_FileID 	*fileIDPtr;	/* Identfies handle to fetch. */
    Boolean	*wouldWaitPtr;  /* OUT: Set to TRUE if call would of waited. */
{
    Hash_Entry	*hashEntryPtr;
    Fs_HandleHeader	*hdrPtr;

    LOCK_MONITOR;

    fs_Stats.handle.fetchCalls++;

    /*
     * Look in the hash table.  A bucket might have been installed by
     * Fsutil_HandleInstall, but the value might be NIL because the
     * handle table's size would have been exceeded by creating the handle.
     */
    *wouldWaitPtr = FALSE;
    hdrPtr = (Fs_HandleHeader *)NIL;
    hashEntryPtr = Hash_LookOnly(fileHashTable, (Address) fileIDPtr);
    if (hashEntryPtr != (Hash_Entry *) NIL) {
	hdrPtr = (Fs_HandleHeader *) Hash_GetValue(hashEntryPtr);
	if (hdrPtr != (Fs_HandleHeader *)NIL) {
	    fs_Stats.handle.fetchHits++;
	    if (hdrPtr->flags & FS_HANDLE_LOCKED) {
		*wouldWaitPtr = TRUE;
		UNLOCK_MONITOR;
		return (Fs_HandleHeader *) NIL;
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
 * Fsutil_HandleLockHdr --
 * Fsutil_HandleLock --
 *
 *	Get a lock on the handle.  Fsutil_HandleLock is a macro defined in fsInt.h
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
Fsutil_HandleLockHdr(hdrPtr)
    register	Fs_HandleHeader	*hdrPtr;
{
    LOCK_MONITOR;

    LOCK_HANDLE(hdrPtr);

    UNLOCK_MONITOR;
}

/*
 *----------------------------------------------------------------------------
 *
 * Fsutil_HandleIncRefCount --
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
Fsutil_HandleIncRefCount(hdrPtr, amount)
    register	Fs_HandleHeader	*hdrPtr;
    int 			amount;
{
    LOCK_MONITOR;

    hdrPtr->refCount += amount;

    UNLOCK_MONITOR;
}

/*
 *----------------------------------------------------------------------------
 *
 * Fsutil_HandleDecRefCount --
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
Fsutil_HandleDecRefCount(hdrPtr)
    register	Fs_HandleHeader	*hdrPtr;
{
    LOCK_MONITOR;

    hdrPtr->refCount--;

    UNLOCK_MONITOR;
}

/*
 *----------------------------------------------------------------------------
 *
 * Fsutil_HandleDup --
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
ENTRY Fs_HandleHeader *
Fsutil_HandleDup(hdrPtr)
    register	Fs_HandleHeader	*hdrPtr;
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
 * Fsutil_HandleValid --
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
Fsutil_HandleValid(hdrPtr)
    register Fs_HandleHeader *hdrPtr;	/* Handle to check. */
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
 * Fsutil_HandleUnlockHdr --
 * Fsutil_HandleUnlock --
 *
 *	Release the lock on the handle. Fsutil_HandleUnlock is a macro defined
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
Fsutil_HandleUnlockHdr(hdrPtr)
    register	Fs_HandleHeader	*hdrPtr;
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
 * Fsutil_HandleReleaseHdr --
 * Fsutil_HandleRelease --
 *
 *	Fsutil_HandleRelease is a macro that does type casting, see fsInt.h
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
Fsutil_HandleReleaseHdr(hdrPtr, locked)
    register Fs_HandleHeader *hdrPtr;  /* Header of handle to release. */
    Boolean	      locked;	   /* TRUE if the handle is already locked. */
{
    LOCK_MONITOR;
    fs_Stats.handle.release++;

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
		hdrPtr->refCount, Fsutil_FileTypeToString(hdrPtr->fileID.type),
		hdrPtr->fileID.serverID, hdrPtr->fileID.major,
		hdrPtr->fileID.minor, HDR_FILE_NAME(hdrPtr));
	return;
    } else if ((hdrPtr->refCount == 0) &&
	       (hdrPtr->flags & FS_HANDLE_REMOVED)) {
	/*
	 * The handle has been removed, and we are the last reference.
	 */
	fs_Stats.handle.limbo--;
	FSUTIL_TRACE_HANDLE(FSUTIL_TRACE_RELEASE_FREE, hdrPtr);
        REMOVE_HANDLE(hdrPtr);
     } else {
	FSUTIL_TRACE_HANDLE(FSUTIL_TRACE_RELEASE_LEAVE, hdrPtr);
	if (locked) {
	    UNLOCK_HANDLE(hdrPtr);
	}
    }
    UNLOCK_MONITOR;
}

/*
 *----------------------------------------------------------------------------
 *
 * Fsutil_HandleRemoveInt --
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
Fsutil_HandleRemoveInt(hdrPtr)
    register Fs_HandleHeader *hdrPtr;  /* Header of handle to remove. */
{
    register	Hash_Entry	*hashEntryPtr;

    if (!(hdrPtr->flags & FS_HANDLE_INVALID)) {
	hashEntryPtr = Hash_LookOnly(fileHashTable, (Address) &hdrPtr->fileID);
	if (hashEntryPtr == (Hash_Entry *) NIL) {
	    UNLOCK_MONITOR;
	    panic("Fsutil_HandleRemoveInt: Couldn't find handle in hash table.\n");
	    LOCK_MONITOR;
	    return;
	}
	Hash_SetValue(hashEntryPtr, NIL);
	Hash_Delete(fileHashTable, hashEntryPtr);
    }
    fs_Stats.handle.exists--;

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
	fs_Stats.handle.limbo++;
	FSUTIL_TRACE_HANDLE(FSUTIL_TRACE_REMOVE_LEAVE, hdrPtr);
	hdrPtr->flags |= FS_HANDLE_REMOVED;
    } else {
	FSUTIL_TRACE_HANDLE(FSUTIL_TRACE_REMOVE_FREE, hdrPtr);
	REMOVE_HANDLE(hdrPtr);
    }
}

/*
 *----------------------------------------------------------------------------
 *
 * Fsutil_HandleRemove --
 * Fsutil_HandleRemoveHdr --
 *
 *	Delete the handle from hash table by calling the internal routine.
 *	Fsutil_HandleRemove is a macro defined in fsInt.h that does type casting.
 *	Removing a handle deletes it from the hash table and unlocks it.
 *	Then, if there are no references to the handle it is freed.  Otherwise
 *	it is marked as deleted and Fsutil_HandleRelease cleans it up.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None (see Fsutil_HandleRemoveInt).
 *
 *----------------------------------------------------------------------------
 *
 */

ENTRY void
Fsutil_HandleRemoveHdr(hdrPtr)
    register Fs_HandleHeader *hdrPtr;	/* Handle to remove. */
{
    LOCK_MONITOR;
    Fsutil_HandleRemoveInt(hdrPtr);
    UNLOCK_MONITOR;
}

/*
 *----------------------------------------------------------------------------
 *
 * Fsutil_HandleAttemptRemove --
 *
 *	Like Fsutil_HandleRemove, but specific to local file handles because
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
Fsutil_HandleAttemptRemove(hdrPtr)
	Fs_HandleHeader *hdrPtr; /* Handle to try and remove. */
{
    register Fsio_FileIOHandle *handlePtr;	
    register Boolean removed;

    handlePtr = (Fsio_FileIOHandle *) hdrPtr;
    LOCK_MONITOR;
    if (handlePtr->hdr.refCount == 0) {
	free((Address)handlePtr->descPtr);
	handlePtr->descPtr = (Fsdm_FileDescriptor *)NIL;
	Fsio_FileSyncLockCleanup(handlePtr);
	Fsutil_HandleRemoveInt((Fs_HandleHeader *)handlePtr);
	removed = TRUE;
    } else {
	removed = FALSE;
	UNLOCK_HANDLE((Fs_HandleHeader *)handlePtr);
    }
    UNLOCK_MONITOR;
    return(removed);
}

/*
 *----------------------------------------------------------------------------
 *
 * Fsutil_GetNextHandle --
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

ENTRY Fs_HandleHeader *
Fsutil_GetNextHandle(hashSearchPtr)
    Hash_Search	*hashSearchPtr;	/* Iterator for going through the hash table. */
{
    register 	Fs_HandleHeader	*hdrPtr;
    register	Hash_Entry	*hashEntryPtr;

    LOCK_MONITOR;

    for (hashEntryPtr = Hash_Next(fileHashTable, hashSearchPtr);
         hashEntryPtr != (Hash_Entry *) NIL;  
	 hashEntryPtr = Hash_Next(fileHashTable, hashSearchPtr)) {
	hdrPtr = (Fs_HandleHeader *) Hash_GetValue(hashEntryPtr);
	if (hdrPtr == (Fs_HandleHeader *)NIL) {
	    /*
	     * Caught handle in the process of being installed.
	     */
	    continue;
	}
	/*
	 * Skip locked handles to avoid hanging the system on locked handle. 
	 */
	if (hdrPtr->flags & FS_HANDLE_LOCKED) {
	    continue;
	}
	if (hdrPtr->flags & (FS_HANDLE_INVALID|FS_HANDLE_REMOVED)) {
	    continue;
	}
	LOCK_HANDLE(hdrPtr);
	UNLOCK_MONITOR;
	return(hdrPtr);
    }

    UNLOCK_MONITOR;
    return((Fs_HandleHeader *) NIL);
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
ENTRY Fs_HandleHeader *
GetNextLRUHandle()
{
    register 	Fs_HandleHeader	*hdrPtr;
    register	List_Links	*listPtr;

    LOCK_MONITOR;
    if (lruHandlesChecked >= fs_Stats.handle.maxNumber) {
	hdrPtr = (Fs_HandleHeader *)NIL;
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
	if (lruHandlesChecked >= fs_Stats.handle.maxNumber) {
	    hdrPtr = (Fs_HandleHeader *)NIL;
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
	fs_Stats.handle.maxNumber += handleLimitInc;
	handleLimitInc = LIMIT_INC(fs_Stats.handle.maxNumber);
	handleScavengeThreashold = THREASHOLD(fs_Stats.handle.maxNumber);
    }
    lruInProgress = FALSE;
    Sync_Broadcast(&lruDone);
    UNLOCK_MONITOR;
}

/*
 *----------------------------------------------------------------------------
 *
 * Fsutil_HandleInvalidate --
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
Fsutil_HandleInvalidate(hdrPtr)
    Fs_HandleHeader *hdrPtr;
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
	    panic("Fsutil_HandleInvalidate: Can't find %s handle <%d,%d,%d>\n",
			Fsutil_FileTypeToString(hdrPtr->fileID.type),
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
 * Fsutil_HandleDescWriteBack --
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
Fsutil_HandleDescWriteBack(shutdown, domain)
    Boolean	shutdown;	/* TRUE if the kernel is being shutdowned. */
    int		domain;		/* Domain number, -1 means all local domains */
{
    Hash_Search			hashSearch;
    register Fsio_FileIOHandle *handlePtr;
    register Fs_HandleHeader	*hdrPtr;
    register Hash_Entry		*hashEntryPtr;
    int				lockedDesc = 0;

    LOCK_MONITOR;

    Hash_StartSearch(&hashSearch);

    for (hashEntryPtr = Hash_Next(fileHashTable, &hashSearch);
         hashEntryPtr != (Hash_Entry *) NIL;  
	 hashEntryPtr = Hash_Next(fileHashTable, &hashSearch)) {
	hdrPtr = (Fs_HandleHeader *) Hash_GetValue(hashEntryPtr);
	if (hdrPtr == (Fs_HandleHeader *)NIL) {
	    /*
	     * Handle has been removed.
	     */
	    continue;
	}
	if (hdrPtr->fileID.type != FSIO_LCL_FILE_STREAM) {
	    continue;
	}
	if (domain >= 0 && hdrPtr->fileID.major != domain) {
	    continue;
	}
	if (hdrPtr->flags & FS_HANDLE_LOCKED) {
	    lockedDesc++;
	    continue;
	}
	handlePtr = (Fsio_FileIOHandle *)hdrPtr;
	if (handlePtr->descPtr == (Fsdm_FileDescriptor *)NIL) {
	    printf("Fsutil_HandleDescWriteBack, no descPtr for <%d,%d> \"%s\"\n",
		hdrPtr->fileID.major, hdrPtr->fileID.minor,
		Fsutil_HandleName(hdrPtr));
	    continue;
	}
	(void)Fsdm_FileDescWriteBack(handlePtr, FALSE);
    }

    if (shutdown & lockedDesc) {
	printf("Fsutil_HandleDescWriteBack: %d descriptors still locked\n",
		    lockedDesc);
    }

    UNLOCK_MONITOR;

    return(lockedDesc);
}

