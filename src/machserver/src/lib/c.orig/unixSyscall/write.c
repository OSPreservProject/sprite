/* 
 * write.c --
 *
 *	Procedure to map from Unix write system call to Sprite.
 *
 * Copyright (C) 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: write.c,v 1.1 88/06/19 14:32:13 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "fs.h"
#include "compatInt.h"


/*
 *----------------------------------------------------------------------
 *
 * write --
 *
 *	Procedure to map from Unix write system call to Sprite Fs_Write.
 *
 * Results:
 *	UNIX_ERROR is returned upon error, with the actual error code
 *	stored in errno.  Upon success, the number of bytes actually
 *	written is returned.
 *
 * Side effects:
 *	The data in the buffer is written to the file at the indicated offset.
 *
 *----------------------------------------------------------------------
 */

int
write(descriptor, buffer, numBytes)
    int descriptor;		/* descriptor for stream to write */
    char *buffer;		/* pointer to buffer area */
    int numBytes;		/* number of bytes to write */
{
    ReturnStatus status;	/* result returned by Fs_Write */
    int amountwritten;		/* place to hold number of bytes written */

    status = Fs_Write(descriptor, numBytes, buffer, &amountwritten);
    if (status != SUCCESS) {
	errno = Compat_MapCode(status);
	return(UNIX_ERROR);
    } else {
	return(amountwritten);
    }
}
