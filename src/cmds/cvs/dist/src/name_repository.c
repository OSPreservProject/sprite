#ifndef lint
static char rcsid[] = "$Id: name_repository.c,v 1.9 89/11/19 23:20:13 berliner Exp $";
#endif !lint

/*
 *    Copyright (c) 1989, Brian Berliner
 *
 *    You may distribute under the terms of the GNU General Public License
 *    as specified in the README file that comes with the CVS 1.0 kit.
 *
 * Name of Repository
 *
 *	Determine the name of the RCS repository and sets "Repository"
 *	accordingly.
 */

#include <sys/param.h>
#include "cvs.h"

Name_Repository()
{
    FILE *fpin;
    char path[MAXPATHLEN];
    char *cp;

    if (!isdir(CVSADM))
	error(0, "there is no version here; do '%s checkout' first", progname);
    if (!isreadable(CVSADM_REP) || !isreadable(CVSADM_ENT))
	error(0, "*PANIC* administration files missing");
    /*
     * The assumption here is the the repository is always contained
     * in the first line of the "Repository" file.
     */
    fpin = open_file(CVSADM_REP, "r");
    if (fgets(Repository, MAXPATHLEN, fpin) == NULL)
	error(1, "cannot read %s", CVSADM_REP);
    (void) fclose(fpin);
    if ((cp = rindex(Repository, '\n')) != NULL)
	*cp = '\0';			/* strip the newline */
    /*
     * If this is a relative repository pathname, turn it into
     * an absolute one by tacking on the CVSROOT environment variable.
     * If the CVSROOT environment variable is not set, die now.
     */
    if (Repository[0] != '/') {
	if (CVSroot == NULL) {
	    (void) fprintf(stderr,
			   "%s: must set the CVSROOT environment variable\n",
			   progname);
	    error(0, "or specify the '-d' option to %s", progname);
	}
	(void) strcpy(path, Repository);
	(void) sprintf(Repository, "%s/%s", CVSroot, path);
    }
    if (!isdir(Repository))
	error(0, "there is no repository %s", Repository);
}
