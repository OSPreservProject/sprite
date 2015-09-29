/* 
 * procTaskThread.c --
 *
 *	Routines for dealing with Mach tasks, threads, and ports.
 *
 * Copyright 1991, 1992 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that this copyright
 * notice appears in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/sprited/proc/RCS/procTaskThread.c,v 1.16 92/05/12 11:58:33 kupfer Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <sprite.h>
#include <ckalloc.h>
#include <mach.h>
#include <mach_error.h>
#include <servers/machid.h>
#include <servers/machid_lib.h>
#include <servers/netname.h>

#include <main.h>		/* for debug flag */
#include <procInt.h>
#include <sig.h>
#include <sync.h>
#include <sys.h>
#include <utils.h>
#include <vm.h>

/* 
 * This is the request port for the MachID server.  We record user 
 * processes with MachID so that we can look at them with ms, vminfo, etc.
 */
static mach_port_t machIdServer = MACH_PORT_NULL;


/*
 *----------------------------------------------------------------------
 *
 * ProcTaskThreadInit --
 *
 *	Initialization for dealing with Mach task/thread interface.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Gets the request port for the MachID server.
 *
 *----------------------------------------------------------------------
 */

void
ProcTaskThreadInit()
{
    kern_return_t kernStatus;
    char *hostname = "";

    kernStatus = netname_look_up(name_server_port, hostname, "MachID",
				 &machIdServer);
    /* 
     * If it didn't work, it's not the end of the world, so don't panic. 
     */
    if (kernStatus != KERN_SUCCESS) {
	printf("Couldn't get MachID server port: %s\n",
	       mach_error_string(kernStatus));
    }
}


/*
 *----------------------------------------------------------------------
 *
 * ProcReleaseTaskInfo --
 *
 *	Decrement the reference count for a process's per-task 
 *	information.  If this is the last reference, kill the task and 
 *	free the data structure.  The process should already be 
 *	locked.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Might kill the task and free some memory.  Nulls out the pointer in 
 *	the PCB.
 *
 *----------------------------------------------------------------------
 */

void
ProcReleaseTaskInfo(procPtr)
    Proc_LockedPCB *procPtr;
{
    kern_return_t kernStatus;
    ProcTaskInfo *taskInfoPtr = procPtr->pcb.taskInfoPtr;

    if (taskInfoPtr == NULL) {
	return;
    }

    if (taskInfoPtr->refCount <= 0) {
	panic("ReleaseTaskInfo: bogus reference count.\n");
    }
    taskInfoPtr->refCount--;

    if (taskInfoPtr->refCount == 0) {
	if (taskInfoPtr->task != MACH_PORT_NULL) {
	    /* 
	     * Kill the task and get rid of our send right for it.
	     */
	    kernStatus = task_terminate(taskInfoPtr->task);
	    if (kernStatus != KERN_SUCCESS &&
	    	    kernStatus != KERN_INVALID_ARGUMENT &&
	            kernStatus != MACH_SEND_INVALID_DEST) {
		printf("ReleaseTaskInfo: can't kill task: %s\n",
		       mach_error_string(kernStatus));
	    }
	    (void)mach_port_deallocate(mach_task_self(),
				       taskInfoPtr->task);
	}
	taskInfoPtr->task = MACH_PORT_NULL;

	Vm_CleanupTask(procPtr);

	ckfree(taskInfoPtr);
    }

    procPtr->pcb.taskInfoPtr = NULL;
}


