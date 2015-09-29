/* 
 * machTypeSymm.c --
 *
 *	Contains the machine specific routine for determining if
 *      a file is an object file for a Sun 2 or a Sun 3.
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
static char rcsid[] = "$Header: /sprite/src/attcmds/file/RCS/machTypeSymm.c,v 1.3 90/10/24 22:29:40 shirriff Exp Locker: shirriff $";
#endif /* not lint */

#include "symmExec.h"
#include "file.h"


/*
 *----------------------------------------------------------------------
 *
 * machTypeSymm --
 *
 *	Determine if the file is a Symm-specific file type.
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
machTypeSymm(bufferSize, buffer, magic, syms, other)
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
    if (header->a_magic == OMAGIC) {
	*magic = 0407;
	*syms = header->a_syms;
	return "symm";
    } else if (header->a_magic == ZMAGIC) {
	*magic = 0413;
	*syms = header->a_syms;
	return "symm";
    } else if (header->a_magic == SMAGIC) {
	*magic = 0413;
	*syms = header->a_syms;
	return "Dynix symm";
    }
    if (FMT_SYM_FORMAT != hostFmt) {
	status = Fmt_Convert("{h2w7}", FMT_SYM_FORMAT, &bufferSize, 
		    (char *) buffer, hostFmt, &swappedSize, swappedHeader);
	if (status) {
#ifndef KERNEL
	    fprintf(stderr, "Fmt_Convert returned %d.\n", status);
#endif
	    return NULL;
	}
	header = (struct exec *) swappedHeader;
	if (header->a_magic == OMAGIC) {
	    *magic = 0407;
	    *syms = header->a_syms;
	    return "symm";
	} else if (header->a_magic == ZMAGIC) {
	    *magic = 0413;
	    *syms = header->a_syms;
	    return "symm";
	} else if (header->a_magic == SMAGIC) {
	    *magic = 0413;
	    *syms = header->a_syms;
	    return "Dynix symm";
	}
    }
    return NULL;
}
