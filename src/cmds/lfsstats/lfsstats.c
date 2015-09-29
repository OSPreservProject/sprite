/* 
 * showStats.c --
 *
 *	Show the stats of an LFS file system
 *
 * Copyright 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/admin/lfsstats/RCS/lfsstats.c,v 1.2 92/04/14 14:12:23 mendel Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include "lfslib.h"
#ifdef _HAS_PROTOTYPES
#include <varargs.h>
#endif
#include <stdio.h>
#include <sys/types.h>
#include <sys/file.h>

#include <sprite.h>
#include <unistd.h>
#include <option.h>
#include <stdlib.h>
#include <string.h>

int	blockSize = 512;

char	*deviceName;

Option optionArray[] = {
    {OPT_DOC, (char *) NULL,  (char *) NULL,
	"This program shows the LfsStats structure of an LFS file system.\n Synopsis: \"lfsstats deviceName\""},
};
/*
 * Forward routine declartions. 
 */

static void ShowStats _ARGS_((Lfs_Stats *statsPtr));

/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	Main routine of lfsstats - parse arguments and do the work.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
main(argc,argv)
    int	argc;
    char *argv[];
{
    Lfs	*lfsPtr;



    argc = Opt_Parse(argc, argv, optionArray, Opt_Number(optionArray), 0);
    if (argc != 2) { 
         Opt_PrintUsage(argv[0], optionArray, Opt_Number(optionArray));
	 exit(1);
    } else {
	deviceName = argv[1];
    }
    lfsPtr = LfsLoadFileSystem(argv[0], deviceName, blockSize, 
			LFS_SUPER_BLOCK_OFFSET,
				O_RDONLY);
    if (lfsPtr == (Lfs *) NULL) {
	exit(1);
    }

    ShowStats(&lfsPtr->stats);
    exit(0);
    return 0;
}
static void
ShowStats(statsPtr)
    Lfs_Stats *statsPtr;
{
#include "statprint.h"
}
