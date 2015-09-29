/* 
 * strtok.c --
 *
 *	Source code for the "strtok" library routine.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/string/RCS/strtok.c,v 1.1 89/03/22 16:08:00 rab Exp $";
#endif

#ifndef __STDC__
#define const
#endif

#include <string.h>

/*
 *----------------------------------------------------------------------
 *
 * strtok --
 *
 *  	Split a string up into tokens
 *
 * Results:
 *      If the first argument is non-NULL then a pointer to the
 *      first token in the string is returned.  Otherwise the
 *      next token of the previous string is returned.  If there
 *      are no more tokens, NULL is returned.
 *
 * Side effects:
 *	Overwrites the delimiting character at the end of each token
 *      with '\0'.
 *
 *----------------------------------------------------------------------
 */

char *
strtok(s, delim)
    char *s;            /* string to search for tokens */
    const char *delim;  /* delimiting characters */
{
    static char *lasts;
    register int ch;

    if (s == 0)
	s = lasts;
    do {
	if ((ch = *s++) == '\0')
	    return 0;
    } while (strchr(delim, ch));
    --s;
    lasts = s + strcspn(s, delim);
    if (*lasts != 0)
	*lasts++ = 0;
    return s;
}

