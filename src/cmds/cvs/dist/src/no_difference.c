#ifndef lint
static char rcsid[] = "$Id: no_difference.c,v 1.7.1.1 91/01/29 07:18:06 berliner Exp $";
#endif !lint

/*
 *    Copyright (c) 1989, Brian Berliner
 *
 *    You may distribute under the terms of the GNU General Public License
 *    as specified in the README file that comes with the CVS 1.0 kit.
 *
 * No Difference
 *
 * The user file looks modified judging from its time stamp; however
 * it needn't be.  No_difference() finds out whether it is or not.
 * If it is, it adds its name to the Mlist.
 * If it is not, it updates the administration.
 *
 * If we're deciding if a modified file that is to be merged is really
 * modified (doGlist is set), we add the name to the Glist if it really
 * is modified, otherwise it is added to the Olist to be simply extracted.
 *
 * Returns non-zero on error.
 */

#include <sys/param.h>
#include "cvs.h"

No_Difference(doGlist)
    int doGlist;
{
    char tmp[MAXPATHLEN];

    (void) sprintf(tmp, "%s/%s%s", CVSADM, CVSPREFIX, User);
    (void) sprintf(prog, "%s/%s -p -q -r%s %s > %s", Rcsbin, RCS_CO,
		   VN_User, Rcs, tmp);
    if (system(prog) == 0) {
	if (xcmp(User, tmp) == 0) {
	    if (cvswrite == FALSE)
		xchmod(User, 0);
	    Version_TS(Rcs, Tag, User);
	    (void) strcpy(TS_Rcs, TS_User);
	    Register(User, VN_User[0] ? VN_User : VN_Rcs, TS_User);
	    if (doGlist) {
		(void) strcat(Olist, " ");
		(void) strcat(Olist, User);
	    }
	} else {
	    if (!iswritable(User))
		xchmod(User, 1);
	    Version_TS(Rcs, Tag, User);
	    if (doGlist) {
		(void) strcat(Glist, " ");
		(void) strcat(Glist, User);
	    } else {
		(void) strcat(Mlist, " ");
		(void) strcat(Mlist, User);
	    }
	}
	(void) unlink(tmp);
    } else {
	warn(0, "could not check out revision %s of %s", VN_User, User);
	(void) unlink(tmp);
	return (1);
    }
    return (0);
}
