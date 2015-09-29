#ifndef lint
static char rcsid[] = "$Id: diff.c,v 1.12 89/11/19 23:40:34 berliner Exp $";
#endif !lint

/*
 *    Copyright (c) 1989, Brian Berliner
 *
 *    You may distribute under the terms of the GNU General Public License
 *    as specified in the README file that comes with the CVS 1.0 kit.
 *
 * Difference
 *
 *	Run diff against versions in the repository.  Options that
 *	are specified are passed on directly to "rcsdiff".
 *
 *	Without any file arguments, runs diff against all the
 *	currently modified files, as listed in the CVS.adm/Mod file.
 */

#include <sys/param.h>
#include "cvs.h"

diff(argc, argv)
    int argc;
    char *argv[];
{
    register int i;
    char rev1[50], rev2[50], tmp[MAXPATHLEN];
    char *revision;
    int c, numopt, err = 0;

    if (argc == -1)
	diff_usage();
    Name_Repository();
    /*
     * Note that we catch all the valid arguments here, so that we
     * can intercept the -r arguments for doing revision diffs
     */
    rev1[0] = rev2[0] = '\0';
    optind = 1;
    while ((c = getopt(argc, argv, "biwtcefhnqr:")) != -1) {
	switch (c) {
	case 'b':	case 'i':	case 'w':
	case 't':	case 'c':	case 'e':
	case 'f':	case 'h':	case 'n':
	case 'q':
	    /* Get_Options will take care of these */
	    break;
	case 'r':
	    if (rev2[0] != '\0')
		error(0, "no more than two revisions can be specified");
	    if (rev1[0] != '\0') {
		(void) strcpy(rev2, optarg);
	    } else {
		(void) strcpy(rev1, optarg);
	    }
	    *optarg = '\0';		/* strip the -r */
	    break;
	case '?':
	default:
	    diff_usage();
	    break;
	}
    }
    numopt = Get_Options(--argc, ++argv);
    argc -= numopt;
    argv += numopt;
    if (argc == 0) {
	if (rev2[0] != '\0')
	    Find_Names(&fileargc, fileargv, MOD);
	else
	    Find_Names(&fileargc, fileargv, ALL);
	argc = fileargc;
	argv = fileargv;
	if (rev2[0] == '\0') {
	    for (i = 0; i < fileargc; i++) {
		(void) strcpy(User, fileargv[i]);
		Locate_RCS();
		if (rev1[0] != '\0') {
		    revision = rev1;
		} else {
		    Version_TS(Rcs, Tag, User);
		    if (VN_User[0] == '\0' || VN_User[0] == '-' ||
			(VN_User[0] == '0' && VN_User[1] == '\0')) {
			fileargv[i][0] = '\0';
		    } else if (VN_Rcs[0] == '\0' || TS_User[0] == '\0' ||
			       strcmp(TS_Rcs, TS_User) == 0) {
			fileargv[i][0] = '\0';
		    } else {
			revision = VN_User;
		    }
		}
		if (fileargv[i][0] != '\0') {
		    (void) sprintf(tmp, "%s/%s%s", CVSADM, CVSPREFIX, User);
		    (void) sprintf(prog, "%s/%s -p -q -r%s %s > %s", Rcsbin,
				   RCS_CO, revision, Rcs, tmp);
		    if (system(prog) == 0 && xcmp(User, tmp) == 0)
			fileargv[i][0] = '\0';
		    (void) unlink(tmp);
		}
	    }
	}
    }
    for (i = 0; i < argc; i++) {
	if (argv[i][0] == '\0')
	    continue;
	(void) strcpy(User, argv[i]);
	Locate_RCS();
	Version_TS(Rcs, Tag, User);
	if (VN_User[0] == '\0') {
	    warn(0, "I know nothing about %s", User);
	    continue;
	} else if (VN_User[0] == '0' && VN_User[1] == '\0') {
	    warn(0, "%s is a new entry, no comparison available",
		 User);
	    continue;
	} else if (VN_User[0] == '-') {
	    warn(0, "%s was removed, no comparison available",
		 User);
	    continue;
	} else {
	    if (VN_Rcs[0] == '\0') {
		warn(0, "cannot find %s", Rcs);
		continue;
	    } else {
		if (TS_User[0] == '\0') {
		    warn(0, "cannot find %s", User);
		    continue;
		}
	    }
	}
	(void) fflush(stdout);
	if (i != 0) {
	    printf("===================================================================\n");
	    (void) fflush(stdout);
	}
	if (rev2[0] != '\0') {
	    (void) sprintf(prog, "%s/%s %s -r%s -r%s %s", Rcsbin, RCS_DIFF,
			   Options, rev1, rev2, Rcs);
	} else if (rev1[0] != '\0') {
	    (void) sprintf(prog, "%s/%s %s -r%s %s", Rcsbin, RCS_DIFF,
			   Options, rev1, Rcs);
	} else {
	    (void) sprintf(prog, "%s/%s %s -r%s %s", Rcsbin, RCS_DIFF,
			   Options, VN_User, Rcs);
	}
	(void) strcat(prog, " 2>&1");
	err += system(prog);
	(void) fflush(stdout);
    }
    exit(err);
}

diff_usage()
{
    (void) fprintf(stderr, "%s %s [rcsdiff-options] [files...]\n", progname,
		   command);
    exit(1);
}
