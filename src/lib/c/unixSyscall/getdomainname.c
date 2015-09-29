/* 
 * getdomainname.c --
 *
 *	Returns the domain name.  This doesn't really work on sprite
 *      so we just return "spicesNvices" which is the right name for
 *      spurnet at Berkeley.
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
static char rcsid[] = "$Header$";
#endif /* not lint */

#define DOMAIN      "spicesNvices"


/*
 *----------------------------------------------------------------------
 *
 * getdomainname --
 *
 *	Returns the domain name.  This doesn't really work on sprite
 *      so we just return "spicesNvices" which is the right name for
 *      spurnet at Berkeley.
 *
 * Results:
 *	Returns 0 on success, -1 otherwise.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
getdomainname(name, namelen)
    char *name;
    int namelen;
{
    if (namelen <= strlen(DOMAIN)) {
	return -1;
    }
    strcpy(name, DOMAIN);
    return 0;
}

