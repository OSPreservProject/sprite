/* 
 * MemPanic.c --
 *
 *	Source code for the "MemPanic" procedure, which is used
 *	internally by the memory allocator to die gracefully after
 *	an unrecoverable error.  Different programs or uses of the
 *	allocator may replace this procedure with something more
 *	suitable for the particular program or use.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/stdlib/RCS/MemPanic.c,v 1.2 88/06/17 18:07:00 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include <sys.h>

/*
 *----------------------------------------------------------------------
 *
 * MemPanic --
 *
 *	MemPanic is a procedure that's called by the memory allocator
 *	when it has uncovered a fatal error.  MemPanic prints the 
 *	message and aborts.  It does NOT return.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The program is exited.
 *
 *----------------------------------------------------------------------
 */

void
MemPanic(message)
    char *message;
{
    Sys_Panic(SYS_FATAL, "MemPanic: %s\n", message);
    exit(1);
}
