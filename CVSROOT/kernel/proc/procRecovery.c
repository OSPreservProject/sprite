/* 
 * procRecovery.c
 *
 *	Routines for process migration recovery.
 *
 * Copyright 1988, 1989, 1990 Regents of the University of California.
 * All rights reserved.
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
#endif /* not lint */

#include "sprite.h"
#include "proc.h"
#include "procMigrate.h"
#include "migrate.h"
#include "recov.h"
#include "sync.h"
#include "rpc.h"
#include "hash.h"
#include "stdlib.h"

static void HostChanged();

static Sync_Lock recovLock;
Sync_Condition recovCondition = {0};
#define LOCKPTR &recovLock

typedef struct DependInfo {
    Proc_PID	processID;	/* the process ID on our host, and the
				 * key into the hash table */
    Proc_PID	peerProcessID;	/* the process ID on the other host. */
    int		hostID;		/* the other host for the process
				 * (home or remote) */
    int		flags;		/* Flags (see below) */
} DependInfo;

/*
 * Define constants for use with migration-recovery interaction.
 *
 *	DEPEND_UNREACHABLE	- host was unable to receive death notice.
 */
#define DEPEND_UNREACHABLE 1

/*
 * Flags for HostChanged.
 *	HOST_CRASHED 	- a host is down, or rebooted.
 *	HOST_UP		- host is reachable.
 */
#define HOST_CRASHED 0
#define HOST_UP 1

/*
 * Hash tables for dependency information.
 */

Hash_Table	dependHashTableStruct;
Hash_Table	*dependHashTable = &dependHashTableStruct;

/*
 * Define a structure for keeping track of dependencies while doing
 * a hash search -- we store the key in a list and do individual hash
 * lookups once the search is over.
 */
typedef struct {
    List_Links links;		/* Pointers within list. */
    Proc_PID processID;		/* Key. */
} DependChain;

#define KERNEL_HASH


/*
 *----------------------------------------------------------------------
 *
 * ProcRecovInit --
 *
 *	Initialize the data structures for process migration recovery.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Hash table is initialized and the recovery system is told to
 *	alert us when it determines that a host has crashed.
 *
 *----------------------------------------------------------------------
 */
void
ProcRecovInit()
{
    Hash_Init(dependHashTable, 0, HASH_ONE_WORD_KEYS);
    Recov_CrashRegister(HostChanged, (ClientData) HOST_CRASHED);
    Sync_LockInitDynamic(&recovLock, "Proc:recovLock");
}


