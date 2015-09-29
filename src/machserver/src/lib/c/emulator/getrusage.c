/* 
 * getrusage.c --
 *
 *	UNIX getrusage() for the Sprite server.
 *
 * Copyright 1992 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that this copyright
 * notice appears in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/lib/c/emulator/RCS/getrusage.c,v 1.1 92/03/13 20:39:58 kupfer Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <mach.h>
#include <sprite.h>
#include <bstring.h>
#include <compatInt.h>
#include <spriteEmuInt.h>
#include <spriteSrv.h>
#include <sys/time.h>
#include <sys/resource.h>


/*
 *----------------------------------------------------------------------
 *
 * getrusage --
 *
 *	Fill in whatever numbers the Sprite server can come up with, and
 *	zero out the rest.
 *
 * Results:
 *	UNIX_ERROR is returned upon error, with the actual error code
 *	stored in errno.  Upon success, UNIX_SUCCESS is returned and the
 *	rusage struct is filled with the available information.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
    
int
getrusage(who, rusage)
     int who;			/* self or terminated children */
     struct rusage *rusage;	/* OUT: usage information */
{
    /* 
     * Right now the Sprite server doesn't provide anything.
     */
    bzero((char *)rusage, sizeof(struct rusage));
    return UNIX_SUCCESS;
}
