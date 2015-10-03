/* version.c --
 *
 *     Returns version from version.h
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char sccsid[] = "%W% VESTA (Berkeley) %G%";
#endif not lint

#include "version.h"	/* automatically generated */
#include "main.h"

char versionString[] = VERSION ;
char *
SpriteVersion()
{
    return(versionString) ;
}
