/* 
 * deleteuser.c --
 *
 *	Delete a users account, remove all their files, remove
 *      them from /etc/passwd and take them off the sprite-users
 *      mailing list.
 *      
 * Copyright 1990 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/admin/deleteuser/RCS/deleteuser.c,v 1.7 91/12/16 12:10:27 kupfer Exp $";
#endif /* not lint */


#include "common.h"
#include <sprite.h>
#include <bstring.h>
#include <errno.h>
#include <fcntl.h>
#include <libc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

/* Forward references: */
static void removeFromMailingLists _ARGS_((CONST char *username));
static void deletePasswdEntry _ARGS_((CONST char *username));
static void removeHomeDirectory _ARGS_((CONST char *homedir));


/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	Process arguments, set uid and signals; then remove users.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
main(argc, argv)
    int argc;
    CONST char **argv;
{
    int i;
    CONST char *username;
    struct passwd *pwd;
    char homeDir[MAXPATHLEN];

    if(argc < 2 || *argv[1] == '-') {
	(void) fprintf(stderr, "Usage: %s username [username ...]\n", argv[0]);
	(void) fprintf(stderr, "Remove sprite accounts.\n");
	exit(EXIT_FAILURE);
    }
#ifndef TEST
    /* 
     * Verify that we're running as root, so as to avoid unpleasant 
     * surprises later.  Because the program is installed setuid, make 
     * sure that the invoking user is in the wheel group.
     */
    SecurityCheck();
#endif
    printf("This program will delete the accounts and erase\n");
    printf("all the files in the home directories.\n");
    if (!yes("Are you sure you want to do this?")) {
	printf("\nquitting\n");
	exit(EXIT_FAILURE);
    }
    (void) signal(SIGHUP, SIG_IGN);
    (void) signal(SIGINT, SIG_IGN);
    (void) signal(SIGQUIT, SIG_IGN);
    (void) signal(SIGTSTP, SIG_IGN);

    /* 
     * For each username, try to get the home directory from the 
     * password file.  If that fails, ask the user if we should 
     * continue (e.g., maybe the user was already removed from the 
     * password file, but not the aliases file).  If the user says to
     * continue, we guess at the home directory.  (XXX We should ask
     * the user.)
     */
    setpwfile(MASTER_PASSWD_FILE);
    if (setpwent() == 0) {
	fprintf(stderr, "Can't change password file to %s\n",
		MASTER_PASSWD_FILE);
	exit(EXIT_FAILURE);
    }
    for (i = 1; i < argc; ++i) {
	username = argv[i];
	if ((pwd = getpwnam(username)) == NULL) {
	    fprintf(stderr, "%s: no such user\n", username);
	    if (yes("Skip this user?")) {
		continue;
	    }
	}
	removeFromMailingLists(username);

	/* 
	 * If we don't know what the user's home directory is, guess 
	 * that it's the default.
	 */
	if (pwd != NULL) {
	    removeHomeDirectory(pwd->pw_dir);
	} else {
	    strcpy(homeDir, USER_DIR);
	    strcat(homeDir, "/");
	    strcat(homeDir, username);
	    removeHomeDirectory(homeDir);
	}
	deletePasswdEntry(username);
    }
    exit(EXIT_SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 *  deletePasswdEntry --
 *
 *      Removes a user's entry from the password file.
 *
 * Results:
 *	none.
 *
 * Side effects:
 *
 *      Exits if there is a system error.
 *
 *----------------------------------------------------------------------
 */
static void
deletePasswdEntry(username)
    CONST char *username;
{
    FILE *tmpfile;
    int fd;
    struct passwd *pwd;
    int found = 0;
    char from[MAXPATHLEN], to[MAXPATHLEN], *tend, *fend;
    char buf1[16], buf2[16];


    (void) printf("Removing %s from %s.\n", username, MASTER_PASSWD_FILE);
    if ((fd = open(PTMP_FILE, O_CREAT|O_EXCL|O_RDWR, 0600)) < 0) {
	(void) fprintf(stderr, "Cannot open %s: %s\n",
	    PTMP_FILE, strerror(errno));
	exit(EXIT_FAILURE);
    }
    if ((tmpfile = fdopen(fd, "w")) == NULL) {
	(void) fprintf(stderr, "Absurd fdopen failure - seek help\n");
	(void) unlink(PTMP_FILE);
	exit(EXIT_FAILURE);
    }
    while ((pwd = getpwent()) != NULL) {
	if (strcmp(pwd->pw_name, username) == 0) {
	    ++found;
	} else {
	    if (pwd->pw_change==0) {
		*buf1 = '\0';
	    } else {
		sprintf(buf1,"%l",pwd->pw_change);
	    }
	    if (pwd->pw_expire==0) {
		*buf2 = '\0';
	    } else {
		sprintf(buf2,"%l",pwd->pw_expire);
	    }
	    fprintf(tmpfile, "%s:%s:%d:%d:%s:%s:%s:%s:%s:%s\n",
			pwd->pw_name,
			pwd->pw_passwd,
			pwd->pw_uid,
			pwd->pw_gid,
			pwd->pw_class,
			buf1,
			buf2,
			pwd->pw_gecos,
			pwd->pw_dir,
			pwd->pw_shell);
	}
    }
    (void) endpwent();
    (void) fclose(tmpfile);

    if (found == 0) {
	(void) fprintf(stderr, "There is no entry for %s in %s.\n",
		       username, MASTER_PASSWD_FILE);
	(void) unlink(PTMP_FILE);
	return;
    }
    if (makedb(PTMP_FILE)) {
	(void) fprintf(stderr, "makedb failed!\n");
	exit(EXIT_FAILURE);
    }

    /*
     * possible race; have to rename four files, and someone could slip
     * in between them.  LOCK_EX and rename the ``passwd.dir'' file first
     * so that getpwent(3) can't slip in; the lock should never fail and
     * it's unclear what to do if it does.  Rename ``ptmp'' last so that
     * passwd/vipw/chpass can't slip in.
     */
    fend = strcpy(from, PTMP_FILE) + strlen(PTMP_FILE);
    tend = strcpy(to, PASSWD_FILE) + strlen(PASSWD_FILE);
    bcopy(".dir", fend, 5);
    bcopy(".dir", tend, 5);
    if ((fd = open(from, O_RDONLY, 0)) >= 0) {
	(void)flock(fd, LOCK_EX);
	/* here we go... */
	if (rename(from, to)) {
	    (void) fprintf(stderr, "Cannot rename %s: %s\n",
		from, strerror(errno));
	}
	bcopy(".pag", fend, 5);
	bcopy(".pag", tend, 5);
	if (rename(from, to)) {
	    (void) fprintf(stderr, "Cannot rename %s: %s\n",
		from, strerror(errno));
	}
    }
    bcopy(".orig", fend, 6);
    if (rename(PASSWD_FILE, PASSWD_BAK)) {
	(void) fprintf(stderr, "Cannot rename %s: %s\n",
	    from, strerror(errno));
    }
    if (rename(MASTER_PASSWD_FILE, MASTER_BAK)) {
	(void) fprintf(stderr, "Cannot rename %s: %s\n",
	    MASTER_PASSWD_FILE, strerror(errno));
    }
    if (rename(from, PASSWD_FILE)) {
	(void) fprintf(stderr, "Cannot rename %s: %s\n",
	    PASSWD_FILE, strerror(errno));
    }
    if (rename(PTMP_FILE, MASTER_PASSWD_FILE)) {
	(void) fprintf(stderr, "Cannot rename %s: %s\n",
	    PTMP_FILE, strerror(errno));
    }
    return;
}


/*
 *----------------------------------------------------------------------
 *
 *  removeFromMailingLists --
 * 
 *      Remove a user from all the mailing list.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *      Modifies /sprite/lib/sendmail/aliases.
 *
 *----------------------------------------------------------------------
 */
static void
removeFromMailingLists(username)
    CONST char *username;
{
    int status;			/* exit status from subprocesses */
    char message[4096];		/* log message for "ci" and user prompt */
    
    /* 
     * For the time being, rather than try to get string manipulation 
     * code right, just invoke an editor and let the user remove all 
     * instances of the name.
     */
    sprintf(message, "Remove %s from the aliases file?", username);
    if (!yes(message)) {
	return;
    }

    sprintf(message, "-mdeleteuser: remove %s.", username);
    
    status = rcsCheckOut(ALIASES);
    if (status < 0) {
	exit(EXIT_FAILURE);
    } else if (status != 0) {
	goto error;
    }

    /* 
     * Give the user ownership of the file, so that she can edit it.
     */
    if (chown(ALIASES, getuid(), -1)  != 0) {
	perror("Can't chown the aliases file");
	goto error;
    }

    if (Misc_InvokeEditor(ALIASES) != 0) {
	fprintf(stderr, "Couldn't invoke editor.\n");
	goto error;
    }

    /* 
     * Check the aliases file back in, even if we didn't actually make 
     * any changes.
     */
    status = rcsCheckIn(ALIASES, message);
    if (status == 0) {
	printf("Removed %s from aliases file.\n", username);
    } else {
	fprintf(stderr, "\nPlease check the aliases file.\n");
	fprintf(stderr, "(Make sure that `%s' is no longer in any lists\n",
	       username);
	fprintf(stderr, "and that the file was checked in.)\n\n");
    }
    return;

 error:
    fprintf(stderr,
	    "\nWarning: unable to remove `%s' from the aliases file.\n",
	    username);
    fprintf(stderr, "You'll have to edit the aliases file by hand.\n\n");
    return;
}


/*
 *----------------------------------------------------------------------
 *
 * removeHomeDirectory --
 *
 *	Remove a home directory.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Deletes all the files in the directory and then unlinks it.  
 *	If the given directory is really a symbolic link, removes the 
 *	link and the directory it points to.
 *
 *----------------------------------------------------------------------
 */
static void
removeHomeDirectory(givenDir)
    CONST char *givenDir;	/* path to the directory, usually a 
				 * link to the real home directory */
{
    char actualDir[MAXPATHLEN]; /* the real home directory */
    char command[MAXPATHLEN + 20]; /* passed to system() */
    struct stat statBuf;
    int n;

    if (lstat(givenDir, &statBuf) < 0) {
	fprintf(stderr, "Can't stat %s: %s\n", givenDir,
		strerror(errno));
	return;
    }
    if ((statBuf.st_mode & S_IFMT) == S_IFDIR) {
	strcpy(actualDir, givenDir);
    } else if ((statBuf.st_mode & S_IFMT) == S_IFLNK) {
	n = readlink(givenDir, actualDir, sizeof(actualDir));
	if (n < 0) {
	    fprintf(stderr, "Cannot read link %s: %s.\n",
		    givenDir, strerror(errno));
	    return;
	}
	actualDir[n] = '\0';
    } else {
	fprintf(stderr,
		"%s isn't a link or a directory; not removing.\n", 
		givenDir);
	return;
    }

    /* 
     * actualDir now contains the path for the directory we want to 
     * delete.
     */

    if ((statBuf.st_mode & S_IFMT) == S_IFLNK) {
	printf("removing symbolic link: %s\n", givenDir);
	if (unlink(givenDir)) {
	    (void) fprintf(stderr, "Cannot unlink %s: %s\n",
			   givenDir, strerror(errno));
	}
    }
    printf("removing home directory: %s\n", actualDir);
    (void) sprintf(command, "rm -rf %s", actualDir);
    if (system(command) != 0) {
	(void) fprintf(stderr, "Error deleting files from %s.\n", actualDir);
    }
    return;
}
