/*-
 * spriteCG2M.c --
 *	Functions to support the sprite CG2 board when treated as a monochrome
 *	frame buffer.
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
 *
 */
#ifndef lint
static char rcsid[] =
"$Header: /sprite/src/X11R3/src/cmds/Xsp/ddx/sprite/RCS/spriteCG2M.c,v 1.1 87/06/16 12:20:29 deboor Exp $ SPRITE (Berkeley)";
#endif lint

#include    "spriteddx.h"
#include    "os.h"


/*-
 *-----------------------------------------------------------------------
 * spriteCG2MProbe --
 *	Attempt to find and map a cg2 framebuffer used as mono
 *
 * Results:
 *	TRUE if everything went ok. FALSE if not.
 *
 * Side Effects:
 *	Memory is allocated for the frame buffer and the buffer is mapped.
 *
 *-----------------------------------------------------------------------
 */
Bool
spriteCG2MProbe (screenInfo, index, fbNum, argc, argv)
    ScreenInfo	  *screenInfo;	/* The screenInfo struct */
    int	    	  index;    	/* The index of pScreen in the ScreenInfo */
    int	    	  fbNum;    	/* Index into spriteFbData array */
    int	    	  argc;	    	/* The number of the Server's arguments. */
    char    	  **argv;   	/* The arguments themselves. Don't change! */
{
    return (FALSE);
}
