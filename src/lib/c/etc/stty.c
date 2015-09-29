/* 
 * stty.c --
 *
 *	Soruce for stty procedure.
 *
 * Copyright 1986 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/lib/c/etc/RCS/stty.c,v 1.3 88/12/31 12:26:15 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include <sgtty.h>

/*
 *----------------------------------------------------------------------
 *
 * stty --
 *
 *      Set terminal characteristics.
 *
 * Results:
 *      0 is normally returned.  If an error occurred, -1 is returned
 *	and errno give details.
 *
 * Side effects:
 *	The terminal characterstics are modified according to buf.
 *
 *----------------------------------------------------------------------
 */


int
stty(fd, buf)
    int fd;			/* File descriptor */
    struct sgttyb *buf;		/* buffer that holds tty info */
{
    return ioctl(fd, TIOCSETP, buf);
}
