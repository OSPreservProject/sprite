/* 
 * printenv.c --
 *
 *	Program to print out the values of all environment variables.
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
static char rcsid[] = "$Header: /sprite/src/cmds/printenv/RCS/printenv.c,v 1.2 90/01/17 17:40:24 shirriff Exp $ SPRITE (Berkeley)";
#endif not lint

#include <stdio.h>

extern char **environ;


/*
 *----------------------------------------------------------------------
 *
 * CompareProc --
 *
 *	Comparison procedure for sorting.  Given pointer to two
 *	string pointers, tell which string is "earlier".
 *
 * Results:
 *	Returns < 0, 0, or > 0 depending on whether a is alphabetically
 *	earlier than, equal to, or later than b.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
CompareProc(a, b)
    char **a, **b;		/* Pointers to two string pointers. */
{
    return strcmp(*a, *b);
}

/*
 *----------------------------------------------------------------------
 *
 * VariableMatch --
 *
 * Results:
 *	Returns 1 if the two strings passed are the same up to an
 * = and/or \0, and 0 otherwise.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
VariableMatch(a, b)
    char *a, *b;
{
    while (*a == *b) {
	a++;
	b++;
    }
    return ((*a == '=' || *a == '\0') && (*b == '=' || *b == '\0'));
}
	
 * 
/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	Main program for printenv.  Sort the environment variables
 *	alphabetically, then print them out.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Stuff gets printed.
 *
 *----------------------------------------------------------------------
 */

    /* ARGSUSED */

main(argc, argv)
    int argc;			/* Count of number of arguments. */
    char **argv;		/* Argument values. */
{
    int envSize, i;

    if (argc > 2) {
	fprintf(stderr, "Usage: %s [environment variable]\n", argv[0]);
    }

    /*
     * See how many environment variables there are.
     */

    for (envSize = 0; ; envSize++) {
	if (environ[envSize] == NULL) {
	    break;
	}
    }

    /*
     * Sort 'em.
     */

    if (argc == 1) {	/* print all variables */
	qsort((char *) environ, envSize, sizeof(char *), CompareProc);

	for (i = 0; i < envSize; i++) {
	    printf("%s\n", environ[i]);
	}
    }
    else {	/* print just variable requested */
	for (i = 0; i < envSize; i++)
	    if (VariableMatch (environ[i], argv[1])) {
		printf ("%s\n", strchr (environ[i], '=') + 1);
		break;
	    }
    }

    exit(0);
}
