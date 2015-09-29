#ifndef lint
static char rcsid[] = "$Id: log.c,v 1.9 89/11/19 23:40:37 berliner Exp $";
#endif !lint

/*
 *    Copyright (c) 1989, Brian Berliner
 *
 *    You may distribute under the terms of the GNU General Public License
 *    as specified in the README file that comes with the CVS 1.0 kit.
 *
 * Print Log Information
 *
 *	Prints the RCS "log" (rlog) information for the specified
 *	files.  With no argument, prints the log information for
 *	all the files in the directory.
 */

#include "cvs.h"

log(argc, argv)
    int argc;
    char *argv[];
{
    register int i;
    int numopt, err = 0;
    char *cp;

    if (argc == -1)
	log_usage();
    Name_Repository();
    /*
     * All 'log' command options are passed directly on to 'rlog'
     */
    numopt = Get_Options(--argc, ++argv);
    argc -= numopt;
    argv += numopt;
    if (argc == 0) {
	Find_Names(&fileargc, fileargv, ALL);
	argc = fileargc;
	argv = fileargv;
    }
    (void) sprintf(prog, "%s/%s %s", Rcsbin, RCS_RLOG, Options);
    cp = prog + strlen(prog);
    for (i = 0; i < argc; i++) {
	(void) strcpy(User, argv[i]);
	Locate_RCS();
	(void) strcpy(cp, " ");
	(void) strcpy(cp+1, Rcs);
	cp += strlen(Rcs) + 1;
    }
    err = system(prog);
    exit(err);
}

static
log_usage()
{
    (void) fprintf(stderr,
		   "%s %s [rlog-options] [files...]\n", progname, command);
    exit(1);
}
