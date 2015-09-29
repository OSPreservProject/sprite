/* 
 * realloc.c --
 *
 *	Source code for the "realloc" library procedure.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/stdlib/RCS/realloc.c,v 1.2 88/07/29 17:04:22 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include <bstring.h>
#include "stdlib.h"

/*
 *----------------------------------------------------------------------
 *
 * realloc --
 *
 *	Change the size of the block referenced by ptr to "size",
 *	possibly moving the block to a larger storage area.
 *
 * Results:
 *	The return value is a pointer to the new area of memory.
 *	The contents of this block will be unchanged up to the
 *	lesserof the new and old sizes.
 *
 * Side effects:
 *	The old block of memory may be released.
 *
 *----------------------------------------------------------------------
 */

char *
realloc(ptr, newSize)
    char 	 *ptr;		/* Ptr to currently allocated block.  If
				 * it's 0, then this procedure behaves
				 * identically to malloc. */
    unsigned int newSize;	/* Size of block after it is extended */
{
    unsigned int curSize;
    char *newPtr;

    if (ptr == 0) {
	return malloc(newSize);
    }
    curSize = Mem_Size(ptr);
    if (newSize <= curSize) {
	return ptr;
    }
    newPtr = malloc(newSize);
    bcopy(ptr, newPtr, (int) curSize);
    free(ptr);
    return(newPtr);
}
