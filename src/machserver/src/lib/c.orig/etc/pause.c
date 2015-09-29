/* 
 * pause.c --
 *
 *	Source for "pause" library procedure.
 *
 * Copyright (C) 1986, 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/lib/c/etc/RCS/pause.c,v 1.2 88/12/31 12:26:17 ouster Exp $ SPRITE (Berkeley)";
#endif not lint


/*
 *----------------------------------------------------------------------
 *
 * pause --
 *
 *	Go to sleep until a signal is received.
 *
 * Results:
 *	Always returns -1, with errno set to EINTR.
 *
 * Side effects:
 *	The process waits until a non-masked signal is received.
 *
 *----------------------------------------------------------------------
 */

int
pause()
{
    return sigpause(sigblock(0));
}
