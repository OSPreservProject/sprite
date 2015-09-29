/* 
 * cfree.c --
 *
 *	Source code for the "cfree" library procedure.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/stdlib/RCS/cfree.c,v 1.1 88/06/27 17:24:06 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include <stdlib.h>

/*
 *----------------------------------------------------------------------
 *
 * cfree --
 *
 *	Free a block of storage allocated with calloc.  Actually,
 *	this routine is identical to free, and isn't really even
 *	needed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Storage gets freed.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */

cfree(block, numElements, size)
    char *block;		/* Address of block to be freed. */
    unsigned int numElements;	/* Number of elements in block (not used). */
    unsigned int size;		/* Size of each element (not used). */
{
    free(block);
}
