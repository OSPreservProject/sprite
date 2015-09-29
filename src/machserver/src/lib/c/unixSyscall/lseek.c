/* 
 * lseek.c --
 *
 *	Procedure to map from Unix access system call to Sprite.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: /r3/kupfer/spriteserver/src/lib/c/unixSyscall/RCS/lseek.c,v 1.2 91/12/01 22:31:51 kupfer Exp $ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include "compatInt.h"
#include <fs.h>
#include <sys/file.h>
#include <errno.h>
#include <status.h>


/*
 *----------------------------------------------------------------------
 *
 * lseek --
 *
 *	procedure for Unix lseek call. 
 *
 * Results:
 *	the old offset if the IOC_Reposition call was successful.
 *	UNIX_ERROR is returned if IOC_Reposition failed.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

long
lseek(filedes, offset, whence)
    int filedes;			/* array of stream identifiers */
    long offset;
    int whence;
{
    ReturnStatus	status;
    int  		base;
    int			newOffset;

    switch(whence) {
	case L_SET:
	    base = IOC_BASE_ZERO;
	    break;
	case L_INCR:
	    base = IOC_BASE_CURRENT;
	    break;
	case L_XTND:
	    base = IOC_BASE_EOF;
	    break;
	default:
	    errno = EINVAL;
	    return(UNIX_ERROR);
    }
    status = Ioc_Reposition(filedes, base, (int) offset, &newOffset);
    if (status != SUCCESS) {
	errno = Compat_MapCode(status);
	return(UNIX_ERROR);
    } else {
	return(newOffset);
    }
}
