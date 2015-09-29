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
static char rcsid[] = "$Header: /sprite/src/lib/c/string/RCS/strsep.c,v 1.2 91/08/05 16:48:37 shirriff Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <stdio.h>

/*
 *----------------------------------------------------------------------
 *
 * strsep --
 *
 *      Extract tokens separated by single separators (similar to strtok).
 *
 * Results:
 *      Returns pointer to next token in the string.
 *
 * Side effects:
 *      Sets token to NULL.  Keeps track of str.
 *
 *----------------------------------------------------------------------
 */

char *
strsep(s, delim)
	register char *s, *delim;
{
	register char *spanp;
	register int c, sc;
	static char *last;
	char *tok;

	if (s == NULL && (s = last) == NULL)
		return(NULL);

	/*
	 * Scan token (scan for delimiters: s += strcspn(s, delim), sort of).
	 * Note that delim must have one NUL; we stop if we see that, too.
	 */
	for (tok = s;; ++s) {
		c = *s;
		spanp = delim;
		do {
			if ((sc = *spanp++) == c) {
				if (c == 0) {
					last = NULL;
					return(tok == s ? NULL : tok);
				}
				*s++ = '\0';
				last = s;
				return(tok);
			}
		} while (sc);
	}
	/* NOTREACHED */
}
