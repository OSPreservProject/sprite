#ifndef lint
static char rcsid[] = "$Id: patch.c,v 1.6.1.1 91/01/18 12:18:45 berliner Exp $";
#endif !lint

/*
 *    Copyright (c) 1989, Brian Berliner
 *
 *    You may distribute under the terms of the GNU General Public License
 *    as specified in the README file that comes with the CVS 1.0 kit.
 *
 * Patch
 *
 *	Create a Larry Wall format "patch" file between a previous release
 *	and the current head of a module, or between two releases.  Can
 *	specify the release as either a date or a revision number.
 */

#include <sys/param.h>
#include <time.h>
#include <ndbm.h>
#include <dirent.h>
#include <ctype.h>
#include "cvs.h"

extern char update_dir[];
extern DBM *open_module();
extern int force_tag_match;

static int patch_recursive = 1;
static int patch_short = 0;
static int toptwo_diffs = 0;
static char rev1[50], rev2[50], date1[50], date2[50];
static char tmpfile1[50], tmpfile2[50], tmpfile3[50];

void patch_cleanup();

patch(argc, argv)
    int argc;
    char *argv[];
{
    register int i;
    int c, err = 0;
    DBM *db;

    if (argc == -1)
	patch_usage();
    rev1[0] = rev2[0] = date1[0] = date2[0] = '\0';
    optind = 1;
    while ((c = getopt(argc, argv, "ftsQqlD:r:")) != -1) {
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
	case 'l':
	    patch_recursive = 0;
	    break;
	case 't':
	    toptwo_diffs = 1;
	    break;
	case 's':
	    patch_short = 1;
	    break;
	case 'D':
	    if (rev2[0] != '\0' || date2[0] != '\0')
		error(0, "no more than two revisions/dates can be specified");
	    if (rev1[0] != '\0' || date1[0] != '\0')
		Make_Date(optarg, date2);
	    else
		Make_Date(optarg, date1);
	    break;
	case 'r':
	    if (rev2[0] != '\0' || date2[0] != '\0')
		error(0, "no more than two revisions/dates can be specified");
	    if (rev1[0] != '\0' || date1[0] != '\0')
		(void) strcpy(rev2, optarg);
	    else
		(void) strcpy(rev1, optarg);
	    break;
	case '?':
	default:
	    patch_usage();
	    break;
	}
    }
    argc -= optind;
    argv += optind;
    if (argc < 1)
	patch_usage();
    if (toptwo_diffs && patch_short)
	error(0, "-t and -s options are mutually exclusive");
    if (toptwo_diffs && (date1[0] != '\0' || date2[0] != '\0' ||
	        	 rev1[0] != '\0' || rev2[0] != '\0')) {
	error(0, "must not specify revisions/dates with -t option!");
    }
    if (!toptwo_diffs &&
	(date1[0] == '\0' && date2[0] == '\0' &&
	 rev1[0] == '\0' && rev2[0] == '\0')) {
	error(0, "must specify at least one revision/date!");
    }
    if (date1[0] != '\0' && date2[0] != '\0') {
	if (datecmp(date1, date2) >= 0)
	    error(0, "second date must come after first date!");
    }
    (void) signal(SIGHUP, patch_cleanup);
    (void) signal(SIGINT, patch_cleanup);
    (void) signal(SIGQUIT, patch_cleanup);
    (void) signal(SIGTERM, patch_cleanup);
    db = open_module();
    for (i = 0; i < argc; i++)
	err += do_module(db, argv[i], PATCH, "Examining");
    close_module(db);
    patch_cleanup(0);
    exit(err);
}

/*
 * This is the recursive function that looks to see if an RCS file
 * has been modified since the revision specified in rev1 or date1,
 * using the revision specified in rev2 or date2 or the head if
 * neither are specified.
 *
 * If the "rcs" argument is NULL, descend the current
 * directory, examining all the files as appropriate; otherwise, just
 * examine the argument rcs file
 */
