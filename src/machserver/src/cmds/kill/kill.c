/* 
 * kill.c --
 *
 *	A program to send a signal to a process or process group.
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
static char rcsid[] = "$Header: /sprite/src/cmds/kill/RCS/kill.c,v 1.6 90/11/11 12:31:59 kupfer Exp $ SPRITE (Berkeley)";
#endif not lint

#include <ctype.h>
#include <errno.h>
#include <option.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Variables and tables used to parse command-line options:
 */

int sendToGroup = 0;
int printSignals = 0;
int signalNumber = SIGTERM;

Option optionArray[] = {
    {OPT_DOC, (char *) NULL, (char *) NULL,
	    "kill [-sig] [options] pid pid ..."},
    {OPT_DOC, (char *) NULL, (char *) NULL,
	    "\"Sig\" may be either a signal name or a number (default: TERM).\n"},
    {OPT_TRUE, "g", (char *) &sendToGroup,
	    "Send signal to group instead of single process"},
    {OPT_TRUE, "l", (char *) &printSignals,
	    "Print out list of valid signal names"}
};

/*
 * Printable names and identifying messages for all signals, indexed by
 * signal number.
 */

struct	info {
	char	*name;
	char	*reason;
} info[] = {
	0, 0,
	"HUP",		"Hangup",
	"INT",		"Interrupt",
	"DEBUG",	"Debug",
	"ILL",		"Illegal instruction",
	0,		"Signal 5",
	"IOT",		"IOT instruction",
	"EMT",		"EMT instruction",
	"FPE",		"Floating-point exception",
	"KILL",		"Killed",
	"MIG",		"Migrated",
	"SEGV",		"Segmentation violation",
	0,		"Signal 12",
	"PIPE",		"Broken pipe",
	"ALRM",		"Alarm clock",
	"TERM",		"Software termination",
	"URG",		"Urgent I/O condition",
	"STOP",		"Suspended (signal)",
	"TSTP",		"Suspended",
	"CONT",		"Continued",
	"CHLD",		"Child status changed",
	"TTIN",		"Suspended (tty input)",
	0,		"Signal 22",
	"IO",		"I/O is possible on a descriptor",
	0,		"Signal 24",
	0,		"Signal 25",
	0,		"Signal 26",
	0,		"Signal 27",
	"WINCH",	"Window changed",
	"MIGHOME",	"Migrated home",
	"USR1",		"User-defined signal 1",
	"USR2",		"User-defined signal 2",
	0,		"Signal 32",
};

/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	Main program for "kill".
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	See the man page for details.
 *
 *----------------------------------------------------------------------
 */

main(argc, argv)
    int argc;
    char **argv;
{
    int i, j, newArgc;
    int result = 0;
    char *sigName, *endPtr;
    char scratch[10];

    /*
     * Make a pass over the options to suck up the signal number or name.
     * Then call Opt_ to parse the remaining options.
     */

    for (i = 1, newArgc = 1; i < argc; i++) {
	char *p;

	p = argv[i];
	if (*p != '-') {
	    argv[newArgc] = argv[i];
	    newArgc++;
	    continue;
	}
	p++;
	if (isdigit(*p)) {
	    signalNumber = strtoul(p, &endPtr, 0);
	    if (*endPtr != 0) {
		(void) fprintf(stderr, "Bad signal number \"%s\".\n", p);
		exit(1);
	    }
	} else {
	    for (j = 1; j <= 32; j++) {
		if ((info[j].name != 0) && (strcmp(p, info[j].name) == 0)) {
		    signalNumber = j;
		    break;
		}

		/*
		 * If this switch didn't match a signal name, save the
		 * argument for Opt_ processing.
		 */

		if (j == 32) {
		    argv[newArgc] = argv[i];
		    newArgc++;
		}
	    }
	}
    }
    argc = Opt_Parse(newArgc, argv, optionArray, Opt_Number(optionArray), 0);

    /*
     * If the user just wants a list of signals, print it and quit.
     */

    if (printSignals) {
	for (i = 0; i < 32; i++) {
	    if (info[i].name == 0) {
		continue;
	    }
	    (void) printf("%2d  %-15s %s\n", i, info[i].name, info[i].reason);
	}
	exit(0);
    }

    if (argc == 1) {
	(void) fprintf(stderr, "Usage:  %s [options] pid pid ...\n", argv[0]);
	exit(1);
    }

    if (signalNumber <= 32) {
	sigName = info[signalNumber].name;
    } else {
	(void) sprintf(scratch, "Signal %d", signalNumber);
	sigName = scratch;
    }

    for (i = 1; i < argc; i++) {
	int pid;
	char *endPtr;

	pid = strtoul(argv[i], &endPtr, 16);
	if ((endPtr == argv[i]) || (*endPtr != 0)) {
	    (void) fprintf(stderr, "Bad process id \"%s\";  ignoring.\n", argv[i]);
	    continue;
	}

	if (sendToGroup) {
	    if (killpg(pid, signalNumber) != 0) {
		    (void) fprintf(stderr,
			    "Couldn't send %s signal to group 0x%x: %s.\n",
			    sigName, pid, strerror(errno));
	    }
	    result = 1;
	} else {
	    if (kill(pid, signalNumber) != 0) {
		    (void) fprintf(stderr,
			    "Couldn't send %s signal to process 0x%x: %s.\n",
			    sigName, pid, strerror(errno));
	    }
	    result = 1;
	}
    }
    return result;
}
