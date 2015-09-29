#ifndef lint
static char rcsid[] = "$Id: remove.c,v 1.9 89/11/19 23:40:43 berliner Exp $";
#endif !lint

/*
 *    Copyright (c) 1989, Brian Berliner
 *
 *    You may distribute under the terms of the GNU General Public License
 *    as specified in the README file that comes with the CVS 1.0 kit.
 *
 * Remove a File
 *
 *	Removes entries from the present version.
 *	The entries will be removed from the RCS repository upon the
 *	next "commit".
 *
 *	"remove" accepts no options, only file names that are to be
 *	removed.  The file must not exist in the current directory
 *	for "remove" to work correctly.
 */

#include <sys/param.h>
#include "cvs.h"

remove(argc, argv)
    int argc;
    char *argv[];
{
    register int i;
    char fname[MAXPATHLEN];
    int err = 0;

    if (argc == 1 || argc == -1)
	remove_usage();
    argc--;
    argv++;
    Name_Repository();
    for (i = 0; i < argc; i++) {
	(void) strcpy(User, argv[i]);
	Version_TS(Rcs, Tag, User);
	if (TS_User[0] != '\0') {
	    warn(0, "%s still exists", User);
	    err++;
	    continue;
	}
	if (VN_User[0] == '\0') {
	    warn(0, "there is no entry for %s", User);
	    err++;
	} else if (VN_User[0] == '0' && VN_User[1] == '\0') {
	    /*
	     * It's a file that has been added, but not commited yet.
	     * So, remove the ,p and ,t file for it and scratch it from
	     * the entries file.
	     */
	    Scratch_Entry(User);
	    (void) sprintf(fname, "%s/%s%s", CVSADM, User, CVSEXT_OPT);
	    (void) unlink(fname);
	    (void) sprintf(fname, "%s/%s%s", CVSADM, User, CVSEXT_LOG);
	    (void) unlink(fname);
	} else if (VN_User[0] == '-') {
	    /*
	     * It's already been flagged for removal, nothing more to do.
	     */
	    warn(0, "%s was already removed", User);
	    err++;
	} else {
	    /*
	     * Re-register it with a negative version number.
	     */
	    (void) strcpy(fname, "-");
	    (void) strcat(fname, VN_User);
	    Register(User, fname, TS_Rcs);
	}
    }
    Entries2Files();			/* and update the Files file */
    exit(err);
}

static
remove_usage()
{
    (void) fprintf(stderr, "%s %s files...\n", progname, command);
    exit(1);
}
