/* 
 * iostats.c --
 *
 *	Measure disk usage.
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
static char rcsid[] = "$Header: /sprite/src/cmds/iostats/RCS/iostats.c,v 1.1 90/10/05 12:54:03 mendel Exp Locker: mendel $ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include <status.h>
#include <option.h>
#include <stdlib.h>
#include <stdio.h>
#include <sysStats.h>

/*
 * Command line options.
 */

int	numDisks = -1;

Option optionArray[] = {
    {OPT_INT, "numDisks", (Address) &numDisks, 
	"\tPrint out disk usage for this number of disks."},
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
    ReturnStatus status = SUCCESS;	/* status of system calls */
    Sys_DiskStats	*statsPtr;
    char		*memPtr;

    argc = Opt_Parse(argc, argv, optionArray, numOptions);

    if (numDisks == -1) {	/* unset */
	numDisks = 100;
    }
    memPtr =  malloc(numDisks * sizeof (Sys_DiskStats));
    bzero(memPtr, numDisks * sizeof (Sys_DiskStats));

    status = Sys_Stats(SYS_DISK_STATS, numDisks, memPtr);
    statsPtr = (Sys_DiskStats *) memPtr;

    while (statsPtr->name[0] != 0) {
	printf("name: %s\n", statsPtr->name);
	printf("controllerID: %d\n", statsPtr->controllerID);
	printf("numSamples: %d\n", statsPtr->numSamples);
	printf("idleCount: %d\n", statsPtr->idleCount);
	printf("diskReads: %d\n", statsPtr->diskReads);
	printf("diskWrites: %d\n", statsPtr->diskWrites);
	statsPtr++;
    }
    exit(status);
}
