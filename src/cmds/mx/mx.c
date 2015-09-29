/* 
 * mx.c --
 *
 *	This file contains the top-level routines for "mx", a mouse-based
 *	editor that runs on top of the X window system and uses the Sx
 *	library package.
 *
 * Copyright (C) 1986, 1987, 1988 Regents of the University of California.
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/cmds/mx/RCS/mx.c,v 1.27 90/11/11 11:52:00 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#define Time SpriteTime
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mxInt.h"

/*
 * Forward references to other procedures defined in this file:
 */

static void		MainSignalProc();

/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	The main program for mx.  Initialize, load file, create window,
 *	and loop processing events.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Almost anything can happen.  Depends on commands that user types.
 *
 *----------------------------------------------------------------------
 */

main(argc, argv)
    int argc;			/* Count of command-line arguments. */
    char **argv;		/* Array of strings containing args. */
{
    int detach = 0;
    int signalPid = 0;
    int i, put;
    char *p;
    Tcl_Interp *interp;		/* Initial interpreter used for error
				 * reporting before there's a window open
				 * within an interpreter of its own to use. */

    /*
     * If running under Sprite, tell the memory allocator to be paranoid
     * about freeing the same block twice.
     */

#ifdef sprite
    extern int memAllowFreeingFree;
    memAllowFreeingFree = 0;
#endif

    /*
     *--------------------------------------------------------------
     * Parse some of the command line arguments, leaving many others
     * to get parsed by Mx_OpenCmd.
     *--------------------------------------------------------------
     */

    p = rindex(argv[0], '/');
    if (p == NULL) {
	p = argv[0];
    } else {
	p++;
    }
    if (strcmp(p, "mxsync") == 0) {
	detach = 0;
    } else {
	detach = 1;
    }
    interp = Tcl_CreateInterp();
    for (i = 1, put = 1; i < argc; i++) {
	if (strcmp(argv[i], "-D") == 0) {
	    detach = 0;
	} else if (strcmp(argv[i], "-s") == 0) {
	    i++;
	    if (i >= argc) {
		fprintf(stderr,
			"Mx: argument for \"%s\" switch missing.\n",
			argv[i-1]);
		exit(1);
	    }
	    signalPid = atoi(argv[i]);
	} else {
	    argv[put] = argv[i];
	    put++;
	}
    }
    argc = put;
    if (detach) {
	Proc_Detach(0);
    }
    Sx_SetErrorHandler();
    (void) signal(SIGINT, MainSignalProc);
    (void) signal(SIGQUIT, MainSignalProc);
    (void) signal(SIGTERM, MainSignalProc);

    /*
     *------------------------------------------------------
     * Create the (first) window.
     *------------------------------------------------------
     */

    if (Mx_OpenCmd((MxWindow *) NULL, interp, argc, argv) != TCL_OK) {
	fprintf(stderr, "Mx quitting: %s\n", interp->result);
	exit(1);
    }

    /*
     *--------------------------------------------
     * Enter a loop reading and processing events.
     *--------------------------------------------
     */

    while (1) {
	XEvent event;
	XNextEvent(mx_Display, &event);

	/*
	 * Part of Mike's hack.  Remove when Mike's gone.
	 */
	 
	if ((signalPid != 0) && (event.type == Expose)) {
	    kill(signalPid, SIGINT);
	    signalPid = 0;
	}
	Sx_HandleEvent(&event);
	if (mx_FileCount == 0) {
	    exit(0);
	}
	Mx_Update();
    }
}

/*
 *----------------------------------------------------------------------
 *
 * MainSignalProc --
 *
 *	Invoked in response to fatal signals.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Clean up Mx windows, then exit.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */

static void
MainSignalProc(sigNum, sigCode)
    int sigNum;			/* Signal number (major class). */
    int sigCode;		/* Signal code (minor class). */
{
    static struct sigvec action = {SIG_DFL, 0, 0};
    struct sigvec old;

    sigvec(sigNum, &action, &old);
    Mx_Cleanup();
    kill(getpid(), sigNum);
}
