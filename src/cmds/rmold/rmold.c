/* 
 * rmold.c --
 *
 *	Program to remove files older than a certain number of days.
 *
 * Copyright 1990 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/admin/rmold/RCS/rmold.c,v 1.2 91/03/12 13:52:57 kupfer Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#ifndef ds3100
#ifndef lint
#define _HAS_PROTOTYPES
#define _HAS_VOIDPTR
#endif
#endif

#include <sprite.h>
#include <errno.h>
#include <option.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

/*
 * Command-line options:
 */

int delete = 1;
int mod = 0;		/* 1 means use modify time for deletion, 0 means
			 * use access time. */
char *timesDirectory = ".";	/* directory to get mod/access times from */

Option optionArray[] = {
    OPT_DOC,	(char *) NULL,	(char *) NULL,
	    "This program deletes files that haven't been modifed recently.\n Synopsis: \"rmold [-print] numDays file file ...\"\n Command-line switches are:",
    OPT_TRUE,	"mod",		(char *) &mod,
	    "Use modify time instead of access time",
    OPT_FALSE,	"print",	(char *) &delete,
	    "Don't delete, just print file names that would be deleted",
    OPT_STRING, "timesFrom",	(char *)&timesDirectory,
	    "Name the directory to determine the access or modify time.",
};

static char *AllocSpace _ARGS_ ((int numNames, char *namesArray[]));


int
main(argc, argv)
    int argc;
    char **argv;
{
    int i;
    u_long cutoffTime;
    struct timeval now;
    struct timezone dummy;
    char *end;
    struct stat buf;
    char *fileName;		/* file name for getting access/mod time */

    /*
     * Check arguments and compute the cutoff time.
     */

    argc = Opt_Parse(argc, argv, optionArray, Opt_Number(optionArray), 0);
    if (argc < 3) {
	fprintf(stderr, "Usage: %s numDays file file ...\n", argv[0]);
	exit(1);
    }
    i = strtoul(argv[1], &end, 0);
    if (end == argv[1]) {
	badArg:
	fprintf(stderr, "Bad \"numDays\" argument \"%s\"\n", argv[1]);
	exit(1);
    }
    i *= 24*3600;
    gettimeofday(&now, &dummy);
    if (i > now.tv_sec) {
	goto badArg;
    }
    cutoffTime = now.tv_sec - i;

    fileName = AllocSpace(argc-2, argv+2);

    /*
     * Scan through all of the files and delete any that haven't been
     * accessed since the cutoff time.
     */

    for (i = 2; i < argc; i++) {
	strcpy(fileName, timesDirectory);
	strcat(fileName, "/");
	strcat(fileName, argv[i]);
	if (stat(fileName, &buf) != 0) {
	    fprintf(stderr, "Couldn't stat \"%s\": %s\n",
		    fileName, strerror(errno));
	    continue;
	}
	if (mod) {
	    if (buf.st_mtime > cutoffTime) {
		continue;
	    }
	} else {
	    if (buf.st_atime > cutoffTime) {
		continue;
	    }
	}
	if (delete && (unlink(argv[i]) != 0)) {
	    fprintf(stderr, "Couldn't remove \"%s\": %s\n",
		    argv[i], strerror(errno));
	    continue;
	}
	printf("%s\n", argv[i]);
    }

    exit(0);
    return 0;			/* suppress -Wall complaint */
}


/*
 *----------------------------------------------------------------------
 *
 * AllocSpace --
 *
 *	Allocate a character buffer large enough for all the file names.
 *
 * Results:
 *	A character array large enough to hold the concatentation of 
 *	the search directory, "/", and any of the arguments.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static char *
AllocSpace(numNames, namesArray)
    int numNames;
    char *namesArray[];		/* list of file names */
{
    int i;			/* index into argv */
    int maxLength = 0;		/* longest length in namesArray */

    for (i = 0; i < numNames; i++) {
	if (strlen(namesArray[i]) > maxLength) {
	    maxLength = strlen(namesArray[i]);
	}
    }

    return malloc(strlen(timesDirectory) + strlen("/") + maxLength + 1);
}
