/* 
 * proc.c --
 *
 *	Miscellaneous run-time library routines for the Proc module.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/lib/c/syscall/RCS/proc.c,v 1.7 90/01/03 17:30:48 douglis Exp $ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include <status.h>
#include <proc.h>
#include <stdio.h>


/*
 *----------------------------------------------------------------------
 *
 * Proc_Exec --
 *
 *	Maps Proc_Exec calls into Proc_ExecEnv calls.  This routine
 *	should not return unless the process cannot be exec'ed.
 *
 *
 * Results:
 *	Error status from Proc_ExecEnv, if any.
 *
 * Side effects:
 *	Refer to Proc_ExecEnv kernel call & man page.
 *
 *----------------------------------------------------------------------
 */

int
Proc_Exec(fileName, argPtrArray, debugMe)
    char *fileName;
    char **argPtrArray;
    Boolean debugMe;
{
    int status;
    extern char **environ;
    extern char **Proc_FetchGlobalEnv();	/* temporary!! */

    /*
     * Install the system-wide environment if ours is non-existent.
     */
    if (environ == (char **) NULL) {
	environ = Proc_FetchGlobalEnv();
    }
    status = Proc_ExecEnv(fileName, argPtrArray, environ, debugMe);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Proc_Wait --
 *
 *	The "normal" interface for waiting on child processes.
 *	This procedure simply invokes the Proc_RawWait system call
 *	and retries the call if the Proc_RawWait call aborted because
 *	of a signal.  See the man page for details on what the kernel
 *	call does.
 *
 * Results:
 *	A standard Sprite return status.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Proc_Wait(numPids, pidArray, block, procIdPtr, reasonPtr, statusPtr,
	subStatusPtr, usagePtr)
    int numPids;		/* Number of entries in pidArray below.
				 * 0 means wait for ANY child. */
    int pidArray[];		/* Array of pids to wait for. */
    Boolean block;		/* TRUE means block;  FALSE means return
				 * immediately if no children are dead. */
    int *procIdPtr;		/* Return ID of dead/stopped process here,
				 * if non-NULL. */
    int *reasonPtr;		/* Return cause of death/stoppage here, if
				 * non-NULL. */
    int *statusPtr;		/* If process exited normally, return exit
				 * status here (if non-NULL).  Otherwise
				 * return signal # here. */
    int *subStatusPtr;		/* Return additional signal status here,
				 * if non-NULL. */
    Proc_ResUsage *usagePtr;	/* Return resource usage info here,
				 * if non-NULL. */
{
    ReturnStatus status;

    do {
	status = Proc_RawWait(numPids, pidArray, block, procIdPtr,
		reasonPtr, statusPtr, subStatusPtr, usagePtr);
    } while (status == GEN_ABORTED_BY_SIGNAL);
    return status;
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_Migrate --
 *
 *	The "normal" interface for invoking process migration.  This
 *	performs extra checks against the process being migrated when
 *	it is already migrated to a different machine.  
 *
 * Results:
 *	A standard Sprite return status.
 *
 * Side effects:
 *	The process is migrated home if it is not already home, then
 *	it is migrated to the node specified.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Proc_Migrate(pid, nodeID)
    Proc_PID pid;
    int	     nodeID;
{
    ReturnStatus status;
    int virtualHost;
    int physicalHost;

    status = Proc_GetHostIDs(&virtualHost, &physicalHost);
    if (status != SUCCESS) {
	return(status);
    }
    if (pid == PROC_MY_PID) {
	if (nodeID != physicalHost && nodeID != virtualHost) {
	    status = Sig_Send(SIG_MIGRATE_HOME, PROC_MY_PID, FALSE);
	    if (status != SUCCESS) {
		return(status);
	    }
	}
    } else {
	int i;
	Proc_PCBInfo info;
	/*
	 * Try to avoid the race condition for migrating other processes
	 * home.  This can be removed once the kernel does remote-to-remote
	 * migration directly.
	 */
#define WAIT_MAX_TIMES 10
#define WAIT_INTERVAL 1
	(void) Sig_Send(SIG_MIGRATE_HOME, pid, TRUE);
	for (i = 0; i < WAIT_MAX_TIMES; i++) {
	    status = Proc_GetPCBInfo(Proc_PIDToIndex(pid),
				     Proc_PIDToIndex(pid), PROC_MY_HOSTID,
				     sizeof(info),
				     &info, (char *) NULL , (int *) NULL);
	    if (status != SUCCESS) {
		return(status);
	    }
	    if (info.state != PROC_MIGRATED) {
		break;
	    }
	    (void) sleep(WAIT_INTERVAL);
	}
	if (i == WAIT_MAX_TIMES) {
	    fprintf(stderr, "Unable to migrate process %x because it wouldn't migrate home.\n", pid);
	    return(FAILURE);
	}
    }
    status = Proc_RawMigrate(pid, nodeID);
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Proc_RemoteExec --
 *
 *	The "normal" interface for invoking remote exec.  This
 *	performs extra checks against the process being migrated when
 *	it is already migrated to a different machine.  
 *
 * Results:
 *	This routine does not return if it succeeds.
 *	A standard Sprite return status is returned upon failure.
 *
 * Side effects:
 *	The process is migrated home if it is not already home, then
 *	a remote exec is performed.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Proc_RemoteExec(fileName, argPtrArray, envPtrArray, host)
    char	*fileName;	/* The name of the file to exec. */
    char	**argPtrArray;	/* The array of arguments to the exec'd 
				 * program. */
    char	**envPtrArray;	/* The array of environment variables for
				 * the exec'd program. */
    int		host;		/* ID of host on which to exec. */
{
    ReturnStatus status;
    int virtualHost;
    int physicalHost;

    status = Proc_GetHostIDs(&virtualHost, &physicalHost);
    if (status != SUCCESS) {
	return(status);
    }
    /*
     * Save a double migration if the exec is local.
     */
    if (physicalHost != host) {
	if (virtualHost != host) {
	    status = Sig_Send(SIG_MIGRATE_HOME, PROC_MY_PID, FALSE);
	    if (status != SUCCESS) {
		return(status);
	    }
	}
	status = Proc_RawRemoteExec(fileName, argPtrArray, envPtrArray, host);
    } else {
	status = Proc_Exec(fileName, argPtrArray, envPtrArray, FALSE);
    }
    return(status);
}
