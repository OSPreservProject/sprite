/*
 * Copyright (c) 1987 University of Maryland Department of Computer Science.
 * All rights reserved.  Permission to copy for any purpose is hereby granted
 * so long as this copyright notice remains intact.
 */

/*
 * Save a string in managed memory.
 */

char	*malloc(), *realloc();
extern int errno;

char *
strsave(s)
	register char *s;
{
	register int l = strlen(s) + 1;
	register char *p = malloc((unsigned) l);

	if (p == 0)
		error(1, errno, "no room for %d bytes of string", l);
	bcopy(s, p, l);
	return (p);
}
