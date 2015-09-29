#ifndef lint
static char rcsid[] = "$Id: set_lock.c,v 1.8 89/11/19 23:20:26 berliner Exp $";
#endif !lint

/*
 *    Copyright (c) 1989, Brian Berliner
 *
 *    You may distribute under the terms of the GNU General Public License
 *    as specified in the README file that comes with the CVS 1.0 kit.
 *
 * Set Lock
 *
 * Lock file support for CVS.  Currently, only "update" and "commit"
 * (and by extension, "checkout") adhere to this locking protocol.
 * Maybe some day, others will too.
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <dirent.h>
#include "cvs.h"

static char lckdir[MAXPATHLEN], lckrfl[MAXPATHLEN], lckwfl[MAXPATHLEN];

/*
 * Remove the lock files (without complaining if they are not there),
 * and do a quick check to see if the Entries file is missing, but
 * the Entries.Backup file is there.
 */
void
Lock_Cleanup(sig)
    int sig;
{
    struct stat sb;

    if (lckrfl[0] != '\0')
	(void) unlink(lckrfl);
    if (lckwfl[0] != '\0')
	(void) unlink(lckwfl);
    if (lckdir[0] != '\0') {
	/*
	 * Only remove the lock directory if it is ours, note that this does
	 * lead to the limitation that one user ID should not be committing
	 * files into the same Repository directory at the same time.
	 * Oh well.
	 */
	if (stat(lckdir, &sb) != -1 && sb.st_uid == geteuid())
	    (void) rmdir(lckdir);
    }
    if (!isfile(CVSADM_ENT) && isfile(CVSADM_ENTBAK)) {
	warn(0, "warning: restoring %s to %s", CVSADM_ENTBAK, CVSADM_ENT);
	rename_file(CVSADM_ENTBAK, CVSADM_ENT);
    }
    if (sig != 0)
	exit(1);
}

/*
 * Create a lock file for readers (like "update" is)
 */
Reader_Lock()
{
    extern char *ctime();
    extern time_t time();
    char *cp;
    time_t now;
    FILE *fp;

    (void) sprintf(lckdir, "%s/%s", Repository, CVSLCK);
    (void) sprintf(lckrfl, "%s/%s.%d", Repository, CVSTFL, getpid());
    (void) signal(SIGHUP, Lock_Cleanup);
    (void) signal(SIGINT, Lock_Cleanup);
    (void) signal(SIGQUIT, Lock_Cleanup);
    (void) signal(SIGTERM, Lock_Cleanup);
    if ((fp = fopen(lckrfl, "w+")) != NULL) {
	(void) fclose(fp);
	(void) unlink(lckrfl);
	set_lock(lckdir);
	(void) sprintf(lckrfl, "%s/%s.%d", Repository, CVSRFL, getpid());
	if ((fp = fopen(lckrfl, "w+")) == NULL)
	    warn(1, "cannot create read lock file %s", lckrfl);
	else
	    (void) fclose(fp);
	if (rmdir(lckdir) < 0)
	    warn(1, "failed to remove lock dir %s", lckdir);
    } else {
	while (isfile(lckdir)) {
	    struct stat sb;

	    (void) time(&now);
	    /*
	     * If the create time of the directory is more than CVSLCKAGE
	     * seconds ago, try to clean-up the lock directory, and if
	     * successful, we are (somewhat) free and clear.
	     */
	    if (stat(lckdir, &sb) != -1 && now >= (sb.st_ctime + CVSLCKAGE)) {
		if (rmdir(lckdir) != -1)
		    break;
	    }
	    cp = ctime(&now);
	    warn(0, "%s: waiting for the lock directory to go away", cp);
	    sleep(CVSLCKSLEEP);
	}
    }
}

/*
 * Create a lock file for writers (like "commit" is)
 */
