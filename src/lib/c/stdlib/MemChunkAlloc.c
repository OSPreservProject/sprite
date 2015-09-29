/* 
 * MemChunkAlloc.c --
 *
 *	Source code for the "MemChunkAlloc" procedure, which is used
 *	internally by the memory allocator to create new memory for
 *	user-level programs.  Different programs or uses of the
 *	allocator may replace this version of MemChunkAlloc with
 *	something appropriate to the particular program.  See memInt.h
 *	for overall information about how the allocator works.
 *
 * Copyright 1985, 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/lib/c/stdlib/RCS/MemChunkAlloc.c,v 1.4 90/09/27 04:42:29 rab Exp $ SPRITE (Berkeley)";
#endif not lint

#include "memInt.h"
#include <stdio.h>
#include <sys/types.h>

/*
 * UNIX library imports:
 */

extern caddr_t	sbrk();

/*
 * The variables below don't exactly belong in this file, but they
 * need to be someplace that's use-dependent (just like MemChunkAlloc)
 * so that they can be initialized differently for different uses
 * of the allocator (e.g. kernel vs. user).
 */

extern int	fprintf();
int		(*memPrintProc)()	= fprintf;
ClientData	memPrintData		= (ClientData) stdout;

/*
 *----------------------------------------------------------------------
 *
 * MemChunkAlloc --
 *
 *	malloc calls this procedure to get another region of storage
 *	from the system (i.e. whenever the storage it's gotten so far
 *	is insufficient to meet a request).  The actual size returned 
 *	may be greater than size but not less.  This region now becomes 
 *	the permanent property of malloc, and will never be returned.  
 *
 * Results:
 *	The return value is a pointer to a new block of memory that
 *	is size bytes long.
 *
 * Side effects:
 *	The virtual address space of the process is extended.
 *	If the VAS can't be increased, the process is terminated.
 *
 *----------------------------------------------------------------------
 */

Address
MemChunkAlloc(size)
    int size;			/* Number of bytes desired.  */
{
    Address result;
    int misAlignment;

    result = (Address) sbrk(size);
    if (result == (Address) -1) {
	panic("MemChunkAlloc couldn't extend heap");
	return(0);		/* should never get here */
    }
    /* Make sure `result' is aligned to hold at least a double */
    if ((misAlignment = (int) result & 7) != 0) {
	result += 8 - misAlignment;
	(Address) sbrk(8 - misAlignment);
    }
    return result;
}
