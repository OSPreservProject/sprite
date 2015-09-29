/* 
 * sethostname.c --
 *
 *	Sets the host name in the kernel.
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
static char rcsid[] = "$Header: /sprite/lib/forms/RCS/proto.c,v 1.6 92/03/02 15:29:56 bmiller Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <stdio.h>

main(argc, argv) 
    int		argc;
    char	**argv;
{
    if (argc != 2) {
	fprintf(stderr, "Usage: sethostname name\n");
	exit(1);
    }
    return sethostname(argv[1], strlen(argv[1]));
}

