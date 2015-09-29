/*-
 * color.c --
 *	
 *
 * Copyright (c) 1987 by the Regents of the University of California
 * Copyright (c) 1987 by Adam de Boor, UC Berkeley
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 *
 */
#ifndef lint
static char rcsid[] =
	"$Header: /mic/X11R3/src/cmds/Xsp/os/sprite/RCS/color.c,v 1.4 89/10/25 18:06:38 tve Exp $ SPRITE (Berkeley)";
#endif lint

#include <dbm.h>
#include "rgb.h"
#include    "spriteos.h"

/*-
 *-----------------------------------------------------------------------
 * OsLookupColor --
 *	Lookup a named color in the color database and return its rgb
 *	value.
 *
 * Results:
 *	TRUE if the color was found, FALSE otherwise.
 *	If the color is found, *pRed, *pGreen and *pBlue will be altered.
 *
 * Side Effects:
 *
 *-----------------------------------------------------------------------
 */
int
OsLookupColor (screen, name, len, pRed, pGreen, pBlue)
    int	    	  screen;   	/* Screen number */
    char    	  *name;    	/* Color name */
    int	    	  len;	    	/* Length of name */
    unsigned short *pRed,   	/* Pointer to red value */
		   *pGreen, 	/* Pointer to green value */
		   *pBlue;  	/* Pointer to blue value */
{
    datum		dbent;
    RGB			rgb;
    char	*lowername;

    /* convert name to lower case */
    lowername = (char *)ALLOCATE_LOCAL(len + 1);
    if (!lowername)
	return(0);
    CopyISOLatin1Lowered ((unsigned char *) lowername, (unsigned char *) name,
			  (int)len);

    dbent.dptr = lowername;
    dbent.dsize = len;
    dbent = fetch (dbent);

    DEALLOCATE_LOCAL(lowername);

    if(dbent.dptr)
    {
	bcopy(dbent.dptr, (char *) &rgb, sizeof (RGB));
	*pRed = rgb.red;
	*pGreen = rgb.green;
	*pBlue = rgb.blue;
	return (1);
    }
    return TRUE;
}
