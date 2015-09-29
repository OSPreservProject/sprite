/* 
 * etc.c --
 *
 *	This file contains a program that exercises the etc
 *	library facilities.  Invoke it with no parameters;  it
 *	will print messages on stderr for any problems it detects
 *	with the string procedures.
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
static char rcsid[] = "$Header: etc.c,v 1.1 88/04/28 10:15:38 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include <stdio.h>
#include <varargs.h>

#define error(string) \
    fprintf(stderr, string); \
    exit(1);

void
varCheck(numArgs, va_alist)
    int numArgs;		/* Number of arguments in addition to this
				 * one.  Must alternate int and double,
				 * in ascending integer order. */
    va_dcl
{
    va_list args;
    int i;
    double d, checkD = 1.0;
    int val, checkVal = 1;

    va_start(args);
    for (i = 1; i <= numArgs; i++) {
	if (i & 1) {
	    val = va_arg(args, int);
	    if (val != checkVal) {
		fprintf(stderr, "varargs error 1, arg %d\n", i);
		exit(1);
	    }
	    checkVal += 1;
	} else {
	    d = va_arg(args, double);
	    if (d != checkD) {
		fprintf(stderr, "varargs error 2, arg %d\n", i);
		exit(1);
	    }
	    checkD += 1.0;
	}
    }
    va_end(args);
}

main()
{
    int result = 0;

    /*
     * varargs
     */

    varCheck(1, 1);
    varCheck(5, 1, 1.0, 2, 2.0, 3);
    varCheck(10, 1, 1.0, 2, 2.0, 3, 3.0, 4, 4.0, 5, 5.0);

    return result;
}
