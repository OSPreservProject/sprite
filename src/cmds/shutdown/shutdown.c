/* 
 * shutdown.c --
 *
 *	Program to shutdown the operating system.
 *
 * Copyright (C) 1988 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/cmds/shutdown/RCS/shutdown.c,v 1.11 90/01/24 07:46:05 douglis Exp Locker: jhh $ SP RITE (Berkeley)";
#endif not lint

#include "sys.h"
#include "option.h"
#include <stdio.h>
#include <sys/file.h>
#include <errno.h>

#define FASTBOOT "/local/fastboot"

/*
 * Options.
 */
static int	halt = 0;		/* Non-zero means halt. */
static int	dontSyncDisks = 0;	/* Non-zero means don't sync the disks
					 * when shutting down the system. */
static int	reboot = 0;		/* Non-zero means reboot. */
static int	debug = 0;		/* Non-zero means enter the debugger. */
static int	fastBoot = 0;		/* Non-zero means don't check the disks
					 * on reboot. */
static int	quickBoot = 0;		/* Whether not to do wall. */
static int	sleepTime = 30;		/* Number of seconds to sleep after
					 * wall. */
static int	singleUser = 0;		/* Non-zero means reboot single user. */
static int	client	= 0;		/* Non-zero means reboot fileserver
					 * without using /boot on local disk. */
static int	rootcmds = 0;		/* Non-zero means run rootcmds. */
static int	debugShutdown = 0;	/* Non-zero means don't really
					 * shut down. */
static int	root = 0;		/* Non-zero means reboot as root. */
static int	nonroot = 0;		/* Non-zero means don't reboot as 
					 * root. */
static char	*rootdisk = NULL;	/* Disk to attach during boot. */
/*
 * String to use when rebooting the system.
 */
char		nullString[] = "";
char		*rebootString = nullString;
char		buffer[100];

/*
 * Flags to command-line options:
 */
Option optionArray[] = {
    {OPT_TRUE, "h", (Address) &halt, "Halt (This is the default)"},
    {OPT_TRUE, "r", (Address) &reboot, "Reboot"},
    {OPT_STRING, "R", (Address) &rebootString, "String to pass to boot prom (implies -r)"},
    {OPT_TRUE, "d", (Address) &debug, "Enter the debugger"},
    {OPT_TRUE, "f", (Address) &fastBoot,
	     "Don't check disk consistency upon reboot (reboot if no other options.)"},
    {OPT_TRUE, "w", (Address) &dontSyncDisks, "Dont write back the cache (Default is to write it back)"},
    {OPT_TRUE, "s", (Address) &singleUser, "Reboot single user mode"},
    {OPT_TRUE, "c", (Address) &client, 
	"Reboot fileserver as a client (don't use /boot on local disk)"},
    {OPT_TRUE, "x", (Address) &rootcmds, "Run rootcmds before diskcmds"},
    {OPT_TRUE, "q", (Address) &quickBoot, "Don't do a wall, and wait, before rebooting"},
    {OPT_INT, "S", (Address) &sleepTime, "Number of seconds to wait after wall"},
    {OPT_TRUE, "D", (Address) &debugShutdown, "Don't actually shut down"},
    {OPT_TRUE, "root", (Address) &root, "Reboot as root server"},
    {OPT_TRUE, "nonroot", (Address) &nonroot, "Don't reboot as root server"},
    {OPT_STRING, "rootdisk", (Address) &rootdisk, "Disk to attach during boot"},
};
int numOptions = sizeof(optionArray) / sizeof(Option);


/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	The main program for shutdown.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Prints information on standard output.
 *
 *----------------------------------------------------------------------
 */
main(argc, argv)
    int argc;
    char *argv[];
{
    int	flags;

    (void) Opt_Parse(argc, argv, optionArray, numOptions, OPT_ALLOW_CLUSTERING);
    if (!quickBoot) {
	char *args[3];
	int fd[2];
	int pid;
	
	sprintf(buffer, "system shutting down in %d seconds\n", sleepTime);
	if (pipe(fd) < 0) {
	    perror("pipe");
	    exit(1);
	}
	if (write(fd[1], buffer, strlen(buffer) + 1) < 0) {
	    perror("write");
	    exit(1);
	}
	close(fd[1]);
	pid = fork();
	if (pid < 0) {
	    perror("fork");
	    exit(1);
	}
	if (pid == 0) {
	    args[0] = "wall";
	    args[1] = "-l";
	    args[2] = 0;
	    if (dup2(fd[0], 0) < 0) {
		perror("dup2");
		exit(0);
	    }
	    execv("/sprite/cmds/wall", args);
	    perror("execv");
	} else {
	    wait(0);
	    sleep(sleepTime);
	}
    }
    strncpy(buffer, rebootString, 100);
    if (strlen(rebootString) > 0) {
	reboot = 1;
    }
    if (dontSyncDisks) {
	flags = 0;
    } else {
	flags = SYS_WRITE_BACK;
    }
    if (fastBoot) {
	char	str[80];
	int	fd;

	fd = open(FASTBOOT, O_CREAT, 0777);
	if (fd < 0) {
	    if (errno != ENOENT) {
		sprintf(str, "Couldn't open %s", FASTBOOT);
		perror(str);
		exit(1);
	    }
	}

	strcat(buffer, " -f");
	if (!halt && !debug) {
	    reboot = 1;
	}
    }
    if (singleUser) {
	strcat(buffer, " -s");
	if (!halt && !debug) {
	    reboot = 1;
	}
    }
    if (client) {
	strcat(buffer, " -c");
	if (!halt && !debug) {
	    reboot = 1;
	}
    }
    if (rootcmds) {
	strcat(buffer, " -x");
	if (!halt && !debug) {
	    reboot = 1;
	}
    }
    if (root) {
	strcat(buffer, " -root");
	if (!halt && !debug) {
	    reboot = 1;
	}
    }
    if (nonroot) {
	strcat(buffer, " -nonroot");
	if (!halt && !debug) {
	    reboot = 1;
	}
    }
    if (rootdisk) {
	strcat(buffer, " -rootdisk ");
	strcat(buffer, rootdisk);
	if (!halt && !debug) {
	    reboot = 1;
	}
    }
    if (debugShutdown) {
	fprintf(stderr, "Would call Sys_Shutdown(%s) here.\n", reboot ? buffer : "");
    } else if (debug) {
	Sys_Shutdown(flags | SYS_KILL_PROCESSES | SYS_DEBUG, NULL);
    } else if (reboot) { 
	Sys_Shutdown(flags | SYS_KILL_PROCESSES | SYS_REBOOT, buffer);
    } else {
	Sys_Shutdown(flags | SYS_KILL_PROCESSES | SYS_HALT, NULL);
    }
}
