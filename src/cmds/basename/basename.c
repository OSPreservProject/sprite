/*
 * basename.c --
 *
 *  Basename deletes any prefix ending with '/' from its first
 *  argument, and also deletes any suffix from the first argument
 *  that matches the second argument.  The resulting string is
 *  printed to the standard output.
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
    char *s;
    int len;

    if (argc < 2) {
	fputs("usage: basename string [suffix]\n", stderr);
	exit(1);
    }
    if ((s = strrchr(argv[1], '/')) == NULL) {
	s = argv[1];
    } else {
	++s;
    }
    if (argc > 2) {
	if ((len = strlen(s) - strlen(argv[2])) >= 0) {
	    if (strcmp(s + len, argv[2]) == 0)
		s[len] = 0;
	}
    }
    puts(s);
    exit(0);
}

