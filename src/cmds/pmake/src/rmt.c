/*-
 * rmt.c --
 *	Functions to handle the exportation of targets. 
 *
 * Copyright (c) 1987 by the Regents of the University of California
 * Copyright (c) 1987 by Adam de Boor, UC Berkeley
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * Interface:
 *	Rmt_Init  	    	Initialize things for this module
 *
 *	Rmt_AddServer	    	Add the given name as the address of
 *	    	  	    	an export server.
 *
 *	Rmt_Begin 	    	Prepare to export another job and tell
 *	    	  	    	if it can actually be exported.
 *
 *	Rmt_Exec  	    	Execute the given shell with argument vector
 *	    	  	    	elsewhere.
 *
 *	Rmt_LastID	    	Return an unique identifier for the last
 *	    	  	    	job exported.
 *
 *	Rmt_Done  	    	Take note that a remote job has finished.
 *
 *	Rmt_Exit		Clean up upon exit.
 *
 */
#ifndef lint
static char rcsid[] =
"$Header: /sprite/src/cmds/pmake/src/RCS/rmt.c,v 1.10 92/05/18 18:08:29 kupfer Exp $ SPRITE (Berkeley)";
#endif lint

#include    <errno.h>
#include    <mig.h>
#include    <stdio.h>
#include    <stdlib.h>
#include    <host.h>
#include    <string.h>
#include    <sys/wait.h>
#include    <sys/signal.h>
#include    <kernel/net.h>
#include    <status.h>
#include    "make.h"
#include    "proc.h"
#include    "job.h"

/*
 * Library imports:
 */

extern char **environ;

static int 	curHost;	/* most recent host chosen for exportation */
static Lst	hosts[2];	/* list of available hosts, for the 2
				   priorities we use */
static Boolean 	initialized = FALSE; /* make sure we've been initialized */

#ifdef RMT_EXPLICIT_HOST_RETURN
static void RmtExit();		/* Exit handler. */
#endif
static void DoExec();
static void RemotePathExec();
static void RmtStatusChange();

/*-
 *-----------------------------------------------------------------------
 * Rmt_Init --
 *	Initialize this module...
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The current host is set to NIL.
 *
 *-----------------------------------------------------------------------
 */
void
Rmt_Init()
{
	curHost = NIL;
	hosts[MIG_LOW_PRIORITY] = Lst_Init(FALSE);
	hosts[MIG_NORMAL_PRIORITY] = Lst_Init(FALSE);
	initialized = TRUE;
#ifdef RMT_EXPLICIT_HOST_RETURN
	atexit(RmtExit);
#endif
}

/*-
 *-----------------------------------------------------------------------
 * Rmt_AddServer --
 *	Add a server to the list of those known.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Who knows?
 *
 *-----------------------------------------------------------------------
 */
/*ARGSUSED*/
void
Rmt_AddServer (name)
    char    *name;
{

}


/*-
 *-----------------------------------------------------------------------
 * RmtStatusChange --
 *
 * 	Note a change in host status, from the mig library.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	May flag for remigration.
 *
 *-----------------------------------------------------------------------
 */
/*ARGSUSED*/
static void
RmtStatusChange (hostID)
    int hostID;
{
    if (hostID == 0) {
	if (DEBUG(RMT)) {
	    printf("New host available...\n");
	}
	jobFull = FALSE;
    } else {
	if (amMake) {
	    printf("*** Warning: host %d reclaimed, and processes may now be running locally (run pmake instead of make!).\n",
		   hostID);
	    return;
	}
	if (DEBUG(RMT)) {
	    printf("Host %d reclaimed.\n", hostID);
	}
	JobFlagForMigration(hostID);
    }
}


/*-
 *-----------------------------------------------------------------------
 * Rmt_Begin --
 *	Prepare to export a job.
 *
 * Results:
 *	TRUE if the job can be exported. FALSE if it cannot.
 *
 * Side Effects:
 *	If the job can be exported, an idle host is selected and stored
 *	in the file-global variable curHost.
 *
 *-----------------------------------------------------------------------
 */
