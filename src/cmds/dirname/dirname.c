/*
 * dirname.c --
 *
 *  Dirname treats its first argument as a filename, and returns the
 *  name of the directory prortion.  It strips off anything following
 *  the last '/'.  The resulting string is printed to the standard output.
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
static char rcsid[] = "$Header: /a/newcmds/basename/RCS/basename.c,v 1.2 88/12/13 11:05:07 rab Exp $";
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void
main(argc, argv)
    int argc;
    char **argv;
{
    char *dirname, *slash;
    int len;

    if (argc < 2) {
	fputs("usage: dirname string\n", stderr);
	exit(EXIT_FAILURE);
    }
    dirname = argv[1];
    if ((slash = strrchr(dirname, '/')) == NULL) {
	dirname = ".";
    } else {
	*slash = '\0';
    }
    puts(dirname);
    exit(EXIT_SUCCESS);
}

