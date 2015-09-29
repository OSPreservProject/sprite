/* 
 * prog.c --
 *
 *	Program to ....
 *
 * Copyright 1992 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/lib/forms/RCS/main.c,v 1.4 92/03/02 15:27:51 bmiller Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <stdio.h>
#include <option.h>

Option optionArray[] = {
    {OPT_TRUE, "f", (char *)&foo, "Describe the -f option"},
};
int numOptions = sizeof(optionArray) / sizeof(Option);


/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	<explain>.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
main(argc, argv)
    int argc;
    char *argv[];
{
    argc = Opt_Parse(argc, argv, optionArray, numOptions, 0);

    exit(0);
}

