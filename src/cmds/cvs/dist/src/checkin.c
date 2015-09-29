#ifndef lint
static char rcsid[] = "$Id: checkin.c,v 1.10 89/11/19 23:19:46 berliner Exp $";
#endif !lint

/*
 *    Copyright (c) 1989, Brian Berliner
 *
 *    You may distribute under the terms of the GNU General Public License
 *    as specified in the README file that comes with the CVS 1.0 kit.
 *
 * Check In
 *
 *	Does a very careful checkin of the file "User", and tries not
 *	to spoil its modification time (to avoid needless recompilations).
 *	When RCS ID keywords get expanded on checkout, however, the
 *	modification time is updated and there is no good way to get
 *	around this.
 *
 *	Returns non-zero on error.
 */

#include <sys/param.h>
#include <ctype.h>
#include "cvs.h"

Checkin(revision, message)
    char *revision;
    char *message;
{
    FILE *fp;
    char fname[MAXPATHLEN];
    char *tag;
    int err = 0;

    /*
     * The revision that is passed in includes the "-r" option
     * as well as a numeric revision, otherwise it is a pointer
     * to a null string.
     */
    if (revision[0] == '-')
	tag = &revision[2];
    else
	tag = revision;
    printf("Checking in %s;\n", User);
    if (!use_editor)
	printf("Log: %s\n", message);
    (void) sprintf(Rcs, "%s/%s%s", Repository, User, RCSEXT);
    (void) sprintf(fname, "%s/%s%s", CVSADM, CVSPREFIX, User);
    /*
     * Move the user file to a backup file, so as to preserve its
     * modification times, then place a copy back in the original
     * file name for the checkin and checkout.
     */
    rename_file(User, fname);
    copy_file(fname, User);
    (void) sprintf(prog, "%s/%s -f %s %s", Rcsbin, RCS_CI, revision, Rcs);
    if ((fp = popen(prog, "w")) == NULL) {
	err++;
    } else {
	(void) fprintf(fp, "%s", message);
	err = pclose(fp);
    }
    if (err == 0) {
	/*
	 * The checkin succeeded, so now check the new file back out
	 * and see if it matches exactly with the one we checked in.
	 * If it does, just move the original user file back, thus
	 * preserving the modes; otherwise, we have no recourse but
	 * to leave the newly checkout file as the user file and remove
	 * the old original user file.
	 */
	(void) sprintf(prog, "%s/%s -q %s %s", Rcsbin, RCS_CO, revision, Rcs);
	(void) system(prog);
	xchmod(User, 1);		/* make it writable */
	if (xcmp(User, fname) == 0)
	    rename_file(fname, User);
	else
	    (void) unlink(fname);
	/*
	 * If we want read-only files, muck the permissions here,
	 * before getting the user time-stamp.
	 */
	if (cvswrite == FALSE) {
	    xchmod(User, 0);
	}
	Version_TS(Rcs, tag, User);
	Register(User, VN_Rcs, TS_User);
    } else {
	/*
	 * The checkin failed, for some unknown reason, so we restore
	 * the original user file, print an error, try to unlock the
	 * (supposedly locked) RCS file, and try to restore
	 * any default branches, if they applied for this file.
	 */
	rename_file(fname, User);
	warn(0, "could not check in %s", User);
	(void) sprintf(prog, "%s/%s -u %s", Rcsbin, RCS, Rcs);
	if (system(prog) != 0)
	    warn(0, "could not UNlock %s", Rcs);
	restore_branch();
	return (1);
    }
    if (revision[0] != '\0') {
	/*
	 * When checking in a specific revision, we may have locked the
	 * wrong branch, so to be sure, we do an extra unlock here
	 * before returning.
	 */
	(void) sprintf(prog, "%s/%s -q -u %s 2>%s", Rcsbin, RCS, Rcs, DEVNULL);
	(void) system(prog);
    }
    return (0);
}

/*
 * Called when the above checkin fails, because we may have to
 * restore the default branch that existed before we attempted
 * the checkin.
 *
 * Scan Blist for a match of the User file, and if it has a branch
 * number tagged with it, do the "rcs -b" to set it back.
 */
static
restore_branch()
{
    char blist[MAXLISTLEN];
    char *cp, *user;

    (void) strcpy(blist, Blist);
    while ((cp = index(blist, ':')) != NULL) {
	user = cp;
	/*
	 * The next line is safe because we "know" that
	 * Blist always starts with a space if it has entries.
	 */
	while (!isspace(user[-1]))
	    user--;
	*cp++ = '\0';
	if (strcmp(User, user) == 0) {
	    (void) sprintf(prog, "%s/%s -q -b%s %s", Rcsbin, RCS,
			   cp, Rcs);
	    if (system(prog) != 0)
		warn(0, "cannot restore default branch %s for %s",
		     cp, Rcs);
	    return;
	}
    }
}
