/* 
 * List_Init.c --
 *
 *	Source code for the List_Init library procedure.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/list/RCS/List_Init.c,v 1.3 90/11/27 11:05:41 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include <stdio.h>
#include "list.h"

extern void panic();

/*
 * ----------------------------------------------------------------------------
 *
 * List_Init --
 *
 *	Initialize a header pointer to point to an empty list.  The List_Links
 *	structure must already be allocated.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The header's pointers are modified to point to itself.
 *
 * ----------------------------------------------------------------------------
 */
void
List_Init(headerPtr)
    register List_Links *headerPtr;  /* Pointer to a List_Links structure 
					to be header */
{
    if (headerPtr == (List_Links *) NIL || !headerPtr) {
	panic("List_Init: invalid header pointer.\n");
    }
    headerPtr->nextPtr = headerPtr;
    headerPtr->prevPtr = headerPtr;
}
