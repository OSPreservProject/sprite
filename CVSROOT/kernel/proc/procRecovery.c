/* 
 * procRecovery.c
 *
 *	Routines for process migration recovery.
 *
 * Copyright 1988 Regents of the University of California.
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
#include "mem.h"

static void MigrateCrash();
static void MigHostIsUp();

static Sync_Lock recovLock = {0,0};
static Sync_Condition recovCondition = {0};
#define LOCKPTR &recovLock

typedef struct DependInfo {
    Proc_PID	processID;	/* the process ID on our host, and the
				 * key into the hash table */
    int		hostID;		/* the other host for the process
				 * (home or remote) */
} DependInfo;

/*
 * Hash tables for dependency information.
 */

Hash_Table	dependHashTableStruct;
Hash_Table	*dependHashTable = &dependHashTableStruct;

#define KERNEL_HASH


/*
 *----------------------------------------------------------------------
 *
 * Proc_RecovInit --
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
Proc_RecovInit()
{
    Hash_Init(dependHashTable, 0, HASH_ONE_WORD_KEYS);
    Recov_CrashRegister(MigrateCrash, (ClientData) NIL);
}


/*
 *----------------------------------------------------------------------------
 *
 * MigHostIsUp --
 *
 *	Wakeup processes waiting for a host to come back or reboot.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Processes waiting for process migration dependencies on any host
 *	are awakened. 
 *
 *----------------------------------------------------------------------------
 *
 */
/* ARGSUSED */
static ENTRY void
MigHostIsUp(hostID, data)
    int hostID;
    ClientData data;
{
    LOCK_MONITOR;
    Sync_Broadcast(&recovCondition);
    UNLOCK_MONITOR;
}

/*
 *----------------------------------------------------------------------
 *
 * MigrateCrash --
 *
 *	Kill off any migrated processes associated with a host that has
 *	crashed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static ENTRY void
MigrateCrash(hostID, clientData)
    int hostID;		/* Host that has rebooted */
    ClientData clientData;	/* IGNORED */
{
    Hash_Search			hashSearch;
    register Hash_Entry		*hashEntryPtr;
    DependInfo			*dependPtr;

    LOCK_MONITOR;
    Sync_Broadcast(&recovCondition);
    Hash_StartSearch(&hashSearch);
    for (hashEntryPtr = Hash_Next(dependHashTable, &hashSearch);
         hashEntryPtr != (Hash_Entry *) NIL;  
	 hashEntryPtr = Hash_Next(dependHashTable, &hashSearch)) {
	dependPtr = (DependInfo *) Hash_GetValue(hashEntryPtr);
	if (dependPtr->hostID == hostID) {
	    Proc_CallFunc(Proc_DestroyMigratedProc,
			  (ClientData) dependPtr->processID, 0);
	}
    }
    UNLOCK_MONITOR;
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_AddMigDependency --
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
Proc_AddMigDependency(processID, hostID)
    Proc_PID processID;		/* process that depends on the host */
    int hostID;			/* host to watch */
{
    DependInfo *dependPtr;
    Hash_Entry *hashEntryPtr;
    Boolean new;
    
    LOCK_MONITOR;

    dependPtr = Mem_New(DependInfo);

    dependPtr->processID = processID;
    dependPtr->hostID = hostID;
#ifdef KERNEL_HASH
    hashEntryPtr = Hash_Find(dependHashTable, (Address) processID);
    new = TRUE;
#else KERNEL_HASH
    hashEntryPtr = Hash_CreateEntry(dependHashTable, (Address) processID, &new);
#endif /* KERNEL_HASH */
    if (!new) {
	if (proc_MigDebugLevel > 0) {
	    Sys_Panic((proc_MigDebugLevel > 4) ? SYS_FATAL : SYS_WARNING,
		      "Proc_AddMigDependency: process %x already registered.\n",
		      processID);
	}
	UNLOCK_MONITOR;
	return;
    }
    Hash_SetValue(hashEntryPtr, (ClientData) dependPtr);
    Recov_RebootRegister(hostID, MigHostIsUp, (ClientData) NIL);
    UNLOCK_MONITOR;
}



/*
 *----------------------------------------------------------------------
 *
 * Proc_RemoveMigDependency --
 *
 *	Remove a process from the table of dependencies on remote machines.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The corresponding entry for the process is removed from the
 *	hash table and freed.
 *
 *----------------------------------------------------------------------
 */
ENTRY void
Proc_RemoveMigDependency(processID)
    Proc_PID processID;		/* process to remove */
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
	if (proc_MigDebugLevel > 0) {
	    Sys_Panic((proc_MigDebugLevel > 4) ? SYS_FATAL : SYS_WARNING,
		      "Proc_RemoveMigDependency: process %x not registered.\n",
		      processID);
	}
	UNLOCK_MONITOR;
	return;
    }
    dependPtr = (DependInfo *) Hash_GetValue(hashEntryPtr);
#ifdef KERNEL_HASH
    Hash_Delete(dependHashTable, hashEntryPtr);
#else KERNEL_HASH
    Hash_DeleteEntry(dependHashTable, hashEntryPtr);
#endif /* KERNEL_HASH */
    Mem_Free ((Address) dependPtr);
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
