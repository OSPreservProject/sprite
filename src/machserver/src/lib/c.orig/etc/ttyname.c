/* 
 * ttyname.c --
 *
 *	Source code for the ttyname library procedure.
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
static char rcsid[] = "$Header: ttyname.c,v 1.2 88/08/25 17:21:00 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

/*
 *----------------------------------------------------------------------
 *
 * ttyname --
 *
 *	This is a Sprite replacement for the UNIX ttyname library
 *	procedure, which supposedly returns the name of the tty
 *	associated with a given file descriptor.  Sprite doesn't
 *	have a /dev with all ttys in it, so it's pretty hard to
 *	emulate the right behavior.
 *
 * Results:
 *	The return value is the name of the file corresponding
 *	to filedes, if we could figure which one it was, or 0
 *	otherwise.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
char *
ttyname(filedes)
    int filedes;		/* File for which to find tty name. */
{
    struct stat buf1, buf2;
    char *tty;

    /*
     * See if the file pointed to by the "TTY" environment variable
     * happens to be the same as filedes.  If not, then return NULL.
     */

    tty = getenv("TTY");
    if ((tty == (char *) 0) || (stat(tty, &buf1) != 0)) {
	return (char *) 0;
    }
    if (fstat(filedes, &buf2) != 0) {
	return (char *) 0;
    }
    if ((buf1.st_dev == buf2.st_dev) && (buf1.st_ino == buf2.st_ino)
	    && (buf1.st_serverID == buf2.st_serverID)) {
	return tty;
    }
    return (char *) 0;
}