patched(rcs)
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
	    } else while ((dp = readdir(dirp)) != NULL) {
		if (strcmp(dp->d_name, ".") == 0 ||
		    strcmp(dp->d_name, "..") == 0 ||
		    strcmp(dp->d_name, CVSATTIC) == 0 ||
		    strcmp(dp->d_name, CVSLCK) == 0)
		    continue;
		if (isdir(dp->d_name) && patch_recursive) {
		    char cwd[MAXPATHLEN];

		    if (getwd(cwd) == NULL) {
			warn(0, "cannot get working directory: %s", cwd);
			err++;
			continue;
		    }
		    if (update_dir[0] == '\0') {
			(void) strcpy(update_dir, dp->d_name);
		    } else {
			(void) strcat(update_dir, "/");
			(void) strcat(update_dir, dp->d_name);
		    }
		    if (!quiet) {
			printf("%s %s: Examining %s\n",
			       progname, command, update_dir);
		    }
		    if (chdir(dp->d_name) < 0) {
			warn(1, "chdir failed; %s ignored", update_dir);
			err++;
			continue;
		    }
		    err += patched((char *)0);
		    if ((cp = rindex(update_dir, '/')) != NULL)
			*cp = '\0';
		    else
			update_dir[0] = '\0';
		    if (chdir(cwd) < 0)
			error(1, "cannot chdir back to %s", cwd);
		    continue;
		}
		if (re_exec(dp->d_name))
		    err += patch_file(dp->d_name);
	    }
	}
	if (dirp)
	    (void) closedir(dirp);
    } else {
	return (patch_file(rcs));
    }
    return (err);
}

/*
 * Called to examine a particular RCS file, as appropriate with the options
 * that were set above.
 */
static
patch_file(rcs)
    char *rcs;
{
    char vers_tag[50], vers_head[50];
    int fd1, fd2, fd3, ret = 0;

    Version_Number(rcs, rev2, date2, vers_head);
    if (vers_head[0] == '\0') {
	if (!really_quiet)
	    warn(0, "cannot find head revision for %s", rcs);
	return (1);
    }
    if (toptwo_diffs && get_rcsdate(rcs, vers_head, date1) != 0) {
	if (!really_quiet)
	    warn(0, "cannot find date in rcs file %s revision %s",
		 rcs, vers_head);
	return (1);
    }
    Version_Number(rcs, rev1, date1, vers_tag);
    if (strcmp(vers_head, vers_tag) == 0)
	return (0);			/* not changed between releases */
    if (patch_short) {
	printf("File ");
	if (update_dir[0] != '\0')
	    printf("%s/", update_dir);
	if (vers_tag[0] == '\0')
	    printf("%s is new; current revision %s\n", rcs, vers_head);
	else
	    printf("%s changed from revision %s to %s\n",
		   rcs, vers_tag, vers_head);
	return (0);
    }
    (void) strcpy(tmpfile1, "/tmp/cvspatch.XXXXXX");
    (void) strcpy(tmpfile2, "/tmp/cvspatch.XXXXXX");
    (void) strcpy(tmpfile3, "/tmp/cvspatch.XXXXXX");
    fd1 = mkstemp(tmpfile1);	(void) close(fd1);
    fd2 = mkstemp(tmpfile2);	(void) close(fd2);
    fd3 = mkstemp(tmpfile3);	(void) close(fd3);
    if (fd1 < 0 || fd2 < 0 || fd3 < 0) {
	warn(0, "cannot create temporary files");
	ret = 1;
	goto out;
    }
    if (vers_tag[0] != '\0') {
	(void) sprintf(prog, "%s/%s -p -q -r%s %s > %s", Rcsbin, RCS_CO,
		       vers_tag, rcs, tmpfile1);
	if (system(prog) != 0) {
	    if (!really_quiet)
		warn(0, "co of revision %s for %s failed", VN_Rcs, rcs);
	    ret = 1;
	    goto out;
	}
    } else if (toptwo_diffs) {
	ret = 1;
	goto out;
    }
    (void) sprintf(prog, "%s/%s -p -q -r%s %s > %s", Rcsbin, RCS_CO,
		   vers_head, rcs, tmpfile2);
    if (system(prog) != 0) {
	if (!really_quiet)
	    warn(0, "co of revision %s for %s failed", VN_Rcs, rcs);
	return (1);
    }
    (void) sprintf(prog, "%s -c %s %s > %s", DIFF, tmpfile1,
		   tmpfile2, tmpfile3);
    if (system(prog) != 0) {
	char file1[MAXPATHLEN], file2[MAXPATHLEN], strippath[MAXPATHLEN];
	char line1[MAXLINELEN], line2[MAXLINELEN];
	char *cp1, *cp2;
	FILE *fp;

	/*
	 * The two revisions are really different, so read the first
	 * two lines of the diff output file, and munge them to include
	 * more reasonable file names that "patch" will understand.
	 */
	fp = open_file(tmpfile3, "r");
	if (fgets(line1, sizeof(line1), fp) == NULL ||
	    fgets(line2, sizeof(line2), fp) == NULL) {
	    warn(1, "failed to read diff file header %s for %s",
		 tmpfile3, rcs);
	    ret = 1;
	    (void) fclose(fp);
	    goto out;
	}
	if (strncmp(line1,"*** ",4) != 0 || strncmp(line2,"--- ",4) != 0 ||
	    (cp1 = index(line1, '\t')) == NULL ||
	    (cp2 = index(line2, '\t')) == NULL) {
	    warn(0, "invalid diff header for %s", rcs);
	    ret = 1;
	    (void) fclose(fp);
	    goto out;
	}
	if (CVSroot != NULL)
	    (void) sprintf(strippath, "%s/", CVSroot);
	else
	    (void) strcpy(strippath, REPOS_STRIP);
	if (strncmp(rcs, strippath, strlen(strippath)) == 0)
	    rcs += strlen(strippath);
	*rindex(rcs, ',') = '\0';
	if (vers_tag[0] != '\0') {
	    (void) sprintf(file1, "%s%s%s:%s", update_dir,
			   update_dir[0] ? "/" : "", rcs, vers_tag);
	} else {
	    (void) strcpy(file1, DEVNULL);
	}
	(void) sprintf(file2, "%s%s%s:%s", update_dir,
		       update_dir[0] ? "/" : "", rcs, vers_head);
	printf("diff -c %s %s\n", file1, file2);
	printf("*** %s%s--- ", file1, cp1);
	if (update_dir[0] != '\0')
	    printf("%s/", update_dir);
	printf("%s%s", rcs, cp2);
	while (fgets(line1, sizeof(line1), fp) != NULL)
	    printf("%s", line1);
	(void) fclose(fp);
    }
out:
    (void) unlink(tmpfile1);
    (void) unlink(tmpfile2);
    (void) unlink(tmpfile3);
    return (ret);
}

