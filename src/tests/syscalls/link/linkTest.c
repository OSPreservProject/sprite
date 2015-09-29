/* 
 * linkTest.c --
 *
 *	Test the link system call.
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
static char rcsid[] = "$Header$";
#endif /* not lint */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int errno;
static int errors;

static char tempfile1[MAXPATHLEN];
static char tempfile2[MAXPATHLEN];

/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *
 * Results:
 *	Exits with a zero exit status if everything works properly,
 *      non-zero otherwise.
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */

void
main(void)
{

    /* 
     *      Create and open a couple temporary files.
     */
    if (tmpnam(tempfile1) != tempfile1) {
	(void) fprintf(stderr, "tmpnam() returned an incorrect value.\n");
	exit(EXIT_FAILURE);
    }
    if (tmpnam(tempfile2) != tempfile2) {
	(void) fprintf(stderr, "tmpnam() returned an incorrect value.\n");
	exit(EXIT_FAILURE);
    }

    /*
     *      Neither name exists, should fail.
     */
    if (link(tempfile1, tempfile2) == 0) {


    }

    /*
     *      create tempfile1
     */
    if ((fd = open(tempfile1, O_RDWR|O_CREAT, 0666)) < 0) {
	(void) fprintf(stderr, "Cannot open %s: %s\n",
	    tempfile1, strerror(errno));
	exit(EXIT_FAILURE);
    }

    /*
     *      First name exists, second does not.  Should succeed.
     */
    if (link(tempfile1, tempfile2) != 0) {
	(void) fprintf(stderr, "link %s to %s failed: %s\n",
	    tempfile1, tempfile2, strerror(errno));


    }



   /*
    *       Open both links.  Write to one, read from the other
    *       and make sure the data matches.
    */


    /*
     *      Both names exist, should fail.
     */
    if (link(tempfile1, tempfile2) {


    }


    /*
     *      Stat both links and make sure the link count is 2
     *      for both links.
     */
    if (stat(tempfile1, &statbuf) {


    }
    if (statbuf.st_nlink != 2) {


    }
    if (stat(tempfile2, &statbuf) {


    }
    if (statbuf.st_nlink != 2) {


    }

    /*
     *      Unlink the first (original) file, then make
     *      sure the second still accesses the file.
     */
    if (unlink(tempfile1)) {


    }


    /*
     *      Try a too long name
     *      Should fail.
     */
    memset(longname, 'a', sizeof(longname) - 1);
    if (link(tempfile1, longname) == 0) {


    }

    /*
     *      Open a file in /usr/tmp and /tmp.
     *      Check the inodes to make sure they are on different
     *      devices.  If they are then try to create a hard link
     *      between them.  Should fail.
     */

    if ((fd1 = open("/tmp/linkTest.tmp", O_RDWR|




    /*
     *      Try a filename containing a character with a high order bit set.
     *      Should fail.
     */







    exit((errors == 0) ? EXIT_SUCCESS: EXIT_FAILURE);
}
















/*
 *----------------------------------------------------------------------
 *
 * testExit --
 *
 *	Fork off a child process that exits with the given exit code.
 *      Wait for the child to exit.  Check the return status and make
 *      sure that it matches the exit status.
 *
 * Results:
 *	none.
 *
 * Side effects:
 *      Prints a message to stderr nad increments `errors' if there is
 *      an error.
 *
 *----------------------------------------------------------------------
 */

static void
testExit(exitCode)
    int exitCode;
{
    int w;
    int child;
    union wait ws;

    switch (child = fork()) {

    case 0:
	exit(exitCode);
	(void) fprintf(stderr, "Exit returned: %s\n", strerror(errno));
	abort();
	(void) fprintf(stderr, "Abort returned: %s\n", strerror(errno));
	for (;;) {
	    continue;
	}

    case -1:
	(void) fprintf(stderr, "Fork failed: %s\n", strerror(errno));
	++errors;
	break;

    default:
	while ((w = wait(&ws)) > 0 && w != child) {
	    continue;
	}
	if (ws.w_retcode != exitCode) {
	    (void) fprintf(stderr, "Exit returned %d, instead of %d.\n",
		ws.w_retcode, exitCode);
	    ++errors;
	}
	break;
    }
    return;
}

