/* 
 * multibyte.c --
 *
 *	Multibyte library functions.
 *      These are just dummy stubs.
 *
 * Copyright 1991 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that this copyright
 * notice appears in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/lib/forms/RCS/proto.c,v 1.5 91/02/09 13:24:44 ouster Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stdtypes.h>

/*
 *----------------------------------------------------------------------
 *
 * mblen --
 *
 *      Determines the number of bytes comprising the multibyte
 *      character pointed to by s.
 *
 * Results:
 *	Returns the number of bytes comprising the multibyte
 *      character pointed to by s.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
mblen(s, n)
    char *s;
    size_t n;
{

    if (s == NULL) {
	return 0;
    }
    return -1;
}

/*
 *----------------------------------------------------------------------
 *
 * mbstowcs --
 *
 *      Converts a sequence of multibyte characters that begins
 *      in the initial shift state from the array pointed to by
 *      s into a sequence of corresponding codes and stores  no
 *      more  than  n  codes into the array pointed to by pwcs.
 *      No multibyte characters that follow  a  null  character
 *      (which  is  converted into a code with value zero) will
 *      be examined or converted.  Each multibyte character  is
 *      converted  as if by a call to mbtowc(), except that the
 *      shift state of mbtowc() is not affected.
 *
 *      No more than n elements will be modified in  the  array
 *      pointed  to  by  pwcs.   If copying takes place between
 *      objects that overlap, the behavior is undefined.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

size_t
mbstowcs(s, pwcs, n)
    char *s;
    wchar_t *pwcs;
    size_t n;
{

    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * mbtowc --
 *
 *      Determines the number of bytes that comprise the multi-
 *      byte  character  pointed to by s.  mbtowc() then deter-
 *      mines  the  code  for  value  of  type   wchar_t   that
 *      corresponds  to that multibyte character.  The value of
 *      the code corresponding to the null caharacter is  zero.
 *      If  the  multibyte  character is valid and pwc is not a
 *      null pointer, mbtowc() stores the code  in  the  object
 *      pointed  to  by  pwc.   At  most  n  bytes of the array
 *      pointed to by s will be examined.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
int
mbtowc(pwc, s, n)
    wchar_t *pwc;
    char *s;
    size_t n;
{

    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * wcstombs --
 *
 *        Converts a sequence of codes that correspond to  multi-
 *        byte  characters from the array pointed to by pwcs into
 *        a sequence of multibyte characters that begins  in  the
 *        initial  shift state and stores these multibyte charac-
 *        ters into the array pointed to by s, stopping if a mul-
 *        tibyte  character  would  exceed  the  limit of n total
 *        bytes or if a null character is stored.  Each  code  is
 *        converted  as if by a call to wctomb(), except that the
 *        shift state of wctomb() is not affected.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
wcstombs(s, pwcs, n)
    char *s;
    wchar_t *pwcs;
    size_t n;
{

    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * wctomb --
 *
 *      Determines the number of bytes needed to represent  the
 *      multibyte  character  corresponding  to  the code whose
 *      value is wchar (including any change in  shift  state).
 *      wctomb()  stores the multibyte character representation
 *      in the array object pointed to by s (if s is not a null
 *      pointer).   At  most, MB_CUR_MAX characters are stored.
 *      If the value of wchar is zero, wctomb() is left in  the
 *      initial shift state.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
int
wctomb(s, wchar)
    char *s;
    wchar_t wchar;
{

    if (s == NULL) {
	return 0;
    }
    return -1;
}

