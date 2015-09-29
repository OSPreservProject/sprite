/* 
 * version.c --
 *
 *     Returns version from version.h  Ideally version.h is recreated
 *     and this file is recompiled just before the main program is loaded.
 *
 * Copyright 1985, 1990 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/lib/misc/RCS/version.c,v 1.2 90/02/06 15:35:59 douglis Exp $ SPRITE (Berkeley)";
#endif not lint

#include "version.h"	/* automatically generated */

static char versionString[] = VERSION ;

char *
Version()
{
    return(versionString) ;
}
