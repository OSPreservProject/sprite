/*-
 * fileIO.c --
 *	functions for os-independent file I/O
 *
 * Copyright (c) 1987 by the Regents of the University of California
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * TODO:
 *	- Snarf compressed-font stuff from 4.2bsd fileio.c
 *
 */
#ifndef lint
static char rcsid[] =
	"$Header: fileIO.c,v 1.2 88/09/08 18:15:35 ouster Exp $ SPRITE (Berkeley)";
#endif lint

#include    "spriteos.h"
#include    <stdio.h>

/*-
 *-----------------------------------------------------------------------
 * FiOpenForRead --
 *	Open a file for reading. The name needn't be null terminated.
 *	We do it here if it ain't.
 *
 * Results:
 *	An FID (FILE *)
 *
 * Side Effects:
 *
 *-----------------------------------------------------------------------
 */
FID
FiOpenForRead (nameLen, name)
    int	    	  nameLen;  	/* Length of the name */
    char    	  *name;    	/* The name itself */
{
    FILE *	  stream;
    char    	  *null_t_name = name;

    if (name[nameLen-1] != '\0') {
	null_t_name = (char *)ALLOCATE_LOCAL (nameLen+1);
	strncpy (null_t_name, name, nameLen);
	null_t_name[nameLen] = '\0';
    }

    stream = fopen (null_t_name, "r");

    if (null_t_name != name) {
	DEALLOCATE_LOCAL(null_t_name);
    }
    return ((FID) stream);
}

/*-
 *-----------------------------------------------------------------------
 * FiRead --
 *	Read from an open stream
 *
 * Results:
 *
 * Side Effects:
 *
 *-----------------------------------------------------------------------
 */
int
FiRead (buf, itemSize, numItems, fid)
    char    	  *buf;
    unsigned	  itemSize;
    unsigned	  numItems;
    FID	    	  fid;
{
    return fread (buf, itemSize, numItems, (FILE *) fid);
}

/*-
 *-----------------------------------------------------------------------
 * FiClose --
 *	Close an open stream
 *
 * Results:
 *
 * Side Effects:
 *
 *-----------------------------------------------------------------------
 */
int
FiClose (fid)
    FID	    fid;
{
    fclose ((FILE *)fid);
    return (0);
}
