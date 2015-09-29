#ifndef lint
static char rcsid[] = "$Id: tag.c,v 1.19.1.2 91/01/29 19:45:54 berliner Exp $";
#endif !lint

/*
 *    Copyright (c) 1989, Brian Berliner
 *
 *    You may distribute under the terms of the GNU General Public License
 *    as specified in the README file that comes with the CVS 1.0 kit.
 *
 * Tag
 *
 *	Add or delete a symbolic name to an RCS file, or a collection
 *	of RCS files.  Uses the modules database, if necessary.
 */

#include <sys/param.h>
#include <ndbm.h>
#include <dirent.h>
#include <ctype.h>
#include "cvs.h"

extern char update_dir[];
extern int run_module_prog;
extern DBM *open_module();

static char *symtag;
static char *numtag = "";		/* must be null string, not pointer */
static int delete = 0;			/* adding a tag by default */
static int tag_recursive = 1;		/* recursive by default */

tag(argc, argv)
    int argc;
    char *argv[];
{
    register int i;
    int c;
    DBM *db;
    int err = 0;

    if (argc == -1)
	tag_usage();
    optind = 1;
    while ((c = getopt(argc, argv, "nfQqldr:D:")) != -1) {
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
	    tag_recursive = 0;
	    break;
	case 'd':
	    delete = 1;
	    /* FALL THROUGH */
	case 'f':
	    /*
	     * Only makes sense when the -r option is specified, or deleting
	     */
	    force_tag_match = 1;
	    break;
	case 'r':
	    numtag = optarg;
	    break;
	case 'D':
	    Make_Date(optarg, Date);
	    break;
	case '?':
	default:
	    tag_usage();
	    break;
	}
    }
    argc -= optind;
    argv += optind;
    if (argc < 2)
	tag_usage();
    symtag = argv[0];
    argc--;
    argv++;
    /*
     * Do some consistency checks on the symbolic tag... I'm not sure
     * how these relate to the checks that RCS does.
     */
    if (isdigit(symtag[0]) || index(symtag, '.') ||
	index(symtag, ':') || index(symtag, ';'))
	error(0, "symbolic tag %s must not contain any of '.:;' or start with 0-9",
	      symtag);
    db = open_module();
    for (i = 0; i < argc; i++)
	err += do_module(db, argv[i], TAG, "Tagging");
    close_module(db);
    exit(err);
}

/*
 * This is the recursive function that adds/deletes tags from
 * RCS files.  If the "rcs" argument is NULL, descend the current
 * directory, tagging all the files as appropriate; otherwise, just
 * tag the argument rcs file
 */
tagit(rcs)
    char *rcs;
{
    DIR *dirp;
    struct dirent *dp;
    char line[10];
    char *cp;
    int err = 0;

    if (rcs == NULL) {
	if ((dirp = opendir(".")) == NULL) {
	    err++;
	} else {
	    (void) sprintf(line, ".*%s$", RCSEXT);
	    if ((cp = re_comp(line)) != NULL) {
		warn(0, "%s", cp);
		err++;
	    } while ((dp = readdir(dirp)) != NULL) {
		if (strcmp(dp->d_name, ".") == 0 ||
		    strcmp(dp->d_name, "..") == 0 ||
		    strcmp(dp->d_name, CVSLCK) == 0)
		    continue;
		if (strcmp(dp->d_name, CVSATTIC) == 0 &&
		    !delete && numtag[0] == '\0')
		    continue;
		if (isdir(dp->d_name) && tag_recursive) {
		    char cwd[MAXPATHLEN];

		    if (getwd(cwd) == NULL) {
			warn(0, "cannot get working directory: %s", cwd);
			continue;
		    }
		    if (update_dir[0] == '\0') {
			(void) strcpy(update_dir, dp->d_name);
		    } else {
			(void) strcat(update_dir, "/");
			(void) strcat(update_dir, dp->d_name);
		    }
		    if (!quiet) {
			printf("%s %s: Tagging %s\n",
			       progname, command, update_dir);
		    }
		    if (chdir(dp->d_name) < 0) {
			warn(0, "chdir failed, %s ignored", update_dir);
			continue;
		    }
		    err += tagit((char *)0);
		    if ((cp = rindex(update_dir, '/')) != NULL)
			*cp = '\0';
		    else
			update_dir[0] = '\0';
		    if (chdir(cwd) < 0)
			error(1, "cannot chdir back to %s", cwd);
		    continue;
		}
		if (re_exec(dp->d_name))
		    err += tag_file(dp->d_name);
	    }
	}
	if (dirp)
	    (void) closedir(dirp);
    } else {
	return (tag_file(rcs));
    }
    return (err);
}

/*
 * Called to tag a particular file, as appropriate with the options
 * that were set above.
 */
tag_file(rcs)
    char *rcs;
{
    char version[50];

#ifdef S_IFLNK
    /*
     * Ooops..  if there is a symbolic link in the source repository
     * which points to a normal file, rcs will do its file renaming
     * mumbo-jumbo and blow away the symbolic link; this is essentially
     * like a copy-on-commit RCS revision control file, but I consider
     * it a bug.  Instead, read the contents of the link and recursively
     * tag the link contents (which may be a symlink itself...).
     */
    if (islink(rcs) && isfile(rcs)) {
	char link_name[MAXPATHLEN];
	int count;

	if ((count = readlink(rcs, link_name, sizeof(link_name))) != -1) {
	    link_name[count] = '\0';
	    return (tag_file(link_name));
	}
    }
#endif

    if (delete) {
	/*
	 * If -d is specified, "force_tag_match" is set, so that this call
	 * to Version_Number() will return a NULL version string if
	 * the symbolic tag does not exist in the RCS file.
	 *
	 * If the -r flag was used, numtag is set, and we only delete
	 * the symtag from files that have contain numtag.
	 *
	 * This is done here because it's MUCH faster than just blindly
	 * calling "rcs" to remove the tag... trust me.
	 */
	if (numtag[0] != '\0') {
	    Version_Number(rcs, numtag, "", version);
	    if (version[0] == '\0')
		return (0);
	}
	Version_Number(rcs, symtag, "", version);
	if (version[0] == '\0')
	    return (0);
	(void) sprintf(prog, "%s/%s -q -N%s %s 2>%s", Rcsbin, RCS,
		       symtag, rcs, DEVNULL);
	if (system(prog) != 0) {
	    warn(0, "failed to remove tag %s for %s", symtag, rcs);
	    return (1);
	}
	return (0);
    }
    Version_Number(rcs, numtag, Date, version);
    if (version[0] == '\0') {
	if (!quiet) {
	    warn(0, "cannot find tag '%s' for %s", numtag[0] ? numtag : "head",
		 rcs);
	    return (1);
	}
	return (0);
    }
    if (isdigit(numtag[0]) && strcmp(numtag, version) != 0) {
	/*
	 * We didn't find a match for the numeric tag that was specified,
	 * but that's OK.  just pass the numeric tag on to rcs, to be
	 * tagged as specified
	 */
	(void) strcpy(version, numtag);
    }
    (void) sprintf(prog, "%s/%s -q -N%s:%s %s", Rcsbin, RCS, symtag,
		   version, rcs);
    if (system(prog) != 0) {
	warn(0, "failed to set tag %s to revision %s for %s",
	     symtag, version, rcs);
	return (1);
    }
    return (0);
}

static
tag_usage()
{
    (void) fprintf(stderr,
	"Usage: %s %s [-Qqlfn] [-d] [-r tag|-D date] tag modules...\n",
		   progname, command);
    exit(1);
}
