/* 
 * getchar.c --
 *
 *	Source code for a procedural version of the getchar macro.
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
static char rcsid[] = "$Header: proto.c,v 1.2 88/03/11 08:39:08 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include <stdio.h>
#undef getchar

/*
 *----------------------------------------------------------------------
 *
 * getchar --
 *
 *	This procedure returns the next input character from stdin.
 *	It's a procedural version of the getchar macro.
 *
 * Results:
 *	The .The result is an integer value that is equal to EOF if an
 *	end of file or error condition was encountered on stdin.
 *	Otherwise, it is the value of the next input character from
 *	stdin.
 *
 * Side effects:
 *	A character is removed from stdin.
 *
 *----------------------------------------------------------------------
 */

int
getchar()
{
    return getc(stdin);
}
