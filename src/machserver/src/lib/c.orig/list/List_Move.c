/* 
 * List_Move.c --
 *
 *	Source code for the List_Move library procedure.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/list/RCS/List_Move.c,v 1.3 90/11/27 11:06:32 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include <stdio.h>
#include "list.h"

extern void panic();

/*
 * ----------------------------------------------------------------------------
 *
 * List_Move --
 *
 *	Move the list element referenced by itemPtr to follow destPtr.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	List ordering is modified.
 *
 * ----------------------------------------------------------------------------
 */
void
List_Move(itemPtr, destPtr)
    register List_Links *itemPtr; /* list element to be moved */
    register List_Links *destPtr; /* element after which it is to be placed */
{
    if (itemPtr == (List_Links *) NIL || destPtr == (List_Links *) NIL
	    || !itemPtr || !destPtr) {
	panic("List_Move: One of the list items is NIL.\n");
    }
    /*
     * It is conceivable that someone will try to move a list element to
     * be after itself.
     */
    if (itemPtr != destPtr) {
	/*
	 * Remove the item.
	 */
        itemPtr->prevPtr->nextPtr = itemPtr->nextPtr;
	itemPtr->nextPtr->prevPtr = itemPtr->prevPtr;
	/*
	 * Insert the item at its new place.
	 */
	itemPtr->nextPtr = destPtr->nextPtr;
	itemPtr->prevPtr = destPtr;
	destPtr->nextPtr->prevPtr = itemPtr;
	destPtr->nextPtr = itemPtr;
    }    
}
