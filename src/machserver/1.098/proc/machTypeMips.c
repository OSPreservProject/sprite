/* 
 * machTypeMips.c --
 *
 *	Contains the machine specific routine for determining if
 *      a file is an object file for a decStation 3100.
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
static char rcsid[] = "$Header: /sprite/src/attcmds/file/RCS/machTypeMips.c,v 1.4 90/10/19 15:25:44 jhh Exp Locker: shirriff $";
#endif /* not lint */

#include <assert.h>
#ifdef KERNEL
#include <sys.h>
#endif

/*
 * Hack warning:  we have to define mips, LANGUAGE_C, and MIPSEL in order
 * to get the proper ds3100 header files.  We must then undefine mips if
 * this isn't a ds3100.
 */
#ifndef mips
#define mips 1
#define notreallymips
#endif

#ifndef LANGUAGE_C
#define LANGUAGE_C
#endif

#ifndef MIPSEL
#define MIPSEL
#endif

#include <ds3100.md/sys/exec.h>
#include <ds3100.md/a.out.h>

#ifdef notreallymips
#undef mips
#endif

#include "file.h"


/*
 *----------------------------------------------------------------------
 *
 * machTypeMips --
 *
 *	Determine if the file is a mips-specific file type.
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
machTypeMips(bufferSize, buffer, magic, syms, other)
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

    assert(HEADERSIZE >= sizeof(struct exec));
    if (bufferSize < sizeof(struct exec)) {
	return NULL;
    }
    header = (struct exec *) buffer;
    if (!N_BADMAG((header->ex_o))) {
	*magic = header->a_magic;
	*syms = header->ex_f.f_nsyms;
	return "mips";
    }
    if (FMT_MIPS_FORMAT != hostFmt) {
	status = Fmt_Convert("{h2w3h2h2w13}", FMT_MIPS_FORMAT, &bufferSize,
	    (char *) buffer, hostFmt, &swappedSize, swappedHeader);
	if (status) {
#ifndef KERNEL
	    fprintf(stderr, "Fmt_Convert returned %d.\n", status);
#endif
	    return NULL;
	}
	header = (struct exec *) swappedHeader;
	if (!N_BADMAG((header->ex_o))) {
	    *magic = header->a_magic;
	    *syms = header->ex_f.f_nsyms;
	    return "mips";
	}
    }
    return NULL;
}

