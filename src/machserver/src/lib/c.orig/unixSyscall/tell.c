/* 
 * tell.c --
 *
 *	Emulates the obsolete tell system call.
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
static char rcsid[] = "$Header$";
#endif /* not lint */

#include <sys/types.h>
#include <sys/file.h>

/*
 *----------------------------------------------------------------------
 *
 * tell --
 *
 * Results:
 *	Returns the current offset in a file.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

off_t
tell(fd)
    int fd;
{

    return lseek(fd, 0L, L_INCR);
}