/*ARGSUSED*/
Boolean
Rmt_Begin (file, argv, gn)
    char    	  *file;
    char    	  **argv;
    GNode 	  *gn;
{
    int hostNumbers[1];
    int hostsAssigned;
    int prio;

    if (noExport) {
	return FALSE;
    }
    if ((gn->type & OP_BACKGROUND) || background) {
	prio = MIG_LOW_PRIORITY;
    } else {
	prio = MIG_NORMAL_PRIORITY;
    }
    while (!Lst_IsEmpty(hosts[prio])) {
	curHost = (int) Lst_DeQueue(hosts[prio]);
	if (Mig_ConfirmIdle(curHost)) {
	    if (DEBUG(RMT)) {
		printf("Rmt_Begin: reusing host %d for migration.\n", curHost);
		fflush(stdout);
	    }
	    return TRUE;
	} else {
	    if (DEBUG(RMT)) {
		printf("Rmt_Begin: couldn't reuse host %d -- no longer available.\n",
		       curHost);
		fflush(stdout);
	    }
	    curHost = NIL;
	}
    }
    hostsAssigned = Mig_RequestIdleHosts(1, prio, 0,
					 RmtStatusChange,
					 hostNumbers);
    if (hostsAssigned <= 0) {
	if (DEBUG(RMT)) {
	    if (hostsAssigned == 0) {
		printf( "Pmake couldn't find an idle node.\n");
	    } else {
		fprintf(stderr, "Pmake couldn't find an idle node: %s.\n",
			strerror(errno));
	    }
	}
	return FALSE;
    } else if (DEBUG(RMT)) {
	printf("Rmt_Begin: selected host %d for migration.\n", hostNumbers[0]);
	fflush(stdout);
    }

    curHost = hostNumbers[0];
    return TRUE;
}


/*-
 *-----------------------------------------------------------------------
 * Rmt_Exec --
 *	Execute a process elsewhere.  The migration is performed by
 *	execing a simple program that immediately migrates and then
 *	execs the program specified in its argument list.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	If a host was found by the last call to Mig_GetIdleNode,
 *	then migrate to that host.  If not, execute locally.  
 *
 *-----------------------------------------------------------------------
 */
/* ARGSUSED */
void
Rmt_Exec (file, argv, traceMe)
    char    *file;
    char    **argv;
    Boolean traceMe;
{
    ReturnStatus status;

    if (curHost != NIL) {
	/*
	 * Okay to migrate, but don't migrate the stream to the global daemon.
	 */
	if (mig_GlobalPdev != -1) {
	    (void) close(mig_GlobalPdev);
	}

	if (DEBUG(RMT)) {
	    printf("Calling RemotePathExec(%s, ...)\n", file);
	}
	RemotePathExec (file, argv, curHost);
	if (DEBUG(RMT)) {
	    perror("RemotePathExec failed");
	}
    }
    status = Proc_ExecEnv (file, argv, environ, traceMe);
    if (DEBUG(RMT)) {
	printf("Proc_ExecEnv of %s failed: %s\n", file,
	       Stat_GetMsg(status));
    }
}

/*-
 *-----------------------------------------------------------------------
 * Rmt_ReExport --
 *	Re-migrate a running process to another node.
 *	We really want to migrate the entire process group.  Until
 *	the kernel can support this operation, we fudge it by going through
 * 	the process table.  (Ugh.)
 *
 * Results:
 *	TRUE if the job can be re-migrated. FALSE if it cannot.
 *
 * Side Effects:
 *	The process may be migrated to another node.
 *	Resets curHost to NIL to indicate no host has been selected.
 *-----------------------------------------------------------------------
 */
