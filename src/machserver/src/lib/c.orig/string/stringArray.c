/* 
 * stringArray.c --
 *
 *	Library functions that create and destroy null-terminated 
 *	arrays of strings.  These routines are compatible with argv, 
 *	host aliases, and other system string arrays.
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
static char rcsid[] = "$Header: /sprite/lib/forms/RCS/proto.c,v 1.5 91/02/09 13:24:44 ouster Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <spriteString.h>


/*
 *----------------------------------------------------------------------
 *
 * String_SaveArray --
 *
 *	Save copies of an array of strings.
 *
 * Results:
 *	Returns a pointer to a null-terminated array of string 
 *	pointers.  The array and the strings themselves are new 
 *	storage, a copy of the given string array.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

char **
String_SaveArray(origPtr)
    char **origPtr;		/* string array to copy */
{
    int numStrings;		/* number of strings in the array */
    int string;			/* index into the array */
    char **newPtr;		/* the result */

    /* 
     * Count the strings in the array.
     */
    for (numStrings = 0; origPtr[numStrings] != NULL; numStrings++) {
	;
    }

    newPtr = (char **)calloc(numStrings+1, sizeof(char *));
    for (string = 0; string < numStrings; string++) {
	newPtr[string] = strdup(origPtr[string]);
    }
    newPtr[numStrings] = NULL;

    return newPtr;
}


/*
 *----------------------------------------------------------------------
 *
 * String_FreeArray --
 *
 *	Free all the storage for a string array.
 *
 * Results:
 *	Returns a nil pointer so that the caller can easily nil out 
 *	the pointer it passed in.
 *
 * Side effects:
 *	The storage is freed.
 *
 *----------------------------------------------------------------------
 */

char **
String_FreeArray(stringsPtr)
    char **stringsPtr;		/* string array to free */
{
    int whichString;		/* index into strings array */

    for (whichString = 0; stringsPtr[whichString] != NULL; whichString++) {
	free(stringsPtr[whichString]);
    }
    free(stringsPtr);

    return NULL;
}
