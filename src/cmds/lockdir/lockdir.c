/***********************************************************************
 *
 *	Copyright (c) Berkeley Softworks 1989 -- All Rights Reserved
 *
 * PROJECT:	  PCGEOS
 * MODULE:	  Tools/lockdir -- atomic directory lock acquisition
 * FILE:	  lockdir.c
 *
 * AUTHOR:  	  Adam de Boor: Sep 17, 1989
 *
 * REVISION HISTORY:
 *	Date	  Name	    Description
 *	----	  ----	    -----------
 *	9/17/89	  ardeb	    Initial version
 *
 * DESCRIPTION:
 *	Simple program to acquire a lock on a directory. Waits
 *	for up to 10 minutes before giving up. Exits 0 if the lock acquired
 *	and non-zero otherwise.
 *
 *	If no argument is given, this assumes it's in the proper directory.
 *
 *	To obtain the lock, we create a temporary file that we attempt
 *	to link to the lock file, since NFS does have an atomic link
 *	operation.
 *
 *	Usage: lockdir [<lockfile name>]
 *
 ***********************************************************************/
#ifndef lint
static char *rcsid =
"$Id: lockdir.c,v 1.4 89/10/10 01:16:02 adam Exp $";
#endif lint

#include    <stdio.h>
#include    <stdlib.h>
#include    <sys/file.h>
#include    <sys/signal.h>
#include    <pwd.h>
#include    <setjmp.h>

#define MAX_TRIES   60	    /* Maximum number of times to try and create
			     * the lock file */
#define SLEEP_FOR   5	    /* Number of seconds to sleep between tries */

jmp_buf	    interrupt;

irq() { longjmp(interrupt, 1); }

extern char *getlogin();

main(argc, argv)
    int	    argc;
    char    **argv;
{
    int	    	    i;	    	/* Attempt counter */
    char    	    *lockfile;	/* File we want to get */
    char    	    *tempfile;	/* Temp file from which we link */
    char    	    *cp;    	/* One past last char wanted in lockfile when
				 * creating the template */
    int	    	    fd;	    	/* Stream open to temporary file */
    char    	    host[64];	/* Current host's name */
    char    	    *user;  	/* User's login name */
    char    	    buf[128];	/* String to write to lock file */

    /*
     * We only allow interrupts while we're sleeping, otherwise we may leave
     * a lockfile around we don't want to.
     */
    signal(SIGINT, SIG_IGN);
    signal(SIGTERM, SIG_IGN);

    /*
     * Figure the name of the lock file.
     */
    if (argc == 1) {
	lockfile = "LOCK";
    } else {
	lockfile=argv[1];
    }

    /*
     * From the lock file, create a template for mkstemp that is any
     * leading path components followed by six X's, to contain the process
     * ID and a letter.
     */
    cp = (char *)rindex(lockfile, '/');
    if (cp++ == NULL) {
	cp = lockfile;
    }
    tempfile = (char *)malloc(cp-lockfile + 7);
    sprintf(tempfile, "%.*sXXXXXX", cp-lockfile, lockfile);

    fd = mkstemp(tempfile);

    if (fd < 0) {
	printf("Couldn't create temp file \"%s\"\n", tempfile);
	exit(1);
    }

    /*
     * Place user@host into the file, then close it.
     */
    user = getlogin();
    if (user == NULL) {
	struct passwd *pwd = getpwuid(getuid());

	if (pwd != NULL) {
	    user = pwd->pw_name;
	} else {
	    user = "?";
	}
    }

    gethostname(host, sizeof(host));
    sprintf(buf, "%s@%s", user, host);
    write(fd, buf, strlen(buf)+1);
    (void)close(fd);
    
    /*
     * Now try and link the temp file to the lock file. If it succeeds,
     * we've got the lock. Else we need to sleep for a bit.
     */
    if (setjmp(interrupt) == 0) {
	for (i = MAX_TRIES; i > 0; i--) {
	    if (link(tempfile, lockfile) < 0) {
		signal(SIGINT, irq);
		signal(SIGTERM, irq);
		
		/*
		 * See who has the thing locked and tell the user about it.
		 * Note that if we can't open the file, we immediately try for
		 * the lock again.
		 */
		fd = open(lockfile, O_RDONLY, 0);
		if (fd >= 0) {
		    (void)read(fd, buf, sizeof(buf));
		    printf("locked by %s. Sleeping for %d seconds...",
			   buf, SLEEP_FOR);
		    fflush(stdout);
		    sleep(SLEEP_FOR);
		}
		
		signal(SIGINT, SIG_IGN);
		signal(SIGTERM, SIG_IGN);
	    } else {
		/*
		 * Remove the temporary file, since the lockfile now points to
		 * the file with the user and hostname.
		 */
		unlink(tempfile);
		exit(0);
	    }
	}
    }
    unlink(tempfile);
    exit(1);
}
