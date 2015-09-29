/* 
 * sethostname.c --
 *
 *	Procedure to simulate Unix sethostname system call
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
static char rcsid[] = "$Header: /sprite/src/lib/c/unixSyscall/RCS/sethostname.c,v 1.1 92/06/16 11:21:13 jhh Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <sys/param.h>
#include <sys/errno.h>
#include "compatInt.h"

/*
 *----------------------------------------------------------------------
 *
 * sethostname --
 *
 *	Sets the host name. 
 *
 * Results:
 *	0 is returned if the call was completed successfully.
 *	Otherwise, -1 is returned and errno gives more information.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
sethostname(name, namelen)
    char	*name;		/* New name of host. */
    int		namelen;	/* Length of name. */
{
    char		tmp[MAXHOSTNAMELEN];
    ReturnStatus	status;

    if (namelen > MAXHOSTNAMELEN) {
	errno = EINVAL;
	return UNIX_ERROR;
    }
    /*
     * We allow names to lacking a terminating null character because the
     * Unix man pages are ambiguous as to whether it is needed.
     */
    if (name[namelen-1] != '\0') {
	if (namelen < MAXHOSTNAMELEN) {
	    if (name[namelen] != '\0') {
		bcopy(name, tmp, namelen);
		tmp[namelen] = '\0';
		name = tmp;
	    }
	} else {
	    errno = EINVAL;
	    return UNIX_ERROR;
	}
    }
    status = Sys_SetHostName(name);
    if (status != SUCCESS) {
	errno = Compat_MapCode(status);
	return UNIX_ERROR;
    }
    return 0;
}