Writer_Lock()
{
    FILE *fp;

    (void) sprintf(lckdir, "%s/%s", Repository, CVSLCK);
    (void) sprintf(lckrfl, "%s/%s.%d", Repository, CVSTFL, getpid());
    (void) sprintf(lckwfl, "%s/%s.%d", Repository, CVSWFL, getpid());
    (void) signal(SIGHUP, Lock_Cleanup);
    (void) signal(SIGINT, Lock_Cleanup);
    (void) signal(SIGQUIT, Lock_Cleanup);
    (void) signal(SIGTERM, Lock_Cleanup);
    if ((fp = fopen(lckrfl, "w+")) == NULL)
	error(1, "you have no write permission in %s", Repository);
    (void) fclose(fp);
    (void) unlink(lckrfl);
    (void) sprintf(lckrfl, "%s/%s.%d", Repository, CVSRFL, getpid());
    set_lock(lckdir);
    if ((fp = fopen(lckwfl, "w+")) == NULL)
	warn(1, "cannot create write lock file %s", lckwfl);
    else
	(void) fclose(fp);
    while (readers_exist()) {
	extern char *ctime();
	extern time_t time();
	char *cp;
	time_t now;

	(void) time(&now);
	cp = ctime(&now);
	cp[24] = ' ';
	warn(0, "%s: waiting for readers to go away", cp);
	sleep(CVSLCKSLEEP);
    }
}

/*
 * readers_exist() returns 0 if there are no reader lock files
 * remaining in the repository; else 1 is returned, to indicate that the
 * caller should sleep a while and try again.
 */
static
readers_exist()
{
    char line[MAXLINELEN];
    DIR *dirp;
    struct dirent *dp;
    char *cp;
    int ret = 0;

again:
    if ((dirp = opendir(Repository)) == NULL)
	error(0, "cannot open directory %s", Repository);
    (void) sprintf(line, "^%s.*", CVSRFL);
    if ((cp = re_comp(line)) != NULL)
	error(0, "%s", cp);
    while ((dp = readdir(dirp)) != NULL) {
	if (re_exec(dp->d_name)) {
	    struct stat sb;
	    long now;

	    (void) time(&now);
	    /*
	     * If the create time of the file is more than CVSLCKAGE
	     * seconds ago, try to clean-up the lock file, and if
	     * successful, re-open the directory and try again.
	     */
	    (void) sprintf(line, "%s/%s", Repository, dp->d_name);
	    if (stat(line, &sb) != -1 && now >= (sb.st_ctime + CVSLCKAGE)) {
		if (unlink(line) != -1) {
		    (void) closedir(dirp);
		    goto again;
		}
	    }
	    ret = 1;
	    break;
	}
    }
    (void) closedir(dirp);
    return (ret);
}

/*
 * Persistently tries to make the directory "lckdir",, which serves as a lock.
 * If the create time on the directory is greater than CVSLCKAGE seconds
 * old, just try to remove the directory.
 */
static
set_lock(lckdir)
    char *lckdir;
{
    extern char *ctime();
    extern time_t time();
    struct stat sb;
    char *cp;
    time_t now;

    /*
     * Note that it is up to the callers of Set_Lock() to
     * arrange for signal handlers that do the appropriate things,
     * like remove the lock directory before they exit.
     */
    for (;;) {
	if (mkdir(lckdir, 0777) < 0) {
	    (void) time(&now);
	    /*
	     * If the create time of the directory is more than CVSLCKAGE
	     * seconds ago, try to clean-up the lock directory, and if
	     * successful, just quietly retry to make it.
	     */
	    if (stat(lckdir, &sb) != -1 && now >= (sb.st_ctime + CVSLCKAGE)) {
		if (rmdir(lckdir) != -1)
		    continue;
	    }
	    cp = ctime(&now);
	    cp[24] = ' ';
	    warn(0, "%s: waiting for the lock to go away", cp);
	    sleep(CVSLCKSLEEP);
	} else {
	    break;
	}
    }
}

