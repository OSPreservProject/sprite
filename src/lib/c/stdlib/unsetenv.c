/* 
 * unsetenv.c --
 *
 *	Procedure to simulate cshell unsetenv call.
 *
 * Copyright (C) 1986 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/lib/c/stdlib/RCS/unsetenv.c,v 1.5 89/03/22 00:47:37 rab Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <sprite.h>
#include <stdlib.h>

extern char **environ;
extern char *malloc();


/*
 *----------------------------------------------------------------------
 *
 * unsetenv --
 *
 *	Unsets the given variable.  Note: taken from MH, temporary, doesn't
 *      free space.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	"Name" will no longer exist as an environment variable.
 *
 *----------------------------------------------------------------------
 */

void
unsetenv(name)
    char	*name;	/* Name of variable. */
{
    register char **envPtr;
    register char **newEnvPtr;
    register char *charPtr;
    register char *namePtr;
    register Boolean found = FALSE;

    for (envPtr = environ; *envPtr != NULL; envPtr++) {
	for (charPtr = *envPtr, namePtr = name;
	     *charPtr == *namePtr; namePtr++) {
	     charPtr++;
	     if (*charPtr == '=') {
		 found = TRUE;
		 break;
	     }
	 }
	if (found) {
	    break;
	}
    }
    if (!found) {
	return;
    }
    for (newEnvPtr = envPtr + 1; *newEnvPtr; newEnvPtr++) {
    }
    newEnvPtr--;
    *envPtr = *newEnvPtr;
    *newEnvPtr = NULL;
}
