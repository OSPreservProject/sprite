#ifndef lint
static char rcsid[] = "$Id: options.c,v 1.5 89/11/19 23:20:18 berliner Exp $";
#endif !lint

/*
 *    Copyright (c) 1989, Brian Berliner
 *
 *    You may distribute under the terms of the GNU General Public License
 *    as specified in the README file that comes with the CVS 1.0 kit.
 *
 * Get Options
 *
 * Collects options from argc/argv and stuffs them into the
 * global Options variable.
 *
 * Returns the number of options grabbed.
 */

#include "cvs.h"

Get_Options(argc, argv)
    int argc;
    char *argv[];
{
    register int i;
    register int numopts = 0;

    Options[0] = '\0';			/* Assume none */
    for (i = 0; i < argc; i++) {
	if (argv[i][0] == '-' || argv[i][0] == '\0') {
	    numopts++;
	    (void) strcat(Options, " ");
	    (void) strcat(Options, argv[i]);
	}
    }
    return (numopts);
}
