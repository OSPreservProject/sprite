/* 
 * atexit.c --
 *
 *	This file contains the source code for the "atexit" library
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
static char rcsid[] = "$Header: /sprite/src/lib/c/stdlib/RCS/atexit.c,v 1.5 92/03/27 13:41:19 rab Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <stdlib.h>

/*
 * Variables shared with exit.c:
 */

extern void (*_exitHandlers[])();	/* Function table. */
extern int _exitNumHandlers;		/* Number of functions currently
					 * registered in table. */
extern long _exitHandlerArgs[];		/* Arguments to pass to functions. */
extern int _exitTableSize;		/* Number of entries in table. */

/*
 *----------------------------------------------------------------------
 *
 * atexit --
 *
 *	Register a function ("func") to be called as part of process
 *	exiting.
 *
 * Results:
 *	The return value is 0 if the registration was successful,
 *	and -1 if registration failed because the table was full.
 *
 * Side effects:
 *	Information will be remembered so that when the process exits
 *	(by calling the "exit" procedure), func will be called.  Func
 *	takes no arguments and returns no result.  If the process
 *	terminates in some way other than by calling exit, then func
 *	will not be invoked.
 *
 *----------------------------------------------------------------------
 */

int
atexit(func)
    void (*func)();			/* Function to call during exit. */
{
    if (_exitNumHandlers >= _exitTableSize) {
	return -1;
    }
    _exitHandlers[_exitNumHandlers] = func;
    _exitHandlerArgs[_exitNumHandlers] = 0;
    _exitNumHandlers += 1;
    return 0;
}
