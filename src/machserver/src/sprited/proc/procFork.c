/* 
 *  procFork.c --
 *
 *	Routines to create new processes.  No monitor routines are required
 *	in this file.  Synchronization to proc table entries is by a call
 *	to the proc table monitor to get a PCB and calls to the family monitor
 *	to put a newly created process into a process family.
 *
 * Copyright (C) 1985, 1988, 1991 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/sprited/proc/RCS/procFork.c,v 1.11 92/07/16 18:06:52 kupfer Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <sprite.h>
#include <byte.h>
#include <ckalloc.h>
#include <cthreads.h>
#include <mach.h>
#include <mach_error.h>
#include <status.h>
#include <string.h>
#include <stdio.h>

#include <fs.h>
#include <main.h>
#include <proc.h>
#include <procInt.h>
#include <procMachInt.h>
#include <procTypes.h>
#include <sig.h>
#include <spriteSrvServer.h>
#include <utils.h>
#include <vm.h>

/* 
 * This is the maximum expected length for a process's name.  Longer names 
 * won't break anything, but they won't get recorded in the C Threads name. 
 */
#define PROC_MAX_NAME_LENGTH	50

/* 
 * This struct packages up the arguments used to initialize a server thread.
 */
typedef struct {
    Proc_ControlBlock *procPtr;	/* the pcb for the "kernel" process */
    Proc_ProcessRoot workProc;	/* the function to execute */
} ServerThreadInfo;


/* Forward references: */

static ReturnStatus ProcMakeServerThread _ARGS_((Proc_ControlBlock *procPtr,
						 Proc_ProcessRoot startProc));
static ReturnStatus ProcSetUserState _ARGS_((Proc_ControlBlock *procPtr,
				Proc_ControlBlock *parentProcPtr,
				Address startPC, Address stateAddr));
static any_t ServerThreadInit _ARGS_((any_t arg));


/*
 *----------------------------------------------------------------------
 *
 * Proc_NewProc --
 *
 *	Create a new process.  This routine is used to fork user processes
 *	(current proc and new proc are both user), create new server
 *	processes (current proc and new proc are both "kernel"), and start
 *	up the first user process (current proc is kernel, and new proc is
 *	user).
 *
 * Results:
 *	Returns a status code and fills in the process ID for the new 
 *	process.
 *
 * Side effects:
 *	Allocates a PCB entry and fills it in.  If creating a user
 *	process, creates its task and thread, plus does some other
 *	initialization.  For a "kernel" process, just creates a new
 *	thread in the server.  In both cases sets up the process's 
 *	state and makes it runnable.
 *
 *----------------------------------------------------------------------
 */
    