/*ARGSUSED*/
Boolean
Rmt_ReExport(pid, gn, hostPtr)
    Proc_PID	  pid;
    GNode 	  *gn;
    int		  *hostPtr;
{
    Boolean foundNode;
    ReturnStatus status;
#define NUM_PCBS 256
    Proc_PCBInfo infos[NUM_PCBS];
    register Proc_PCBInfo *infoPtr;
    int pcbsUsed;
    int i;

    if (curHost == NIL) {
	if (DEBUG(RMT)) {
	    printf("Rmt_ReExport selecting a new node.\n");
	}
	foundNode = Rmt_Begin((char *) NIL, (char **) NIL, gn);
	if (!foundNode) {
	    if (DEBUG(RMT)) {
		printf("Rmt_ReExport couldn't find a new node.\n");
	    }
	    return(FALSE);
	}
    }

    /*
     * Dump the entire process table into our memory.
     */

    status = Proc_GetPCBInfo(0, NUM_PCBS-1, homeHost,
			     sizeof(Proc_PCBInfo),
			     infos, (Proc_PCBArgString *) NULL, &pcbsUsed);
    if (status != SUCCESS) {
	fprintf(stderr, "Couldn't read process table: %s\n",
		Stat_GetMsg(status));
	return(FALSE);
    }

    for (i = 0, infoPtr = infos; i < pcbsUsed; i++, infoPtr++) {
	if (infoPtr->familyID == pid) {
	    if (infoPtr->state == PROC_UNUSED ||
		infoPtr->state == PROC_EXITING ||
		infoPtr->state == PROC_DEAD) {
		continue;
	    }
	    if (DEBUG(RMT)) {
		printf("Rmt_ReExport: remigrating process %x to host %d.\n",
		       infoPtr->processID, curHost);
		fflush(stdout);
	    }
	    /*
	     * Make sure the process is running.
	     */
	    kill(infoPtr->processID, SIGCONT);

	    /*
	     * Try to migrate it.  If the process has exited in the meantime,
	     * don't sweat it.
	     */
	    status = Proc_Migrate(infoPtr->processID, curHost);
	    if (status != SUCCESS && status != PROC_INVALID_PID) {
		fprintf(stderr, "Error: can't migrate to host %d: %s\n",
			curHost, Stat_GetMsg(status));
		curHost = NIL;
		return(FALSE);
	    }
	}
    }
    if (hostPtr != (int *) NULL) {
	*hostPtr = curHost;
    }
    curHost = NIL;
    return(TRUE);
}

/*-
 *-----------------------------------------------------------------------
 * Rmt_LastID --
 *	Return a "unique" identifier for the last job exported with Rmt_Exec
 *
 * Results:
 *	The host to which the last process was exported is returned.
 *	This host ID is used later in Rmt_Done.
 *
 * Side Effects:
 *	Resets the current host for migration to be NIL.
 *
 *-----------------------------------------------------------------------
 */
int
Rmt_LastID(pid)
    Proc_PID	  pid;
{
    int host = curHost;
    
    curHost = NIL;
    return(host);
}

/*-
 *-----------------------------------------------------------------------
 * Rmt_Done --
 *	Register the completion of a remote job.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The host on which the job executed is freed for future migrations.
 *
 *-----------------------------------------------------------------------
 */
/*ARGSUSED*/
void
Rmt_Done (id, gn)
    int	    id;
    GNode *gn;
{
    int prio;

    if ((gn->type & OP_BACKGROUND) || background) {
	prio = MIG_LOW_PRIORITY;
    } else {
	prio = MIG_NORMAL_PRIORITY;
    }

    if (DEBUG(RMT)) {
	printf("Rmt_Done(host=%d, priority=%d) called.\n",
	       id, prio);
    }
    if (Lst_EnQueue(hosts[prio], (ClientData) id) != 0) {
	fprintf(stderr, "pmake: error enqueuing free host.\n");
	if (Mig_Done(id) < 0) {
	    perror("Mig_Done");
	}
    }
}

/*-
 *-----------------------------------------------------------------------
 * RmtExit --
 *	Clean up before exiting pmake, by releasing the reserved hosts.
 *	Called as an exit handler.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The hosts on which any jobs executed are freed for future migrations.
 *
 *-----------------------------------------------------------------------
 */
/*ARGSUSED*/
#ifdef RMT_EXPLICIT_HOST_RETURN
static void
RmtExit()
{
    int host;
    
    if (DEBUG(RMT)) {
	printf("RmtExit called.\n");
    }
    if (!initialized) {
	if (DEBUG(RMT)) {
	    printf("Rmt module not initialized.\n");
	}
	return;
    }
    while (!Lst_IsEmpty(hosts[MIG_LOW_PRIORITY])) {
	host = (int) Lst_DeQueue(hosts[MIG_LOW_PRIORITY]);
	if (DEBUG(RMT)) {
	    printf("Releasing host %d.\n", host);
	}
	if (Mig_Done(host) < 0) {
	    perror("Mig_Done");
	}
    }
    while (!Lst_IsEmpty(hosts[MIG_NORMAL_PRIORITY])) {
	host = (int) Lst_DeQueue(hosts[MIG_NORMAL_PRIORITY]);
	if (DEBUG(RMT)) {
	    printf("Releasing host %d.\n", host);
	}
	if (Mig_Done(host) < 0) {
	    perror("Mig_Done");
	}
    }
    if (DEBUG(RMT)) {
	printf("RmtExit finished.\n");
    }
}
#endif


