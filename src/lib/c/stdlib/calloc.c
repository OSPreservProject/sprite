/* 
 * calloc.c --
 *
 *	Source code for the "calloc" library procedure.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/stdlib/RCS/calloc.c,v 1.2 88/07/29 17:04:26 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include <bstring.h>
#include "stdlib.h"

/*
 *----------------------------------------------------------------------
 *
 * calloc --
 *
 *	Allocate a zero-filled block of storage.
 *
 * Results:
 *	The return value is a pointer to numElems*elemSize bytes of
 *	dynamically-allocated memory, all of which have been
 *	initialized to zero.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

char *
calloc(numElems, elemSize)
    unsigned int numElems;	/* Number of elements to allocate. */
    unsigned int elemSize;	/* Size of each element. */
{
    unsigned int totalSize;
    char *result;

    totalSize = numElems*elemSize;
    result = malloc(totalSize);
    bzero(result, (int) totalSize);
    return (char *) result;
}