/*
 *----------------------------------------------------------------------
 *
 * ProcNewTaskInfo --
 *
 *	Create a new TaskInfo object.
 *
 * Results:
 *	Returns a pointer to an initialized ProcTaskInfo, with a 
 *	reference count of 1.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ProcTaskInfo *
ProcNewTaskInfo()
{
    ProcTaskInfo *infoPtr;

    infoPtr = (ProcTaskInfo *)ckalloc(sizeof(ProcTaskInfo));
    if (infoPtr == NULL) {
	panic("NewTaskInfo: out of memory.\n");
    }

    infoPtr->refCount = 1;
    infoPtr->task = MACH_PORT_NULL;
    Vm_NewProcess(&infoPtr->vmInfo);

    return infoPtr;
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_ExceptionToPCB --
 *
 *	Map an exception port to a PCB.
 *
 * Results:
 *	Returns a pointer to the PCB for the given exception port.
 *	Returns NULL if there was a problem getting the PCB for the
 *	exception port.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Proc_ControlBlock *
Proc_ExceptionToPCB(port)
    mach_port_t port;
{
    Proc_ControlBlock *procPtr;

    /* 
     * Because all of our ports point to allocated memory, it's okay 
     * to indirect once.  However, before indirecting again (to check 
     * the magic number), make sure the back pointer really does point 
     * to the start of a PCB.
     */
    procPtr = *(Proc_ControlBlock **)port;
    if (&procPtr->backPtr != (Proc_ControlBlock **)port) {
	procPtr = NULL;
    } else if (procPtr->magic != PROC_PCB_MAGIC_NUMBER) {
	procPtr = NULL;
    }

    return procPtr;
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_MakeReady --
 *
 *	Starts a process running.
 *
 * Results:
 *	status code.
 *
 * Side effects:
 *	Sets the state variable in the PCB.  For user processes, makes the 
 *	thread runnable (the task is already).
 *
 *----------------------------------------------------------------------
 */

