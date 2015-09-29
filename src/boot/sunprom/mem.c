/* 
 * mem.c --
 *
 *	A simple (and small) memory allocator for the SCSI disk bootstrap
 *	program.
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

#ifdef notdef
static char rcsid[] = "$Header: /sprite/src/boot/sunprom/RCS/mem.c,v 1.2 90/07/17 15:42:30 mendel Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include "sprite.h"
    extern int end;

static char *memEnd = (char *) &end;

/*
 *----------------------------------------------------------------------
 *
 * malloc --
 *
 *     Allocate a block of memory of the given size starting at the
 *     current end of kernel memory.
 *
 * Results:
 *	A pointer to the allocated memory
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */



char *
malloc(numBytes)
{
    char	*addr;

    addr =  memEnd;

    memEnd += (numBytes + 3) & ~3;
    bzero(addr, numBytes);
    return(addr);
}

void
free(address)
    char *address;
{
    return;
}
void
_free(address)
    char *address;
{
    return;
}


