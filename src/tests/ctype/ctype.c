/* 
 * ctype.c --
 *
 *	This file contains a program that exercises the ctype
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
static char rcsid[] = "$Header: ctype.c,v 1.1 88/04/27 18:02:43 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include <stdio.h>
#include <ctype.h>

char *alnum =	"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
char *alpha =	"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
char *cntrl =	"\1\2\3\4\5\6\7\10\11\12\13\14\15\16\17\20\21\22\23\24\25\26\27\30\31\32\33\34\35\36\37\177";
char *digit =	"1234567890";
char *graph =	"!\42#$%&'()*+`,-./:;<=>?@[]\\_^{}|~abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
char *lower =	"abcdefghijklmnopqrstuvwxyz";
char *print =	" !\42#$%&'()*+`,-./:;<=>?@[]\\_^{}|~abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
char *punct =	"!\42#$%&'()*+`,-./:;<=>?@[]\\_^{}|~";
char *space =	" \11\12\13\14\15";
char *upper =	"ABCDEFGHIJKLMNOPQRSTUVWXYZ";
char *xdigit =	"01234567890ABCDEFabcdef";

#define check(string, macro, msg)			\
    setflags(string);					\
    checkflags(macro, msg);

#define checkflags(macro,msg)					\
    for (i = -1; i < 256; i++) {				\
	if (macro(i) ? !flags[i+1] : flags[i+1]) {		\
	    fprintf(stderr, "%s at character 0x%02x\n", msg, i);	\
	    result = 1;						\
	}							\
    }

char flags[256];

void
setflags(string)
    char *string;
{
    int i;

    for (i = 0; i < 257; i++) {
	flags[i] = 0;
    }
    for ( ; *string != 0; string++) {
	flags[1+*string] = 1;
    }
}

main()
{
    int result = 0;
    int i;

    /*
     * isalnum
     */

    check(alnum, isalnum, "isalnum error 1");
#undef isalnum;
    check(alnum, isalnum, "isalnum error 2");

    /*
     * isalpha
     */

    check(alpha, isalpha, "isalpha error 1");
#undef isalpha;
    check(alpha, isalpha, "isalpha error 2");

    /*
     * iscontrl
     */

    setflags(cntrl);
    flags[1] = 1;
    checkflags(iscntrl, "iscntrl error 1");
#undef iscntrl
    checkflags(iscntrl, "iscntrl error 2");

    /*
     * isdigit
     */

    check(digit, isdigit, "isdigit error 1");
#undef isdigit;
    check(digit, isdigit, "isdigit error 2");

    /*
     * isgraph
     */

    check(graph, isgraph, "isgraph error 1");
#undef isgraph;
    check(graph, isgraph, "isgraph error 2");

    /*
     * islower
     */

    check(lower, islower, "islower error 1");
#undef islower;
    check(lower, islower, "islower error 2");

    /*
     * isprint
     */

    check(print, isprint, "isprint error 1");
#undef isprint;
    check(print, isprint, "isprint error 2");

    /*
     * ispunct
     */

    check(punct, ispunct, "ispunct error 1");
#undef ispunct;
    check(punct, ispunct, "ispunct error 2");

    /*
     * isspace
     */

    check(space, isspace, "isspace error 1");
#undef isspace;
    check(space, isspace, "isspace error 2");

    /*
     * isupper
     */

    check(upper, isupper, "isupper error 1");
#undef isupper;
    check(upper, isupper, "isupper error 2");

    /*
     * isxdigit
     */

    check(xdigit, isxdigit, "isxdigit error 1");
#undef isxdigit;
    check(xdigit, isxdigit, "isxdigit error 2");

    return result;
}
