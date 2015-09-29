/* 
 * main.c --
 *
 *	Program to exec a command with the given arguments, but with the sprite
 *	userid, since this program will be setuid sprite.
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
static char rcsid[] = "$Header: /sprite/lib/forms/RCS/proto.c,v 1.5 91/02/09 13:24:44 ouster Exp $ SPRITE (Berkeley)";
#endif /* not lint */


/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	Exec a program with the given arguments.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The program is run as whichever user this program is setuid.
 *
 *----------------------------------------------------------------------
 */
main(argc, argv)
int	argc;
char	*argv[];
{
    argv[0] = "cvs";
    execvp(argv[0], argv);

    exit(0);
}
