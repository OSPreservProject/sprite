#ifndef lint
static char rcsid[] = "$Id: add.c,v 1.1 91/07/11 12:57:46 jhh Exp Locker: jhh $";
#endif !lint

/*
 *    Copyright (c) 1989, Brian Berliner
 *
 *    You may distribute under the terms of the GNU General Public License
 *    as specified in the README file that comes with the CVS 1.0 kit.
 *
 * Add
 *
 *	Adds a file or directory to the RCS source repository.  For a file,
 *	the entry is marked as "needing to be added" in the user's own
 *	CVS.adm directory, and really added to the repository when it is
 *	committed.  For a directory, it is added at the appropriate place
 *	in the source repository and a CVS.adm directory is generated
 *	within the directory.
 *
 *	The -m option is currently the only supported option.  Some may wish
 *	to supply standard "rcs" options here, but I've found that this
 *	causes more trouble than anything else.
 *
 *	The user files or directories must already exist.  For a directory,
 *	it must not already have a CVS.adm file in it.
 *
 *	An "add" on a file that has been "remove"d but not committed will
 *	cause the file to be resurrected.
 */

#include <sys/param.h>
#include "cvs.h"

add(argc, argv)
    int argc;
    char *argv[];
{
    char tmp[MAXPATHLEN], message[MAXMESGLEN];
    register int i;
    int c, err = 0;

    if (argc == 1 || argc == -1)
	add_usage();
    message[0] = '\0';
    optind = 1;
    while ((c = getopt(argc, argv, "m:")) != -1) {
	switch (c) {
	case 'm':
	    if (strlen(optarg) >= sizeof(message)) {
		warn(0, "warning: message too long; truncated!");
		(void) strncpy(message, optarg, sizeof(message));
		message[sizeof(message) - 1] = '\0';
	    } else
		(void) strcpy(message, optarg);
	    break;
	case '?':
	default:
	    add_usage();
	    break;
	}
    }
    argc -= optind;
    argv += optind;
    Name_Repository();
    for (i = 0; i < argc; i++) {
	(void) strcpy(User, argv[i]);
	if (isdir(User)) {
	    err += add_directory(User);
	    continue;
	}
	if (islink(User)) {
	    err += add_link(User);
	    continue;
	}
	(void) sprintf(tmp, "%s/%s", Repository, User);
	if (isfile(tmp)) {
	    if (islink(tmp)) {
		warn(0, "%s already is a symbolic in the repository", User);
	    } else if (isdir(tmp)) {
		warn(0, "%s already is a directory in the repository", User);
	    }
	    err++;
	    continue;
	}
	(void) sprintf(Rcs, "%s/%s%s", Repository, User, RCSEXT);
	Version_TS(Rcs, Tag, User);
	if (VN_User[0] == '\0') {
	    /*
	     * No entry available, TS_Rcs is invalid
	     */
	    if (VN_Rcs[0] == '\0') {
		/*
		 * There is no RCS file either
		 */
		if (TS_User[0] == '\0') {
		    /*
		     * There is no user file either
		     */
		    warn(0, "nothing known about %s", User);
		    err++;
		} else {
		    /*
		     * There is a user file, so build the entry for it
		     */
		    if (Build_Entry(message) != 0)
			err++;
		}
	    } else {
		/*
		 * There is an RCS file already, so somebody else
		 * must've added it
		 */
		warn(0, "%s added independently by second party", User);
		err++;
	    }
	} else if (VN_User[0] == '0' && VN_User[1] == '\0') {
	    /*
	     * An entry for a new-born file, TS_Rcs is dummy,
	     * but that is inappropriate here
	     */
	    warn(0, "%s has already been entered", User);
	    err++;
	} else if (VN_User[0] == '-') {
	    /*
	     * An entry for a removed file, TS_Rcs is invalid
	     */
	    if (TS_User[0] == '\0') {
		/*
		 * There is no user file (as it should be)
		 */
		if (VN_Rcs[0] == '\0') {
		    /*
		     * There is no RCS file, so somebody else must've
		     * removed it from under us
		     */
		    warn(0, "cannot resurrect %s; RCS file removed by second party",
			 User);
		    err++;
		} else {
		    /*
		     * There is an RCS file, so remove the "-" from the
		     * version number and restore the file
		     */
		    (void) strcpy(tmp, VN_User+1);
		    (void) strcpy(VN_User, tmp);
		    (void) sprintf(tmp, "Resurrected %s", User);
		    Register(User, VN_User, tmp);
		    if (update(2, argv+i-1) == 0) {
			warn(0, "%s, version %s, resurrected", User, VN_User);
		    } else {
			warn(0, "could not resurrect %s", User);
			err++;
		    }
		}
	    } else {
		/*
		 * The user file shouldn't be there
		 */
		warn(0, "%s should be removed and is still there", User);
		err++;
	    }
	} else {
	    /*
	     * A normal entry, TS_Rcs is valid, so it must already be there
	     */
	    warn(0, "%s already exists, with version number %s", User, VN_User);
	    err++;
	}
    }
    Entries2Files();			/* update CVS.adm/Files file */
    exit(err);
}

