#ifndef lint
static char rcsid[] = "$Id: checkout.c,v 1.19 89/11/19 23:40:30 berliner Exp $";
#endif !lint

/*
 *    Copyright (c) 1989, Brian Berliner
 *
 *    You may distribute under the terms of the GNU General Public License
 *    as specified in the README file that comes with the CVS 1.0 kit.
 *
 * Create Version
 *
 *	"checkout" creates a "version" of an RCS repository.  This version
 *	is owned totally by the user and is actually an independent
 *	copy, to be dealt with as seen fit.  Once "checkout" has been called
 *	in a given directory, it never needs to be called again.  The
 *	user can keep up-to-date by calling "update" when he feels like it;
 *	this will supply him with a merge of his own modifications
 *	and the changes made in the RCS original.  See "update" for details.
 *
 *	"checkout" can be given a list of directories or files to be updated
 *	and in the case of a directory, will recursivley create any
 *	sub-directories that exist in the repository.
 *
 *	When the user is satisfied with his own modifications, the
 *	present version can be committed by "commit"; this keeps the present
 *	version in tact, usually.
 *
 *	The call is
 *		cvs checkout [options] <module-name>...
 *
 *	And the options supported are:
 *		-f		Forces the symbolic tag specified with
 *				the -r option to match in the RCS file, else
 *				the RCS file is not extracted.
 *		-Q		Causes "update" to be really quiet.
 *		-q		Causes "update" and tag mis-matches to
 *				be quiet; "update" just doesn't print a
 *				message as it chdirs down a level.
 *		-c		Cat's the modules file, sorted, to stdout.
 *		-n		Causes "update" to *not* run any checkout prog.
 *		-l		Only updates the local directory, not recursive.
 *		-p		Prunes empty directories after checking them out
 *		-r tag		Checkout revision 'tag', subject to the
 *				setting of the -f option.
 *		-D date-string	Checkout the most recent file equal to or
 *				before the specifed date.
 *
 *	"checkout" creates a directory ./CVS.adm, in which it keeps its
 *	administration, in two files, Repository and Entries.
 *	The first contains the name of the repository.  The second
 *	contains one line for each registered file,
 *	consisting of the version number it derives from,
 *	its time stamp at derivation time and its name.  Both files
 *	are normal files and can be edited by the user, if necessary (when
 *	the repository is moved, e.g.)
 */

#include <sys/param.h>
#include <ndbm.h>
#include "cvs.h"

extern int update_prune_dirs;
extern int update_recursive;
extern int run_module_prog;
extern DBM *open_module();

checkout(argc, argv)
    int argc;
    char *argv[];
{
    register int i;
    int c;
    DBM *db;
    int cat = 0, err = 0;

    if (argc == -1)
	checkout_usage();
    optind = 1;
    while ((c = getopt(argc, argv, "nflpQqcr:D:")) != -1) {
	switch (c) {
	case 'n':
	    run_module_prog = 0;
	    break;
	case 'Q':
	    really_quiet = 1;
	    /* FALL THROUGH */
	case 'q':
	    quiet = 1;
	    break;
	case 'l':
	    update_recursive = 0;
	    break;
	case 'p':
	    update_prune_dirs = 1;
	    break;
	case 'c':
	    cat = 1;
	    break;
	case 'f':
	    force_tag_match = 1;
	    break;
	case 'r':
	    (void) strcpy(Tag, optarg);
	    break;
	case 'D':
	    Make_Date(optarg, Date);
	    break;
	case '?':
	default:
	    checkout_usage();
	    break;
	}
    }
    argc -= optind;
    argv += optind;
    if ((!cat && argc == 0) || (cat && argc != 0) || (Tag[0] && Date[0]))
	checkout_usage();
    if (cat) {
	cat_module();
	exit(0);
    }
    db = open_module();
    for (i = 0; i < argc; i++)
	err += do_module(db, argv[i], CHECKOUT, "Updating");
    close_module(db);
    exit(err);
}

Build_Dirs_and_chdir(dir)
    char *dir;
{
    FILE *fp;
    char path[MAXPATHLEN];
    char *slash;
    char *cp;

    (void) strcpy(path, dir);
    for (cp = path; (slash = index(cp, '/')) != NULL; cp = slash+1) {
	*slash = '\0';
	(void) mkdir(cp, 0777);
	if (chdir(cp) < 0) {
	    warn(1, "cannot chdir to %s", cp);
	    return (1);
	}
	if (!isfile(CVSADM)) {
	    (void) sprintf(Repository, "%s/%s", CVSroot, path);
	    Create_Admin(Repository, DFLT_RECORD);
	    fp = open_file(CVSADM_ENTSTAT, "w+");
	    (void) fclose(fp);
	}
	*slash = '/';
    }
    (void) mkdir(cp, 0777);
    if (chdir(cp) < 0) {
	warn(1, "cannot chdir to %s", cp);
	return (1);
    }
    return (0);
}

static
checkout_usage()
{
    (void) fprintf(stderr,
	"Usage: %s %s [-Qqlfnp] [-c] [-r tag|-D date] modules...\n",
		   progname, command);
    exit(1);
}
