/* 
 *  procFork.c --
 *
 *	Routines to create new processes.  No monitor routines are required
 *	in this file.  Synchronization to proc table entries is by a call
 *	to the proc table monitor to get a PCB and calls to the family monitor
 *	to put a newly created process into a process family.
 *
 * Copyright (C) 1985, 1988 Regents of the University of California
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
#include "mach.h"
#include "list.h"
#include "proc.h"
#include "procInt.h"
#include "sched.h"
#include "status.h"
#include <stdlib.h>
#include "string.h"
#include "sync.h"
#include "sys.h"
#include "timer.h"
#include "vm.h"
#include "prof.h"

static ReturnStatus    InitUserProc();


/*
 *----------------------------------------------------------------------
 *
 * Proc_Fork --
 *
 *	Process the fork system call.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Proc_Fork(shareHeap, pidPtr)
    Boolean	shareHeap;
    Proc_PID	*pidPtr;
{
    Proc_PID		*newPidPtr;
    int			numBytes;
    ReturnStatus	status;
    
    /*
     * Make the pointer to the process id that is to be returned accessible.
     */

    Vm_MakeAccessible(VM_OVERWRITE_ACCESS, 
		      sizeof(Proc_PID), (Address) pidPtr,
		      &numBytes, (Address *) &newPidPtr);
    if (numBytes < sizeof(Proc_PID)) {
	return(SYS_ARG_NOACCESS);
    }

    /*
     * Start up the new process.  The PC where to begin execution doesn't 
     * matter since it has already been stored in the proc table entry before
     * we were called.
     */

    status = Proc_NewProc((Address) 0, PROC_USER, shareHeap, newPidPtr,
			  (char *)NIL);

    Vm_MakeUnaccessible((Address) newPidPtr, numBytes);

    return(status);
}

    parentProcPtr = Proc_GetPCB(procPtr->parentID);
    Proc_Lock(parentProcPtr);
    if (!(parentProcPtr->genFlags & PROC_VFORKPARENT)) {
	panic("VForkWakeup called but VFORKPARENT flag == 0.");
    }
    parentProcPtr->genFlags &= ~PROC_VFORKPARENT;
    Proc_Unlock(parentProcPtr);

    LOCK_MONITOR;
    Sync_Broadcast(&vforkCondition);
    UNLOCK_MONITOR;

    procPtr->genFlags &= ~PROC_VFORKCHILD;

}


/*
Proc_NewProc(PC, procType, shareHeap, pidPtr, procName)
 *
 * Proc_NewProc --
 *
 *	Allocates a PCB and initializes it.
 *
 * Results:
 *
 * Side effects:
 *	PCB initialized and made runnable.
 *
 * ----------------------------------------------------------------------------
 */

