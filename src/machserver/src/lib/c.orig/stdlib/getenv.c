/* 
 * getenv.c --
 *
 *	Source code for the "getenv" library procedure.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/stdlib/RCS/getenv.c,v 1.2 89/03/22 00:47:13 rab Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <stdio.h>
#include <stdlib.h>

extern char **environ;

/*
 *----------------------------------------------------------------------
 *
 * getenv --
 *
 *	Locate an environment variable by a given name.
 *
 * Results:
 *	The return value is a pointer to the value associated with
 *	name, or 0 if there is no value registered for name.  The return
 *	value points into environment storage, which is only guaranteed
 *	to persist until the next call to setenv.
 *
 * Side effects
 *	None.
 *
 *----------------------------------------------------------------------
 */

char *
getenv(name)
    char	*name;		/* Name to retrieve value for. */
{
    char **envPtr;		/* pointer into list of environ. vars. */
    register char *charPtr;	/* point into one environment variable */
    register char *namePtr;	/* pointer into name */

    for (envPtr = environ; *envPtr != NULL; envPtr++) {
	for (charPtr = *envPtr, namePtr = name; *charPtr == *namePtr;
		charPtr++, namePtr++) {
	    if (*charPtr == '=') {
		break;
	    }
	}
	if ((*charPtr == '=') && (*namePtr == NULL)) {
	    return charPtr+1;
	}
    }
    return NULL;
}
