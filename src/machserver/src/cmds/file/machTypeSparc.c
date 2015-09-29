/* 
 * machTypeSparc.c --
 *
 *	Contains the machine specific routine for determining if
 *      a file is an object file for a Sun 4.
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
static char rcsid[] = "$Header: /sprite/src/attcmds/file/RCS/machTypeSparc.c,v 1.3 90/10/19 15:25:45 jhh Exp Locker: shirriff $";
#endif /* not lint */

#include <sun4.md/sys/exec.h>
#include <sun4.md/a.out.h>
#include "file.h"


/*
 *----------------------------------------------------------------------
 *
 * machTypeSparc --
 *
 *	Determine if the file is a sparc-specific file type.
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
machTypeSparc(bufferSize, buffer, magic, syms, other)
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
	if (header->a_machtype == M_SPARC) {
	    /*
	     * We can't just use the declared bitfield to check for
	     * dynamic, since the bitorder is different on different
	     * machines.
	     */
	    if (((unsigned char *)header)[0]&0x80) {
		*other = "dynamically linked";
	    }
	    return "sparc";
	}
    }
    if (FMT_SPARC_FORMAT != hostFmt) {
	status = Fmt_Convert("{b1b1h1w7}", FMT_SPARC_FORMAT, &bufferSize,
	    (char *) buffer, hostFmt, &swappedSize, swappedHeader);
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
	    if (header->a_machtype == M_SPARC) {
		if (((unsigned char *)header)[0]&0x80) {
		    *other = "dynamically linked";
		}
		return "sparc";
	    }
	}
    }
    return NULL;
}
