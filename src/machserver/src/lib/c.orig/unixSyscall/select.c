/* 
 * select.c --
 *
 *	Procedure to map from Unix select system call to Sprite.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/lib/c/unixSyscall/RCS/select.c,v 1.2 89/03/22 12:20:49 douglis Exp $ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "status.h"
#include "fs.h"
#include "compatInt.h"
#include <sys/time.h>


/*
 *----------------------------------------------------------------------
 *
 * select --
 *
 *	Procedure to map from Unix select system call to Sprite Fs_Select.
 *
 * Results:
 *	UNIX_ERROR is returned upon error, with the actual error code
 *	stored in errno.  The number of ready descriptors is returned
 *	upon success.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
select(width, readfds, writefds, exceptfds, timeout)
    int width, *readfds, *writefds, *exceptfds;
    struct timeval *timeout;
{
    ReturnStatus status;	/* result returned by Fs_Select */
    int numReady;
    Time spriteTimeout;
    Time *timeoutPtr = NULL;

    if (timeout != NULL) {
	spriteTimeout.seconds = timeout->tv_sec;
	spriteTimeout.microseconds = timeout->tv_usec;
	timeoutPtr = &spriteTimeout;
    }
    status = Fs_RawSelect(width, timeoutPtr, readfds, writefds,
			  exceptfds, &numReady);

    if (status != SUCCESS) {
	if (status == FS_TIMEOUT) {
	    if (readfds != NULL) {
		*readfds = 0;
	    }
	    if (writefds != NULL) {
		*writefds = 0;
	    }
	    if (exceptfds != NULL) {
		*exceptfds = 0;
	    }
	    return(0);
	} else {
	    errno = Compat_MapCode(status);
	    return(UNIX_ERROR);
	}
    } else {
	return(numReady);
    }
}
