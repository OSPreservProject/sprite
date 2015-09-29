#ifndef lint
static char rcsid[] = "$Id: join.c,v 1.4 89/11/19 23:40:36 berliner Exp $";
#endif !lint

/*
 *    Copyright (c) 1989, Brian Berliner
 *
 *    You may distribute under the terms of the GNU General Public License
 *    as specified in the README file that comes with the CVS 1.0 kit.
 *
 * Join
 *
 *	Do an implicit checkout and merge between RCS revisions in the
 *	RCS file.  This is most always used after the "checkin" script
 *	has added new revisions to files that have been locally modified.
 *	A manual "join" command can be run on the files listed in the
 *	"checkin" output to produce an "rcsmerge" of the current revision
 *	and the specified vendor branch revision.
 *
 *	Need to be careful if the local user file has been checked out and
 *	is modified.  Don't want to mess with it in this case.
 */

#include "cvs.h"

extern char update_dir[];
extern int force_tag_match;

join(argc, argv)
    int argc;
    char *argv[];
{
    register int i;
    int c, err = 0;

    if (argc == -1)
	join_usage();
    optind = 1;
    while ((c = getopt(argc, argv, "fQqD:r:")) != -1) {
	switch (c) {
	case 'Q':
	    really_quiet = 1;
	    /* FALL THROUGH */
	case 'q':
	    quiet = 1;
	    break;
	case 'f':
	    force_tag_match = 1;
	    break;
	case 'D':
	    if (Tag[0] != '\0' || Date[0] != '\0')
		error(0, "no more than one revision/date can be specified");
	    Make_Date(optarg, Date);
	    break;
	case 'r':
	    if (Tag[0] != '\0' || Date[0] != '\0')
		error(0, "no more than one revision/date can be specified");
	    (void) strcpy(Tag, optarg);
	    break;
	case '?':
	default:
	    join_usage();
	    break;
	}
    }
    argc -= optind;
    argv += optind;
    if (argc < 1)
	join_usage();
    if (Tag[0] == '\0' && Date[0] == '\0')
	error(0, "must specify one revision/date!");
    Name_Repository();
    Reader_Lock();
    (void) Collect_Sets(argc, argv);
    if (Clist[0] || Glist[0] || Mlist[0] || Alist[0] || Rlist[0] || Wlist[0])
	error(0, "conflicts exist; cannot join a modified file");
    if (Dlist[0])
	error(0, "cannot join directories -%s", Dlist);
    for (i = 0; i < argc; i++)
	err += join_file(argv[i]);
    Lock_Cleanup(0);
    exit(err);
}

/*
 * Called for each file that is to be "join"ed.  Need to find out if the
 * file is modified or not, and blow it off if it is.
 */
static
join_file(file)
    char *file;
{
    char vers[50];

    (void) strcpy(User, file);
    (void) sprintf(Rcs, "%s/%s%s", Repository, User, RCSEXT);
    Version_Number(Rcs, Tag, Date, vers);
    if (vers[0] == '\0') {
	if (!quiet)
	    warn(0, "cannot find revision %s for %s", Tag[0] ? Tag:Date, Rcs);
	return (1);
    }
    (void) unlink(User);
    Scratch_Entry(User);
    (void) sprintf(prog, "%s/%s -j%s %s %s", Rcsbin, RCS_CO, vers, Rcs, User);
    if (system(prog) != 0) {
	if (!quiet)
	    warn(0, "co of revision %s for %s failed", VN_Rcs, Rcs);
	return (1);
    }
    if (!really_quiet)
	printf("J %s\n", User);
    if (cvswrite == TRUE)
	xchmod(User, 1);
    Version_TS(Rcs, "", User);		/* For head */
    (void) sprintf(TS_User, "joined %s", User);	/* To appear to be modified */
    Register(User, VN_Rcs, TS_User);
    return (0);
}

static
join_usage()
{
    (void) fprintf(stderr, "%s %s [-Qqf] [-r tag|-D date] files...\n",
		   progname, command);
    exit(1);
}
