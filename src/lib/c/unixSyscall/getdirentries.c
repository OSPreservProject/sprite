/* 
 * getdirentries.c --
 *
 *	Procedure to map from Unix getdirentries system call to Sprite.
 *
 * Copyright (C) 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: getdirentries.c,v 1.2 88/07/29 17:39:29 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "fs.h"

#include "compatInt.h"


/*
 *----------------------------------------------------------------------
 *
 * getdirentries --
 *
 *	Procedure to map from Unix getdirentries system call to Sprite 
 *	Fs_?.
 *
 *	This routine does not fully implement the getdirentries
 *	semantics. It does enough to keep the readdir library routine
 *	happy.
 *
 * Results:
 *	amount read 	- if the call was successful.
 *	UNIX_ERROR 	- the call was not successful. 
 *			  The actual error code stored in errno.  
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
getdirentries(fd, buf, nbytes, basep)
    int  fd;
    char *buf;
    int nbytes;
    long *basep;
{
    ReturnStatus status;	/* result returned by Fs_Read */
    int	amountRead;

    /*
     * Read an entry in the directory specified by fd.
     */

    status = Fs_Read(fd, nbytes, buf, &amountRead);
    if (status != SUCCESS) {
	errno = Compat_MapCode(status);
	return(UNIX_ERROR);
    } else {
	return(amountRead);
    }
}
