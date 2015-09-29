#ifndef lint
static char rcsid[] = "$Id: build_entry.c,v 1.2 91/07/29 11:53:37 jhh Exp $";
#endif !lint

/*
 *    Copyright (c) 1989, Brian Berliner
 *
 *    You may distribute under the terms of the GNU General Public License
 *    as specified in the README file that comes with the CVS 1.0 kit.
 *
 * Build Entry
 *
 *	Builds an entry for a new file and sets up "CVS.adm/file",[pt] by
 *	interrogating the user.
 *
 *	Returns non-zero on error.
 */

#include <sys/param.h>
#include "cvs.h"

Build_Entry(message)
    char *message;
{
    char fname[MAXPATHLEN];
    char line[MAXLINELEN];
    FILE *fp, *fptty;

    /*
     * There may be an old file with the same name in the Attic!
     * This is, perhaps, an awkward place to check for this, but
     * other places are equally awkward.
     */
    (void) sprintf(fname, "%s/%s/%s%s", Repository, CVSATTIC, User, RCSEXT);
    if (isreadable(fname)) {
	warn(0, "there is an old file %s already in %s/%s", User,
	     Repository, CVSATTIC);
	return (1);
    }
    /*
     * The options for the "add" command are store in the file CVS.adm/User,p
     */
    (void) sprintf(fname, "%s/%s%s", CVSADM, User, CVSEXT_OPT);
    fp = open_file(fname, "w+");
    if (fprintf(fp, "%s\n", Options) == EOF)
	error(1, "cannot write %s", fname);
    (void) fclose(fp);
    /*
     * And the requested log is read directly from the user and stored
     * in the file User,t.  If the "message" argument is set, then the
     * user specified the -m option to add, and it is not necessary to
     * query him from the terminal.
     */
    (void) sprintf(fname, "%s/%s%s", CVSADM, User, CVSEXT_LOG);
    fp = open_file(fname, "w+");
    if (message[0] == '\0') {
	printf("RCS file: %s\n", Rcs);
	printf("enter description, terminated with ^D or '.':\n");
	printf("NOTE: This is NOT the log message!\n");
#ifdef sprite
	fptty = open_file(getenv("TTY"), "r");
#else
	fptty = open_file("/dev/tty", "r");
#endif
	for (;;) {
	    printf(">> ");
	    (void) fflush(stdout);
	    if (fgets(line, sizeof(line), fptty) == NULL ||
		(line[0] == '.' && line[1] == '\n'))
		break;
	    if (fputs(line, fp) == EOF)
		error(1, "cannot write to %s", fname);
	}
	printf("done\n");
	(void) fclose(fptty);
    } else {
	if (fputs(message, fp) == EOF)
	    error(1, "cannot write to %s", fname);
    }
    (void) fclose(fp);
    /*
     * Create the entry now, since this allows the user to interrupt
     * us above without needing to clean anything up (well, we could
     * clean up the ,p and ,t files, but who cares).
     */
    (void) sprintf(line, "Initial %s", User);
    Register(User, "0", line);
    return (0);
}
