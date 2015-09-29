/* 
 * putenv.c --
 *
 *	Puts a string of the form `name=value' into the environment.
 *
 * Copyright 1991 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that this copyright
 * notice appears in all copies.  The University of California
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


/*
 *----------------------------------------------------------------------
 *
 * putenv --
 *
 *	Puts a string of the form `name=value' into the environment.
 *
 * Results:
 *	Returns 0 on success, otherwize returns -1.
 *
 * Side effects:
 *	Changes environment.
 *
 *----------------------------------------------------------------------
 */

int
putenv(string)
    char *string;
{
    char *name;
    char *value;

    if ((name = malloc(strlen(string) + 1)) == NULL) {
	return -1;
    }
    strcpy(name, string);
    if ((value = strchr(name, '=')) == NULL) {
	free(name);
	return -1;
    }
    *value = '\0';
    ++value;
    setenv(name, value);
    free(name);
    return 0;
}

