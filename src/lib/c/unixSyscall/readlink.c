/* 
 * readlink.c --
 *
 *	Procedure to map from Unix readlink system call to Sprite.
 *
 * Copyright (C) 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/lib/c/unixSyscall/RCS/readlink.c,v 1.4 90/03/23 10:29:33 douglis Exp $ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "fs.h"
#include "compatInt.h"


/*
 *----------------------------------------------------------------------
 *
 * readlink --
 *
 *	Procedure to map from Unix readlink system call to Sprite Fs_ReadLink.
 *
 * Results:
 *	UNIX_ERROR is returned upon error, with the actual error code
 *	stored in errno.  Upon success, the number of bytes actually
 *	read (ie. the length of the link's target pathname) is returned.
 *	
 *
 * Side effects:
 *	The buffer is filled with the number of bytes indicated by
 *	the length parameter.  
 *
 *----------------------------------------------------------------------
 */

int
readlink(link, buffer, numBytes)
    char *link;			/* name of link file to read */
    char *buffer;		/* pointer to buffer area */
    int numBytes;		/* number of bytes to read */
{
    ReturnStatus status;	/* result returned by Fs_Read */
    int amountRead;		/* place to hold number of bytes read */

    status = Fs_ReadLink(link, numBytes, buffer, &amountRead);
    if (status != SUCCESS) {
	errno = Compat_MapCode(status);
	return(UNIX_ERROR);
    } else {
	/*
	 * Sprite's Fs_ReadLink includes the terminating null character
	 * in the character count return (amountRead) while Unix doesn't.
	 *
	 * ** NOTE ** this check can go away  once all hosts are running
	 * kernels that fix this before returning the value.
	 */
	if (buffer[amountRead-1] == '\0') {
	    amountRead--;
	}
	
	return(amountRead);
    }
}
