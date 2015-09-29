/* 
 * machTypeSpur.c --
 *
 *	Contains the machine specific routine for determining if
 *      a file is an object file for spur.
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
static char rcsid[] = "$Header: /sprite/src/attcmds/file/RCS/machTypeSpur.c,v 1.2 90/02/07 23:06:50 shirriff Exp Locker: shirriff $";
#endif /* not lint */

#include <spur.md/sys/exec.h>
#include <spur.md/a.out.h>
#include "file.h"


/*
 *----------------------------------------------------------------------
 *
 * machTypeSpur --
 *
 *	Determine if the file is a spur-specific file type.
 *
 * Results:
 *	If successful, return the name of the machine.  Otherwise
 *      return NULL.
 *
 * Side effects:
 *	Modifies the `magic' number.
 *
 *----------------------------------------------------------------------
 */

char *
machTypeSpur(bufferSize, buffer, magic, syms, other)
    int		bufferSize;	/* size of buffer */
    char	*buffer;	/* buffer containing header */
    int         *magic;         /* pointer to magic number */
    int         *syms;          /* pointer to symbols */
    char	**other;	/* other information to return */
{


    struct exec		*header;
    char		swappedHeader[sizeof(*header) * 2];
    int			swappedSize = sizeof(swappedHeader);
    int			status;

    *other = "";

    if (bufferSize < sizeof(struct exec)) {
	return NULL;
    }
    header = (struct exec *) buffer;
    if (!N_BADMAG(*header)) {
	*magic = header->a_magic;
	*syms = header->a_syms;
	return "spur";
    }
    if (FMT_SPUR_FORMAT != hostFmt) {
	status = Fmt_Convert("{w12}", FMT_SPUR_FORMAT, &bufferSize, buffer,
			hostFmt, &swappedSize, swappedHeader);
	if (status) {
#ifndef KERNEL
	    fprintf(stderr, "Fmt_Convert returned %d.\n", status);
#endif
	    return NULL;
	}
	header = (struct exec *) swappedHeader;
	if (!N_BADMAG(*header)) {
	    *magic = header->a_magic;
	    *syms = header->a_syms;
	    return "spur";
	}
    }
    return NULL;
}
