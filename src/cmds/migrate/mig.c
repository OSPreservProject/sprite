/* 
 * mig.c --
 *
 *	Program to perform remote execution using process migration.
 *
 * Copyright 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/cmds/migrate/RCS/mig.c,v 1.14 90/09/24 14:38:37 douglis Exp $ SPRITE (Berkeley)";
#endif not lint


#include <sprite.h>
#include <status.h>
#include <option.h>
#include <proc.h>
#include <stdio.h>
#include <stdlib.h>
#include <host.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <mig.h>

/*
 * Library imports:
 */

extern char **environ;

/*
 * Forward declarations
 */
static void DoExec();
static int RemotePathExec();

int debug = 0;
char *host = NULL;
int hostID = -1;
char *procIDString = NULL;
int local = 0;
int background = 0;
int backPrio = 0;
int verbose = 0;
char *execName = NULL;

Option optionArray[] = {
    {OPT_STRING, "h", (Address) &host,
	 "String identifier for host to migrate onto (default is random selection)."},
    {OPT_INT, "H", (Address) &hostID,
	 "Numeric identifier for host to migrate onto."},
    {OPT_TRUE, "b", (Address) &background,
	 "Execute command in background and print processID."},
    {OPT_TRUE, "B", (Address) &backPrio,
	 "Execute command at background priority."},
    {OPT_TRUE, "l", (Address) &local,
	 "Execute command locally if no idle node is available."},
    {OPT_TRUE, "D", (Address) &debug,
	 "Enable debugging messages."},
    {OPT_STRING, "E", (Address) &execName,
	 "Exec the command specified by this argument, setting argv[0] to the first argument following the option list."},
    {OPT_TRUE, "v", (Address) &verbose,
	 "Print useful information, such as which host is selected."},
    {OPT_STRING, "p", (Address) &procIDString,
	 "Process ID of process to migrate (default is to start shell, or issue command given as remaining arguments)"},
};

/*
 * Default shell to invoke if none found in environment.
 */
#define SHELL "csh"

