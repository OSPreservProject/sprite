/* 
 * Fs_Truncate.c --
 *
 *	Source code for the Fs_Truncate library procedure.
 *
 * Copyright 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: Fs_Truncate.c,v 1.1 88/06/19 14:29:13 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include <fs.h>


/*
 *----------------------------------------------------------------------
 *
 * Fs_Truncate --
 *
 *	Truncate a file to a given length, given its name.  This maps the
 *	operation into an Fs_Open, Fs_IOControl, and then closes the file.
 *
 * Results:
 *	Result from Fs_IOControl.
 *
 * Side effects:
 *	Shorten the file to length bytes.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Fs_Truncate(pathName, length)
    char *pathName;		/* Name of the file to truncate */
    int	 length;		/* The size to truncate to */
{
    ReturnStatus status;
    int streamID;

    status = Fs_Open(pathName, FS_WRITE, 0, &streamID);
    if (status != SUCCESS) {
	return(status);
    }
    status = Ioc_Truncate(streamID, length);
    (void) Fs_Close(streamID);
    return(status);
}
