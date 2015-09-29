/* 
 * _ExecArgs.c --
 *
 *	Source code for the _ExecArgs library utility procedure.
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
static char rcsid[] = "$Header: _ExecArgs.c,v 1.2 88/07/28 17:49:57 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include <varargs.h>

/*
 * Library imports:
 */

extern char *malloc();


/*
 *----------------------------------------------------------------------
 *
 * _ExecArgs --
 *
 *	Collect an argument list, as passed to execl or execle, into
 *	a dynamically-allocated array.
 *
 * Results:
 *	The return value is a pointer to an array, dynamically
 *	allocated, that contains the argv values pointed to by
 *	args.  Args is updated so that the next va_arg will return
 *	the argument just after the terminating 0 in the argv
 *	list.  It is the caller's responsibility to free the
 *	return value if that is desired.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

char **
_ExecArgs(args)
    va_list *args;		/* Pointer to arguments containing a variable-
				 * length collection of argv values, with
				 * a zero value to terminate. */
{
    va_list arg2;
    int count, i;
    register char **array;

    arg2 = *args;
    for (count = 1; va_arg(arg2, char *) != 0; count++) {
	/* Null loop body. */
    }
    array = (char **) malloc((unsigned) (count * sizeof(char *)));
    for (i = 0; i < count; i++) {
	array[i] = va_arg(*args, char *);
    }
    return array;
}
