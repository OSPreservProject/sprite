/*
 * profil.c --
 *
 *	Procedure to map from Unix profil system call to Sprite.
 *
 * Copyright 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header$";
#endif

#include "sprite.h"
#include "compatInt.h"


/*
 *----------------------------------------------------------------------
 *
 * profile --
 *
 *	Procedure to map from Unix profil system call to Sprite Prof_Profil.
 *
 * Results:
 *	Always returns UNIX_SUCCESS.
 *
 * Side effects:
 *	The process is scheduled for profiling.
 *
 *----------------------------------------------------------------------
 */

int
profil(buffer, bufsize, offset, scale)
    char *buffer;   /* pointer to buffer where profile data will be stored */
    int bufsize;    /* size of buffer */
    int offset;     /* offset to be subtracted from pc */
    int scale;      /* scale that pc is divided by */
{

    Prof_Profil(buffer, bufsize, offset, scale);
    return UNIX_SUCCESS;
}

