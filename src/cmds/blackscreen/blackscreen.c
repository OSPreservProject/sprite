/* 
 * blackscreen.c --
 *
 *	Source code for the "blackscreen" program, which turns
 *	the screen black until a character is typed.
 *
 * Copyright 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/cmds/blackscreen/RCS/blackscreen.c,v 1.2 91/08/12 16:23:19 dlong Exp $ SPRITE (Berkeley)";
#endif not lint

#include <stdio.h>
#include <sys.h>

main(argc, argv)
    int argc;
    char *argv[];
{
    char	c;

    if (argc == 1) {
	Sys_EnableDisplay(FALSE);
	(void) getchar();
	Sys_EnableDisplay(TRUE);
    } else if (argv[1][1] == 'n') {	/* on */
	Sys_EnableDisplay(TRUE);
    } else {
	Sys_EnableDisplay(FALSE);	/* off */
    }
    exit(0);
}