void
patch_cleanup(sig)
    int sig;
{
    if (tmpfile1[0] != '\0')
	(void) unlink(tmpfile1);
    if (tmpfile2[0] != '\0')
	(void) unlink(tmpfile2);
    if (tmpfile3[0] != '\0')
	(void) unlink(tmpfile3);
    if (sig != 0)
	exit(1);
}

/*
 * Lookup the specified revision in the ,v file and return, in the date
 * argument, the date specified for the revision *minus one second*,
 * so that the logically previous revision will be foun by Version_Number()
 * later.
 *
 * Returns zero on success, non-zero on failure.
 */
static
get_rcsdate(rcs, rev, date)
    char *rcs;
    char *rev;
    char *date;
{
    char line[MAXLINELEN];
    struct tm tm, *ftm;
    char *cp, *semi;
    time_t revdate;
    FILE *fp;
    int ret = 1;

    if ((fp = fopen(rcs, "r")) == NULL)
	return (1);
    while (fgets(line, sizeof(line), fp) != NULL) {
	if (!isdigit(line[0]))
	    continue;
	if ((cp = rindex(line, '\n')) != NULL)
	    *cp = '\0';
	if (strcmp(line, rev) == 0) {
	    if (fgets(line, sizeof(line), fp) == NULL)
		break;
	    for (cp = line; *cp && !isspace(*cp); cp++)
		;
	    while (*cp && isspace(*cp))
		cp++;
	    if (*cp && (semi = index(cp, ';')) != NULL) {
		ret = 0;
		*semi = '\0';
		semi[-1]--;	/* This works even if semi[-1] was '0'.  */
	    }
	    break;
	}
    }
    (void) fclose(fp);
    return (ret);
}

static
patch_usage()
{
    (void) fprintf(stderr,
	"%s %s [-Qqlf] [-s|-t] [-r tag|-D date [-r tag|-D date]] modules...\n",
		   progname, command);
    exit(1);
}