main(argc, argv)
    int  argc;
    char *argv[];
{
    ReturnStatus status;
    char *myName;
    char **argArray;
    char *shell;
    int pid;
    Host_Entry *hostPtr;
    int i;
    int selectedHost;
    int doCmd;
    int procID;
    int flags;
    
    argc = Opt_Parse(argc, argv, optionArray, Opt_Number(optionArray), 
		     OPT_ALLOW_CLUSTERING|OPT_OPTIONS_FIRST);

    myName = argv[0];
    if (hostID != -1 && host != (char *) NULL) {
	fprintf(stderr, "%s: -h and -H options are mutually exclusive\n",
		myName);
	exit(1);
    }
    if (debug) {
	verbose = 1;
    }
    if (procIDString != (char *) NULL) {
	if (background) {
	    fprintf(stderr, "%s: -b and -p options are mutually exclusive\n",
		    myName);
	    exit(1);
	}
	procID = strtol(procIDString, (char **) NULL, 16);
	if (procID <= 0) {
	    (void)fprintf(stderr, "%s: invalid process ID: %s.\n", myName,
			  procIDString);
	    exit(1);
	}
	doCmd = 0;
	if (argc > 1) {
	    (void)fprintf(stderr, "%s: extra arguments ignored.\n", myName);
	}
	if (local) {
	    (void)fprintf(stderr,
			  "%s: -l argument ignored when migrating process.\n",
			  myName);
	    local = 0;
	}
	flags = MIG_PROC_AGENT;
    } else {
	doCmd = 1;
	procID = PROC_MY_PID;
	if (argc == 1) {
	    argArray = (char **) malloc(2 * sizeof(char *));
	    shell = getenv("SHELL");
	    if (shell == (char *) NULL) {
		shell = SHELL;
	    }
	    argArray[0] = shell;
    	    execName = shell;
	    argArray[1] = NULL;
	} else {
	    argArray = &argv[1];
	    if (!execName) {
		execName = argArray[0];
	    }
	}
	flags = background ? MIG_PROC_AGENT : 0;
    }  
    if (host != (char *) NULL) {
	selectedHost = 0;
	hostPtr = Host_ByName(host);
	if (hostPtr == (Host_Entry *) NULL) {
	    fprintf(stderr, "%s: %s: no such host.\n", myName, host);
	    exit(1);
	}
	hostID = hostPtr->id;
	Host_End();
    } else if (hostID != -1) {
	selectedHost = 0;
    } else {
	int hostNumbers[1];
	int hostsAssigned;
	
	selectedHost = 1;
	(void) signal(SIGINT, SIG_IGN);
	hostsAssigned = Mig_RequestIdleHosts(1, backPrio ?
					     MIG_LOW_PRIORITY :
					     MIG_NORMAL_PRIORITY,
					     flags,
					     (void (*)()) NULL, hostNumbers);
	if (hostsAssigned < 0) {
	    perror("Mig_RequestIdleHosts");
	    exit(1);
	}
	if (hostsAssigned == 0) {
	    if (!local) {
		fprintf(stderr, "%s: no idle host available.\n", myName);
		exit(1);
	    }
	    hostID = 0;
	} else {
	    hostID = hostNumbers[0];
	}
	if (verbose) {
	    if (hostID) {
		hostPtr = Host_ByID(hostID);
		if (hostPtr == (Host_Entry *) NULL) {
		    fprintf(stderr, "%s: unable to find host %d.\n",
			    myName, hostID);
		    exit(1);
		}
		fprintf(stderr, "%s: migrating to host %s.\n", myName,
			hostPtr->name);
		Host_End();
	    } else {
		fprintf(stderr,
			"%s: no idle host available; running locally.\n",
			myName);
	    }
	}
    }

    if ((selectedHost || background) && hostID && doCmd) {
	/*
	 * Wait for the process to complete so we can register that we're
	 * through with the host.
	 */
	pid = fork();
	if (pid < 0) {
	    perror("Error forking child");
	    exit(1);
	}
		
	if (pid) {
	    union wait status;
	    int error;
	    int exited = 0;
	    int exitCode;


	    if (background) {
		printf("%x\n", pid);
		exit(0);
	    }
	    do {
	    
		error = wait(&status);
		if (error == -1) {
		    perror("wait");
		    exit(1);
		} else if (error != pid && debug) {
		    (void) fprintf(stderr, "%s: received status %d from wait.\n",
				   myName, error);
		} else if (error == pid) {
		    if (status.w_stopval != WSTOPPED) {
			exited = 1;
			if (status.w_termsig != 0) {
			    if (debug) {
				(void) fprintf(stderr,
					       "%s: process was signalled.\n",
					       myName);
			    }
			    exitCode = status.w_termsig;
			} else {
			    exitCode = status.w_retcode;
			}
		    } else {
			if (debug) {
			    (void) fprintf(stderr,
					   "%s: process was suspended.\n",
					   myName);
			}
		    }
		}
	    } while (!exited);
#ifdef undef
	    if (debug) {
		(void) fprintf(stderr,
			       "%s: Exited(%d).  Calling Mig_Done(%d).\n",
			       myName, exitCode, hostID);
		(void) fflush(stderr);
	    }
	    (void) Mig_Done(hostID);
#endif
	    exit(exitCode);
	} else {
	    (void) signal(SIGINT, SIG_DFL);
	}
    }
    if (hostID && !doCmd) {
	/*
	 * We're migrating someone else.
	 */
	
	if (debug) {
	    (void) fprintf(stderr, "Calling Proc_Migrate(%d, %d).\n", procID,
			   hostID);
	    (void) fflush(stderr);
	}
	
	status = Proc_Migrate(procID, hostID);
	if (status != SUCCESS) {
	    fprintf(stderr, "%s: error in Proc_Migrate: %s\n",
		    myName, Stat_GetMsg(status));
	    fflush(stderr);
	    if (!local) {
		exit(1);
	    }
	}
    }
    if (!doCmd) {
	exit(0);
    }
    /*
     * Still the child here, or the only process if we were running locally
     * and never forked.
     */
    if (debug) {
	(void) fprintf(stderr, "Calling RemotePathExec(%s,...,%d).\n",
		       execName, hostID);
	(void) fflush(stderr);
    }
    status = RemotePathExec(execName, argArray, hostID);
    perror("Error execing program");
    exit(status);
}


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
 *	A program may be execed over this one.
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
    if (debug) {
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
 *	This procedure returns only if the exec fails.  In this case
 *	the return value is -1.
 *
 * Side effects:
 *	Overlays the current process with a new image.  See the man
 *	page for details.
 *
 *----------------------------------------------------------------------
 */

static int
RemotePathExec(name, argv, hostID)
    char *name;			/* Name of file containing program to exec. */
    char **argv;		/* Array of arguments to pass to program. */
    int hostID;			/* ID of host on which to exec */
{
    char *path;
    char *fullName;
    register char *first, *last;
    int size, noAccess;

    noAccess = 0;

    if (index(name, '/') != 0) {
	/*
	 * If the name specifies a path, don't search for it on the search path,
	 * just try and execute it.
	 */
	DoExec(name, argv, hostID);
	return -1;
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

	if (debug) {
	    fprintf(stderr, "Trying DoExec(\"%s\")....\n", fullName);
	}
	DoExec(fullName, argv, hostID);
	if (debug) {
	    fprintf(stderr, "DoExec(\"%s\") => %d.\n", fullName, errno);
	}
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
    free((char *) fullName);
    return -1;
}