ReturnStatus
Proc_NewProc(PC, procType, shareHeap, pidPtr, procName, vforkFlag)
    Address 	PC;		/* The program counter where to start. */
    int		procType;	/* One of PROC_KERNEL or PROC_USER. */
    Boolean	shareHeap;	/* TRUE if share heap, FALSE if not. */
    Proc_PID	*pidPtr;	/* A pointer to where to return the process
				   ID in. */
    char	*procName;	/* Name for process control block */
    Boolean     vforkFlag;      /* Added for vfork */
{
    ReturnStatus	status;
    Proc_ControlBlock 	*procPtr;	/* The new process being created */
    Proc_ControlBlock 	*parentProcPtr;	/* The parent of the new process,
					 * the one that is making this call */
    Boolean		migrated = FALSE;

    parentProcPtr = Proc_GetActualProc();

    if (parentProcPtr->genFlags & PROC_FOREIGN) {
    procPtr = ProcGetUnusedPCB();
    if (pidPtr != (Proc_PID *) NIL) {
	*pidPtr		= procPtr->processID;
    }

    procPtr->Prof_Scale = 0;
    Prof_Enable(procPtr, parentProcPtr->Prof_Buffer, 
        parentProcPtr->Prof_BufferSize, parentProcPtr->Prof_Offset,
	parentProcPtr->Prof_Scale);

    procPtr->processor		= parentProcPtr->processor;
    procPtr->state 		= PROC_READY;
    procPtr->genFlags 		= procType;
    if (vforkFlag) {
	procPtr->genFlags |= PROC_VFORKCHILD;
    }
    procPtr->syncFlags		= 0;
    procPtr->schedFlags		= 0;
    procPtr->exitFlags		= 0;

    if (!migrated) {
	procPtr->parentID 	= parentProcPtr->processID;
    } else {
	procPtr->parentID 	= parentProcPtr->peerProcessID;
    }
    procPtr->familyID 		= parentProcPtr->familyID;
    procPtr->userID 		= parentProcPtr->userID;

    procPtr->billingRate 	= parentProcPtr->billingRate;
    procPtr->recentUsage 	= 0;
    procPtr->weightedUsage 	= 0;
    procPtr->unweightedUsage 	= 0;
     */
    procPtr->kernelCpuUsage.ticks 	= timer_TicksZeroSeconds;
    procPtr->userCpuUsage.ticks 	= timer_TicksZeroSeconds;
    procPtr->childKernelCpuUsage.ticks = timer_TicksZeroSeconds;
    procPtr->childUserCpuUsage.ticks 	= timer_TicksZeroSeconds;
    procPtr->numQuantumEnds	= 0;
    procPtr->numWaitEvents	= 0;
    procPtr->event		= NIL;

    procPtr->kcallTable		= mach_NormalHandlers;
    procPtr->unixProgress	= parentProcPtr->unixProgress;

    /* 
     * Free up the old argument list, if any.  Note, this could be put
     * in Proc_Exit, but is put here for consistency with the other
     * reinitializations of control block fields.  
p     */

    if (procPtr->argString != (Address) NIL) {
	free((Address) procPtr->argString);
	procPtr->argString = (Address) NIL;
    }

    /*
     * Create the argument list for the child.  If no name specified, take
     * the list from the parent.  If one is specified, just make a one-element
     * list containing that name.
     */
    if (procName != (char *)NIL) {
	procPtr->argString = (char *) malloc(strlen(procName) + 1);
	(void) strcpy(procPtr->argString, procName);
    } else if (parentProcPtr->argString != (Address) NIL) {
	procPtr->argString =
		(char *) malloc(strlen(parentProcPtr->argString) + 1);
	(void) strcpy(procPtr->argString, parentProcPtr->argString);
    }

    if (!migrated) {
	if (ProcFamilyInsert(procPtr, procPtr->familyID) != SUCCESS) {
	    panic("Proc_NewProc: ProcFamilyInsert failed\n");
	}
    }

    /*
     *  Initialize our child list to remove any old links.
     *  If not migrated, insert this PCB entry into the list
     *  of children of our parent.
     */
    List_Init((List_Links *) procPtr->childList);
    if (!migrated) {
	List_Insert((List_Links *) &(procPtr->siblingElement), 
		    LIST_ATREAR(parentProcPtr->childList));
    }
    Sig_Fork(parentProcPtr, procPtr);

    Vm_ProcInit(procPtr);

    /*
     * If the process is migrated, setup its process state on the home node.
     */
    if (migrated) {
	status = ProcRemoteFork(parentProcPtr, procPtr);
	if (status != SUCCESS) {
	    /*
	     * We couldn't fork on the home node, so free up the new
	     * process that we were in the process of allocating.
	     */

	    ProcFreePCB(procPtr);

	    return(status);
	}

	/*
	 * Change the returned process ID to be the process ID on the home
	 * node.
	 */
	if (pidPtr != (Proc_PID *) NIL) {
	    *pidPtr = procPtr->peerProcessID;
	}
    } else {
	procPtr->peerHostID = NIL;
	procPtr->peerProcessID = NIL;
    }
	status = InitUserProc(procPtr, parentProcPtr, shareHeap);
    /*
     * Set up the virtual memory of the new process.
     */

    if (procType == PROC_KERNEL) {
	status = Mach_SetupNewState(procPtr, (Mach_State *)NIL,
				    Sched_StartKernProc, PC, FALSE);
	if (status != SUCCESS) {
	    /*
	     * We are out of kernel stacks.
	     */
	    ProcFreePCB(procPtr);
	    return(status);
	}
    } else {
	status = InitUserProc(procPtr, parentProcPtr, shareHeap, vforkFlag);
	if (status != SUCCESS) {
	    ProcFreePCB(procPtr);

	    return(status);
	}
    }

    /*
     * Mark ourselves waiting, if necessary
     */
    if (vforkFlag) {
	Proc_Lock(parentProcPtr);
	parentProcPtr->genFlags |= PROC_VFORKPARENT;
	Proc_Unlock(parentProcPtr);
    }

    Mach_SetReturnVal(procPtr, (int) PROC_CHILD_PROC);
     * Set up the environment of the process.
     */

    if (!migrated) {
	ProcSetupEnviron(procPtr);
    }
    /*
     * Put the process on the ready queue.
     */
    Sched_MakeReady(procPtr);

    /*
     * Now make the parent wait until child exec`s or exits
     */
    if (vforkFlag) {
	LOCK_MONITOR;
	while (parentProcPtr->genFlags & PROC_VFORKPARENT) {
	    Sync_Wait(&vforkCondition, FALSE);
	}
	UNLOCK_MONITOR;
    }

    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
InitUserProc(procPtr, parentProcPtr, shareHeap)
 * InitUserProc --
 *
 *	Initalize the state for a user process.  This involves allocating
 *	the segments for the new process.
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static ReturnStatus
InitUserProc(procPtr, parentProcPtr, shareHeap, vforkFlag)
    register	Proc_ControlBlock	*procPtr;	/* PCB to initialized.*/
    register	Proc_ControlBlock	*parentProcPtr;	/* Parent's PCB. */
    Boolean				shareHeap;	/* TRUE => share heap
							 * with parent. */
    Boolean				vforkFlag;	/* TRUE => share all
     * is a copy of the parents.  Finally the heap segment is either a copy
    ReturnStatus	status;

    /*
     * Set up a kernel stack for the process.
     */
    status = Vm_SegmentDup(parentProcPtr->vmPtr->segPtrArray[VM_STACK],
    /*
    if (status != SUCCESS) {
	Mach_FreeState(procPtr);
	return(status);
     * or the same as the parent depending on the share heap flag.
     */
    if (shareHeap) {
    procPtr->vmPtr->segPtrArray[VM_SYSTEM] = (Vm_Segment *) NIL;

    if (vforkFlag) {
	Vm_SegmentIncRef(parentProcPtr->vmPtr->segPtrArray[VM_STACK], procPtr);
	procPtr->vmPtr->segPtrArray[VM_STACK] = 
				parentProcPtr->vmPtr->segPtrArray[VM_STACK];
    } else {
	status = Vm_SegmentDup(parentProcPtr->vmPtr->segPtrArray[VM_STACK],
			procPtr, &(procPtr->vmPtr->segPtrArray[VM_STACK]));
	if (status != SUCCESS) {
	    Mach_FreeState(procPtr);
    if (shareHeap || vforkFlag) {
	Vm_SegmentIncRef(parentProcPtr->vmPtr->segPtrArray[VM_HEAP], procPtr);
	procPtr->vmPtr->segPtrArray[VM_HEAP] = 
				parentProcPtr->vmPtr->segPtrArray[VM_HEAP];
    } else {
	status = Vm_SegmentDup(parentProcPtr->vmPtr->segPtrArray[VM_HEAP],
			   procPtr, &(procPtr->vmPtr->segPtrArray[VM_HEAP]));
	if (status != SUCCESS) {
	    Vm_SegmentDelete(procPtr->vmPtr->segPtrArray[VM_STACK], procPtr);
	    Mach_FreeState(procPtr);
	    return(status);
	}
    }

    if (parentProcPtr->vmPtr->sharedSegs != (List_Links *)NIL) {
	Vm_CopySharedMem(parentProcPtr, procPtr);
    }

    Vm_SegmentIncRef(parentProcPtr->vmPtr->segPtrArray[VM_CODE], procPtr);
    procPtr->vmPtr->segPtrArray[VM_CODE] =
				parentProcPtr->vmPtr->segPtrArray[VM_CODE];

    return(SUCCESS);
}
