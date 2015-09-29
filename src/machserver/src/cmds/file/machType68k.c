/* 
 * machType68k.c --
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
static char rcsid[] = "$Header: /sprite/src/attcmds/file/RCS/machType68k.c,v 1.3 90/10/19 15:25:42 jhh Exp Locker: shirriff $";
#endif /* not lint */

#include <sun3.md/sys/exec.h>
#include <sun3.md/a.out.h>
#include "file.h"


/*
 *----------------------------------------------------------------------
 *
 * machType68k --
 *
 *	Determine if the file is a 68k-specific file type.
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
machType68k(bufferSize, buffer, magic, syms, other)
    int		bufferSize;	/* size of buffer */
    char	*buffer;	/* buffer containing header */
    int         *magic;         /* pointer to magic number */
    int         *syms;          /* pointer to symbols */
    char	**other;	/* Return other information. */		
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
	if (header->a_machtype == M_68020) {
	    *magic = header->a_magic;
	    *syms = header->a_syms;
	    if (((unsigned char *)header)[0]&0x80) {
		*other = "dynamically linked";
	    }
	    return "sun3";
	} else if (header->a_machtype == M_68010) {
	    *magic = header->a_magic;
	    *syms = header->a_syms;
	    if (header->a_dynamic) {
		*other = "dynamically linked";
	    }
	    return "sun2";
	}
    }
    if (FMT_68K_FORMAT != hostFmt) {
	status = Fmt_Convert("{b2h1w7}", FMT_68K_FORMAT, &bufferSize, 
			(char *) buffer, hostFmt, &swappedSize, swappedHeader);
	if (status) {
#ifndef KERNEL
	    fprintf(stderr, "Fmt_Convert returned %d.\n", status);
#endif
	    return NULL;
	}
	header = (struct exec *) swappedHeader;
	if (!N_BADMAG(*header)) {
	    if (header->a_machtype == M_68020) {
		*magic = header->a_magic;
		*syms = header->a_syms;
		if (((unsigned char *)header)[0]&0x80) {
		    *other = "dynamically linked";
		}
		return "sun3";
	    } else if (header->a_machtype == M_68010) {
		*magic = header->a_magic;
		*syms = header->a_syms;
		if (header->a_dynamic) {
		    *other = "dynamically linked";
		}
		return "sun2";
	    }
	}
    }
    return NULL;
}