ReturnStatus
Proc_NewProc(startPC, stateAddr, procType, shareHeap, pidPtr, procName)
    Address	startPC;	/* The start address for the new process */
    Address	stateAddr;	/* the address of the saved user state
				 * (when a user process forks a child) */
    int		procType;	/* One of PROC_KERNEL or PROC_USER. */
    Boolean	shareHeap;	/* TRUE if share heap, FALSE if not. */
    Proc_PID	*pidPtr;	/* A pointer to where to return the process
				   ID in; possibly null. */
    char	*procName;	/* Name for process control block, possibly 
				 * NULL */
{
    ReturnStatus	status = SUCCESS;
    Proc_ControlBlock 	*procPtr;	/* The new process being created */
    Proc_ControlBlock 	*parentProcPtr;	/* The parent of the new process,
					 * the one that is making this call */
    Boolean		migrated = FALSE;

    if (shareHeap) {
	/* 
	 * What we really want to do in this case is just create a new
	 * thread, use the parent's task (increment the reference
	 * count for the task).
	 */
	panic("Proc_NewProc: shared heap not implemented yet.\n");
    }
    if (!main_MultiThreaded) {
	panic("Proc_NewProc called too early during startup.\n");
    }

    parentProcPtr = Proc_GetActualProc();

    if (parentProcPtr->genFlags & PROC_FOREIGN) {
	migrated = TRUE;
    }

    procPtr = (Proc_ControlBlock *)ProcGetUnusedPCB();
    if (pidPtr != NULL) {
	*pidPtr		= procPtr->processID;
    }

    /*
     * Sanity checks.
     */
    if (procPtr->rpcClientProcess != NULL) {
	panic("Proc_NewProc: non-null rpcClientProcess.\n");
    }
    if (procPtr->locksHeld != 0) {
	panic("Proc_NewProc: new process has locks.\n");
    }
#ifdef LOCKDEP
    if (procPtr->lockStackSize != 0) {
	panic("Proc_NewProc: new process has locks registered.\n");
    }
#endif

#ifdef SPRITED_PROFILING
    procPtr->Prof_Scale = 0;
    Prof_Enable(procPtr, parentProcPtr->Prof_Buffer, 
        parentProcPtr->Prof_BufferSize, parentProcPtr->Prof_Offset,
	parentProcPtr->Prof_Scale);
#endif

    procPtr->genFlags 		|= procType;
    procPtr->syncFlags		= 0;
    procPtr->exitFlags		= 0;

    if (!migrated) {
	procPtr->parentID 	= parentProcPtr->processID;
    } else {
	procPtr->parentID 	= parentProcPtr->peerProcessID;
    }
    procPtr->familyID 		= parentProcPtr->familyID;
    procPtr->userID 		= parentProcPtr->userID;
    procPtr->effectiveUserID 	= parentProcPtr->effectiveUserID;

    procPtr->currCondPtr	= NULL;
    procPtr->unixProgress	= parentProcPtr->unixProgress;

    /* 
     * Free up the old argument list, if any.  Note, this could be put
     * in Proc_Exit, but is put here for consistency with the other
     * reinitializations of control block fields.  
     */

    if (procPtr->argString != (Address) NIL) {
	ckfree((Address) procPtr->argString);
	procPtr->argString = (Address) NIL;
    }

    /*
     * Create the argument list for the child.  If no name specified, take
     * the list from the parent.  If one is specified, just make a one-element
     * list containing that name.
     */
    if (procName != NULL) {
	procPtr->argString = ckstrdup(procName);
    } else if (parentProcPtr->argString != (Address) NIL) {
	procPtr->argString = ckstrdup(parentProcPtr->argString);
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
    Sig_Fork(parentProcPtr, Proc_AssertLocked(procPtr));

    if (!migrated) {
	procPtr->peerHostID = NIL;
	procPtr->peerProcessID = NIL;
    } else {
	status = ProcRemoteFork(parentProcPtr, procPtr);
	if (status != SUCCESS) {
	    /*
	     * We couldn't fork on the home node, so free up the new
	     * process that we were in the process of allocating.
	     */
	    ProcFreePCB(Proc_AssertLocked(procPtr));
	    goto done;
	}

	/*
	 * Change the returned process ID to be the process ID on the home
	 * node.
	 */
	if (pidPtr != (Proc_PID *) NIL) {
	    *pidPtr = procPtr->peerProcessID;
	}
    }

    /* 
     * The PCB fields should now be consistent, so it's okay to unlock the 
     * PCB.
     */
    Proc_Unlock(Proc_AssertLocked(procPtr));

    /* 
     * We now have the following state left to set up: machine state, 
     * environment variables, and FS state.  For user processes, they are 
     * done in this order because (a) setting up the machine state could 
     * fail, (b) the other two routines can't be conveniently undone, and 
     * (c) we can keep the child from running until initialization is 
     * complete.  For "kernel" processes the machine state is done last 
     * because (a) none of the routines should fail and (b) we don't have a 
     * convenient way to keep the child from running once its machine state 
     * has been set up.
     */
    if (procType == PROC_KERNEL) {
	ProcSetupEnviron(procPtr);
	Fs_InheritState(parentProcPtr, procPtr);
	Proc_SetState(procPtr, PROC_READY);
	status = ProcMakeServerThread(procPtr, (Proc_ProcessRoot)startPC);
	if (status != SUCCESS) {
	    panic("Proc_NewProc: couldn't make server thread.\n");
	}
    } else {
	/* 
	 * Set up the Mach task, thread, and ports, and set its state.
	 */
	status = ProcMakeTaskThread(procPtr, parentProcPtr);
	if (status == SUCCESS) {
	    status = ProcSetUserState(procPtr, parentProcPtr, startPC,
				      stateAddr);
	    if (status != SUCCESS) {
		Proc_Lock(procPtr);
		ProcFreeTaskThread(Proc_AssertLocked(procPtr));
		Proc_Unlock(Proc_AssertLocked(procPtr));
	    }
	}
	if (status != SUCCESS) {
	    /*
	     * We couldn't set up the process, so free whatever stuff
	     * we've already allocated for it.
	     */
	    if (migrated) {
		Proc_Lock(procPtr);
	    } else {
		ProcFamilyRemove(procPtr);
		Proc_Lock(procPtr);
		List_Remove((List_Links *) &(procPtr->siblingElement));
	    }
	    ProcFreePCB(Proc_AssertLocked(procPtr));
	    goto done;
	}

	/*
	 * Set up the environment and FS state for the process.
	 */
	if (!migrated) {
	    ProcSetupEnviron(procPtr);
	}
	Fs_InheritState(parentProcPtr, procPtr);
    }

    /* 
     * Notify the user when the "init" process is started.
     */
    if ((parentProcPtr->genFlags & PROC_KERNEL) &&
	(procPtr->genFlags & PROC_USER)) {
	printf("Starting %s\n", main_InitPath);
    }

    /* 
     * Start the process.  (Kernel processes were started above.  We could 
     * do the kernel and user processes at the same time if we relax the 
     * process state sanity checks.)
     */
    if (procPtr->genFlags & PROC_USER) {
	Proc_Lock(procPtr);
	Proc_MakeReady(Proc_AssertLocked(procPtr));
	Proc_Unlock(Proc_AssertLocked(procPtr));
    }

 done:
    return status;
}


/*
 *----------------------------------------------------------------------
 *
 * ProcMakeServerThread --
 *
 *	Create a thread for a "kernel" process.
 *
 * Results:
 *	Returns a status code.  Because cthread_fork never fails, this 
 *	should always be SUCCESS.
 *
 * Side effects:
 *	The thread is created and detached.
 *
 *----------------------------------------------------------------------
 */

static ReturnStatus
ProcMakeServerThread(procPtr, startProc)
    Proc_ControlBlock *procPtr;	/* the PCB for the new process */
    Proc_ProcessRoot startProc;	/* where the new thread should start 
				 * executing */
{
    cthread_t newThreadPtr;
    ServerThreadInfo *infoPtr;

    infoPtr = (ServerThreadInfo *)ckalloc(sizeof(ServerThreadInfo));
    if (infoPtr == NULL) {
	panic("ProcMakeServerThread: out of memory.\n");
    }
    infoPtr->procPtr = procPtr;
    infoPtr->workProc = startProc;

    newThreadPtr = cthread_fork(ServerThreadInit, (any_t)infoPtr);
    if (newThreadPtr == NO_CTHREAD) {
	/* "can't happen" */
	panic("ProcMakeServerThread: no more threads.\n");
    }

    cthread_detach(newThreadPtr);

    return SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * ServerThreadInit --
 *
 *	Initialization for a server thread.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sets the name and PCB context for the thread.  Before exiting, 
 *	resets the thread name and frees the argument list that was 
 *	passed in.
 *
 *----------------------------------------------------------------------
 */

static any_t
ServerThreadInit(arg)
    any_t arg;			/* arguments for the new thread */
{
    ServerThreadInfo *infoPtr = (ServerThreadInfo *)arg;
    Proc_ControlBlock *procPtr;
    char threadName[PROC_MAX_NAME_LENGTH + 50];

    /* 
     * If debugging is turned on, wire the current C Thread to a 
     * kernel thread, so that gdb will be able to find it.
     */
    if (main_DebugFlag) {
	cthread_wire();
    }

    /* 
     * Set the thread's name to be the pid of the server process.  Note 
     * that we don't return from this process until the thread is done, so 
     * there's no need to make a safe copy of the thread name.
     */
    procPtr = infoPtr->procPtr;
    if (procPtr->argString != NULL &&
	    strlen(procPtr->argString) < PROC_MAX_NAME_LENGTH) {
	sprintf(threadName, "%s (pid %x)", procPtr->argString,
		infoPtr->procPtr->processID);
    } else {
	sprintf(threadName, "(pid %x)", infoPtr->procPtr->processID);
    }
    cthread_set_name(cthread_self(), threadName);

    /*
     * Set the thread's context to the given PCB.
     */
    Proc_SetCurrentProc(infoPtr->procPtr);

    /* 
     * Call the given routine.
     */
    (*(infoPtr->workProc))();

    /* 
     * The thread is done, so clean up after it.
     */
    printf("thread for pid %x is exiting.\n", infoPtr->procPtr->processID);
    ckfree(infoPtr);
    Proc_Exit(0);

    return 0;			/* lint */
}


/*
 *----------------------------------------------------------------------
 *
 * ProcSetUserState --
 *
 *	Set up the VM and registers for a user process.
 *
 * Results:
 *	SUCCESS or an error code.
 *
 * Side effects:
 *	Mucks with the given process's VM and registers.
 *
 *----------------------------------------------------------------------
 */

static ReturnStatus
ProcSetUserState(procPtr, parentProcPtr, startPC, stateAddr)
    Proc_ControlBlock *procPtr;	/* the process to initialize */
    Proc_ControlBlock *parentProcPtr; /* its parent */
    Address startPC;		/* the start address for the child (not
				 * used for the first user process) */
    Address stateAddr;		/* address of saved state for the child */
{
    ReturnStatus status = SUCCESS;

    /* 
     * If the parent process is a "kernel" process, then we're starting the 
     * very first user process.  Just exec the "init" program.
     */
    if (parentProcPtr->genFlags & PROC_KERNEL) {
	return Proc_KernExec(procPtr, main_InitPath, main_InitArgArray);
    }

    /* 
     * To create a user process from another user process, make a copy of 
     * the parent.  Then set its PC and stack pointer to the values given 
     * us. 
     */
    status = Vm_Fork(procPtr, parentProcPtr);
    if (status != SUCCESS) {
	return status;
    }
    return ProcMachSetRegisters(procPtr, stateAddr, startPC);
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_ForkStub --
 *
 *	Process a fork request for a user process.
 *
 * Results:
 *	Returns KERN_SUCCESS.  Fills in the Sprite status code and "pending
 *	signals" flag.  If successful, fills in the child's process ID.
 *
 * Side effects:
 *	The user process is cloned, its stack pointer and PC are set to the 
 *	given values, and the child process is set in motion.
 *
 *----------------------------------------------------------------------
 */

kern_return_t
Proc_ForkStub(serverPort, startAddr, startStack, statusPtr, pidPtr,
	      sigPendingPtr)
    mach_port_t serverPort;	/* request port */
    vm_address_t startAddr;	/* starting PC for the child */
    vm_address_t startStack;	/* starting stack pointer for the child */
    ReturnStatus *statusPtr;	/* OUT: status code */
    Proc_PID *pidPtr;		/* OUT: child's process ID */
    boolean_t *sigPendingPtr;	/* OUT: is there a signal pending */
{
#ifdef lint
    serverPort = serverPort;
#endif
    *statusPtr = Proc_NewProc((Address)startAddr, (Address)startStack,
			      PROC_USER, FALSE, pidPtr, (char *)NULL);
    *sigPendingPtr = Sig_Pending(Proc_GetCurrentProc());
    return KERN_SUCCESS;
}