/*
 *----------------------------------------------------------------------
 *
 * HostChanged --
 *
 *	Kill off any migrated processes associated with a host that has
 *	crashed.  Notify anyone waiting for a change in host.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static ENTRY void
HostChanged(hostID, clientData)
    int hostID;		/* Host that has rebooted */
    ClientData clientData;	/* Whether the host crashed. */
{
    Hash_Search			hashSearch;
    register Hash_Entry		*hashEntryPtr;
    DependInfo			*dependPtr;
    List_Links			dependList;
    DependChain			*chainPtr;
    Boolean			crashed;

    LOCK_MONITOR;

    crashed = ((int) clientData) == HOST_CRASHED;
    
    Sync_Broadcast(&recovCondition);
    Hash_StartSearch(&hashSearch);
    List_Init(&dependList);
    for (hashEntryPtr = Hash_Next(dependHashTable, &hashSearch);
         hashEntryPtr != (Hash_Entry *) NIL;  
	 hashEntryPtr = Hash_Next(dependHashTable, &hashSearch)) {
	dependPtr = (DependInfo *) Hash_GetValue(hashEntryPtr);
	if (dependPtr->hostID == hostID) {
	    if (crashed && !(dependPtr->flags & DEPEND_UNREACHABLE)) {
		/*
		 * This process still exists on our host.
		 */
		Proc_CallFunc(Proc_DestroyMigratedProc,
			      (ClientData) dependPtr->processID, 0);
	    } else if (crashed || (dependPtr->flags & DEPEND_UNREACHABLE)){
		/*
		 * Either the host crashed but we already killed the
		 * process and just want to get rid of the hash entry,
		 * or it has come back and we want to notify it about
		 * a dead process.
		 */
		chainPtr = (DependChain *) malloc(sizeof(DependChain));
		chainPtr->processID = dependPtr->processID;
		List_InitElement(&chainPtr->links);
		List_Insert(&chainPtr->links, LIST_ATREAR(&dependList));
		if (!crashed) {
		    Proc_CallFunc(ProcMigKillRemoteCopy,
				  (ClientData) dependPtr->peerProcessID, 0);
		}
		    
	    }  
	}
    }
    /*
     * Now clean up the list of dependencies to do Hash_Deletes.
     */
    while (!List_IsEmpty(&dependList)) {
	chainPtr = (DependChain *) List_First(&dependList);
	List_Remove((List_Links *) chainPtr);
#ifdef KERNEL_HASH
	hashEntryPtr = Hash_LookOnly(dependHashTable,
				     (Address) chainPtr->processID);
	if (hashEntryPtr != (Hash_Entry *) NIL) {
#else KERNEL_HASH
        hashEntryPtr = Hash_FindEntry(dependHashTable,
				      (Address) chainPtr->processID);
	if (hashEntryPtr != (Hash_Entry *) NULL) {
#endif				/* KERNEL_HASH */
	    dependPtr = (DependInfo *) Hash_GetValue(hashEntryPtr);
#ifdef KERNEL_HASH
	    Hash_Delete(dependHashTable, hashEntryPtr);
#else KERNEL_HASH
	    Hash_DeleteEntry(dependHashTable, hashEntryPtr);
#endif				/* KERNEL_HASH */
	    Recov_RebootUnRegister(dependPtr->hostID, HostChanged,
				   clientData);
	    free ((Address) dependPtr);
	}
	free((char *) chainPtr);
    }
    UNLOCK_MONITOR;
}


/*
 *----------------------------------------------------------------------
 *
 * ProcMigAddDependency --
 *
 *	Note a dependency of a process on a remote machine.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory is allocated for a structure to be put in the hash table.
 *
 *----------------------------------------------------------------------
 */
ENTRY void
ProcMigAddDependency(processID, peerProcessID)
    Proc_PID processID;		/* process that depends on the host */
    Proc_PID peerProcessID;	/* process on the other host */
{
    DependInfo *dependPtr;
    Hash_Entry *hashEntryPtr;
    Boolean new;
    
    LOCK_MONITOR;


    dependPtr = (DependInfo *) malloc(sizeof (DependInfo));

    dependPtr->processID = processID;
    dependPtr->peerProcessID = peerProcessID;
    dependPtr->hostID = Proc_GetHostID(peerProcessID);
    dependPtr->flags = 0;
#ifdef KERNEL_HASH
    hashEntryPtr = Hash_Find(dependHashTable, (Address) processID);
    new = (hashEntryPtr->value == (Address) NIL);
#else KERNEL_HASH
    hashEntryPtr = Hash_CreateEntry(dependHashTable, (Address) processID, &new);
#endif /* KERNEL_HASH */
    if (!new) {
	if (proc_MigDebugLevel > 0) {
	    if (proc_MigDebugLevel > 4) {
		panic("ProcMigAddDependency: process %x already registered.\n",
		      processID);
	    } else {
		printf(
		"%s ProcMigAddDependency: process %x already registered.\n",
		      "Warning:", processID);
	    }
	}
	UNLOCK_MONITOR;
	return;
    }
    Hash_SetValue(hashEntryPtr, (ClientData) dependPtr);
    Recov_RebootRegister(dependPtr->hostID, HostChanged, (ClientData) HOST_UP);
    UNLOCK_MONITOR;
}



/*
 *----------------------------------------------------------------------
 *
 * ProcMigRemoveDependency --
 *
 *	Remove a process from the table of dependencies on remote machines.
 *	If the other host wasn't notified, defer the removal until the other
 * 	host is known to be down or rebooted, and notify the other host
 *	of the death of the process if it should come back.  After this
 *	routine is called, the routines in this file are responsible for
 *	eventually getting rid of the dependency (e.g., the caller
 * 	won't call again).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The corresponding entry for the process is removed from the
 *	hash table and freed, or flagged for future removal.
 *
 *----------------------------------------------------------------------
 */
ENTRY void
ProcMigRemoveDependency(processID, notified)
    Proc_PID processID;		/* process to remove */
    Boolean notified;		/* Whether other host was notified of death. */
{
    DependInfo *dependPtr;
    Hash_Entry *hashEntryPtr;
    
    LOCK_MONITOR;

#ifdef KERNEL_HASH
    hashEntryPtr = Hash_LookOnly(dependHashTable, (Address) processID);
    if (hashEntryPtr == (Hash_Entry *) NIL) {
#else KERNEL_HASH
    hashEntryPtr = Hash_FindEntry(dependHashTable, (Address) processID);
    if (hashEntryPtr == (Hash_Entry *) NULL) {
#endif /* KERNEL_HASH */
#ifdef notdef
	/*
	 * (We're not going to care if it's already been removed because we
	 * are called to get rid of processIDs that may already have been
	 * removed, and we're just making doubly sure.)
	 */
	if (proc_MigDebugLevel > 0) {
	    if (proc_MigDebugLevel > 4) {
		panic("ProcMigRemoveDependency: process %x not registered.\n",
		      processID);
	    } else {
		printf(
		    "%s ProcMigRemoveDependency: process %x not registered.\n",
		    "Warning:", processID);
	    }
	}
#endif /* notdef */
	UNLOCK_MONITOR;
	return;
    }
    dependPtr = (DependInfo *) Hash_GetValue(hashEntryPtr);
    if (!notified) {
	dependPtr->flags |= DEPEND_UNREACHABLE;
	UNLOCK_MONITOR;
	return;
    }
#ifdef KERNEL_HASH
    Hash_Delete(dependHashTable, hashEntryPtr);
#else KERNEL_HASH
    Hash_DeleteEntry(dependHashTable, hashEntryPtr);
#endif /* KERNEL_HASH */
    Recov_RebootUnRegister(dependPtr->hostID, HostChanged,
			   (ClientData) HOST_UP);
    free ((Address) dependPtr);
    UNLOCK_MONITOR;
}




/*
 *----------------------------------------------------------------------
 *
 * Proc_WaitForHost --
 *
 *	Wait until a host has fully crashed, come back, or rebooted.
 *	This will return a failure code if the host crashed.
 *
 * Results:
 *	SUCCESS - the host is useable again.
 *	NET_UNREACHABLE_HOST - the host crashed.
 *	GEN_ABORTED_BY_SIGNAL - a signal was received while waiting.
 *
 * Side effects:
 *	Block the process if waiting for the host to return or crash.
 *
 *----------------------------------------------------------------------
 */
ENTRY ReturnStatus
Proc_WaitForHost(hostID)
    int hostID;
{
    register ReturnStatus status = SUCCESS;
    int hostState;

    LOCK_MONITOR;
    hostState = Recov_GetClientState(hostID);
    while (hostState & RECOV_HOST_DYING) {
	if (Sync_Wait(&recovCondition, TRUE)) {
	    status = GEN_ABORTED_BY_SIGNAL;
	    break;
	}
	hostState = Recov_GetClientState(hostID);
    }
    if (status == SUCCESS) {
	if (hostState & (RECOV_HOST_DEAD | RECOV_HOST_BOOTING)) {
	    /*
	     * Host has crashed and is either out of touch or rebooting now.
	     * Cause migrated clients to get killed.
	     */
	    status = NET_UNREACHABLE_HOST;
	} else if (hostState & RECOV_HOST_ALIVE) {
	    /*
	     * Host is back, so continue business as usual.
	     */
	    status = SUCCESS;
	}
    }
    UNLOCK_MONITOR;
    return(status);
}
