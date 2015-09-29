char rcsid[] = "$Id: main.c,v 1.2 91/09/10 16:12:21 jhh Exp $\nPatch level ###\n";

/*
 *    Copyright (c) 1989, Brian Berliner
 *
 *    You may distribute under the terms of the GNU General Public License
 *    as specified in the README file that comes with the CVS 1.0 kit.
 *
 * This is the main C driver for the CVS system.
 *
 * Credit to Dick Grune, Vrije Universiteit, Amsterdam, for writing
 * the shell-script CVS system that this is based on.
 *
 * Usage:
 *	cvs [options] command [options] [files/modules...]
 *
 * Where "command" is composed of:
 *		checkout	Check out a module/dir/file
 *		update		Brings work tree in sync with repository
 *		commit		Checks files into the repository
 *		diff		Runs diffs between revisions
 *		log		Prints to "rlog" information for files
 *		add		Adds an entry to the repository
 *		remove		Removes an entry from the repository
 *		status		Status info on the revisions
 *		join		Merge two RCS revisions
 *		patch		"patch" format diff listing between releases
 *		tag		Add/delete a symbolic tag to the RCS file
 *
 * Future:
 *		checkin		Adds new *directories* to the repository
 *				Currently being done by an external shell
 *				script.
 *
 * Brian Berliner
 * 4/20/89
 */

#include <sys/param.h>
#include "cvs.h"
#include "patchlevel.h"

char *progname, *command;

char *fileargv[MAXFILEPERDIR];
int fileargc;

int use_editor = TRUE;
int cvswrite = !CVSREAD_DFLT;
int really_quiet = FALSE;
int quiet = FALSE;
int force_tag_match = FALSE;

/*
 * Globals for the lists created in Collect_Sets()
 */
char Clist[MAXLISTLEN], Glist[MAXLISTLEN], Mlist[MAXLISTLEN];
char Olist[MAXLISTLEN], Alist[MAXLISTLEN], Rlist[MAXLISTLEN];
char Wlist[MAXLISTLEN], Llist[MAXLISTLEN], Blist[MAXLISTLEN];
char Dlist[MAXLISTLEN];

/*
 * Globals which refer to a particular file
 */
char Repository[MAXPATHLEN];
char User[MAXPATHLEN], Rcs[MAXPATHLEN];
char VN_User[MAXPATHLEN], TS_User[MAXPATHLEN];
char VN_Rcs[MAXPATHLEN], TS_Rcs[MAXPATHLEN];
char Tag[MAXPATHLEN], Date[MAXPATHLEN];

/*
 * Options is used to hold options passed on to system(), while prog
 * is alwys used in the calls to system().
 */
char Options[MAXPATHLEN];
char prog[MAXPROGLEN];

/*
 * Defaults, for the environment variables that are not set
 */
char *Rcsbin = RCSBIN_DFLT;
char *Editor = EDITOR_DFLT;
char *CVSroot = CVSROOT_DFLT;

