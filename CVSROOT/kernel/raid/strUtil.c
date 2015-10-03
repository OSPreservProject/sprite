/* 
 * strUtil.c --
 *
 * Copyright 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif /* not lint */

#include <ctype.h>
#include <strings.h>
#include "sprite.h"
#include "fs.h"


/*
 *----------------------------------------------------------------------
 *
 * ScanLine --
 *
 *	Copy first line in ps1 to s2 replacing \n with \0.
 *	Change ps1 to point to the 1st character of second line.
 *
 * Results:
 *	NIL if no line is found s2 otherwise.
 *
 * Side effects:
 *	ps1 points to next line.
 *
 *----------------------------------------------------------------------
 */

char *
ScanLine(ps1, s2)
    char **ps1, *s2;
{
    char *s1, *retstr;

    retstr = s2;
    s1 = *ps1;
    while ( *s1 != '\0' && *s1 != '\n' ) {
        *s2++ = *s1++;
    }
    if ( *s1 == '\0' ) {
        return ( (char *) NIL );
    }
    *s2 = '\0';
    *ps1 = s1+1;
    return ( retstr );
}


/*
 *----------------------------------------------------------------------
 *
 * ScanWord --
 *
 *	Copy first word in *ps1 to s2 (words are delimited by white space).
 *	Change ps1 to point to the 2nd character after the first word
 *	(i.e. skip over the delimiting whitespace).  This allows ps1 and
 *	s2 to initially point to the same character string.
 *
 * Results:
 *	Returns s2 or NIL if no word is found.
 *
 * Side effects:
 *	ps1 points to the first character after the delimiting whitespace.
 *
 *----------------------------------------------------------------------
 */

char *
ScanWord(ps1, s2)
    char **ps1, *s2;
{
    char *s1, *retstr;

    retstr = s2;
    for ( s1 = *ps1; *s1 != '\0' && isspace(*s1); s1++ ) {
    }
    while ( *s1 != '\0' && !isspace(*s1) ) {
        *s2++ = *s1++;
    }
    if ( *s1 == '\0' ) {
        return ( (char *) NIL );
    }
    *s2 = '\0';
    *ps1 = s1+1;
    return ( retstr );
}
