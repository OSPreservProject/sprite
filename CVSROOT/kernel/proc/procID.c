/*
 *  procID.c --
 *
 *	Routines to get and set the various identifiers of a process.
 *	The routines implement the system calls of the same name. 
 *	Synchronization to process table entries is done by locking the
 *	process's PCB.
 *
 * Copyright (C) 1986, 1988 Regents of the University of California
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
#include "byte.h"
#include "vm.h"

/*
 * Define a macro to get the minimum of two values.  Note: it is *not*
 * side-effect free.  If "a" or "b" is a function call, the function will
 * be called twice.  Of course, there shouldn't be side-effects in
 * conditional expressions.
 */

#define Min(a,b) ((a) < (b) ? (a) : (b))


/*
 *----------------------------------------------------------------------
 *
 * Proc_GetIDs --
 *
 *	Returns the process ID, user ID and effective user ID of the current
 *	process.
 *
 * Results:
 *	SYS_ARG_NOACCESS - 	the arguments were not accessible.
 *	PROC_INVALID_PID -	the pid argument was illegal.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Proc_GetIDs(procIDPtr, parentIDPtr, userIDPtr, effUserIDPtr)
    Proc_PID 	*procIDPtr;	/* Where to return pid. */
    Proc_PID 	*parentIDPtr;	/* Where to return parent's pid */
    int 	*userIDPtr;	/* Where to return real user id. */
    int 	*effUserIDPtr;	/* Where to return effective user id. */
{
    register Proc_ControlBlock 	*procPtr;

    procPtr = Proc_GetEffectiveProc();

    /*
     *  Copy the information to the out parameters.
     */

    if (procIDPtr != USER_NIL) {
	if (Proc_ByteCopy(FALSE, sizeof(Proc_PID), 
	    (Address) &(procPtr->processID), (Address) procIDPtr) != SUCCESS){
	    return(SYS_ARG_NOACCESS);
	}
    }

    if (parentIDPtr != USER_NIL) {
	if (Proc_ByteCopy(FALSE, sizeof(Proc_PID), 
	    (Address) &(procPtr->parentID), (Address) parentIDPtr) != SUCCESS){
	    return(SYS_ARG_NOACCESS);
	}
    }

    if (userIDPtr != USER_NIL) {
	if (Proc_ByteCopy(FALSE, sizeof(int), 
	    (Address) &(procPtr->userID), (Address) userIDPtr) != SUCCESS){
	    return(SYS_ARG_NOACCESS);
	}
    }

    if (effUserIDPtr != USER_NIL) {
	if (Proc_ByteCopy(FALSE, sizeof(int), 
	    (Address) &(procPtr->effectiveUserID), 
	    (Address) effUserIDPtr) != SUCCESS){
	    return(SYS_ARG_NOACCESS);
	}
    }
    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_SetIDs --
 *
 *	Changes the user ID or current user ID for the current process.
 *  	If an argument is USER_NIL, the corresponding value in
 *	the PCB structure is not changed.
 *
 * Results:
 *	SYS_ARG_NOACCESS - 	the arguments were not accessible.
 *	PROC_INVALID_PID -	the pid argument was illegal.
 *
 * Side effects:
 *	The user ID and/or effective user ID for a process may change.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Proc_SetIDs(userID, effUserID)
    int 	userID;
    int 	effUserID;
{
    register	Proc_ControlBlock 	*procPtr;

    procPtr = Proc_GetEffectiveProc();

    if (userID == PROC_NO_ID) {
	userID = procPtr->userID;;
    }
    if (effUserID == PROC_NO_ID) {
	effUserID = procPtr->effectiveUserID;
    }
    if (userID != procPtr->userID && userID != procPtr->effectiveUserID &&
	procPtr->effectiveUserID != PROC_SUPER_USER_ID) {
       return(PROC_UID_MISMATCH);
    }
    if (effUserID != procPtr->userID && effUserID != procPtr->effectiveUserID &&
	procPtr->effectiveUserID != PROC_SUPER_USER_ID) {
       return(PROC_UID_MISMATCH);
    }
    procPtr->userID = userID;
    procPtr->effectiveUserID = effUserID;

    if (procPtr->state == PROC_MIGRATED) {
	return(Proc_MigSendUserInfo(procPtr));
    }
    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_GetGroupIDs --
 *
 *	Returns all the group IDs of a process if the gidArrayPtr
 *	argument is not USER_NIL. Also returns the actual number of
 *	groups IDs in the process's PCB structure if trueNumGidsPtr
 *	is not USER_NIL.
 *
 *	TODO: Move me to fs.
 *
 * Results:
 *	SYS_ARG_NOACCESS - 	the arguments were not accessible.
 *	SYS_INVALID_ARG - 	the argument was was invalid.
 *	PROC_INVALID_PID -	the pid argument was illegal.
 *	The group IDs are returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Proc_GetGroupIDs(numGIDs, gidArrayPtr, trueNumGIDsPtr)
    int		numGIDs;		/* Number of group ids in gidArrayPtr.*/
    int		*gidArrayPtr;		/* Array of group ids. */
    int		*trueNumGIDsPtr;	/* Number of group ids actually 
					 * returned. */
{
    register	Fs_ProcessState 	*fsPtr;
    int 				trueNumGIDs;

    fsPtr = (Proc_GetEffectiveProc())->fsPtr;

    if (numGIDs < 0) {
	return(SYS_INVALID_ARG);
    }
    trueNumGIDs = Min(numGIDs, fsPtr->numGroupIDs);
    if (trueNumGIDs > 0 && gidArrayPtr != USER_NIL) {
	if (Proc_ByteCopy(FALSE, trueNumGIDs * sizeof(int),
			  (Address) fsPtr->groupIDs,
			  (Address) gidArrayPtr) != SUCCESS) {
	    return(SYS_ARG_NOACCESS);
	}
    }

    if (trueNumGIDsPtr != USER_NIL) {
	if (Proc_ByteCopy(FALSE, sizeof(int), (Address) &fsPtr->numGroupIDs,
		      (Address) trueNumGIDsPtr) != SUCCESS) {
	    return(SYS_ARG_NOACCESS);
	}
    }
    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_SetGroupIDs --
 *
 *	Changes all the group IDs for a process.
 *
 * Results:
 *	SYS_ARG_NOACCESS - 	the argument was not accessible.
 *	SYS_INVALID_ARG - 	the argument was was invalid.
 *	PROC_INVALID_PID -	the pid argument was illegal.
 *
 * Side effects:
 *	The process's group IDs are changed.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Proc_SetGroupIDs(numGIDs, gidArrayPtr)
    int		numGIDs;	/* Number of group ids in gidArrayPtr. */
    int		*gidArrayPtr;	/* Array of group ids. */
{
    register	Proc_ControlBlock	*procPtr;
    register	Fs_ProcessState		*fsPtr;
    int 				*newGidArrayPtr;
    int 				size;
    int					i;

    /*
     * See if there's anything to do before we validate the
     * other arguments.
     */

    if ((numGIDs <= 0)  ||  (numGIDs > 128) || (gidArrayPtr == USER_NIL)) {
	return(SYS_INVALID_ARG);
    }

    /*
     * Need to protect against abritrary group setting.
     */
    procPtr = Proc_GetEffectiveProc();
    if (procPtr->effectiveUserID != 0) {
	return(GEN_NO_PERMISSION);
    }

    Vm_MakeAccessible(VM_READONLY_ACCESS,
		    numGIDs * sizeof(int), (Address) gidArrayPtr,
		    &size, (Address *) &newGidArrayPtr);
    if (size != (numGIDs * sizeof(int))) {
	return(SYS_ARG_NOACCESS);
    }

    /*
     *  If the current group ID table is too small, allocate space
     *	for a larger one.
     */

    fsPtr = procPtr->fsPtr;
    if (fsPtr->numGroupIDs < numGIDs) {
	Mem_Free((Address) fsPtr->groupIDs);
	fsPtr->groupIDs = (int *) Mem_Alloc(numGIDs * sizeof(int));
    }

    for (i=0; i < numGIDs; i++) {
	fsPtr->groupIDs[i] = newGidArrayPtr[i];
    }
    fsPtr->numGroupIDs = numGIDs;

    Vm_MakeUnaccessible((Address) newGidArrayPtr, size);

    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * ProcAddToGroupList --
 *
 *	Add the given group ID to the given processes list of groups.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A new group is added to the process's group if not already there.
 *
 *----------------------------------------------------------------------
 */
void
ProcAddToGroupList(procPtr, gid)
    Proc_ControlBlock	*procPtr;
    int			gid;
{
    register	Fs_ProcessState *fsPtr = procPtr->fsPtr;
    int 	*newGidArrayPtr;
    int		i;

    /*
     * See if this gid is already in the list.
     */
    for (i = 0; i < fsPtr->numGroupIDs; i++) {
	if (gid == fsPtr->groupIDs[i]) {
	    return;
	}
    }

    /*
     * Have to add the new group ID to the list.
     */
    newGidArrayPtr = (int *)Mem_Alloc((fsPtr->numGroupIDs + 1) * sizeof(int));
    Byte_Copy(sizeof(int) * fsPtr->numGroupIDs,
	      (Address)fsPtr->groupIDs, (Address)newGidArrayPtr);
    Mem_Free((Address)fsPtr->groupIDs);
    fsPtr->groupIDs = newGidArrayPtr;
    fsPtr->groupIDs[fsPtr->numGroupIDs] = gid;
    fsPtr->numGroupIDs++;
}
