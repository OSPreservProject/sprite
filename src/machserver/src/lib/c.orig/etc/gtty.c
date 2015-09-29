/* 
 * gtty.c --
 *
 *	Source for gtty procedure.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/etc/RCS/gtty.c,v 1.3 88/12/31 12:25:51 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include <sgtty.h>

/*
 *----------------------------------------------------------------------
 *
 * gtty --
 *
 *      Procedure to simulate Unix gtty call.
 *
 * Results:
 *      0 is normally returned.  If an error occurred, -1 is returned
 *	and errno give more details.  Buf is filled in with the
 *	terminal characteristics of fd. 
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */


int
gtty(fd, buf)
    int fd;			/* File descriptor. */
    struct sgttyb *buf;		/* Buffer to hold tty info */
{
    return ioctl(fd, TIOCGETP, buf);
}