main(argc, argv)
    int argc;
    char *argv[];
{
    extern char *getenv();
    register char *cp;
    register int c;
    int help = FALSE, err = 0;

    /*
     * Just save the last component of the path for error messages
     */
    if ((progname = rindex(argv[0], '/')) == NULL)
	progname = argv[0];
    else
	progname++;

    /*
     * Query the environment variables up-front, so that
     * they can be overridden by command line arguments
     */
    if ((cp = getenv(RCSBIN_ENV)) != NULL)
	Rcsbin = cp;
    if ((cp = getenv(EDITOR_ENV)) != NULL)
	Editor = cp;
    if ((cp = getenv(CVSROOT_ENV)) != NULL)
	CVSroot = cp;
    if (getenv(CVSREAD_ENV) != NULL)
	cvswrite = FALSE;

    optind = 1;
    while ((c = getopt(argc, argv, "rwvb:e:d:H")) != -1) {
	switch (c) {
	case 'r':
	    cvswrite = FALSE;
	    break;
	case 'w':
	    cvswrite = TRUE;
	    break;
	case 'v':
	    (void) sprintf(index(rcsid, '#'), "%d\n", PATCHLEVEL);
	    (void) fputs(rcsid, stdout);
	    (void) fputs("\nCopyright (c) 1989, Brian Berliner\n\n\
CVS may be copied only under the terms of the GNU General Public License,\n\
a copy of which can be found with the CVS 1.0 distribution kit.\n", stdout);
	    exit(0);
	    break;
	case 'b':
	    Rcsbin = optarg;
	    break;
	case 'e':
	    Editor = optarg;
	    break;
	case 'd':
	    CVSroot = optarg;
	    break;
	case 'H':
	    help = TRUE;
	    break;
	case '?':
	default:
	    usage();
	}
    }
    argc -= optind;
    argv += optind;
    if (argc < 1)
	usage();

    /*
     * Specifying just the '-H' flag to the sub-command causes a Usage
     * message to be displayed.
     */
    command = cp = argv[0];
    if (help == TRUE || (argc > 1 && strcmp(argv[1], "-H") == 0))
	argc = -1;

    /*
     * Make standard output line buffered, so that progress can be
     * monitored when redirected to a file, but only when we're not
     * running the "patch" command, as it generates lots of output
     * to stdout -- just leave it block buffered.
     */
    if (strcmp(cp, "patch") != 0)
	setlinebuf(stdout);

    if (strcmp(cp, "update") == 0)
	err += update(argc, argv);
    else if (strcmp(cp, "commit") == 0 || strcmp(cp, "ci") == 0)
	commit(argc, argv);
    else if (strcmp(cp, "diff") == 0)
	diff(argc, argv);
    else if (strcmp(cp, "checkout") == 0 || strcmp(cp, "co") == 0 ||
		strcmp(cp, "get") == 0)
	checkout(argc, argv);
    else if (strcmp(cp, "log") == 0)
	log(argc, argv);
    else if (strcmp(cp, "status") == 0)
	status(argc, argv);
    else if (strcmp(cp, "add") == 0)
	add(argc, argv);
    else if (strcmp(cp, "remove") == 0)
	remove(argc, argv);
    else if (strcmp(cp, "join") == 0)
	join(argc, argv);
    else if (strcmp(cp, "patch") == 0)
	patch(argc, argv);
    else if (strcmp(cp, "tag") == 0)
	tag(argc, argv);
    else if (strcmp(cp, "info") == 0)
	info(argc, argv);
   else
	usage();			/* oops.. no match */
    Lock_Cleanup(0);
    exit(err);
}

static
usage()
{
    (void) fprintf(stderr,
	"Usage: %s [cvs-options] command [command-options] [files...]\n",
		   progname);
    (void) fprintf(stderr, "\tWhere 'cvs-options' are:\n");
    (void) fprintf(stderr, "\t\t-r\t\tMake checked-out files read-only\n");
    (void) fprintf(stderr,
	"\t\t-w\t\tMake checked-out files read-write (default)\n");
    (void) fprintf(stderr, "\t\t-v\t\tCVS version and copyright\n");
    (void) fprintf(stderr, "\t\t-b bindir\tFind RCS programs in 'bindir'\n");
    (void) fprintf(stderr,
	"\t\t-e editor\tUse 'editor' for editing log information\n");
    (void) fprintf(stderr, "\t\t-d CVS_root_directory\n");
    (void) fprintf(stderr, "\t\t\t\t\Points to the root of the CVS tree\n");
    (void) fprintf(stderr, "\tand 'command' is:\n");
    (void) fprintf(stderr, "\t\tcheckout\tCreates a new CVS directory\n");
    (void) fprintf(stderr,
	"\t\tupdate\t\tBrings work tree in sync with repository\n");
    (void) fprintf(stderr,
	"\t\tcommit\t\tChecks files into the repository\n");
    (void) fprintf(stderr, "\t\tdiff\t\tRuns diffs between revisions\n");
    (void) fprintf(stderr,
	"\t\tlog\t\tPrints out 'rlog' information for files\n");
    (void) fprintf(stderr, "\t\tstatus\t\tStatus info on the revisions\n");
    (void) fprintf(stderr, "\t\tadd\t\tAdds an entry to the repository\n");
    (void) fprintf(stderr,
	"\t\tremove\t\tRemoves an entry from the repository\n");
    (void) fprintf(stderr,
	"\t\tjoin\t\tJoins an RCS revision with the head on checkout\n");
    (void) fprintf(stderr,
	"\t\tpatch\t\t\"patch\" format diffs between releases\n");
    (void) fprintf(stderr, "\t\ttag\t\tAdd a symbolic tag to the RCS file\n");
    (void) fprintf(stderr,
	"\tSpecify the '-H' option to each command for Usage information\n");
    exit(1);
}
