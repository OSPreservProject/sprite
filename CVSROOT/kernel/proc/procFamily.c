/*
 *  procFamily.c --
 *
 *	Routines to manage process families.  Families are implemented using
 *	a hash table each entry of which points to a process family list.
 *	This hash table and process families in general are managed by a 
 *	monitor lock.
 *
 *	The only field in the proc table managed by routines in this monitor
 *	is the familyID field.
 *
 * Copyright (C) 1986 Regents of the University of California
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
#include "mem.h"
#include "status.h"
#include "sync.h"
#include "sched.h"
#include "hash.h"

static	Sync_Lock	familyLock = SYNC_LOCK_INIT_STATIC();
#define	LOCKPTR &familyLock
Sync_Condition	familyCondition;

static Hash_Table	famHashTableStruct;
static Hash_Table	*famHashTable = &famHashTableStruct;

#define	FAMILY_HASH_SIZE	8

/*
 * A family list header.
 */

typedef struct {
    List_Links		famList;	/* Pointer to family list. */
    Boolean		locked;		/* TRUE => family locked. */
    Sync_Condition	condition;	/* Condition to wait on when family
					 * locked. */
    int			userID;		/* Effective userid of process that
					 * created the family. */
} FamilyHeader;


/*
 *----------------------------------------------------------------------
 *
 * ProcFamilyHashInit --
 *
 *	Initialize the family id hash table.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Family id hash table initialized.
 *
 *----------------------------------------------------------------------
 */

void
ProcFamilyHashInit()
{
    Hash_Init(famHashTable, FAMILY_HASH_SIZE, Hash_Size(sizeof(int)));
}


/*
 *----------------------------------------------------------------------
 *
 * ProcFamilyInsert --
 *
 *	Insert the process into the given family.
 *
 * Results:
 *	PROC_UID_MISMATCH if effective userid of process does not match
 *	userid of family.  SUCCESS otherwise.
 *
 * Side effects:
 *	Process inserted into family list.  If family doesn't exist then
 *	the family is created with the userid of the given process.
 *
 *----------------------------------------------------------------------
 */

