/*
 * Copyright (c) 1988 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/lib/c/string/RCS/strdup.c,v 1.3 91/08/05 16:50:17 kupfer Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <bstring.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>

/*
 *----------------------------------------------------------------------
 *
 * strdup --
 *
 *      Malloc and copy a string.
 *
 * Results:
 *      Returns pointer to the copy of the string.
 *
 * Side effects:
 *      Mallocs space for the new string.
 *
 *----------------------------------------------------------------------
 */

char *
strdup(str)
	char *str;
{
	int len;
	char *copy, *malloc();

	len = strlen(str) + 1;
	if (!(copy = malloc((u_int)len)))
		return((char *)NULL);
	bcopy(str, copy, len);
	return(copy);
}