void
Proc_MakeReady(procPtr)
    Proc_LockedPCB *procPtr;	/* the locked process to start */
{
    kern_return_t kernStatus;

    Proc_SetState((Proc_ControlBlock *)procPtr, PROC_READY);
    Sync_Broadcast(&procPtr->pcb.resumeCondition);
    if (procPtr->pcb.genFlags & PROC_USER) {
	kernStatus = thread_resume(procPtr->pcb.thread);
	if (kernStatus != KERN_SUCCESS) {
	    printf("Proc_MakeReady: couldn't start pid %x: %s\n",
		   procPtr->pcb.processID, mach_error_string(kernStatus));
	    (void)Sig_SendProc(procPtr, SIG_KILL, TRUE, SIG_NO_CODE,
			       (Address)0);
	}
    }
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_SyscallToPCB --
 *
 *	Map a system call request port to a PCB.
 *
 * Results:
 *	Returns a pointer to the PCB for the given system call port.  
 *	Returns NULL if there was a problem getting the PCB.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Proc_ControlBlock *
Proc_SyscallToPCB(syscallPort)
    mach_port_t syscallPort;
{
    Proc_ControlBlock *procPtr;

    procPtr = (Proc_ControlBlock *)syscallPort;
    if (procPtr->magic != PROC_PCB_MAGIC_NUMBER) {
	procPtr = NULL;
    }

    return procPtr;
}


/*
 *----------------------------------------------------------------------
 *
 * ProcMakeServicePort --
 *
 *	Create a service port for a process, using the given name.
 *
 * Results:
 *	Returns the given name.
 *
 * Side effects:
 *	Creates the port and allocates a send right for it.  Adds the 
 *	service port to the system request port set.
 *
 *----------------------------------------------------------------------
 */

mach_port_t
ProcMakeServicePort(newPort)
    mach_port_t newPort;	/* the name to give the port */
{
    kern_return_t kernStatus;

    kernStatus = mach_port_allocate_name(mach_task_self(),
					 MACH_PORT_RIGHT_RECEIVE,
					 newPort);
    if (kernStatus != KERN_SUCCESS) {
	panic("ProcMakeServicePort couldn't allocate service port: %s\n",
	       mach_error_string(kernStatus));
    }
    kernStatus = mach_port_insert_right(mach_task_self(), newPort, newPort,
					MACH_MSG_TYPE_MAKE_SEND);
    if (kernStatus != KERN_SUCCESS) {
	panic("%s couldn't create send right for service port: %s\n",
	       "ProcMakeServicePort", mach_error_string(kernStatus));
    }
    kernStatus = mach_port_move_member(mach_task_self(), newPort,
				       sys_RequestPort);
    if (kernStatus != KERN_SUCCESS) {
	panic("ProcMakeServicePort can't move service port to port set: %s\n",
	       mach_error_string(kernStatus));
    }

    return newPort;
}


/*
 *----------------------------------------------------------------------
 *
 * ProcDestroyServicePort --
 *
 *	Destroy the given port.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Nils out the place where the port was named.
 *
 *----------------------------------------------------------------------
 */

void
ProcDestroyServicePort(portPtr)
    mach_port_t *portPtr;	/* IN: the port to free; OUT: set to nil */
{
    kern_return_t kernStatus;

    if (*portPtr == MACH_PORT_NULL) {
	return;
    }

    kernStatus = mach_port_destroy(mach_task_self(), *portPtr);
    if (kernStatus != KERN_SUCCESS) {
	printf("ProcDestroyServicePort: can't destroy port: %s\n",
	       mach_error_string(kernStatus));
    }

    *portPtr = MACH_PORT_NULL;
}


/*
 *----------------------------------------------------------------------
 *
 * ProcFreeTaskThread --
 *
 *	Blow away the Mach resources held by the given locked process.  
 *	Because the process's VM resources are tied to the task, this also 
 *	frees the process's VM resources.
 *
 * Results:
 *	None.  (If something goes wrong, what else is there to do except 
 *	complain?)
 *
 * Side effects:
 *	Destroys the process's thread and cleans up the procPtr
 *	structure.  Kills the task if this is the last process using
 *	it.
 *
 *----------------------------------------------------------------------
 */

void
ProcFreeTaskThread(procPtr)
    Proc_LockedPCB *procPtr;	/* the process to kill */
{
    /* 
     * Vm_Copy{In,Out} (and possibly other routines) need the task
     * information, but we don't have any good way to synchronize with
     * them.  If the process is voluntarily exiting, then that's not a
     * problem.  If the process is being killed by some other process, make
     * sure it doesn't have a thread handling a request for it.
     */
    if ((Proc_ControlBlock *)procPtr != Proc_GetCurrentProc() &&
		procPtr->pcb.genFlags & PROC_BEING_SERVED) {
	panic("ProcFreeTaskThread: potential race.\n");
    }

#ifdef SPRITED_ACCOUNTING
    UndefinedProcedure();
#endif
    ProcKillThread(procPtr);
    ProcReleaseTaskInfo(procPtr);
    ProcDestroyServicePort(&procPtr->pcb.syscallPort);
    ProcDestroyServicePort(&procPtr->pcb.exceptionPort);
}


/*
 *----------------------------------------------------------------------
 *
 * ProcKillThread --
 *
 *	Kill a user process's Mach thread and release our reference to it.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Destroys the thread and deallocates our copy of the name.
 *
 *----------------------------------------------------------------------
 */

void
ProcKillThread(procPtr)
    Proc_LockedPCB *procPtr;	/* the process, should be locked. */
{
    kern_return_t kernStatus;

    if (procPtr->pcb.thread != MACH_PORT_NULL) {
	kernStatus = thread_terminate(procPtr->pcb.thread);
	if (kernStatus != KERN_SUCCESS &&
	    	kernStatus != KERN_INVALID_ARGUMENT &&
	        kernStatus != MACH_SEND_INVALID_DEST) {
	    printf("ProcFreeTaskThread: can't kill thread: %s\n",
		   mach_error_string(kernStatus));
	}
	(void)mach_port_deallocate(mach_task_self(), procPtr->pcb.thread);
    }
    procPtr->pcb.thread = MACH_PORT_NULL;
}


/*
 *----------------------------------------------------------------------
 *
 * ProcMakeTaskThread --
 *
 *	Make the Mach task, thread, and ports for a user process.
 *
 * Results:
 *	Returns a status code.
 *
 * Side effects:
 *	Fills in the given PCB with information about the created 
 *	objects. 
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
ProcMakeTaskThread(procPtr, parentProcPtr)
    Proc_ControlBlock *procPtr;	/* the process being created */
    Proc_ControlBlock *parentProcPtr; /* the currently executing process */
{
    kern_return_t kernStatus;
    mach_port_t parentTask;	/* parent task for the new task */
    ReturnStatus status = SUCCESS;
    
    Proc_Lock(procPtr);

    procPtr->taskInfoPtr = ProcNewTaskInfo();

    parentTask = (parentProcPtr->genFlags & PROC_KERNEL) ?
	mach_task_self() : parentProcPtr->taskInfoPtr->task;
    kernStatus = task_create(parentTask, FALSE,
			     &procPtr->taskInfoPtr->task);
    if (kernStatus != KERN_SUCCESS) {
	printf("ProcMakeTaskThread: couldn't create child task: %s\n",
	       mach_error_string(kernStatus));
	status = Utils_MapMachStatus(kernStatus);
	goto done;
    }
    kernStatus = thread_create(procPtr->taskInfoPtr->task,
			       &procPtr->thread);
    if (kernStatus != KERN_SUCCESS) {
	printf("ProcMakeTaskThread: couldn't create child thread: %s\n",
	       mach_error_string(kernStatus));
	ProcReleaseTaskInfo(Proc_AssertLocked(procPtr));
	status = Utils_MapMachStatus(kernStatus);
	goto done;
    }

    /* 
     * Register the task with the MachID server.  This lets us use tools 
     * like vminfo to poke around inside user processes.
     * XXX The authorization port (mach_task_self currently) appears to be 
     * ignored by machid_mach_register.
     */
    if (machIdServer != MACH_PORT_NULL) {
	kernStatus = machid_mach_register(machIdServer, mach_task_self(),
					  procPtr->taskInfoPtr->task,
					  MACH_TYPE_TASK,
					  &procPtr->taskInfoPtr->machId);
	if (kernStatus != KERN_SUCCESS) {
	    printf("%s: couldn't register pid %x with MachID server: %s\n",
		   "ProcMakeTaskThread", procPtr->processID,
		   mach_error_string(kernStatus));
	}
    }

    /* 
     * Set up the exception and system call request ports for the 
     * process.
     */
    
    procPtr->syscallPort = ProcMakeServicePort((mach_port_t)procPtr);
    procPtr->exceptionPort = ProcMakeServicePort(
					(mach_port_t)&procPtr->backPtr);
    kernStatus = task_set_bootstrap_port(procPtr->taskInfoPtr->task,
					 procPtr->syscallPort);
    if (kernStatus != KERN_SUCCESS) {
	panic("%s: can't install system call request port: %s\n",
	      "ProcMakeTaskThread", mach_error_string(kernStatus));
    }
    kernStatus = thread_set_exception_port(procPtr->thread,
					   procPtr->exceptionPort);
    if (kernStatus != KERN_SUCCESS) {
	panic("ProcMakeTaskThread: can't install exception port: %s\n",
	       mach_error_string(kernStatus));
    }

    /* 
     * Null out the exception port for the task, so that if we bobble the
     * thread exception message, gdb won't get the exception message and
     * get thoroughly confused.  A possible alternative would be to tie the
     * task error port to a simple routine that always causes the thread to
     * get terminated.
     */
    kernStatus = task_set_exception_port(procPtr->taskInfoPtr->task,
					 MACH_PORT_NULL);
    if (kernStatus != KERN_SUCCESS) {
	panic("ProcMakeTaskThread: can't install task exception port: %s\n",
	       mach_error_string(kernStatus));
    }

 done:
    Proc_Unlock(Proc_AssertLocked(procPtr));
    return status;
}
