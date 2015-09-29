#ifndef lint
static char rcsid[] = "$Id: status.c,v 1.13 89/11/19 23:40:44 berliner Exp $";
#endif !lint

/*
 *    Copyright (c) 1989, Brian Berliner
 *
 *    You may distribute under the terms of the GNU General Public License
 *    as specified in the README file that comes with the CVS 1.0 kit.
 *
 * Status Information
 *
 *	Prints three lines of information for each of its arguments,
 *	one for the user file (line 1), one for the newest RCS file
 *	(line 3) and one for the RCS file both derive from (line 2).
 */

#include "cvs.h"

status(argc, argv)
    int argc;
    char *argv[];
{
    register int i;
    int c;
    int long_format = 0;

    if (argc == -1)
	status_usage();
    optind = 1;
    while ((c = getopt(argc, argv, "l")) != -1) {
	switch (c) {
	case 'l':
	    /*
	     * XXX - long format not done yet; should probably display
	     * other people that have checked out the current repository,
	     * or pieces thereof.
	     */
	    long_format = 1;
	    break;
	case '?':
	default:
	    status_usage();
	    break;
	}
    }
    argc -= optind;
    argv += optind;
    Name_Repository();
    if (long_format)
	error(0, "long format status output not done yet");
    if (argc == 0) {
	Find_Names(&fileargc, fileargv, ALL);
	argc = fileargc;
	argv = fileargv;
    }
    for (i = 0; i < argc; i++) {
	(void) strcpy(User, argv[i]);
	Locate_RCS();
	Version_TS(Rcs, Tag, User);
	if (TS_User[0] == '\0') {
	    printf("File:\tno file %s\n", User);
	} else {
	    printf("File:\t%s\n", User);
	}
	if (VN_User[0] == '\0') {
	    printf("From:\tno entry for %s\n", User);
	} else if (VN_User[0] == '0' && VN_User[1] == '\0') {
	    printf("From:\tNew file!\n");
	} else {
	    /*
	     * Only print the modification time
	     */
	    printf("From:\t%s\t%s\n", VN_User, &TS_Rcs[25]);
	}
	if (VN_Rcs[0] == '\0') {
	    printf("RCS:\tno %s\n", Rcs);
	} else {
	    printf("RCS:\t%s\t%s\n", VN_Rcs, Rcs);
	}
	printf("\n");
    }
    exit(0);
}

static
status_usage()
{
    (void) fprintf(stderr, "Usage: %s %s [files...]\n", progname, command);
    exit(1);
}