/*
 *-----------------------------------------------------------------------
 *
 * DoExec --
 *
 *	Function to actually execute a program. If the exec didn't succeed
 *	because the file isn't in a.out format, attempt to execute
 *	it as a bourne shell script.
 *
 * Results:
 *	None.  Doesn't even return unless the exec failed.
 *
 * Side Effects:
 *	A program may be execed over this one.  Sets errno if the exec 
 *	failed.
 *
 *-----------------------------------------------------------------------
 */

static void
DoExec(file, argv, hostID)
    char *file;			/* File to execute. */
    char **argv;		/* Arguments to the program. */
    int hostID;			/* ID of host on which to exec */
{
    ReturnStatus status;

    status = Proc_RemoteExec(file, argv, environ, hostID);
    if (DEBUG(RMT)) {
	fprintf(stderr, "Proc_RemoteExec(\"%s\"): %s\n", file,
		Stat_GetMsg(status));
    }
    errno = Compat_MapCode(status);
    if (errno == ENOEXEC) {
	/*
	 * Attempt to execute the file as a shell script using
	 * the Bourne shell)
	 */
	register char **newargv;
	register int i;

	for (i = 0; argv[i] != 0; i++) {
	    /* Empty loop body */
	}
	newargv = (char **) malloc((unsigned) ((i+1)*sizeof (char *)));
	newargv[0] = "sh";
	newargv[1] = file;
	for (i = 1; argv[i] != 0; i++) {
	    newargv[i+1] = argv[i];
	}
	newargv[i+1] = 0;
	status = Proc_RemoteExec("/sprite/cmds/sh", newargv, environ, hostID);
	errno = Compat_MapCode(status);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * RemotePathExec --
 *
 *	Execute a process, using the current environment variable,
 *	instead of an explicitly-supplied one.  Also, imitate the
 *	shell's actions in trying each directory in a search path
 *	(given by the "PATH" environment variable).  Also, specify
 *	a remote host.  This is taken from the library execve call.
 *
 * Results:
 *	None.  Returns only if there was an error.
 *
 * Side effects:
 *	Overlays the current process with a new image.  See the man
 *	page for details.  Sets errno if the exec failed.
 *
 *----------------------------------------------------------------------
 */

static void
RemotePathExec(name, argv, hostID)
    char *name;			/* Name of file containing program to exec. */
    char **argv;		/* Array of arguments to pass to program. */
    int hostID;			/* ID of host on which to exec */
{
    char *path;
    char *fullName;
    register char *first, *last;
    int size, noAccess;
    int tmpErrno;		/* copy of errno from DoExec */

    noAccess = 0;

    if (index(name, '/') != 0) {
	/*
	 * If the name specifies a path, don't search for it on the search path,
	 * just try and execute it.
	 */
	DoExec(name, argv, hostID);
	return;
    }

    path = getenv("PATH");
    if (path == 0) {
	path = "/sprite/cmds";
    }
    fullName = malloc((unsigned) (strlen(name) + strlen(path)) + 2);
    for (first = path; ; first = last+1) {

	/*
	 * Generate the next file name to try.
	 */

	for (last = first; (*last != 0) && (*last != ':'); last++) {
	    /* Empty loop body. */
	}
	size = last-first;
	(void) strncpy(fullName, first, size);
	if (last[-1] != '/') {
	    fullName[size] = '/';
	    size++;
	}
	(void) strcpy(fullName + size, name);

	if (DEBUG(RMT)) {
	    fprintf(stderr, "Trying DoExec(\"%s\")....\n", fullName);
	}
	DoExec(fullName, argv, hostID);
	tmpErrno = errno;
	if (DEBUG(RMT)) {
	    fprintf(stderr, "DoExec(\"%s\") => %d.\n", fullName, errno);
	}
	errno = tmpErrno;
	if (errno == EACCES) {
	    noAccess = 1;
	} else if (errno != ENOENT) {
	    break;
	}
	if (*last == 0) {
	    /*
	     * Hit the end of the path. We're done.
	     * If there existed a file by the right name along the search path,
	     * but its permissions were wrong, return FS_NO_ACCESS. Else return
	     * whatever we just got back.
	     */
	    if (noAccess) {
		errno = EACCES;
	    }
	    break;
	}
    }

    tmpErrno = errno;
    free((char *) fullName);
    errno = tmpErrno;
}
