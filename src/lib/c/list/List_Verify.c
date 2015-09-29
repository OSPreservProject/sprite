/* 
 * List_Verify.c --
 *
 *	Description.
 *
 * Copyright 1992 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that this copyright
 * notice appears in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/lib/forms/RCS/proto.c,v 1.6 92/03/02 15:29:56 bmiller Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include "list.h"

#define LIST_NIL ((List_Links *) NIL)

/*
 *----------------------------------------------------------------------
 *
 * List_Verify --
 *
 *	Verifies that the given list isn't corrupted. The descriptionPtr
 *	should be at least 80 characters.
 *
 * Results:
 *	SUCCESS if the list looks ok, FAILURE otherwise. If FAILURE is
 *	returned then a description is returned in the descriptionPtr
 *	array.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
List_Verify(headerPtr, descriptionPtr)
    List_Links	*headerPtr; 	/* Header of list to check. */
    char	*descriptionPtr;/* Description of what went wrong. */
{
    List_Links	*itemPtr;
    List_Links	*prevPtr;
    List_Links	*nextPtr;
    int		index;;

    if ((List_Prev(headerPtr) == LIST_NIL) || (List_Prev(headerPtr) == NULL)) {
	sprintf(descriptionPtr,
	    "Header prevPtr is bogus: 0x%x.\n", List_Prev(headerPtr));
	return FAILURE;
    }
    if ((List_Next(headerPtr) == LIST_NIL) || (List_Next(headerPtr) == NULL)) {
	sprintf(descriptionPtr, 
	    "Header nextPtr is bogus: 0x%x.\n", List_Next(headerPtr));
	return FAILURE;
    }
    itemPtr = List_First(headerPtr);
    prevPtr = headerPtr;
    index = 1;
    while(List_IsAtEnd(headerPtr, itemPtr)) {
	if (List_Prev(itemPtr) != prevPtr) {
	    sprintf(descriptionPtr, 
		"Item %d doesn't point back at previous item.\n", index);
	    return FAILURE;
	}
	nextPtr = List_Next(itemPtr);
	if ((nextPtr == LIST_NIL) || (nextPtr == NULL)) {
	    sprintf(descriptionPtr, 
		"Item %d nextPtr is bogus: 0x%x.\n", index, nextPtr);
	    return FAILURE;
	}
	if (List_Prev(nextPtr) != itemPtr) {
	    sprintf(descriptionPtr,
		"Next item doesn't point back at item %d.\n", index);
	    return FAILURE;
	}
	prevPtr = itemPtr;
	itemPtr = nextPtr;
	index++;
    }
    return SUCCESS;
}