/*
 * The specified user file is really a directory.  So, let's make sure that
 * it is created in the RCS source repository, and that the user's
 * directory is updated to include a CVS.adm directory.
 *
 * Returns 1 on failure, 0 on success.
 */
static
add_directory(dir)
    char *dir;
{
    char cwd[MAXPATHLEN], rcsdir[MAXPATHLEN];
    char message[MAXPATHLEN+100];

    if (index(dir, '/') != NULL) {
	warn(0, "directory %s not added; must be a direct sub-directory", dir);
	return (1);
    }
    if (strcmp(dir, CVSADM) == 0) {
	warn(0, "cannot add a '%s' directory", CVSADM);
	return (1);
    }
    if (getwd(cwd) == NULL) {
	warn(0, "cannot get working directory: %s", cwd);
	return (1);
    }
    if (chdir(dir) < 0) {
	warn(1, "cannot chdir to %s", dir);
	return (1);
    }
    if (isfile(CVSADM)) {
	warn(0, "%s/%s already exists", dir, CVSADM);
	goto out;
    }
    (void) sprintf(rcsdir, "%s/%s", Repository, dir);
    if (isfile(rcsdir) && (!isdir(rcsdir)) || (islink(rcsdir))) {
	warn(0, "%s is not a directory; %s not added", rcsdir, dir);
	goto out;
    }
    (void) sprintf(message, "Directory %s added to the repository\n", rcsdir);
    if (!isdir(rcsdir)) {
	int omask;
	FILE *fptty;
	char line[MAXLINELEN];

#ifdef sprite
	fptty = open_file(getenv("TTY"), "r");
#else
	fptty = open_file("/dev/tty", "r");
#endif
	printf("Add directory %s to the repository (y/n) [n] ? ", rcsdir);
	(void) fflush(stdout);
	if (fgets(line, sizeof(line), fptty) == NULL ||
	    (line[0] != 'y' && line[0] != 'Y')) {
	    warn(0, "directory %s not added", rcsdir);
	    (void) fclose(fptty);
	    goto out;
	}
	(void) fclose(fptty);
	omask = umask(2);
	if (mkdir(rcsdir, 0777) < 0) {
	    warn(1, "cannot mkdir %s", rcsdir);
	    (void) umask(omask);
	    goto out;
	}
	(void) umask(omask);
	(void) strcpy(Llist, " - New directory"); /* for title in message */
	Update_Logfile(rcsdir, message);
    }
    Create_Admin(rcsdir, DFLT_RECORD);
    printf("%s", message);
out:
    if (chdir(cwd) < 0)
	error(1, "cannot chdir to %s", cwd);
    return (0);
}

/*
 * The specified user file is really a symbolic link.  Add it to the
 * repository.
 * Returns 1 on failure, 0 on success.
 */
static
add_link(link)
    char *link;
{
    char rcslink[MAXPATHLEN];
    char message[MAXPATHLEN+100];
    char dest[MAXPATHLEN];
    int	 size;

    (void) sprintf(rcslink, "%s/%s", Repository, link);
    if (isfile(rcslink)) {
	(void) sprintf(message, 
	    "%s already exists; %s not added\n", rcslink, link);
	goto out;
    }
    size = readlink(link, dest, MAXPATHLEN);
    if (size == -1) {
	warn(1, "cannot read contents of symbolic link %s", link);
	return (0);
    }
    if (symlink(dest, rcslink)) {
	warn(1, "cannot make symbolic link %s", rcslink);
	return (0);
    }
    (void) sprintf(message, "Symbolic link %s added to the repository\n", 
	rcslink);
out:
    printf("%s", message);
    return (0);
}

static
add_usage()
{
    (void) fprintf(stderr,
		   "%s %s [-m 'message'] files...\n", progname, command);
    exit(1);
}
