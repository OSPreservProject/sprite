/* 
 * writev.c --
 *
 *	Procedure to map from Unix writev system call to Sprite.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/lib/c/unixSyscall/RCS/writev.c,v 1.2 90/08/20 17:18:43 kupfer Exp $ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "fs.h"
#include "compatInt.h"
#include <sys/types.h>
#include <sys/uio.h>



/*
 *----------------------------------------------------------------------
 *
 * writev --
 *
 *	Procedure to map from Unix writev system call to Sprite Fs_Write.
 *
 * Results:
 *	UNIX_ERROR is returned upon error, with the actual error code
 *	stored in errno.  Upon success, the number of bytes actually
 *	written is returned.
 *
 * Side effects:
 *	The data in the buffers is written to the file at the indicated offset.
 *
 *----------------------------------------------------------------------
 */

int
writev(descriptor, iov, ioveclen)
    int descriptor;		/* descriptor for stream to write */
    register struct iovec *iov;	/* array of io vectors. */
    int ioveclen;		/* number of iovec's in iov. */
{
    ReturnStatus status;	/* result returned by Fs_Write */
    int amountWritten;		/* place to hold number of bytes written */
    int totalWritten = 0;	/* place to hold total # of bytes written */
    int i;

    for (i=0; i < ioveclen; i++, iov++) {
	status = Fs_Write(descriptor, iov->iov_len, iov->iov_base, 
							&amountWritten);
	if (status != SUCCESS) {
	    break;
	} else {
	    totalWritten += amountWritten;
	}
    }
    if (status != SUCCESS && totalWritten == 0) {
	errno = Compat_MapCode(status);
	return(UNIX_ERROR);
    } else {
	return(totalWritten);
    }
}
