/* 
 * recovcmd.c --
 *
 *	User interface to recovery related commands.
 *
 * Copyright (C) 1986 Regents of the University of California
 * All rights reserved.
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/cmds/recovcmd/RCS/recovcmd.c,v 1.3 90/04/18 20:18:06 mgbaker Exp Locker: mgbaker $ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "status.h"
#include "option.h"
#include "stdio.h"
#include "sysStats.h"

/*
 * Command line options.
 */

int	recovAbsolutePings = -1;
int	recovStats = -1;
int	printLevel = -1;

Option optionArray[] = {
    {OPT_TRUE, "abson", (Address) &recovAbsolutePings, 
	"\tMake recovery ping interval absolute."},
    {OPT_FALSE, "absoff", (Address) &recovAbsolutePings, 
	"\tMake recovery ping interval relative to when it finishes pinging."},
    {OPT_TRUE, "recovStats", (Address) &recovStats, 
	"\tPrint out recovery statistics."},
    {OPT_INT, "printLevel", (Address) &printLevel, 
	"\tSet recov trace print level (0=off, 1=default, 10=highest)."},
};
int numOptions = sizeof(optionArray) / sizeof(Option);


/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	Collects arguments and branches to the code for the command.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Calls the command.
 *
 *----------------------------------------------------------------------
 */
main(argc, argv)
    int argc;
    char *argv[];
{
    register ReturnStatus status = SUCCESS;	/* status of system calls */

    argc = Opt_Parse(argc, argv, optionArray, numOptions);

    /*
     * Set various recov system flags.
     */
    if (recovAbsolutePings != -1) {
	status = Sys_Stats(SYS_RECOV_ABS_PINGS, recovAbsolutePings, NIL);
    }
    if (printLevel != -1) {
	status = Sys_Stats(SYS_RECOV_PRINT, printLevel, NIL);
    }
#ifdef NOTDEF
    if (recovStats != -1) {
	Recov_Stats	stats;

	status = Sys_Stats(SYS_RECOV_STATS, sizeof (Recov_Stats), &stats)
	if (status != SUCCESS) {
	    fprintf(stderr, "Recov stats failed: error %x.\n", status)
	} else {
	    fprintf(stderr,
		    "Haven't figured out what to do about this option yet.\n");
	}
    }
#endif NOTDEF
    exit(status);
}
