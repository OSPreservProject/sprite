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

void Proc_MigrateCrash();
#ifdef notdef
void Proc_HostIsUp();
#endif /* */

static Sync_Lock recovLock = {0,0};
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
 *	alert us about reboots (to kill associated processes) and
 *	when a host is "back" after being unreachable.  [not implemented]
 *
 *----------------------------------------------------------------------
 */
void
Proc_RecovInit()
{
    Hash_Init(dependHashTable, 0, HASH_ONE_WORD_KEYS);
    Recov_CrashRegister(Proc_MigrateCrash, (ClientData) NIL);
#ifdef notdef
    Recov_BackRegister(Proc_MigrateHostIsUp, (ClientData) NIL);
#endif /* */
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_MigrateCrash --
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
ENTRY void
Proc_MigrateCrash(hostID, clientData)
    int hostID;		/* Host that has rebooted */
    ClientData clientData;	/* IGNORED */
{
    Hash_Search			hashSearch;
    register Hash_Entry		*hashEntryPtr;
    DependInfo			*dependPtr;

    LOCK_MONITOR;
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
	return;
    }
    Hash_SetValue(hashEntryPtr, (ClientData) dependPtr);
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



