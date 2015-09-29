#ifndef lint
static char rcsid[] = "$Id: locate_rcs.c,v 1.5 89/11/19 23:20:05 berliner Exp $";
#endif !lint

/*
 *    Copyright (c) 1989, Brian Berliner
 *
 *    You may distribute under the terms of the GNU General Public License
 *    as specified in the README file that comes with the CVS 1.0 kit.
 *
 * Locate RCS File
 *
 * Called when the RCS file sought may be in the Attic directory.
 * Sets the global Rcs variable to the correct file.
 */

#include <sys/param.h>
#include "cvs.h"

Locate_RCS()
{
    char old[MAXPATHLEN];

    (void) sprintf(Rcs, "%s/%s%s", Repository, User, RCSEXT);
    (void) sprintf(old, "%s/%s/%s%s", Repository, CVSATTIC, User, RCSEXT);
    if (!isreadable(Rcs)) {
	if (isreadable(old)) {
	    (void) strcpy(Rcs, old);
	} else {
	    /*
	     * it is treated as if it were in the repository
	     */
	}
    }
}
