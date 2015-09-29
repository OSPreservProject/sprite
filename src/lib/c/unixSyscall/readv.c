/* 
 * readv.c --
 *
 *	Procedure to map from Unix readv system call to Sprite.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/lib/c/unixSyscall/RCS/readv.c,v 1.2 90/08/15 15:25:11 kupfer Exp $ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "fs.h"
#include "sys.h"
#include "compatInt.h"
#include <sys/types.h>
#include <sys/uio.h>


/*
 *----------------------------------------------------------------------
 *
 * readv --
 *
 *	Procedure to map from Unix readv system call to Sprite Fs_Read.
 *
 * Results:
 *	UNIX_ERROR is returned upon error, with the actual error code
 *	stored in errno.  Upon success, the number of bytes actually
 *	read is returned.
 *
 * Side effects:
 *	The buffers are filled with the number of bytes indicated by
 *	the returned value.  (Each buffer has no more bytes than was 
 *	specified by that buffer's length parameter.)
 *
 *----------------------------------------------------------------------
 */

int
readv(stream, iov, iovcnt)
    int stream;			/* descriptor for stream to read. */
    register struct iovec *iov;	/* pointer to array of iovecs. */
    int iovcnt;			/* number of  iovecs in iov. */
{
    ReturnStatus status;	/* result returned by Fs_Read */
    int amountRead;		/* place to hold number of bytes read */
    int totalRead = 0;	/* place to hold total # of bytes written */
    int i;

    for (i=0; i < iovcnt; i++, iov++) {
	status = Fs_Read(stream, iov->iov_len, iov->iov_base, 
							&amountRead);
	if (status != SUCCESS) {
	    break;
	} else {
	    totalRead += amountRead;
	}
    }

    if (status != SUCCESS && totalRead == 0) {
	errno = Compat_MapCode(status);
	return(UNIX_ERROR);
    } else {
	return(totalRead);
    }
}
