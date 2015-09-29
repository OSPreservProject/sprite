/* 
 * read.c --
 *
 *	Procedure to map from Unix read system call to Sprite.
 *
 * Copyright (C) 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: read.c,v 1.1 88/06/19 14:31:51 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "fs.h"
#include "compatInt.h"


/*
 *----------------------------------------------------------------------
 *
 * read --
 *
 *	Procedure to map from Unix read system call to Sprite Fs_Read.
 *
 * Results:
 *	UNIX_ERROR is returned upon error, with the actual error code
 *	stored in errno.  Upon success, the number of bytes actually
 *	read is returned.
 *
 * Side effects:
 *	The buffer is filled with the number of bytes indicated by
 *	the length parameter.  
 *
 *----------------------------------------------------------------------
 */

int
read(descriptor, buffer, numBytes)
    int descriptor;		/* descriptor for stream to read */
    char *buffer;		/* pointer to buffer area */
    int numBytes;		/* number of bytes to read */
{
    ReturnStatus status;	/* result returned by Fs_Read */
    int amountRead;		/* place to hold number of bytes read */

    status = Fs_Read(descriptor, numBytes, buffer, &amountRead);
    if (status != SUCCESS) {
	errno = Compat_MapCode(status);
	return(UNIX_ERROR);
    } else {
	return(amountRead);
    }
}
