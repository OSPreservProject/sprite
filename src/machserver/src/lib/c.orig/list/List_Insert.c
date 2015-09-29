/* 
 * List_Insert.c --
 *
 *	Source code for the List_Insert library procedure.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/list/RCS/List_Insert.c,v 1.5 90/11/27 11:06:20 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include <stdio.h>
#include "list.h"

extern void panic();

/*
 * ----------------------------------------------------------------------------
 *
 * List_Insert --
 *
 *	Insert the list element pointed to by itemPtr into a List after 
 *	destPtr.  Perform a primitive test for self-looping by returning
 *	failure if the list element is being inserted next to itself.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The list containing destPtr is modified to contain itemPtr.
 *
 * ----------------------------------------------------------------------------
 */
void
List_Insert(itemPtr, destPtr)
    register	List_Links *itemPtr;	/* structure to insert */
    register	List_Links *destPtr;	/* structure after which to insert it */
{
    if (itemPtr == (List_Links *) NIL || destPtr == (List_Links *) NIL
	    || !itemPtr || !destPtr) {
	panic("List_Insert: itemPtr (%x) or destPtr (%x) is NIL.\n",
		  (unsigned int) itemPtr, (unsigned int) destPtr);
	return;
    }
    if (itemPtr == destPtr) {
	panic("List_Insert: trying to insert something after itself.\n");
	return;
    }
    itemPtr->nextPtr = destPtr->nextPtr;
    itemPtr->prevPtr = destPtr;
    destPtr->nextPtr->prevPtr = itemPtr;
    destPtr->nextPtr = itemPtr;
}