ENTRY ReturnStatus
ProcFamilyInsert(procPtr, familyID)
    Proc_ControlBlock	*procPtr;
    int			familyID;
{
    register	Hash_Entry	*hashEntryPtr;
    register	FamilyHeader	*famHdrPtr;

    LOCK_MONITOR;
    if (familyID == PROC_NO_FAMILY) {
	UNLOCK_MONITOR;
	return(SUCCESS);
    }

again:
    hashEntryPtr = Hash_Find(famHashTable, (Address) familyID);
    famHdrPtr = (FamilyHeader *) Hash_GetValue(hashEntryPtr);
    if (famHdrPtr == (FamilyHeader *) NIL) {
	famHdrPtr = (FamilyHeader *) malloc(sizeof(FamilyHeader));
	List_Init(&famHdrPtr->famList);
	Hash_SetValue(hashEntryPtr, famHdrPtr);
	famHdrPtr->locked = FALSE;
	famHdrPtr->userID = procPtr->effectiveUserID;
    } else {
	if (famHdrPtr->locked) {
	    (void) Sync_Wait(&famHdrPtr->condition, FALSE);
	    goto again;
	}
#ifdef CHECK_PROT
	if (famHdrPtr->userID != procPtr->effectiveUserID &&
	        procPtr->effectiveUserID != PROC_SUPER_USER_ID) {
	    printf("Uid-mismatch: pid %x puid %d fid %x fuid %d\n",
		    procPtr->processID, procPtr->effectiveUserID,
		    familyID, famHdrPtr->userID);
	    UNLOCK_MONITOR;
	    return(PROC_UID_MISMATCH);
	}
#endif /* */
    }
    List_Insert((List_Links *) &(procPtr->familyElement), 
		LIST_ATFRONT(&famHdrPtr->famList));

    UNLOCK_MONITOR;
    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * ProcFamilyRemove --
 *
 *	Remove the process from the given family list.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Process removed from its family list.  If process isn't in a family
 *	then nothing happens.
 *
 *----------------------------------------------------------------------
 */

ENTRY void
ProcFamilyRemove(procPtr)
    Proc_ControlBlock	*procPtr;
{
    register	Hash_Entry	*hashEntryPtr;
    register	FamilyHeader	*famHdrPtr;

    LOCK_MONITOR;

    if (procPtr->familyID == PROC_NO_FAMILY) {
	UNLOCK_MONITOR;
	return;
    }

    hashEntryPtr = Hash_LookOnly(famHashTable, (Address) procPtr->familyID);
    if (hashEntryPtr == (Hash_Entry *) NIL) {
	panic("ProcFamilyRemove: Family not in hash table\n");
    }
    famHdrPtr = (FamilyHeader *) Hash_GetValue(hashEntryPtr);
    while (famHdrPtr->locked) {
	(void) Sync_Wait(&famHdrPtr->condition, FALSE);
    }

    List_Remove((List_Links *) &(procPtr->familyElement));
    if (List_IsEmpty(&famHdrPtr->famList)) {
	free((Address) famHdrPtr);
	Hash_Delete(famHashTable, hashEntryPtr);
    }

    UNLOCK_MONITOR;
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_LockFamily --
 *
 *	Prevent the given family from being modified.
 *
 * Results:
 *	PROC_INVALID_FAMILYID 	- The pid argument was illegal.
 *	PROC_UID_MISMATCH	- The callers uid doesn't match the uid of the
 *				  family.
 *
 * Side effects:
 *	Process family lock flag set.
 *
 *----------------------------------------------------------------------
 */

ENTRY ReturnStatus
Proc_LockFamily(familyID, familyListPtr, userIDPtr)
    int		familyID;		/* Family to lock. */
    List_Links	**familyListPtr;	/* Where to return pointer to family
					 * list. */
    int		*userIDPtr;		/* Place to return the user id of the
					 * family. */
{
    register	Hash_Entry	*hashEntryPtr;
    register	FamilyHeader	*famHdrPtr;

    LOCK_MONITOR;

again:
    hashEntryPtr = Hash_LookOnly(famHashTable, (Address) familyID);
    if (hashEntryPtr == (Hash_Entry *) NIL) {
	UNLOCK_MONITOR;
	return(PROC_INVALID_FAMILY_ID);
    }
    famHdrPtr = (FamilyHeader *) Hash_GetValue(hashEntryPtr);

    if (famHdrPtr->locked) {
	/*
	 * The family is locked so wait for it to become unlocked.  When
	 * wake up start over because the family could have been deleted while
	 * we were sleeping.
	 */
	(void) Sync_Wait(&famHdrPtr->condition, FALSE);
	goto again;
    }
    famHdrPtr->locked = TRUE;
    *familyListPtr = &(famHdrPtr->famList);
    *userIDPtr = famHdrPtr->userID;

    UNLOCK_MONITOR;

    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_UnlockFamily --
 *
 *	Allow the family to be modified.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Family id header element lock bit cleared.
 *
 *----------------------------------------------------------------------
 */

ENTRY void
Proc_UnlockFamily(familyID)
    int	familyID;
{
    register	Hash_Entry	*hashEntryPtr;
    register	FamilyHeader	*famHdrPtr;

    LOCK_MONITOR;

    hashEntryPtr = Hash_LookOnly(famHashTable, (Address) familyID);
    if (hashEntryPtr == (Hash_Entry *) NIL) {
	panic("Proc_UnlockFamily: Family doesn't exist\n");
    }
    famHdrPtr = (FamilyHeader *) Hash_GetValue(hashEntryPtr);

    if (!famHdrPtr->locked) {
	panic("Proc_UnlockFamily: Family isn't locked\n");
    }

    famHdrPtr->locked = FALSE;
    Sync_Broadcast(&famHdrPtr->condition);

    UNLOCK_MONITOR;
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_GetFamilyID --
 *
 *	Returns the ID of the head of the process family
 *	for the given process.
 *
 * Results:
 *	SYS_ARG_NOACCESS - 	the argument was not accessible.
 *	SYS_INVALID_ARG - 	the argument was was invalid.
 *	PROC_INVALID_PID -	the pid argument was illegal.
 *	PROC_UID_MISMATCH -	the calling process does not have permission
 *				to read the given process's family id.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Proc_GetFamilyID(pid, familyIDPtr)
    Proc_PID	pid;
    Proc_PID	*familyIDPtr;
{
    register	Proc_ControlBlock 	*procPtr;
    Proc_PID				familyID;

    if (familyIDPtr == USER_NIL) {
	return(SYS_INVALID_ARG);
    } 

    if (pid == PROC_MY_PID) {
	procPtr = Proc_GetEffectiveProc();
	Proc_Lock(procPtr);
    } else {
	/*
	 *   Get the PCB entry for the given process.
	 */
	procPtr = Proc_LockPID(pid);
	if (procPtr == (Proc_ControlBlock *) NIL) {
	    return(PROC_INVALID_PID);
	}
	if (!Proc_HasPermission(procPtr->effectiveUserID)) {
	    Proc_Unlock(procPtr);
	    return(PROC_UID_MISMATCH);
	}
    }

    familyID = procPtr->familyID;
    Proc_Unlock(procPtr);
    if (Proc_ByteCopy(FALSE, sizeof(Proc_PID), (Address) &familyID, 
		      (Address) familyIDPtr) != SUCCESS){
	return(SYS_ARG_NOACCESS);
    }
    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_SetFamilyID --
 *
 *	Changes the family ID of a process to the one specified.
 *
 *
 * Results:
 *	PROC_UID_MISMATCH - 	The current process did not have the same
 *				uid as the family or the calling process
 *				does not have permission to set the given
 *				process's family id.
 *	PROC_INVALID_FAMILY_ID  The given family ID is not a valid process ID.
 *
 * Side effects:
 *	Changes the family ID field in the Proc_ControlBlock for a process.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Proc_SetFamilyID(pid, familyID)
    Proc_PID	pid;
    Proc_PID	familyID;
{
    register Proc_ControlBlock	*procPtr;
    Proc_ControlBlock		*famProcPtr;
    ReturnStatus		status;

    /*
     * Make sure that the familyID is a valid process ID.  This is necessary
     * to let process group signals to be sent across machines.
     */
    famProcPtr = Proc_LockPID(familyID);
    if (famProcPtr == (Proc_ControlBlock *) NIL) {
	return(PROC_INVALID_FAMILY_ID);
    }
    Proc_Unlock(famProcPtr);

    if (pid == PROC_MY_PID) {
	procPtr = Proc_GetEffectiveProc();
	Proc_Lock(procPtr);
    } else {
	/*
	 *   Get the PCB entry for the given process.
	 */
	procPtr = Proc_LockPID(pid);
	if (procPtr == (Proc_ControlBlock *) NIL) {
	    return(PROC_INVALID_PID);
	}
	if (!Proc_HasPermission(procPtr->effectiveUserID)) {
	    Proc_Unlock(procPtr);
	    return(PROC_UID_MISMATCH);
	}
    }

    ProcFamilyRemove(procPtr);
    status = ProcFamilyInsert(procPtr, (int) familyID);
    if (status == SUCCESS) {
	procPtr->familyID = familyID;
    } else {
	procPtr->familyID = PROC_NO_FAMILY;
    }

    if (procPtr->state == PROC_MIGRATED) {
	status = Proc_MigSendUserInfo(procPtr);
    }

    Proc_Unlock(procPtr);

    return(status);
}
