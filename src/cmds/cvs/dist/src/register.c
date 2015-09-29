#ifndef lint
static char rcsid[] = "$Id: register.c,v 1.5 89/11/19 23:20:21 berliner Exp $";
#endif !lint

/*
 *    Copyright (c) 1989, Brian Berliner
 *
 *    You may distribute under the terms of the GNU General Public License
 *    as specified in the README file that comes with the CVS 1.0 kit.
 *
 * Register
 *
 *	Enters the given file name/version/time-stamp into the administration,
 *	removing the old entry first, if necessary.
 */

#include "cvs.h"

Register(fname, vn, ts)
    char *fname;
    char *vn;
    char *ts;
{
    FILE *fpin;

    Scratch_Entry(fname);
    fpin = open_file(CVSADM_ENT, "a");
    if (fprintf(fpin, "%s|%s|\n", vn, ts) == EOF)
	error(1, "cannot write to file %s", fname);
    (void) fclose(fpin);
}
