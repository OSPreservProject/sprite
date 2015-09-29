/* 
 * exit.c --
 *
 *	This file contains the source code for the "exit" library
 *	procedure.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/stdlib/RCS/exit.c,v 1.9 92/03/27 13:41:59 rab Exp $ SPRITE (Berkeley)";
#endif not lint

#include <stdio.h>
#include <stdlib.h>
#include <sprite.h>
#include <proc.h>

/*
 * Variables shared with atexit.c, and on_exit.c:
 */

void (*_exitHandlers[32])();		/* Function table. */
int _exitNumHandlers = 0;		/* Number of functions currently
					 * registered in table. */
long _exitHandlerArgs[32];		/* Arguments to pass to functions. */
int _exitTableSize = 32;		/* Number of entries in table. */

/*
 *----------------------------------------------------------------------
 *
 * exit --
 *
 *	Terminate the process.
 *
 * Results:
 *	Never returns.
 *
 * Side effects:
 *	Any procedures registered by calls to "atexit" are invoked,
 *	and any open I/O streams are flushed and closed.
 *
 *----------------------------------------------------------------------
 */

int
exit(status)
    int status;			/* Status to return to parent process.  0 is
				 * the normal value for "success". */
{
    while (_exitNumHandlers > 0) {
	_exitNumHandlers -= 1;
	(*_exitHandlers[_exitNumHandlers])(_exitHandlerArgs[_exitNumHandlers]);
    }
    _cleanup();
    _exit(status);
}
