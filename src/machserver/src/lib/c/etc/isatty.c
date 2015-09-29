/* 
 * isatty.c --
 *
 *	Procedure to map from Unix isatty system call to Sprite.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: /r3/kupfer/spriteserver/src/lib/c/etc/RCS/isatty.c,v 1.1 91/12/08 14:49:11 kupfer Exp $ SPRITE (Berkeley)";
#endif not lint

#include <sgtty.h>

/*
 *----------------------------------------------------------------------
 *
 * isatty --
 *
 *	Returns non-zero if the given file descriptor refers to a device
 *	with terminal-like characteristics.
 *
 * Results:
 *	Non-zero means fd has terminal-like behavior, zero means it
 *	doesn't.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
isatty(fd)
    int fd;                             /* stream identifier */
{
    struct sgttyb sgttyb;

    if (ioctl(fd, TIOCGETP, (char *) &sgttyb) == -1) {
	return 0;
    }
    return 1;
}
