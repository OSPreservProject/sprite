/* devVid.c -
 *
 *     This file contains video routines.
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
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"


/*
 * ----------------------------------------------------------------------------
 *
 * Dev_VidEnable --
 *
 *     Enable the video board.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     None.
 *
 * ----------------------------------------------------------------------------
 */
ReturnStatus
Dev_VidEnable(on)
    Boolean	on;
{
    return SUCCESS;
}
